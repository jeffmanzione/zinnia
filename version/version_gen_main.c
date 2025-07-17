// Writes a C implementation of VERSION_H_FILE functions based on the provided
// version and current timestamp.
//
// Usage: version_gen_bin "1.0.0" > version.c

#include <stdio.h>
#include <stdlib.h>

#include "util/time.h"

// Source: https://en.wikipedia.org/wiki/ISO_8601
#define MAX_ISO_TIMESTAMP_CHARS 27
#define MINUTES_PER_HOUR 60
#define MICROS_PER_MINUTE (1000 * 1000 * 60)

#define VERSION_H_FILE "version/version.h"
#define VERSION_FN_NAME "version_string"
#define TIMESTAMP_FN_NAME "version_timestamp_string"

#define FILE_STATEMENT_SEP "\n\n"

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

static const char *INCLUDE_HEADERS[] = {
    "util/time.h",
};

void print_version_h_include() { printf("#include \"%s\"", VERSION_H_FILE); }

void print_required_includes() {
  for (int i = 0; i < ARRAY_LENGTH(INCLUDE_HEADERS); ++i) {
    printf("#include \"%s\"\n", INCLUDE_HEADERS[i]);
  }
}

void print_version_fn(const char version[]) {
  printf("const char *%s() {\n  return \"%s\";\n}", VERSION_FN_NAME, version);
}

void print_timestamp_fn() {
  const Timestamp local_now = current_local_timestamp();
  const Timestamp gmt_now = current_gmt_timestamp();

  const int64_t tz_diff_mins =
      (timestamp_to_micros(&local_now) - timestamp_to_micros(&gmt_now)) /
      MICROS_PER_MINUTE;
  const TimezoneOffset tz_offset = create_timezone_offset(
      tz_diff_mins / MINUTES_PER_HOUR, tz_diff_mins % MINUTES_PER_HOUR);

  // Add character for nullterm
  char timestamp_str[MAX_ISO_TIMESTAMP_CHARS + 1];
  timestamp_to_iso8601(&local_now, &tz_offset, timestamp_str);
  printf("const char *%s() {\n  return \"%s\";\n}", TIMESTAMP_FN_NAME,
         timestamp_str);
}

int main(int argc, const char *args[]) {
  if (argc != 2) {
    fprintf(stderr, "Expected exactly 1 argument. Received %d", argc - 1);
    return EXIT_FAILURE;
  }

  print_version_h_include();
  printf(FILE_STATEMENT_SEP);
  print_required_includes();
  printf(FILE_STATEMENT_SEP);
  print_version_fn(args[1]);
  printf(FILE_STATEMENT_SEP);
  print_timestamp_fn();

  return EXIT_SUCCESS;
}