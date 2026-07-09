#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_

#include "zinnia/entity/native/builder/function_context.h"
#include "zinnia/vm/module_manager.h"

// void ModuleBuilder_init(ModuleBuilder *builder, ModuleManager *,
//                               Module *);

void ModuleBuilder_verify_signature(ModuleBuilder *builder,
                                    ModuleBuilderInitFn init);
void ModuleBuilder_add_function(ModuleBuilder *builder, const char name[],
                                NativeFunctionHandlerFn handler);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_NATIVE_BUILDER_BUILDER_H_ */