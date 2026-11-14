package de.fuchslabs.display.protocol

/**
 * [LatestSlot] whose values carry the generation (connection generation, display epoch) they belong to.
 *
 * The generation is stored together with the value at [offer] time, so a consumer that was scheduled
 * for an older generation can never mislabel a value of a newer one. A value of an older generation
 * never replaces a newer one: once a generation was seen (by [offer] or [advance]), offers of older
 * generations are dropped instead of stored.
 *
 * Scheduling contract as for [LatestSlot]: [offer] returns true only when the value was stored into an
 * empty slot; the producer then schedules exactly one delivery, which calls [take]. [take] may return
 * null (the value was discarded by [advance] or [clear] in the meantime); the delivery then does nothing.
 * Thread-safe; drop callbacks run outside the internal lock.
 */
class GenerationSlot<T : Any> {
    class Entry<T>(val generation: Int, val value: T)

    private val lock = Any()
    private var pending: Entry<T>? = null
    private var newest = Int.MIN_VALUE

    /**
     * Stores [value] for [generation]. Returns true when a delivery must be scheduled. A superseded, never
     * delivered value, or [value] itself when its generation is outdated, is passed to [onDropped].
     */
    fun offer(generation: Int, value: T, onDropped: (T) -> Unit = {}): Boolean {
        val dropped: T?
        val schedule: Boolean
        synchronized(lock) {
            if (generation < newest) { dropped = value; schedule = false }
            else {
                newest = generation
                val old = pending
                pending = Entry(generation, value)
                dropped = old?.value
                schedule = old == null
            }
        }
        if (dropped != null) onDropped(dropped)
        return schedule
    }

    /** Removes and returns the pending value with its generation, or null when nothing is pending. */
    fun take(): Entry<T>? = synchronized(lock) { pending.also { pending = null } }

    /**
     * Declares [generation] current: a pending value of an older generation is discarded (passed to
     * [onDropped]) and later offers of older generations are rejected.
     */
    fun advance(generation: Int, onDropped: (T) -> Unit = {}) {
        val dropped: T?
        synchronized(lock) {
            if (generation > newest) newest = generation
            val p = pending
            if (p != null && p.generation < newest) { pending = null; dropped = p.value } else dropped = null
        }
        if (dropped != null) onDropped(dropped)
    }

    /** Discards a pending value of any generation; it is passed to [onDropped]. */
    fun clear(onDropped: (T) -> Unit = {}) { take()?.let { onDropped(it.value) } }
}
