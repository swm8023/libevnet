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
    int i;
    char buf[4096];
    char hello[50] = "Hello World";
    BUF_P buff = buff_new();

    buff_write(buff, hello, strlen(hello));
    printf("%d\n", BUFF_USED(buff));
    int len = buff_read(buff, 4, buf, 4096);
    buf[len] = 0;
    printf("%d %s \n", BUFF_USED(buff), buf);

/*
    EL_P loop = evt_loop_init();

    evt_io_init(&e, io_cb, STDIN_FILENO, EVT_READ);
    evt_io_start(loop, &e);

    evt_timer_init(&et1, timer_cb1, SECOND(2), 0);
    evt_timer_init(&et2, timer_cb2, SECOND(3), SECOND(3));
    evt_timer_init(&et3, timer_cb3, SECOND(8), SECOND(6));
    evt_timer_start(loop, &et1);
    evt_timer_start(loop, &et2);
    evt_timer_start(loop, &et3);
    evt_loop_run(loop);
*/
	return 0;
}