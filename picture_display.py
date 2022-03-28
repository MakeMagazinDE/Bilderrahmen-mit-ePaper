import os
from convert_bmp import convert_bmp


def dither_picture(pic_no, input_filename):
    params = "-channel luminance -auto-level -modulate 120,200 -gravity center -resize 600x448^ "
    params += "-extent 600x448 -background white -dither FloydSteinberg -define dither:diffusion-amount=75% "
    params += "-remap eink-7color.png -depth 4"
    imagemagic_call = "convert" if os.name == 'posix' else "magick"
    system_call = "{call} {input_filename} {params} -type Palette BMP3:{filename}".format(
        call=imagemagic_call,
        input_filename=input_filename,
        params=params,
        filename=os.path.join("dithered", str(pic_no) + ".bmp"))
    os.system(system_call)
    convert_bmp(os.path.join(os.path.curdir, "dithered", str(pic_no) + ".bmp"),
                os.path.join(os.path.curdir, "converted", str(pic_no) + ".bmp"))


files = [f for f in os.listdir('original') if f.lower().endswith('jpg') and not f.startswith('.')]
if not os.path.isdir('dithered'):
    os.mkdir('dithered')
if not os.path.isdir('converted'):
    os.mkdir('converted')

for n, filename in enumerate(files):
    dither_picture(n, os.path.join('original', filename))
