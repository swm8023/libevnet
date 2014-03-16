#ifndef EVNET_FILELOG_H
#define EVNET_FILELOG_H

#include <stdint.h>

#include <evbase/thread.h>
#include <evbase/util.h>
#include <evnet/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
extern int max_free;
extern int max_full;
#endif

#define FILELOG_BUFS_SIZE   10
#define FILELOG_BUFS_FREE   5
#define FILELOG_BUFSZ       (4096*1024)
#define FILELOG_STATUS_RUN  0x01
#define FILELOG_STATUS_QUIT 0x02

struct async_filelog {
    FILE *fp;
    lock_t flog_lock;
    cond_t flog_cond;
    uint8_t status;
    pthread_t pid;

    BUF_P usingbuf;
    BUF_P *fullbufs;
    BUF_P *emptybufs;
    int fullbuf_size;
    int fullbuf_cnt;
    int emptybuf_size;
    int emptybuf_cnt;
};

int filelog_init(const char*);
void filelog_quit();
void filelog_destroy();

#ifdef __cplusplus
}
#endif

#endif //EVBASE_LOG_H