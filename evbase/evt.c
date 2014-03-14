#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <evbase/evt.h>
#include <evbase/util.h>
#include <evbase/log.h>
#include <evbase/epoll.h>

static void evt_list_add_tail(EBL_P*, EBL_P);
static void evt_list_add(EBL_P*, EBL_P);
static void evt_list_del(EBL_P*, EBL_P);
static void evt_do_loop_wakeup(EL_P, struct evt_io*);

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

    /* before && after event */
    loop->evt_befores_head = NULL;
    loop->evt_afters_head = NULL;

    /* timer event */
    loop->timer_heap_size = LOOP_INIT_EVTSIZE;
    loop->timer_heap_cnt = 0;    /* start from 1 */
    loop->timer_heap = (struct evt_timer**)
        mm_malloc(sizeof(struct evt_timer*) * LOOP_INIT_EVTSIZE);

    /* async queue */
    lock_alloc(loop->asyncq_lock);
    loop->asyncq_size = LOOP_INIT_EVTSIZE;
    loop->asyncq_cnt = 0;
    loop->asyncq = (struct event_param*)
        mm_malloc(sizeof(struct event_param) * LOOP_INIT_EVTSIZE);


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
    loop->pending[0] = (EB_P*)mm_malloc(sizeof(EB_P) * LOOP_INIT_PENDSIZE);

    /* init default callback */
    /* a event do nothing, used when remove a pending event */
    loop->empty_ev = (struct evt_before*)mm_malloc(sizeof(struct evt_before));
    evt_before_init(loop->empty_ev, NULL);

    /* event fd*/
    loop->eventfd = eventfd(0, 0);
    if (loop->eventfd < 0) {
        log_error("init eventfd error");
        goto loop_init_failed;
    }
    fd_nonblock(loop->eventfd);
    loop->eventio = (struct evt_io*)mm_malloc(sizeof(struct evt_io));
    evt_io_init(loop->eventio, evt_do_loop_wakeup, loop->eventfd, EVT_READ);
    evt_io_start(loop, loop->eventio);

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
    loop->owner_thread = thread_id();
    loop->status = LOOP_STATU_STARTED | LOOP_STATU_RUNNING;

    while (1) {
        EBL_P eb;

        update_cached_time();

        /* do event before poll */
        for (eb = loop->evt_befores_head; eb; eb = eb->next) {
            evt_append_pending(loop, eb);
        }
        evt_execute_pending(loop);

        /* update fd changes (poll update) */
        evt_fd_changes_update(loop);

        /* cal the poll time (in us)*/
        update_cached_time();
        loop->poll_time_us = LOOP_INIT_POLLUS;
        if (loop->timer_heap_cnt > 0 && (loop->poll_time_us == -1 ||
            loop->timer_heap[1]->timestamp - CACHED_TIME < loop->poll_time_us)) {
            loop->poll_time_us = loop->timer_heap[1]->timestamp - CACHED_TIME;
            if (loop->poll_time_us < 0)
                loop->poll_time_us = 0;
        }

        /* poll */
        loop->poll_dispatch(loop);
        update_cached_time();

        /* queue timer event*/
        while (loop->timer_heap_cnt > 0) {
            struct evt_timer *top = loop->timer_heap[1];
            if (top->timestamp > CACHED_TIME)
                break;
            evt_append_pending(loop, top);
            HEAP_POP(loop->timer_heap, struct evt_timer*, loop->timer_heap_cnt, TIMERP_CMP);

            /* if the timer should repeat, repush to the heap */
            if (top->repeat > 0) {
                top->timestamp = top->repeat;
                evt_timer_start(loop, top);
            }
        }

        /*queue event after poll */
        for (eb = loop->evt_afters_head; eb; eb = eb->next) {
            evt_append_pending(loop, eb);
        }
        evt_execute_pending(loop);
    }
}

void evt_append_pending(EL_P loop, void *w) {
    EB_P ev = (EB_P)w;
    int pri = ev->priority;

    assert(pri <= loop->priority_max);
    /* only append it if the event not in pending queue */
    if (ev->pendpos == 0) {
        check_and_expand_array(loop->pending[pri], EB_P, loop->pending_size[pri],
            loop->pending_cnt[pri] + 1, multi_two, init_array_zero);

        loop->pending[pri][loop->pending_cnt[pri]] = ev;
        ev->pendpos = ++loop->pending_cnt[pri];
    }
}

void evt_execute_pending(EL_P loop) {
    int i, j;

    /* execute high priority event first*/
    for (i = 0; i <= loop->priority_max; i++) {
        for (j = 0; j < loop->pending_cnt[i]; j++) {
            EB_P ev = loop->pending[i][j];
            ev->pendpos = 0;
            if (ev->cb) {
                ev->cb(loop, ev);
            }
        }
        loop->pending_cnt[i] = 0;
    }
}


/* io event && fd operation*/
void evt_io_start(EL_P loop, struct evt_io* w) {
    FDI_P fdi;
#ifndef NDEBUG
    int debug_osize = loop->fds_size;
#endif
    /* adjust event param */
    if (w->active == 1) return;
    w->active = 1;
    adjust_between(w->priority, 0, loop->priority_max);

    /* check if fd event need expand (if need, expand it)*/
    check_and_expand_array(loop->fds, struct fd_info, loop->fds_size,
        w->fd + 1, multi_two, init_array_zero);

    fdi = loop->fds + w->fd;

#ifndef NDEBUG
    if (debug_osize != loop->fds_size)
        log_debug("fds size increased %d => %d", debug_osize, loop->fds_size);
#endif

    /* add to fd's event list */
    evt_list_add(&fdi->head, (EBL_P)w);

    /* add fd to change list */
    evt_fd_change(loop, w->fd);
}

void evt_io_stop(EL_P loop, struct evt_io *ev) {
    /* if ev is in pending queue, use a empty ev instead it */
    if (ev->active == 0) return;
    ev->active = 0;
    if (ev->pendpos) {
        loop->pending[ev->priority][ev->pendpos-1] = (EB_P)loop->empty_ev;
        ev->pendpos = 0;
    }

    /* remove from fd's event list */
    evt_list_del(&(loop->fds[ev->fd]).head, (EBL_P)ev);

    /* add fd to change list */
    evt_fd_change(loop, ev->fd);
}

void evt_fd_change(EL_P loop, int fd) {
    FDI_P fdi = loop->fds + fd;

    /* if this fd is not in fd_mod array, append it */
    if (!(fdi->flag & FD_FLAG_CHANGE)) {
        check_and_expand_array(loop->fds_mod, int, loop->fds_mod_size,
            loop->fds_mod_cnt + 1, add_one, init_array_noop);
        loop->fds_mod[loop->fds_mod_cnt] = fd;
        ++loop->fds_mod_cnt;
        fdi->flag != FD_FLAG_CHANGE;
    }
}

void evt_fd_changes_update(EL_P loop) {
    int i;
    EBL_P w;

    /* for each fd change, check if do poll update*/
    for (i = 0; i < loop->fds_mod_cnt; i++) {
        int fd = loop->fds_mod[i];
        FDI_P fdi = loop->fds + fd;
        uint8_t oevt = fdi->events;

        fdi->events = 0;
        fdi->flag &= ~FD_FLAG_CHANGE;
        /* get events focusing now */
        for (w = fdi->head; w; w = w->next) {
            fdi->events |= ((struct evt_io*)w)->event;
        }
        /* if focused event really changed, do poll update */
        if (oevt != fdi->events) {
            loop->poll_update(loop, fd, oevt, fdi->events);
        }
    }
    /* clear */
    loop->fds_mod_cnt = 0;
}

/* timer event */
void evt_timer_start(EL_P loop, struct evt_timer* ev) {
    /* adjust event param */
    if (ev->active == 1 && ev->repeat == 0) return;
    ev->active = 1;
    adjust_between(ev->priority, 0, loop->priority_max);

    /* will push 1, +1; start from 1, +1; so need size +2 */
    check_and_expand_array(loop->timer_heap, struct evt_timer*, loop->timer_heap_size,
            loop->timer_heap_cnt + 2, add_one, init_array_noop);

    /* adjust relative time to absolute time */
    ev->timestamp += get_cached_time();

    /* push the evt_timer into heap */
    HEAP_PUSH(loop->timer_heap, struct evt_timer*, loop->timer_heap_cnt, ev, TIMERP_CMP);
}

void evt_timer_stop(EL_P loop, struct evt_timer* ev) {
    /* if ev is in pending queue, use a empty ev instead it */
    if (ev->active == 0) return;
    ev->active = 0;
    if (ev->pendpos) {
        loop->pending[ev->priority][ev->pendpos-1] = (EB_P)loop->empty_ev;
        ev->pendpos = 0;
    }

    if (ev->heap_pos > 0) {
        HEAP_DELETE(loop->timer_heap, struct evt_timer*, ev->heap_pos,
            loop->timer_heap_cnt, TIMERP_CMP);
    }

}

/* before event && after event */
void evt_before_start(EL_P loop, struct evt_before* ev) {
    /* adjust event param */
    if (ev->active == 1) return;
    ev->active = 1;
    adjust_between(ev->priority, 0, loop->priority_max);

    /* add to before events list */
    evt_list_add_tail(&loop->evt_befores_head, (EBL_P)ev);
}

void evt_before_stop(EL_P loop, struct evt_before* ev) {
    /* if ev is in pending queue, use a empty ev instead it */
    if (ev->active == 0) return;
    ev->active = 0;
    if (ev->pendpos) {
        loop->pending[ev->priority][ev->pendpos-1] = (EB_P)loop->empty_ev;
        ev->pendpos = 0;
    }

    /* remove from befores list */
    evt_list_del(&loop->evt_befores_head, (EBL_P)ev);
}

void evt_after_start(EL_P loop, struct evt_after* ev) {
    /* adjust event param */
    if (ev->active == 1) return;
    ev->active = 1;
    adjust_between(ev->priority, 0, loop->priority_max);

    /* add to after events list */
    evt_list_add_tail(&loop->evt_afters_head, (EBL_P)ev);
}

void evt_after_stop(EL_P loop, struct evt_after* ev) {
    /* if ev is in pending queue, use a empty ev instead it */
    if (ev->active == 0) return;
    ev->active = 0;
    if (ev->pendpos) {
        loop->pending[ev->priority][ev->pendpos-1] = (EB_P)loop->empty_ev;
        ev->pendpos = 0;
    }

    /* remove from afters list */
    evt_list_del(&loop->evt_afters_head, (EBL_P)ev);
}

/* list operation */
static void evt_list_add(EBL_P* head, EBL_P elm) {
    elm->next = *head;
    *head = elm;
}

static void evt_list_add_tail(EBL_P* head, EBL_P elm) {
    while (*head)
        head = &(*head)->next;
    *head = elm;
    elm->next = NULL;
}

static void evt_list_del(EBL_P* head, EBL_P elm) {
    while (*head) {
        if (*head == elm) {
            *head = elm->next;
            break;
        }
        head = &(*head)->next;
    }
}

/* evt_pool */
EP_P evt_pool_init_with_flag(int num, int flag) {
    EP_P pool = NULL;
    int i;

    pool = (EP_P)mm_malloc(sizeof(struct evt_pool));
    memset(pool, 0, sizeof(struct evt_pool));

    lock_alloc(pool->run_lock);
    cond_alloc(pool->run_cond);

    pool->runs = 0;
    pool->loops = num;
    pool->pre_loop = num - 1;
    pool->get_next_loop = default_pool_get_next_loop;

    pool->loop = (EL_P*)mm_malloc(sizeof(EL_P) * num);
    memset(pool->loop, 0, sizeof(EL_P) * num);

    for (i = 0; i < num; i++) {
        pool->loop[i] = evt_loop_init_with_flag(flag);
        if (pool->loop[i] == NULL) {
            log_error("evt pool init fail");
            goto evt_pool_init_fail;
        }
    }
    pool->status = POOL_STATU_INIT;
    return pool;

evt_pool_init_fail:
    if (pool) {
        for (i = 0; i < num; i++) {
            if (pool->loop[i]) {
                evt_loop_destroy(pool->loop[i]);
            }
        }
        mm_free(pool->loop);
        lock_free(pool->run_lock);
        cond_free(pool->run_cond);
    }
    mm_free(pool);
    return NULL;
}

EP_P evt_pool_init(int num) {
    return evt_pool_init_with_flag(num, 0);
}

void evt_pool_destroy(EP_P pool) {

}

static EL_P default_pool_get_next_loop(EP_P pool) {
    pool->pre_loop = (pool->pre_loop + 1) % pool->loops;
    return pool->loop[pool->pre_loop];
}

static THREAD_FUNCTION(new_loop_thread, p) {
    EP_P pool = (EP_P)p;
    int run_id;

    /* lock run-id */
    lock_lock(pool->run_lock);
    run_id = pool->runs;
    pool->runs++;
    lock_unlock(pool->run_lock);

    evt_loop_run(pool->loop[run_id]);
}

int evt_pool_run(EP_P pool) {
    int i;
    thread_t th;
    pool->runs = 1;

    for (i = 1; i < pool->loops; i++) {
        thread_start(th, new_loop_thread, pool);
    }

    evt_loop_run(pool->loop[0]);

}

/* async operation from other thread */
void evt_loop_asyncq_append(EL_P loop, struct event_param* evp) {
    lock_lock(loop->asyncq_lock);
    check_and_expand_array(loop->asyncq, struct event_param, loop->asyncq_size,
        loop->asyncq_cnt + 1, multi_two, init_array_noop);
    memcpy(loop->asyncq + loop->asyncq_cnt, evp, sizeof(struct event_param));
    loop->asyncq_cnt++;
    lock_unlock(loop->asyncq_lock);
}

void wake_up_loop(EL_P loop) {
    int64_t val = 1;
    eventfd_write(loop->eventfd, val);
    /* log_debug("evfd: %d", loop->eventfd); */
}

static void evt_do_loop_wakeup(EL_P loop, struct evt_io* ev) {
    int64_t val;
    int i;
    eventfd_read(loop->eventfd, &val);

    lock_lock(loop->asyncq_lock);
    log_inner("loop->asyncq_cnt %d", loop->asyncq_cnt);

    /* do everything in the async queue */
    for (i = 0; i < loop->asyncq_cnt; i++) {
        struct event_param *ep = &loop->asyncq[i];
        /* ev_io_start */
        if (ep->type == EVENT_PARAM_EIOST) {
            struct evt_io *eio = (struct evt_io*)ep->arg;
            evt_io_start(loop, eio);
        } else if (0) {

        }
        /* if release the arg's space*/
        if (ep->temp && ep->arg) {
            mm_free(ep->arg);
        }
    }
    /* clear */
    loop->asyncq_cnt = 0;
    lock_unlock(loop->asyncq_lock);
}