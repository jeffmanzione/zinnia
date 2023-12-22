#include "entity/entity.h"

#include <inttypes.h>

#include "debug/debug.h"
#include "entity/object.h"
#include "entity/primitive.h"

Entity NONE_ENTITY = {.type = NONE};
Entity TRUE_ENTITY = {.type = PRIMITIVE,
                      .pri = {._type = PRIMITIVE_BOOL, ._bool_val = true}};
Entity FALSE_ENTITY = {.type = PRIMITIVE,
                       .pri = {._type = PRIMITIVE_BOOL, ._bool_val = false}};

EntityType etype(const Entity *e) {
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

Entity entity_char(const int8_t c) {
  Entity e = {.type = PRIMITIVE};
  pset_char(&e.pri, c);
  return e;
}

Entity entity_int(const int64_t i) {
  Entity e = {.type = PRIMITIVE};
  pset_int(&e.pri, i);
  return e;
}

Entity entity_float(const double d) {
  Entity e = {.type = PRIMITIVE};
  pset_float(&e.pri, d);
  return e;
}

Entity entity_primitive_ptr(const Primitive *p) {
  Entity e = {.type = PRIMITIVE, .pri = *p};
  return e;
}

Entity entity_primitive(Primitive p) {
  Entity e = {.type = PRIMITIVE, .pri = p};
  return e;
}

void _primitive_print(const Primitive *p, FILE *file) {
  switch (p->_type) {
  case PRIMITIVE_BOOL:
    fprintf(file, "%s", p->_bool_val ? "True" : "False");
    break;
  case PRIMITIVE_CHAR:
    fprintf(file, "%c", (char)p->_char_val);
    break;
  case PRIMITIVE_INT:
    fprintf(file, "%" PRId64, p->_int_val);
    break;
  case PRIMITIVE_FLOAT:
    fprintf(file, "%f", p->_float_val);
    break;
  default:
    FATALF("Unknwn primitive type.");
  }
}

void _object_print(const Object *obj, FILE *file) {
  if (NULL != obj->_class->_print_fn) {
    obj->_class->_print_fn(obj, file);
    return;
  }
  fprintf(file, "Instance of %s.%s", obj->_class->_module->_name,
          obj->_class->_name);
}

void entity_print(const Entity *e, FILE *file) {
  ASSERT(NOT_NULL(e), NOT_NULL(file));
  switch (e->type) {
  case NONE:
    fprintf(file, "None");
    break;
  case PRIMITIVE:
    _primitive_print(&e->pri, file);
    break;
  case OBJECT:
    _object_print(e->obj, file);
    break;
  default:
    FATALF("Unknown entity type: %d", e->type);
  }
}

Entity entity_none() {
  Entity e = {.type = NONE};
  return e;
}

Entity *object_get(Object *obj, const char field[]) {
  ASSERT(NOT_NULL(obj), NOT_NULL(field));
  return &((Member *)keyedlist_lookup(&obj->_members, field))->entity;
}

Entity entity_object(Object *obj) {
  Entity e = {.type = OBJECT, .obj = obj};
  return e;
}