#include "seed/seed.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zip.h>

#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"

typedef void dll_handle;
typedef zip_t znseed_t;

struct manifest_row {
  char *module_name;
  char *source_filepath;
  char *dll_filepath;
  char *dll_init_fn;
};

#define MANIFEST_FILENAME (const char *)"manifest"

#define PERRORF(perror_msg, fmt, ...)  \
  {                                    \
    fprintf(stderr, fmt, __VA_ARGS__); \
    perror(perror_msg);                \
  }

// int main(int argc, char *argv[]) {
//   // const char *seed_name = argv[1];
//   // const char *seed_filepath = argv[2];
//   // const char *source_file = argv[3];
//   // const char *dll_file = argv[4];
//   // const char *dll_init_fn = argv[5];

//   // const char seed_name[] = "dynamic";
//   // const char seed_filepath[] ="dynamic.znseed";
//   const char *seed_filepath = argv[1];

//   // struct zinnia_module_info infos[] = {
//   //     {
//   //         .module_name = "examples/dynamic/custom",
//   //         .source_input_filepath = "examples/dynamic/custom.zn",
//   //         .source_output_filepath = "examples/dynamic/custom.zn",
//   //         .dll_input_filepath = "bazel-bin/examples/dynamic/_custom.dll",
//   //         .dll_output_filepath = "examples/dynamic/custom.dll",
//   //         .dll_init_fn = "_init_custom",
//   //     },
//   //     {
//   //         .module_name = "examples/dynamic/dynamic",
//   //         .source_input_filepath = "examples/dynamic/dynamic.zn",
//   //         .source_output_filepath = "examples/dynamic/dynamic.zn",
//   //         .dll_input_filepath = NULL,
//   //         .dll_output_filepath = NULL,
//   //         .dll_init_fn = NULL,
//   //     },
//   // };

//   // create_znseed_file(seed_name, sizeof(infos) / sizeof(infos[0]), infos,
//   //                    seed_filepath);

//   char error_buf[255];
//   if (!load_znseed_file(NULL, seed_filepath, error_buf)) {
//     fprintf(stderr, "%s\n", error_buf);
//     return EXIT_FAILURE;
//   }

//   return EXIT_SUCCESS;
// }

void write_file_to_seed_(const char input_filepath[],
                         const char output_filepath[], znseed_t *seed) {
  zip_source_t *src = zip_source_file(seed,
                                      /*fname=*/input_filepath,
                                      /*start=*/0,
                                      /*end=*/ZIP_LENGTH_TO_END);
  if (src == NULL) {
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s' --> '%s': %s\n",
            input_filepath, output_filepath, zip_strerror(seed));
  }
  if (zip_file_add(seed, output_filepath, src,
                   ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(src);
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s' --> '%s': %s\n",
            input_filepath, output_filepath, zip_strerror(seed));
  }
}

void write_manifest_to_seed_(const char seed_name[], size_t num_modules,
                             const struct zinnia_module_info module_infos[],
                             znseed_t *seed) {
  FILE *tmp_manifest = tmpfile();

  fprintf(tmp_manifest, "%s\n", seed_name);

  for (int i = 0; i < num_modules; ++i) {
    const struct zinnia_module_info *info = &module_infos[i];
    if (info->dll_output_filepath != NULL && info->dll_init_fn != NULL) {
      fprintf(tmp_manifest, "%s:%s:%s:%s\n", info->module_name,
              info->source_output_filepath, info->dll_output_filepath,
              info->dll_init_fn);
    } else {
      fprintf(tmp_manifest, "%s:%s\n", info->module_name,
              info->source_output_filepath);
    }
  }

  rewind(tmp_manifest);

  zip_source_t *src = zip_source_filep(seed,
                                       /*fname=*/tmp_manifest,
                                       /*start=*/0,
                                       /*end=*/ZIP_LENGTH_TO_END);

  if (src == NULL) {
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s': %s\n",
            MANIFEST_FILENAME, zip_strerror(seed));
  }

  if (zip_file_add(seed, MANIFEST_FILENAME, src,
                   ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(src);
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s': %s\n",
            MANIFEST_FILENAME, zip_strerror(seed));
  }
}

void create_znseed_file(const char seed_name[], size_t num_modules,
                        const struct zinnia_module_info module_infos[],
                        const char seed_filepath[]) {
  int errorp;
  znseed_t *seed = zip_open(seed_filepath, ZIP_CREATE | ZIP_TRUNCATE, &errorp);

  if (seed == NULL) {
    zip_error_t error;
    zip_error_init_with_code(&error, errorp);
    PERRORF(zip_error_strerror(&error), "Cannot open seed '%s': %s\n",
            seed_filepath, zip_error_strerror(&error));
    zip_error_fini(&error);
  }

  write_manifest_to_seed_(seed_name, num_modules, module_infos, seed);

  for (int i = 0; i < num_modules; ++i) {
    const struct zinnia_module_info *info = &module_infos[i];
    write_file_to_seed_(info->source_input_filepath,
                        info->source_output_filepath, seed);
    if (info->dll_input_filepath != NULL && info->dll_output_filepath != NULL &&
        info->dll_init_fn != NULL) {
      write_file_to_seed_(info->dll_input_filepath, info->dll_output_filepath,
                          seed);
    }
  }
  if (zip_close(seed) < 0) {
    PERRORF(zip_strerror(seed), "Failed to close seed '%s': %s", seed_filepath,
            zip_strerror(seed));
  }
}

bool copy_zip_file_to_stream_(znseed_t *seed, const char filepath[], FILE *out,
                              char *error_buf) {
  // Stat to get the size of the file.
  zip_stat_t stat;
  zip_stat_init(&stat);
  if (zip_stat(seed, filepath, /*flags=*/0, &stat) < 0) {
    sprintf(error_buf, "Failed to stat file in seed %s: %s", filepath,
            zip_strerror(seed));
    return false;
  }

  zip_file_t *file = zip_fopen(seed, filepath, /*flags=*/0);
  if (file == NULL) {
    sprintf(error_buf, "Failed to open file in seed %s: %s", filepath,
            zip_strerror(seed));
    return false;
  }

  char *buffer = (char *)malloc(stat.size);
  if (!buffer) {
    sprintf(error_buf, "Failed to allocate memory for file in seed %s: %s",
            filepath, zip_strerror(seed));
    zip_fclose(file);
    return false;
  }

  zip_fread(file, buffer, stat.size);
  fwrite(buffer, 1, stat.size, out);

  free(buffer);
  zip_fclose(file);

  return true;
}

bool copy_zip_file_to_file_(znseed_t *seed, const char seed_filepath[],
                            const char out_filepath[], char *error_buf) {
  FILE *out = fopen(out_filepath, "wb");

  if (out == NULL) {
    sprintf(error_buf, "Failed to open file for writing: %s", out_filepath);
    return false;
  }

  const bool result =
      copy_zip_file_to_stream_(seed, seed_filepath, out, error_buf);

  fclose(out);

  return result;
}

dll_handle *open_dll_from_seed_(znseed_t *seed, const char seed_dll_filepath[],
                                char *error_buf) {
  char *path, *filename, *ext;
  split_path_file(seed_dll_filepath, &path, &filename, &ext);

  char *tmp_filename = combine_path_file("/tmp", "tmp_XXXXXX", ext);

  int tmp_fd = mkstemps(tmp_filename, strlen(ext));

  free(path);
  free(filename);
  free(ext);

  if (tmp_fd < 0) {
    sprintf(error_buf, "Failed to create tmp file: %s", tmp_filename);
    free(tmp_filename);
    unlink(tmp_filename);
    return NULL;
  }

  FILE *tmp_ddl_file = fdopen(tmp_fd, "wb+");

  if (!copy_zip_file_to_stream_(seed, seed_dll_filepath, tmp_ddl_file,
                                error_buf)) {
    unlink(tmp_filename);
    fclose(tmp_ddl_file);
    close(tmp_fd);
    remove(tmp_filename);
    free(tmp_filename);
    return NULL;
  }

  // Flush written data and close.
  fclose(tmp_ddl_file);

  dll_handle *dll_handle = dlopen(tmp_filename, RTLD_LAZY);

  // Must occur after dlopen() because if closed before, the file can be
  // tampered.
  close(tmp_fd);

  if (unlink(tmp_filename) < 0) {
    sprintf(error_buf, "Failed to unlink temporary file copy of %s: %s",
            seed_dll_filepath, tmp_filename);
    free(tmp_filename);
    return NULL;
  }

  free(tmp_filename);
  return dll_handle;
}

char *parse_manifest_line_(char *seg_start, struct manifest_row *row) {
  char *seg_end = strchr(seg_start, ':');
  row->module_name = strndup(seg_start, seg_end - seg_start);
  seg_start = seg_end + 1;

  seg_end = strpbrk(seg_start, ":\n");
  row->source_filepath = strndup(seg_start, seg_end - seg_start);
  seg_start = seg_end + 1;

  if (*seg_end == ':') {
    seg_end = strchr(seg_start, ':');
    row->dll_filepath = strndup(seg_start, seg_end - seg_start);
    seg_start = seg_end + 1;

    seg_end = strchr(seg_start, '\n');
    row->dll_init_fn = strndup(seg_start, seg_end - seg_start);
  } else {
    row->dll_filepath = NULL;
    row->dll_init_fn = NULL;
  }

  // printf("%s:%s:%s:%s\n", row->module_name, row->source_filepath,
  //        row->dll_filepath == NULL ? "(none)" : row->dll_filepath,
  //        row->dll_init_fn == NULL ? "(none)" : row->dll_init_fn);

  // Skip over newline.
  return seg_end + 1;
}

void free_manifest_line_(struct manifest_row *row) {
  free(row->module_name);
  free(row->source_filepath);
  if (row->dll_filepath != NULL && row->dll_init_fn != NULL) {
    free(row->dll_filepath);
    free(row->dll_init_fn);
  }
}

int read_entire_file_from_znseed_(znseed_t *seed, const char filepath[],
                                  char **target, char *error_buf) {
  zip_stat_t stat_buf;
  zip_stat_init(&stat_buf);
  if (zip_stat(seed, filepath, /*flags=*/0, &stat_buf) < 0) {
    sprintf(error_buf, "Failed to stat file in seed %s: %s", filepath,
            zip_strerror(seed));
    *target = NULL;
    return false;
  }

  zip_file_t *file = zip_fopen(seed, filepath, /*flags=*/0);
  if (file == NULL) {
    sprintf(error_buf, "Failed to open file in seed %s: %s", filepath,
            zip_strerror(seed));
    *target = NULL;
    return -1;
  }

  const zip_int16_t file_len = stat_buf.size;

  char *file_buf = malloc(file_len + 1);

  if (file_buf == NULL) {
    sprintf(error_buf, "Failed to allocate memory for file in seed %s",
            filepath);
    zip_fclose(file);
    *target = NULL;
    return -1;
  }

  zip_int64_t bytes_read = zip_fread(file, file_buf, file_len);
  if (bytes_read != file_len) {
    sprintf(error_buf, "Failed to fread file in seed %s: %s", filepath,
            zip_strerror(seed));
    free(file_buf);
    zip_fclose(file);
    *target = NULL;
    return -1;
  }

  zip_fclose(file);
  file_buf[file_len] = 0x0;
  *target = file_buf;
  return file_len;
}

bool load_znseed_file(VM *vm, const char seed_filepath[], char *error_buf) {
  int errorp;
  znseed_t *seed = zip_open(seed_filepath, ZIP_RDONLY, &errorp);

  if (seed == NULL) {
    zip_error_t error;
    zip_error_init_with_code(&error, errorp);
    sprintf(error_buf, "Cannot open seed '%s': %s", seed_filepath,
            zip_error_strerror(&error));
    zip_error_fini(&error);
    return false;
  }

  char *file_buf;
  const int file_len = read_entire_file_from_znseed_(seed, MANIFEST_FILENAME,
                                                     &file_buf, error_buf);
  if (file_len < 0) {
    zip_close(seed);
    return false;
  }

  char *seg_start;
  // Skip first line with seed name.
  char *seg_end = strchr(file_buf, '\n');
  seg_start = seg_end + 1;

  ModuleManager *mm = vm_module_manager(vm);

  do {
    struct manifest_row line;
    seg_start = parse_manifest_line_(seg_start, &line);

    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat(seed, line.source_filepath, /*flags=*/0, &stat) < 0) {
      FATALF("Failed to stat file in seed %s: %s", line.source_filepath,
             zip_strerror(seed));
    }

    char *source_file_content;
    const int source_len = read_entire_file_from_znseed_(
        seed, line.source_filepath, &source_file_content, error_buf);
    if (source_len < 0) {
      FATALF("Error loading znseed file %s: %s", line.source_filepath,
             error_buf);
    }
    // TODO: Deal with freeing soure content and segs somewhere.
    const char **source_segs = (const char **)malloc(sizeof(char *));
    source_segs[0] = source_file_content;

    void *dll = NULL;
    if (line.dll_filepath != NULL && line.dll_init_fn != NULL) {
      char error_buf[255];  // TODO: Set a resonable size.
      error_buf[0] = '\0';
      dll = open_dll_from_seed_(seed, line.dll_filepath, error_buf);
      if (strlen(error_buf) > 0) {
        FATALF("Error loading znseed: %s", error_buf);
      }
    }

    mm_register_module_with_callback(
        mm, line.source_filepath, line.source_filepath, source_segs, 1,
        dll ? (NativeCallback)dlsym(dll, line.dll_init_fn) : NULL);
    // Forces module to be loaded eagerly.
    modulemanager_lookup(mm, line.module_name);

  } while ((seg_start - file_buf) < file_len);

  free(file_buf);
  zip_close(seed);
  return true;
}

bool copy_escaped_znseed_to_tmpfile(const char escaped_binary_content[],
                                    TmpSeedFile *tmp_seed) {
  char filename[] = "/tmp/tmp_XXXXXX" ZNSEED_EXTENSION;
  tmp_seed->fd = mkstemps(filename, strlen(ZNSEED_EXTENSION));
  tmp_seed->filename = strdup(filename);
  tmp_seed->file = fdopen(tmp_seed->fd, "wb+");

  char *unesc_binary;
  const int bytes_written = unescape(escaped_binary_content, &unesc_binary);

  fwrite(unesc_binary, 1, bytes_written, tmp_seed->file);
  free(unesc_binary);
  rewind(tmp_seed->file);
  // TODO: Check at each stage for success.
  return true;
}

void znseed_tmp_close(TmpSeedFile *tmp_seed) {
  fclose(tmp_seed->file);
  close(tmp_seed->fd);
  unlink(tmp_seed->filename);
  free(tmp_seed->filename);
}
