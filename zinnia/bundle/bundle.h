#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_BUNDLE_BUNDLE_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_BUNDLE_BUNDLE_H_

#include <stdbool.h>
#include <stddef.h>

#include "zinnia/vm/vm.h"

#define ZNBUNDLE_EXTENSION ".znbundle"
// Deprecated: bundles were previously called seeds.
#define ZNSEED_EXTENSION ".znseed"

// Returns whether filepath names a bundle (.znbundle or legacy .znseed).
bool is_bundle_file(const char filepath[]);

struct zinnia_module_info {
  // Name of the module.
  const char *module_name;
  // Path of input source file.
  const char *source_input_filepath;
  // Path of output source file in bundle.
  const char *source_output_filepath;
  // Path of input dll file.
  const char *dll_input_filepath;
  // Path of output dll file in bundle.
  const char *dll_output_filepath;
  // Name of initialization function in the dll.
  const char *dll_init_fn;
};

// Creates a .znbundle (ZIP) file for a given .zn library and C program.
//
// If bundle_name is "mybundle", the following would be created:
//   mybundle.znbundle
//     ∟ manifest
//     ∟ mybundle.zn
//     ∟ mybundle.so
//     ∟ mybundle_helper.zn
void create_znbundle_file(const char bundle_name[], size_t num_modules,
                          const struct zinnia_module_info module_infos[],
                          const char bundle_filepath[]);

bool load_znbundle_file(VM *vm, const char bundle_filepath[], char *error_buf);

typedef struct {
  char *filename;
} TmpBundleFile;

bool copy_znbundle_bytes_to_tmpfile(const unsigned char binary_content[],
                                    size_t content_size,
                                    TmpBundleFile *tmp_bundle);

void znbundle_tmp_finalize(TmpBundleFile *tmp_bundle);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_BUNDLE_BUNDLE_H_ */