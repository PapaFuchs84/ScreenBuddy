package de.fuchslabs.display

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

/**
 * Optionaler Start beim Booten. Ab Android 10 duerfen Hintergrund-Receiver keine Activities starten; dort greift
 * zuverlaessig nur der Launcher-Modus (HOME-App wird vom System beim Boot gestartet).
 */
class BootReceiver : BroadcastReceiver() {
    override fun onReceive(ctx: Context, intent: Intent) {
        if (intent.action != Intent.ACTION_BOOT_COMPLETED) return
        val s = Store(ctx)
        if (s.bootStart && s.paired() != null) {
            try {
                ctx.startActivity(Intent(ctx, DisplayActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            } catch (_: Exception) {}
        }
    }
}
