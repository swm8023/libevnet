#ifndef EVNET_BUFFER_H
#define EVNET_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFF_INIT_SIZE 4096
typedef struct buffer* BUF_P;
typedef struct buffer BUF;
struct buffer {
    char *buf;
    int size;
    int r_ind;
    int w_ind;
};

#define BUFF_LEFT(buf)         ((buf)->size - (buf)->w_ind)
#define BUFF_USED(buf)         ((buf)->w_ind - (buf)->r_ind)
#define BUFF_REAL_LEFT(buf)    ((buf)->size - (buf)->w_ind + (buf)->r_ind)
#define BUFF_SIZE(buf)         ((buf)->size)
#define BUFF_SETRIND(buf, pos) ((buf)->r_ind) = (pos))
#define BUFF_SETWIND(buf, pos) ((buf)->w_ind) = (pos))


void buff_init(BUF_P, int);
void buff_clear(BUF_P);
BUF_P buff_new();
BUF_P buff_new_size(int size);
void buff_free(BUF_P);
int buff_expand(BUF_P, int);
const char* buff_peek(BUF_P);
int buff_write(BUF_P, const char*, int);
int buff_read(BUF_P, int, char*, int);
int buff_readall(BUF_P, char*, int);
int buff_fd_read(BUF_P, int);
int buff_fd_write(BUF_P, int);

#ifdef __cplusplus
}
#endif

#endif //EVNET_BUFFER_H