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

void init_classes(Heap* heap, Module* builtin);

#endif /* ENTITY_CLASS_CLASSES_ */