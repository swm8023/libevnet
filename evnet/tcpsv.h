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

#define TCP_SERVER_BREATH 0x01
#define TCP_SERVER_POOL   0x02

/* tcp server */
struct tcp_srv {
    char name[10];
    uint8_t flag;

    int fd;
    SA addr;

    atomic32 cltcnt;

    EL_P loop_list;
    EL_P loop_on;
    int loops;

    void (*on_accept_comp)();
    void (*get_next_loop)();
};

/* init and destroy */
TCPSRV_P tcp_server_init(int);
TCPSRV_P tcp_server_init_v1(const char*addr, int);
TCPSRV_P tcp_server_init_v2(SA *addr);
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

    BUF_P inbuf;
    BUF_P outbuf;

    void (*on_read_comp)();
    void (*on_write_comp)();
};

#ifdef __cplusplus
}
#endif

#endif //EVNET_TCPCV_H