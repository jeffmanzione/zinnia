// compiler.c
//
// Created on: Oct 28, 2020
//     Author: Jeff Manzione

#include "compile/compile.h"

#include <stdbool.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "lang/parser/lang_parser.h"
#include "lang/parser/parser.h"
#include "lang/semantic_analyzer/definitions.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/optimization/optimize.h"
#include "program/tape.h"
#include "program/tape_binary.h"
#include "struct/map.h"
#include "struct/set.h"
#include "struct/struct_defaults.h"
#include "util/args/commandline.h"
#include "util/args/commandlines.h"
#include "util/args/lib_finder.h"
#include "util/file/file_info.h"
#include "util/file/file_util.h"
#include "util/string.h"
#include "vm/intern.h"
#include "vm/module_manager.h"

Tape *_read_file(const char fn[], bool opt) {
  FileInfo *fi = file_info(fn);

  Q tokens;
  Q_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);

  if (Q_size(&tokens) > 1) {
    fatal_on_token(fn, fi, &tokens);
    return NULL;
  } else {
    SemanticAnalyzer sa;
    semantic_analyzer_init(&sa, semantic_analyzer_init_fn);
    ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

    Tape *tape = tape_create();
    tape_set_external_source(tape, fn);
    semantic_analyzer_produce(&sa, etree, tape);

    if (etree->type != rule_file_level_statement_list) {
      tape_ins_int(tape, EXIT, 0, token_create(TOKEN_WORD, 0, 0, "", 0));
    }

    if (NULL == tape_module_name(tape)) {
      char *dir_path, *module_name_tmp, *ext;
      split_path_file(fn, &dir_path, &module_name_tmp, &ext);
      char *module_name = intern(module_name_tmp);
      RELEASE(module_name_tmp);
      RELEASE(dir_path);
      RELEASE(ext);
      tape_module(tape, token_create(TOKEN_WORD, 0, 0, module_name,
                                     strlen(module_name)));
    }

    semantic_analyzer_delete(&sa, etree);
    semantic_analyzer_finalize(&sa);

    parser_delete_st(&parser, stree);
    parser_finalize(&parser);

    Q_finalize(&tokens);

    if (opt) {
      tape = optimize(tape);
    }
    tape_set_body(tape, fi);
    file_info_delete(fi);
    return tape;
  }
}

void write_tape(const char fn[], const Tape *tape, bool out_zna,
                const char machine_dir[], bool out_znb,
                const char bytecode_dir[], bool minimize) {
  char *path, *file_name, *ext;
  split_path_file(fn, &path, &file_name, &ext);

  if (out_zna && ends_with(fn, ".zn")) {
    make_dir_if_does_not_exist(machine_dir);
    char *file_path = combine_path_file(machine_dir, file_name, ".zna");
    FILE *file = FILE_FN(file_path, "wb");
    tape_write(tape, file, minimize);
    fclose(file);
    RELEASE(file_path);
  }
  if (out_znb && !ends_with(fn, ".znb")) {
    make_dir_if_does_not_exist(bytecode_dir);
    char *file_path = combine_path_file(bytecode_dir, file_name, ".znb");
    FILE *file = FILE_FN(file_path, "wb");
    tape_write_binary(tape, file);
    fclose(file);
    RELEASE(file_path);
  }
  RELEASE(path);
  RELEASE(file_name);
  RELEASE(ext);
}

void compile_to_assembly(const char file_name[], FILE *out) {
  Tape *tape = _read_file(file_name, /* opt= */ true);
  tape_write(tape, out, /* minimize */ true);
  tape_delete(tape);
}

Map *compile(const Set *source_files, bool out_zna, const char machine_dir[],
             bool out_znb, const char bytecode_dir[], bool opt, bool minimize) {
  optimize_init();

  M_iter srcs = set_iter((Set *)source_files);
  Map *src_map = map_create_default();
  for (; has(&srcs); inc(&srcs)) {
    const char *src = value(&srcs);
    Tape *tape = _read_file(src, opt);
    map_insert(src_map, src, tape);
    write_tape(src, tape, out_zna, machine_dir, out_znb, bytecode_dir,
               minimize);
  }

  optimize_finalize();
  return src_map;
}

int zinniac(int argc, const char *argv[]) {
  alloc_init();
  strings_init();

  ArgConfig *config = argconfig_create();
  argconfig_compile(config);
  ArgStore *store = commandline_parse_args(config, argc, argv);

  const bool out_zna = argstore_lookup_bool(store, ArgKey__OUT_ASSEMBLY);
  const bool minimize = argstore_lookup_bool(store, Argkey__MINIMIZE);
  const char *machine_dir =
      argstore_lookup_string(store, ArgKey__ASSEMBLY_OUT_DIR);
  const bool out_znb = argstore_lookup_bool(store, ArgKey__OUT_BINARY);
  const char *bytecode_dir = argstore_lookup_string(store, ArgKey__BIN_OUT_DIR);
  const bool opt = argstore_lookup_bool(store, ArgKey__OPTIMIZE);

#ifdef DEBUG
  Map *src_map =
#endif
      compile(argstore_sources(store), out_zna, machine_dir, out_znb,
              bytecode_dir, opt, minimize);

#ifdef DEBUG
  M_iter tapes = map_iter(src_map);
  for (; has(&tapes); inc(&tapes)) {
    tape_delete(value(&tapes));
  }
  map_delete(src_map);

  argstore_delete(store);
  argconfig_delete(config);
  strings_finalize();
  token_finalize_all();
  alloc_finalize();
#endif

  return EXIT_SUCCESS;
}