# ScreenBuddy 2.5 – Hardware Acceptance

**27 October 2026, separate Redmi 8 acceptance:** signed Companion 0.2.1-preview + isolated Windows 2.5.0 passed USB-tethered pairing, native landscape/portrait output, real finger gestures, automatic 5-second page rotation and saved-key reconnect after server restart. User confirmation and API evidence are recorded in the [test record](../cpp/TESTPROTOKOLL.md). A local TLS test client also verified rejection of one invalid PIN. This does not approve the physical USB display or Windows target-system upgrade.

**Status:** Acceptance of the final candidate is open. The latest serial demo documented here **FAILED**; on 26 October 2026 the user reported that the image fault had been resolved. This record does not grant RC1 or release approval.

This procedure checks real output and interaction on a Windows PC with a USB display. Record date, device, build/commit, and tester for each run. Builds and automated tests do not replace physical acceptance.

## 1. Prerequisites and safeguards

- Windows PC, candidate build, USB display, and cable. For Companion: Pixel phone with Companion and network access to the PC.
- For this run: `COM7`, USB `VID_1A86`, `PID_5722`; test configuration `C:\Users\admin\FuchsLabs\screenbuddy-hardware-acceptance\2026-10-25\test\config.toml`.
- Before changing installation, configuration, Companion, or update state, verify restorable backups. This run's backups are private at `C:\Users\admin\FuchsLabs\screenbuddy-hardware-acceptance\2026-10-25\private-backup`; do not copy their contents into the repository.
- Before each serial test, check COM-port owner, VID/PID, and device; only one process may hold the port. Use a dedicated test configuration and no production devices, tokens, or URLs. Do not run multiple app instances.
- Do not put credentials, pairing PINs, tokens, private URLs, logs, screenshots, or photos in Git, issues, wiki pages, or releases. Keep evidence private and redact it before sharing.

## 2. Preflight and baseline

1. Record version, commit/build identifier, executable path, and SHA-256.
2. Separately verify installed version and running process: full executable path, PID, and displayed version. Filename or installer status alone is insufficient.
3. Check COM port, VID/PID, device connection, and configuration path; ensure the old app released the port.
4. Before testing, observe the device for at least 60 seconds: static and changing content, brightness, stability, lines/artifacts, flicker, dropouts, colors, and orientation. Photos are private only.
5. Baseline passes only if the device is stable without new artifacts. If a fault already exists, mark the run “not evaluable.”

## 3. Serial demo and real app

### 3.1 Demo smoke test

1. Confirm no other process owns COM7 and the test build uses the separate test configuration.
2. Start the demo with the intended `--demo` option; record time, command, build identifier, port, and exit status.
3. Observe the physical start/end, frames, order/colors, completeness, flicker, lines, dropouts, and recovery afterward.
4. Check the log locally for errors; record exit code and frames sent. Do not publish private logs.
5. Stop the process and confirm port release. Test the real app separately; the demo is not app acceptance.

### 3.2 Real app

1. Back up the previous state, start the real app with the test configuration, and verify expected content on the device.
2. Run for at least 10 minutes: static content for at least 2 minutes, then multiple updates. Check complete frames, colors/orientation, latency, flicker, lines, dropouts, restarts, and CPU/memory growth.
3. Close and reopen normally. Output must resume without manual port release or PC restart.
4. Record timestamp and severity. A visual fault is a failure even if exit code/log look clean.

### Current recorded result

**User update, 26 October 2026:** The user reports that the image fault is resolved and does not want further root-cause work. The report did not include a build hash or visual acceptance of the final candidate. The failed run below remains as a historical test record.

**Visual-test chronology on 25 October 2026:** An earlier, separately recorded `2.5.0-dev.2` candidate (`ScreenBuddy-2.5.0-dev.2.exe`, SHA-256 `fc5e8afe814bfd9f607bd3574fb3ab6bb6d9a5213bb5b46150579f48a891d2e6`, after PR #16) ran for about four minutes on COM7 with a copy of the user's settings. According to the manual acceptance record, both displays showed a good image and touch worked. The display, Discord, RSS, and Companion connection were active during that run. This was a positive, time-limited visual check, not full upgrade or endurance acceptance. The **later** COM7 demo with a further developed local build failed as described below. The earlier positive observation remains recorded; the latest failure is open for the next candidate. The earlier record is outside the repository at `release-2.5.0-dev.2/TESTPROTOKOLL-Handabnahme-2026-10-25.md`.

ScreenBuddy `2.5.0-dev.2` on COM7 (`VID_1A86`, `PID_5722`) sent five frames in the `--demo` run; exit code 0, zero log errors. The visible output was corrupted: blue lines and flicker, including on a static frame. The user confirms the image was normal before the test. The old app also showed a corrupted image afterward; a controlled restart of the old app did not help. The serial visual test **FAILED** and remains a historical finding. Visual acceptance with the current candidate build hash is still missing. With the app stopped, the USB display was then unplugged for 10 seconds and reconnected. After restarting the old app, the user confirmed: “image normal again.” COM7 and the old executable path were verified. Hashes of the previous `config.toml` and `companion.json` are unchanged. Recovery after the power cycle does not undo the failed 2.5 demo test or establish its cause. The photo remains private and must not be added to the repository.

### Interpretation and final-candidate acceptance

The serial image path (`cpp/src/serial_display.cpp`) is unchanged since 2.3.0 except for VID/PID detection: RGB565 conversion, 3,840-byte blocks, 115,200 baud, flow control, and reset behavior are the same. The five-second write timeout applies to **each** `WriteFile` call, not to an entire frame. At 115,200 baud, a 3,840-byte block takes about 0.33 seconds in theory. Thus, the calculation “307,200 bytes take 26.7 seconds, so the five-second timeout expires” does not explain this result. Successful writes and exit code 0 also do not prove that the panel displayed every pixel correctly.

Before approval, record the regular visual acceptance of the final 2.5 candidate with its build hash, the same test configuration and device. The historical failed run used `page_seconds = 8` and `reset_on_start = false`. If the fault recurs, stop the test, restore the old app in a controlled manner, and assess the device only after releasing the port. Keep logs and images private.

## 4. Physical observation prompts

Answer for every run:

- Is the image complete and stable, without blue or other lines? Does it flicker on static content or during updates?
- Are colors, brightness, and orientation correct? Are pixel rows/columns or regions missing?
- How many frames/updates are missing or late? Are there interruptions?
- Does output remain stable after an app restart and brief USB reconnection?
- Are there unintended touch inputs, if touch exists? Test touch separately; image output does not pass touch.
- Do visible result, log, and exit code agree? Visual observation counts for pass/fail.

## 5. Companion on Pixel (independent)

Assess Companion separately from USB output; passing one section does not prove the other.

1. Record Pixel app version and connection prerequisites; use no personal accounts or production data.
2. Open the Companion page on the PC and pair only through its displayed QR code or time-limited PIN. Do not photograph, record, or share PIN/QR contents.
3. Verify the expected page appears on the Pixel and only intended data/functions are visible.
4. Disconnect, close the app/page, and reconnect. Reconnection must work with no orphaned or duplicate session.
5. If a test account/device is available, check an incorrect/expired PIN and a rejected connection. Access must be denied and errors must reveal no secrets.
6. Record Companion pass/fail separately; never include pairing secrets in evidence.

### Current isolated Companion hardware test

ScreenBuddy `2.5.0-dev.2` ran with `--no-lcd` on a separate test port; the existing USB app was left untouched. The first run was misconfigured (`[system] enabled=false`) and showed 0 pages and the welcome screen. After correcting the configuration to System + Friends, the API returned two pages. The automatic index changed from 0 to 1 after 9 seconds. There was one paired and one active client. The user confirmed both pages were correct and stable. After restart and reconnect, there was still one active client. The test instance was stopped and ports released. **Passed:** page rendering, automatic switching, and reconnect in this isolated setup. **Still open:** negative PIN check and separate disconnect/reconnect testing. No IP, PIN, fingerprints, logs, or photos belong in the repository; private evidence remains private.

## 6. Native UI: mouse and keyboard

1. Use the mouse to open/close and move the settings window, minimize/restore it, and operate all controls.
2. Operate without a mouse: check Tab order, visible focus, Enter/Space, Escape, arrow keys, and clear labels.
3. Change/save values, close and reopen; check persistence and runtime effect. Invalid values must be clearly rejected or safely corrected.
4. Keyboard operation must not trigger unintended actions. Check 100%, 150%, and 200% Windows scaling and high contrast where available.

## 7. Upgrade and rollback

Perform only in a controlled test profile with verified backups.

1. Verify starting installation, configuration, and Companion/update state; confirm backups are restorable.
2. Upgrade to the intended 2.5 build. Verify installer status and then the executable, path, and version actually running.
3. Check settings, autostart (if in scope), USB output, and Companion connection. Configuration must be preserved.
4. Roll back and verify the old version that actually starts and its configuration; reassess USB and Companion independently.
5. Fail if installation reports success but the wrong/old executable runs, configuration is lost, or device/Companion cannot be restored.

## 8. Privacy and security

- Never include pairing PINs/QR codes, tokens, passwords, private URLs, personal content, or complete configurations in commits, repositories, public issues, releases, or screenshots.
- Redact logs before sharing; share only minimal necessary excerpts and keep raw logs private.
- Keep the backup folder and private photos outside the repository. This document intentionally includes no screenshots or logs.
- Check rejected pairing attempts, disconnect, and reconnect. Do not test against other people's devices, accounts, or networks.

## 9. Evidence and release gates

For each test case record at least build/commit, date, tester, device/VID/PID/COM, configuration variant, steps, expected/observed result, and pass/fail. Public/versioned evidence must be redacted and free of secrets.

| Gate | Pass criterion | Current status |
|---|---|---|
| Build/process | Build identifier and executable actually running verified | COM7 and old executable verified; local `2.5.0-dev.2` tested |
| Serial demo | Frames visible without artifacts; port released afterward | **FAIL**: five frames; lines/flicker visible |
| Power cycle | Stable after USB unplug/replug; result recorded | Confirmed: unplugged for 10 seconds; old app then showed a normal image; cause unknown |
| Real app | 10-minute visual test and restart pass | Open |
| Pixel Companion | Pairing, operation, disconnect, reconnect pass | Partially passed: two pages, switching, and reconnect confirmed; negative PIN and separate disconnect/reconnect test remain open |
| Native UI | Mouse, keyboard, persistence, scaling pass | Open |
| Upgrade/rollback | Version and configuration verified both ways | Open |
| Privacy/security | No secrets in repository; pairing/disconnect checked | Open |

Overall approval requires all applicable gates to pass with no open critical defects. The current result is **not hardware approval and not RC1 approval**.
