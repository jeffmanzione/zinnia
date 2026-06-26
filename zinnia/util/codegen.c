#include "zinnia/util/codegen.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/string_util.h"

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

void print_literal_(char *text, int num_chars, FILE *target) {
  fprintf(target, "\n  \"%.*s\"", num_chars, text);
}

void print_string_as_var(const char var_name[], const char content[],
                         int max_literal_total_length,
                         int max_literal_precat_length,
                         bool avoid_splitting_tokens, FILE *target) {
  char *escaped_content;
  const int escaped_input_len = escape(content, &escaped_content);
  fprintf(target, "const char *%s[] = {", var_name);
  int start = 0, end = max_literal_precat_length;
  int current_literal_len = 0;
  for (; end <= escaped_input_len; end += max_literal_precat_length) {
    // Ensure that the closing quote is not inadvertently escaped.
    while (escaped_content[end - 1] == '\\') {
      --end;
    }
    if (avoid_splitting_tokens) {
      int new_end = end;
      while (escaped_content[new_end] != ' ') {
        --new_end;
      }
      // Only backtrack if the resulting literal will be non-empty.
      if (new_end > start) {
        end = new_end;
      }
    }
    int len = min(end - start, escaped_input_len - start);
    // Each precat literal counts an extra character toward to string literal
    // length total.
    current_literal_len += len + 1;
    print_literal_(escaped_content + start, len, target);
    start = end;

    if (current_literal_len > max_literal_total_length) {
      fprintf(target, ",");
      current_literal_len = 0;
    }
  }
  if (start < escaped_input_len) {
    print_literal_(escaped_content + start, escaped_input_len - start, target);
  }
  fprintf(target, "};\n\n");
  RELEASE(escaped_content);
}

void print_data_as_char_array_var(const char var_name[], const char content[],
                                  uint32_t content_size, FILE *target) {
  fprintf(target, "const unsigned char %s[] = {\n  ", var_name);
  for (int i = 0; i < content_size; ++i) {
    fprintf(target, "0x%02x, ", content[i]);
  }
  fprintf(target, "};\n\n");
}