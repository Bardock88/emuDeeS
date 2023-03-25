package com.antique.emudees

enum class Type {
    Switch
}

class SettingSection(title: String, settings: ArrayList<Setting>) {
    val title: String
    val settings: ArrayList<Setting>

    init {
        this.title = title
        this.settings = settings
    }
}

class Setting(name: String, description: String, identifier: String, type: Type) {
    val name: String
    val description: String
    val identifier: String

    val type: Type

    init {
        this.name = name
        this.description = description
        this.identifier = identifier

        this.type = type
    }
}