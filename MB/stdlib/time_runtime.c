#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdint.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int64_t time_now_ms(void) {
    return (int64_t)GetTickCount64();
}

void time_sleep_ms(int64_t milliseconds) {
    if (milliseconds <= 0) return;
    while (milliseconds > (int64_t)UINT32_MAX) {
        Sleep(UINT32_MAX);
        milliseconds -= (int64_t)UINT32_MAX;
    }
    Sleep((DWORD)milliseconds);
}
#else
#include <errno.h>
#include <time.h>

int64_t time_now_ms(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

void time_sleep_ms(int64_t milliseconds) {
    if (milliseconds <= 0) return;
    struct timespec remaining = {
        milliseconds / 1000,
        (milliseconds % 1000) * 1000000
    };
    while (nanosleep(&remaining, &remaining) != 0 && errno == EINTR) {}
}
#endif

int64_t time_elapsed_ms(int64_t start_ms) {
    return time_now_ms() - start_ms;
}
