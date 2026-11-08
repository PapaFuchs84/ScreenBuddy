import sys, glob, os
from PIL import Image, ImageDraw
fr, out, t0, t1, hold = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
files = sorted(glob.glob(os.path.join(fr, '*.png')))
items = []
prev = None
for f in files:
    t = int(os.path.basename(f)[:-4].split('_')[1])
    if t < t0 or t > t1: continue
    im = Image.open(f).convert('RGB')
    if prev is not None and im.tobytes() == prev: continue
    prev = im.tobytes()
    # Anonymisierung: Netzwerkname im Hintergrund der Systemseite verdecken
    d = ImageDraw.Draw(im)
    px = im.getpixel((420, 275))
    d.rectangle([405, 250, 470, 266], fill=px)
    items.append((t, im))
durs = [max(40, items[i+1][0]-items[i][0]) for i in range(len(items)-1)] + [hold]
pal = [im.quantize(colors=96, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE) for _, im in items]
pal[0].save(out, save_all=True, append_images=pal[1:], duration=durs, loop=0, optimize=True, disposal=1)
print(len(items), 'frames', sum(durs)/1000, 's', os.path.getsize(out)//1024, 'KB')
