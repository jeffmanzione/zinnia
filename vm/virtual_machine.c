// virtual_machine.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/virtual_machine.h"

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
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      *task_mutable_resval(task) = task_popstack(task);
      break;
    case INSTRUCTION_ID:
      *task_mutable_resval(task) = context_lookup(context, ins->id);
      break;
    case INSTRUCTION_PRIMITIVE:
      *task_mutable_resval(task) = entity_primitive(&ins->val);
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

inline Context *_execute_CALL(VM *vm, Task *task, Context *context,
                              const Instruction *ins) {
  // switch (ins->type) {
  //   case INSTRUCTION_ID:
  //     Entity fn = context_lookup(context, ins->id);
  //     if (fn.type != OBJECT) {
  //       // TODO: This should be a recoverable error.
  //       ERROR("Invalid arg type=%d for CALL.", ins->type);
  //       return;
  //     }
  //     if (fn.type != OBJECT) {
  //       // TODO: This should be a recoverable error.
  //       ERROR("Invalid arg type=%d for CALL.", ins->type);
  //       return;
  //     }
  //   default:
  //     ERROR("Invalid arg type=%d for CALL.", ins->type);
  // }
  return NULL;
}

inline void _execute_RET(VM *vm, Task *task, Context *context,
                         const Instruction *ins) {
  switch (ins->type) {
    case INSTRUCTION_NO_ARG:
      break;
    case INSTRUCTION_ID:
      *task_mutable_resval(context->parent_task) =
          context_lookup(context, ins->id);
      break;
    case INSTRUCTION_PRIMITIVE:
      *task_mutable_resval(context->parent_task) = entity_primitive(&ins->val);
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
  Context *context =
      alist_get(&task->context_stack, alist_len(&task->context_stack) - 1);
  for (;;) {
    const Instruction *ins = context_ins(context);
    instruction_write(ins, stdout);
    fprintf(stdout, "\n");
    switch (ins->op) {
      case RES:
        _execute_RES(vm, task, context, ins);
        break;
      case LET:
        _execute_LET(vm, task, context, ins);
        break;
      case SET:
        _execute_SET(vm, task, context, ins);
        break;
      case CALL:
        _execute_CALL(vm, task, context, ins);
        goto jump_out_of_task_loop;
      case RET:
        _execute_RET(vm, task, context, ins);
        goto jump_out_of_task_loop;
      case NBLK:
        context = _execute_NBLK(vm, task, context, ins);
        break;
      case BBLK:
        context = _execute_BBLK(vm, task, context, ins);
      case EXIT:
        goto jump_out_of_task_loop;
    }
    context->ins++;
  }
jump_out_of_task_loop:
  return TASK_COMPLETE;
}

inline ModuleManager *vm_module_manager(VM *vm) { return &vm->mm; }