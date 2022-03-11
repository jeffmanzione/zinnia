// file.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef UTIL_FILE_H_
#define UTIL_FILE_H_

char *find_file_by_name(const char dir[], const char file_prefix[]);

typedef struct __DirIter DirIter;

DirIter *directory_iter(const char dir_name[]);
void diriter_close(DirIter *d);
const char *diriter_next_file(DirIter *d);

#endif /* UTIL_FILE_H_ */