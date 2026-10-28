// optimize.h
//
// Created on: Jan 13, 2018
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZE_H_

#include "zinnia/program/bytecode.h"
#include "zinnia/program/optimization/optimizer.h"

void optimize_init();
void optimize_finalize();
Bytecode *optimize(Bytecode *const t);

void register_optimizer(const char name[], const Optimizer o);

void Int32_swap(void *x, void *y);
int Int32_compare(void *x, void *y);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZE_H_ */
