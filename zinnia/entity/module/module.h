#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_MODULE_MODULE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_MODULE_MODULE_H_

#include "c-data-structures/stable_maplike.h"
#include "zinnia/entity/object.h"
#include "zinnia/program/tape.h"

void module_init(Module *module, const char name[], const char full_path[],
                 const char relative_path[], const char key[], Tape *tape);
void module_finalize(Module *module);
const char *module_name(const Module *const module);
const Tape *module_tape(const Module *const module);

Function *module_add_function(Module *module, const char name[],
                              uint32_t ins_pos, bool is_const, bool is_asyc);
Class *module_add_class(Module *module, const char name[], const Class *super);
Object *module_lookup(Module *module, const char name[]);
const Class *module_lookup_class(const Module *module, const char name[]);

FunctionMapIterator module_functions(Module *module);
ClassMapIterator module_classes(Module *module);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_MODULE_MODULE_H_*/
