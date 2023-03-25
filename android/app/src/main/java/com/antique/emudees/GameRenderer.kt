package com.antique.emudees

import android.graphics.Bitmap
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.opengl.GLUtils
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


class GameRenderer(private val gameSurface: GameSurface) : GLSurfaceView.Renderer {
    private var program: Int = 0
    private lateinit var textures: IntArray
    private lateinit var bitmap: Bitmap
    private var width: Int = 0
    private var height: Int = 0

    private var highRes3D: Int = 0
    private var gbaMode: Boolean = false

    private val fragmentShader =
        "precision mediump float;" + "uniform sampler2D uTexture;" + "varying vec2 vTexCoord;" +
                "void main()" + "{" +
                "    gl_FragColor = texture2D(uTexture, vTexCoord);" +
                "}"

    private val vertexShader =
        "precision mediump float;" + "uniform mat4 uProjection;" + "attribute vec2 aPosition;" + "attribute vec2 aTexCoord;" + "varying vec2 vTexCoord;" +
                "void main()" + "{" +
                "    gl_Position = uProjection * vec4(aPosition.xy, 0.0, 1.0);" +
                "    vTexCoord = aTexCoord;" +
                "}"


    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        // Compile the vertex shader
        // Compile the vertex shader
        val vertShader = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER)
        GLES20.glShaderSource(vertShader, vertexShader)
        GLES20.glCompileShader(vertShader)

        // Compile the fragment shader

        // Compile the fragment shader
        val fragShader = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER)
        GLES20.glShaderSource(fragShader, fragmentShader)
        GLES20.glCompileShader(fragShader)

        // Attach the shaders to the program

        // Attach the shaders to the program
        program = GLES20.glCreateProgram()
        GLES20.glAttachShader(program, vertShader)
        GLES20.glAttachShader(program, fragShader)
        GLES20.glLinkProgram(program)
        GLES20.glUseProgram(program)

        GLES20.glDeleteShader(vertShader)
        GLES20.glDeleteShader(fragShader)

        // Set up texturing

        // Set up texturing
        textures = IntArray(1)
        GLES20.glGenTextures(1, textures, 0)
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0])
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_S,
            GLES20.GL_CLAMP_TO_EDGE
        )
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_WRAP_T,
            GLES20.GL_CLAMP_TO_EDGE
        )
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MIN_FILTER,
            if (1 == 1) GLES20.GL_LINEAR else GLES20.GL_NEAREST
        )
        GLES20.glTexParameteri(
            GLES20.GL_TEXTURE_2D,
            GLES20.GL_TEXTURE_MAG_FILTER,
            if (1 == 1) GLES20.GL_LINEAR else GLES20.GL_NEAREST
        )

        bitmap = Bitmap.createBitmap(256, 192 * 2, Bitmap.Config.ARGB_8888)
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        val projection = floatArrayOf(
            2.0f / width, 0.0f, 0.0f, 0.0f,
            0.0f, -2.0f / height, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 1.0f
        )

        // Pass the data to the shaders

        // Pass the data to the shaders
        val data: FloatBuffer =
            ByteBuffer.allocateDirect(projection.size * 4).order(ByteOrder.nativeOrder())
                .asFloatBuffer()
        data.put(projection)
        data.position(0)
        GLES20.glUniformMatrix4fv(
            GLES20.glGetUniformLocation(program, "uProjection"),
            1,
            false,
            data
        )

        // Update the layout

        // Update the layout
        GLES20.glViewport(0, 0, width, height)
        updateLayout(width, height)
        this.width = width
        this.height = height
    }

    override fun onDrawFrame(gl: GL10?) {
        // Update the resolution if the high-res 3D setting changed
        // Update the resolution if the high-res 3D setting changed
        if (highRes3D != getHighRes3D()) {
            highRes3D = getHighRes3D()
            bitmap = Bitmap.createBitmap(
                (if (gbaMode) 240 else 256) shl highRes3D,
                (if (gbaMode) 160 else 192 * 2) shl highRes3D, Bitmap.Config.ARGB_8888
            )
        }

        // Update the layout if GBA mode changed

        // Update the layout if GBA mode changed
        if (gbaMode != (gameSurface.isGbaMode() && getGbaCrop() != 0)) {
            gbaMode = !gbaMode
            updateLayout(width, height)
            bitmap = Bitmap.createBitmap(
                (if (gbaMode) 240 else 256) shl highRes3D,
                (if (gbaMode) 160 else 192 * 2) shl highRes3D, Bitmap.Config.ARGB_8888
            )
        }

        // Clear the display

        // Clear the display
        GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f)
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT)

        // Wait until a new frame is ready to prevent stuttering
        // This sucks, but buffers are automatically swapped, so returning would cause even worse stutter

        // Wait until a new frame is ready to prevent stuttering
        // This sucks, but buffers are automatically swapped, so returning would cause even worse stutter
        while (!copyFramebuffer(bitmap, gbaMode) && gameSurface.isRunning());
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0)

        if (gbaMode) {
            // Draw the GBA screen
            drawScreen(
                getTopX().toFloat(),
                getTopY().toFloat(),
                getTopWidth().toFloat(),
                getTopHeight().toFloat(),
                0.0f,
                0.0f,
                1.0f,
                1.0f
            )
        } else {
            // Draw the DS top and bottom screens
            drawScreen(
                getTopX().toFloat(),
                getTopY().toFloat(),
                getTopWidth().toFloat(),
                getTopHeight().toFloat(),
                0.0f,
                0.0f,
                1.0f,
                0.5f
            )
            drawScreen(
                getBotX().toFloat(),
                getBotY().toFloat(),
                getBotWidth().toFloat(),
                getBotHeight().toFloat(),
                0.0f,
                0.5f,
                1.0f,
                1.0f
            )
        }
    }

    private fun drawScreen(
        x: Float,
        y: Float,
        w: Float,
        h: Float,
        s1: Float,
        t1: Float,
        s2: Float,
        t2: Float
    ) {
        val rot: Int = getScreenRotation()

        // Arrange the S coordinates for rotation
        val s = arrayOf(
            floatArrayOf(s1, s1, s2, s2),
            floatArrayOf(s1, s2, s1, s2),
            floatArrayOf(s2, s1, s2, s1)
        )

        // Arrange the T coordinates for rotation
        val t = arrayOf(
            floatArrayOf(t1, t2, t1, t2),
            floatArrayOf(t2, t2, t1, t1),
            floatArrayOf(t1, t1, t2, t2)
        )

        // Define the vertices
        val vertices = floatArrayOf(
            x, y, s[rot][0], t[rot][0],
            x, y + h, s[rot][1], t[rot][1],
            x + w, y, s[rot][2], t[rot][2],
            x + w, y + h, s[rot][3], t[rot][3]
        )
        val data = ByteBuffer.allocateDirect(vertices.size * 4).order(ByteOrder.nativeOrder())
            .asFloatBuffer()
        data.put(vertices)

        // Pass the position data to the shaders
        data.position(0)
        val position = GLES20.glGetAttribLocation(program, "aPosition")
        GLES20.glVertexAttribPointer(position, 2, GLES20.GL_FLOAT, false, 4 * 4, data)
        GLES20.glEnableVertexAttribArray(position)

        // Pass the texture coordinate data to the shaders
        data.position(2)
        val texCoord = GLES20.glGetAttribLocation(program, "aTexCoord")
        GLES20.glVertexAttribPointer(texCoord, 2, GLES20.GL_FLOAT, false, 4 * 4, data)
        GLES20.glEnableVertexAttribArray(texCoord)
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4)
    }

    private external fun copyFramebuffer(bitmap: Bitmap, gbaMode: Boolean): Boolean
    private external fun getGbaCrop(): Int
    private external fun getHighRes3D(): Int
    private external fun getScreenRotation(): Int
    private external fun updateLayout(width: Int, height: Int)

    private external fun getBotHeight(): Int
    private external fun getBotWidth(): Int
    private external fun getBotX(): Int
    private external fun getBotY(): Int
    private external fun getTopHeight(): Int
    private external fun getTopWidth(): Int
    private external fun getTopX(): Int
    private external fun getTopY(): Int
}