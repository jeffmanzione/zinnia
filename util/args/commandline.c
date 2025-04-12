// commandline.c
//
// Created on: May 28, 2018
//     Author: Jeff

#include "util/args/commandline.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "debug/debug.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"
#include "version/version.h"

#define _CONST_CHAR_POINTER(ptr) ((const char *)(ptr))

#define ARGSTORE_LOOKUP_RETVAL(typet, retval)                                  \
  retval argstore_lookup_##typet(const ArgStore *store, ArgKey key) {          \
    const Arg *arg = argstore_get(store, key);                                 \
    if (!arg->used) {                                                          \
      FATALF("Store did not have key: %d", key);                               \
    }                                                                          \
    if (ArgType__##typet != arg->type) {                                       \
      FATALF("Expected a " #typet " for key=%d. Was %d.", key, arg->type);     \
    }                                                                          \
    return (retval)arg->typet##_val;                                           \
  }

#define ARGSTORE_LOOKUP(typet) ARGSTORE_LOOKUP_RETVAL(typet, typet)

struct __Argstore {
  const ArgConfig *config;
  Arg *args;
  Set src_files;
  Map program_args;
};

struct __ArgConfig {
  Map arg_names;
  Arg *args;
  bool mandatory_sources;
};

ArgConfig *argconfig_create() {
  ArgConfig *config = MNEW(ArgConfig);
  config->args = CNEW_ARR(Arg, ArgKey__END);
  config->mandatory_sources = true;
  map_init_default(&config->arg_names);
  argconfig_add(config, ArgKey__VERSION, "version", 'v', arg_bool(false));
  return config;
}

void argconfig_delete(ArgConfig *config) {
  ASSERT(NOT_NULL(config));
  int i;
  for (i = 0; i < ArgKey__END; ++i) {
    if (ArgType__stringlist == config->args[i].type) {
      RELEASE(config->args[i].stringlist_val);
    }
  }
  RELEASE(config->args);
  map_finalize(&config->arg_names);
  RELEASE(config);
}

ArgStore *argstore_create(const ArgConfig *config) {
  ASSERT(NOT_NULL(config));
  ArgStore *store = MNEW(ArgStore);
  store->config = config;
  store->args = CNEW_ARR(Arg, ArgKey__END);
  set_init_default(&store->src_files);
  map_init_default(&store->program_args);
  return store;
}

void argstore_delete(ArgStore *store) {
  ASSERT(NOT_NULL(store));
  set_finalize(&store->src_files);
  map_finalize(&store->program_args);
  int i;
  for (i = 0; i < ArgKey__END; ++i) {
    if (ArgType__stringlist == store->args[i].type) {
      RELEASE(store->args[i].stringlist_val);
    }
  }
  RELEASE(store->args);
  RELEASE(store);
}

const Arg *argstore_get(const ArgStore *store, ArgKey key) {
  ASSERT(NOT_NULL(store), key > ArgKey__NONE, key < ArgKey__END);
  Arg *arg = &store->args[key];
  if (!arg->used && NULL != store->config) {
    arg = &store->config->args[key];
  }
  return arg;
}

ARGSTORE_LOOKUP(int);
ARGSTORE_LOOKUP(float);
ARGSTORE_LOOKUP_RETVAL(bool, _Bool);
ARGSTORE_LOOKUP_RETVAL(string, const char *);
ARGSTORE_LOOKUP_RETVAL(stringlist, const char **);

const Set *argstore_sources(const ArgStore *const store) {
  ASSERT(NOT_NULL(store));
  return &store->src_files;
}

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
                   char short_name, Arg arg_default) {
  ASSERT(NOT_NULL(config));
  const char *arg_name = intern(name);
  if (!map_insert(&config->arg_names, arg_name, (uint32_t *)key)) {
    FATALF("Argument key string '%s' already added.", arg_name);
  }
  if ('\0' != short_name) {
    if (ArgType__bool != arg_default.type) {
      FATALF("Arguments with short names (%s, %c) must be of type bool.", name,
             short_name);
    }
    const char *short_name_str = intern_range(&short_name, 0, 1);
    if (!map_insert(&config->arg_names, short_name_str, (uint32_t *)key)) {
      FATALF("Argument short key string '%s' already added.", short_name_str);
    }
  }
  if (config->args[key].used) {
    FATALF("Trying to map arg key to '%s', but is already used.", arg_name);
  }
  config->args[key] = arg_default;
}

void argconfig_set_mandatory_sources(ArgConfig *config, bool enabled) {
  config->mandatory_sources = enabled;
}

bool _parse_argument(const char *arg, Pair *pair) {
  if (strlen(arg) <= 2) {
    fprintf(stderr,
            "ERROR: Argument '%s' is malformed. Arguments must be at least 3 "
            "characters long.\n",
            arg);
    return false;
  }
  if (0 != strncmp("--", arg, 2)) {
    fprintf(
        stderr,
        "ERROR: Argument '%s' is malformed. Arguments must start with a '-'.\n",
        arg);
    return false;
  }
  if (0 == strncmp("---", arg, 3)) {
    fprintf(stderr,
            "ERROR: Argument '%s' is malformed. Arguments must start with "
            "either 1 or 2 dashes.\n",
            arg);
    return false;
  }
  // Ex: --nobehappy
  if (0 == strncmp("--no", arg, 4)) {
    if (NULL != strchr(arg, '=')) {
      fprintf(stderr,
              "ERROR: Argument '%s' is malformed. Argumements prefixed with "
              "'--no' cannot be set equal to a value.\n",
              arg);
      return false;
    }
    pair->key = intern(arg + 4);
    pair->value = intern("false");
    return true;
  }
  char *eq = strchr(arg, '=');
  // Ex: --behappy
  if (NULL == eq) {
    pair->key = intern(arg + 2);
    pair->value = intern("true");
    return true;
  }
  // Ex: --behappy=true
  pair->key = intern_range(arg, 2, eq - arg);
  pair->value = intern_range(arg, eq - arg + 1, strlen(arg));
  return true;
}

void _parse_arguments(int argc, const char *const argv[], Map *args) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    Pair pair;
    if (!_parse_argument(arg, &pair)) {
      FATALF("Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
             "--arg1 --noarg2 --arg3=5]");
    }
    map_insert(args, pair.key, pair.value);
  }
}

bool _parse_compiler_argument(const char *arg, Map *args) {
  if (strlen(arg) <= 1) {
    fprintf(stderr,
            "ERROR: Argument '%s' is malformed. Compiler arguments must be at "
            "least 2 characters long.\n",
            arg);
    return false;
  }
  if ('-' != arg[0]) {
    fprintf(stderr,
            "ERROR: Argument '%s' is malformed. Compiler arguments must be "
            "prefixed by a '-'.\n",
            arg);
    return false;
  }
  if ('-' != arg[1]) {
    if (NULL != strchr(arg, '=')) {
      fprintf(stderr,
              "ERROR: Argument '%s' is malformed. Compiler argumements cannot "
              "be set equal to a value.\n",
              arg);
      return false;
    }
    int i;
    for (i = 1; i < strlen(arg); ++i) {
      char *key = intern_range(arg, i, i + 1);
      char *value = intern("1");
      map_insert(args, key, value);
    }
    return true;
  }
  Pair pair;
  if (!_parse_argument(arg, &pair)) {
    return false;
  }
  map_insert(args, pair.key, pair.value);
  return true;
}

void _parse_compiler_args(int argc, const char *const argv[], Map *args) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    if (!_parse_compiler_argument(arg, args)) {
      FATALF("Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
             "--arg1 --noarg2 --arg3=5]");
    }
  }
}

void _parse_sources(int argc, const char *const argv[], Set *sources) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    if ('-' == arg[0]) {
      fprintf(
          stderr,
          "ERROR: Source '%s' is malformed. Sources must not start with '-'\n",
          arg);
      FATALF("Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
             "--arg1 --noarg2 --arg3=5]");
    }
    set_insert(sources, intern(arg));
  }
}

int _num_args(int argc, const char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-' || 0 == strcmp(arg, "--")) {
      return i - 1;
    }
  }
  return argc - 1;
}

int _index_of_sources(int argc, const char *argv[]) {
  int i;
  for (i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    // -- is a special argument that indicates that subsequent arguments should
    // go to the program, not runner.
    if (0 == strcmp(arg, "--")) {
      return -1;
    }
    if (arg[0] != '-') {
      return i;
    }
  }
  return -1;
}

int _index_of_double_dash(int argc, const char *argv[]) {
  int i;
  for (i = 1; i < argc; ++i) {
    if (0 == strcmp(argv[i], "--")) {
      return i;
    }
  }
  return -1;
}

ArgStore *commandline_parse_args(ArgConfig *config, int argc,
                                 const char *argv[]) {
  ASSERT(NOT_NULL(config));
  ArgStore *store = argstore_create(config);
  Map args;
  map_init_default(&args);

  int num_args = _num_args(argc, argv);
  int index_of_sources = _index_of_sources(argc, argv);
  int index_of_dd = _index_of_double_dash(argc, argv);

  _parse_compiler_args(num_args, argv + 1, &args);
  M_iter args_iter = map_iter(&args);
  for (; has(&args_iter); inc(&args_iter)) {
    const char *k = _CONST_CHAR_POINTER(key(&args_iter));
    const char *v = _CONST_CHAR_POINTER(value(&args_iter));
    ArgKey arg_key = (ArgKey)map_lookup(&config->arg_names, k);
    if (ArgKey__NONE == arg_key) {
      FATALF("Unknown Arg name: %s", k);
    }
    Arg arg = config->args[arg_key];
    if (ArgType__none == arg.type) {
      FATALF("Unknown Arg type: %s", k);
    }
    store->args[arg_key] = arg_parse(arg.type, v);
    if (store->args[arg_key].type != arg.type) {
      FATALF(
          "ArgType of input does not match preset type. Was: %d, Expected: %d.",
          store->args[arg_key].type, arg.type);
    }
  }
  map_finalize(&args);

  if (index_of_dd > 0) {
    _parse_arguments(argc - index_of_dd - 1, argv + index_of_dd + 1,
                     &store->program_args);
  } else {
    index_of_dd = argc;
  }

  if (index_of_sources >= 0) {
    _parse_sources(index_of_dd - index_of_sources, argv + index_of_sources,
                   &store->src_files);
  }

  const bool version = argstore_lookup_bool(store, ArgKey__VERSION);
  if (version) {
    printf("%s@%s\n", version_string(), version_timestamp_string());
    exit(EXIT_SUCCESS);
  }

  if (config->mandatory_sources && index_of_sources < 0) {
    fprintf(stderr, "ERROR: No sources.\n");
    FATALF("Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
           "--arg1 --noarg2 --arg3=5]");
  }

  return store;
}

const Map *argstore_program_args(const ArgStore *store) {
  return &store->program_args;
}