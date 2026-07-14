#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_

#include "zinnia/entity/entity.h"
#include "zinnia/entity/object.h"
#include "zinnia/vm/process/processes.h"

// Extra
#include "zinnia/entity/string/string_helper.h"

typedef struct {
  Task *task_;
  Context *ctx_;
  const Entity *args_;
  Entity retval_;
} FunctionContext;

typedef void (*NativeFunctionHandlerFn)(FunctionContext *);

void FunctionContext_init(FunctionContext *fn_ctx, Task *task, Context *ctx,
                          const Entity *args);
const Entity *FunctionContext_args(FunctionContext *fn_ctx);
void FunctionContext_raise_error(FunctionContext *, const char fmt[], ...);
const Entity *FunctionContext_get_retval(FunctionContext *fn_ctx);
Entity *FunctionContext_mutable_retval(FunctionContext *);
void FunctionContext_set_retval(FunctionContext *, Entity *retval);
void FunctionContext_set_retval_obj(FunctionContext *, Object *retval);
Object *FunctionContext_create_string(FunctionContext *fn_ctx, const char src[],
                                      size_t len);
Object *FunctionContext_create_tuple(FunctionContext *fn_ctx, size_t size, ...);

/* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_ */
#endif