"""Build a review bundle from an explicit artifact/document allowlist, never from the checkout tree."""
from pathlib import Path
import hashlib
import json
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
output = root / 'dist/ScreenBuddy-2.9.6-preparation.zip'
files = [root / name for name in [
    'LICENSE','THIRD-PARTY-NOTICES.md','docs/RELEASE-PREPARATION-2.9.6.md',
    'docs/STORE-DISTRIBUTION.md','docs/STORE-DISTRIBUTION.en.md',
    'dist/android/ScreenBuddy-Companion-1.0.4.apk','dist/android/SHA256SUMS.txt',
    'dist/windows-local-test/ScreenBuddy-2.9.6.0-x64.msix','dist/windows-local-test/README.md']]
for folder in ['packaging/windows']:
    files += [p for p in (root / folder).rglob('*') if p.is_file() and p.suffix.lower() in {'.ps1','.md','.json','.png','.in'}]
for p in files:
    if not p.is_file(): raise SystemExit('Missing bundle input: ' + str(p))
    if p.suffix.lower() in {'.pfx','.p12','.jks','.keystore','.log','.toml','.properties'}:
        raise SystemExit('Private/build input refused: ' + str(p))
commit = subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
manifest = {'kind':'release-preparation-not-public-release','windowsVersion':'2.9.6','companionVersion':'1.0.4',
            'sourceCommit':commit,'storeAccountReactivationPending':True,'officialStoreIdentityPending':True,
            'publicExeSigning':'blocked-no-certificate','playStorePublication':False,
            'files':[{'path':p.relative_to(root).as_posix(),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(set(files))]}
readme = """# ScreenBuddy release preparation

Unpublished Windows 2.9.6 and Companion 1.0.4 candidates. See docs/RELEASE-PREPARATION-2.9.6.md and packaging/windows/RELEASE-CHECKLIST.md for completed checks and remaining work.

The APK is signed with the established Companion release key. The MSIX has a local test identity and is unsigned; it must not be submitted or distributed as a public release. No standalone EXE is included because trusted Windows code signing is not yet configured. No Play Store upload occurred.

Build/signing scripts in this bundle are reference copies; run the commands from the complete repository checkout, not from this documentation bundle. Never include credentials, signing keys or personal configuration in a submission.
"""
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    z.writestr('README.md',readme)
    z.writestr('MANIFEST.json',json.dumps(manifest,indent=2))
    for p in sorted(set(files)): z.write(p,p.relative_to(root).as_posix())
(root / 'dist/PREPARATION-SHA256SUMS.txt').write_text(hashlib.sha256(output.read_bytes()).hexdigest()+'  '+output.name+'\n',encoding='ascii')
print(output)
print(str(len(files))+' allowlisted inputs; no standalone EXE or private material')
