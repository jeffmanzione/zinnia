
#include "zinnia/examples/static/pid.h"

#include "zinnia/entity/native/native.h"
#include "zinnia/util/platform.h"

#ifndef OS_WINDOWS
#include <unistd.h>
#endif

Entity _current_process_id(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  long pid;
#ifdef OS_WINDOWS
  pid = GetCurrentProcessId();
#else
  pid = getpid();
#endif
  return entity_int(pid);
}

void init_pid(ModuleManager *mm, Module *pid) {
  verify_init_fn_signature(init_pid);

  native_function(pid, mm->intern("__current_process_id"), _current_process_id);
}