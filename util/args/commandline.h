// commandline.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef UTIL_ARGS_COMMANDLINE_H_
#define UTIL_ARGS_COMMANDLINE_H_

#include "struct/set.h"
#include "util/args/commandline_arg.h"

#define ARGSTORE_TEMPLATE_RETVAL(type, retval)                                 \
  retval argstore_lookup_##type(const ArgStore *store, ArgKey key)

#define ARGSTORE_TEMPLATE(type) ARGSTORE_TEMPLATE_RETVAL(type, type)

typedef enum {
  ArgKey__NONE = 0,
  ArgKey__OUT_BINARY,
  ArgKey__OUT_ASSEMBLY,
  ArgKey__BIN_OUT_DIR,
  ArgKey__ASSEMBLY_OUT_DIR,
  ArgKey__OPTIMIZE,
  ArgKey__LIB_LOCATION,
  ArgKey__MAX_PROCESS_OBJECT_COUNT,
  ArgKey__ASYNC,
  ArgKey__END,
} ArgKey;

typedef struct __ArgConfig ArgConfig;
typedef struct __Argstore ArgStore;

ArgConfig *argconfig_create();
void argconfig_delete(ArgConfig *config);

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
                   char short_name, Arg arg_default);

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
                                 const char *argv[]);

const Set *argstore_sources(const ArgStore *const store);

ARGSTORE_TEMPLATE(int);
ARGSTORE_TEMPLATE(float);
ARGSTORE_TEMPLATE_RETVAL(bool, _Bool);
ARGSTORE_TEMPLATE_RETVAL(string, const char *);
ARGSTORE_TEMPLATE_RETVAL(stringlist, const char **);

const Arg *argstore_get(const ArgStore *store, ArgKey key);
void argstore_delete(ArgStore *store);

const Map *argstore_program_args(const ArgStore *store);

#endif /* UTIL_ARGS_COMMANDLINE_H_ */