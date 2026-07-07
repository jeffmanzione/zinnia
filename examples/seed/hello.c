#include "examples/seed/hello.h"

#include "zinnia/entity/native/builder/function_context.h"

#define GREET_PREFIX "Hello, "
#define GREET_SUFFIX "!\n"

static void greet_(NativeFunctionContext *fn_ctx) {
  char *str;
  int str_len;
  if (!extract_string(NativeFunctionContext_args(fn_ctx), &str, &str_len)) {
    NativeFunctionContext_raise_error(fn_ctx, "Argument must be a string.");
    return;
  }

  char *buf =
      (char *)malloc(strlen(GREET_PREFIX) + str_len + strlen(GREET_SUFFIX) + 1);
  buf[0] = 0x0;

  sprintf(buf, GREET_PREFIX "%.*s" GREET_SUFFIX, str_len, str);

  NativeFunctionContext_set_retval_obj(
      fn_ctx, NativeFunctionContext_create_string(fn_ctx, buf, strlen(buf)));

  free(buf);
}

void init_hello(NativeModuleBuilder *builder) {
  NativeModuleBuilder_verify_signature(builder, init_hello);
  NativeModuleBuilder_add_function(builder, "__greet", greet_);
}