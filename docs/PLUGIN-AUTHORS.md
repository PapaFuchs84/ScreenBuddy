# Submitting plugins for ScreenBuddy

Short guide for authors. German: [PLUGIN-AUTOREN.md](PLUGIN-AUTOREN.md). Format: [tutorial](PLUGIN-TUTORIAL.en.md) and [package format](PLUGIN-PACKAGE-FORMAT.md). Rules: [store terms](PLUGIN-STORE-TERMS.md).

1. **Write the plugin** following the tutorial (one file `<id>.flplugin.json`, no code).
2. **Required store fields:** `description` (at least 10 characters), `author`, `license` (`MIT` or `CC0-1.0`), `category` (`system`, `games`, `smart-home`, `info`, `other`), `min_app_version`. Plugins with sources: `refresh_seconds` of at least 60 and a `sample` for each source.
3. **Check:** `fl-plugin-validate --store --app-version 2.5.0 your-file.flplugin.json`. The output lists the hosts data would go to; they must fit the purpose.
4. **Submit:** pull request to the `ScreenBuddy-Plugins` repository at `plugins/<id>/<id>.flplugin.json` (once it is public). CI repeats the check; a maintainer reviews purpose, hosts and license.
5. **Updates:** bump the version (`X.Y.Z`). If it adds hosts, users must consent again. Not allowed: credentials, tokens or keys in the manifest.
