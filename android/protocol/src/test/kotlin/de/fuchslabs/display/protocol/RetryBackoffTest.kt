package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test

class RetryBackoffTest {
    @Test fun doublesUpToTheBound() {
        val b = RetryBackoff(5_000L, 30_000L)
        assertEquals(listOf(5_000L, 10_000L, 20_000L, 30_000L, 30_000L), List(5) { b.next() })
    }

    @Test fun resetStartsOver() {
        val b = RetryBackoff(1_000L, 8_000L)
        b.next(); b.next(); b.next()
        b.reset()
        assertEquals(1_000L, b.next())
        assertEquals(2_000L, b.next())
    }

    @Test fun hugeBoundDoesNotOverflow() {
        val b = RetryBackoff(Long.MAX_VALUE / 2 + 1, Long.MAX_VALUE)
        assertEquals(Long.MAX_VALUE / 2 + 1, b.next())
        assertEquals(Long.MAX_VALUE, b.next())
        assertEquals(Long.MAX_VALUE, b.next())
    }

    @Test(expected = IllegalArgumentException::class)
    fun initialAboveMaxIsRejected() {
        RetryBackoff(10L, 5L)
    }

    @Test(expected = IllegalArgumentException::class)
    fun nonPositiveInitialIsRejected() {
        RetryBackoff(0L, 5L)
    }
}
