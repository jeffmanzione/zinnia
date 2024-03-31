#include "lang/semantic_analyzer/definitions.h"

#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/lang_lexer.h"
#include "lang/parser/lang_parser.h"
#include "vm/intern.h"

void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func);
FunctionDef populate_function_variant(
    SemanticAnalyzer *analyzer, const SyntaxTree *stree, RuleFn def,
    RuleFn signature_with_qualifier, RuleFn signature_no_qualifier,
    RuleFn fn_identifier, RuleFn function_parameters_no_parameters,
    RuleFn function_parameters_present, FuncDefPopulator def_populator,
    FuncArgumentsPopulator args_populator);

FunctionDef populate_function(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree);

void populate_class_def(ClassSignature *def, const SyntaxTree *stree) {
  def->parent_classes = alist_create(ClassName, 2);
  // No parent classes.
  if (IS_SYNTAX(stree, rule_identifier)) {
    def->name.token = stree->token;
  } else if (IS_SYNTAX(stree, rule_class_name_and_inheritance)) {
    const SyntaxTree *class_inheritance = stree;
    ASSERT(CHILD_IS_SYNTAX(class_inheritance, 0, rule_identifier));
    def->name.token = CHILD_SYNTAX_AT(class_inheritance, 0)->token;
    ASSERT(CHILD_IS_SYNTAX(class_inheritance, 1, rule_parent_classes));
    const SyntaxTree *parent_classes = CHILD_SYNTAX_AT(class_inheritance, 1);
    if (CHILD_IS_SYNTAX(parent_classes, 1, rule_identifier)) {
      ClassName name = {.token = CHILD_SYNTAX_AT(parent_classes, 1)->token,
                        .module = NULL};
      alist_append(def->parent_classes, &name);
    } else if (CHILD_IS_SYNTAX(parent_classes, 1, rule_parent_class)) {
      const SyntaxTree *parent_class = CHILD_SYNTAX_AT(parent_classes, 1);
      ClassName name = {.token = CHILD_SYNTAX_AT(parent_class, 2)->token,
                        .module = CHILD_SYNTAX_AT(parent_class, 0)->token};
      alist_append(def->parent_classes, &name);
    } else {
      FATALF("Multiple inheritance no longer supported.");
    }
  } else {
    FATALF("Unknown class name composition.");
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
  if (IS_SYNTAX(argument, rule_const_new_parameter)) {
    arg.is_const = true;
    arg.const_token = CHILD_SYNTAX_AT(argument, 0)->token;
    argument = CHILD_SYNTAX_AT(argument, 1);
  }
  if (IS_SYNTAX(argument, rule_new_parameter_elt_with_default)) {
    arg.has_default = true;
    arg.default_value = semantic_analyzer_populate(
        analyzer, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(argument, 1), 1));
    argument = CHILD_SYNTAX_AT(argument, 0);
  }
  if (IS_SYNTAX(argument, rule_new_field_parameter)) {
    arg.arg_name = CHILD_SYNTAX_AT(argument, 1)->token;
    arg.field_token = CHILD_SYNTAX_AT(argument, 0)->token;
    arg.is_field = true;
  } else {
    ASSERT(IS_SYNTAX(argument, rule_identifier));
    arg.arg_name = argument->token;
  }
  return arg;
}

Arguments set_constructor_args(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree, const Token *token) {
  Arguments args = {
      .token = token,
      .count_required = 0,
      .count_optional = 0,
      .is_named = false,
  };
  args.args = alist_create(Argument, 4);
  if (IS_SYNTAX(stree, rule_new_named_parameters)) {
    args.is_named = true;
    stree = CHILD_SYNTAX_AT(stree, 1);
  }
  if (!IS_SYNTAX(stree, rule_new_parameter_list)) {
    Argument arg = populate_constructor_argument(analyzer, stree);
    add_arg(&args, &arg);
    return args;
  }
  Argument arg =
      populate_constructor_argument(analyzer, CHILD_SYNTAX_AT(stree, 0));
  add_arg(&args, &arg);
  const SyntaxTree *cur = CHILD_SYNTAX_AT(stree, 1);
  while (true) {
    Argument arg =
        populate_constructor_argument(analyzer, CHILD_SYNTAX_AT(cur, 1));
    add_arg(&args, &arg);
    if (CHILD_COUNT(cur) == 2) {
      break;
    }
    cur = CHILD_SYNTAX_AT(cur, 2);
  }
  return args;
}

void set_method_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  ASSERT(IS_SYNTAX(fn_identifier, rule_method_identifier));
  func->def_token = CHILD_SYNTAX_AT(fn_identifier, 0)->token;
  const SyntaxTree *fn_name = CHILD_SYNTAX_AT(fn_identifier, 1);
  if (IS_SYNTAX(fn_name, rule_identifier)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NONE;
  } else if (IS_TOKEN(fn_name, SYMBOL_EQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__EQUIV;
  } else if (IS_TOKEN(fn_name, SYMBOL_NEQUIV)) {
    func->fn_name = fn_name->token;
    func->special_method = SpecialMethod__NEQUIV;
  } else if (CHILD_COUNT(fn_name) == 2 &&
             CHILD_IS_TOKEN(fn_name, 0, SYMBOL_LBRACKET) &&
             CHILD_IS_TOKEN(fn_name, 1, SYMBOL_RBRACKET)) {
    func->fn_name = CHILD_SYNTAX_AT(fn_name, 0)->token;
    func->special_method = SpecialMethod__ARRAY_INDEX;
  } else if (CHILD_COUNT(fn_name) == 3 &&
             CHILD_IS_TOKEN(fn_name, 0, SYMBOL_LBRACKET) &&
             CHILD_IS_TOKEN(fn_name, 1, SYMBOL_RBRACKET) &&
             CHILD_IS_TOKEN(fn_name, 2, SYMBOL_EQUALS)) {
    func->fn_name = CHILD_SYNTAX_AT(fn_name, 0)->token;
    func->special_method = SpecialMethod__ARRAY_SET;
  } else {
    FATALF("Unknown method name type.");
  }
}

FunctionDef populate_method(SemanticAnalyzer *analyzer,
                            const SyntaxTree *stree) {
  return populate_function_variant(
      analyzer, stree, rule_method_definition,
      rule_method_signature_with_qualifier, rule_method_signature_no_qualifier,
      rule_method_identifier, rule_function_parameters_no_parameters,
      rule_function_parameters_present, set_method_def, set_function_args);
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
      rule_new_parameters_no_params, rule_new_parameters_present, set_new_def,
      set_constructor_args);
}

FieldDef populate_field_statement(const Token *field_token,
                                  const SyntaxTree *stree) {
  FieldDef field = {.name = stree->token, .field_token = field_token};
  return field;
}

void populate_field_statements(const SyntaxTree *stree, ClassDef *class) {
  ASSERT(CHILD_IS_TOKEN(stree, 0, KEYWORD_FIELD));
  const Token *field_token = CHILD_SYNTAX_AT(stree, 0)->token;

  if (!CHILD_IS_SYNTAX(stree, 1, rule_identifier_list)) {
    ASSERT(CHILD_IS_SYNTAX(stree, 1, rule_identifier));
    FieldDef field =
        populate_field_statement(field_token, CHILD_SYNTAX_AT(stree, 1));
    alist_append(class->fields, &field);
    return;
  }
  FieldDef field = populate_field_statement(
      field_token, CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0));
  alist_append(class->fields, &field);

  const SyntaxTree *statement = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 1);
  while (true) {
    FieldDef field =
        populate_field_statement(field_token, CHILD_SYNTAX_AT(statement, 1));
    alist_append(class->fields, &field);
    if (CHILD_COUNT(statement) == 2) {
      break;
    }
    statement = CHILD_SYNTAX_AT(statement, 2);
  }
}

void populate_static_field_statement(SemanticAnalyzer *analyzer,
                                     const SyntaxTree *stree, ClassDef *class) {
  StaticDef *def = alist_add(class->statics);
  def->static_token = CHILD_SYNTAX_AT(stree, 0)->token;
  def->name = CHILD_SYNTAX_AT(stree, 1)->token;
  def->value = semantic_analyzer_populate(analyzer, CHILD_SYNTAX_AT(stree, 3));
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
        const Argument *arg = (Argument *)al_value(&args);
        if (!arg->is_field) {
          continue;
        }
        FieldDef field = {.name = arg->arg_name,
                          .field_token = arg->field_token};
        alist_append(class->fields, &field);
      }
    }
  } else if (IS_SYNTAX(stree, rule_static_field_statement)) {
    populate_static_field_statement(analyzer, stree, class);
  } else {
    FATALF("Unknown class_statement.");
  }
}

void populate_class_statements(SemanticAnalyzer *analyzer, ClassDef *class,
                               const SyntaxTree *stree) {
  ASSERT(CHILD_IS_TOKEN(stree, 0, SYMBOL_LBRACE));
  if (CHILD_IS_TOKEN(stree, 1, SYMBOL_RBRACE)) {
    // Empty body.
    return;
  }
  if (!CHILD_IS_SYNTAX(stree, 1, rule_class_statement_list)) {
    populate_class_statement(analyzer, class, CHILD_SYNTAX_AT(stree, 1));
    return;
  }
  populate_class_statement(analyzer, class,
                           CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0));
  const SyntaxTree *statement = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 1);
  while (true) {
    if (!IS_SYNTAX(statement, rule_class_statement_list1)) {
      populate_class_statement(analyzer, class, statement);
      break;
    }
    populate_class_statement(analyzer, class, CHILD_SYNTAX_AT(statement, 0));
    statement = CHILD_SYNTAX_AT(statement, 1);
  }
}

ClassDef populate_class(SemanticAnalyzer *analyzer, const SyntaxTree *stree) {
  ClassDef class;
  class.has_constructor = false;
  class.annots = alist_create(Annotation, 3);
  class.fields = alist_create(FieldDef, 4);
  class.statics = alist_create(StaticDef, 4);
  class.methods = alist_create(FunctionDef, 6);
  populate_class_def(&class.def, CHILD_SYNTAX_AT(stree, 1));

  const SyntaxTree *body = CHILD_SYNTAX_AT(stree, 2);
  if (IS_SYNTAX(body, rule_class_compound_statement)) {
    populate_class_statements(analyzer, &class, body);
  } else {
    populate_class_statement(analyzer, &class, body);
  }
  return class;
}

void delete_class(SemanticAnalyzer *analyzer, ClassDef *class) {
  if (class->has_constructor) {
    delete_function(analyzer, &class->constructor);
  }

  alist_delete(class->def.parent_classes);
  alist_delete(class->fields);

  int i;
  for (i = 0; i < alist_len(class->statics); ++i) {
    StaticDef *sta = (StaticDef *)alist_get(class->statics, i);
    semantic_analyzer_delete(analyzer, sta->value);
  }
  alist_delete(class->statics);

  for (i = 0; i < alist_len(class->methods); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(class->methods, i);
    delete_function(analyzer, func);
  }
  alist_delete(class->methods);

  AL_iter annots = alist_iter(class->annots);
  for (; al_has(&annots); al_inc(&annots)) {
    delete_annotation(analyzer, (Annotation *)al_value(&annots));
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
      num_ins += tape_ins_no_arg(tape, PNIL, field->name);
      num_ins += tape_ins_text(tape, RES, SELF, field->name);
      num_ins += tape_ins(tape, FLD, field->name);
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

int produce_static(SemanticAnalyzer *analyzer, ClassDef *class, StaticDef *sta,
                   Tape *tape) {
  int num_ins = 0;
  num_ins += semantic_analyzer_produce(analyzer, sta->value, tape);
  num_ins += tape_ins_no_arg(tape, PUSH, sta->static_token);
  num_ins += tape_ins(tape, RES, class->def.name.token);
  num_ins += tape_ins(tape, FLD, sta->name);
  return num_ins;
}

int produce_statics(SemanticAnalyzer *analyzer, ClassDef *class, Tape *tape) {
  int num_ins = 0;
  int num_statics = alist_len(class->statics);
  for (int i = 0; i < num_statics; ++i) {
    StaticDef *sta = (StaticDef *)alist_get(class->statics, i);
    num_ins += produce_static(analyzer, class, sta, tape);
  }
  return num_ins;
}

int produce_class(SemanticAnalyzer *analyzer, ClassDef *class, Tape *tape) {
  int num_ins = 0;
  if (alist_len(class->def.parent_classes) > 1) {
    FATALF("Cannot have more than one super class.");
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
    FATALF("Must be annotation.");
  }
  if (CHILD_IS_SYNTAX(stree, 1, rule_identifier)) {
    annot.class_name = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (CHILD_IS_SYNTAX(stree, 1, rule_annotation_not_called)) {
    annot.class_name = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 2)->token;
    annot.prefix = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0)->token;
  } else if (CHILD_IS_SYNTAX(stree, 1, rule_annotation_no_arguments)) {
    const SyntaxTree *class = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0);
    annot.is_called = true;
    annot.has_args = false;
    if (IS_SYNTAX(class, rule_identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, rule_annotation_not_called)) {
      annot.class_name = CHILD_SYNTAX_AT(class, 2)->token;
      annot.prefix = CHILD_SYNTAX_AT(class, 0)->token;
    }
  } else if (CHILD_IS_SYNTAX(stree, 1, rule_annotation_with_arguments)) {
    const SyntaxTree *class = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 0);
    const SyntaxTree *args_list = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, 1), 2);
    annot.is_called = true;
    annot.has_args = true;
    if (IS_SYNTAX(class, rule_identifier)) {
      annot.class_name = class->token;
    } else if (IS_SYNTAX(class, rule_annotation_not_called)) {
      annot.class_name = CHILD_SYNTAX_AT(class, 2)->token;
      annot.prefix = CHILD_SYNTAX_AT(class, 0)->token;
    }
    annot.args_tuple = semantic_analyzer_populate(analyzer, args_list);
  }
  return annot;
}

void populate_annotation_list(SemanticAnalyzer *analyzer,
                              const SyntaxTree *stree, AList *annots) {
  if (IS_SYNTAX(stree, rule_annotation)) {
    *((Annotation *)alist_add(annots)) = populate_annotation(analyzer, stree);
    return;
  }
  *((Annotation *)alist_add(annots)) =
      populate_annotation(analyzer, CHILD_SYNTAX_AT(stree, 0));
  if (!CHILD_IS_SYNTAX(stree, 1, rule_annotation) &&
      !CHILD_IS_SYNTAX(stree, 1, rule_annotation_list1)) {
    FATALF("Unknown annotation.");
  }
  populate_annotation_list(analyzer, CHILD_SYNTAX_AT(stree, 1), annots);
}

void set_function_def(const SyntaxTree *fn_identifier, FunctionDef *func) {
  func->def_token = CHILD_SYNTAX_AT(fn_identifier, 0)->token;
  func->fn_name = CHILD_SYNTAX_AT(fn_identifier, 1)->token;
}

SyntaxTree *populate_signature(SemanticAnalyzer *analyzer,
                               const SyntaxTree *stree, FunctionDef *func,
                               RuleFn signature_with_qualifier,
                               RuleFn signature_no_qualifier) {
  SyntaxTree *func_sig;
  int start_index = 0;

  if (CHILD_IS_SYNTAX(stree, 0, rule_method_signature) ||
      CHILD_IS_SYNTAX(stree, 0, rule_function_signature)) {
    stree = CHILD_SYNTAX_AT(stree, 0);
    start_index = 1;
    populate_annotation_list(analyzer, CHILD_SYNTAX_AT(stree, 0),
                             &func->annots);
  }
  if (CHILD_IS_SYNTAX(stree, start_index, signature_with_qualifier)) {
    func_sig = CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, start_index), 0);
    populate_function_qualifiers(
        CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(stree, start_index), 1),
        &func->is_const, &func->const_token, &func->is_async,
        &func->async_token);
  } else {
    ASSERT(CHILD_IS_SYNTAX(stree, start_index, signature_no_qualifier));
    func_sig = CHILD_SYNTAX_AT(stree, start_index);
  }
  return func_sig;
}

FunctionDef populate_function_variant(
    SemanticAnalyzer *analyzer, const SyntaxTree *stree, RuleFn def,
    RuleFn signature_with_qualifier, RuleFn signature_no_qualifier,
    RuleFn fn_identifier, RuleFn function_parameters_no_parameters,
    RuleFn function_parameters_present, FuncDefPopulator def_populator,
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
  alist_init(&func.annots, Annotation, 3);
  ASSERT(IS_SYNTAX(stree, def));

  const SyntaxTree *func_sig = populate_signature(
      analyzer, stree, &func, signature_with_qualifier, signature_no_qualifier);

  ASSERT(CHILD_IS_SYNTAX(func_sig, 0, fn_identifier));
  def_populator(CHILD_SYNTAX_AT(func_sig, 0), &func);

  func.has_args = !IS_SYNTAX(CHILD_SYNTAX_AT(func_sig, 1),
                             function_parameters_no_parameters);
  if (func.has_args) {
    ASSERT(CHILD_IS_SYNTAX(func_sig, 1, function_parameters_present));
    const SyntaxTree *func_args =
        CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(func_sig, 1), 1);
    func.args =
        args_populator(analyzer, func_args,
                       CHILD_SYNTAX_AT(CHILD_SYNTAX_AT(func_sig, 1), 0)->token);
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
      rule_function_parameters_no_parameters, rule_function_parameters_present,
      set_function_def, set_function_args);
}

Import populate_import(SemanticAnalyzer *analyzer, const SyntaxTree *stree) {
  Import import = {.import_token = CHILD_SYNTAX_AT(stree, 0)->token,
                   .src_name = NULL,
                   .as_token = NULL,
                   .use_name = NULL,
                   .is_string_import = false};
  if (CHILD_IS_SYNTAX(stree, 1, rule_identifier)) {
    import.src_name = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (CHILD_IS_SYNTAX(stree, 1, rule_string_literal)) {
    import.src_name = CHILD_SYNTAX_AT(stree, 1)->token;
    import.is_string_import = true;
  } else {
    FATALF("Unknown rule.");
  }
  if (CHILD_COUNT(stree) > 2 && CHILD_IS_TOKEN(stree, 2, KEYWORD_AS)) {
    import.as_token = CHILD_SYNTAX_AT(stree, 2)->token;
    import.use_name = CHILD_SYNTAX_AT(stree, 3)->token;
  }
  return import;
}

void populate_fi_statement(SemanticAnalyzer *analyzer, const SyntaxTree *stree,
                           ModuleDef *module) {
  if (IS_SYNTAX(stree, rule_module_statement)) {
    if (module->name.is_named) {
      FATALF("Module named twice: first '%s' then '%s'.",
             module->name.module_name->text,
             CHILD_SYNTAX_AT(stree, 1)->token->text);
    }
    module->name.is_named = true;
    module->name.module_token = CHILD_SYNTAX_AT(stree, 0)->token;
    module->name.module_name = CHILD_SYNTAX_AT(stree, 1)->token;
  } else if (IS_SYNTAX(stree, rule_import_statement)) {
    Import import = populate_import(analyzer, stree);
    alist_append(module->imports, &import);
  } else if (IS_SYNTAX(stree, rule_class_definition_no_annotation)) {
    ClassDef class = populate_class(analyzer, stree);
    alist_append(module->classes, &class);
  } else if (IS_SYNTAX(stree, rule_class_definition_with_annotation)) {
    ClassDef class = populate_class(analyzer, CHILD_SYNTAX_AT(stree, 1));
    populate_annotation_list(analyzer, CHILD_SYNTAX_AT(stree, 0), class.annots);
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
    delete_function(analyzer, (FunctionDef *)al_value(&functions));
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

int produce_function_annotation(SemanticAnalyzer *analyzer,
                                const FunctionDef *func,
                                const Annotation *annot, Tape *tape) {
  int num_ins = 0;

  if (NULL != annot->prefix) {
    num_ins += tape_ins(tape, RES, annot->prefix);
    num_ins += tape_ins(tape, GTSH, annot->class_name);
  } else {
    num_ins += tape_ins(tape, PUSH, annot->class_name);
  }
  if (annot->has_args) {
    num_ins += semantic_analyzer_produce(analyzer, annot->args_tuple, tape);
    num_ins += tape_ins_no_arg(tape, CALL, annot->class_name);
  } else {
    num_ins += tape_ins_no_arg(tape, CLLN, annot->class_name);
  }
  // For call to annotate.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);
  // For call to add_annotation.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);

  num_ins += tape_ins_text(tape, GET, ANNOTATE_KEY, annot->class_name);
  num_ins += tape_ins_int(tape, IF, 2, annot->class_name);
  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);
  num_ins += tape_ins_int(tape, JMP, 2, annot->class_name);
  num_ins += tape_ins(tape, RES, func->fn_name);
  num_ins += tape_ins_text(tape, CALL, ANNOTATE_KEY, annot->class_name);

  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);
  num_ins += tape_ins(tape, RES, func->fn_name);
  num_ins += tape_ins_no_arg(tape, PUSH, func->fn_name);
  num_ins += tape_ins_int(tape, PEEK, 2, func->fn_name);
  num_ins +=
      tape_ins_text(tape, CALL, intern("add_annotation"), annot->class_name);
  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);

  return num_ins;
}

int produce_method_annotation(SemanticAnalyzer *analyzer, const ClassDef *class,
                              const FunctionDef *func, const Annotation *annot,
                              Tape *tape) {
  int num_ins = 0;

  if (NULL != annot->prefix) {
    num_ins += tape_ins(tape, RES, annot->prefix);
    num_ins += tape_ins(tape, GTSH, annot->class_name);
  } else {
    num_ins += tape_ins(tape, PUSH, annot->class_name);
  }
  if (annot->has_args) {
    num_ins += semantic_analyzer_produce(analyzer, annot->args_tuple, tape);
    num_ins += tape_ins_no_arg(tape, CALL, annot->class_name);
  } else {
    num_ins += tape_ins_no_arg(tape, CLLN, annot->class_name);
  }
  // For call to annotate.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);
  // For call to add_annotation.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);

  num_ins += tape_ins_text(tape, GET, ANNOTATE_KEY, annot->class_name);
  num_ins += tape_ins_int(tape, IF, 2, annot->class_name);
  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);
  num_ins += tape_ins_int(tape, JMP, 2, annot->class_name);
  num_ins += tape_ins(tape, RES, class->def.name.token);
  num_ins += tape_ins(tape, GET, func->fn_name);
  num_ins += tape_ins_text(tape, CALL, ANNOTATE_KEY, annot->class_name);

  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);
  num_ins += tape_ins(tape, RES, class->def.name.token);
  num_ins += tape_ins(tape, GET, func->fn_name);
  num_ins += tape_ins_no_arg(tape, PUSH, func->fn_name);
  num_ins += tape_ins_int(tape, PEEK, 2, func->fn_name);
  num_ins +=
      tape_ins_text(tape, CALL, intern("add_annotation"), annot->class_name);
  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);

  return num_ins;
}

int produce_class_annotation(SemanticAnalyzer *analyzer, const ClassDef *class,
                             const Annotation *annot, Tape *tape) {
  int num_ins = 0;

  if (NULL != annot->prefix) {
    num_ins += tape_ins(tape, RES, annot->prefix);
    num_ins += tape_ins(tape, GTSH, annot->class_name);
  } else {
    num_ins += tape_ins(tape, PUSH, annot->class_name);
  }
  if (annot->has_args) {
    num_ins += semantic_analyzer_produce(analyzer, annot->args_tuple, tape);
    num_ins += tape_ins_no_arg(tape, CALL, annot->class_name);
  } else {
    num_ins += tape_ins_no_arg(tape, CLLN, annot->class_name);
  }
  // For call to annotate.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);
  // For call to add_annotation.
  num_ins += tape_ins_no_arg(tape, PUSH, annot->class_name);

  num_ins += tape_ins_text(tape, GET, ANNOTATE_KEY, annot->class_name);
  num_ins += tape_ins_int(tape, IF, 2, annot->class_name);
  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);
  num_ins += tape_ins_int(tape, JMP, 2, annot->class_name);
  num_ins += tape_ins(tape, RES, class->def.name.token);
  num_ins += tape_ins_text(tape, CALL, ANNOTATE_KEY, annot->class_name);

  num_ins += tape_ins_no_arg(tape, RES, annot->class_name);
  num_ins += tape_ins(tape, PUSH, class->def.name.token);
  num_ins +=
      tape_ins_text(tape, CALL, intern("add_annotation"), annot->class_name);

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
    num_ins += tape_ins(tape, LMDL, import->src_name);
    num_ins += tape_ins(tape, MSET,
                        NULL == import->use_name ? import->src_name
                                                 : import->use_name);
  }
  // Superclasses
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    if (alist_len(class->def.parent_classes) == 0) {
      continue;
    }
    ClassName *super = (ClassName *)alist_get(class->def.parent_classes, 0);
    num_ins += tape_ins(tape, PUSH, class->def.name.token);

    if (NULL != super->module) {
      num_ins += tape_ins(tape, RES, super->module);
      num_ins += tape_ins(tape, GET, super->token);
    } else {
      num_ins += tape_ins(tape, RES, super->token);
    }

    num_ins += tape_ins_text(tape, CALL, intern("$__set_super"), super->token);
  }
  // Function annotations
  for (i = 0; i < alist_len(module->functions); ++i) {
    FunctionDef *func = (FunctionDef *)alist_get(module->functions, i);
    AL_iter annots = alist_iter(&func->annots);
    for (; al_has(&annots); al_inc(&annots)) {
      num_ins += produce_function_annotation(
          analyzer, func, (Annotation *)al_value(&annots), tape);
    }
  }
  // Class annotations
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    for (int j = 0; j < alist_len(class->methods); ++j) {
      FunctionDef *func = (FunctionDef *)alist_get(class->methods, j);
      AL_iter annots = alist_iter(&func->annots);
      for (; al_has(&annots); al_inc(&annots)) {
        num_ins += produce_method_annotation(
            analyzer, class, func, (Annotation *)al_value(&annots), tape);
      }
    }

    AL_iter annots = alist_iter(class->annots);
    for (; al_has(&annots); al_inc(&annots)) {
      num_ins += produce_class_annotation(
          analyzer, class, (Annotation *)al_value(&annots), tape);
    }
  }
  // Statics
  for (i = 0; i < alist_len(module->classes); ++i) {
    ClassDef *class = (ClassDef *)alist_get(module->classes, i);
    num_ins += produce_statics(analyzer, class, tape);
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
