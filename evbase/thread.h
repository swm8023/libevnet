#ifndef EVBASE_THREAD_H
#define EVBASE_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

/* current thread */
extern __thread int cr_thread_id;
extern __thread const char* cr_thread_name;
#define THREAD_ID cr_thread_id
int thread_id();
const char* thread_name();


/* atocmic val */
typedef volatile int32_t atomic32;
typedef volatile int64_t atomic64;
#define atomic_get(x)           (__sync_val_compare_and_swap(&(x), 0, 0))
#define atomic_get_add(x, t)    (__sync_fetch_and_add(&(x), t))
#define atomic_add_get(x, t)    (__sync_add_and_fetch(&(x), t))
#define atomic_increment(x)     (__sync_add_and_fetch(&(x), 1))
#define atomic_decrement(x)     (__sync_add_and_fetch(&(x), -1))


/* mutex lock */
typedef void* lock_t;
struct lock_ops {
    const char* lockname;
    int feature;
    void *(*alloc)(int);
    void (*free)(void *, int);
    int (*lock)(void *, int);
    int (*unlock)(void *, int);
};
extern struct lock_ops scnet_lock_ops;
void set_lock_ops(struct lock_ops);

#define lock_alloc_attr(lockvar, mode) if (scnet_lock_ops.alloc) \
    lockvar = scnet_lock_ops.alloc(mode)
#define lock_free_attr(lockvar, mode) if (lockvar && scnet_lock_ops.free) \
    scnet_lock_ops.free(lockvar, mode)
#define lock_lock_attr(lockvar, mode) if (lockvar && scnet_lock_ops.lock) \
    scnet_lock_ops.lock(lockvar, mode)
#define lock_unlock_attr(lockvar, mode) if (lockvar && scnet_lock_ops.unlock) \
    scnet_lock_ops.unlock(lockvar, mode)

#define lock_lock(lockvar) lock_lock_attr(lockvar, 0)
#define lock_unlock(lockvar) lock_unlock_attr(lockvar, 0)
#define lock_alloc(lockvar) lock_alloc_attr(lockvar, 0)
#define lock_free(lockvar) lock_free_attr(lockvar, 0)


/* condition */
typedef void* cond_t;
struct cond_ops {
    const char* condname;
    int feature;
    void *(*alloc)(int);
    void (*free)(void *, int);
    int (*signal)(void *cond, int);
    int (*wait)(void *cond, void *lock,
        const struct timeval *, int);
};
extern struct cond_ops scnet_cond_ops;
void set_cond_ops(struct cond_ops);

#define cond_alloc_attr(condvar, mode) if (scnet_cond_ops.alloc) \
    condvar = scnet_cond_ops.alloc(mode)
#define cond_free_attr(condvar, mode) if (condvar && scnet_cond_ops.free) \
    scnet_cond_ops.free(condvar, mode)
#define cond_signal_attr(condvar, mode) if (condvar && scnet_cond_ops.signal) \
    scnet_cond_ops.signal(condvar, mode)
#define cond_wait_attr(condvar, lockvar, timeout, mode) if (condvar && scnet_cond_ops.alloc) \
    scnet_cond_ops.wait(condvar, lockvar, timeout, mode)

#define cond_alloc(condvar) cond_alloc_attr(condvar, 0)
#define cond_free(condvar) cond_free_attr(condvar, 0)
#define cond_signal(condvar) cond_signal_attr(condvar, 0)
#define cond_broadcast(condvar) cond_signal_attr(condvar, 1)
#define cond_wait(condvar, lockvar) cond_wait_attr(condvar, lockvar, NULL, 0)
#define cond_wait_timed(condvar, lockvar, timeval) cond_wait_attr(condvar, lockvar, timeval, 0)



/* thread (just some macros) */
#define thread_t pthread_t
#define THREAD_FUNCTION(fn, val) void* fn(void *val)
#define THREAD_RETURN() return (NULL)
#define thread_start(pt, fn, arg) pthread_create(&(pt), NULL, fn, arg)
#define thread_join(pt) pthread_join(pt, NULL)
#define thread_detach(pt) pthread_detach(pt)


#ifdef __cplusplus
}
#endif
#endif  //EVBASE_THREAD_H