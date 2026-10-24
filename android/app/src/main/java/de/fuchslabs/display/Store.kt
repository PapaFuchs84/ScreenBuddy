package de.fuchslabs.display

import android.content.ComponentName
import android.content.Context
import android.content.pm.PackageManager
import de.fuchslabs.display.protocol.Protocol

data class PairedPc(val host: String, val port: Int, val fingerprint: String, val name: String, val key: String)

class Store(private val ctx: Context) {
    private val sp = ctx.getSharedPreferences("fuchslabs", Context.MODE_PRIVATE)

    fun paired(): PairedPc? {
        val host = sp.getString("host", null) ?: return null
        val fp = sp.getString("fp", null) ?: return null
        val enc = sp.getString("key_enc", null) ?: return null
        val key = KeyVault.decrypt(enc) ?: return null
        return PairedPc(host, sp.getInt("port", Protocol.DEFAULT_PORT), fp, sp.getString("name", host) ?: host, key)
    }

    fun savePaired(p: PairedPc) {
        sp.edit().putString("host", p.host).putInt("port", p.port).putString("fp", p.fingerprint)
            .putString("name", p.name).putString("key_enc", KeyVault.encrypt(p.key)).apply()
    }

    /** Aktualisiert nur den Host (z. B. nach mDNS-Fund mit neuer IP). */
    fun updateHost(host: String, port: Int) { sp.edit().putString("host", host).putInt("port", port).apply() }

    fun forget() { sp.edit().clear().apply(); KeyVault.delete(); setLauncherEnabled(false) }

    var bootStart: Boolean
        get() = sp.getBoolean("boot", false)
        set(v) { sp.edit().putBoolean("boot", v).apply() }

    var launcherMode: Boolean
        get() = sp.getBoolean("launcher", false)
        set(v) { sp.edit().putBoolean("launcher", v).apply(); setLauncherEnabled(v) }

    private fun setLauncherEnabled(on: Boolean) {
        ctx.packageManager.setComponentEnabledSetting(
            ComponentName(ctx, LauncherActivity::class.java),
            if (on) PackageManager.COMPONENT_ENABLED_STATE_ENABLED else PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP
        )
    }
}
