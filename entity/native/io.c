// io.c
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione
#include "entity/native/io.h"

#include <stdio.h>

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/class/classes.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/tuple/tuple.h"
#include "vm/intern.h"

typedef struct {
  FILE *fp;
} _File;

char *_String_nullterm(String *str) {
  uint32_t str_len = String_size(str);
  char *out = ALLOC_ARRAY2(char, str_len + 1);
  memmove(out, str->table, str_len);
  out[str_len] = 0;
  return out;
}

void __file_init(Object *obj) {
  _File *f = ALLOC2(_File);
  f->fp = NULL;
  obj->_internal_obj = f;
}

void __file_delete(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  _File *f = (_File *)obj->_internal_obj;
  if (NULL != f->fp) {
    fclose(f->fp);
  }
}

Entity _file_constructor(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type) {
    ERROR("Invalid input for File.");
    return NONE_ENTITY;
  }
  _File *f = (_File *)obj->_internal_obj;

  char *fn, *mode;
  if (Class_String == args->obj->_class) {
    fn = _String_nullterm((String *)args->obj->_internal_obj);
    mode = intern("r");
    f->fp = fopen(fn, mode);
    DEALLOC(fn);
  } else if (Class_Tuple == args->obj->_class) {
    Tuple *tup = (Tuple *)args->obj->_internal_obj;
    if (tuple_size(tup) < 2) {
      ERROR("Too few arguments for File constructor.");
    }
    const Entity *e_fn = tuple_get(tup, 0);
    // entity_print(e_fn, stdout);
    if (NULL == e_fn || OBJECT != e_fn->type ||
        Class_String != e_fn->obj->_class) {
      ERROR("File name must be a String.");
    }
    if (0 == strncmp("__STDOUT__", ((String *)e_fn->obj->_internal_obj)->table,
                     10)) {
      f->fp = stdout;
    } else if (0 == strncmp("__STDERR__",
                            ((String *)e_fn->obj->_internal_obj)->table, 10)) {
      f->fp = stderr;
    } else {
      const Entity *e_mode = tuple_get(tup, 1);
      if (NULL == e_mode || OBJECT != e_mode->type ||
          Class_String != e_mode->obj->_class) {
        ERROR("File mode must be a String.");
      }
      fn = _String_nullterm((String *)e_fn->obj->_internal_obj);
      mode = _String_nullterm((String *)e_mode->obj->_internal_obj);
      f->fp = fopen(fn, mode);
      DEALLOC(fn);
      DEALLOC(mode);
    }
  } else {
    ERROR("Unknown input.");
  }
  return entity_object(obj);
}

Entity _file_close(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  if (NULL == f->fp) {
    return NONE_ENTITY;
  }
  if (f->fp != stdin && f->fp != stdout && f->fp != stderr) {
    fclose(f->fp);
  }
  f->fp = NULL;
  return NONE_ENTITY;
}

Entity _file_puts(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  ASSERT(NOT_NULL(f));
  if (NULL == args || NONE == args->type || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return NONE_ENTITY;
  }
  String *string = (String *)args->obj->_internal_obj;
  // DEBUGF("%d", String_size(string));
  fprintf(f->fp, "%*s", String_size(string), string->table);
  // fflush(f->fp);
  return NONE_ENTITY;
}

void io_add_native(Module *io) {
  Class *file = native_class(io, intern("__File"), __file_init, __file_delete);
  native_method(file, CONSTRUCTOR_KEY, _file_constructor);
  native_method(file, intern("__close"), _file_close);
  native_method(file, intern("__puts"), _file_puts);
}