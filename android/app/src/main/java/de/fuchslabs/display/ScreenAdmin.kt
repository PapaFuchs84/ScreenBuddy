package de.fuchslabs.display

import android.app.admin.DeviceAdminReceiver
import android.app.admin.DevicePolicyManager
import android.content.ComponentName
import android.content.Context

/** Geraeteadministrator nur fuer "Bildschirm sperren" (lockNow). Keine weiteren Richtlinien. */
class ScreenAdmin : DeviceAdminReceiver() {
    companion object {
        fun component(ctx: Context) = ComponentName(ctx, ScreenAdmin::class.java)
        fun isActive(ctx: Context): Boolean =
            (ctx.getSystemService(Context.DEVICE_POLICY_SERVICE) as DevicePolicyManager).isAdminActive(component(ctx))
        /** Sperrt den Bildschirm; false, wenn nicht freigegeben (z. B. Android TV). */
        fun lockNow(ctx: Context): Boolean {
            if (!isActive(ctx)) return false
            return try { (ctx.getSystemService(Context.DEVICE_POLICY_SERVICE) as DevicePolicyManager).lockNow(); true } catch (_: Exception) { false }
        }
    }
}
