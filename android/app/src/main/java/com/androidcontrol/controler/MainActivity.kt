package com.androidcontrol.controler

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        val info = DeviceInfoProvider.get(this)

        val connectionStatus = findViewById<TextView>(R.id.connectionStatus)
        val deviceName = findViewById<TextView>(R.id.deviceName)
        val androidVersion = findViewById<TextView>(R.id.androidVersion)
        val resolution = findViewById<TextView>(R.id.resolution)
        val battery = findViewById<TextView>(R.id.battery)
        val serial = findViewById<TextView>(R.id.serial)
        val serviceStatus = findViewById<TextView>(R.id.serviceStatus)
        val clipboardStatus = findViewById<TextView>(R.id.clipboardStatus)
        val fileStatus = findViewById<TextView>(R.id.fileStatus)
        val btnToggle = findViewById<Button>(R.id.btnToggleService)
        val btnCopy = findViewById<Button>(R.id.btnCopy)
        val btnPaste = findViewById<Button>(R.id.btnPaste)

        deviceName.text = info.displayName
        androidVersion.text = "Android ${info.androidVersion} • API ${info.apiLevel}"
        resolution.text = info.resolution
        battery.text = if (info.batteryLevel >= 0) "${info.batteryLevel}% Battery" else "Battery —"
        serial.text = info.serial

        fun refresh() {
            val running = ControlService.isRunning(this)
            connectionStatus.text = if (running) "● Desktop connected" else "○ Waiting for desktop"
            connectionStatus.setTextColor(if (running) getColor(R.color.teal) else getColor(R.color.text_secondary))
            serviceStatus.text = if (running) "✓ Connection Service" else "○ Connection Service"
            clipboardStatus.text = "✓ Clipboard"
            fileStatus.text = "✓ File Transfer (via ADB)"
            btnToggle.text = if (running) "Stop Service" else "Start Service"
        }
        refresh()

        btnToggle.setOnClickListener {
            if (ControlService.isRunning(this)) {
                ControlService.stop(this)
                Toast.makeText(this, "Service stopped", Toast.LENGTH_SHORT).show()
            } else {
                if (android.os.Build.VERSION.SDK_INT >= 33) {
                    val perm = android.Manifest.permission.POST_NOTIFICATIONS
                    if (checkSelfPermission(perm) != android.content.pm.PackageManager.PERMISSION_GRANTED) {
                        requestPermissions(arrayOf(perm), 1001)
                        return@setOnClickListener
                    }
                }
                ControlService.start(this)
                Toast.makeText(this, "Service started", Toast.LENGTH_SHORT).show()
            }
            refresh()
        }

        btnCopy.setOnClickListener {
            val cm = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
            cm.setPrimaryClip(ClipData.newPlainText("test", "Hello from controler"))
            Toast.makeText(this, "Copied (desktop can sync via scrcpy)", Toast.LENGTH_SHORT).show()
        }
        btnPaste.setOnClickListener {
            val cm = getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
            val txt = cm.primaryClip?.getItemAt(0)?.text?.toString() ?: "(empty)"
            Toast.makeText(this, "Clipboard: $txt", Toast.LENGTH_LONG).show()
        }
    }
}
