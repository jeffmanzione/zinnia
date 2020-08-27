
#include <stdlib.h>

#include "alloc/alloc.h"
#include "debug/debug.h"
#include "lang/parser/parser.h"
#include "lang/semantics/semantics.h"
#include "vm/intern.h"
#include "vm/module_manager.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"
#include "vm/virtual_machine.h"

int main(int arc, char *args[]) {
  alloc_init();
  strings_init();
  parsers_init();
  semantics_init();

  VM *vm = vm_create();
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = modulemanager_read(mm, "test.jl");
  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, main_module->_reflection, main_module, 0);
  vm_run_process(vm, vm_main_process(vm));

  vm_delete(vm);

  semantics_finalize();
  parsers_finalize();
  strings_finalize();
  token_finalize_all();
  alloc_finalize();
  return EXIT_SUCCESS;
}