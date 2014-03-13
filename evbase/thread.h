#ifndef EVBASE_THREAD_H
#define EVBASE_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
    
/* current thread */
extern __thread int cr_thread_id;
extern __thread const char* cr_thread_name;
#define THREAD_ID cr_thread_id
int thread_id();
const char* thread_name();

/* atomic var */
typedef int32_t atomic32;
typedef int64_t atomic64;

#ifdef __cplusplus
}
#endif
#endif  //EVBASE_THREAD_H