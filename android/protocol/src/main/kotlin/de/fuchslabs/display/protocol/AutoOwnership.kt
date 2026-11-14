package de.fuchslabs.display.protocol

/**
 * Who owns the visible display: the user (manual) or the automatic standby mode. Only an
 * automatically owned display may lead to locking the device when the PC disappears.
 */
object AutoOwnership {
    /**
     * Ownership after a further start intent reached the existing display instance.
     *
     * [auto]: the intent comes from the automatic mode. [wasAuto]: ownership so far.
     * [started]: the display is currently started (visible).
     *
     * A manual intent always makes the display manual. An automatic intent takes ownership only
     * when it actually brings the display forward; a display the user opened and still has on
     * screen stays manual.
     */
    fun afterNewIntent(auto: Boolean, wasAuto: Boolean, started: Boolean): Boolean =
        auto && (wasAuto || !started)
}
