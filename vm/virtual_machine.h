#ifndef HEAP_VIRTUAL_MACHINE_H_
#define HEAP_VIRTUAL_MACHINE_H_

typedef struct __VM VM;

VM *vm_create();
void vm_delete(VM *vm);

bool vm_execute(VM *vm, Ins *ins);

#endif /* HEAP_VIRTUAL_MACHINE_H_ */