#include <unistd.h>

#include <evutil/evt.h>
#include <evutil/util.h>


EL_P evt_loop_init() {
    EL_P loop = (EL_P)mm_malloc(sizeof(struct evt_loop));

    loop->owner_thread = 0;
    loop->status = LOOP_STATU_INIT;

    loop->priority_max = 0;

    loop->fds_size = LOOP_INIT_FDS;
    loop->fds_mod_size = LOOP_INIT_FDS;
    loop->fds = (FDI_P)mm_malloc(sizeof(struct fd_info) * LOOP_INIT_FDS);
    loop->fdchange = (int*)mm_malloc(sizeof(int) * LOOP_INIT_FDS);


}
int evt_loop_quit(EL_P loop) {

}
int evt_loop_destroy(EL_P loop) {

}
int evt_loop_run(EL_P loop) {

}