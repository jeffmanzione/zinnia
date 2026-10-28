#include "zinnia/entity/value.h"

#include <inttypes.h>

#include "zinnia/util/error.h"

IMPL_STABLE_MAPLIKE(ValueMap, char *, Value);

Value NONE_VALUE = {.type = NONE};
Value TRUE_VALUE = {.type = PRIMITIVE,
                    .pri = {._type = PRIMITIVE_BOOL, ._bool_val = true}};
Value FALSE_VALUE = {.type = PRIMITIVE,
                     .pri = {._type = PRIMITIVE_BOOL, ._bool_val = false}};

ValueType etype(const Value *e) {
  ASSERT(e != NULL);
  return e->type;
}

const Object *object(const Value *e) {
  ASSERT(e != NULL);
  return e->obj;
}

Object *object_m(Value *e) {
  ASSERT(e != NULL);
  return e->obj;
}

Value value_char(const int8_t c) {
  Value e = {.type = PRIMITIVE};
  pset_char(&e.pri, c);
  return e;
}

Value value_int(const int64_t i) {
  Value e = {.type = PRIMITIVE};
  pset_int(&e.pri, i);
  return e;
}

Value value_float(const double d) {
  Value e = {.type = PRIMITIVE};
  pset_float(&e.pri, d);
  return e;
}

Value value_primitive_ptr(const Primitive *p) {
  Value e = {.type = PRIMITIVE, .pri = *p};
  return e;
}

Value value_primitive(Primitive p) {
  Value e = {.type = PRIMITIVE, .pri = p};
  return e;
}

void primitive_print_(const Primitive *p, FILE *file) {
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

void object_print_(const Object *obj, FILE *file) {
  if (NULL != obj->_class->_print_fn) {
    obj->_class->_print_fn(obj, file);
    return;
  }
  fprintf(file, "Instance of %s.%s", obj->_class->_module->_name,
          obj->_class->_name);
}

void value_print(const Value *e, FILE *file) {
  ASSERT(e != NULL);
  ASSERT(file != NULL);
  switch (e->type) {
    case NONE:
      fprintf(file, "None");
      break;
    case PRIMITIVE:
      primitive_print_(&e->pri, file);
      break;
    case OBJECT:
      object_print_(e->obj, file);
      break;
    default:
      FATALF("Unknown entity type: %d", e->type);
  }
}

Value value_none() {
  Value e = {.type = NONE};
  return e;
}

Value *object_get(Object *obj, const char field[]) {
  ASSERT(obj != NULL);
  ASSERT(field != NULL);
  return ValueMap_find_ref(&obj->_members, field, sizeof(char *));
}

Value value_object(Object *obj) {
  Value e = {.type = OBJECT, .obj = obj};
  return e;
}