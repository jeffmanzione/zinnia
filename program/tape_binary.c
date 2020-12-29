// tape_binary.h
//
// Created on: Nov 1, 2020
//     Author: Jeff

#include "program/tape_binary.h"

#include "alloc/arena/intern.h"
#include "program/serialization/deserialize.h"
#include "program/serialization/serialize.h"
#include "struct/alist.h"
#include "struct/keyed_list.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"

#define MAX_STIRNG_SZ 1024

#define _CONST_CHAR_POINTER(ptr) ((const char *)(ptr))

void tape_read_binary(Tape *const tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  AList strings;
  alist_init(&strings, char *, DEFAULT_ARRAY_SZ);

  uint16_t num_strings;
  deserialize_type(file, uint16_t, &num_strings);
  int i;
  char buf[MAX_STIRNG_SZ];
  for (i = 0; i < num_strings; ++i) {
    deserialize_string(file, buf, MAX_STIRNG_SZ);
    char *str = intern(buf);
    *((char **)alist_add(&strings)) = str;
  }
  Token fake;
  fake.text = *((char **)alist_get(&strings, 0));
  tape_module(tape, &fake);
  uint16_t num_refs;
  deserialize_type(file, uint16_t, &num_refs);
  for (i = 0; i < num_refs; ++i) {
    uint16_t ref_name_index, ref_index;
    deserialize_type(file, uint16_t, &ref_name_index);
    deserialize_type(file, uint16_t, &ref_index);
    char *ref_name = *((char **)alist_get(&strings, (uint32_t)ref_name_index));
    tape_start_func_at_index(tape, ref_name, ref_index, false);
  }
  uint16_t num_classes;
  deserialize_type(file, uint16_t, &num_classes);
  for (i = 0; i < num_classes; i++) {
    uint16_t class_name_index, class_start, class_end, num_parents, num_methods;
    deserialize_type(file, uint16_t, &class_name_index);
    deserialize_type(file, uint16_t, &class_start);
    deserialize_type(file, uint16_t, &class_end);
    char *class_name =
        *((char **)alist_get(&strings, (uint32_t)class_name_index));
    ClassRef *cref = tape_start_class_at_index(tape, class_name, class_start);

    deserialize_type(file, uint16_t, &num_parents);
    int j;
    for (j = 0; j < num_parents; ++j) {
      uint16_t parent_name_index;
      deserialize_type(file, uint16_t, &parent_name_index);
      char *parent_name =
          *((char **)alist_get(&strings, (uint32_t)parent_name_index));
      alist_append(&cref->supers, &parent_name);
    }

    deserialize_type(file, uint16_t, &num_methods);
    for (j = 0; j < num_methods; ++j) {
      uint16_t method_name_index, method_index;
      deserialize_type(file, uint16_t, &method_name_index);
      char *method_name =
          *((char **)alist_get(&strings, (uint32_t)method_name_index));
      deserialize_type(file, uint16_t, &method_index);
      tape_start_func_at_index(tape, method_name, method_index, false);
    }
    tape_end_class_at_index(tape, class_end);
  }
  uint16_t num_ins;
  deserialize_type(file, uint16_t, &num_ins);
  for (i = 0; i < num_ins; i++) {
    Instruction ins;
    deserialize_ins(file, &strings, &ins);
    tape_ins_raw(tape, &ins);
  }
  alist_finalize(&strings);
}

void _insert_string(AList *strings, Map *string_index, const char str[]) {
  if (map_lookup(string_index, str)) {
    return;
  }
  map_insert(string_index, str, (void *)alist_append(strings, &str));
}

void _intern_all_strings(const Tape *tape, AList *strings, Map *string_index) {
  _insert_string(strings, string_index, tape_module_name(tape));
  KL_iter class_iter = tape_classes(tape);
  for (; kl_has(&class_iter); kl_inc(&class_iter)) {
    _insert_string(strings, string_index, kl_key(&class_iter));
    ClassRef *cref = (ClassRef *)kl_value(&class_iter);
    AL_iter supers = alist_iter(&cref->supers);
    for (; al_has(&supers); al_inc(&supers)) {
      _insert_string(strings, string_index, *((char **)al_value(&supers)));
    }
    KL_iter funcs = keyedlist_iter(&cref->func_refs);
    for (; kl_has(&funcs); kl_inc(&funcs)) {
      FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
      _insert_string(strings, string_index, fref->name);
    }
  }
  KL_iter funcs = tape_functions(tape);
  for (; kl_has(&funcs); kl_inc(&funcs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&funcs);
    _insert_string(strings, string_index, fref->name);
  }
  int i;
  for (i = 0; i < tape_size(tape); i++) {
    const Instruction *ins = tape_get(tape, i);
    if (INSTRUCTION_ID == ins->type) {
      _insert_string(strings, string_index, ins->id);
    } else if (INSTRUCTION_STRING == ins->type) {
      _insert_string(strings, string_index, ins->str);
    }
  }
}

void _serialize_all_strings(const AList *strings, const Map *string_index,
                            WBuffer *buffer) {
  uint16_t num_strings = (uint16_t)alist_len(strings);
  serialize_type(buffer, uint16_t, num_strings);
  int i;
  for (i = 0; i < alist_len(strings); ++i) {
    serialize_str(buffer, *((char **)alist_get(strings, i)));
  }
}

void _serialize_function_ref(const FunctionRef *fref, WBuffer *buffer,
                             const AList *strings, Map *string_index) {
  uint16_t ref_name_index =
      (uint16_t)(uintptr_t)map_lookup(string_index, fref->name);
  uint16_t ref_index = (uint16_t)fref->index;
  serialize_type(buffer, uint16_t, ref_name_index);
  serialize_type(buffer, uint16_t, ref_index);
}

void _serialize_class_ref(const ClassRef *cref, WBuffer *buffer,
                          const AList *strings, Map *string_index) {
  uint16_t class_name_index =
      (uint16_t)(uintptr_t)map_lookup(string_index, cref->name);
  uint16_t class_start = (uint16_t)cref->start_index;
  uint16_t class_end = (uint16_t)cref->end_index;
  serialize_type(buffer, uint16_t, class_name_index);
  serialize_type(buffer, uint16_t, class_start);
  serialize_type(buffer, uint16_t, class_end);

  uint16_t num_parents = alist_len(&cref->supers);
  serialize_type(buffer, uint16_t, num_parents);
  AL_iter supers = alist_iter(&cref->supers);
  for (; al_has(&supers); al_inc(&supers)) {
    uint16_t parent_name_index = (uint16_t)(uintptr_t)map_lookup(
        string_index, *((char **)al_value(&supers)));
    serialize_type(buffer, uint16_t, parent_name_index);
  }

  uint16_t num_methods = alist_len(&cref->func_refs._list);
  serialize_type(buffer, uint16_t, num_methods);
  KL_iter methods = keyedlist_iter((KeyedList *)&cref->func_refs);
  for (; kl_has(&methods); kl_inc(&methods)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&methods);
    _serialize_function_ref(fref, buffer, strings, string_index);
  }
}

void tape_write_binary(const Tape *const tape, FILE *file) {
  ASSERT(NOT_NULL(tape), NOT_NULL(file));
  AList strings;
  alist_init(&strings, char *, DEFAULT_ARRAY_SZ);
  Map string_index;
  map_init_default(&string_index);

  _intern_all_strings(tape, &strings, &string_index);

  WBuffer buffer;
  buffer_init(&buffer, file, 512);

  _serialize_all_strings(&strings, &string_index, &buffer);

  uint16_t num_refs = (uint16_t)tape_func_count(tape);
  serialize_type(&buffer, uint16_t, num_refs);
  KL_iter refs = tape_functions(tape);
  for (; kl_has(&refs); kl_inc(&refs)) {
    FunctionRef *fref = (FunctionRef *)kl_value(&refs);
    _serialize_function_ref(fref, &buffer, &strings, &string_index);
  }

  uint16_t num_classes = (uint16_t)tape_class_count(tape);
  serialize_type(&buffer, uint16_t, num_classes);
  KL_iter classes = tape_classes(tape);
  for (; kl_has(&classes); kl_inc(&classes)) {
    ClassRef *cref = (ClassRef *)kl_value(&classes);
    _serialize_class_ref(cref, &buffer, &strings, &string_index);
  }

  uint16_t num_ins = (uint16_t)tape_size(tape);
  serialize_type(&buffer, uint16_t, num_ins);
  int i;
  for (i = 0; i < num_ins; i++) {
    serialize_ins(&buffer, tape_get(tape, i), &string_index);
  }

  buffer_finalize(&buffer);
  map_finalize(&string_index);
  alist_finalize(&strings);
}
