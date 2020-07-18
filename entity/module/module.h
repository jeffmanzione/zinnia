#ifndef OBJECT_MODULE_MODULE_H_
#define OBJECT_MODULE_MODULE_H_

#include "entity/object.h"
#include "program/tape.h"

void module_init(Module *module, const char name[], Tape *tape);
void module_finalize(Module *module);
const char *module_name(const Module *const module);

Function *module_add_function(Module *module, const char name[],
                              uint32_t ins_pos);
Class *module_add_class(Module *module, const char name[]);

#endif /* OBJECT_MODULE_MODULE_H_ */