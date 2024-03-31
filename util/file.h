// file.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef UTIL_FILE_H_
#define UTIL_FILE_H

#include <stdbool.h>

#include "struct/alist.h"

void replace_backslashes(char *str);

char *find_file_by_name(const char dir[], const char file_prefix[]);

bool is_dir(const char file_path[]);

typedef struct __DirIter DirIter;

DirIter *directory_iter(const char dir_name[]);
void diriter_close(DirIter *d);
const char *diriter_next_file(DirIter *d);

typedef struct __FileLoc FileLoc;
typedef struct __FileLocs FileLocs;

typedef struct {
  FileLocs *_locs;
  int _i;
} FileLoc_iter;

const char *file_loc_full_path(FileLoc *loc);
const char *file_loc_base_path(FileLoc *loc);
const char *file_loc_relative_path(FileLoc *loc);

FileLocs *file_locs_create(const char path[]);
void file_locs_delete(FileLocs *locs);

FileLoc_iter file_locs_iter(FileLocs *locs);
bool fl_has(FileLoc_iter *iter);
FileLoc *fl_value(FileLoc_iter *iter);
void fl_inc(FileLoc_iter *iter);

#endif /* UTIL_FILE_H_ */