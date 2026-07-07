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
} NativeFunctionContext;

typedef void (*NativeFunctionHandlerFn)(NativeFunctionContext *);

void NativeFunctionContext_init(NativeFunctionContext *fn_ctx, Task *task,
                                Context *ctx, const Entity *args);
const Entity *NativeFunctionContext_args(NativeFunctionContext *fn_ctx);
void NativeFunctionContext_raise_error(NativeFunctionContext *,
                                       const char fmt[], ...);
const Entity *NativeFunctionContext_get_retval(NativeFunctionContext *fn_ctx);
Entity *NativeFunctionContext_mutable_retval(NativeFunctionContext *);
void NativeFunctionContext_set_retval(NativeFunctionContext *, Entity *retval);
void NativeFunctionContext_set_retval_obj(NativeFunctionContext *,
                                          Object *retval);
Object *NativeFunctionContext_create_string(NativeFunctionContext *fn_ctx,
                                            const char src[], size_t len);

/* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_FUNCTION_CONTEXT_H_ */
#endif