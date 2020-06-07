// semantics.h
//
// Created on: Dec 27, 2019
//     Author: Jeff Manzione

#ifndef LANG_SEMANTICS_SEMANTICS_H_
#define LANG_SEMANTICS_SEMANTICS_H_

#include "lang/parser/parser.h"
#include "program/tape.h"

typedef struct ExpressionTree_ ExpressionTree;

void semantics_init();
void semantics_finalize();

ExpressionTree *populate_expression(const SyntaxTree *tree);
int produce_instructions(ExpressionTree *tree, Tape *tape);
void delete_expression(ExpressionTree *tree);

#endif /* LANG_SEMANTICS_SEMANTICS_H_ */