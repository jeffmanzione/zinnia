// intern.h
//
// Created on: Feb 11, 2018
//     Author: Jeff Manzione

#ifndef VM_INTERN_H_
#define VM_INTERN_H_

extern char *ARRAYLIKE_INDEX_KEY;
extern char *ARRAYLIKE_SET_KEY;
extern char *ARRAY_NAME;
extern char *CLASS_KEY;
extern char *CLASS_NAME;
extern char *CMP_FN_NAME;
extern char *CONSTRUCTOR_KEY;
extern char *EQ_FN_NAME;
extern char *ERROR_NAME;
extern char *FALSE_KEYWORD;
extern char *FUNCTION_NAME;
extern char *FUNCTION_REF_NAME;
extern char *FUTURE_NAME;
extern char *HAS_NEXT_FN_NAME;
extern char *HASH_KEY;
extern char *IN_FN_NAME;
extern char *ITER_FN_NAME;
extern char *MODULE_KEY;
extern char *MODULE_NAME;
extern char *NAME_KEY;
extern char *NEQ_FN_NAME;
extern char *NEXT_FN_NAME;
extern char *NIL_KEYWORD;
extern char *OBJECT_NAME;
extern char *OBJ_KEY;
extern char *PARENT_CLASS;
extern char *PROCESS_NAME;
extern char *RANGE_CLASS_NAME;
extern char *REMOTE_CLASS_NAME;
extern char *RESULT_VAL;
extern char *SELF;
extern char *STACKLINE_NAME;
extern char *STRING_NAME;
extern char *SUPER_KEY;
extern char *TASK_NAME;
extern char *TRUE_KEYWORD;
extern char *TUPLE_NAME;
extern char *VALUE_KEY;

void strings_init();
void strings_finalize();

#endif /* VM_INTERN_H_ */