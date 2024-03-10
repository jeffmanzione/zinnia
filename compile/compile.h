// compiler.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#ifndef COMPILE_COMPILE_H_
#define COMPILE_COMPILE_H_

#include <stdio.h>

#include "program/tape.h"
#include "struct/map.h"
#include "struct/set.h"

Map *compile(const Set *source_files, bool out_ja, const char machine_dir[],
             bool out_jb, const char bytecode_dir[], bool opt);
void compile_to_assembly(const char file_name[], FILE *out);

void write_tape(const char fn[], const Tape *tape, bool out_ja,
                const char machine_dir[], bool out_jb,
                const char bytecode_dir[]);
int jasperc(int argc, const char *argv[]);

#endif /* COMPILE_COMPILE_H_ */