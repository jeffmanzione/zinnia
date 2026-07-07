#include "zinnia/entity/native/builder/function_context.h"

#include <stdarg.h>

#include "zinnia/entity/native/error.h"

void NativeFunctionContext_init(NativeFunctionContext *fn_ctx, Task *task,
                                Context *ctx, const Entity *args) {
  fn_ctx->task_ = task;
  fn_ctx->ctx_ = ctx;
  fn_ctx->args_ = args;
  fn_ctx->retval_ = NONE_ENTITY;
}

const Entity *NativeFunctionContext_args(NativeFunctionContext *fn_ctx) {
  return fn_ctx->args_;
}

void NativeFunctionContext_raise_error(NativeFunctionContext *fn_ctx,
                                       const char fmt[], ...) {
  va_list args;
  va_start(args, fmt);
  fn_ctx->retval_ = raise_errorv(fn_ctx->task_, fn_ctx->ctx_, fmt, args);
  va_end(args);
}

Entity *NativeFunctionContext_mutable_retval(NativeFunctionContext *fn_ctx) {
  return &fn_ctx->retval_;
}

void NativeFunctionContext_set_retval(NativeFunctionContext *fn_ctx,
                                      Entity *retval) {
  fn_ctx->retval_ = *retval;
}

void NativeFunctionContext_set_retval_obj(NativeFunctionContext *fn_ctx,
                                          Object *retval) {
  fn_ctx->retval_ = entity_object(retval);
}

const Entity *NativeFunctionContext_get_retval(NativeFunctionContext *fn_ctx) {
  return &fn_ctx->retval_;
}

Object *NativeFunctionContext_create_string(NativeFunctionContext *fn_ctx,
                                            const char src[], size_t len) {
  return string_new(fn_ctx->task_->parent_process->heap, src, len);
}