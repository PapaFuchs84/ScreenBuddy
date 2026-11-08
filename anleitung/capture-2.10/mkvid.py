import glob, os, sys, subprocess
from PIL import Image, ImageDraw, ImageFont
v = sys.argv[1]; ff = sys.argv[2]; out_gif = sys.argv[3]; out_mp4 = sys.argv[4]
files = sorted(glob.glob(os.path.join(v, 'f', '*.png')))
assert len(files) == 21, len(files)
def anon_bf6(im):
    d = ImageDraw.Draw(im); bg = im.getpixel((900, 10))
    font = ImageFont.truetype(r'C:\Windows\Fonts\seguisb.ttf', 35)
    for y, n in zip([135, 245, 356, 466, 577], ['FuchsKumpel', 'Lena_Sniper', 'MaxPower99', 'TomTheTank', 'Mia2015']):
        d.rectangle([205, y-26, 640, y+22], fill=bg); d.text((213, y-24), n, font=font, fill=(232, 232, 238))
    return im
frames = []
for i, f in enumerate(files):
    im = Image.open(f).convert('RGB')
    if i in (17, 18): im = anon_bf6(im)
    dur = 300 if i <= 15 else 1500
    frames.append((im, dur))
os.makedirs(os.path.join(v, 'p'), exist_ok=True)
lst = []
for i, (im, dur) in enumerate(frames):
    p = os.path.join(v, 'p', '%03d.png' % i); im.save(p); lst.append("file '%s'\nduration %.2f" % (p.replace('\\', '/'), dur/1000))
lst.append("file '%s'" % os.path.join(v, 'p', '020.png').replace('\\', '/'))
open(os.path.join(v, 'list.txt'), 'w').write('\n'.join(lst))
small = [im.resize((760, 360), Image.Resampling.LANCZOS).quantize(colors=64, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE) for im, _ in frames]
small[0].save(out_gif, save_all=True, append_images=small[1:], duration=[d for _, d in frames], loop=0, optimize=True, disposal=1)
subprocess.run([ff, '-y', '-hide_banner', '-loglevel', 'error', '-f', 'concat', '-safe', '0', '-i', os.path.join(v, 'list.txt'), '-vf', 'scale=1520:720,format=yuv420p', '-r', '10', '-c:v', 'libx264', '-crf', '28', '-movflags', '+faststart', out_mp4], check=True)
print(os.path.getsize(out_gif)//1024, 'KB gif', os.path.getsize(out_mp4)//1024, 'KB mp4')
