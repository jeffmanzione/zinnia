#include "examples/seed/hello.h"

#define GREET_PREFIX "Hello, "
#define GREET_SUFFIX "!\n"

static void greet_(FunctionContext *fn_ctx) {
  char *str;
  int str_len;
  if (!extract_string(FunctionContext_args(fn_ctx), &str, &str_len)) {
    FunctionContext_raise_error(fn_ctx, "Argument must be a string.");
    return;
  }

  const int buf_size =
      strlen(GREET_PREFIX) + str_len + strlen(GREET_SUFFIX) + 1;
  char *buf = (char *)malloc(buf_size);

  snprintf(buf, buf_size, GREET_PREFIX "%.*s" GREET_SUFFIX, str_len, str);

  FunctionContext_set_retval_obj(
      fn_ctx, FunctionContext_create_string(fn_ctx, buf, strlen(buf)));

  free(buf);
}

void init_hello(ModuleBuilder *builder) {
  ModuleBuilder_verify_signature(builder, init_hello);
  ModuleBuilder_add_function(builder, "__greet", greet_);
}