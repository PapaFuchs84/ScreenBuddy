# ScreenBuddy · Features in detail

Moved out of the README (status 2.9.3). German: [FUNKTIONEN.md](FUNKTIONEN.md). Overview of all documents (German): [docs/README.md](README.md).

## Contents

- [System page touch and update notice (from 2.2.0)](#system-page-touch-and-update-notice-from-220)
- [Touch targets and playtime (from 2.0.8)](#touch-targets-and-playtime-from-208)
- [Home Assistant (from 2.1.0)](#home-assistant-from-210)
- [New on streaming (from 2.8.0)](#new-on-streaming-from-280)
- [Spotify (from 2.8.0)](#spotify-from-280)
- [Help for API keys and tokens (since 2.9.1)](#help-for-api-keys-and-tokens-since-291)
- [PlayStation friends (from 2.9.3)](#playstation-friends-from-293)
- [Colors & design (from 2.9.0)](#colors--design-from-290)
- [Android Companion app (from 2.3.0, multiple devices from 2.7.0)](#android-companion-app-from-230-multiple-devices-from-270)
- [Plugin store and plugins (from 2.6.0)](#plugin-store-and-plugins-from-260)
- [Browse views (from 2.0.4)](#browse-views-from-204)

## System page touch and update notice (from 2.2.0)

**Touch on the system page:** With touch active (screen output only), tapping the **RAM** card opens Task Manager, **Drive** opens “Programs and Features” (`appwiz.cpl`), and **Network** opens the speed test <https://www.speedtest.net/de> in the default browser. The CPU and GPU cards do not react. Windows opens the windows, usually on the main screen.

**Update notice:** At most once a day (at the earliest 45 seconds after start) the app queries the public GitHub releases, without a key or account. If a newer published version exists, a balloon appears at the fox icon (clicking it opens the settings) and **Settings → Updates & startup** shows “New version available: …” with **Open release page** for the download page. **Check for updates now** queries outside the schedule (at most every 5 minutes). Nothing is ever downloaded or installed; to update you download the new EXE yourself and start it. Turn it off under **Settings → Updates & startup → Automatically check for updates** or with `update_check = false` in `[ui]`. Only an ordinary request with the program name and version in the user agent is sent; as with any request, GitHub sees your IP address. The time of the last check is stored in `update-check.json` next to `config.toml`.

## Touch targets and playtime (from 2.0.8)

Developed as 2.0.7; that version was not released separately, so the feature is available from 2.0.8. With touch enabled, tapping a BF6 player opens their GameTools profile in the browser. The leaderboard also shows hours played. Tapping a friend preferably opens the Steam chat with that person (Steam app required). Without a Steam account, their Discord profile opens (choose **Message** there), otherwise their Xbox profile on xbox.com or (from 2.9.3) their PlayStation profile on my.playstation.com in the browser. If the target's app is not installed, the next target is used. Windows and tabs are controlled by the platform app/browser. [Usage, prerequisites and limits](TOUCH-2.0.7.en.md). [Validation](../cpp/TESTPROTOKOLL.md).

## Home Assistant (from 2.1.0)

**Home Assistant** page with up to 4 tiles. **Sensors** (including binary sensors) show value and unit. **Switches** (switch, light, fan, input boolean) show On/Off and can be flipped by tapping when touch is active. Without touch only sensors can be chosen in the settings. Set it up under **Settings → Home Assistant**: address (e.g. `http://homeassistant.local:8123`), a long-lived access token (profile in Home Assistant → Security) and the entities from the list that **Check connection** loads straight from Home Assistant. Like the other keys, the token is stored DPAPI-encrypted in `config.toml`. The app polls the REST API every 15 seconds; tapping a switch takes effect immediately. Locks, covers and alarm panels are deliberately not switchable. With an `http://` address the token and values cross your home network unencrypted; use `https://` if Home Assistant offers it.

## New on streaming (from 2.8.0)

The **New on streaming** page shows movies and series released in the last 30 days that are included in the subscription (flatrate) of the providers you choose. Each row has the title, date, movie or series, and the provider logo (at most 3 per title, further ones as “+n”). Up to 6 rows, newest first. If a logo is missing or still loading, the abbreviation (e.g. NF, PV, D+, ATV+, WOW, RTL+) is shown instead.

- **Setup:** under **Settings → New on streaming** turn the page on and enter a free TMDB key (on an existing installation also tick the page under *Pages & order*) (themoviedb.org → Settings → API; the v3 key or the “API Read Access Token” both work). Like the other keys it is stored DPAPI-encrypted in `config.toml` and never logged or shown. **Check key & load providers** loads your region's provider list from TMDB; tick the providers you want (up to 30). Preselected: Netflix (8), Prime Video (9 and 119), Disney+ (337), Apple TV+ (350), WOW (30) and RTL+ (2750, region DE). If an ID differs in your region, fix it via the loaded list.
- **Region:** follows the app language (German: DE, English: US); can be overridden with two letters in the **Region** field (`region` under `[streaming]`), e.g. `AT`.
- **Requests:** GET only, over HTTPS to `api.themoviedb.org/3`. Each refresh makes two discover requests (movies, series) plus – only for the shown entries, remembered once – one provider request per entry; at most hourly (1 hour cache), with short pauses between requests. On errors the old entries stay visible and the app retries after 10 minutes (15 minutes on “rate limit”, next hour on an invalid key).
- **Provider logos:** TMDB supplies a `logo_path` per provider; the image comes from `image.tmdb.org` (size w92, GET only over HTTPS, no redirects, size limit, PNG/JPEG only). Each logo is loaded once in the background and stored under `cache/tmdb-logos/<provider ID>.png` in the data folder (file name from the number only), then scaled in memory to row height as a rounded square. The display never waits for it; until then, or on errors, the abbreviation is shown. The logos are trademarks of the providers ([THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md)). TMDB additionally sees the image requests.
- **Display without data:** “No key” (page on, but no key), “Loading data …”, “No data” (error or offline, never a crash).
- **Privacy:** TMDB sees your IP address and the chosen region/provider list, not your accounts at the streaming services.
- **Credit:** This product uses the TMDB API but is not endorsed or certified by TMDB. Provider data: JustWatch. The page carries the footer “TMDB / JustWatch”; the notice is also in the settings (Updates & startup → Credits) and in [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md). The official TMDB logo appears in the attribution of the README and THIRD-PARTY-NOTICES.md (from 2.9.0).

## Spotify (from 2.8.0)

Shows the current track with cover and progress; with touch (monitor, Companion) buttons control previous, play/pause and next (Spotify Premium required). Sign-in uses PKCE with your own Spotify developer app; the token is stored DPAPI-encrypted in `spotify.token`. While music plays, an amplifier (16 bars) shows the Windows system output level, read locally and not recorded (can be switched off). If music plays elsewhere (phone, speaker, Connect) and the system output stays silent for 4 s, it shows an animation labelled “simulated”. Bar colour is selectable (level green/yellow/red, theme gradient, single colour). A change of the default audio device is detected. Setup and limits: [SPOTIFY.en.md](SPOTIFY.en.md). From 2.9.0 the analysis only starts when the visible playback page is drawn and ends at most three seconds after leaving it. From 2.9.1 remote playback supports the simulated display.

## Help for API keys and tokens (since 2.9.1)

Wherever ScreenBuddy needs credentials, the settings show a card **“How to get …”** under the field with short numbered steps and buttons that open the service’s official page in your browser: Discord (bot token), Steam (web API key and privacy), Xbox (OpenXBL), TMDB (API key), Spotify (client ID) and Home Assistant (long-lived access token). Only fixed https addresses of the providers are opened (`cpp/src/key_help.cpp`). The app never reads, logs or repeats keys; pasted values are stored encrypted as before. Battlefield 6 and blogs need no key. For the optional PlayStation code there is the card **“How to get the PlayStation code (NPSSO)”** with the steps and two buttons (playstation.com and Sony’s code page after sign-in); details in [PlayStation friends](#playstation-friends-from-293).

## PlayStation friends (from 2.9.3)

**Unofficial, optional, off by default, at your own risk.** Sony offers no official interface for friends lists. ScreenBuddy therefore takes the same route as the PlayStation app: the NPSSO code of the signed-in browser session is exchanged for a short-lived access token (the publicly known sign-in identifiers of Sony’s Android app are built in for this). Sony may change or block this interface at any time and may restrict the account; use is your responsibility.

- **Setup:** **Settings → Friends**, **PlayStation** card. The help **“How to get the PlayStation code (NPSSO)”** leads to the code in a few steps: sign in at playstation.com, open Sony’s code page in the same browser, paste the code (or the whole line) into **PlayStation code (NPSSO)**, **Check connection**, then **Save & apply**. Without a saved code PlayStation is off and no requests go to Sony.
- **Code:** 64 characters, works like a password and expires after about two months; just fetch a new one. Like the other keys it is stored DPAPI-encrypted in `config.toml`, never logged, never sent to the browser and sent only to Sony. The whole answer of the code page (`{"npsso":"…"}`) is accepted too; Sony’s authorisation code with a dot (`v3.…`) is accepted during sign-in. Connection diagnostics go to the log (without the code).
- **Display:** PlayStation friends appear in the combined friends list with the badge **P**, online status and current game. Refresh every 2 minutes, at most 200 friends; avatars are loaded only from Sony domains over HTTPS. The same person on several platforms appears once; for people with different names use **Merge people** or add `Name|…|psn:OnlineID` to the link lines.
- **Touch:** tapping a friend prefers the Steam chat, then the Discord and Xbox profiles; if the person only has PlayStation, their profile on my.playstation.com opens in the browser.
- **Validation:** parsing of Sony’s answers is covered by automated tests with fixed sample data (`fl-psn-tests`). Live against Sony: with a fresh code the friends list returned one PlayStation account in 2.9.3 without errors (see the [test record](../cpp/TESTPROTOKOLL.md)); online status and game were not checked on the devices.

## Colors & design (from 2.9.0)

**Settings → Colors & design:** Light (FuchsLabs), Dark (FuchsLabs), **Match Windows** or your own themes. Windows mode: light/dark and accent from Windows, changes apply live, high contrast uses the system colours; text and bars are contrast-corrected automatically. Custom themes are JSON files (editor with live preview, contrast warning, duplicate, delete, import, export); six examples (Cupertino, Material and Fluent style, each light and dark) are in `cpp/examples/themes`. From 2.9.1 themes can be installed through the “Themes” card in the plugin store; from 2.9.2 there are also design systems with their own window compositions. The choice applies to the USB display, monitor and Companion. Details, format and limits: [THEMES.md](THEMES.md) (bilingual).

**Status dots in the navigation:** green = set up and on, grey = not set up or off; only for data sources and connections, with a legend. Since 2.9.1 a tooltip explains the state when you rest the mouse on a dot.

## Android Companion app (from 2.3.0, multiple devices from 2.7.0)

Use an Android phone or tablet as an additional display alongside your USB/HDMI screen. From 2.3.0 the Companion was available as a preview APK; the current Companion app 1.0.2 (versionCode 6, Android 8+) is a separate download. Since 2.7.0 several Companions can run at once, **synchronised or independent**.

- **Own pages and timers:** compared with USB/HDMI, the Companion has its own page list and interval (under **Companion**). In independent mode (`sync = false`) each device additionally has its own page position, resolution, input and optionally its own page list. Data sources and theme remain shared. The default is synchronised display.
- **Pairing via QR code or PIN:** show the pairing code under **Settings → Companion → Pair device**. The Android app scans the QR code or connects using address, port (default 8777) and a 6-digit PIN; a SHA-256 certificate fingerprint secures the TLS connection.
- **Touch and gestures:** in independent mode a swipe navigates only that device; in synchronised mode it navigates all. Taps and swipes are described in the [Companion notes](COMPANION.en.md).
- **Native resolution and rotation:** the phone reports orientation and resolution; the PC renders matching frames.
- [Setup and limits](COMPANION.en.md) · [Protocol](COMPANION-PROTOKOLL.md) (German)

Validation: see the [2.7 release notes](RELEASE-NOTES-2.7.md). The automated multi-device checks are simulations; adb gestures do not replace physical finger acceptance. When switching from a debug APK the signature differs and a direct update is not possible (see the [2.6 release notes](RELEASE-NOTES-2.6.md)). Never uninstall the Companion app automatically: local data and pairings would be lost.

## Plugin store and plugins (from 2.6.0)

Settings page **Plugin store** (next to Companion and Plugins, can be switched off with `[ui] plugin_store`): search, check and install reviewed data plugins from the signed store `PapaFuchs84/ScreenBuddy-Plugins`. Signed index and signed revocation list (Ed25519, pinned public key), packages checked by SHA-256 and host declarations, daily background sync. Before installation the card shows which hosts a plugin would send data to. Installed plugins stay off until you enable them under **Plugins**; every host needs your consent. Since 2.7.0 store installations and updates stay disabled even without network sources; direct local manifest import behaves as before. Enabled plugins can be sorted and shown/hidden under **Pages & order**. The store shows signed preview images.

More: [Plugin system](PLUGIN-SYSTEM.en.md) · [Tutorial](PLUGIN-TUTORIAL.en.md) · [Store terms](PLUGIN-STORE-TERMS.md) · [Examples](../cpp/examples/plugins/README.md). Online plugins and CRL/OCSP checks remain experimental.

## Browse views (from 2.0.4)

Since 2.0.4 the running display can be advanced with a button press, without saving or restarting. In the native interface (from 2.5.0) **Show previous page** and **Show next page** under **Settings → Companion** do this; they act on the Companion output (on all connected devices in independent mode). For the main display the step is available only through the local interface (`POST /api/pages/step`). The former “Previous view”/“Next view” buttons under **Overview** do not exist in the native interface. Changes to page selection and order take effect after **Save & apply**.
