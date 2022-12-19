// module_manager.c
//
// Created on: Jul 10, 2020
//     Author: Jeff Manzione

#include "vm/module_manager.h"

#include "alloc/arena/intern.h"
#include "entity/class/class.h"
#include "entity/class/classes_def.h"
#include "entity/function/function.h"
#include "entity/module/module.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/lexer/token.h"
#include "lang/parser/lang_parser.h"
#include "lang/parser/parser.h"
#include "lang/semantic_analyzer/definitions.h"
#include "lang/semantic_analyzer/expression_tree.h"
#include "lang/semantic_analyzer/semantic_analyzer.h"
#include "program/optimization/optimize.h"
#include "program/tape_binary.h"
#include "struct/struct_defaults.h"
#include "util/file/file_info.h"
#include "util/file/file_util.h"
#include "util/string.h"
#include "vm/intern.h"

#define ERROR_LINES_PRECEEDING 1
#define ERROR_LINES_PROCEEDING 2

struct _ModuleInfo {
  Module module;
  FileInfo *fi;
  const char *file_name, *module_name_from_file, *inlined_file;
  bool is_inlined_file, is_loaded, has_native_callback, is_dynamic;
  NativeCallback native_callback;
};

void add_reflection_to_module(ModuleManager *mm, Module *module);

void modulemanager_init(ModuleManager *mm, Heap *heap) {
  ASSERT(NOT_NULL(mm));
  mm->_heap = heap;
  keyedlist_init(&mm->_modules, ModuleInfo, 100);
  set_init_default(&mm->_files_processed);
  mm->intern = intern;
}

void modulemanager_finalize(ModuleManager *mm) {
  ASSERT(NOT_NULL(mm));
  KL_iter iter = keyedlist_iter(&mm->_modules);
  for (; kl_has(&iter); kl_inc(&iter)) {
    ModuleInfo *module_info = (ModuleInfo *)kl_value(&iter);
    DEBUGF("%s", module_info_file_name(module_info));
    if (!module_info->is_loaded) {
      continue;
    }
    if (NULL != module_info->fi) {
      file_info_delete(module_info->fi);
    }
    module_finalize(&module_info->module);
  }
  keyedlist_finalize(&mm->_modules);
  set_finalize(&mm->_files_processed);
}

bool _hydrate_class(Module *module, ClassRef *cref) {
  ASSERT(NOT_NULL(module), NOT_NULL(cref));
  const Class *super = NULL;
  if (alist_len(&cref->supers) > 0) {
    super =
        module_lookup_class(module, *((char **)alist_get(&cref->supers, 0)));
    // Need to reprocess this later since its super is not yet processed.
    if (NULL == super) {
      return false;
    }
  } else {
    super = Class_Object;
  }
  Class *class = module_add_class(module, cref->name, super);
  KL_iter fields = keyedlist_iter(&cref->field_refs);
  for (; kl_has(&fields); kl_inc(&fields)) {
    FieldRef *fref = (FieldRef *)kl_value(&fields);
    class_add_field(class, fref->name);
  }
  KL_iter funcs = keyedlist_iter(&cref->func_refs);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
    class_add_function(class, fref->name, fref->index, fref->is_const,
                       fref->is_async);
  }
  return true;
}

ModuleInfo *_create_moduleinfo(ModuleManager *mm, const char module_name[],
                               const char file_name[]) {
  ASSERT(NOT_NULL(mm));
  ModuleInfo *module_info;
  ModuleInfo *old = (ModuleInfo *)keyedlist_insert(
      &mm->_modules, mm->intern(module_name), (void **)&module_info);
  if (old != NULL) {
    FATALF("Module by name '%s' already exists.", module_name);
  }
  module_info->file_name = (NULL != file_name) ? mm->intern(file_name) : NULL;
  module_info->module_name_from_file = module_name;
  module_info->is_inlined_file = false;
  module_info->inlined_file = NULL;
  return module_info;
}

const char *module_info_file_name(ModuleInfo *mi) { return mi->file_name; }

ModuleInfo *_modulemanager_hydrate(ModuleManager *mm, Tape *tape,
                                   ModuleInfo *module_info) {
  ASSERT(NOT_NULL(mm), NOT_NULL(tape), NOT_NULL(module_info));

  Module *module = &module_info->module;
  module_init(module, tape_module_name(tape), tape);

  KL_iter funcs = tape_functions(tape);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
    module_add_function(module, fref->name, fref->index, fref->is_const,
                        fref->is_async);
  }

  KL_iter classes = tape_classes(tape);
  Q classes_to_process;
  Q_init(&classes_to_process);
  Map waiting_for_class;
  map_init_default(&waiting_for_class);
  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    if (!_hydrate_class(module, cref)) {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  while (Q_size(&classes_to_process) > 0) {
    ClassRef *cref = (ClassRef *)Q_pop(&classes_to_process);
    if (!_hydrate_class(module, cref)) {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  Q_finalize(&classes_to_process);
  map_finalize(&waiting_for_class);
  return module_info;
}

void add_reflection_to_function(Heap *heap, Object *parent, Function *func) {
  if (NULL == func->_reflection) {
    func->_reflection = heap_new(heap, Class_Function);
  }
  func->_reflection->_function_obj = func;
  object_set_member_obj(heap, parent, func->_name, func->_reflection);
}

void _add_reflection_to_class(Heap *heap, Module *module, Class *class) {
  ASSERT(NOT_NULL(heap), NOT_NULL(module), NOT_NULL(class));
  if (NULL == class->_reflection) {
    class->_reflection = heap_new(heap, Class_Class);
  }
  if (NULL == class->_super && class != Class_Object) {
    class->_super = Class_Object;
  }
  class->_reflection->_class_obj = class;
  object_set_member_obj(heap, module->_reflection, class->_name,
                        class->_reflection);

  KL_iter funcs = class_functions(class);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    add_reflection_to_function(heap, class->_reflection, func);
  }

  Object *field_arr = heap_new(heap, Class_Array);
  object_set_member_obj(heap, class->_reflection, FIELDS_PRIVATE_KEY,
                        field_arr);
  KL_iter fields = class_fields(class);
  for (; kl_has(&fields); kl_inc(&fields)) {
    Field *field = (Field *)kl_value(&fields);
    Entity str =
        entity_object(string_new(heap, field->name, strlen(field->name)));
    array_add(heap, field_arr, &str);
  }
}

void add_reflection_to_module(ModuleManager *mm, Module *module) {
  ASSERT(NOT_NULL(mm), NOT_NULL(module));
  module->_reflection = heap_new(mm->_heap, Class_Module);
  module->_reflection->_module_obj = module;
  KL_iter funcs = module_functions(module);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    Function *func = (Function *)kl_value(&funcs);
    add_reflection_to_function(mm->_heap, func->_module->_reflection, func);
  }
  KL_iter classes = module_classes(module);
  for (; kl_has(&classes); kl_inc(&classes)) {
    Class *class = (Class *)kl_value(&classes);
    _add_reflection_to_class(mm->_heap, module, class);
  }
}

FileInfo *_module_info_get_file(ModuleInfo *module_info) {
  if (module_info->is_inlined_file) {
    return file_info_sfile(sfile_open(module_info->inlined_file));
  }
  return file_info(module_info->file_name);
}

Module *_read_jv(ModuleManager *mm, ModuleInfo *module_info) {
  FileInfo *fi = _module_info_get_file(module_info);

  Q tokens;
  Q_init(&tokens);

  lexer_tokenize(fi, &tokens);

  Parser parser;
  parser_init(&parser, rule_file_level_statement_list,
              /*ignore_newline=*/false);
  SyntaxTree *stree = parser_parse(&parser, &tokens);
  stree = parser_prune_newlines(&parser, stree);

  if (Q_size(&tokens) > 1) {
    fatal_on_token(module_info_file_name(module_info), fi, &tokens);
    return NULL;
  } else {
    SemanticAnalyzer sa;
    semantic_analyzer_init(&sa, semantic_analyzer_init_fn);
    ExpressionTree *etree = semantic_analyzer_populate(&sa, stree);

    Tape *tape = tape_create();
    semantic_analyzer_produce(&sa, etree, tape);
    tape_set_body(tape, fi);

    if (NULL == tape_module_name(tape)) {
      tape_module(tape,
                  token_create(TOKEN_WORD, 0, 0,
                               module_info->module_name_from_file,
                               strlen(module_info->module_name_from_file)));
    }

    semantic_analyzer_delete(&sa, etree);
    semantic_analyzer_finalize(&sa);

    parser_delete_st(&parser, stree);
    parser_finalize(&parser);

    Q_finalize(&tokens);

    tape = optimize(tape);
    _modulemanager_hydrate(mm, tape, module_info);
    module_info->fi = fi;
    return &module_info->module;
  }
}

Module *_read_ja(ModuleManager *mm, ModuleInfo *module_info) {
  FileInfo *fi = _module_info_get_file(module_info);
  Q tokens;
  Q_init(&tokens);
  lexer_tokenize(fi, &tokens);

  Tape *tape = tape_create();
  tape_read(tape, &tokens);
  _modulemanager_hydrate(mm, tape, module_info);
  module_info->fi = fi;

  Q_finalize(&tokens);
  return &module_info->module;
}

Module *_read_jb(ModuleManager *mm, ModuleInfo *module_info) {
  FILE *file = fopen(module_info->file_name, "rb");
  if (NULL == file) {
    FATALF("Cannot open file '%s'. Exiting...", module_info->file_name);
  }
  Tape *tape = tape_create();
  tape_read_binary(tape, file);
  _modulemanager_hydrate(mm, tape, module_info);
  return &module_info->module;
}

ModuleInfo *mm_register_module(ModuleManager *mm, const char fn[],
                               const char *inlined_file) {
  return mm_register_module_with_callback(mm, fn, inlined_file, NULL);
}

ModuleInfo *
_create_and_init_moduleinfo(ModuleManager *mm, const char *module_name,
                            const char *file_name, const char *inlined_file,
                            NativeCallback native_callback, bool is_dynamic) {
  ModuleInfo *module_info = _create_moduleinfo(mm, module_name, file_name);
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

ModuleInfo *mm_register_module_with_callback(ModuleManager *mm, const char fn[],
                                             const char *inlined_file,
                                             NativeCallback callback) {
  ASSERT(NOT_NULL(mm), NOT_NULL(fn));

  char *dir_path, *module_name_tmp, *ext;
  split_path_file(fn, &dir_path, &module_name_tmp, &ext);

  char *module_name = mm->intern(module_name_tmp);
  DEALLOC(module_name_tmp);
  // Module already exists.
  ModuleInfo *module_info =
      (ModuleInfo *)keyedlist_lookup(&mm->_modules, module_name);
  if (NULL != module_info) {
    DEALLOC(dir_path);
    DEALLOC(ext);
    return module_info;
  }

  module_info =
      _create_and_init_moduleinfo(mm, module_name, fn, inlined_file, callback,
                                  /*is_dynamic=*/false);

  DEALLOC(dir_path);
  DEALLOC(ext);

  return module_info;
}

ModuleInfo *mm_register_dynamic_module(ModuleManager *mm,
                                       const char module_name[],
                                       NativeCallback init_fn) {
  ModuleInfo *module_info = _create_and_init_moduleinfo(
      mm, mm->intern(module_name), NULL, NULL, init_fn,
      /*is_dynamic*/ true);
  module_init(&module_info->module, mm->intern(module_name), NULL);
  return module_info;
}

Module *modulemanager_load(ModuleManager *mm, ModuleInfo *module_info) {
  Module *module = NULL;
  if (!module_info->is_loaded) {
    if (!module_info->is_dynamic) {
      if (ends_with(module_info->file_name, ".jb")) {
        module = _read_jb(mm, module_info);
      } else if (ends_with(module_info->file_name, ".ja")) {
        module = _read_ja(mm, module_info);
      } else if (ends_with(module_info->file_name, ".jv")) {
        module = _read_jv(mm, module_info);
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

Module *modulemanager_lookup(ModuleManager *mm, const char module_name[]) {
  ModuleInfo *module_info =
      (ModuleInfo *)keyedlist_lookup(&mm->_modules, mm->intern(module_name));
  if (NULL == module_info) {
    return NULL;
  }
  return modulemanager_load(mm, module_info);
}

const FileInfo *modulemanager_get_fileinfo(const ModuleManager *mm,
                                           const Module *m) {
  ASSERT(NOT_NULL(mm), NOT_NULL(m), NOT_NULL(m->_name));
  const ModuleInfo *mi =
      (ModuleInfo *)keyedlist_lookup((KeyedList *)&mm->_modules, m->_name);
  if (NULL == mi) {
    return NULL;
  }
  return mi->fi;
}

void modulemanager_update_module(ModuleManager *mm, Module *m,
                                 Map *new_classes) {
  ASSERT(NOT_NULL(mm), NOT_NULL(m), NOT_NULL(new_classes));
  Tape *tape = (Tape *)m->_tape; // bless

  KL_iter classes = tape_classes(tape);
  Q classes_to_process;
  Q_init(&classes_to_process);

  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    Class *c = (Class *)module_lookup_class(m, cref->name); // bless
    if (NULL != c) {
      continue;
    }
    if (_hydrate_class(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name); // bless
      map_insert(new_classes, cref->name, c);
      _add_reflection_to_class(mm->_heap, m, c);
    } else {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
  while (Q_size(&classes_to_process) > 0) {
    ClassRef *cref = (ClassRef *)Q_pop(&classes_to_process);
    Class *c = (Class *)module_lookup_class(m, cref->name); // bless
    if (_hydrate_class(m, cref)) {
      c = (Class *)module_lookup_class(m, cref->name); // bless
      map_insert(new_classes, cref->name, c);
      _add_reflection_to_class(mm->_heap, m, c);
    } else {
      *Q_add_last(&classes_to_process) = cref;
    }
  }
}

// Returns the first token if only newlines are present in [tokens].
Token *_find_first_non_newline_token(Q *tokens) {
  Token *token = (Token *)Q_get(tokens, 0);
  Q_iter iter = Q_iterator(tokens);
  for (; Q_has(&iter); Q_inc(&iter)) {
    Token *tok = *((Token **)Q_value(&iter));
    if (tok->type != TOKEN_NEWLINE) {
      token = tok;
      break;
    }
  }
  return token;
}

void fatal_on_token(const char file_name[], FileInfo *fi, Q *tokens) {
  Token *token = _find_first_non_newline_token(tokens);
  // Apparently LineInfo is 1-based, but Token is 0-based here.
  const int line_in_file = token->line + 1;
  const int error_line_start = max(0, line_in_file - ERROR_LINES_PRECEEDING);
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