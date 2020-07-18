// entity.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_OBJECT_H_
#define ENTITY_OBJECT_H_

#include "entity/object.h"
#include "entity/primitive.h"

// Contains a primitive, Object, or represents nullptr.
typedef struct _Entity Entity;

typedef enum { NONE, PRIMITIVE, OBJECT } EntityType;

struct _Entity {
  EntityType type;
  union {
    Primitive pri;
    Object *obj;
  };
};

// Gets the entity type from an entity.
EntityType etype(const Entity *e);
// Extracts a const Object from an entity.
const Object *object(const Entity *e);
// Extracts a mutable Object from an entity.
Object *object_m(Entity *e);

Entity entity_char(const int8_t c);
Entity entity_int(const int32_t i);
Entity entity_float(const double d);

Entity entity_primitive(const Primitive *p);

Entity entity_none();

#endif /* ENTITY_OBJECT_H_ */