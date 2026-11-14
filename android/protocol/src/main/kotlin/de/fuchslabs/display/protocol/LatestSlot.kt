package de.fuchslabs.display.protocol

import java.util.concurrent.atomic.AtomicReference

/**
 * Bounded hand-over between a producer and one consumer thread: holds at most one undelivered value.
 *
 * A newer value replaces an undelivered older one, so a congested consumer never accumulates a queue.
 * [offer] returns true only when the slot was empty; the producer then schedules exactly one delivery,
 * which calls [take]. Whoever cancels scheduled deliveries must also [clear] the slot, otherwise the
 * next [offer] would wait for a delivery that no longer exists.
 */
class LatestSlot<T : Any> {
    private val ref = AtomicReference<T?>()

    /** Stores [value]. A superseded, never delivered value is passed to [onDropped]. */
    fun offer(value: T, onDropped: (T) -> Unit = {}): Boolean {
        val old = ref.getAndSet(value) ?: return true
        onDropped(old)
        return false
    }

    /** Removes and returns the pending value, or null when nothing is pending. */
    fun take(): T? = ref.getAndSet(null)

    /** Discards a pending value; it is passed to [onDropped]. */
    fun clear(onDropped: (T) -> Unit = {}) { take()?.let(onDropped) }
}
