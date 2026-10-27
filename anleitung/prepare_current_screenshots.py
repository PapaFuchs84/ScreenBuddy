"""Prepare release images from native captures and the current C++ demo renderer.

Usage: python anleitung/prepare_current_screenshots.py --build cpp/build/screenbuddy-dev

Native settings PNGs must already exist in anleitung/bilder[/en]. Capture the real
ScreenBuddy window with a dedicated FUCHSLABS_INSTANCE, --no-lcd, its own config
and HTTP port, no credentials, disabled data sources and update checks. Never
use screenshots.mjs for native UI proof: its legacy workflow rewrites the DOM.
This script neither opens native windows nor edits screenshot pixels.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
NATIVE = ("start", "prev0", "prev4", "display", "pages", "system", "friends-native",
          "discord", "bf6", "blogs", "ha", "speichern", "companion", "plugins", "design", "app")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--renderer-only", action="store_true",
                        help="Prepare only fixed demo exports; leave native capture provenance pending")
    args = parser.parse_args()
    build = args.build.resolve()
    renderer = build / "fl-tests.exe"
    app = build / "ScreenBuddy.exe"
    if not renderer.is_file() or not app.is_file():
        parser.error("The build must contain ScreenBuddy.exe and fl-tests.exe")
    work = build / "release-screenshot-render"
    sources = []
    missing = []
    for lang in ("de", "en"):
        rendered = work / lang
        subprocess.run([str(renderer), "render", str(rendered), lang], check=True)
        images = ROOT / "anleitung" / "bilder"
        docs = ROOT / "docs" / "screenshots"
        if lang == "en":
            images /= "en"
            docs /= "en"
        images.mkdir(parents=True, exist_ok=True)
        docs.mkdir(parents=True, exist_ok=True)
        display_names = {
            "bf6": "display-bf6-dark" if lang == "en" else "display-bf6-dunkel",
            "blogs": "display-blogs-dark" if lang == "en" else "display-blogs-dunkel",
            "freunde": "display-friends" if lang == "en" else "display-freunde",
            "system": "display-system",
        }
        for page, name in display_names.items():
            src = rendered / f"480x320-dunkel-{page}.png"
            dest = docs / f"{name}.png"
            shutil.copyfile(src, dest)
            sources.append((dest, "current C++ renderer; fixed synthetic test fixture"))
        for page, name in {"ha": "ha_display", "system": "page_system", "blogs": "page_blogs",
                           "freunde": "page_friends", "bf6": "page_bf6"}.items():
            dest = images / f"{name}.png"
            shutil.copyfile(rendered / f"480x320-dunkel-{page}.png", dest)
            sources.append((dest, "current C++ renderer; fixed synthetic test fixture"))
        for name in NATIVE:
            src = images / f"{name}.png"
            if args.renderer_only or not src.is_file():
                missing.append(str(src.relative_to(ROOT)))
                continue
            dest = docs / f"settings-{'overview' if name == 'prev0' else name}.png"
            shutil.copyfile(src, dest)
            sources.extend(((src, "actual native window; isolated no-LCD instance; no accounts"),
                            (dest, "actual native window; isolated no-LCD instance; no accounts")))
    manifest = {
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "app_sha256": sha256(app),
        "renderer_sha256": sha256(renderer),
        "hardware_evidence": False,
        "note": "Native connection status belongs to --no-lcd. Display images are demo renderer exports, not device photographs.",
        "images": [{"path": str(p.relative_to(ROOT)).replace("\\", "/"),
                    "sha256": sha256(p), "source": source} for p, source in sources],
        "missing_native_captures": missing,
    }
    target = ROOT / "docs" / "screenshots" / "capture-manifest.json"
    target.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Prepared {len(sources)} image entries; {len(missing)} native captures missing")
    return 1 if missing and not args.renderer_only else 0


if __name__ == "__main__":
    raise SystemExit(main())
