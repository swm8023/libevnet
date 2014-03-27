#ifndef EVNET_TCPSV_H
#define EVNET_TCPSV_H

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

#define TCP_DEFAULT_FLAG   0x00

#define TCP_SERVER_BREATH  0x01
#define TCP_SERVER_LOOPS   0x02
#define TCP_SERVER_NOREUSE 0x04
#define TCP_SERVER_ACBLOCK 0x08

#define TCP_CLIENT_WAITCLS 0x01

//#define TCPCLT_P struct tcp_clt*
//#define TCPSRV_P struct tcp_srv*
typedef struct tcp_clt* TCPCLT_P;
typedef struct tcp_srv* TCPSRV_P;
/* tcp server */
struct tcp_srv {
    char name[10];
    int flag;

    int fd;
    SA addr;

    atomic32 cltcnt;

    EP_P pool_on;
    EL_P loop_on;
    int loops;

    struct evt_io* srv_io;
    void (*on_accept_comp)(TCPCLT_P);
    void (*on_read_comp)(TCPCLT_P, BUF_P, int);
    void (*on_write_comp)(TCPCLT_P, BUF_P, int);
    void (*on_client_close)(TCPCLT_P);
    EL_P (*get_next_loop)(EP_P);

    void *data;
};

#define tcp_set_accept_comp_cb(server, cb) \
    (server)->on_accept_comp = (cb)
#define tcp_set_read_comp_cb(server, cb) \
    (server)->on_read_comp = (cb)
#define tcp_set_write_comp_cb(server, cb) \
    (server)->on_write_comp = (cb)
#define tcp_set_client_close_cb(server, cb) \
    (server)->on_client_close = (cb)
#define tcp_set_next_loop_cb(server, cb) \
    (server)->get_next_loop = (cb)


/* init and destroy */
TCPSRV_P tcp_server_init(int, int);
TCPSRV_P tcp_server_init_v1(const char*addr, int, int);
TCPSRV_P tcp_server_init_v2(SA *addr, int);
void tcp_server_free(TCPSRV_P);

int tcp_server_bind_loop(TCPSRV_P, EL_P);
int tcp_server_bind_pool(TCPSRV_P, EP_P);


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
    void *data;
};

int tcp_set_srvdata(TCPSRV_P, void*);
int tcp_set_clidata(TCPCLT_P, void*);

int tcp_send(TCPCLT_P, const char*, int);
int tcp_delay_send(TCPCLT_P, const char*, int, int);
int tcp_flush(TCPCLT_P);
int tcp_buffer_send(TCPCLT_P, BUF_P);

#ifdef __cplusplus
}
#endif

#endif //EVNET_TCPCV_H