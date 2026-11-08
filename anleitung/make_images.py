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

def data(path, lossless=False, width=None):
    with Image.open(path) as im:
        if width and im.width > width:
            im = im.resize((width, round(im.height * width / im.width)), Image.Resampling.LANCZOS)
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
    'ha_pick': 'ha', 'design': 'design',
    'cupertino_light':'cupertino-light', 'cupertino_dark':'cupertino-dark',
    'fluent_light':'fluent-light', 'fluent_dark':'fluent-dark',
    'material_light':'material-light', 'material_dark':'material-dark',
}
# Neue Aufnahmen 2.10 (Standard-Design Material-Orange dunkel), je Sprache; Schluessel -> Datei in docs/screenshots/2.10/<de|en>/
NEW = {
    's_uebersicht': '01-uebersicht', 's_anzeige': '02-anzeige', 's_sperrzone': '03-sperrzone',
    's_seiten': '04-seiten-reihenfolge', 's_design': '05-design', 's_ki': '06-ki-status',
    's_game': '07-game-mode', 's_game_erk': '08-game-erkennung', 's_spotify': '09-spotify',
    's_ansichten': '10-eigene-ansichten', 's_ansicht_game': '11-ansicht-game-mode', 's_zonen': '12-zonen-editor',
    's_freunde': '14-freunde', 's_discord': '15-discord', 's_bf6': '16-battlefield', 's_blogs': '17-blogs',
    's_streaming': '18-streaming', 's_ha': '19-homeassistant', 's_erweit': '20-erweiterungen', 's_app': '21-app',
}
# Alte Schluessel zeigen jetzt auf die neuen Aufnahmen (gleiche Namen wie in den Vorlagen)
ALIAS = {'start': 's_uebersicht', 'bar': 's_uebersicht', 'uebersicht': 's_seiten', 'discord': 's_discord', 'bf6': 's_bf6',
         'blogs': 's_blogs', 'display': 's_anzeige', 'app': 's_app', 'freunde': 's_freunde', 'ha_setup': 's_ha',
         'ha_pick': 's_ha', 'design': 's_design'}
SHOTS = HERE.parent / 'docs' / 'screenshots' / '2.10' / ('en' if EN else 'de')
imgs = {key: data(SHOTS / (name + '.png'), width=1280) for key, name in NEW.items()}
for old, new in ALIAS.items():
    imgs[old] = imgs[new]
gif = HERE.parent / 'docs' / 'screenshots' / '2.10' / 'game-mode-intro.gif'
imgs['gif_game'] = 'data:image/gif;base64,' + base64.b64encode(gif.read_bytes()).decode('ascii')
imgs.update({key: data(SRC / (filename + '.png')) for key, filename in native.items() if key not in imgs})
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

