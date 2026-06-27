// module_manager.c
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/module_manager.h"

#include <string.h>

#include "file-utils/file_info.h"
#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "language-tools/intern.h"
#include "language-tools/lexer/token.h"
#include "language-tools/parser/parser.h"
#include "language-tools/semantic_analyzer/expression_tree.h"
#include "language-tools/semantic_analyzer/semantic_analyzer.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/class.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/function/function.h"
#include "zinnia/entity/module/module.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/lang/lexer/lang_lexer.h"
#include "zinnia/lang/parser/lang_parser.h"
#include "zinnia/lang/semantic_analyzer/definitions.h"
#include "zinnia/program/optimization/optimize.h"
#include "zinnia/program/tape_binary.h"
#include "zinnia/util/file.h"
#include "zinnia/vm/intern.h"

IMPL_MAPLIKE(ClassPtrMap, char *, Class *);

#define ERROR_LINES_PRECEEDING 1
#define ERROR_LINES_PROCEEDING 2

struct ModuleInfo_ {
  Module module;
  FileInfo *fi;
  const char *file_path, *relative_file_path, *module_name_from_file,
      *inlined_file, *key;
  bool is_inlined_file, is_loaded, has_native_callback, is_dynamic;
  NativeModuleInitFn native_callback;
};

IMPL_STABLE_MAPLIKE(ModuleInfoMap, char *, ModuleInfo);

bool verify_init_fn_signature(NativeModuleInitFn fn) { return fn != NULL; }

void add_reflection_to_module(ModuleManager *mm, Module *module);

void modulemanager_init(ModuleManager *mm, Heap *heap) {
  ASSERT(mm != NULL);
  mm->_heap = heap;
  mm->_modules = malloc(sizeof(ModuleInfoMap));
  ModuleInfoMap_init(mm->_modules, hash_interned_string,
                     compare_interned_strings);
  // set_init_default(&mm->_files_processed);
  mm->intern = global_intern;
}

void modulemanager_finalize(ModuleManager *mm) {
  ASSERT(mm != NULL);
  ModuleInfoMapIterator iter;
  ModuleInfoMap_iterator(&iter, mm->_modules);
  for (; ModuleInfoMap_has_entry(&iter); ModuleInfoMap_next_entry(&iter)) {
    ModuleInfo *module_info = ModuleInfoMap_mutable_value(&iter);
    if (!module_info->is_loaded) {
      continue;
    }
    if (NULL != module_info->inlined_file) {
      RELEASE((ModuleInfo *)module_info->inlined_file);
    }
    if (NULL != module_info->fi) {
      file_info_delete(module_info->fi);
    }
    module_finalize(&module_info->module);
  }
  ModuleInfoMap_finalize(mm->_modules);
  RELEASE(mm->_modules);
  // set_finalize(&mm->_files_processed);
}

bool hydrate_class_(Module *module, ClassRef *cref) {
  ASSERT(module != NULL);
  ASSERT(cref != NULL);
  const Class *super = NULL;
  if (CharPtrArray_size(&cref->supers) > 0) {
    super = module_lookup_class(module,
                                CharPtrArray_get_unchecked(&cref->supers, 0));
    // Need to reprocess this later since its super is not yet processed.
    if (NULL == super) {
      return false;
    }
  } else {
    super = Class_Object;
  }
  Class *class = module_add_class(module, cref->name, super);
  FieldRefMapIterator fields;
  FieldRefMap_iterator(&fields, &cref->field_refs);
  for (; FieldRefMap_has_entry(&fields); FieldRefMap_next_entry(&fields)) {
    FieldRef *fref = FieldRefMap_mutable_value(&fields);
    class_add_field(class, fref->name);
  }
  FunctionRefMapIOIterator funcs;
  FunctionRefMap_io_iterator(&funcs, &cref->func_refs);
  for (; FunctionRefMap_io_has_next(&funcs); FunctionRefMap_io_next(&funcs)) {
    FunctionRef *fref = FunctionRefMap_io_mutable_value(&funcs);
    class_add_function(class, fref->name, fref->index, fref->is_const,
                       fref->is_async);
  }
  return true;
}

ModuleInfo *create_moduleinfo_(ModuleManager *mm, const char module_name[],
                               const char module_key[], const char full_path[],
                               const char relative_path[]) {
  ASSERT(mm != NULL);
  ModuleInfo *module_info;
  if (!ModuleInfoMap_insert(mm->_modules, mm->intern(module_key),
                            sizeof(char *), &module_info)) {
    FATALF("Module by name '%s' already exists.", module_name);
  }
  module_info->file_path = (NULL != full_path) ? mm->intern(full_path) : NULL;
  module_info->relative_file_path =
      (NULL != relative_path) ? mm->intern(relative_path) : NULL;
  module_info->key = module_key;
  module_info->module_name_from_file = module_name;
  module_info->is_inlined_file = false;
  module_info->inlined_file = NULL;
  return module_info;
}

const char *module_info_file_name(ModuleInfo *mi) { return mi->file_path; }

ModuleInfo *modulemanager_hydrate_(ModuleManager *mm, Tape *tape,
                                   ModuleInfo *module_info) {
  ASSERT(mm != NULL);
  ASSERT(tape != NULL);
  ASSERT(module_info != NULL);

  Module *module = &module_info->module;
  module_init(module, tape_module_name(tape), module_info->file_path,
              module_info->relative_file_path, module_info->key, tape);

  FunctionRefMapIOIterator funcs = tape_functions(tape);
  for (; FunctionRefMap_io_has_next(&funcs); FunctionRefMap_io_next(&funcs)) {
    FunctionRef *fref = FunctionRefMap_io_mutable_value(&funcs);
    module_add_function(module, fref->name, fref->index, fref->is_const,
                        fref->is_async);
  }

  ClassRefMapIOIterator classes = tape_classes(tape);
  VoidPtrArray classes_to_process;
  VoidPtrArray_init(&classes_to_process);
  // Map waiting_for_class;
  // map_init_default(&waiting_for_class);
  for (; ClassRefMap_io_has_next(&classes); ClassRefMap_io_next(&classes)) {
    ClassRef *cref = ClassRefMap_io_mutable_value(&classes);
    if (!hydrate_class_(module, cref)) {
      *VoidPtrArray_push_back_ref(&classes_to_process) = cref;
    }
  }
  while (VoidPtrArray_size(&classes_to_process) > 0) {
    ClassRef *cref =
        (ClassRef *)VoidPtrArray_pop_front_unchecked(&classes_to_process);
    if (!hydrate_class_(module, cref)) {
      *VoidPtrArray_push_back_ref(&classes_to_process) = cref;
    }
  }
  VoidPtrArray_finalize(&classes_to_process);
  // map_finalize(&waiting_for_class);
  return module_info;
}

void add_reflection_to_function(Heap *heap, Object *parent, Function *func) {
  if (NULL == func->_reflection) {
    func->_reflection = heap_new(heap, Class_Function);
  }
  func->_reflection->_function_obj = func;
  object_set_member_obj(heap, parent, func->_name, func->_reflection);

  object_set_member_obj(heap, func->_reflection, ANNOTATIONS_KEY,
                        heap_new(heap, Class_Array));
}

void add_reflection_to_class_(Heap *heap, Module *module, Class *class) {
  ASSERT(heap != NULL);
  ASSERT(module != NULL);
  ASSERT(class != NULL);
  if (NULL == class->_reflection) {
    class->_reflection = heap_new(heap, Class_Class);
  }
  if (NULL == class->_super && class != Class_Object) {
    class->_super = Class_Object;
  }
  class->_reflection->_class_obj = class;
  object_set_member_obj(heap, module->_reflection, class->_name,
                        class->_reflection);

  FunctionMapIterator funcs = class_functions(class);
  for (; FunctionMap_has_entry(&funcs); FunctionMap_next_entry(&funcs)) {
    Function *func = FunctionMap_mutable_value(&funcs);
    add_reflection_to_function(heap, class->_reflection, func);
  }

  Object *field_arr = heap_new(heap, Class_Array);
  object_set_member_obj(heap, class->_reflection, FIELDS_PRIVATE_KEY,
                        field_arr);
  FieldMapIterator fields = class_fields(class);
  for (; FieldMap_has_entry(&fields); FieldMap_next_entry(&fields)) {
    Field *field = FieldMap_mutable_value(&fields);
    Entity str = entity_object(
        istring_new_no_intern(heap, field->name, strlen(field->name)));
    array_add(heap, field_arr, &str);
  }

  object_set_member_obj(heap, class->_reflection, ANNOTATIONS_KEY,
                        heap_new(heap, Class_Array));
}

void add_reflection_to_module(ModuleManager *mm, Module *module) {
  ASSERT(mm != NULL);
  ASSERT(module != NULL);
  module->_reflection = heap_new(mm->_heap, Class_Module);
  module->_reflection->_module_obj = module;
  FunctionMapIterator funcs = module_functions(module);
  for (; FunctionMap_has_entry(&funcs); FunctionMap_next_entry(&funcs)) {
    Function *func = FunctionMap_mutable_value(&funcs);
    add_reflection_to_function(mm->_heap, func->_module->_reflection, func);
  }
  ClassMapIterator classes = module_classes(module);
  for (; ClassMap_has_entry(&classes); ClassMap_next_entry(&classes)) {
    Class *class = ClassMap_mutable_value(&classes);
    add_reflection_to_class_(mm->_heap, module, class);
  }
}

FileInfo *module_info_get_file_(ModuleInfo *module_info) {
  if (module_info->is_inlined_file) {
    return file_info_sfile(sfile_open(module_info->inlined_file));
  }
  return file_info(module_info->file_path);
}

Module *_read_zn(ModuleManager *mm, ModuleInfo *module_info) {
  FileInfo *fi = module_info_get_file_(module_info);

  TokenArray tokens;
  TokenArray_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);

  if (TokenArray_size(&tokens) > 1) {
    fatal_on_token(module_info_file_name(module_info), fi, &tokens);
    return NULL;
  } else {
    SemanticAnalyzer sa;
    semantic_analyzer_init(&sa, semantic_analyzer_init_fn);
    ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

    Tape *tape = tape_create();
    semantic_analyzer_produce(&sa, etree, tape);
    tape_set_body(tape, fi);

    if (etree->type != rule_file_level_statement_list) {
      tape_ins_int(tape, EXIT, 0, token_create(TOKEN_WORD, 0, 0, "", 0));
    }

    if (NULL == tape_module_name(tape)) {
      tape_module(
          tape,
          token_create(TOKEN_WORD, 0, 0, module_info->module_name_from_file,
                       strlen(module_info->module_name_from_file)));
    }

    semantic_analyzer_delete(&sa, etree);
    semantic_analyzer_finalize(&sa);

    parser_delete_st(&parser, stree);
    parser_finalize(&parser);

    TokenArray_finalize(&tokens);

    tape = optimize(tape);
    modulemanager_hydrate_(mm, tape, module_info);
    module_info->fi = fi;
    return &module_info->module;
  }
}

Module *_read_zna(ModuleManager *mm, ModuleInfo *module_info) {
  FileInfo *fi = module_info_get_file_(module_info);
  TokenArray tokens;
  TokenArray_init(&tokens);
  lexer_tokenize(fi, &tokens);

  Tape *tape = tape_create();
  tape_read(tape, &tokens);
  modulemanager_hydrate_(mm, tape, module_info);
  module_info->fi = fi;

  TokenArray_finalize(&tokens);
  return &module_info->module;
}

Module *_read_znb(ModuleManager *mm, ModuleInfo *module_info) {
  FILE *file = FILE_FN(module_info->file_path, "rb");
  if (NULL == file) {
    FATALF("Cannot open file '%s'. Exiting...", module_info->file_path);
  }
  Tape *tape = tape_create();
  tape_read_binary(tape, file);
  modulemanager_hydrate_(mm, tape, module_info);
  return &module_info->module;
}

ModuleInfo *mm_register_module(ModuleManager *mm, const char full_path[],
                               const char relative_path[],
                               const char *inlined_file_segs[],
                               int num_inlined_file_segs) {
  return mm_register_module_with_callback(mm, full_path, relative_path,
                                          inlined_file_segs,
                                          num_inlined_file_segs, NULL);
}

ModuleInfo *create_and_init_moduleinfo_(
    ModuleManager *mm, const char *module_name, const char *module_key,
    const char *full_path, const char *relative_path, const char *inlined_file,
    NativeModuleInitFn native_callback, bool is_dynamic) {
  ModuleInfo *module_info =
      create_moduleinfo_(mm, module_name, module_key, full_path, relative_path);
  module_info->has_native_callback = native_callback != NULL;
  module_info->native_callback = native_callback;
  module_info->is_loaded = false;
  module_info->is_dynamic = is_dynamic;
  if (NULL != inlined_file) {
    module_info->is_inlined_file = true;
    module_info->inlined_file = inlined_file;
  }
  return module_info;
}

ModuleInfo *mm_register_module_with_callback(ModuleManager *mm,
                                             const char full_path[],
                                             const char relative_path[],
                                             const char *inlined_file_segs[],
                                             int num_inlined_file_segs,
                                             NativeModuleInitFn callback) {
  ASSERT(mm != NULL);
  ASSERT(full_path != NULL);
  ASSERT(relative_path != NULL);

  char *dir_path, *module_name_tmp, *ext;
  split_path_file(full_path, &dir_path, &module_name_tmp, &ext);

  const char *module_name = mm->intern(module_name_tmp);

  const char *module_key;

  // lib is a magic directory name.
  if (NULL == dir_path || strlen(dir_path) == 0 ||
      0 == strcmp(dir_path, "zinnia/lib/")) {
    module_key = module_name;
  } else {
    char *path_no_ext = strdup(relative_path);
    replace_backslashes(path_no_ext);
    if (ends_with(path_no_ext, ".zn")) {
      path_no_ext[strlen(path_no_ext) - 3] = '\0';
    } else {
      path_no_ext[strlen(path_no_ext) - 4] = '\0';
    }
    int offset = starts_with(path_no_ext, "./") ? 2 : 0;
    module_key = mm->intern(path_no_ext + offset);
    free(path_no_ext);
  }

  RELEASE(module_name_tmp);
  // Module already exists.
  ModuleInfo *module_info =
      ModuleInfoMap_find_ref(mm->_modules, module_key, sizeof(char *));
  if (NULL != module_info) {
    RELEASE(dir_path);
    RELEASE(ext);
    return module_info;
  }

  char *inlined_file = NULL;
  if (inlined_file_segs != NULL) {
    int total_file_content_len = 0;
    for (int i = 0; i < num_inlined_file_segs; ++i) {
      total_file_content_len += strlen(inlined_file_segs[i]);
    }
    inlined_file = CNEW_ARR(char, total_file_content_len + 1);
    inlined_file[0] = 0x0;
    for (int i = 0; i < num_inlined_file_segs; ++i) {
      strcat(inlined_file, inlined_file_segs[i]);
    }
  }
  module_info =
      create_and_init_moduleinfo_(mm, module_name, module_key, full_path,
                                  relative_path, inlined_file, callback,
                                  /*is_dynamic=*/false);
  RELEASE(dir_path);
  RELEASE(ext);

  return module_info;
}

ModuleInfo *mm_register_dynamic_module(ModuleManager *mm,
                                       const char module_name[],
                                       NativeModuleInitFn init_fn) {
  ModuleInfo *module_info = create_and_init_moduleinfo_(
      mm, mm->intern(module_name), mm->intern(module_name), NULL, NULL, NULL,
      init_fn,
      /*is_dynamic*/ true);
  module_init(&module_info->module, mm->intern(module_name), NULL, NULL,
              module_name, NULL);
  return module_info;
}

Module *modulemanager_load(ModuleManager *mm, ModuleInfo *module_info) {
  Module *module = NULL;
  if (!module_info->is_loaded) {
    if (!module_info->is_dynamic) {
      if (ends_with(module_info->file_path, ".znb")) {
        module = _read_znb(mm, module_info);
      } else if (ends_with(module_info->file_path, ".zna")) {
        module = _read_zna(mm, module_info);
      } else if (ends_with(module_info->file_path, ".zn")) {
        module = _read_zn(mm, module_info);
      } else {
        FATALF("Unknown file type.");
      }
    } else {
      module = &module_info->module;
    }
    module_info->is_loaded = true;
    if (module_info->has_native_callback) {
      module_info->native_callback(mm, module);
    }
    add_reflection_to_module(mm, module);
    heap_make_root(mm->_heap, module->_reflection);
  }
  return &module_info->module;
}

Module *modulemanager_lookup(ModuleManager *mm, const char module_key[]) {
  ModuleInfo *module_info = ModuleInfoMap_find_ref(
      mm->_modules, mm->intern(module_key), sizeof(char *));
  if (NULL == module_info) {
    return NULL;
  }
  return modulemanager_load(mm, module_info);
}

const FileInfo *modulemanager_get_fileinfo(const ModuleManager *mm,
                                           const Module *m) {
  ASSERT(mm != NULL);
  ASSERT(m != NULL);
  ASSERT(m->_relative_path != NULL);
  const ModuleInfo *mi =
      ModuleInfoMap_find_ref(mm->_modules, m->_key, sizeof(char *));
  if (NULL == mi) {
    return NULL;
  }
  return mi->fi;
}

void modulemanager_update_module(ModuleManager *mm, Module *m,
                                 ClassPtrMap *new_classes) {
  ASSERT(mm != NULL);
  ASSERT(m != NULL);
  ASSERT(new_classes != NULL);
  Tape *tape = (Tape *)m->_tape;  // bless

  ClassRefMapIOIterator classes = tape_classes(tape);
  ClassRefPtrArray classes_to_process;
  ClassRefPtrArray_init(&classes_to_process);

  for (; ClassRefMap_io_has_next(&classes); ClassRefMap_io_next(&classes)) {
    ClassRef *cref = ClassRefMap_io_mutable_value(&classes);
    Class *c = (Class *)module_lookup_class(m, cref->name);  // bless
    if (NULL != c) {
      continue;
    }
    if (hydrate_class_(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name);  // bless
      ClassPtrMap_insert(new_classes, cref->name, sizeof(char *), c);
      add_reflection_to_class_(mm->_heap, m, c);
    } else {
      ClassRefPtrArray_push_back(&classes_to_process, cref);
    }
  }
  while (ClassRefPtrArray_size(&classes_to_process) > 0) {
    ClassRef *cref = ClassRefPtrArray_pop_front_unchecked(&classes_to_process);
    Class *c = (Class *)module_lookup_class(m, cref->name);  // bless
    if (hydrate_class_(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name);  // bless
      ClassPtrMap_insert(new_classes, cref->name, sizeof(char *), c);
      add_reflection_to_class_(mm->_heap, m, c);
    } else {
      ClassRefPtrArray_push_back(&classes_to_process, cref);
    }
  }
}

// Returns the first token if only newlines are present in [tokens].
const Token *find_first_non_newline_token_(TokenArray *tokens) {
  const Token *token = TokenArray_get_unchecked(tokens, 0);
  TokenArrayIterator iter;
  TokenArray_iterator(&iter, tokens);
  for (; TokenArray_has_next(&iter); TokenArray_next(&iter)) {
    const Token *tok = *TokenArray_value(&iter);
    if (tok->type != TOKEN_NEWLINE) {
      token = tok;
      break;
    }
  }
  return token;
}

void fatal_on_token(const char file_name[], FileInfo *fi, TokenArray *tokens) {
  const Token *token = find_first_non_newline_token_(tokens);
  // Apparently LineInfo is 1-based, but Token is 0-based here.
  const int line_in_file = token->line + 1;
  const int error_line_start = line_in_file - ERROR_LINES_PRECEEDING < 0
                                   ? 0
                                   : line_in_file - ERROR_LINES_PRECEEDING;
  const int error_line_end = line_in_file + ERROR_LINES_PROCEEDING;

  fprintf(stderr, "Could not parse line %d, file: %s\n", line_in_file,
          file_name);
  for (int i = error_line_start; i <= error_line_end; ++i) {
    const LineInfo *li = file_info_lookup(fi, i);
    if (NULL == li) {
      continue;
    }
    const char *pre_line = (i == line_in_file) ? ">> " : "   ";
    fprintf(stderr, "%s%d: %s", pre_line, i, li->line_text);
  }
  fprintf(stderr, "\nFatally exiting.");
  exit(1);
}