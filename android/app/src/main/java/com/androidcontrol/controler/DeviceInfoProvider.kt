package com.androidcontrol.controler

import android.content.Context
import android.os.BatteryManager
import android.os.Build
import android.util.DisplayMetrics
import android.view.WindowManager

data class DeviceInfo(
    val manufacturer: String = Build.MANUFACTURER,
    val model: String = Build.MODEL,
    val androidVersion: String = Build.VERSION.RELEASE ?: "unknown",
    val apiLevel: Int = Build.VERSION.SDK_INT,
    val serial: String = try { Build.getSerial() } catch (_: Exception) { @Suppress("DEPRECATION") Build.SERIAL ?: "unknown" },
    val resolution: String = "",
    val batteryLevel: Int = -1
) {
    val displayName: String get() {
        val base = "$manufacturer $model".trim()
        return if (base.isEmpty()) serial else base
    }
}

object DeviceInfoProvider {
    fun get(context: Context): DeviceInfo {
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val metrics = DisplayMetrics()
        @Suppress("DEPRECATION")
        wm.defaultDisplay.getMetrics(metrics)
        val res = "${metrics.widthPixels}x${metrics.heightPixels}"
        val bm = context.getSystemService(Context.BATTERY_SERVICE) as BatteryManager
        val level = try { bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY) } catch (_: Exception) { -1 }
        return DeviceInfo(resolution = res, batteryLevel = if (level in 0..100) level else -1)
    }
}
