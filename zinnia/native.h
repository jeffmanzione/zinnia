#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_

#include "zinnia/entity/native/builder/builder.h"
#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/util/platform.h"

#ifdef OS_WINDOWS
#define NATIVE_FN __declspec(dllexport)
#else
#define NATIVE_FN
#endif

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_NATIVE_H_ */