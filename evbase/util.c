#include <evbase/util.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

/* socket operation */
int fd_reuse(int fd) {
    const int on = 1;
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(int));
#elif defined SO_REUSEADDR
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
#endif
    return 0;
}

int fd_cloexec(int fd) {
    int flags = fcntl(fd, F_GETFD, 0); 
    fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    return 0;
}

int fd_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

int ignore_sigpipe() {
    struct sigaction act;
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == 0){
        log_trace("ignore SIGPIPRI error()");
    }
}

/* time opertion */
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