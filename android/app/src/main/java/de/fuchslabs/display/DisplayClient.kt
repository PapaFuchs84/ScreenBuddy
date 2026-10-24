package de.fuchslabs.display

import android.os.Handler
import android.os.Looper
import de.fuchslabs.display.protocol.*
import okhttp3.*
import okio.ByteString
import java.util.concurrent.TimeUnit
import javax.net.ssl.SSLException

/**
 * WebSocket-Client (OkHttp, TLS mit Fingerprint-Pinning). Alle Listener-Aufrufe erfolgen auf dem Main-Thread.
 * Auto-Reconnect mit Backoff (1..10 s), solange [start] aktiv und kein fataler Fehler aufgetreten ist.
 */
class DisplayClient(private val target: Target, private val listener: Listener) {

    /** [pin] gesetzt = Kopplung; sonst [key] = normale Anmeldung. [fingerprint] null = Erstkontakt (muss bestaetigt werden). */
    data class Target(
        val host: String, val port: Int, val fingerprint: String?,
        val pin: String? = null, val key: String? = null,
        val screenW: Int, val screenH: Int, val deviceName: String
    )

    interface Listener {
        fun onState(state: State)
        fun onUnverifiedFingerprint(fp: String, trust: () -> Unit)
        fun onPaired(key: String, fingerprint: String)
        fun onReady(w: Int, h: Int)
        fun onFrame(jpeg: ByteArray)
        fun onFatal(kind: Fatal, detail: String)
    }
    enum class State { CONNECTING, CONNECTED, RECONNECTING, STOPPED }
    enum class Fatal { SERVER_ERROR, CERT_MISMATCH }

    private val main = Handler(Looper.getMainLooper())
    private var ws: WebSocket? = null
    private var running = false
    private var attempt = 0
    private var generation = 0
    private var lastRx = 0L
    private var everConnected = false

    fun start() {
        running = true; attempt = 0
        connect()
    }

    fun stop() {
        running = false; generation++
        main.removeCallbacksAndMessages(null)
        ws?.close(1000, "bye"); ws = null
        listener.onState(State.STOPPED)
    }

    fun sendTouch(x: Int, y: Int) { ws?.send(ClientMessage.touch(x, y)) }

    private fun connect() {
        if (!running) return
        val gen = ++generation
        listener.onState(if (everConnected) State.RECONNECTING else State.CONNECTING)
        val tm = PinningTrustManager(target.fingerprint)
        val client = OkHttpClient.Builder()
            .sslSocketFactory(tm.socketFactory(), tm)
            .hostnameVerifier(PinningTrustManager.AnyHost)
            .connectTimeout(5, TimeUnit.SECONDS)
            .pingInterval(0, TimeUnit.SECONDS)
            .build()
        val host = if (target.host.contains(':') && !target.host.startsWith("[")) "[${target.host}]" else target.host
        val req = Request.Builder().url("wss://$host:${target.port}/").build()
        var paired = false
        ws = client.newWebSocket(req, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                main.post {
                    if (gen != generation) return@post
                    val fp = tm.seenFingerprint
                    if (target.fingerprint == null && fp != null) {
                        // Erstkontakt: Nutzer muss Fingerabdruck bestaetigen, bevor PIN gesendet wird.
                        listener.onUnverifiedFingerprint(fp) { if (gen == generation) sendHello(webSocket) }
                    } else sendHello(webSocket)
                }
            }
            override fun onMessage(webSocket: WebSocket, text: String) {
                main.post { if (gen == generation) handleText(webSocket, text, tm) }
            }
            override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
                val f = VideoFrame.parse(bytes.toByteArray()) ?: return
                main.post { if (gen == generation) { lastRx = now(); listener.onFrame(f.payload) } }
            }
            override fun onClosing(webSocket: WebSocket, code: Int, reason: String) { webSocket.close(1000, null) }
            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                main.post { if (gen == generation) retry() }
            }
            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                main.post {
                    if (gen != generation) return@post
                    if (t is SSLException && target.fingerprint != null) {
                        running = false
                        listener.onFatal(Fatal.CERT_MISMATCH, t.message ?: "")
                        listener.onState(State.STOPPED)
                    } else retry()
                }
            }
        })
        scheduleWatchdog(gen)
    }

    private fun sendHello(w: WebSocket) {
        val t = target
        val msg = if (t.pin != null) ClientMessage.helloWithPin(t.pin, t.screenW, t.screenH, t.deviceName)
        else ClientMessage.helloWithKey(t.key ?: "", t.screenW, t.screenH, t.deviceName)
        w.send(msg)
        lastRx = now()
    }

    private fun handleText(w: WebSocket, text: String, tm: PinningTrustManager) {
        lastRx = now()
        when (val m = ServerMessage.parse(text)) {
            is ServerMessage.Paired -> listener.onPaired(m.key, tm.seenFingerprint ?: target.fingerprint ?: "")
            is ServerMessage.Ready -> {
                attempt = 0; everConnected = true
                listener.onState(State.CONNECTED); listener.onReady(m.w, m.h)
            }
            is ServerMessage.Ping -> w.send(ClientMessage.pong())
            is ServerMessage.Error -> {
                running = false; generation++
                w.close(1000, null); ws = null
                listener.onFatal(Fatal.SERVER_ERROR, "${m.code}: ${m.msg}")
                listener.onState(State.STOPPED)
            }
            else -> {}
        }
    }

    /** Eigener 5-s-Ping und 15-s-Stille-Erkennung (Protokoll), unabhaengig von OkHttp-Frames. */
    private fun scheduleWatchdog(gen: Int) {
        main.postDelayed({
            if (gen != generation || !running) return@postDelayed
            if (now() - lastRx > Protocol.DEAD_AFTER_MS) { ws?.cancel(); retry(); return@postDelayed }
            ws?.send(ClientMessage.ping())
            scheduleWatchdog(gen)
        }, Protocol.PING_INTERVAL_MS)
    }

    private fun retry() {
        if (!running) return
        generation++ // alte Callbacks verwerfen
        ws?.cancel(); ws = null
        val delay = minOf(1000L * (1 shl minOf(attempt, 3)), 10_000L)
        attempt++
        listener.onState(State.RECONNECTING)
        main.postDelayed({ connect() }, delay)
    }

    private fun now() = System.currentTimeMillis()
}
