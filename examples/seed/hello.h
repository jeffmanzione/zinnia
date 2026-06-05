#ifndef EXAMPLES_SEED_HELLO_H_
#define EXAMPLES_SEED_HELLO_H_


#include "entity/object.h"
#include "vm/module_manager.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
void init_hello(ModuleManager *mm, Module *hello);

#endif /* EXAMPLES_SEED_HELLO_H_ */