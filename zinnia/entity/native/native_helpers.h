#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_HELPERS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_HELPERS_H_

#include "zinnia/entity/array/array.h"
#include "zinnia/entity/entity.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/tuple/tuple.h"

#define EXTRACT_TUPLE_ARGS(var, args, num_args, task, ctx)                    \
  if (!IS_TUPLE(args)) {                                                      \
    return raise_error(task, ctx, "Expected tuple(%d) argument", num_args);   \
  }                                                                           \
  const Tuple *var = (Tuple *)args->obj->_internal_obj;                       \
  if (tuple_size(var) != num_args) {                                          \
    return raise_error(task, ctx, "Expects tuple(%d) but received tuple(%d)", \
                       num_args, tuple_size(var));                            \
  }

#define EXTRACT_INT_AT_INDEX_OR_THROW(var, tuple, index)                     \
  if (!IS_INT(tuple_get(tuple, index))) {                                    \
    return raise_error(task, ctx,                                            \
                       "Expected argument at index %d to be an Int", index); \
  }                                                                          \
  var = int_of(&tuple_get(tuple, index)->pri);

#define EXTRACT_FLOAT_AT_INDEX_OR_THROW(var, tuple, index)                     \
  if (!IS_FLOAT(tuple_get(tuple, index))) {                                    \
    return raise_error(task, ctx,                                              \
                       "Expected argument at index %d to be an Float", index); \
  }                                                                            \
  var = float_of(&tuple_get(tuple, index)->pri);

#define EXTRACT_BOOL_AT_INDEX_OR_THROW(var, tuple, index)                     \
  if (!IS_BOOL(tuple_get(tuple, index))) {                                    \
    return raise_error(task, ctx,                                             \
                       "Expected argument at index %d to be an Bool", index); \
  }                                                                           \
  var = bool_of(&tuple_get(tuple, index)->pri);

#define EXCTRACT_STRING_OR_THROW(str, str_len, entity) \
  char *str;                                           \
  int str_len;                                         \
  if (!extract_string(entity, &str, &str_len)) {       \
    return raise_error(task, ctx, "Must be a string"); \
  }

#define EXCTRACT_STRING_AT_INDEX_OR_THROW(str, str_len, tuple, index) \
  char *str;                                                          \
  int str_len;                                                        \
  if (!extract_string(tuple_get(tuple, index), &str, &str_len)) {     \
    return raise_error(task, ctx, "Must be a string");                \
  }

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_NATIVE_HELPERS_H_ */