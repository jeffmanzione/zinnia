// error.c
//
// Created on: Aug 22, 2020
//     Author: Jeff Manzione

#include "entity/error/error.h"

#include "entity/object.h"
#include "struct/alist.h"

void error_init(Error *e, const char msg[]) {
  e->msg = msg;
  alist_init(&e->stacktrace, StackLine, DEFAULT_ARRAY_SZ);
}

void error_finalize(Error *e) { alist_finalize(&e->stacktrace); }

void error_add_stackline(Error *e, Module *m, Function *f,
                         const Instruction *ins) {
  StackLine *sl = (StackLine *)alist_add(&e->stacktrace);
  sl->module = m;
  sl->func = f;
  const SourceMapping *sm = tape_get_source(m->_tape, f->_ins_pos);
  if (NULL != sm) {
    sl->token = sm->token;
  } else {
    sl->token = NULL;
  }
}