package com.antique.emudees

import android.annotation.SuppressLint
import android.content.Context
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup.LayoutParams
import android.widget.Button


@SuppressLint("ClickableViewAccessibility", "ViewConstructor", "AppCompatCustomView")
class GameButton(
    context: Context,
    resId: Int,
    btnId: Int,
    x: Float,
    y: Float,
    width: Float,
    height: Float
) : Button(context) {
    private val identifier: Int

    init {
        setBackgroundResource(resId)
        identifier = btnId
        setX(x)
        setY(y)
        layoutParams = LayoutParams(width.toInt(), height.toInt())
        alpha = 0.9f

        // Handle button touches
        if (identifier in 4..7) {
            setOnTouchListener { view: View, motionEvent: MotionEvent ->
                when (motionEvent.action) {
                    MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                        if (motionEvent.x > view.width * 2 / 3) pressKey(4) else releaseKey(4)
                        if (motionEvent.x < view.width * 1 / 3) pressKey(5) else releaseKey(5)
                        if (motionEvent.y < view.height * 1 / 3) pressKey(6) else releaseKey(6)
                        if (motionEvent.y > view.height * 2 / 3) pressKey(7) else releaseKey(7)
                    }

                    MotionEvent.ACTION_UP -> {
                        releaseKey(4)
                        releaseKey(5)
                        releaseKey(6)
                        releaseKey(7)
                    }
                }

                true
            }
        } else {
            setOnTouchListener { _: View, motionEvent: MotionEvent ->
                when (motionEvent.action) {
                    MotionEvent.ACTION_DOWN -> pressKey(identifier)
                    MotionEvent.ACTION_UP -> releaseKey(identifier)
                }

                true
            }
        }
    }

    private external fun pressKey(key: Int)
    private external fun releaseKey(key: Int)
}