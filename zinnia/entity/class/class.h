// class.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_CLASS_CLASS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_CLASS_CLASS_H_

#include "zinnia/entity/object.h"

Class *class_init(Class *cls, const char name[], const Class *super,
                  const Module *module);
void class_finalize(Class *cls);
Function *class_add_function(Class *cls, const char name[], uint32_t ins_pos,
                             bool is_const, bool is_async);
Field *class_add_field(Class *cls, const char name[]);

FunctionMapIterator class_functions(Class *cls);
FieldMapIterator class_fields(Class *cls);

// TODO: Consider merging these 2 functions.
const Function *class_get_function(const Class *cls, const char name[]);
// const FunctionRef *class_get_function_ref(const Class *cls, const char
// name[]);

bool inherits_from(const Class *class, Class *possible_super);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_CLASS_CLASS_H_ */