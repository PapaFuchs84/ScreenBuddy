# Additive app documentation deployment

Prepared and executed additively on 10 November 2026 after review. The build script itself never deploys; the two reviewed HTML files were added to the verified live owning source and active release, then fetched over HTTPS with exact content-hash verification. See website-pages/deployment-verification.json.

## Reviewable files

Run `python packaging/windows/Build-AppPages.py`. The only public payload is:

- `website-pages/screenbuddy/privacy/index.html`
- `website-pages/screenbuddy/support/index.html`

Both pages contain the reviewed German and English Markdown, accessible language anchors, inline CSS/system fonts and links to existing public legal/contact pages. No JavaScript, cookies, remote fonts, new host, copied private address or old website redesign is included. `website-pages/provenance.json` records source/output hashes for local review; it need not be public.

Proposed final URLs:

- https://fuchslabs.duckdns.org/screenbuddy/privacy/
- https://fuchslabs.duckdns.org/screenbuddy/support/

Use these in Store properties and packaged Help & privacy only after verifying the live deployment returns these actual contents over HTTPS.

## Owning website and persistence

The available local website source is `C:/Users/admin/FuchsLabs/website-2.10-official-media/fuchslabs-website`. Its earlier HANDOFF says it was a pending archive-based website update. Do not overwrite the live website from that whole directory or assume it is a fresh mirror of the server.

Read-only inspection of its deployment sources establishes the intended layout: `deploy/fuchslabs-update.sh` builds `/opt/fuchslabs/app/dist`, creates timestamped `/srv/fuchslabs/releases/<stamp>` and atomically switches `/srv/fuchslabs/current`. The public domain is served through Caddy/NPMplus. The update timer can replace a release, so copying only into `current` would not persist.

Before applying, inspect the actual live owning source and service paths. Preserve all unrelated live files. Add the two HTML payloads to the live source's `public/screenbuddy/privacy/index.html` and `public/screenbuddy/support/index.html` (Astro public assets pass through unchanged). Also add only those two files to the current public release for immediate availability, using temporary adjacent files and atomic rename. If either route already exists, compare it and back up the two existing route files before replacing; do not delete directories or switch the whole website release. Keep ownership/mode consistent with adjacent public files. A deployment operator must resolve actual server access, paths and permissions from current state; this note contains no credentials.

The existing website-only privacy declaration remains at `/datenschutz/`. Link the app-specific notice from it or the ScreenBuddy section when reviewing the actual current owning source. Existing DE/EN/FR marketing text claiming that an update check is the app's only request is inaccurate for the current app: catalog, configured integrations and game artwork also use the network. A targeted replacement is prepared below; do not redeploy unrelated archived marketing content.

Suggested German replacement: **Lokale Daten, optionale Verbindungen.** „Einstellungen bleiben lokal. Updates, Plugin-/Theme-Katalog und aktivierte Integrationen können ihre jeweiligen Anbieter kontaktieren. Einzelheiten und Abschaltmöglichkeiten stehen im App-Datenschutz.“

Suggested English replacement: **Local data, optional connections.** “Settings stay local. Updates, the plugin/theme catalog and enabled integrations can contact their respective providers. See the app privacy notice for details and controls.”

Suggested French replacement: **Données locales, connexions facultatives.** « Les paramètres restent locaux. Les mises à jour, le catalogue de plugins/thèmes et les intégrations activées peuvent contacter leurs fournisseurs respectifs. Consultez la notice de confidentialité de l’application pour les détails et les réglages. »

## Verify and rollback

After deployment, retrieve both exact HTTPS URLs, check the German and English sections, controller/contact, AI opt-in and external-network explanations, support links, status 200, content type and no authentication requirement. Check the actual HTML against the reviewed hashes if the server serves it unchanged. Open the packaged app controls and confirm they land on the intended page. Record verification time and deployed source/release path locally.

Rollback is limited to these added files or their saved previous versions in both the live owning source's public folder and active release. Never roll back the whole website to the older local archive just to remove these pages. Do not delete any other screenbuddy routes.

## Executed Companion 1.1 amendment

The owner explicitly approved the exact pve3/CT403 helper transfer and only the two app-route updates after an initial automatic review rejection. The unchanged reviewed helper preflighted known old hashes, saved old copies outside the public tree and atomically updated the owning public source and active release. On 10 November 20:13:55 UTC, both public HTTPS pages returned 200 with correct German/English sections and exact new SHA-256 values: privacy `c948b103bc8259dee107fccc574afcde5d80865e5710c92f6f0552b0e2582eca`; support `f20a1a23eb4ba2619db09a1f40ccb3599acf6a3a66fe5494deecdf5c4b0362e2`. The amendment is deployed, not awaiting approval. The verification JSON records backup location and previous revision; no unrelated website files or services were changed.
