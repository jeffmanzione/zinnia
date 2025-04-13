#include "package/package.h"

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "alloc/arena/intern.h"
#include "compile/compile.h"
#include "debug/debug.h"
#include "program/optimization/optimize.h"
#include "struct/map.h"
#include "struct/struct_defaults.h"
#include "util/codegen.h"
#include "util/file/file_info.h"
#include "util/file/file_util.h"
#include "util/string.h"
#include "util/string_util.h"
#include "version/version.h"
#include "vm/intern.h"

#define MAX_VAR_NAME_LEN 127

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

bool _extract_file(const char input_file_name[], char **dir_path,
                   char **file_base, char **ext, char **escaped_input) {
  split_path_file(input_file_name, dir_path, file_base, ext);
  FILE *input_file = FILE_FN(input_file_name, "rb");
  if (NULL == input_file) {
    fprintf(stderr, "Could not open file: \"%s\".", input_file_name);
    return false;
  }
  char *input;
  getall(input_file, &input);
  *escaped_input = escape(input);
  RELEASE(input);
  return true;
}

char *_compile_to_string(const char file_name[]) {
  if (ends_with(file_name, ".zn")) {
    FILE *assembly_file = tmpfile();
    compile_to_assembly(file_name, assembly_file);

    rewind(assembly_file);

    char *content = NULL;
    getall(assembly_file, &content);
    return content;
  } else if (ends_with(file_name, ".zna")) {
    FILE *file = FILE_FN(file_name, "rb");
    if (NULL == file) {
      FATALF("File does not exist: %s", file_name);
    }
    char *content = NULL;
    getall(file, &content);
    return content;
  } else {
    FATALF("Uknown file type: %s", file_name);
    return NULL;
  }
}

typedef struct {
  char *src;
  char *init_fn;
} _NativeModuleInfo;

void _populate_native_modules(FILE *file, Map *map, Set *hdrs) {
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
    _NativeModuleInfo *m = MNEW(_NativeModuleInfo);
    m->src = intern_range(line, 0, first_colon - line);
    m->init_fn =
        intern_range(line, first_colon - line + 1, second_colon - line);
    map_insert(map, m->src, m);
    char *prev_comma = second_colon, *comma = NULL;
    while (NULL != (comma = find_str(prev_comma + 1, strlen(prev_comma + 1),
                                     ",", strlen(",")))) {
      set_insert(hdrs, intern_range(prev_comma, 1, comma - prev_comma));
      prev_comma = comma;
    }
    set_insert(hdrs, intern_range(prev_comma, 1, strlen(prev_comma)));

    RELEASE(line);
  }
  file_info_delete(fi);
}

bool _is_alphanumeric(const char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z');
}

char *_convert_lib_path_to_var_name(const char lib_path[]) {
  char *var_name = ALLOC_STRDUP(lib_path);
  for (int i = 0; i < strlen(var_name); ++i) {
    const char c = var_name[i];
    if (!_is_alphanumeric(c)) {
      var_name[i] = '_';
    }
  }
  return var_name;
}

int zinniap(int argc, const char *args[]) {
  if (0 == strcmp(args[1], "--version")) {
    printf("%s@%s\n", version_string(), version_timestamp_string());
    return EXIT_SUCCESS;
  }

  alloc_init();
  strings_init();
  optimize_init();

  const char *out_file_name = args[1];
  FILE *out = FILE_FN(out_file_name, "w");
  if (NULL == out) {
    fprintf(stderr, "Could not open file: \"%s\".\n", out_file_name);
    return EXIT_FAILURE;
  }

  const char *znmodules_file_name = args[2];
  FILE *znmodules = FILE_FN(znmodules_file_name, "rb");
  if (NULL == znmodules) {
    fprintf(stderr, "Could not open file: \"%s\".\n", znmodules_file_name);
    return EXIT_FAILURE;
  }

  Map native_modules;
  map_init_default(&native_modules);
  Set extra_hdrs;
  set_init_default(&extra_hdrs);

  _populate_native_modules(znmodules, &native_modules, &extra_hdrs);

  fprintf(out, "#include <stdlib.h>\n\n"
               "#include \"alloc/alloc.h\"\n"
               "#include \"alloc/arena/intern.h\"\n"
               "#include \"run/run.h\"\n"
               "#include \"struct/alist.h\"\n"
               "#include \"util/args/commandline.h\"\n"
               "#include \"util/args/commandlines.h\"\n"
               "#include \"vm/intern.h\"\n");

  M_iter extra_hdrs_it = set_iter(&extra_hdrs);
  for (; has(&extra_hdrs_it); inc(&extra_hdrs_it)) {
    fprintf(out, "#include \"%s\"\n", (char *)value(&extra_hdrs_it));
  }

  fprintf(out, "\n");

  for (int i = 3; i < argc; ++i) {
    const char *file_name = args[i];
    if (map_lookup(&native_modules, intern(file_name))) {
      continue;
    }
    DEBUGF("Processing source: %s", file_name);

    const char *assembly = _compile_to_string(file_name);
    const char *var_name = _convert_lib_path_to_var_name(file_name);

    char lib_var_name[MAX_VAR_NAME_LEN + 1];
    lib_var_name[0] = 0x0;
    sprintf(lib_var_name, "LIB_%s", var_name);
    print_string_as_var(lib_var_name, assembly, out);

    RELEASE(var_name);
    RELEASE(assembly);
  }

  M_iter it = map_iter(&native_modules);
  for (; has(&it); inc(&it)) {
    _NativeModuleInfo *info = (_NativeModuleInfo *)value(&it);
    const char *file_name = info->src;
    DEBUGF("Processing source: %s", file_name);
    char *dir_path, *file_base, *ext;
    split_path_file(file_name, &dir_path, &file_base, &ext);

    const char *assembly = _compile_to_string(file_name);
    const char *var_name = _convert_lib_path_to_var_name(file_name);

    char lib_var_name[MAX_VAR_NAME_LEN + 1];
    lib_var_name[0] = 0x0;
    sprintf(lib_var_name, "LIB_%s", var_name);
    print_string_as_var(lib_var_name, assembly, out);

    RELEASE(var_name);
    RELEASE(assembly);
  }

  fprintf(out,
          "int main(int argc, const char *argv[]) {\n"
          "  alloc_init();\n"
          "  strings_init();\n"
          "  ArgConfig *config = argconfig_create();\n"
          "  argconfig_package(config);\n"
          "  ArgStore *store = commandline_parse_args(config, argc, argv);\n"
          "  AList srcs;\n"
          "  alist_init(&srcs, char *, argc - 1);\n"
          "  AList src_contents;\n"
          "  alist_init(&src_contents, FileParts, argc - 1);\n"
          "  AList init_fns;\n"
          "  alist_init(&init_fns, void *, argc - 1);\n");

  for (int i = 3; i < argc; ++i) {
    const char *file_name = args[i];
    if (map_lookup(&native_modules, intern(file_name))) {
      continue;
    }
    char *dir_path, *file_base, *ext;
    split_path_file(file_name, &dir_path, &file_base, &ext);
    char *escaped_dir_path = escape(dir_path);

    const char *lib_var_name = _convert_lib_path_to_var_name(file_name);

    fprintf(out, "  *(char **)alist_add(&srcs) = \"%s%s.zna\";\n",
            escaped_dir_path, file_base);
    fprintf(out,
            "  *(FileParts *)alist_add(&src_contents) = (FileParts){ .parts = "
            "LIB_%s, .num_parts = sizeof(LIB_%s) / sizeof(LIB_%s[0])};\n",
            lib_var_name, lib_var_name, lib_var_name);
    fprintf(out, "  *(void **)alist_add(&init_fns) = NULL;\n");

    RELEASE(lib_var_name);
    RELEASE(escaped_dir_path);
    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
  }

  it = map_iter(&native_modules);
  for (; has(&it); inc(&it)) {
    const _NativeModuleInfo *m = (_NativeModuleInfo *)value(&it);

    const char *var_name = _convert_lib_path_to_var_name(m->src);

    char *dir_path, *file_base, *ext;
    split_path_file(m->src, &dir_path, &file_base, &ext);
    char *escaped_dir_path = escape(dir_path);
    fprintf(out, "  *(char **)alist_add(&srcs) = \"%s%s.zna\";\n",
            escaped_dir_path, file_base);
    fprintf(out, "  *(char **)alist_add(&src_contents) = (char*) LIB_%s;\n",
            var_name);
    fprintf(out, "  *(void **)alist_add(&init_fns) =  %s;\n", m->init_fn);

    RELEASE(var_name);
    RELEASE(escaped_dir_path);
    RELEASE(dir_path);
    RELEASE(file_base);
    RELEASE(ext);
  }

  fprintf(out, "  run_files(&srcs, &src_contents, &init_fns, store);\n"
               "  alist_finalize(&srcs);\n"
               "  alist_finalize(&src_contents);\n"
               "  alist_finalize(&init_fns);\n"
               "#ifdef DEBUG\n"
               "  argstore_delete(store);\n"
               "  argconfig_delete(config);\n"
               "  token_finalize_all();\n"
               "  strings_finalize();\n"
               "  alloc_finalize();\n"
               "#endif\n"
               "  return EXIT_SUCCESS;\n"
               "}\n");

#ifdef DEBUG
  M_iter iter = map_iter(&native_modules);
  for (; has(&iter); inc(&iter)) {
    void *val = value(&iter);
    RELEASE(val);
  }
  map_finalize(&native_modules);

  set_finalize(&extra_hdrs);

  optimize_finalize();
  strings_finalize();
  token_finalize_all();
  alloc_finalize();
#endif

  return EXIT_SUCCESS;
}