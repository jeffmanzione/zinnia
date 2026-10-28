#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_

#include "zinnia/entity/object.h"
#include "zinnia/entity/value.h"
#include "zinnia/vm/isolate/isolates.h"

// Extra
#include "zinnia/entity/string/string_helper.h"

typedef struct {
  Frame *frame_;
  Scope *ctx_;
  const Value *args_;
  Value retval_;
} FunctionContext;

typedef void (*NativeFunctionHandlerFn)(FunctionContext *);

void FunctionContext_init(FunctionContext *fn_ctx, Frame *frame, Scope *ctx,
                          const Value *args);
const Value *FunctionContext_args(FunctionContext *fn_ctx);
void FunctionContext_raise_error(FunctionContext *, const char fmt[], ...);
const Value *FunctionContext_get_retval(FunctionContext *fn_ctx);
Value *FunctionContext_mutable_retval(FunctionContext *);
void FunctionContext_set_retval(FunctionContext *, Value *retval);
void FunctionContext_set_retval_obj(FunctionContext *, Object *retval);
Object *FunctionContext_create_string(FunctionContext *fn_ctx, const char src[],
                                      size_t len);
Object *FunctionContext_create_tuple(FunctionContext *fn_ctx, size_t size, ...);

/* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_ */
#endif