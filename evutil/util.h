#ifndef EVUTIL_UTIL_H
#define EVUTIL_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>

#include <evutil/log.h>

/* debug */
#define STRING(x) #x
#define DEFINE_TEST(x) STRING(x)


/* memory function */
static inline void *mm_realloc(void *p, int size) {
    p = realloc(p, size);
    if (p == NULL && size) {
        log_fatal("memory alloc failed");
    }
    return p;
}
#define mm_malloc(size) mm_realloc(0, (size))
#define mm_free(p) free(p)


#ifdef __cplusplus
}
#endif
#endif  //EVUTIL_UTIL_H