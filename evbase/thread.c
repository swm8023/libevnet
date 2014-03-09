#include <sys/syscall.h>

#include <evbase/thread.h>


//current thread
__thread int cr_thread_id = 0;
__thread const char* cr_thread_name = "unknow";

int thread_id() {
    if (cr_thread_id == 0) {
        cr_thread_id = syscall(SYS_gettid);
    }
    return cr_thread_id;
}
const char* thread_name() {
    return cr_thread_name;
}