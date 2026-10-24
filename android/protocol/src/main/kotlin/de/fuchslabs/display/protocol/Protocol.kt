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
        val host = q["host"]?.takeIf { it.isNotBlank() && it.length <= 253 } ?: return null
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
