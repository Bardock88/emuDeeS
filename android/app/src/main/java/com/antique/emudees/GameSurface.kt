package com.antique.emudees

import android.content.Context
import android.opengl.GLSurfaceView

class GameSurface(context: Context) : GLSurfaceView(context) {
    private val gameRenderer: GameRenderer

    init {

        setEGLContextClientVersion(2)

        gameRenderer = GameRenderer(gameSurface = this)
        setRenderer(gameRenderer)
    }

    external fun isRunning(): Boolean
    external fun isGbaMode(): Boolean

    external fun pressScreen(x: Int, y: Int)
    external fun releaseScreen()
}