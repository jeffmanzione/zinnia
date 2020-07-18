#include "entity/entity.h"

#include "debug/debug.h"
#include "entity/object.h"
#include "entity/primitive.h"

inline EntityType etype(const Entity *e) {
  ASSERT(NOT_NULL(e));
  return e->type;
}

const Object *object(const Entity *e) {
  ASSERT(NOT_NULL(e));
  return e->obj;
}

Object *object_m(Entity *e) {
  ASSERT(NOT_NULL(e));
  return e->obj;
}

inline Entity entity_char(const int8_t c) {
  Entity e = {.type = PRIMITIVE};
  pset_char(&e.pri, c);
  return e;
}

inline Entity entity_int(const int32_t i) {
  Entity e = {.type = PRIMITIVE};
  pset_int(&e.pri, i);
  return e;
}

inline Entity entity_float(const double d) {
  Entity e = {.type = PRIMITIVE};
  pset_float(&e.pri, d);
  return e;
}

inline Entity entity_primitive(const Primitive *p) {
  Entity e = {.type = PRIMITIVE, .pri = *p};
  return e;
}

Entity entity_none() {
  Entity e = {.type = NONE};
  return e;
}