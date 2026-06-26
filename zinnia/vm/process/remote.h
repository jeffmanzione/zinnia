// remote.h
//
// Created on: Dec 26, 2022
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_REMOTE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_REMOTE_H_

#include "zinnia/entity/entity.h"
#include "zinnia/vm/process/processes.h"

typedef struct Remote_ Remote;

Object *create_remote_object(Heap *heap, Process *remote_process,
                             Object *remote_object);
void create_remote_on_object(Object *obj);

void remote_delete(Remote *remote);

Object *remote_get_object(Remote *remote);
Process *remote_get_process(Remote *remote);

Remote *extract_remote_from_obj(Object *remote_obj);
Remote *extract_remote(Entity *remote_entity);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_PROCESS_REMOTE_H_ */
