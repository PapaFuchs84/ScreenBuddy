# ScreenBuddy Microsoft Store — 11 November 2026

## Current state

The Windows developer account is active and the Store product/submission draft exists. There is no confirmed certification submission or public Store release yet. Independent final audit passed with no findings. The reviewed local commit and generated preparation provenance identify the reproducible source state; no push is recorded in this preparation.

- Reserved title: **ScreenBuddy by FuchsLabs**, selected by the owner because the plain name was unavailable.
- Store ID: `9P03461RLGG0`; submission draft: `1152921505702369346`.
- Identity: `49510Herbst.ScreenBuddybyFuchsLabs`; publisher: `CN=816E06C8-23FD-4A47-9D83-2F67C6F1C211`; exact publisher display name: **Herbst**.
- Package family: `49510Herbst.ScreenBuddybyFuchsLabs_x6tj6b3k2dr8c`.
- Microsoft Developer Agreement **8.11** was explicitly approved and its acceptance verified on 10 November.
- Properties is Complete: Utilities + tools, no misleading Backup/manage subcategory; MR-PC, OneDrive backup, game recording and alternate-drive declarations are off.
- Pricing is Complete at **0 EUR** (11 November).
- The 8.83 MB package was transferred and the portal shows Validating…; acceptance is not yet confirmed. The main agent is completing localized listings, live IARC and submission options.

## Exact candidate

`dist/store-submission-20261110/ScreenBuddy-2.11.0.0-x64.msix`

MSIX SHA-256: `d050177351a490e56d84ff4b61235466e7ba1bf111d2f2e3774a046165d7f6f7`.

Included EXE SHA-256: `b78e2dc576beea84c169b888349ce1cb80ca594d9403dea43f0dff8eeab2a466`.

Title, identity, publisher, architecture/version and binary were checked against official portal fields (`dist/store-submission-20261110/package-verification.json`). Microsoft signs the Store MSIX during certification. This is not a publicly signed standalone EXE release. Never submit a validation identity as the official product.

Worktree: `feature/msstore-2.11`, base `7b8d0c1`. Upstream `4b39e04` was inspected for Companion 1.1.0; Windows source was unchanged upstream before this Store work. Android release/build changes are outside this Windows Store preparation.

## Completed preparation

- Package detection, LocalState, Store-managed updates, initially disabled StartupTask and source-preserving explicit import remain in place.
- Packaged FPS setup/vendor-autostart changes cannot elevate or modify groups/tasks. DE/EN instructions explain external administrator setup; existing FPS/HWiNFO data remain usable.
- Packaged Help & privacy links point to the real public pages. Startup navigation safely returns unavailable before initialization; integration tests exercise early requests.
- Packaging hashes only the newly built MSIX, prevents stale asset-folder merges, copies six policy/support/form/reviewer documents and supports the exact XML-escaped reserved DisplayName.
- Four current DE/EN Game Mode/editor samples (1600 × 1200) use actual native Page/TreeCanvas code in a developer package. Manifest/visual review confirm final binary, correct language and synthetic data. No desktop capture, real account or physical-device evidence is implied.

## Public app documentation

[App privacy](https://fuchslabs.duckdns.org/screenbuddy/privacy/) · [Support](https://fuchslabs.duckdns.org/screenbuddy/support/)

Both bilingual pages include Companion 1.1 automation/permission explanations. After explicit owner approval resolved the initial automatic transfer rejection, only these two routes were updated in the live owning source and active release using known-old-hash guards and backups. HTTPS 200, both languages and exact new content hashes were verified on 10 November. Record: `packaging/windows/website-pages/deployment-verification.json`. No unrelated website files or services changed.

## Final validation

- Original native runner **25/25 suites PASS**, using a temporary private Windows desktop without switching the visible desktop. Existing assertions/goldens preserved.
- Core 3,556; layout 1,877; builtin images 4,221 plus hit maps 4,221; Companion integration 40; installer-preservation cases 2. Extra Game Mode 290 and custom-view 2,144 checks passed.
- Current developer-package checks passed MakeAppx, startup enable/restore, import/WhatIf/overwrite protection, synthetic 2.11.1.0 update, LocalState retention and uninstall. Both elevation guards passed without real group/task mutations.
- Actual WACK under the official identity: **overall PASS**, no partial run, **13/13 required PASS**. Ten optional tests passed; one optional Blocked executables/S-mode heuristic warns. Microsoft documents optional Desktop Bridge checks as informational. Neither 24/24 nor full S-mode support is claimed.
- Following approved UAC/manual Quit, the original installed app was restarted, its local API responded HTTP 200 and display status was OK. Only our developer test package was removed. No user process was killed or certificate trust installed.

Sanitized current summary: `packaging/windows/validation-summary-20261110.json`. Raw WACK XML/transcripts and machine/user logs remain ignored/local. Preparation artifacts contain no private configurations, pairings, credentials or signing material.

## Remaining work

1. Verify package upload/validation in the live Store draft; complete localized listings, live IARC and submission options. Factual form preparation/reviewer notes are in `packaging/windows/STORE-FORMS-DRAFT.md` and `CERTIFICATION-NOTES.md`; do not invent an IARC certificate.
2. Keep the generated preparation ZIP/provenance synchronized with the reviewed local commit and any final portal-state record. Do not replace the immutable candidate without new verification.
3. Keep signed Store installation/servicing and agreed physical-device acceptance distinct from developer registration/automated tests. These are not claimed complete.
4. Submit for Microsoft certification only after actual required portal sections/checks are complete. Observe certification/public availability before reporting them done.

The generated `dist/ScreenBuddy-2.11.0-StorePreparation.zip` records its exact source commit in provenance.json and its contents in SHA256SUMS.txt. Inspect those records when resuming. Its unsigned development EXEs are local-only; never upload the whole ZIP as a public Windows release.

## Historical evidence

The 9 November disabled-account observation/unsent reactivation draft are historical, not current blockers. Earlier 23/25/font/focus diagnostics and 2.9.6 preparation remain dated in the release checklist and historical validation files. Final font/private-desktop prerequisites resolved environmental failures without replacing goldens. See both distribution-document languages and `packaging/windows/RELEASE-CHECKLIST.md` for the evidence trail.
