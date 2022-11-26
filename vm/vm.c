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

Entity *class_get_field(const Class *cls, const char name[]) {
  const Class *class = cls;
  while (NULL != class) {
    Entity *field = object_get(cls->_reflection, name);
    if (NULL != field) {
      return field;
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
      Entity *field_value = class_get_field(obj->_class_obj, field);
      if (NULL == field_value) {
        return NONE_ENTITY;
      }
      return *field_value;
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

void _task_inc_all_context(Process *process, Task *task) {
  Heap *heap = process->heap;
  if (NULL != task->parent_task) {
    heap_inc_edge(heap, task->_reflection, task->parent_task->_reflection);
  }
  M_iter dependent_tasks = set_iter(&task->dependent_tasks);
  for (; has(&dependent_tasks); inc(&dependent_tasks)) {
    Task *dependent_task = (Task *)value(&dependent_tasks);
    heap_inc_edge(heap, dependent_task->_reflection, task->_reflection);
  }
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
    heap_inc_edge(heap, task->_reflection, ctx->_reflection);
    heap_inc_edge(heap, ctx->_reflection, ctx->self.obj);
    if (NULL != ctx->error) {
      heap_inc_edge(heap, ctx->_reflection, ctx->error);
    }
    ctx = ctx->previous_context;
  }
}

void _task_dec_all_context(Process *process, Task *task) {
  Heap *heap = process->heap;
  if (NULL != task->parent_task) {
    heap_dec_edge(heap, task->_reflection, task->parent_task->_reflection);
  }
  M_iter dependent_tasks = set_iter(&task->dependent_tasks);
  for (; has(&dependent_tasks); inc(&dependent_tasks)) {
    Task *dependent_task = (Task *)value(&dependent_tasks);
    heap_dec_edge(heap, dependent_task->_reflection, task->_reflection);
  }
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
    heap_dec_edge(heap, ctx->_reflection, ctx->self.obj);
    heap_dec_edge(heap, task->_reflection, ctx->_reflection);
    if (NULL != ctx->error) {
      heap_dec_edge(heap, ctx->_reflection, ctx->error);
    }
    ctx = ctx->previous_context;
  }
}

void _delete_completed_tasks(Process *process) {
  M_iter completed_tasks = set_iter(&process->completed_tasks);
  for (; has(&completed_tasks); inc(&completed_tasks)) {
    Task *completed_task = (Task *)value(&completed_tasks);
    set_remove(&process->completed_tasks, completed_task);
    process_delete_task(process, completed_task);
  }
}

void _inc_queued_tasks(Process *process) {
  Q_iter queued_tasks = Q_iterator(&process->queued_tasks);
  for (; Q_has(&queued_tasks); Q_inc(&queued_tasks)) {
    Task *queued_task = *(Task **)Q_value(&queued_tasks);
    heap_inc_edge(process->heap, process->_reflection,
                  queued_task->_reflection);
    _task_inc_all_context(process, queued_task);
  }
}

void _dec_queued_tasks(Process *process) {
  Q_iter queued_tasks = Q_iterator(&process->queued_tasks);
  for (; Q_has(&queued_tasks); Q_inc(&queued_tasks)) {
    Task *queued_task = *(Task **)Q_value(&queued_tasks);
    heap_dec_edge(process->heap, process->_reflection,
                  queued_task->_reflection);
    _task_dec_all_context(process, queued_task);
  }
}

void _inc_task_set(Process *process, Set *task_set) {
  M_iter tasks = set_iter(task_set);
  for (; has(&tasks); inc(&tasks)) {
    Task *task = (Task *)value(&tasks);
    heap_inc_edge(process->heap, process->_reflection, task->_reflection);
    _task_inc_all_context(process, task);
  }
}

void _dec_task_set(Process *process, Set *task_set) {
  M_iter tasks = set_iter(task_set);
  for (; has(&tasks); inc(&tasks)) {
    Task *task = (Task *)value(&tasks);
    heap_dec_edge(process->heap, process->_reflection, task->_reflection);
    _task_dec_all_context(process, task);
  }
}

uint32_t process_collect_garbage(Process *process) {
  ASSERT(NOT_NULL(process));
  uint32_t deleted_nodes_count;

  SYNCHRONIZED(process->heap_access_lock, {
    SYNCHRONIZED(process->task_queue_lock, {
      CRITICAL(process->task_waiting_cs, {
        _delete_completed_tasks(process);

        _task_inc_all_context(process, process->current_task);
        _inc_queued_tasks(process);
        _inc_task_set(process, &process->waiting_tasks);
        _inc_task_set(process, &process->background_tasks);

        deleted_nodes_count = heap_collect_garbage(process->heap);

        _task_dec_all_context(process, process->current_task);
        _dec_queued_tasks(process);
        _dec_task_set(process, &process->waiting_tasks);
        _dec_task_set(process, &process->background_tasks);
      });
    });
  });
  return deleted_nodes_count;
}