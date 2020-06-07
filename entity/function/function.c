// func.c
//
// Created on: May 31, 2020
//     Author: Jeff Manzione

#include "entity/function/function.h"

#include "debug/debug.h"
#include "entity/object.h"

inline void function_init(Function *f, const char name[],
                          const Module *module) {
  ASSERT(NOT_NULL(f), NOT_NULL(name));
  f->_name = name;
  f->_module = module;
}

inline void function_finalize(Function *f) { ASSERT(NOT_NULL(f)); }