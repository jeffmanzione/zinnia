#ifndef SEED_SEED_H_
#define SEED_SEED_H_

#include <stdbool.h>
#include <stddef.h>

#include "vm/vm.h"

#define ZNSEED_EXTENSION ".znseed"

struct zinnia_module_info {
  // Name of the module.
  const char *module_name;
  // Path of input source file.
  const char *source_input_filepath;
  // Path of output source file in seed.
  const char *source_output_filepath;
  // Path of input dll file.
  const char *dll_input_filepath;
  // Path of output dll file in seed.
  const char *dll_output_filepath;
  // Name of initialization function in the dll.
  const char *dll_init_fn;
};

// Creates a .znseed (ZIP) file for a given .zn library and C program.
//
// If seed_name is "myseed", the following would be created:
//   myseed.znseed
//     ∟ manifest
//     ∟ myseed.zn
//     ∟ myseed.so
//     ∟ myseed_helper.zn
void create_znseed_file(const char seed_name[], size_t num_modules,
                        const struct zinnia_module_info module_infos[],
                        const char seed_filepath[]);

bool load_znseed_file(VM *vm, const char seed_filepath[], char *error_buf);

typedef struct {
  FILE *file;
  int fd;
  char *filename;
} TmpSeedFile;

bool copy_escaped_znseed_to_tmpfile(const char escaped_binary_content[],
                                    TmpSeedFile *tmp_seed);

void znseed_tmp_close(TmpSeedFile *tmp_seed);

#endif /* SEED_SEED_H_ */