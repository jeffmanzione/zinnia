// compiler.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione


#ifndef COMPILE_COMPILE_H_
#define COMPILE_COMPILE_H_

#include "util/args/commandline.h"
#include "struct/set.h"

void compile(const Set *source_files, const ArgStore *store);

#endif /* COMPILE_COMPILE_H_ */