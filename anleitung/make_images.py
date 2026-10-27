"""Build embedded DE/EN guides from anonymous native ScreenBuddy captures.

Usage: python anleitung/make_images.py anleitung/bilder [en]
The English guide currently uses the same German-labelled native captures and
says so explicitly. Never synthesize English labels or use private screenshots.
"""
import base64
import io
import re
import sys
from pathlib import Path
from PIL import Image

HERE = Path(__file__).resolve().parent
RES = HERE.parent / 'cpp' / 'res'
SRC = Path(sys.argv[1]).resolve()
EN = len(sys.argv) > 2 and sys.argv[2] == 'en'

def data(path, lossless=False):
    with Image.open(path) as im:
        out = io.BytesIO()
        if lossless:
            im.save(out, 'PNG', optimize=True)
            kind = 'png'
        else:
            im.convert('RGB').save(out, 'JPEG', quality=84, optimize=True, progressive=True)
            kind = 'jpeg'
    return 'data:image/' + kind + ';base64,' + base64.b64encode(out.getvalue()).decode('ascii')

native = {
    'start': 'start', 'uebersicht': 'pages', 'discord': 'discord',
    'bf6': 'bf6', 'blogs': 'blogs', 'display': 'display',
    'bar': 'start', 'app': 'app', 'freunde': 'friends-native', 'ha_setup': 'ha',
    'ha_pick': 'ha',
}
imgs = {key: data(SRC / (filename + '.png')) for key, filename in native.items()}
page_root = SRC / ('en' if EN else '')
for key in ('page_system', 'page_blogs', 'ha_display'):
    imgs[key] = data(page_root / (key + '.png'), lossless=True)
with Image.open(RES / 'app.ico') as icon:
    out = io.BytesIO()
    icon.convert('RGBA').resize((64, 64), Image.Resampling.LANCZOS).save(out, 'PNG')
    imgs['icon'] = 'data:image/png;base64,' + base64.b64encode(out.getvalue()).decode('ascii')

source = HERE / ('anleitung.template.en.html' if EN else 'anleitung.template.html')
html = source.read_text(encoding='utf-8')
html = re.sub(r'\{\{img:(\w+)\}\}', lambda m: imgs[m.group(1)], html)
target = RES / ('GUIDE.html' if EN else 'ANLEITUNG.html')
target.write_text(html, encoding='utf-8')
print(target.name, len(html) // 1024, 'KB')
