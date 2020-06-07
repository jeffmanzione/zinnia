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