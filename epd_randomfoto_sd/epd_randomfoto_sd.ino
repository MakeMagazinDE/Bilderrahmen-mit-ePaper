// We are building a photo frame that changes the picture every hour during the day,
// has storage space for tens of thousands of vacation pictures, and has a battery life
// of several months.
// In the following the Arduino sketch `epd_randomfoto_sd.ino` is explained.
// The sketch uses various examples of the GxEPD2 library.
// Author: Florian Sommer
// Microcontroller: FeatherS2 by Unexpected Maker
// Based on: https://github.com/ZinggJM/GxEPD2

// We first import the required libraries.
#include <SPI.h>
#include <SD.h>
#include <GxEPD2_7C.h>

// Then we set various #defines for the libraries to configure the display.
#define GxEPD2_DISPLAY_CLASS GxEPD2_7C
#define GxEPD2_DRIVER_CLASS GxEPD2_565c // Waveshare 5.65" 7-color (3C graphics)
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60*60      /* Time ESP32 will go to sleep (in seconds) */
// For the FeatherS2 we set specific #define`s: ledPin and LDO2 pin (`3V3O`).
#define ledPin 13
#define LDO2Pin 21

File myFile;
// With `RTC_DATA_ATTR` we set a persistent memory for a random number in the real time clock memory.
// This is not deleted in the battery-saving deep sleep mode of the microcontroller and can be read
// out again when waking up. With this, a different seed (= initial value) is always set for the
// Random Number Generator when waking up, so that the images are always randomly selected.
RTC_DATA_ATTR long randomState = 0;

GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ 1, /*DC=*/ 3, /*RST=*/ 5, /*BUSY=*/ -1));

// The setup() method is where all the code happens. The Arduino method loop() is never reached.
void setup()
{
  // Here we read the ambient light sensor of the FeatherS2. If it is too dark, the function mysleep()
  // is called, which sends the controller to sleep for one hour. Thereby it is extremely economical.
  // Thus, no unnecessary power is consumed at night, when it is too dark to enjoy new images anyway.
  if (analogRead(4) < 100) { // Read ambient light sensor. If too dark: Sleep
    mysleep();
  }
  // After that we configure LedPin and LDO2 Pin and switch them on. The display electronics are
  // now supplied with voltage.
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, true);
  pinMode(LDO2Pin, OUTPUT);
  digitalWrite(LDO2Pin, true);

  if (!SD.begin(33)) {
    while (1);
  }
  // Here we read the previously created file anzahl.txt and store the value in the
  // integer variable n.
  myFile = SD.open("/anzahl.txt");
  char n_string[6] = "00001";
  if (myFile) {
    n_string[0] = myFile.read();
    n_string[1] = myFile.read();
    n_string[2] = myFile.read();
    n_string[3] = myFile.read();
    n_string[4] = myFile.read();
  }
  int n = 0;
  n = atoi(n_string);
  myFile.close();

  // After that we initialize the Random Number Generator with the variable
  // from the Real Time Clock Memory
  randomSeed(randomState);
  // A random number between 1 and the number of frames is generated
  long randNumber = random(1, n+1);
  // A new random number is stored again in the Real Time Clock Memory
  randomState = random();
  // This ensures that other random numbers will be generated the next time it wakes up.
  // This can be omitted if using the Bluetooth or Wifi functions of the microcontroller,
  // because these can be used as entropy source for the random number generator.
  // For power saving reasons, however, these remain deactivated in our case.

  // Then we use the previously sampled random number randNumber to select the image file
  // on the microSD card. Its size is known (4bit per pixel + header)
  myFile = SD.open("/" + String(randNumber) + ".bmp");
  size_t       fileSize = 134518;
  // With this image size, memory is reserved.
  unsigned char * downloaded_file = (unsigned char *) malloc(fileSize);
  // Finally the data is copied from the card to memory.
  if (myFile) {
    myFile.read(downloaded_file, fileSize);
  }
  else
  {
    mysleep();
  }

  // Now we initialize the display. This automatically creates a SPI connection,
  // but it uses the wrong standard SPI pins. Therefore SPI is terminated again and
  // restarted with the correct pins for the FeatherS2 board. After that we wait for one second.
  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(36, 37, 35, 34); // map and init SPI pins SCK(36), MISO(37), MOSI(35), SS(34)
  delay(1000);
  // *** end of special handling for FeatherS2 board *** //

  // In the following lines we read the header of the image file.
  uint32_t index = 0;
  uint16_t bmpsignature = read16_buf(downloaded_file, &index);
  uint32_t fileSize_bmp = read32_buf(downloaded_file, &index);
  uint32_t creatorBytes = read32_buf(downloaded_file, &index);
  uint32_t imageOffset = read32_buf(downloaded_file, &index); // Start of image data
  uint32_t headerSize = read32_buf(downloaded_file, &index);
  uint32_t width  = read32_buf(downloaded_file, &index);
  uint32_t height = read32_buf(downloaded_file, &index);
  uint16_t planes = read16_buf(downloaded_file, &index);
  uint16_t depth = read16_buf(downloaded_file, &index); // bits per pixel
  uint32_t format = read32_buf(downloaded_file, &index);

  // We replace existing displays on the screen with white.
  display.clearScreen();
  delay(1000);
  // After one second, we transmit the image to the display.
  display.drawNative(downloaded_file+imageOffset, NULL, 0, 0, (int16_t) width, (int16_t) height, false, true, false);

  // Then we clear the memory buffer, turn off the display and
  // call the mysleep() function.
  delay(1);
  free(downloaded_file);
  display.powerOff();
  mysleep();
}

/* The main loop -------------------------------------------------------------*/
void loop()
{
  //
}

void mysleep(){
  // The function mysleep() first deactivates the perephery power supply LDO2,
  // then the LED. Then a `wakeup time` is set to one hour and the microcontroller
  // is put into deep_sleep, which turns off all electronics except the Real Time Clock
  // and its memory. After one hour the chip is reactivated and
  // executes the script again, where RTC_DATA_ATTR long randomState now already
  // has the value we stored in point 9. So pretty sure another image will be selected
  // and displayed.
  digitalWrite(LDO2Pin, false);
  digitalWrite(ledPin, false);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

uint32_t read32_buf(unsigned char *buf, uint32_t *index)
{
	uint32_t data;
	memcpy (&data, buf + *index, 4);
	*index += 4;
	return data;
}
uint16_t read16_buf(unsigned char *buf, uint32_t *index)
{
	uint16_t data;
	memcpy (&data, buf + *index, 2);
	*index += 2;
	return data;
}
