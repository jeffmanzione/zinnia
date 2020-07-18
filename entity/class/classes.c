// classes.c
//
// Created on: Jul 12, 2020
//     Author: Jeff Manzione

#include "entity/class/classes.h"

#include "debug/debug.h"
#include "entity/class/class.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "vm/intern.h"

Class *Class_Object;
Class *Class_Class;
Class *Class_Function;

void init_classes(Heap *heap, Module *builtin) {
  Class_Object = module_add_class(builtin, OBJECT_NAME);
  Class_Object->_reflection = heap_new(heap, Class_Object);
  Class_Object->_reflection->_class_obj = Class_Object;

  Class_Class = module_add_class(builtin, CLASS_NAME);
  Class_Class->_super = Class_Object;
  Class_Class->_reflection = heap_new(heap, Class_Object);
  Class_Class->_reflection->_class_obj = Class_Class;
}