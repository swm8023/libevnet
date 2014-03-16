#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>

#include <evbase/util.h>
#include <evbase/thread.h>


/* current thread */
__thread int cr_thread_id = 0;
__thread const char* cr_thread_name = "unknow";

int thread_id() {
    if (cr_thread_id == 0) {
        cr_thread_id = syscall(SYS_gettid);
    }
    return cr_thread_id;
}

const char* thread_name() {
    return cr_thread_name;
}


static int pthread_mutex_lock_lock(void *p, int mode);
static int pthread_mutex_lock_unlock(void *p, int mode);
static void pthread_mutex_lock_mm_free(void *p, int mode);
static void *pthread_mutex_lock_alloc(int mode);

static void *pthread_cond_lock_alloc(int);
static void pthread_cond_lock_mm_free(void *, int);
static int pthread_cond_lock_signal(void *cond, int);
static int pthread_cond_lock_wait(void *cond, void *lock, int64_t, int);


/* mutex lock */
struct lock_ops scnet_lock_ops = {
    "pthread_mutex_lock",
    0,
    pthread_mutex_lock_alloc,
    pthread_mutex_lock_mm_free,
    pthread_mutex_lock_lock,
    pthread_mutex_lock_unlock,
};

void set_lock_ops(struct lock_ops ops) {
    scnet_lock_ops = ops;
}

static void *pthread_mutex_lock_alloc(int mode) {
    pthread_mutex_t *lock = (pthread_mutex_t*)mm_malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t *attr = NULL;
    if (pthread_mutex_init(lock, attr)) {
        log_error("pthread_mutex_init error");
        mm_free(lock);
        return NULL;
    }
    return lock;
}

static void pthread_mutex_lock_mm_free(void *p, int mode) {
    pthread_mutex_t *lock = (pthread_mutex_t*)p;
    pthread_mutex_destroy(lock);
    mm_free(lock);
}

static int pthread_mutex_lock_lock(void *p, int mode) {
    pthread_mutex_t *lock = (pthread_mutex_t*)p;
    return pthread_mutex_lock(lock);
}

static int pthread_mutex_lock_unlock(void *p, int mode) {
    pthread_mutex_t *lock = (pthread_mutex_t*)p;
    return pthread_mutex_unlock(lock);
}

/* condition */
struct cond_ops scnet_cond_ops = {
    "pthread_cond",
    0,
    pthread_cond_lock_alloc,
    pthread_cond_lock_mm_free,
    pthread_cond_lock_signal,
    pthread_cond_lock_wait,
};

void set_cond_ops(struct cond_ops ops) {
    scnet_cond_ops = ops;
}

static void *pthread_cond_lock_alloc(int mode) {
    pthread_cond_t *cond = (pthread_cond_t*)mm_malloc(sizeof(pthread_cond_t));
    if (pthread_cond_init(cond, NULL)) {
        log_error("pthread_cond_init error");
        mm_free(cond);
        return NULL;
    }
    return cond;
}

static void pthread_cond_lock_mm_free(void *p, int mode) {
    pthread_cond_t *cond = (pthread_cond_t*)p;
    pthread_cond_destroy(cond);
    mm_free(p);
}

static int pthread_cond_lock_signal(void *p, int mode) {
    pthread_cond_t *cond = (pthread_cond_t*)p;
    if (mode == 0) { /* signal */
        return pthread_cond_signal(cond);
    } else if (mode == 1) { /* broadcast */
        return pthread_cond_broadcast(cond);
    }
}

static int pthread_cond_lock_wait(void *p, void *k, int64_t time_us, int mode) {
    pthread_cond_t *cond = (pthread_cond_t*)p;
    pthread_mutex_t *lock = (pthread_mutex_t*)k;
    if (time_us) {
        struct timespec timeout;
        int64_t timenow_us = get_time_us() + time_us;
        timeout.tv_sec  = timenow_us / MICOR_SECOND;
        timeout.tv_nsec = timenow_us % MICOR_SECOND * 1000;
        pthread_cond_timedwait(cond, lock, &timeout);
        //log_fatal("pthread_cond_wait_timed not supported");
    } else {
        return pthread_cond_wait(cond, lock);
    }
}