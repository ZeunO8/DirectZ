#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <thread>
#include <DirectZ.hpp>

ANativeWindow* android_window = nullptr;
EventInterface* event_interface = 0;

std::shared_ptr<std::thread> update_thread;
bool updating = false;

void glue_update()
{
    updating = true;
    while (updating)
    {
        poll_events();
        update();
        render();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_dev_zeucor_mdirecz_MainActivity_nativeInit(JNIEnv* env, jobject, jobject surface, jobject assetManager)
{
    bool recreate = false;
    if (android_window)
    {
        ANativeWindow_release(android_window);
        android_window = nullptr;
        recreate = true;
    }
    android_window = ANativeWindow_fromSurface(env, surface);
    AAssetManager* android_asset_manager = 0;
    if (assetManager != nullptr)
    {
        android_asset_manager = AAssetManager_fromJava(env, assetManager);
        if (!android_asset_manager)
        {
            LOGE("Failed to get AAssetManager");
            return;
        }
    }
    if (!android_window)
    {
        LOGE("Failed to obtain ANativeWindow");
        return;
    }
    auto width = float(ANativeWindow_getWidth(android_window));
    auto height = float(ANativeWindow_getHeight(android_window));
    if (recreate)
    {
        assert(event_interface);
        event_interface->recreate_window(android_window, width, height);
    }
    else
    {
        LOGI("Successfully obtained ANativeWindow");
        // call app-lib implementation of init
        if (!(event_interface = init({
            .title = "Example Window",
            .x = 0,
            .y = 0,
            .width = width,
            .height = height,
            .android_window = android_window,
            .android_asset_manager = android_asset_manager
        })))
        {
            LOGE("Failed to call DirectZ init");
            return;
        }
    }
    update_thread = std::make_shared<std::thread>(glue_update);
}

extern "C" JNIEXPORT void JNICALL
Java_dev_zeucor_mdirecz_MainActivity_nativeOnTouch(JNIEnv* env, jclass,
                                                   jint action, jint pointerIndex, jint pointerId,
                                                   jfloat x, jfloat y, jfloat pressure, jfloat size)
{
    event_interface->touch_event(action, pointerIndex, pointerId, x, y, pressure, size);
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_dev_zeucor_mdirecz_MainActivity_nativeDestroy(JNIEnv* env, jobject)
{
    updating = false;
    if (update_thread->joinable())
        update_thread->join();
    event_interface->destroy_surface();
}