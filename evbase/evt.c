#include <unistd.h>
#include <string.h>

#include <evbase/evt.h>
#include <evbase/util.h>
#include <evbase/log.h>
#include <evbase/epoll.h>

EL_P evt_loop_init_with_flag(int flag) {
    int i;

    EL_P loop = (EL_P)mm_malloc(sizeof(struct evt_loop));
    memset(loop, 0, sizeof(struct evt_loop));

    loop->owner_thread = 0;
    loop->status = LOOP_STATU_INIT;

    loop->priority_max = 0;

    /* init fds */
    loop->fds_size = LOOP_INIT_FDS;
    loop->fds_mod_size = LOOP_INIT_FDS;
    loop->fds_mod_cnt = 0;
    loop->fds = (FDI_P)mm_malloc(sizeof(struct fd_info) * LOOP_INIT_FDS);
    loop->fds_mod = (int*)mm_malloc(sizeof(int) * LOOP_INIT_FDS);


    /* init backend */
    if (0) {
        /* only supprot epoll in first version*/
    } else {
        loop->poll_init = epoll_init;
    }
    loop->poll_time_us = LOOP_INIT_POLLUS;
    loop->poll_more_ptr = loop->poll_init(loop);
    if (loop->poll_more_ptr == NULL) {
        goto loop_init_failed;
    }

    /* init pending queue (only one queue on init) */
    loop->pending_size[0] = LOOP_INIT_PENDSIZE;
    loop->pending_cnt[0] = 0;
    loop->pending[0] = (EB_P)mm_malloc(sizeof(EB_P) * LOOP_INIT_PENDSIZE);



    return loop;

loop_init_failed:
    log_error("evt_loop init failed!");
    /* release resouce */

    return NULL;
}

EL_P evt_loop_init() {
    /* without flag, use default config */
    return evt_loop_init_with_flag(0);
}

int evt_loop_quit(EL_P loop) {

}

int evt_loop_destroy(EL_P loop) {

}

int evt_loop_run(EL_P loop) {
    while (1) {
        evt_fd_changes_update(loop);
        loop->poll_dispatch(loop);
        evt_execute_pending(loop);
    }
}

void evt_append_pending(EL_P loop, void *w, uint8_t event) {
    log_trace("appending a event");


}

void evt_execute_pending(EL_P loop) {

}

/* io event && fd operation*/
void evt_io_start(EL_P loop, struct evt_io* w) {
    FDI_P fdi;
#ifndef NDEBUG
    int debug_osize = loop->fds_size;
    int debug_ocsize = loop->fds_mod_size;
#endif
    /* adjust evnet param */
    w->active = 1;
    if (w->priority > loop->priority_max)
        w->priority = loop->priority_max;
    if (w->priority < 0)
        w->priority = 0;

    /* check if fd event need expand (if need, expand it)*/
    check_and_expand_array(loop->fds, struct fd_info, loop->fds_size,
        w->fd + 1, multi_two, init_array_zero);
    fdi = loop->fds + w->fd;

#ifndef NDEBUG
    if (debug_osize != loop->fds_size)
        log_debug("fds size increased %d => %d", debug_osize, loop->fds_size);
#endif

    /* add to fd's event list */
    ((EBL_P)w)->next = fdi->head;
    fdi->head = (EBL_P)w;

}

void evt_fd_change(EL_P loop) {

}

void evt_fd_changes_update(EL_P loop) {

}