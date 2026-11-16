// © 2026 FuchsLabs
// Festes Frame-Raster für Animationen (rein, ohne Uhr und ohne Windows-Abhängigkeit, testbar).
// "cadence" ist der ideale Zeitpunkt des nächsten Frames. Er rückt immer um genau ein Intervall weiter,
// unabhängig davon, wie spät der Aufwachzeitpunkt war: Timer-Überschwinger akkumulieren nicht.
#pragma once

namespace fl::pacing {

// Wird nach dem Zeichnen eines Frames zum Zeitpunkt t aufgerufen; liefert die nächste Rasterzeit (= neue cadence).
// Im Raster (t höchstens ein Intervall nach der Rasterzeit, höchstens ein halbes davor) bleibt das Raster erhalten.
// Nach einem Stillstand, einem Neustart oder bei einem viel zu frühen Aufruf wird neu am Zeitpunkt t ausgerichtet.
inline double advance(double& cadence, double t, double interval) {
    const bool on_time = cadence > 0 && t >= cadence - interval * .5 && t - cadence < interval;
    cadence = (on_time ? cadence : t) + interval;
    return cadence;
}

// Frühester Zeitpunkt für den nächsten Frame, wenn eine Datenmeldung (dirty) während der Animation eintrifft:
// die Rasterzeit, nicht last_render + Intervall (das wäre um die Verspätung des letzten Frames später und
// würde den Jitter in das Raster tragen). Ohne gültiges Raster fällt es auf last_render + Intervall zurück.
inline double earliest_next(double cadence, double last_render, double interval) {
    return cadence > last_render && cadence <= last_render + interval ? cadence : last_render + interval;
}

}  // namespace fl::pacing
