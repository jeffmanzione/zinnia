#include "util/codegen.h"

#include "alloc/alloc.h"
#include "util/string_util.h"

// See
// https://learn.microsoft.com/en-us/cpp/c-language/maximum-string-length?view=msvc-170
#define DEFAULT_MAX_LITERAL_TOTAL_LENGTH 65535
#define DEFAULT_MAX_LITERAL_PRECAT_LENGTH 100

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

void print_string_as_var_default(const char var_name[], const char content[],
                                 FILE *target) {
  print_string_as_var(var_name, content, DEFAULT_MAX_LITERAL_TOTAL_LENGTH,
                      DEFAULT_MAX_LITERAL_PRECAT_LENGTH,
                      /*avoid_splitting_tokens*/ true, target);
}

void print_string_as_var(const char var_name[], const char content[],
                         int max_literal_total_length,
                         int max_literal_precat_length,
                         bool avoid_splitting_tokens, FILE *target) {
  const char *escaped_content = escape(content);
  const escaped_input_len = strlen(escaped_content);
  fprintf(target, "const char *%s[] = {", var_name);
  int start = 0, end = max_literal_precat_length;
  int current_literal_len = 0;
  for (; end <= escaped_input_len; end += max_literal_precat_length) {
    // Ensure that the closing quote is not inadvertently escaped.
    while (escaped_content[end - 1] == '\\') {
      --end;
    }
    if (avoid_splitting_tokens) {
      while (escaped_content[end] != ' ') {
        --end;
      }
    }
    int len = min(end - start, escaped_input_len - start);
    // Each precat literal counts an extra character toward to string literal
    // length total.
    current_literal_len += len + 1;
    fprintf(target, "\n  \"%.*s\"", len, escaped_content + start);
    start = end;

    if (current_literal_len > max_literal_total_length) {
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
