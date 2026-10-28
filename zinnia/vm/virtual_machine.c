// virtual_machine.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "zinnia/vm/virtual_machine.h"

#include <stdarg.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/array/array.h"
#include "zinnia/entity/class/classes_def.h"
#include "zinnia/entity/module/modules.h"
#include "zinnia/entity/native/async.h"
#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/entity/native/builtin.h"
#include "zinnia/entity/native/error.h"
#include "zinnia/entity/native/native.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/string/string.h"
#include "zinnia/entity/string/string_helper.h"
#include "zinnia/entity/tuple/tuple.h"
#include "zinnia/entity/value.h"
#include "zinnia/heap/heap.h"
#include "zinnia/util/sync/mutex.h"
#include "zinnia/util/sync/thread.h"
#include "zinnia/vm/builtin_modules.h"
#include "zinnia/vm/intern.h"
#include "zinnia/vm/isolate/actor_ref.h"
#include "zinnia/vm/isolate/frame.h"
#include "zinnia/vm/isolate/isolate.h"
#include "zinnia/vm/isolate/isolates.h"
#include "zinnia/vm/isolate/scope.h"

#define DEFAULT_THREADPOOL_SIZE 6

bool isolate_maybe_collect_garbage(Isolate *isolate);
bool _call_function_base(Frame *frame, Scope *scope, const Function *func,
                         Object *self, Scope *parent_scope);
void _mark_frame_complete(Isolate *isolate, Frame *frame, bool should_push);
void _execute_RES(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_PUSH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_RNIL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_PNIL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_PEEK(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_DUP(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_FLD(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_LET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_SET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_GET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_GTSH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
bool _execute_CALL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_RET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
Scope *_execute_NBLK(VM *vm, Frame *frame, Scope *scope,
                     const Instruction *ins);
Scope *_execute_BBLK(VM *vm, Frame *frame, Scope *scope,
                     const Instruction *ins);
void _execute_EXIT(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_JMP(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_IF(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_NOT(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_ANEW(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
bool _execute_AIDX(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
bool _execute_ASET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_TUPL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
void _execute_IS(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
bool _execute_LMDL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);
bool _execute_CTCH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins);

#define MATH_OP(op, symbol)                                             \
  Primitive _execute_primitive_##op(const Primitive *p1,                \
                                    const Primitive *p2) {              \
    if (PRIMITIVE_BOOL == ptype(p1) || PRIMITIVE_BOOL == ptype(p2)) {   \
      FATALF("Op not valid for bool types.");                           \
    }                                                                   \
    if (PRIMITIVE_FLOAT == ptype(p1) || PRIMITIVE_FLOAT == ptype(p2)) { \
      return primitive_float(float_of(p1) symbol float_of(p2));         \
    }                                                                   \
    if (PRIMITIVE_INT == ptype(p1) || PRIMITIVE_INT == ptype(p2)) {     \
      return primitive_int(int_of(p1) symbol int_of(p2));               \
    }                                                                   \
    return primitive_int(char_of(p1) symbol char_of(p2));               \
  }

#define MATH_OP_INT(op, symbol)                                         \
  Primitive _execute_primitive_##op(const Primitive *p1,                \
                                    const Primitive *p2) {              \
    if (PRIMITIVE_BOOL == ptype(p1) || PRIMITIVE_BOOL == ptype(p2)) {   \
      FATALF("Op not valid for bool types.");                           \
    }                                                                   \
    if (PRIMITIVE_FLOAT == ptype(p1) || PRIMITIVE_FLOAT == ptype(p2)) { \
      FATALF("Op not valid for float types.");                          \
    }                                                                   \
    if (PRIMITIVE_INT == ptype(p1) || PRIMITIVE_INT == ptype(p2)) {     \
      return primitive_int(int_of(p1) symbol int_of(p2));               \
    }                                                                   \
    return primitive_int(char_of(p1) symbol char_of(p2));               \
  }

#define PRIMITIVE_OP(op, symbol, math_fn)                                      \
  math_fn;                                                                     \
  void _execute_##op(VM *vm, Frame *frame, Scope *scope,                       \
                     const Instruction *ins) {                                 \
    const Value *acc, *lookup;                                                 \
    Value first, second, tmp;                                                  \
    switch (ins->type) {                                                       \
      case INSTRUCTION_NO_ARG:                                                 \
        second = frame_popstack(frame);                                        \
        if (PRIMITIVE != second.type) {                                        \
          raise_error(frame, scope, "RHS for op '%s' must be primitive.",      \
                      #op);                                                    \
          return;                                                              \
        }                                                                      \
        first = frame_popstack(frame);                                         \
        if (PRIMITIVE != first.type) {                                         \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.",      \
                      #op);                                                    \
          return;                                                              \
        }                                                                      \
        *frame_mutable_acc(frame) =                                            \
            value_primitive(_execute_primitive_##op(&first.pri, &second.pri)); \
        break;                                                                 \
      case INSTRUCTION_ID:                                                     \
        acc = frame_get_acc(frame);                                            \
        if (NULL != acc && PRIMITIVE != acc->type) {                           \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.",      \
                      #op);                                                    \
          return;                                                              \
        }                                                                      \
        lookup = scope_lookup(scope, ins->id, &tmp);                           \
        if (NULL != lookup && PRIMITIVE != lookup->type) {                     \
          raise_error(frame, scope, "RHS for op '%s' must be primitive.",      \
                      #op);                                                    \
          return;                                                              \
        }                                                                      \
        *frame_mutable_acc(frame) =                                            \
            value_primitive(_execute_primitive_##op(&acc->pri, &lookup->pri)); \
        break;                                                                 \
      case INSTRUCTION_PRIMITIVE:                                              \
        acc = frame_get_acc(frame);                                            \
        if (NULL != acc && PRIMITIVE != acc->type) {                           \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.",      \
                      #op);                                                    \
          return;                                                              \
        }                                                                      \
        *frame_mutable_acc(frame) =                                            \
            value_primitive(_execute_primitive_##op(&acc->pri, &ins->val));    \
        break;                                                                 \
      default:                                                                 \
        FATALF("Invalid arg type=%d for " #op ".", ins->type);                 \
    }                                                                          \
  }

#define PRIMITIVE_BOOL_OP(op, symbol, math_fn)                            \
  math_fn;                                                                \
  void _execute_##op(VM *vm, Frame *frame, Scope *scope,                  \
                     const Instruction *ins) {                            \
    const Value *acc, *lookup;                                            \
    Value first, second, tmp;                                             \
    Primitive result;                                                     \
    switch (ins->type) {                                                  \
      case INSTRUCTION_NO_ARG:                                            \
        second = frame_popstack(frame);                                   \
        if (PRIMITIVE != second.type) {                                   \
          raise_error(frame, scope, "RHS for op '%s' must be primitive.", \
                      #op);                                               \
          return;                                                         \
        }                                                                 \
        first = frame_popstack(frame);                                    \
        if (PRIMITIVE != first.type) {                                    \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.", \
                      #op);                                               \
          return;                                                         \
        }                                                                 \
        result = _execute_primitive_##op(&first.pri, &second.pri);        \
        *frame_mutable_acc(frame) =                                       \
            int_of(&result) == 0 ? FALSE_VALUE : TRUE_VALUE;              \
        break;                                                            \
      case INSTRUCTION_ID:                                                \
        acc = frame_get_acc(frame);                                       \
        if (NULL != acc && PRIMITIVE != acc->type) {                      \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.", \
                      #op);                                               \
          return;                                                         \
        }                                                                 \
        lookup = scope_lookup(scope, ins->id, &tmp);                      \
        if ((NULL == lookup) ||                                           \
            (NULL != lookup && PRIMITIVE != lookup->type)) {              \
          raise_error(frame, scope, "RHS for op '%s' must be primitive.", \
                      #op);                                               \
          return;                                                         \
        }                                                                 \
        result = _execute_primitive_##op(&acc->pri, &lookup->pri);        \
        *frame_mutable_acc(frame) =                                       \
            int_of(&result) == 0 ? FALSE_VALUE : TRUE_VALUE;              \
        break;                                                            \
      case INSTRUCTION_PRIMITIVE:                                         \
        acc = frame_get_acc(frame);                                       \
        if (NULL != acc && PRIMITIVE != acc->type) {                      \
          raise_error(frame, scope, "LHS for op '%s' must be primitive.", \
                      #op);                                               \
          return;                                                         \
        }                                                                 \
        result = _execute_primitive_##op(&acc->pri, &ins->val);           \
        *frame_mutable_acc(frame) =                                       \
            int_of(&result) == 0 ? FALSE_VALUE : TRUE_VALUE;              \
        break;                                                            \
      default:                                                            \
        FATALF("Invalid arg type=%d for " #op ".", ins->type);            \
    }                                                                     \
  }

PRIMITIVE_OP(ADD, +, MATH_OP(ADD, +));
PRIMITIVE_OP(SUB, -, MATH_OP(SUB, -));
PRIMITIVE_OP(MULT, *, MATH_OP(MULT, *));
PRIMITIVE_OP(DIV, /, MATH_OP(DIV, /));
PRIMITIVE_BOOL_OP(LT, <, MATH_OP(LT, <));
PRIMITIVE_BOOL_OP(GT, >, MATH_OP(GT, >));
PRIMITIVE_BOOL_OP(LTE, <=, MATH_OP(LTE, <=));
PRIMITIVE_BOOL_OP(GTE, >=, MATH_OP(GTE, >=));
PRIMITIVE_OP(MOD, %, MATH_OP_INT(MOD, %));
PRIMITIVE_OP(AND, &&, MATH_OP_INT(AND, &&));
PRIMITIVE_OP(OR, ||, MATH_OP_INT(OR, ||));
PRIMITIVE_OP(BAND, &, MATH_OP_INT(BAND, &));
PRIMITIVE_OP(BXOR, ^, MATH_OP_INT(BXOR, ^));
PRIMITIVE_OP(BOR, |, MATH_OP_INT(BOR, |));

Value string_concat(Frame *frame, const Value *s1, const Value *s2) {
  ASSERT(IS_STRING(s1));
  ASSERT(IS_STRING(s2));

  char *s1_str;
  int s1_str_len;
  extract_string(s1, &s1_str, &s1_str_len);

  char *s2_str;
  int s2_str_len;
  extract_string(s2, &s2_str, &s2_str_len);

  Object *new = string_new(frame->parent_isolate->heap, s1_str, s1_str_len);
  String *new_str = (String *)new->_internal_obj;
  String_append_raw(new_str, s2_str, s2_str_len);
  return value_object(new);
}

void _execute_ADD_with_string(VM *vm, Frame *frame, Scope *scope,
                              const Instruction *ins) {
  if (INSTRUCTION_NO_ARG == ins->type) {
    const Value *first_ptr = frame_peekstack_n(frame, 1);
    const Value *second_ptr = frame_peekstack(frame);
    if (IS_STRING(first_ptr) && IS_STRING(second_ptr)) {
      Value second = frame_popstack(frame);
      Value first = frame_popstack(frame);
      *frame_mutable_acc(frame) = string_concat(frame, &first, &second);
      return;
    }
  } else if (INSTRUCTION_ID == ins->type) {
    const Value *acc = frame_get_acc(frame);
    if (IS_STRING(acc)) {
      Value tmp;
      Value *lookup = scope_lookup(scope, ins->id, &tmp);
      if (!IS_STRING(lookup)) {
        raise_error(frame, scope, "RHS for op '+' must be String.");
        return;
      }
      *frame_mutable_acc(frame) = string_concat(frame, acc, lookup);
      return;
    }
  }
  _execute_ADD(vm, frame, scope, ins);
}

void _execute_INC(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  const int inc_amount = ins->op == INC ? 1 : -1;
  Value tmp;
  Value *stored = scope_lookup(scope, ins->id, &tmp);
  if (NULL == stored || NONE == stored->type || OBJECT == stored->type) {
    raise_error(frame, scope, "Can only increment a primitive.");
  }
  switch (ptype(&stored->pri)) {
    case PRIMITIVE_CHAR:
      stored->pri = primitive_char(pchar(&stored->pri) + inc_amount);
      break;
    case PRIMITIVE_INT:
      stored->pri = primitive_int(pint(&stored->pri) + inc_amount);
      break;
    case PRIMITIVE_FLOAT:
      stored->pri = primitive_float(pfloat(&stored->pri) + inc_amount);
      break;
    default:
      FATALF("Unknown primitive type.");
  }
  *frame_mutable_acc(frame) = *stored;
}

void _execute_BOR_with_obj(VM *vm, Frame *frame, Scope *scope,
                           const Instruction *ins) {
  Value first, second;
  const Value *tmp;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      tmp = frame_peekstack_n(frame, 1);
      if (PRIMITIVE == tmp->type) {
        _execute_BOR(vm, frame, scope, ins);
        break;
      }
      second = frame_popstack(frame);
      first = frame_popstack(frame);
      *frame_mutable_acc(frame) = (NONE == first.type) ? second : first;
      break;
    case INSTRUCTION_ID:
      first = *frame_get_acc(frame);
      if (PRIMITIVE == first.type) {
        _execute_BOR(vm, frame, scope, ins);
        break;
      }
      if (NONE != first.type) {
        *frame_mutable_acc(frame) = first;
        break;
      }
      tmp = scope_lookup(scope, ins->id, &second);
      *frame_mutable_acc(frame) = (NULL == tmp) ? NONE_VALUE : *tmp;
      break;
    case INSTRUCTION_PRIMITIVE:
      first = *frame_get_acc(frame);
      if (PRIMITIVE == first.type) {
        _execute_BOR(vm, frame, scope, ins);
        break;
      }
      if (NONE != first.type) {
        break;
      }
      *frame_mutable_acc(frame) = value_primitive(ins->val);
      break;
    default:
      FATALF("Invalid arg type=%d for BOR.", ins->type);
  }
}

Value _module_filename(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  const FileInfo *fi = modulemanager_get_fileinfo(
      vm_module_manager(frame->parent_isolate->vm), obj->_module_obj);
  if (NULL == fi) {
    return NONE_VALUE;
  }
  if (NULL == file_info_name(fi)) {
    return NONE_VALUE;
  }
  return value_object(istring_new(frame->parent_isolate->heap,
                                  file_info_name(fi),
                                  strlen(file_info_name(fi))));
}

Value _module_source_filename(Frame *frame, Scope *ctx, Object *obj,
                              Value *args) {
  const char *source_fn = tape_get_external_source(obj->_module_obj->_tape);
  if (NULL != source_fn) {
    return value_object(
        istring_new(frame->parent_isolate->heap, source_fn, strlen(source_fn)));
  } else {
    return NONE_VALUE;
  }
}

Value _stackline_linetext(Frame *frame, Scope *ctx, Object *obj, Value *args) {
  if (NULL == stackline_module(obj)) {
    return NONE_VALUE;
  }
  const FileInfo *fi = modulemanager_get_fileinfo(
      vm_module_manager(frame->parent_isolate->vm), stackline_module(obj));
  const LineInfo *li = file_info_lookup(fi, stackline_linenum(obj) + 1);
  return (NULL == li)
             ? NONE_VALUE
             : value_object(string_new(frame->parent_isolate->heap,
                                       li->line_text, strlen(li->line_text)));
}

void _add_filename_method(VM *vm) {
  Function *filename =
      native_method(Class_Module, global_intern("filename"), _module_filename);
  add_reflection_to_function(vm_main_isolate(vm)->heap,
                             Class_Module->_reflection, filename);
  Function *source_filename = native_method(
      Class_Module, global_intern("source_filename"), _module_source_filename);
  add_reflection_to_function(vm_main_isolate(vm)->heap,
                             Class_Module->_reflection, source_filename);
  Function *linetext = native_method(Class_StackLine, global_intern("linetext"),
                                     _stackline_linetext);
  add_reflection_to_function(vm_main_isolate(vm)->heap,
                             Class_StackLine->_reflection, linetext);
}

VM *vm_create(const char *lib_location, uint32_t max_isolate_object_count,
              bool async_enabled) {
  VM *vm = MNEW(VM);
  vm->async_enabled = async_enabled;
  vm->exit_code = EXIT_SUCCESS;
  IsolateArray_init(&vm->isolates);
  HeapConf heap_conf = {
      .mgraph_config = {.eager_delete_edges = true, .eager_delete_nodes = true},
      .max_object_count = max_isolate_object_count};
  vm->base_heap_conf = heap_conf;
  vm->isolate_create_lock = mutex_create();
  vm->blocking_pool = threadpool_create(DEFAULT_THREADPOOL_SIZE);
  vm->main = create_isolate_no_reflection(vm);
  modulemanager_init(&vm->mm, vm->main->heap);
  register_builtin(&vm->mm, vm->main->heap, lib_location);
  // Have to put this here since there was no way else to get around the
  // circular dependency.
  _add_filename_method(vm);
  add_reflection_to_isolate(vm->main);
  return vm;
}

void vm_delete(VM *vm) {
  ASSERT(vm != NULL);
  IsolateArrayIterator iter;
  IsolateArray_iterator(&iter, &vm->isolates);
  for (; IsolateArray_has_next(&iter); IsolateArray_next(&iter)) {
    Isolate *proc = IsolateArray_mutable_value(&iter);
    isolate_finalize(proc);
  }
  IsolateArray_finalize(&vm->isolates);
  mutex_close(vm->isolate_create_lock);
  threadpool_delete(vm->blocking_pool);
  modulemanager_finalize(&vm->mm);
  RELEASE(vm);
}

Isolate *vm_main_isolate(VM *vm) { return vm->main; }

bool _execute_EQ(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  const Value *acc, *lookup;
  Value first, second, tmp;
  bool result;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      second = frame_popstack(frame);
      first = frame_popstack(frame);
      if (IS_OBJECT(&first)) {
        if (IS_OBJECT(&second) && first.obj == second.obj) {
          *frame_mutable_acc(frame) =
              (EQ == ins->op) ? TRUE_VALUE : FALSE_VALUE;
          return false;
        }
        const Function *f = class_get_function(
            first.obj->_class, (EQ == ins->op) ? EQ_FN_NAME : NEQ_FN_NAME);
        if (NULL != f) {
          *frame_mutable_acc(frame) = second;
          return _call_function_base(frame, scope, f, first.obj, scope);
        }
        *frame_mutable_acc(frame) = (EQ == ins->op) ? FALSE_VALUE : TRUE_VALUE;
        return false;
      }
      if (IS_NONE(&first)) {
        *frame_mutable_acc(frame) =
            IS_NONE(&second) && (EQ == ins->op) ? TRUE_VALUE : FALSE_VALUE;
        return false;
      }
      if (IS_NONE(&second)) {
        *frame_mutable_acc(frame) = (EQ == ins->op) ? FALSE_VALUE : TRUE_VALUE;
        return false;
      }
      if (PRIMITIVE != second.type) {
        raise_error(frame, scope, "RHS for op 'EQ' must be primitive (1).");
        return false;
      }
      if (PRIMITIVE != first.type) {
        raise_error(frame, scope, "LHS for op 'EQ' must be primitive (1).");
        return false;
      }
      result = primitive_equals(&first.pri, &second.pri);
      *frame_mutable_acc(frame) =
          ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
              ? TRUE_VALUE
              : FALSE_VALUE;
      break;
    case INSTRUCTION_ID:
      acc = frame_get_acc(frame);
      lookup = scope_lookup(scope, ins->id, &tmp);
      if (IS_OBJECT(acc)) {
        first = *acc;
        if (IS_OBJECT(lookup) && first.obj == lookup->obj) {
          *frame_mutable_acc(frame) =
              (EQ == ins->op) ? TRUE_VALUE : FALSE_VALUE;
          return false;
        }
        const Function *f = class_get_function(
            first.obj->_class, (EQ == ins->op) ? EQ_FN_NAME : NEQ_FN_NAME);
        if (NULL != f) {
          *frame_mutable_acc(frame) = (lookup == NULL) ? NONE_VALUE : *lookup;
          return _call_function_base(frame, scope, f, first.obj, scope);
        }

        *frame_mutable_acc(frame) = (EQ == ins->op) ? FALSE_VALUE : TRUE_VALUE;
        return false;
      }
      if (NULL != acc && PRIMITIVE != acc->type) {
        raise_error(frame, scope, "LHS for op 'EQ' must be primitive (2).");
        return false;
      }
      if (NULL != lookup && PRIMITIVE != lookup->type) {
        raise_error(frame, scope, "RHS for op 'EQ' must be primitive (2).");
        return false;
      }
      result = primitive_equals(&acc->pri, &lookup->pri);
      *frame_mutable_acc(frame) =
          ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
              ? TRUE_VALUE
              : FALSE_VALUE;
      break;
    case INSTRUCTION_PRIMITIVE:
      acc = frame_get_acc(frame);
      if (NULL != acc && PRIMITIVE != acc->type) {
        raise_error(frame, scope, "LHS for op 'EQ' must be primitive (3).");
        return false;
      }
      result = primitive_equals(&acc->pri, &ins->val);
      *frame_mutable_acc(frame) =
          ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
              ? TRUE_VALUE
              : FALSE_VALUE;
      break;
    default:
      FATALF("Invalid arg type=%d for EQ.", ins->type);
  }
  return false;
}

void _execute_RES(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value *member;
  Value tmp;
  Object *str;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *frame_mutable_acc(frame) = frame_popstack(frame);
      break;
    case INSTRUCTION_ID:
      member = scope_lookup(scope, ins->id, &tmp);
      *frame_mutable_acc(frame) = (NULL == member) ? NONE_VALUE : *member;
      break;
    case INSTRUCTION_PRIMITIVE:
      *frame_mutable_acc(frame) = value_primitive(ins->val);
      break;
    case INSTRUCTION_STRING:
      str = ins->op == IRES ? istring_new(frame->parent_isolate->heap, ins->str,
                                          strlen(ins->str))
                            : string_new(frame->parent_isolate->heap, ins->str,
                                         strlen(ins->str));
      *frame_mutable_acc(frame) = value_object(str);
      break;
    default:
      FATALF("Invalid arg type=%d for RES.", ins->type);
  }
}

void _execute_PEEK(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value *member;
  Value tmp;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *frame_mutable_acc(frame) = *frame_peekstack(frame);
      break;
    case INSTRUCTION_PRIMITIVE:
      *frame_mutable_acc(frame) = *frame_peekstack_n(frame, pint(&ins->val));
      break;
    case INSTRUCTION_ID:
      member = scope_lookup(scope, ins->id, &tmp);
      *frame_mutable_acc(frame) = (NULL == member) ? NONE_VALUE : *member;
      break;
    default:
      FATALF("Invalid arg type=%d for PEEK.", ins->type);
  }
}

void _execute_DUP(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for DUP.", ins->type);
  }
  Value peek = *frame_peekstack(frame);
  *frame_pushstack(frame) = peek;
}

void _execute_PUSH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Object *str;
  Value *member;
  Value tmp;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *frame_pushstack(frame) = *frame_get_acc(frame);
      break;
    case INSTRUCTION_ID:
      member = scope_lookup(scope, ins->id, &tmp);
      *frame_pushstack(frame) = (NULL == member) ? NONE_VALUE : *member;
      break;
    case INSTRUCTION_PRIMITIVE:
      *frame_pushstack(frame) = value_primitive(ins->val);
      break;
    case INSTRUCTION_STRING:
      // TODO: Maybe precompute the length of the string?
      str = ins->op == IPSH ? istring_new(frame->parent_isolate->heap, ins->str,
                                          strlen(ins->str))
                            : string_new(frame->parent_isolate->heap, ins->str,
                                         strlen(ins->str));
      *frame_pushstack(frame) = value_object(str);
      break;
    default:
      FATALF("Invalid arg type=%d for PUSH.", ins->type);
  }
}

void _execute_PNIL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for PNIL.", ins->type);
  }
  *frame_pushstack(frame) = NONE_VALUE;
}

void _execute_RNIL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for RNIL.", ins->type);
  }
  *frame_mutable_acc(frame) = NONE_VALUE;
}

void _execute_PTRU(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for PTRU.", ins->type);
  }
  *frame_pushstack(frame) = TRUE_VALUE;
}

void _execute_RTRU(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for RTRU.", ins->type);
  }
  *frame_mutable_acc(frame) = TRUE_VALUE;
}

void _execute_PFLS(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for PFLS.", ins->type);
  }
  *frame_pushstack(frame) = FALSE_VALUE;
}

void _execute_RFLS(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for RFLS.", ins->type);
  }
  *frame_mutable_acc(frame) = FALSE_VALUE;
}

void _execute_FLD(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    FATALF("Invalid arg type=%d for FLD.", ins->type);
  }
  const Value *acc = frame_get_acc(frame);
  if (NULL == acc || OBJECT != acc->type) {
    raise_error(frame, scope,
                "Attempted to set field '%s' on something not an object.",
                ins->id);
    return;
  }
  Value obj = frame_popstack(frame);
  object_set_member(frame->parent_isolate->heap, acc->obj, ins->id, &obj);
}

void _execute_LET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_ID:
      scope_let(scope, ins->id, frame_get_acc(frame));
      break;
    default:
      FATALF("Invalid arg type=%d for LET.", ins->type);
  }
}

void _execute_SET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_ID:
      scope_set(scope, ins->id, frame_get_acc(scope->parent_frame));
      break;
    default:
      FATALF("Invalid arg type=%d for SET.", ins->type);
  }
}

void _execute_MSET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value tmp;
  Value *mdl = scope_lookup(scope, TMP_MODULE_HOLDER, &tmp);
  if (!IS_CLASS(mdl, Class_Module)) {
    FATALF("Temp held module must be of type Module.");
  }
  switch (ins->type) {
    case INSTRUCTION_ID:
      object_set_member_obj(frame->parent_isolate->heap,
                            scope->module->_reflection, ins->id, mdl->obj);
      break;
    case INSTRUCTION_STRING:
      object_set_member_obj(frame->parent_isolate->heap,
                            scope->module->_reflection,
                            mdl->obj->_module_obj->_name, mdl->obj);
      break;
    default:
      FATALF("Invalid arg type=%d for MSET.", ins->type);
  }
}

void _execute_GET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    FATALF("Invalid arg type=%d for GET.", ins->type);
  }
  const Value *e = frame_get_acc(frame);
  if (NULL == e || OBJECT != e->type) {
    raise_error(frame, scope, "Attempted to get field '%s' from a %s.", ins->id,
                (e == NULL || NONE == e->type) ? "None" : "Primtive");
    return;
  }
  *frame_mutable_acc(frame) = object_get_maybe_wrap(
      e->obj, ins->id, frame->parent_isolate->heap, scope);
}

void _execute_GTSH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    FATALF("Invalid arg type=%d for GTSH.", ins->type);
  }
  const Value *e = frame_get_acc(frame);
  if (NULL == e || OBJECT != e->type) {
    raise_error(frame, scope, "Attempted to get field '%s' from a %s.", ins->id,
                (e == NULL || NONE == e->type) ? "None" : "Primtive");
    return;
  }
  Value get_result = object_get_maybe_wrap(e->obj, ins->id,
                                           frame->parent_isolate->heap, scope);
  *frame_pushstack(frame) = get_result;
}

Scope *_execute_as_new_frame(Frame *frame, Object *self, Module *m,
                             uint32_t ins_pos) {
  Frame *new_frame = isolate_create_unqueued_frame(frame->parent_isolate);
  new_frame->parent_frame = frame;
  Scope *ctx = frame_create_scope(new_frame, self, m, ins_pos);
  *frame_mutable_acc(new_frame) = *frame_get_acc(frame);
  return ctx;
}

typedef struct {
  Frame *frame;
  Scope *scope;
  const Function *func;
  Object *self;
} BlockingCallArgs;

void _execute_blocking(BlockingCallArgs *args) {
  NativeFn native_fn = (NativeFn)args->func->_native_fn;
  *frame_mutable_acc(args->frame) =
      native_fn(args->frame, args->scope, args->self,
                (Value *)frame_get_acc(args->frame));
}

void _execute_blocking_callback(BlockingCallArgs *args) {
  isolate_remove_blocking_frame(args->frame->parent_isolate, args->frame);
  args->frame->state = FRAME_COMPLETE;
  _mark_frame_complete(args->frame->parent_isolate, args->frame,
                       /*should_push=*/false);
  RELEASE(args);
}

Frame *_maybe_load_module(Frame *frame, Module *module) {
  if (module->_is_initialized) {
    return NULL;
  }
  module->_is_initialized = true;
  if (NULL == frame_get_acc(frame)) {
    *frame_mutable_acc(frame) = NONE_VALUE;
  }
  Scope *new_ctx = _execute_as_new_frame(frame, module->_reflection, module, 0);
  isolate_enqueue_frame(frame->parent_isolate, new_ctx->parent_frame);
  FrameSet_insert(&new_ctx->parent_frame->dependent_frames, frame,
                  sizeof(Frame *));
  return new_ctx->parent_frame;
}

BlockingCallArgs *_create_blocking_call_args(Frame *frame, Scope *scope,
                                             const Function *func,
                                             Object *self) {
  BlockingCallArgs *args = MNEW(BlockingCallArgs);
  args->frame = frame;
  args->scope = scope;
  args->func = func;
  args->self = self;
  return args;
}

// Context is only necessary for native functions.
bool _call_function_base(Frame *frame, Scope *scope, const Function *func,
                         Object *self, Scope *parent_scope) {
  Isolate *isolate = frame->parent_isolate;
  if (func->_is_native) {
    NativeFn native_fn = (NativeFn)func->_native_fn;
    if (NULL == native_fn) {
      FATALF("Invalid native function.");
    }
    if (func->_is_blocking && isolate->vm->async_enabled) {
      ASSERT(func->_is_async);
      Frame *new_frame = isolate_create_unqueued_frame(isolate);
      new_frame->parent_frame = frame;
      *frame_mutable_acc(new_frame) = *frame_get_acc(frame);
      *frame_mutable_acc(frame) = value_object(future_create(new_frame));
      BlockingCallArgs *args =
          _create_blocking_call_args(new_frame, scope, func, self);
      isolate_add_blocking_frame(isolate, new_frame, isolate->vm->blocking_pool,
                                 (VoidFnPtr)_execute_blocking,
                                 (VoidFnPtr)_execute_blocking_callback,
                                 (VoidPtr)args);
      return false;
    }
    *frame_mutable_acc(frame) =
        native_fn(frame, scope, self, (Value *)frame_get_acc(frame));
    return false;
  } else if (func->_is_native2) {
    NativeFunctionHandlerFn native_fn =
        (NativeFunctionHandlerFn)func->_native_fn2;
    FunctionContext fn_ctx;
    FunctionContext_init(&fn_ctx, frame, scope, frame_get_acc(frame));
    native_fn(&fn_ctx);
    *frame_mutable_acc(frame) = *FunctionContext_get_retval(&fn_ctx);
    return false;
  }
  Scope *fn_ctx = _execute_as_new_frame(frame, self, (Module *)func->_module,
                                        func->_ins_pos);
  scope_set_function(fn_ctx, func);
  Frame *module_main_frame =
      _maybe_load_module(fn_ctx->parent_frame, (Module *)self->_class->_module);
  if (NULL != module_main_frame && !func->_is_async) {
    fn_ctx->parent_frame->state = FRAME_WAITING;
    isolate_insert_waiting_frame(fn_ctx->parent_frame->parent_isolate,
                                 fn_ctx->parent_frame);
  } else {
    isolate_enqueue_frame(fn_ctx->parent_frame->parent_isolate,
                          fn_ctx->parent_frame);
  }
  if (func->_is_anon) {
    fn_ctx->previous_scope = parent_scope;
  }
  if (func->_is_async && frame->parent_isolate->vm->async_enabled) {
    *frame_mutable_acc(frame) =
        value_object(future_create(fn_ctx->parent_frame));
    return false;
  }
  FrameSet_insert(&fn_ctx->parent_frame->dependent_frames, frame,
                  sizeof(Frame *));
  return true;
}

bool _call_method(Frame *frame, Object *obj, Scope *scope,
                  const Instruction *ins) {
  ASSERT(obj != NULL);
  ASSERT(ins != NULL);
  ASSERT(INSTRUCTION_ID == ins->type);
  const Class *class = (Class *)obj->_class;
  Value method =
      object_get_maybe_wrap(obj, ins->id, frame->parent_isolate->heap, scope);
  if (NONE == method.type) {
    raise_error(frame, scope, "Failed to find method '%s' on %s", ins->id,
                class->_name);
    return false;
  }
  if (OBJECT != method.type) {
    raise_error(frame, scope, "Attempted to treat '%s' on %s as a method.",
                ins->id, class->_name);
    return false;
  }
  if (Class_FunctionRef != method.obj->_class) {
    raise_error(frame, scope,
                "Attempted to treat '%s' of type %s on %s as a method.",
                ins->id, method.obj->_class->_name, class->_name);
    return false;
  }
  return _call_function_base(frame, scope, function_ref_get_func(method.obj),
                             function_ref_get_object(method.obj),
                             function_ref_get_parent_scope(method.obj));
}

bool _call_function(Frame *frame, Scope *scope, Function *func) {
  return _call_function_base(frame, scope, func, func->_module->_reflection,
                             scope);
}

bool _execute_CALL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value fn;
  if (INSTRUCTION_ID == ins->type) {
    if (CLLN == ins->op) {
      *frame_mutable_acc(frame) = NONE_VALUE;
    }
    Value obj = frame_popstack(frame);
    if (OBJECT != obj.type) {
      const char *type_str = NONE == obj.type                    ? "None"
                             : ptype(&obj.pri) == PRIMITIVE_CHAR ? "Char"
                             : ptype(&obj.pri) == PRIMITIVE_INT  ? "Int"
                                                                 : "Float";
      raise_error(frame, scope, "Calling function '%s' on type %s.", ins->id,
                  type_str);
      return false;
    }
    if (Class_Module == obj.obj->_class) {
      Module *m = obj.obj->_module_obj;
      ASSERT(m != NULL);
      Object *fn_obj = module_lookup(m, ins->id);
      if (NULL == fn_obj) {
        return _call_method(frame, obj.obj, scope, ins);
      }
      fn = value_object(fn_obj);
    } else {
      return _call_method(frame, obj.obj, scope, ins);
    }
  } else {
    ASSERT(INSTRUCTION_NO_ARG == ins->type);
    fn = frame_popstack(frame);
  }
  if (fn.type != OBJECT) {
    raise_error(frame, scope,
                "Attempted to call something not a function (not an object).");
    return false;
  }
  if (fn.obj->_class == Class_Class) {
    Class *class = fn.obj->_class_obj;
    Object *obj = heap_new(frame->parent_isolate->heap, class);
    const Function *constructor = class_get_function(class, CONSTRUCTOR_KEY);
    if (NULL == constructor) {
      *frame_mutable_acc(frame) = value_object(obj);
      return false;
    } else {
      if (CLLN == ins->op) {
        *frame_mutable_acc(frame) = NONE_VALUE;
      }
      return _call_function_base(frame, scope, constructor, obj, scope);
    }
  }
  if (fn.obj->_class == Class_FunctionRef) {
    if (CLLN == ins->op) {
      *frame_mutable_acc(frame) = NONE_VALUE;
    }
    return _call_function_base(frame, scope, function_ref_get_func(fn.obj),
                               function_ref_get_object(fn.obj),
                               function_ref_get_parent_scope(fn.obj));
  }
  if (fn.obj->_class != Class_Function) {
    raise_error(frame, scope,
                "Attempted to call something not a function (class=%s).",
                fn.obj->_class->_name);
    return false;
  }
  Function *func = fn.obj->_function_obj;
  if (CLLN == ins->op) {
    *frame_mutable_acc(frame) = NONE_VALUE;
  }
  return _call_function(frame, scope, func);
}

bool _execute_WAIT(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  const Value *acc = frame_get_acc(frame);
  // Only wait for futures.
  if (!IS_CLASS(acc, Class_Future)) {
    return false;
  }
  Future *future = (Future *)acc->obj->_internal_obj;
  if (!future_is_complete(future)) {
    FrameSet_insert(&future_get_frame(future)->dependent_frames, frame,
                    sizeof(Frame *));
    return true;
  }
  *frame_mutable_acc(frame) =
      *future_get_value(frame->parent_isolate->heap, acc->obj);
  return false;
}

void _execute_RET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value tmp;
  // if (NULL == task->parent_task) {
  //   return;
  // }
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      // *task_mutable_resval(task->dependent_task) = *task_get_resval(task);
      break;
    case INSTRUCTION_ID:
      *frame_mutable_acc(frame) = *scope_lookup(scope, ins->id, &tmp);
      break;
    case INSTRUCTION_PRIMITIVE:
      *frame_mutable_acc(frame) = value_primitive(ins->val);
      break;
    default:
      FATALF("Invalid arg type=%d for RET.", ins->type);
  }
}

Scope *_execute_NBLK(VM *vm, Frame *frame, Scope *scope,
                     const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      return frame_create_scope(scope->parent_frame, scope->self.obj,
                                scope->module, scope->ins);
    default:
      FATALF("Invalid arg type=%d for NBLK.", ins->type);
  }
  return NULL;
}

Scope *_execute_BBLK(VM *vm, Frame *frame, Scope *scope,
                     const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      return frame_back_scope(frame);
    default:
      FATALF("Invalid arg type=%d for BBLK.", ins->type);
  }
  return NULL;
}

void _execute_JMP(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    FATALF("Invalid arg type=%d for JMP.", ins->type);
  }
  scope->ins += ins->val._int_val;
}

void _execute_IF(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    FATALF("Invalid arg type=%d for IF.", ins->type);
  }
  const Value *acc = frame_get_acc(frame);
  bool is_false = IS_FALSE(acc);
  if ((is_false && (IFN == ins->op)) || (!is_false && (IF == ins->op))) {
    scope->ins += ins->val._int_val;
  }
}

void _execute_EXIT(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE == ins->type) {
    // Implicit exit at the end of a module, which does not set the exit code.
    *frame_mutable_acc(frame) = value_primitive(ins->val);
  } else {
    // Explicit `exit` statement.
    const Value *exit_val = frame_get_acc(frame);
    if (IS_INT(exit_val)) {
      vm->exit_code = (int)pint(&exit_val->pri);
    }
  }
  frame->state = FRAME_COMPLETE;
  scope->ins++;
}

void _execute_NOT(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid arg type=%d for NOT.", ins->type);
  }
  const Value *acc = frame_get_acc(frame);
  *frame_mutable_acc(frame) = IS_FALSE(acc) ? TRUE_VALUE : FALSE_VALUE;
}

void _execute_ANEW(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    FATALF("Invalid ANEW, ID type.");
  }
  Object *array_obj = heap_new(frame->parent_isolate->heap, Class_Array);
  *frame_mutable_acc(frame) = value_object(array_obj);
  if (INSTRUCTION_NO_ARG == ins->type) {
    return;
  }
  if (INSTRUCTION_PRIMITIVE != ins->type || PRIMITIVE_INT != ptype(&ins->val)) {
    FATALF("Invalid ANEW requires int primitive.");
  }
  int32_t num_args = pint(&ins->val);
  int i;
  for (i = 0; i < num_args; ++i) {
    Value e = frame_popstack(frame);
    array_add(frame->parent_isolate->heap, array_obj, &e);
  }
}

bool _execute_AIDX(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Object *arr_obj;
  const Value *index = NULL;
  Value index_e;

  Value arr_value = frame_popstack(frame);
  if (OBJECT != arr_value.type) {
    raise_error(frame, scope, "Invalid array index on non-indexable.");
    return false;
  }
  arr_obj = arr_value.obj;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      index = frame_get_acc(frame);
      break;
    case INSTRUCTION_ID:
      index = scope_lookup(scope, ins->id, &index_e);
      break;
    case INSTRUCTION_PRIMITIVE:
      if (PRIMITIVE_INT != ptype(&ins->val) || pint(&ins->val) < 0) {
        raise_error(frame, scope, "Invalid array index.");
        return false;
      }
      index_e = value_primitive(ins->val);
      index = &index_e;
      break;
    default:
      FATALF("Invalid arg type=%d for AIDX.", ins->type);
  }
  if (NULL == index) {
    raise_error(frame, scope, "Invalid array index.");
    return false;
  }
  if (Class_Array == arr_obj->_class) {
    Array *arr = (Array *)arr_obj->_internal_obj;
    if (PRIMITIVE != index->type || PRIMITIVE_INT != ptype(&index->pri) ||
        pint(&index->pri) < 0) {
      raise_error(frame, scope, "Invalid array index.");
      return false;
    }
    int32_t i_index = pint(&index->pri);
    if (i_index >= Array_size(arr)) {
      raise_error(frame, scope, "Invalid array index.");
      return false;
    }
    *frame_mutable_acc(frame) = *Array_get_ref_unchecked(arr, i_index);
    return false;
  }
  if (Class_Tuple == arr_obj->_class) {
    Tuple *tuple = (Tuple *)arr_obj->_internal_obj;
    if (PRIMITIVE != index->type || PRIMITIVE_INT != ptype(&index->pri) ||
        pint(&index->pri) < 0) {
      raise_error(frame, scope, "Invalid tuple index.");
      return false;
    }
    int32_t i_index = pint(&index->pri);
    if (i_index >= tuple_size(tuple)) {
      raise_error(frame, scope, "Invalid tuple index.");
      return false;
    }
    *frame_mutable_acc(frame) = *tuple_get(tuple, i_index);
    return false;
  }

  const Function *aidx_fn =
      class_get_function(arr_obj->_class, ARRAYLIKE_INDEX_KEY);
  if (NULL != aidx_fn) {
    if (INSTRUCTION_PRIMITIVE == ins->type || INSTRUCTION_ID == ins->type) {
      *frame_mutable_acc(frame) = *index;
    }
    return _call_function_base(frame, scope, aidx_fn, arr_obj, scope);
  }

  raise_error(frame, scope, "Invalid array index on non-indexable.");
  return false;
}

bool _execute_ASET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  Value arr_value = frame_popstack(frame);
  Value new_val = frame_popstack(frame);
  const Value *index = frame_get_acc(frame);
  if (OBJECT != arr_value.type) {
    raise_error(frame, scope, "Cannot set index value on non-indexable.");
    return false;
  }

  if (Class_Array == arr_value.obj->_class) {
    if (NULL == index || PRIMITIVE != index->type ||
        PRIMITIVE_INT != ptype(&index->pri)) {
      raise_error(frame, scope, "Cannot index with non-int.");
      return false;
    }
    int32_t i_index = pint(&index->pri);
    if (i_index < 0) {
      raise_error(frame, scope, "Array index out of bounds: %d", i_index);
      return false;
    }
    array_set(frame->parent_isolate->heap, arr_value.obj, i_index, &new_val);
    return false;
  }
  const Function *aset_fn =
      class_get_function(arr_value.obj->_class, ARRAYLIKE_SET_KEY);
  if (NULL != aset_fn) {
    Object *args = heap_new(frame->parent_isolate->heap, Class_Tuple);
    args->_internal_obj = tuple_create(2);
    Tuple *t = (Tuple *)args->_internal_obj;
    *tuple_get_mutable(t, 0) = *index;
    *tuple_get_mutable(t, 1) = new_val;
    *frame_mutable_acc(frame) = value_object(args);
    return _call_function_base(frame, scope, aset_fn, arr_value.obj, scope);
  }
  raise_error(frame, scope, "Cannot set index value on non-indexable.");
  return false;
}

void _execute_TUPL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    FATALF("Invalid TUPL, ID type.");
  }
  uint32_t num_args = pint(&ins->val);
  Object *tuple_obj = heap_new(frame->parent_isolate->heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(num_args);
  *frame_mutable_acc(frame) = value_object(tuple_obj);
  if (INSTRUCTION_NO_ARG == ins->type) {
    return;
  }
  if (INSTRUCTION_PRIMITIVE != ins->type || PRIMITIVE_INT != ptype(&ins->val)) {
    FATALF("Invalid TUPL requires int primitive.");
  }
  int i;
  for (i = 0; i < num_args; ++i) {
    Value e = frame_popstack(frame);
    tuple_set(frame->parent_isolate->heap, tuple_obj, i, &e);
  }
}

void _execute_TLEN(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Invalid TLEN type.");
  }
  const Value *e = frame_peekstack(frame);
  if (NULL == e || OBJECT != e->type || Class_Tuple != e->obj->_class) {
    *frame_mutable_acc(frame) = value_int(-1);
    return;
  }
  *frame_mutable_acc(frame) =
      value_int(tuple_size((Tuple *)e->obj->_internal_obj));
}

void _execute_TGTE(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type || PRIMITIVE_INT != ptype(&ins->val)) {
    raise_error(frame, scope, "Invalid TLEN type.");
    return;
  }
  const Value *e = frame_get_acc(frame);

  int test_len = pint(&ins->val);
  if (1 == test_len) {
    *frame_mutable_acc(frame) =
        (NULL != e && NONE != e->type) ? TRUE_VALUE : FALSE_VALUE;
    return;
  }
  if (NULL == e || OBJECT != e->type || Class_Tuple != e->obj->_class) {
    *frame_mutable_acc(frame) = FALSE_VALUE;
    return;
  }
  Tuple *t = (Tuple *)e->obj->_internal_obj;
  uint32_t tlen = tuple_size(t);
  *frame_mutable_acc(frame) = tlen >= test_len ? TRUE_VALUE : FALSE_VALUE;
}

void _execute_TGET(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type || PRIMITIVE_INT != ptype(&ins->val)) {
    raise_error(frame, scope, "Invalid TGET type.");
    return;
  }
  int32_t index = pint(&ins->val);
  const Value *e = frame_get_acc(frame);
  if (!IS_TUPLE(e)) {
    if (NULL != e && 0 == index) {
      return;
    }
    *frame_mutable_acc(frame) = value_int(-1);
    raise_error(frame, scope, "Attempted to index something not a tuple.");
    return;
  }
  Tuple *t = (Tuple *)e->obj->_internal_obj;
  if (index < 0 || index >= tuple_size(t)) {
    raise_error(frame, scope,
                "Tuple index out of bounds. Index=%d, Tuple.len=%d.", index,
                tuple_size(t));
    return;
  }
  *frame_mutable_acc(frame) = *tuple_get(t, index);
}

void _execute_IS(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    FATALF("Weird type for IS.");
  }
  Value rhs = frame_popstack(frame);
  Value lhs = frame_popstack(frame);
  if (OBJECT != rhs.type || Class_Class != rhs.obj->_class) {
    raise_error(frame, scope,
                "Cannot perform type-check against a non-object type.");
    return;
  }
  if (lhs.type != OBJECT) {
    *frame_mutable_acc(frame) = FALSE_VALUE;
    return;
  }
  if (inherits_from(lhs.obj->_class, rhs.obj->_class_obj)) {
    *frame_mutable_acc(frame) = TRUE_VALUE;
  } else {
    *frame_mutable_acc(frame) = FALSE_VALUE;
  }
}

bool _execute_LMDL(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type && INSTRUCTION_STRING != ins->type) {
    FATALF("Weird type for LMDL.");
  }
  Module *module = modulemanager_lookup(
      &vm->mm, ins->type == INSTRUCTION_ID ? ins->id : ins->str);
  if (NULL == module) {
    raise_error(frame, scope, "Module '%s' not found.",
                ins->type == INSTRUCTION_ID ? ins->id : ins->str);
    return false;
  }
  bool result = NULL != _maybe_load_module(frame, module);
  Value module_reflection = value_object(module->_reflection);
  scope_set(scope, TMP_MODULE_HOLDER, &module_reflection);
  return result;
}

bool _execute_CTCH(VM *vm, Frame *frame, Scope *scope, const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    FATALF("Invalid arg type=%d for CTCH.", ins->type);
  }
  scope->catch_ins = scope->ins + ins->val._int_val + 1;
  return true;
}

void _execute_RAIS(VM *vm, Frame *frame, Scope *scope) {
  const Value *err = frame_get_acc(frame);
  if (OBJECT != err->type || !inherits_from(err->obj->_class, Class_Error)) {
    raise_error(frame, scope, "raise can only be invoked with an Error().");
    return;
  }
  raise_error_with_object(frame, scope, err->obj);
}

bool _attemp_catch_error(Frame *frame, Scope *ctx) {
  while (ctx->catch_ins < 0 && NULL != (ctx = frame_back_scope(frame)));
  // There was no try/catch block.
  if (NULL == ctx) {
    frame->state = FRAME_ERROR;
    return false;
  }
  ctx->ins = ctx->catch_ins;
  ctx->error = NULL;
  ctx->catch_ins = -1;
  return true;
}

// Please forgive me father, for I have sinned.
FrameState vm_execute_frame(VM *vm, Frame *frame) {
  frame->state = FRAME_RUNNING;
  frame->wait_reason = NOT_WAITING;
  // This only happens when an error bubbles up to the main task
  if (NULL == frame->current) {
    return FRAME_COMPLETE;
  }
  Scope *scope = frame->current;

  if (frame->child_frame_has_error) {
    const Value *error_e = frame_get_acc(frame);
    ASSERT(error_e != NULL);
    ASSERT(OBJECT == error_e->type);
    ASSERT(inherits_from(error_e->obj->_class, Class_Error));
    scope->error = error_e->obj;
    frame->child_frame_has_error = false;
  }
  for (;;) {
    if (NULL != scope->error) {
      // char *tmp = CNEW_ARR(char, 100);
      // size_t sz;
      // getline(&tmp, &sz, stdin);
      if (!_attemp_catch_error(frame, scope)) {
        goto end_of_loop;
      }
    }
    const Instruction *ins = scope_ins(scope);
#ifdef DEBUG
    SYNCHRONIZED(vm->isolate_create_lock, {
      fprintf(stdout, "(p=%p, t=%p, c=%p) ", frame->parent_isolate, frame,
              frame->current);
      instruction_write(ins, stdout, /* minimize */ false);
      fprintf(stdout, "\n");
      fflush(stdout);
    });
#endif
    switch (ins->op) {
      case RES:
      case IRES:
        _execute_RES(vm, frame, scope, ins);
        break;
      case RNIL:
        _execute_RNIL(vm, frame, scope, ins);
        break;
      case PUSH:
      case IPSH:
        _execute_PUSH(vm, frame, scope, ins);
        break;
      case PNIL:
        _execute_PNIL(vm, frame, scope, ins);
        break;
      case PSRS:
        _execute_PUSH(vm, frame, scope, ins);
        *frame_mutable_acc(frame) = *frame_peekstack(frame);
        break;
      case PEEK:
        _execute_PEEK(vm, frame, scope, ins);
        break;
      case DUP:
        _execute_DUP(vm, frame, scope, ins);
        break;
      case FLD:
        _execute_FLD(vm, frame, scope, ins);
        break;
      case LET:
        _execute_LET(vm, frame, scope, ins);
        break;
      case SET:
        _execute_SET(vm, frame, scope, ins);
        break;
      case MSET:
        _execute_MSET(vm, frame, scope, ins);
        break;
      case GET:
        _execute_GET(vm, frame, scope, ins);
        break;
      case GTSH:
        _execute_GTSH(vm, frame, scope, ins);
        break;
      case CALL:
      case CLLN:
        if (_execute_CALL(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FN_CALL;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case RET:
        _execute_RET(vm, frame, scope, ins);
        frame->state = FRAME_COMPLETE;
        scope->ins++;
        goto end_of_loop;
      case NBLK:
        scope = _execute_NBLK(vm, frame, scope, ins);
        break;
      case BBLK:
        scope = _execute_BBLK(vm, frame, scope, ins);
        break;
      case JMP:
        _execute_JMP(vm, frame, scope, ins);
        break;
      case IF:
      case IFN:
        _execute_IF(vm, frame, scope, ins);
        break;
      case EXIT:
        _execute_EXIT(vm, frame, scope, ins);
        goto end_of_loop;
      case ADD:
        _execute_ADD_with_string(vm, frame, scope, ins);
        break;
      case SUB:
        _execute_SUB(vm, frame, scope, ins);
        break;
      case MULT:
        _execute_MULT(vm, frame, scope, ins);
        break;
      case DIV:
        _execute_DIV(vm, frame, scope, ins);
        break;
      case MOD:
        _execute_MOD(vm, frame, scope, ins);
        break;
      case INC:
      case DEC:
        _execute_INC(vm, frame, scope, ins);
        break;
      case AND:
        _execute_AND(vm, frame, scope, ins);
        break;
      case OR:
        _execute_OR(vm, frame, scope, ins);
        break;
      case BAND:
        _execute_BAND(vm, frame, scope, ins);
        break;
      case BXOR:
        _execute_BXOR(vm, frame, scope, ins);
        break;
      case BOR:
        _execute_BOR_with_obj(vm, frame, scope, ins);
        break;
      case LT:
        _execute_LT(vm, frame, scope, ins);
        break;
      case GT:
        _execute_GT(vm, frame, scope, ins);
        break;
      case LTE:
        _execute_LTE(vm, frame, scope, ins);
        break;
      case GTE:
        _execute_GTE(vm, frame, scope, ins);
        break;
      case EQ:
      case NEQ:
        if (_execute_EQ(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FN_CALL;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case IS:
        _execute_IS(vm, frame, scope, ins);
        break;
      case NOT:
        _execute_NOT(vm, frame, scope, ins);
        break;
      case ANEW:
        _execute_ANEW(vm, frame, scope, ins);
        break;
      case AIDX:
        if (_execute_AIDX(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FN_CALL;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case ASET:
        if (_execute_ASET(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FN_CALL;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case TUPL:
        _execute_TUPL(vm, frame, scope, ins);
        break;
      case TLEN:
        _execute_TLEN(vm, frame, scope, ins);
        break;
      case TGET:
        _execute_TGET(vm, frame, scope, ins);
        break;
      case TGTE:
        _execute_TGTE(vm, frame, scope, ins);
        break;
      case CTCH:
        _execute_CTCH(vm, frame, scope, ins);
        break;
      case RAIS:
        _execute_RAIS(vm, frame, scope);
        break;
      case LMDL:
        if (_execute_LMDL(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FN_CALL;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case WAIT:
        if (_execute_WAIT(vm, frame, scope, ins)) {
          frame->state = FRAME_WAITING;
          frame->wait_reason = WAITING_ON_FUTURE;
          scope->ins++;
          goto end_of_loop;
        }
        break;
      case RTRU:
        _execute_RTRU(vm, frame, scope, ins);
        break;
      case PTRU:
        _execute_PTRU(vm, frame, scope, ins);
        break;
      case RFLS:
        _execute_RFLS(vm, frame, scope, ins);
        break;
      case PFLS:
        _execute_PFLS(vm, frame, scope, ins);
        break;
      default:
        FATALF("Unknown instruction: %s", op_to_str(ins->op));
    }
    scope->ins++;
  }
end_of_loop:
  return frame->state;
}

void _mark_actor_frame_complete(Isolate *isolate, Frame *frame,
                                bool should_push) {
  Frame *actor_frame = future_get_frame(frame->actor_future);
  Isolate *callee_isolate = actor_frame->parent_isolate;
  SYNCHRONIZED(callee_isolate->heap_access_lock, {
    *frame_mutable_acc(actor_frame) =
        value_copy(frame_get_acc(isolate->current_frame), callee_isolate->heap);
  });

  actor_frame->state = frame->state;

  isolate_remove_waiting_frame(callee_isolate, actor_frame);
  _mark_frame_complete(callee_isolate, actor_frame,
                       /*should_push=*/should_push);
}

void _broadcast_to_dependent_frames(Isolate *isolate, Frame *frame,
                                    bool should_push) {
  FrameSetIterator dependent_frames;
  FrameSet_iterator(&dependent_frames, &frame->dependent_frames);
  for (; FrameSet_has_next(&dependent_frames);
       FrameSet_next(&dependent_frames)) {
    Frame *dependent_frame = *FrameSet_mutable_value(&dependent_frames);
    // Only requeue parent task if it is waiting.
    if (FRAME_WAITING != dependent_frame->state) {
      continue;
    } else if (FRAME_ERROR == frame->state) {
      dependent_frame->child_frame_has_error = true;
    }
    *frame_mutable_acc(dependent_frame) = *frame_get_acc(frame);
    if (should_push) {
      isolate_push_frame(dependent_frame->parent_isolate, dependent_frame);
    } else {
      isolate_enqueue_frame(dependent_frame->parent_isolate, dependent_frame);
    }
    isolate_remove_waiting_frame(dependent_frame->parent_isolate,
                                 dependent_frame);
    condition_broadcast(isolate->frame_wait_cond);
  }
}

void _mark_frame_complete(Isolate *isolate, Frame *frame, bool should_push) {
  isolate_mark_frame_complete(isolate, frame);
  if (NULL != frame->actor_future) {
    _mark_actor_frame_complete(isolate, frame, should_push);
  }
  _broadcast_to_dependent_frames(isolate, frame, should_push);
}

bool _isolate_is_done(Isolate *isolate) {
  bool is_done = false;

  SYNCHRONIZED(isolate->frame_queue_lock, {
    CRITICAL(isolate->frame_waiting_cs, {
      if (FrameSet_size(&isolate->waiting_frames) == 0 &&
          FrameArray_size(&isolate->queued_frames) == 0) {
        is_done = true;
      }
    });
  });
  return is_done;
}

bool _isolate_should_broadcast_to_parent(Isolate *isolate) {
  if (NULL == isolate->future) {
    return false;
  }
  bool should_broadcast = false;

  SYNCHRONIZED(isolate->frame_queue_lock, {
    CRITICAL(isolate->frame_waiting_cs, {
      if (isolate->keep_alive) {
        should_broadcast =
            FrameSet_size(&isolate->waiting_frames) == 1 &&
            FrameArray_size(&isolate->queued_frames) == 0 &&
            FrameSet_contains(&isolate->waiting_frames,
                              isolate->keep_alive_frame, sizeof(Frame *));
      } else if (FrameSet_size(&isolate->waiting_frames) == 0 &&
                 FrameArray_size(&isolate->queued_frames) == 0) {
        should_broadcast = true;
      }
    });
  });
  return should_broadcast;
}

void _isolate_broadcast_to_parent(Isolate *isolate) {
  Frame *isolate_frame = future_get_frame(isolate->future);

  if (isolate->keep_alive) {
    const Value *actor_result = frame_get_acc(isolate->current_frame);
    if (IS_OBJECT(actor_result)) {
      // Must keep actor objects since other isolates assume it will always
      // exist. In the future, there must be a better cleanup process.
      heap_make_root(isolate->heap, actor_result->obj);
      *frame_mutable_acc(isolate_frame) = value_object(create_actor_object(
          isolate_frame->parent_isolate->heap, isolate, actor_result->obj));
    } else {
      *frame_mutable_acc(isolate_frame) = *actor_result;
    }
  } else {
    SYNCHRONIZED(isolate_frame->parent_isolate->heap_access_lock, {
      *frame_mutable_acc(isolate_frame) =
          value_copy(frame_get_acc(isolate->current_frame),
                     isolate_frame->parent_isolate->heap);
    });
  }
  isolate_remove_waiting_frame(isolate_frame->parent_isolate, isolate_frame);
  isolate_frame->state = FRAME_COMPLETE;
  _mark_frame_complete(isolate_frame->parent_isolate, isolate_frame,
                       /*should_push=*/false);
}

void _isolate_handle_error(Isolate *isolate, Frame *frame) {
  if (NULL == frame->parent_frame ||
      frame->parent_isolate != frame->parent_frame->parent_isolate) {
    if (NULL != frame->actor_future) {
      _mark_frame_complete(isolate, frame, /*should_push=*/true);
      return;
    }
    isolate->vm->exit_code = EXIT_FAILURE;
    Object *errorln = module_lookup(Module_io, global_intern("errorln"));
    ASSERT(errorln != NULL);
    ASSERT(Class_Function == errorln->_class);
    _call_function(frame, (Scope *)NULL, errorln->_function_obj);
    return;
  }
  _mark_frame_complete(frame->parent_isolate, frame,
                       /*should_push=*/true);
  *frame_mutable_acc(frame->parent_frame) = *frame_get_acc(frame);

  // Only remove parent from waiting set if it was only waiting on this task.
  if (FrameSet_size(&frame->parent_frame->dependent_frames) > 0) {
    return;
  }
  isolate_remove_waiting_frame(frame->parent_frame->parent_isolate,
                               frame->parent_frame);
  isolate_push_frame(frame->parent_frame->parent_isolate, frame->parent_frame);
}

void isolate_run(Isolate *isolate) {
  VM *vm = isolate->vm;
  Frame *frame;
top_of_fn:
  while (NULL != (frame = isolate_pop_frame(isolate))) {
    isolate->current_frame = frame;
    FrameState frame_state;
    SYNCHRONIZED(isolate->heap_access_lock,
                 { frame_state = vm_execute_frame(vm, frame); });
    // Release heap mutex
#ifdef DEBUG
    SYNCHRONIZED(vm->isolate_create_lock, {
      fprintf(stdout, "<-- ");
      value_print(frame_get_acc(frame), stdout);
      fprintf(stdout, "\n");
      DEBUGF("TaskState=%s p=%p t=%p c=%p", frame_state_str(frame_state),
             isolate, frame, frame->current);
    });
#endif
    switch (frame_state) {
      case FRAME_WAITING:
        isolate_insert_waiting_frame(isolate, frame);
        break;
      case FRAME_ERROR:
        _isolate_handle_error(isolate, frame);
        break;
      case FRAME_COMPLETE:
        _mark_frame_complete(isolate, frame, /*should_push=*/false);
        break;
      default:
        FATALF("Unknown TaskState = %d.", frame_state);
    }
    while (!VoidPtrArray_is_empty(&isolate->waiting_blocking_work)) {
      Work *w = (Work *)VoidPtrArray_pop_back_unchecked(
          &isolate->waiting_blocking_work);
      threadpool_execute_work(vm->blocking_pool, w);
    }
  }

  if (isolate_maybe_collect_garbage(isolate)) {
    FATALF("MEMORY LIMIT EXCEEDED");
    raise_error(isolate->current_frame, isolate->current_frame->current,
                "Out of memory: Max object count for process exceeded.");
  }

  if (_isolate_should_broadcast_to_parent(isolate)) {
    _isolate_broadcast_to_parent(isolate);
  }

  if (_isolate_is_done(isolate)) {
    DEBUGF("Process is complete.");
    return;
  }

  int waiting_frame_count;
  CRITICAL(isolate->frame_waiting_cs,
           { waiting_frame_count = FrameSet_size(&isolate->waiting_frames); });
  CRITICAL(isolate->frame_waiting_cs, {
    while (FrameSet_size(&isolate->waiting_frames) != 0 &&
           FrameSet_size(&isolate->waiting_frames) == waiting_frame_count &&
           isolate_queue_size(isolate) == 0) {
      condition_wait(isolate->frame_wait_cond);
    }
  });
  goto top_of_fn;
}

void *_isolate_run_return_void_ptr(void *ptr) {
  ASSERT(ptr != NULL);
  isolate_run((Isolate *)ptr);
  return NULL;
}

ThreadHandle isolate_run_in_new_thread(Isolate *isolate) {
  return isolate->thread =
             thread_create(AS_VOID_FN(_isolate_run_return_void_ptr), isolate);
}

bool isolate_maybe_collect_garbage(Isolate *isolate) {
  ASSERT(isolate != NULL);

  Heap *heap = isolate->heap;
  const uint32_t object_count_thresh =
      heap_object_count_threshold_for_garbage_collection(heap);
  uint32_t object_count = heap_object_count(heap);
  if (object_count < object_count_thresh) {
    return false;
  }
  // heap_print_debug_summary(heap);
  isolate_collect_garbage(isolate);
  object_count = heap_object_count(heap);
  const uint32_t max_object_count = heap_max_object_count(heap);

  if (object_count >= max_object_count) {
    heap_set_object_count_threshold_for_garbage_collection(heap,
                                                           max_object_count);
    return true;
  } else if (object_count >= object_count_thresh) {
    heap_set_object_count_threshold_for_garbage_collection(
        heap, (max_object_count + object_count) / 2);
  }
  return false;
}
