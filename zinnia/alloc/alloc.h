#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ALLOC_ALLOC_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ALLOC_ALLOC_H_

#include <stdlib.h>
#include <string.h>

#define MNEW(type) malloc(sizeof(type))
#define MNEW_ARR(type, num) malloc(sizeof(type) * num)

#define CNEW(type) calloc(1, sizeof(type))
#define CNEW_ARR(type, num) calloc(num, sizeof(type))

#define REALLOC(ptr, type, num) realloc(ptr, sizeof(type) * num)
#define RELEASE(ptr) free((void *)ptr)

#define ALLOC_STRNDUP(str, len) strndup(str, len)

#ifndef STRNDUP_AVAILABLE
char *strndup(const char *s, size_t n);
#endif

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ALLOC_ALLOC_H_ */