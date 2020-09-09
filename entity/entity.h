// entity.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_ENTITY_H_
#define ENTITY_ENTITY_H_

#include <stdio.h>

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

extern Entity NONE_ENTITY;

// Gets the entity type from an entity.
EntityType etype(const Entity *e);
// Extracts a const Object from an entity.
const Object *object(const Entity *e);
// Extracts a mutable Object from an entity.
Object *object_m(Entity *e);

Entity entity_char(const int8_t c);
Entity entity_int(const int32_t i);
Entity entity_float(const double d);

Entity entity_primitive_ptr(const Primitive *p);
Entity entity_primitive(Primitive p);

void entity_print(const Entity *e, FILE *file);

Entity entity_none();

Entity *object_get(Object *obj, const char field[]);

Entity entity_object(Object *obj);

#endif /* ENTITY_ENTITY_H_ */