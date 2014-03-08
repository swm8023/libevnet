#ifndef EVUTIL_EVT_H
#define EVUTIL_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>





struct evt_loop;
typedef struct evt_loop* EL_P;

/* events */
#define EVT_CALLBACK(type) \
    void (*cb)(struct evt_loop*, struct type*)

#define EVT_BASE(type)  \
    int active;         \
    int pos;            \
    int priority;       \
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
    int event;
};

struct evt_timer{
    EVT_BASE(evt_timer);

    int64_t timestamp;
};

/* fd */
struct fd_info {
    int fd;
    uint8_t oevt;
    uint8_t nevt;
    uint8_t revt;
    EBL_P head;
};
typedef struct fd_info* FDI_P;


/* evt_loop */
#define LOOP_STATU_INIT    0x01
#define LOOP_STATU_STARTED 0x02
#define LOOP_STATU_RUNNING 0x04
#define LOOP_STATU_PAUSE   0x08
#define LOOP_STATU_QUITING 0x10
#define LOOP_STATU_STOP    0x20
#define LOOP_INIT_FDS      32

struct evt_loop {
    /* status */
    int owner_thread;
    uint8_t status;

    /* config */
    uint8_t priority_max;   /* 0 ~ max*/

    /* io event */
    int *fds_mod;
    FDI_P *fds;
    int fds_size;
    int fds_mod_size;
};

EL_P evt_loop_init();
int evt_loop_quit(EL_P);
int evt_loop_destroy(EL_P);
int evt_loop_run(EL_P);

#ifdef __cplusplus
}
#endif
#endif  //EVUTIL_UTIL_H