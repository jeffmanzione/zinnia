// entity.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef ENTITY_ENTITY_H_
#define ENTITY_ENTITY_H_

#include <stdio.h>

#include "entity/object.h"
#include "entity/primitive.h"

#define IS_CLASS(e, class)                                                     \
  (NULL != (e) && OBJECT == (e)->type && ((class) == (e)->obj->_class))
#define IS_NONE(e) ((NULL == (e)) || (NONE == (e)->type))
#define IS_FALSE(e) (IS_NONE(e) || (IS_BOOL(e) && false == pbool(&(e)->pri)))
#define IS_TRUE(e) (!IS_FALSE(e))
#define IS_OBJECT(e) ((NULL != (e)) && (OBJECT == (e)->type))
#define IS_PRIMITIVE(e) ((NULL != (e)) && (PRIMITIVE == (e)->type))
#define IS_BOOL(e) (IS_PRIMITIVE(e) && (PRIMITIVE_BOOL == ptype(&(e)->pri)))
#define IS_CHAR(e) (IS_PRIMITIVE(e) && (PRIMITIVE_CHAR == ptype(&(e)->pri)))
#define IS_INT(e) (IS_PRIMITIVE(e) && (PRIMITIVE_INT == ptype(&(e)->pri)))
#define IS_FLOAT(e) (IS_PRIMITIVE(e) && (PRIMITIVE_FLOAT == ptype(&(e)->pri)))
#define IS_TUPLE(e)                                                            \
  ((NULL != e) && (OBJECT == e->type) && (Class_Tuple == e->obj->_class))
#define IS_ARRAY(e)                                                            \
  ((NULL != e) && (OBJECT == e->type) && (Class_Array == e->obj->_class))

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
extern Entity TRUE_ENTITY;
extern Entity FALSE_ENTITY;

// Gets the entity type from an entity.
EntityType etype(const Entity *e);
// Extracts a const Object from an entity.
const Object *object(const Entity *e);
// Extracts a mutable Object from an entity.
Object *object_m(Entity *e);

Entity entity_char(const int8_t c);
Entity entity_int(const int64_t i);
Entity entity_float(const double d);

Entity entity_primitive_ptr(const Primitive *p);
Entity entity_primitive(Primitive p);

void entity_print(const Entity *e, FILE *file);

Entity entity_none();

Entity *object_get(Object *obj, const char field[]);

Entity entity_object(Object *obj);

#endif /* ENTITY_ENTITY_H_ */