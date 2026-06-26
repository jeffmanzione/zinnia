// compiler.c
//
// Created on: Oct 28, 2020
//     Author: Jeff Manzione

#include "zinnia/compile/compile.h"

#include <stdbool.h>
#include <stdlib.h>

#include "file-utils/file_info.h"
#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "language-tools/intern.h"
#include "language-tools/lexer/token.h"
#include "language-tools/parser/parser.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "language-tools/semantic_analyzer/semantic_analyzer.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/lang/parser/lang_parser.h"
#include "zinnia/lang/semantic_analyzer/definitions.h"
#include "zinnia/program/optimization/optimize.h"
#include "zinnia/program/tape.h"
#include "zinnia/program/tape_binary.h"
#include "zinnia/util/args/commandlines.h"
#include "zinnia/util/args/lib_finder.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/module_manager.h"

IMPL_MAPLIKE(TapeNameMap, char *, Tape *);

Tape *read_file_(const char fn[], bool opt) {
  FileInfo *fi = file_info(fn);

  TokenArray tokens;
  TokenArray_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);

  if (TokenArray_size(&tokens) > 1) {
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
      const char *module_name = global_intern(module_name_tmp);
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

    TokenArray_finalize(&tokens);

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
  Tape *tape = read_file_(file_name, /* opt= */ true);
  tape_write(tape, out, /* minimize */ true);
  tape_delete(tape);
}

void compile(const SourceNameSet *source_files, bool out_zna,
             const char machine_dir[], bool out_znb, const char bytecode_dir[],
             bool opt, bool minimize, TapeNameMap *src_map) {
  optimize_init();

  SourceNameSetIterator srcs;
  SourceNameSet_iterator(&srcs, source_files);
  for (; SourceNameSet_has_next(&srcs); SourceNameSet_next(&srcs)) {
    const char *src = *SourceNameSet_value(&srcs);
    Tape *tape = read_file_(src, opt);
    TapeNameMap_insert(src_map, src, sizeof(char *), tape);
    write_tape(src, tape, out_zna, machine_dir, out_znb, bytecode_dir,
               minimize);
  }

  optimize_finalize();
}

int zinniac(int argc, const char *argv[]) {
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

  TapeNameMap src_map;
  TapeNameMap_init(&src_map, hash_interned_string, compare_interned_strings);
  compile(argstore_sources(store), out_zna, machine_dir, out_znb, bytecode_dir,
          opt, minimize, &src_map);

#ifdef DEBUG
  TapeNameMapIterator tapes;
  TapeNameMap_iterator(&tapes, &src_map);
  for (; TapeNameMap_has_entry(&tapes); TapeNameMap_next_entry(&tapes)) {
    tape_delete(*TapeNameMap_mutable_value(&tapes));
  }
  TapeNameMap_finalize(&src_map);

  argstore_delete(store);
  argconfig_delete(config);
  strings_finalize();
  token_finalize_all();
#endif

  return EXIT_SUCCESS;
}