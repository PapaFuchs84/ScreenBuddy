# © 2026 FuchsLabs
"""FuchsLabs-CI: Farben und das Fuchs-Logo (nur Flächen, daher bis 16 px scharf).

Hauptfarben sind Fuchs-Orange und Creme; Navy ist die Schrift- und Kontrastfarbe.
"""
from __future__ import annotations

from PIL import Image, ImageDraw

ORANGE = (247, 127, 0)       # Fuchs-Orange #F77F00 – Flächen, Kopfzeile, Balken, Logo-Grund
GLUT = (176, 82, 0)          # Glut #B05200 – Orange als Schrift auf Creme (gut lesbar)
CREME = (255, 244, 230)      # Creme #FFF4E6 – Hintergrund, Fuchskopf
SAND = (250, 228, 200)       # Sand #FAE4C8 – Karten auf Creme
LINE = (238, 210, 176)       # Trennlinien #EED2B0
NAVY = (20, 33, 61)          # Navy #14213D – Schrift, Augen; im Dunkelmodus Karten
NAVY_TIEF = (14, 22, 40)     # Navy tief #0E1628 – Hintergrund im Dunkelmodus
NAVY_LINIE = (39, 57, 95)    # Trennlinien im Dunkelmodus #27395F
NEBEL = (104, 96, 92)        # Nebel #68605C – Nebentext
PFIRSICH = (253, 214, 170)   # Pfirsich #FDD6AA – Fuchsbrust

# Fuchskopf im 64er-Raster
_HEAD = [(9, 7), (25, 19), (39, 19), (55, 7), (55, 31), (32, 57), (9, 31)]
_EARS = ([(13, 14), (22, 21), (13, 26)], [(51, 14), (42, 21), (51, 26)])
_CHEEKS = ([(9, 31), (27, 38), (32, 57)], [(55, 31), (37, 38), (32, 57)])
_INK = ([(28, 51), (36, 51), (32, 57)],                       # Nase
        [(21, 30), (25, 27), (29, 30), (25, 32)],             # Augen
        [(35, 30), (39, 27), (43, 30), (39, 32)])


def fox(size: int, badge: bool = True) -> Image.Image:
    """Creme-Fuchs; mit badge auf orangem, abgerundetem Quadrat (App-/Tray-Symbol)."""
    ss = 8
    s = size * ss
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    pad = 0.0
    if badge:
        d.rounded_rectangle((0, 0, s - 1, s - 1), radius=int(s * 0.22), fill=ORANGE)
        pad = 0.08 if size >= 24 else 0.03
    k, o = s * (1 - 2 * pad) / 64, s * pad

    def poly(pts, col):
        d.polygon([(o + x * k, o + y * k) for x, y in pts], fill=col)

    poly(_HEAD, CREME)
    if size >= 24:
        for e in _EARS:
            poly(e, GLUT)
    for c in _CHEEKS:
        poly(c, PFIRSICH)
    for p in _INK:
        poly(p, NAVY)
    return img.resize((size, size), Image.LANCZOS)


def save_ico(path) -> None:
    """Mehrgrößen-.ico; jede Größe einzeln gezeichnet, damit 16 px nicht verschwimmt."""
    sizes = (16, 24, 32, 48, 64, 128, 256)
    big = fox(256)
    big.save(path, format="ICO", sizes=[(n, n) for n in sizes],
             append_images=[fox(n) for n in sizes[:-1]])
