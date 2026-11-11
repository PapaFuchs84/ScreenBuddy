"""Create a local-only Store preparation ZIP from explicit public-safe inputs.

This never signs, uploads, deploys or creates an official-identity MSIX.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--build", type=Path, required=True)
parser.add_argument("--summary", type=Path, required=True,
                    help="Reviewed/sanitized test summary JSON with binarySHA256")
parser.add_argument("--output", type=Path, default=ROOT / "dist" / "ScreenBuddy-2.11.0-StorePreparation")
parser.add_argument("--submission-directory", type=Path,
                    help="Optional verified official-identity submission directory")
parser.add_argument("--identity", type=Path, help="Official public package identity JSON (no credentials)")
args = parser.parse_args()
build = args.build.resolve()
target = args.output.resolve()
archive = target.parent / (target.name + ".zip")
if target.exists() or archive.exists():
    raise SystemExit("Choose a fresh output directory; existing artifacts are not replaced.")
summary = json.loads(args.summary.read_text(encoding="utf-8-sig"))
binary_hash = hashlib.sha256((build / "ScreenBuddy.exe").read_bytes()).hexdigest()
if summary.get("binarySHA256", "").lower() != binary_hash:
    raise SystemExit("Reviewed summary does not match the current development binary.")
target.mkdir(parents=True)


def copy(source, relative):
    destination = target / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


for name in ("ScreenBuddy.exe", "ScreenBuddyStartup.exe"):
    copy(build / name, Path("cpp/build/msstore") / name)
for name in ("LICENSE", "THIRD-PARTY-NOTICES.md", "README.md", "README.en.md", "CHANGELOG.md", "CHANGELOG.en.md"):
    copy(ROOT / name, Path(name))
for name in ("STORE-DISTRIBUTION.md", "STORE-DISTRIBUTION.en.md"):
    copy(ROOT / "docs" / name, Path("docs") / name)
if (ROOT / "cpp/tests/Run-ReleaseChecks.ps1").is_file():
    copy(ROOT / "cpp/tests/Run-ReleaseChecks.ps1", Path("cpp/tests/Run-ReleaseChecks.ps1"))
packaging = ROOT / "packaging/windows"
documents = (
    "Build-Msix.ps1", "Prepare-Store.ps1", "Import-StandaloneData.ps1", "Test-Msix.ps1",
    "AppxManifest.xml.in", "store-identity.example.json", "STORE-LISTING.md",
    "APP-PRIVACY.de.md", "APP-PRIVACY.en.md", "APP-SUPPORT.de.md", "APP-SUPPORT.en.md",
    "STORE-FORMS-DRAFT.md", "CERTIFICATION-NOTES.md", "PRIVACY-DATA-INVENTORY.md",
    "RELEASE-CHECKLIST.md", "WEBSITE-ADDITIVE-DEPLOYMENT.md", "Build-AppPages.py",
    "Export-StorePreparation.py",
    "validation-summary-20261110.json",
)
for name in documents:
    copy(packaging / name, Path("packaging/windows") / name)
for directory in ("Assets", "ListingAssets", "Screenshots", "website-pages"):
    for source in sorted((packaging / directory).rglob("*")):
        if source.is_file() and not source.is_symlink():
            copy(source, Path("packaging/windows") / source.relative_to(packaging))
copy(args.summary, Path("test-summary.json"))
official = None
if args.submission_directory:
    if not args.identity:
        raise SystemExit("An official identity file is required with --submission-directory.")
    directory = args.submission_directory.resolve()
    verification = json.loads((directory / "package-verification.json").read_text(encoding="utf-8"))
    if verification.get("officialIdentity") is not True or verification.get("binarySHA256") != binary_hash:
        raise SystemExit("Official package verification does not match this candidate.")
    version = verification["version"]
    if not isinstance(version, str) or any(c not in "0123456789." for c in version):
        raise SystemExit("Unexpected official package version.")
    package = directory / ("ScreenBuddy-" + version + "-x64.msix")
    if hashlib.sha256(package.read_bytes()).hexdigest() != verification["msixSHA256"]:
        raise SystemExit("Official MSIX hash differs from its verification record.")
    identity = json.loads(args.identity.read_text(encoding="utf-8-sig"))
    fields = ("IdentityName", "Publisher", "PublisherDisplayName", "DisplayName", "StoreId", "PackageFamilyName")
    official = {key: identity[key] for key in fields}
    if not all(isinstance(value, str) and value.strip() for value in official.values()):
        raise SystemExit("Incomplete official public identity.")
    if official["IdentityName"] != verification["identityName"] or official["Publisher"] != verification["publisher"] or official["DisplayName"] != verification["displayName"]:
        raise SystemExit("Official identity differs from the verified package.")
    copy(package, Path("official-submission") / package.name)
    copy(directory / "package-verification.json", Path("official-submission/package-verification.json"))
    (target / "official-submission/store-identity.json").write_text(json.dumps(official, indent=2) + "\n", encoding="utf-8")

# Tracked source/docs patch plus an explicit snapshot of modified C++ files makes
# the dirty local candidate reviewable without bundling build logs or user data.
base = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
branch = subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip()
patch = subprocess.check_output(["git", "diff", "--binary", "HEAD", "--", "cpp", "docs", "packaging/windows", "README.md", "README.en.md", "CHANGELOG.md", "CHANGELOG.en.md"], cwd=ROOT)
(target / "source-changes.patch").write_bytes(patch)
changed = subprocess.check_output(["git", "diff", "--name-only", "HEAD", "--", "cpp"], cwd=ROOT, text=True).splitlines()
for relative in changed:
    source = ROOT / relative
    if source.is_file() and source.suffix in (".cpp", ".h", ".md", ".txt", ".py", ".ps1"):
        copy(source, Path("modified-source") / relative)
provenance = {"sourceCommit": base, "branch": branch, "dirtyPatchSHA256": hashlib.sha256(patch).hexdigest(),
              "binarySHA256": binary_hash, "officialStoreIdentity": official is not None, "localDevelopmentOnly": True,
              "publicStandaloneRelease": False, "privateConfigurationIncluded": False,
              "rawMachineLogsIncluded": False}
(target / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
(target / "README-LOCAL-ONLY.txt").write_text("""SCREENBUDDY STORE PREPARATION — LOCAL DEVELOPMENT ONLY

The EXEs are local development binaries. This is NOT a signed public standalone
release and NOT an official-identity Store submission. Do not publish this ZIP as
a Windows release. No private configuration, pairings, certificates or raw
machine logs are included. Read test-summary.json for actual validation limits.

Before submission, reserve the real product and copy the exact Partner Center
identity values into a separate local store-identity.json. Never upload the
example identity or use a validation package as the official product.

From this preparation root on Windows with the Windows SDK installed:
powershell.exe -NoProfile -File packaging/windows/Prepare-Store.ps1 -IdentityFile ./store-identity.json -BuildDirectory ./cpp/build/msstore -OutputDirectory ./official-submission

That command builds an unsigned submission MSIX using the official identity.
Microsoft signs the MSIX as part of Store certification. Public standalone EXEs
have a separate trusted Authenticode requirement; this archive bypasses no gate.

The public policy/support pages must be deployed and verified before submission.
Complete the live Partner Center IARC questionnaire; no final age rating or
certificate is invented here. Review the actual package, screenshots and notes,
then submit through the approved developer account. Validate official-identity
installation/servicing and keep the current physical test scope explicit.

Source provenance: checkout the sourceCommit in provenance.json, review/apply
source-changes.patch and copy the accompanying newly prepared packaging files.
When present, modified-source contains exact edited C++ source snapshots for
review. A clean source commit needs no dirty source snapshot.
""", encoding="utf-8")
if official:
    readme = target / "README-LOCAL-ONLY.txt"
    text = readme.read_text(encoding="utf-8").replace(
        "release and NOT an official-identity Store submission.",
        "release. The separate official-submission folder contains the verified\nunsigned official-identity MSIX candidate, NOT a Microsoft-certified release.")
    text += "\nThe reserved title is " + official["DisplayName"] + ". The public official identity\nis in official-submission/store-identity.json; it contains no credentials.\nSubmit the verified MSIX separately through the existing Store draft after all\nchecks and live form requirements are satisfied. Never upload the whole ZIP.\n"
    readme.write_text(text, encoding="utf-8")
files = sorted(p for p in target.rglob("*") if p.is_file())
lines = [hashlib.sha256(p.read_bytes()).hexdigest() + "  " + p.relative_to(target).as_posix() for p in files]
(target / "SHA256SUMS.txt").write_text("\n".join(lines) + "\n", encoding="ascii")
with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED) as z:
    for p in sorted(target.rglob("*")):
        if p.is_file():
            z.write(p, target.name + "/" + p.relative_to(target).as_posix())
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix(archive.suffix + ".sha256").write_text(digest + "  " + archive.name + "\n", encoding="ascii")
print(str(archive))
print("SHA256 " + digest)
