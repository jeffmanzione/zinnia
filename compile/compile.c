// compiler.c
//
// Created on: Oct 28, 2020
//     Author: Jeff Manzione

#include <stdbool.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "lang/lexer/file_info.h"
#include "lang/parser/parser.h"
#include "lang/semantics/expression_tree.h"
#include "lang/semantics/semantics.h"
#include "struct/map.h"
#include "struct/set.h"
#include "util/args/commandline.h"
#include "util/args/commandlines.h"
#include "util/args/lib_finder.h"
#include "util/file.h"
#include "util/string.h"
#include "vm/intern.h"

Tape *read_file(const char fn[]) {
  FileInfo *fi = file_info(fn);
  SyntaxTree stree = parse_file(fi);
  ExpressionTree *etree = populate_expression(&stree);
  Tape *tape = tape_create();
  produce_instructions(etree, tape);
  delete_expression(etree);
  syntax_tree_delete(&stree);
  return tape;
}

void write_tape(const char fn[], Tape *tape, bool out_jm,
                const char machine_dir[]) {
  char *path, *file_name, *ext;
  split_path_file(fn, &path, &file_name, &ext);

  if (out_jm && ends_with(fn, ".jl")) {
    make_dir_if_does_not_exist(machine_dir);
    FILE *file =
        FILE_FN(combine_path_file(machine_dir, file_name, ".jm"), "wb");
    tape_write(tape, file);
    fclose(file);
  }
  // TODO: Handle outputting .jb.
}

void compile(const Set *source_files, const ArgStore *store) {
  parsers_init();
  semantics_init();

  const bool out_jm = argstore_lookup_bool(store, ArgKey__OUT_MACHINE);
  const char *machine_dir =
      argstore_lookup_string(store, ArgKey__MACHINE_OUT_DIR);

  M_iter srcs = set_iter((Set *)source_files);
  for (; has(&srcs); inc(&srcs)) {
    const char *src = value(&srcs);
    Tape *tape = read_file(src);
    write_tape(src, tape, out_jm, machine_dir);
    tape_delete(tape);
  }
  semantics_finalize();
  parsers_finalize();
}

int main(int argc, const char *argv[]) {
  alloc_init();
  strings_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  compile(argstore_sources(store), store);

  strings_finalize();
  token_finalize_all();
  alloc_finalize();

  return EXIT_SUCCESS;
}