// error.h
//
// Created on: Aug 21, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_ERROR_ERROR_H_
#define ENTITY_ERROR_ERROR_H_

#include "entity/object.h"
#include "program/instruction.h"

void error_init(Error *e, const char msg[]);
void error_finalize(Error *e);
void error_add_stackline(Error *e, Module *m, Function *f,
                         const Instruction *ins);

#endif /* ENTITY_ERROR_ERROR_H_ */