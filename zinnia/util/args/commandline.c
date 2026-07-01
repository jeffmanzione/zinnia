// commandline.c
//
// Created on: May 28, 2018
//     Author: Jeff

#include "zinnia/util/args/commandline.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zinnia/alloc/alloc.h"
#include "zinnia/util/error.h"
#include "zinnia/util/void_array.h"
#include "zinnia/version/version.h"
#include "zinnia/vm/intern.h"

IMPL_SETLIKE(SourceNameSet, char *);
IMPL_MAPLIKE(ArgKeyMap, char *, ArgKey);
IMPL_MAPLIKE(ArgMap, char *, char *);

#define _CONST_CHAR_POINTER(ptr) ((const char *)(ptr))

#define ARGSTORE_LOOKUP_RETVAL(typet, retval)                              \
  retval argstore_lookup_##typet(const ArgStore *store, ArgKey key) {      \
    const Arg *arg = argstore_get(store, key);                             \
    if (!arg->used) {                                                      \
      FATALF("Store did not have key: %d", key);                           \
    }                                                                      \
    if (ArgType__##typet != arg->type) {                                   \
      FATALF("Expected a " #typet " for key=%d. Was %d.", key, arg->type); \
    }                                                                      \
    return (retval)arg->typet##_val;                                       \
  }

#define ARGSTORE_LOOKUP(typet) ARGSTORE_LOOKUP_RETVAL(typet, typet)

struct Argstore__ {
  const ArgConfig *config;
  Arg *args;
  SourceNameSet src_files;
  ArgMap program_args;
};

struct ArgConfig__ {
  ArgKeyMap arg_names;
  Arg *args;
  bool allow_compiler_args_and_sources;
};

ArgConfig *argconfig_create() {
  ArgConfig *config = MNEW(ArgConfig);
  config->args = CNEW_ARR(Arg, ArgKey__END);
  config->allow_compiler_args_and_sources = true;
  ArgKeyMap_init(&config->arg_names, hash_interned_string,
                 compare_interned_strings);
  argconfig_add(config, ArgKey__VERSION, "zinnia/version", 'v',
                arg_bool(false));
  return config;
}

void argconfig_delete(ArgConfig *config) {
  ASSERT(config != NULL);
  int i;
  for (i = 0; i < ArgKey__END; ++i) {
    if (ArgType__stringlist == config->args[i].type) {
      RELEASE(config->args[i].stringlist_val);
    }
  }
  RELEASE(config->args);
  ArgKeyMap_finalize(&config->arg_names);
  RELEASE(config);
}

ArgStore *argstore_create(const ArgConfig *config) {
  ASSERT(config != NULL);
  ArgStore *store = MNEW(ArgStore);
  store->config = config;
  store->args = CNEW_ARR(Arg, ArgKey__END);
  SourceNameSet_init(&store->src_files, hash_interned_string,
                     compare_interned_strings);
  ArgMap_init(&store->program_args, hash_interned_string,
              compare_interned_strings);
  return store;
}

void argstore_delete(ArgStore *store) {
  ASSERT(store != NULL);
  SourceNameSet_finalize(&store->src_files);
  ArgMap_finalize(&store->program_args);
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
  ASSERT(store != NULL);
  ASSERT(key > ArgKey__NONE);
  ASSERT(key < ArgKey__END);
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

const SourceNameSet *argstore_sources(const ArgStore *const store) {
  ASSERT(store != NULL);
  return &store->src_files;
}

void argconfig_add(ArgConfig *config, ArgKey key, const char name[],
                   char short_name, Arg arg_default) {
  ASSERT(config != NULL);
  const char *arg_name = global_intern(name);
  if (!ArgKeyMap_insert(&config->arg_names, arg_name, sizeof(char *), key)) {
    FATALF("Argument key string '%s' already added.", arg_name);
  }
  if ('\0' != short_name) {
    if (ArgType__bool != arg_default.type) {
      FATALF("Arguments with short names (%s, %c) must be of type bool.", name,
             short_name);
    }
    const char *short_name_str = global_intern_range(&short_name, 0, 1);
    if (!ArgKeyMap_insert(&config->arg_names, short_name_str, sizeof(char *),
                          key)) {
      FATALF("Argument short key string '%s' already added.", short_name_str);
    }
  }
  if (config->args[key].used) {
    FATALF("Trying to map arg key to '%s', but is already used.", arg_name);
  }
  config->args[key] = arg_default;
}

void argconfig_set_allow_compiler_args_and_sources(ArgConfig *config,
                                                   bool enabled) {
  config->allow_compiler_args_and_sources = enabled;
}

bool parse_argument_(const char *arg, ArgMapPair *pair) {
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
    pair->key = (char *)global_intern(arg + 4);
    pair->value = (char *)global_intern("false");
    return true;
  }
  char *eq = strchr(arg, '=');
  // Ex: --behappy
  if (NULL == eq) {
    pair->key = (char *)global_intern(arg + 2);
    pair->value = (char *)global_intern("true");
    return true;
  }
  // Ex: --behappy=true
  pair->key = (char *)global_intern_range(arg, 2, eq - arg - 2);
  pair->value = (char *)global_intern(eq + 1);
  return true;
}

void parse_arguments_(int argc, const char *const argv[], ArgMap *args) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    ArgMapPair pair;
    if (!parse_argument_(arg, &pair)) {
      FATALF(
          "Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
          "--arg1 --noarg2 --arg3=5]");
    }
    ArgMap_insert(args, pair.key, sizeof(char *), pair.value);
  }
}

bool parse_compiler_argument_(const char *arg, ArgMap *args) {
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
      const char *key = global_intern_range(arg, i, 1);
      const char *value = global_intern("1");
      ArgMap_insert(args, key, sizeof(char *), value);
    }
    return true;
  }
  ArgMapPair pair;
  if (!parse_argument_(arg, &pair)) {
    return false;
  }
  ArgMap_insert(args, pair.key, sizeof(char *), pair.value);
  return true;
}

void parse_compiler_args_(int argc, const char *const argv[], ArgMap *args) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    if (!parse_compiler_argument_(arg, args)) {
      FATALF(
          "Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
          "--arg1 --noarg2 --arg3=5]");
    }
  }
}

void parse_sources_(int argc, const char *const argv[],
                    SourceNameSet *sources) {
  int i;
  for (i = 0; i < argc; ++i) {
    const char *arg = argv[i];
    if ('-' == arg[0]) {
      fprintf(
          stderr,
          "ERROR: Source '%s' is malformed. Sources must not start with '-'\n",
          arg);
      FATALF(
          "Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
          "--arg1 --noarg2 --arg3=5]");
    }
    SourceNameSet_insert(sources, global_intern(arg), sizeof(char *));
  }
}

int num_args_(int argc, const char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-' || 0 == strcmp(arg, "--")) {
      return i - 1;
    }
  }
  return argc - 1;
}

int find_index_of_sources_(int argc, const char *argv[]) {
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

int find_index_of_double_dash_(int argc, const char *argv[]) {
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
  ASSERT(config != NULL);
  ArgStore *store = argstore_create(config);

  const int num_args = num_args_(argc, argv);
  const int sources_index = config->allow_compiler_args_and_sources
                                ? find_index_of_sources_(argc, argv)
                                : -1;
  int double_dash_index = config->allow_compiler_args_and_sources
                              ? find_index_of_double_dash_(argc, argv)
                              : 0;

  if (config->allow_compiler_args_and_sources) {
    ArgMap args;
    ArgMap_init(&args, hash_interned_string, compare_interned_strings);
    parse_compiler_args_(num_args, argv + 1, &args);
    ArgMapIterator args_iter;
    ArgMap_iterator(&args_iter, &args);
    for (; ArgMap_has_entry(&args_iter); ArgMap_next_entry(&args_iter)) {
      const char *k = ArgMap_key(&args_iter);
      const char *v = *ArgMap_value(&args_iter);
      ArgKey arg_key =
          ArgKeyMap_find(&config->arg_names, k, sizeof(char *), ArgKey__NONE);
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
            "ArgType of input does not match preset type. Was: %d, Expected: "
            "%d.",
            store->args[arg_key].type, arg.type);
      }
    }
    ArgMap_finalize(&args);
  }

  const bool version = argstore_lookup_bool(store, ArgKey__VERSION);
  if (version) {
    printf("%s@%s\n", version_string(), version_timestamp_string());
    exit(EXIT_SUCCESS);
  }

  if (!config->allow_compiler_args_and_sources || double_dash_index > 0) {
    const int num_args = argc - double_dash_index - 1;
    const char **args_start = argv + double_dash_index + 1;
    parse_arguments_(num_args, args_start, &store->program_args);
  } else {
    double_dash_index = argc;
  }

  if (config->allow_compiler_args_and_sources || sources_index >= 0) {
    const int num_sources = double_dash_index - sources_index;
    const char **sources_start = argv + sources_index;
    parse_sources_(num_sources, sources_start, &store->src_files);
  }

  if (config->allow_compiler_args_and_sources && sources_index < 0) {
    fprintf(stderr, "ERROR: No sources.\n");
    FATALF(
        "Could not parse arguments. Format: zinnia [-abc] d.zn e.zn [-- "
        "--arg1 --noarg2 --arg3=5]");
  }

  return store;
}

const ArgMap *argstore_program_args(const ArgStore *store) {
  return &store->program_args;
}