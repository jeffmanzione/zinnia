#include "zinnia/alloc/alloc.h"

#ifndef STRNDUP_AVAILABLE
char *strndup(const char *str, size_t chars) {
  char *buffer;
  int n;
  buffer = (char *)malloc(sizeof(char) * (chars + 1));
  if (buffer) {
    for (n = 0; ((n < chars) && (str[n] != 0)); n++) {
      buffer[n] = str[n];
    }
    buffer[n] = '\0';
  }
  return buffer;
}
#endif