// slots.c
//
// Variables kept in slots: see slots.h.

#include "zinnia/program/slots.h"

#include <stdlib.h>
#include <string.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/error.h"
#include "zinnia/util/void_array.h"
#include "zinnia/vm/intern.h"

#define ANON_PREFIX "$anon"

// A function, method, lambda or module body: the instructions [start, end),
// less those of the lambdas inside it.
typedef struct {
  uint32_t start, end;
  int parent;  // Index of the innermost body around it, or -1.
  bool is_anon;
  CharPtrArray introduced;  // Names it assigns or takes as parameters.
  CharPtrArray mentioned;   // Names it or a lambda inside it uses.
  CharPtrArray slots;       // Names kept in its slots, in slot order.
} Body_;

static bool contains_(const CharPtrArray *names, const char *name) {
  for (int i = 0; i < CharPtrArray_size(names); ++i) {
    if (0 == strcmp(CharPtrArray_get_unchecked(names, i), name)) {
      return true;
    }
  }
  return false;
}

static int index_of_(const CharPtrArray *names, const char *name) {
  for (int i = 0; i < CharPtrArray_size(names); ++i) {
    if (0 == strcmp(CharPtrArray_get_unchecked(names, i), name)) {
      return i;
    }
  }
  return -1;
}

static void add_once_(CharPtrArray *names, const char *name) {
  if (!contains_(names, name)) {
    CharPtrArray_push_back(names, (char *)name);
  }
}

static void add_body_(Body_ **bodies, int *num_bodies, int *capacity,
                      uint32_t start, uint32_t end, bool is_anon) {
  if (*num_bodies == *capacity) {
    *capacity = (0 == *capacity) ? 16 : *capacity * 2;
    *bodies = (NULL == *bodies) ? MNEW_ARR(Body_, *capacity)
                                : REALLOC(*bodies, Body_, *capacity);
  }
  Body_ *body = *bodies + (*num_bodies)++;
  body->start = start;
  body->end = end;
  body->parent = -1;
  body->is_anon = is_anon;
  CharPtrArray_init(&body->introduced);
  CharPtrArray_init(&body->mentioned);
  CharPtrArray_init(&body->slots);
}

static int compare_bodies_(const void *a, const void *b) {
  const Body_ *x = a, *y = b;
  if (x->start != y->start) {
    return x->start < y->start ? -1 : 1;
  }
  // The longer of two bodies starting together is the outer one.
  return x->end > y->end ? -1 : (x->end < y->end ? 1 : 0);
}

static bool starts_body_(const Bytecode *bytecode, uint32_t index) {
  return index < bytecode_size(bytecode) &&
         SLOTS == bytecode_get(bytecode, index)->op;
}

static void add_anon_starts_(const Bytecode *bytecode,
                             FunctionRefMapIOIterator it, IntArray *anon) {
  for (; FunctionRefMap_io_has_next(&it); FunctionRefMap_io_next(&it)) {
    const FunctionRef *ref = FunctionRefMap_io_value(&it);
    if (starts_body_(bytecode, ref->index) &&
        0 == strncmp(ref->name, ANON_PREFIX, strlen(ANON_PREFIX))) {
      IntArray_push_back(anon, ref->index);
    }
  }
}

static bool contains_int_(const IntArray *ints, int value) {
  for (int i = 0; i < IntArray_size(ints); ++i) {
    if (IntArray_get_unchecked(ints, i) == value) {
      return true;
    }
  }
  return false;
}

static int compare_ints_(const void *a, const void *b) {
  return *(const int *)a - *(const int *)b;
}

// Finds the bodies of [bytecode], sorted by start with each body's parent set,
// and for each instruction the body it belongs to (-1 for none).
static Body_ *find_bodies_(const Bytecode *bytecode, int *num_bodies,
                           int **owners) {
  const uint32_t size = bytecode_size(bytecode);
  IntArray named, anon;
  IntArray_init(&named);
  IntArray_init(&anon);
  add_anon_starts_(bytecode, bytecode_functions(bytecode), &anon);
  ClassRefMapIOIterator classes = bytecode_classes(bytecode);
  for (; ClassRefMap_io_has_next(&classes); ClassRefMap_io_next(&classes)) {
    const ClassRef *cls = ClassRefMap_io_value(&classes);
    FunctionRefMapIOIterator methods;
    FunctionRefMap_io_iterator(&methods, &cls->func_refs);
    add_anon_starts_(bytecode, methods, &anon);
  }
  // Every other SLOTS starts a named body: a function, a method, or a module's
  // own code (more than one when classes are added to a module at run time).
  for (uint32_t i = 0; i < size; ++i) {
    if (starts_body_(bytecode, i) && !contains_int_(&anon, (int)i)) {
      IntArray_push_back(&named, (int)i);
    }
  }
  if (IntArray_size(&named) > 0) {
    qsort((int *)IntArray_get_ref_unchecked(&named, 0), IntArray_size(&named),
          sizeof(int), compare_ints_);
  }

  Body_ *bodies = NULL;
  int capacity = 0;
  *num_bodies = 0;
  // A named body runs to the next one; a lambda's is the jump over it.
  for (int i = 0; i < IntArray_size(&named); ++i) {
    const int start = IntArray_get_unchecked(&named, i);
    if (i > 0 && start == IntArray_get_unchecked(&named, i - 1)) {
      continue;
    }
    const int end = (i + 1 < IntArray_size(&named))
                        ? IntArray_get_unchecked(&named, i + 1)
                        : (int)size;
    add_body_(&bodies, num_bodies, &capacity, start, end, false);
  }
  for (int i = 0; i < IntArray_size(&anon); ++i) {
    const int start = IntArray_get_unchecked(&anon, i);
    ASSERT(start > 0);
    const Instruction *jump = bytecode_get(bytecode, start - 1);
    ASSERT(JMP == jump->op);
    add_body_(&bodies, num_bodies, &capacity, start,
              start + (uint32_t)pint(&jump->val), true);
  }
  IntArray_finalize(&named);
  IntArray_finalize(&anon);
  if (0 == *num_bodies) {
    *owners = NULL;
    return bodies;
  }
  qsort(bodies, *num_bodies, sizeof(Body_), compare_bodies_);

  // Bodies nest, so the innermost body around each instruction is the last
  // one, in start order, that contains it.
  *owners = MNEW_ARR(int, size);
  for (uint32_t i = 0; i < size; ++i) {
    (*owners)[i] = -1;
  }
  for (int b = 0; b < *num_bodies; ++b) {
    Body_ *body = bodies + b;
    for (int a = b - 1; a >= 0; --a) {
      if (bodies[a].start <= body->start && body->end <= bodies[a].end) {
        body->parent = a;
        break;
      }
    }
    for (uint32_t i = body->start; i < body->end && i < size; ++i) {
      (*owners)[i] = b;
    }
  }
  return bodies;
}

static void delete_bodies_(Body_ *bodies, int num_bodies, int *owners) {
  for (int b = 0; b < num_bodies; ++b) {
    CharPtrArray_finalize(&bodies[b].introduced);
    CharPtrArray_finalize(&bodies[b].mentioned);
    CharPtrArray_finalize(&bodies[b].slots);
  }
  if (NULL != bodies) {
    RELEASE(bodies);
  }
  if (NULL != owners) {
    RELEASE(owners);
  }
}

static bool creates_variable_(const Instruction *ins) {
  return INSTRUCTION_ID == ins->type &&
         (LET == ins->op || LETC == ins->op || SET == ins->op ||
          SETC == ins->op);
}

void bytecode_assign_slots(Bytecode *bytecode) {
  int num_bodies, *owners;
  Body_ *bodies = find_bodies_(bytecode, &num_bodies, &owners);
  if (0 == num_bodies) {
    delete_bodies_(bodies, num_bodies, owners);
    return;
  }
  const uint32_t size = bytecode_size(bytecode);
  for (uint32_t i = 0; i < size; ++i) {
    const Instruction *ins = bytecode_get(bytecode, i);
    const int owner = owners[i];
    if (owner < 0 || SLOTS == ins->op || INSTRUCTION_ID != ins->type) {
      continue;
    }
    if (creates_variable_(ins)) {
      add_once_(&bodies[owner].introduced, ins->id);
    }
    for (int b = owner; b >= 0; b = bodies[b].parent) {
      add_once_(&bodies[b].mentioned, ins->id);
    }
  }
  for (int b = 0; b < num_bodies; ++b) {
    Body_ *body = bodies + b;
    for (int n = 0; n < CharPtrArray_size(&body->introduced); ++n) {
      const char *name = CharPtrArray_get_unchecked(&body->introduced, n);
      if (0 == strcmp("self", name)) {
        continue;
      }
      // A lambda inside it mentions the name, and may outlive the call.
      bool captured = false;
      for (int c = 0; c < num_bodies && !captured; ++c) {
        if (bodies[c].parent == b && contains_(&bodies[c].mentioned, name)) {
          captured = true;
        }
      }
      // For a lambda, a body around it creates the name.
      for (int a = body->is_anon ? body->parent : -1; a >= 0 && !captured;
           a = bodies[a].parent) {
        if (contains_(&bodies[a].introduced, name)) {
          captured = true;
        }
      }
      if (!captured) {
        CharPtrArray_push_back(&body->slots, (char *)name);
      }
    }
  }
  for (uint32_t i = 0; i < size; ++i) {
    Instruction *ins = bytecode_get_mutable(bytecode, i);
    const int owner = owners[i];
    if (owner < 0 || INSTRUCTION_ID != ins->type || SLOTS == ins->op) {
      continue;
    }
    const int slot = index_of_(&bodies[owner].slots, ins->id);
    ins->slot = (slot < 0) ? 0 : (uint16_t)(slot + 1);
  }
  for (int b = 0; b < num_bodies; ++b) {
    Body_ *body = bodies + b;
    size_t len = 0;
    for (int n = 0; n < CharPtrArray_size(&body->slots); ++n) {
      len += strlen(CharPtrArray_get_unchecked(&body->slots, n)) + 1;
    }
    char *names = MNEW_ARR(char, len + 1);
    names[0] = '\0';
    for (int n = 0; n < CharPtrArray_size(&body->slots); ++n) {
      if (n > 0) {
        strcat(names, " ");
      }
      strcat(names, CharPtrArray_get_unchecked(&body->slots, n));
    }
    Instruction *slots_ins = bytecode_get_mutable(bytecode, body->start);
    slots_ins->type = INSTRUCTION_STRING;
    slots_ins->str = global_intern(names);
    slots_ins->slot = (uint16_t)CharPtrArray_size(&body->slots);
    RELEASE(names);
  }
  delete_bodies_(bodies, num_bodies, owners);
}

void bytecode_resolve_slot_names(Bytecode *bytecode) {
  int num_bodies, *owners;
  Body_ *bodies = find_bodies_(bytecode, &num_bodies, &owners);
  for (int b = 0; b < num_bodies; ++b) {
    Body_ *body = bodies + b;
    Instruction *slots_ins = bytecode_get_mutable(bytecode, body->start);
    if (INSTRUCTION_STRING != slots_ins->type) {
      slots_ins->slot = 0;
      continue;
    }
    // The names are separated by spaces.
    const char *names = slots_ins->str;
    while ('\0' != *names) {
      const char *end = strchr(names, ' ');
      const size_t len = (NULL == end) ? strlen(names) : (size_t)(end - names);
      char *name = MNEW_ARR(char, len + 1);
      memcpy(name, names, len);
      name[len] = '\0';
      CharPtrArray_push_back(&body->slots, (char *)global_intern(name));
      RELEASE(name);
      names += len + (NULL == end ? 0 : 1);
    }
    slots_ins->slot = (uint16_t)CharPtrArray_size(&body->slots);
  }
  const uint32_t size = bytecode_size(bytecode);
  for (uint32_t i = 0; i < size; ++i) {
    Instruction *ins = bytecode_get_mutable(bytecode, i);
    if (INSTRUCTION_ID != ins->type || 0 == ins->slot || NULL != ins->id) {
      continue;
    }
    const int owner = owners == NULL ? -1 : owners[i];
    if (owner < 0 || ins->slot > CharPtrArray_size(&bodies[owner].slots)) {
      FATALF("Slot operand without a matching slot.");
    }
    ins->id = CharPtrArray_get_unchecked(&bodies[owner].slots, ins->slot - 1);
  }
  delete_bodies_(bodies, num_bodies, owners);
}
