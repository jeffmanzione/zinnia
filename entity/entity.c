#include "entity/entity.h"

#include "debug/debug.h"
#include "entity/object.h"
#include "entity/primitive.h"

Entity NONE_ENTITY = {.type = NONE};

inline EntityType etype(const Entity *e) {
  ASSERT(NOT_NULL(e));
  return e->type;
}

const Object *object(const Entity *e) {
  ASSERT(NOT_NULL(e));
  return e->obj;
}

inline Object *object_m(Entity *e) {
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

inline Entity entity_none() {
  Entity e = {.type = NONE};
  return e;
}

inline Entity *object_get(Object *obj, const char field[]) {
  ASSERT(NOT_NULL(obj), NOT_NULL(field));
  return (Entity *)keyedlist_lookup(&obj->_members, field);
}