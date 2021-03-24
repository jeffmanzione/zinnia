// io.c
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione
#include "entity/native/io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

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
#include "struct/map.h"
#include "struct/set.h"
#include "util/file/file_util.h"
#include "vm/intern.h"

#define MAX_EVENTS 1024
#define FILE_NAME_LENGTH_ESTIMATE 16
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUFFER_LENGTH \
  (MAX_EVENTS * (EVENT_SIZE + FILE_NAME_LENGTH_ESTIMATE))

static Class *Class_WatchDir;

typedef struct {
  FILE *fp;
} _File;

typedef struct {
  bool is_closed;
  int fd, length;
  char buffer[EVENT_BUFFER_LENGTH];
} _FileWatcher;

typedef struct {
  int wd;
  char *dir;
} _WatchDir;

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

void _watch_dir_init(Object *obj) {
  _WatchDir *wd = ALLOC2(_WatchDir);
  obj->_internal_obj = wd;
}

void _watch_dir_delete(Object *obj) {
  _WatchDir *wd = obj->_internal_obj;
  if (NULL == wd) {
    return;
  }
  free(wd->dir);
  DEALLOC(wd);
}

void _file_watcher_init(Object *obj) {
  _FileWatcher *fw = ALLOC2(_FileWatcher);
  fw->fd = inotify_init();
  fw->is_closed = false;
  obj->_internal_obj = fw;
}

void _file_watcher_delete(Object *obj) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  if (NULL == fw) {
    return;
  }
  if (!fw->is_closed && fw->fd >= 0) {
    close(fw->fd);
    fw->is_closed = true;
  }
  DEALLOC(fw);
}

Entity _file_watcher_is_valid(Task *task, Context *ctx, Object *obj,
                              Entity *args) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  return entity_int(fw->fd >= 0);
}

Entity _file_watcher_watch(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  String *dir = args->obj->_internal_obj;
  Object *wd_obj = heap_new(task->parent_process->heap, Class_WatchDir);
  _WatchDir *wd = (_WatchDir *)wd_obj->_internal_obj;
  char *dir_str = strndup(dir->table, String_size(dir));
  wd->wd = inotify_add_watch(fw->fd, dir_str, IN_ALL_EVENTS);
  if (wd < 0) {
    return raise_error(task, ctx, "Could not watch directory: '%s'", dir_str);
  }
  return entity_object(wd_obj);
}

Entity _file_watcher_unwatch(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  _WatchDir *wd = (_WatchDir *)args->obj->_internal_obj;
  inotify_rm_watch(fw->fd, wd->wd);
  return NONE_ENTITY;
}

Entity _file_watcher_read(Task *task, Context *ctx, Object *obj, Entity *args) {
  // printf("IN_ACCESS=%d\n", IN_ACCESS);
  // printf("IN_ATTRIB=%d\n", IN_ATTRIB);
  // printf("IN_CLOSE=%d\n", IN_CLOSE);
  // printf("IN_CLOSE_WRITE=%d\n", IN_CLOSE_WRITE);
  // printf("IN_CLOSE_NOWRITE=%d\n", IN_CLOSE_NOWRITE);
  // printf("IN_CREATE=%d\n", IN_CREATE);
  // printf("IN_DELETE=%d\n", IN_DELETE);
  // printf("IN_DELETE_SELF=%d\n", IN_DELETE_SELF);
  // printf("IN_MODIFY=%d\n", IN_MODIFY);
  // printf("IN_MOVE=%d\n", IN_MOVE);
  // printf("IN_MOVE_SELF=%d\n", IN_MOVE_SELF);
  // printf("IN_MOVED_FROM=%d\n", IN_MOVED_FROM);
  // printf("IN_MOVED_TO=%d\n", IN_MOVED_TO);
  // printf("IN_OPEN=%d\n", IN_OPEN);
  // printf("IN_IGNORED=%d\n", IN_IGNORED);
  // printf("IN_ISDIR=%d\n", IN_ISDIR);
  // printf("IN_Q_OVERFLOW=%d\n", IN_Q_OVERFLOW);
  // printf("IN_UNMOUNT=%d\n", IN_UNMOUNT);
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  int length = read(fw->fd, fw->buffer, EVENT_BUFFER_LENGTH);
  if (length < 0) {
    return raise_error(task, ctx, "read() failed.");
  }
  fw->length = length;
  return entity_int(length);
}

Entity _file_watcher_get_read(Task *task, Context *ctx, Object *obj,
                              Entity *args) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  int length = fw->length;
  int i = 0;
  fflush(stdout);
  Object *arr_obj = heap_new(task->parent_process->heap, Class_Array);
  while (i < length) {
    struct inotify_event *event = (struct inotify_event *)&fw->buffer[i];
    Entity name = entity_object(string_new(task->parent_process->heap,
                                           event->name, strlen(event->name)));
    Entity mask = entity_int(event->mask);
    Object *tuple_obj = heap_new(task->parent_process->heap, Class_Tuple);
    tuple_obj->_internal_obj = tuple_create(2);
    Entity tuple_e = entity_object(tuple_obj);
    tuple_set(task->parent_process->heap, tuple_obj, 0, &name);
    tuple_set(task->parent_process->heap, tuple_obj, 1, &mask);

    array_add(task->parent_process->heap, arr_obj, &tuple_e);
    i += EVENT_SIZE + event->len;
  }
  fw->length = 0;
  return entity_object(arr_obj);
}

Entity _file_watcher_close(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  _FileWatcher *fw = (_FileWatcher *)obj->_internal_obj;
  ASSERT(NOT_NULL(fw));
  if (!fw->is_closed && fw->fd >= 0) {
    close(fw->fd);
    fw->is_closed = true;
  }
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

  Class_WatchDir = native_class(io, intern("__WatchDir"), _watch_dir_init,
                                _watch_dir_delete);
  Class *file_watcher = native_class(io, intern("__FileWatcher"),
                                     _file_watcher_init, _file_watcher_delete);
  native_method(file_watcher, intern("__is_valid"), _file_watcher_is_valid);
  native_method(file_watcher, intern("__watch"), _file_watcher_watch);
  native_method(file_watcher, intern("__unwatch"), _file_watcher_unwatch);
  native_background_method(file_watcher, intern("__read"), _file_watcher_read);
  native_method(file_watcher, intern("__get_read"), _file_watcher_get_read);
  native_method(file_watcher, intern("__close"), _file_watcher_close);
}
