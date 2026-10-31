# Spotify page

[Deutsch](SPOTIFY.md) · **English**

Shows the current track with cover art, artist, album and progress on the display. With touch (monitor, Companion) the buttons ⏮ ⏯ ⏭ control playback.

## Set up (one time, free)

1. Create an app at [developer.spotify.com/dashboard](https://developer.spotify.com/dashboard) (tick "Web API").
2. Add exactly the address from *Settings → Spotify* as **Redirect URI** (default `http://127.0.0.1:8765/spotify/callback`; follows the settings page port).
3. Paste the **client ID** into ScreenBuddy, turn on "Show page", save.
4. Click "Sign in to Spotify" and approve in the browser.
5. On an existing installation, tick the page under *Settings → Pages & order* (display and, if wanted, the Companion page list). New pages are not shown automatically. Done.

No client secret is needed (sign-in uses PKCE).

## Limits

- **Controlling needs Spotify Premium** (Spotify's rule). Showing the track works without Premium.
- A Spotify device must be active (the app on phone/PC is running or was used recently), otherwise Spotify reports "no active device".
- Apps in Spotify development mode are limited to a few users that you approve in the dashboard under *User Management*. Your own account is always included.
- Polling every 4 s (playing) or 8 s (paused); when Spotify rate-limits, ScreenBuddy pauses briefly.

## Privacy

- Permissions: read and control current playback. Nothing more.
- The refresh token is DPAPI-encrypted in `spotify.token` next to `config.toml`, never in the configuration; only your Windows account can read it. "Sign out" deletes the file.
- Cover art is only loaded from `i.scdn.co`.

Spotify is a trademark of Spotify AB; FuchsLabs is not affiliated with it.