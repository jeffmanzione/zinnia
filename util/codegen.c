#include "util/codegen.h"

#include "alloc/alloc.h"
#include "util/string_util.h"

// This *should* keep the individual concatenated liberals < 509 characters
// which is the max length of string literal in ANSI.
#define LOOSE_LITERAL_PRECAT_UPPER_BOUND 100
#define LOOSE_LITERAL_UPPER_BOUND 480

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

void print_string_as_var(const char var_name[], const char content[],
                         FILE *target) {
  const char *escaped_content = escape(content);
  const escaped_input_len = strlen(escaped_content);
  fprintf(target, "const char *%s[] = {", var_name);
  int start = 0, end = LOOSE_LITERAL_PRECAT_UPPER_BOUND;
  int current_literal_len = 0;
  for (; end <= escaped_input_len; end += LOOSE_LITERAL_PRECAT_UPPER_BOUND) {
    while (escaped_content[end] != ' ' && end < escaped_input_len) {
      ++end;
    }
    int len = min(end - start, escaped_input_len - start);
    current_literal_len += len;
    fprintf(target, "\n  \"%.*s\"", len, escaped_content + start);
    start = end;

    if (current_literal_len > LOOSE_LITERAL_UPPER_BOUND) {
      fprintf(target, ",");
      current_literal_len = 0;
    }
  }
  if (start < escaped_input_len) {
    // Means that a comma was just added.
    fprintf(target, "\n  \"%.*s\"", escaped_input_len - start,
            escaped_content + start);
  }
  fprintf(target, "};\n\n");
  RELEASE(escaped_content);
}
