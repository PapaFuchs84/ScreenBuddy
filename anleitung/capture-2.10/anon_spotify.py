import sys
from PIL import Image, ImageDraw, ImageFont
src, dst = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGB'); d = ImageDraw.Draw(im)
bg = im.getpixel((900, 10))
title_c = im.getpixel((690, 152)); art_c = (240, 135, 20); alb_c = (200, 198, 206)
# Cover ersetzen: Verlauf Orange -> Navy mit Notensymbol
x0, y0, x1, y1 = 85, 108, 621, 645
for y in range(y0, y1):
    t = (y - y0) / (y1 - y0)
    c = (int(247*(1-t) + 20*t), int(127*(1-t) + 33*t), int(0*(1-t) + 61*t))
    d.line([(x0, y), (x1, y)], fill=c)
f_big = ImageFont.truetype(r'C:\Windows\Fonts\seguisym.ttf', 260)
d.text(((x0+x1)//2, (y0+y1)//2 - 20), '\u266A', font=f_big, fill=(255, 244, 230), anchor='mm')
f_s = ImageFont.truetype(r'C:\Windows\Fonts\segoeuisl.ttf', 34)
d.text(((x0+x1)//2, y1 - 50), 'Beispiel-Cover', font=f_s, fill=(255, 244, 230), anchor='mm')
# Texte ersetzen
d.rectangle([650, 120, 1400, 285], fill=bg)
d.text((659, 126), 'Beispiel-Titel', font=ImageFont.truetype(r'C:\Windows\Fonts\segoeuib.ttf', 45), fill=(196, 194, 206))
d.text((659, 190), 'Beispiel-Band', font=ImageFont.truetype(r'C:\Windows\Fonts\segoeui.ttf', 37), fill=art_c)
d.text((659, 240), 'Beispiel-Album', font=ImageFont.truetype(r'C:\Windows\Fonts\segoeui.ttf', 28), fill=alb_c)
im.save(dst)
