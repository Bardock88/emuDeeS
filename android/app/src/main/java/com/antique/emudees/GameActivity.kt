package com.antique.emudees

import android.annotation.SuppressLint
import android.os.Build
import android.os.Bundle
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback
import androidx.annotation.RequiresApi
import androidx.constraintlayout.widget.ConstraintLayout

class GameActivity : ComponentActivity() {
    private lateinit var gameSurface: GameSurface

    private var running: Boolean = false

    private lateinit var core: Thread

    private lateinit var layout: ConstraintLayout
    private var buttons = ArrayList<GameButton>()

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        gameSurface = GameSurface(this)

        gameSurface.focusable = View.FOCUSABLE
        gameSurface.isFocusable = true


        // DETECT SCREEN TOUCHES, FOR "TOUCH SCREEN" TYPE INTERACTIONS
        gameSurface.setOnTouchListener { _, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE ->
                    gameSurface.pressScreen(event.x.toInt(), event.y.toInt())

                MotionEvent.ACTION_UP ->
                    gameSurface.releaseScreen()
            }

            true
        }

        val density = resources.displayMetrics.density
        val width = resources.displayMetrics.widthPixels
        val height = resources.displayMetrics.heightPixels

        layout = ConstraintLayout(this)
        layout.addView(gameSurface)

        buttons.add(
            GameButton(
                this,
                R.drawable.a,
                0,
                width - (density * 65),
                height - (density * 135),
                (density * 55),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.b,
                1,
                width - (density * 105),
                height - (density * 95),
                (density * 55),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.x,
                10,
                width - (density * 105),
                height - (density * 175),
                (density * 55),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.y,
                11,
                width - (density * 145),
                height - (density * 135),
                (density * 55),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.plus,
                3,
                width / 2 + (density * 15),
                height - (density * 40),
                (density * 35),
                (density * 35)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.minus,
                2,
                width / 2 - (density * 50),
                height - (density * 40),
                (density * 35),
                (density * 35)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.trigger,
                9,
                (density * 10),
                height - (density * 250),
                (density * 60),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.trigger_flipped,
                8,
                width - (density * 70),
                height - (density * 250),
                (density * 60),
                (density * 55)
            )
        )
        buttons.add(
            GameButton(
                this,
                R.drawable.dpad,
                4,
                (density * 10),
                height - (density * 174),
                (density * 132),
                (density * 132)
            )
        )
        buttons.forEach { action ->
            layout.addView(action)
        }

        setContentView(layout)

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                pauseCore()
                finish()
            }
        })
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (event != null) {
            if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
                when (keyCode) {
                    KeyEvent.KEYCODE_BUTTON_A -> pressKey(0)
                    KeyEvent.KEYCODE_BUTTON_B -> pressKey(1)
                    KeyEvent.KEYCODE_BUTTON_X -> pressKey(10)
                    KeyEvent.KEYCODE_BUTTON_Y -> pressKey(11)

                    KeyEvent.KEYCODE_BUTTON_SELECT -> pressKey(2)
                    KeyEvent.KEYCODE_BUTTON_START -> pressKey(3)

                    KeyEvent.KEYCODE_BUTTON_L1 -> pressKey(9)
                    KeyEvent.KEYCODE_BUTTON_R1 -> pressKey(8)
                }

                return true
            } else {
                when (keyCode) {
                    KeyEvent.KEYCODE_DPAD_UP -> pressKey(6)
                    KeyEvent.KEYCODE_DPAD_DOWN -> pressKey(7)
                    KeyEvent.KEYCODE_DPAD_LEFT -> pressKey(5)
                    KeyEvent.KEYCODE_DPAD_RIGHT -> pressKey(4)
                }

                return true
            }
        }

        return super.onKeyDown(keyCode, event)
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {
        if (event != null) {
            if (event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD) {
                when (keyCode) {
                    KeyEvent.KEYCODE_BUTTON_A -> releaseKey(0)
                    KeyEvent.KEYCODE_BUTTON_B -> releaseKey(1)
                    KeyEvent.KEYCODE_BUTTON_X -> releaseKey(10)
                    KeyEvent.KEYCODE_BUTTON_Y -> releaseKey(11)

                    KeyEvent.KEYCODE_BUTTON_SELECT -> releaseKey(2)
                    KeyEvent.KEYCODE_BUTTON_START -> releaseKey(3)

                    KeyEvent.KEYCODE_BUTTON_L1 -> releaseKey(9)
                    KeyEvent.KEYCODE_BUTTON_R1 -> releaseKey(8)
                }

                return true
            } else {
                when (keyCode) {
                    KeyEvent.KEYCODE_DPAD_UP -> releaseKey(6)
                    KeyEvent.KEYCODE_DPAD_DOWN -> releaseKey(7)
                    KeyEvent.KEYCODE_DPAD_LEFT -> releaseKey(5)
                    KeyEvent.KEYCODE_DPAD_RIGHT -> releaseKey(4)
                }

                return true
            }
        }

        return super.onKeyUp(keyCode, event)
    }

    @RequiresApi(Build.VERSION_CODES.R)
    override fun onResume() {
        super.onResume()
        window.insetsController?.hide(-0x1)
        actionBar?.hide()

        resumeCore()
    }

    @RequiresApi(Build.VERSION_CODES.R)
    override fun onPause() {
        super.onPause()
        window.insetsController?.show(-0x1)
        actionBar?.show()

        pauseCore()
    }

    private fun pauseCore() {
        running = false
        stopAudio()

        // Wait for the emulator to stop
        try {
            core.join()
        } catch (e: Exception) {
            // Oh well, I guess
        }

        // Write the save file and pause rendering
        writeSave()
    }

    private fun resumeCore() {
        running = true
        startAudio()

        core = object : Thread() {
            override fun run() {
                while (running)
                    runFrame()
            }
        }

        core.priority = Thread.MAX_PRIORITY
        core.start()
    }

    private external fun restartCore()
    private external fun runFrame()
    private external fun startAudio()
    private external fun stopAudio()
    private external fun writeSave()
    private external fun pressKey(key: Int)
    private external fun releaseKey(key: Int)
}