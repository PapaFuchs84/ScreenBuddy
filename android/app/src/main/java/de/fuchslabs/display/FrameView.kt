package de.fuchslabs.display

import android.content.Context
import android.graphics.*
import android.view.GestureDetector
import android.view.KeyEvent
import android.view.MotionEvent
import android.os.Build
import android.view.View
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
        invalidate()
        if (contentWidth > 0 && contentHeight > 0) onSizeChangedPx?.invoke(contentWidth, contentHeight)
    }

    fun setRenderSize(w: Int, h: Int) { rw = w; rh = h; invalidate() }

    private val decoder = Executors.newSingleThreadExecutor { r -> Thread(r, "frame-decode").apply { isDaemon = true } }
    /** Neuestes noch nicht dekodiertes Frame, zusammen mit der Epoche, in der es angenommen wurde. */
    private val latest = GenerationSlot<ByteArray>()

    private class Decoded(val epoch: Int, val bitmap: Bitmap, val bg: Int)
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
    private val showNext = android.view.Choreographer.FrameCallback {
        showScheduled = false
        freeDrawnOut()
        var d: Decoded? = null
        var more = false
        synchronized(queue) {
            while (queue.isNotEmpty()) {
                val q = queue.removeFirst()
                if (q.epoch == epoch) { d = q; break } else release(q.bitmap)
            }
            more = queue.isNotEmpty()
        }
        d?.let { show(it) }
        if (more) scheduleShow()
    }
    private val scheduleShowRunnable = Runnable { scheduleShow() }

    /** Nur Main-Thread. */
    private fun scheduleShow() {
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
        if (b.isMutable) synchronized(pool) { if (pool.size < 3) { pool.addLast(b); return } }
        b.recycle()
    }

    /**
     * Nimmt das Frame an; Decodierung im Hintergrund, nur das neueste Frame wird dekodiert. Die Epoche wird hier
     * vergeben und bleibt am Frame: ein noch fuer ein frueheres Ziel geplanter Decoder kann ein neues Frame
     * dadurch nicht der alten Epoche zuordnen. Nur auf dem Main-Thread aufrufen.
     */
    fun setFrame(jpeg: ByteArray) {
        if (!ImageLimits.bytesOk(jpeg.size)) return
        if (latest.offer(epoch, jpeg)) decoder.execute { decodeLatest() }
    }

    /**
     * Verwirft wartende, noch nicht angezeigte Bilder (Stopp oder Zielwechsel). Das angezeigte Bild bleibt stehen.
     * Nur auf dem Main-Thread aufrufen.
     */
    fun clearPending() {
        epoch++ // zuerst: ein gerade laufender Decoder liefert danach nichts mehr aus
        latest.advance(epoch) // wartendes Frame der alten Epoche verwerfen
        removeCallbacks(scheduleShowRunnable)
        if (showScheduled) { android.view.Choreographer.getInstance().removeFrameCallback(showNext); showScheduled = false }
        val dropped = synchronized(queue) { val l = queue.toList(); queue.clear(); l }
        dropped.forEach { release(it.bitmap) } // nie angezeigt, daher gefahrlos wiederverwendbar
    }

    private fun decodeLatest() {
        // Frame und Epoche gemeinsam entnehmen: massgeblich ist die Epoche bei setFrame(), nicht die beim Dekodieren.
        val f = latest.take() ?: return
        val e = f.generation
        val jpeg = f.value
        try {
            val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, bounds)
            if (!ImageLimits.dimsOk(bounds.outWidth, bounds.outHeight)) return
            val reuse = obtain(bounds.outWidth, bounds.outHeight)
            val opts = BitmapFactory.Options().apply { inMutable = true; inBitmap = reuse }
            val b = (try { BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, opts) } catch (_: IllegalArgumentException) { null })
                ?: run { reuse?.let { release(it) }; BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, BitmapFactory.Options().apply { inMutable = true }) } ?: return
            val bg = b.getPixel(0, 0) // Letterbox-Hintergrund = Randfarbe des Bildes
            // Hoechstens zwei warten; ein drittes verdraengt das aelteste (nie angezeigt, daher gefahrlos zurueck).
            val drop = synchronized(queue) {
                queue.addLast(Decoded(e, b, bg))
                if (queue.size > 2) queue.removeFirst() else null
            }
            drop?.let { release(it.bitmap) }
            post(scheduleShowRunnable)
        } catch (_: Exception) {} catch (_: OutOfMemoryError) {}
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (contentWidth > 0 && contentHeight > 0) onSizeChangedPx?.invoke(contentWidth, contentHeight)
    }

    override fun onDraw(c: Canvas) {
        val b = bmp ?: return
        if (lastDrawn !== b) {
            lastDrawn?.let { drawnOut.add(it); scheduleCleanup() }
            lastDrawn = b
        }
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
