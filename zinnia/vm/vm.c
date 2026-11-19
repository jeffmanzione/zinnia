// vm.c
//
// Created on: Jan 1, 2021
//     Author: Jeff Manzione

#include "zinnia/vm/vm.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/value/class/classes_def.h"
#include "zinnia/vm/isolate/isolate.h"
#include "zinnia/vm/isolate/scope.h"

IMPL_ARRAYLIKE(IsolateArray, Isolate *);

Isolate *create_isolate_no_reflection(VM *vm) {
  Isolate *isolate = MNEW(Isolate);
  IsolateArray_push_back(&vm->isolates, isolate);
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

Value *class_get_field(const Class *cls, const char name[]) {
  const Class *class = cls;
  while (NULL != class) {
    Value *field = object_get(cls->_reflection, name);
    if (NULL != field) {
      return field;
    }
    class = class->_super;
  }
  return NULL;
}

Value object_get_maybe_wrap(Object *obj, const char field[], Heap *heap,
                            Scope *ctx) {
  Value member = NONE_VALUE;
  const Value *member_ptr =
      (Class_Class == obj->_class) ? NULL : object_get(obj, field);

  if (NULL == member_ptr) {
    const Function *f = class_get_function(obj->_class, field);
    if (NULL != f) {
      member = value_object(f->_reflection);
    } else {
      Value *field_value = class_get_field(obj->_class, field);
      if (NULL != field_value) {
        return *field_value;
      }
      if (obj->_class == Class_Class) {
        field_value = class_get_field(obj->_class_obj, field);
        if (NULL == field_value) {
          return NONE_VALUE;
        }
        return *field_value;
      }
    }
  } else {
    member = *member_ptr;
  }
  if (OBJECT == member.type && Class_Function == member.obj->_class) {
    return value_object(
        wrap_function_in_ref(member.obj->_function_obj, obj, heap, ctx));
  }
  return member;
}

// Marks what [frame] refers to, so the collection that follows keeps it. Values
// in other heaps are left alone by heap_mark().
static void frame_mark_(Isolate *isolate, Frame *frame) {
  Heap *heap = isolate->heap;
  heap_mark(heap, frame->_reflection);
  if (NULL != frame->parent_frame && frame->parent_isolate == isolate) {
    heap_mark(heap, frame->parent_frame->_reflection);
  }
  FrameSetIterator dependent_frames;
  FrameSet_iterator(&dependent_frames, &frame->dependent_frames);
  for (; FrameSet_has_next(&dependent_frames);
       FrameSet_next(&dependent_frames)) {
    Frame *dependent_frame = *FrameSet_mutable_value(&dependent_frames);
    heap_mark(heap, dependent_frame->_reflection);
  }
  ValueStackIterator stack;
  ValueStack_iterator(&stack, &frame->value_stack);
  for (; ValueStack_has_next(&stack); ValueStack_next(&stack)) {
    Value *e = ValueStack_mutable_value(&stack);
    if (OBJECT == e->type) {
      heap_mark(heap, e->obj);
    }
  }
  if (OBJECT == frame->acc.type) {
    heap_mark(heap, frame->acc.obj);
  }
  for (int i = 0; i < SlotStack_size(&frame->slots); ++i) {
    const Slot *slot = SlotStack_get_ref_unchecked(&frame->slots, i);
    if (slot->is_set && OBJECT == slot->value.type) {
      heap_mark(heap, slot->value.obj);
    }
  }
  Scope *ctx = frame->current;
  while (NULL != ctx) {
    // A scope may have no object on the heap, so the frame keeps what it
    // refers to.
    if (NULL != ctx->_reflection) {
      heap_mark(heap, ctx->_reflection);
    }
    if (NULL != ctx->previous_scope &&
        NULL != ctx->previous_scope->_reflection) {
      heap_mark(heap, ctx->previous_scope->_reflection);
    }
    heap_mark(heap, ctx->self.obj);
    if (NULL != ctx->error) {
      heap_mark(heap, ctx->error);
    }
    ctx = scope_dynamic_parent(ctx);
  }
}

void delete_completed_frames_(Isolate *isolate) {
  FrameSetIterator completed_frames;
  FrameSet_iterator(&completed_frames, &isolate->completed_frames);
  for (; FrameSet_has_next(&completed_frames);
       FrameSet_next(&completed_frames)) {
    Frame *completed_frame = *FrameSet_mutable_value(&completed_frames);
    // The frame that ran last can be among them, and isn't marked once freed.
    if (completed_frame == isolate->current_frame) {
      isolate->current_frame = NULL;
    }
    isolate_delete_frame(isolate, completed_frame);
  }
  // TODO: Switch to set_clear() after that is implemented.
  FrameSet_finalize(&isolate->completed_frames);
  FrameSet_init(&isolate->completed_frames, hash_frame, compare_frames);
}

static void mark_queued_frames_(Isolate *isolate) {
  FrameArrayIterator queued_frames;
  FrameArray_iterator(&queued_frames, &isolate->queued_frames);
  for (; FrameArray_has_next(&queued_frames); FrameArray_next(&queued_frames)) {
    Frame *queued_frame = *FrameArray_mutable_value(&queued_frames);
    frame_mark_(isolate, queued_frame);
  }
}

static void mark_frame_set_(Isolate *isolate, FrameSet *frame_set) {
  FrameSetIterator frames;
  FrameSet_iterator(&frames, frame_set);
  for (; FrameSet_has_next(&frames); FrameSet_next(&frames)) {
    Frame *frame = *FrameSet_mutable_value(&frames);
    frame_mark_(isolate, frame);
  }
}

uint32_t isolate_collect_garbage(Isolate *isolate) {
  ASSERT(isolate != NULL);
  uint32_t deleted_nodes_count;

  // frame_waiting_cs is always taken before frame_queue_lock.
  SYNCHRONIZED(isolate->heap_access_lock, {
    CRITICAL(isolate->frame_waiting_cs, {
      SYNCHRONIZED(isolate->frame_queue_lock, {
        delete_completed_frames_(isolate);

        if (NULL != isolate->current_frame) {
          frame_mark_(isolate, isolate->current_frame);
        }
        if (isolate->keep_alive) {
          frame_mark_(isolate, isolate->keep_alive_frame);
        }
        mark_queued_frames_(isolate);
        mark_frame_set_(isolate, &isolate->waiting_frames);
        mark_frame_set_(isolate, &isolate->blocking_frames);
        deleted_nodes_count = heap_collect_garbage(isolate->heap);
      });
    });
  });
  return deleted_nodes_count;
}