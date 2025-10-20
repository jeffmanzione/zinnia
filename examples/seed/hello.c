#include "examples/seed/hello.h"

#include "alloc/arena/intern.h"
#include "entity/class/classes_def.h"
#include "entity/entity.h"
#include "entity/native/error.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"

#define GREET_PREFIX "Hello, "
#define GREET_SUFFIX "!\n"

Entity _greet(Task *task, Context *ctx, Object *obj, Entity *args) {
  char *str;
  int str_len;
  if (!extract_string(args, &str, &str_len)) {
    return raise_error(task, ctx, "Argument must be a string.");
  }

  char *buf =
      (char *)malloc(strlen(GREET_PREFIX) + str_len + strlen(GREET_SUFFIX) + 1);
  buf[0] = 0x0;

  sprintf(buf, GREET_PREFIX "%.*s" GREET_SUFFIX, str_len, str);

  Object *res = string_new(task->parent_process->heap, buf, strlen(buf));

  free(buf);
  return entity_object(res);
}

void init_hello(ModuleManager *mm, Module *hello) {
  native_function(hello, mm->intern("__greet"), _greet);
}