package de.fuchslabs.display

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Typeface
import android.os.Bundle
import android.text.InputType
import android.view.Gravity
import android.view.View
import android.widget.*
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.journeyapps.barcodescanner.ScanContract
import com.journeyapps.barcodescanner.ScanOptions
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
        store = Store(this)
        val orange = getColor(R.color.fl_orange); val navy = getColor(R.color.fl_navy); val creme = getColor(R.color.fl_creme)
        val dp = resources.displayMetrics.density
        fun pad(v: View, p: Int) = v.setPadding((p * dp).toInt(), (p * dp).toInt(), (p * dp).toInt(), (p * dp).toInt())
        fun label(s: Int) = TextView(this).apply { setText(s); setTextColor(navy); textSize = 16f; typeface = Typeface.DEFAULT_BOLD; pad(this, 4) }
        fun button(s: Int, click: () -> Unit) = Button(this).apply {
            setText(s); setTextColor(creme); setBackgroundColor(orange); setOnClickListener { click() }
        }

        val col = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; pad(this, 16) }
        col.addView(ImageView(this).apply { setImageResource(R.mipmap.ic_launcher) },
            LinearLayout.LayoutParams((72 * dp).toInt(), (72 * dp).toInt()))
        col.addView(TextView(this).apply { setText(R.string.title_connect); textSize = 22f; setTextColor(navy); typeface = Typeface.DEFAULT_BOLD; pad(this, 4) })

        lastBtn = button(R.string.btn_scan) { store.paired()?.let { startActivity(Intent(this, DisplayActivity::class.java)) } }
        col.addView(lastBtn)
        col.addView(button(R.string.btn_scan) { requestScan() })

        col.addView(label(R.string.section_found))
        list = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        col.addView(list)

        col.addView(label(R.string.section_manual))
        host = EditText(this).apply { setHint(R.string.hint_host); inputType = InputType.TYPE_TEXT_VARIATION_URI; setSingleLine() }
        port = EditText(this).apply { setHint(R.string.hint_port); setText(Protocol.DEFAULT_PORT.toString()); inputType = InputType.TYPE_CLASS_NUMBER; setSingleLine() }
        pin = EditText(this).apply { setHint(R.string.hint_pin); inputType = InputType.TYPE_CLASS_NUMBER; setSingleLine() }
        col.addView(host); col.addView(port); col.addView(pin)
        col.addView(button(R.string.btn_manual_pair) { manualPair() })

        col.addView(label(R.string.section_launcher))
        val sw1 = Switch(this).apply {
            setText(R.string.setting_launcher); isChecked = store.launcherMode
            setOnCheckedChangeListener { _, on -> setLauncher(on) }
        }
        col.addView(sw1)
        col.addView(TextView(this).apply { setText(R.string.setting_launcher_desc); setTextColor(getColor(R.color.fl_nebel)); textSize = 13f })
        col.addView(Switch(this).apply {
            setText(R.string.setting_boot); isChecked = store.bootStart
            setOnCheckedChangeListener { _, on -> store.bootStart = on }
        })
        forgetBtn = Button(this).apply { setText(R.string.btn_forget); setOnClickListener { store.forget(); sw1.isChecked = false; refresh() } }
        col.addView(forgetBtn)

        setContentView(ScrollView(this).apply { setBackgroundColor(creme); addView(col) })
        discovery = Discovery(this) { found = it; refresh() }
    }

    override fun onStart() { super.onStart(); discovery.start(); refresh() }
    override fun onStop() { discovery.stop(); super.onStop() }

    private fun refresh() {
        val p = store.paired()
        lastBtn.visibility = if (p != null) View.VISIBLE else View.GONE
        if (p != null) lastBtn.text = getString(R.string.btn_connect_last, p.name)
        forgetBtn.visibility = if (p != null) View.VISIBLE else View.GONE
        // Gekoppelter PC per mDNS gefunden (z. B. neue IP): Adresse aktualisieren.
        if (p != null) found.firstOrNull { it.name == p.name }?.let { if (it.host != p.host) store.updateHost(it.host, it.port) }
        list.removeAllViews()
        if (found.isEmpty()) list.addView(TextView(this).apply { setText(R.string.found_none); setTextColor(getColor(R.color.fl_nebel)) })
        for (f in found) list.addView(Button(this).apply {
            text = "${f.name}  (${f.host}:${f.port})"
            setOnClickListener { host.setText(f.host); port.setText(f.port.toString()); pin.requestFocus() }
        })
    }

    private fun requestScan() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) launchScan()
        else camPerm.launch(Manifest.permission.CAMERA)
    }

    private fun launchScan() {
        scan.launch(ScanOptions().setDesiredBarcodeFormats(ScanOptions.QR_CODE).setBeepEnabled(false).setOrientationLocked(false))
    }

    private fun manualPair() {
        val h = host.text.toString().trim(); val prt = port.text.toString().toIntOrNull(); val pn = pin.text.toString().trim()
        if (h.isEmpty() || prt == null || prt !in 1..65535 || pn.length != 6 || !pn.all(Char::isDigit)) { toast(R.string.err_input); return }
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
