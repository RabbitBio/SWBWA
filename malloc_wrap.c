#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef USE_MALLOC_WRAPPERS
/* Don't wrap ourselves */
#  undef USE_MALLOC_WRAPPERS
#endif
#include "malloc_wrap.h"

void *wrap_calloc(size_t nmemb, size_t size,
                                  const char *file, unsigned int line, const char *func) {
    size_t total_size = nmemb * size;
    void *p = _sw_xmalloc(total_size);
    if (p) memset(p, 0, total_size);
        else {
                fprintf(stderr,
                                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                                func, nmemb * size, file, line, strerror(errno));
                exit(EXIT_FAILURE);
        }
        return p;
}

void *wrap_malloc(size_t size,
                                  const char *file, unsigned int line, const char *func) {
        void *p = _sw_xmalloc(size);
        if (NULL == p) {
                fprintf(stderr,
                                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                                func, size, file, line, strerror(errno));
                exit(EXIT_FAILURE);
        }
        return p;
}

//void *wrap_realloc(void *ptr, size_t size,
//                   const char *file, unsigned int line, const char *func) {
//    if (ptr == NULL) {
//        return _sw_xmalloc(size);
//    }
//    if (size == 0) {
//        _sw_xfree(ptr);
//        return NULL;
//    }
//
//    size_t old_size = size << 1; 
//    void *new_ptr = _sw_xmalloc(size);
//    if (!new_ptr) {
//        fprintf(stderr, "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
//                func, size, file, line, strerror(errno));
//        exit(EXIT_FAILURE);
//    }
//
//    size_t copy_size = old_size < size ? old_size : size;
//    memcpy(new_ptr, ptr, copy_size);
//
//    _sw_xfree(ptr);
//
//    return new_ptr;
//}

void *wrap_realloc(void *ptr, size_t size,
                                   const char *file, unsigned int line, const char *func) {
        void *p = realloc(ptr, size);
        if (NULL == p) {
                fprintf(stderr,
                                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                                func, size, file, line, strerror(errno));
                exit(EXIT_FAILURE);
        }
        return p;
}

char *wrap_strdup(const char *s,
                  const char *file, unsigned int line, const char *func) {
    size_t len = strlen(s) + 1;
    char *p = _sw_xmalloc(len);
    if (p) {
        memcpy(p, s, len);
    } else {
        fprintf(stderr, "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                func, len, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void wrap_free(void *ptr,
               const char *file, unsigned int line, const char *func) {
    _sw_xfree(ptr);
}
