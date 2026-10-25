"""Regression: an immediate pairing session survives discarded form edits.

Uses a temporary --no-lcd instance, its own free API/TLS ports and test-only
Companion state. No installed instance, display/COM port or saved pairing is used.
Reuses the native settings smoke test's MSAA client; prints no pairing secrets.
"""
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import tempfile
import time
from urllib.request import Request, ProxyHandler, build_opener

import native_settings_smoke as ui


def free_port():
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def main():
    build = ui.BUILD.resolve()
    exe = build / "ScreenBuddy.exe"
    assert exe.is_file(), "ScreenBuddy test build missing"
    ui.OLE32.CoInitializeEx(None, 2)
    api_port, tls_port = free_port(), free_port()
    while tls_port == api_port or tls_port == 8765:
        tls_port = free_port()
    local_http = build_opener(ProxyHandler({}))

    def api(path, post=False):
        request = Request(f"http://127.0.0.1:{api_port}/api/{path}",
                          data=b"{}" if post else None,
                          headers={"X-FL": "1", "Content-Type": "application/json"})
        with local_http.open(request, timeout=2) as response:
            return json.load(response)

    def active():
        return api("companion/status").get("pairing", {}).get("active", False)

    checks = 0
    with tempfile.TemporaryDirectory(prefix="native-pairing-", dir=build,
                                     ignore_cleanup_errors=True) as folder:
        config = Path(folder) / "config.toml"
        config.write_text(
            '[display]\noutput = "serial"\nbrightness = 80\n'
            f'[ui]\nport = {api_port}\nupdate_check = false\nsetup_done = false\n'
            'language = "de"\nsettings_theme = "hell"\n'
            f'[companion]\nenabled = true\nport = {tls_port}\npage_seconds = 12\n'
            '[system]\nenabled = false\n', encoding="utf-8")
        env = dict(os.environ, FUCHSLABS_INSTANCE=f"-native-pairing-{api_port}")
        process = subprocess.Popen(
            [str(exe), "--run", "--no-lcd", "--settings", "--config", str(config),
             "--log", str(Path(folder) / "test.log")],
            env=env, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            frames = []
            last_status = {}
            connection_error = []

            def ready():
                frames[:] = [h for h, shown in ui.windows(process.pid, "ScreenBuddyNativeSettings") if shown]
                try:
                    last_status.update(api("companion/status"))
                except OSError as error:
                    reason = getattr(error, "reason", None)
                    connection_error[:] = [type(error).__name__, getattr(error, "code", None),
                                          type(reason).__name__, getattr(reason, "errno", None)]
                    return False
                return bool(frames) and last_status.get("running", False)

            try:
                ui.wait(ready, 20, "Isolated settings/Companion test instance did not start")
            except AssertionError:
                # Boolean diagnostics only: never include PINs, addresses, logs or fingerprints.
                raise AssertionError(
                    f"Isolated startup failed: exit={process.poll()}, window={bool(frames)}, "
                    f"enabled={last_status.get('enabled')}, running={last_status.get('running')}, "
                    f"no_lan={'Heimnetz' in last_status.get('error', '')}, transport={connection_error}") from None
            frame = frames[0]
            content = ui.content_window(frame)
            nav = ui.Acc(frame)

            def navigate(name):
                nav.do(nav.find(name, ui.ROLE_PAGETAB, exact=True))

            def names():
                return [name for _, name, _ in ui.Acc(content).items()]

            def pairing_visible():
                current = names()
                return "QR-Code zum Koppeln" in current and "Abbrechen" in current

            def start_pairing():
                def enabled():
                    body = ui.Acc(content)
                    button = body.find("Kopplungscode anzeigen", ui.ROLE_PUSHBUTTON)
                    state = ui.VARIANT()
                    body._m(14, ui.VARIANT, ui.ctypes.POINTER(ui.VARIANT))(
                        body.ptr, ui.child(button), ui.ctypes.byref(state))
                    return not state.val & 1  # STATE_SYSTEM_UNAVAILABLE

                ui.wait(enabled, 8, "Pairing button did not become enabled")
                body = ui.Acc(content)
                body.do(body.find("Kopplungscode anzeigen", ui.ROLE_PUSHBUTTON))
                ui.wait(lambda: pairing_visible() and active(), 8, "Test pairing did not start")

            def presentation():
                current = names()
                pin = next((name for name in current if re.fullmatch(r"\d{3} \d{3}", name)), None)
                fingerprint = next((name for name in current if name.startswith("SHA-256: ")), None)
                countdown = next((re.search(r"noch (\d+):(\d{2})", name) for name in current
                                  if re.search(r"noch (\d+):(\d{2})", name)), None)
                assert pin and fingerprint and countdown, "Pairing presentation is incomplete"
                return pin, fingerprint, int(countdown[1]) * 60 + int(countdown[2])

            def edit_and_discard(slider_name):
                body = ui.Acc(content)
                slider = body.find(slider_name, ui.ROLE_SLIDER, exact=True)
                before = body.value(slider)
                body.focus(slider)
                ui.key(frame, 0x27)
                assert body.value(slider) != before, "Test edit did not change the slider"
                save = nav.find("Speichern & anwenden", ui.ROLE_PUSHBUTTON)
                assert "(1 Änderung)" in nav.name(save), "Test edit was not counted"
                nav.do(nav.find("Verwerfen", ui.ROLE_PUSHBUTTON))
                ui.wait(lambda: "(0 " in nav.name(save), 8, "Discard did not restore the form")

            navigate("Companion")
            start_pairing()
            ui.wait(lambda: any("noch " in name for name in names()), 4, "Pairing countdown missing")
            original_pin, original_print, original_left = presentation()
            checks += 2
            # Leave the status-poll grace period before rebuilding the page.
            time.sleep(4.2)
            edit_and_discard("Seitenwechsel")
            assert pairing_visible() and active(), "Discard hid an active pairing session"
            pin, fingerprint, left = presentation()
            assert (pin, fingerprint) == (original_pin, original_print), "Discard replaced pairing credentials"
            assert 0 < left < original_left, "Discard reset the pairing deadline"
            checks += 5

            # Discard while a different page is visible, then lazily rebuild Companion.
            navigate("Anzeige")
            edit_and_discard("Helligkeit")
            navigate("Companion")
            assert pairing_visible() and active(), "Reopening after discard hid an active pairing session"
            pin, fingerprint, _ = presentation()
            assert (pin, fingerprint) == (original_pin, original_print), "Reopening replaced pairing credentials"
            checks += 4

            body = ui.Acc(content)
            body.do(body.find("Abbrechen", ui.ROLE_PUSHBUTTON, exact=True))
            ui.wait(lambda: not active() and not pairing_visible(), 8, "Cancel after rebuild did not end pairing")
            checks += 1

            # A fresh session is possible; server-side completion clears the cached presentation.
            start_pairing()
            api("companion/pair/cancel", post=True)
            ui.wait(lambda: not pairing_visible(), 8, "Server-ended pairing stayed visible")
            edit_and_discard("Seitenwechsel")
            assert not pairing_visible(), "Discard restored an ended pairing session"
            checks += 4
            print(f"native Companion rebuild: {checks} checks passed (same session, deadline, cancel, server completion)")
        finally:
            try:
                api("quit", post=True)
            except OSError:
                pass
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=5)


if __name__ == "__main__":
    main()
