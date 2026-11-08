import sys
from PIL import Image, ImageDraw, ImageFont
src, dst = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGB'); d = ImageDraw.Draw(im)
bg = im.getpixel((900, 10))
print('bg', bg)
font = ImageFont.truetype(r'C:\Windows\Fonts\seguisb.ttf', 35)
names = ['FuchsKumpel', 'Lena_Sniper', 'MaxPower99', 'TomTheTank', 'Mia2015']
rows = [135, 245, 356, 466, 577]
for y, n in zip(rows, names):
    d.rectangle([205, y-26, 640, y+22], fill=bg)
    d.text((213, y-24), n, font=font, fill=(232, 232, 238))
im.save(dst)

