// io.c
//
// Created on: Sept 02, 2020
//     Author: Jeff Manzione
#include "zinnia/entity/native/io.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef OS_WINDOWS
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "file-utils/file_utils.h"
#include "language-tools/intern.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/native/native_helpers.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/util/error.h"
#include "zinnia/vm/intern.h"

#define MAX_EVENTS 1024
#define FILE_NAME_LENGTH_ESTIMATE 16

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif

static Class *Class_WatchDir;

typedef struct {
  FILE *fp;
} File_;

#ifndef OS_WINDOWS

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUFFER_LENGTH \
  (MAX_EVENTS * (EVENT_SIZE + FILE_NAME_LENGTH_ESTIMATE))

typedef struct {
  bool is_closed;
  int fd, length;
  char buffer[EVENT_BUFFER_LENGTH];
} FileWatcher_;

typedef struct {
  int wd;
  char *dir;
} WatchDir_;

#endif

char *String_nullterm_(String *str) {
  uint32_t str_len = String_size(str);
  char *out = MNEW_ARR(char, str_len + 1);
  memcpy(out, str->table, str_len);
  out[str_len] = 0;
  return out;
}

void _file_init_(Object *obj) {
  File_ *f = MNEW(File_);
  f->fp = NULL;
  obj->_internal_obj = f;
}

void _file_delete_(Object *obj) {
  if (NULL == obj->_internal_obj) {
    return;
  }
  File_ *f = (File_ *)obj->_internal_obj;
  if (NULL != f->fp && f->fp != stdout && f->fp != stderr) {
    fclose(f->fp);
  }
  RELEASE(f);
}

Entity file_constructor_(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type) {
    return raise_error(task, ctx, "Invalid input for File.");
  }
  File_ *f = (File_ *)obj->_internal_obj;

  const char *fn, *mode;
  if (IS_STRING(args)) {
    fn = entity_string_copy(args);
    mode = global_intern("r");
    f->fp = fopen(fn, mode);
    RELEASE(fn);
  } else if (Class_Tuple == args->obj->_class) {
    Tuple *tup = (Tuple *)args->obj->_internal_obj;
    if (tuple_size(tup) < 2) {
      return raise_error(task, ctx, "Too few arguments for File constructor.");
    }

    EXCTRACT_STRING_AT_INDEX_OR_THROW(fn, fn_len, tup, 0);

    if (0 == strncmp("__STDOUT__", fn, min(fn_len, 10))) {
      f->fp = stdout;
    } else if (0 == strncmp("__STDERR__", fn, min(fn_len, 10))) {
      f->fp = stderr;
    } else if (0 == strncmp("__STDIN__", fn, min(fn_len, 9))) {
      f->fp = stdin;
    } else {
      EXCTRACT_STRING_AT_INDEX_OR_THROW(mode, mode_len, tup, 1);
      fn = ALLOC_STRNDUP(fn, fn_len);
      mode = ALLOC_STRNDUP(mode, mode_len);
      f->fp = fopen(fn, mode);
      RELEASE(fn);
      RELEASE(mode);
      if (NULL == f->fp) {
        return raise_error(task, ctx, "File '%s' could not be opened.", fn);
      }
    }
  } else {
    return raise_error(task, ctx, "Unknown input.");
  }
  return entity_object(obj);
}

Entity file_close_(Task *task, Context *ctx, Object *obj, Entity *args) {
  File_ *f = (File_ *)obj->_internal_obj;
  if (NULL == f->fp) {
    return NONE_ENTITY;
  }
  if (f->fp != stdin && f->fp != stdout && f->fp != stderr) {
    fclose(f->fp);
  }
  f->fp = NULL;
  return NONE_ENTITY;
}

Entity file_gets_(Task *task, Context *ctx, Object *obj, Entity *args) {
  File_ *f = (File_ *)obj->_internal_obj;
  ASSERT(f != NULL);
  ASSERT(f->fp != NULL);
  if (NULL == args || PRIMITIVE != args->type ||
      PRIMITIVE_INT != ptype(&args->pri)) {
    return native_background_raise_error(task, ctx, "Invalid input to gets.");
  }
  char *buf = MNEW_ARR(char, pint(&args->pri) + 1);
  Entity string;
  if (fgets(buf, pint(&args->pri), f->fp)) {
    string = entity_object(native_background_string_new(task->parent_process,
                                                        buf, pint(&args->pri)));
  } else {
    string = NONE_ENTITY;
  }
  RELEASE(buf);
  return string;
}

Entity file_getline_(Task *task, Context *ctx, Object *obj, Entity *args) {
  File_ *f = (File_ *)obj->_internal_obj;
  ASSERT(f != NULL);
  ASSERT(f->fp != NULL);
  char *line = NULL;
  size_t len = 0;
  int nread = getline(&line, &len, f->fp);
  Entity string;
  if (-1 == nread) {
    string = NONE_ENTITY;
  } else {
    string = entity_object(
        native_background_string_new(task->parent_process, line, nread));
  }
  if (line != NULL) {
    RELEASE(line);
  }
  return string;
}

// This is vulnerable to files with \0 inside them.
Entity file_getall_(Task *task, Context *ctx, Object *obj, Entity *args) {
  File_ *f = (File_ *)obj->_internal_obj;
  // Get length of file to realloc size and avoid buffer reallocs.
  fseek(f->fp, 0, SEEK_END);
  long fsize = ftell(f->fp);
  rewind(f->fp);
  // Create string and copy the file into it.
  Object *str = native_background_string_new(task->parent_process, NULL, fsize);
  String *string = (String *)str->_internal_obj;
  String_set(string, fsize - 1, '\0');
  // Can be less than read on Windows because \r gets dropped.
  int actually_read = fread(string->table, sizeof(char), fsize, f->fp);
  // If this happens then something is really wrong.
  ASSERT(actually_read <= fsize);
  String_rshrink(string, fsize - actually_read);
  return entity_object(str);
}

Entity file_puts_(Task *task, Context *ctx, Object *obj, Entity *args) {
  File_ *f = (File_ *)obj->_internal_obj;
  ASSERT(f != NULL);
  if (!IS_STRING(args)) {
    return NONE_ENTITY;
  }
  char *string;
  int len;
  extract_string(args, &string, &len);
  fprintf(f->fp, "%.*s", len, string);
  return NONE_ENTITY;
}

#ifndef OS_WINDOWS
void watch_dir_init_(Object *obj) {
  WatchDir_ *wd = MNEW(WatchDir_);
  obj->_internal_obj = wd;
}

void watch_dir_delete_(Object *obj) {
  WatchDir_ *wd = obj->_internal_obj;
  if (NULL == wd) {
    return;
  }
  free(wd->dir);
  RELEASE(wd);
}

void file_watcher_init_(Object *obj) {
  FileWatcher_ *fw = MNEW(FileWatcher_);
  fw->fd = inotify_init();
  fw->is_closed = false;
  obj->_internal_obj = fw;
}

void file_watcher_delete_(Object *obj) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  if (NULL == fw) {
    return;
  }
  if (!fw->is_closed && fw->fd >= 0) {
    close(fw->fd);
    fw->is_closed = true;
  }
  RELEASE(fw);
}

Entity file_watcher_is_valid_(Task *task, Context *ctx, Object *obj,
                              Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
  return entity_int(fw->fd >= 0);
}

Entity file_watcher_watch_(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
  String *dir = args->obj->_internal_obj;
  Object *wd_obj = heap_new(task->parent_process->heap, Class_WatchDir);
  WatchDir_ *wd = (WatchDir_ *)wd_obj->_internal_obj;
  char *dir_str = ALLOC_STRNDUP(dir->table, String_size(dir));
  wd->wd = inotify_add_watch(fw->fd, dir_str, IN_ALL_EVENTS);
  if (wd < 0) {
    return raise_error(task, ctx, "Could not watch directory: '%s'", dir_str);
  }
  return entity_object(wd_obj);
}

Entity file_watcher_unwatch_(Task *task, Context *ctx, Object *obj,
                             Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
  WatchDir_ *wd = (WatchDir_ *)args->obj->_internal_obj;
  inotify_rm_watch(fw->fd, wd->wd);
  return NONE_ENTITY;
}

Entity file_watcher_read_(Task *task, Context *ctx, Object *obj, Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
  int length = read(fw->fd, fw->buffer, EVENT_BUFFER_LENGTH);
  if (length < 0) {
    return raise_error(task, ctx, "read() failed.");
  }
  fw->length = length;
  return entity_int(length);
}

Entity file_watcher_get_read_(Task *task, Context *ctx, Object *obj,
                              Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
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

Entity file_watcher_close_(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  FileWatcher_ *fw = (FileWatcher_ *)obj->_internal_obj;
  ASSERT(fw != NULL);
  if (!fw->is_closed && fw->fd >= 0) {
    close(fw->fd);
    fw->is_closed = true;
  }
  return NONE_ENTITY;
}
#endif

void io_add_native(ModuleManager *mm, Module *io) {
  Class *file =
      native_class(io, global_intern("__File"), _file_init_, _file_delete_);
  native_method(file, CONSTRUCTOR_KEY, file_constructor_);
  native_background_method(file, global_intern("__close"), file_close_);
  native_background_method(file, global_intern("__gets"), file_gets_);
  native_background_method(file, global_intern("__getline"), file_getline_);
  native_background_method(file, global_intern("__getall"), file_getall_);
  native_background_method(file, global_intern("__puts"), file_puts_);

#ifndef OS_WINDOWS
  Class_WatchDir = native_class(io, global_intern("__WatchDir"),
                                watch_dir_init_, watch_dir_delete_);
  Class *file_watcher = native_class(io, global_intern("__FileWatcher"),
                                     file_watcher_init_, file_watcher_delete_);
  native_method(file_watcher, global_intern("__is_valid"),
                file_watcher_is_valid_);
  native_method(file_watcher, global_intern("__watch"), file_watcher_watch_);
  native_method(file_watcher, global_intern("__unwatch"),
                file_watcher_unwatch_);
  native_background_method(file_watcher, global_intern("__read"),
                           file_watcher_read_);
  native_method(file_watcher, global_intern("__get_read"),
                file_watcher_get_read_);
  native_method(file_watcher, global_intern("__close"), file_watcher_close_);
#endif
}
