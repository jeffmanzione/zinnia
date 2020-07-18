// class.h
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#ifndef OBJECT_CLASS_CLASS_H_
#define OBJECT_CLASS_CLASS_H_

#include "entity/object.h"

Class *class_init(Class *cls, const char name[], const Class *super,
                  const Module *module);
void class_finalize(Class *cls);
Function *class_add_function(Class *cls, const char name[], uint32_t ins_pos);

#endif /* OBJECT_CLASS_CLASS_H_ */