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
