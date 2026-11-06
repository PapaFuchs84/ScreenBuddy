"""Two simulated phones against an isolated app with companion.sync = false (independent devices).

Usage: python cpp/tests/app_companion_multi.py
Isolated like app_companion_integration.py: free OS ports, --no-lcd, never touches COM7 or port 8765.
"""
import json
import base64
import sys
from pathlib import Path
import os
import socket
import struct
import subprocess
import time
import tempfile
import urllib.request

import app_companion_integration as base
from app_companion_integration import BUILD, WS, check, wait_for


class ProfileWS(WS):
    key = None
    def recv(self):
        msg = super().recv()
        if msg.get('t') == 'paired': self.key = msg['key']
        return msg


def jpeg(ws, width, height):
    until = time.monotonic() + 15
    while time.monotonic() < until:
        msg = ws.recv()
        if msg.get('t') == 'binary':
            # Windows-Bordmittel statt zusätzlicher Python-Pakete; nur synthetische Testbilder.
            script = r"""Add-Type -AssemblyName System.Drawing
$bytes=[Convert]::FromBase64String([Console]::In.ReadToEnd())
$stream=[IO.MemoryStream]::new($bytes,0,$bytes.Length)
$image=[Drawing.Bitmap]::new($stream)
$color=$image.GetPixel(8,8); $uniform=$true; $edgeContent=$false; $firstContent=-1
for($y=8;$y -lt $image.Height-8;$y+=16) {
 for($x=8;$x -lt $image.Width-8;$x+=16) {
  if($x -lt 40 -or $x -ge $image.Width-40 -or $y -lt 40 -or $y -ge $image.Height-40) {
   $c=$image.GetPixel($x,$y)
   if([Math]::Abs([int]$c.R-$color.R) -gt 8 -or [Math]::Abs([int]$c.G-$color.G) -gt 8 -or [Math]::Abs([int]$c.B-$color.B) -gt 8) {$uniform=$false}
  }
 }
}
# Header colour differs from the bottom-left background; scan its middle row exactly.
$color=$image.GetPixel(0,$image.Height-1)
for($x=0;$x -lt $image.Width;$x++) {
 $c=$image.GetPixel($x,80)
 if([Math]::Abs([int]$c.R-$color.R) -gt 24 -or [Math]::Abs([int]$c.G-$color.G) -gt 24 -or [Math]::Abs([int]$c.B-$color.B) -gt 24) {
  if($firstContent -lt 0) {$firstContent=$x}
 }
}
# Negative control: content in at least one outer strip, compared to centre background.
$bg=$image.GetPixel([int]($image.Width/2),[int]($image.Height/2))
for($y=8;$y -lt $image.Height-8;$y+=16) {
 foreach($x in @(8,24,($image.Width-24),($image.Width-8))) {
  $c=$image.GetPixel($x,$y)
  if([Math]::Abs([int]$c.R-$bg.R) -gt 24 -or [Math]::Abs([int]$c.G-$bg.G) -gt 24 -or [Math]::Abs([int]$c.B-$bg.B) -gt 24) {$edgeContent=$true}
 }
}
@{width=$image.Width;height=$image.Height;uniform=$uniform;edgeContent=$edgeContent;firstContent=$firstContent}|ConvertTo-Json -Compress
$image.Dispose();$stream.Dispose()
"""
            result = subprocess.run(['powershell','-NoProfile','-Command',script],
                                    input=base64.b64encode(msg['data'][1:]), capture_output=True, check=True)
            image = json.loads(result.stdout)
            if (image['width'],image['height']) == (width,height): return image
    raise AssertionError('JPEG not received')


def frames_until(ws, seconds):
    """Binary frames received within the time (pings are answered by WS.recv)."""
    ws.s.settimeout(0.4)
    count, end = 0, time.monotonic() + seconds
    while time.monotonic() < end:
        try:
            if ws.recv().get('t') == 'binary':
                count += 1
        except (TimeoutError, socket.timeout):
            pass
    return count


def run(sync):
    ports = []
    for _ in range(2):
        with socket.socket() as s:
            s.bind(('127.0.0.1', 0)); ports.append(s.getsockname()[1])
    ui, phone = ports
    check(not {ui, phone} & {8765, 8777, 8799}, 'Isolated test ports')
    work = Path(tempfile.mkdtemp(prefix=f'integration-multi-{sync}-',dir=BUILD))
    config = work / 'config.integration.toml'
    config.write_text(f'''[display]
output = "serial"
visible_pages = ["system", "friends"]
page_seconds = 600
[companion]
enabled = true
port = {phone}
pages = ["system", "friends"]
page_seconds = 600
sync = {str(sync).lower()}
[ui]
port = {ui}
setup_done = true
update_check = false
[discord]
enabled = false
[friends]
enabled = true
include_discord = false
[system]
enabled = true
''', encoding='utf-8')
    root = f'http://127.0.0.1:{ui}'

    def api(path, body=None, raw=False):
        req = urllib.request.Request(root + path, data=None if body is None else json.dumps(body).encode(),
                                     headers={'X-FL': '1', 'Content-Type': 'application/json'})
        with urllib.request.urlopen(req, timeout=4) as r:
            data = r.read()
        return data if raw else json.loads(data)
    env = dict(os.environ, FUCHSLABS_INSTANCE=f'-companion-multi-{ui}')
    p = subprocess.Popen([str(BUILD / 'ScreenBuddy.exe'), '--run', '--config', str(config), '--log', str(work / 'test.log'), '--no-lcd'],
                         env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    a = b = None
    try:
        status = wait_for(lambda: (x if (x := api('/api/companion/status')).get('running') else None))
        a = ProfileWS(status['addresses'][0], phone, status['fingerprint'])
        a.send({'t': 'hello', 'v': 1, 'pin': api('/api/companion/pair', {})['pin'], 'w': 960, 'h': 720, 'name': 'Phone A'})
        a.ready(960, 720)
        b = ProfileWS(status['addresses'][0], phone, status['fingerprint'])
        b.send({'t': 'hello', 'v': 1, 'pin': api('/api/companion/pair', {})['pin'], 'w': 720, 'h': 960, 'name': 'Phone B'})
        if sync:
            b.ready(960, 720)   # shared resolution: the first device decides
            check(True, 'sync on: second phone gets the shared resolution')
            return
        b.ready(720, 960)
        check(True, 'sync off: every phone gets its own resolution')
        def index(name):
            for cl in api('/api/companion/status').get('clients', []):
                if cl.get('name') == name and 'navigation' in cl:
                    return cl['navigation']['index']
            return None
        wait_for(lambda: index('Phone A') is not None and index('Phone B') is not None)
        a0, b0 = index('Phone A'), index('Phone B')
        a.send({'t': 'swipe', 'dir': 'left'})
        wait_for(lambda: index('Phone A') != a0)
        time.sleep(1.0)
        check(index('Phone B') == b0, 'sync off: swipe on phone A leaves phone B on its page')
        a1 = index('Phone A')
        b.send({'t': 'swipe', 'dir': 'left'})
        wait_for(lambda: index('Phone B') != b0)
        check(index('Phone A') == a1, 'sync off: swipe on phone B leaves phone A on its page')
        a.send({'t': 'resize', 'w': 1520, 'h': 720}); a.ready(1520, 720)
        time.sleep(0.5)
        b.send({'t': 'resize', 'w': 960, 'h': 720}); b.ready(960, 720)
        check(True, 'sync off: each phone rotates independently')
        # Kopplungen sind ausschließlich Testdaten. Profil vor Neustart in den Store setzen.
        keys = (a.key, b.key)
        a.close(); b.close(); a = b = None
        api('/api/quit', {}); p.wait(timeout=12)
        check(p.returncode == 0, 'Profile setup: isolated app exits cleanly')
        store_path = work / 'companion.json'
        data = json.loads(store_path.read_text(encoding='utf-8'))
        for d in data['devices']:
            if d['name'] == 'Phone B': d['keepout'] = dict(top=48,right=48,bottom=48,left=48)
        store_path.write_text(json.dumps(data), encoding='utf-8')
        p = subprocess.Popen([str(BUILD / 'ScreenBuddy.exe'), '--run', '--config', str(config), '--log', str(work / 'test.log'), '--no-lcd'],
                             env=env, creationflags=subprocess.CREATE_NO_WINDOW)
        status = wait_for(lambda: (x if (x := api('/api/companion/status')).get('running') else None))
        a = ProfileWS(status['addresses'][0], phone, status['fingerprint'])
        a.send(dict(t='hello',v=1,key=keys[0],w=2400,h=1080)); a.ready(2400,1080)
        b = ProfileWS(status['addresses'][0], phone, status['fingerprint'])
        b.send(dict(t='hello',v=1,key=keys[1],w=1920,h=1080)); b.ready(1920,1080)
        image_a = jpeg(a,2400,1080)
        check(image_a['width'] == 2400, 'Profile A: full ready and JPEG size')
        check(image_a['edgeContent'] and image_a['firstContent'] == 0, 'Profile A: zero keepout has content up to the edge')
        image = jpeg(b,1920,1080)
        check((image['width'],image['height']) == (1920,1080), 'Profile B: full ready and JPEG size')
        check(image['uniform'], 'Profile B: all four keepout strips are uniform (JPEG tolerance 8)')
        check(abs(image['firstContent'] - image_a['firstContent'] - 48) <= 2, 'Profile B: content begins at x=48 (JPEG tolerance 2 pixels)')
        wait_for(lambda: index('Phone B') is not None)
        before = index('Phone B')
        for x,y in ((20,540),(960,20),(1900,540),(960,1060)):
            b.send(dict(t='touch',x=x,y=y)); time.sleep(1.8)
            check(index('Phone B') == before, f'Profile B: keepout touch ({x},{y}) leaves navigation unchanged')
        # Kontrolltipp im Inhalt: bei einer Kachel und zwei Seiten muss Navigation wechseln.
        b.send(dict(t='touch',x=1900-48,y=1000))
        wait_for(lambda: index('Phone B') != before)
        check(True, 'Profile B: content touch changes navigation')

    finally:
        for w in (a, b):
            if w:
                w.close()
        try:
            api('/api/quit', {})
        except OSError:
            pass
        try:
            p.wait(timeout=12)
        except subprocess.TimeoutExpired:
            p.terminate(); p.wait(timeout=5)
        check(p.returncode == 0, 'Isolated app exits cleanly')


if __name__ == '__main__':
    if len(sys.argv) > 1:
        BUILD = Path(sys.argv[1]).resolve().parent
    run(True)
    run(False)
    print(f'{len(base.checks)} multi-device checks passed')
