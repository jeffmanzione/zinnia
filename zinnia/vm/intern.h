// intern.h
//
// Created on: Feb 11, 2018
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_INTERN_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_INTERN_H_

extern const char *ADDRESS_INT_KEY;
extern const char *ADDRESS_HEX_KEY;
extern const char *ANNOTATE_KEY;
extern const char *ANNOTATIONS_KEY;
extern const char *ARRAYLIKE_INDEX_KEY;
extern const char *ARRAYLIKE_SET_KEY;
extern const char *ARRAY_NAME;
extern const char *CLASS_KEY;
extern const char *CLASS_NAME;
extern const char *CMP_FN_NAME;
extern const char *CONSTRUCTOR_KEY;
extern const char *CONTEXT_NAME;
extern const char *EQ_FN_NAME;
extern const char *ERROR_NAME;
extern const char *FALSE_KEYWORD;
extern const char *FIELDS_PRIVATE_KEY;
extern const char *FIELDS_KEY;
extern const char *FUNCTION_NAME;
extern const char *FUNCTION_REF_NAME;
extern const char *FUTURE_NAME;
extern const char *HAS_NEXT_FN_NAME;
extern const char *HASH_KEY;
extern const char *IN_FN_NAME;
extern const char *ISTRING_NAME;
extern const char *ITER_FN_NAME;
extern const char *MAIN_KEY;
extern const char *MODULE_KEY;
extern const char *MODULE_NAME;
extern const char *NAME_KEY;
extern const char *NEQ_FN_NAME;
extern const char *NEXT_FN_NAME;
extern const char *NIL_KEYWORD;
extern const char *OBJECT_NAME;
extern const char *OBJ_KEY;
extern const char *PARENT_CLASS;
extern const char *PROCESS_NAME;
extern const char *RANGE_CLASS_NAME;
extern const char *REMOTE_CLASS_NAME;
extern const char *RESULT_VAL;
extern const char *SELF;
extern const char *STACKLINE_NAME;
extern const char *STRING_NAME;
extern const char *SUPER_KEY;
extern const char *TASK_NAME;
extern const char *TMP_MODULE_HOLDER;
extern const char *TO_S_KEY;
extern const char *TRUE_KEYWORD;
extern const char *TUPLE_NAME;
extern const char *VALUE_KEY;

void strings_init();
void strings_finalize();

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_INTERN_H_ */