# Drittanbieter-Hinweise / Third-Party Notices

ScreenBuddy (früher FuchsLabs USB-Display) enthält oder nutzt die folgenden Komponenten. Deren Lizenzen bleiben unverändert bestehen und gelten unabhängig von der Lizenz des FuchsLabs-eigenen Codes.
This product includes or uses the components below. Their licenses remain unchanged and apply independently of the license of FuchsLabs' own code.

Stand: 2.5.0-dev.2 (C++), Python 1.2 (historisch). Die Versionen der vcpkg-Pakete ergeben sich aus der Baseline in `cpp/vcpkg.json`; maßgeblich sind die `copyright`-Dateien im vcpkg-Installationsordner (`share/<paket>/copyright`) des jeweiligen Builds.

## C++-Version 2.0 (in der EXE statisch gelinkt)

| Komponente | Lizenz (SPDX) | Copyright-Inhaber | Quelle |
|---|---|---|---|
| D++ (dpp) 10.1.6 | Apache-2.0 | Brainboxdotcc und Mitwirkende | https://dpp.dev |
| nlohmann/json | MIT | Niels Lohmann | https://github.com/nlohmann/json |
| toml++ | MIT | Mark Gillard | https://github.com/marzer/tomlplusplus |
| pugixml | MIT | Arseny Kapoulkine | https://pugixml.org |
| cpp-httplib | MIT | yhirose | https://github.com/yhirose/cpp-httplib |
| OpenSSL (Abhängigkeit von D++; auch TLS der Companion-App) | Apache-2.0 | The OpenSSL Project Authors | https://www.openssl.org |
| Opus (Abhängigkeit von D++) | BSD-3-Clause | Xiph.Org Foundation, Skype Limited u. a. | https://opus-codec.org |
| zlib (Abhängigkeit von D++) | Zlib | Jean-loup Gailly, Mark Adler | https://zlib.net |
| QR Code generator library (C++) 1.8.0 | MIT | Project Nayuki | https://www.nayuki.io/page/qr-code-generator-library · `cpp/third_party/qrcodegen` |
| Microsoft Edge WebView2 SDK 1.0.4258.31 (Header, statischer Loader) | BSD-3-Clause | Microsoft Corporation | `cpp/third_party/webview2/LICENSE.txt`, `NOTICE.txt` |

Die WebView2-Laufzeit selbst wird nicht mitgeliefert (Windows-Voraussetzung). Windows-Komponenten (Direct2D, DirectWrite, WIC, WinHTTP, DPAPI, DNS-SD/mDNS) sind Teil von Windows.

Schriften: Die App bündelt keine Schriften. Sie verwendet die auf dem PC vorhandenen Windows-Schriften Bahnschrift und Segoe UI (Microsoft).

## Python-Version 1.2 (historisch, im Paket 1.x)

| Komponente | Lizenz |
|---|---|
| discord.py | MIT |
| aiohttp und Abhängigkeiten (aiosignal, frozenlist, multidict, yarl, propcache, aiohappyeyeballs, attrs, idna) | Apache-2.0 / MIT / BSD-3-Clause (je Paket) |
| pyserial | BSD-3-Clause |
| Pillow | MIT-CMU (HPND) |
| PyInstaller (nur Build) | GPL-2.0-or-later mit Bootloader-Ausnahme; die erzeugte EXE darf beliebig lizenziert werden |

Wer ein 1.x-Paket weitergibt, legt die Lizenztexte der enthaltenen Pakete bei (sie liegen in den `*.dist-info`-Ordnern der Pakete).

## MIT-Lizenztext (gilt für die oben als MIT genannten Komponenten)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Apache-2.0-Volltext: https://www.apache.org/licenses/LICENSE-2.0.txt · BSD-3-Clause und Zlib: siehe `share/<paket>/copyright` bzw. `cpp/third_party/webview2/NOTICE.txt`.

## Dienste und Daten (keine Software, aber Nutzungsbedingungen)

ScreenBuddy ruft nur auf Wunsch des Nutzers und mit dessen eigenen Zugangsdaten folgende Dienste ab. Es werden keine Inhalte dieser Dienste im Repository oder im Installer verteilt; abgerufene Daten werden nur lokal angezeigt.

- Discord (Bot-Token des Nutzers) – Discord Developer Terms / Policy
- Steam Web API (API-Schlüssel des Nutzers) – Steam Web API Terms of Use
- OpenXBL / xbl.io (Schlüssel des Nutzers) – Bedingungen von OpenXBL
- gametools.network (Battlefield-6-Statistik, inkl. Rang-Icons von EA/DICE) – Bedingungen des Betreibers
- TMDB API v3 (api.themoviedb.org, Key des Nutzers) für die Seite „Streaming-Neustarts“ – TMDB API Terms of Use. **This product uses the TMDB API but is not endorsed or certified by TMDB.** Anbieterdaten (Watch Providers): **JustWatch**. Es werden Titel, Datum und die Anbieter-Logos angezeigt (Bilder von `image.tmdb.org`, geliefert von TMDB/JustWatch; einmal geladen und lokal unter `cache/tmdb-logos/` zwischengespeichert, Rückfall: Kürzel). Die Anbieter-Logos sind Marken bzw. geschützte Zeichen der jeweiligen Anbieter und werden nur zur Kennzeichnung des Dienstes gezeigt. Das offizielle TMDB-Logo ist noch nicht eingebunden (Textnachweis)
- RSS-/Atom-Feeds, die der Nutzer einträgt (z. B. heise.de) – Rechte liegen bei den jeweiligen Verlagen; es werden nur Titel angezeigt

## Marken

TMDB, JustWatch, Netflix, Prime Video, Disney+, Apple TV+, WOW, RTL+, Battlefield, EA, Discord, Steam, Xbox, Windows, Segoe und Bahnschrift sind Marken bzw. geschützte Namen ihrer jeweiligen Inhaber. FuchsLabs steht in keiner Verbindung zu ihnen und wird von ihnen nicht unterstützt. Name und Fuchs-Logo „FuchsLabs“ sind von einer Open-Source-Lizenz des Codes ausgenommen.

## TMDB-Logo / TMDB logo

Das offizielle TMDB-Logo (docs/logo/tmdb.svg) stammt von https://www.themoviedb.org/about/logos-attribution und wird gemäß den TMDB-Attributionsregeln nur zur Quellenangabe verwendet. This product uses the TMDB API but is not endorsed or certified by TMDB.

