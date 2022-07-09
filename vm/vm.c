// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/vm.h"

#include "entity/class/classes_def.h"
#include "vm/process/context.h"
#include "vm/process/process.h"

Process *create_process_no_reflection(VM *vm) {
  Process *process = alist_add(&vm->processes);
  process_init(process, &vm->base_heap_conf);
  process->vm = vm;
  return process;
}

void add_reflection_to_process(Process *process) {
  process->_reflection = heap_new(process->heap, Class_Process);
  process->_reflection->_internal_obj = process;
  heap_make_root(process->heap, process->_reflection);
}

ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

Process *vm_create_process(VM *vm) {
  Process *process;
  SYNCHRONIZED(vm->process_create_lock, {
    process = create_process_no_reflection(vm);
    add_reflection_to_process(process);
  });
  return process;
}

Object *class_get_function_ref(const Class *cls, const char name[]) {
  const Class *class = cls;
  while (NULL != class) {
    Entity *fref = object_get(cls->_reflection, name);
    if (NULL != fref && fref->obj->_class == Class_FunctionRef) {
      return fref->obj;
    }
    class = class->_super;
  }
  return NULL;
}

Entity object_get_maybe_wrap(Object *obj, const char field[], Task *task,
                             Context *ctx) {
  Entity member;
  const Entity *member_ptr =
      (Class_Class == obj->_class) ? NULL : object_get(obj, field);
  if (NULL == member_ptr) {
    const Function *f = class_get_function(obj->_class, field);
    if (NULL != f) {
      member = entity_object(f->_reflection);
    } else {
      Object *fref = class_get_function_ref(obj->_class, field);
      if (NULL == fref) {
        return NONE_ENTITY;
      }
      return entity_object(fref);
    }
  } else {
    member = *member_ptr;
  }
  if (OBJECT == member.type && Class_Function == member.obj->_class) {
    return entity_object(
        wrap_function_in_ref(member.obj->_function_obj, obj, task, ctx));
  }
  return member;
}