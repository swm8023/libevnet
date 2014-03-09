#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include <evbase/epoll.h>
#include <evbase/evt.h>
#include <evbase/util.h>


void *epoll_init(EL_P loop) {
    EEPOLL_P ept = (EEPOLL_P)mm_malloc(sizeof(struct evt_epoll));
    memset(ept, 0, sizeof(struct evt_epoll));

    /* use epoll_create1 first */
#ifdef EPOLL_CLOEXEC
    ept->fd = epoll_create1(EPOLL_CLOEXEC);
#else
    ept->fd = epoll_create(1);
    if (ept->fd >= 0) 
        fd_cloexec(ept->fd)
#endif

    if (ept->fd < 0) {
        goto epoll_init_failed;
    }
    ept->feature = loop->poll_feature;
    ept->nevent = EPOLL_INIT_NEVENT;
    ept->event = (EP_EVT*)mm_malloc(sizeof(EP_EVT) * EPOLL_INIT_NEVENT);

    /* init fucntion pointer with loop*/
    loop->poll_destroy = epoll_destroy;
    loop->poll_dispatch = epoll_dispatch;
    loop->poll_update = epoll_update;

    log_inner("epoll(%d) init complete", ept->fd);
    return ept;

epoll_init_failed:
    log_error("epoll init failed!");
    if (ept->fd > 0)
        close (ept->fd);
    if (ept->event)
        mm_free(ept->event);
    mm_free(ept);
    return NULL;
}

void epoll_destroy(EL_P loop) {
    EEPOLL_P ept = (EEPOLL_P)loop->poll_more_ptr;
    if (ept->fd > 0)
        close(ept->fd);
    if (ept->event)
        mm_free(ept->event);
    mm_free(ept);
}

int epoll_dispatch(EL_P loop) {
    int i, j;
    EEPOLL_P ept = (EEPOLL_P)loop->poll_more_ptr;

    /* cal wait time */

    int wait_ms = loop->poll_time_us;
    if (wait_ms > 0) wait_ms /= 1000;

    int evtcnt = epoll_wait(ept->fd, ept->event, ept->nevent, wait_ms);
    log_inner("epoll(%d) return %d events", ept->fd, evtcnt);
    if (evtcnt < 0) {
        if (errno == EINTR) {
            return 0;
        }
        return -1;
    }
    /* append event to pendind queue */
    for (i = 0; i < evtcnt; i++) {
        EP_EVT *evt = ept->event + i;
        int fd = evt->data.fd;
        uint8_t revents = (((evt->events & (EPOLLIN  | EPOLLERR | EPOLLHUP)) ? EVT_READ  : 0)
                         | ((evt->events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) ? EVT_WRITE : 0));
        FDI_P fdi = loop->fds + fd;
        EBL_P w;

        for (w = fdi->head; w; w = w->next) {
            struct evt_io* wio = (struct evt_io*)w;
            uint8_t evt = wio->event & revents;
            if (evt) {
                evt_append_pending(loop, (EB_P)wio, evt);
            }
        }





    }
    /* if evtcnt == nevent,     events array */
    if (evtcnt == ept->nevent && ept->nevent < EPOLL_MAX_NEVENT) {
        ept->nevent *= 2;
        ept->event = mm_realloc(ept->event, sizeof(EP_EVT) * ept->nevent);
        log_inner("epoll(%d) nevent expand to %d", ept->fd, ept->nevent);
    }
    return evtcnt;
}

int epoll_update(EL_P loop, int fd) {

}