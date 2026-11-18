package de.fuchslabs.display

import android.content.Context
import android.graphics.*
import android.view.GestureDetector
import android.view.KeyEvent
import android.view.MotionEvent
import android.os.Build
import android.os.SystemClock
import android.view.View
import de.fuchslabs.display.protocol.PinTracker
import de.fuchslabs.display.protocol.ServerMessage
import de.fuchslabs.display.protocol.TransitionArming
import de.fuchslabs.display.protocol.TransitionClock
import de.fuchslabs.display.protocol.TransitionMath
import de.fuchslabs.display.protocol.TransitionStyle
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import de.fuchslabs.display.protocol.DpadNavigation
import de.fuchslabs.display.protocol.Letterbox
import de.fuchslabs.display.protocol.Protocol
import de.fuchslabs.display.protocol.GenerationSlot
import de.fuchslabs.display.protocol.ImageLimits
import de.fuchslabs.display.protocol.SwipeClassifier
import de.fuchslabs.display.protocol.SwipeDir
import java.util.concurrent.Executors
import kotlin.math.abs

/** Zeichnet das JPEG-Frame mit Letterbox und sendet Taps in Renderkoordinaten. */
class FrameView(ctx: Context) : View(ctx) {
    private companion object { const val TAG = "FrameView"; const val HW_MIN_PIXELS = 1_500_000L }
    var onTouchRender: ((Int, Int) -> Unit)? = null
    var onSwipe: ((SwipeDir) -> Unit)? = null
    /** Tatsaechliche Groesse der Rendeflaeche in Pixeln (nach Layout/Rotation). */
    var onSizeChangedPx: ((Int, Int) -> Unit)? = null
    private var bmp: Bitmap? = null
    private var rw = 480
    private var rh = 320
    // Bilineares Filtering statt Nearest-Neighbor beim (seltenen) Skalieren; Normalfall: Bild hat Zielaufloesung (1:1).
    private val paint = Paint(Paint.FILTER_BITMAP_FLAG).apply { isFilterBitmap = true; isDither = true }
    private val bleedPaint = Paint(Paint.FILTER_BITMAP_FLAG).apply { isFilterBitmap = true }
    private var bgColor = Color.parseColor("#0E1628")
    private val density = resources.displayMetrics.density
    private val swipeMinDistance = density * 64
    private val swipeMinVelocity = density * 400 // dp/s -> px/s
    private val gestures = GestureDetector(ctx, object : GestureDetector.SimpleOnGestureListener() {
        override fun onDown(e: MotionEvent): Boolean = true
        override fun onFling(e1: MotionEvent?, e2: MotionEvent, velocityX: Float, velocityY: Float): Boolean {
            val dir = SwipeClassifier.classify(e2.x - downX, e2.y - downY, velocityX, swipeMinDistance, swipeMinVelocity)
                ?: return false
            swiped = true
            onSwipe?.invoke(dir)
            return true
        }
    })
    private var swiped = false
    private var downX = 0f
    private var downY = 0f
    private val slop = resources.displayMetrics.density * 16

    // Sicherer Bereich: Kamera-Ausschnitt (DisplayCutout) und abgerundete Ecken (ab Android 12) werden ausgespart.
    private var insL = 0
    private var insT = 0
    private var insR = 0
    private var insB = 0
    val contentWidth: Int get() = (width - insL - insR).coerceAtLeast(0)
    val contentHeight: Int get() = (height - insT - insB).coerceAtLeast(0)

    init {
        setBackgroundColor(Color.parseColor("#0E1628"))
        // Fokussierbar, damit eine Fernbedienung (D-Pad) Key-Events an diese View statt ins Leere schickt.
        isFocusable = true
        isFocusableInTouchMode = true
        ViewCompat.setOnApplyWindowInsetsListener(this) { _, wi ->
            val cut = wi.getInsets(WindowInsetsCompat.Type.displayCutout())
            // Ein Rechteck verliert an einer Rundung mit Radius r etwa r * (1 - 1/sqrt(2)) je Achse.
            var cl = 0; var ct = 0; var cr = 0; var cb = 0
            if (Build.VERSION.SDK_INT >= 31) {
                val w = wi.toWindowInsets()
                fun r(pos: Int) = w?.getRoundedCorner(pos)?.radius ?: 0
                val k = 0.2929f
                cl = (maxOf(r(android.view.RoundedCorner.POSITION_TOP_LEFT), r(android.view.RoundedCorner.POSITION_BOTTOM_LEFT)) * k).toInt()
                cr = (maxOf(r(android.view.RoundedCorner.POSITION_TOP_RIGHT), r(android.view.RoundedCorner.POSITION_BOTTOM_RIGHT)) * k).toInt()
                ct = (maxOf(r(android.view.RoundedCorner.POSITION_TOP_LEFT), r(android.view.RoundedCorner.POSITION_TOP_RIGHT)) * k).toInt()
                cb = (maxOf(r(android.view.RoundedCorner.POSITION_BOTTOM_LEFT), r(android.view.RoundedCorner.POSITION_BOTTOM_RIGHT)) * k).toInt()
            }
            setSafeInsets(maxOf(cut.left, cl), maxOf(cut.top, ct), maxOf(cut.right, cr), maxOf(cut.bottom, cb))
            wi
        }
    }

    private fun setSafeInsets(l: Int, t: Int, r: Int, b: Int) {
        if (l == insL && t == insT && r == insR && b == insB) return
        insL = l; insT = t; insR = r; insB = b
        abortTransition()
        invalidate()
        if (contentWidth > 0 && contentHeight > 0) onSizeChangedPx?.invoke(contentWidth, contentHeight)
    }

    fun setRenderSize(w: Int, h: Int) { if (w != rw || h != rh) abortTransition(); rw = w; rh = h; invalidate() }

    private val decoder = Executors.newSingleThreadExecutor { r -> Thread(r, "frame-decode").apply { isDaemon = true } }
    /** Neuestes noch nicht dekodiertes Frame, zusammen mit der Epoche, in der es angenommen wurde. */
    private val latest = GenerationSlot<Raw>()
    private class Raw(val seq: Long, val jpeg: ByteArray)
    private var frameSeq = 0L   // nur Main-Thread

    private class Decoded(val epoch: Int, val bitmap: Bitmap, val bg: Int, val seq: Long)
    /**
     * Hoechstens zwei dekodierte, noch nicht angezeigte Bilder. Sie werden im Takt der Bildschirmaktualisierung
     * (Choreographer) einzeln gezeigt: Bilder, die mit Netzwerk-Schwankung dicht hintereinander ankommen, fallen so
     * nicht mehr weg, sondern verteilen sich auf die naechsten Aktualisierungen. Zugriff nur unter synchronized(queue).
     */
    private val queue = ArrayDeque<Decoded>()
    private var showScheduled = false   // nur Main-Thread
    /** Wird von [clearPending] erhoeht; Bilder aus einer frueheren Epoche werden nicht mehr angezeigt. */
    @Volatile private var epoch = 0
    /** Epoche des angezeigten Bildes; nur Main-Thread. Tippen zaehlt erst, wenn ein Bild des aktuellen Ziels sichtbar ist. */
    private var shownEpoch = -1
    private val showNext = android.view.Choreographer.FrameCallback { frameTimeNanos ->
        showScheduled = false
        freeDrawnOut()
        if (anim != null) return@FrameCallback // waehrend eines Uebergangs wartet das neueste Bild in der Queue
        var d: Decoded? = null
        var more = false
        synchronized(queue) {
            while (queue.isNotEmpty()) {
                val q = queue.removeFirst()
                if (q.epoch == epoch) { d = q; break } else release(q.bitmap)
            }
            more = queue.isNotEmpty()
        }
        d?.let {
            val tx = arming.claim(it.seq, SystemClock.uptimeMillis())
            if (tx == null || !startTransition(tx, it, frameTimeNanos)) show(it)
        }
        if (more && anim == null) scheduleShow()
    }
    private val scheduleShowRunnable = Runnable { scheduleShow() }

    // ---- Seitenuebergaenge (Faehigkeit "xfade1"); Math und Zeitlogik in de.fuchslabs.display.protocol ----
    private val arming = TransitionArming()
    /** "von"- und "nach"-Bitmap sind waehrend der Animation angeheftet: weder Pool noch inBitmap-Reuse noch Recycle. */
    private val pins = PinTracker<Bitmap>()
    private class Anim(val tx: ServerMessage.Transition, val from: Bitmap, val d: Decoded, val bgFrom: Int, val hud: HudPainter?) {
        val clock = TransitionClock(tx.ms)
        var progress = 0.0
        // Randlos-Untergrund wie in onDraw: Randpixel beider Bilder laufen bis an die Bildschirmkanten (einmal je Uebergang).
        val fromShader = BitmapShader(from, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP)
        val toShader = BitmapShader(d.bitmap, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP)
        var shaderScale = Float.NaN; var shaderLeft = Float.NaN; var shaderTop = Float.NaN
    }
    private val bleedMatrix = Matrix()
    private val marginShade = Paint()
    private val bleedToPaint =Paint(Paint.FILTER_BITMAP_FLAG).apply { isFilterBitmap = true }
    private var anim: Anim? = null   // nur Main-Thread
    private val animPaint = Paint(Paint.FILTER_BITMAP_FLAG).apply { isFilterBitmap = true }
    private val srcR = Rect(); private val dstR = Rect()
    private val animTick = android.view.Choreographer.FrameCallback { tickTransition(it) }

    /** true = Uebergang laeuft (dann zeigt [d] erst dessen Ende); false = normal anzeigen (Groesse/Epoche passt nicht usw.). */
    private fun startTransition(tx: ServerMessage.Transition, d: Decoded, frameTimeNanos: Long): Boolean {
        val from = bmp ?: return false
        val to = d.bitmap
        if (from.isRecycled || to.isRecycled || from === to || shownEpoch != d.epoch) return false
        if (from.width != to.width || from.height != to.height) return false
        if (contentWidth <= 0 || contentHeight <= 0 || !isShown) return false
        pins.pin(from); pins.pin(to)
        val hud = if (tx.style == TransitionStyle.HUD) takeHudPrep(tx, to.width, to.height) else null
        anim = Anim(tx, from, d, bgColor, hud)
        tickTransition(frameTimeNanos) // Fortschritt 0 im selben Takt
        return true
    }

    /**
     * HudPainter (Shader, Kachel-Bitmap, Rasterlinien) wird schon beim Eintreffen der Transition-Nachricht auf dem
     * Decoder-Thread vorbereitet, vor dem Dekodieren des Zielbildes (FIFO-Executor); der erste Animationstakt baut
     * dann nichts mehr auf. Das Bild hat bei einem Uebergang die Groesse des angezeigten Bildes.
     */
    private class HudPrep(val tx: ServerMessage.Transition, val w: Int, val h: Int, val task: java.util.concurrent.FutureTask<HudPainter>)
    private var hudPrep: HudPrep? = null   // nur Main-Thread

    private fun prepareHud(tx: ServerMessage.Transition) {
        hudPrep = null
        val b = bmp ?: return
        if (tx.style != TransitionStyle.HUD || b.isRecycled) return
        val w = b.width; val h = b.height
        val task = java.util.concurrent.FutureTask<HudPainter> { HudPainter(w, h, tx) }
        hudPrep = HudPrep(tx, w, h, task)
        decoder.execute(task)
    }

    private fun takeHudPrep(tx: ServerMessage.Transition, w: Int, h: Int): HudPainter {
        val p = hudPrep
        hudPrep = null
        if (p != null && p.tx === tx && p.w == w && p.h == h) {
            try { return p.task.get() } catch (_: Exception) {}
        } else p?.task?.cancel(false)
        return HudPainter(w, h, tx) // Rueckfall (z. B. Groesse hat sich geaendert)
    }

    private fun tickTransition(frameTimeNanos: Long) {
        val a = anim ?: return
        val p = a.clock.progress(frameTimeNanos)
        a.progress = p
        if (a.clock.finished(p)) { finishTransition(); return }
        invalidate()
        android.view.Choreographer.getInstance().postFrameCallback(animTick)
    }

    /** Beendet einen laufenden Uebergang sofort und zeigt das Zielbild (Abbruch bei Abriss, Resize, Pause, Zielwechsel). */
    fun abortTransition() {
        arming.clear()
        hudPrep?.task?.cancel(false); hudPrep = null
        if (anim != null) finishTransition()
    }

    private fun finishTransition() {
        val a = anim ?: return
        anim = null
        android.view.Choreographer.getInstance().removeFrameCallback(animTick)
        val deferred = pins.unpinAll()
        show(a.d) // gibt "von" regulaer frei (nach dem naechsten Zeichenlauf), "nach" wird zum angezeigten Bild
        for (b in deferred) {
            if (b === a.from || b === a.d.bitmap) continue
            if (b === lastDrawn) continue // wird beim naechsten Zeichenlauf ohnehin abgeloest
            release(b)
        }
        // Waehrenddessen eingetroffene Bilder: nur das neueste behalten und anzeigen.
        val dropped = synchronized(queue) { val l = ArrayList<Decoded>(); while (queue.size > 1) l.add(queue.removeFirst()); l }
        dropped.forEach { release(it.bitmap) }
        val more = synchronized(queue) { queue.isNotEmpty() }
        if (more) scheduleShow()
    }

    /** Nur Main-Thread. */
    private fun scheduleShow() {
        if (anim != null) return
        if (showScheduled) return
        showScheduled = true
        android.view.Choreographer.getInstance().postFrameCallback(showNext)
    }

    private fun show(d: Decoded) {
        // Ein nie gezeichnetes Bitmap kann sofort zurueck. Das zuletzt gezeichnete bleibt, bis ein ersetzender
        // Zeichenlauf abgeschlossen ist (siehe onDraw/freeDrawnOut).
        val old = bmp
        bmp = d.bitmap; bgColor = d.bg or Color.BLACK; shownEpoch = d.epoch; invalidate()
        if (old != null && old !== lastDrawn) release(old)
    }

    /**
     * Gibt Bitmaps frei, die ein frueherer Zeichenlauf abgeloest hat. Laeuft im Choreographer-Takt, also erst
     * nachdem der vorherige Zeichenlauf samt Abgleich mit dem Render-Thread fertig ist.
     */
    // Eigener Aufraeum-Takt: gibt abgeloeste Bitmaps auch frei, wenn danach kein weiteres Bild mehr kommt.
    // Wird von clearPending() bewusst nicht entfernt.
    private var cleanupScheduled = false   // nur Main-Thread
    private val cleanup = android.view.Choreographer.FrameCallback { cleanupScheduled = false; freeDrawnOut() }
    private fun scheduleCleanup() {
        if (cleanupScheduled) return
        cleanupScheduled = true
        android.view.Choreographer.getInstance().postFrameCallback(cleanup)
    }

    private fun freeDrawnOut() {
        if (drawnOut.isEmpty()) return
        for (b in drawnOut) if (b !== bmp && b !== lastDrawn) release(b)
        drawnOut.clear()
    }

    // Bitmaps werden wiederverwendet (inBitmap) statt je Bild neu angelegt: bei 60 Bildern/s sind das sonst
    // bis zu 4–8 MB je Bild fuer die Speicherbereinigung, die auf schwachen Geraeten sichtbar ruckelt.
    private val pool = ArrayDeque<Bitmap>()       // freie, veraenderbare Bitmaps; Zugriff nur unter synchronized(pool)
    private val drawnOut = ArrayList<Bitmap>()    // von einem Zeichenlauf abgeloest, Freigabe im naechsten Takt; Main-Thread
    private var lastDrawn: Bitmap? = null         // nur Main-Thread

    private fun obtain(w: Int, h: Int): Bitmap? {
        val stale = ArrayList<Bitmap>()
        val hit = synchronized(pool) {
            // Andere Groessen (z. B. nach Drehung oder verspaeteter Rueckgabe) bei jedem Zugriff verdraengen.
            val it = pool.iterator()
            while (it.hasNext()) { val b = it.next(); if (b.width != w || b.height != h || !b.isMutable || b.isRecycled) { it.remove(); stale.add(b) } }
            pool.removeFirstOrNull()
        }
        stale.forEach { it.recycle() }
        return hit
    }

    private fun release(b: Bitmap) {
        if (b.isRecycled) return
        if (!pins.requestRelease(b)) return // angeheftet (laufender Uebergang): Freigabe folgt in finishTransition
        if (b.isMutable) synchronized(pool) { if (pool.size < 3) { pool.addLast(b); return } }
        b.recycle()
    }

    /**
     * Nimmt das Frame an; Decodierung im Hintergrund, nur das neueste Frame wird dekodiert. Die Epoche wird hier
     * vergeben und bleibt am Frame: ein noch fuer ein frueheres Ziel geplanter Decoder kann ein neues Frame
     * dadurch nicht der alten Epoche zuordnen. Nur auf dem Main-Thread aufrufen.
     */
    fun setFrame(jpeg: ByteArray, transition: ServerMessage.Transition? = null) {
        if (!ImageLimits.bytesOk(jpeg.size)) return
        val seq = ++frameSeq
        // Dieses Bild (oder ein neueres, falls es ueberholt wird) ist das Ziel des angekuendigten Uebergangs.
        if (transition != null) { arming.arm(transition, seq, SystemClock.uptimeMillis()); prepareHud(transition) }
        if (latest.offer(epoch, Raw(seq, jpeg))) decoder.execute { decodeLatest() }
    }

    /**
     * Verwirft wartende, noch nicht angezeigte Bilder (Stopp oder Zielwechsel). Das angezeigte Bild bleibt stehen.
     * Nur auf dem Main-Thread aufrufen.
     */
    fun clearPending() {
        epoch++ // zuerst: ein gerade laufender Decoder liefert danach nichts mehr aus
        latest.advance(epoch) // wartendes Frame der alten Epoche verwerfen
        abortTransition() // laufender Uebergang endet mit dem Zielbild; Armierung faellt weg
        removeCallbacks(scheduleShowRunnable)
        if (showScheduled) { android.view.Choreographer.getInstance().removeFrameCallback(showNext); showScheduled = false }
        val dropped = synchronized(queue) { val l = queue.toList(); queue.clear(); l }
        dropped.forEach { release(it.bitmap) } // nie angezeigt, daher gefahrlos wiederverwendbar
    }

    private fun decodeLatest() {
        // Frame und Epoche gemeinsam entnehmen: massgeblich ist die Epoche bei setFrame(), nicht die beim Dekodieren.
        val f = latest.take() ?: return
        val e = f.generation
        val jpeg = f.value.jpeg
        val seq = f.value.seq
        try {
            val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, bounds)
            if (!ImageLimits.dimsOk(bounds.outWidth, bounds.outHeight)) return
            val reuse = obtain(bounds.outWidth, bounds.outHeight)
            val opts = BitmapFactory.Options().apply { inMutable = true; inBitmap = reuse }
            val b = (try { BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, opts) } catch (_: IllegalArgumentException) { null })
                ?: run { reuse?.let { release(it) }; BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, BitmapFactory.Options().apply { inMutable = true }) } ?: return
            val bg = b.getPixel(0, 0) // Letterbox-Hintergrund = Randfarbe des Bildes (vor der HW-Kopie: getPixel geht dort nicht)
            val shown = toHardware(b) ?: b.also { it.prepareToDraw() } // Rueckfall: bisheriger Pfad
            // Hoechstens zwei warten; ein drittes verdraengt das aelteste (nie angezeigt, daher gefahrlos zurueck).
            val drop = synchronized(queue) {
                queue.addLast(Decoded(e, shown, bg, seq))
                if (queue.size > 2) queue.removeFirst() else null
            }
            drop?.let { release(it.bitmap) }
            post(scheduleShowRunnable)
        } catch (_: Exception) {} catch (_: OutOfMemoryError) {}
    }

    /**
     * Hardware-Bitmaps: Bei schwacher GPU kostete das Hochladen eines neuen Software-Bitmaps im ersten Zeichenlauf
     * (Render-Thread) ~44 ms je Bild. copy(HARDWARE) laedt die Textur hier auf dem Decoder-Thread hoch; angezeigt wird
     * die Hardware-Kopie, der Software-Puffer geht sofort zurueck in den inBitmap-Pool. Hardware-Bitmaps sind nicht
     * veraenderbar und landen daher nie im Pool, sondern werden bei der Freigabe recycelt (siehe [release]).
     * Liefert copy() null oder wirft (z. B. Emulator ohne GL), bleibt der Pfad fuer die Laufzeit der View aus.
     */
    @Volatile private var hwOff = false
    private val isTv = ctx.packageManager.hasSystemFeature(android.content.pm.PackageManager.FEATURE_LEANBACK)
    private val debugLog = (ctx.applicationInfo.flags and android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE) != 0
    private var hwStatN = 0; private var hwStatSumNs = 0L; private var hwStatMaxNs = 0L   // nur Decoder-Thread

    /** Nur Decoder-Thread. Gibt bei Erfolg [sw] an den Pool zurueck; null = bisherigen Pfad mit [sw] gehen. */
    private fun toHardware(sw: Bitmap): Bitmap? {
        if (hwOff) return null
        // Nur grosse Bilder (TV/1080p und mehr): Gemessen bringt der Hardware-Pfad dort den Grossteil (BeyondTV: p99
        // 45-54 -> 25-35 ms, Frames ueber 33 ms 75-159 -> 0-6 je 30 s); bei kleinen Bildern (Redmi 8, 1445x680) kostet
        // die Kopie mehr, als der Upload spart (p99 +3-5 ms). Fernseher nutzen ihn immer.
        if (sw.width.toLong() * sw.height < HW_MIN_PIXELS && !isTv) return null
        val t0 = System.nanoTime()
        val hw = try { sw.copy(Bitmap.Config.HARDWARE, false) } catch (t: Exception) { disableHw("copy: $t"); return null }
                 catch (t: OutOfMemoryError) { disableHw("copy: $t"); return null }
        if (hw == null || hw.config != Bitmap.Config.HARDWARE) { hw?.recycle(); disableHw("copy lieferte ${hw?.config}"); return null }
        val dt = System.nanoTime() - t0
        release(sw) // nie angezeigt: gefahrlos zurueck in den Pool
        if (debugLog) {
            hwStatN++; hwStatSumNs += dt; if (dt > hwStatMaxNs) hwStatMaxNs = dt
            if (hwStatN >= 60) {
                android.util.Log.d(TAG, "copy(HARDWARE) ${hw.width}x${hw.height}: avg %.1f ms, max %.1f ms ueber %d Bilder"
                    .format(hwStatSumNs / 1e6 / hwStatN, hwStatMaxNs / 1e6, hwStatN))
                hwStatN = 0; hwStatSumNs = 0L; hwStatMaxNs = 0L
            }
        }
        return hw
    }

    private fun disableHw(why: String) {
        if (hwOff) return
        hwOff = true
        android.util.Log.w(TAG, "Hardware-Bitmaps aus, Rueckfall auf Software-Bitmaps ($why)")
    }

    private fun isHw(b: Bitmap) = b.config == Bitmap.Config.HARDWARE

    /**
     * Hardware-Bitmaps lassen sich nur auf einem hardwarebeschleunigten Canvas zeichnen; ein Software-Canvas
     * (Software-Layer, View.draw in ein Bitmap fuer Screenshots o. Ae.) wuerde werfen. Dann nur Hintergrund. Ist die
     * View selbst nicht beschleunigt, zusaetzlich den Hardware-Pfad abschalten (folgende Bilder kommen als Software-
     * Bitmaps); ein einmaliges Zeichnen in ein fremdes Software-Canvas schaltet ihn nicht ab.
     */
    private fun blockedOnSoftwareCanvas(c: Canvas, vararg bs: Bitmap): Boolean {
        if (c.isHardwareAccelerated || bs.none { isHw(it) }) return false
        if (!isHardwareAccelerated) disableHw("View nicht hardwarebeschleunigt")
        c.drawColor(bgColor)
        return true
    }

    override fun onDetachedFromWindow() { abortTransition(); super.onDetachedFromWindow() }
    override fun onWindowVisibilityChanged(visibility: Int) {
        if (visibility != VISIBLE) abortTransition()
        super.onWindowVisibilityChanged(visibility)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        abortTransition()
        super.onSizeChanged(w, h, oldw, oldh)
        if (contentWidth > 0 && contentHeight > 0) onSizeChangedPx?.invoke(contentWidth, contentHeight)
    }

    override fun onDraw(c: Canvas) {
        val a = anim
        if (a != null) { drawTransition(c, a); return }
        val b = bmp ?: return
        if (lastDrawn !== b) {
            lastDrawn?.let { drawnOut.add(it); scheduleCleanup() }
            lastDrawn = b
        }
        if (blockedOnSoftwareCanvas(c, b)) return
        c.drawColor(bgColor)
        val lb = Letterbox.compute(contentWidth, contentHeight, b.width.coerceAtLeast(1), b.height.coerceAtLeast(1))
        val ox = insL.toFloat(); val oy = insT.toFloat()
        val dst = RectF(ox + lb.left, oy + lb.top, ox + lb.left + b.width * lb.scale, oy + lb.top + b.height * lb.scale)
        // Randlos: das Bild sitzt im sicheren Bereich, seine Randpixel laufen bis an die Bildschirmkanten weiter
        // (Hintergrund hinter Ausschnitt und Rundungen, Inhalt nur im sicheren Bereich).
        val shader = BitmapShader(b, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP)
        val m = Matrix().apply { setScale(lb.scale, lb.scale); postTranslate(dst.left, dst.top) }
        shader.setLocalMatrix(m)
        bleedPaint.shader = shader
        c.drawRect(0f, 0f, width.toFloat(), height.toFloat(), bleedPaint)
    }

    private fun box(c: Canvas, p: Paint, x0: Float, y0: Float, x1: Float, y1: Float) { if (x1 > x0 && y1 > y0) c.drawRect(x0, y0, x1, y1, p) }

    /** Zeichnet [p] nur in den Streifen ausserhalb des Rechtecks (l,t,r,b), begrenzt auf die Spalten [xl, xr). */
    private fun strips(c: Canvas, p: Paint, xl: Float, xr: Float, l: Float, t: Float, r: Float, b: Float, w: Float, h: Float) {
        box(c, p, xl, 0f, xr, t); box(c, p, xl, b, xr, h)
        val y0 = maxOf(t, 0f); val y1 = minOf(b, h)
        box(c, p, xl, y0, minOf(xr, l), y1); box(c, p, maxOf(xl, r), y0, xr, y1)
    }

    private fun hudSide(c: Canvas, a: Anim, xl: Float, xr: Float, old: Boolean, l: Float, t: Float, r: Float, b: Float, w: Float, h: Float) {
        if (xr <= xl) return
        bleedPaint.shader = if (old) a.fromShader else a.toShader
        strips(c, bleedPaint, xl, xr, l, t, r, b, w, h)
        if (old) strips(c, marginShade, xl, xr, l, t, r, b, w, h)
    }

    /** Zeichnet den Uebergang nur mit Canvas-Primitiven (Clip, drawBitmap, Verlaeufe); Geometrie in Bildpixeln. */
    private fun drawTransition(c: Canvas, a: Anim) {
        val from = a.from; val to = a.d.bitmap
        if (from.isRecycled || to.isRecycled) return
        val bw = from.width; val bh = from.height
        val tx = a.tx
        val e = TransitionMath.eased(tx.style, a.progress)
        // "von" wird gezeichnet: fuer die Freigabe-Abfolge als zuletzt gezeichnet vormerken (siehe onDraw).
        if (lastDrawn !== from) { lastDrawn?.let { drawnOut.add(it); scheduleCleanup() }; lastDrawn = from }
        if (blockedOnSoftwareCanvas(c, from, to)) return
        val lb = Letterbox.compute(contentWidth, contentHeight, bw.coerceAtLeast(1), bh.coerceAtLeast(1))
        // Untergrund wie onDraw (Randlos per BitmapShader): Start = Normaldarstellung des alten, Ende = die des neuen Bildes.
        // Dazwischen blendet der Rand-Bleed des neuen Bildes ueber den des alten; die Bildflaeche selbst zeichnet das Folgende.
        if (a.shaderScale != lb.scale || a.shaderLeft != insL + lb.left || a.shaderTop != insT + lb.top) {
            a.shaderScale = lb.scale; a.shaderLeft = insL + lb.left; a.shaderTop = insT + lb.top
            bleedMatrix.setScale(lb.scale, lb.scale); bleedMatrix.postTranslate(a.shaderLeft, a.shaderTop)
            a.fromShader.setLocalMatrix(bleedMatrix); a.toShader.setLocalMatrix(bleedMatrix)
        }
        // Randbereich (Ausschnitt/Rundungen/Letterbox) folgt der sichtbaren Bildkante. Ein Ueberblenden der Randpixel beider
        // Bilder passt nur beim Fade; Slide und HUD teilen den Rand an der Trennlinie wie das Bild selbst (sonst zeigen
        // Streifen links/oben farbig den falschen Seitenrand und springen am Ende).
        val sw = width.toFloat(); val sh = height.toFloat()
        val sc = lb.scale
        // Nur die Streifen ausserhalb des Bildrechtecks (Ausschnitt/Rundungen/Letterbox) brauchen den Rand-Untergrund; das Bild
        // selbst zeichnet das Folgende. Kein Vollbild-Shader-Durchgang unter dem Bild (Fuellrate schwacher GPUs).
        val cl = a.shaderLeft; val ct = a.shaderTop; val cr = cl + bw * sc; val cb = ct + bh * sc
        when (tx.style) {
            TransitionStyle.FADE -> {
                bleedPaint.shader = a.fromShader
                strips(c, bleedPaint, 0f, sw, cl, ct, cr, cb, sw, sh)
                if (e > 0.0) {
                    bleedToPaint.shader = a.toShader
                    bleedToPaint.alpha = TransitionMath.fadeAlpha(e)
                    strips(c, bleedToPaint, 0f, sw, cl, ct, cr, cb, sw, sh)
                }
            }
            TransitionStyle.SLIDE -> {
                // Beide Bilder verschoben (CLAMP verlaengert deren Randpixel); links der Trennlinie das linke Bild.
                val s = TransitionMath.slide(bw, tx.dir, e)
                val oldLeft = tx.dir >= 0
                val split = a.shaderLeft + (if (oldLeft) s.newDstLeft else s.oldDstLeft) * sc
                val leftShader = if (oldLeft) a.fromShader else a.toShader
                val rightShader = if (oldLeft) a.toShader else a.fromShader
                val leftShift = if (oldLeft) s.oldDstLeft - s.oldSrcLeft else s.newDstLeft - s.newSrcLeft
                val rightShift = if (oldLeft) s.newDstLeft - s.newSrcLeft else s.oldDstLeft - s.oldSrcLeft
                bleedMatrix.setScale(sc, sc); bleedMatrix.postTranslate(a.shaderLeft + leftShift * sc, a.shaderTop)
                leftShader.setLocalMatrix(bleedMatrix)
                bleedMatrix.setScale(sc, sc); bleedMatrix.postTranslate(a.shaderLeft + rightShift * sc, a.shaderTop)
                rightShader.setLocalMatrix(bleedMatrix)
                a.shaderScale = Float.NaN // Matrizen wurden veraendert: Cache fuer den Normalfall ungueltig
                bleedPaint.shader = leftShader; strips(c, bleedPaint, 0f, split, cl, ct, cr, cb, sw, sh)
                bleedPaint.shader = rightShader; strips(c, bleedPaint, split, sw, cl, ct, cr, cb, sw, sh)
            }
            TransitionStyle.HUD -> {
                val g = TransitionMath.hud(bw, bh, e, tx.dir)
                val split = a.shaderLeft + g.line * sc
                val oldLeft = tx.dir >= 0
                marginShade.color = tx.shade or (0xFF shl 24); marginShade.alpha = (g.shadeAlpha * 255 + .5).toInt()
                // alte Seite (abgedunkelt wie im Bild) und neue Seite, jeweils bis an die Bildschirmkante
                hudSide(c, a, 0f, split, oldLeft, cl, ct, cr, cb, sw, sh)
                hudSide(c, a, split, sw, !oldLeft, cl, ct, cr, cb, sw, sh)
            }
        }
        c.save()
        c.translate(insL + lb.left, insT + lb.top)
        c.scale(lb.scale, lb.scale)
        c.clipRect(0f, 0f, bw.toFloat(), bh.toFloat())
        when (tx.style) {
            TransitionStyle.SLIDE -> {
                val s = TransitionMath.slide(bw, tx.dir, e)
                if (s.oldSrcRight > s.oldSrcLeft) {
                    srcR.set(s.oldSrcLeft, 0, s.oldSrcRight, bh)
                    dstR.set(s.oldDstLeft, 0, s.oldDstLeft + (s.oldSrcRight - s.oldSrcLeft), bh)
                    c.drawBitmap(from, srcR, dstR, animPaint)
                }
                if (s.newSrcRight > s.newSrcLeft) {
                    srcR.set(s.newSrcLeft, 0, s.newSrcRight, bh)
                    dstR.set(s.newDstLeft, 0, s.newDstLeft + (s.newSrcRight - s.newSrcLeft), bh)
                    c.drawBitmap(to, srcR, dstR, animPaint)
                }
            }
            TransitionStyle.FADE -> {
                animPaint.alpha = 255
                c.drawBitmap(from, 0f, 0f, animPaint)
                animPaint.alpha = TransitionMath.fadeAlpha(e)
                c.drawBitmap(to, 0f, 0f, animPaint)
                animPaint.alpha = 255
            }
            TransitionStyle.HUD -> a.hud?.draw(c, from, to, TransitionMath.hud(bw, bh, e, tx.dir), e, animPaint)
        }
        c.restore()
    }

    /** Vorgerendertes Overlay der alten HUD-Seite (Scanzeilen + Raster); einmal je Bildgroesse und Farbe, danach nur noch gezeichnet. */
    private object HudOverlay {
        private var key = ""
        private var bmp: Bitmap? = null
        @Synchronized fun get(bw: Int, bh: Int, tx: ServerMessage.Transition, g0: TransitionMath.HudGeometry): Bitmap {
            val k = "$bw,$bh,${tx.shade},${tx.accent}"
            bmp?.let { if (k == key && !it.isRecycled) return it }
            val b = Bitmap.createBitmap(bw, bh, Bitmap.Config.ARGB_8888)
            val c = Canvas(b)
            // Scanzeilen: jede dritte Bildzeile mit der Schattenfarbe (1x3-Kachel, in Bildpixeln wiederholt).
            val tile = Bitmap.createBitmap(1, 3, Bitmap.Config.ARGB_8888)
            tile.setPixel(0, 0, tx.shade or (0xFF shl 24)); tile.setPixel(0, 1, 0); tile.setPixel(0, 2, 0)
            val scan = Paint().apply { shader = BitmapShader(tile, Shader.TileMode.REPEAT, Shader.TileMode.REPEAT); alpha = (g0.scanExtraAlpha * 255 + .5).toInt() }
            c.drawRect(0f, 0f, bw.toFloat(), bh.toFloat(), scan)
            val pts = ArrayList<Float>()
            var y = 0
            while (y < bh) { pts.add(0f); pts.add(y + .5f); pts.add(bw.toFloat()); pts.add(y + .5f); y += g0.grid }
            var x = 0
            while (x < bw) { pts.add(x + .5f); pts.add(0f); pts.add(x + .5f); pts.add(bh.toFloat()); x += g0.grid }
            c.drawLines(pts.toFloatArray(), Paint().apply { color = tx.accent; alpha = 40; strokeWidth = 1f; style = Paint.Style.STROKE })
            b.prepareToDraw()
            key = k; bmp = b
            return b
        }
    }

    /**
     * HUD-Scan des Servers (render.cpp, "HUD scan") mit Canvas-Primitiven: alte Seite abgedunkelt zur Schattenfarbe mit
     * Scanzeilen und Raster, Scanstrahl mit hellem Kern und quadratischem Glow, Nachleuchten und Eck-Klammern auf der Strahlkante.
     * Bewusst weggelassen (Leistung auf schwachen Geraeten): Skalenstriche, gestrichelte Schiene, Leiterbahnen mit Pulsen.
     * Alle Shader und Paints entstehen einmal je Uebergang; pro Bild werden nur Matrizen und Alpha gesetzt.
     */
    private class HudPainter(bw: Int, bh: Int, tx: ServerMessage.Transition) {
        private val g0 = TransitionMath.hud(bw, bh, 0.0, tx.dir)
        private val back = g0.back
        private val glowM = Matrix(); private val afterM = Matrix()
        // Alte Seite = Bild mit Schattenfarbe (ColorFilter SRC_ATOP, je Deckkraftstufe vorgebaut) + ein vorgerendertes Overlay
        // aus Scanzeilen und Raster (einmal je Groesse/Farbe, siehe HudOverlay): zwei Durchgaenge statt vier, keine Allokation je Bild.
        private val overlay: Bitmap = HudOverlay.get(bw, bh, tx, TransitionMath.hud(bw, bh, 0.0, tx.dir))
        private val shadeFilters = arrayOfNulls<ColorFilter>(256)
        private val fromPaint = Paint(Paint.FILTER_BITMAP_FLAG).apply { isFilterBitmap = true }
        private val rr = Rect()
        private val glowPaint = Paint()
        private val afterPaint = Paint()
        private val corePaint = Paint().apply {
            val k = 90 / 256f
            fun m(s: Int) = (((tx.accent shr s) and 0xFF) * (1 - k) + 255 * k + .5f).toInt()
            color = Color.rgb(m(16), m(8), m(0))
        }
        private val bracketPaint = Paint().apply { color = tx.accent2; alpha = 235 }
        private val glowShader: LinearGradient
        private val afterShader: LinearGradient

        init {
            // Quadratisch abfallender Glow: Gewicht (1-|d|/glow)^2, symmetrisch um die Strahlmitte.
            val n = 8
            val gc = IntArray(n + 1); val gp = FloatArray(n + 1)
            for (i in 0..n) {
                val f = 1.0 - kotlin.math.abs(i * 2.0 / n - 1.0)
                gc[i] = (tx.accent and 0xFFFFFF) or ((f * f * 255 + .5).toInt() shl 24); gp[i] = i / n.toFloat()
            }
            glowShader = LinearGradient(-g0.glow.toFloat(), 0f, g0.glow.toFloat(), 0f, gc, gp, Shader.TileMode.CLAMP)
            val ac = IntArray(n + 1); val ap = FloatArray(n + 1)
            for (i in 0..n) {
                val t = 1.0 - i / n.toDouble()
                ac[i] = (tx.accent2 and 0xFFFFFF) or ((t * t * 255 + .5).toInt() shl 24); ap[i] = i / n.toFloat()
            }
            afterShader = LinearGradient(0f, 0f, g0.trail.toFloat(), 0f, ac, ap, Shader.TileMode.CLAMP)
            glowPaint.shader = glowShader
            afterPaint.shader = afterShader
            val rgb = tx.shade and 0xFFFFFF
            val maxA = (0.46 * 255 + .5).toInt()   // groesste Schattendeckkraft (TransitionMath.hud: .46)
            for (al in 1..maxA) shadeFilters[al] = PorterDuffColorFilter((al shl 24) or rgb, PorterDuff.Mode.SRC_ATOP)
        }

        fun draw(c: Canvas, from: Bitmap, to: Bitmap, g: TransitionMath.HudGeometry, eased: Double, bmpPaint: Paint) {
            val w = g.width.toFloat(); val h = g.height.toFloat()
            // Neue Seite: unveraendert (Teilrechteck statt Clip).
            if (g.newHi > g.newLo) {
                rr.set(g.newLo, 0, g.newHi, g.height)
                c.drawBitmap(to, rr, rr, bmpPaint)
            }
            // Alte Seite: zur Schattenfarbe abgedunkelt (ColorFilter), darueber Scanzeilen und Raster (Overlay).
            if (g.oldHi > g.oldLo) {
                rr.set(g.oldLo, 0, g.oldHi, g.height)
                val al = (g.shadeAlpha * 255 + .5).toInt().coerceIn(0, 255)
                fromPaint.colorFilter = if (al > 0) shadeFilters[al] else null
                c.drawBitmap(from, rr, rr, fromPaint)
                c.drawBitmap(overlay, rr, rr, bmpPaint)
            }
            val line = g.line.toFloat()
            // Nachleuchten auf der neuen Seite hinter dem Strahl.
            afterM.setScale(back.toFloat(), 1f); afterM.postTranslate(line, 0f)
            afterShader.setLocalMatrix(afterM)
            afterPaint.alpha = (g.afterglowAlpha * 255 + .5).toInt()
            c.drawRect(minOf(line, line + back * g.trail), 0f, maxOf(line, line + back * g.trail), h, afterPaint)
            // Neon-Glow und heller Kern.
            glowM.setTranslate(line, 0f)
            glowShader.setLocalMatrix(glowM)
            c.drawRect(line - g.glow, 0f, line + g.glow, h, glowPaint)
            c.drawRect(line - 1f, 0f, line + 1f, h, corePaint)
            // Eck-Klammern an Ober- und Unterkante, auf der alten Seite des Strahls.
            val x0 = line - back * g.glow; val x1 = line - back * (g.glow + g.bracketLen)
            val xs = line - back * (g.glow + g.bracketLen - g.bracketThick)
            val padT = g.bracketPad.toFloat(); val th = g.bracketThick.toFloat(); val len = g.bracketLen.toFloat()
            c.drawRect(minOf(x0, x1), padT, maxOf(x0, x1), padT + th, bracketPaint)
            c.drawRect(minOf(xs, x1), padT, maxOf(xs, x1), padT + len, bracketPaint)
            c.drawRect(minOf(x0, x1), h - padT - th, maxOf(x0, x1), h - padT, bracketPaint)
            c.drawRect(minOf(xs, x1), h - padT - len, maxOf(xs, x1), h - padT, bracketPaint)
        }
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.actionMasked == MotionEvent.ACTION_DOWN) { downX = e.x; downY = e.y; swiped = false }
        gestures.onTouchEvent(e)
        when (e.actionMasked) {
            MotionEvent.ACTION_UP -> if (!swiped && abs(e.x - downX) < slop && abs(e.y - downY) < slop) {
                val shown = bmp ?: return true
                // Nach einem Zielwechsel zeigt das stehende Bild noch den vorherigen PC: kein Tippen dorthin weiterleiten.
                if (shownEpoch != epoch) return true
                if (shown.width != rw || shown.height != rh) return true
                val lb = Letterbox.compute(contentWidth, contentHeight, rw, rh)
                lb.toRender(e.x - insL, e.y - insT)?.let { (x, y) -> onTouchRender?.invoke(x, y) }
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean = super.performClick()

    // D-Pad LEFT/RIGHT auf das Wisch-Protokoll abgebildet (siehe DpadNavigation); CENTER/ENTER
    // bewusst ohne Aktion konsumiert. repeatCount != 0 (gehaltene Taste) wird verschluckt, damit
    // genau ein Tastendruck genau einen Seitenwechsel ausloest.
    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        val swipe = DpadNavigation.toSwipe(keyCode)
        if (swipe != null) {
            if (event.repeatCount == 0) onSwipe?.invoke(swipe)
            return true
        }
        if (DpadNavigation.isConsumedWithoutAction(keyCode)) return true
        return super.onKeyDown(keyCode, event)
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean {
        if (DpadNavigation.toSwipe(keyCode) != null || DpadNavigation.isConsumedWithoutAction(keyCode)) return true
        return super.onKeyUp(keyCode, event)
    }
}
