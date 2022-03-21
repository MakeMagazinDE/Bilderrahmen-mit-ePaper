import os
from convert_bmp import convert_bmp

def dither_picture(n, file):
    systemcall = f"convert '{file}' -channel luminance -auto-level -modulate 120,200 -gravity center -resize 600x448^ -extent 600x448 -background white -dither FloydSteinberg -define dither:diffusion-amount=75% -remap eink-7color.png -depth 4 -type Palette BMP3:dithered/{n}.bmp"
    os.system(systemcall)
    convert_bmp(f"./dithered/{n}.bmp", f"./converted/{n}.bmp")

path = './'
files = [f for f in os.listdir(path+'original/') if f.lower().endswith('jpg')]
if not os.path.isdir(path+'dithered/'):
    os.mkdir(path+'dithered/')
if not os.path.isdir(path+'converted/'):
    os.mkdir(path+'converted/')

for n,file in enumerate(files):
    dither_picture(n, 'original/'+file)
