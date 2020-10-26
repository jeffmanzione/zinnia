// intern.h
//
// Created on: Feb 11, 2018
//     Author: Jeff Manzione

#include "vm/intern.h"

#include "alloc/arena/intern.h"

char *ADDRESS_KEY;
char *ANON_FUNCTION_NAME;
char *ARGS_KEY;
char *ARGS_NAME;
char *ARRAYLIKE_INDEX_KEY;
char *ARRAYLIKE_SET_KEY;
char *ARRAY_NAME;
char *BUILTIN_MODULE_NAME;
char *CALLER_KEY;
char *CALL_KEY;
char *CLASSES_KEY;
char *CLASS_KEY;
char *CLASS_NAME;
char *CMP_FN_NAME;
char *CONSTRUCTOR_KEY;
char *CURRENT_BLOCK;
char *DECONSTRUCTOR_KEY;
char *EMPTY_TUPLE_KEY;
char *EQ_FN_NAME;
char *ERROR_KEY;
char *ERROR_NAME;
char *EXTERNAL_FUNCTION_NAME;
char *EXTERNAL_METHOD_NAME;
char *EXTERNAL_METHODINSTANCE_NAME;
char *FALSE_KEYWORD;
char *FILE_NAME;
char *FUNCTION_NAME;
char *FUNCTION_REF_NAME;
char *FUNCTIONS_KEY;
char *HAS_NEXT_FN_NAME;
char *HASH_KEY;
char *INITIALIZED;
char *INS_INDEX;
char *IN_FN_NAME;
char *IP_FIELD;
char *IS_ANONYMOUS;
char *IS_EXTERNAL_KEY;
char *IS_ITERATOR_BLOCK_KEY;
char *ITER_FN_NAME;
char *LENGTH_KEY;
char *METHODS_KEY;
char *METHOD_INSTANCE_NAME;
char *METHOD_KEY;
char *METHOD_NAME;
char *MODULES;
char *MODULE_FIELD;
char *MODULE_KEY;
char *MODULE_NAME;
char *NAME_KEY;
char *NEQ_FN_NAME;
char *NEXT_FN_NAME;
char *NIL_KEYWORD;
char *OBJECT_NAME;
char *OBJ_KEY;
char *OLD_RESVALS;
char *PARENT;
char *PARENTS_KEY;
char *PARENT_CLASS;
char *PARENT_MODULE;
char *RANGE_CLASS_NAME;
char *RESULT_VAL;
char *ROOT;
char *SAVED_BLOCKS;
char *SELF;
char *STACK;
char *STACKLINE_NAME;
char *STACK_SIZE_NAME;
char *STRING_NAME;
char *SUPER_KEY;
char *THREADS_KEY;
char *TMP_VAL;
char *TRUE_KEYWORD;
char *TUPLE_NAME;

void _strings_insert_constants() {
  ADDRESS_KEY = intern("$adr");
  ANON_FUNCTION_NAME = intern("AnonymousFunction");
  ARGS_KEY = intern("$args");
  ARGS_NAME = intern("args");
  ARRAYLIKE_INDEX_KEY = intern("__index__");
  ARRAYLIKE_SET_KEY = intern("__set__");
  ARRAY_NAME = intern("Array");
  BUILTIN_MODULE_NAME = intern("builtin");
  CALLER_KEY = intern("$caller");
  CALL_KEY = intern("call");
  CLASSES_KEY = intern("classes");
  CLASS_KEY = intern("class");
  CLASS_NAME = intern("Class");
  CMP_FN_NAME = intern("__cmp__");
  CONSTRUCTOR_KEY = intern("new");
  CURRENT_BLOCK = intern("$block");
  EMPTY_TUPLE_KEY = intern("$empty_tuple");
  DECONSTRUCTOR_KEY = intern("$deconstructor");
  EQ_FN_NAME = intern("__eq__");
  ERROR_KEY = intern("$has_error");
  ERROR_NAME = intern("Error");
  EXTERNAL_FUNCTION_NAME = intern("ExternalFunction");
  EXTERNAL_METHOD_NAME = intern("ExternalMethod");
  EXTERNAL_METHODINSTANCE_NAME = intern("ExternalMethodInstance");
  FALSE_KEYWORD = intern("False");
  FILE_NAME = intern("_File");
  FUNCTION_NAME = intern("Function");
  FUNCTION_REF_NAME = intern("FunctionRef");
  FUNCTIONS_KEY = intern("functions");
  HAS_NEXT_FN_NAME = intern("has_next");
  HASH_KEY = intern("hash");
  INITIALIZED = intern("$initialized");
  INS_INDEX = intern("$ins");
  IN_FN_NAME = intern("__in__");
  IP_FIELD = intern("$ip");
  IS_ANONYMOUS = intern("$is_anonymous");
  IS_EXTERNAL_KEY = intern("$is_external");
  IS_ITERATOR_BLOCK_KEY = intern("$is_iterator_block");
  ITER_FN_NAME = intern("iter");
  LENGTH_KEY = intern("len");
  METHODS_KEY = intern("$methods");
  METHOD_INSTANCE_NAME = intern("MethodInstance");
  METHOD_KEY = intern("$method");
  METHOD_NAME = intern("Method");
  MODULES = intern("$modules");
  MODULE_FIELD = intern("$module");
  MODULE_KEY = intern("module");
  MODULE_NAME = intern("Module");
  NAME_KEY = intern("name");
  NEQ_FN_NAME = intern("__neq__");
  NEXT_FN_NAME = intern("next");
  NIL_KEYWORD = intern("None");
  OBJECT_NAME = intern("Object");
  OBJ_KEY = intern("obj");
  OLD_RESVALS = intern("$old_resvals");
  PARENT = intern("$parent");
  PARENTS_KEY = intern("parents");
  PARENT_CLASS = intern("parent_class");
  PARENT_MODULE = intern("module");
  RANGE_CLASS_NAME = intern("Range");
  RESULT_VAL = intern("$resval");
  ROOT = intern("$root");
  SAVED_BLOCKS = intern("$saved_blocks");
  SELF = intern("self");
  STACK = intern("$stack");
  STACKLINE_NAME = intern("StackLine");
  STACK_SIZE_NAME = intern("$stack_size");
  STRING_NAME = intern("String");
  SUPER_KEY = intern("super");
  THREADS_KEY = intern("$threads");
  TMP_VAL = intern("$tmp");
  TRUE_KEYWORD = intern("True");
  TUPLE_NAME = intern("Tuple");
}

void strings_init() {
  intern_init();
  _strings_insert_constants();
}

void strings_finalize() { intern_finalize(); }