#ifndef EVBASE_UTIL_H
#define EVBASE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <evbase/log.h>

/* debug */
#ifndef NDEBUG
#define STRING(x) #x
#define DEFINE_TEST(x) STRING(x)
#else
#define STRING(x)
#define DEFINE_TEST(x)
#endif

/* memory function */
static inline void *mm_realloc(void *p, int size) {
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
#define init_array_zero(array, type, size_) memset(array, 0, sizeof(type)*(size_))
#define init_array_noop(array, type, size_)
#define check_and_expand_array(array, type, size, need, expand, init) do {   \
    if ((need) > (size)) {                                                   \
        int osize_ = (size);                                                 \
        while ((size) < (need))                                              \
            (size) = expand(size);                                           \
        (array) = (type*)mm_realloc(array, (size)*sizeof(type));             \
        init((array)+(osize_), type, (size)-(osize_));                       \
    }                                                                        \
} while (0)

/* compare */
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))
#define adjust_between(x, left, right) do {     \
    if ((x) < (left))  (x) = (left);            \
    if ((x) > (right)) (x) = (right);           \
} while (0)

/* bit operation */
#define RSHIFT(x) ((x) >> 1)
#define LSHIFT(x) ((x) << 1)
#define LCHILD(x) LSHIFT(x)
#define RCHILD(x) (LSHIFT(x)|1)


/* fd operation */
#define fd_cloexec(fd)
#define fd_nonblock(fd)

/* min heap (root=1)*/
#define HEAP_FIX_UP(heap, type, pos, size, comp) do {      \
    int pos_ = (pos);                                      \
    type val_ =  (heap)[pos_];                             \
    while (pos_ > 1 && comp(val_, (heap)[RSHIFT(pos_)])) { \
        (heap)[pos_] = (heap)[RSHIFT(pos_)];               \
        pos_ = RSHIFT(pos_);                               \
    }                                                      \
    (heap)[pos_] = val_;                                   \
} while (0)

#define HEAP_FIX_DOWN(heap, type, pos, size, comp) do {    \
    int pos_ = (pos);                                      \
    type val_ = (heap)[pos_];                              \
    while (LCHILD(pos_) <= (size)) {                       \
        int npos_ = LCHILD(pos_);                          \
        npos_ += (RCHILD(pos_) <= (size) &&                \
            comp((heap)[npos_+1], (heap)[npos_]) ? 1 : 0); \
        if (comp(val_, (heap)[npos_]))                     \
            break;                                         \
        (heap)[pos_] = (heap)[npos_];                      \
        pos_ = npos_;                                      \
    }                                                      \
    (heap)[pos_] = val_;                                   \
} while (0)

#define HEAP_INIT(heap, type, size, comp) do {             \
    int ind_ = (size) / 2 + 1;                             \
    while (--ind_)                                         \
        HEAP_FIX_DOWN(heap, type, ind_, size, comp);       \
} while (0)

#define HEAP_SORT(heap, type, size, comp) do {             \
    int size_ = (size);                                    \
    while (size_ > 0) {                                    \
        type tmp_ = (heap)[1];                             \
        (heap)[1] = (heap[size_]);                         \
        (heap)[size_--] = tmp_;                            \
        HEAP_FIX_DOWN(heap, type, 1, size_, comp);         \
    }                                                      \
} while (0)

#define HEAP_DELETE(heap, type, pos, size, comp) do {      \
    (heap)[pos] = (heap)[(size)--];                        \
    if (pos > 1 && comp((heap)[pos], (heap)[RSHIFT(pos)])){\
        HEAP_FIX_UP(heap, type, pos, size, comp);          \
    } else {                                               \
        HEAP_FIX_DOWN(heap, type, pos, size, comp);        \
    }                                                      \
} while (0)

#define HEAP_PUSH(heap, type, size, elm, comp) do {        \
    (heap)[++(size)] = (elm);                              \
    HEAP_FIX_UP(heap, type, size, size, comp);             \
} while (0)

#define HEAP_POP(heap, type, size, comp)                   \
    HEAP_DELETE(heap, type, 1, size, comp)


/* time */
#define MICOR_SECOND 1000000
extern __thread int64_t cached_time;
#define CACHED_TIME cached_time
int64_t get_cached_time();
int64_t update_cached_time();
void now_to_string(char*, int);
void time_to_string(int64_t, char*,int);


#ifdef __cplusplus
}
#endif
#endif  //EVBASE_UTIL_H