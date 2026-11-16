package de.fuchslabs.display.protocol

import java.util.Collections
import java.util.IdentityHashMap
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow

/** Uebergangsstil der Servernachricht {"t":"transition"}. */
enum class TransitionStyle(val wire: String) {
    HUD("hud"), SLIDE("slide"), FADE("fade");

    companion object { fun fromWire(s: String): TransitionStyle? = values().firstOrNull { it.wire == s } }
}

/** Faehigkeit, die der Client im hello meldet: er animiert Seitenuebergaenge selbst. */
const val CAP_XFADE1 = "xfade1"

/**
 * Reine Animationsmathematik; spiegelt cpp/src/render.cpp (page_transition) des Servers.
 * Alle Geometrien in Bildpixeln des Renderbildes.
 */
object TransitionMath {
    fun clamp01(p: Double): Double = if (p.isNaN() || p <= 0.0) 0.0 else if (p >= 1.0) 1.0 else p

    /** ease-in-out cubic (Slide, Fade). */
    fun easeCubic(p: Double): Double = if (p < .5) 4 * p * p * p else 1 - (-2 * p + 2).pow(3) / 2

    /** ease-in-out quint (HUD). */
    fun easeQuint(p: Double): Double = if (p < .5) 16 * p * p * p * p * p else 1 - (-2 * p + 2).pow(5) / 2

    fun eased(style: TransitionStyle, progress: Double): Double {
        val p = clamp01(progress)
        return if (style == TransitionStyle.HUD) easeQuint(p) else easeCubic(p)
    }

    /** Zeitbasierter Fortschritt 0..1; [durationMs] <= 0 liefert sofort 1. */
    fun progress(startNanos: Long, nowNanos: Long, durationMs: Int): Double {
        if (durationMs <= 0) return 1.0
        return clamp01((nowNanos - startNanos).toDouble() / (durationMs * 1_000_000.0))
    }

    /** Verschiebung in Pixeln, wie std::lround(eased * w) im Server (nie ueber der Breite). */
    fun slideOffset(w: Int, eased: Double): Int = min(w, floor(eased * w + .5).toInt()).coerceAtLeast(0)

    /** Fade-Gewicht des Zielbildes 0..255 (Server: lround(eased*256) von 256). */
    fun fadeAlpha(eased: Double): Int = min(255, floor(eased * 255 + .5).toInt()).coerceAtLeast(0)

    /**
     * Slide-Aufteilung: Das alte Bild belegt [oldSrcLeft, oldSrcRight) -> Ziel ab x=[oldDstLeft];
     * das neue [newSrcLeft, newSrcRight) -> Ziel ab x=[newDstLeft]. Gleiches Ergebnis wie die Zeilenkopie im Server.
     */
    data class SlideSpans(
        val d: Int,
        val oldSrcLeft: Int, val oldSrcRight: Int, val oldDstLeft: Int,
        val newSrcLeft: Int, val newSrcRight: Int, val newDstLeft: Int,
    )

    fun slide(w: Int, dir: Int, eased: Double): SlideSpans {
        val d = slideOffset(w, eased)
        return if (dir >= 0) SlideSpans(d, d, w, 0, 0, d, w - d)
        else SlideSpans(d, 0, w - d, d, w - d, w, 0)
    }

    /** Geometrie des HUD-Scans (Server: Zweig "HUD scan"), zusammengefasst fuer Zeichenroutinen. */
    data class HudGeometry(
        val width: Int, val height: Int,
        val line: Int, val back: Int,
        val oldLo: Int, val oldHi: Int, val newLo: Int, val newHi: Int,
        val glow: Int, val trail: Int, val unit: Double,
        /** Deckkraft der Schattenfarbe ueber dem alten Bild (1 - dim). */
        val shadeAlpha: Double,
        /** Zusaetzliche Deckkraft der Scanzeilen (jede dritte Zeile). */
        val scanExtraAlpha: Double,
        /** Deckkraft des Nachleuchtens an der Strahlkante. */
        val afterglowAlpha: Double,
        val grid: Int, val bracketLen: Int, val bracketThick: Int, val bracketPad: Int,
    )

    fun hud(w: Int, h: Int, eased: Double, dir: Int): HudGeometry {
        val scan = if (dir >= 0) (1 - eased) * w else eased * w
        val line = floor(scan + .5).toInt().coerceIn(0, w)
        val back = if (dir >= 0) 1 else -1
        val unit = max(1.0, min(w / 480.0, h / 320.0))
        fun r(v: Double) = floor(v + .5).toInt()
        return HudGeometry(
            width = w, height = h, line = line, back = back,
            oldLo = if (dir >= 0) 0 else line, oldHi = if (dir >= 0) line else w,
            newLo = if (dir >= 0) line else 0, newHi = if (dir >= 0) w else line,
            glow = max(5, r(w * .012)), trail = max(28, r(w * .11)), unit = unit,
            shadeAlpha = .46 * min(1.0, eased * 1.8),
            scanExtraAlpha = .3, // alte Seite: Zeile y%3==0 ist 0,7x so hell wie die uebrigen
            afterglowAlpha = 72.0 / 256.0 * (1 - .5 * eased),
            grid = max(16, r(24 * unit)),
            bracketLen = max(10, r(26 * unit)), bracketThick = max(2, r(2 * unit)), bracketPad = max(3, r(6 * unit)),
        )
    }

    /** Parst "#RRGGBB" zu opakem ARGB; null bei ungueltiger Form. */
    fun parseColor(s: String?): Int? {
        if (s == null || s.length != 7 || s[0] != '#') return null
        var v = 0
        for (i in 1..6) {
            val c = Character.digit(s[i], 16)
            if (c < 0) return null
            v = (v shl 4) or c
        }
        return v or (0xFF shl 24)
    }
}

/**
 * Haelt die Servernachricht "transition" bis zum naechsten Binaerframe. Wird auf dem Netzwerk-Thread geschrieben
 * und auf dem Main-Thread gelesen. Die Nachricht haengt am Zielframe (Reihenfolge ist so unabhaengig von
 * Handler-Posts) und verfaellt, wenn dieser nicht binnen [timeoutMs] kommt. Ersetzt ein neueres Bild ein noch
 * nicht ausgeliefertes, behaelt es die Nachricht ("nur das neueste").
 */
class TransitionCarrier(private val timeoutMs: Long = 1000) {
    private var current = 0 // Generation der aktuellen Verbindung; Aufrufe anderer Generationen werden ignoriert
    private var waiting: ServerMessage.Transition? = null
    private var waitingAt = 0L
    private var undelivered: ServerMessage.Transition? = null
    private var undeliveredAt = 0L // Eingangszeit der Nachricht: auch eine weitergereichte Nachricht verfaellt nach [timeoutMs]
    private var lastDelivered: ServerMessage.Transition? = null

    /** Neue Verbindung: verwirft alles und ignoriert ab jetzt Aufrufe aelterer Generationen. */
    @Synchronized fun advance(generation: Int) { clear(); current = generation }

    @Synchronized fun onTransition(t: ServerMessage.Transition, nowMs: Long, generation: Int = current) {
        if (generation != current) return
        waiting = t; waitingAt = nowMs
    }

    /**
     * Neues Binaerbild eingetroffen: liefert die zugehoerige Nachricht (oder null). Die Nachricht ist erst mit
     * [delivered] verbraucht; wird das Bild abgewiesen, muss der Aufrufer [discard] rufen.
     */
    @Synchronized fun onFrame(nowMs: Long, generation: Int = current): ServerMessage.Transition? {
        if (generation != current) return null
        val w = waiting
        waiting = null
        if (w != null) {
            if (nowMs - waitingAt <= timeoutMs) { undelivered = w; undeliveredAt = waitingAt } else undelivered = null
        } else if (undelivered != null && nowMs - undeliveredAt > timeoutMs) undelivered = null
        return undelivered
    }

    /** Bild wird zugestellt: true genau einmal je Nachricht (Wiederholung durch ein ueberholtes Bild wird verworfen). */
    @Synchronized fun delivered(t: ServerMessage.Transition?): Boolean {
        if (t == null) return false
        if (undelivered === t) undelivered = null
        if (lastDelivered === t) return false
        lastDelivered = t
        return true
    }

    /** Das Bild mit [t] wurde abgewiesen (alte Generation, vor ready): die Nachricht darf nicht auf spaetere Bilder uebergehen. */
    @Synchronized fun discard(t: ServerMessage.Transition?) {
        if (t == null) return
        if (undelivered === t) undelivered = null
        if (waiting === t) waiting = null
    }

    @Synchronized fun clear() { waiting = null; undelivered = null; lastDelivered = null }
}

/**
 * Bindet eine Nachricht an das Zielbild: nur ein Bild, das nach der Nachricht eingetroffen ist (Folgenummer
 * >= [arm]), wird zum Ziel; spaeter als [timeoutMs] dekodiert = verworfen.
 */
class TransitionArming(private val timeoutMs: Long = 1000) {
    private var t: ServerMessage.Transition? = null
    private var fromSeq = 0L
    private var armedAt = 0L
    val armed: Boolean get() = t != null

    fun arm(t: ServerMessage.Transition, firstTargetSeq: Long, nowMs: Long) { this.t = t; fromSeq = firstTargetSeq; armedAt = nowMs }

    /** Dekodiertes Bild [seq]: liefert die Nachricht, wenn es das Ziel ist; veraltete Armierung wird dabei geloescht. */
    fun claim(seq: Long, nowMs: Long): ServerMessage.Transition? {
        val cur = t ?: return null
        if (nowMs - armedAt > timeoutMs) { t = null; return null }
        if (seq < fromSeq) return null // Bild von vor dem Uebergang: normal zeigen, Armierung bleibt
        t = null
        return cur
    }

    fun clear() { t = null }
}

/**
 * Anheften von Bitmaps (oder beliebigen Objekten) waehrend einer Animation: ein angeheftetes Objekt darf nicht in
 * den Pool zurueck oder recycelt werden. Eine in der Zwischenzeit angeforderte Freigabe wird aufgeschoben.
 */
class PinTracker<T : Any> {
    private val pinned: MutableSet<T> = Collections.newSetFromMap(IdentityHashMap())
    private val deferred: MutableSet<T> = Collections.newSetFromMap(IdentityHashMap())

    // Freigaben kommen auch vom Decoder-Thread, daher synchronisiert.
    @Synchronized fun pin(x: T) { pinned.add(x) }
    @Synchronized fun isPinned(x: T) = x in pinned
    val pinCount: Int get() = synchronized(this) { pinned.size }

    /** true = Aufrufer darf sofort freigeben; false = angeheftet, Freigabe wird mit [unpinAll] nachgereicht. */
    @Synchronized fun requestRelease(x: T): Boolean {
        if (x in pinned) { deferred.add(x); return false }
        return true
    }

    /** Hebt alle Anheftungen auf; liefert die waehrenddessen aufgeschobenen Freigaben. */
    @Synchronized fun unpinAll(): List<T> {
        val out = deferred.toList()
        pinned.clear(); deferred.clear()
        return out
    }
}

/** Zeitzustand einer laufenden Animation (reine Logik, Zeit in Choreographer-Nanosekunden). */
class TransitionClock(val durationMs: Int) {
    private var start = Long.MIN_VALUE
    /** Fortschritt fuer den Frame [frameTimeNanos]; der erste Aufruf legt den Start fest (Fortschritt 0). */
    fun progress(frameTimeNanos: Long): Double {
        if (start == Long.MIN_VALUE) start = frameTimeNanos
        return TransitionMath.progress(start, frameTimeNanos, durationMs)
    }
    fun finished(p: Double) = p >= 1.0
}
