// compiler.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#ifndef COMPILE_COMPILE_H_
#define COMPILE_COMPILE_H_

#include "program/tape.h"
#include "struct/set.h"
#include "util/args/commandline.h"

Map *compile(const Set *source_files, const ArgStore *store);
void write_tape(const char fn[], const Tape *tape, bool out_ja,
                const char machine_dir[], bool out_jb,
                const char bytecode_dir[]);
int jlc(int argc, const char *argv[]);

#endif /* COMPILE_COMPILE_H_ */