// object.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_OBJECT_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "c-data-structures/arraylike.h"
#include "c-data-structures/maplike.h"
#include "c-data-structures/stable_maplike.h"
#include "memory-wrapper/memory_graph.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/dll.h"
#include "zinnia/util/sync/mutex.h"


typedef struct Object_ Object;
typedef struct Class_ Class;
typedef struct Module_ Module;
typedef struct Function_ Function;
typedef struct EntityCopier_ EntityCopier;
typedef struct Entity_ Entity;
typedef struct Field_ Field;

typedef void (*ObjDelFn)(Object *);
typedef void (*ObjInitFn)(Object *);
// TODO: This should only be temporary until to_s() is supported.
typedef void (*ObjPrintFn)(const Object *, FILE *);
typedef void (*ObjCopyFn)(EntityCopier *copier, Object *src, Object *target);

DEFINE_STABLE_MAPLIKE(EntityMap, char *, Entity);
DEFINE_STABLE_MAPLIKE(ClassMap, char *, Class);
DEFINE_STABLE_MAPLIKE(FieldMap, char *, Field);
DEFINE_STABLE_MAPLIKE(FunctionMap, char *, Function);

// Represents an object with properties.
struct Object_ {
  const Node *_node_ref;
  const Class *_class;
  EntityMap _members;  // Member

  // If the object is reflected.
  union {
    Module *_module_obj;
    Class *_class_obj;
    Function *_function_obj;
    void *_internal_obj;
  };
};

struct Field_ {
  const char *name;
};

struct Class_ {
  Object *_reflection;

  const char *_name;
  const Class *_super;
  const Module *_module;
  FieldMap _fields;
  FunctionMap _functions;
  ObjInitFn _init_fn;
  ObjDelFn _delete_fn;
  ObjPrintFn _print_fn;
  ObjCopyFn _copy_fn;
};

struct Module_ {
  Object *_reflection;

  const char *_name;
  const Tape *_tape;
  ClassMap _classes;
  FunctionMap _functions;
  bool _is_initialized;
  Mutex _write_mutex;

  const char *_full_path;
  const char *_relative_path;
  const char *_key;

  DlHandle dl;
};

struct Function_ {
  Object *_reflection;

  const char *_name;
  const Module *_module;
  const Class *_parent_class;

  bool _is_native;
  bool _is_native2;
  bool _is_anon;
  bool _is_async;
  bool _is_const;
  bool _is_background;
  union {
    uint32_t _ins_pos;
    void *_native_fn;   // NativeFn
    void *_native_fn2;  // FunctionContext
  };
};

typedef struct {
  Object *obj;
  const Function *func;
  void *parent_context;  // To avoid circular dependency.
} _FunctionRef;

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_OBJECT_H_ */