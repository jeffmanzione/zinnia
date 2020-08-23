// classes.c
//
// Created on: Jul 12, 2020
//     Author: Jeff Manzione

#include "entity/class/classes.h"

#include "debug/debug.h"
#include "entity/array/array.h"
#include "entity/class/class.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "heap/heap.h"
#include "vm/intern.h"

Class *Class_Object;
Class *Class_Class;
Class *Class_Function;
Class *Class_Module;
Class *Class_Array;
// Class *Class_Error;

void init_classes(Heap *heap, Module *builtin) {
  Class_Object = module_add_class(builtin, OBJECT_NAME);
  Class_Class = module_add_class(builtin, CLASS_NAME);
  Class_Function = module_add_class(builtin, FUNCTION_NAME);
  Class_Module = module_add_class(builtin, MODULE_NAME);
  Class_Array = module_add_class(builtin, ARRAY_NAME);
  // Class_Error = module_add_class(builtin, ERROR_NAME);

  Class_Class->_super = NULL;
  Class_Object->_reflection = heap_new(heap, Class_Class);
  Class_Object->_reflection->_class_obj = Class_Object;

  Class_Class->_super = Class_Object;
  Class_Class->_reflection = heap_new(heap, Class_Class);
  Class_Class->_reflection->_class_obj = Class_Class;

  Class_Function->_super = Class_Object;
  Class_Function->_reflection = heap_new(heap, Class_Class);
  Class_Function->_reflection->_class_obj = Class_Function;

  Class_Module->_super = Class_Object;
  Class_Module->_reflection = heap_new(heap, Class_Class);
  Class_Module->_reflection->_class_obj = Class_Module;

  Class_Array->_super = Class_Object;
  Class_Array->_reflection = heap_new(heap, Class_Class);
  Class_Array->_reflection->_class_obj = Class_Array;
  Class_Array->_init_fn = __array_init;
  Class_Array->_delete_fn = __array_delete;
  Class_Array->_print_fn = __array_print;

  // Class_Error->_super = Class_Object;
  // Class_Error->_reflection = heap_new(heap, Class_Class);
  // Class_Error->_reflection->_class_obj = Class_Error;
}