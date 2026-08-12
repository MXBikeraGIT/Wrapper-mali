#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Spams the given message for 5 seconds. Blocked by a global mutex so calls wait for each other.
void spam_log_blocking(const char *tag, const char *message);

#ifdef __cplusplus
}
#endif
