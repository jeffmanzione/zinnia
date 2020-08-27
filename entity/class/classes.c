// classes.c
//
// Created on: Jul 12, 2020
//     Author: Jeff Manzione

#include "entity/class/classes.h"

#include "debug/debug.h"
#include "entity/array/array.h"
#include "entity/class/class.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "vm/intern.h"

Class *Class_Object;
Class *Class_Class;
Class *Class_Function;
Class *Class_FunctionRef;
Class *Class_Module;
Class *Class_Array;
Class *Class_String;
Class *Class_Tuple;
// Class *Class_Error;

void builtin_classes(Heap *heap, Module *builtin) {
  Class_Object = module_add_class(builtin, OBJECT_NAME, NULL);
  Class_Class = module_add_class(builtin, CLASS_NAME, Class_Object);
  Class_Function = module_add_class(builtin, FUNCTION_NAME, Class_Object);
  Class_FunctionRef =
      module_add_class(builtin, FUNCTION_REF_NAME, Class_Object);
  Class_Module = module_add_class(builtin, MODULE_NAME, Class_Object);
  Class_Array = module_add_class(builtin, ARRAY_NAME, Class_Object);
  Class_String = module_add_class(builtin, STRING_NAME, Class_Object);
  Class_Tuple = module_add_class(builtin, TUPLE_NAME, Class_Object);
  // Class_Error = module_add_class(builtin, ERROR_NAME);

  Class_Object->_super = NULL;
  Class_Object->_reflection = heap_new(heap, Class_Class);
  Class_Object->_reflection->_class_obj = Class_Object;

  Class_Class->_super = Class_Object;
  Class_Class->_reflection = heap_new(heap, Class_Class);
  Class_Class->_reflection->_class_obj = Class_Class;

  Class_Function->_super = Class_Object;
  Class_Function->_reflection = heap_new(heap, Class_Class);
  Class_Function->_reflection->_class_obj = Class_Function;

  Class_FunctionRef->_super = Class_Object;
  Class_FunctionRef->_reflection = heap_new(heap, Class_Class);
  Class_FunctionRef->_reflection->_class_obj = Class_FunctionRef;
  Class_FunctionRef->_init_fn = __function_ref_create;
  Class_FunctionRef->_delete_fn = __function_ref_delete;
  Class_FunctionRef->_print_fn = __function_ref_print;

  Class_Module->_super = Class_Object;
  Class_Module->_reflection = heap_new(heap, Class_Class);
  Class_Module->_reflection->_class_obj = Class_Module;

  Class_Array->_super = Class_Object;
  Class_Array->_reflection = heap_new(heap, Class_Class);
  Class_Array->_reflection->_class_obj = Class_Array;
  Class_Array->_init_fn = __array_init;
  Class_Array->_delete_fn = __array_delete;
  Class_Array->_print_fn = __array_print;

  Class_String->_super = Class_Object;
  Class_String->_reflection = heap_new(heap, Class_Class);
  Class_String->_reflection->_class_obj = Class_String;
  Class_String->_init_fn = __string_create;
  Class_String->_delete_fn = __string_delete;
  Class_String->_print_fn = __string_print;

  Class_Tuple->_super = Class_Object;
  Class_Tuple->_reflection = heap_new(heap, Class_Class);
  Class_Tuple->_reflection->_class_obj = Class_Tuple;
  Class_Tuple->_init_fn = __tuple_create;
  Class_Tuple->_delete_fn = __tuple_delete;
  Class_Tuple->_print_fn = __tuple_print;

  // Class_Error->_super = Class_Object;
  // Class_Error->_reflection = heap_new(heap, Class_Class);
  // Class_Error->_reflection->_class_obj = Class_Error;
}