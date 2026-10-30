"""Two simulated phones against an isolated app with companion.sync = false (independent devices).

Usage: python cpp/tests/app_companion_multi.py
Isolated like app_companion_integration.py: free OS ports, --no-lcd, never touches COM7 or port 8765.
"""
import json
import os
import socket
import struct
import subprocess
import time
import urllib.request

import app_companion_integration as base
from app_companion_integration import BUILD, WS, check, wait_for


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
    work = BUILD / f'integration-multi-{sync}'; work.mkdir(exist_ok=True)
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
        a = WS(status['addresses'][0], phone, status['fingerprint'])
        a.send({'t': 'hello', 'v': 1, 'pin': api('/api/companion/pair', {})['pin'], 'w': 960, 'h': 720, 'name': 'Phone A'})
        a.ready(960, 720)
        b = WS(status['addresses'][0], phone, status['fingerprint'])
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
    run(True)
    run(False)
    print(f'{len(base.checks)} multi-device checks passed')
