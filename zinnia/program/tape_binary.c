// tape_binary.h
//
// Created on: Nov 1, 2020
//     Author: Jeff

#include "zinnia/program/tape_binary.h"

#include "zinnia/program/serialization/deserialize.h"
#include "zinnia/program/serialization/serialize.h"
#include "zinnia/util/void_array.h"
#include "zinnia/vm/intern.h"

#define MAX_STIRNG_SZ 1024

#define _CONST_CHAR_POINTER(ptr) ((const char *)(ptr))

void tape_read_binary(Tape *const tape, FILE *file) {
  ASSERT(tape != NULL);
  ASSERT(file != NULL);
  CharPtrArray strings;
  CharPtrArray_init(&strings);

  uint16_t num_strings;
  deserialize_type(file, uint16_t, &num_strings);
  int i;
  char buf[MAX_STIRNG_SZ];
  for (i = 0; i < num_strings; ++i) {
    deserialize_string(file, buf, MAX_STIRNG_SZ);
    const char *str = global_intern(buf);
    *CharPtrArray_push_back_ref(&strings) = (char *)str;
  }
  Token fake;
  fake.text = CharPtrArray_get_unchecked(&strings, 0);
  tape_module(tape, &fake);

  uint8_t has_source_map;
  deserialize_type(file, uint8_t, &has_source_map);
  if (has_source_map) {
    tape_set_external_source(tape, CharPtrArray_get_unchecked(&strings, 1));
  }

  uint16_t num_refs;
  deserialize_type(file, uint16_t, &num_refs);
  for (i = 0; i < num_refs; ++i) {
    uint16_t ref_name_index, ref_index;
    deserialize_type(file, uint16_t, &ref_name_index);
    deserialize_type(file, uint16_t, &ref_index);
    const char *ref_name =
        CharPtrArray_get_unchecked(&strings, (uint32_t)ref_name_index);
    tape_start_func_at_index(tape, ref_name, ref_index, false);
  }
  uint16_t num_classes;
  deserialize_type(file, uint16_t, &num_classes);
  for (i = 0; i < num_classes; ++i) {
    uint16_t class_name_index, class_start, class_end, num_parents, num_methods;
    deserialize_type(file, uint16_t, &class_name_index);
    deserialize_type(file, uint16_t, &class_start);
    deserialize_type(file, uint16_t, &class_end);
    const char *class_name =
        CharPtrArray_get_unchecked(&strings, (uint32_t)class_name_index);
    ClassRef *cref = tape_start_class_at_index(tape, class_name, class_start);

    deserialize_type(file, uint16_t, &num_parents);
    int j;
    for (j = 0; j < num_parents; ++j) {
      uint16_t parent_name_index;
      deserialize_type(file, uint16_t, &parent_name_index);
      char *parent_name =
          CharPtrArray_get_unchecked(&strings, (uint32_t)parent_name_index);
      CharPtrArray_push_back(&cref->supers, parent_name);
    }

    deserialize_type(file, uint16_t, &num_methods);
    for (j = 0; j < num_methods; ++j) {
      uint16_t method_name_index, method_index;
      deserialize_type(file, uint16_t, &method_name_index);
      char *method_name =
          CharPtrArray_get_unchecked(&strings, (uint32_t)method_name_index);
      deserialize_type(file, uint16_t, &method_index);
      tape_start_func_at_index(tape, method_name, method_index, false);
    }
    DEBUGF("TEST1");
    tape_end_class_at_index(tape, class_end);
  }
  uint16_t num_ins;
  deserialize_type(file, uint16_t, &num_ins);
  for (i = 0; i < num_ins; ++i) {
    Instruction ins;
    deserialize_ins(file, &strings, &ins);
    tape_ins_raw(tape, &ins);
    if (has_source_map) {
      SourceMapping *sm =
          (SourceMapping *)tape_get_source(tape, tape_size(tape) - 1);
      uint16_t source_line;
      uint16_t source_col;
      deserialize_type(file, uint16_t, &source_line);
      deserialize_type(file, uint16_t, &source_col);
      sm->source_line = source_line;
      sm->source_col = source_col;
      sm->source_token = token_create(0, source_line, source_col, NULL, 0);
    }
  }
  CharPtrArray_finalize(&strings);
}

void insert_string_(CharPtrArray *strings, StringIndexMap *string_index,
                    const char str[]) {
  if (StringIndexMap_contains(string_index, str, sizeof(char *))) {
    return;
  }
  CharPtrArray_push_back(strings, (char *)str);
  StringIndexMap_insert(string_index, str, sizeof(char *),
                        CharPtrArray_size(strings) - 1);
}

void intern_all_strings_(const Tape *tape, CharPtrArray *strings,
                         StringIndexMap *string_index) {
  insert_string_(strings, string_index, tape_module_name(tape));
  if (NULL != tape_get_external_source(tape)) {
    insert_string_(strings, string_index, tape_get_external_source(tape));
  }
  ClassRefMapIOIterator class_iter = tape_classes(tape);
  for (; ClassRefMap_io_has_next(&class_iter);
       ClassRefMap_io_next(&class_iter)) {
    const ClassRef *cref = ClassRefMap_io_value(&class_iter);
    insert_string_(strings, string_index, cref->name);

    CharPtrArrayIterator supers;
    CharPtrArray_iterator(&supers, &cref->supers);
    for (; CharPtrArray_has_next(&supers); CharPtrArray_next(&supers)) {
      insert_string_(strings, string_index, *CharPtrArray_value(&supers));
    }
    FunctionRefMapIterator funcs;
    FunctionRefMap_iterator(&funcs, &cref->func_refs);
    for (; FunctionRefMap_has_entry(&funcs);
         FunctionRefMap_next_entry(&funcs)) {
      const FunctionRef *fref = FunctionRefMap_value(&funcs);
      insert_string_(strings, string_index, fref->name);
    }
    FieldRefMapIterator fields;
    FieldRefMap_iterator(&fields, &cref->field_refs);
    for (; FieldRefMap_has_entry(&fields); FieldRefMap_next_entry(&fields)) {
      const FieldRef *fref = FieldRefMap_value(&fields);
      insert_string_(strings, string_index, fref->name);
    }
  }
  FunctionRefMapIOIterator funcs = tape_functions(tape);
  for (; FunctionRefMap_io_has_next(&funcs); FunctionRefMap_io_next(&funcs)) {
    const FunctionRef *fref = FunctionRefMap_io_value(&funcs);
    insert_string_(strings, string_index, fref->name);
  }
  int i;
  for (i = 0; i < tape_size(tape); i++) {
    const Instruction *ins = tape_get(tape, i);
    if (INSTRUCTION_ID == ins->type) {
      insert_string_(strings, string_index, ins->id);
    } else if (INSTRUCTION_STRING == ins->type) {
      insert_string_(strings, string_index, ins->str);
    }
  }
}

void serialize_all_strings_(const CharPtrArray *strings,
                            const StringIndexMap *string_index,
                            WBuffer *buffer) {
  uint16_t num_strings = (uint16_t)CharPtrArray_size(strings);
  serialize_type(buffer, uint16_t, num_strings);
  int i;
  for (i = 0; i < CharPtrArray_size(strings); ++i) {
    serialize_str(buffer, CharPtrArray_get_unchecked(strings, i));
  }
}

void serialize_function_ref_(const FunctionRef *fref, WBuffer *buffer,
                             const CharPtrArray *strings,
                             const StringIndexMap *string_index) {
  uint16_t ref_name_index = (uint16_t)StringIndexMap_find(
      string_index, fref->name, sizeof(char *), 0);
  ASSERT(ref_name_index >= 0);
  uint16_t ref_index = (uint16_t)fref->index;
  serialize_type(buffer, uint16_t, ref_name_index);
  serialize_type(buffer, uint16_t, ref_index);
}

void serialize_class_ref_(const ClassRef *cref, WBuffer *buffer,
                          const CharPtrArray *strings,
                          const StringIndexMap *string_index) {
  uint16_t class_name_index = (uint16_t)StringIndexMap_find(
      string_index, cref->name, sizeof(char *), 0);
  uint16_t class_start = (uint16_t)cref->start_index;
  uint16_t class_end = (uint16_t)cref->end_index;
  serialize_type(buffer, uint16_t, class_name_index);
  serialize_type(buffer, uint16_t, class_start);
  serialize_type(buffer, uint16_t, class_end);

  uint16_t num_parents = CharPtrArray_size(&cref->supers);
  serialize_type(buffer, uint16_t, num_parents);
  CharPtrArrayIterator supers;
  CharPtrArray_iterator(&supers, &cref->supers);
  for (; CharPtrArray_has_next(&supers); CharPtrArray_next(&supers)) {
    uint16_t parent_name_index = (uint16_t)StringIndexMap_find(
        string_index, *CharPtrArray_value(&supers), sizeof(char *), -1);
    serialize_type(buffer, uint16_t, parent_name_index);
  }

  uint16_t num_methods = FunctionRefMap_size(&cref->func_refs);
  serialize_type(buffer, uint16_t, num_methods);
  FunctionRefMapIterator methods;
  FunctionRefMap_iterator(&methods, &cref->func_refs);
  for (; FunctionRefMap_has_entry(&methods);
       FunctionRefMap_next_entry(&methods)) {
    const FunctionRef *fref = FunctionRefMap_value(&methods);
    serialize_function_ref_(fref, buffer, strings, string_index);
  }
}

void tape_write_binary(const Tape *const tape, FILE *file) {
  ASSERT(tape != NULL);
  ASSERT(file != NULL);
  CharPtrArray strings;
  CharPtrArray_init(&strings);
  StringIndexMap string_index;
  StringIndexMap_init(&string_index, hash_interned_string,
                      compare_interned_strings);

  intern_all_strings_(tape, &strings, &string_index);

  WBuffer buffer;
  buffer_init(&buffer, file, 512);

  serialize_all_strings_(&strings, &string_index, &buffer);

  uint8_t has_source_map = NULL != tape_get_external_source(tape);
  serialize_type(&buffer, uint8_t, has_source_map);

  uint16_t num_refs = (uint16_t)tape_func_count(tape);
  serialize_type(&buffer, uint16_t, num_refs);
  FunctionRefMapIOIterator refs = tape_functions(tape);
  for (; FunctionRefMap_io_has_next(&refs); FunctionRefMap_io_next(&refs)) {
    const FunctionRef *fref = FunctionRefMap_io_value(&refs);
    serialize_function_ref_(fref, &buffer, &strings, &string_index);
  }

  uint16_t num_classes = (uint16_t)tape_class_count(tape);
  serialize_type(&buffer, uint16_t, num_classes);
  ClassRefMapIOIterator classes = tape_classes(tape);
  for (; ClassRefMap_io_has_next(&classes); ClassRefMap_io_next(&classes)) {
    const ClassRef *cref = ClassRefMap_io_value(&classes);
    serialize_class_ref_(cref, &buffer, &strings, &string_index);
  }

  uint16_t num_ins = (uint16_t)tape_size(tape);
  serialize_type(&buffer, uint16_t, num_ins);
  int i;
  for (i = 0; i < num_ins; ++i) {
    serialize_ins(&buffer, tape_get(tape, i), &string_index);
    if (has_source_map) {
      const SourceMapping *sm = tape_get_source(tape, i);
      uint16_t source_line = sm->line;
      uint16_t source_col = sm->col;
      serialize_type(&buffer, uint16_t, source_line);
      serialize_type(&buffer, uint16_t, source_col);
    }
  }

  buffer_finalize(&buffer);
  StringIndexMap_finalize(&string_index);
  CharPtrArray_finalize(&strings);
}
