package de.fuchslabs.display.protocol

import org.json.JSONObject
import org.junit.Assert.*
import org.junit.Test

class PageTransitionTest {
    private fun tx(style: TransitionStyle = TransitionStyle.HUD, ms: Int = 450) =
        ServerMessage.Transition(style, 1, ms, 0xFF00C8FF.toInt(), 0xFF0078FF.toInt(), 0xFF0E1628.toInt())

    // ---- Protokoll ----
    @Test fun helloCarriesCaps() {
        val o = JSONObject(ClientMessage.helloWithKey("K", 1, 2, "x"))
        assertEquals(1, o.getInt("v")) // Protokollversion unveraendert
        val caps = o.getJSONArray("caps")
        assertEquals(1, caps.length()); assertEquals("xfade1", caps.getString(0))
        assertTrue(JSONObject(ClientMessage.helloWithPin("123456", 1, 2, "x")).getJSONArray("caps").toString().contains("xfade1"))
        assertEquals(1, Protocol.VERSION)
    }

    @Test fun parsesTransition() {
        val m = ServerMessage.parse("""{"t":"transition","style":"slide","dir":-1,"ms":450,"accent":"#00C8FF","accent2":"#0078ff","shade":"#0E1628"}""")
        assertEquals(ServerMessage.Transition(TransitionStyle.SLIDE, -1, 450, 0xFF00C8FF.toInt(), 0xFF0078FF.toInt(), 0xFF0E1628.toInt()), m)
        assertEquals(TransitionStyle.HUD, (ServerMessage.parse("""{"t":"transition","style":"hud","dir":1,"ms":300}""") as ServerMessage.Transition).style)
        assertEquals(TransitionStyle.FADE, (ServerMessage.parse("""{"t":"transition","style":"fade","dir":1,"ms":1}""") as ServerMessage.Transition).style)
    }

    @Test fun transitionDefaultsForMissingColors() {
        val m = ServerMessage.parse("""{"t":"transition","style":"hud","dir":1,"ms":450}""") as ServerMessage.Transition
        assertEquals(0xFF00C8FF.toInt(), m.accent); assertEquals(0xFF0E1628.toInt(), m.shade)
    }

    @Test fun transitionBadFieldsRejected() {
        val bad = listOf(
            """{"t":"transition"}""",
            """{"t":"transition","style":"wipe","dir":1,"ms":450}""",
            """{"t":"transition","style":5,"dir":1,"ms":450}""",
            """{"t":"transition","style":"hud","dir":0,"ms":450}""",
            """{"t":"transition","style":"hud","dir":2,"ms":450}""",
            """{"t":"transition","style":"hud","dir":"1","ms":450}""",
            """{"t":"transition","style":"hud","dir":1}""",
            """{"t":"transition","style":"hud","dir":1,"ms":0}""",
            """{"t":"transition","style":"hud","dir":1,"ms":-5}""",
            """{"t":"transition","style":"hud","dir":1,"ms":999999}""",
            """{"t":"transition","style":"hud","dir":1,"ms":"450"}""",
            """{"t":"transition","style":"hud","dir":1,"ms":450,"accent":"red"}""",
            """{"t":"transition","style":"hud","dir":1,"ms":450,"accent":"#GG0000"}""",
            """{"t":"transition","style":"hud","dir":1,"ms":450,"shade":"#12345"}""",
            """{"t":"transition","style":"hud","dir":1,"ms":450,"accent2":null}""",
            """{"t":"transition","style":"hud","dir":1,"ms":450,"accent":12}""",
        )
        for (s in bad) assertNull(s, ServerMessage.parse(s))
    }

    @Test fun unknownTypesStillIgnoredAndTransitionOnlyAfterReady() {
        assertEquals(ServerMessage.Unknown("zzz"), ServerMessage.parse("""{"t":"zzz"}"""))
        val g = HandshakeGate(); g.onOpen(false)
        assertFalse(g.acceptText(tx())) // vor ready nicht
        assertTrue(g.acceptText(ServerMessage.Ready(480, 320)))
        assertTrue(g.acceptText(tx()))
        assertFalse(g.acceptText(ServerMessage.Unknown("zzz")))
        assertFalse(g.acceptText(null))
    }

    // ---- Easing / Geometrie ----
    private val eps = 1e-12
    @Test fun easingMatchesServerFormulas() {
        assertEquals(0.0, TransitionMath.easeCubic(0.0), eps); assertEquals(1.0, TransitionMath.easeCubic(1.0), eps)
        assertEquals(0.5, TransitionMath.easeCubic(.5), eps)
        assertEquals(4 * .25 * .25 * .25, TransitionMath.easeCubic(.25), eps)
        assertEquals(1 - Math.pow(-2 * .75 + 2, 3.0) / 2, TransitionMath.easeCubic(.75), eps)
        assertEquals(0.0, TransitionMath.easeQuint(0.0), eps); assertEquals(1.0, TransitionMath.easeQuint(1.0), eps)
        assertEquals(0.5, TransitionMath.easeQuint(.5), eps)
        assertEquals(16 * Math.pow(.25, 5.0), TransitionMath.easeQuint(.25), eps)
        assertEquals(1 - Math.pow(.5, 5.0) / 2, TransitionMath.easeQuint(.75), eps)
        // Quint ist am Anfang flacher als Cubic.
        assertTrue(TransitionMath.easeQuint(.25) < TransitionMath.easeCubic(.25))
        var prev = -1.0
        for (i in 0..100) { val v = TransitionMath.eased(TransitionStyle.HUD, i / 100.0); assertTrue(v >= prev); prev = v }
        assertEquals(TransitionMath.easeCubic(.3), TransitionMath.eased(TransitionStyle.FADE, .3), eps)
        assertEquals(TransitionMath.easeCubic(.3), TransitionMath.eased(TransitionStyle.SLIDE, .3), eps)
        assertEquals(TransitionMath.easeQuint(.3), TransitionMath.eased(TransitionStyle.HUD, .3), eps)
        assertEquals(0.0, TransitionMath.eased(TransitionStyle.HUD, -3.0), eps)
        assertEquals(1.0, TransitionMath.eased(TransitionStyle.SLIDE, 7.0), eps)
    }

    @Test fun timeBasedProgress() {
        assertEquals(0.0, TransitionMath.progress(1_000, 1_000, 450), eps)
        assertEquals(.5, TransitionMath.progress(0, 225_000_000L, 450), 1e-9)
        assertEquals(1.0, TransitionMath.progress(0, 900_000_000L, 450), eps)
        assertEquals(0.0, TransitionMath.progress(10, 5, 450), eps) // Zeit rueckwaerts
        assertEquals(1.0, TransitionMath.progress(0, 1, 0), eps)
        val c = TransitionClock(450)
        assertEquals(0.0, c.progress(5_000_000_000L), eps) // erster Frame = Start
        assertEquals(.5, c.progress(5_225_000_000L), 1e-9)
        assertTrue(c.finished(c.progress(5_450_000_000L)))
    }

    @Test fun slideOffsetAndSpans() {
        assertEquals(0, TransitionMath.slideOffset(480, 0.0))
        assertEquals(240, TransitionMath.slideOffset(480, .5))
        assertEquals(480, TransitionMath.slideOffset(480, 1.0))
        assertEquals(480, TransitionMath.slideOffset(480, 1.7))
        assertEquals(3, TransitionMath.slideOffset(5, .5)) // 2.5 -> 3 (std::lround rundet weg von 0)
    }

    @Test fun slideSpansForward() {
        val s = TransitionMath.slide(480, 1, .25) // d = 120
        assertEquals(120, s.d)
        assertEquals(120, s.oldSrcLeft); assertEquals(480, s.oldSrcRight); assertEquals(0, s.oldDstLeft)   // Altes rutscht nach links raus
        assertEquals(0, s.newSrcLeft); assertEquals(120, s.newSrcRight); assertEquals(360, s.newDstLeft)   // Neues kommt von rechts
        // luckenlos: alt endet dort, wo neu beginnt
        assertEquals(s.oldDstLeft + (s.oldSrcRight - s.oldSrcLeft), s.newDstLeft)
    }

    @Test fun slideSpansBackward() {
        val s = TransitionMath.slide(480, -1, .25)
        assertEquals(0, s.oldSrcLeft); assertEquals(360, s.oldSrcRight); assertEquals(120, s.oldDstLeft)   // Altes nach rechts
        assertEquals(360, s.newSrcLeft); assertEquals(480, s.newSrcRight); assertEquals(0, s.newDstLeft)   // Neues von links
        assertEquals(s.newDstLeft + (s.newSrcRight - s.newSrcLeft), s.oldDstLeft)
        val end = TransitionMath.slide(480, -1, 1.0)
        assertEquals(0, end.oldSrcRight - end.oldSrcLeft); assertEquals(480, end.newSrcRight - end.newSrcLeft)
    }

    @Test fun fadeAlpha() {
        assertEquals(0, TransitionMath.fadeAlpha(0.0)); assertEquals(255, TransitionMath.fadeAlpha(1.0))
        assertEquals(128, TransitionMath.fadeAlpha(.5)); assertEquals(255, TransitionMath.fadeAlpha(3.0))
    }

    @Test fun hudGeometryForward() {
        val start = TransitionMath.hud(480, 320, 0.0, 1)
        assertEquals(480, start.line); assertEquals(1, start.back)
        assertEquals(0, start.oldLo); assertEquals(480, start.oldHi); assertEquals(480, start.newLo); assertEquals(480, start.newHi)
        val mid = TransitionMath.hud(480, 320, .5, 1)
        assertEquals(240, mid.line); assertEquals(0, mid.oldLo); assertEquals(240, mid.oldHi); assertEquals(240, mid.newLo); assertEquals(480, mid.newHi)
        assertEquals(.46 * .9, mid.shadeAlpha, 1e-9)
        assertEquals(0.0, start.shadeAlpha, eps)
        assertEquals(.46, TransitionMath.hud(480, 320, 1.0, 1).shadeAlpha, 1e-9) // nie ganz dunkel
        assertEquals(6, mid.glow)      // round(480*.012)=6
        assertEquals(53, mid.trail)    // round(480*.11)=53
        assertEquals(24, mid.grid)
    }

    @Test fun hudGeometryBackward() {
        val mid = TransitionMath.hud(480, 320, .25, -1)
        assertEquals(120, mid.line); assertEquals(-1, mid.back)
        assertEquals(120, mid.oldLo); assertEquals(480, mid.oldHi); assertEquals(0, mid.newLo); assertEquals(120, mid.newHi)
    }

    @Test fun hudScalesWithResolution() {
        val g = TransitionMath.hud(1920, 1080, .5, 1)
        assertEquals(3.375, g.unit, 1e-9)   // min(1920/480, 1080/320)
        assertEquals(81, g.grid)            // round(24*3.375)
    }

    @Test fun colorParsing() {
        assertEquals(0xFF112233.toInt(), TransitionMath.parseColor("#112233"))
        assertEquals(0xFFAABBCC.toInt(), TransitionMath.parseColor("#aAbBcC"))
        for (s in listOf(null, "", "112233", "#11223", "#1122334", "#11223G", "# 12233")) assertNull(s, TransitionMath.parseColor(s))
    }

    // ---- Zuordnung Nachricht <-> Zielbild ----
    @Test fun carrierAttachesToNextFrame() {
        val c = TransitionCarrier(1000)
        val t = tx()
        assertNull(c.onFrame(0))
        c.onTransition(t, 100)
        assertSame(t, c.onFrame(200))
        assertTrue(c.delivered(t))
        assertNull(c.onFrame(300))
    }

    @Test fun carrierExpiresAfterTimeout() {
        val c = TransitionCarrier(1000)
        c.onTransition(tx(), 0)
        assertNull(c.onFrame(1001))
        c.onTransition(tx(), 0)
        assertNotNull(c.onFrame(1000))
    }

    @Test fun carrierKeepsMessageWhenNewerFrameOvertakes() {
        val c = TransitionCarrier()
        val t = tx()
        c.onTransition(t, 0)
        assertSame(t, c.onFrame(10)) // Zielbild B, noch nicht zugestellt
        assertSame(t, c.onFrame(20)) // neueres Bild C ueberholt B und erbt die Nachricht
        assertTrue(c.delivered(t))   // C wird zugestellt
        assertFalse(c.delivered(t))  // ein spaeter noch zugestelltes B animiert nicht erneut
        assertNull(c.onFrame(30))
    }

    @Test fun carrierClearDropsEverything() {
        val c = TransitionCarrier()
        c.onTransition(tx(), 0); c.clear()
        assertNull(c.onFrame(1))
        assertFalse(c.delivered(null))
    }

    @Test fun armingTargetsOnlyFramesAfterMessage() {
        val a = TransitionArming(1000)
        val t = tx()
        a.arm(t, 5, 0)
        assertNull(a.claim(4, 10))           // Altbild: normal zeigen, bleibt armiert
        assertTrue(a.armed)
        assertSame(t, a.claim(5, 20))
        assertFalse(a.armed)
        assertNull(a.claim(6, 30))           // nur einmal
    }

    @Test fun armingNewerFrameIsTarget() {
        val a = TransitionArming(1000)
        val t = tx(); a.arm(t, 5, 0)
        assertSame(t, a.claim(9, 100))
    }

    @Test fun armingExpires() {
        val a = TransitionArming(1000)
        a.arm(tx(), 1, 0)
        assertNull(a.claim(1, 1001)); assertFalse(a.armed)
        a.arm(tx(), 1, 0); a.clear(); assertNull(a.claim(1, 1))
    }

    // ---- Pin/Release ----
    private class B

    @Test fun pinnedItemsAreNotReleasedUntilUnpinned() {
        val p = PinTracker<B>()
        val from = B(); val to = B(); val other = B()
        p.pin(from); p.pin(to)
        assertEquals(2, p.pinCount)
        assertFalse(p.requestRelease(from))
        assertFalse(p.requestRelease(to))
        assertFalse(p.requestRelease(from)) // doppelt: nur einmal nachgereicht
        assertTrue(p.requestRelease(other))
        val deferred = p.unpinAll()
        assertEquals(2, deferred.size)
        assertTrue(deferred.any { it === from } && deferred.any { it === to })
        assertEquals(0, p.pinCount)
        assertTrue(p.requestRelease(from))  // jetzt frei
        assertTrue(p.unpinAll().isEmpty())
    }

    @Test fun pinUsesIdentityNotEquality() {
        val p = PinTracker<String>()
        val a = String(charArrayOf('x')); val b = String(charArrayOf('x'))
        p.pin(a)
        assertFalse(p.requestRelease(a))
        assertTrue(p.requestRelease(b))
    }

    @Test fun unreleasedWithoutRequestsReturnsNothing() {
        val p = PinTracker<B>(); val x = B()
        p.pin(x)
        assertTrue(p.unpinAll().isEmpty())
        assertFalse(p.isPinned(x))
    }
}
