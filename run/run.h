// run.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#ifndef RUN_RUN_H_
#define RUN_RUN_H_

#include "struct/set.h"
#include "util/args/commandline.h"

void run(const Set *source_files, ArgStore *store);
int jlr(int argc, const char *argv[]);

#endif /* RUN_RUN_H_ */