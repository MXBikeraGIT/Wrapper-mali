#pragma once
#include <stdio.h>

#define ANDROID_LOG_INFO 3
#define ANDROID_LOG_WARN 4
#define ANDROID_LOG_ERROR 6

#define __android_log_print(prio, tag, fmt, ...) \
    printf("[%s] " fmt "\n", tag, ##__VA_ARGS__)
