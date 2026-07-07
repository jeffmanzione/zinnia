#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_

#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/vm/module_manager.h"

// void NativeModuleBuilder_init(NativeModuleBuilder *builder, ModuleManager *,
//                               Module *);

void NativeModuleBuilder_verify_signature(NativeModuleBuilder *builder,
                                          NativeModuleBuilderInitFn init);
void NativeModuleBuilder_add_function(NativeModuleBuilder *builder,
                                      const char name[],
                                      NativeFunctionHandlerFn handler);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_ */