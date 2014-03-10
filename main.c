#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evbase/evt.h>
#include <evbase/util.h>

struct evt_io e, e2;
struct evt_before eb1, eb2;
struct evt_after ea1, ea2;

void io_cb(EL_P loop, struct evt_io* e) {
    log_trace("in1...");
    char buf[1024];
    read(STDIN_FILENO, buf, 1024);
    log_trace("io_cb1");
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
int main (int argc, char *argv[]) {

    //testdef();
    //printf("%s\n", DEFINE_TEST(LOG_LEVEL_T(LOG_INNER)));


    int i;
    EL_P loop = evt_loop_init();

    evt_io_init(&e, STDIN_FILENO, EVT_READ, io_cb);
    evt_io_start(loop, &e);

    evt_before_init(&eb1, before_cb1);
    evt_before_init(&eb2, before_cb2);
    evt_before_start(loop, &eb1);
    evt_before_start(loop, &eb2);

    evt_after_init(&ea1, after_cb1);
    evt_after_init(&ea2, after_cb2);
    evt_after_start(loop, &ea1);
    evt_after_start(loop, &ea2);

    evt_loop_run(loop);
	return 0;
}