// slots.h
//
// Variables kept in slots: numbered places in a call's frame, instead of
// entries looked up by name in a scope.

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SLOTS_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SLOTS_H_

#include "zinnia/program/bytecode.h"

// Gives each body of [bytecode] (each function, method, lambda and the
// module's own code, which each start with a SLOTS instruction) slots for the
// variables only it creates: those it assigns or takes as parameters, that no
// lambda inside it mentions and, for a lambda, that no body around it
// creates. Their operands get slot numbers and their SLOTS instruction lists
// their names.
void bytecode_assign_slots(Bytecode *bytecode);

// Restores the names of slot operands read as numbers, from their bodies'
// SLOTS instructions, and the slot counts of those instructions.
void bytecode_resolve_slot_names(Bytecode *bytecode);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_PROGRAM_SLOTS_H_ */
