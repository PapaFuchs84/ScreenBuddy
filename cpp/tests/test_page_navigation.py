"""Exercise the real app/HTTP navigation without credentials or display hardware.

Usage: python tests/test_page_navigation.py build/FuchsLabs-USB-Display.exe [expected-version]
The app runs on port 8766 with its test-instance guard and --no-lcd.
"""
import contextlib
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

BASE = 'http://127.0.0.1:8766'
checks = 0


def check(condition, message):
    global checks
    if not condition:
        raise AssertionError(message)
    checks += 1


def request(path, body=None, headers=None):
    headers = {'X-FL': '1', **(headers or {})}
    if body is not None:
        headers['Content-Type'] = 'application/json'
    data = body if isinstance(body, bytes) else json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(BASE + path, data=data, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def state():
    return json.loads(request('/api/pages')[1])


def wait_for(predicate, timeout=8):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            current = state()
            if predicate(current):
                return current
        except (OSError, ValueError):
            pass
        time.sleep(.1)
    raise AssertionError('Timed out waiting for navigation state')


@contextlib.contextmanager
def instance(exe, system=True, friends=True, duration=60):
    # Refuse to interfere with another test/server on the chosen port.
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', 8766))
    with tempfile.TemporaryDirectory(prefix='fl-pages-2.0.4-', dir=exe.parent) as directory:
        config = Path(directory) / 'config.toml'
        config.write_text(f'''[ui]
port = 8766
setup_done = true
[display]
pages = ["system", "friends"]
page_seconds = {duration}
[system]
enabled = {str(system).lower()}
[friends]
enabled = {str(friends).lower()}
include_discord = false
[discord]
enabled = false
[bf6]
enabled = false
[rss]
enabled = false
''', encoding='utf-8')
        before = hashlib.sha256(config.read_bytes()).digest()
        env = {**os.environ, 'FUCHSLABS_INSTANCE': '-page-navigation-2.0.4'}
        proc = subprocess.Popen([str(exe), '--run', '--no-lcd', '-c', str(config),
                                 '--log', str(Path(directory) / 'test.log')], env=env,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            wait_for(lambda s: s['total'] == int(system) + int(friends) and
                     request('/api/preview.png')[0] == 200, timeout=20)
            yield proc
            check(proc.poll() is None, 'navigation must not restart the app')
            check(hashlib.sha256(config.read_bytes()).digest() == before, 'navigation must not save config')
        finally:
            if proc.poll() is None:
                try:
                    request('/api/quit', {})
                    proc.wait(timeout=15)
                except (OSError, subprocess.TimeoutExpired):
                    proc.kill()
                    proc.wait()
                    raise


def step(delta, expected):
    code, body = request('/api/pages/step', {'delta': delta})
    check(code == 200 and json.loads(body)['ok'], 'valid navigation request')
    return wait_for(lambda s: s['index'] == expected)


def main():
    exe = Path(sys.argv[1]).resolve()
    expected_version = sys.argv[2] if len(sys.argv) > 2 else '2.3.0'
    with instance(exe):
        check(json.loads(request('/api/config')[1])['version'] == expected_version, 'API version')
        check(state()['can_step'], 'multiple views enable navigation')
        check(state()['current'] == ['system'], 'initial system view')
        check(step(1, 1)['current'] == ['friends'], 'next view')
        check(step(1, 0)['current'] == ['system'], 'forward wraparound')
        check(step(-1, 1)['current'] == ['friends'], 'backward wraparound')
        check(step(-1, 0)['current'] == ['system'], 'previous view')
        check(request('/api/preview.png')[1].startswith(b'\x89PNG'), 'preview stays available')
        for invalid in [{}, [], None, {'delta': 0}, {'delta': 2}, {'delta': '1'},
                        {'delta': True}, {'delta': 1.0}, {'delta': 2**100}, {'delta': -2**100}]:
            check(request('/api/pages/step', json.dumps(invalid).encode())[0] == 400,
                  f'reject invalid navigation input: {invalid!r}')
        check(request('/api/pages/step', b'{broken')[0] == 400, 'reject malformed JSON')
        check(request('/api/pages/step', {'delta': 1}, {'X-FL': ''})[0] == 403, 'require write header')
        check(request('/api/pages/step', {'delta': 1}, {'Origin': 'https://example.com'})[0] == 403,
              'reject foreign origin')
        check(request('/api/pages/step', {'delta': 1}, {'Sec-Fetch-Site': 'cross-site'})[0] == 403,
              'reject cross-site write')
        check(request('/api/pages', headers={'Sec-Fetch-Site': 'cross-site'})[0] == 403,
              'reject cross-site state access')
        check(request('/api/pages', headers={'Host': 'example.com'})[0] == 403, 'reject foreign host')
        check(state()['index'] == 0, 'invalid requests must not change the view')
    for system in [True, False]:
        with instance(exe, system=system, friends=False):
            s = state()
            check(s['total'] == int(system), 'single/empty view count')
            check(not s['can_step'], 'disable navigation when all views fit')
            step(1, 0)
            step(-1, 0)
    with instance(exe, duration=3):
        wait_for(lambda s: s['index'] == 1, timeout=6)
        check(True, 'automatic cycling remains active')
        step(-1, 0)
        time.sleep(1.5)
        check(state()['index'] == 0, 'manual step resets the page duration')
        wait_for(lambda s: s['index'] == 1, timeout=5)
        check(True, 'automatic cycling resumes after manual navigation')
    print(f'{checks} page navigation/API checks passed; no display hardware used.')


if __name__ == '__main__':
    main()
