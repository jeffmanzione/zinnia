// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "zinnia/vm/vm.h"

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/vm/process/context.h"
#include "zinnia/vm/process/process.h"

IMPL_ARRAYLIKE(ProcessArray, Process);

Process *create_process_no_reflection(VM *vm) {
  Process *process = ProcessArray_push_back_ref(&vm->processes);
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

Entity object_get_maybe_wrap(Object *obj, const char field[], Heap *heap,
                             Context *ctx) {
  Entity member = NONE_ENTITY;
  const Entity *member_ptr =
      (Class_Class == obj->_class) ? NULL : object_get(obj, field);

  if (NULL == member_ptr) {
    const Function *f = class_get_function(obj->_class, field);
    if (NULL != f) {
      member = entity_object(f->_reflection);
    } else {
      Entity *field_value = class_get_field(obj->_class, field);
      if (NULL != field_value) {
        return *field_value;
      }
      if (obj->_class == Class_Class) {
        field_value = class_get_field(obj->_class_obj, field);
        if (NULL == field_value) {
          return NONE_ENTITY;
        }
        return *field_value;
      }
    }
  } else {
    member = *member_ptr;
  }
  if (OBJECT == member.type && Class_Function == member.obj->_class) {
    return entity_object(
        wrap_function_in_ref(member.obj->_function_obj, obj, heap, ctx));
  }
  return member;
}

// Need to investigate whether there is an issue with creating edges cross-heap.
void task_inc_all_context_(Process *process, Task *task) {
  Heap *heap = process->heap;
  heap_inc_edge(heap, process->_reflection, task->_reflection);

  if (NULL != task->parent_task && task->parent_process == process) {
    heap_inc_edge(heap, task->_reflection, task->parent_task->_reflection);
  }
  TaskSetIterator dependent_tasks;
  TaskSet_iterator(&dependent_tasks, &task->dependent_tasks);
  for (; TaskSet_has_next(&dependent_tasks); TaskSet_next(&dependent_tasks)) {
    Task *dependent_task = *TaskSet_mutable_value(&dependent_tasks);
    heap_inc_edge(heap, dependent_task->_reflection, task->_reflection);
  }
  EntityStackIterator stack;
  EntityStack_iterator(&stack, &task->entity_stack);
  for (; EntityStack_has_next(&stack); EntityStack_next(&stack)) {
    Entity *e = EntityStack_mutable_value(&stack);
    if (OBJECT == e->type) {
      heap_inc_edge(heap, task->_reflection, e->obj);
    }
  }
  if (OBJECT == task->resval.type) {
    heap_inc_edge(heap, task->_reflection, task->resval.obj);
  }
  Context *ctx = task->current;
  while (NULL != ctx) {
    // printf("_task_inc_all_context task=%p ctx=%p self=%p\n",
    // task->_reflection,
    //        ctx->_reflection, IS_NONE(&ctx->self) ? NULL : ctx->self.obj);
    heap_inc_edge(heap, task->_reflection, ctx->_reflection);
    heap_inc_edge(heap, ctx->_reflection, ctx->self.obj);
    if (NULL != ctx->error) {
      heap_inc_edge(heap, ctx->_reflection, ctx->error);
    }
    ctx = ctx->previous_context;
  }
  // printf("done\n");
}

void task_dec_all_context_(Process *process, Task *task) {
  Heap *heap = process->heap;
  heap_dec_edge(heap, process->_reflection, task->_reflection);

  if (NULL != task->parent_task && task->parent_process == process) {
    heap_dec_edge(heap, task->_reflection, task->parent_task->_reflection);
  }
  TaskSetIterator dependent_tasks;
  TaskSet_iterator(&dependent_tasks, &task->dependent_tasks);
  for (; TaskSet_has_next(&dependent_tasks); TaskSet_next(&dependent_tasks)) {
    Task *dependent_task = *TaskSet_mutable_value(&dependent_tasks);
    heap_dec_edge(heap, dependent_task->_reflection, task->_reflection);
  }
  EntityStackIterator stack;
  EntityStack_iterator(&stack, &task->entity_stack);
  for (; EntityStack_has_next(&stack); EntityStack_next(&stack)) {
    Entity *e = EntityStack_mutable_value(&stack);
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

void delete_completed_tasks_(Process *process) {
  TaskSetIterator completed_tasks;
  TaskSet_iterator(&completed_tasks, &process->completed_tasks);
  for (; TaskSet_has_next(&completed_tasks); TaskSet_next(&completed_tasks)) {
    Task *completed_task = *TaskSet_mutable_value(&completed_tasks);
    process_delete_task(process, completed_task);
  }
  // TODO: Switch to set_clear() after that is implemented.
  TaskSet_finalize(&process->completed_tasks);
  TaskSet_init(&process->completed_tasks, hash_task, compare_tasks);
}

void inc_queued_tasks_(Process *process) {
  TaskArrayIterator queued_tasks;
  TaskArray_iterator(&queued_tasks, &process->queued_tasks);
  for (; TaskArray_has_next(&queued_tasks); TaskArray_next(&queued_tasks)) {
    Task *queued_task = *TaskArray_mutable_value(&queued_tasks);
    task_inc_all_context_(process, queued_task);
  }
}

void dec_queued_tasks_(Process *process) {
  TaskArrayIterator queued_tasks;
  TaskArray_iterator(&queued_tasks, &process->queued_tasks);
  for (; TaskArray_has_next(&queued_tasks); TaskArray_next(&queued_tasks)) {
    Task *queued_task = *TaskArray_mutable_value(&queued_tasks);
    task_dec_all_context_(process, queued_task);
  }
}

void inc_task_set_(Process *process, TaskSet *task_set) {
  TaskSetIterator tasks;
  TaskSet_iterator(&tasks, task_set);
  for (; TaskSet_has_next(&tasks); TaskSet_next(&tasks)) {
    Task *task = *TaskSet_mutable_value(&tasks);
    task_inc_all_context_(process, task);
  }
}

void dec_task_set_(Process *process, TaskSet *task_set) {
  TaskSetIterator tasks;
  TaskSet_iterator(&tasks, task_set);
  for (; TaskSet_has_next(&tasks); TaskSet_next(&tasks)) {
    Task *task = *TaskSet_mutable_value(&tasks);
    task_dec_all_context_(process, task);
  }
}

uint32_t process_collect_garbage(Process *process) {
  ASSERT(process != NULL);
  uint32_t deleted_nodes_count;

  SYNCHRONIZED(process->heap_access_lock, {
    SYNCHRONIZED(process->task_queue_lock, {
      CRITICAL(process->task_waiting_cs, {
        delete_completed_tasks_(process);

        task_inc_all_context_(process, process->current_task);
        if (process->is_remote) {
          task_inc_all_context_(process, process->remote_non_daemon_task);
        }
        inc_queued_tasks_(process);
        inc_task_set_(process, &process->waiting_tasks);
        inc_task_set_(process, &process->background_tasks);

        // printf("process_collect_garbage(%p)\n", process->heap);

        deleted_nodes_count = heap_collect_garbage(process->heap);

        task_dec_all_context_(process, process->current_task);
        if (process->is_remote) {
          task_dec_all_context_(process, process->remote_non_daemon_task);
        }
        dec_queued_tasks_(process);
        dec_task_set_(process, &process->waiting_tasks);
        dec_task_set_(process, &process->background_tasks);
      });
    });
  });
  return deleted_nodes_count;
}