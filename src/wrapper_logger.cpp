#include "wrapper_logger.h"
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <pthread.h>

#if __has_include(<android/log.h>)
#include <android/log.h>
#define LOG_TO_LOGCAT 1
#else
#define LOG_TO_LOGCAT 0
#endif

// Shared global mutex across all translation units
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

extern "C" void spam_log_blocking(const char *tag, const char *message) {
    // Acquire lock - if another event is spamming, this thread WAITS
    pthread_mutex_lock(&g_log_mutex);

    // 5 seconds total: 50 iterations * 100ms
    for (int i = 0; i < 50; i++) {
#if LOG_TO_LOGCAT
        __android_log_print(ANDROID_LOG_INFO, tag, "=== %s ===", message);
#endif
        std::cout << "[" << tag << "] === " << message << " ===" << std::endl;
        fflush(stdout);
        usleep(100000); // 100ms
    }

    pthread_mutex_unlock(&g_log_mutex);
}
