// optimizers.h
//
// Created on: Mar 3, 2018
//     Author: Jeff

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZERS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZERS_H_

#include "zinnia/program/optimization/optimizer.h"
#include "zinnia/program/tape.h"

void optimizer_ResPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_SetRes(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end);
void optimizer_SetPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_GetPush(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_JmpRes(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end);
void optimizer_PushRes(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_ResPush2(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end);
void optimizer_RetRet(OptimizeHelper *oh, const Tape *const tape, int start,
                      int end);
void optimizer_PeekRes(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_PeekPush(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end);
void optimizer_Increment(OptimizeHelper *oh, const Tape *const tape, int start,
                         int end);
void optimizer_SetEmpty(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end);
void optimizer_PushResEmpty(OptimizeHelper *oh, const Tape *const tape,
                            int start, int end);
void optimizer_PeekPeek(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end);
void optimizer_PushRes2(OptimizeHelper *oh, const Tape *const tape, int start,
                        int end);
void optimizer_SimpleMath(OptimizeHelper *oh, const Tape *const tape, int start,
                          int end);
void optimizer_Nil(OptimizeHelper *oh, const Tape *const tape, int start,
                   int end);
void optimizer_ResAidx(OptimizeHelper *oh, const Tape *const tape, int start,
                       int end);
void optimizer_StringConcat(OptimizeHelper *oh, const Tape *const tape,
                            int start, int end);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_OPTIMIZATION_OPTIMIZERS_H_ */
