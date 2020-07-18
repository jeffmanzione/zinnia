// object.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef OBJECT_OBJECT_H_
#define OBJECT_OBJECT_H_

#include <stdint.h>

#include "alloc/memory_graph/memory_graph.h"
#include "program/tape.h"
#include "struct/keyed_list.h"
#include "struct/map.h"

typedef struct _Object Object;
typedef struct _Class Class;
typedef struct _Module Module;
typedef struct _Function Function;

// Represents an object with properties.
struct _Object {
  const Node *_node_ref;
  const Class *_class;
  KeyedList _members;

  // If the object is reflected.
  union {
    Module *_module_obj;
    Class *_class_obj;
    Function *_function_obj;
  };
};

struct _Class {
  Object *_reflection;

  const char *_name;
  const Class *_super;
  const Module *_module;
  KeyedList _functions;
};

struct _Module {
  Object *_reflection;

  const char *_name;
  const Tape *_tape;
  KeyedList _classes;
  KeyedList _functions;
};

struct _Function {
  Object *_reflection;

  const char *_name;
  const Module *_module;
  uint32_t _ins_pos;
};

#endif /* OBJECT_OBJECT_H_ */