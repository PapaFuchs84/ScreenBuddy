"""Silent simulation; the app starts only through Start-GameMode-Test.ps1."""
import json
from pathlib import Path
import subprocess
import time
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'cpp/build/game-mode-test'
DATA = OUT / 'data'
BUILD = ROOT / 'cpp/build/screenbuddy-release'
BASE = 'http://127.0.0.1:18766'


def api(path, body=None):
    req = Request(BASE + path, data=None if body is None else json.dumps(body).encode(),
                  headers={'X-FL': '1', 'Content-Type': 'application/json'})
    with urlopen(req, timeout=3) as response:
        raw = response.read()
        return raw if path.endswith('.png') else json.loads(raw)


def wait(predicate, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            value = predicate()
            if value:
                return value
        except OSError:
            pass
        time.sleep(.1)
    raise AssertionError('Timed out waiting for isolated test state')


def view(id):
    blocks = ['system.game_name', 'system.game_time', 'system.game_logo', 'system.fps']
    return {'schema_version': 1, 'id': id, 'name': 'Example Game',
            'layout': {'columns': [1, 1], 'rows': [1, 1], 'tiles': [
                {'source': 'system', 'block': block, 'zone': {'id': f't{i}', 'column': i % 2, 'row': i // 2},
                 'column_span': 1, 'row_span': 1, 'density': 2}
                for i, block in enumerate(blocks)]}, 'devices': {}}


DATA.mkdir(parents=True, exist_ok=True)
(DATA / 'views').mkdir(exist_ok=True)
for id in ['game-test', 'normal-test']:
    (DATA / 'views' / (id + '.view.json')).write_text(json.dumps(view(id)), encoding='utf-8')
dummy_path = BUILD / 'fl-game-dummy.exe'
config = f'''[ui]
port = 18766
enabled = true
setup_done = true
standard_theme_choice = "accepted"
update_check = false
plugin_store = false

[ai]
enabled = false
automatic_usage = false

[display]
pages = ["custom:normal-test"]
visible_pages = ["custom:normal-test"]
page_seconds = 600
theme = "material-orange"
screen_off_on_exit = false

[system]
enabled = false

[discord]
enabled = false

[companion]
enabled = false
port = 18777

[game_mode]
enabled = true
target_display = "primary"
view = "custom:game-test"
intro = true
reduce_motion = false
ping_target = "127.0.0.1"
additional_games = {json.dumps(str(dummy_path))}
exclusions = ""
'''
(DATA / 'config.toml').write_text(config, encoding='utf-8')
app_pid = None
dummy = None
try:
    launched = subprocess.run(['powershell.exe', '-NoProfile', '-File',
                              str(ROOT / 'cpp/tests/Start-GameMode-Test.ps1')],
                             cwd=ROOT, capture_output=True, text=True)
    assert launched.returncode == 0, launched.stderr
    app_pid = json.loads(launched.stdout)['pid']
    initial = wait(lambda: (nav if (nav := api('/api/pages')).get('current') else None))
    assert initial['current'] == ['custom:normal-test'], initial
    dummy = subprocess.Popen([str(dummy_path)], cwd=ROOT, creationflags=0x08000000)
    time.sleep(1)
    assert not api('/api/pages').get('game_mode', False), 'start debounce bypassed'
    active = wait(lambda: (nav if (nav := api('/api/pages')).get('game_mode') else None))
    assert active['current'] == ['custom:game-test'], active
    time.sleep(.8)  # Capture a visible intro phase, after the initial dimming frame.
    (OUT / 'intro.png').write_bytes(api('/api/preview.png'))
    time.sleep(3)
    (OUT / 'active.png').write_bytes(api('/api/preview.png'))
    dummy.terminate()
    dummy.wait(timeout=5)
    dummy = None
    time.sleep(1)
    assert api('/api/pages').get('game_mode'), 'end debounce bypassed'
    restored = wait(lambda: (nav if not (nav := api('/api/pages')).get('game_mode') else None))
    assert restored['current'] == ['custom:normal-test'], restored
    (OUT / 'restored.png').write_bytes(api('/api/preview.png'))
    report = {'passed': True, 'simulation': 'silent dummy / exact additional EXE path',
              'ui_port': 18766, 'ai_port': 18765, 'ai_receiver': 'disabled: port occupied, not needed for game-mode', 'start_debounce': True,
              'end_debounce': True, 'rotation_restored': True,
              'screenshots': ['intro.png', 'active.png', 'restored.png'],
              'physical_device_test': False, 'real_game_test': False}
    (OUT / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))
finally:
    if dummy:
        dummy.terminate()
        dummy.wait(timeout=5)
    if app_pid:
        api('/api/quit', {})
