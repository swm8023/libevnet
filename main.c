#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evbase/evt.h>
#include <evbase/util.h>
#include <evnet/tcpsv.h>
#include <evnet/udpsv.h>


atomic64 cal_tt = 0;
int sec = 10;

void on_accept_comp(TCPCLT_P client) {
    //log_debug("Accept a connect fd:%d", client->fd);
}
void on_read_comp(TCPCLT_P client, BUF_P buf, int len) {
    atomic_add_get(cal_tt, BUFF_USED(buf));
    tcp_buffer_send(client, buf);
}
void on_write_comp(TCPCLT_P client, BUF_P buf , int len) {

}
void on_udp_read(UDPSRV_P server, SA_P addr, const char* cont, int len) {
    udp_send(server, addr, cont, len);
}

void timer_cb(EL_P loop, struct evt_timer* ev) {
    TCPSRV_P server = (TCPSRV_P)ev->data;

    log_warn("ttl => %lfMb/s client => %d", atomic_get(cal_tt)/1024.0/1024.0/sec,
    atomic_get(server->cltcnt));
}
int main (int argc, char *argv[]) {
    ignore_sigpipe();

    set_default_logif_level(LOG_WARN);

    EP_P pool = evt_pool_init(4);
    TCPSRV_P server  = tcp_server_init(8887, TCP_DEFAULT_FLAG);
    //tcp_set_accept_comp_cb(server, on_accept_comp);
    tcp_set_read_comp_cb(server, on_read_comp);
    //tcp_set_write_comp_cb(server, on_write_comp);
    //udp_set_read_cb(userver, on_udp_read);

    tcp_server_bind_pool(server, pool);
    //udp_server_bind_pool(userver, pool);

    struct evt_timer evtm;
    evt_timer_init(&evtm, timer_cb, SECOND(10), SECOND(10));
    evt_set_data(&evtm, server);
    evt_timer_start(pool->get_next_loop(pool), &evtm);

    evt_pool_run(pool);


    /*
    EL_P loop = evt_loop_init();
    TCPSRV_P server  = tcp_server_init(8887, TCP_DEFAULT_FLAG);
    UDPSRV_P userver = udp_server_init(8887, UDP_DEFAULT_FLAG);
    tcp_set_accept_comp_cb(server, on_accept_comp);
    tcp_set_read_comp_cb(server, on_read_comp);
    tcp_set_write_comp_cb(server, on_write_comp);

    udp_set_read_cb(userver, on_udp_read);

    tcp_server_bind_loop(server, loop);
    udp_server_bind_loop(userver, loop);
    evt_loop_run(loop);
    */

	return 0;
}