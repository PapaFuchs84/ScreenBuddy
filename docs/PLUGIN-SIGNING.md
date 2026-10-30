# Plugin signing (phase 2)

© 2026 FuchsLabs · **Status:** library, tool and tests are implemented. **Not yet available:** the production key (the maintainer must generate it offline), the store repository with index build via Actions, downloading in the app and the store UI (phase 3). So far the app only verifies in tests; there is no active store.

## Idea

The store consists of data that a maintainer reviews and signs with an **Ed25519** key. The app only knows the public key (pinned in code, `store_public_key_hex`) and installs nothing that does not match. A compromised server or GitHub account therefore cannot slip in unsigned plugins.

## Store files

| File | Content |
| --- | --- |
| `index.json` | All plugins: ID, name, version, author, license, category, `min_app_version`, path, **SHA-256**, size, **hosts**; plus `sequence` and `generated`. |
| `index.json.sig` | Detached signature (Base64, 88 characters) over the exact bytes of `index.json`. |
| `revoked.json` + `.sig` | Revocation list: `{"id", "versions": "all" or ["1.0.0"], "reason"}`, also signed. |
| `plugins/<id>/<id>.flplugin.json` | The plugin itself (one file). |

A domain prefix (`…Plugin-Index-v1` / `…Plugin-Revoked-v1`) is prepended when signing, so an index signature never validates as a revocation-list signature or vice versa.

## Verification order in the app

1. **Signature first.** Invalid or missing: the document is not parsed further.
2. Strict parsing (at most 1 MiB, unknown fields and duplicate keys rejected, paths only `plugins/<id>/<id>.flplugin.json`).
3. **Rollback protection:** `sequence` must not be lower than the last accepted one (`sequence_acceptable`).
4. After download, `package_matches`: size, SHA-256, valid manifest, and ID, version, license, author, category, `min_app_version` and **hosts** equal the signed index entry.
5. Revoked plugins (`is_revoked`) are disabled at the next sync.

Existing protections stay: activation off by default, per-host consent, re-consent when sources change.

## Tool `fl-plugin-sign`

```text
fl-plugin-sign keygen <private.pem>
fl-plugin-sign build-index <plugins-root> <out-dir> <sequence>
fl-plugin-sign sign   index|revoked <file> <private.pem>
fl-plugin-sign verify index|revoked <file> <public-hex>
```

`build-index` only accepts plugins that pass `fl-plugin-validate --store` and requires the layout `<root>/<id>/<id>.flplugin.json`. `keygen` never overwrites an existing key file.

## Key management (policy)

- The maintainer generates the production key **offline** with `keygen`; the PEM file never goes into the repository or chats. For CI later, store it as a GitHub secret and write it to a temporary file only at run time.
- The public key (hex) is entered in `plugin_sign.h` and shipped with an app release.
- **Rotation:** generate a new key, ship a release with both keys, sign the index with the new one, remove the old key after two releases. This multi-key flow is **not implemented yet**; currently there is exactly one key. If the key is lost or compromised, only a new app release helps.
- The revocation list does not stop an **old, correctly signed version** from circulating; use rollback protection and the revocation list together.

## Limits

- A signature proves the maintainer published the index, not that a plugin is harmless. Approval remains a human review.
- `generated` is informational only; there is no expiry. An attacker can replay an old valid index as long as the app does not yet know its `sequence` (first start).
- Tested: 60 checks (keys, signature, tampering, domain separation, index, package, revocation) and an end-to-end run of the tool on Linux. Windows/MSVC: see the PR.
