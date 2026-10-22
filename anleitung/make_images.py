"""Bereitet die Screenshots für ANLEITUNG.html vor (zuschneiden, verkleinern) und baut die Datei zusammen.

Aufruf: python anleitung\\make_images.py <ordner-mit-screenshots> [en]
Erwartet start, prev0 (Übersicht mit Systemseite in der Vorschau), prev4 (Vorschau mit Blogseite), discord, bf6,
blogs, display, speichern, freunde als PNG (aufgenommen mit screenshots.mjs). Ergebnis: cpp\\res\\ANLEITUNG.html,
mit „en“ die englische GUIDE.html aus anleitung.template.en.html (Screenshots der englischen Einstellungsseite).
"""
import base64
import io
import re
import sys
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
RES = HERE.parent / 'cpp' / 'res'  # dort liegen Symbol und die eingebetteten Anleitungen der C++-App
SRC = Path(sys.argv[1])


def jpg(im, q=82):
    buf = io.BytesIO()
    im.convert('RGB').save(buf, 'JPEG', quality=q, optimize=True, progressive=True)
    return 'data:image/jpeg;base64,' + base64.b64encode(buf.getvalue()).decode()


def png(im):
    buf = io.BytesIO()
    im.save(buf, 'PNG', optimize=True)
    return 'data:image/png;base64,' + base64.b64encode(buf.getvalue()).decode()


def load(name):
    return Image.open(SRC / f'{name}.png')


imgs = {name: jpg(load(name)) for name in ('start', 'discord', 'bf6', 'blogs', 'display')}
imgs['uebersicht'] = jpg(load('prev0'))
imgs['bar'] = jpg(load('speichern').crop((230, 790, 1280, 860)), 90)
imgs['page_system'] = jpg(load('prev0').crop((291, 138, 759, 451)), 90)
imgs['page_blogs'] = jpg(load('prev4').crop((291, 138, 759, 451)), 90)
imgs['freunde'] = jpg(load('freunde').crop((250, 460, 1190, 1420)))
imgs['icon'] = png(Image.open(RES / 'app.ico').convert('RGBA').resize((64, 64), Image.LANCZOS))

EN = len(sys.argv) > 2 and sys.argv[2] == 'en'
html = (HERE / ('anleitung.template.en.html' if EN else 'anleitung.template.html')).read_text(encoding='utf-8')
html = re.sub(r'\{\{img:(\w+)\}\}', lambda m: imgs[m.group(1)], html)
(RES / ('GUIDE.html' if EN else 'ANLEITUNG.html')).write_text(html, encoding='utf-8')
print('GUIDE.html' if EN else 'ANLEITUNG.html', len(html) // 1024, 'KB')
