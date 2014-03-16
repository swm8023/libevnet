#include <string.h>
#include <stdio.h>
#include <evnet/filelog.h>
#include <evbase/log.h>
#include <unistd.h>

/* file log interface */
static void log_file_output_cb(const char *, size_t);
static void log_file_flush_cb();
static void log_file_fatal_cb();

#ifndef NDEBUG
int max_free = 0;
int max_full = 0;
#endif


static struct log_if file_log_if = {
    (uint8_t)(LOG_MASK),
    log_file_output_cb,
    log_file_flush_cb,
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        log_file_fatal_cb
    }
};
static struct async_filelog *aflog;

static THREAD_FUNCTION(flog_backend, p) {
    int i;
    /* used to make condition section short */
    int pre_fullbufcnt = 0;
    aflog->pid = pthread_self();
    while (aflog->status & FILELOG_STATUS_RUN) {
        /* cs start */
        lock_lock(aflog->flog_lock);

        if (pre_fullbufcnt > 0) {
            /* append previous full buffers to empty buffers first*/
            check_and_expand_array(aflog->emptybufs, BUF_P, aflog->emptybuf_size,
                aflog->emptybuf_cnt + pre_fullbufcnt, add_one, init_array_noop);
            for (i = 0; i < pre_fullbufcnt; i++)
                aflog->emptybufs[aflog->emptybuf_cnt++] = aflog->fullbufs[i];
            /* aflog->fullbuf_cnt maybe increased when write fullbuf to file,
                now move increased full buffers to begin of the array.(just some pointers)
            */
            if (pre_fullbufcnt < aflog->fullbuf_cnt) {
                memmove(aflog->fullbufs, aflog->fullbufs + pre_fullbufcnt,
                    sizeof(BUF_P) * (aflog->fullbuf_cnt - pre_fullbufcnt));
            }
            /* decrease fullbuf_cnt */
            aflog->fullbuf_cnt -= pre_fullbufcnt;
        }
        /* wait until the full buffers not empty, but don't use while here,
           because sometimes it should be wake up even if no buffer is full
         */
        if (aflog->fullbuf_cnt == 0) {
            /* almost wait for 3 second, then write using buffer to file */
            cond_wait_timed(aflog->flog_cond, aflog->flog_lock, SECOND(3));
        }

        /* append using buffer to full buffers */
        if (BUFF_USED(aflog->usingbuf) > 0) {
            check_and_expand_array(aflog->fullbufs, BUF_P, aflog->fullbuf_size,
                aflog->fullbuf_cnt+1, add_one, init_array_noop);
            aflog->fullbufs[aflog->fullbuf_cnt++] = aflog->usingbuf;
            aflog->usingbuf = (aflog->emptybuf_cnt == 0 ? buff_new_size(FILELOG_BUFSZ) :
                aflog->emptybufs[--aflog->emptybuf_cnt]);
        }

        pre_fullbufcnt = aflog->fullbuf_cnt;
        lock_unlock(aflog->flog_lock);

        /* cs end, write 0~pre_fullbufcnt to file */
        for (i = 0; i < pre_fullbufcnt; i++) {
            BUF_P buff = aflog->fullbufs[i];
            fwrite(buff_peek(buff), BUFF_USED(buff), 1, aflog->fp);
            /* clear the buffer */
            buff_clear(buff);
        }
        fflush(aflog->fp);
    }
#ifndef NDEBUG
    max_free = aflog->emptybuf_size;
    max_full = aflog->fullbuf_size;
#endif
}


int filelog_init(const char *filepath) {
    int i;
    thread_t flog_thread;
    FILE *fp;
    if ((fp = fopen(filepath,"a"))==NULL) {
        printf("cant open the log file!");
        return -1;
    }

    aflog = (struct async_filelog*)mm_malloc(sizeof(struct async_filelog));
    aflog->fp = fp;
    /* alloc buffer, init 5 free buffer */
    aflog->fullbuf_size  =  FILELOG_BUFS_SIZE;
    aflog->emptybuf_size = FILELOG_BUFS_SIZE;
    aflog->fullbuf_cnt  = 0;
    aflog->emptybuf_cnt = FILELOG_BUFS_FREE;
    aflog->fullbufs  = (BUF_P*)mm_malloc(sizeof(BUF_P) * FILELOG_BUFS_SIZE);
    aflog->emptybufs = (BUF_P*)mm_malloc(sizeof(BUF_P) * FILELOG_BUFS_SIZE);
    for (i = 0; i < aflog->emptybuf_cnt; i++) {
        aflog->emptybufs[i] = buff_new_size(FILELOG_BUFSZ);
    }
    /* using last free buffer as using buffer */
    aflog->usingbuf = aflog->emptybufs[--aflog->emptybuf_cnt];

    /* alloc lock*/
    lock_alloc(aflog->flog_lock);
    cond_alloc(aflog->flog_cond);

    /* backend thread, write full buffer to file */
    aflog->status = FILELOG_STATUS_RUN;
    thread_start(flog_thread, flog_backend, NULL);

    /* set default log interface to file log interface */
    default_log_if = &file_log_if;
    return 0;
}
void filelog_destroy() {
    int i;
    /* wait the thread quit */
    aflog->status = FILELOG_STATUS_QUIT;
    cond_signal(aflog->flog_cond);
    thread_join(aflog->pid);
    /* flush the buffers */
    for (i = 0; i < aflog->fullbuf_cnt; i++) {
        BUF_P buff = aflog->fullbufs[i];
        fwrite(buff_peek(buff), BUFF_USED(buff), 1, aflog->fp);
    }
    BUF_P buff = aflog->usingbuf;
    fwrite(buff_peek(buff), BUFF_USED(buff), 1, aflog->fp);
    fflush(aflog->fp);
    fclose(aflog->fp);

}

void filelog_quit() {

}

// real    0m27.901s
// user    0m2.775s
// sys 0m10.547s
// static void log_file_output_cb(const char *cont, size_t len) {
//     lock_lock(aflog->flog_lock);
//     fwrite(cont, len, 1, aflog->fp);
//     lock_unlock(aflog->flog_lock);
// }

// real    0m30.603s
// user    0m3.310s
// sys 0m11.066s
// static void log_file_output_cb(const char *cont, size_t len) {
//     lock_lock(aflog->flog_lock);
//     if (BUFF_LEFT(aflog->usingbuf) < len) {
//         buff_write(aflog->usingbuf, cont, len);
//     } else {
//         BUF_P buf = aflog->usingbuf;
//         fwrite(buff_peek(buf), BUFF_USED(buf), 1, aflog->fp);
//         buff_clear(buf);
//         buff_write(aflog->usingbuf, cont, len);
//     }
//     lock_unlock(aflog->flog_lock);
// }

// real    0m14.238s
// user    0m9.165s
// sys 0m2.107s

/* don't use log_xxx in this function, a recursive lock will be generated */
static void log_file_output_cb(const char *cont, size_t len) {
    lock_lock(aflog->flog_lock);
    /* nearly never happend, buffer size is bigger enough*/
    if (BUFF_SIZE(aflog->usingbuf) < len)
        return;
    /* using buffer is enough */
    if (BUFF_LEFT(aflog->usingbuf) > len) {
        buff_write(aflog->usingbuf, cont, len);
    /* not enough, switch to a empty buff */
    } else {
        /* push using buffer to full buffer list */
        check_and_expand_array(aflog->fullbufs, BUF_P, aflog->fullbuf_size, 
            aflog->fullbuf_cnt+1, add_one, init_array_noop);
        aflog->fullbufs[aflog->fullbuf_cnt++] = aflog->usingbuf;
        /* if there is any buffer in empty buffers, use it as using buffer */
        if (aflog->emptybuf_cnt > 0) {
            aflog->usingbuf = aflog->emptybufs[--aflog->emptybuf_cnt];
        /* no empty buffer, alloc new one as using buffer*/
        } else {
            aflog->usingbuf = buff_new_size(FILELOG_BUFSZ);
        }
        buff_write(aflog->usingbuf, cont, len);
        /* wake up backend log thread to write full buffers to file */
        cond_signal(aflog->flog_cond);
    }
    lock_unlock(aflog->flog_lock);
}

static void log_file_flush_cb() {
    cond_signal(aflog->flog_cond);
    fflush(aflog->fp);
}

static void log_file_fatal_cb() {
    filelog_quit();
    _exit(-1);
}