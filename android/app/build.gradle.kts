plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    signingConfigs {
        create("release") {
            storeFile =
                file("D:\\Storage\\Windows 11 22H2\\projects\\source\\repos\\emuPlace\\emuDeeS\\android\\emudees.jks")
            storePassword = "591010"
            keyAlias = "release"
            keyPassword = "591010"
        }
    }
    namespace = "com.antique.emudees"
    compileSdkPreview = "UpsideDownCake"

    defaultConfig {
        applicationId = "com.antique.emudees"
        minSdk = 29
        targetSdk = 33
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        vectorDrawables {
            useSupportLibrary = true
        }

        externalNativeBuild {
            cmake {
                cppFlags += ""
            }
        }
        signingConfig = signingConfigs.getByName("release")
        setProperty("archivesBaseName", "emudees-android-arm64-v8a-${defaultConfig.versionName}.${defaultConfig.versionCode}")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("release")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    kotlinOptions {
        jvmTarget = "11"
    }
    buildFeatures {
        compose = true
    }
    composeOptions {
        kotlinCompilerExtensionVersion = "1.4.3"
    }
    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }

    externalNativeBuild {
        cmake {
            version = "3.25.1"
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }

    buildToolsVersion = "34.0.0 rc2"
    ndkVersion = "25.2.9519653"
}

dependencies {

    implementation("androidx.core:core-ktx:1.12.0-alpha01")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.6.1")
    implementation("androidx.activity:activity-compose:1.8.0-alpha02")
    implementation(platform("androidx.compose:compose-bom:2023.04.00"))
    implementation("androidx.compose.ui:ui:1.5.0-alpha02")
    implementation("androidx.compose.ui:ui-graphics:1.5.0-alpha02")
    implementation("androidx.compose.ui:ui-tooling-preview:1.5.0-alpha02")
    implementation("androidx.compose.material3:material3:1.1.0-beta02")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.preference:preference-ktx:1.2.0")

    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test.ext:junit:1.1.5")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    androidTestImplementation(platform("androidx.compose:compose-bom:2023.04.00"))
    androidTestImplementation("androidx.compose.ui:ui-test-junit4:1.5.0-alpha02")
    debugImplementation("androidx.compose.ui:ui-tooling:1.5.0-alpha02")
    debugImplementation("androidx.compose.ui:ui-test-manifest:1.5.0-alpha02")
}