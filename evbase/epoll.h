#ifndef EVBASE_EPOLL_H
#define EVBASE_EPOLL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <evbase/evt.h>

#define EPOLL_INIT_NEVENT   32
#define EPOLL_MAX_NEVENT    4069

typedef struct evt_epoll* EEPOLL_P;
typedef struct epoll_event EP_EVT;
struct evt_epoll {
    int fd;
    int feature;
    int nevent;
    struct epoll_event *event;
};


void *epoll_init(EL_P);
void epoll_destroy(EL_P);
int epoll_dispatch(EL_P);
int epoll_update(EL_P, int, uint8_t, uint8_t);


#ifdef __cplusplus
}
#endif
#endif  //EVBASE_EPOLL_H