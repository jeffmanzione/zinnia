#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_

#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/native/builder/builder.h"
#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/util/platform.h"

#ifdef OS_WINDOWS
#define NATIVE_FN __declspec(dllexport)
#else
#define NATIVE_FN
#endif

#define EXTRACT_TUPLE_ARGS2(var, fn_ctx, num_args)                          \
  const Entity *args = FunctionContext_args(fn_ctx);                        \
  if (!IS_TUPLE(args)) {                                                    \
    FunctionContext_raise_error(fn_ctx, "Expected tuple(%d) argument",      \
                                num_args);                                  \
    return;                                                                 \
  }                                                                         \
  const Tuple *var = (Tuple *)args->obj->_internal_obj;                     \
  if (tuple_size(var) != num_args) {                                        \
    FunctionContext_raise_error(fn_ctx,                                     \
                                "Expects tuple(%d) but received tuple(%d)", \
                                num_args, tuple_size(var));                 \
    return;                                                                 \
  }

#define EXTRACT_INT_AT_INDEX_OR_THROW2(var, fn_ctx, tuple, index)     \
  if (!IS_INT(tuple_get(tuple, index))) {                             \
    FunctionContext_raise_error(                                      \
        fn_ctx, "Expected argument at index %d to be an Int", index); \
    return;                                                           \
  }                                                                   \
  var = int_of(&tuple_get(tuple, index)->pri);

#define EXTRACT_FLOAT_AT_INDEX_OR_THROW2(var, fn_ctx, tuple, index)     \
  if (!IS_FLOAT(tuple_get(tuple, index))) {                             \
    FunctionContext_raise_error(                                        \
        fn_ctx, "Expected argument at index %d to be an Float", index); \
    return;                                                             \
  }                                                                     \
  var = float_of(&tuple_get(tuple, index)->pri);

#define EXTRACT_BOOL_AT_INDEX_OR_THROW2(var, fn_ctx, tuple, index)     \
  if (!IS_BOOL(tuple_get(tuple, index))) {                             \
    FunctionContext_raise_error(                                       \
        fn_ctx, "Expected argument at index %d to be an Bool", index); \
    return;                                                            \
  }                                                                    \
  var = bool_of(&tuple_get(tuple, index)->pri);

#define EXCTRACT_STRING_OR_THROW2(str, str_len, fn_ctx, entity) \
  char *str;                                                    \
  int str_len;                                                  \
  if (!extract_string(entity, &str, &str_len)) {                \
    FunctionContext_raise_error(fn_ctx, "Must be a string");    \
    return;                                                     \
  }

#define EXCTRACT_STRING_AT_INDEX_OR_THROW2(str, str_len, fn_ctx, tuple, index) \
  char *str;                                                                   \
  int str_len;                                                                 \
  if (!extract_string(tuple_get(tuple, index), &str, &str_len)) {              \
    FunctionContext_raise_error(fn_ctx, "Must be a string");                   \
    return;                                                                    \
  }

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_ */