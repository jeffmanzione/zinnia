#include "zinnia/seed/seed.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zip.h>

#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "zinnia/util/dll.h"
#include "zinnia/util/string_util.h"

typedef void dll_handle;
typedef zip_t znseed_t;

struct manifest_row {
  char *module_name;
  char *source_filepath;
  char *dll_filepath;
  char *dll_init_fn;
};

#define MANIFEST_FILENAME (const char *)"manifest"

#define PERRORF(perror_msg, fmt, ...)    \
  {                                      \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    perror(perror_msg);                  \
    exit(-1);                            \
  }

void write_file_to_seed_(const char input_filepath[],
                         const char output_filepath[], znseed_t *seed) {
  zip_source_t *src = zip_source_file(seed,
                                      /*fname=*/input_filepath,
                                      /*start=*/0,
                                      /*end=*/ZIP_LENGTH_TO_END);
  if (src == NULL) {
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s' --> '%s'\n",
            input_filepath, output_filepath);
  }
  zip_stat_t stat;
  zip_stat_init(&stat);
  if (zip_source_stat(src, &stat) < 0) {
    zip_error_t *err = zip_source_error(src);
    PERRORF(zip_error_strerror(err), "Failed to stat source: %s\n",
            input_filepath);
  }
  if (zip_file_add(seed, output_filepath, src,
                   ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(src);
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s' --> '%s'\n",
            input_filepath, output_filepath);
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
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s'\n",
            MANIFEST_FILENAME);
  }

  zip_stat_t stat;
  zip_stat_init(&stat);
  if (zip_source_stat(src, &stat) < 0) {
    zip_error_t *err = zip_source_error(src);
    PERRORF(zip_error_strerror(err), "Failed to stat source: %s\n",
            MANIFEST_FILENAME);
  }

  if (zip_file_add(seed, MANIFEST_FILENAME, src,
                   ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
    zip_source_free(src);
    PERRORF(zip_strerror(seed), "Error adding file to seed '%s'\n",
            MANIFEST_FILENAME);
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
    PERRORF(zip_error_strerror(&error), "Cannot open seed '%s'\n",
            seed_filepath);
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
    PERRORF(zip_strerror(seed), "Failed to close seed '%s'\n", seed_filepath);
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

  FILE *tmp_ddl_file = fdopen(tmp_fd, "wb");

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

  dll_handle *dll_handle;
  if (!load_dynamic_library(tmp_filename, &dll_handle, error_buf)) {
    close(tmp_fd);
    free(tmp_filename);
    return NULL;
  }

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
      error_buf[0] = 0x0;
      dll = open_dll_from_seed_(seed, line.dll_filepath, error_buf);
      if (strlen(error_buf) > 0) {
        FATALF("Error loading znseed: %s", error_buf);
      }
    }

    void *init_fn_handle = NULL;
    if (dll && !load_dynamic_function(dll, line.dll_init_fn, &init_fn_handle,
                                      error_buf)) {
      // Should do something probably.
      FATALF("Error loading znseed: %s", error_buf);
    }

    mm_register_module_with_callback2(
        mm, line.source_filepath, line.source_filepath, source_segs, 1,
        (NativeModuleBuilderInitFn)init_fn_handle);
    // Forces module to be loaded eagerly.
    modulemanager_lookup(mm, line.module_name);

  } while ((seg_start - file_buf) < file_len);

  free(file_buf);
  zip_close(seed);
  return true;
}

bool copy_znseed_bytes_to_tmpfile(const unsigned char binary_content[],
                                  size_t content_size, TmpSeedFile *tmp_seed) {
  char filename[] = "/tmp/tmp_XXXXXX" ZNSEED_EXTENSION;
  int fd = mkstemps(filename, strlen(ZNSEED_EXTENSION));
  tmp_seed->filename = strdup(filename);
  FILE *file = fdopen(fd, "wb+");

  fwrite(binary_content, sizeof(char), content_size, file);
  fclose(file);
  close(fd);
  // TODO: Check at each stage for success.
  return true;
}

void znseed_tmp_finalize(TmpSeedFile *tmp_seed) {
  unlink(tmp_seed->filename);
  free(tmp_seed->filename);
}
