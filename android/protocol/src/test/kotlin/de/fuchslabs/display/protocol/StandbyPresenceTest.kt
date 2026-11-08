package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test

class StandbyPresenceTest {
    @Test fun firstSightOpensOnce() {
        val state = StandbyPresence()
        assertTrue(state.update(true, 100L))
        assertTrue(state.present)
        assertFalse(state.update(true, 200L))
        assertNull(state.missingSinceMs)
        assertNull(state.remainingGraceMs(200L))
    }

    @Test fun repeatedAbsenceDoesNotExtendOriginalDeadline() {
        val state = StandbyPresence()
        state.update(true, 100L)
        state.update(false, 1_000L)
        state.update(false, 19_000L)
        assertEquals(1_000L, state.missingSinceMs)
        assertEquals(2_000L, state.remainingGraceMs(19_000L))
        assertFalse(state.expire(20_999L))
        assertTrue(state.expire(21_000L))
    }

    @Test fun returnWithinGraceCancelsClosureWithoutDuplicateOpen() {
        val state = StandbyPresence()
        state.update(true, 0L)
        state.update(false, 100L)
        assertFalse(state.update(true, 20_099L))
        assertNull(state.missingSinceMs)
        assertNull(state.remainingGraceMs(20_100L))
        assertFalse(state.expire(20_100L))
        assertTrue(state.present)
        state.update(false, 30_000L)
        assertEquals(20_000L, state.remainingGraceMs(30_000L))
    }

    @Test fun expiryClosesOnceAndNextArrivalOpens() {
        val state = StandbyPresence()
        state.update(true, 0L)
        state.update(false, 100L)
        assertEquals(0L, state.remainingGraceMs(25_000L))
        assertTrue(state.expire(25_000L))
        assertFalse(state.present)
        assertNull(state.missingSinceMs)
        assertFalse(state.expire(30_000L))
        assertTrue(state.update(true, 30_001L))
    }

    @Test fun neverPresentDoesNotScheduleOrClose() {
        val state = StandbyPresence()
        assertFalse(state.update(false, 0L))
        assertFalse(state.update(false, 50_000L))
        assertNull(state.missingSinceMs)
        assertNull(state.remainingGraceMs(50_000L))
        assertFalse(state.expire(100_000L))
    }

    @Test fun resetCancelsPendingDeadlineAndAllowsFreshOpen() {
        val state = StandbyPresence()
        state.update(true, 0L)
        state.update(false, 100L)
        state.reset()
        assertFalse(state.present)
        assertNull(state.remainingGraceMs(30_000L))
        assertFalse(state.expire(30_000L))
        assertTrue(state.update(true, 30_001L))
    }

    @Test fun zeroGraceExpiresAtFirstAbsence() {
        val state = StandbyPresence(0L)
        state.update(true, 100L)
        state.update(false, 200L)
        assertTrue(state.expire(200L))
    }

    @Test(expected = IllegalArgumentException::class)
    fun negativeGraceIsRejected() {
        StandbyPresence(-1L)
    }
}
