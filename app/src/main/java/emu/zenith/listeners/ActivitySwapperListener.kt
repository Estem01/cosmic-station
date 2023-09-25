package emu.zenith.listeners

import android.content.Context
import android.content.Intent
import android.util.AttributeSet
import androidx.preference.Preference
import emu.zenith.prefers.BiosActivity

class ActivitySwapperListener @JvmOverloads
    constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr : Int = androidx.preference.R.attr.preferenceStyle) :
    Preference(context, attrs, defStyleAttr) {

    private fun launchActivity() {
        var swapTo: Intent? = null
        when (title) {
            "BIOS Model Configuration" -> {
                swapTo = Intent(context, BiosActivity::class.java)
            }
        }
        swapTo?.let {
            it.putExtra("Activity-Provider", "Zenith")
            context.startActivity(it)
        }
    }

    override fun onClick() = launchActivity()
}