## Companion app (from 2.3.0): independent displays

**Status 30 October 2026:** Companion app 1.0 is a separate download and not part of the Windows EXE. The Redmi 8 acceptance over USB tethering on 27 October 2026 (landscape/portrait output, real finger gestures, automatic page rotation and reconnect without a new PIN) refers to the predecessor 0.2.1-preview (versionCode 3, signed with a new RSA-3072 release key, APK v2 signature verified). No separate device test for 1.0 is documented in the [test record](../cpp/TESTPROTOKOLL.md). The previous debug APK uses a different certificate and cannot be directly updated with the new APK. Do not uninstall automatically: local data and pairings would be removed. [Build, signing and migration](../android/README.md). Earlier device results below apply to 0.2.0-preview.

The USB display or HDMI monitor and Android Companion can run at the same time, with separate page selections, positions, timers and touch targets. Data sources, accounts and theme stay shared. Enable and configure each data source in its tab before selecting its page for an output.

1. Keep the existing display type. Enable **Companion**, then choose **Save & apply**.
2. Install the Companion app 1.0 on Android (separate download). The phone and PC must reach each other on a private IPv4 network; USB tethering also works.
3. Under **Companion → Pair a device**, show the local code. Scan its QR code or enter address, port and PIN in the app and compare the certificate fingerprint. Keep pairing codes private.
4. Under **Companion → Independent Companion display**, choose its pages and timer (3–600 seconds). The arrows affect only the phone view. Page order follows the saved selection; checking a page again adds it to the end.
5. Under **Display → Pages on USB display / monitor**, choose the other output's pages. These extra controls appear only while Companion is enabled or selected as the display type. With Companion disabled, existing page behavior is preserved.

The phone reports its actual view dimensions on connection and rotation. The PC renders at that resolution, including Redmi landscape at 1520 × 720, without hard-coded device dimensions. Memory limits: 2560 pixels per edge and 4,194,304 pixels; larger requests are reduced proportionally. The first connected device reporting a size determines the shared Companion resolution. Other phones share its pages and preserve aspect ratio when scaling. After it disconnects, a size report from a remaining or newly connected device can take ownership.

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
