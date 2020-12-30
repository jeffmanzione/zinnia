// process.c
//
// Created on: Dec 28, 2020
//     Author: Jeff Manzione

#include "entity/native/process.h"

#include "entity/class/classes.h"
#include "vm/intern.h"


void _remote_init(Object *obj) {}
void _remote_delete(Object *obj) {}

void process_add_native(Module *process) {
  Class_Remote =
      native_class(process, REMOTE_CLASS_NAME, _remote_init, _remote_delete);
}
