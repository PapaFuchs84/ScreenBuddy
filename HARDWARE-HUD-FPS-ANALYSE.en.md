# Global animation: frame rate and limits

[Deutsch](HARDWARE-HUD-FPS-ANALYSE.md) · local validation dated 14 November 2026, no publication.

The previous five frames/s cap is not a requirement of the Companion protocol or Android app. No measured justification for that value is established in the original local commit. It primarily budgets full-frame JPEG encoding, transfer and decoding. A fresh Redmi v2 retest at that cap observed 49 UI redraws over 12.093 seconds: 4.05/s.

## Decision for local v3

The target now applies globally to **visible animation** on monitors and Companion: **30 frames/s**, configurable from **5 to 60** under **Display → Animations** (`display.animation_fps`). This includes HUD, active AI states/row transitions, playing Spotify progress/meters, and visible native, custom-view, Game Intro and plugin transitions. Static views retain source/display cadence and the normal Companion five frames/s transport cap. `system.hardware_hud_fps` is a migration alias only; an explicitly configured global value takes precedence. Global app/device validation remains pending.

Animated production is already paced. The fast path is therefore intended to avoid a redundant transport timer for trusted visible animation; this global extension still needs validation. Latest-frame storage, version deduplication, synchronous sending/backpressure and bounded shutdown remain; no unbounded frame queue is introduced. The target and FPS metadata are cadence/resource hints, not a guarantee or hard limit on delivered frames; mixed or otherwise dirty views may generate additional updates. Android app and wire format are unchanged. Sensor polling does not increase.

**30 is the chosen starting point for the global setting:** the earlier HUD-only transport candidate achieved about 21 UI redraws/s. A target of 60 was the smoothest measured setting at about 31/s, with higher CPU and bandwidth cost. 60 remains available; unrestricted output was not tested and is not established as a safe default.

## Historical HUD-only candidate: physical Redmi 8

This measurement is dated 13 November 2026 and covers only that HUD, not all newly global animated views. Android 14, 1520 × 720 pixels, separate debug test app, independent Companion output without another display output. Three twelve-second measurement windows with no screenshots or screen recording during measurement, aligned to the same GPU/CPU page phase.

| Target frames/s | Android UI redraws/s | Host CPU cores | Redmi CPU cores | TLS Mbit/s |
|---:|---:|---:|---:|---:|
| 20 | 16.06 | 0.414 | 0.848 | 13.090 |
| 30 | 21.31 | 0.439 | 1.029 | 17.456 |
| 60 | 31.38 | 0.693 | 1.261 | 26.196 |

One CPU-core equivalent means one continuously busy logical core, not a percentage of the whole machine. UI rate comes from Android `gfxinfo`; it is neither counted network-frame rate nor physical panel refresh. TLS traffic includes transport overhead and does not establish LAN capacity.

All three connections stayed active. Thermal status remained 0 and battery temperature rose from 27.0 to 27.2 °C during the short comparison. Android reported 4.39% jank at target 60; this deadline metric is not the stream's frame rate. PC background load was not eliminated. These are not long-term, battery or hardware-maximum measurements and do not establish achieved 60 frames/s.

Historical HUD-only candidate for this table: `F099702F29136203E44E20602A9813A844ED62B3B56466923704F761263CBFC4`. This table characterises that transport candidate. The later HUD-only build with default 30 passed 88 HUD and 3556 regression checks; 434 Companion checks passed on the same transport code. The final physical smoke test of build `96BC99AD4D179E8E0A8C9407415E5CB0C2238E1B83B31151664DE92408B18316` passed for all three pages in landscape (36.41 s) and portrait (35.80 s): live values, cycling, movement and no observed overlaps. The test configuration deliberately omitted `hardware_hud_fps`; the API confirmed compiled default 30. This is not a new timed frame-rate measurement.

Local report: [producer-paced/BENCHMARK-RESULT.md](redmi-validation/fps-benchmark-2026-11-13/producer-paced/BENCHMARK-RESULT.md).

## Earlier transport candidate: six targets

This separate run retained the additional transport timer. It is preserved as history and must not be presented as a final-path measurement. Background load and short individual windows do not support a strict causal comparison between variants.

| Target frames/s | Android UI redraws/s | Host CPU cores | Redmi CPU cores | TLS Mbit/s |
|---:|---:|---:|---:|---:|
| 5 | 3.97 | 0.113 | 0.232 | 3.26 |
| 10 | 7.97 | 0.137 | 0.458 | 6.48 |
| 15 | 10.65 | 0.275 | 0.592 | 8.66 |
| 20 | 15.85 | 0.263 | 0.846 | 12.91 |
| 30 | 16.82 | 0.444 | 0.891 | 13.84 |
| 60 | 31.11 | 0.607 | 1.262 | 25.52 |

Local report: [first comparison](redmi-validation/fps-benchmark-2026-11-13/BENCHMARK-RESULT.md).

## Mixed outputs and native USB: new cases still pending

Before additional separation, a static Companion page was redrawn unnecessarily by a fast primary HUD: approximately 5 render calls/s at primary target 5 and 17.5/s at target 60, while the metrics surface needed new data only about once/s. A separate 0.5-second metrics-view guard is intended to limit this; its global app validation is pending. Sensor polling does not increase.

A physical native Rev-A USB display (480 × 320) accepted controller writes without optical readback. Source target 5 yielded approximately 1.65 show calls/s, targets 30/60 each approximately 2.27/s; mean USB show time was 428–540 ms, with a measured partial-transfer rate around 170 kB/s. This does not establish optically displayed frames or full-frame 30/60 frames/s. The additional fixed HUD one-second pause is removed in favour of transfer-paced output plus a short idle pause; plugin transition budgets remain bounded. Actual global app cases are pending. The normal Windows app was briefly paused for the isolated raw-transfer test, then restored with confirmed display status.

AI/Spotify/game/other pages also use synthetic renderer/transport examples. Those examples do not establish real Spotify login/control or gameplay acceptance. Passing historical HUD checks and benchmarks do not constitute a passing global test.

## Scope

Both comparisons ran in the foreground with the screen awake, using USB ADB reverse and a temporary local relay. LAN, background, reboot and extended battery operation were not accepted. Production host access policy and normal Companion pairing were preserved; the test host and forwarding were stopped afterwards. Personal configurations, pairing data and raw logs must not be included in the test package.
