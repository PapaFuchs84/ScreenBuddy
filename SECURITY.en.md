# Security

[Deutsch](SECURITY.md) · **English**

## Supported versions

| Version | Security fixes |
|---|---|
| 2.3.0 | current release |
| 2.0.0–2.2.0 | update to 2.3.0 |
| 1.2 | historical |
| 1.1 | on request only |
| 1.0 | ❌ |

## Reporting a vulnerability

Please **do not open a public issue**. Instead:

- Message Basti (FuchsLabs) directly.
- If enabled in the repository, you can also use **Security → Report a vulnerability** on GitHub.

Helpful details: affected version, what an attacker could do, steps to reproduce, and a suggested fix if you have one. **Do not send real tokens or keys**, not even your own.

FuchsLabs confirms receipt as soon as possible, keeps you updated on the fix and credits you in the changelog if you wish.

## What the security model covers

- The settings page only listens on `127.0.0.1` and must not be usable from the network or from other websites (host check, custom header for changes, `Sec-Fetch-Site` check, no framing).
- The Discord token, the Steam and Xbox keys and the Home Assistant token are stored only DPAPI-encrypted in `config.toml`, never appear in the log and are never sent to the browser.
- Data from the internet (feeds, avatars, player names) is size-limited and only drawn as text.
- Companion app (off by default): listens only on private IPv4 addresses of the home network (port 8777, never 8765), accepts connections only from private address ranges and speaks TLS only (self-signed certificate; the app checks its fingerprint). Pairing only with the PIN shown on the settings page (2 minutes, single use, locked for 5 minutes after 5 failed attempts). Device keys (32 bytes) and the TLS key are stored DPAPI-encrypted in `companion.json`. A paired device can trigger the same touch targets as a touchscreen (open browser/chat, Home Assistant switches).
- Plugins are plain JSON data without program code. Sources need HTTPS, public addresses and consent per host; credentials stay DPAPI-protected. The plugin store (if active) checks the Ed25519 signature, SHA-256 and the revocation list before every installation, see [Plugin signing](docs/PLUGIN-SIGNING.md).
- The app does not need admin rights.

## Reporting a plugin

Report a plugin that sends data to unexpected hosts, deceives users, infringes rights or trademarks, or uses third-party content without permission via **Issues → "Report a plugin"** (template). Please include the plugin ID, version and what looks wrong. If a plugin is **acutely dangerous** (for example data exfiltration), report it privately like a vulnerability, see above. The aim is to answer reports within 72 hours; dangerous plugins are blocked through the signed revocation list.

**Out of scope:** anyone with access to your Windows user account can read stored credentials the same way the app does. Other programs on the same PC can open the settings page but do not receive any credentials.

The latest review report is in [`SICHERHEITSREVIEW.md`](SICHERHEITSREVIEW.md) (German).

> The linked review covers historical Python 1.x. It is not a full C++ 2.0 audit; see [current test record](cpp/TESTPROTOKOLL.md). WebView2 Runtime is a separate prerequisite.
