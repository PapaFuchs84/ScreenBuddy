package de.fuchslabs.display

import android.app.AlarmManager
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
import android.os.SystemClock
import android.provider.Settings
import android.util.Log
import de.fuchslabs.display.protocol.ProbePolicy
import de.fuchslabs.display.protocol.StandbyPresence

/**
 * Bereitschaftsdienst ("Automatik"): sucht per mDNS den gekoppelten PC. Erscheint er, wird die Anzeige geoeffnet
 * und der Bildschirm eingeschaltet; verschwindet er (ScreenBuddy beendet, mDNS-Abmeldung), wird nach kurzer
 * Karenzzeit die Anzeige geschlossen und der Bildschirm gesperrt.
 */
class StandbyService : Service() {
    private val main = Handler(Looper.getMainLooper())
    private var discovery: Discovery? = null
    private val presence = StandbyPresence()
    private var target: PairedPc? = null
    private var active = false
    private val policy = ProbePolicy()
    private var probing = false
    private val goneTask = Runnable { onPcGone() }

    override fun onBind(i: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val store = Store(this)
        val paired = store.paired()
        if (!store.autoMode || paired == null) { stopSelf(); return START_NOT_STICKY }
        startForeground(1, notification())
        Log.d(TAG, "Standby started")
        active = true
        if (target != paired) {
            main.removeCallbacks(goneTask)
            presence.reset()
            policy.reset()
            discovery?.stop(); discovery = null
            target = paired
        }
        if (discovery == null) {
            discovery = Discovery(this) { list -> onFound(list) }.also { it.start() }
        }
        // Gesperrte Geraete (z. B. Redmi 8) empfangen oft kein mDNS: zusaetzlich periodisch per TCP pruefen.
        scheduleProbe()
        runProbe()
        return START_STICKY
    }

    override fun onDestroy() {
        Log.d(TAG, "Standby stopped")
        active = false
        cancelProbe()
        main.removeCallbacks(goneTask)
        presence.reset()
        policy.reset()
        discovery?.stop(); discovery = null
        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).cancel(2)
        super.onDestroy()
    }

    private fun onFound(list: List<FoundPc>) {
        if (!isEnabled()) return
        val p = target ?: return
        val here = list.any { it.host == p.host && it.port == p.port }
        Log.d(TAG, "Discovery update: count=${list.size}, pairedMatch=$here")
        policy.onMdns(here)
        applyPresence(here)
    }

    private fun applyPresence(here: Boolean) {
        val now = SystemClock.elapsedRealtime()
        val opened = presence.update(here, now)
        main.removeCallbacks(goneTask)
        presence.remainingGraceMs(now)?.let { main.postDelayed(goneTask, it) }
        if (opened) showDisplay()
    }

    private fun isEnabled(): Boolean {
        if (!active) return false
        val store = Store(this)
        return store.autoMode && target != null && store.paired() == target
    }

    private fun showDisplay() {
        Log.d(TAG, "Opening display; overlay=${Settings.canDrawOverlays(this)}")
        val i = Intent(this, DisplayActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_SINGLE_TOP)
        try {
            if (Build.VERSION.SDK_INT < 29 || Settings.canDrawOverlays(this)) {
                startActivity(i)
                // Nur beim automatischen Oeffnen wecken, nicht fuer die Benachrichtigung.
                wakeScreen()
                return
            }
        } catch (_: Exception) {}
        // Keine Anruf-/Alarm-Vollbildberechtigung fuer einen Display-Client beanspruchen.
        val pi = PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT)
        val n = Notification.Builder(this, CHANNEL_ALERT).setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle(getString(R.string.app_name)).setContentText(getString(R.string.notif_open))
            .setCategory(Notification.CATEGORY_STATUS).setContentIntent(pi).setAutoCancel(true).build()
        try { (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).notify(2, n) } catch (_: SecurityException) {}
    }

    private fun onPcGone() {
        if (!isEnabled() || !presence.expire(SystemClock.elapsedRealtime())) return
        Log.d(TAG, "Presence grace expired; closing display")
        sendBroadcast(Intent(ACTION_CLOSE).setPackage(packageName))
        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).cancel(2)
        // Telefon: sperren (Geraeteadmin). Android TV: kein App-Weg, Anzeige schliesst nur.
        if (!packageManager.hasSystemFeature(PackageManager.FEATURE_LEANBACK)) ScreenAdmin.lockNow(this)
    }

    private fun probePendingIntent(): PendingIntent = PendingIntent.getForegroundService(
        this, 3, Intent(this, StandbyService::class.java).setAction(ACTION_PROBE),
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT)

    /** Inexakter Alarm, der auch im Ruhezustand ausloest (kein Sonderrecht noetig). */
    private fun scheduleProbe() {
        try {
            (getSystemService(ALARM_SERVICE) as AlarmManager).setAndAllowWhileIdle(
                AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + PROBE_INTERVAL_MS, probePendingIntent())
        } catch (e: Exception) { Log.w(TAG, "Probe alarm failed: ${e.javaClass.simpleName}") }
    }

    private fun cancelProbe() {
        try { (getSystemService(ALARM_SERVICE) as AlarmManager).cancel(probePendingIntent()) } catch (_: Exception) {}
    }

    private fun runProbe() {
        val p = target ?: return
        if (probing || !isEnabled()) return
        // Bei ausgeschaltetem Bildschirm ist mDNS auf manchen Geraeten blind: dann gilt es nicht als Beleg.
        val interactive = (getSystemService(POWER_SERVICE) as PowerManager).isInteractive
        if (!policy.shouldProbe(presence.present, interactive)) return
        probing = true
        val lock = try {
            (getSystemService(POWER_SERVICE) as PowerManager).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "fuchslabs:probe")
                .also { it.setReferenceCounted(false); it.acquire(PROBE_LOCK_MS) }
        } catch (_: Exception) { null }
        Thread {
            val ok = PcProbe.reachable(p.host, p.port)
            main.post {
                probing = false
                try { if (lock?.isHeld == true) lock.release() } catch (_: Exception) {}
                if (!isEnabled() || target != p) return@post
                Log.d(TAG, "Probe ${p.host}:${p.port} reachable=$ok")
                when (policy.onProbe(ok, interactive)) {
                    true -> applyPresence(true)
                    false -> applyPresence(false)
                    null -> {}
                }
            }
        }.start()
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
        private const val TAG = "CompanionStandby"
        const val ACTION_CLOSE = "de.fuchslabs.display.CLOSE"
        private const val ACTION_PROBE = "de.fuchslabs.display.PROBE"
        private const val PROBE_INTERVAL_MS = 60_000L
        private const val PROBE_LOCK_MS = 8_000L
        private const val CHANNEL_STANDBY = "standby"; private const val CHANNEL_ALERT = "alert"
        fun sync(ctx: Context) {
            val s = Store(ctx)
            val i = Intent(ctx, StandbyService::class.java)
            if (s.autoMode && s.paired() != null) ctx.startForegroundService(i) else ctx.stopService(i)
        }
    }
}
