package de.fuchslabs.display.protocol

/** Keeps a discovered PC present until its original absence grace period expires. */
class StandbyPresence(private val graceMs: Long = 20_000L) {
    init { require(graceMs >= 0L) }

    var present: Boolean = false
        private set
    var missingSinceMs: Long? = null
        private set

    /** Returns true only for a new presence, not a return within the grace period. */
    fun update(here: Boolean, nowMs: Long): Boolean {
        if (here) {
            val newlyPresent = !present
            present = true
            missingSinceMs = null
            return newlyPresent
        }
        if (present && missingSinceMs == null) missingSinceMs = nowMs
        return false
    }

    /** The original deadline is retained when discovery repeats an absence. */
    fun remainingGraceMs(nowMs: Long): Long? = missingSinceMs?.let {
        (graceMs - (nowMs - it).coerceAtLeast(0L)).coerceAtLeast(0L)
    }

    /** Returns true exactly once when a pending absence reaches its deadline. */
    fun expire(nowMs: Long): Boolean {
        if (!present || remainingGraceMs(nowMs) != 0L) return false
        reset()
        return true
    }

    fun reset() {
        present = false
        missingSinceMs = null
    }
}
