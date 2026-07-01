#include "zinnia/package/package.h"

#include <stdio.h>
#include <stdlib.h>

#include "c-data-structures/stable_maplike.h"
#include "file-utils/file_info.h"
#include "file-utils/file_utils.h"
#include "file-utils/string_utils.h"
#include "zinnia/alloc/alloc.h"
#include "zinnia/compile/compile.h"
#include "zinnia/program/optimization/optimize.h"
#include "zinnia/seed/seed.h"
#include "zinnia/util/codegen.h"
#include "zinnia/util/error.h"
#include "zinnia/util/string_util.h"
#include "zinnia/util/void_array.h"
#include "zinnia/version/version.h"
#include "zinnia/vm/intern.h"

#define MAX_VAR_NAME_LEN 127

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

bool extract_file_(const char input_file_name[], char **dir_path,
                   char **file_base, char **ext, char **escaped_input) {
  split_path_file(input_file_name, dir_path, file_base, ext);
  FILE *input_file = FILE_FN(input_file_name, "rb");
  if (NULL == input_file) {
    fprintf(stderr, "Could not open file: \"%s\".", input_file_name);
    return false;
  }
  char *input;
  getall(input_file, &input);
  escape(input, escaped_input);
  RELEASE(input);
  return true;
}

size_t compile_to_string_(const char file_name[], char **target) {
  if (ends_with(file_name, ".zn")) {
    FILE *assembly_file = tmpfile();
    compile_to_assembly(file_name, assembly_file);
    rewind(assembly_file);
    return getall(assembly_file, target);

  } else if (ends_with(file_name, ".zna") ||
             ends_with(file_name, ZNSEED_EXTENSION)) {
    FILE *file = FILE_FN(file_name, "rb");
    if (NULL == file) {
      FATALF("File does not exist: %s", file_name);
    }
    return getall(file, target);

  } else {
    FATALF("Uknown file type: %s", file_name);
    *target = NULL;
    return 0;
  }
}

typedef struct {
  char *src;
  char *init_fn;
} NativeModuleInfo_;

DEFINE_STABLE_MAPLIKE(NativeModuleInfoMap, char *, NativeModuleInfo_);
IMPL_STABLE_MAPLIKE(NativeModuleInfoMap, char *, NativeModuleInfo_);

void populate_native_modules_(FILE *file, NativeModuleInfoMap *map,
                              CharPtrSet *hdrs) {
  FileInfo *fi = file_info_file(file);
  LineInfo *li;
  while (NULL != (li = file_info_getline(fi))) {
    char *line = strdup(li->line_text);
    if (ends_with(line, "\n")) {
      line[strlen(line) - 1] = '\0';
    }
    char *first_colon = find_str(line, strlen(line), ":", strlen(":"));
    char *second_colon =
        find_str(first_colon + 1, strlen(line), ":", strlen(":"));
    NativeModuleInfo_ *m;
    const char *src = global_intern_range(line, 0, first_colon - line);
    NativeModuleInfoMap_insert(map, src, sizeof(char *), &m);
    m->src = (char *)src;
    m->init_fn = (char *)global_intern_range(line, first_colon - line + 1,
                                             (second_colon - first_colon) - 1);
    char *prev_comma = second_colon, *comma = NULL;
    while (NULL != (comma = find_str(prev_comma + 1, strlen(prev_comma + 1),
                                     ",", strlen(",")))) {
      CharPtrSet_insert(hdrs,
                        global_intern_range(prev_comma, 1, comma - prev_comma),
                        sizeof(char *));
      prev_comma = comma;
    }
    CharPtrSet_insert(hdrs,
                      global_intern_range(prev_comma, 1, strlen(prev_comma)),
                      sizeof(char *));

    free(line);
  }
  file_info_delete(fi);
}

bool is_alphanumeric_(const char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

char *convert_lib_path_to_var_name_(const char lib_path[]) {
  char *var_name = ALLOC_STRNDUP(lib_path, strlen(lib_path));
  for (int i = 0; i < strlen(var_name); ++i) {
    const char c = var_name[i];
    if (!is_alphanumeric_(c)) {
      var_name[i] = '_';
    }
  }
  return var_name;
}

int zinniap(int argc, const char *argv[]) {
  if (0 == strcmp(argv[1], "--version")) {
    printf("%s@%s\n", version_string(), version_timestamp_string());
    return EXIT_SUCCESS;
  }

  strings_init();
  optimize_init();

  const char *out_file_name = argv[1];
  FILE *out = FILE_FN(out_file_name, "w");
  if (NULL == out) {
    fprintf(stderr, "Could not open file: \"%s\".\n", out_file_name);
    return EXIT_FAILURE;
  }

  const char *znmodules_file_name = argv[2];
  FILE *znmodules = FILE_FN(znmodules_file_name, "rb");
  if (NULL == znmodules) {
    fprintf(stderr, "Could not open file: \"%s\".\n", znmodules_file_name);
    return EXIT_FAILURE;
  }

  NativeModuleInfoMap native_modules;
  NativeModuleInfoMap_init(&native_modules, hash_interned_string,
                           compare_interned_strings);
  CharPtrSet extra_hdrs;
  CharPtrSet_init(&extra_hdrs, hash_interned_string, compare_interned_strings);

  populate_native_modules_(znmodules, &native_modules, &extra_hdrs);

  fprintf(out,
          "#include <stdlib.h>\n\n"
          "#include \"zinnia/alloc/alloc.h\"\n"
          "#include \"language-tools/intern.h\"\n"
          "#include \"language-tools/lexer/token.h\"\n"
          "#include \"zinnia/run/run.h\"\n"
          "#include \"zinnia/util/args/commandline.h\"\n"
          "#include \"zinnia/util/args/commandlines.h\"\n"
          "#include \"zinnia/util/void_array.h\"\n"
          "#include \"zinnia/vm/intern.h\"\n");

  CharPtrSetIterator extra_hdrs_it;
  CharPtrSet_iterator(&extra_hdrs_it, &extra_hdrs);
  for (; CharPtrSet_has_next(&extra_hdrs_it); CharPtrSet_next(&extra_hdrs_it)) {
    fprintf(out, "#include \"%s\"\n", *CharPtrSet_value(&extra_hdrs_it));
  }

  fprintf(out, "\n");

  for (int i = 3; i < argc; ++i) {
    const char *file_name = argv[i];
    if (NativeModuleInfoMap_contains(&native_modules, global_intern(file_name),
                                     sizeof(char *))) {
      continue;
    }

    const char *var_name = convert_lib_path_to_var_name_(file_name);
    char lib_var_name[MAX_VAR_NAME_LEN + 1];
    lib_var_name[0] = 0x0;
    sprintf(lib_var_name, "LIB_%s", var_name);

    char *content = NULL;
    const size_t content_size = compile_to_string_(file_name, &content);

    if (ends_with(file_name, ZNSEED_EXTENSION)) {
      DEBUGF("Processing seed: %s", file_name);
      print_data_as_char_array_var(lib_var_name, content, content_size, out);
    } else {
      DEBUGF("Processing source: %s", file_name);

      print_string_as_var_default(lib_var_name, content, out);
    }
    RELEASE(var_name);
    RELEASE(content);
  }

  NativeModuleInfoMapIterator it;
  NativeModuleInfoMap_iterator(&it, &native_modules);
  for (; NativeModuleInfoMap_has_entry(&it);
       NativeModuleInfoMap_next_entry(&it)) {
    const NativeModuleInfo_ *info = NativeModuleInfoMap_value(&it);
    const char *file_name = info->src;

    DEBUGF("Processing source: %s", file_name);
    char *dir_path, *file_base, *ext;
    split_path_file(file_name, &dir_path, &file_base, &ext);

    char *assembly = NULL;
    compile_to_string_(file_name, &assembly);
    const char *var_name = convert_lib_path_to_var_name_(file_name);

    char lib_var_name[MAX_VAR_NAME_LEN + 1];
    lib_var_name[0] = 0x0;
    sprintf(lib_var_name, "LIB_%s", var_name);
    print_string_as_var_default(lib_var_name, assembly, out);

    RELEASE(var_name);
    RELEASE(assembly);
  }

  fprintf(out,
          "int main(int argc, const char *argv[]) {\n"
          "  strings_init();\n"
          "  ArgConfig *config = argconfig_create();\n"
          "  argconfig_package(config);\n"
          "  ArgStore *store = commandline_parse_args(config, argc, argv);\n"
          "  CharPtrArray srcs;\n"
          "  CharPtrArray_init(&srcs);\n"
          "  FilePartsArray src_contents;\n"
          "  FilePartsArray_init(&src_contents);\n"
          "  VoidPtrArray init_fns;\n"
          "  VoidPtrArray_init(&init_fns);\n");

  for (int i = 3; i < argc; ++i) {
    const char *file_name = argv[i];
    if (NativeModuleInfoMap_contains(&native_modules, global_intern(file_name),
                                     sizeof(char *))) {
      continue;
    }

    // if (ends_with(file_name, ZNSEED_EXTENSION)) {
    //   DEBUGF("Processing seed: %s", file_name);
    //   continue;
    // }
    const bool is_seed = ends_with(file_name, ZNSEED_EXTENSION);

    const char *type = is_seed ? "FP_FILE_SEED" : "FP_FILE_SOURCE";

    char *dir_path, *file_base, *ext;
    split_path_file(file_name, &dir_path, &file_base, &ext);
    char *escaped_dir_path;
    escape(dir_path, &escaped_dir_path);

    const char *lib_var_name = convert_lib_path_to_var_name_(file_name);

    fprintf(out, "  CharPtrArray_push_back(&srcs, \"%s%s.zna\");\n",
            escaped_dir_path, file_base);

    if (is_seed) {
      fprintf(out,
              "  *FilePartsArray_push_back_ref(&src_contents) = (FileParts){ "
              ".type = %s,\n.seed_data = { .content_bytes = LIB_%s, "
              ".content_size = sizeof(LIB_%s)}};\n",
              type, lib_var_name, lib_var_name);
    } else {
      fprintf(out,
              "  *FilePartsArray_push_back_ref(&src_contents) = (FileParts){ "
              ".type = %s,\n"
              ".source = { .parts = LIB_%s, .num_parts = sizeof(LIB_%s) / "
              "sizeof(LIB_%s[0])}};\n",
              type, lib_var_name, lib_var_name, lib_var_name);
    }
    fprintf(out, "  VoidPtrArray_push_back(&init_fns, NULL);\n");

    RELEASE(lib_var_name);
    RELEASE(escaped_dir_path);
    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
  }

  NativeModuleInfoMap_iterator(&it, &native_modules);
  for (; NativeModuleInfoMap_has_entry(&it);
       NativeModuleInfoMap_next_entry(&it)) {
    const NativeModuleInfo_ *m = NativeModuleInfoMap_value(&it);

    const char *var_name = convert_lib_path_to_var_name_(m->src);

    char *dir_path, *file_base, *ext;
    split_path_file(m->src, &dir_path, &file_base, &ext);
    char *escaped_dir_path;
    escape(dir_path, &escaped_dir_path);
    fprintf(out, "  CharPtrArray_push_back(&srcs, \"%s%s.zna\");\n",
            escaped_dir_path, file_base);
    fprintf(out,
            "  *FilePartsArray_push_back_ref(&src_contents) = (FileParts){ "
            ".type = FP_FILE_SOURCE,\n"
            ".source = { .parts = LIB_%s, .num_parts = sizeof(LIB_%s) / "
            "sizeof(LIB_%s[0])}};\n",
            var_name, var_name, var_name);
    fprintf(out, "  VoidPtrArray_push_back(&init_fns,  (void *) %s);\n",
            m->init_fn);

    RELEASE(var_name);
    RELEASE(escaped_dir_path);
    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
  }

  fprintf(out,
          "  run_files(&srcs, &src_contents, &init_fns, store);\n"
          "#ifdef DEBUG\n "
          "  CharPtrArray_finalize(&srcs);\n"
          "  FilePartsArray_finalize(&src_contents);\n"
          "  VoidPtrArray_finalize(&init_fns);\n"
          "  argstore_delete(store);\n"
          "  argconfig_delete(config);\n"
          "  token_finalize_all();\n"
          "  strings_finalize();\n"
          "#endif \n "
          "  return EXIT_SUCCESS;\n"
          "}\n");

#ifdef DEBUG
  NativeModuleInfoMap_finalize(&native_modules);

  CharPtrSet_finalize(&extra_hdrs);

  optimize_finalize();
  strings_finalize();
  token_finalize_all();
#endif

  return EXIT_SUCCESS;
}