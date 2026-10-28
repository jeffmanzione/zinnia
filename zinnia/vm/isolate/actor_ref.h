// actor_ref.h
//
// Created on: Dec 26, 2022
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ACTOR_REF_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ACTOR_REF_H_

#include "zinnia/entity/entity.h"
#include "zinnia/vm/isolate/isolates.h"

typedef struct ActorRef_ ActorRef;

Object *create_actor_object(Heap *heap, Isolate *actor_isolate,
                            Object *actor_object);
void create_actor_on_object(Object *obj);

void actor_delete(ActorRef *actor);

Object *actor_get_object(ActorRef *actor);
Isolate *actor_get_isolate(ActorRef *actor);

ActorRef *extract_actor_from_obj(Object *actor_obj);
ActorRef *extract_actor(Entity *actor_entity);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_VM_ISOLATE_ACTOR_REF_H_ */
