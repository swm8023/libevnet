#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <evbase/log.h>

static void log_console_output_cb(const char *, size_t);
static void log_console_flush_cb();
static void log_console_fatal_cb();

static const char* log_level_name[LOG_LEVELS] = {
    "LOG_INNER",
    "LOG_TRACE",
    "LOG_DEBUG",
    "LOG_WARN ",
    "LOG_ERROR",
    "LOG_FATAL"
};

static struct log_if default_log_if_s = {
    (uint8_t)(LOG_MASK),
    log_console_output_cb,
    log_console_flush_cb,
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        log_console_fatal_cb
    }
};
struct log_if *default_log_if = &default_log_if_s;


void log_append(struct log_if *logif, uint8_t level_index, const char *fmt, ...) {
    char buf[LOG_BUFSIZE + 1], timebuf[30];
    size_t buflen = 0;

    now_to_string(timebuf, sizeof timebuf);
    buflen = snprintf(buf, LOG_BUFSIZE, "%s%5d %s ", log_level_name[level_index],
        thread_id(), timebuf);

    va_list ap;
    va_start(ap, fmt);
    buflen += vsnprintf(buf + buflen, LOG_BUFSIZE - buflen, fmt, ap);
    va_end(ap);

    buf[buflen] = '\n';
    buf[++buflen] = '\0';

    if (logif->output_cb) {
        logif->output_cb(buf, buflen);
    }

    if (logif->level_cb[level_index]) {
        logif->level_cb[level_index](buf, buflen);
    }

}

static void log_console_output_cb(const char *str, size_t size) {
    fwrite(str, size, 1, stdout);
}
static void log_console_flush_cb() {
    fflush(stdout);
}
static void log_console_fatal_cb() {
    printf("!!!fatal error, program exit!!!\n");
    log_console_flush_cb();
    _exit(-1);
}