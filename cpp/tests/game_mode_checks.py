"""Existing offline/unit suites plus Game-Mode; never launches the application EXE."""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'cpp/build/screenbuddy-release'
OUT = ROOT / 'cpp/build/game-mode-checks'
(OUT / 'temp').mkdir(parents=True, exist_ok=True)
env = dict(os.environ, FUCHSLABS_INSTANCE='gamemode-unit',
           FUCHSLABS_INSTALL_DIR=str(OUT / 'data'),
           FUCHSLABS_LEGACY_DIR=str(OUT / 'missing-legacy'),
           TEMP=str(OUT / 'temp'), TMP=str(OUT / 'temp'))
names = ['fl-game-tests', 'fl-custom-view-tests', 'fl-builtin-catalog-tests',
         'fl-layout-edit-tests', 'fl-layout-ui-tests', 'fl-device-profile-tests',
         'fl-tests', 'fl-tmdb-tests', 'fl-psn-tests', 'fl-key-help-tests',
         'fl-amp-tests', 'fl-plugin-tests', 'fl-plugin-zones-tests',
         'fl-plugin-schedule-tests', 'fl-plugin-example-tests',
         'fl-plugin-sign-tests', 'fl-plugin-catalog-tests', 'fl-plugin-store-tests',
         'fl-plugin-transport-tests', 'fl-plugin-settings-tests',
         'fl-plugin-snapshot-tests', 'fl-builtin-snapshot-tests',
         'fl-monitor-tests', 'fl-companion-tests']
commands = [(name, [str(BUILD / (name + '.exe'))]) for name in names]
commands += [('distribution-standalone', [str(BUILD / 'fl-distribution-tests.exe'),
                                         'standalone', str(OUT / 'distribution.json')]),
             ('ai-adapter', ['powershell.exe', '-NoProfile', '-File',
                             str(ROOT / 'cpp/tests/ai_adapter_tests.ps1')])]
results = []
for name, command in commands:
    completed = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, timeout=300)
    (OUT / (name + '.txt')).write_bytes(completed.stdout + completed.stderr)
    results.append({'suite': name, 'exit_code': completed.returncode,
                    'passed': completed.returncode == 0})
    (OUT / 'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(name + (': PASS' if completed.returncode == 0 else ': FAIL'), flush=True)
    if completed.returncode:
        print((completed.stdout + completed.stderr).decode(errors='replace')[-5000:])
        sys.exit(1)
print(f'{len(results)} suites passed; no installer, main-app port or physical device test.')
