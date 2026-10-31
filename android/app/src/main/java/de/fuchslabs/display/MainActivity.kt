package de.fuchslabs.display

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Typeface
import android.os.Bundle
import android.text.InputType
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.widget.*
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.journeyapps.barcodescanner.ScanContract
import com.journeyapps.barcodescanner.ScanOptions
import de.fuchslabs.display.protocol.HostPolicy
import de.fuchslabs.display.protocol.PairingUri
import de.fuchslabs.display.protocol.Protocol

/** Einrichtung: mDNS-Liste, QR-Kopplung, manuelle IP, Launcher-Einstellungen. */
class MainActivity : AppCompatActivity() {
    private lateinit var store: Store
    private lateinit var discovery: Discovery
    private lateinit var list: LinearLayout
    private lateinit var lastBtn: Button
    private lateinit var forgetBtn: Button
    private var found: List<FoundPc> = emptyList()
    private lateinit var host: EditText
    private lateinit var port: EditText
    private lateinit var pin: EditText
    /** Geraete ohne jede Kamera (z. B. viele Android-TV-Boxen) sollen den QR-Scan weder anbieten noch danach fragen. */
    private val hasCamera: Boolean by lazy { packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_ANY) }

    private val scan = registerForActivityResult(ScanContract()) { r ->
        val text = r.contents ?: return@registerForActivityResult
        val info = PairingUri.parse(text)
        if (info == null) toast(R.string.err_qr_invalid) else
            startActivity(Intent(this, DisplayActivity::class.java)
                .putExtra(DisplayActivity.EXTRA_HOST, info.host).putExtra(DisplayActivity.EXTRA_PORT, info.port)
                .putExtra(DisplayActivity.EXTRA_FP, info.fingerprint).putExtra(DisplayActivity.EXTRA_PIN, info.pin)
                .putExtra(DisplayActivity.EXTRA_NAME, info.host))
    }
    private val camPerm = registerForActivityResult(ActivityResultContracts.RequestPermission()) { ok ->
        if (ok) launchScan() else toast(R.string.err_camera)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Auch waehrend der Einrichtung darf der TV nicht in den Bildschirmschoner wechseln.
        // Das Window-Flag wirkt nur, solange diese Activity sichtbar ist.
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        store = Store(this)
        val text = getColor(R.color.ui_text); val textSec = getColor(R.color.ui_text_secondary); val onPrimary = getColor(R.color.ui_on_primary); val bg = getColor(R.color.ui_bg)
        val night = (resources.configuration.uiMode and android.content.res.Configuration.UI_MODE_NIGHT_MASK) == android.content.res.Configuration.UI_MODE_NIGHT_YES
        androidx.core.view.WindowInsetsControllerCompat(window, window.decorView).apply { isAppearanceLightStatusBars = !night; isAppearanceLightNavigationBars = !night }
        val dp = resources.displayMetrics.density
        fun pad(v: View, p: Int) = v.setPadding((p * dp).toInt(), (p * dp).toInt(), (p * dp).toInt(), (p * dp).toInt())
        fun label(s: Int) = TextView(this).apply { setText(s); setTextColor(text); textSize = 16f; typeface = Typeface.DEFAULT_BOLD; setPadding((4 * dp).toInt(), (20 * dp).toInt(), (4 * dp).toInt(), (6 * dp).toInt()) }
        fun gap(v: View, top: Int = 8) = v.apply { layoutParams = LinearLayout.LayoutParams(-1, -2).apply { topMargin = (top * dp).toInt() } }
        fun field(e: EditText) = e.apply { setTextColor(text); setHintTextColor(textSec); setBackgroundResource(R.drawable.field_bg); textSize = 16f; minHeight = (52 * dp).toInt(); setPadding((14 * dp).toInt(), 0, (14 * dp).toInt(), 0); isFocusable = true }
        fun button(s: Int, click: () -> Unit) = Button(this).apply {
            setText(s); setTextColor(onPrimary); setBackgroundResource(R.drawable.btn_primary); isAllCaps = false; textSize = 16f; typeface = Typeface.DEFAULT_BOLD
            minHeight = (52 * dp).toInt(); stateListAnimator = null; setOnClickListener { click() }
        }

        val col = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; pad(this, 20) }
        fun secondary(s: String, click: () -> Unit) = Button(this).apply {
            this.text = s; setTextColor(text); setBackgroundResource(R.drawable.btn_secondary); isAllCaps = false; textSize = 15f
            minHeight = (48 * dp).toInt(); stateListAnimator = null; setOnClickListener { click() }
        }
        col.addView(ImageView(this).apply { setImageResource(R.mipmap.ic_launcher) },
            LinearLayout.LayoutParams((72 * dp).toInt(), (72 * dp).toInt()))
        col.addView(TextView(this).apply { setText(R.string.title_connect); textSize = 24f; setTextColor(text); typeface = Typeface.DEFAULT_BOLD; pad(this, 4) })

        lastBtn = button(R.string.btn_scan) { store.paired()?.let { startActivity(Intent(this, DisplayActivity::class.java)) } }
        col.addView(gap(lastBtn))
        // Ohne Kamera (z. B. Android TV) QR-Einstieg ausblenden statt einer Berechtigung, die ohnehin scheitert;
        // manuelle Kopplung bleibt der einzige, hervorgehobene Weg.
        if (hasCamera) col.addView(gap(button(R.string.btn_scan) { requestScan() }))
        else col.addView(TextView(this).apply { setText(R.string.info_no_camera); setTextColor(textSec); textSize = 14f; pad(this, 4) })

        col.addView(label(R.string.section_found))
        list = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        col.addView(list)

        col.addView(label(R.string.section_manual))
        host = field(EditText(this)).apply { setHint(R.string.hint_host); inputType = InputType.TYPE_TEXT_VARIATION_URI; setSingleLine() }
        port = field(EditText(this)).apply { setHint(R.string.hint_port); setText(Protocol.DEFAULT_PORT.toString()); inputType = InputType.TYPE_CLASS_NUMBER; setSingleLine() }
        pin = field(EditText(this)).apply { setHint(R.string.hint_pin); inputType = InputType.TYPE_CLASS_NUMBER; setSingleLine() }
        col.addView(gap(host)); col.addView(gap(port)); col.addView(gap(pin))
        col.addView(gap(button(R.string.btn_manual_pair) { manualPair() }, 12))

        col.addView(label(R.string.section_orientation))
        val modes = listOf("landscape" to R.string.orient_landscape, "auto" to R.string.orient_auto, "portrait" to R.string.orient_portrait)
        col.addView(RadioGroup(this).apply {
            orientation = RadioGroup.VERTICAL
            val ids = modes.map { (key, res) ->
                RadioButton(this@MainActivity).apply { id = View.generateViewId(); setText(res); tag = key; setTextColor(text); textSize = 16f; minHeight = (48 * dp).toInt() }
                    .also { addView(it) }
            }
            ids.firstOrNull { it.tag == store.orientation }?.isChecked = true
            setOnCheckedChangeListener { g, id -> store.orientation = g.findViewById<View>(id).tag as String }
        })

        col.addView(label(R.string.section_launcher))
        val sw1 = Switch(this).apply {
            setTextColor(text); minHeight = (48 * dp).toInt(); setText(R.string.setting_launcher); isChecked = store.launcherMode
            setOnCheckedChangeListener { _, on -> setLauncher(on) }
        }
        col.addView(sw1)
        col.addView(TextView(this).apply { setText(R.string.setting_launcher_desc); setTextColor(textSec); textSize = 14f; pad(this, 4) })
        col.addView(Switch(this).apply {
            setTextColor(text); minHeight = (48 * dp).toInt(); setText(R.string.setting_boot); isChecked = store.bootStart
            setOnCheckedChangeListener { _, on -> store.bootStart = on }
        })
        forgetBtn = secondary(getString(R.string.btn_forget)) { store.forget(); sw1.isChecked = false; refresh() }
        col.addView(gap(forgetBtn, 20))

        setContentView(ScrollView(this).apply { setBackgroundColor(bg); isFillViewport = true; addView(col) })
        discovery = Discovery(this) { found = it; refresh() }
    }

    override fun onStart() { super.onStart(); discovery.start(); refresh() }
    override fun onStop() { discovery.stop(); super.onStop() }

    private fun refresh() {
        val dp = resources.displayMetrics.density
        val p = store.paired()
        lastBtn.visibility = if (p != null) View.VISIBLE else View.GONE
        if (p != null) lastBtn.text = getString(R.string.btn_connect_last, p.name)
        forgetBtn.visibility = if (p != null) View.VISIBLE else View.GONE
        // mDNS-Funde ueberschreiben nie den gespeicherten Host/Port (Spoofing-Schutz); nur manuell neu koppeln.
        list.removeAllViews()
        if (found.isEmpty()) list.addView(TextView(this).apply { setText(R.string.found_none); setTextColor(getColor(R.color.ui_text_secondary)) })
        for (f in found) list.addView(Button(this).apply {
            text = "${f.name}  (${f.host}:${f.port})"
            setTextColor(getColor(R.color.ui_text)); setBackgroundResource(R.drawable.btn_secondary); isAllCaps = false; textSize = 15f
            minHeight = (48 * dp).toInt(); stateListAnimator = null
            layoutParams = LinearLayout.LayoutParams(-1, -2).apply { topMargin = (8 * dp).toInt() }
            setOnClickListener { host.setText(f.host); port.setText(f.port.toString()); pin.requestFocus() }
        })
    }

    private fun requestScan() {
        if (!hasCamera) return // Abwehr falls doch aufgerufen; der Button ist bereits ausgeblendet.
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) launchScan()
        else camPerm.launch(Manifest.permission.CAMERA)
    }

    private fun launchScan() {
        scan.launch(ScanOptions().setDesiredBarcodeFormats(ScanOptions.QR_CODE).setBeepEnabled(false).setOrientationLocked(false))
    }

    private fun manualPair() {
        val h = host.text.toString().trim(); val prt = port.text.toString().toIntOrNull(); val pn = pin.text.toString().trim()
        if (!HostPolicy.isAllowed(h) || prt == null || !HostPolicy.isValidPort(prt) || pn.length != 6 || !pn.all(Char::isDigit)) { toast(R.string.err_input); return }
        // Ohne Fingerprint (null): DisplayActivity zeigt ihn zur Bestaetigung (TOFU), bevor die PIN gesendet wird.
        startActivity(Intent(this, DisplayActivity::class.java)
            .putExtra(DisplayActivity.EXTRA_HOST, h).putExtra(DisplayActivity.EXTRA_PORT, prt)
            .putExtra(DisplayActivity.EXTRA_PIN, pn).putExtra(DisplayActivity.EXTRA_NAME, h))
    }

    private fun setLauncher(on: Boolean) {
        store.launcherMode = on
        if (on) // Oeffnet den Android-Dialog zur Auswahl der Standard-Home-App.
            try { startActivity(Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) } catch (_: Exception) {}
    }

    private fun toast(r: Int) = Toast.makeText(this, r, Toast.LENGTH_LONG).show()
}
