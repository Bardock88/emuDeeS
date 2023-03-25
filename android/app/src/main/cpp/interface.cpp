#include <android/bitmap.h>
#include <jni.h>
#include <string>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "core.h"
#include "settings.h"
#include "nds_icon.h"
#include "screen_layout.h"

int screenFilter = 1;
int showFpsCounter = 0;

std::string ndsPath, gbaPath;
Core *core = nullptr;
ScreenLayout layout;
uint32_t framebuffer[256 * 192 * 8];

SLEngineItf audioEngine;
SLObjectItf audioEngineObj;
SLObjectItf audioMixerObj;
SLObjectItf audioPlayerObj;
SLPlayItf audioPlayer;
SLAndroidSimpleBufferQueueItf audioBufferQueue;
int16_t audioBuffer[1024 * 2];


void audioCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    uint32_t *original = core->spu.getSamples(699);

    for (int i = 0; i < 1024; i++) {
        uint32_t sample = original[i * 699 / 1024];
        audioBuffer[i * 2 + 0] = sample >> 0;
        audioBuffer[i * 2 + 1] = sample >> 16;
    }

    (*audioBufferQueue)->Enqueue(audioBufferQueue, audioBuffer, sizeof(audioBuffer));
    delete[] original;
}

// TODO: GameBrowser.kt
extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_loadSettings(JNIEnv *env, jobject object, jstring string) {
    const char *str = env->GetStringUTFChars(string, nullptr);
    std::string path = str;
    env->ReleaseStringUTFChars(string, str);

    std::vector<Setting> platformSettings = {
            Setting("screenFilter", &screenFilter, false),
            Setting("showFpsCounter", &showFpsCounter, false)
    };

    ScreenLayout::addSettings();
    Settings::add(platformSettings);

    if (!Settings::load(path + "/emudees.ini")) {
        Settings::setBios7Path(path + "/core/bios7.bin");
        Settings::setBios9Path(path + "/core/bios9.bin");
        Settings::setFirmwarePath(path + "/core/firmware.bin");
        Settings::setGbaBiosPath(path + "/core/gba_bios.bin");
        Settings::setSdImagePath(path + "/core/sd.img");
        Settings::save();
    }
}

extern "C" JNIEXPORT jint JNICALL Java_com_antique_emudees_GameBrowser_startCore(JNIEnv *env, jobject object) {
    try {
        if (core == nullptr)
            core = new Core(ndsPath, gbaPath);
        else
            delete core;
        return 0;
    } catch (CoreError error) {
        return error;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setNDSPath(JNIEnv *env, jobject object, jstring string) {
    const char *str = env->GetStringUTFChars(string, nullptr);
    ndsPath = str;
    env->ReleaseStringUTFChars(string, str);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setGBPath(JNIEnv *env, jobject object, jstring value) {
    const char *str = env->GetStringUTFChars(value, nullptr);
    gbaPath = str;
    env->ReleaseStringUTFChars(value, str);
}

/*
extern "C" JNIEXPORT jstring JNICALL
Java_com_antique_emudees_GameBrowser_getNDSPath(JNIEnv *env, jclass clazz) {
    return env->NewStringUTF(ndsPath.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_antique_emudees_GameBrowser_getGbaPath(JNIEnv *env, jclass clazz) {
    return env->NewStringUTF(gbaPath.c_str());
}
 */

// TODO: GameActivity.kt
extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_startAudio(JNIEnv *env, jobject object) {
    slCreateEngine(&audioEngineObj, 0, nullptr, 0, nullptr, nullptr);
    (*audioEngineObj)->Realize(audioEngineObj, SL_BOOLEAN_FALSE);
    (*audioEngineObj)->GetInterface(audioEngineObj, SL_IID_ENGINE, &audioEngine);
    (*audioEngine)->CreateOutputMix(audioEngine, &audioMixerObj, 0, nullptr, nullptr);
    (*audioMixerObj)->Realize(audioMixerObj, SL_BOOLEAN_FALSE);

    // Define the audio source and format
    SLDataLocator_AndroidSimpleBufferQueue audioBufferLoc = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM audioFormat =
            {
                    SL_DATAFORMAT_PCM,
                    2,
                    SL_SAMPLINGRATE_48,
                    SL_PCMSAMPLEFORMAT_FIXED_16,
                    SL_PCMSAMPLEFORMAT_FIXED_16,
                    SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                    SL_BYTEORDER_LITTLEENDIAN
            };
    SLDataSource audioSource = {&audioBufferLoc, &audioFormat};

    // Initialize the audio player
    SLDataLocator_OutputMix audioMixer = {SL_DATALOCATOR_OUTPUTMIX, audioMixerObj};
    SLDataSink audioSink = {&audioMixer, nullptr};
    SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*audioEngine)->CreateAudioPlayer(audioEngine, &audioPlayerObj, &audioSource, &audioSink, 2,
                                      ids, req);

    // Set up the audio buffer queue and callback
    (*audioPlayerObj)->Realize(audioPlayerObj, SL_BOOLEAN_FALSE);
    (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_PLAY, &audioPlayer);
    (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_BUFFERQUEUE, &audioBufferQueue);
    (*audioBufferQueue)->RegisterCallback(audioBufferQueue, audioCallback, nullptr);
    (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PLAYING);

    // Initiate playback with an empty buffer
    memset(audioBuffer, 0, sizeof(audioBuffer));
    (*audioBufferQueue)->Enqueue(audioBufferQueue, audioBuffer, sizeof(audioBuffer));
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_stopAudio(JNIEnv *env, jobject object) {
    (*audioPlayerObj)->Destroy(audioPlayerObj);
    (*audioMixerObj)->Destroy(audioMixerObj);
    (*audioEngineObj)->Destroy(audioEngineObj);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_pressKey(JNIEnv *env, jobject object, jint int1) {
    core->input.pressKey(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_releaseKey(JNIEnv *env, jobject object, jint int1) {
    core->input.releaseKey(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_runFrame(JNIEnv *env, jobject object) {
    core->runFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_writeSave(JNIEnv *env, jobject object) {
    core->isGbaMode() ? core->cartridgeGba.writeSave() : core->cartridgeNds.writeSave();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_restartCore(JNIEnv *env, jobject jobject1) {
    if (core == nullptr)
        core = new Core(ndsPath, gbaPath);
    else
        delete core;
}

// TODO: GameButton.kt
extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameButton_pressKey(JNIEnv *env, jobject object, jint int1) {
    core->input.pressKey(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameButton_releaseKey(JNIEnv *env, jobject object, jint int1) {
    core->input.releaseKey(int1);
}

// TODO: GameRenderer.kt
extern "C" JNIEXPORT jboolean JNICALL
Java_com_antique_emudees_GameRenderer_copyFramebuffer(JNIEnv *env, jobject object1, jobject object2, jboolean boolean) {
    if (!core->gpu.getFrame(framebuffer, boolean))
        return false;

    // Copy the frame to the bitmap
    uint32_t *data;
    AndroidBitmap_lockPixels(env, object2, (void **) &data);
    size_t count = (boolean ? (240 * 160) : (256 * 192 * 2)) << (Settings::getHighRes3D() * 2);
    memcpy(data, framebuffer, count * sizeof(uint32_t));
    AndroidBitmap_unlockPixels(env, object2);
    return true;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getHighRes3D(JNIEnv *env, jobject object) {
    return Settings::getHighRes3D();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getScreenRotation(JNIEnv *env, jobject object) {
    return ScreenLayout::getScreenRotation();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getGbaCrop(JNIEnv *env, jobject object) {
    return ScreenLayout::getGbaCrop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameRenderer_updateLayout(JNIEnv *env, jobject object, jint int1, jint int2) {
    layout.update(int1, int2, core->isGbaMode());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getTopX(JNIEnv *env, jobject object) {
    return layout.getTopX();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getBotX(JNIEnv *env, jobject object) {
    return layout.getBotX();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getTopY(JNIEnv *env, jobject object) {
    return layout.getTopY();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getBotY(JNIEnv *env, jobject object) {
    return layout.getBotY();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getTopWidth(JNIEnv *env, jobject object) {
    return layout.getTopWidth();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getBotWidth(JNIEnv *env, jobject object) {
    return layout.getBotWidth();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getTopHeight(JNIEnv *env, jobject object) {
    return layout.getTopHeight();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_GameRenderer_getBotHeight(JNIEnv *env, jobject object) {
    return layout.getBotHeight();
}

// TODO: GameSurface.kt
extern "C" JNIEXPORT jboolean JNICALL
Java_com_antique_emudees_GameSurface_isGbaMode(JNIEnv *env, jobject object) {
    return core->isGbaMode();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_antique_emudees_GameSurface_isRunning(JNIEnv *env, jobject object) {
    return core->isRunning();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameSurface_pressScreen(JNIEnv *env, jobject object, jint int1, jint int2) {
    core->input.pressScreen();
    core->spi.setTouch(layout.getTouchX(int1, int2), layout.getTouchY(int1, int2));
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameSurface_releaseScreen(JNIEnv *env, jobject object) {
    core->input.releaseScreen();
    core->spi.clearTouch();
}

// TODO: Rom.kt
extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_Rom_getNDSIcon(JNIEnv *env, jobject jobject1, jstring jstring1,
                                        jobject jobject2) {
    const char *str = env->GetStringUTFChars(jstring1, nullptr);
    nds_icon nds_icon(str);
    env->ReleaseStringUTFChars(jstring1, str);

    uint32_t *data;
    AndroidBitmap_lockPixels(env, jobject2, (void **) &data);
    memcpy(data, nds_icon.icon(), 32 * 32 * sizeof(uint32_t));
    AndroidBitmap_unlockPixels(env, jobject2);
}

extern "C" JNIEXPORT jstring JNICALL Java_com_antique_emudees_Rom_getRomName(JNIEnv* env, jobject jobject1, jstring jstring1) {
    return env->NewStringUTF("");
}

// TODO: SettingsMenu.kt
extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setDirectBoot(JNIEnv* env, jobject object, jboolean boolean) {
    Settings::setDirectBoot(boolean);
}

extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setThreaded2D(JNIEnv *env, jobject object, jboolean boolean) {
    Settings::setThreaded2D(boolean);
}

extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setThreaded3D(JNIEnv *env, jobject object, jint int1) {
    Settings::setThreaded3D(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setHighRes3D(JNIEnv *env, jobject object, jint int1) {
    Settings::setHighRes3D(int1);
}


/*

 extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_runFrame(JNIEnv *env, jobject object) {
    core->runFrame();
}

 * extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_writeSave(JNIEnv *env, jobject object) {
    if (core->isGbaMode())
        core->cartridgeGba.writeSave();
    else
        core->cartridgeNds.writeSave();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_restartCore(JNIEnv *env, jclass clazz) {
    if (core)
        delete core;
    core = new Core(ndsPath, gbaPath);
}


extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setThreaded3D(JNIEnv *env, jobject object, jint int1) {
    Settings::setThreaded3D(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setHighRes3D(JNIEnv *env, jobject object, jint int1) {
    Settings::setHighRes3D(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setScreenRotation(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setScreenRotation(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_setScreenArrangement(JNIEnv *env, jclass obj, jint value) {
    ScreenLayout::setScreenArrangement(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameBrowser_saveSettings(JNIEnv *env, jclass clazz) {
    Settings::save();
}


// TODO: SETTINGS
extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setDirectBoot(JNIEnv* env, jobject object, jboolean boolean) {
    Settings::setDirectBoot(boolean);
}

extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setThreaded2D(JNIEnv *env, jobject object, jboolean boolean) {
    Settings::setThreaded2D(boolean);
}

extern "C" JNIEXPORT void JNICALL Java_com_antique_emudees_SettingsMenu_setThreaded3D(JNIEnv *env, jobject object, jint int1) {
    Settings::setThreaded3D(int1);
}



extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_DeeSRenderer_getScreenFilter(JNIEnv *env, jclass clazz) {
    return screenFilter;
}




// The below functions are pretty much direct forwarders to core functions

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getDirectBoot(JNIEnv *env, jclass clazz) {
    return Settings::getDirectBoot();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getFpsLimiter(JNIEnv *env, jclass clazz) {
    return Settings::getFpsLimiter();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getThreaded2D(JNIEnv *env, jclass clazz) {
    return Settings::getThreaded2D();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getThreaded3D(JNIEnv *env, jclass clazz) {
    return Settings::getThreaded3D();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getHighRes3D(JNIEnv *env, jclass clazz) {
    return Settings::getHighRes3D();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getScreenRotation(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getScreenRotation();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getScreenArrangement(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getScreenArrangement();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getScreenSizing(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getScreenSizing();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getScreenGap(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getScreenGap();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getIntegerScale(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getIntegerScale();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getGbaCrop(JNIEnv *env, jclass clazz) {
    return ScreenLayout::getGbaCrop();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getScreenFilter(JNIEnv *env, jclass clazz) {
    return screenFilter;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_SettingsMenu_getShowFpsCounter(JNIEnv *env, jclass clazz) {
    return showFpsCounter;
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setFpsLimiter(JNIEnv *env, jclass clazz, jint value) {
    Settings::setFpsLimiter(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setHighRes3D(JNIEnv *env, jclass clazz, jint value) {
    Settings::setHighRes3D(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setScreenRotation(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setScreenRotation(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setScreenArrangement(JNIEnv *env, jclass obj, jint value) {
    ScreenLayout::setScreenArrangement(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setScreenSizing(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setScreenSizing(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setScreenGap(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setScreenGap(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setIntegerScale(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setIntegerScale(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setGbaCrop(JNIEnv *env, jclass clazz, jint value) {
    ScreenLayout::setGbaCrop(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setScreenFilter(JNIEnv *env, jclass clazz, jint value) {
    screenFilter = value;
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_setShowFpsCounter(JNIEnv *env, jclass clazz, jint value) {
    showFpsCounter = value;
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_SettingsMenu_saveSettings(JNIEnv *env, jclass clazz) {
    Settings::save();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_DeeSActivity_getShowFpsCounter(JNIEnv *env, jclass clazz) {
    return showFpsCounter;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_antique_emudees_DeeSActivity_getFps(JNIEnv *env, jclass clazz) {
    return core->getFps();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_antique_emudees_GameSurface_isGbaMode(JNIEnv *env, jobject object) {
    return core->isGbaMode();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_runFrame(JNIEnv *env, jobject object) {
    core->runFrame();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_antique_emudees_GameSurface_isRunning(JNIEnv *env, jobject object) {
    return core->isRunning();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_writeSave(JNIEnv *env, jobject object) {
    if (core->isGbaMode())
        core->cartridgeGba.writeSave();
    else
        core->cartridgeNds.writeSave();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameActivity_restartCore(JNIEnv *env, jclass clazz) {
    if (core)
        delete core;
    core = new Core(ndsPath, gbaPath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameSurface_pressScreen(JNIEnv *env, jobject object, jint int1, jint int2) {
    core->input.pressScreen();
    core->spi.setTouch(layout.getTouchX(int1, int2), layout.getTouchY(int1, int2));
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameSurface_releaseScreen(JNIEnv *env, jobject object) {
    core->input.releaseScreen();
    core->spi.clearTouch();
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_DeeSActivity_resizeGbaSave(JNIEnv *env, jclass clazz, jint size) {
    core->cartridgeGba.resizeSave(size);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_DeeSActivity_resizeNdsSave(JNIEnv *env, jclass clazz, jint size) {
    core->cartridgeNds.resizeSave(size);
}


extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameButton_pressKey(JNIEnv *env, jobject object, jint int1) {
    core->input.pressKey(int1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_antique_emudees_GameButton_releaseKey(JNIEnv *env, jobject object, jint int1) {
    core->input.releaseKey(int1);
}
*/