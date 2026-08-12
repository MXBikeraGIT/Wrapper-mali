#include "main_hook.hpp"
#include <cstring>

typedef void* (*android_dlopen_ext_fn)(const char*, int, const android_dlextinfo*, void*);
typedef void* (*dlopen_fn)(const char*, int);

static android_dlopen_ext_fn real_android_dlopen_ext = nullptr;
static dlopen_fn real_dlopen = nullptr;

static bool should_redirect_driver(const char* filename) {
    if (!filename) return false;
    if (strstr(filename, "libvulkan.so") != nullptr) return true;
    if (strstr(filename, "vulkan.") != nullptr) return true;
    return false;
}

extern "C" {

void* hook_dlopen(const char* filename, int flags) {
    if (should_redirect_driver(filename)) {
        LOGI("Redirecting dlopen request: %s -> %s", filename, TARGET_WRAPPER_SO);
        return real_dlopen ? real_dlopen(TARGET_WRAPPER_SO, flags) : dlopen(TARGET_WRAPPER_SO, flags);
    }
    return real_dlopen ? real_dlopen(filename, flags) : dlopen(filename, flags);
}

void* hook_android_dlopen_ext(const char* filename, int flags, const android_dlextinfo* extinfo, void* caller_addr) {
    if (should_redirect_driver(filename)) {
        LOGI("Redirecting android_dlopen_ext request: %s -> %s", filename, TARGET_WRAPPER_SO);
        return real_android_dlopen_ext ? real_android_dlopen_ext(TARGET_WRAPPER_SO, flags, extinfo, caller_addr) : nullptr;
    }
    return real_android_dlopen_ext ? real_android_dlopen_ext(filename, flags, extinfo, caller_addr) : nullptr;
}

__attribute__((constructor))
void init_main_hook() {
    LOGI("Initializing libmain_hook loader interposer...");
    real_dlopen = (dlopen_fn)dlsym(RTLD_NEXT, "dlopen");
    real_android_dlopen_ext = (android_dlopen_ext_fn)dlsym(RTLD_NEXT, "android_dlopen_ext");
}

}
