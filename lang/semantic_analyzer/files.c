#include "lang/semantic_analyzer/definitions.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func);
FunctionDef populate_function_variant(
    const SyntaxTree *stree, RuleFn def, RuleFn signature_with_qualifier,
    RuleFn signature_no_qualifier, RuleFn fn_identifier,
    RuleFn function_arguments_no_args, RuleFn function_arguments_present,
    FuncDefPopulator def_populator, FuncArgumentsPopulator args_populator);

FunctionDef populate_function(const SyntaxTree *stree);

void populate_class_def(ClassSignature *def, const SyntaxTree *stree) {
  def->parent_classes = alist_create(ClassName, 2);
  // No parent classes.
  if (IS_SYNTAX(stree, rule_identifier)) {
    def->name.token = stree->token;
  } else if (IS_SYNTAX(stree, rule_class_name_and_inheritance)) {
    const SyntaxTree *class_inheritance = stree;
    ASSERT(IS_SYNTAX(class_inheritance->first, rule_identifier));
    def->name.token = class_inheritance->first->token;
    ASSERT(IS_SYNTAX(class_inheritance->second, rule_parent_classes));
    if (IS_SYNTAX(class_inheritance->second->second, rule_identifier)) {
      ClassName name = {.token = class_inheritance->second->second->token};
      alist_append(def->parent_classes, &name);
    } else {
      ASSERT(
          IS_SYNTAX(class_inheritance->second->second, rule_parent_class_list));
      ClassName name = {.token =
                            class_inheritance->second->second->first->token};
      alist_append(def->parent_classes, &name);
      const SyntaxTree *parent_class =
          class_inheritance->second->second->second;
      while (true) {
        if (IS_SYNTAX(parent_class, rule_parent_class_list1)) {
          if (IS_TOKEN(parent_class->first, SYMBOL_COMMA)) {
            name.token = parent_class->second->token;
            alist_append(def->parent_classes, &name);
            break;
          } else {
            name.token = parent_class->first->second->token;
            alist_append(def->parent_classes, &name);
            parent_class = parent_class->second;
          }
        } else {
          ASSERT(IS_SYNTAX(parent_class, rule_identifier));
          name.token = parent_class->token;
          alist_append(def->parent_classes, &name);
          break;
        }
      }
    }
  } else {
    ERROR("Unknown class name composition.");
  }
}

Argument populate_constructor_argument(SemanticAnalyzer *analyzer,
                                       const SyntaxTree *stree) {
  Argument arg = {.is_const = false,
                  .const_token = NULL,
                  .field_token = NULL,
                  .is_field = false,
                  .has_default = false,
                  .default_value = NULL};
  const SyntaxTree *argument = stree;
  if (IS_SYNTAX(argument, rule_const_new_argument)) {
    arg.is_const = true;
    arg.const_token = argument->first->token;
    argument = argument->second;
  }
  if (IS_SYNTAX(argument, rule_new_arg_elt_with_default)) {
    arg.has_default = true;
    arg.default_value =
        semantic_analyzer_populate(analyzer, argument->second->second);
    argument = argument->first;
  }
  if (IS_SYNTAX(argument, rule_new_field_arg)) {
    arg.arg_name = argument->second->token;
    arg.field_token = argument->first->token;
    arg.is_field = true;
  } else {
    ASSERT(IS_SYNTAX(argument, rule_identifier));
    arg.arg_name = argument->token;
  }
  return arg;
}

Arguments set_constructor_args(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree, const Token *token) {
  Arguments args = {.token = token, .count_required = 0, .count_optional = 0};
  args.args = alist_create(Argument, 4);
  if (!IS_SYNTAX(stree, rule_new_argument_list)) {
    Argument arg = populate_constructor_argument(analyzer, stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg = populate_constructor_argument(analyzer, stree->first);
  add_arg(&args, &arg);
  const SyntaxTree *cur = stree->second;
  while (true) {
    if (IS_TOKEN(cur->first, SYMBOL_COMMA)) {
      // Must be last arg.
      Argument arg = populate_constructor_argument(analyzer, cur->second);
      add_arg(&args, &arg);
      break;
    }
    Argument arg = populate_constructor_argument(analyzer, cur->first->second);
    add_arg(&args, &arg);
    cur = cur->second;
  }
  return args;
}

void set_method_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  ASSERT(IS_SYNTAX(fn_identifier, rule_method_identifier));
  func->def_token = fn_identifier->first->token;
  const SyntaxTree *fn_name = fn_identifier->second;
  if (IS_SYNTAX(fn_name, rule_identifier)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NONE;
  } else if (IS_TOKEN(fn_name, SYMBOL_EQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__EQUIV;
  } else if (IS_TOKEN(fn_name, SYMBOL_NEQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NEQUIV;
  } else if (IS_TOKEN2(fn_name, SYMBOL_LBRACKET, SYMBOL_RBRACKET)) {
    func->fn_name = fn_name->first->token;
    func->special_method = SpecialMethod__ARRAY_INDEX;
  } else if (IS_TOKEN3(fn_name, SYMBOL_LBRACKET, SYMBOL_RBRACKET,
                       SYMBOL_EQUALS)) {
    func->fn_name = fn_name->first->token;
    func->special_method = SpecialMethod__ARRAY_SET;
  } else {
    ERROR("Unknown method name type.");
  }
}

FunctionDef populate_method(SemanticAnalyzer *analyzer,
                            const SyntaxTree *stree) {
  return populate_function_variant(
      analyzer, stree, rule_method_definition,
      rule_method_signature_with_qualifier, rule_method_signature_no_qualifier,
      rule_method_identifier, rule_function_arguments_no_args,
      rule_function_arguments_present, set_method_def, set_function_args);
}

void set_new_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  ASSERT(IS_SYNTAX(fn_identifier, rule_new_expression));
  func->def_token = fn_identifier->token;
  func->fn_name = fn_identifier->token;
}

FunctionDef populate_constructor(SemanticAnalyzer *analyzer,
                                 const SyntaxTree *stree) {
  return populate_function_variant(
      analyzer, stree, rule_new_definition, rule_new_signature_const,
      rule_new_signature_nonconst, rule_new_expression,
      rule_new_arguments_no_args, rule_new_arguments_present, set_new_def,
      set_constructor_args);
}

FieldDef populate_field_statement(const Token *field_token,
                                  const SyntaxTree *stree) {
  FieldDef field = {.name = stree->token, .field_token = field_token};
  return field;
}

void populate_field_statements(const SyntaxTree *stree, ClassDef *class) {
  ASSERT(IS_TOKEN(stree->first, KEYWORD_FIELD));
  const Token *field_token = stree->first->token;

  if (!IS_SYNTAX(stree->second, rule_identifier_list)) {
    ASSERT(IS_SYNTAX(stree->second, rule_identifier));
    FieldDef field = populate_field_statement(field_token, stree->second);
    alist_append(class->fields, &field);
    return;
  }
  FieldDef field = populate_field_statement(field_token, stree->second->first);
  alist_append(class->fields, &field);

  const SyntaxTree *statement = stree->second->second;
  while (true) {
    if (IS_TOKEN(statement->first, SYMBOL_COMMA)) {
      FieldDef field = populate_field_statement(field_token, statement->second);
      alist_append(class->fields, &field);
      break;
    }
    FieldDef field =
        populate_field_statement(field_token, statement->first->second);
    alist_append(class->fields, &field);
    statement = statement->second;
  }
}

void populate_class_statement(SemanticAnalyzer *analyzer, ClassDef *class,
                              const SyntaxTree *stree) {
  if (IS_SYNTAX(stree, rule_field_statement)) {
    populate_field_statements(stree, class);
  } else if (IS_SYNTAX(stree, rule_method_definition)) {
    FunctionDef method = populate_method(analyzer, stree);
    alist_append(class->methods, &method);
  } else if (IS_SYNTAX(stree, rule_new_definition)) {
    class->constructor = populate_constructor(analyzer, stree);
    class->has_constructor = true;
    if (class->constructor.has_args) {
      AL_iter args = alist_iter(class->constructor.args.args);
      for (; al_has(&args); al_inc(&args)) {
        Argument *arg = (Argument *)al_value(&args);
        if (!arg->is_field) {
          continue;
        }
        FieldDef *field = (FieldDef *)alist_add(class->fields);
        field->name = arg->arg_name;
        field->field_token = arg->arg_name;
      }
    }
  } else {
    ERROR("Unknown class_statement.");
  }
}

void populate_class_statements(SemanticAnalyzer *analyzer, ClassDef *class,
                               const SyntaxTree *stree) {
  ASSERT(IS_TOKEN(stree->first, SYMBOL_LBRACE));
  if (IS_TOKEN(stree->second, SYMBOL_RBRACE)) {
    // Empty body.
    return;
  }
  if (!IS_SYNTAX(stree->second->first, rule_class_statement_list)) {
    populate_class_statement(analyzer, class, stree->second->first);
    return;
  }
  populate_class_statement(analyzer, class, stree->second->first->first);
  const SyntaxTree *statement = stree->second->first->second;
  while (true) {
    if (!IS_SYNTAX(statement, rule_class_statement_list1)) {
      populate_class_statement(analyzer, class, statement);
      break;
    }
    populate_class_statement(analyzer, class, statement->first);
    statement = statement->second;
  }
}

ClassDef populate_class(SemanticAnalyzer *analyzer, const SyntaxTree *stree) {
  // TODO HANDLE ANNOTATIONS.
  ASSERT(!IS_LEAF(stree->first), IS_TOKEN(stree->first->first, CLASS));
  ClassDef class;
  class.has_constructor = false;
  class.has_annot = false;
  class.fields = alist_create(FieldDef, 4);
  class.methods = alist_create(FunctionDef, 6);
  populate_class_def(&class.def, stree->first->second);

  const SyntaxTree *body = stree->second;
  if (IS_SYNTAX(body, rule_class_compound_statement)) {
    populate_class_statements(analyzer, &class, body);
  } else {
    populate_class_statement(analyzer, &class, body);
  }
  return class;
}

void delete_annotation(SemanticAnalyzer *analyzer, Annotation *annot) {
  if (annot->has_args && NULL != annot->args_tuple) {
    semantic_analyzer_delete(analyzer, annot->args_tuple);
  }
}

void delete_class(SemanticAnalyzer *analyzer, ClassDef *class) {
  if (class->has_constructor) {
    delete_function(analyzer, &class->constructor);
  }
  alist_delete(class->def.parent_classes);
  alist_delete(class->fields);
  int i;
  for (i = 0; i < alist_len(class->methods); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(class->methods, i);
    delete_function(analyzer, func);
  }
  alist_delete(class->methods);
  if (class->has_annot) {
    delete_annotation(analyzer, &class->annot);
  }
}

int produce_constructor(SemanticAnalyzer *analyzer, ClassDef *class,
                        Tape *tape) {
  int num_ins = 0;

  if (class->has_constructor) {
    num_ins += tape_label(tape, class->constructor.fn_name);
  } else {
    num_ins += tape_label_text(tape, CONSTRUCTOR_KEY);
  }

  int num_fields = alist_len(class->fields);
  if (num_fields > 0) {
    num_ins += tape_ins_no_arg(tape, PUSH, class->def.name.token);
    int i;
    for (i = 0; i < num_fields; ++i) {
      FieldDef *field = (FieldDef *)alist_get(class->fields, i);
      num_ins += tape_ins_no_arg(tape, PNIL, field->name) +
                 tape_ins_text(tape, RES, SELF, field->name) +
                 tape_ins(tape, FLD, field->name);
    }
    num_ins += tape_ins_no_arg(tape, RES, class->def.name.token);
  }

  if (class->has_constructor && class->constructor.has_args) {
    num_ins += produce_arguments(analyzer, &class->constructor.args, tape);
  }

  if (class->has_constructor) {
    FunctionDef *func = &class->constructor;
    num_ins += semantic_analyzer_produce(analyzer, func->body, tape);
    num_ins += tape_ins_text(tape, RES, SELF, func->fn_name);
    // TODO: Uncomment when const is implemented.
    // if (func->is_const) {
    //   num_ins += tape_ins_no_arg(tape, CNST, func->const_token);
    // }
    num_ins += tape_ins_no_arg(tape, RET, func->def_token);
  } else {
    num_ins += tape_ins_text(tape, RES, SELF, class->def.name.token);
    num_ins += tape_ins_no_arg(tape, RET, class->def.name.token);
  }
  return num_ins;
}

int produce_class(SemanticAnalyzer *analyzer, ClassDef *class, Tape *tape) {
  int num_ins = 0;
  if (alist_len(class->def.parent_classes) > 1) {
    ERROR("Cannot have more than one super class.");
  }
  num_ins += tape_class(tape, class->def.name.token);
  // Fields
  AL_iter fields = alist_iter(class->fields);
  for (; al_has(&fields); al_inc(&fields)) {
    FieldDef *f = (FieldDef *)al_value(&fields);
    tape_field(tape, f->name->text);
  }
  // Constructor
  if (class->has_constructor || alist_len(class->fields) > 0) {
    num_ins += produce_constructor(analyzer, class, tape);
  }
  int i, num_methods = alist_len(class->methods);
  for (i = 0; i < num_methods; ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(class->methods, i);
    num_ins += produce_function(analyzer, func, tape);
  }
  num_ins += tape_endclass(tape, class->def.name.token);
  return num_ins;
}

Annotation populate_annotation(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree) {
  Annotation annot = {.prefix = NULL,
                      .class_name = NULL,
                      .is_called = false,
                      .has_args = false};
  if (!IS_SYNTAX(stree, rule_annotation)) {
    ERROR("Must be annotation.");
  }
  if (CHILD_IS_SYNTAX(stree, 1, rule_identifier)) {
    annot.class_name = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (IS_SYNTAX(stree->second, rule_annotation_not_called)) {
    annot.class_name = stree->second->second->second->token;
    annot.prefix = stree->second->first->token;
  } else if (IS_SYNTAX(stree->second, rule_annotation_no_arguments)) {
    const SyntaxTree *class = stree->second->first;
    annot.is_called = true;
    annot.has_args = false;
    if (IS_SYNTAX(class, rule_identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, rule_annotation_not_called)) {
      annot.class_name = class->second->second->token;
      annot.prefix = class->first->token;
    }
  } else if (IS_SYNTAX(stree->second, rule_annotation_with_arguments)) {
    const SyntaxTree *class = stree->second->first;
    const SyntaxTree *args_list = stree->second->second->second->first;
    annot.is_called = true;
    annot.has_args = true;
    if (IS_SYNTAX(class, rule_identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, rule_annotation_not_called)) {
      annot.class_name = class->second->second->token;
      annot.prefix = class->first->token;
    }
    annot.args_tuple = semantic_analyzer_populate(analyzer, args_list);
  }
  return annot;
}

void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  func->def_token = CHILD_SYNTAX_AT(fn_identifier, 0)->token;
  func->fn_name = CHILD_SYNTAX_AT(fn_identifier, 1)->token;
}

FunctionDef populate_function_variant(
    SemanticAnalyzer *analyzer, const SyntaxTree *stree, RuleFn def,
    RuleFn signature_with_qualifier, RuleFn signature_no_qualifier,
    RuleFn fn_identifier, RuleFn function_arguments_no_args,
    RuleFn function_arguments_present, FuncDefPopulator def_populator,
    FuncArgumentsPopulator args_populator) {
  FunctionDef func = {.def_token = NULL,
                      .fn_name = NULL,
                      .special_method = SpecialMethod__NONE,
                      .has_args = false,
                      .is_const = false,
                      .const_token = NULL,
                      .is_async = false,
                      .async_token = NULL,
                      .body = NULL};
  ASSERT(IS_SYNTAX(stree, def));

  const SyntaxTree *func_sig;
  if (CHILD_IS_SYNTAX(stree, 0, signature_with_qualifier)) {
    func_sig = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 0), 0);
    populate_function_qualifiers(CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 0), 1),
                                 &func.is_const, &func.const_token,
                                 &func.is_async, &func.async_token);
  } else {
    ASSERT(CHILD_IS_SYNTAX(stree, 0, signature_no_qualifier));
    func_sig = CHILD_SYNTAX_AT(stree, 0);
  }

  ASSERT(CHILD_IS_SYNTAX(func_sig, 0, fn_identifier));
  def_populator(CHILD_SYNTAX_AT(func_sig, 0), &func);

  func.has_args =
      !IS_SYNTAX(CHILD_SYNTAX_AT(func_sig, 1), function_arguments_no_args);
  if (func.has_args) {
    ASSERT(CHILD_IS_SYNTAX(func_sig, 1, function_arguments_present));
    const SyntaxTree *func_args =
        CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(func_sig, 1), 1);
    func.args = args_populator(
        func_args, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(func_sig, 1), 0)->token);
  }
  func.body = semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 1));
  return func;
}

FunctionDef populate_function(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree) {
  return populate_function_variant(
      analyzer, stree, rule_function_definition,
      rule_function_signature_with_qualifier,
      rule_function_signature_no_qualifier, rule_def_identifier,
      rule_function_arguments_no_args, rule_function_arguments_present,
      set_function_def, set_function_args);
}

void populate_fi_statement(SemanticAnalyzer *analyzer, const SyntaxTree *stree,
                           ModuleDef *module) {
  if (IS_SYNTAX(stree, rule_module_statement)) {
    if (module->name.is_named) {
      ERROR("Module named twice: first '%s' then '%s'.",
            module->name.module_name->text,
            CHILD_SYNTAX_AT(stree, 1)->token->text);
    }
    module->name.is_named = true;
    module->name.module_token = CHILD_SYNTAX_AT(stree, 0)->token;
    module->name.module_name = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (IS_SYNTAX(stree, rule_import_statement)) {
    if (!CHILD_IS_SYNTAX(stree, 1, rule_identifier)) {
      ERROR("import AS not yet supported.");
    }
    Import import = {.import_token = CHILD_SYNTAX_AT(stree, 0)->token,
                     .module_name = CHILD_SYNTAX_AT(stree, 1)->token};
    alist_append(module->imports, &import);
  } else if (IS_SYNTAX(stree, rule_class_definition_no_annotation)) {
    ClassDef class = populate_class(analyzer, stree);
    alist_append(module->classes, &class);
  } else if (IS_SYNTAX(stree, rule_class_definition_with_annotation)) {
    ClassDef class = populate_class(analyzer, CHILD_SYNTAX_AT(stree, 1));
    class.has_annot = true;
    class.annot = populate_annotation(analyzer, CHILD_SYNTAX_AT(stree, 0));
    alist_append(module->classes, &class);
  } else if (IS_SYNTAX(stree, rule_function_definition)) {
    FunctionDef func = populate_function(analyzer, stree);
    alist_append(module->functions, &func);
  } else {
    ExpressionTree *etree = semantic_analyzer_populate(analyzer, stree);
    alist_append(module->statements, &etree);
  }
}

ModuleDef populate_module_def(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree) {
  ModuleDef module_def;
  ModuleName module_name = {
      .is_named = false, .module_token = NULL, .module_name = NULL};
  module_def.name = module_name;
  module_def.imports = alist_create(Import, 4);
  module_def.classes = alist_create(ClassDef, 4);
  module_def.functions = alist_create(FunctionDef, 4);
  module_def.statements = alist_create(ExpressionTree *, DEFAULT_ARRAY_SZ);

  populate_fi_statement(analyzer, CHILD_SYNTAX_AT(stree, 0), &module_def);
  const SyntaxTree *cur = CHILD_SYNTAX_AT(stree, 1);
  while (true) {
    if (!IS_SYNTAX(cur, rule_file_level_statement_list1)) {
      populate_fi_statement(analyzer, cur, &module_def);
      break;
    }
    populate_fi_statement(analyzer, CHILD_SYNTAX_AT(cur, 0), &module_def);
    cur = CHILD_SYNTAX_AT(cur, 1);
  }
  return module_def;
}

POPULATE_IMPL(file_level_statement_list, const SyntaxTree *stree,
              SemanticAnalyzer *analyzer) {
  file_level_statement_list->def = populate_module_def(analyzer, stree);
}

void _delete_statement(SemanticAnalyzer *analyzer, void *ptr) {
  ExpressionTree *statement = *((ExpressionTree **)ptr);
  semantic_analyzer_delete(analyzer, statement);
}

void delete_module_def(SemanticAnalyzer *analyzer, ModuleDef *module) {
  alist_delete(module->imports);

  for (AL_iter classes = alist_iter(module->classes); al_has(&classes);
       al_inc(&classes)) {
    delete_class(analyzer, (ClassDef *)al_value(&classes));
  }
  alist_delete(module->classes);

  for (AL_iter functions = alist_iter(module->functions); al_has(&functions);
       al_inc(&functions)) {
    delete_function(analyzer, (ClassDef *)al_value(&functions));
  }
  alist_delete(module->functions);

  for (AL_iter statements = alist_iter(module->statements); al_has(&statements);
       al_inc(&statements)) {
    _delete_statement(analyzer, (ClassDef *)al_value(&statements));
  }
  alist_delete(module->statements);
}

DELETE_IMPL(file_level_statement_list, SemanticAnalyzer *analyzer) {
  delete_module_def(analyzer, &file_level_statement_list->def);
}

int produce_annotation(SemanticAnalyzer *analyzer, const ClassDef *class,
                       Tape *tape) {
  int num_ins = 0;
  const Annotation *annot = &class->annot;

  if (NULL != annot->prefix) {
    num_ins += tape_ins(tape, RES, annot->prefix) +
               tape_ins(tape, GTSH, annot->class_name);
  } else {
    num_ins += tape_ins(tape, PUSH, annot->class_name);
  }
  if (annot->has_args) {
    num_ins += semantic_analyzer_produce(analyzer, annot->args_tuple, tape) +
               tape_ins_no_arg(tape, CALL, annot->class_name);

  } else {
    num_ins += tape_ins_no_arg(tape, CLLN, annot->class_name);
  }
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name) +
             tape_ins(tape, RES, class->def.name.token) +
             tape_ins_text(tape, CALL, intern("annotate"), annot->class_name);
  return num_ins;
}

int produce_module_def(SemanticAnalyzer *analyzer, ModuleDef *module,
                       Tape *tape) {
  int num_ins = 0;
  if (module->name.is_named) {
    num_ins += tape_module(tape, module->name.module_name);
  }
  int i;
  // Imports
  for (i = 0; i < alist_len(module->imports); ++i) {
    Import *import = (Import *)alist_get(module->imports, i);
    num_ins += tape_ins(tape, LMDL, import->module_name);
  }
  // Annotations
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    if (!class->has_annot) {
      continue;
    }
    num_ins += produce_annotation(analyzer, class, tape);
  }
  // Superclasses
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    if (alist_len(class->def.parent_classes) == 0) {
      continue;
    }
    ClassName *super = (ClassName *)alist_get(class->def.parent_classes, 0);
    num_ins += tape_ins(tape, PUSH, class->def.name.token) +
               tape_ins(tape, RES, super->token) +
               tape_ins_text(tape, CALL, intern("$__set_super"), super->token);
  }
  // Statements
  for (i = 0; i < alist_len(module->statements); ++i) {
    ExpressionTree *statement =
        *((ExpressionTree **)alist_get(module->statements, i));
    num_ins += semantic_analyzer_produce(analyzer, statement, tape);
  }
  num_ins += tape_ins_int(tape, EXIT, 0,
                          module->name.module_token == NULL
                              ? token_create(TOKEN_INTEGER, -1, -1, "0", 1)
                              : module->name.module_token);
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    num_ins += produce_class(analyzer, class, tape);
  }
  for (i = 0; i < alist_len(module->functions); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(module->functions, i);
    num_ins += produce_function(analyzer, func, tape);
  }
  return num_ins;
}

PRODUCE_IMPL(file_level_statement_list, SemanticAnalyzer *analyzer,
             Tape *target) {
  return produce_module_def(analyzer, &file_level_statement_list->def, target);
}
