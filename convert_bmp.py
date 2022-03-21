import numpy as np

def splitByte(b):
    lowerhalf = b & 15
    upperhalf = (b >> 4) & 15
    return [upperhalf, lowerhalf]

def convert_bmp(filename, outfile, headerlength=118):
    file = open(filename, 'rb')
    data = file.read()
    file.close()
    databytes = [b for b in data][headerlength:]
    origpicture = np.array([[hex(sb) for sb in splitByte(b)] for b in databytes])
    newpicture = origpicture.copy()
    newpicture[origpicture == '0x6'] = '0x1'
    newpicture[origpicture == '0x1'] = '0x4'
    newpicture[origpicture == '0x5'] = '0x3'
    newpicture[origpicture == '0x3'] = '0x6'
    newpicture[origpicture == '0x4'] = '0x5'
    newpicture = [list(e) for e in list(newpicture)]
    newpicture = [int(f"{entry[0][-1]}{entry[1][-1]}", 16) for entry in newpicture]
    header = list(data[:headerlength])
    newdata = header + newpicture
    file = open(outfile, 'wb')
    file.write(bytearray(newdata))
    file.close()
