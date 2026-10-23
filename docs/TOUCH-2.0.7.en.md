# Touch targets and playtime in 2.0.7

[Deutsch](TOUCH-2.0.7.md)

**Local development build, not published or installed.** The current download remains [v2.0.6](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.6).

Enable **Display → Touch control**, then select **Save & apply**. Existing configurations with touch disabled keep that setting. HDMI/USB-C also needs the USB touch connection and correct Windows display assignment. Windows 10/11 x64 and WebView2 Evergreen for settings remain prerequisites.

- **Battlefield 6 leaderboard:** Tap a player to open their GameTools profile in the default browser. Platform and name come from the configured account; special characters are URL-encoded. Error rows and the loading screen do not open a profile. The leaderboard also shows API-supplied playtime rounded to whole hours; missing values are omitted. Rank, K/D and win rate remain visible.
- **Friends and Discord friends:** Tap a person. Linked accounts shown as online are tried first, followed by the remaining accounts. Both groups use **Discord → Steam → Xbox** order. Discord/Steam require registered desktop apps. Missing apps and launch failures fall through to the next entry.
- **Steam** opens the private chat. **Discord** and **Xbox** open nothing as of 2.0.8: Discord lacks the direct-message channel ID and Xbox has no chat link; the 2.0.7 profile variant was removed at the user's request. Whether a new window, tab or existing window is used depends on the platform app/browser. The display app sends no messages.
- **Blogs:** Posts continue to open their HTTP/HTTPS links. Swiping still changes pages; tapping empty areas, headers/footers or entries without a target uses the existing page navigation.

Selection uses the last rendered data snapshot, including sorting, subpages, wide tiles and empty rows on the final page. Background refreshes cannot change the identity behind an already displayed row. A failed launch does not also change pages; `usb-display.log` records a hint without account IDs or target URLs.

## Troubleshooting

| Observation | Action |
|---|---|
| Swiping and tapping do nothing | Enable touch and check its USB connection and Windows display assignment. |
| Steam opens instead of Discord | Check the Discord desktop app and Windows protocol association. Online status and linked accounts also determine priority. |
| Only the profile opens | This is the approved Discord/Xbox behavior; choose Message there. |
| Xbox shows the wrong profile or no message action | Sign in with the intended Microsoft account, check the original gamertag and privacy/contact permissions. |
| No target opens | Check installed platform apps, the Windows default browser and the local app log. Launch success does not establish successful sign-in or permission to chat. |

## Validation and references

Automated checks use sample data and substituted launch functions: no real chats, browser tabs or messages. See the [test record](../cpp/TESTPROTOKOLL.md) for build results, test counts and remaining acceptance checks. Physical touch and opening in signed-in platform clients remain to be validated for 2.0.7. Earlier test results do not automatically apply to this build.

Profile URLs follow the [GameTools website](https://github.com/community-network/gametools.network-website). Steam uses the [Steam browser protocol](https://developer.valvesoftware.com/wiki/Steam_browser_protocol). Discord uses the [profile link described in its documentation repository](https://github.com/discord/discord-api-docs/discussions/5183), which does not guarantee direct-message or window control. Xbox uses its [web profile](https://account.xbox.com/Profile).
