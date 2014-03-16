#ifndef EVBASE_EVT_H
#define EVBASE_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <evbase/thread.h>
#include <evbase/util.h>


struct evt_loop;
typedef struct evt_loop* EL_P;

/* events */
#define EVT_CALLBACK(type) \
    void (*cb)(struct evt_loop*, struct type*)

#define EVT_BASE(type)  \
    uint8_t active;     \
    uint8_t priority;   \
    int pendpos;        \
    void *data;         \
    EVT_CALLBACK(type);

#define EVT_BASE_LIST(type)     \
    EVT_BASE(type)              \
    struct evt_base_list *next;

struct evt_base {
    EVT_BASE(evt_base)
};

struct evt_base_list {
    EVT_BASE_LIST(evt_base_list);
};

typedef struct evt_base* EB_P;
typedef struct evt_base_list* EBL_P;

struct evt_io{
    EVT_BASE_LIST(evt_io)

    int fd;
    uint8_t event;
};

struct evt_timer{
    EVT_BASE(evt_timer);

    int heap_pos;
    int64_t repeat;
    int64_t timestamp;
};

struct evt_before {
    EVT_BASE_LIST(evt_before);
};

struct evt_after {
    EVT_BASE_LIST(evt_after);
};

#define evt_set_data(ev_, data_) ((ev_)->data = (data_))

#define evt_base_init(ev_, cb_) do {    \
    (ev_)->active   = 0;                \
    (ev_)->priority = 0;                \
    (ev_)->pendpos  = 0;                \
    (ev_)->data     = NULL;             \
    (ev_)->cb       = (cb_);            \
} while (0)


#define evt_io_init(ev_, cb_, fd_, event_) do { \
    evt_base_init((ev_), (cb_));                \
    (ev_)->event = (event_);                    \
    (ev_)->fd = (fd_);                          \
} while (0)

#define evt_timer_init(ev_, cb_, after_, repeat_) do { \
    evt_base_init((ev_), (cb_));                       \
    (ev_)->repeat = (repeat_);                         \
    (ev_)->timestamp = (after_);                       \
} while (0)

#define evt_before_init(ev_, cb_) evt_base_init((ev_), (cb_))
#define evt_after_init(ev_, cb_)  evt_base_init((ev_), (cb_))

void evt_io_start(EL_P, struct evt_io*);
void evt_io_stop(EL_P, struct evt_io*);
void evt_timer_start(EL_P, struct evt_timer*);
void evt_timer_stop(EL_P, struct evt_timer*);
void evt_before_start(EL_P, struct evt_before*);
void evt_before_stop(EL_P, struct evt_before*);
void evt_after_start(EL_P, struct evt_after*);
void evt_after_stop(EL_P, struct evt_after*);

#define TIMERP_CMP(ta, tb) ((ta)->timestamp < (tb)->timestamp)

/* fd */
typedef struct fd_info* FDI_P;
struct fd_info {
    EBL_P head;
    //int fd;
    uint8_t events;
    uint8_t revents;   /* event return by poll*/
    uint8_t flag;
};

void evt_fd_changes_update(EL_P);
void evt_fd_change(EL_P, int);

#define EVT_READ      0x01
#define EVT_WRITE     0x02

#define FD_FLAG_CHANGE    0x01



/* evt_loop */
#define LOOP_STATU_INIT    0x01
#define LOOP_STATU_STARTED 0x02
#define LOOP_STATU_RUNNING 0x04
#define LOOP_STATU_PAUSE   0x08
#define LOOP_STATU_QUITING 0x10
#define LOOP_STATU_STOP    0x20

#define LOOP_INIT_FDS      32
#define LOOP_INIT_PENDSIZE 32
#define LOOP_INIT_EVTSIZE  32
#define LOOP_PRIORITY_MAX  10
#define LOOP_INIT_POLLUS   30000000

struct evt_loop {
    /* status */
    int owner_thread;
    uint8_t status;

    /* config */
    uint8_t priority_max;   /* 0 ~ max*/

    /* io event */
    int *fds_mod;
    FDI_P fds;
    int fds_size;
    int fds_mod_cnt;
    int fds_mod_size;

    /* before event && after event (list's head)*/
    EBL_P evt_befores_head;
    EBL_P evt_afters_head;

    /* timer event */
    struct evt_timer** timer_heap;
    int timer_heap_cnt;
    int timer_heap_size;

    /* event pending to run */
    EB_P *pending[LOOP_PRIORITY_MAX];
    int pending_cnt[LOOP_PRIORITY_MAX];
    int pending_size[LOOP_PRIORITY_MAX];

    /* queue of callback from other thread */
    lock_t asyncq_lock;
    struct event_param* asyncq;
    int asyncq_size;
    int asyncq_cnt;
    int eventfd;
    struct evt_io* eventio;

    /* backend */
    int poll_feature;
    int64_t poll_time_us;      /*in microsecond*/
    void *poll_more_ptr;
    void *(*poll_init)(EL_P);
    void (*poll_destroy)(EL_P);
    int (*poll_dispatch)(EL_P);
    int (*poll_update)(EL_P, int, uint8_t, uint8_t);

    /* quit lock */
    lock_t quit_lock;
    cond_t quit_cond;

    /* some default callback */
    struct evt_before *empty_ev;   /* be used when stop a pending event */
};


EL_P evt_loop_init_with_flag(int);
EL_P evt_loop_init();
int evt_loop_quit(EL_P);
int evt_loop_destroy(EL_P);
int evt_loop_run(EL_P);

void evt_append_pending(EL_P, void* /*EB_P*/);
void evt_execute_pending(EL_P loop);

/* evt_pool */
#define POOL_STATU_INIT         0x01
#define POOL_STATU_STARTED      0x02
#define POOL_STATU_RUNNING      0x04
#define POOL_STATU_PAUSE        0x08
#define POOL_STATU_QUITING      0x10
#define POOL_STATU_STOP         0x20
#define LOOP_STATU_WAITDESTROY  0x40

typedef struct evt_pool* EP_P;
struct evt_pool {
    int runs;
    int loops;
    int status;
    EL_P *loop;

    lock_t run_lock;
    cond_t run_cond;

    int pre_loop;
    EL_P (*get_next_loop)(EP_P);
};

EP_P evt_pool_init_with_flag(int, int);
EP_P evt_pool_init(int);
void evt_pool_destroy(EP_P);
int evt_pool_run(EP_P);
static EL_P default_pool_get_next_loop(EP_P);
static THREAD_FUNCTION(new_loop_thread, p);


void evt_loop_asyncq_append(EL_P, struct event_param*);
void wake_up_loop(EL_P);

#ifdef __cplusplus
}
#endif
#endif  //EVBASE_UTIL_H