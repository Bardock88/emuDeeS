package com.antique.emudees

import android.graphics.Bitmap
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import java.io.File
import java.lang.Long.signum
import java.text.CharacterIterator
import java.text.StringCharacterIterator
import kotlin.io.path.fileSize
import kotlin.math.abs

class Rom(file: File) {
    val icon: ImageBitmap

    val absolutePath: String

    val name: String
    val size: String
    val system: String

    init {
        val bitmap = Bitmap.createBitmap(32, 32, Bitmap.Config.ARGB_8888)
        getNDSIcon(file.absolutePath, bitmap)
        this.icon = bitmap.asImageBitmap()

        this.absolutePath = file.absolutePath

        this.name = getRomName(file.absolutePath)
        this.size = humanReadableByteCountBin(file.toPath().fileSize())
        this.system = file.extension.uppercase()
    }

    private fun humanReadableByteCountBin(bytes: Long): String {
        val absB = if (bytes == Long.MIN_VALUE) Long.MAX_VALUE else abs(bytes)
        if (absB < 1024)
            return "$bytes B"

        var value = absB
        val ci: CharacterIterator = StringCharacterIterator("KMGTPE")
        var i = 40

        while (i >= 0 && absB > 0xfffccccccccccccL shr i) {
            value = value shr 10
            ci.next()
            i -= 10
        }

        value *= signum(bytes).toLong()
        return String.format("%.1f %ciB", value / 1024.0, ci.current())
    }

    private external fun getNDSIcon(path: String, bitmap: Bitmap)
    private external fun getRomName(path: String): String
}