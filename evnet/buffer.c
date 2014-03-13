#include <string.h>
#include <unistd.h>

#include <evbase/util.h>
#include <evbase/evt.h>
#include <evbase/log.h>
#include <evnet/buffer.h>

BUF_P buff_new() {
    BUF_P buf;
    buf = (char*)mm_malloc(sizeof(BUF));
    buff_init(buf);
    return buf;
}

void buff_free(BUF_P buf) {
    if (buf)
        mm_free(buf->buf);
    mm_free(buf);
}

void buff_init(BUF_P buf) {
    buf->size = BUFF_INIT_SIZE;
    buf->buf = (char*)mm_malloc(buf->size);
    buf->r_ind = buf->w_ind = 0;
}



int buff_expand(BUF_P buf, int size) {
    int oldsize = BUFF_USED(buf);
    /* expand bufsize = bufsize * 2 until bigger than size */
    check_and_expand_array(buf->buf, char, buf->size, size, multi_two, init_array_noop);

    log_inner("buffer size expand to %d", BUFF_SIZE(buf));
    return 0;
}

const char* buff_peak(BUF_P buf) {
    /* return the pointer point to data started */
    return buf->buf + buf->r_ind;
}

int buff_write(BUF_P buf, const char* str, int len) {
    /* if the buffer is actually empty */
    if (buf->r_ind == buf->w_ind) {
        buf->r_ind = buf->w_ind = 0;
    }
    /* if space not enough */
    if (len > BUFF_LEFT(buf)) {
        /* real enough! expand it*/
        if (len > BUFF_REAL_LEFT(buf)) {
            buff_expand(buf, BUFF_USED(buf) + len);
        }
        /* move data to the begin of buffer */
        memmove(buf->buf, buf->buf + buf->r_ind, BUFF_USED(buf));
        buf->w_ind = BUFF_USED(buf);
        buf->r_ind = 0;
    }
    /* append data here */
    memcpy(buf->buf + buf->w_ind, str, len);
    buf->w_ind += len;
    return len;
}

int buff_read(BUF_P buf, int len, char* dst, int dstlen) {
    /* adjust len */
    if (len > dstlen) {
        len = dstlen;
    }
    if (len > BUFF_USED(buf)) {
        len = BUFF_USED(buf);
    }
    memcpy(dst, buf->buf + buf->r_ind, len);
    buf->r_ind += len;
    return len;
}

int buff_readall(BUF_P buf, char* dst, int dstlen) {
    return buff_read(buf, BUFF_USED(buf), dst, dstlen);
}

/* read into buffer with fd*/
int buff_fd_read(BUF_P buf, int fd) {
    int len = 0;
    /* if the buffer actually empty */
    if (buf->r_ind == buf->w_ind) {
        buf->r_ind = buf->w_ind = 0;
    }
    /* if space not enough */
    if (0 == BUFF_LEFT(buf)) {
        /* real enough! expand it*/
        if (0 == BUFF_REAL_LEFT(buf)) {
            buff_expand(buf, BUFF_USED(buf) + len);
        }
        /* move data to the begin of buffer */
        memmove(buf->buf, buf->buf + buf->r_ind, BUFF_USED(buf));
        buf->w_ind = BUFF_USED(buf);
        buf->r_ind = 0;
    }
    /* use syscall 'read' */
    len = read(fd, buf->buf + buf->w_ind, BUFF_LEFT(buf));
    if (len <= 0) {
        return len;
    }
    buf->w_ind += len;
    return len;
}

/* write from buffer with fd */
int buff_fd_write(BUF_P buf, int fd) {
    int len = 0;
    if (0 == BUFF_USED(buf)) {
        return 0;
    }
    len = write(fd, buf->buf + buf->r_ind, BUFF_USED(buf));
    if (len <= 0) {
        return len;
    }
    buf->r_ind += len;
    return len;
}
