// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "vm/vm.h"

#include "entity/class/classes.h"
#include "vm/process/context.h"
#include "vm/process/process.h"

Process *create_process_no_reflection(VM *vm) {
  Process *process = alist_add(&vm->processes);
  process_init(process);
  process->vm = vm;
  return process;
}

void add_reflection_to_process(Process *process) {
  process->_reflection = heap_new(process->heap, Class_Process);
  process->_reflection->_internal_obj = process;
  heap_make_root(process->heap, process->_reflection);
}

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

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

void _task_inc_all_context(Heap *heap, Task *task) {
  AL_iter stack = alist_iter(&task->entity_stack);
  for (; al_has(&stack); al_inc(&stack)) {
    Entity *e = al_value(&stack);
    if (OBJECT == e->type) {
      heap_inc_edge(heap, task->_reflection, e->obj);
    }
  }
  if (OBJECT == task->resval.type) {
    heap_inc_edge(heap, task->_reflection, task->resval.obj);
  }
  Context *ctx = task->current;
  while (NULL != ctx) {
    heap_inc_edge(heap, task->_reflection, ctx->member_obj);
    ctx = ctx->previous_context;
  }
}

void _task_dec_all_context(Heap *heap, Task *task) {
  AL_iter stack = alist_iter(&task->entity_stack);
  for (; al_has(&stack); al_inc(&stack)) {
    Entity *e = al_value(&stack);
    if (OBJECT == e->type) {
      heap_dec_edge(heap, task->_reflection, e->obj);
    }
  }
  if (OBJECT == task->resval.type) {
    heap_dec_edge(heap, task->_reflection, task->resval.obj);
  }
  Context *ctx = task->current;
  while (NULL != ctx) {
    heap_dec_edge(heap, task->_reflection, ctx->member_obj);
    ctx = ctx->previous_context;
  }
}

int collect_garbage(Process *process) {
  Heap *heap = process->heap;
  uint32_t deleted_nodes_count;

  SYNCHRONIZED(process->task_waiting_lock, {
    SYNCHRONIZED(process->task_queue_lock, {
      SYNCHRONIZED(process->task_create_lock, {
        _task_inc_all_context(heap, process->current_task);
        Q_iter queued_tasks = Q_iterator(&process->queued_tasks);
        for (; Q_has(&queued_tasks); Q_inc(&queued_tasks)) {
          Task *queued_task = (Task *)Q_value(&queued_tasks);
          _task_inc_all_context(heap, queued_task);
        }
        M_iter waiting_tasks = set_iter(&process->waiting_tasks);
        for (; has(&waiting_tasks); inc(&waiting_tasks)) {
          Task *waiting_task = (Task *)value(&waiting_tasks);
          _task_inc_all_context(heap, waiting_task);
        }

        deleted_nodes_count = heap_collect_garbage(heap);

        _task_dec_all_context(heap, process->current_task);
        queued_tasks = Q_iterator(&process->queued_tasks);
        for (; Q_has(&queued_tasks); Q_inc(&queued_tasks)) {
          Task *queued_task = (Task *)Q_value(&queued_tasks);
          _task_dec_all_context(heap, queued_task);
        }
        waiting_tasks = set_iter(&process->waiting_tasks);
        for (; has(&waiting_tasks); inc(&waiting_tasks)) {
          Task *waiting_task = (Task *)value(&waiting_tasks);
          _task_dec_all_context(heap, waiting_task);
        }
      });
    });
  });
  return deleted_nodes_count;
}