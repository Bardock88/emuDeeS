package com.antique.emudees

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.annotation.RequiresApi
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LargeTopAppBar
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.antique.emudees.ui.theme.EmuDeeSTheme
import java.io.File
import java.util.SortedMap

class GameBrowser : ComponentActivity() {
    companion object {
        init {
            System.loadLibrary("emuDeeS")
        }
    }

    @RequiresApi(Build.VERSION_CODES.R)
    @OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (!Environment.isExternalStorageManager()) {
            val intent = Intent()
            intent.action = Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION
            intent.data = Uri.fromParts("package", packageName, null)

            startActivity(intent)
        } else {
            val path = Environment.getExternalStorageDirectory().absolutePath + "/Android/media"
            createDirectories(path = path)
            loadSettings(path = "$path/com.antique.emuDeeS")

            val systems = loadSystems(path = path)

            setContent {
                EmuDeeSTheme {
                    Surface(
                        modifier = Modifier.fillMaxSize(),
                        color = MaterialTheme.colorScheme.background
                    ) {
                        Scaffold(
                            topBar = {
                                LargeTopAppBar(
                                    title = {
                                        Text(
                                            text = "DeeS"
                                        )
                                    },
                                    actions = {
                                        IconButton(
                                            onClick = {
                                                startActivity(
                                                    Intent(
                                                        this@GameBrowser,
                                                        SettingsMenu::class.java
                                                    )
                                                )
                                            }
                                        ) {
                                            Icon(
                                                imageVector = Icons.Filled.Settings,
                                                contentDescription = null
                                            )
                                        }
                                    }
                                )
                            }
                        ) { paddingValues ->
                            LazyColumn(
                                contentPadding = paddingValues,
                                content = {
                                    systems.forEach { (system, roms) ->
                                        stickyHeader {
                                            Text(
                                                text = system,
                                                modifier = Modifier.padding(
                                                    start = 16.dp
                                                ),
                                                color = MaterialTheme.colorScheme.primary,
                                                fontWeight = FontWeight.Bold
                                            )
                                        }

                                        items(roms.size) { index ->
                                            ListItem(
                                                headlineContent = {
                                                    Text(
                                                        text = roms[index].name
                                                    )
                                                },
                                                modifier = Modifier.clickable {
                                                    when (system) {
                                                        "NDS" ->
                                                            setNDSPath(
                                                                path = roms[index].absolutePath
                                                            )

                                                        else ->
                                                            setGBPath(
                                                                path = roms[index].absolutePath
                                                            )
                                                    }

                                                    if (startCore() == 0) {
                                                        startActivity(
                                                            Intent(
                                                                this@GameBrowser,
                                                                GameActivity::class.java
                                                            )
                                                        )
                                                    }
                                                },
                                                leadingContent = {
                                                    Image(
                                                        bitmap = roms[index].icon,
                                                        contentDescription = null,
                                                        modifier = Modifier
                                                            .clip(
                                                                shape = RoundedCornerShape(
                                                                    size = 8.dp
                                                                )
                                                            )
                                                            .size(
                                                                size = 32.dp
                                                            )
                                                    )
                                                }
                                            )
                                        }
                                    }
                                }
                            )
                        }
                    }
                }
            }
        }
    }


    private fun createDirectories(path: String) {
        val root = File("$path/com.antique.emuDeeS")
        if (!root.exists())
            root.mkdir()

        val core = File("$path/com.antique.emuDeeS/core")
        if (!core.exists())
            core.mkdir()

        val roms = File("$path/com.antique.emuDeeS/roms")
        if (!roms.exists())
            roms.mkdir()
    }

    private fun loadSystems(path: String): SortedMap<String, List<Rom>> {
        val roms = ArrayList<Rom>()

        File("$path/com.antique.emuDeeS/roms").listFiles { file ->
            arrayOf(
                "gb",
                "gba",
                "nds"
            ).contains(file.extension)
        }?.forEach { file ->
            roms.add(element = Rom(file = file))
        }

        return roms.groupBy { it.system }.toSortedMap()
    }

    private external fun loadSettings(path: String)
    private external fun setGBPath(path: String)
    private external fun setNDSPath(path: String)
    private external fun startCore(): Int
}