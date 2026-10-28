// async.c
//
// Created on: Nov 11, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/async.h"

#include "zinnia/util/platform.h"

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef OS_WINDOWS
#include <synchapi.h>
#endif

#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/vm/vm.h"

struct Future_ {
  Frame *frame;
  bool _is_complete, _is_result_set;
};

void future_init_(Object *obj) {
  Future *f = MNEW(Future);
  f->_is_complete = false;
  f->_is_result_set = false;
  f->frame = NULL;
  obj->_internal_obj = f;
}

void future_delete_(Object *obj) { RELEASE(obj->_internal_obj); }

Object *future_create(Frame *frame) {
  Heap *heap = frame->parent_isolate->heap;
  Object *future_obj = heap_new(heap, Class_Future);
  Future *future = (Future *)future_obj->_internal_obj;
  future->frame = frame;
  return future_obj;
}

bool future_is_complete(Future *f) {
  if (f->_is_complete) {
    return true;
  }
  f->_is_complete =
      (f->frame->state == FRAME_COMPLETE) || (f->frame->state == FRAME_ERROR);
  return f->_is_complete;
}

Entity future_value_(Frame *frame, Scope *ctx, Object *obj, Entity *args) {
  Object *f_obj = heap_new(frame->parent_isolate->heap, Class_Future);
  Future *f = (Future *)f_obj->_internal_obj;
  f->_is_complete = true;
  f->_is_result_set = true;
  object_set_member(frame->parent_isolate->heap, f_obj, RESULT_VAL, args);
  return entity_object(f_obj);
}

const Entity *future_get_value(Heap *heap, Object *obj) {
  ASSERT(obj != NULL);
  ASSERT(obj->_class == Class_Future);
  Future *f = (Future *)obj->_internal_obj;
  if (!f->_is_result_set) {
    const Entity *result = frame_get_acc(f->frame);
    object_set_member(heap, obj, RESULT_VAL, result);
    f->_is_result_set = true;
    return result;
  }
  return object_get(obj, RESULT_VAL);
}

Frame *future_get_frame(Future *f) { return f->frame; }

Entity spawn_isolate_(Frame *frame, Scope *ctx, Object *obj, Entity *args) {
  if (!IS_CLASS(args, Class_Tuple)) {
    return raise_error(
        frame, ctx,
        "Cannot call __spawn_isolate with something other than (Function, "
        "ANY, ANY).");
  }

  EXTRACT_TUPLE_ARGS(tuple, args, 3, frame, ctx);

  const Entity *fn = tuple_get(tuple, 0);
  const Entity *fn_args = tuple_get(tuple, 1);
  const Entity *keep_alive_arg = tuple_get(tuple, 2);
  if (!IS_OBJECT(fn) || !(inherits_from(fn->obj->_class, Class_Function) ||
                          IS_CLASS(fn, Class_FunctionRef))) {
    return raise_error(frame, ctx,
                       "__spawn_isolate expects (Function, ANY, ANY).");
  }

  VM *vm = frame->parent_isolate->vm;
  Isolate *new_isolate = vm_create_isolate(vm);
  new_isolate->keep_alive = !IS_NONE(keep_alive_arg);
  Frame *new_frame = isolate_create_frame(new_isolate);
  new_frame->parent_frame = frame;
  Scope *new_ctx;

  const Function *f;
  Object *self;
  if (IS_CLASS(fn, Class_Function)) {
    f = fn->obj->_function_obj;
    self = f->_module->_reflection;
  } else if (IS_CLASS(fn, Class_FunctionRef)) {
    f = function_ref_get_func(fn->obj);
    self = function_ref_get_object(fn->obj);
  } else {
    return raise_error(frame, ctx, "__spawn_isolate expects (Function, ANY).");
  }

  BULK_COPY(copier, new_isolate->heap, {
    Entity self_e = entity_object(self);
    self = entitycopier_copy(&copier, &self_e).obj;
    new_ctx =
        frame_create_scope(new_frame, self, (Module *)f->_module, f->_ins_pos);
    *frame_mutable_acc(new_frame) = entitycopier_copy(&copier, fn_args);
  });

  scope_set_function(new_ctx, f);

  // Ensures that the remote task does not end while it is still accessible.
  if (new_isolate->keep_alive) {
    new_isolate->keep_alive_frame = isolate_create_unqueued_frame(new_isolate);
    isolate_insert_waiting_frame(new_isolate, new_isolate->keep_alive_frame);
  }

  return entity_object(new_isolate->_reflection);
}

Entity sleep__(Frame *frame, Scope *ctx, Object *obj, Entity *args) {
  double sleep_duration_sec = 0;
  if (IS_INT(args)) {
    sleep_duration_sec = pint(&args->pri);
  } else if (IS_FLOAT(args)) {
    sleep_duration_sec = pfloat(&args->pri);
  } else {
    return raise_error(frame, ctx,
                       "sleep() expected to be called with number.");
  }
#if defined(OS_WINDOWS)
  // Accepts millis as an unsigned long instead of double seconds.
  Sleep(sleep_duration_sec * 1000);
#else
  sleep(sleep_duration_sec);
#endif
  return NONE_ENTITY;
}

Entity validate_actor_call_(Frame *current_frame, Scope *current_ctx,
                            Entity *args) {
  if (!IS_CLASS(args, Class_Tuple)) {
    return raise_error(current_frame, current_ctx,
                       "Cannot call __actor_call with something other than "
                       "(Remote, String, args).");
  }

  EXTRACT_TUPLE_ARGS(tuple, args, 3, current_frame, current_ctx);

  const Entity *actor_entity = tuple_get(tuple, 0);
  if (!IS_CLASS(actor_entity, Class_ActorRef)) {
    return raise_error(current_frame, current_ctx,
                       "__actor_call expects (ActorRef, String, args).");
  }

  const Entity *fn_name_entity = tuple_get(tuple, 1);
  if (!IS_STRING(fn_name_entity)) {
    return raise_error(current_frame, current_ctx,
                       "__actor_call expects (ActorRef, String, args).");
  }
  return NONE_ENTITY;
}

Entity create_future_for_frame_(Frame *current_frame, Frame *actor_frame) {
  Frame *new_frame =
      isolate_create_unqueued_frame(current_frame->parent_isolate);
  new_frame->parent_frame = current_frame;

  Object *future_obj = future_create(new_frame);
  actor_frame->actor_future = (Future *)future_obj->_internal_obj;

  isolate_insert_waiting_frame(current_frame->parent_isolate, new_frame);

  return entity_object(future_obj);
}

Entity actor_call_(Frame *current_frame, Scope *current_ctx, Object *obj,
                   Entity *args) {
  Entity error = validate_actor_call_(current_frame, current_ctx, args);
  if (NONE != error.type) {
    return error;
  }

  const Tuple *tuple = (Tuple *)args->obj->_internal_obj;
  const Entity *actor_entity = tuple_get(tuple, 0);
  const Entity *fn_name_entity = tuple_get(tuple, 1);
  const Entity *fn_args = tuple_get(tuple, 2);

  ActorRef *actor = extract_actor_from_obj(actor_entity->obj);
  Isolate *actor_isolate = actor_get_isolate(actor);
  Object *actor_object = actor_get_object(actor);

  const char *fn_name = intern_entity(fn_name_entity);

  Entity fn =
      object_get_maybe_wrap(actor_object, fn_name, actor_isolate->heap, NULL);

  const Function *f;
  if (IS_CLASS(&fn, Class_Function)) {
    f = fn.obj->_function_obj;
  } else if (IS_CLASS(&fn, Class_FunctionRef)) {
    f = function_ref_get_func(fn.obj);
  } else {
    return raise_error(current_frame, current_ctx,
                       "Could not find method '%s' on actor.", fn_name);
  }

  Frame *actor_frame = isolate_create_unqueued_frame(actor_isolate);
  // NOTE: This is a reference to a task in another heap.
  actor_frame->parent_frame = current_frame;

  Scope *actor_ctx = frame_create_scope(actor_frame, actor_object,
                                        (Module *)f->_module, f->_ins_pos);

  // printf("context=%p, task=%p\n", remote_ctx->_reflection,
  //        remote_task->_reflection);

  scope_set_function(actor_ctx, f);

  SYNCHRONIZED(current_frame->parent_isolate->heap_access_lock, {
    SYNCHRONIZED(actor_isolate->heap_access_lock, {
      *frame_mutable_acc(actor_frame) =
          entity_copy(fn_args, actor_isolate->heap);
    });
  });

  isolate_enqueue_frame(actor_isolate, actor_frame);
  Entity future_entity = create_future_for_frame_(current_frame, actor_frame);
  condition_broadcast(actor_isolate->frame_wait_cond);
  return future_entity;
}

void async_add_native(ModuleManager *mm, Module *async) {
  Class_Future = native_class(async, FUTURE_NAME, future_init_, future_delete_);
  native_function(async, VALUE_KEY, future_value_);
  native_function(async, global_intern("__spawn_isolate"), spawn_isolate_);
  native_blocking_function(async, global_intern("__sleep"), sleep__);
  native_function(async, global_intern("__actor_call"), actor_call_);
}