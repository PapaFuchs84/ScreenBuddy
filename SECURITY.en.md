# Security

[Deutsch](SECURITY.md) · **English**

## Supported versions

| Version | Security fixes |
|---|---|
| 1.2 | ✅ |
| 1.1 | on request only |
| 1.0 | ❌ |

## Reporting a vulnerability

Please **do not open a public issue**. Instead:

- Message Basti (FuchsLabs) directly.
- Once the repository is public, you can also use **Security → Report a vulnerability** on GitHub.

Helpful details: affected version, what an attacker could do, steps to reproduce, and a suggested fix if you have one. **Do not send real tokens or keys**, not even your own.

FuchsLabs confirms receipt as soon as possible, keeps you updated on the fix and credits you in the changelog if you wish.

## What the security model covers

- The settings page only listens on `127.0.0.1` and must not be usable from the network or from other websites (host check, custom header for changes, `Sec-Fetch-Site` check, no framing).
- The Discord token and the Steam and Xbox keys are stored only DPAPI-encrypted in `config.toml`, never appear in the log and are never sent to the browser.
- Data from the internet (feeds, avatars, player names) is size-limited and only drawn as text.
- The app does not need admin rights.

**Out of scope:** anyone with access to your Windows user account can read stored credentials the same way the app does. Other programs on the same PC can open the settings page but do not receive any credentials.

The latest review report is in [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) (German).
