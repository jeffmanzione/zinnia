// virtual_machine.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/virtual_machine.h"

#include "entity/class/classes.h"
#include "heap/heap.h"
#include "struct/alist.h"
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
void _execute_LET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_SET(VM *vm, Task *task, Context *context, const Instruction *ins);
void _execute_CALL(VM *vm, Task *task, Context *context,
                   const Instruction *ins);
void _execute_RET(VM *vm, Task *task, Context *context, const Instruction *ins);
Context *_execute_NBLK(VM *vm, Task *task, Context *context,
                       const Instruction *ins);
Context *_execute_BBLK(VM *vm, Task *task, Context *context,
                       const Instruction *ins);

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

PRIMITIVE_OP(ADD, +, MATH_OP(ADD, +));
PRIMITIVE_OP(SUB, -, MATH_OP(SUB, -));
PRIMITIVE_OP(MULT, *, MATH_OP(MULT, *));
PRIMITIVE_OP(DIV, /, MATH_OP(DIV, /));
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

inline void _execute_RES(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  Entity *member;
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
    default:
      ERROR("Invalid arg type=%d for RES.", ins->type);
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
      ERROR("Invalid arg type=%d for RES.", ins->type);
  }
}

inline void _execute_LET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_ID:
      context_let(context, ins->id, task_get_resval(context->parent_task));
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

inline void _execute_CALL(VM *vm, Task *task, Context *context,
                          const Instruction *ins) {
  Entity fn;
  switch (ins->type) {
    case INSTRUCTION_ID:
      fn = *context_lookup(context, ins->id);
      break;
    case INSTRUCTION_NO_ARG:
      fn = task_popstack(task);
      break;
    default:
      ERROR("Invalid arg type=%d for CALL.", ins->type);
  }
  if (fn.type != OBJECT) {
    // TODO: This should be a recoverable error.
    ERROR("Invalid fn type=%d for CALL.", fn.type);
    return;
  }
  if (fn.obj->_class != Class_Function) {
    // TODO: This should be a recoverable error.
    ERROR("Invalid fn class type=%d for CALL.", fn.type);
    return;
  }
  Function *func = fn.obj->_function_obj;
  Task *new_task = process_create_task(task->parent_process);
  new_task->dependent_task = task;
  task_create_context(new_task, func->_module->_reflection,
                      (Module *)func->_module, func->_ins_pos);
  return;
}

inline void _execute_RET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_mutable_resval(context->parent_task) = *task_get_resval(task);
      break;
    case INSTRUCTION_ID:
      *task_mutable_resval(context->parent_task) =
          *context_lookup(context, ins->id);
      break;
    case INSTRUCTION_PRIMITIVE:
      *task_mutable_resval(context->parent_task) = entity_primitive(ins->val);
      break;
    default:
      ERROR("Invalid arg type=%d for RET.", ins->type);
  }
}

inline Context *_execute_NBLK(VM *vm, Task *task, Context *context,
                              const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      return task_create_context(context->parent_task, context->self,
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

// Please forgive me father, for I have sinned.
TaskState vm_execute_task(VM *vm, Task *task) {
  task->state = TASK_RUNNING;
  task->wait_reason = NOT_WAITING;
  Context *context =
      alist_get(&task->context_stack, alist_len(&task->context_stack) - 1);
  for (;;) {
    const Instruction *ins = context_ins(context);
    instruction_write(ins, stdout);
    fprintf(stdout, "\n");
    fflush(stdout);
    switch (ins->op) {
      case RES:
        _execute_RES(vm, task, context, ins);
        break;
      case PUSH:
        _execute_PUSH(vm, task, context, ins);
        break;
      case LET:
        _execute_LET(vm, task, context, ins);
        break;
      case SET:
        _execute_SET(vm, task, context, ins);
        break;
      case CALL:
      case CLLN:
        _execute_CALL(vm, task, context, ins);
        task->state = TASK_WAITING;
        task->wait_reason = WAITING_ON_FN_CALL;
        context->ins++;
        goto end_of_loop;
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
      case EXIT:
        task->state = TASK_COMPLETE;
        context->ins++;
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
    DEBUGF("TaskState=%s", task_state_str(task_state));
    switch (task_state) {
      case TASK_ERROR:
        ERROR("OH NO!");
        break;
      case TASK_WAITING:
        set_insert(&process->waiting_tasks, task);
        break;
      case TASK_COMPLETE:
        if (NULL != task->dependent_task) {
          Q_enqueue(&process->queued_tasks, task->dependent_task);
          set_remove(&process->waiting_tasks, task->dependent_task);
        }
        break;
      default:
        ERROR("Some unknown TaskState.");
    }
  }
}