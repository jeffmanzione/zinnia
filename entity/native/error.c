// error.c
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#include "entity/native/error.h"

#include <stdint.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

Class *Class_StackLine;

typedef struct {
  Module *module;
  Function *func;
  Token *error_token, *source_error_token;
  const char *source_linetext;
} _StackLine;

void _error_init(Object *obj) {}
void _error_delete(Object *obj) {}

void _stackline_init(Object *obj) { obj->_internal_obj = ALLOC2(_StackLine); }
void _stackline_delete(Object *obj) { DEALLOC(obj->_internal_obj); }

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
  ASSERT(NOT_NULL(stackline), Class_StackLine == stackline->_class);
  _StackLine *sl = (_StackLine *)stackline->_internal_obj;
  return NULL != sl->error_token ? sl->error_token->line : -1;
}

Module *stackline_module(Object *stackline) {
  ASSERT(NOT_NULL(stackline), Class_StackLine == stackline->_class);
  _StackLine *sl = (_StackLine *)stackline->_internal_obj;
  return sl->module;
}

Entity _stackline_module(Task *task, Context *ctx, Object *obj, Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return entity_object(sl->module->_reflection);
}

Entity _stackline_function(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  if (NULL == sl->func) {
    return NONE_ENTITY;
  }
  return entity_object(sl->func->_reflection);
}

Entity _token_tuple(Task *task, const Token *token) {
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

Entity _stackline_token(Task *task, Context *ctx, Object *obj, Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return _token_tuple(task, sl->error_token);
}

Entity _stackline_source_token(Task *task, Context *ctx, Object *obj,
                               Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return _token_tuple(task, sl->source_error_token);
}

Entity _stackline_has_source_map(Task *task, Context *ctx, Object *obj,
                                 Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return NULL == sl->source_error_token ? NONE_ENTITY : entity_int(1);
}

Entity _stackline_source_linetext(Task *task, Context *ctx, Object *obj,
                                  Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return (NULL == sl->source_linetext)
             ? NONE_ENTITY
             : entity_object(string_new(task->parent_process->heap,
                                        sl->source_linetext,
                                        strlen(sl->source_linetext)));
}

Entity _error_constructor(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return raise_error(task, ctx, "Error argument is not a String.");
  }
  object_set_member_obj(task->parent_process->heap, obj, intern("message"),
                        args->obj);

  Object *stacktrace = heap_new(task->parent_process->heap, Class_Array);
  Task *t = task;
  while (NULL != t) {
    Context *c;
    for (c = t->current; c != NULL; c = c->previous_context) {
      Object *stackline = heap_new(task->parent_process->heap, Class_StackLine);
      _StackLine *sl = (_StackLine *)stackline->_internal_obj;
      sl->module = c->module;
      sl->func = (Function *)c->func; // blessed
      sl->error_token =
          (Token *)tape_get_source(c->tape, c->ins - ((c == ctx) ? 0 : 1))
              ->token; // blessed
      sl->source_error_token =
          (Token *)tape_get_source(c->tape, c->ins - ((c == ctx) ? 0 : 1))
              ->source_token; // blessed
      sl->source_linetext =
          (NULL == sl->source_error_token)
              ? NULL
              : tape_get_sourceline(c->tape, sl->source_error_token->line);
      Entity sl_e = entity_object(stackline);
      array_add(task->parent_process->heap, stacktrace, &sl_e);
    }
    t = t->parent_task;
  }
  object_set_member_obj(task->parent_process->heap, obj, intern("stacktrace"),
                        stacktrace);
  return entity_object(obj);
}

Object *error_new(Task *task, Context *ctx, Object *error_msg) {
  Entity error_msg_e = entity_object(error_msg);
  Object *err = heap_new(task->parent_process->heap, Class_Error);
  _error_constructor(task, ctx, err, &error_msg_e);
  return err;
}

void error_add_native(ModuleManager *mm, Module *error) {
  Class_Error = native_class(error, ERROR_NAME, _error_init, _error_delete);
  native_method(Class_Error, CONSTRUCTOR_KEY, _error_constructor);

  Class_StackLine =
      native_class(error, STACKLINE_NAME, _stackline_init, _stackline_delete);
  native_method(Class_StackLine, MODULE_KEY, _stackline_module);
  native_method(Class_StackLine, intern("function"), _stackline_function);
  native_method(Class_StackLine, intern("__token"), _stackline_token);
  native_method(Class_StackLine, intern("__has_source_map"),
                _stackline_has_source_map);
  native_method(Class_StackLine, intern("__source_token"),
                _stackline_source_token);
  native_method(Class_StackLine, intern("source_linetext"),
                _stackline_source_linetext);
}
