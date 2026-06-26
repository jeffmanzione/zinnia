#include "zinnia/util/void_array.h"

IMPL_ARRAYLIKE(VoidPtrArray, void *);
IMPL_ARRAYLIKE(CharPtrArray, char *);
IMPL_ARRAYLIKE(IntArray, int);
IMPL_SETLIKE(CharPtrSet, char *);

uint32_t hash_interned_string(const char *str, uint32_t size) {
  return (uint32_t)(intptr_t)str;
}

int32_t compare_interned_strings(const char *str1, uint32_t size1,
                                 const char *str2, uint32_t size2) {
  return (intptr_t)str1 - (intptr_t)str2;
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
#define FNV_32_PRIME_ (0x01000193)
#define FNV_1A_32_OFFSET_ (0x811C9DC5)

// Is true if any bytes in the provided 32-bit integer are equivalent to 0x00.
#define HAS_NULL(x)                                      \
  (((x & 0x000000FF) == 0) || ((x & 0x0000FF00) == 0) || \
   ((x & 0x00FF0000) == 0) || ((x & 0xFF000000) == 0))

uint32_t hash_string(const char *ptr, uint32_t size) {
  unsigned char *s = (unsigned char *)ptr;
  uint32_t hval = FNV_1A_32_OFFSET_;
  for (uint32_t i = 0; i < size; ++i) {
    hval *= FNV_32_PRIME_;
    hval ^= (uint32_t)*s++;
  }
  return hval;
}

int32_t compare_strings(const char *ptr1, uint32_t size1, const char *ptr2,
                        uint32_t size2) {
  if (ptr1 == ptr2) {
    return 0;
  }
  if (NULL == ptr1) {
    return -1;
  }
  if (NULL == ptr2) {
    return 1;
  }
  return memcmp(ptr1, ptr2, size1 > size2 ? size1 : size2);
}

uint32_t hash_int(const int i, uint32_t size) { return i; }
int32_t compare_ints(const int i1, uint32_t size1, const int i2,
                     uint32_t size2) {
  return i1 - i2;
}
