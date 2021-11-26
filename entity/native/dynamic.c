// dynamic.c
//
// Created on: Nov 12, 2021
//     Author: Jeff Manzione

#include "entity/native/dynamic.h"

#include "entity/entity.h"
#include "entity/native/native.h"
#include "vm/module_manager.h"
#include "vm/process/processes.h"

Entity _open_c_lib(Task *task, Context *ctx, Object *obj, Entity *args) {}

void dynamic_add_native(ModuleManager *mm, Module *dynamic) {
  native_function(dynamic, intern("__open_c_lib"), _open_c_lib);
}