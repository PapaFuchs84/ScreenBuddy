package de.fuchslabs.display.protocol

/**
 * Combines mDNS sightings with periodic TCP reachability probes of the paired PC.
 *
 * Some devices (e.g. Redmi 8 with the screen off) receive no mDNS while locked. A reachable
 * probe is therefore positive evidence on its own. A failed probe only counts as absence when
 * mDNS does not report the PC either and the failure repeated, so a single Wi-Fi hiccup can
 * never close a display that mDNS still sees.
 *
 * mDNS silence alone is never absence either: a lost announcement or a failed discovery only
 * requests TCP confirmation ([lossNeedsProbe]); the closing countdown starts when [onProbe]
 * returns false, i.e. after [failuresForAbsence] consecutive failed probes.
 */
class ProbePolicy(private val failuresForAbsence: Int = 2) {
    init { require(failuresForAbsence >= 1) }

    var mdnsHere: Boolean = false
        private set
    private var failures = 0

    /**
     * Records the mDNS state. A sighting that vouches for the PC ([here] and [mdnsReliable]) also
     * clears earlier probe failures, whether or not a probe runs while it lasts: a failure from
     * before the sighting must not count towards the threshold after a later mDNS loss.
     */
    fun onMdns(here: Boolean, mdnsReliable: Boolean = true) {
        mdnsHere = here
        if (here && mdnsReliable) failures = 0
    }

    /** True when mDNS no longer reports a PC that counts as present: confirm by TCP now. */
    fun lossNeedsProbe(present: Boolean): Boolean = present && !mdnsHere

    /**
     * Nothing to learn while the PC is already present and mDNS still sees it.
     * [mdnsReliable] is false while the screen is off: mDNS may then be blind, so a stale
     * sighting (or a missed goodbye) must not suppress probing.
     */
    fun shouldProbe(present: Boolean, mdnsReliable: Boolean = true): Boolean =
        !(present && mdnsHere && mdnsReliable)

    /** true = PC reachable (present), false = confirmed absent, null = no conclusion. */
    fun onProbe(reachable: Boolean, mdnsReliable: Boolean = true): Boolean? {
        if (reachable) { failures = 0; return true }
        // Failures only count while mDNS does not vouch for the PC: the threshold then always
        // means consecutive unvouched failures, never stale ones from an earlier phase.
        if (mdnsHere && mdnsReliable) { failures = 0; return null }
        failures++
        return if (failures >= failuresForAbsence) false else null
    }

    /**
     * True while a present PC has failed probes below the threshold: probe again soon instead
     * of leaving the display in doubt until the next periodic probe.
     */
    fun needsQuickRetry(present: Boolean, mdnsReliable: Boolean = true): Boolean =
        present && !(mdnsHere && mdnsReliable) && failures in 1 until failuresForAbsence

    fun reset() {
        mdnsHere = false
        failures = 0
    }
}
