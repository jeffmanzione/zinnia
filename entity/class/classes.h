// classes.h
//
// Created on: Jul 12, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_CLASS_CLASSES_
#define ENTITY_CLASS_CLASSES_

#include "entity/class/class.h"
#include "entity/object.h"
#include "heap/heap.h"

extern Class* Class_Object;
extern Class* Class_Class;
extern Class* Class_Function;
extern Class* Class_FunctionRef;
extern Class* Class_Module;
extern Class* Class_Array;
extern Class* Class_String;
extern Class* Class_Tuple;
extern Class* Class_Error;

void builtin_classes(Heap* heap, Module* builtin);

#endif /* ENTITY_CLASS_CLASSES_ */