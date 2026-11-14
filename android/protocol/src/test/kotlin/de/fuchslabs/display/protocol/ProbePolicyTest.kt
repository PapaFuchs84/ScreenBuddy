package de.fuchslabs.display.protocol

import org.junit.Assert.*
import org.junit.Test

class ProbePolicyTest {
    @Test fun reachableProbeIsPresenceEvenWithoutMdns() {
        val p = ProbePolicy()
        assertEquals(true, p.onProbe(true))
    }

    @Test fun singleFailureIsNoConclusion() {
        val p = ProbePolicy()
        assertNull(p.onProbe(false))
    }

    @Test fun repeatedFailureWithoutMdnsConfirmsAbsence() {
        val p = ProbePolicy()
        assertNull(p.onProbe(false))
        assertEquals(false, p.onProbe(false))
    }

    @Test fun successResetsFailureCounter() {
        val p = ProbePolicy()
        assertNull(p.onProbe(false))
        assertEquals(true, p.onProbe(true))
        assertNull(p.onProbe(false))
    }

    @Test fun mdnsSightingOverridesProbeFailures() {
        val p = ProbePolicy()
        p.onMdns(true)
        repeat(5) { assertNull(p.onProbe(false)) }
    }

    @Test fun probeSkippedOnlyWhilePresentAndSeenByMdns() {
        val p = ProbePolicy()
        assertTrue(p.shouldProbe(false))
        p.onMdns(true)
        assertTrue(p.shouldProbe(false))
        assertFalse(p.shouldProbe(true))
        p.onMdns(false)
        assertTrue(p.shouldProbe(true))
    }

    @Test fun staleMdnsWhileScreenOffDoesNotSuppressProbeOrAbsence() {
        val p = ProbePolicy()
        p.onMdns(true)
        assertFalse(p.shouldProbe(true))
        assertTrue(p.shouldProbe(true, mdnsReliable = false))
        assertNull(p.onProbe(false, mdnsReliable = false))
        assertEquals(false, p.onProbe(false, mdnsReliable = false))
    }

    @Test fun mdnsLossOfPresentPcOnlyRequestsProbe() {
        val p = ProbePolicy()
        p.onMdns(true)
        assertFalse(p.lossNeedsProbe(true))
        p.onMdns(false)
        assertTrue(p.lossNeedsProbe(true))
        assertFalse(p.lossNeedsProbe(false)) // nothing to close while not present
        assertTrue(p.shouldProbe(true))
    }

    @Test fun reachablePcSurvivesMdnsLoss() {
        val p = ProbePolicy()
        val presence = StandbyPresence()
        assertEquals(true, p.onProbe(true))
        presence.update(true, 0L)
        p.onMdns(false) // discovery failed or announcement lost
        assertTrue(p.lossNeedsProbe(presence.present))
        assertEquals(true, p.onProbe(true))
        assertFalse(p.needsQuickRetry(presence.present))
        assertNull(presence.missingSinceMs) // no closing countdown was started
    }

    @Test fun countdownStartsOnlyAfterFailedProbeThreshold() {
        val p = ProbePolicy(3)
        val presence = StandbyPresence()
        presence.update(true, 0L)
        p.onMdns(false)
        assertFalse(p.needsQuickRetry(true)) // no failed probe yet
        assertNull(p.onProbe(false))
        assertTrue(p.needsQuickRetry(true))
        assertNull(p.onProbe(false))
        assertTrue(p.needsQuickRetry(true))
        assertNull(presence.missingSinceMs)
        assertEquals(false, p.onProbe(false))
        assertFalse(p.needsQuickRetry(true)) // threshold reached: the grace countdown takes over
        presence.update(false, 9_000L)
        assertEquals(20_000L, presence.remainingGraceMs(9_000L))
    }

    @Test fun quickRetryOnlyForPresentUnvouchedPc() {
        val p = ProbePolicy()
        assertNull(p.onProbe(false))
        assertFalse(p.needsQuickRetry(false)) // absent PC: the periodic probe is enough
        assertTrue(p.needsQuickRetry(true))
        p.onMdns(true)
        assertFalse(p.needsQuickRetry(true)) // mDNS vouches again
        assertTrue(p.needsQuickRetry(true, mdnsReliable = false))
    }

    @Test fun failuresWhileMdnsVouchesDoNotShortenThreshold() {
        val p = ProbePolicy()
        p.onMdns(true)
        repeat(5) { assertNull(p.onProbe(false)) }
        p.onMdns(false)
        assertNull(p.onProbe(false))
        assertEquals(false, p.onProbe(false))
    }

    @Test fun resetClearsState() {
        val p = ProbePolicy()
        p.onMdns(true)
        p.onProbe(false)
        p.reset()
        assertFalse(p.mdnsHere)
        assertNull(p.onProbe(false))
    }

    @Test(expected = IllegalArgumentException::class)
    fun zeroThresholdIsRejected() {
        ProbePolicy(0)
    }
}
