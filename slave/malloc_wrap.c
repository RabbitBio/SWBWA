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
	//void *p = calloc(nmemb, size);
    size_t total_size = nmemb * size;
    void *p = libc_aligned_malloc(total_size);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, nmemb * size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
	}
    memset(p, 0, total_size);
	return p;
}

void *wrap_malloc(size_t size,
				  const char *file, unsigned int line, const char *func) {
	void *p = libc_aligned_malloc(size);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, size, file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return p;
}

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
	char *p = strdup(s);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, strlen(s), file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return p;
}

void wrap_free(void *ptr, const char *file, unsigned int line, const char *func) {
    if (ptr != NULL) {
        libc_aligned_free(ptr);
        fprintf(stderr, "[%s] Freed memory at %s line %u\n", func, file, line);
    } else {
        fprintf(stderr, "[%s] Attempt to free NULL pointer at %s line %u\n", func, file, line);
    }
}
