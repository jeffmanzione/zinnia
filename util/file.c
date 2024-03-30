// file.c
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#include "util/file.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "util/string.h"

#include "util/platform.h"

#ifdef OS_WINDOWS

#include <io.h>
#include <windows.h>
#define F_OK 0
#define ssize_t int

#else

#include <dirent.h>
#include <unistd.h>

#endif

char *find_file_by_name(const char dir[], const char file_prefix[]) {
  size_t fn_len = strlen(dir) + strlen(file_prefix) + 3;
  if (!ends_with(dir, "/")) {
    fn_len++;
  }
  const char *fn = ALLOC_ARRAY2(char, fn_len + 1);
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
    char *to_return = intern_range(fn, 0, fn_len);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".zna");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern_range(fn, 0, fn_len);
    DEALLOC(fn);
    return to_return;
  }
  strcpy(pos, ".zn");
  if (access(fn, F_OK) != -1) {
    char *to_return = intern_range(fn, 0, fn_len);
    DEALLOC(fn);
    return to_return;
  }
  DEALLOC(fn);
  return NULL;
}

bool is_dir(const char file_path[]) {
  struct stat path_stat;
  stat(file_path, &path_stat);
  return S_ISDIR(path_stat.st_mode);
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
  DirIter *d = ALLOC2(DirIter);
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
  DEALLOC(d);
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
