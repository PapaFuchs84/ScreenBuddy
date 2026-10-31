# ScreenBuddy · Features in detail

Moved out of the README (as of 2.7.0). German: [FUNKTIONEN.md](FUNKTIONEN.md). Overview of all documents (German): [docs/README.md](README.md).

## Contents

- [System page touch and update notice (from 2.2.0)](#system-page-touch-and-update-notice-from-220)
- [Touch targets and playtime (from 2.0.7)](#touch-targets-and-playtime-from-207)
- [Home Assistant (from 2.1.0)](#home-assistant-from-210)
- [Android Companion app (from 2.3.0, multiple devices from 2.7.0)](#android-companion-app-from-230-multiple-devices-from-270)
- [Plugin store and plugins (from 2.6.0)](#plugin-store-and-plugins-from-260)
- [Browse views (from 2.0.4)](#browse-views-from-204)

## System page touch and update notice (from 2.2.0)

**Touch on the system page:** With touch active (screen output only), tapping the **RAM** card opens Task Manager, **Drive** opens “Programs and Features” (`appwiz.cpl`), and **Network** opens the speed test <https://www.speedtest.net/de> in the default browser. The CPU and GPU cards do not react. Windows opens the windows, usually on the main screen.

**Update notice:** At most once a day (at the earliest 45 seconds after start) the app queries the public GitHub releases, without a key or account. If a newer published version exists, a balloon appears at the fox icon (clicking it opens the settings) and **Settings → Start** shows “New version … available” with a link to the download page. **Check now** queries outside the schedule (at most every 5 minutes). Nothing is ever downloaded or installed; to update you download the new EXE yourself and start it. Turn it off under **Settings → Start → Notify about new versions** or with `update_check = false` in `[ui]`. Only an ordinary request with the program name and version in the user agent is sent; as with any request, GitHub sees your IP address. The time of the last check is stored in `update-check.json` next to `config.toml`.

## Touch targets and playtime (from 2.0.7)

With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Tapping a friend preferably opens the Steam chat with that person (Steam app required). Without a Steam account, their Discord profile opens (choose **Message** there), otherwise their Xbox profile on xbox.com in the browser. If the target's app is not installed, the next target is used. Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](TOUCH-2.0.7.en.md). [Validation](../cpp/TESTPROTOKOLL.md).

## Home Assistant (from 2.1.0)

**Home Assistant** page with up to 4 tiles. **Sensors** (including binary sensors) show value and unit. **Switches** (switch, light, fan, input boolean) show On/Off and can be flipped by tapping when touch is active. Without touch only sensors can be chosen in the settings. Set it up under **Settings → Home Assistant**: address (e.g. `http://homeassistant.local:8123`), a long-lived access token (profile in Home Assistant → Security) and the entities from the list that **Check connection** loads straight from Home Assistant. Like the other keys, the token is stored DPAPI-encrypted in `config.toml`. The app polls the REST API every 15 seconds; tapping a switch takes effect immediately. Locks, covers and alarm panels are deliberately not switchable. With an `http://` address the token and values cross your home network unencrypted; use `https://` if Home Assistant offers it.

## Android Companion app (from 2.3.0, multiple devices from 2.7.0)

Use an Android phone or tablet as an additional display alongside your USB/HDMI screen. The Companion app is version 1.0.0 as a separate download. Since 2.7.0 several Companions can run at once, **synchronised or independent**.

- **Own pages and timers:** each device can have its own page list and interval (under **Companion**); data sources and theme remain shared.
- **Pairing via QR code or PIN:** click **Pair device** under the **Companion** tab. The Android app scans the QR code or connects using address, port (8777) and a 6-digit PIN; a SHA-256 certificate fingerprint secures the TLS connection.
- **Touch and gestures:** swipes navigate pages only on that device. Interactive tiles (smart home, friends, system actions) respond to finger taps.
- **Native resolution and rotation:** the phone reports orientation and resolution; the PC renders matching frames.
- [Setup and limits](COMPANION.en.md) · [Protocol](COMPANION-PROTOKOLL.md) (German)

Validation: see the [2.7 release notes](RELEASE-NOTES-2.7.md). The automated multi-device checks are simulations; adb gestures do not replace physical finger acceptance. Moving from the 0.2.1 preview to 1.0 requires uninstalling the old app because of a different signature.

## Plugin store and plugins (from 2.6.0)

Settings page **Plugin store** (next to Companion and Plugins, can be switched off with `[ui] plugin_store`): search, check and install reviewed data plugins from the signed store `PapaFuchs84/ScreenBuddy-Plugins`. Signed index and signed revocation list (Ed25519, pinned public key), packages checked by SHA-256 and host declarations, daily background sync. Before installation the card shows which hosts a plugin would send data to. Installed plugins stay off until you enable them under **Plugins**; every host needs your consent. Since 2.7.0 store installations and updates stay disabled even without network sources; direct local manifest import behaves as before. Enabled plugins can be sorted and shown/hidden under **Pages & order**. The store shows signed preview images.

More: [Plugin system](PLUGIN-SYSTEM.en.md) · [Tutorial](PLUGIN-TUTORIAL.en.md) · [Store terms](PLUGIN-STORE-TERMS.md) · [Examples](../cpp/examples/plugins/README.md). Online plugins and CRL/OCSP checks remain experimental.

## Browse views (from 2.0.4)

In **Settings → Overview**, use **Previous view** and **Next view** to cycle through the running display views directly. The position and visible pages appear below the live preview. Long lists include their subpages; wide displays advance by one tile group, as with touch navigation. Each manual change restarts the configured page duration, then automatic cycling continues. Buttons are disabled when there are no views or all views already fit on screen. No save or restart is needed. Changes to page selection and order take effect after **Save & apply**.
