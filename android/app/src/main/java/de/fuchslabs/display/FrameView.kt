package de.fuchslabs.display

import android.content.Context
import android.graphics.*
import android.view.MotionEvent
import android.view.View
import de.fuchslabs.display.protocol.Letterbox
import de.fuchslabs.display.protocol.Protocol
import kotlin.math.abs

/** Zeichnet das JPEG-Frame mit Letterbox und sendet Taps in Renderkoordinaten. */
class FrameView(ctx: Context) : View(ctx) {
    var onTouchRender: ((Int, Int) -> Unit)? = null
    private var bmp: Bitmap? = null
    private var rw = 480
    private var rh = 320
    private val paint = Paint(Paint.FILTER_BITMAP_FLAG)
    private var downX = 0f
    private var downY = 0f
    private val slop = resources.displayMetrics.density * 16

    init { setBackgroundColor(Color.parseColor("#0E1628")) }

    fun setRenderSize(w: Int, h: Int) { rw = w; rh = h; invalidate() }

    fun setFrame(jpeg: ByteArray) {
        val b = BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size) ?: return
        bmp = b; invalidate()
    }

    override fun onDraw(c: Canvas) {
        val b = bmp ?: return
        val lb = Letterbox.compute(width, height, b.width.coerceAtLeast(1), b.height.coerceAtLeast(1))
        val dst = RectF(lb.left, lb.top, lb.left + b.width * lb.scale, lb.top + b.height * lb.scale)
        c.drawBitmap(b, null, dst, paint)
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> { downX = e.x; downY = e.y }
            MotionEvent.ACTION_UP -> if (abs(e.x - downX) < slop && abs(e.y - downY) < slop) {
                val lb = Letterbox.compute(width, height, rw, rh)
                lb.toRender(e.x, e.y)?.let { (x, y) -> onTouchRender?.invoke(x, y) }
                performClick()
            }
        }
        return true
    }

    override fun performClick(): Boolean = super.performClick()
}
