
#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "lang/lexer/file_info.h"
#include "lang/lexer/lexer.h"
#include "lang/parser/parser.h"
#include "lang/semantics/semantics.h"
#include "program/instruction.h"
#include "program/op.h"
#include "program/tape.h"
#include "vm/intern.h"
#include "vm/module_manager.h"
#include "vm/process/context.h"
#include "vm/process/process.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"
#include "vm/virtual_machine.h"

int main(int arc, char *args[]) {
  alloc_init();
  intern_init();
  strings_init();
  parsers_init();
  semantics_init();

  // FILE *tmp = tmpfile();
  // char file[] =
  //     "module test\ndef test(x) {\n  y = x\n}\nclass Test {\n  new(field "
  //     "x) {}\n  method test(x) {\n    return x\n  }\n}\n";
  // printf("%s", file);
  // fprintf(tmp, "%s", file);
  // rewind(tmp);
  // FileInfo *fi = file_info_file(tmp);

  // SyntaxTree stree = parse_file(fi);

  // ExpressionTree *etree = populate_expression(&stree);
  // Tape *tape = tape_create();
  // produce_instructions(etree, tape);

  // tape_write(tape, stdout);

  VM *vm = vm_create();
  ModuleManager *mm = vm_module_manager(vm);
  Module *main_module = modulemanager_read(mm, "test.jl");
  Task *task = process_create_task(vm_main_process(vm));
  task_create_context(task, NULL, main_module, 0);
  vm_execute_task(vm, task);

  vm_delete(vm);

  semantics_finalize();
  parsers_finalize();
  strings_init();
  intern_finalize();
  alloc_finalize();

  return EXIT_SUCCESS;
}