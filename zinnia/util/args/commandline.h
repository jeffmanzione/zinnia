// commandline.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ARGS_COMMANDLINE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ARGS_COMMANDLINE_H_

#include "c-data-structures/maplike.h"
#include "c-data-structures/setlike.h"
#include "zinnia/util/args/commandline_arg.h"

#define ARGSTORE_TEMPLATE_RETVAL(type, retval) \
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
  Argkey__MINIMIZE,
  ArgKey__MAX_PROCESS_OBJECT_COUNT,
  ArgKey__ASYNC,
  ArgKey__VERSION,
  ArgKey__END,
} ArgKey;

typedef struct ArgConfig__ ArgConfig;
typedef struct Argstore__ ArgStore;

DEFINE_SETLIKE(SourceNameSet, char *);
DEFINE_MAPLIKE(ArgKeyMap, char *, ArgKey);
DEFINE_MAPLIKE(ArgMap, char *, char *);

ArgConfig *argconfig_create();
void argconfig_delete(ArgConfig *config);

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
                   char short_name, Arg arg_default);
void argconfig_set_allow_compiler_args_and_sources(ArgConfig *config,
                                                   bool enabled);

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
                                 const char *argv[]);

const SourceNameSet *argstore_sources(const ArgStore *const store);

ARGSTORE_TEMPLATE(int);
ARGSTORE_TEMPLATE(float);
ARGSTORE_TEMPLATE_RETVAL(bool, _Bool);
ARGSTORE_TEMPLATE_RETVAL(string, const char *);
ARGSTORE_TEMPLATE_RETVAL(stringlist, const char **);

const Arg *argstore_get(const ArgStore *store, ArgKey key);
void argstore_delete(ArgStore *store);

const ArgMap *argstore_program_args(const ArgStore *store);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ARGS_COMMANDLINE_H_ */