// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "zinnia/vm/vm.h"

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/vm/isolate/isolate.h"
#include "zinnia/vm/isolate/scope.h"

IMPL_ARRAYLIKE(IsolateArray, Isolate);

Isolate *create_isolate_no_reflection(VM *vm) {
  Isolate *isolate = IsolateArray_push_back_ref(&vm->isolates);
  isolate_init(isolate, &vm->base_heap_conf);
  isolate->vm = vm;
  return isolate;
}

void add_reflection_to_isolate(Isolate *isolate) {
  isolate->_reflection = heap_new(isolate->heap, Class_Isolate);
  isolate->_reflection->_internal_obj = isolate;
  heap_make_root(isolate->heap, isolate->_reflection);
}

ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

Isolate *vm_create_isolate(VM *vm) {
  Isolate *isolate;
  SYNCHRONIZED(vm->isolate_create_lock, {
    isolate = create_isolate_no_reflection(vm);
    add_reflection_to_isolate(isolate);
  });
  return isolate;
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
                             Scope *ctx) {
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
void frame_inc_all_scope_(Isolate *isolate, Frame *frame) {
  Heap *heap = isolate->heap;
  heap_inc_edge(heap, isolate->_reflection, frame->_reflection);

  if (NULL != frame->parent_frame && frame->parent_isolate == isolate) {
    heap_inc_edge(heap, frame->_reflection, frame->parent_frame->_reflection);
  }
  FrameSetIterator dependent_frames;
  FrameSet_iterator(&dependent_frames, &frame->dependent_frames);
  for (; FrameSet_has_next(&dependent_frames);
       FrameSet_next(&dependent_frames)) {
    Frame *dependent_frame = *FrameSet_mutable_value(&dependent_frames);
    heap_inc_edge(heap, dependent_frame->_reflection, frame->_reflection);
  }
  EntityStackIterator stack;
  EntityStack_iterator(&stack, &frame->entity_stack);
  for (; EntityStack_has_next(&stack); EntityStack_next(&stack)) {
    Entity *e = EntityStack_mutable_value(&stack);
    if (OBJECT == e->type) {
      heap_inc_edge(heap, frame->_reflection, e->obj);
    }
  }
  if (OBJECT == frame->acc.type) {
    heap_inc_edge(heap, frame->_reflection, frame->acc.obj);
  }
  Scope *ctx = frame->current;
  while (NULL != ctx) {
    // printf("_task_inc_all_context task=%p ctx=%p self=%p\n",
    // task->_reflection,
    //        ctx->_reflection, IS_NONE(&ctx->self) ? NULL : ctx->self.obj);
    heap_inc_edge(heap, frame->_reflection, ctx->_reflection);
    heap_inc_edge(heap, ctx->_reflection, ctx->self.obj);
    if (NULL != ctx->error) {
      heap_inc_edge(heap, ctx->_reflection, ctx->error);
    }
    ctx = ctx->previous_scope;
  }
  // printf("done\n");
}

void frame_dec_all_scope_(Isolate *isolate, Frame *frame) {
  Heap *heap = isolate->heap;
  heap_dec_edge(heap, isolate->_reflection, frame->_reflection);

  if (NULL != frame->parent_frame && frame->parent_isolate == isolate) {
    heap_dec_edge(heap, frame->_reflection, frame->parent_frame->_reflection);
  }
  FrameSetIterator dependent_frames;
  FrameSet_iterator(&dependent_frames, &frame->dependent_frames);
  for (; FrameSet_has_next(&dependent_frames);
       FrameSet_next(&dependent_frames)) {
    Frame *dependent_frame = *FrameSet_mutable_value(&dependent_frames);
    heap_dec_edge(heap, dependent_frame->_reflection, frame->_reflection);
  }
  EntityStackIterator stack;
  EntityStack_iterator(&stack, &frame->entity_stack);
  for (; EntityStack_has_next(&stack); EntityStack_next(&stack)) {
    Entity *e = EntityStack_mutable_value(&stack);
    if (OBJECT == e->type) {
      heap_dec_edge(heap, frame->_reflection, e->obj);
    }
  }
  if (OBJECT == frame->acc.type) {
    heap_dec_edge(heap, frame->_reflection, frame->acc.obj);
  }
  Scope *ctx = frame->current;
  while (NULL != ctx) {
    heap_dec_edge(heap, ctx->_reflection, ctx->self.obj);
    heap_dec_edge(heap, frame->_reflection, ctx->_reflection);
    if (NULL != ctx->error) {
      heap_dec_edge(heap, ctx->_reflection, ctx->error);
    }
    ctx = ctx->previous_scope;
  }
}

void delete_completed_frames_(Isolate *isolate) {
  FrameSetIterator completed_frames;
  FrameSet_iterator(&completed_frames, &isolate->completed_frames);
  for (; FrameSet_has_next(&completed_frames);
       FrameSet_next(&completed_frames)) {
    Frame *completed_frame = *FrameSet_mutable_value(&completed_frames);
    isolate_delete_frame(isolate, completed_frame);
  }
  // TODO: Switch to set_clear() after that is implemented.
  FrameSet_finalize(&isolate->completed_frames);
  FrameSet_init(&isolate->completed_frames, hash_frame, compare_frames);
}

void inc_queued_frames_(Isolate *isolate) {
  FrameArrayIterator queued_frames;
  FrameArray_iterator(&queued_frames, &isolate->queued_frames);
  for (; FrameArray_has_next(&queued_frames); FrameArray_next(&queued_frames)) {
    Frame *queued_frame = *FrameArray_mutable_value(&queued_frames);
    frame_inc_all_scope_(isolate, queued_frame);
  }
}

void dec_queued_frames_(Isolate *isolate) {
  FrameArrayIterator queued_frames;
  FrameArray_iterator(&queued_frames, &isolate->queued_frames);
  for (; FrameArray_has_next(&queued_frames); FrameArray_next(&queued_frames)) {
    Frame *queued_frame = *FrameArray_mutable_value(&queued_frames);
    frame_dec_all_scope_(isolate, queued_frame);
  }
}

void inc_frame_set_(Isolate *isolate, FrameSet *frame_set) {
  FrameSetIterator frames;
  FrameSet_iterator(&frames, frame_set);
  for (; FrameSet_has_next(&frames); FrameSet_next(&frames)) {
    Frame *frame = *FrameSet_mutable_value(&frames);
    frame_inc_all_scope_(isolate, frame);
  }
}

void dec_frame_set_(Isolate *isolate, FrameSet *frame_set) {
  FrameSetIterator frames;
  FrameSet_iterator(&frames, frame_set);
  for (; FrameSet_has_next(&frames); FrameSet_next(&frames)) {
    Frame *frame = *FrameSet_mutable_value(&frames);
    frame_dec_all_scope_(isolate, frame);
  }
}

uint32_t isolate_collect_garbage(Isolate *isolate) {
  ASSERT(isolate != NULL);
  uint32_t deleted_nodes_count;

  SYNCHRONIZED(isolate->heap_access_lock, {
    SYNCHRONIZED(isolate->frame_queue_lock, {
      CRITICAL(isolate->frame_waiting_cs, {
        delete_completed_frames_(isolate);

        frame_inc_all_scope_(isolate, isolate->current_frame);
        if (isolate->keep_alive) {
          frame_inc_all_scope_(isolate, isolate->keep_alive_frame);
        }
        inc_queued_frames_(isolate);
        inc_frame_set_(isolate, &isolate->waiting_frames);
        inc_frame_set_(isolate, &isolate->blocking_frames);

        // printf("process_collect_garbage(%p)\n", process->heap);

        deleted_nodes_count = heap_collect_garbage(isolate->heap);

        frame_dec_all_scope_(isolate, isolate->current_frame);
        if (isolate->keep_alive) {
          frame_dec_all_scope_(isolate, isolate->keep_alive_frame);
        }
        dec_queued_frames_(isolate);
        dec_frame_set_(isolate, &isolate->waiting_frames);
        dec_frame_set_(isolate, &isolate->blocking_frames);
      });
    });
  });
  return deleted_nodes_count;
}