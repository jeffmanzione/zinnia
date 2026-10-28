// actor_ref.c
//
// Created on: Dec 26, 2022
//     Author: Jeff Manzione

#include "zinnia/vm/isolate/actor_ref.h"

#include "zinnia/alloc/alloc.h"
#include "zinnia/entity/class/classes_def.h"

struct ActorRef_ {
  Object *actor_object;
  Isolate *actor_isolate;
};

void actor_init__(ActorRef *actor, Isolate *actor_isolate,
                  Object *actor_object) {
  actor->actor_isolate = actor_isolate;
  actor->actor_object = actor_object;
}

Object *create_actor_object(Heap *heap, Isolate *actor_isolate,
                            Object *actor_object) {
  Object *object = heap_new(heap, Class_ActorRef);
  actor_init__((ActorRef *)object->_internal_obj, actor_isolate, actor_object);
  return object;
}

void create_actor_on_object(Object *obj) {
  ActorRef *actor = MNEW(ActorRef);
  actor->actor_isolate = NULL;
  actor->actor_object = NULL;
  obj->_internal_obj = actor;
}

void actor_delete(ActorRef *actor) { RELEASE(actor); }

Object *actor_get_object(ActorRef *actor) { return actor->actor_object; }

Isolate *actor_get_isolate(ActorRef *actor) { return actor->actor_isolate; }

ActorRef *extract_actor(Entity *actor_entity) {
  ASSERT(IS_CLASS(actor_entity, Class_ActorRef));
  return (ActorRef *)actor_entity->obj->_internal_obj;
}

ActorRef *extract_actor_from_obj(Object *actor_obj) {
  return (ActorRef *)actor_obj->_internal_obj;
}