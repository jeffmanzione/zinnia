// compiler.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_COMPILE_COMPILE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_COMPILE_COMPILE_H_

#include <stdio.h>

#include "c-data-structures/maplike.h"
#include "zinnia/program/tape.h"
#include "zinnia/util/args/commandline.h"

DEFINE_MAPLIKE(TapeNameMap, char *, Tape *);

void compile(const SourceNameSet *source_files, bool out_zna,
             const char machine_dir[], bool out_znb, const char bytecode_dir[],
             bool opt, bool minimize, TapeNameMap *src_map);
void compile_to_assembly(const char file_name[], FILE *out);

void write_tape(const char fn[], const Tape *tape, bool out_zna,
                const char machine_dir[], bool out_znb,
                const char bytecode_dir[], bool machine);
int zinniac(int argc, const char *argv[]);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_COMPILE_COMPILE_H_ */