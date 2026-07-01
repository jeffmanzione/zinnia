// error.c
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/error.h"

#include <stdint.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/error.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/process/context.h"
#include "zinnia/vm/process/processes.h"
#include "zinnia/vm/process/task.h"

Class *Class_StackLine;

typedef struct {
  Module *module;
  Function *func;
  Token *error_token, *source_error_token;
  const char *source_linetext;
} StackLine_;

void error_init_(Object *obj) {}
void error_delete_(Object *obj) {}

void stackline_init_(Object *obj) { obj->_internal_obj = CNEW(StackLine_); }
void stackline_delete_(Object *obj) { RELEASE(obj->_internal_obj); }

Entity raise_error_with_object(Task *task, Context *context, Object *err) {
  context->error = err;
  *task_mutable_resval(task) = entity_object(err);
  return entity_object(err);
}

Entity raise_error(Task *task, Context *context, const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  int num_chars = vsprintf(buffer, fmt, args);
  va_end(args);
  Object *error_msg = string_new(task->parent_process->heap, buffer, num_chars);
  Object *err = error_new(task, context, error_msg);
  return raise_error_with_object(task, context, err);
}

Entity native_background_raise_error(Task *task, Context *context,
                                     const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  int num_chars = vsprintf(buffer, fmt, args);
  va_end(args);
  Object *err;
  SYNCHRONIZED(task->parent_process->heap_access_lock, {
    err = error_new(task, context,
                    string_new(task->parent_process->heap, buffer, num_chars));
  });
  return raise_error_with_object(task, context, err);
}

uint32_t stackline_linenum(Object *stackline) {
  ASSERT(stackline != NULL);
  ASSERT(Class_StackLine == stackline->_class);
  StackLine_ *sl = (StackLine_ *)stackline->_internal_obj;
  return NULL != sl->error_token ? sl->error_token->line : -1;
}

Module *stackline_module(Object *stackline) {
  ASSERT(stackline != NULL);
  ASSERT(Class_StackLine == stackline->_class);
  StackLine_ *sl = (StackLine_ *)stackline->_internal_obj;
  return sl->module;
}

Entity stackline_module_(Task *task, Context *ctx, Object *obj, Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  if (NULL == sl->module) {
    return NONE_ENTITY;
  }
  return entity_object(sl->module->_reflection);
}

Entity stackline_function_(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  if (NULL == sl->func) {
    return NONE_ENTITY;
  }
  return entity_object(sl->func->_reflection);
}

Entity token_tuple_(Task *task, const Token *token) {
  if (NULL == token) {
    return NONE_ENTITY;
  }
  Object *tuple_obj = heap_new(task->parent_process->heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(3);

  Entity token_text;
  if (NULL == token->text) {
    token_text = NONE_ENTITY;
  } else {
    token_text = entity_object(
        string_new(task->parent_process->heap, token->text, token->len));
  }
  tuple_set(task->parent_process->heap, tuple_obj, 0, &token_text);
  Entity line = entity_int(token->line);
  tuple_set(task->parent_process->heap, tuple_obj, 1, &line);
  Entity col = entity_int(token->col);
  tuple_set(task->parent_process->heap, tuple_obj, 2, &col);
  return entity_object(tuple_obj);
}

Entity stackline_token_(Task *task, Context *ctx, Object *obj, Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return token_tuple_(task, sl->error_token);
}

Entity stackline_source_token_(Task *task, Context *ctx, Object *obj,
                               Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return token_tuple_(task, sl->source_error_token);
}

Entity stackline_has_source_map_(Task *task, Context *ctx, Object *obj,
                                 Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return NULL == sl->source_error_token ? FALSE_ENTITY : TRUE_ENTITY;
}

Entity stackline_source_linetext_(Task *task, Context *ctx, Object *obj,
                                  Entity *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return (NULL == sl->source_linetext)
             ? NONE_ENTITY
             : entity_object(string_new(task->parent_process->heap,
                                        sl->source_linetext,
                                        strlen(sl->source_linetext)));
}

Entity error_constructor_(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (!IS_STRING(args)) {
    return raise_error(task, ctx, "Error argument is not a String.");
  }
  object_set_member_obj(task->parent_process->heap, obj,
                        global_intern("message"), args->obj);
  Object *stacktrace = heap_new(task->parent_process->heap, Class_Array);
  Task *t = task;
  while (NULL != t) {
    Context *c;
    for (c = t->current; NULL != c; c = c->previous_context) {
      Object *stackline = heap_new(task->parent_process->heap, Class_StackLine);
      StackLine_ *sl = (StackLine_ *)stackline->_internal_obj;
      sl->module = c->module;
      sl->func = (Function *)c->func;  // blessed
      const SourceMapping *sm =
          tape_get_source(c->tape, c->ins - ((c == ctx) ? 0 : 1));
      sl->error_token = (Token *)sm->token;                // blessed
      sl->source_error_token = (Token *)sm->source_token;  // blessed
      if (NULL != sl->source_error_token) {
        sl->source_linetext =
            tape_get_sourceline(c->tape, sl->source_error_token->line);
      } else {
        sl->source_linetext = NULL;
      }
      Entity sl_e = entity_object(stackline);
      array_add(task->parent_process->heap, stacktrace, &sl_e);
    }
    t = t->parent_task;
  }
  object_set_member_obj(task->parent_process->heap, obj,
                        global_intern("stacktrace"), stacktrace);
  return entity_object(obj);
}

Object *error_new(Task *task, Context *ctx, Object *error_msg) {
  Entity error_msg_e = entity_object(error_msg);
  Object *err = heap_new(task->parent_process->heap, Class_Error);
  error_constructor_(task, ctx, err, &error_msg_e);
  return err;
}

void stackline_copy_(EntityCopier *copier, Object *src_obj,
                     Object *target_obj) {
  StackLine_ *src = (StackLine_ *)src_obj->_internal_obj;
  StackLine_ *target = (StackLine_ *)target_obj->_internal_obj;
  *target = *src;
}

void error_add_native(ModuleManager *mm, Module *error) {
  Class_Error = native_class(error, ERROR_NAME, error_init_, error_delete_);
  native_method(Class_Error, CONSTRUCTOR_KEY, error_constructor_);
  Class_StackLine =
      native_class(error, STACKLINE_NAME, stackline_init_, stackline_delete_);
  Class_StackLine->_copy_fn = (ObjCopyFn)stackline_copy_;
  native_method(Class_StackLine, MODULE_KEY, stackline_module_);
  native_method(Class_StackLine, global_intern("function"),
                stackline_function_);
  native_method(Class_StackLine, global_intern("__token"), stackline_token_);
  native_method(Class_StackLine, global_intern("__has_source_map"),
                stackline_has_source_map_);
  native_method(Class_StackLine, global_intern("__source_token"),
                stackline_source_token_);
  native_method(Class_StackLine, global_intern("source_linetext"),
                stackline_source_linetext_);
}
