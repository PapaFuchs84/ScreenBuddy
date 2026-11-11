# ScreenBuddy help and support

ScreenBuddy displays PC information on supported USB panels, an additional Windows monitor, or Android devices with the separately installed Companion app. Windows and Companion releases have separate version numbers.

## Getting started

- [Guide and source code](https://github.com/PapaFuchs84/ScreenBuddy)
- [Downloads and release notes](https://github.com/PapaFuchs84/ScreenBuddy/releases)
- [Report an issue](https://github.com/PapaFuchs84/ScreenBuddy/issues)
- Email: [fuchslabs2026@gmail.com](mailto:fuchslabs2026@gmail.com)
- [App privacy](https://fuchslabs.duckdns.org/screenbuddy/privacy/)

## Common questions

**Which Windows version is required?** The Store package targets x64 Windows build 19041 or newer. An additional monitor or compatible USB panel is needed only for that output type. Not every USB display is supported. Android output requires the separate Companion app and connectivity to the PC, such as Wi-Fi or USB tethering; the APK is not included in the Windows MSIX.

**Where is my data?** The Store edition uses its package's `LocalState`. The standalone edition retains its separate data directory. Close both apps before switching. The documented explicit import copies selected data without overwriting destination files and preserves the source. Do not attach complete configuration files publicly.

**Updates and startup?** Microsoft delivers Store updates according to Windows/Store settings. Users enable startup in ScreenBuddy; Windows policies can prevent it. The standalone edition's Registry startup setting is not changed automatically.

**Blank USB panel?** Check compatibility, cable and selected COM port. Vendor software can occupy the port. The Store app can stop a running vendor process with your existing user privileges; manage its startup outside ScreenBuddy in Windows Task Scheduler, with administrator assistance if needed.

**Companion cannot connect?** Check the PC address, reachable port, network/tethering, firewall and correct pairing. Never publish pairing codes. After a device change, forget the previous PC connection and pair again if needed. Do not delete Windows or Android app data without considering existing pairings.

**FPS shows n/a?** Enable Game Mode and select a suitable view. ETW needs the relevant Windows permissions; “Check FPS permission” explains administrator-assisted membership of “Performance Log Users”, followed by signing out and back in. The Store app does not change group membership itself. Start ScreenBuddy before an anti-cheat game. Existing permissions or read-only HWiNFO PresentMon readings can be used; measurement sources are not always available.

**Third-party or AI values missing?** Check the enabled integration and its setup. Providers can require their own accounts/keys and change interfaces. AI account access is optional, separately confirmed and can be disabled. Missing values are `n/a`, not invented live measurements.

**Companion automation (from 1.1.0)?** Pair and check manual display first. Optional automation discovers the PC locally through mDNS or a TCP availability check. Android can delay background starts and alarms. Check notification permission and, if needed, “Display over other apps” in Android settings. Optional device-administrator screen locking is available only on supported phones; you may need to revoke it in Android before uninstalling. Do not broadly disable firewalls or screen locks. See the current [Companion guide](https://github.com/PapaFuchs84/ScreenBuddy/blob/main/docs/COMPANION.en.md).

## Send a useful report

Include Windows build, ScreenBuddy version, Store or standalone edition, output type/device model, exact steps, expected result and actual result. For Companion, also include its app version and Android version. Use sample data in images. Remove names, private URLs, tokens, passwords, pairing codes and personal content from logs and screenshots. GitHub issues are public; do not send private information there.

Support is offered as part of a private project without a guaranteed response time. For privacy requests use the contact address and [app privacy notice](https://fuchslabs.duckdns.org/screenbuddy/privacy/).
