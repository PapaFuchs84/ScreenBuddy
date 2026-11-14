package de.fuchslabs.display

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.net.wifi.WifiManager
import android.os.Handler
import android.os.Looper
import de.fuchslabs.display.protocol.HostPolicy
import de.fuchslabs.display.protocol.Protocol

data class FoundPc(val serviceName: String, val name: String, val host: String, val port: Int)

/**
 * mDNS-Erkennung von _fuchslabs-display._tcp ueber NsdManager (kein Play Services noetig).
 * [onRunning] meldet auf dem Main-Thread true, sobald das System die Suche gestartet hat, und false, wenn sie
 * ohne [stop] endet (Startfehler oder vom System beendet); danach ist [isRunning] false und [start] erneut moeglich.
 */
class Discovery(
    ctx: Context,
    private val onRunning: (Boolean) -> Unit = {},
    private val onChange: (List<FoundPc>) -> Unit
) {
    private val nsd = ctx.getSystemService(Context.NSD_SERVICE) as NsdManager
    private val wifi = ctx.applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
    private var multicastLock: WifiManager.MulticastLock? = null
    private val main = Handler(Looper.getMainLooper())
    private val found = LinkedHashMap<String, FoundPc>()
    private data class PendingService(val info: NsdServiceInfo, val token: Long)
    private val queue = ArrayDeque<PendingService>()
    private val services = mutableMapOf<String, Long>()
    private var resolving = false
    private var listener: NsdManager.DiscoveryListener? = null
    private var generation = 0L
    private var nextToken = 0L

    // NsdManager callbacks can arrive after stop(), or after a service was lost and
    // rediscovered. Both identities must still match before publishing a result.
    private fun isCurrent(run: Long) = listener != null && generation == run

    /** true zwischen [start] und [stop] bzw. bis die Suche fehlschlaegt oder vom System beendet wird. */
    val isRunning: Boolean get() = listener != null

    /** Liefert false, wenn die Suche nicht laeuft, weil der Start sofort fehlgeschlagen ist. */
    fun start(): Boolean {
        if (listener != null) return true
        val run = ++generation
        val callbacks = object : NsdManager.DiscoveryListener {
            override fun onDiscoveryStarted(serviceType: String) {
                main.post { if (isCurrent(run)) onRunning(true) }
            }
            override fun onDiscoveryStopped(serviceType: String) { ended() }
            override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) { ended() }
            override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {}
            private fun ended() {
                main.post {
                    if (!isCurrent(run)) return@post
                    clearRun()
                    onChange(emptyList())
                    onRunning(false)
                }
            }
            override fun onServiceFound(info: NsdServiceInfo) {
                main.post {
                    if (!isCurrent(run)) return@post
                    val token = ++nextToken
                    services[info.serviceName] = token
                    queue.add(PendingService(info, token))
                    next(run)
                }
            }
            override fun onServiceLost(info: NsdServiceInfo) {
                main.post {
                    if (!isCurrent(run)) return@post
                    services.remove(info.serviceName)
                    if (found.remove(info.serviceName) != null) onChange(found.values.toList())
                }
            }
        }
        listener = callbacks
        // Some devices filter mDNS again when the setup activity leaves the
        // foreground. Keep multicast reception available only during discovery.
        try {
            multicastLock = wifi?.createMulticastLock("fuchslabs:discovery")?.also {
                it.setReferenceCounted(false)
                it.acquire()
            }
        } catch (_: Exception) { multicastLock = null }
        return try {
            nsd.discoverServices(Protocol.SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, callbacks)
            true
        } catch (_: Exception) { clearRun(); false }
    }

    fun stop() {
        val callbacks = listener
        clearRun()
        if (callbacks != null) try { nsd.stopServiceDiscovery(callbacks) } catch (_: Exception) {}
    }

    private fun clearRun() {
        ++generation
        listener = null
        queue.clear(); services.clear(); found.clear(); resolving = false
        val lock = multicastLock
        multicastLock = null
        try { if (lock?.isHeld == true) lock.release() } catch (_: Exception) {}
    }

    @Suppress("DEPRECATION")
    private fun next(run: Long) {
        if (!isCurrent(run) || resolving) return
        while (queue.isNotEmpty() && services[queue.first().info.serviceName] != queue.first().token) queue.removeFirst()
        if (queue.isEmpty()) return
        resolving = true
        val pending = queue.removeFirst()
        val info = pending.info
        try {
            nsd.resolveService(info, object : NsdManager.ResolveListener {
                override fun onResolveFailed(si: NsdServiceInfo, errorCode: Int) {
                    main.post {
                        if (!isCurrent(run)) return@post
                        resolving = false; next(run)
                    }
                }
                override fun onServiceResolved(si: NsdServiceInfo) {
                    main.post {
                        if (!isCurrent(run)) return@post
                        if (services[info.serviceName] == pending.token) {
                            val v = si.attributes["v"]?.toString(Charsets.UTF_8)
                            val addr = si.host?.hostAddress
                            if (addr != null && HostPolicy.isAllowed(addr) && HostPolicy.isValidPort(si.port) && (v == null || v == "1")) {
                                val name = si.attributes["name"]?.toString(Charsets.UTF_8) ?: si.serviceName
                                found[info.serviceName] = FoundPc(info.serviceName, name, addr, si.port)
                                onChange(found.values.toList())
                            } else if (found.remove(info.serviceName) != null) onChange(found.values.toList())
                        }
                        if (isCurrent(run)) { resolving = false; next(run) }
                    }
                }
            })
        } catch (_: Exception) { resolving = false; next(run) }
    }
}
