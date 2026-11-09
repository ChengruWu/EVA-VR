#include "eva_bridge.h"

#include <android/log.h>
#include <mutex>
#include <vector>

namespace {
JavaVM* g_vm = nullptr;
jobject g_activity = nullptr;
jmethodID g_poll_haptic = nullptr;
jmethodID g_send_frame = nullptr;
jmethodID g_is_connected = nullptr;
std::mutex g_mutex;
std::mutex g_image_mutex;
std::vector<unsigned char> g_image;
int g_image_width = 0, g_image_height = 0;
}

extern "C" JNIEXPORT void JNICALL
Java_org_eva_pico_input_MainActivity_setSimulationPixels(JNIEnv* env, jobject, jintArray pixels, jint width, jint height) {
    if (!pixels || width <= 0 || height <= 0 || width > 1280 || height > 720 || env->GetArrayLength(pixels) != width * height) return;
    std::vector<jint> argb(static_cast<size_t>(width) * height);
    env->GetIntArrayRegion(pixels, 0, width * height, argb.data());
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    std::vector<unsigned char> rgba(argb.size() * 4);
    for (size_t i = 0; i < argb.size(); ++i) {
        const uint32_t p = static_cast<uint32_t>(argb[i]);
        rgba[4*i] = (p >> 16) & 255; rgba[4*i+1] = (p >> 8) & 255;
        rgba[4*i+2] = p & 255; rgba[4*i+3] = (p >> 24) & 255;
    }
    std::lock_guard<std::mutex> lock(g_image_mutex);
    g_image.swap(rgba); g_image_width = width; g_image_height = height;
}

bool eva_bridge_take_simulation_pixels(std::vector<unsigned char>& rgba, int& width, int& height) {
    std::lock_guard<std::mutex> lock(g_image_mutex);
    if (g_image.empty()) return false;
    rgba.swap(g_image); width = g_image_width; height = g_image_height;
    return true;
}

void eva_bridge_set_activity(JNIEnv* env, jobject activity) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_activity != nullptr) {
        env->DeleteGlobalRef(g_activity);
        g_activity = nullptr;
    }
    g_vm = nullptr;
    if (env == nullptr || activity == nullptr) return;
    env->GetJavaVM(&g_vm);
    g_activity = env->NewGlobalRef(activity);
    jclass cls = env->GetObjectClass(activity);
    g_poll_haptic = env->GetMethodID(cls, "pollHaptic", "()[D");
    g_send_frame = env->GetMethodID(cls, "sendFrame", "(Ljava/lang/String;)V");
    g_is_connected = env->GetMethodID(cls, "isConnected", "()Z");
    env->DeleteLocalRef(cls);
}

bool eva_bridge_poll_haptic(double command[4]) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_vm == nullptr || g_activity == nullptr || g_poll_haptic == nullptr) return false;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        attached = true;
    }
    auto* values = static_cast<jdoubleArray>(env->CallObjectMethod(g_activity, g_poll_haptic));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        values = nullptr;
    }
    bool ok = false;
    if (values != nullptr && env->GetArrayLength(values) >= 4) {
        env->GetDoubleArrayRegion(values, 0, 4, command);
        ok = true;
        env->DeleteLocalRef(values);
    }
    if (attached) g_vm->DetachCurrentThread();
    return ok;
}

void eva_bridge_send_frame(const std::string& json) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_vm == nullptr || g_activity == nullptr || g_send_frame == nullptr) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    jstring message = env->NewStringUTF(json.c_str());
    env->CallVoidMethod(g_activity, g_send_frame, message);
    env->DeleteLocalRef(message);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attached) g_vm->DetachCurrentThread();
}


bool eva_bridge_is_connected() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_vm == nullptr || g_activity == nullptr || g_is_connected == nullptr) return false;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        attached = true;
    }
    const jboolean connected = env->CallBooleanMethod(g_activity, g_is_connected);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attached) g_vm->DetachCurrentThread();
    return connected == JNI_TRUE;
}
