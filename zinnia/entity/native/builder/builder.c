#include "zinnia/entity/native/builder/builder.h"

#include <stdarg.h>

#include "zinnia/entity/module/module.h"

// Compile-time check that the function matches the expected signature for
// module initialization.
void ModuleBuilder_verify_signature(ModuleBuilder *builder,
                                    ModuleBuilderInitFn init) {
  builder->is_verified = true;
}

#define TOS_(x) (#x)

void ModuleBuilder_add_function(ModuleBuilder *builder, const char name[],
                                NativeFunctionHandlerFn handler) {
  if (!builder->is_verified) {
    FATALF("%s must be verified before usage.", TOS_(ModuleBuilderInitFn));
  }
  Function *fn = module_add_function(builder->module, builder->mm->intern(name),
                                     0, false, false);
  fn->_is_native2 = true;
  fn->_native_fn2 = handler;
}
