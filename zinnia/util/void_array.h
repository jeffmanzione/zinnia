#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_VOID_ARRAY_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_VOID_ARRAY_H_

#include "c-data-structures/arraylike.h"
#include "c-data-structures/setlike.h"

DEFINE_ARRAYLIKE(VoidPtrArray, void *);
DEFINE_ARRAYLIKE(CharPtrArray, char *);
DEFINE_ARRAYLIKE(IntArray, int);

uint32_t hash_interned_string(const char *str, uint32_t size);
int32_t compare_interned_strings(const char *str1, uint32_t size1,
                                 const char *str2, uint32_t size2);

uint32_t hash_string(const char *ptr, uint32_t size);
int32_t compare_strings(const char *ptr1, uint32_t size1, const char *ptr2,
                        uint32_t size2);

uint32_t hash_int(const int i, uint32_t size);
int32_t compare_ints(const int i1, uint32_t size1, const int i2,
                     uint32_t size2);

DEFINE_SETLIKE(CharPtrSet, char *);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_VOID_ARRAY_H_ */