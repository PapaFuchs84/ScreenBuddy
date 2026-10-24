package de.fuchslabs.display

import android.content.Context
import android.graphics.*
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
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

    init { setBackgroundColor(Color.parseColor("#0E1628")) }

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
        if (w > 0 && h > 0) onSizeChangedPx?.invoke(w, h)
    }

    override fun onDraw(c: Canvas) {
        val b = bmp ?: return
        c.drawColor(bgColor)
        val lb = Letterbox.compute(width, height, b.width.coerceAtLeast(1), b.height.coerceAtLeast(1))
        val dst = RectF(lb.left, lb.top, lb.left + b.width * lb.scale, lb.top + b.height * lb.scale)
        c.drawBitmap(b, null, dst, paint)
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (e.actionMasked == MotionEvent.ACTION_DOWN) { downX = e.x; downY = e.y; swiped = false }
        gestures.onTouchEvent(e)
        when (e.actionMasked) {
            MotionEvent.ACTION_UP -> if (!swiped && abs(e.x - downX) < slop && abs(e.y - downY) < slop) {
                val lb = Letterbox.compute(width, height, rw, rh)
                lb.toRender(e.x, e.y)?.let { (x, y) -> onTouchRender?.invoke(x, y) }
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean = super.performClick()
}
