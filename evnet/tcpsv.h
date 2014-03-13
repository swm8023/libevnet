#ifndef EVNET_TCPSV_H
#define EVNET_TCPCV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <evnet/buffer.h>
#include <evbase/thread.h>

typedef struct sockaddr_in SA;
typedef struct sockaddr_in* SA_P;
typedef struct tcp_srv* TCPSRV_P;
typedef struct tcp_clt* TCPCLT_P;

#define TCP_DEFAULT_FLAG   0x00

#define TCP_SERVER_BREATH  0x01
#define TCP_SERVER_LOOPS   0x02
#define TCP_SERVER_NOREUSE 0x04
#define TCP_SERVER_ACBLOCK 0x08

#define TCP_CLIENT_WAITCLS 0x01


/* tcp server */
struct tcp_srv {
    char name[10];
    int flag;

    int fd;
    SA addr;

    atomic32 cltcnt;

    EL_P *loop_list;
    EL_P loop_on;
    int loops;

    struct evt_io* srv_io;
    void (*on_accept_comp)(TCPCLT_P);
    void (*on_read_comp)(TCPCLT_P, BUF_P, int);
    void (*on_write_comp)(TCPCLT_P, BUF_P, int);
    //void (*on_close_comp)(TCPCLT_P);
    EL_P (*get_next_loop)(TCPSRV_P);
};

#define TCP_SET_ACCEPT_COMP_CB(server, cb) \
    (server)->on_accept_comp = (cb)
#define TCP_SET_READ_COMP_CB(server, cb) \
    (server)->on_read_comp = (cb)
#define TCP_SET_WRITE_COMP_CB(server, cb) \
    (server)->on_write_comp = (cb)


/* init and destroy */
TCPSRV_P tcp_server_init(int, int);
TCPSRV_P tcp_server_init_v1(const char*addr, int, int);
TCPSRV_P tcp_server_init_v2(SA *addr, int);
void tcp_server_free(TCPSRV_P);

int tcp_server_bind_loop(TCPSRV_P, EL_P);
//int tcp_server_bind_loops(TCPSRV_P, );

/* tcp client */
struct tcp_clt {
    uint8_t flag;
    int fd;
    SA addr;
    TCPSRV_P relate_srv;

    EL_P loop_on;

    struct evt_io* clt_io[2];
    BUF_P inbuf;
    BUF_P outbuf;
};

int tcp_send(TCPCLT_P, const char*, int);

#ifdef __cplusplus
}
#endif

#endif //EVNET_TCPCV_H