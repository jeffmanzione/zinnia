#ifndef OBJECT_MODULE_MODULE_H_
#define OBJECT_MODULE_MODULE_H_

#include "entity/object.h"

Module *module_create(const char name[]);
void module_delete(Module *module);

Function *module_add_function(Module *module, const char name[]);
Class *module_add_class(Module *module, const char name[]);

#endif /* OBJECT_MODULE_MODULE_H_ */