#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evbase/evt.h>
#include <evbase/util.h>
#include <evnet/tcpsv.h>

struct evt_io e, e2;
struct evt_before eb1, eb2;
struct evt_after ea1, ea2;
struct evt_timer et1, et2, et3;
void io_cb(EL_P loop, struct evt_io* e) {
    char buf[1024];
    read(STDIN_FILENO, buf, 1024);
    log_trace("io event happen....");
}
void before_cb1(EL_P loop, struct evt_before *e) {
    log_trace("evt before1");
}

void before_cb2(EL_P loop, struct evt_before *e) {
    log_trace("evt before2");
}
void after_cb1(EL_P loop, struct evt_after *e) {
    log_trace("evt after1");
}

void after_cb2(EL_P loop, struct evt_after *e) {
    log_trace("evt after2");
}

void testdef(){
    printf("%s\n", DEFINE_TEST(check_and_expand_array(loop->fds, struct fd_info, 
        loop->fds_size, w->fd + 1, multi_two, init_array_zero)));

}

void timer_cb1(EL_P loop, struct evt_timer *e) {
    log_trace("timer event1 happen...... %d", e->repeat);
}

void timer_cb2(EL_P loop, struct evt_timer *e) {
    log_trace("timer event2 happen...... %d", e->repeat);
}
void timer_cb3(EL_P loop, struct evt_timer *e) {
    log_trace("timer event3 happen...... %d", e->repeat);
    evt_timer_stop(loop, &et2);
}
int main (int argc, char *argv[]) {

    EL_P loop = evt_loop_init();
    TCPSRV_P server = tcp_server_init(8887, TCP_DEFAULT_FLAG);

    tcp_server_bind_loop(server, loop);

    evt_loop_run(loop);

	return 0;
}