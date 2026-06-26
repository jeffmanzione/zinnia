#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_

#include "zinnia/entity/object.h"
#include "zinnia/vm/module_manager.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void init_hello(ModuleManager *mm, Module *hello);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_ */