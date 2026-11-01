// error.c
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#include "zinnia/entity/native/error.h"

#include <stdint.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/entity/value.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/error.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/isolate/frame.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/isolate/scope.h"

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

Value raise_error_with_object(Frame *frame, Scope *scope, Object *err) {
  scope->error = err;
  *frame_mutable_acc(frame) = value_object(err);
  return value_object(err);
}

Value raise_errorv(Frame *frame, Scope *scope, const char fmt[], va_list args) {
  char buffer[1024];
  int num_chars = vsprintf(buffer, fmt, args);
  Object *error_msg =
      string_new(frame->parent_isolate->heap, buffer, num_chars);
  Object *err = error_new(frame, scope, error_msg);
  return raise_error_with_object(frame, scope, err);
}

Value raise_error(Frame *frame, Scope *scope, const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  Value retval = raise_errorv(frame, scope, fmt, args);
  va_end(args);
  return retval;
}

Value native_blocking_raise_error(Frame *frame, Scope *scope, const char fmt[],
                                  ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  int num_chars = vsprintf(buffer, fmt, args);
  va_end(args);
  Object *err;
  SYNCHRONIZED(frame->parent_isolate->heap_access_lock, {
    err = error_new(frame, scope,
                    string_new(frame->parent_isolate->heap, buffer, num_chars));
    native_pin_blocking_object(frame->parent_isolate, err);
  });
  return raise_error_with_object(frame, scope, err);
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

Value stackline_module_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  if (NULL == sl->module) {
    return NONE_VALUE;
  }
  return value_object(sl->module->_reflection);
}

Value stackline_function_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  if (NULL == sl->func) {
    return NONE_VALUE;
  }
  return value_object(sl->func->_reflection);
}

Value token_tuple_(Frame *frame, const Token *token) {
  if (NULL == token) {
    return NONE_VALUE;
  }
  Object *tuple_obj = heap_new(frame->parent_isolate->heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(3);

  Value token_text;
  if (NULL == token->text) {
    token_text = NONE_VALUE;
  } else {
    token_text = value_object(
        string_new(frame->parent_isolate->heap, token->text, token->len));
  }
  tuple_set(frame->parent_isolate->heap, tuple_obj, 0, &token_text);
  Value line = value_int(token->line);
  tuple_set(frame->parent_isolate->heap, tuple_obj, 1, &line);
  Value col = value_int(token->col);
  tuple_set(frame->parent_isolate->heap, tuple_obj, 2, &col);
  return value_object(tuple_obj);
}

Value stackline_token_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return token_tuple_(frame, sl->error_token);
}

Value stackline_source_token_(Frame *frame, Scope *ctx, Object *obj,
                              Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return token_tuple_(frame, sl->source_error_token);
}

Value stackline_has_source_map_(Frame *frame, Scope *ctx, Object *obj,
                                Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return NULL == sl->source_error_token ? FALSE_VALUE : TRUE_VALUE;
}

Value stackline_source_linetext_(Frame *frame, Scope *ctx, Object *obj,
                                 Value *args) {
  StackLine_ *sl = (StackLine_ *)obj->_internal_obj;
  return (NULL == sl->source_linetext)
             ? NONE_VALUE
             : value_object(string_new(frame->parent_isolate->heap,
                                       sl->source_linetext,
                                       strlen(sl->source_linetext)));
}

Value error_constructor_(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (!IS_STRING(args)) {
    return raise_error(frame, ctx, "Error argument is not a String.");
  }
  object_set_member_obj(frame->parent_isolate->heap, obj,
                        global_intern("message"), args->obj);
  Object *stacktrace = heap_new(frame->parent_isolate->heap, Class_Array);
  Frame *t = frame;
  while (NULL != t) {
    Scope *c;
    for (c = t->current; NULL != c; c = c->previous_scope) {
      Object *stackline =
          heap_new(frame->parent_isolate->heap, Class_StackLine);
      StackLine_ *sl = (StackLine_ *)stackline->_internal_obj;
      sl->module = c->module;
      sl->func = (Function *)c->func;  // blessed
      const SourceMapping *sm =
          bytecode_get_source(c->bytecode, c->ins - ((c == ctx) ? 0 : 1));
      sl->error_token = (Token *)sm->token;                // blessed
      sl->source_error_token = (Token *)sm->source_token;  // blessed
      if (NULL != sl->source_error_token) {
        sl->source_linetext =
            bytecode_get_sourceline(c->bytecode, sl->source_error_token->line);
      } else {
        sl->source_linetext = NULL;
      }
      Value sl_e = value_object(stackline);
      array_add(frame->parent_isolate->heap, stacktrace, &sl_e);
    }
    t = t->parent_frame;
  }
  object_set_member_obj(frame->parent_isolate->heap, obj,
                        global_intern("stacktrace"), stacktrace);
  return value_object(obj);
}

Object *error_new(Frame *frame, Scope *ctx, Object *error_msg) {
  Value error_msg_e = value_object(error_msg);
  Object *err = heap_new(frame->parent_isolate->heap, Class_Error);
  error_constructor_(frame, ctx, err, &error_msg_e);
  return err;
}

void stackline_copy_(ValueCopier *copier, Object *src_obj, Object *target_obj) {
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
