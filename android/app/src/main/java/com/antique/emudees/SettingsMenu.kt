package com.antique.emudees

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LargeTopAppBar
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.preference.PreferenceManager
import com.antique.emudees.ui.theme.EmuDeeSTheme

class SettingsMenu : ComponentActivity() {
    @OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val preferences = PreferenceManager.getDefaultSharedPreferences(this) // preparation
        val editor = preferences.edit()

        setContent {
            val settings = arrayListOf<SettingSection>()
            settings.add(SettingSection(title = "General", settings = arrayListOf(
                Setting(
                    name = "Direct Boot",
                    description = "This option when enabled will boot roms automatically, bypassing the NDS home screen.",
                    identifier = "directBoot",
                    type = Type.Switch
                )
            )))

            settings.add(SettingSection(title = "Performance", settings = arrayListOf(
                Setting(
                    name = "Multithreaded 2D",
                    description = "This option when enabled will allow 2D emulation to use up to 2 threads.",
                    identifier = "threaded2D",
                    type = Type.Switch
                ),
                Setting(
                    name = "Multithreaded 3D",
                    description = "This option when enabled will allow 3D emulation to use up to 4 threads.",
                    identifier = "threaded3D",
                    type = Type.Switch
                )
            )))

            settings.add(SettingSection(title = "Rendering", settings = arrayListOf(
                Setting(
                    name = "Upscale 3D",
                    description = "This option when enabled will upscale textures within 3D emulation.",
                    identifier = "highRes3D",
                    type = Type.Switch
                )
            )))

            var remDirectBoot by remember { mutableStateOf(preferences.getBoolean("directBoot", true)) }
            var remMultithreaded2D by remember { mutableStateOf(preferences.getBoolean("threaded2D", false)) }
            var remMultithreaded3D by remember { mutableStateOf(preferences.getBoolean("threaded3D", false)) }
            var remHighRes3D by remember { mutableStateOf(preferences.getBoolean("highRes3D", false)) }

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
                                        text = "Settings"
                                    )
                                },
                                navigationIcon = {
                                    IconButton(
                                        onClick = {
                                            finish()
                                        }
                                    ) {
                                        Icon(
                                            imageVector = Icons.Filled.ArrowBack,
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
                                settings.forEach {
                                    stickyHeader {
                                        Text(
                                            text = it.title,
                                            modifier = Modifier.padding(
                                                start = 16.dp
                                            ),
                                            color = MaterialTheme.colorScheme.primary,
                                            fontWeight = FontWeight.Bold
                                        )
                                    }

                                    items(it.settings.size) { index ->
                                        ListItem(
                                            headlineContent = {
                                                Text(
                                                    text = it.settings[index].name
                                                )
                                            },
                                            supportingContent = {
                                                Text(
                                                    text = it.settings[index].description,
                                                    color = MaterialTheme.colorScheme.secondary
                                                )
                                            },
                                            trailingContent = {
                                                when (it.settings[index].type) {
                                                    Type.Switch -> {
                                                        when (it.settings[index].identifier) {
                                                            "directBoot" -> {
                                                                Switch(
                                                                    checked = remDirectBoot,
                                                                    onCheckedChange = { checked ->
                                                                        remDirectBoot = checked
                                                                        setDirectBoot(remDirectBoot)
                                                                        editor.putBoolean(
                                                                            it.settings[index].identifier,
                                                                            remDirectBoot
                                                                        )
                                                                        editor.apply()
                                                                    }
                                                                )
                                                            }

                                                            "threaded2D" -> {
                                                                Switch(
                                                                    checked = remMultithreaded2D,
                                                                    onCheckedChange = { checked ->
                                                                        remMultithreaded2D = checked
                                                                        setThreaded2D(remMultithreaded2D)
                                                                        editor.putBoolean(
                                                                            it.settings[index].identifier,
                                                                            remMultithreaded2D
                                                                        )
                                                                        editor.apply()
                                                                    }
                                                                )
                                                            }

                                                            "threaded3D" -> {
                                                                Switch(
                                                                    checked = remMultithreaded3D,
                                                                    onCheckedChange = { checked ->
                                                                        remMultithreaded3D = checked
                                                                        setThreaded3D(
                                                                            if (remMultithreaded3D) {
                                                                                4
                                                                            } else {
                                                                                2
                                                                            }
                                                                        )
                                                                        editor.putBoolean(
                                                                            it.settings[index].identifier,
                                                                            remMultithreaded3D
                                                                        )
                                                                        editor.apply()
                                                                    }
                                                                )
                                                            }

                                                            "highRes3D" -> {
                                                                Switch(
                                                                    checked = remHighRes3D,
                                                                    onCheckedChange = { checked ->
                                                                        remHighRes3D = checked
                                                                        setHighRes3D(remHighRes3D)
                                                                        editor.putBoolean(
                                                                            it.settings[index].identifier,
                                                                            remHighRes3D
                                                                        )
                                                                        editor.apply()
                                                                    }
                                                                )
                                                            }

                                                            else -> {}
                                                        }
                                                    }
                                                }
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

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                finish()
            }
        })
    }

    private external fun setDirectBoot(directBoot: Boolean)
    private external fun setHighRes3D(highRes3D: Boolean)
    private external fun setThreaded2D(threaded2D: Boolean)
    private external fun setThreaded3D(threaded3D: Int)
}