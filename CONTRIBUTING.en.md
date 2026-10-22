# Contributing to FuchsLabs USB-Display 2.0

[Deutsch](CONTRIBUTING.md) · **English**

The current implementation is C++20 under `cpp/`. Root Python files remain the historical Version 1.x source. Public visibility does not change the project's licence.

## Development setup

Windows x64, Visual Studio Build Tools 2022 with C++/Windows SDK and CMake/Ninja, Git, and a bootstrapped vcpkg checkout at `%USERPROFILE%\FuchsLabs\vcpkg`.

```bat
git clone https://github.com/PapaFuchs84/fuchslabs-usb-display.git
cd fuchslabs-usb-display\cpp
build.bat nopause
build\FuchsLabs-USB-Display.exe --run --no-lcd -c test.toml
```

Run from a developer terminal with CMake/Ninja available. `cpp/build.bat` builds Release, runs checks and copies the single app/installer EXE to `cpp/dist/`. The root `build.bat` still builds Python 1.x. WebView2 Evergreen Runtime must be installed separately to use the settings window. See [build notes](cpp/README.md#build).

## Workflow

1. Branch from `main`, make focused changes and explain the resulting behavior.
2. Compile and run checks appropriate to the change. `cpp/build/fl-tests.exe` covers core behavior and `cpp/build/fl-monitor-tests.exe` covers gesture basics; its `window` mode visibly tests a secondary screen with injected touch input; use `--demo --no-lcd --preview preview` for sample output.
3. Check affected UI in German and English. Hardware tests require exclusive access to the COM port: stop the other display app first.
4. Documentation is part of every change: update affected READMEs, quick start, reference, version/download links, usage and test status in the same task, without a separate request. Keep existing German/English versions and both root changelogs consistent; for C++ also update `cpp/CHANGELOG.md`. See [project rules](AGENTS.md).
5. Open a pull request against `main` with validation details; use sample data in screenshots.

## Code rules

- Follow existing C++20 style, use RAII and keep background work bounded.
- App/display translations: `cpp/src/i18n.cpp`; settings translations: `cpp/res/webui.html`.
- Colours/logo: `cpp/src/brand.h`; rendering: `gfx.cpp` and `render.cpp`. Windows fonts only; no brightness commands during a display frame transfer.
- Dependency changes: `cpp/vcpkg.json`, its pinned baseline and `cpp/overlay-ports/`. Keep third-party licence notices.
- Use only authorized/public data APIs; do not implement Discord self-bots or unofficial PlayStation logins.
- Store credentials through `cpp/src/secure.cpp` (Windows DPAPI). Never commit configuration, credentials, logs or private screenshots; review `git diff --cached`.

## Versions and reports

Update `cpp/CMakeLists.txt`, `cpp/src/util.h` and `cpp/res/app.rc` consistently for a code release. Publish EXE plus SHA-256 checksum and state runtime prerequisites and outstanding tests. Documentation-only changes do not replace an already published EXE.

Bugs: [feedback guide](TESTPHASE.en.md). Vulnerabilities: [SECURITY.en.md](SECURITY.en.md); do not disclose secrets in a public issue. The old [review](SICHERHEITSREVIEW.md) is not a complete C++ audit.
