#include <string.h>

#include <evnet/udpsv.h>
#include <evbase/log.h>

static void udp_server_read(EL_P, struct evt_io*);

UDPSRV_P udp_server_init(int port, int flag) {
    SA addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return udp_server_init_v2(&addr, flag);
}

UDPSRV_P udp_server_init_v1(const char* s_addr, int port, int flag) {
    SA addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return udp_server_init_v2(&addr, flag);
}

UDPSRV_P udp_server_init_v2(SA* addr, int flag) {
    int listen_fd = -1;
    UDPSRV_P server = NULL;

    listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_fd < 0) {
        log_error("udp_server socket error!");
        goto udp_server_init_fail;
    }
    /* socket option */
    if ((flag & UDP_SERVER_NOREUSE) == 0) {
        fd_reuse(listen_fd);
    }
    if ((flag & UDP_SERVER_BLOCK) == 0) {
        fd_nonblock(listen_fd);
    }

    if (bind(listen_fd, (struct sockaddr*)addr, sizeof(SA)) < 0) {
        log_error("udp_server_bind error!");
        goto udp_server_init_fail;
    }

    server = (UDPSRV_P)mm_malloc(sizeof(struct udp_srv));
    memset(server, 0, sizeof(struct udp_srv));
    strcpy(server->name, "unknown");
    server->flag = flag;

    server->fd = listen_fd;
    memcpy(&server->addr, addr, sizeof(SA));

    server->loop_on = NULL;

    server->srv_io = (struct evt_io*)mm_malloc(sizeof(struct evt_io));
    evt_io_init(server->srv_io, udp_server_read, listen_fd, EVT_READ);
    evt_set_data(server->srv_io, server);

    server->on_read = NULL;

    log_inner("udp server(%d) init success, bind port %d",
        listen_fd, ntohs(addr->sin_port));
    return server;

udp_server_init_fail:
    if (listen_fd != -1) {
        close(listen_fd);
    }
    if (server != NULL) {
        mm_free(server->srv_io);
    }
    mm_free(server);
    return NULL;
}

UDPSRV_P udp_server_free(UDPSRV_P server) {
    if (server->fd != -1) {
        close(server->fd);
    }
    if (server != NULL) {
        if (server->loop_on && server->srv_io) {
            evt_io_stop(server->loop_on, server->srv_io);
        }
        mm_free(server->srv_io);
    }
    mm_free(server);
}

int udp_server_bind_loop(UDPSRV_P server, EL_P loop) {
    if (server == NULL || loop == NULL ) {
        log_error("NULL Pointer in udp server bind loop");
        return -1;
    }
    server->loop_on = loop;
    /* io event start here */
    /* only start in loop_on's thread */
    if (server->loop_on->owner_thread == 0
        || server->loop_on->owner_thread == thread_id()) {
        evt_io_start(server->loop_on, server->srv_io);
    } else {
        struct event_param ep;
        ep.type = EVENT_PARAM_EIOST;
        ep.temp = 0;
        ep.arg = server->srv_io;
        evt_loop_asyncq_append(server->loop_on, &ep);
        wake_up_loop(server->loop_on);
    }
}

/* not different for udp use loop or pool */
int udp_server_bind_pool(UDPSRV_P server, EP_P pool) {
    return udp_server_bind_loop(server, pool->get_next_loop(pool));
}

static void udp_server_read(EL_P loop, struct evt_io* ev) {
    UDPSRV_P server = (UDPSRV_P)ev->data;
    SA addr;
    socklen_t socklen = sizeof(SA);
    int len;
    char buf[UDP_BUFSZIE];
    len = recvfrom(server->fd, buf, UDP_BUFSZIE, 0,
        (struct sockaddr*)&addr, &socklen);
    /* Don't handle error this version*/
    if (len <= 0) {
        log_error("udp server(%d) recv error", server->fd);
        return;
    }
    if (server->on_read) {
        server->on_read(server, &addr, buf, len);
    }
}

void udp_send(UDPSRV_P server, SA_P addr, const char* cont, int len) {
    int s_len = sendto(server->fd, cont, len, 0,
        (struct sockaddr*)addr, sizeof(SA));
    if (len <= 0) {
        log_error("udp server(%d) send error", server->addr);
    }
}
