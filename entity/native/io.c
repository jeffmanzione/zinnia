// io.c
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione
#include "entity/native/io.h"

#include <stdio.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "entity/class/classes.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "util/file/file_util.h"
#include "vm/intern.h"

#define FILE_WATCHER_EVENT_SIZE (sizeof(struct inotify_event))
#define FILE_WATCHER_BUFFER_LENGTH (1024 * (FILE_WATCHER_EVENT_SIZE + 16))

typedef struct {
  FILE *fp;
} _File;

typedef struct {
  int fd;
  Set wds;
} _FileWatcher;

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
  if (NULL != f->fp && f->fp != stdout && f->fp != stderr) {
    fclose(f->fp);
  }
}

Entity _file_constructor(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type) {
    return raise_error(task, ctx, "Invalid input for File.");
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
      return raise_error(task, ctx, "Too few arguments for File constructor.");
    }
    const Entity *e_fn = tuple_get(tup, 0);
    // entity_print(e_fn, stdout);
    if (NULL == e_fn || OBJECT != e_fn->type ||
        Class_String != e_fn->obj->_class) {
      return raise_error(task, ctx, "File name must be a String.");
    }
    if (0 == strncmp("__STDOUT__", ((String *)e_fn->obj->_internal_obj)->table,
                     10)) {
      f->fp = stdout;
    } else if (0 == strncmp("__STDERR__",
                            ((String *)e_fn->obj->_internal_obj)->table, 10)) {
      f->fp = stderr;
    } else if (0 == strncmp("__STDIN__",
                            ((String *)e_fn->obj->_internal_obj)->table, 9)) {
      f->fp = stdin;
    } else {
      const Entity *e_mode = tuple_get(tup, 1);
      if (NULL == e_mode || OBJECT != e_mode->type ||
          Class_String != e_mode->obj->_class) {
        return raise_error(task, ctx, "File mode must be a String.");
      }
      fn = _String_nullterm((String *)e_fn->obj->_internal_obj);
      mode = _String_nullterm((String *)e_mode->obj->_internal_obj);
      f->fp = fopen(fn, mode);
      DEALLOC(fn);
      DEALLOC(mode);
      if (NULL == f->fp) {
        return raise_error(task, ctx, "File could not be opened.");
      }
    }
  } else {
    return raise_error(task, ctx, "Unknown input.");
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

Entity _file_gets(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  ASSERT(NOT_NULL(f), NOT_NULL(f->fp));
  if (NULL == args || PRIMITIVE != args->type || INT != ptype(&args->pri)) {
    return raise_error(task, ctx, "Invalid input to gets.");
  }
  char *buf = ALLOC_ARRAY2(char, pint(&args->pri) + 1);
  Entity string;
  if (fgets(buf, pint(&args->pri), f->fp)) {
    string = entity_object(
        string_new(task->parent_process->heap, buf, pint(&args->pri)));
  } else {
    string = NONE_ENTITY;
  }
  DEALLOC(buf);
  return string;
}

Entity _file_getline(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  ASSERT(NOT_NULL(f), NOT_NULL(f->fp));
  char *line = NULL;
  size_t len = 0;
  int nread = getline(&line, &len, f->fp);
  Entity string;
  if (-1 == nread) {
    string = NONE_ENTITY;
  } else {
    string = entity_object(string_new(task->parent_process->heap, line, nread));
  }
  if (line != NULL) {
    DEALLOC(line);
  }
  return string;
}

// This is vulnerable to files with \0 inside them.
Entity _file_getall(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  // Get length of file to realloc size and avoid buffer reallocs.
  fseek(f->fp, 0, SEEK_END);
  long fsize = ftell(f->fp);
  rewind(f->fp);
  // Create string and copy the file into it.
  Object *str = string_new(task->parent_process->heap, NULL, fsize);
  String *string = (String *)str->_internal_obj;
  String_set(string, fsize - 1, '\0');
  // Can be less than read on Windows because \r gets dropped.
  int actually_read = fread(string->table, sizeof(char), fsize, f->fp);
  // If this happens then something is really wrong.
  ASSERT(actually_read <= fsize);
  String_rshrink(string, fsize - actually_read);
  return entity_object(str);
}

Entity _file_puts(Task *task, Context *ctx, Object *obj, Entity *args) {
  _File *f = (_File *)obj->_internal_obj;
  ASSERT(NOT_NULL(f));
  if (NULL == args || NONE == args->type || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    return NONE_ENTITY;
  }
  String *string = (String *)args->obj->_internal_obj;
  fprintf(f->fp, "%.*s", String_size(string), string->table);
  return NONE_ENTITY;
}

void io_add_native(ModuleManager *mm, Module *io) {
  Class *file = native_class(io, intern("__File"), __file_init, __file_delete);
  native_method(file, CONSTRUCTOR_KEY, _file_constructor);
  native_background_method(file, intern("__close"), _file_close);
  native_background_method(file, intern("__gets"), _file_gets);
  native_background_method(file, intern("__getline"), _file_getline);
  native_background_method(file, intern("__getall"), _file_getall);
  native_background_method(file, intern("__puts"), _file_puts);
}