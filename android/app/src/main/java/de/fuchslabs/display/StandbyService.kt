package de.fuchslabs.display

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.PowerManager
import android.provider.Settings

/**
 * Bereitschaftsdienst ("Automatik"): sucht per mDNS den gekoppelten PC. Erscheint er, wird die Anzeige geoeffnet
 * und der Bildschirm eingeschaltet; verschwindet er (ScreenBuddy beendet, mDNS-Abmeldung), wird nach kurzer
 * Karenzzeit die Anzeige geschlossen und der Bildschirm gesperrt.
 */
class StandbyService : Service() {
    private val main = Handler(Looper.getMainLooper())
    private var discovery: Discovery? = null
    private var present = false
    private val goneTask = Runnable { onPcGone() }

    override fun onBind(i: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (!Store(this).autoMode || Store(this).paired() == null) { stopSelf(); return START_NOT_STICKY }
        startForeground(1, notification())
        if (discovery == null) {
            discovery = Discovery(this) { list -> onFound(list) }.also { it.start() }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        main.removeCallbacks(goneTask)
        discovery?.stop(); discovery = null
        super.onDestroy()
    }

    private fun onFound(list: List<FoundPc>) {
        val p = Store(this).paired() ?: return
        val here = list.any { it.host == p.host }
        if (here) {
            main.removeCallbacks(goneTask)
            if (!present) { present = true; showDisplay() }
        } else if (present) {
            main.removeCallbacks(goneTask)
            main.postDelayed(goneTask, GRACE_MS)
        }
    }

    private fun showDisplay() {
        wakeScreen()
        val i = Intent(this, DisplayActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_SINGLE_TOP)
        try {
            if (Build.VERSION.SDK_INT < 29 || Settings.canDrawOverlays(this)) startActivity(i)
            else {
                // Ohne Einblend-Freigabe nur Vollbild-Benachrichtigung (je nach Geraet/Hersteller eingeschraenkt).
                val pi = PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT)
                val n = Notification.Builder(this, CHANNEL_ALERT).setSmallIcon(R.mipmap.ic_launcher)
                    .setContentTitle(getString(R.string.app_name)).setContentText(getString(R.string.notif_open))
                    .setCategory(Notification.CATEGORY_CALL).setFullScreenIntent(pi, true).setContentIntent(pi).setAutoCancel(true).build()
                (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).notify(2, n)
            }
        } catch (_: Exception) {}
    }

    private fun onPcGone() {
        present = false
        sendBroadcast(Intent(ACTION_CLOSE).setPackage(packageName))
        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).cancel(2)
        // Telefon: sperren (Geraeteadmin). Android TV: kein App-Weg, Anzeige schliesst nur.
        if (!packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) ScreenAdmin.lockNow(this)
    }

    @Suppress("DEPRECATION")
    private fun wakeScreen() {
        try {
            val pm = getSystemService(Context.POWER_SERVICE) as PowerManager
            pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK or PowerManager.ACQUIRE_CAUSES_WAKEUP, "fuchslabs:wake").acquire(5000)
        } catch (_: Exception) {}
    }

    private fun notification(): Notification {
        val nm = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        nm.createNotificationChannel(NotificationChannel(CHANNEL_STANDBY, getString(R.string.notif_channel_standby), NotificationManager.IMPORTANCE_MIN))
        nm.createNotificationChannel(NotificationChannel(CHANNEL_ALERT, getString(R.string.notif_channel_alert), NotificationManager.IMPORTANCE_HIGH))
        val pi = PendingIntent.getActivity(this, 1, Intent(this, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE)
        return Notification.Builder(this, CHANNEL_STANDBY).setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle(getString(R.string.app_name)).setContentText(getString(R.string.notif_standby))
            .setContentIntent(pi).setOngoing(true).build()
    }

    companion object {
        const val ACTION_CLOSE = "de.fuchslabs.display.CLOSE"
        private const val CHANNEL_STANDBY = "standby"; private const val CHANNEL_ALERT = "alert"
        private const val GRACE_MS = 20_000L
        fun sync(ctx: Context) {
            val s = Store(ctx)
            val i = Intent(ctx, StandbyService::class.java)
            if (s.autoMode && s.paired() != null) ctx.startForegroundService(i) else ctx.stopService(i)
        }
    }
}
