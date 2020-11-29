// virtual_machine.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/virtual_machine.h"

#include <stdarg.h>

#include "alloc/arena/intern.h"
#include "entity/array/array.h"
#include "entity/class/classes.h"
#include "entity/module/modules.h"
#include "entity/native/async.h"
#include "entity/native/builtin.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "struct/alist.h"
#include "vm/intern.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

struct _VM {
  ModuleManager mm;

  AList processes;
  Process *main;
};

bool _call_function_base(Task *task, Context *context, const Function *func,
                         Object *self, Context *parent_context);
void _execute_RES(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_PUSH(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_RNIL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_PNIL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_PEEK(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_DUP(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_FLD(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_LET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_SET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_GET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_GTSH(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
bool _execute_CALL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_RET(VM *vm, Task *task, Context *context, const Instruction *ins);
Context *_execute_NBLK(VM *vm, Task *task, Context *context,
                       const Instruction *ins);
Context *_execute_BBLK(VM *vm, Task *task, Context *context,
                       const Instruction *ins);
void _execute_EXIT(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_JMP(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_IF(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_NOT(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_ANEW(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
bool _execute_AIDX(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
bool _execute_ASET(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_TUPL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_IS(VM *vm, Task *task, Context *context, const Instruction *ins);
bool _execute_LMDL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
bool _execute_CTCH(VM *vm, Task *task, Context *context,
                   const Instruction *ins);

#define MATH_OP(op, symbol)                                                    \
  Primitive _execute_primitive_##op(const Primitive *p1,                       \
                                    const Primitive *p2) {                     \
    if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {                            \
      return primitive_float(float_of(p1) symbol float_of(p2));                \
    }                                                                          \
    if (INT == ptype(p1) || INT == ptype(p2)) {                                \
      return primitive_int(int_of(p1) symbol int_of(p2));                      \
    }                                                                          \
    return primitive_int(char_of(p1) symbol char_of(p2));                      \
  }

#define MATH_OP_INT(op, symbol)                                                \
  Primitive _execute_primitive_##op(const Primitive *p1,                       \
                                    const Primitive *p2) {                     \
    if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {                            \
      ERROR("Op not valid for FP types.");                                     \
    }                                                                          \
    if (INT == ptype(p1) || INT == ptype(p2)) {                                \
      return primitive_int(int_of(p1) symbol int_of(p2));                      \
    }                                                                          \
    return primitive_int(char_of(p1) symbol char_of(p2));                      \
  }

#define PRIMITIVE_OP(op, symbol, math_fn)                                      \
  math_fn;                                                                     \
  void _execute_##op(VM *vm, Task *task, Context *context,                     \
                     const Instruction *ins) {                                 \
    const Entity *resval, *lookup;                                             \
    Entity first, second, tmp;                                                 \
    switch (ins->type) {                                                       \
    case INSTRUCTION_NO_ARG:                                                   \
      second = task_popstack(task);                                            \
      if (PRIMITIVE != second.type) {                                          \
        raise_error(task, context, "RHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      first = task_popstack(task);                                             \
      if (PRIMITIVE != first.type) {                                           \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      *task_mutable_resval(task) =                                             \
          entity_primitive(_execute_primitive_##op(&first.pri, &second.pri));  \
      break;                                                                   \
    case INSTRUCTION_ID:                                                       \
      resval = task_get_resval(task);                                          \
      if (NULL != resval && PRIMITIVE != resval->type) {                       \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      lookup = context_lookup(context, ins->id, &tmp);                         \
      if (NULL != lookup && PRIMITIVE != lookup->type) {                       \
        raise_error(task, context, "RHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      *task_mutable_resval(task) = entity_primitive(                           \
          _execute_primitive_##op(&resval->pri, &lookup->pri));                \
      break;                                                                   \
    case INSTRUCTION_PRIMITIVE:                                                \
      resval = task_get_resval(task);                                          \
      if (NULL != resval && PRIMITIVE != resval->type) {                       \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      *task_mutable_resval(task) =                                             \
          entity_primitive(_execute_primitive_##op(&resval->pri, &ins->val));  \
      break;                                                                   \
    default:                                                                   \
      ERROR("Invalid arg type=%d for " #op ".", ins->type);                    \
    }                                                                          \
  }

#define PRIMITIVE_BOOL_OP(op, symbol, math_fn)                                 \
  math_fn;                                                                     \
  void _execute_##op(VM *vm, Task *task, Context *context,                     \
                     const Instruction *ins) {                                 \
    const Entity *resval, *lookup;                                             \
    Entity first, second, tmp;                                                 \
    Primitive result;                                                          \
    switch (ins->type) {                                                       \
    case INSTRUCTION_NO_ARG:                                                   \
      second = task_popstack(task);                                            \
      if (PRIMITIVE != second.type) {                                          \
        raise_error(task, context, "RHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      first = task_popstack(task);                                             \
      if (PRIMITIVE != first.type) {                                           \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      result = _execute_primitive_##op(&first.pri, &second.pri);               \
      *task_mutable_resval(task) =                                             \
          int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result);       \
      break;                                                                   \
    case INSTRUCTION_ID:                                                       \
      resval = task_get_resval(task);                                          \
      if (NULL != resval && PRIMITIVE != resval->type) {                       \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      lookup = context_lookup(context, ins->id, &tmp);                         \
      if (NULL != lookup && PRIMITIVE != lookup->type) {                       \
        raise_error(task, context, "RHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      result = _execute_primitive_##op(&resval->pri, &lookup->pri);            \
      *task_mutable_resval(task) =                                             \
          int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result);       \
      break;                                                                   \
    case INSTRUCTION_PRIMITIVE:                                                \
      resval = task_get_resval(task);                                          \
      if (NULL != resval && PRIMITIVE != resval->type) {                       \
        raise_error(task, context, "LHS for op '%s' must be primitive.", #op); \
        return;                                                                \
      }                                                                        \
      result = _execute_primitive_##op(&resval->pri, &ins->val);               \
      *task_mutable_resval(task) =                                             \
          int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result);       \
      break;                                                                   \
    default:                                                                   \
      ERROR("Invalid arg type=%d for " #op ".", ins->type);                    \
    }                                                                          \
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

Entity _module_filename(Task *task, Context *ctx, Object *obj, Entity *args) {
  const FileInfo *fi = modulemanager_get_fileinfo(
      vm_module_manager(task->parent_process->vm), obj->_module_obj);
  if (NULL == fi) {
    return NONE_ENTITY;
  }
  return entity_object(string_new(task->parent_process->heap,
                                  file_info_name(fi),
                                  strlen(file_info_name(fi))));
}

Entity _stackline_linetext(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  const FileInfo *fi = modulemanager_get_fileinfo(
      vm_module_manager(task->parent_process->vm), stackline_module(obj));
  const LineInfo *li = file_info_lookup(fi, stackline_linenum(obj) + 1);
  return (NULL == li)
             ? NONE_ENTITY
             : entity_object(string_new(task->parent_process->heap,
                                        li->line_text, strlen(li->line_text)));
}

void _add_filename_method(VM *vm) {
  Function *filename =
      native_method(Class_Module, intern("filename"), _module_filename);
  add_reflection_to_function(vm_main_process(vm)->heap,
                             Class_StackLine->_reflection, filename);
  Function *linename =
      native_method(Class_StackLine, intern("linetext"), _stackline_linetext);
  add_reflection_to_function(vm_main_process(vm)->heap,
                             Class_StackLine->_reflection, linename);
}

Process *_create_process_no_reflection(VM *vm) {
  Process *process = alist_add(&vm->processes);
  process_init(process);
  process->vm = vm;
  return process;
}

void _add_reflection_to_process(Process *process) {
  process->_reflection = heap_new(process->heap, Class_Process);
  process->_reflection->_internal_obj = process;
  heap_make_root(process->heap, process->_reflection);
}

VM *vm_create() {
  VM *vm = ALLOC2(VM);
  alist_init(&vm->processes, Process, DEFAULT_ARRAY_SZ);
  vm->main = _create_process_no_reflection(vm);
  modulemanager_init(&vm->mm, vm->main->heap);
  // Have to put this here since there was no way else to get around the
  // circular dependency.
  _add_filename_method(vm);
  _add_reflection_to_process(vm->main);
  return vm;
}

void vm_delete(VM *vm) {
  ASSERT(NOT_NULL(vm));
  AL_iter iter = alist_iter(&vm->processes);
  for (; al_has(&iter); al_inc(&iter)) {
    Process *proc = (Process *)al_value(&iter);
    process_finalize(proc);
  }
  alist_finalize(&vm->processes);
  modulemanager_finalize(&vm->mm);
  DEALLOC(vm);
}

Process *vm_create_process(VM *vm) {
  Process *process = _create_process_no_reflection(vm);
  _add_reflection_to_process(process);
  return process;
}

inline Process *vm_main_process(VM *vm) { return vm->main; }

bool _execute_EQ(VM *vm, Task *task, Context *context, const Instruction *ins) {
  const Entity *resval, *lookup;
  Entity first, second, tmp;
  bool result;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    second = task_popstack(task);
    first = task_popstack(task);
    if (OBJECT == first.type) {
      const Function *f = class_get_function(
          first.obj->_class, (EQ == ins->op) ? EQ_FN_NAME : NEQ_FN_NAME);
      if (NULL != f) {
        *task_mutable_resval(task) = second;
        return _call_function_base(task, context, f, first.obj, context);
      }
    }
    if (PRIMITIVE != second.type) {
      raise_error(task, context, "RHS for op 'EQ' must be primitive.");
      return false;
    }
    if (PRIMITIVE != first.type) {
      raise_error(task, context, "LHS for op 'EQ' must be primitive.");
      return false;
    }
    result = primitive_equals(&first.pri, &second.pri);
    *task_mutable_resval(task) =
        ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
            ? entity_int(1)
            : NONE_ENTITY;
    break;
  case INSTRUCTION_ID:
    resval = task_get_resval(task);
    if (NULL != resval && PRIMITIVE != resval->type) {
      raise_error(task, context, "LHS for op 'EQ' must be primitive.");
      return false;
    }
    lookup = context_lookup(context, ins->id, &tmp);
    if (NULL != lookup && PRIMITIVE != lookup->type) {
      raise_error(task, context, "RHS for op 'EQ' must be primitive.");
      return false;
    }
    result = primitive_equals(&first.pri, &lookup->pri);
    *task_mutable_resval(task) =
        ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
            ? entity_int(1)
            : NONE_ENTITY;
    break;
  case INSTRUCTION_PRIMITIVE:
    resval = task_get_resval(task);
    if (NULL != resval && PRIMITIVE != resval->type) {
      raise_error(task, context, "LHS for op 'EQ' must be primitive.");
      return false;
    }
    result = primitive_equals(&resval->pri, &ins->val);
    *task_mutable_resval(task) =
        ((result && (EQ == ins->op)) || (!result && (NEQ == ins->op)))
            ? entity_int(1)
            : NONE_ENTITY;
    break;
  default:
    ERROR("Invalid arg type=%d for EQ.", ins->type);
  }
  return false;
}

inline void _execute_RES(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  Entity *member;
  Entity tmp;
  Object *str;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    *task_mutable_resval(task) = task_popstack(task);
    break;
  case INSTRUCTION_ID:
    member = context_lookup(context, ins->id, &tmp);
    *task_mutable_resval(task) = (NULL == member) ? NONE_ENTITY : *member;
    break;
  case INSTRUCTION_PRIMITIVE:
    *task_mutable_resval(task) = entity_primitive(ins->val);
    break;
  case INSTRUCTION_STRING:
    str = heap_new(task->parent_process->heap, Class_String);
    // TODO: Maybe precompute the length of the string?
    __string_init(str, ins->str + 1, strlen(ins->str) - 2);
    *task_mutable_resval(task) = entity_object(str);
    break;
  default:
    ERROR("Invalid arg type=%d for RES.", ins->type);
  }
}

inline void _execute_PEEK(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  Entity *member;
  Entity tmp;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    *task_mutable_resval(task) = *task_peekstack(task);
    break;
  case INSTRUCTION_ID:
    member = context_lookup(context, ins->id, &tmp);
    *task_mutable_resval(task) = (NULL == member) ? NONE_ENTITY : *member;
    break;
  default:
    ERROR("Invalid arg type=%d for PEEK.", ins->type);
  }
}

inline void _execute_DUP(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Invalid arg type=%d for DUP.", ins->type);
  }
  Entity peek = *task_peekstack(task);
  *task_pushstack(task) = peek;
}

inline void _execute_PUSH(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  // DEBUGF("TEST");
  Object *str;
  Entity *member;
  Entity tmp;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    *task_pushstack(task) = *task_get_resval(task);
    break;
  case INSTRUCTION_ID:
    member = context_lookup(context, ins->id, &tmp);
    *task_pushstack(task) = (NULL == member) ? NONE_ENTITY : *member;
    break;
  case INSTRUCTION_PRIMITIVE:
    *task_pushstack(task) = entity_primitive(ins->val);
    break;
  case INSTRUCTION_STRING:
    str = heap_new(task->parent_process->heap, Class_String);
    // TODO: Maybe precompute the length of the string?
    __string_init(str, ins->str + 1, strlen(ins->str) - 2);
    *task_pushstack(task) = entity_object(str);
    break;
  default:
    ERROR("Invalid arg type=%d for PUSH.", ins->type);
  }
  // DEBUGF("TEST2");
}

inline void _execute_PNIL(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Invalid arg type=%d for PNIL.", ins->type);
  }
  *task_pushstack(task) = NONE_ENTITY;
}

inline void _execute_RNIL(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Invalid arg type=%d for RNIL.", ins->type);
  }
  *task_mutable_resval(task) = NONE_ENTITY;
}

inline void _execute_FLD(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    ERROR("Invalid arg type=%d for FLD.", ins->type);
  }
  const Entity *resval = task_get_resval(task);
  if (NULL == resval || OBJECT != resval->type) {
    raise_error(task, context,
                "Attempted to set field '%s' on something not an object.",
                ins->id);
    return;
  }
  Entity obj = task_popstack(task);
  object_set_member(task->parent_process->heap, resval->obj, ins->id, &obj);
}

inline void _execute_LET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
  case INSTRUCTION_ID:
    context_let(context, ins->id, task_get_resval(task));
    break;
  default:
    ERROR("Invalid arg type=%d for LET.", ins->type);
  }
}

inline void _execute_SET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
  case INSTRUCTION_ID:
    context_set(context, ins->id, task_get_resval(context->parent_task));
    break;
  default:
    ERROR("Invalid arg type=%d for SET.", ins->type);
  }
}

Entity _object_get_maybe_wrap(Object *obj, const char field[], Task *task,
                              Context *ctx) {
  Entity member;
  const Entity *member_ptr = object_get(obj, field);
  if (NULL == member_ptr) {
    const Function *f = class_get_function(obj->_class, field);
    if (NULL == f) {
      return NONE_ENTITY;
    }
    member = entity_object(f->_reflection);
  } else {
    member = *member_ptr;
  }
  if (OBJECT == member.type && Class_Function == member.obj->_class) {
    return entity_object(
        wrap_function_in_ref(member.obj->_function_obj, obj, task, ctx));
  }
  return member;
}

inline void _execute_GET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    ERROR("Invalid arg type=%d for GET.", ins->type);
  }
  const Entity *e = task_get_resval(task);
  if (NULL == e || OBJECT != e->type) {
    raise_error(task, context, "Attempted to get field '%s' from a %s.",
                ins->id, (e == NULL || NONE == e->type) ? "None" : "Primtive");
    return;
  }
  *task_mutable_resval(task) =
      _object_get_maybe_wrap(e->obj, ins->id, task, context);
}

inline void _execute_GTSH(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    ERROR("Invalid arg type=%d for GTSH.", ins->type);
  }
  const Entity *e = task_get_resval(task);
  if (NULL == e || OBJECT != e->type) {
    raise_error(task, context, "Attempted to get field '%s' from a %s.",
                ins->id, (e == NULL || NONE == e->type) ? "None" : "Primtive");
    return;
  }
  Entity get_result = _object_get_maybe_wrap(e->obj, ins->id, task, context);
  *task_pushstack(task) = get_result;
}

Context *_execute_as_new_task(Task *task, Object *self, Module *m,
                              uint32_t ins_pos) {
  Task *new_task = process_create_task(task->parent_process);
  new_task->dependent_task = task;
  Context *ctx = task_create_context(new_task, self, m, ins_pos);
  *task_mutable_resval(new_task) = *task_get_resval(task);
  return ctx;
}

// VERY IMPORTANT: context is only necessary for native functions!
bool _call_function_base(Task *task, Context *context, const Function *func,
                         Object *self, Context *parent_context) {
  if (func->_is_native) {
    NativeFn native_fn = (NativeFn)func->_native_fn;
    if (NULL == native_fn) {
      ERROR("Invalid native function.");
    }
    *task_mutable_resval(task) =
        native_fn(task, context, self, (Entity *)task_get_resval(task));
    return false;
  }
  Context *fn_ctx =
      _execute_as_new_task(task, self, (Module *)func->_module, func->_ins_pos);
  context_set_function(fn_ctx, func);
  if (func->_is_anon) {
    fn_ctx->previous_context = parent_context;
  }
  if (func->_is_async) {
    *task_mutable_resval(task) =
        entity_object(future_create(fn_ctx->parent_task));
    return false;
  }
  return true;
}

bool _call_method(Task *task, Object *obj, Context *context,
                  const Instruction *ins) {
  ASSERT(NOT_NULL(obj), NOT_NULL(ins), INSTRUCTION_ID == ins->type);
  const Class *class = (Class *)obj->_class;

  Entity method = _object_get_maybe_wrap(obj, ins->id, task, context);
  if (NONE == method.type) {
    raise_error(task, context, "Failed to find method '%s' on %s", ins->id,
                class->_name);
    return false;
  }
  if (OBJECT != method.type) {
    raise_error(task, context, "Attempted to treat '%s' on %s as a method.",
                ins->id, class->_name);
    return false;
  }
  if (Class_FunctionRef != method.obj->_class) {
    raise_error(task, context,
                "Attempted to treat '%s' of type '' on %s as a method.",
                ins->id, method.obj->_class->_name, class->_name);
    return false;
  }
  return _call_function_base(task, context, function_ref_get_func(method.obj),
                             function_ref_get_object(method.obj),
                             function_ref_get_parent_context(method.obj));
}

bool _call_function(Task *task, Context *context, Function *func) {
  return _call_function_base(task, context, func, func->_module->_reflection,
                             context);
}

inline bool _execute_CALL(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  Entity fn;
  if (INSTRUCTION_ID == ins->type) {
    if (CLLN == ins->op) {
      *task_mutable_resval(task) = NONE_ENTITY;
    }
    Entity obj = task_popstack(task);
    if (OBJECT != obj.type) {
      raise_error(task, context, "Calling function on non-object.");
      return false;
    }
    if (Class_Module == obj.obj->_class) {
      Module *m = obj.obj->_module_obj;
      ASSERT(NOT_NULL(m));
      Object *fn_obj = module_lookup(m, ins->id);
      if (NULL == fn_obj) {
        return _call_method(task, obj.obj, context, ins);
      }
      fn = entity_object(fn_obj);
    } else {
      return _call_method(task, obj.obj, context, ins);
    }
  } else {
    ASSERT(INSTRUCTION_NO_ARG == ins->type);
    fn = task_popstack(task);
  }
  if (fn.type != OBJECT) {
    raise_error(task, context,
                "Attempted to call something not a function (not an object).");
    return false;
  }
  if (fn.obj->_class == Class_Class) {
    Class *class = fn.obj->_class_obj;
    Object *obj = heap_new(task->parent_process->heap, class);
    const Function *constructor = class_get_function(class, CONSTRUCTOR_KEY);
    if (NULL == constructor) {
      *task_mutable_resval(task) = entity_object(obj);
      return false;
    } else {
      if (CLLN == ins->op) {
        *task_mutable_resval(task) = NONE_ENTITY;
      }
      return _call_function_base(task, context, constructor, obj, context);
    }
  }
  if (fn.obj->_class == Class_FunctionRef) {
    if (CLLN == ins->op) {
      *task_mutable_resval(task) = NONE_ENTITY;
    }
    return _call_function_base(task, context, function_ref_get_func(fn.obj),
                               function_ref_get_object(fn.obj),
                               function_ref_get_parent_context(fn.obj));
  }
  if (fn.obj->_class != Class_Function) {
    raise_error(task, context,
                "Attempted to call something not a function (class=%s).",
                fn.obj->_class->_name);
    return false;
  }
  Function *func = fn.obj->_function_obj;
  if (CLLN == ins->op) {
    *task_mutable_resval(task) = NONE_ENTITY;
  }
  return _call_function(task, context, func);
}

inline bool _execute_WAIT(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  const Entity *resval = task_get_resval(task);
  // Only wait for futures.
  if (NULL == resval || OBJECT != resval->type ||
      Class_Future != resval->obj->_class) {
    return false;
  }
  Future *future = (Future *)resval->obj->_internal_obj;
  if (TASK_COMPLETE != future->task->state &&
      TASK_ERROR != future->task->state) {
    return true;
  }
  *task_mutable_resval(task) = *task_get_resval(future->task);
  return false;
}

inline void _execute_RET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  Entity tmp;
  if (NULL == task->dependent_task) {
    return;
  }
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    *task_mutable_resval(task->dependent_task) = *task_get_resval(task);
    break;
  case INSTRUCTION_ID:
    *task_mutable_resval(task->dependent_task) =
        *context_lookup(context, ins->id, &tmp);
    break;
  case INSTRUCTION_PRIMITIVE:
    *task_mutable_resval(task->dependent_task) = entity_primitive(ins->val);
    break;
  default:
    ERROR("Invalid arg type=%d for RET.", ins->type);
  }
}

inline Context *_execute_NBLK(VM *vm, Task *task, Context *context,
                              const Instruction *ins) {
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    return task_create_context(context->parent_task, context->self.obj,
                               context->module, context->ins);
  default:
    ERROR("Invalid arg type=%d for NBLK.", ins->type);
  }
  return NULL;
}

inline Context *_execute_BBLK(VM *vm, Task *task, Context *context,
                              const Instruction *ins) {
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    return task_back_context(task);
  default:
    ERROR("Invalid arg type=%d for BBLK.", ins->type);
  }
  return NULL;
}

inline void _execute_JMP(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    ERROR("Invalid arg type=%d for JMP.", ins->type);
  }
  context->ins += ins->val._int_val;
}

inline void _execute_IF(VM *vm, Task *task, Context *context,
                        const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    ERROR("Invalid arg type=%d for IF.", ins->type);
  }
  const Entity *resval = task_get_resval(task);
  bool is_false = (NULL == resval) || (NONE == resval->type);
  if ((is_false && (IFN == ins->op)) || (!is_false && (IF == ins->op))) {
    context->ins += ins->val._int_val;
  }
}

inline void _execute_EXIT(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    ERROR("Invalid resval type.");
  }
  *task_mutable_resval(task) = entity_primitive(ins->val);
  task->state = TASK_COMPLETE;
  context->ins++;
}

inline void _execute_NOT(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Invalid arg type=%d for NOT.", ins->type);
  }
  const Entity *resval = task_get_resval(task);
  *task_mutable_resval(task) =
      (NULL == resval) || (NONE == resval->type) ? entity_int(1) : NONE_ENTITY;
}

void _execute_ANEW(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    ERROR("Invalid ANEW, ID type.");
  }
  Object *array_obj = heap_new(task->parent_process->heap, Class_Array);
  *task_mutable_resval(task) = entity_object(array_obj);
  if (INSTRUCTION_NO_ARG == ins->type) {
    return;
  }
  if (INSTRUCTION_PRIMITIVE != ins->type || INT != ptype(&ins->val)) {
    ERROR("Invalid ANEW requires int primitive.");
  }
  int32_t num_args = pint(&ins->val);
  int i;
  for (i = 0; i < num_args; ++i) {
    Entity e = task_popstack(task);
    array_add(task->parent_process->heap, array_obj, &e);
  }
}

bool _execute_AIDX(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  Object *arr_obj;
  const Entity *index = NULL;
  Entity index_e;

  Entity arr_entity = task_popstack(task);
  if (OBJECT != arr_entity.type) {
    raise_error(task, context, "Invalid array index on non-indexable.");
    return false;
  }
  arr_obj = arr_entity.obj;
  switch (ins->type) {
  case INSTRUCTION_NO_ARG:
    index = task_get_resval(task);
    break;
  case INSTRUCTION_ID:
    index = context_lookup(context, ins->id, &index_e);
    break;
  case INSTRUCTION_PRIMITIVE:
    if (INT != ptype(&ins->val) || pint(&ins->val) < 0) {
      raise_error(task, context, "Invalid array index.");
      return false;
    }
    index_e = entity_primitive(ins->val);
    index = &index_e;
    break;
  default:
    ERROR("Invalid arg type=%d for AIDX.", ins->type);
  }
  if (NULL == index) {
    raise_error(task, context, "Invalid array index.");
    return false;
  }
  if (Class_Array == arr_obj->_class) {
    Array *arr = (Array *)arr_obj->_internal_obj;
    if (PRIMITIVE != index->type || INT != ptype(&index->pri) ||
        pint(&index->pri) < 0) {
      raise_error(task, context, "Invalid array index.");
      return false;
    }
    int32_t i_index = pint(&index->pri);
    if (i_index >= Array_size(arr)) {
      raise_error(task, context, "Invalid array index.");
      return false;
    }
    *task_mutable_resval(task) = *Array_get_ref(arr, i_index);
    return false;
  }
  if (Class_Tuple == arr_obj->_class) {
    Tuple *tuple = (Tuple *)arr_obj->_internal_obj;
    if (PRIMITIVE != index->type || INT != ptype(&index->pri) ||
        pint(&index->pri) < 0) {
      raise_error(task, context, "Invalid tuple index.");
      return false;
    }
    int32_t i_index = pint(&index->pri);
    if (i_index >= tuple_size(tuple)) {
      raise_error(task, context, "Invalid tuple index.");
      return false;
    }
    *task_mutable_resval(task) = *tuple_get(tuple, i_index);
    return false;
  }

  const Function *aidx_fn =
      class_get_function(arr_obj->_class, ARRAYLIKE_INDEX_KEY);
  if (NULL != aidx_fn) {
    return _call_function_base(task, context, aidx_fn, arr_obj, context);
  }

  raise_error(task, context, "Invalid array index on non-indexable.");
  return false;
}

bool _execute_ASET(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  Entity arr_entity = task_popstack(task);
  Entity new_val = task_popstack(task);
  const Entity *index = task_get_resval(task);
  if (OBJECT != arr_entity.type) {
    raise_error(task, context, "Cannot set index value on non-indexable.");
    return false;
  }

  if (Class_Array == arr_entity.obj->_class) {
    if (NULL == index || PRIMITIVE != index->type ||
        INT != ptype(&index->pri)) {
      raise_error(task, context, "Cannot index with non-int.");
      return false;
    }
    array_set(task->parent_process->heap, arr_entity.obj, pint(&index->pri),
              &new_val);
    return false;
  }
  const Function *aset_fn =
      class_get_function(arr_entity.obj->_class, ARRAYLIKE_SET_KEY);
  if (NULL != aset_fn) {
    Object *args = heap_new(task->parent_process->heap, Class_Tuple);
    args->_internal_obj = tuple_create(2);
    Tuple *t = (Tuple *)args->_internal_obj;
    *tuple_get_mutable(t, 0) = *index;
    *tuple_get_mutable(t, 1) = new_val;
    *task_mutable_resval(task) = entity_object(args);
    return _call_function_base(task, context, aset_fn, arr_entity.obj, context);
  }
  raise_error(task, context, "Cannot set index value on non-indexable.");
  return false;
}

void _execute_TUPL(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    ERROR("Invalid TUPL, ID type.");
  }
  uint32_t num_args = pint(&ins->val);
  Object *tuple_obj = heap_new(task->parent_process->heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(num_args);
  *task_mutable_resval(task) = entity_object(tuple_obj);
  if (INSTRUCTION_NO_ARG == ins->type) {
    return;
  }
  if (INSTRUCTION_PRIMITIVE != ins->type || INT != ptype(&ins->val)) {
    ERROR("Invalid TUPL requires int primitive.");
  }
  int i;
  for (i = 0; i < num_args; ++i) {
    Entity e = task_popstack(task);
    tuple_set(task->parent_process->heap, tuple_obj, i, &e);
  }
}

void _execute_TLEN(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Invalid TLEN type.");
  }
  const Entity *e = task_peekstack(task);
  if (NULL == e || OBJECT != e->type || Class_Tuple != e->obj->_class) {
    *task_mutable_resval(task) = entity_int(-1);
    return;
  }
  *task_mutable_resval(task) =
      entity_int(tuple_size((Tuple *)e->obj->_internal_obj));
}

void _execute_TGTE(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type || INT != ptype(&ins->val)) {
    raise_error(task, context, "Invalid TLEN type.");
    return;
  }
  const Entity *e = task_get_resval(task);
  if (NULL == e || OBJECT != e->type || Class_Tuple != e->obj->_class) {
    *task_mutable_resval(task) = NONE_ENTITY;
    return;
  }
  Tuple *t = (Tuple *)e->obj->_internal_obj;
  uint32_t tlen = tuple_size(t);
  *task_mutable_resval(task) =
      tlen >= pint(&ins->val) ? entity_int(1) : NONE_ENTITY;
}

void _execute_TGET(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type || INT != ptype(&ins->val)) {
    raise_error(task, context, "Invalid TGET type.");
    return;
  }
  int32_t index = pint(&ins->val);
  const Entity *e = task_get_resval(task);
  if (NULL == e || OBJECT != e->type || Class_Tuple != e->obj->_class) {
    if (NULL != e && 0 == index) {
      return;
    }
    *task_mutable_resval(task) = entity_int(-1);
    raise_error(task, context, "Attempted to index something not a tuple.");
    return;
  }
  Tuple *t = (Tuple *)e->obj->_internal_obj;
  if (index < 0 || index >= tuple_size(t)) {
    raise_error(task, context,
                "Tuple index out of bounds. Index=%d, Tuple.len=%d.", index,
                tuple_size(t));
    return;
  }
  *task_mutable_resval(task) = *tuple_get(t, index);
}

bool _inherits_from(const Class *class, Class *possible_super) {
  for (;;) {
    if (NULL == class) {
      return false;
    }
    if (class == possible_super) {
      return true;
    }
    class = class->_super;
  }
}

void _execute_IS(VM *vm, Task *task, Context *context, const Instruction *ins) {
  if (INSTRUCTION_NO_ARG != ins->type) {
    ERROR("Weird type for IS.");
  }
  Entity rhs = task_popstack(task);
  Entity lhs = task_popstack(task);
  if (OBJECT != rhs.type || Class_Class != rhs.obj->_class) {
    raise_error(task, context,
                "Cannot perform type-check against a non-object type.");
    return;
  }
  if (lhs.type != OBJECT) {
    *task_mutable_resval(task) = NONE_ENTITY;
    return;
  }
  if (_inherits_from(lhs.obj->_class, rhs.obj->_class_obj)) {
    *task_mutable_resval(task) = entity_int(1);
  } else {
    *task_mutable_resval(task) = NONE_ENTITY;
  }
}

bool _execute_LMDL(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    ERROR("Weird type for LMDL.");
  }
  Module *module = modulemanager_lookup(&vm->mm, ins->id);
  if (NULL == module) {
    raise_error(task, context, "Module '%s' not found.", ins->id);
    return false;
  }
  object_set_member_obj(task->parent_process->heap,
                        context->module->_reflection, ins->id,
                        module->_reflection);
  if (module->_is_initialized) {
    return false;
  }
  module->_is_initialized = true;
  if (NULL == task_get_resval(task)) {
    *task_mutable_resval(task) = NONE_ENTITY;
  }
  _execute_as_new_task(task, module->_reflection, module, 0);
  return true;
}

bool _execute_CTCH(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_PRIMITIVE != ins->type) {
    ERROR("Invalid arg type=%d for CTCH.", ins->type);
  }
  context->catch_ins = context->ins + ins->val._int_val + 1;
  return true;
}

bool _attemp_catch_error(Task *task, Context *ctx) {
  // *task_mutable_resval(task) = entity_object(ctx->error);
  while (ctx->catch_ins < 0 && NULL != (ctx = task_back_context(task)))
    ;
  // There was no try/catch block.
  if (NULL == ctx) {
    task->state = TASK_ERROR;
    return false;
  }
  ctx->ins = ctx->catch_ins;
  ctx->error = NULL;
  ctx->catch_ins = -1;
  return true;
}

// Please forgive me father, for I have sinned.
TaskState vm_execute_task(VM *vm, Task *task) {
  task->state = TASK_RUNNING;
  task->wait_reason = NOT_WAITING;
  // This only happens when an error bubbles up to the main task
  if (NULL == task->current) {
    return TASK_COMPLETE;
  }
  Context *context = task->current;

  if (task->child_task_has_error) {
    const Entity *error_e = task_get_resval(task);
    ASSERT(NOT_NULL(error_e), OBJECT == error_e->type,
           Class_Error == error_e->obj->_class);
    context->error = error_e->obj;
    task->child_task_has_error = false;
  }
  for (;;) {
    if (NULL != context->error) {
      if (!_attemp_catch_error(task, context)) {
        goto end_of_loop;
      }
    }
    const Instruction *ins = context_ins(context);
#ifdef DEBUG
    instruction_write(ins, stdout);
    fprintf(stdout, "\n");
    fflush(stdout);
#endif
    switch (ins->op) {
    case RES:
      _execute_RES(vm, task, context, ins);
      break;
    case RNIL:
      _execute_RNIL(vm, task, context, ins);
      break;
    case PUSH:
      _execute_PUSH(vm, task, context, ins);
      break;
    case PNIL:
      _execute_PNIL(vm, task, context, ins);
      break;
    case PEEK:
      _execute_PEEK(vm, task, context, ins);
      break;
    case DUP:
      _execute_DUP(vm, task, context, ins);
      break;
    case FLD:
      _execute_FLD(vm, task, context, ins);
      break;
    case LET:
      _execute_LET(vm, task, context, ins);
      break;
    case SET:
      _execute_SET(vm, task, context, ins);
      break;
    case GET:
      _execute_GET(vm, task, context, ins);
      break;
    case GTSH:
      _execute_GTSH(vm, task, context, ins);
      break;
    case CALL:
    case CLLN:
      if (_execute_CALL(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
      }
      break;
    case RET:
      _execute_RET(vm, task, context, ins);
      task->state = TASK_COMPLETE;
      context->ins++;
      goto end_of_loop;
    case NBLK:
      context = _execute_NBLK(vm, task, context, ins);
      break;
    case BBLK:
      context = _execute_BBLK(vm, task, context, ins);
      break;
    case JMP:
      _execute_JMP(vm, task, context, ins);
      break;
    case IF:
    case IFN:
      _execute_IF(vm, task, context, ins);
      break;
    case EXIT:
      _execute_EXIT(vm, task, context, ins);
      goto end_of_loop;
    case ADD:
      _execute_ADD(vm, task, context, ins);
      break;
    case SUB:
      _execute_SUB(vm, task, context, ins);
      break;
    case MULT:
      _execute_MULT(vm, task, context, ins);
      break;
    case DIV:
      _execute_DIV(vm, task, context, ins);
      break;
    case MOD:
      _execute_MOD(vm, task, context, ins);
      break;
    case AND:
      _execute_AND(vm, task, context, ins);
      break;
    case OR:
      _execute_OR(vm, task, context, ins);
      break;
    case LT:
      _execute_LT(vm, task, context, ins);
      break;
    case GT:
      _execute_GT(vm, task, context, ins);
      break;
    case LTE:
      _execute_LTE(vm, task, context, ins);
      break;
    case GTE:
      _execute_GTE(vm, task, context, ins);
      break;
    case EQ:
    case NEQ:
      if (_execute_EQ(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
      }
      break;
    case IS:
      _execute_IS(vm, task, context, ins);
      break;
    case NOT:
      _execute_NOT(vm, task, context, ins);
      break;
    case ANEW:
      _execute_ANEW(vm, task, context, ins);
      break;
    case AIDX:
      if (_execute_AIDX(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
      }
      break;
    case ASET:
      if (_execute_ASET(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
      }
      break;
    case TUPL:
      _execute_TUPL(vm, task, context, ins);
      break;
    case TLEN:
      _execute_TLEN(vm, task, context, ins);
      break;
    case TGET:
      _execute_TGET(vm, task, context, ins);
      break;
    case TGTE:
      _execute_TGTE(vm, task, context, ins);
      break;
    case CTCH:
      _execute_CTCH(vm, task, context, ins);
      break;
    case LMDL:
      if (_execute_LMDL(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
      }
      break;
    case WAIT:
      if (_execute_WAIT(vm, task, context, ins)) {
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FUTURE;
        context->ins++;
        goto end_of_loop;
      }
      break;
    default:
      ERROR("Unknown instruction: %s", op_to_str(ins->op));
    }
    context->ins++;
  }
end_of_loop:
  return task->state;
}

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }

void vm_run_process(VM *vm, Process *process) {
  while (Q_size(&process->queued_tasks) > 0) {
    Task *task = Q_dequeue(&process->queued_tasks);
    process->current_task = task;
    TaskState task_state = vm_execute_task(vm, task);
#ifdef DEBUG
    fprintf(stdout, "<-- ");
    entity_print(task_get_resval(task), stdout);
    fprintf(stdout, "\n");
#endif
    DEBUGF("TaskState=%s", task_state_str(task_state));
    switch (task_state) {
    case TASK_WAITING:
      set_insert(&process->waiting_tasks, task);
      break;
    case TASK_ERROR:
      if (NULL == task->dependent_task) {
        Object *errorln = module_lookup(Module_io, intern("errorln"));
        ASSERT(NOT_NULL(errorln), Class_Function == errorln->_class);
        _call_function(task, (Context *)NULL, errorln->_function_obj);
      } else {
        task->dependent_task->child_task_has_error = true;
        *task_mutable_resval(task->dependent_task) = *task_get_resval(task);
        set_insert(&process->completed_tasks, task);
        Q_enqueue(&process->queued_tasks, task->dependent_task);
        set_remove(&process->waiting_tasks, task->dependent_task);
      }
      break;
    case TASK_COMPLETE:
      set_insert(&process->completed_tasks, task);
      // Only requeue parent task if it is waiting.
      if (NULL != task->dependent_task &&
          TASK_WAITING == task->dependent_task->state) {
        Q_enqueue(&process->queued_tasks, task->dependent_task);
        set_remove(&process->waiting_tasks, task->dependent_task);
      }
      break;
    default:
      ERROR("Some unknown TaskState.");
    }
    // getchar();
  }
}