#ifndef EVNET_UDPSV_H
#define EVNET_UDPSV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <evbase/evt.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct sockaddr_in SA;
typedef struct sockaddr_in* SA_P;

typedef struct udp_srv* UDPSRV_P;

#define UDP_BUFSZIE 8192

#define UDP_DEFAULT_FLAG   0x00
#define UDP_SERVER_NOREUSE 0x01
#define UDP_SERVER_BLOCK   0x02

struct udp_srv {
    char name[10];
    int flag;

    int fd;
    SA addr;

    EL_P loop_on;

    struct evt_io* srv_io;
    void (*on_read)(UDPSRV_P, SA_P, const char*, int);
};

#define udp_set_read_cb(server, cb) \
    (server)->on_read = (cb)

UDPSRV_P udp_server_init(int, int);
UDPSRV_P udp_server_init_v1(const char*, int, int);
UDPSRV_P udp_server_init_v2(SA*, int);

UDPSRV_P udp_server_free(UDPSRV_P);
int udp_server_bind_loop(UDPSRV_P, EL_P);
int udp_server_bind_pool(UDPSRV_P, EP_P);

void udp_send(UDPSRV_P, SA_P, const char*, int);


#ifdef __cplusplus
}
#endif

#endif //EVNET_UDPSV_H