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

Map *compile(const Set *source_files, bool out_zna, const char machine_dir[],
             bool out_znb, const char bytecode_dir[], bool opt, bool minimize);
void compile_to_assembly(const char file_name[], FILE *out);

void write_tape(const char fn[], const Tape *tape, bool out_zna,
                const char machine_dir[], bool out_znb,
                const char bytecode_dir[], bool machine);
int zinniac(int argc, const char *argv[]);

#endif /* COMPILE_COMPILE_H_ */