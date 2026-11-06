"""Run existing Windows checks against an explicitly selected build, without device access."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
build = args.build.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
cpp = Path(__file__).resolve().parents[1]
env = dict(os.environ, FUCHSLABS_INSTANCE='release-validation', SCREENBUDDY_TEST_BUILD_DIR=str(build))
results = []
suites = ['fl-tests','fl-tmdb-tests','fl-psn-tests','fl-key-help-tests','fl-amp-tests','fl-plugin-tests',
          'fl-plugin-example-tests','fl-plugin-sign-tests','fl-plugin-catalog-tests','fl-plugin-store-tests',
          'fl-plugin-transport-tests','fl-plugin-settings-tests','fl-plugin-snapshot-tests','fl-monitor-tests','fl-companion-tests']
commands = [(name, [str(build / (name + '.exe'))]) for name in suites]
commands += [('installer-preservation', [sys.executable, str(cpp / 'tests/installer_preserves_config.py'), str(build / 'ScreenBuddy.exe')]),
             ('companion-integration', [sys.executable, str(cpp / 'tests/app_companion_integration.py')]),
             ('standalone-distribution', [str(build / 'fl-distribution-tests.exe'), 'standalone', str(out / 'standalone.json')])]
for name, command in commands:
    completed = subprocess.run(command, cwd=cpp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out / (name + '.txt')).write_bytes(completed.stdout)
    results.append({'suite': name, 'exitCode': completed.returncode, 'passed': completed.returncode == 0})
    (out / 'results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print(name + (': PASS' if completed.returncode == 0 else ': FAIL'), flush=True)
    if completed.returncode:
        print(completed.stdout.decode('utf-8', errors='replace')[-4000:])
        sys.exit(1)
print(str(len(results)) + ' suites passed; no physical device acceptance implied.')
