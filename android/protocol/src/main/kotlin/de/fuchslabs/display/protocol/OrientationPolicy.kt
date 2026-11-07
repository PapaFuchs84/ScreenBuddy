package de.fuchslabs.display.protocol

/** TV ignoriert die gespeicherte Auswahl, ohne sie zu verändern. Handy bleibt unverändert. */
fun orientationFor(isTv: Boolean, stored: String): String =
    if (isTv) "landscape" else stored
