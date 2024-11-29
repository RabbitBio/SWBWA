#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef USE_MALLOC_WRAPPERS
/* Don't wrap ourselves */
#  undef USE_MALLOC_WRAPPERS
#endif
#include "malloc_wrap.h"

extern double t_malloc;
extern double t_free;

void *wrap_calloc(size_t nmemb, size_t size,
				  const char *file, unsigned int line, const char *func) {
    double t0 = realtime();
	void *p = calloc(nmemb, size);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, nmemb * size, file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
    t_malloc += realtime() - t0;
	return p;
}

void *wrap_malloc(size_t size,
				  const char *file, unsigned int line, const char *func) {
    double t0 = realtime();
	void *p = malloc(size);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, size, file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
    t_malloc += realtime() - t0;
	return p;
}

void *wrap_realloc(void *ptr, size_t size,
				   const char *file, unsigned int line, const char *func) {
    double t0 = realtime();
	void *p = realloc(ptr, size);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, size, file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
    t_malloc += realtime() - t0;
	return p;
}

char *wrap_strdup(const char *s,
				  const char *file, unsigned int line, const char *func) {
    double t0 = realtime();
	char *p = strdup(s);
	if (NULL == p) {
		fprintf(stderr,
				"[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
				func, strlen(s), file, line, strerror(errno));
		exit(EXIT_FAILURE);
	}
    t_malloc += realtime() - t0;
	return p;
}


void wrap_free(void *ptr, const char *file, unsigned int line, const char *func) {
    double t0 = realtime();
	//if (ptr == NULL) {
	//	fprintf(stderr,
	//			"[%s] Attempted to free a NULL pointer at %s line %u: %s\n",
	//			func, file, line, strerror(errno));
	//	return;
	//}
	free(ptr);
    t_free += realtime() - t0;
}
