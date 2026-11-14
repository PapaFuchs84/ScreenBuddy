package de.fuchslabs.display.protocol

/**
 * Combines mDNS sightings with periodic TCP reachability probes of the paired PC.
 *
 * Some devices (e.g. Redmi 8 with the screen off) receive no mDNS while locked. A reachable
 * probe is therefore positive evidence on its own. A failed probe only counts as absence when
 * mDNS does not report the PC either and the failure repeated, so a single Wi-Fi hiccup can
 * never close a display that mDNS still sees.
 */
class ProbePolicy(private val failuresForAbsence: Int = 2) {
    init { require(failuresForAbsence >= 1) }

    var mdnsHere: Boolean = false
        private set
    private var failures = 0

    fun onMdns(here: Boolean) { mdnsHere = here }

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
        failures++
        return if (!(mdnsHere && mdnsReliable) && failures >= failuresForAbsence) false else null
    }

    fun reset() {
        mdnsHere = false
        failures = 0
    }
}
