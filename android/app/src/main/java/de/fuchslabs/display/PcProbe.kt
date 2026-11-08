package de.fuchslabs.display

import de.fuchslabs.display.protocol.HostPolicy
import java.net.InetSocketAddress
import java.net.Socket

/**
 * Reiner TCP-Verbindungstest zum gekoppelten PC (kein TLS, keine PIN, es werden keine Daten gesendet).
 * Faengt den Fall ab, dass ein gesperrtes Geraet keine mDNS-Ankuendigungen mehr empfaengt.
 */
object PcProbe {
    fun reachable(host: String, port: Int, timeoutMs: Int = 2500): Boolean {
        if (!HostPolicy.isAllowed(host) || !HostPolicy.isValidPort(port)) return false
        return try {
            Socket().use { it.connect(InetSocketAddress(host, port), timeoutMs); true }
        } catch (_: Exception) { false }
    }
}
