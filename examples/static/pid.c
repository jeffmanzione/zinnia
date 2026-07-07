
#include "examples/static/pid.h"

#ifndef OS_WINDOWS
#include <unistd.h>
#endif

#include "zinnia/entity/native/builder/function_context.h"

void current_process_id_(NativeFunctionContext *fn_ctx) {
  long pid;
#ifdef OS_WINDOWS
  pid = GetCurrentProcessId();
#else
  pid = getpid();
#endif
  *NativeFunctionContext_mutable_retval(fn_ctx) = entity_int(pid);
}

void init_pid(NativeModuleBuilder *builder) {
  NativeModuleBuilder_verify_signature(builder, init_pid);
  NativeModuleBuilder_add_function(builder, "__current_process_id",
                                   current_process_id_);
}