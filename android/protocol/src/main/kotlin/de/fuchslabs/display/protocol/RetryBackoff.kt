package de.fuchslabs.display.protocol

/** Doubling retry delay with an upper bound; [reset] after a success. */
class RetryBackoff(private val initialMs: Long = 5_000L, private val maxMs: Long = 300_000L) {
    init { require(initialMs in 1..maxMs) }

    private var nextMs = initialMs

    /** Delay before the next attempt; each call doubles the following delay up to [maxMs]. */
    fun next(): Long {
        val delay = nextMs
        nextMs = if (nextMs > maxMs / 2) maxMs else nextMs * 2
        return delay
    }

    fun reset() { nextMs = initialMs }
}
