#include "zinnia/entity/native/builder/function_context.h"

#include <stdarg.h>

#include "zinnia/entity/native/error.h"

void FunctionContext_init(FunctionContext *fn_ctx, Task *task, Context *ctx,
                          const Entity *args) {
  fn_ctx->task_ = task;
  fn_ctx->ctx_ = ctx;
  fn_ctx->args_ = args;
  fn_ctx->retval_ = NONE_ENTITY;
}

const Entity *FunctionContext_args(FunctionContext *fn_ctx) {
  return fn_ctx->args_;
}

void FunctionContext_raise_error(FunctionContext *fn_ctx, const char fmt[],
                                 ...) {
  va_list args;
  va_start(args, fmt);
  fn_ctx->retval_ = raise_errorv(fn_ctx->task_, fn_ctx->ctx_, fmt, args);
  va_end(args);
}

Entity *FunctionContext_mutable_retval(FunctionContext *fn_ctx) {
  return &fn_ctx->retval_;
}

void FunctionContext_set_retval(FunctionContext *fn_ctx, Entity *retval) {
  fn_ctx->retval_ = *retval;
}

void FunctionContext_set_retval_obj(FunctionContext *fn_ctx, Object *retval) {
  fn_ctx->retval_ = entity_object(retval);
}

const Entity *FunctionContext_get_retval(FunctionContext *fn_ctx) {
  return &fn_ctx->retval_;
}

Object *FunctionContext_create_string(FunctionContext *fn_ctx, const char src[],
                                      size_t len) {
  return string_new(fn_ctx->task_->parent_process->heap, src, len);
}