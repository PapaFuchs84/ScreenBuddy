package de.fuchslabs.display

import android.content.Context
import android.graphics.*
import android.view.GestureDetector
import android.view.MotionEvent
import android.os.Build
import android.view.View
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import de.fuchslabs.display.protocol.Letterbox
import de.fuchslabs.display.protocol.Protocol
import de.fuchslabs.display.protocol.ImageLimits
import de.fuchslabs.display.protocol.SwipeClassifier
import de.fuchslabs.display.protocol.SwipeDir
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicReference
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
    private val latest = AtomicReference<ByteArray?>()

    /** Nimmt das Frame an; Decodierung im Hintergrund, nur das neueste Frame wird dekodiert. */
    fun setFrame(jpeg: ByteArray) {
        if (!ImageLimits.bytesOk(jpeg.size)) return
        if (latest.getAndSet(jpeg) == null) decoder.execute { decodeLatest() }
    }

    private fun decodeLatest() {
        val jpeg = latest.getAndSet(null) ?: return
        try {
            val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size, bounds)
            if (!ImageLimits.dimsOk(bounds.outWidth, bounds.outHeight)) return
            val b = BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size) ?: return
            val bg = b.getPixel(0, 0) // Letterbox-Hintergrund = Randfarbe des Bildes
            post { bmp = b; bgColor = bg or Color.BLACK; invalidate() }
        } catch (_: Exception) {} catch (_: OutOfMemoryError) {}
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (contentWidth > 0 && contentHeight > 0) onSizeChangedPx?.invoke(contentWidth, contentHeight)
    }

    override fun onDraw(c: Canvas) {
        val b = bmp ?: return
        c.drawColor(bgColor)
        val lb = Letterbox.compute(contentWidth, contentHeight, b.width.coerceAtLeast(1), b.height.coerceAtLeast(1))
        val ox = insL.toFloat(); val oy = insT.toFloat()
        val dst = RectF(ox + lb.left, oy + lb.top, ox + lb.left + b.width * lb.scale, oy + lb.top + b.height * lb.scale)
        c.drawBitmap(b, null, dst, paint)
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.actionMasked == MotionEvent.ACTION_DOWN) { downX = e.x; downY = e.y; swiped = false }
        gestures.onTouchEvent(e)
        when (e.actionMasked) {
            MotionEvent.ACTION_UP -> if (!swiped && abs(e.x - downX) < slop && abs(e.y - downY) < slop) {
                val shown = bmp ?: return true
                if (shown.width != rw || shown.height != rh) return true
                val lb = Letterbox.compute(contentWidth, contentHeight, rw, rh)
                lb.toRender(e.x - insL, e.y - insT)?.let { (x, y) -> onTouchRender?.invoke(x, y) }
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean = super.performClick()
}
