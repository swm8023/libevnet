#include <evbase/util.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>

/* time */
__thread int64_t cached_time = 0;
int64_t get_cached_time() {
    return cached_time ? cached_time :
        (cached_time = update_cached_time());
}

int64_t update_cached_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return cached_time = seconds * MICOR_SECOND + tv.tv_usec;
}

void now_to_string(char* buf, int len) {
    assert(len >= 25);
    int64_t micro_seconds = get_cached_time();
    time_t seconds = micro_seconds / MICOR_SECOND;
    int microseconds = micro_seconds % MICOR_SECOND;
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);

    snprintf(buf, len, "%4d%02d%02d %02d:%02d:%02d.%06d",
      tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
      microseconds);
}

void time_to_string(int64_t micro_seconds, char* buf, int len) {
    assert(len >= 25);
    time_t seconds = micro_seconds / MICOR_SECOND;
    int microseconds = micro_seconds % MICOR_SECOND;
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);

    snprintf(buf, len, "%4d%02d%02d %02d:%02d:%02d.%06d",
      tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
      microseconds);
}