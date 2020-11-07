// optimize.h
//
// Created on: Jan 13, 2018
//     Author: Jeff

#ifndef PROGRAM_OPTIMIZATION_OPTIMIZE_H_
#define PROGRAM_OPTIMIZATION_OPTIMIZE_H_

#include "program/optimization/optimizer.h"
#include "program/tape.h"

void optimize_init();
void optimize_finalize();
Tape *optimize(Tape *const t);

void register_optimizer(const char name[], const Optimizer o);

void Int32_swap(void *x, void *y);
int Int32_compare(void *x, void *y);

#endif /* PROGRAM_OPTIMIZATION_OPTIMIZE_H_ */
