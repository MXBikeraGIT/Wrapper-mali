#ifndef MAIN_HOOK_HPP
#define MAIN_HOOK_HPP

#include <dlfcn.h>
#include <android/dlext.h>
#include <android/log.h>
#include <string>

#define LOG_TAG "MainHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define TARGET_WRAPPER_SO "libvulkan_wrapper.so"

#ifdef __cplusplus
extern "C" {
#endif

void* hook_android_dlopen_ext(const char* filename, int flags, const android_dlextinfo* extinfo, void* caller_addr);
void* hook_dlopen(const char* filename, int flags);

__attribute__((visibility("default"))) void init_main_hook();

#ifdef __cplusplus
}
#endif

#endif
