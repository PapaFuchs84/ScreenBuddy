# Microsoft Store form preparation

Prepared 10 November 2026 from the current Windows source. These are reviewable draft entries, not submitted answers or an issued rating. The actual Partner Center controls and IARC questions must be read when account access is available.

## Product and properties

| Field | Prepared value / reason |
| --- | --- |
| Product type | MSIX app, not Game. Game Mode displays information about other games; ScreenBuddy itself is a desktop information utility. |
| Name | ScreenBuddy by FuchsLabs, reserved after the owner selected this alternative because ScreenBuddy was unavailable. Store ID 9P03461RLGG0; exact official identity in STORE-LISTING.md. |
| Primary category | Utilities + tools, verified in the completed Properties page. Subcategory left blank; Backup/manage would misdescribe this app. A secondary category is optional. |
| Price | Proposed Free, matching the freely downloadable project; confirm the actual live selection with the owner before submitting. No subscription, in-app digital-goods purchase or paid random-item mechanism is implemented in the reviewed Windows sources. |
| Architecture / OS | x64, Windows Desktop; package minimum 10.0.19041.0. No Xbox/HoloLens/mobile package or ARM-native claim. |
| Languages | German (de-DE) and English (en-US). DE/EN copy and sample assets accompany this preparation. |
| Privacy: accesses/transmits personal information | Yes. Optional integrations handle account/player/presence data, credentials, device pairing, local session status and provider requests. A blanket “no collection / no network” answer is incorrect. |
| Privacy URL | https://fuchslabs.duckdns.org/screenbuddy/privacy/ — use only after public deployment and HTTP/content verification. |
| Support URL | https://fuchslabs.duckdns.org/screenbuddy/support/ — use only after public deployment and HTTP/content verification. |
| Support email | fuchslabs2026@gmail.com (existing public FuchsLabs contact verified by the owner agent). |
| Website | https://fuchslabs.duckdns.org/ |
| Hardware | Keyboard/mouse and a Windows desktop suffice to configure/review. A compatible USB panel, extra monitor or Android Companion is needed only for the selected output. Do not mark a camera, GPS, VR headset or touchscreen as required for all Windows users. The optional QR camera belongs to the separate Android app. |
| Accounts | No FuchsLabs account needed. Optional services require their own accounts/credentials; reviewers can test settings, sample layouts and local monitor output without them. |
| Mixed reality | No immersive/MR declaration. |
| Generative AI | No model prompt or content-generation feature. The AI Status feature displays local client status and optional account-usage values; do not claim it generates text/images. |
| Package capability | runFullTrust, justified in CERTIFICATION-NOTES.md. No allowElevation. |

Source anchors: AppxManifest.xml.in; cpp/src/config.cpp; ui_page_game_mode.cpp; game_detect.cpp; ai_status.cpp; ai_usage.cpp; friends.cpp; discord.cpp; spotify.cpp; tmdb.cpp; rss.cpp; ha.cpp; distribution.cpp; perflog.cpp; winsetup.cpp. Exact portal product/publisher values are recorded in STORE-LISTING.md; never substitute guessed values.

## IARC: evidence prepared for the live questionnaire

The [IARC FAQ](https://globalratings.iarcservices.com/faq/) states that its questionnaire is available only through participating storefronts. [Microsoft's MSIX guidance](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/age-ratings) explains that questions depend on app category, can change, and that publisher display name/email are shared with IARC. Therefore no offline answer sheet can honestly claim a final PEGI/ESRB/USK age or issued IARC certificate.

| Topic if asked | Current factual basis |
| --- | --- |
| App category | Utility / other non-game application. It is not a game, social network, full web browser or search engine. Map to the matching current IARC option after reading its wording. |
| Bundled violence, sex/nudity, drugs, offensive language, horror, gambling | No such authored content or game mechanics are intentionally bundled in the utility's reviewed source and sample assets. Answer the downloaded-app/bundled-content questions for the actual final package; do not extend this automatically to online content. |
| Online content | Present as optional integrations: user-selected RSS, music metadata/art, streaming titles/art, game/friend presence, game logos and plugin content. These can contain third-party names, images or text. Do not select “no online content” merely because the app is a utility. |
| User content / interaction | Discord/friends display third-party names, avatars and presence; RSS and plugins can display remote text/images. The app does not itself implement a public posting, chat composer or content-sharing network. The exact IARC definition of interaction/UGC must be compared with this display-only behavior in the live question. |
| Unrestricted web access | App links open the default browser; the app is not a general-purpose embedded browser. User-selected RSS/HTTPS plugin sources remain external-content features and must be disclosed under the applicable question. |
| Location | No GPS/precise-user-location sharing feature found. Network hosts receive ordinary IP/request metadata; configured service region and chosen ping/server addresses are different concepts from precise location. |
| Purchases / gambling | No app-implemented payments, purchasable random items, betting or cash-prize mechanisms found. Opening an ordinary external service/browser link does not justify claiming the app is a storefront. Read the current question's scope. |
| Moderation / filters | External integrations are optional/configured, not an in-app public content-upload service. Do not invent an in-app moderation or age filter. If a live question requires stronger controls for externally sourced content, resolve the requirement before certification. |

Do not fabricate an existing IARC ID. An actual previous certificate can be reused only if the owner supplies one for this app and the live process accepts it. The result, regional ratings, certificate ID and any restricted markets are account-side completion items.

## Submission options

Use the certification notes below, list the separate Companion dependency, and select a reviewable release schedule. Do not switch to immediate publication merely because it is the default while tests or public pages are incomplete. Account terms/identity verification remain the owner's actions. Store certification is Microsoft's action, not a local build result.

Current primary references: [App properties](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/enter-app-properties), [MSIX submission](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/create-app-submission), [Store policies](https://learn.microsoft.com/en-us/windows/apps/publish/store-policies-and-code-of-conduct).

## Verified live progress, 11 November

Product `9P03461RLGG0`, submission `1152921505702369346`. Microsoft Developer Agreement 8.11 accepted with explicit owner approval. Properties is Complete after clearing inappropriate MR-PC, OneDrive backup, game recording and alternate-drive declarations. This record does not imply that pricing/listings/IARC/package upload or certification submission have completed.
