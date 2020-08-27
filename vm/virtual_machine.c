// virtual_machine.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/virtual_machine.h"

#include "entity/array/array.h"
#include "entity/class/classes.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string.h"
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

void _execute_RES(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_PUSH(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_RNIL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_PNIL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_PEEK(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_FLD(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_LET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_SET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_GET(VM *vm, Task *task, Context *context, const Instruction *ins);
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
void _execute_AIDX(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_TUPL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_IS(VM *vm, Task *task, Context *context, const Instruction *ins);

bool _execute_primitive_EQ(const Primitive *p1, const Primitive *p2);

double _float_of(const Primitive *p) {
  switch (ptype(p)) {
    case INT:
      return (double)pint(p);
    case CHAR:
      return (double)pchar(p);
    default:
      return pfloat(p);
  }
}

int32_t _int_of(const Primitive *p) {
  switch (ptype(p)) {
    case INT:
      return pint(p);
    case CHAR:
      return (int32_t)pchar(p);
    default:
      return (int32_t)pfloat(p);
  }
}

int8_t _char_of(const Primitive *p) {
  switch (ptype(p)) {
    case INT:
      return (int8_t)pint(p);
    case CHAR:
      return (int8_t)pchar(p);
    default:
      return (int8_t)pfloat(p);
  }
}

#define MATH_OP(op, symbol)                                       \
  Primitive _execute_primitive_##op(const Primitive *p1,          \
                                    const Primitive *p2) {        \
    if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {               \
      return primitive_float(_float_of(p1) symbol _float_of(p2)); \
    }                                                             \
    if (INT == ptype(p1) || INT == ptype(p2)) {                   \
      return primitive_int(_int_of(p1) symbol _int_of(p2));       \
    }                                                             \
    return primitive_int(_char_of(p1) symbol _char_of(p2));       \
  }

#define MATH_OP_INT(op, symbol)                             \
  Primitive _execute_primitive_##op(const Primitive *p1,    \
                                    const Primitive *p2) {  \
    if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {         \
      ERROR("Op not valid for FP types.");                  \
    }                                                       \
    if (INT == ptype(p1) || INT == ptype(p2)) {             \
      return primitive_int(_int_of(p1) symbol _int_of(p2)); \
    }                                                       \
    return primitive_int(_char_of(p1) symbol _char_of(p2)); \
  }

#define PRIMITIVE_OP(op, symbol, math_fn)                         \
  math_fn;                                                        \
  void _execute_##op(VM *vm, Task *task, Context *context,        \
                     const Instruction *ins) {                    \
    const Entity *resval, *lookup;                                \
    Entity first, second;                                         \
    switch (ins->type) {                                          \
      case INSTRUCTION_NO_ARG:                                    \
        second = task_popstack(task);                             \
        if (PRIMITIVE != second.type) {                           \
          ERROR("RHS must be primitive.");                        \
        }                                                         \
        first = task_popstack(task);                              \
        if (PRIMITIVE != first.type) {                            \
          ERROR("LHS must be primitive.");                        \
        }                                                         \
        *task_mutable_resval(task) = entity_primitive(            \
            _execute_primitive_##op(&first.pri, &second.pri));    \
        break;                                                    \
      case INSTRUCTION_ID:                                        \
        resval = task_get_resval(task);                           \
        if (NULL != resval && PRIMITIVE != resval->type) {        \
          ERROR("LHS must be primitive.");                        \
        }                                                         \
        lookup = context_lookup(context, ins->id);                \
        if (NULL != lookup && PRIMITIVE != lookup->type) {        \
          ERROR("RHS must be primitive.");                        \
        }                                                         \
        *task_mutable_resval(task) = entity_primitive(            \
            _execute_primitive_##op(&resval->pri, &lookup->pri)); \
        break;                                                    \
      case INSTRUCTION_PRIMITIVE:                                 \
        resval = task_get_resval(task);                           \
        if (NULL != resval && PRIMITIVE != resval->type) {        \
          ERROR("LHS must be primitive.");                        \
        }                                                         \
        *task_mutable_resval(task) = entity_primitive(            \
            _execute_primitive_##op(&resval->pri, &ins->val));    \
      default:                                                    \
        ERROR("Invalid arg type=%d for RES.", ins->type);         \
    }                                                             \
  }

#define PRIMITIVE_BOOL_OP(op, symbol, math_fn)                              \
  math_fn;                                                                  \
  void _execute_##op(VM *vm, Task *task, Context *context,                  \
                     const Instruction *ins) {                              \
    const Entity *resval, *lookup;                                          \
    Entity first, second;                                                   \
    Primitive result;                                                       \
    switch (ins->type) {                                                    \
      case INSTRUCTION_NO_ARG:                                              \
        second = task_popstack(task);                                       \
        if (PRIMITIVE != second.type) {                                     \
          ERROR("RHS must be primitive.");                                  \
        }                                                                   \
        first = task_popstack(task);                                        \
        if (PRIMITIVE != first.type) {                                      \
          ERROR("LHS must be primitive.");                                  \
        }                                                                   \
        result = _execute_primitive_##op(&first.pri, &second.pri);          \
        *task_mutable_resval(task) =                                        \
            _int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result); \
        break;                                                              \
      case INSTRUCTION_ID:                                                  \
        resval = task_get_resval(task);                                     \
        if (NULL != resval && PRIMITIVE != resval->type) {                  \
          ERROR("LHS must be primitive.");                                  \
        }                                                                   \
        lookup = context_lookup(context, ins->id);                          \
        if (NULL != lookup && PRIMITIVE != lookup->type) {                  \
          ERROR("RHS must be primitive.");                                  \
        }                                                                   \
        result = _execute_primitive_##op(&resval->pri, &lookup->pri);       \
        *task_mutable_resval(task) =                                        \
            _int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result); \
        break;                                                              \
      case INSTRUCTION_PRIMITIVE:                                           \
        resval = task_get_resval(task);                                     \
        if (NULL != resval && PRIMITIVE != resval->type) {                  \
          ERROR("LHS must be primitive.");                                  \
        }                                                                   \
        result = _execute_primitive_##op(&resval->pri, &ins->val);          \
        *task_mutable_resval(task) =                                        \
            _int_of(&result) == 0 ? NONE_ENTITY : entity_primitive(result); \
      default:                                                              \
        ERROR("Invalid arg type=%d for RES.", ins->type);                   \
    }                                                                       \
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

VM *vm_create() {
  VM *vm = ALLOC2(VM);
  alist_init(&vm->processes, Process, DEFAULT_ARRAY_SZ);
  vm->main = vm_create_process(vm);
  modulemanager_init(&vm->mm, vm->main->heap);
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
  Process *process = alist_add(&vm->processes);
  process_init(process);
  return process;
}

inline Process *vm_main_process(VM *vm) { return vm->main; }

inline bool _execute_primitive_EQ(const Primitive *p1, const Primitive *p2) {
  if (FLOAT == ptype(p1) || FLOAT == ptype(p2)) {
    return _float_of(p1) == _float_of(p2);
  }
  if (INT == ptype(p1) || INT == ptype(p2)) {
    return _int_of(p1) == _int_of(p2);
  }
  return _char_of(p1) == _char_of(p2);
}

void _execute_EQ(VM *vm, Task *task, Context *context, const Instruction *ins) {
  const Entity *resval, *lookup;
  Entity first, second;
  bool result;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      second = task_popstack(task);
      if (PRIMITIVE != second.type) {
        ERROR("RHS must be primitive.");
      }
      first = task_popstack(task);
      if (PRIMITIVE != first.type) {
        ERROR("LHS must be primitive.");
      }
      result = _execute_primitive_EQ(&first.pri, &second.pri);
      *task_mutable_resval(task) =
          (((result == 0) && (EQ == ins->op)) ||
           ((result != 0) && (NEQ == ins->op)))
              ? NONE_ENTITY
              : (result == 0) ? entity_int(1) : entity_int(result);
      break;
    case INSTRUCTION_ID:
      resval = task_get_resval(task);
      if (NULL != resval && PRIMITIVE != resval->type) {
        ERROR("LHS must be primitive.");
      }
      lookup = context_lookup(context, ins->id);
      if (NULL != lookup && PRIMITIVE != lookup->type) {
        ERROR("RHS must be primitive.");
      }
      result = _execute_primitive_EQ(&first.pri, &lookup->pri);
      *task_mutable_resval(task) =
          (((result == 0) && (EQ == ins->op)) ||
           ((result != 0) && (NEQ == ins->op)))
              ? NONE_ENTITY
              : (result == 0) ? entity_int(1) : entity_int(result);
      break;
    case INSTRUCTION_PRIMITIVE:
      resval = task_get_resval(task);
      if (NULL != resval && PRIMITIVE != resval->type) {
        ERROR("LHS must be primitive.");
      }
      result = _execute_primitive_EQ(&resval->pri, &ins->val);
      *task_mutable_resval(task) =
          (((result == 0) && (EQ == ins->op)) ||
           ((result != 0) && (NEQ == ins->op)))
              ? NONE_ENTITY
              : (result == 0) ? entity_int(1) : entity_int(result);
    default:
      ERROR("Invalid arg type=%d for RES.", ins->type);
  }
}

inline void _execute_RES(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  Entity *member;
  Object *str;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_mutable_resval(task) = task_popstack(task);
      break;
    case INSTRUCTION_ID:
      member = context_lookup(context, ins->id);
      *task_mutable_resval(task) = (NULL == member) ? NONE_ENTITY : *member;
      break;
    case INSTRUCTION_PRIMITIVE:
      *task_mutable_resval(task) = entity_primitive(ins->val);
      break;
    case INSTRUCTION_STRING:
      str = heap_new(task->parent_process->heap, Class_String);
      // TODO: Maybe precomute the length of the string?
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
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_mutable_resval(task) = *task_peekstack(task);
      break;
    case INSTRUCTION_ID:
      member = context_lookup(context, ins->id);
      *task_mutable_resval(task) = (NULL == member) ? NONE_ENTITY : *member;
      break;
    default:
      ERROR("Invalid arg type=%d for PEEK.", ins->type);
  }
}

inline void _execute_PUSH(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  Entity *member;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_pushstack(task) = *task_get_resval(task);
      break;
    case INSTRUCTION_ID:
      member = context_lookup(context, ins->id);
      *task_pushstack(task) = (NULL == member) ? NONE_ENTITY : *member;
      break;
    case INSTRUCTION_PRIMITIVE:
      *task_pushstack(task) = entity_primitive(ins->val);
      break;
    default:
      ERROR("Invalid arg type=%d for PUSH.", ins->type);
  }
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
    ERROR("Attempted to get '%s' on non-object.", ins->id);
  }
  Entity obj = task_popstack(task);
  object_set_member(task->parent_process->heap, resval->obj, ins->id, &obj);
}

inline void _execute_LET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_ID:
      context_let(context, ins->id, task_get_resval(context->parent_task));
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

inline void _execute_GET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (INSTRUCTION_ID != ins->type) {
    ERROR("Invalid arg type=%d for GET.", ins->type);
  }
  const Entity *e = task_get_resval(task);
  if (NULL == e || OBJECT != e->type) {
    ERROR("Attempting to field a non-object.");
  }
  const Entity *member = object_get(e->obj, ins->id);
  if (NULL == member) {
    const Function *f = class_get_function(e->obj->_class, ins->id);
    if (NULL != f) {
      Object *fn_ref = heap_new(task->parent_process->heap, Class_FunctionRef);
      __function_ref_init(fn_ref, e->obj, f);
      member = object_set_member_obj(task->parent_process->heap, e->obj,
                                     ins->id, fn_ref);
    }
  }
  *task_mutable_resval(task) = (NULL == member) ? NONE_ENTITY : *member;
}

bool _call_function_base(Task *task, Context *context, const Function *func,
                         Object *self) {
  if (func->_is_native) {
    NativeFn native_fn = (NativeFn)func->_native_fn;
    if (NULL == native_fn) {
      ERROR("Invalid native function.");
    }
    *task_mutable_resval(task) =
        native_fn(task, context, self, (Entity *)task_get_resval(task));
    return false;
  }
  Task *new_task = process_create_task(task->parent_process);
  new_task->dependent_task = task;
  task_create_context(new_task, self, (Module *)func->_module, func->_ins_pos);
  *task_mutable_resval(new_task) = *task_get_resval(task);
  return true;
}

bool _call_method(Task *task, Context *context, const Instruction *ins) {
  ASSERT(NOT_NULL(ins), INSTRUCTION_ID == ins->type);
  Entity obj = task_popstack(task);
  ASSERT(OBJECT == obj.type);
  const Class *class = (Class *)obj.obj->_class;
  const Function *fn;
  int i;
  for (i = 0; i < 10; ++i) {
    fn = class_get_function(class, ins->id);
    if (NULL != fn) {
      break;
    }
    if (NULL == class->_super) {
      ERROR("Method does not exist.");
      return true;
    }
    class = class->_super;
  }
  return _call_function_base(task, context, fn, obj.obj);
}

bool _call_function(Task *task, Context *context, Function *func) {
  return _call_function_base(task, context, func, func->_module->_reflection);
}

inline bool _execute_CALL(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    if (CLLN == ins->op) {
      *task_mutable_resval(task) = NONE_ENTITY;
    }
    return _call_method(task, context, ins);
  }
  ASSERT(INSTRUCTION_NO_ARG == ins->type);
  Entity fn = task_popstack(task);
  if (fn.type != OBJECT) {
    // TODO: This should be a recoverable error.
    ERROR("Invalid fn type=%d for CALL.", fn.type);
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
      return _call_function_base(task, context, constructor, obj);
    }
  }
  if (fn.obj->_class == Class_FunctionRef) {
    if (CLLN == ins->op) {
      *task_mutable_resval(task) = NONE_ENTITY;
    }
    return _call_function_base(task, context, function_ref_get_func(fn.obj),
                               function_ref_get_object(fn.obj));
  }
  if (fn.obj->_class != Class_Function) {
    // TODO: This should be a recoverable error.
    ERROR("Invalid fn class type=%d for CALL.", fn.type);
  }
  Function *func = fn.obj->_function_obj;
  if (CLLN == ins->op) {
    *task_mutable_resval(task) = NONE_ENTITY;
  }
  return _call_function(task, context, func);
}

inline void _execute_RET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  if (NULL == task->dependent_task) {
    return;
  }
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_mutable_resval(task->dependent_task) = *task_get_resval(task);
      break;
    case INSTRUCTION_ID:
      *task_mutable_resval(task->dependent_task) =
          *context_lookup(context, ins->id);
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
    ERROR("Invalid arg type=%d for IF.", ins->type);
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

void _execute_AIDX(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  Object *arr_obj;
  const Entity *tmp;
  uint32_t index;

  Entity arr_entity = task_popstack(task);
  if (OBJECT != arr_entity.type) {
    ERROR("Invalid array index on non-indexable.");
  }
  arr_obj = arr_entity.obj;
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      tmp = task_get_resval(task);
      if (NULL == tmp || PRIMITIVE != tmp->type || INT != ptype(&tmp->pri) ||
          pint(&tmp->pri) < 0) {
        ERROR("Invalid array index.");
      }
      index = pint(&tmp->pri);
      break;
    case INSTRUCTION_ID:
      tmp = context_lookup(context, ins->id);
      if (NULL == tmp || PRIMITIVE != tmp->type || INT != ptype(&tmp->pri) ||
          pint(&tmp->pri) < 0) {
        ERROR("Invalid array index.");
      }
      break;
    case INSTRUCTION_PRIMITIVE:
      if (INT != ptype(&ins->val) || pint(&ins->val) < 0) {
        ERROR("Invalid array index.");
      }
      index = pint(&ins->val);
      break;
    default:
      ERROR("Invalid arg type=%d for AIDX.", ins->type);
  }
  if (Class_Array == arr_obj->_class) {
    Array *arr = (Array *)arr_obj->_internal_obj;
    if (index >= Array_size(arr)) {
      ERROR("Invalid array index.");
    }
    *task_mutable_resval(task) = *Array_get_ref(arr, index);
    return;
  }
  if (Class_Tuple == arr_obj->_class) {
    Tuple *tuple = (Tuple *)arr_obj->_internal_obj;
    if (index >= tuple_size(tuple)) {
      ERROR("Invalid array index.");
    }
    *task_mutable_resval(task) = *tuple_get(tuple, index);
    return;
  }
  ERROR("Invalid array index on non-indexable.");
}

void _execute_TUPL(VM *vm, Task *task, Context *context,
                   const Instruction *ins) {
  if (INSTRUCTION_ID == ins->type) {
    ERROR("Invalid TUPL, ID type.");
  }
  int32_t num_args = pint(&ins->val);
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
    ERROR("Cannot perform type-check against a non-object type.");
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

// Please forgive me father, for I have sinned.
TaskState vm_execute_task(VM *vm, Task *task) {
  task->state = TASK_RUNNING;
  task->wait_reason = NOT_WAITING;
  Context *context =
      alist_get(&task->context_stack, alist_len(&task->context_stack) - 1);
  for (;;) {
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
        _execute_EQ(vm, task, context, ins);
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
        _execute_AIDX(vm, task, context, ins);
        break;
      case TUPL:
        _execute_TUPL(vm, task, context, ins);
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
    TaskState task_state = vm_execute_task(vm, task);
#ifdef DEBUG
    fprintf(stdout, "<-- ");
    entity_print(task_get_resval(task), stdout);
    fprintf(stdout, "\n");
#endif
    DEBUGF("TaskState=%s", task_state_str(task_state));
    switch (task_state) {
      case TASK_ERROR:
        ERROR("OH NO!");
        break;
      case TASK_WAITING:
        set_insert(&process->waiting_tasks, task);
        break;
      case TASK_COMPLETE:
        set_insert(&process->completed_tasks, task);
        if (NULL != task->dependent_task) {
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