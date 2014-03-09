#ifndef EVBASE_UTIL_H
#define EVBASE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <evbase/log.h>

/* debug */
#define STRING(x) #x
#define DEFINE_TEST(x) STRING(x)


/* memory function */
static inline void *mm_realloc(void *p, int size) {
    printf("==%d\n", size);
    p = realloc(p, size);
    if (p == NULL && size) {
        log_fatal("memory allocate failed");
    }
    return p;
}
#define mm_malloc(size) mm_realloc(0, (size))
#define mm_free(p) free(p)

/* array operation */
#define multi_two(x) ((x) << 1)
#define add_one(x)   ((x) + 1)
#define init_array_zero(array, type, size) \
    memset((array), 0, sizeof(type)*(size))
#define check_and_expand_array(array, type, size, need, expand, init) do {   \
    if ((need) > (size)) {                                                   \
        int osize_ = (size);                                                 \
        while ((size) < (need))                                              \
            (size) = expand(size);                                           \
        (array) = (type*)mm_realloc((array), (size)*sizeof(type));           \
        init_array_zero((array)+osize_, (type), ((size)-osize_));            \
    }                                                                        \
} while (0)

/* fd operation */
#define fd_cloexec(fd)
#define fd_nonblock(fd)

#ifdef __cplusplus
}
#endif
#endif  //EVBASE_UTIL_H