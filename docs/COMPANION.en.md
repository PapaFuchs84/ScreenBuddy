## Companion app (from 2.3.0): independent displays

Current version: Companion 1.1.1 (code 10), targetSdk 36, dark default design with orange accents; [build and Store preparation](STORE-DISTRIBUTION.en.md). 1.1.1 is a bug-fix release for 1.1.0 (frame hand-over, automatic mode, re-pairing); the protocol is unchanged and stays compatible with ScreenBuddy 2.12.0 and older. The predecessors were 1.1.0 and 1.0.3.

## Automatic mode (Companion 1.1.0)

Automatic mode defaults to off and requires an existing pairing. It uses the PC Companion service's existing local mDNS announcement; no PC protocol change is required. The PC and Android device must be reachable on the private network, and the guest network, router or firewall must not block mDNS. Network changes or a changed PC address can prevent detection; connect manually and check the saved connection in that case.

While mDNS discovery is active, the app holds a Wi-Fi multicast lock so Android can receive announcements in the background; it releases the lock when discovery stops or fails. Continuous discovery with automatic mode enabled may increase battery consumption. Global Wi-Fi/power-saving settings are not changed; there is no continuous CPU wake lock.

Some devices (observed on the Redmi 8) receive no mDNS while the screen is locked; a time-limited CPU wake lock did not change that. The app therefore also checks via an Android alarm (targeted every minute, measured at about every 105 seconds on the Redmi 8), whether the paired PC is reachable on its companion port (a plain TCP connect without TLS, PIN or data, private networks only; the PC briefly uses one connection slot for it). If the PC is reachable, automatic mode opens the display even when mDNS stays silent. Android throttles such alarms in idle mode (Doze), so opening may take noticeably longer than a minute. With the screen off, mDNS alone is not treated as proof that the PC is present.

1. Pair as described below and verify the manual display first. Keep the Companion service enabled in ScreenBuddy.
2. In Android setup, under **Automatic mode**, enable opening the display when ScreenBuddy starts. A standby service then searches for the paired PC; Android displays a service notification.
3. On Android 13 or later, allow notifications. **Allow display over other apps** permits opening the display from the background. Without that grant, the app offers an ordinary notification to tap; it does not request full-screen notification access. Background launches and waking the screen depend on Android version and manufacturer restrictions.
4. Phones only: **Allow screen lock** opens Android's device-administrator grant. It is used only for locking. Without that grant the display can close, but the app does not lock the screen. Since 1.1.1 the app also locks only when automatic mode opened the display itself and that display is still in the foreground when it closes; if only the notification was offered, the display has been left in the meantime, or a manually opened display was already visible when the PC appeared, the device stays unlocked. After a device-administrator lock, Android may require a PIN, pattern or password instead of biometrics for the next unlock. Android TV does not offer this grant; the app sends no explicit TV-off, standby or HDMI-CEC commands. Depending on the device, waking it may still trigger HDMI-CEC through the Android system.

After the PC is detected as absent, a 20-second grace period applies. Since 1.1.1 a missing mDNS announcement or a failed mDNS discovery alone no longer counts as absence: the app then checks by TCP immediately and repeats a failed check after a few seconds. The grace period starts only after two consecutive failed checks; a PC that is still reachable keeps its display. If the announcement returns or the PC is reachable again during the grace period, closing is cancelled. Automatic mode restarts a failed mDNS discovery at growing intervals (5 seconds up to at most 5 minutes). This is not an exact timer from quitting the Windows app: a network interruption can have the same effect and Android may delay detection. An existing PIN/biometric lock is not removed.

In Android system settings, the app is named **FuchsLabs Display**. Search under that name for notifications, “Display over other apps”, device administrators and battery optimization, even though this guide calls it the Companion app. If automatic mode stops working in the background, check the device-specific status under **Battery optimization → All apps → FuchsLabs Display**. Labels vary by Android version; on the Redmi, you may need to scroll down the app's settings view to see the options. If Android already says the app requires unrestricted battery use and the allowed-background-use option is greyed out, no additional battery exception is needed. This status alone does not explain a discovery failure.

Turn the setup switch off to stop automatic mode. An already granted device-administrator permission remains an Android system setting; revoke it under device administrators, including before uninstalling where required. After an Android restart, enabled automatic mode attempts to resume standby. Android battery saving, force-stop and manufacturer restrictions can prevent it; open the app manually and check system permissions if necessary. Manual use remains available without automatic mode.

**Automated validation, 8 November 2026:** 40 JVM protocol tests passed (including the TCP reachability-check logic); debug and signed release APKs built. **Device test, Redmi 8 (Android 14), 9 November 2026, screen off:** with ScreenBuddy closed on the PC, the display closed after two failed checks and the 20-second grace period, about 2.5 minutes after the screen was switched off. After restarting ScreenBuddy, the display opened and the screen turned on 74 seconds after the PC started. One run each, not repeated. Not yet tested: Pixel (not reachable via adb), Android TV with a real PC start/stop, repeats and other devices. The APK signature was verified and the signing certificate matches the published Companion app 1.0.3. Debug and release lint: 37 warnings, 0 errors. Build and signature verification do not establish device acceptance.

**Validation of 1.1.1, 14 November 2026 (automated only):** 56 JVM tests of the protocol module passed (new among them: hand-over of only the latest frame, TCP confirmation after mDNS loss, discovery retry intervals) and the Kotlin debug build compiles. No APK was signed or installed on a device; the fixes have not yet been accepted on Redmi, Pixel or TV. The device results above apply to 1.1.0: the time until closing measured there may be shorter in 1.1.1 because a failed check is repeated after a few seconds instead of at the next alarm.

**Candidate validation limits:** Real permission dialogs, waking from a locked state, Android boot startup, PC start/quit, network interruptions/reconnection and physical touch/remote operation on Redmi, Pixel and TV still require acceptance for 1.1.0. Earlier results below apply to their named versions.

**Status 31 October 2026:** The current Companion app 1.0.3 (versionCode 7; new light and dark login design; since 1.0.1 it keeps clear of rounded corners and the camera cut-out) is a separate download and not part of the Windows EXE. The Redmi 8 acceptance over USB tethering on 27 October 2026 (landscape/portrait output, real finger gestures, automatic page rotation and reconnect without a new PIN) refers to the predecessor 0.2.1-preview (versionCode 3, signed with a new RSA-3072 release key, APK v2 signature verified). For Companion app 1.0.3 the interface was installed on a Pixel 10 Pro, Redmi 8 and BeyondTV on 31 October 2026 and checked by screenshot; a full connection test is not documented in the [test record](../cpp/TESTPROTOKOLL.md). The previous debug APK uses a different certificate and cannot be directly updated with the new APK. Do not uninstall automatically: local data and pairings would be removed. [Build, signing and migration](../android/README.md). Earlier device results below apply to 0.2.0-preview.

The USB display or HDMI monitor and Android Companion can run at the same time, with separate page selections, positions, timers and touch targets. Data sources, accounts and theme stay shared. Enable and configure each data source in its tab before selecting its page for an output.

While Companion setup or its full-screen display is visible, the screen stays on and the Android TV screensaver is suppressed. After leaving the app, normal TV settings apply again. The app does not change global screensaver or power settings.

On Android TV the app appears in the TV launcher. With the remote, left/right switches the page, OK has no function, and Back (twice) leaves the display. Without a camera the QR scan is omitted; enter address, port and PIN manually. **Test status:** BeyondTV (Android 11) on 31 October 2026 with ADB key input; physical remote, standby/CEC and long-term use are untested.

1. Keep the existing display type. Enable **Companion**, then choose **Save & apply**.
2. Install the Companion app 1.0.3 on Android (separate download). The phone and PC must reach each other on a private IPv4 network; USB tethering also works.
3. Under **Companion → Pair a device**, show the local code. Scan its QR code or enter address, port and PIN in the app and compare the certificate fingerprint. Keep pairing codes private.
4. Under **Companion → Pages for Companion**, choose its pages and set the **Page interval** under **Companion output** at the top (3–600 seconds). The arrows affect only the phone view. Page order follows the saved selection; checking a page again adds it to the end.
5. Under **Pages & order → Pages on USB display / monitor**, choose the other output's pages. These extra controls appear only while Companion is enabled or selected as the display type. With Companion disabled, existing page behavior is preserved.

The phone reports its actual view dimensions on connection and rotation. The PC renders at that resolution, including Redmi landscape at 1520 × 720, without hard-coded device dimensions. Memory limits: 2560 pixels per edge and 4,194,304 pixels; larger requests are reduced proportionally. With `[companion] sync = true` (default) the first connected device reporting a size determines the shared Companion resolution; other phones share its pages and preserve aspect ratio when scaling. With `sync = false` every phone works independently (see below). After it disconnects, a size report from a remaining or newly connected device can take ownership.

Swipe left for the next page or right for the previous page. Taps use the corresponding output's targets; stale coordinates are dropped while dimensions change. Wide views may show several pages side by side. An empty selection shows the welcome screen.

```toml
[display]
output = "serial"
visible_pages = ["system", "friends"]
page_seconds = 30

[companion]
enabled = true
port = 8777
pages = ["rss", "system"]
page_seconds = 5
```

Phone only: `[display] output = "network"`; pages and timing also come from `[companion]`. Companion remains off by default. It may not use port 8765 or the current settings port. Pairings and keys stay local and are excluded from the repository.

Validation: see [test status](../cpp/TESTPROTOKOLL.md). The predecessor 0.2.1-preview was installed and tested on a real Redmi 8 over USB tethering: native landscape/portrait output, Android-injected swipes/tap, automatic page rotation and key-based reconnect passed. The user confirmed a sharp image, slow/normal finger swipes and taps. Physical USB output in the same new PC instance, Wi-Fi, launcher/boot and multiple-device behavior remain untested.

## Several phones at once

Up to 8 devices can be paired and several connected at the same time. The switch **Settings → Companion → Show the same on all devices** (`[companion] sync`, default on, applies after saving and restarting) decides the behaviour:

- **Synchronised (on):** all phones show the same picture and switch together (as before).
- **Independent (off):** every phone has its own page position, resolution (landscape/portrait), swipe and tap handling and switches by itself after `page_seconds`. Touch targets and toggles apply to the device that was tapped. Each device can also get **its own page list** (Settings → Companion → “Pages for the selected device”; saved immediately). An own list is binding: pages that are missing are not shown on that device. “Use shared list” restores the list under “Pages for Companion”. Each device’s position is part of the status (`/api/companion/status`, `clients[].navigation`).

In independent mode the preview and “Show previous/next page” in the settings refer to the first connected device and to all connected devices respectively. The page lists live in `companion.json` (DPAPI-protected like the pairings). The protocol is unchanged; older app versions work without changes. **Test status:** automated checks with simulated devices (server tests and an isolated app run with two test clients); Pixel 10 Pro and Redmi 8 were checked on 30 October 2026 with different device page lists, injected swipes and automatic switching; see the [2.7.0 release notes](RELEASE-NOTES-2.7.md) for the full limits.

## Device profile: keepout and Android TV (Phase 4a, development)

Partial patches such as `{"companion":{"keepout":{"top":5}}}` preserve unspecified edges. Shared mode uses only `companion.keepout`; the device card explains that individual edges have no effect in that mode. Differently sized image content is clipped to the usable area as well.

Android TV stays in landscape (`SENSOR_LANDSCAPE`). Orientation selection is hidden there; any previously stored choice is ignored and retained. Phones keep their Landscape/Portrait/Automatic behaviour. TV has no touch actions; D-pad left/right navigates as before, while CENTER/ENTER remains without action.

The PC Companion settings offer four shared default edges (`companion.keepout`) and an individual keepout for the selected paired device. Individual edges are saved immediately through the Companion store, apply only in independent mode (`companion.sync = false`), and can be reset to the shared default. A missing field in older pairings means no individual override; all edges default to 0. Phone camera cutouts remain client-side as well.

Top/right/bottom/left use the reported full frame size without rotation. Integer pixels 0–1000 are accepted; rendering limits each edge to one third of the smaller frame dimension. Below 160 × 120 usable pixels, the entire keepout is ignored (warning at most once per view/minute). Content is drawn at usable size and placed in a full frame with the theme background; previews include this border. `ready.w/h` and touch positions remain full-frame coordinates (unchanged protocol). The PC shifts targets per device; border taps cause neither actions nor navigation, and stale frame sizes are rejected.

Usage: [navigation](NAVIGATION.en.md#device-profile-and-keepout-phase-4a-development). Validation: [test record](../cpp/TESTPROTOKOLL.md); physical TV/enclosure acceptance remains open.

## Plugin taps (Phase 4c, development)

Schema 2 plugins can offer HTTPS links and the next cycling-zone entry. Links open on the PC, including taps from a phone. The separate grant in Settings → Plugins defaults to off and displays the link hosts. Targets require 1.0 s stability on Companion or 0.3 s on a monitor and are blocked during transitions; denied taps do not navigate. Areas without a target retain thirds navigation. Android TV gains no tap/D-pad action. See the [plugin system](PLUGIN-SYSTEM.en.md#tap-targets-for-plugin-blocks-phase-4c-development-for-2100). Physical acceptance is pending.
