package de.fuchslabs.display.protocol

import org.json.JSONObject
import org.junit.Assert.*
import org.junit.Test

class ProtocolTest {
    private val fp = "ab".repeat(32)

    @Test fun pairingUriValid() {
        val p = PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=8777&fp=$fp&pin=123456")!!
        assertEquals("192.168.1.20", p.host); assertEquals(8777, p.port)
        assertEquals(fp, p.fingerprint); assertEquals("123456", p.pin)
    }
    @Test fun pairingUriColonFingerprintAndUppercase() {
        val colon = fp.uppercase().chunked(2).joinToString(":")
        assertEquals(fp, PairingUri.parse("fuchslabs://pair?host=h&port=1&fp=$colon&pin=000001")!!.fingerprint)
    }
    @Test fun pairingUriDefaultPort() {
        assertEquals(8777, PairingUri.parse("fuchslabs://pair?host=h&fp=$fp&pin=123456")!!.port)
    }
    @Test fun pairingUriRejectsBad() {
        assertNull(PairingUri.parse("https://pair?host=h&port=1&fp=$fp&pin=123456"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=h&port=1&fp=zz&pin=123456"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=h&port=1&fp=$fp&pin=12345"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=h&port=1&fp=$fp&pin=12345a"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=h&port=70000&fp=$fp&pin=123456"))
        assertNull(PairingUri.parse("fuchslabs://pair?port=1&fp=$fp&pin=123456"))
        assertNull(PairingUri.parse(""))
    }
    @Test fun serverMessages() {
        assertEquals(ServerMessage.Ready(480, 320), ServerMessage.parse("""{"t":"ready","w":480,"h":320}"""))
        assertEquals(ServerMessage.Paired("QUJD"), ServerMessage.parse("""{"t":"paired","key":"QUJD"}"""))
        assertEquals(ServerMessage.Error("pin","falsch"), ServerMessage.parse("""{"t":"error","code":"pin","msg":"falsch"}"""))
        assertEquals(ServerMessage.Ping, ServerMessage.parse("""{"t":"ping"}"""))
        assertEquals(ServerMessage.Pong, ServerMessage.parse("""{"t":"pong"}"""))
        assertEquals(ServerMessage.Unknown("zzz"), ServerMessage.parse("""{"t":"zzz"}"""))
    }
    @Test fun serverMessagesInvalid() {
        assertNull(ServerMessage.parse("nope"))
        assertNull(ServerMessage.parse("{}"))
        assertNull(ServerMessage.parse("""{"t":"ready","w":480}"""))
        assertNull(ServerMessage.parse("""{"t":"ready","w":0,"h":10}"""))
        assertNull(ServerMessage.parse("""{"t":"paired"}"""))
    }
    @Test fun clientHello() {
        val o = JSONObject(ClientMessage.helloWithPin("123456", 1920, 1080, "Tab \"A\""))
        assertEquals("hello", o.getString("t")); assertEquals(1, o.getInt("v"))
        assertEquals("123456", o.getString("pin")); assertFalse(o.has("key"))
        assertEquals("Tab \"A\"", o.getString("name"))
        val k = JSONObject(ClientMessage.helloWithKey("K", 1, 2, "x"))
        assertEquals("K", k.getString("key")); assertFalse(k.has("pin"))
    }
    @Test fun clientTouch() {
        val o = JSONObject(ClientMessage.touch(10, 20))
        assertEquals("touch", o.getString("t")); assertEquals(10, o.getInt("x")); assertEquals(20, o.getInt("y"))
    }
    @Test fun videoFrame() {
        val f = VideoFrame.parse(byteArrayOf(1, 0xFF.toByte(), 0xD8.toByte()))!!
        assertEquals(2, f.payload.size)
        assertNull(VideoFrame.parse(byteArrayOf(2, 1, 2)))
        assertNull(VideoFrame.parse(byteArrayOf(1)))
        assertNull(VideoFrame.parse(byteArrayOf()))
    }
    @Test fun letterboxMapping() {
        val lb = Letterbox.compute(960, 800, 480, 320) // scale 2, Balken oben/unten (80)
        assertEquals(2f, lb.scale, 0f); assertEquals(80f, lb.top, 0f)
        assertEquals(0 to 0, lb.toRender(0f, 80f))
        assertEquals(479 to 319, lb.toRender(959f, 719f))
        assertNull(lb.toRender(100f, 10f))
        assertEquals(240 to 160, lb.toRender(480f, 400f))
    }
}
