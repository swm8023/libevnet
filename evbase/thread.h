#ifndef EVBASE_THREAD_H
#define EVBASE_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif



//current thread
extern __thread int cr_thread_id;
extern __thread const char* cr_thread_name;
int thread_id();
const char* thread_name();


#ifdef __cplusplus
}
#endif
#endif  //EVBASE_THREAD_H