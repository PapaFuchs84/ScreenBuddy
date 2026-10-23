# Touch targets and playtime in 2.0.7

[Deutsch](TOUCH-2.0.7.md)

**The friend behavior on this page applies from 2.1.0 (local development build, not published; the change originates in 2.0.9).** New in 2.1.0: on the Home Assistant page, tapping a switch tile flips that switch (only with touch active; sensors do not react to taps). The current download remains [v2.0.8](https://github.com/PapaFuchs84/fuchslabs-usb-display/releases/tag/v2.0.8), where tapping a friend only opens the Steam chat.

Enable **Display → Touch control**, then select **Save & apply**. Existing configurations with touch disabled keep that setting. HDMI/USB-C also needs the USB touch connection and correct Windows display assignment. Windows 10/11 x64 and WebView2 Evergreen for settings remain prerequisites.

- **Battlefield 6 leaderboard:** Tap a player to open their GameTools profile in the default browser. Platform and name come from the configured account; special characters are URL-encoded. Error rows and the loading screen do not open a profile. The leaderboard also shows API-supplied playtime rounded to whole hours; missing values are omitted. Rank, K/D and win rate remain visible.
- **Friends and Discord friends:** Tap a person. The order is fixed: **Steam chat**, otherwise **Discord profile**, otherwise **Xbox profile**. Steam and Discord require registered desktop apps; a missing app or failed launch falls through to the next target. Online status is not considered.
- **Steam** opens the private chat. **Discord** opens the person's profile; choose **Message** there (no direct chat link exists because the bot lacks the direct-message channel ID). **Xbox** opens the profile on xbox.com in the browser (no chat link available), also with a path to Message. Whether a new window, tab or existing window is used depends on the platform app/browser. The display app sends no messages.
- **Blogs:** Posts continue to open their HTTP/HTTPS links. Swiping still changes pages; tapping empty areas, headers/footers or entries without a target uses the existing page navigation.

Selection uses the last rendered data snapshot, including sorting, subpages, wide tiles and empty rows on the final page. Background refreshes cannot change the identity behind an already displayed row. A failed launch does not also change pages; `usb-display.log` records a hint without account IDs or target URLs.

## Troubleshooting

| Observation | Action |
|---|---|
| Swiping and tapping do nothing | Enable touch and check its USB connection and Windows display assignment. |
| Steam opens instead of Discord | Intended: Steam takes priority when the person has a linked Steam account. |
| Only the profile opens | Intended for Discord and Xbox (no chat link available); choose Message there. Without the Discord desktop app, Xbox is used if linked. |
| Xbox shows the wrong profile or no message action | Sign in with the intended Microsoft account, check the original gamertag and privacy/contact permissions. |
| No target opens | Check installed platform apps, the Windows default browser and the local app log. Launch success does not establish successful sign-in or permission to chat. |

## Validation and references

Automated checks use sample data and substituted launch functions: no real chats, browser tabs or messages. See the [test record](../cpp/TESTPROTOKOLL.md) for build results, test counts and remaining acceptance checks. Physical touch and opening in signed-in platform clients remain to be validated for 2.0.7. Earlier test results do not automatically apply to this build.

Profile URLs follow the [GameTools website](https://github.com/community-network/gametools.network-website). Steam uses the [Steam browser protocol](https://developer.valvesoftware.com/wiki/Steam_browser_protocol). Discord uses the [profile link described in its documentation repository](https://github.com/discord/discord-api-docs/discussions/5183), which does not guarantee direct-message or window control. Xbox uses its [web profile](https://account.xbox.com/Profile).
