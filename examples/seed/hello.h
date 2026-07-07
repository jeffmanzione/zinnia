#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_

#include "zinnia/native.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void init_hello(NativeModuleBuilder *builder);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_SEED_HELLO_H_ */