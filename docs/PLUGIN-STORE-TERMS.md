# Plugin store: terms of use, rules and privacy

© 2026 FuchsLabs · **Draft, not legally reviewed.** Before the store goes public, Basti or a qualified person should review these texts. The reference to the operator's imprint is also still to be added. German: [PLUGIN-STORE-BEDINGUNGEN.md](PLUGIN-STORE-BEDINGUNGEN.md).

## 1. What this is

The plugin store is a collection of small data files (`*.flplugin.json`) for ScreenBuddy. A plugin contains **no program code**. Plugins come from FuchsLabs or third parties. **Third-party plugins are not from FuchsLabs**, even when listed in the store.

## 2. For users

- The store is **optional** and can be switched off in the settings. Local plugins keep working without it.
- Before installation every plugin shows **which hosts it sends data to**. It stays off after installation. Every host needs your consent; consent is reset on update.
- The store is provided "as is", **without warranty** for availability, fitness or correctness of plugins. Check the terms of the services a plugin queries (e.g. weather APIs).
- Badges: **"Official"** only for FuchsLabs plugins, **"Reviewed"** after a maintainer's review, **"Community"** for the rest. A badge is not a guarantee.

## 3. For submitters

1. **License:** your plugin is under **MIT** or **CC0-1.0** (`license` field). You confirm that you are the author or hold all needed rights to texts and content.
2. **Content:** no legal violations, no trademark misuse, no deception about purpose or sender, **no tracking, ads or affiliate links** in displays, no third-party data without permission, no circumvention of the queried service's terms.
3. **Trademark:** the name FuchsLabs and the logo must not be used as sender (see [TRADEMARKS.md](../TRADEMARKS.md)).
4. **Technical:** the plugin passes `fl-plugin-validate --store`. Hosts must fit the purpose and be named in the description.
5. FuchsLabs may **reject, request changes to or block** plugins (signed revocation list). Repeated violations can lead to exclusion of the GitHub account.

## 4. Reports

Report suspicious plugins via **Issues → "Report a plugin"**; for acute danger use the private route in [SECURITY.en.md](../SECURITY.en.md). Goal: reply within 72 hours.

## 5. Privacy

- ScreenBuddy contacts the store at most **once a day** (and when you click "Refresh store") at the address of the store repository (GitHub Pages). The hosting provider technically sees your IP address; FuchsLabs sends **no usage data, identifiers or telemetry** and stores nothing about you.
- Plugin settings and credentials stay local (DPAPI-protected). They only go to hosts you consented to.
- For submissions via GitHub, GitHub's privacy notices apply; your GitHub name appears as author.

## 6. Liability

FuchsLabs is not liable for third-party plugins. Statutory liability for intent and gross negligence and for injury to life, body and health remains unaffected.
