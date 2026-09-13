package com.androidcontrol.controler

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat

class ControlService : Service() {
    companion object {
        const val CHANNEL_ID = "controler_channel"
        const val NOTIF_ID = 2002
        fun start(context: Context) {
            val i = Intent(context, ControlService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) context.startForegroundService(i) else context.startService(i)
        }
        fun stop(context: Context) { context.stopService(Intent(context, ControlService::class.java)) }
        fun isRunning(context: Context): Boolean =
            context.getSharedPreferences("controler_prefs", Context.MODE_PRIVATE).getBoolean("running", false)
    }

    private var clipboardManager: ClipboardManager? = null

    override fun onCreate() {
        super.onCreate()
        createChannel()
        getSharedPreferences("controler_prefs", MODE_PRIVATE).edit().putBoolean("running", true).apply()
        clipboardManager = getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForeground(NOTIF_ID, buildNotification())
        return START_STICKY
    }

    override fun onDestroy() {
        getSharedPreferences("controler_prefs", MODE_PRIVATE).edit().putBoolean("running", false).apply()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun buildNotification(): Notification {
        val info = DeviceInfoProvider.get(this)
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentTitle("controler — Connected")
            .setContentText("${info.displayName} • Android ${info.androidVersion} • ${info.resolution}")
            .setOngoing(true)
            .setCategory(Notification.CATEGORY_SERVICE)
            .build()
    }

    private fun createChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val ch = NotificationChannel(CHANNEL_ID, "controler", NotificationManager.IMPORTANCE_LOW)
            ch.description = "controler service for desktop connection"
            (getSystemService(NotificationManager::class.java)).createNotificationChannel(ch)
        }
    }
}
