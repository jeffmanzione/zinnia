// intern.h
//
// Created on: Feb 11, 2018
//     Author: Jeff Manzione

#include "zinnia/vm/intern.h"

const char *ADDRESS_INT_KEY;
const char *ADDRESS_HEX_KEY;
const char *ANNOTATE_KEY;
const char *ANNOTATIONS_KEY;
const char *ARRAYLIKE_INDEX_KEY;
const char *ARRAYLIKE_SET_KEY;
const char *ARRAY_NAME;
const char *CLASS_KEY;
const char *CLASS_NAME;
const char *CMP_FN_NAME;
const char *CONSTRUCTOR_KEY;
const char *CONTEXT_NAME;
const char *EQ_FN_NAME;
const char *ERROR_NAME;
const char *FALSE_KEYWORD;
const char *FIELDS_KEY;
const char *FIELDS_PRIVATE_KEY;
const char *FUNCTION_NAME;
const char *FUNCTION_REF_NAME;
const char *FUTURE_NAME;
const char *HAS_NEXT_FN_NAME;
const char *HASH_KEY;
const char *IN_FN_NAME;
const char *ISTRING_NAME;
const char *ITER_FN_NAME;
const char *MAIN_KEY;
const char *MODULE_KEY;
const char *MODULE_NAME;
const char *NAME_KEY;
const char *NEQ_FN_NAME;
const char *NEXT_FN_NAME;
const char *NIL_KEYWORD;
const char *OBJECT_NAME;
const char *OBJ_KEY;
const char *PARENT_CLASS;
const char *PROCESS_NAME;
const char *RANGE_CLASS_NAME;
const char *REMOTE_CLASS_NAME;
const char *RESULT_VAL;
const char *SELF;
const char *STACKLINE_NAME;
const char *STRING_NAME;
const char *SUPER_KEY;
const char *TASK_NAME;
const char *TMP_MODULE_HOLDER;
const char *TO_S_KEY;
const char *TRUE_KEYWORD;
const char *TUPLE_NAME;
const char *VALUE_KEY;

void strings_insert_constants_() {
  ADDRESS_INT_KEY = global_intern("$adr");
  ADDRESS_HEX_KEY = global_intern("$hex_addr");
  ANNOTATE_KEY = global_intern("annotate");
  ANNOTATIONS_KEY = global_intern("annotations");
  ARRAYLIKE_INDEX_KEY = global_intern("__index__");
  ARRAYLIKE_SET_KEY = global_intern("__set__");
  ARRAY_NAME = global_intern("Array");
  CLASS_KEY = global_intern("class");
  CLASS_NAME = global_intern("Class");
  CMP_FN_NAME = global_intern("__cmp__");
  CONSTRUCTOR_KEY = global_intern("new");
  CONTEXT_NAME = global_intern("Context");
  EQ_FN_NAME = global_intern("__eq__");
  ERROR_NAME = global_intern("Error");
  FALSE_KEYWORD = global_intern("False");
  FIELDS_PRIVATE_KEY = global_intern("_fields");
  FIELDS_KEY = global_intern("fields");
  FUNCTION_NAME = global_intern("Function");
  FUNCTION_REF_NAME = global_intern("FunctionRef");
  FUTURE_NAME = global_intern("Future");
  HAS_NEXT_FN_NAME = global_intern("has_next");
  HASH_KEY = global_intern("hash");
  IN_FN_NAME = global_intern("__in__");
  ISTRING_NAME = global_intern("IString");
  ITER_FN_NAME = global_intern("iter");
  MAIN_KEY = global_intern("__main");
  MODULE_KEY = global_intern("module");
  MODULE_NAME = global_intern("Module");
  NAME_KEY = global_intern("name");
  NEQ_FN_NAME = global_intern("__neq__");
  NEXT_FN_NAME = global_intern("next");
  NIL_KEYWORD = global_intern("None");
  OBJECT_NAME = global_intern("Object");
  OBJ_KEY = global_intern("obj");
  PARENT_CLASS = global_intern("parent_class");
  PROCESS_NAME = global_intern("Process");
  RANGE_CLASS_NAME = global_intern("Range");
  REMOTE_CLASS_NAME = global_intern("Remote");
  RESULT_VAL = global_intern("$resval");
  SELF = global_intern("self");
  STACKLINE_NAME = global_intern("StackLine");
  STRING_NAME = global_intern("String");
  SUPER_KEY = global_intern("super");
  TASK_NAME = global_intern("Task");
  TMP_MODULE_HOLDER = global_intern("$tmp_module_holder");
  TO_S_KEY = global_intern("to_s");
  TRUE_KEYWORD = global_intern("True");
  TUPLE_NAME = global_intern("Tuple");
  VALUE_KEY = global_intern("value");
}

void strings_init() {
  global_string_intern_pool_init();
  strings_insert_constants_();
}

void strings_finalize() { global_string_intern_pool_finalize(); }