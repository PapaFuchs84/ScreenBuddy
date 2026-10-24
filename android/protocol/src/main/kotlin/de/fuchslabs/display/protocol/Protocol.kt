package de.fuchslabs.display.protocol

import org.json.JSONException
import org.json.JSONObject

/** Companion-Protokoll v1 (docs/COMPANION-PROTOKOLL.md). */
object Protocol {
    const val VERSION = 1
    const val DEFAULT_PORT = 8777
    const val SERVICE_TYPE = "_fuchslabs-display._tcp"
    const val PING_INTERVAL_MS = 5_000L
    const val DEAD_AFTER_MS = 15_000L
    const val FRAME_JPEG: Int = 1
}

/** Inhalt des Kopplungs-QR-Codes: fuchslabs://pair?host=..&port=..&fp=..&pin=.. */
data class PairingInfo(val host: String, val port: Int, val fingerprint: String, val pin: String)

object Fingerprint {
    /** Normalisiert "AB:CD:.." / "abcd.." zu 64 Zeichen Kleinbuchstaben-Hex; null wenn ungueltig. */
    fun normalize(raw: String?): String? {
        val s = raw?.trim()?.replace(":", "")?.replace(" ", "")?.lowercase() ?: return null
        return if (s.length == 64 && s.all { it in '0'..'9' || it in 'a'..'f' }) s else null
    }
}

object PairingUri {
    /** Parst die QR-Nutzlast; null wenn Schema/Felder ungueltig. Kein java.net.URI (tolerant gegenueber IPv6/Sonderzeichen). */
    fun parse(text: String): PairingInfo? {
        val prefix = "fuchslabs://pair?"
        val t = text.trim()
        if (!t.startsWith(prefix, ignoreCase = true)) return null
        val q = HashMap<String, String>()
        for (part in t.substring(prefix.length).split('&')) {
            val i = part.indexOf('=')
            if (i <= 0) continue
            q[part.substring(0, i)] = percentDecode(part.substring(i + 1)) ?: return null
        }
        val host = q["host"]?.trim()?.takeIf { HostPolicy.isAllowed(it) } ?: return null
        val port = (q["port"] ?: Protocol.DEFAULT_PORT.toString()).toIntOrNull()?.takeIf { it in 1..65535 } ?: return null
        val fp = Fingerprint.normalize(q["fp"]) ?: return null
        val pin = q["pin"]?.takeIf { it.length == 6 && it.all(Char::isDigit) } ?: return null
        return PairingInfo(host, port, fp, pin)
    }

    private fun percentDecode(s: String): String? = try {
        java.net.URLDecoder.decode(s.replace("+", "%2B"), "UTF-8")
    } catch (e: IllegalArgumentException) { null }
}

/** Nachrichten Server -> Client. */
sealed class ServerMessage {
    data class Ready(val w: Int, val h: Int) : ServerMessage()
    data class Paired(val key: String) : ServerMessage()
    data class Error(val code: String, val msg: String) : ServerMessage()
    object Ping : ServerMessage()
    object Pong : ServerMessage()
    data class Unknown(val type: String) : ServerMessage()

    companion object {
        /** Parst einen Text-Frame; null bei ungueltigem JSON oder fehlenden Pflichtfeldern. */
        fun parse(text: String): ServerMessage? = try {
            val o = JSONObject(text)
            when (val t = o.optString("t", "")) {
                "ready" -> {
                    val w = o.getInt("w"); val h = o.getInt("h")
                    if (w in 1..16384 && h in 1..16384) Ready(w, h) else null
                }
                "paired" -> o.getString("key").takeIf { it.isNotEmpty() }?.let { Paired(it) }
                "error" -> Error(o.optString("code", "unknown"), o.optString("msg", ""))
                "ping" -> Ping
                "pong" -> Pong
                "" -> null
                else -> Unknown(t)
            }
        } catch (e: JSONException) { null }
    }
}

/** Nachrichten Client -> Server (als JSON-Text). */
object ClientMessage {
    fun helloWithPin(pin: String, w: Int, h: Int, name: String): String =
        hello(w, h, name).put("pin", pin).toString()

    fun helloWithKey(key: String, w: Int, h: Int, name: String): String =
        hello(w, h, name).put("key", key).toString()

    fun touch(x: Int, y: Int): String = JSONObject().put("t", "touch").put("x", x).put("y", y).toString()
    fun ping(): String = """{"t":"ping"}"""
    fun pong(): String = """{"t":"pong"}"""

    private fun hello(w: Int, h: Int, name: String) =
        JSONObject().put("t", "hello").put("v", Protocol.VERSION).put("w", w).put("h", h).put("name", name)
}

/** Binary-Frame: Byte 0 = Typ (1 = JPEG), Rest = Nutzdaten. */
class VideoFrame(val type: Int, val payload: ByteArray) {
    companion object {
        fun parse(data: ByteArray): VideoFrame? =
            if (data.size < 2 || (data[0].toInt() and 0xFF) != Protocol.FRAME_JPEG) null
            else VideoFrame(Protocol.FRAME_JPEG, data.copyOfRange(1, data.size))
    }
}

/** Letterbox-Geometrie und Abbildung View-Pixel -> Renderkoordinaten. */
data class Letterbox(val left: Float, val top: Float, val scale: Float, val rw: Int, val rh: Int) {
    /** Liefert (x,y) in 0..w-1/0..h-1 oder null, wenn ausserhalb des Bildes. */
    fun toRender(px: Float, py: Float): Pair<Int, Int>? {
        val x = (px - left) / scale
        val y = (py - top) / scale
        if (x < 0f || y < 0f || x >= rw || y >= rh) return null
        return x.toInt().coerceIn(0, rw - 1) to y.toInt().coerceIn(0, rh - 1)
    }

    companion object {
        fun compute(viewW: Int, viewH: Int, rw: Int, rh: Int): Letterbox {
            val s = minOf(viewW.toFloat() / rw, viewH.toFloat() / rh)
            return Letterbox((viewW - rw * s) / 2f, (viewH - rh * s) / 2f, s, rw, rh)
        }
    }
}

/** Zielpolicy: nur literale private IPv4 (10/8, 172.16/12, 192.168/16). Keine DNS-Namen, kein IPv6, keine oeffentlichen IPs. */
object HostPolicy {
    /** Strikte Dotted-Quad-Syntax (4 Dezimalzahlen 0..255, keine fuehrenden Nullen); null wenn ungueltig. */
    fun parseIpv4(s: String): IntArray? {
        val parts = s.split('.')
        if (parts.size != 4) return null
        val out = IntArray(4)
        for ((i, p) in parts.withIndex()) {
            if (p.isEmpty() || p.length > 3 || !p.all { it in '0'..'9' } || (p.length > 1 && p[0] == '0')) return null
            val v = p.toInt()
            if (v > 255) return null
            out[i] = v
        }
        return out
    }

    fun isAllowed(host: String?): Boolean {
        val a = parseIpv4(host ?: return false) ?: return false
        return a[0] == 10 || (a[0] == 172 && a[1] in 16..31) || (a[0] == 192 && a[1] == 168)
    }

    fun isValidPort(port: Int) = port in 1..65535
}

/** Grenzen fuer eingehende JPEG-Frames (Schutz vor Speicher-/Decompression-Bombs). */
object ImageLimits {
    const val MAX_BYTES = 2 * 1024 * 1024
    const val MAX_DIM = 2048

    fun bytesOk(size: Int) = size in 1..MAX_BYTES
    fun dimsOk(w: Int, h: Int) = w in 1..MAX_DIM && h in 1..MAX_DIM
}

/**
 * Zustandsautomat der Verbindung. Vor der Nutzerfreigabe eines Erstkontakt-Fingerabdrucks (AWAITING_TRUST)
 * wird keine Servernachricht und kein Frame verarbeitet; Frames erst nach "ready".
 */
class HandshakeGate {
    enum class Phase { CONNECTING, AWAITING_TRUST, HELLO_SENT, READY, CLOSED }

    var phase = Phase.CONNECTING
        private set

    /** TLS/WebSocket offen. true = hello sofort senden; false = Nutzerfreigabe abwarten. */
    fun onOpen(needsTrust: Boolean): Boolean {
        if (phase != Phase.CONNECTING) return false
        phase = if (needsTrust) Phase.AWAITING_TRUST else Phase.HELLO_SENT
        return !needsTrust
    }

    /** Nutzer hat bestaetigt. true nur genau einmal aus AWAITING_TRUST (dann hello senden). */
    fun trust(): Boolean {
        if (phase != Phase.AWAITING_TRUST) return false
        phase = Phase.HELLO_SENT
        return true
    }

    fun close() { phase = Phase.CLOSED }

    /** Darf diese Servernachricht verarbeitet werden? Aendert bei Ready die Phase. */
    fun acceptText(m: ServerMessage?): Boolean = when (phase) {
        Phase.HELLO_SENT -> when (m) {
            is ServerMessage.Ready -> { phase = Phase.READY; true }
            is ServerMessage.Paired, is ServerMessage.Error, ServerMessage.Ping, ServerMessage.Pong -> true
            else -> false
        }
        Phase.READY -> m is ServerMessage.Ping || m is ServerMessage.Pong || m is ServerMessage.Error || m is ServerMessage.Ready
        else -> false
    }

    fun acceptFrame() = phase == Phase.READY

    /** Watchdog (Ping/Stille-Erkennung) ist waehrend des Trust-Prompts pausiert. */
    fun watchdogActive() = phase == Phase.HELLO_SENT || phase == Phase.READY
}
