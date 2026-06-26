// file.c
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "zinnia/util/file.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "c-data-structures/arraylike.h"
#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "language-tools/intern.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/util/platform.h"

#ifdef OS_WINDOWS

#include <io.h>
#include <windows.h>
#define F_OK 0
#define ssize_t int

#else

#include <dirent.h>
#include <unistd.h>

#endif

struct __FileLoc {
  char *full_path;
  char *base_path;
  char *relative_path;
};

DEFINE_ARRAYLIKE(FileLocArray, FileLoc);
IMPL_ARRAYLIKE(FileLocArray, FileLoc);

struct __FileLocs {
  FileLocArray locs;
};

const char *find_file_by_name(const char dir[], const char file_prefix[]) {
  size_t fn_len = strlen(dir) + strlen(file_prefix) + 3;
  if (!ends_with(dir, "/")) {
    fn_len++;
  }
  char *fn = MNEW_ARR(char, fn_len + 1);
  char *pos = (char *)fn;
  strcpy(pos, dir);
  pos += strlen(dir);
  if (!ends_with(dir, "/")) {
    strcpy(pos++, "/");
  }
  strcpy(pos, file_prefix);
  pos += strlen(file_prefix);
  strcpy(pos, ".znb");
  if (access(fn, F_OK) != -1) {
    const char *to_return = global_intern_range(fn, 0, fn_len);
    RELEASE(fn);
    return to_return;
  }
  strcpy(pos, ".zna");
  if (access(fn, F_OK) != -1) {
    const char *to_return = global_intern_range(fn, 0, fn_len);
    RELEASE(fn);
    return to_return;
  }
  strcpy(pos, ".zn");
  if (access(fn, F_OK) != -1) {
    const char *to_return = global_intern_range(fn, 0, fn_len);
    RELEASE(fn);
    return to_return;
  }
  RELEASE(fn);
  return NULL;
}

bool is_dir(const char file_path[]) {
  struct stat path_stat;
  stat(file_path, &path_stat);
  return (path_stat.st_mode & S_IFDIR) != 0;
}

struct __DirIter {
#ifdef OS_WINDOWS
  const char *dir_name;
  WIN32_FIND_DATA ffd;
  HANDLE h_find;
#else
  DIR *dir;
#endif
};

DirIter *directory_iter(const char dir_name[]) {
  DirIter *d = MNEW(DirIter);
#ifdef OS_WINDOWS
  d->dir_name = dir_name;
  d->h_find = NULL;
#else
  d->dir = opendir(dir_name);
#endif
  return d;
}

void diriter_close(DirIter *d) {
#ifdef OS_WINDOWS
  if (NULL != d->h_find && INVALID_HANDLE_VALUE != d->h_find) {
    FindClose(d->h_find);
  }
#else
  if (NULL != d->dir) {
    closedir(d->dir);
  }
#endif
  RELEASE(d);
}

const char *diriter_next_file(DirIter *d) {
#ifdef OS_WINDOWS
  if (INVALID_HANDLE_VALUE == d->h_find) {
    return NULL;
  }
  // First call.
  if (NULL == d->h_find) {
    d->h_find = FindFirstFile(d->dir_name, &d->ffd);
    if (INVALID_HANDLE_VALUE == d->h_find) {
      // TODO: Should this complain?
      return NULL;
    }
    return d->ffd.cFileName;
  }
  if (FindNextFile(d->h_find, &d->ffd)) {
    return d->ffd.cFileName;
  }
  return NULL;
#else
  struct dirent *dnet = readdir(d->dir);
  if (NULL == dnet) {
    return NULL;
  }
  return dnet->d_name;
#endif
}

void _file_loc_init(FileLoc *loc, const char full_path[],
                    const char base_path[]) {
  loc->full_path = strdup(full_path);
  replace_backslashes(loc->full_path);
  loc->base_path = strdup(base_path);
  replace_backslashes(loc->base_path);

  int base_path_len = strlen(base_path);

  char after_base_path = *(full_path + base_path_len);
  int relative_start_index;
  relative_start_index = ('/' == after_base_path || '\\' == after_base_path)
                             ? base_path_len + 1
                             : base_path_len;
  loc->relative_path = strdup(loc->full_path + relative_start_index);
}

void _file_loc_finalize(FileLoc *loc) {
  free(loc->full_path);
  free(loc->base_path);
  free(loc->relative_path);
}

const char *file_loc_full_path(FileLoc *loc) { return loc->full_path; }

const char *file_loc_base_path(FileLoc *loc) { return loc->base_path; }

const char *file_loc_relative_path(FileLoc *loc) { return loc->relative_path; }

void replace_backslashes(char *str) {
  int str_len = strlen(str);
  for (int i = 0; i < str_len; ++i) {
    if (str[i] == '\\') {
      str[i] = '/';
    }
  }
}

void _traverse_dir(const char base_path[], const char dir_path[],
                   FileLocArray *locs) {
  char *dir_path_with_wildcard = combine_path_file(dir_path, "*", NULL);
  DirIter *di = directory_iter(dir_path_with_wildcard);
  while (true) {
    const char *fn = diriter_next_file(di);
    if (NULL == fn) {
      break;
    }
    if (0 == strcmp(fn, ".") || 0 == strcmp(fn, "..")) {
      continue;
    }
    char *full_fn = combine_path_file(dir_path, fn, NULL);
    if (is_dir(full_fn)) {
      _traverse_dir(base_path, full_fn, locs);
    } else if (ends_with(fn, ".zn") || ends_with(fn, ".zna") ||
               ends_with(fn, ".znb")) {
      FileLoc *loc = FileLocArray_push_back_ref(locs);
      _file_loc_init(loc, full_fn, base_path);
    }
    RELEASE(full_fn);
  }
  RELEASE(dir_path_with_wildcard);
}

FileLocs *file_locs_create(const char path[]) {
  FileLocs *locs = MNEW(FileLocs);
  FileLocArray_init(&locs->locs);
  _traverse_dir(path, path, &locs->locs);
  return locs;
}

void file_locs_delete(FileLocs *locs) {
  FileLocArrayIterator iter;
  FileLocArray_iterator(&iter, &locs->locs);
  for (; FileLocArray_has_next(&iter); FileLocArray_next(&iter)) {
    FileLoc *loc = FileLocArray_mutable_value(&iter);
    _file_loc_finalize(loc);
  }
  FileLocArray_finalize(&locs->locs);
  RELEASE(locs);
}

FileLoc_iter file_locs_iter(FileLocs *locs) {
  FileLoc_iter iter = {._locs = locs, ._i = 0};
  return iter;
}

bool fl_has(FileLoc_iter *iter) {
  return iter->_i < FileLocArray_size(&iter->_locs->locs);
}

FileLoc *fl_value(FileLoc_iter *iter) {
  return FileLocArray_mutable_ref_unchecked(&iter->_locs->locs, iter->_i);
}

void fl_inc(FileLoc_iter *iter) { iter->_i++; }