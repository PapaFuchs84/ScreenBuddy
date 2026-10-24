package de.fuchslabs.display

import android.app.Activity
import android.content.Intent
import android.os.Bundle

/** HOME-Einstiegspunkt (nur aktiv, wenn "Als Startbildschirm nutzen" eingeschaltet ist). */
class LauncherActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val target = if (Store(this).paired() != null) DisplayActivity::class.java else MainActivity::class.java
        startActivity(Intent(this, target))
        finish()
    }
}
