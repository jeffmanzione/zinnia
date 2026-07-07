#include "zinnia/entity/native/builder/builder.h"

#include <stdarg.h>

#include "zinnia/entity/module/module.h"

void NativeModuleBuilder_verify_signature(NativeModuleBuilder *builder,
                                          NativeModuleBuilderInitFn init) {
  // This is just a compile-time check.
  builder->is_verified = true;
}

#define TOS_(x) (#x)

void NativeModuleBuilder_add_function(NativeModuleBuilder *builder,
                                      const char name[],
                                      NativeFunctionHandlerFn handler) {
  if (!builder->is_verified) {
    FATALF("%s must be verified before usage.",
           TOS_(NativeModuleBuilderInitFn));
  }
  Function *fn = module_add_function(builder->module, builder->mm->intern(name),
                                     0, false, false);
  fn->_is_native2 = true;
  fn->_native_fn2 = handler;
}
