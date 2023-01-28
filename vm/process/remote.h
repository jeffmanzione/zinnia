// remote.h
//
// Created on: Dec 26, 2022
//     Author: Jeff Manzione

#ifndef VM_PROCESS_REMOTE_H_
#define VM_PROCESS_REMOTE_H_

#include "entity/entity.h"
#include "vm/process/processes.h"

typedef struct _Remote Remote;

Object *create_remote_object(Heap *heap, Process *remote_process,
                             Object *remote_object);
void create_remote_on_object(Object *obj);

void remote_delete(Remote *remote);

Object *remote_get_object(Remote *remote);
Process *remote_get_process(Remote *remote);

Remote *extract_remote_from_obj(Object *remote_obj);
Remote *extract_remote(Entity *remote_entity);

#endif /* VM_PROCESS_REMOTE_H_ */
