#include <stdio.h>
#include <string.h>

#include <evbase/log.h>
#include <evbase/evt.h>
#include <evbase/util.h>

void io_cb(EL_P loop, struct evt_io* e) {
    log_trace("io_cb");
}

void testdef(){
    printf("%s\n", DEFINE_TEST(check_and_expand_array(loop->fds, struct fd_info, 
        loop->fds_size, w->fd + 1, multi_two, init_array_zero)));

}
int main (int argc, char *argv[]) {

    testdef();
    //printf("%s\n", DEFINE_TEST(LOG_LEVEL_T(LOG_INNER)));
    struct evt_io e;
    int i;
    EL_P loop = evt_loop_init();

for (i = 5; i < 1000; i++) {
    evt_io_init(&e, i, EVT_READ, io_cb);
    evt_io_start(loop, &e);
}
 //   evt_loop_run(loop);
	return 0;
}