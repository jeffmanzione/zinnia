// process.c
//
// Created on: Dec 28, 2020
//     Author: Jeff Manzione

#include "entity/native/process.h"

#include "entity/class/class.h"
#include "entity/class/classes.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "util/sync/thread.h"
#include "vm/intern.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/virtual_machine.h"

void _remote_init(Object *obj) {}
void _remote_delete(Object *obj) {}

Entity _create_process(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (IS_CLASS(args, Class_Tuple)) {
    Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  } else if (IS_OBJECT(args) && inherits_from(obj->_class, Class_Function)) {

  } else {
    ERROR("Cannot call create_process with something other than function.");
  }

  Process *p = vm_create_process(task->parent_process->vm);
  Task *t = process_create_task(p);

  ThreadHandle thread = process_run_in_new_thread(p);
  return entity_object(p->_reflection);
}

void process_add_native(Module *process) {
  Class_Remote =
      native_class(process, REMOTE_CLASS_NAME, _remote_init, _remote_delete);
  native_function(process, intern("create_process"), _create_process);
}
