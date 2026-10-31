package de.fuchslabs.display.protocol

/**
 * Bildet die D-Pad-Navigation einer Fernbedienung auf das vorhandene Wisch-Protokoll ab (reine,
 * testbare Logik ohne Android-Abhaengigkeit; die Key-Codes entsprechen android.view.KeyEvent).
 */
object DpadNavigation {
    const val KEYCODE_DPAD_LEFT = 21
    const val KEYCODE_DPAD_RIGHT = 22
    const val KEYCODE_DPAD_CENTER = 23
    const val KEYCODE_ENTER = 66

    /**
     * DPAD_RIGHT (vorwaerts/naechste Seite) entspricht einem Fingerwisch nach links (SwipeDir.LEFT);
     * DPAD_LEFT (zurueck/vorige Seite) entspricht SwipeDir.RIGHT. Die Namen von SwipeDir bezeichnen
     * die Wischrichtung des Fingers, nicht die resultierende Navigationsrichtung der Taste.
     * null = keine Navigationstaste.
     */
    fun toSwipe(keyCode: Int): SwipeDir? = when (keyCode) {
        KEYCODE_DPAD_RIGHT -> SwipeDir.LEFT
        KEYCODE_DPAD_LEFT -> SwipeDir.RIGHT
        else -> null
    }

    /**
     * CENTER/OK und ENTER werden bewusst ohne Aktion konsumiert: keine beliebige PC-Eingabe an die
     * Bildschirmmitte senden, solange keine definierte TV-Aktion dafuer existiert.
     */
    fun isConsumedWithoutAction(keyCode: Int): Boolean =
        keyCode == KEYCODE_DPAD_CENTER || keyCode == KEYCODE_ENTER
}
