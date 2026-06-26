#include "zinnia/util/error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void errorf__(int line_num, const char func_name[], const char file_name[],
              const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "Error in file(%s) in function(%s) at line(%d): ", file_name,
          func_name, line_num);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  fflush(stderr);
  va_end(args);
  exit(1);
}

void debugf__(int line_num, const char func_name[], const char file_name[],
              const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stdout, "[D] %s:%d(%s): ", file_name, line_num, func_name);
  vfprintf(stdout, fmt, args);
  fprintf(stdout, "\n");
  fflush(stdout);
  va_end(args);
}