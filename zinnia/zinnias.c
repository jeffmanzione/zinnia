#include <stdio.h>
#include <stdlib.h>

#include "zinnia/seed/seed.h"

#define NUM_NON_MODULE_ARGS 3

void read_arg_(const char arg[], struct zinnia_module_info *module_info) {
  const char *window_start = arg, *window_end = NULL;

  window_end = strchr(window_start, ':');
  module_info->module_name = strndup(window_start, window_end - window_start);

  window_start = window_end + 1;
  window_end = strchr(window_start, ':');
  module_info->source_input_filepath =
      strndup(window_start, window_end - window_start);

  window_start = window_end + 1;
  window_end = strchr(window_start, ':');

  if (window_end == NULL) {
    // No DLLs included.
    module_info->source_output_filepath = strdup(window_start);
    module_info->dll_input_filepath = NULL;
    module_info->dll_output_filepath = NULL;
    module_info->dll_init_fn = NULL;
  } else {
    module_info->source_output_filepath =
        strndup(window_start, window_end - window_start);

    window_start = window_end + 1;
    window_end = strchr(window_start, ':');
    module_info->dll_input_filepath =
        strndup(window_start, window_end - window_start);

    window_start = window_end + 1;
    window_end = strchr(window_start, ':');
    module_info->dll_output_filepath =
        strndup(window_start, window_end - window_start);

    window_start = window_end + 1;
    module_info->dll_init_fn = strdup(window_start);
  }
}

void free_arg_(struct zinnia_module_info *module_info) {
  free((char *)module_info->module_name);
  free((char *)module_info->source_input_filepath);
  free((char *)module_info->source_output_filepath);
  if (module_info->dll_input_filepath != NULL) {
    free((char *)module_info->dll_input_filepath);
    free((char *)module_info->dll_output_filepath);
    free((char *)module_info->dll_init_fn);
  }
}

// zinnias
//   seed_name
//   out/path/to/seed_name
//   mod1:path/to/mod1/mod1.zn:zip_path/to/mod1/mod1.zn
//   mod2:path/to/mod2/mod2.zn:zip_path/to/mod2.mod2.zn:path/to/mod2/mod2.dll:zip_path/to/mod2/mod2.dll
int main(int argc, const char *argv[]) {
  if (argc <= NUM_NON_MODULE_ARGS) {
    fprintf(stderr, "Too few arguments (were %d).\n", argc - 1);
    return EXIT_FAILURE;
  }

  const char *seed_name = argv[1];
  const char *seed_filepath = argv[2];

  // Subtract all non-module args.
  const int num_modules = argc - NUM_NON_MODULE_ARGS;

  struct zinnia_module_info *module_infos =
      calloc(sizeof(struct zinnia_module_info), num_modules);

  for (int i = 0; i < argc - NUM_NON_MODULE_ARGS; ++i) {
    read_arg_(argv[NUM_NON_MODULE_ARGS + i], &module_infos[i]);
  }

  create_znseed_file(seed_name, num_modules, module_infos, seed_filepath);

#ifdef DEBUG
  for (int i = 0; i < argc - NUM_NON_MODULE_ARGS; ++i) {
    free_arg_(&module_infos[i]);
  }
  free(module_infos);
#endif

  return EXIT_SUCCESS;
}