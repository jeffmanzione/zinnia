// object.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_OBJECT_H_
#define ENTITY_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "alloc/memory_graph/memory_graph.h"
#include "lang/lexer/token.h"
#include "program/tape.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"
#include "struct/map.h"

typedef struct _Object Object;
typedef struct _Class Class;
typedef struct _Module Module;
typedef struct _Function Function;

typedef void (*ObjDelFn)(Object *);
typedef void (*ObjInitFn)(Object *);
// TODO: This should only be temporary until to_s() is supported.
typedef void (*ObjPrintFn)(const Object *, FILE *);

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
    void *_internal_obj;
  };
};

struct _Class {
  Object *_reflection;

  const char *_name;
  const Class *_super;
  const Module *_module;
  KeyedList _functions;
  ObjInitFn _init_fn;
  ObjDelFn _delete_fn;
  ObjPrintFn _print_fn;
};

struct _Module {
  Object *_reflection;

  const char *_name;
  const Tape *_tape;
  KeyedList _classes;
  KeyedList _functions;
  bool _is_initialized;
};

struct _Function {
  Object *_reflection;

  const char *_name;
  const Module *_module;
  const Class *_parent_class;

  bool _is_native;
  bool _is_anon;
  bool _is_async;
  bool _is_const;
  union {
    uint32_t _ins_pos;
    void *_native_fn; // NativeFn
  };
};

#endif /* ENTITY_OBJECT_H_ */