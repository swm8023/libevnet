#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <evbase/util.h>
#include <evbase/evt.h>
#include <evbase/log.h>
#include <evnet/tcpsv.h>


static void tcp_server_accpet(EL_P, struct evt_io*);
static void tcp_client_read(EL_P, struct evt_io*);
static void tcp_client_write(EL_P, struct evt_io*);

/* init tcp server with port number */
TCPSRV_P tcp_server_init(int port, int flag) {
    SA addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return tcp_server_init_v2(&addr, flag);
}

/* init tcp server with addr in string format and port number */
TCPSRV_P tcp_server_init_v1(const char* addr_str, int port, int flag) {
    SA addr;
    addr.sin_family = AF_INET;
    if (1 != inet_pton(AF_INET, addr_str, &addr.sin_addr)) {
        log_error("inet_pton error, can't init tcp server");
        return NULL;
    }
    addr.sin_port = htons(port);
    return tcp_server_init_v2(&addr, flag);
}

/* init tcp server with a struct of SA */
TCPSRV_P tcp_server_init_v2(SA *addr, int flag) {
    int listen_fd = -1;
    TCPSRV_P server = NULL;

    /* socket -> bind -> listen */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        log_error("tcp_server socket error!");
        goto tcp_server_init_fail;
    }
    /* socket option */
    if ((flag & TCP_SERVER_NOREUSE) == 0) {
        fd_reuse(listen_fd);
    }
    if ((flag & TCP_SERVER_ACBLOCK) == 0) {
        fd_nonblock(listen_fd);
    }

    if (bind(listen_fd, (struct sockaddr*)addr, sizeof(SA)) < 0) {
        log_error("tcp_server bind error!");
        goto tcp_server_init_fail;
    }
    if (listen(listen_fd, 1024) < 0) {
        log_error("tcp_server listen error!");
        goto tcp_server_init_fail;
    }

    /* new server and init */
    server = (TCPSRV_P)mm_malloc(sizeof(struct tcp_srv));
    memset(server, 0, sizeof(struct tcp_srv));
    strcpy(server->name, "unknown");
    server->flag = flag;

    server->fd = listen_fd;
    memcpy(&server->addr, addr, sizeof(SA));
    server->cltcnt = 0;

    server->loop_list = NULL;
    server->loop_on   = NULL;
    server->loops     = 0;

    /* init io event of accept, start after bind ev_loop */
    server->srv_io = (struct evt_io*)mm_malloc(sizeof(struct evt_io));
    evt_io_init(server->srv_io, tcp_server_accpet, listen_fd, EVT_READ);
    evt_set_data(server->srv_io, server);

    server->on_accept_comp = NULL;
    server->on_read_comp   = NULL;
    server->on_write_comp  = NULL;
    server->get_next_loop  = NULL;

    log_inner("tcp server(%d) init success, listen at %d",
        listen_fd, ntohs(addr->sin_port));
    return server;

    /* failed */
tcp_server_init_fail:
    if (listen_fd != -1) {
        close(listen_fd);
    }
    if (server != NULL) {
        mm_free(server->srv_io);
    }
    mm_free(server);
    return NULL;
}

void tcp_server_free(TCPSRV_P server) {
    if (server->fd != -1) {
        close(server->fd);
    }
    if (server != NULL) {
        if (server->loop_on) {
            evt_io_stop(server->loop_on, server->srv_io);
        }
        mm_free(server->srv_io);
    }
    mm_free(server);
}

int tcp_server_bind_loop(TCPSRV_P server, EL_P loop) {
    if (server == NULL || loop == NULL ) {
        log_error("NULL Pointer in tcp server bind loop");
        return -1;
    }
    server->flag &= ~TCP_SERVER_LOOPS;
    server->loop_list = NULL;
    server->loops = 1;
    server->loop_on = loop;
    /* io event start here */
    evt_io_start(loop, server->srv_io);

}

static void tcp_server_accpet(EL_P loop, struct evt_io* ev) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(SA);
    int client_fd = -1;
    TCPCLT_P client = NULL;
    TCPSRV_P server = (TCPSRV_P)ev->data;

    /* accept new connection */
    client_fd = accept(server->fd, (struct sockaddr*)&addr, &len);
    if (client_fd < 0) {
        log_warn("accept new client failed");
        /* fd not enough. how to do ? */
        if (errno == EMFILE) {

        }
        return;
    } else {
        log_inner("accept new client(%d)", client_fd);
    }
    /* socket option */
    fd_nonblock(fd);

    /* new client and init */
    client = (TCPCLT_P)mm_malloc(sizeof(struct tcp_clt));
    memset(client, 0, sizeof(struct tcp_clt));

    client->flag = TCP_DEFAULT_FLAG;
    client->fd = client_fd;
    client->relate_srv = server;
    memcpy(&client->addr, &addr, sizeof(SA));

    if ((server->flag & TCP_SERVER_LOOPS) && server->get_next_loop) {
        client->loop_on = server->get_next_loop(server);
    } else {
        client->loop_on = server->loop_on;
    }

    /* client's read(0) and write(1) event , only start read */
    client->clt_io[0] = (struct evt_io*)mm_malloc(sizeof(struct evt_io));
    evt_io_init(client->clt_io[0], tcp_client_read, client_fd, EVT_READ);
    evt_set_data(client->clt_io[0], client);

    client->clt_io[1] = (struct evt_io*)mm_malloc(sizeof(struct evt_io));
    evt_io_init(client->clt_io[1], tcp_client_write, client_fd, EVT_WRITE);
    evt_set_data(client->clt_io[1], client);

    evt_io_start(client->loop_on, client->clt_io[0]);

    /* client io buffer */
    client->inbuf = buff_new();
    client->outbuf = buff_new();

    /* callback */
    if (server->on_accept_comp) {
        server->on_accept_comp(server, client);
    }
    return;
}

static void tcp_client_read(EL_P loop, struct evt_io* ev) {
    TCPCLT_P client = (TCPCLT_P)client->data;
    TCPSRV_P server = (TCPSRV_P)client->relate_srv;

    int len = buff_fd_read(client->inbuf, client->fd);

    /* read EOF */
    if (len == 0) {
        /* check if there is any data in send buffer, if so,
           set client flag to make it close after all data sent
        */
        log_trace("fd(%d) read EOF", client->fd);
        if (BUFFER_USED(client->outbuf) == 0) {
            
        }
    }
}

static void tcp_client_write(EL_P loop, struct evt_io* ev) {

}

static void tcp_client_close(TCPCLT_P client) {

}