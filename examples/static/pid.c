
#include "examples/static/pid.h"

#ifndef OS_WINDOWS
#include <unistd.h>
#endif

static void current_process_id_(FunctionContext *fn_ctx) {
  long pid;
#ifdef OS_WINDOWS
  pid = GetCurrentProcessId();
#else
  pid = getpid();
#endif
  *FunctionContext_mutable_retval(fn_ctx) = entity_int(pid);
}

void init_pid(ModuleBuilder *builder) {
  ModuleBuilder_verify_signature(builder, init_pid);
  ModuleBuilder_add_function(builder, "__current_process_id",
                             current_process_id_);
}