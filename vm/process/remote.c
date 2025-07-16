// remote.c
//
// Created on: Dec 26, 2022
//     Author: Jeff Manzione

#include "vm/process/remote.h"

#include "entity/class/classes_def.h"

struct _Remote {
  Object *remote_object;
  Process *remote_process;
};

void __remote_init(Remote *remote, Process *remote_process,
                   Object *remote_object) {
  remote->remote_process = remote_process;
  remote->remote_object = remote_object;
}

Object *create_remote_object(Heap *heap, Process *remote_process,
                             Object *remote_object) {
  Object *object = heap_new(heap, Class_Remote);
  __remote_init((Remote *)object->_internal_obj, remote_process, remote_object);
  return object;
}

void create_remote_on_object(Object *obj) {
  Remote *remote = MNEW(Remote);
  remote->remote_process = NULL;
  remote->remote_object = NULL;
  obj->_internal_obj = remote;
}

void remote_delete(Remote *remote) { RELEASE(remote); }

Object *remote_get_object(Remote *remote) { return remote->remote_object; }

Process *remote_get_process(Remote *remote) { return remote->remote_process; }

Remote *extract_remote(Entity *remote_entity) {
  ASSERT(IS_CLASS(remote_entity, Class_Remote));
  return (Remote *)remote_entity->obj->_internal_obj;
}

Remote *extract_remote_from_obj(Object *remote_obj) {
  return (Remote *)remote_obj->_internal_obj;
}