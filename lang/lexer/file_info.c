// file_info.c
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#include "lang/lexer/file_info.h"

#include <stdio.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "util/file.h"

#define DEFAULT_NUM_LINES 128

struct FileInfo_ {
  char *name;
  FILE *fp;
  LineInfo **lines;
  int num_lines;
  int array_len;
};

LineInfo *line_info(FileInfo *fi, char line_text[], int line_num) {
  LineInfo *li = ALLOC(LineInfo);
  li->line_text = intern(line_text);
  li->line_num = line_num;
  return li;
}

void _line_info_delete(LineInfo *li) { DEALLOC(li); }

FileInfo *file_info(const char fn[]) {
  FILE *file = FILE_FN(fn, "r");
  FileInfo *fi = file_info_file(file);
  file_info_set_name(fi, fn);
  return fi;
}

inline void file_info_set_name(FileInfo *fi, const char fn[]) {
  ASSERT(NOT_NULL(fi), NOT_NULL(fn));
  fi->name = intern(fn);
}

FileInfo *file_info_file(FILE *file) {
  FileInfo *fi = ALLOC(FileInfo);
  fi->name = NULL;
  fi->fp = file;
  ASSERT_NOT_NULL(fi->fp);
  fi->num_lines = 0;
  fi->array_len = DEFAULT_NUM_LINES;
  fi->lines = ALLOC_ARRAY(LineInfo *, fi->array_len);
  return fi;
}

inline void file_info_close_file(FileInfo *fi) {
  if (NULL == fi->fp) {
    return;
  }
  fclose(fi->fp);
  fi->fp = NULL;
}

inline FILE *file_info_fp(FileInfo *fi) {
  ASSERT(NOT_NULL(fi));
  return fi->fp;
}

void file_info_delete(FileInfo *fi) {
  ASSERT_NOT_NULL(fi);
  ASSERT_NOT_NULL(fi->lines);
  int i;
  for (i = 0; i < fi->num_lines; i++) {
    _line_info_delete(fi->lines[i]);
  }
  DEALLOC(fi->lines);
  file_info_close_file(fi);
  DEALLOC(fi);
}

LineInfo *file_info_append(FileInfo *fi, char line_text[]) {
  int num_lines = fi->num_lines;
  LineInfo *li = fi->lines[fi->num_lines++] =
      line_info(fi, line_text, num_lines);
  if (fi->num_lines >= fi->array_len) {
    fi->array_len += DEFAULT_NUM_LINES;
    fi->lines = REALLOC(fi->lines, LineInfo *, fi->array_len);
  }
  return li;
}

inline const LineInfo *file_info_lookup(const FileInfo *fi, int line_num) {
  if (line_num < 1 || line_num > fi->num_lines) {
    return NULL;
  }
  return fi->lines[line_num - 1];
}

inline int file_info_len(const FileInfo *fi) { return fi->num_lines; }

inline const char *file_info_name(const FileInfo *fi) { return fi->name; }
