package de.fuchslabs.display

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Handler
import android.os.Looper
import de.fuchslabs.display.protocol.Protocol

data class FoundPc(val serviceName: String, val name: String, val host: String, val port: Int)

/** mDNS-Erkennung von _fuchslabs-display._tcp ueber NsdManager (kein Play Services noetig). */
class Discovery(ctx: Context, private val onChange: (List<FoundPc>) -> Unit) {
    private val nsd = ctx.getSystemService(Context.NSD_SERVICE) as NsdManager
    private val main = Handler(Looper.getMainLooper())
    private val found = LinkedHashMap<String, FoundPc>()
    private val queue = ArrayDeque<NsdServiceInfo>()
    private var resolving = false
    private var active = false

    private val listener = object : NsdManager.DiscoveryListener {
        override fun onDiscoveryStarted(serviceType: String) {}
        override fun onDiscoveryStopped(serviceType: String) {}
        override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {}
        override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {}
        override fun onServiceFound(info: NsdServiceInfo) { main.post { queue.add(info); next() } }
        override fun onServiceLost(info: NsdServiceInfo) {
            main.post { found.remove(info.serviceName); onChange(found.values.toList()) }
        }
    }

    fun start() {
        if (active) return
        active = true
        try { nsd.discoverServices(Protocol.SERVICE_TYPE, NsdManager.PROTOCOL_DNS_SD, listener) } catch (_: Exception) { active = false }
    }

    fun stop() {
        if (!active) return
        active = false
        try { nsd.stopServiceDiscovery(listener) } catch (_: Exception) {}
        queue.clear(); resolving = false
    }

    @Suppress("DEPRECATION")
    private fun next() {
        if (resolving || queue.isEmpty()) return
        resolving = true
        val info = queue.removeFirst()
        try {
            nsd.resolveService(info, object : NsdManager.ResolveListener {
                override fun onResolveFailed(si: NsdServiceInfo, errorCode: Int) { main.post { resolving = false; next() } }
                override fun onServiceResolved(si: NsdServiceInfo) {
                    main.post {
                        val v = si.attributes["v"]?.toString(Charsets.UTF_8)
                        val addr = si.host?.hostAddress
                        if (addr != null && (v == null || v == "1")) {
                            val name = si.attributes["name"]?.toString(Charsets.UTF_8) ?: si.serviceName
                            found[si.serviceName] = FoundPc(si.serviceName, name, addr.substringBefore('%'), si.port)
                            onChange(found.values.toList())
                        }
                        resolving = false; next()
                    }
                }
            })
        } catch (_: Exception) { resolving = false }
    }
}
