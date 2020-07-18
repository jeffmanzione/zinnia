// file_info.h
//
// Created on: Jan 6, 2016
//     Author: Jeff Manzione

#ifndef LANG_LEXER_FILE_INFO_H_
#define LANG_LEXER_FILE_INFO_H_

#include <stdio.h>

#include "lang/lexer/token.h"

typedef struct LineInfo_ {
  char *line_text;
  Token *tokens;  // Sparse array of tokens corresponding to words
  int line_num;
} LineInfo;

typedef struct FileInfo_ FileInfo;

FileInfo *file_info(const char fn[]);
FileInfo *file_info_file(FILE *file);
void file_info_set_name(FileInfo *fi, const char fn[]);
LineInfo *file_info_append(FileInfo *fi, char line_text[]);
const LineInfo *file_info_lookup(const FileInfo *fi, int line_num);
int file_info_len(const FileInfo *fi);
const char *file_info_name(const FileInfo *fi);
void file_info_delete(FileInfo *fi);
void file_info_close_file(FileInfo *fi);
FILE *file_info_fp(FileInfo *fi);

#endif /* LANG_LEXER_FILE_INFO_H_ */