// file.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef UTIL_FILE_H_
#define UTIL_FILE_H_

#include <stdio.h>

#define FILE_FN(fn, op_type) file_fn(fn, op_type, __LINE__, __func__, __FILE__)
#define FILE_OP(file, operation)                                         \
  file_op(file, ({ void __fn__ operation __fn__; }), __LINE__, __func__, \
          __FILE__)
#define FILE_OP_FN(fn, op_type, operation) \
  FILE_OP(FILE_FN(fn, op_type), operation)

typedef void (*FileHandler)(FILE *);

FILE *file_fn(const char fn[], const char op_type[], int line_num,
              const char func_name[], const char file_name[]);
void file_op(FILE *file, FileHandler operation, int line_num,
             const char func_name[], const char file_name[]);

#endif /* UTIL_FILE_H_ */