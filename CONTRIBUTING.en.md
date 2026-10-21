# Contributing to FuchsLabs USB-Display

[Deutsch](CONTRIBUTING.md) · **English**

Great that you want to help! The repository is private; anyone invited by FuchsLabs can contribute.

## Bugs and ideas

- Report **bugs** via *Issues → New issue → Bug report*, **ideas** via *Idea or feedback*. What a good report needs and where the log is: [`TESTPHASE.en.md`](TESTPHASE.en.md).
- **Security vulnerabilities** never as an issue, but as described in [`SECURITY.en.md`](SECURITY.en.md).

## Development setup

Windows 10/11, Python 3.11+ (64-bit), Git.

```bat
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display
build.bat
.venv\Scripts\python fuchslabs_display.py --no-lcd
```

`build.bat` creates the `.venv` and builds the EXE. With `--no-lcd` the app runs without a display, and the settings page is at `http://127.0.0.1:8765/`. Sample pictures of all pages without hardware: `--demo --no-lcd --preview preview`.

## Workflow

1. Create a branch from `main` for each change, e.g. `fix/display-reconnect` or `feature/new-page`.
2. Small, self-contained commits with a clear message. The project's commit messages are German; English is fine too.
3. Before the pull request:
   - `.venv\Scripts\python test_friends.py` passes.
   - The app starts with `--no-lcd`, and affected pages look right with `--demo --preview` (light and dark, German and English).
   - Visible changes are listed under **[Unreleased]** in [`CHANGELOG.md`](CHANGELOG.md) and [`CHANGELOG.en.md`](CHANGELOG.en.md); new settings also in [`DOKUMENTATION.md`](DOKUMENTATION.md).
4. Open a pull request against `main` and briefly describe: before, after, how you tested. Attach a picture for visual changes.

## Code rules

- **Style:** Python like the existing code. Code comments and source texts are German.
- **Languages:** every text a user sees exists in German and English. App and display texts are translated in `i18n.py`, settings page texts in the `EN` table in `webui.html`. Settings page texts should be simple enough for a child to understand.
- **Lightweight:** the app runs permanently in the background. No new dependencies without a good reason; packages are pinned to exact versions in `requirements.txt`.
- **Design:** colours and logo only from `brand.py`. Windows fonts only (Bahnschrift, Segoe UI), no web fonts.
- **Display:** pages draw in logical 480 × 320 coordinates; `render.py` scales to the real resolution. No brightness commands during an image transfer.
- **Data sources:** official or public APIs only. Nothing that violates terms of service or puts accounts at risk (no Discord self-bot, no unofficial PlayStation access).
- **New login fields** in the settings page get fold-out help with a link to the right page.

## Secrets and private data

- **Never commit:** `config.toml`, tokens, API keys, logs, build folders. `.gitignore` covers this; still check `git status` before every commit.
- Store credentials only via `secure.py` (DPAPI), never write them to the log, never return them to the browser.
- **Screenshots** with sample names only. `anleitung/screenshots.mjs` replaces private names (`FL_LANG=en` for English); look at every picture yourself before committing.

## Versions

Versions follow [SemVer](https://semver.org/). For a release: bump the version in `webui.py` and `version-info.txt`, move *Unreleased* in both changelogs to the new version, set the tag `vX.Y`.
