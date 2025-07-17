// intern.h
//
// Created on: Feb 11, 2018
//     Author: Jeff Manzione

#include "vm/intern.h"

#include "alloc/arena/intern.h"

char *ADDRESS_INT_KEY;
char *ADDRESS_HEX_KEY;
char *ANNOTATE_KEY;
char *ANNOTATIONS_KEY;
char *ARRAYLIKE_INDEX_KEY;
char *ARRAYLIKE_SET_KEY;
char *ARRAY_NAME;
char *CLASS_KEY;
char *CLASS_NAME;
char *CMP_FN_NAME;
char *CONSTRUCTOR_KEY;
char *CONTEXT_NAME;
char *EQ_FN_NAME;
char *ERROR_NAME;
char *FALSE_KEYWORD;
char *FIELDS_KEY;
char *FIELDS_PRIVATE_KEY;
char *FUNCTION_NAME;
char *FUNCTION_REF_NAME;
char *FUTURE_NAME;
char *HAS_NEXT_FN_NAME;
char *HASH_KEY;
char *IN_FN_NAME;
char *ISTRING_NAME;
char *ITER_FN_NAME;
char *MAIN_KEY;
char *MODULE_KEY;
char *MODULE_NAME;
char *NAME_KEY;
char *NEQ_FN_NAME;
char *NEXT_FN_NAME;
char *NIL_KEYWORD;
char *OBJECT_NAME;
char *OBJ_KEY;
char *PARENT_CLASS;
char *PROCESS_NAME;
char *RANGE_CLASS_NAME;
char *REMOTE_CLASS_NAME;
char *RESULT_VAL;
char *SELF;
char *STACKLINE_NAME;
char *STRING_NAME;
char *SUPER_KEY;
char *TASK_NAME;
char *TMP_MODULE_HOLDER;
char *TRUE_KEYWORD;
char *TUPLE_NAME;
char *VALUE_KEY;

void _strings_insert_constants() {
  ADDRESS_INT_KEY = intern("$adr");
  ADDRESS_HEX_KEY = intern("$hex_addr");
  ANNOTATE_KEY = intern("annotate");
  ANNOTATIONS_KEY = intern("annotations");
  ARRAYLIKE_INDEX_KEY = intern("__index__");
  ARRAYLIKE_SET_KEY = intern("__set__");
  ARRAY_NAME = intern("Array");
  CLASS_KEY = intern("class");
  CLASS_NAME = intern("Class");
  CMP_FN_NAME = intern("__cmp__");
  CONSTRUCTOR_KEY = intern("new");
  CONTEXT_NAME = intern("Context");
  EQ_FN_NAME = intern("__eq__");
  ERROR_NAME = intern("Error");
  FALSE_KEYWORD = intern("False");
  FIELDS_PRIVATE_KEY = intern("_fields");
  FIELDS_KEY = intern("fields");
  FUNCTION_NAME = intern("Function");
  FUNCTION_REF_NAME = intern("FunctionRef");
  FUTURE_NAME = intern("Future");
  HAS_NEXT_FN_NAME = intern("has_next");
  HASH_KEY = intern("hash");
  IN_FN_NAME = intern("__in__");
  ISTRING_NAME = intern("IString");
  ITER_FN_NAME = intern("iter");
  MAIN_KEY = intern("__main");
  MODULE_KEY = intern("module");
  MODULE_NAME = intern("Module");
  NAME_KEY = intern("name");
  NEQ_FN_NAME = intern("__neq__");
  NEXT_FN_NAME = intern("next");
  NIL_KEYWORD = intern("None");
  OBJECT_NAME = intern("Object");
  OBJ_KEY = intern("obj");
  PARENT_CLASS = intern("parent_class");
  PROCESS_NAME = intern("Process");
  RANGE_CLASS_NAME = intern("Range");
  REMOTE_CLASS_NAME = intern("Remote");
  RESULT_VAL = intern("$resval");
  SELF = intern("self");
  STACKLINE_NAME = intern("StackLine");
  STRING_NAME = intern("String");
  SUPER_KEY = intern("super");
  TASK_NAME = intern("Task");
  TMP_MODULE_HOLDER = intern("$tmp_module_holder");
  TRUE_KEYWORD = intern("True");
  TUPLE_NAME = intern("Tuple");
  VALUE_KEY = intern("value");
}

void strings_init() {
  intern_init();
  _strings_insert_constants();
}

void strings_finalize() { intern_finalize(); }