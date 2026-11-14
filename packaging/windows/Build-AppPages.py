"""Build two self-contained bilingual app documentation pages; never deploy."""
from pathlib import Path
import hashlib
import html
import json
import re

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "website-pages"


def inline(value):
    value = html.escape(value, quote=True)
    value = re.sub(r"`([^`]+)`", r"<code>\1</code>", value)
    value = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", value)
    # Only the controlled source documents' HTTPS/mailto links become links.
    return re.sub(r"\[([^\]]+)\]\((https://[^\s)]+|mailto:[^\s)]+)\)",
                  r'<a href="\2">\1</a>', value)


def render(source):
    out, paragraph, in_list = [], [], False

    def flush():
        if paragraph:
            out.append("<p>" + inline(" ".join(paragraph)) + "</p>")
            paragraph.clear()

    for line in source.splitlines():
        if not line.strip():
            flush()
            if in_list:
                out.append("</ul>")
                in_list = False
        elif line.startswith("#"):
            flush()
            if in_list:
                out.append("</ul>")
                in_list = False
            count = len(line) - len(line.lstrip("#"))
            level = min(6, count + 1)
            out.append(f"<h{level}>" + inline(line[count:].strip()) + f"</h{level}>")
        elif line.startswith("- "):
            flush()
            if not in_list:
                out.append("<ul>")
                in_list = True
            out.append("<li>" + inline(line[2:]) + "</li>")
        else:
            paragraph.append(line)
    flush()
    if in_list:
        out.append("</ul>")
    return "\n".join(out)


CSS = """html{color-scheme:light dark}body{font:1rem/1.65 system-ui,sans-serif;margin:0;background:#17151b;color:#eeeaf2}main,header,footer{max-width:78ch;margin:auto;padding:1.5rem}a{color:#ffb56d}nav{display:flex;gap:1rem;flex-wrap:wrap}h1,h2,h3{line-height:1.25}h2{margin-top:2rem}section{scroll-margin-top:1rem;border-top:1px solid #5a5260;margin-top:2rem}code{overflow-wrap:anywhere}a:focus-visible{outline:3px solid #ffb56d;outline-offset:3px}li{margin:.5rem 0}@media print{body{color:#111;background:#fff}a{color:#111}nav{display:none}}"""

manifest = {"generatedFrom": "reviewed local Markdown, no deployment", "files": []}
for page, stem, title in [("privacy", "APP-PRIVACY", "Datenschutz · Privacy"),
                          ("support", "APP-SUPPORT", "Hilfe · Support")]:
    sections = []
    for lang in ("de", "en"):
        path = ROOT / f"{stem}.{lang}.md"
        content = path.read_text(encoding="utf-8")
        sections.append(f'<section id="{lang}" lang="{lang}">\n{render(content)}\n</section>')
        manifest["files"].append({"source": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    target = OUT / "screenbuddy" / page / "index.html"
    target.parent.mkdir(parents=True, exist_ok=True)
    document = f'''<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ScreenBuddy — {title}</title><meta name="description" content="ScreenBuddy app documentation in German and English"><style>{CSS}</style></head>
<body><header><nav aria-label="Navigation"><a href="/">FuchsLabs</a><a href="#de" lang="de">Deutsch</a><a href="#en" lang="en">English</a><a href="/screenbuddy/privacy/">Datenschutz / Privacy</a><a href="/screenbuddy/support/">Hilfe / Support</a></nav><h1>ScreenBuddy — {title}</h1></header><main>{''.join(sections)}</main><footer><a href="/impressum/">Impressum / Legal notice</a> · <a href="/datenschutz/">Website-Datenschutz / Website privacy</a></footer></body></html>
'''
    target.write_text(document, encoding="utf-8", newline="\n")
    manifest["files"].append({"output": target.relative_to(OUT).as_posix(), "sha256": hashlib.sha256(target.read_bytes()).hexdigest()})
(OUT / "provenance.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print("Built two standalone bilingual pages: " + str(OUT))
