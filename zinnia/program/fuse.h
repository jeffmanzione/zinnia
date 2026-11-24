// fuse.h
//
// Instructions the VM runs together, as one.
#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_FUSE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_FUSE_H_

#include <stdint.h>

#include "zinnia/program/bytecode.h"

// Marks the instructions of [bytecode] from [start] on that the VM can run
// together with the ones after them (see Fuse), so it dispatches once for
// both. Every instruction stays where it is, so a jump to one that's fused to
// the one before it just runs it on its own, as before. Slots must already be
// assigned. Code before [start] is left alone, since it may be running.
void bytecode_fuse(Bytecode *bytecode, uint32_t start);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_FUSE_H_ */
