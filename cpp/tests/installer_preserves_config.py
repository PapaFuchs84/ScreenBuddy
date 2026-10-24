"""Exercise installation settings migration and snapshots in isolated Windows folders."""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from urllib.error import URLError
from urllib.request import Request, urlopen

CPP = Path(__file__).resolve().parents[1]
BUILD = CPP / "build"


def free_port() -> int:
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def stop_app(port: int) -> None:
    request = Request(f"http://127.0.0.1:{port}/api/quit", data=b"{}", method="POST",
                      headers={"X-FL": "1", "Content-Type": "application/json"})
    try:
        with urlopen(request, timeout=2):
            pass
    except (OSError, URLError):
        pass
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        try:
            with urlopen(f"http://127.0.0.1:{port}/api/status", timeout=0.3):
                time.sleep(0.2)
        except (OSError, URLError):
            return
    raise AssertionError("isolated installed app did not stop")


def run_install(source_exe: Path, install_dir: Path, port: int, log: Path) -> None:
    legacy = install_dir.parent / "missing-legacy"
    env = dict(os.environ)
    env.update({
        "FUCHSLABS_INSTANCE": f"-installer-config-{port}",
        "FUCHSLABS_INSTALL_DIR": str(install_dir),
        "FUCHSLABS_LEGACY_DIR": str(legacy),
    })
    result = subprocess.run(
        [str(source_exe), "--install", "--silent", "--no-lcd", "--log", str(log)],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30,
    )
    assert result.returncode == 0, f"isolated installer failed ({result.returncode})"
    assert (install_dir / source_exe.name).is_file(), "installer did not create the isolated application"

    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        try:
            with urlopen(f"http://127.0.0.1:{port}/api/status", timeout=1):
                stop_app(port)
                return
        except (OSError, URLError):
            time.sleep(0.2)
    raise AssertionError("isolated installed app did not start")


def assert_snapshot(config: Path, expected: bytes) -> None:
    snapshot = config.with_name("config.toml.install-backup")
    assert config.is_file() and snapshot.is_file(), "installer omitted settings or their snapshot"
    assert config.read_bytes() == expected, "installer changed settings"
    assert snapshot.read_bytes() == expected, "settings snapshot differs from the source"


def check_existing_settings(exe: Path, root: Path) -> None:
    port = free_port()
    install_dir = root / "existing-install"
    install_dir.mkdir()
    settings = (f'[display]\noutput = "serial"\n\n'
                f'[ui]\nlanguage = "en"\nport = {port}\nupdate_check = false\n').encode()
    config = install_dir / "config.toml"
    config.write_bytes(settings)
    run_install(exe, install_dir, port, root / "existing.log")
    assert_snapshot(config, settings)


def check_portable_settings(exe: Path, root: Path) -> None:
    port = free_port()
    source_dir = root / "portable"
    install_dir = root / "fresh-install"
    source_dir.mkdir()
    install_dir.mkdir()
    portable_exe = source_dir / exe.name
    shutil.copy2(exe, portable_exe)
    settings = (f'[display]\noutput = "serial"\n\n'
                f'[ui]\nlanguage = "de"\nport = {port}\nupdate_check = false\n').encode()
    (source_dir / "config.toml").write_bytes(settings)
    companion_data = b'{"version": 1, "devices": []}'
    (source_dir / "companion.json").write_bytes(companion_data)
    run_install(portable_exe, install_dir, port, root / "portable.log")
    assert_snapshot(install_dir / "config.toml", settings)
    assert (install_dir / "companion.json").read_bytes() == companion_data, "installer omitted companion store"


def main() -> int:
    if os.name != "nt":
        print("installer config checks are Windows-only; skipped")
        return 0
    exe = Path(sys.argv[1] if len(sys.argv) > 1 else BUILD / "FuchsLabs-USB-Display.exe").resolve()
    if not exe.is_file():
        raise SystemExit(f"missing application build: {exe}")
    BUILD.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="installer-config-", dir=BUILD) as temp:
        root = Path(temp).resolve()
        if not root.is_relative_to(BUILD.resolve()):
            raise SystemExit("temporary installer test escaped the build folder")
        check_existing_settings(exe, root)
        print("installer retained existing config and made a local snapshot")
        check_portable_settings(exe, root)
        print("local install migrated portable config and made a local snapshot")
    print("2 installer config checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
