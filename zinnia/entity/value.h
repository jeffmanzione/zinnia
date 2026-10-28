// value.h
//
// Created on: May 28, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_VALUE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_VALUE_H_

#include <stdio.h>

#include "c-data-structures/stable_maplike.h"
#include "zinnia/entity/object.h"
#include "zinnia/entity/primitive.h"

#define IS_CLASS(e, class) \
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
#define IS_TUPLE(e) \
  ((NULL != e) && (OBJECT == e->type) && (Class_Tuple == e->obj->_class))
#define IS_ARRAY(e) \
  ((NULL != e) && (OBJECT == e->type) && (Class_Array == e->obj->_class))

// Contains a primitive, Object, or represents nullptr.
typedef struct Value_ Value;

typedef enum { NONE, PRIMITIVE, OBJECT } ValueType;

struct Value_ {
  ValueType type;
  union {
    Primitive pri;
    Object *obj;
  };
};

extern Value NONE_VALUE;
extern Value TRUE_VALUE;
extern Value FALSE_VALUE;

// Gets the entity type from an entity.
ValueType etype(const Value *e);
// Extracts a const Object from an entity.
const Object *object(const Value *e);
// Extracts a mutable Object from an entity.
Object *object_m(Value *e);

Value value_char(const int8_t c);
Value value_int(const int64_t i);
Value value_float(const double d);

Value value_primitive_ptr(const Primitive *p);
Value value_primitive(Primitive p);

void value_print(const Value *e, FILE *file);

Value value_none();

Value *object_get(Object *obj, const char field[]);

Value value_object(Object *obj);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_ENTITY_VALUE_H_ */