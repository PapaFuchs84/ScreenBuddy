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
        assertEquals(fp, PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=1&fp=$colon&pin=000001")!!.fingerprint)
    }
    @Test fun pairingUriDefaultPort() {
        assertEquals(8777, PairingUri.parse("fuchslabs://pair?host=192.168.1.20&fp=$fp&pin=123456")!!.port)
    }
    @Test fun pairingUriRejectsBad() {
        assertNull(PairingUri.parse("https://pair?host=192.168.1.20&port=1&fp=$fp&pin=123456"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=1&fp=zz&pin=123456"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=1&fp=$fp&pin=12345"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=1&fp=$fp&pin=12345a"))
        assertNull(PairingUri.parse("fuchslabs://pair?host=192.168.1.20&port=70000&fp=$fp&pin=123456"))
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

    @Test fun hostPolicyAllowsPrivateIpv4Only() {
        for (h in listOf("10.0.0.1", "172.16.0.1", "172.31.255.254", "192.168.1.20"))
            assertTrue(h, HostPolicy.isAllowed(h))
        for (h in listOf("8.8.8.8", "172.15.0.1", "172.32.0.1", "169.254.1.1", "127.0.0.1", "192.169.0.1",
            "example.com", "pc.local", "::1", "fe80::1", "[::1]", "10.0.0", "10.0.0.256", "010.0.0.1",
            "10.0.0.1.5", "10.0.0.1:80", "10.0.0.-1", "", " ", "0x0a.0.0.1", "2130706433"))
            assertFalse(h, HostPolicy.isAllowed(h))
        assertFalse(HostPolicy.isAllowed(null))
    }
    @Test fun pairingUriRejectsNonPrivateHosts() {
        for (h in listOf("example.com", "8.8.8.8", "%3A%3A1", "169.254.1.1"))
            assertNull(h, PairingUri.parse("fuchslabs://pair?host=$h&port=1&fp=$fp&pin=123456"))
    }
    @Test fun imageLimits() {
        assertTrue(ImageLimits.bytesOk(1)); assertTrue(ImageLimits.bytesOk(2 * 1024 * 1024))
        assertFalse(ImageLimits.bytesOk(0)); assertFalse(ImageLimits.bytesOk(2 * 1024 * 1024 + 1))
        assertTrue(ImageLimits.dimsOk(2048, 2048)); assertFalse(ImageLimits.dimsOk(2049, 10))
        assertFalse(ImageLimits.dimsOk(0, 10)); assertFalse(ImageLimits.dimsOk(10, -1))
    }

    private val ready = ServerMessage.Ready(480, 320)
    private val paired = ServerMessage.Paired("k")

    @Test fun gateBlocksEverythingBeforeTrust() {
        val g = HandshakeGate()
        assertFalse(g.acceptText(ready)) // vor onOpen
        assertFalse(g.onOpen(needsTrust = true))
        assertEquals(HandshakeGate.Phase.AWAITING_TRUST, g.phase)
        assertFalse(g.acceptText(paired)); assertFalse(g.acceptText(ready))
        assertFalse(g.acceptText(ServerMessage.Ping)); assertFalse(g.acceptFrame())
        assertFalse(g.watchdogActive())
        assertEquals(HandshakeGate.Phase.AWAITING_TRUST, g.phase)
    }
    @Test fun gateTrustOnlyOnceAndThenFlows() {
        val g = HandshakeGate()
        g.onOpen(true)
        assertTrue(g.trust()); assertFalse(g.trust())
        assertTrue(g.watchdogActive()); assertFalse(g.acceptFrame()) // Frames erst nach ready
        assertTrue(g.acceptText(paired))
        assertTrue(g.acceptText(ready)); assertTrue(g.acceptFrame())
    }
    @Test fun gateKnownFingerprintSendsHelloImmediately() {
        val g = HandshakeGate()
        assertTrue(g.onOpen(false))
        assertFalse(g.trust())
        assertFalse(g.onOpen(true)) // zweites onOpen wirkungslos
        assertEquals(HandshakeGate.Phase.HELLO_SENT, g.phase)
    }
    @Test fun gateRejectsPairedAfterReadyAndFramesWhenClosed() {
        val g = HandshakeGate()
        g.onOpen(false); g.acceptText(ready)
        assertFalse(g.acceptText(paired))
        g.close(); assertFalse(g.acceptFrame()); assertFalse(g.acceptText(ServerMessage.Ping)); assertFalse(g.watchdogActive())
    }
}
