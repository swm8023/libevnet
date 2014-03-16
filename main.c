#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evnet/filelog.h>
#include <evbase/evt.h>
#include <evbase/util.h>
#include <evnet/tcpsv.h>
#include <evnet/udpsv.h>
#include <evbase/thread.h>
#include <evplug/http.h>


int main (int argc, char *argv[]) {
    ignore_sigpipe();
    set_default_logif_level(LOG_WARN);

    http_start(8887, 4);

/*
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
*/

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