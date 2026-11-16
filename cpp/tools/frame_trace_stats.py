#!/usr/bin/env python3
"""Auswertung der Companion-Frame-Telemetrie (SB_FRAME_TRACE=<datei.csv>).

Aufruf:  python frame_trace_stats.py trace.csv [--gap-ms 250] [--device ID]

Ein "Übergang" (Animationslauf) ist eine zusammenhängende Folge von Frames mit animating=1 desselben Geräts,
deren Abstand höchstens --gap-ms beträgt. Je Gerät werden ausgegeben: Frame-Abstände (p50/p95/p99/max in ms,
nur innerhalb von Animationsläufen), Frames je Übergang, effektive fps (Frames-1 / Dauer) sowie die mittleren
Render-/Encode-/Sendedauern. Nur Standardbibliothek.
"""
import argparse
import csv
import sys
from collections import defaultdict

COLUMNS = ("ts_us", "device", "seq", "render_us", "encode_us", "send_us", "bytes", "gap_us", "animating")


def percentile(values, p):
    """Lineare Interpolation (wie numpy.percentile, 'linear'); p in 0..100."""
    if not values:
        return float("nan")
    ordered = sorted(values)
    if len(ordered) == 1:
        return float(ordered[0])
    pos = (len(ordered) - 1) * p / 100.0
    low = int(pos)
    high = min(low + 1, len(ordered) - 1)
    return ordered[low] + (ordered[high] - ordered[low]) * (pos - low)


def load(path):
    rows = []
    with open(path, newline="", encoding="utf-8") as handle:
        for line_no, rec in enumerate(csv.DictReader(handle), start=2):
            try:
                rows.append({
                    "ts": int(rec["ts_us"]), "device": rec["device"], "seq": int(rec["seq"]),
                    "render": int(rec["render_us"]), "encode": int(rec["encode_us"]), "send": int(rec["send_us"]),
                    "bytes": int(rec["bytes"]), "animating": rec["animating"].strip() == "1",
                })
            except (KeyError, ValueError, TypeError):
                print(f"Zeile {line_no} übersprungen (ungültig)", file=sys.stderr)
    return rows


def split_runs(rows, max_gap_us):
    """Zerlegt die Zeilen eines Geräts (nach ts sortiert) in Animationsläufe."""
    runs, current = [], []
    for row in sorted(rows, key=lambda r: r["ts"]):
        if not row["animating"]:
            if current:
                runs.append(current)
                current = []
            continue
        if current and row["ts"] - current[-1]["ts"] > max_gap_us:
            runs.append(current)
            current = []
        current.append(row)
    if current:
        runs.append(current)
    return runs


def analyse(rows, max_gap_us):
    """Kennzahlen für die Zeilen eines Geräts; liefert ein dict (gaps in ms)."""
    runs = split_runs(rows, max_gap_us)
    gaps = [(b["ts"] - a["ts"]) / 1000.0 for run in runs for a, b in zip(run, run[1:])]
    spans = [(run[-1]["ts"] - run[0]["ts"]) / 1e6 for run in runs if len(run) > 1]
    frames = sum(len(run) for run in runs)
    animated = [r for run in runs for r in run]
    total_span = sum(spans)
    mean = lambda key: (sum(r[key] for r in animated) / len(animated) / 1000.0) if animated else float("nan")
    return {
        "runs": len(runs),
        "frames": frames,
        "frames_per_run": frames / len(runs) if runs else float("nan"),
        "gap_p50": percentile(gaps, 50), "gap_p95": percentile(gaps, 95), "gap_p99": percentile(gaps, 99),
        "gap_max": max(gaps) if gaps else float("nan"),
        "fps": (sum(len(run) - 1 for run in runs if len(run) > 1) / total_span) if total_span > 0 else float("nan"),
        "render_ms": mean("render"), "encode_ms": mean("encode"), "send_ms": mean("send"),
        "kb": (sum(r["bytes"] for r in animated) / len(animated) / 1024.0) if animated else float("nan"),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv_file")
    parser.add_argument("--gap-ms", type=float, default=250.0, help="größter Abstand innerhalb eines Übergangs (Standard 250)")
    parser.add_argument("--device", help="nur dieses Gerät")
    args = parser.parse_args(argv)

    rows = load(args.csv_file)
    by_device = defaultdict(list)
    for row in rows:
        if args.device is None or row["device"] == args.device:
            by_device[row["device"]].append(row)
    if not by_device:
        print("Keine Daten.")
        return 1
    for device, device_rows in sorted(by_device.items()):
        stats = analyse(device_rows, args.gap_ms * 1000.0)
        print(f"Gerät {device}: {len(device_rows)} Frames gesamt, {stats['frames']} davon animiert in {stats['runs']} Übergängen")
        if not stats["runs"]:
            continue
        print(f"  Frames je Übergang: {stats['frames_per_run']:.1f}   effektive fps: {stats['fps']:.1f}")
        print(f"  Abstand [ms]: p50 {stats['gap_p50']:.1f}  p95 {stats['gap_p95']:.1f}  p99 {stats['gap_p99']:.1f}  max {stats['gap_max']:.1f}")
        print(f"  Mittel [ms]: Render {stats['render_ms']:.2f}  Encode {stats['encode_ms']:.2f}  Senden {stats['send_ms']:.2f}  ({stats['kb']:.1f} kB/Frame)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
