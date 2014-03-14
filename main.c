#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evbase/evt.h>
#include <evbase/util.h>
#include <evnet/tcpsv.h>
#include <evnet/udpsv.h>

struct evt_io e, e2;
struct evt_before eb1, eb2;
struct evt_after ea1, ea2;
struct evt_timer et1, et2, et3;

void on_accept_comp(TCPCLT_P client) {
    //log_debug("Accept a connect fd:%d", client->fd);
}
void on_read_comp(TCPCLT_P client, BUF_P buf, int len) {
    char str[1000];
    int lens = buff_readall(buf, str, 1000);
    tcp_send(client, str, lens);
}
void on_write_comp(TCPCLT_P client, BUF_P buf , int len) {

}
void on_udp_read(UDPSRV_P server, SA_P addr, const char* cont, int len) {
    udp_send(server, addr, cont, len);
}

int main (int argc, char *argv[]) {
    ignore_sigpipe();

    EP_P pool = evt_pool_init(4);
    TCPSRV_P server  = tcp_server_init(8887, TCP_DEFAULT_FLAG);
    UDPSRV_P userver = udp_server_init(8887, UDP_DEFAULT_FLAG);
    tcp_set_accept_comp_cb(server, on_accept_comp);
    tcp_set_read_comp_cb(server, on_read_comp);
    tcp_set_write_comp_cb(server, on_write_comp);

    udp_set_read_cb(userver, on_udp_read);

    tcp_server_bind_pool(server, pool);
    udp_server_bind_pool(userver, pool);
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