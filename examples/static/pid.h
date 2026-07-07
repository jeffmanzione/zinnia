#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_STATIC_PID_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_STATIC_PID_H_

#include "zinnia/native.h"

#ifdef OS_WINDOWS
__declspec(dllexport)
#endif
    void init_pid(NativeModuleBuilder *builder);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_EXAMPLES_STATIC_PID_H_ */