package de.fuchslabs.display

import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.util.DisplayMetrics
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.TextView
import android.widget.FrameLayout
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.activity.OnBackPressedCallback

/** Vollbild-Anzeige: immersiv, Bildschirm bleibt an, Auto-Reconnect. Extras: pairing (Kopplung) optional. */
class DisplayActivity : AppCompatActivity(), DisplayClient.Listener {
    private lateinit var store: Store
    private lateinit var frameView: FrameView
    private lateinit var status: TextView
    private var client: DisplayClient? = null
    private var pairingPin: String? = null
    private var lastBack = 0L
    private var trustDialog: AlertDialog? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        store = Store(this)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        frameView = FrameView(this)
        status = TextView(this).apply {
            setTextColor(getColor(R.color.fl_creme)); textSize = 16f
            setBackgroundColor(0xAA14213D.toInt()); setPadding(32, 16, 32, 16)
        }
        val root = FrameLayout(this)
        root.addView(frameView, FrameLayout.LayoutParams(-1, -1))
        root.addView(status, FrameLayout.LayoutParams(-2, -2, android.view.Gravity.CENTER))
        setContentView(root)
        requestedOrientation = when (store.orientation) {
            "auto" -> ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR
            "portrait" -> ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT
            else -> ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
        }
        frameView.onTouchRender = { x, y -> client?.sendTouch(x, y) }
        frameView.onSwipe = { dir -> client?.sendSwipe(dir) }
        frameView.onSizeChangedPx = { w, h -> client?.updateSize(w, h) }
        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                val n = System.currentTimeMillis()
                if (n - lastBack < 2000) {
                    // Zurueck zur Einrichtung (im Launcher-Modus der einzige Weg zu den Einstellungen).
                    startActivity(Intent(this@DisplayActivity, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
                    finish()
                } else { lastBack = n; Toast.makeText(this@DisplayActivity, R.string.exit_hint, Toast.LENGTH_SHORT).show() }
            }
        })
        hideSystemUi()
    }

    override fun onStart() {
        super.onStart()
        startClient()
    }

    override fun onStop() {
        trustDialog?.dismiss(); trustDialog = null
        client?.stop(); client = null
        super.onStop()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) hideSystemUi()
    }

    private fun hideSystemUi() {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).apply {
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            hide(WindowInsetsCompat.Type.systemBars())
        }
    }

    // configChanges faengt Rotation ab; die neue Groesse meldet FrameView.onSizeChanged. Layout neu anstossen.
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        hideSystemUi()
    }

    /** Tatsaechliche Pixelgroesse der Anzeige in aktueller Rotation (Vollbild, ohne Systemleisten-Abzug). */
    private fun realSize(): Pair<Int, Int> {
        val v = if (frameView.width > 0 && frameView.height > 0) frameView.width to frameView.height else null
        if (v != null) return v
        val dm = DisplayMetrics()
        @Suppress("DEPRECATION") windowManager.defaultDisplay.getRealMetrics(dm)
        return dm.widthPixels to dm.heightPixels
    }

    private fun startClient() {
        val (sw, sh) = realSize()
        val name = Build.MODEL ?: "Android"
        val t: DisplayClient.Target
        val pin = intent.getStringExtra(EXTRA_PIN)
        if (pin != null) {
            pairingPin = pin
            t = DisplayClient.Target(
                intent.getStringExtra(EXTRA_HOST) ?: "", intent.getIntExtra(EXTRA_PORT, 8777),
                intent.getStringExtra(EXTRA_FP), pin = pin,
                screenW = sw, screenH = sh, deviceName = name
            )
        } else {
            val p = store.paired()
            if (p == null) { startActivity(Intent(this, MainActivity::class.java)); finish(); return }
            t = DisplayClient.Target(p.host, p.port, p.fingerprint, key = p.key,
                screenW = sw, screenH = sh, deviceName = name)
        }
        client = DisplayClient(t, this).also { it.start() }
    }

    // --- DisplayClient.Listener (Main-Thread) ---
    override fun onState(state: DisplayClient.State) {
        status.visibility = if (state == DisplayClient.State.CONNECTED) View.GONE else View.VISIBLE
        status.setText(when (state) {
            DisplayClient.State.CONNECTING -> if (pairingPin != null) R.string.status_pairing else R.string.status_connecting
            DisplayClient.State.RECONNECTING -> R.string.status_reconnecting
            else -> R.string.status_idle
        })
    }

    override fun onUnverifiedFingerprint(fp: String, trust: () -> Unit) {
        val shown = fp.chunked(2).joinToString(":").uppercase().chunked(48).joinToString("\n")
        trustDialog = AlertDialog.Builder(this).setTitle(R.string.dlg_fp_title)
            .setMessage(getString(R.string.dlg_fp_msg, shown))
            .setPositiveButton(R.string.dlg_trust) { _, _ -> trust() }
            .setNegativeButton(R.string.dlg_cancel) { _, _ -> finish() }
            .setCancelable(false).show()
    }

    override fun onPaired(key: String, fingerprint: String) {
        val host = intent.getStringExtra(EXTRA_HOST) ?: return
        val port = intent.getIntExtra(EXTRA_PORT, 8777)
        val name = intent.getStringExtra(EXTRA_NAME) ?: host
        store.savePaired(PairedPc(host, port, fingerprint, name, key))
        Toast.makeText(this, getString(R.string.paired_ok, name), Toast.LENGTH_SHORT).show()
        // Danach normale Anmeldung mit Schluessel (Protokoll: "Danach hello mit key").
        client?.stop(); client = null
        intent.removeExtra(EXTRA_PIN); pairingPin = null
        startClient()
    }

    override fun onReady(w: Int, h: Int) = frameView.setRenderSize(w, h)
    override fun onFrame(jpeg: ByteArray) = frameView.setFrame(jpeg)

    override fun onFatal(kind: DisplayClient.Fatal, detail: String) {
        val msg = when (kind) {
            DisplayClient.Fatal.CERT_MISMATCH -> getString(R.string.err_cert)
            DisplayClient.Fatal.SERVER_ERROR -> getString(R.string.err_server, detail)
            DisplayClient.Fatal.INVALID_TARGET -> getString(R.string.err_input)
        }
        AlertDialog.Builder(this).setMessage(msg)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                startActivity(Intent(this, MainActivity::class.java)); finish()
            }.setCancelable(false).show()
    }

    companion object {
        const val EXTRA_HOST = "host"; const val EXTRA_PORT = "port"; const val EXTRA_FP = "fp"
        const val EXTRA_PIN = "pin"; const val EXTRA_NAME = "name"
    }
}
