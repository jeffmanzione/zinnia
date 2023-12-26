#include "package/package.h"

#include <stdio.h>
#include <stdlib.h>

#include "alloc/alloc.h"
#include "compile/compile.h"
#include "util/file/file_util.h"
#include "util/string.h"
#include "util/string_util.h"

#define STRING_BLOCK_LEN 400

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
  DEALLOC(input);
  return true;
}

void _write_file_chunks(const char file_content[], FILE *out) {
  int start = 0, end = STRING_BLOCK_LEN;
  for (; end <= strlen(file_content); end += STRING_BLOCK_LEN) {
    while (file_content[end] != ' ' && end < strlen(file_content)) {
      ++end;
    }
    int len = min(end - start, strlen(file_content + start));
    fprintf(out, "\n  \"%.*s\"", len, file_content + start);
    start = end;
  }
  if (start < strlen(file_content)) {
    fprintf(out, "\n  \"%.*s\"", (int)strlen(file_content + start),
            file_content + start);
  }
}

char *_compile_to_file(const char file_name[]) {
  if (ends_with(file_name, ".jp")) {

    FILE *assembly_file = tmpfile();
    compile_to_assembly(file_name, assembly_file);

    rewind(assembly_file);

    char *content = NULL, *assembly_content = NULL;
    getall(assembly_file, &content);
    char *escaped_assembly = escape(content);

    DEALLOC(content);
    DEALLOC(assembly_content);
    fclose(assembly_file);
    return escaped_assembly;
  } else if (ends_with(file_name, ".ja")) {
    FILE *file = FILE_FN(file_name, "rb");
    if (NULL == file) {
      FATALF("File does not exist: %s", file_name);
    }
    char *content = NULL;
    getall(file, &content);
    char *escaped_content = escape(content);
    DEALLOC(content);
    return escaped_content;
  } else {
    FATALF("Uknown file type: %s", file_name);
    return NULL;
  }
}

int jasperp(int argc, const char *args[]) {
  alloc_init();
  strings_init();
  optimize_init();

  const char *out_file_name = args[1];
  FILE *out = FILE_FN(out_file_name, "w");
  if (NULL == out) {
    fprintf(stderr, "Could not open file: \"%s\".", out_file_name);
    return EXIT_FAILURE;
  }

  fprintf(out, "#include <stdlib.h>\n\n"
               "#include \"alloc/alloc.h\"\n"
               "#include \"alloc/arena/intern.h\"\n"
               "#include \"run/run.h\"\n"
               "#include \"struct/alist.h\"\n"
               "#include \"util/args/commandline.h\"\n"
               "#include \"util/args/commandlines.h\"\n\n");

  for (int i = 2; i < argc; ++i) {
    const char *file_name = args[i];
    DEBUGF("Processing source: %s\n", file_name);
    char *dir_path, *file_base, *ext, *file_content;
    split_path_file(file_name, &dir_path, &file_base, &ext);

    const char *escaped_assembly = _compile_to_file(file_name);

    fprintf(out, "const char LIB_%s[] =", file_base);
    _write_file_chunks(escaped_assembly, out);
    fprintf(out, ";\n\n");

    DEALLOC(dir_path);
    DEALLOC(file_base);
    DEALLOC(ext);
    DEALLOC(escaped_assembly);
  }

  fprintf(out,
          "int main(int argc, const char *argv[]) {\n"
          "  alloc_init();\n"
          "  strings_init();\n"
          "  ArgConfig *config = argconfig_create();\n"
          "  argconfig_packaged(config);\n"
          "  ArgStore *store = commandline_parse_args(config, argc, argv);\n");

  fprintf(out, "  AList srcs;\n"
               "  alist_init(&srcs, char *, argc - 1);\n"
               "  AList src_contents;\n"
               "  alist_init(&src_contents, char *, argc - 1);\n");

  for (int i = 2; i < argc; ++i) {
    const char *file_name = args[i];
    char *dir_path, *file_base, *ext, *file_content;
    split_path_file(file_name, &dir_path, &file_base, &ext);
    char *escaped_dir_path = escape(dir_path);
    fprintf(out, "  *(char **)alist_add(&srcs) = \"%s/%s.ja\";\n",
            escaped_dir_path, file_base);
    fprintf(out, "  *(char **)alist_add(&src_contents) =  (char*) LIB_%s;\n",
            file_base);

    DEALLOC(escaped_dir_path);
    DEALLOC(dir_path);
    DEALLOC(file_base);
    DEALLOC(ext);
  }

  fprintf(out, "  run_files(&srcs, &src_contents, store);\n"
               "  alist_finalize(&srcs);\n"
               "  alist_finalize(&src_contents);\n"
               "#ifdef DEBUG\n"
               "  argstore_delete(store);\n"
               "  argconfig_delete(config);\n"
               "  strings_finalize();\n"
               "  token_finalize_all();\n"
               "  alloc_finalize();\n"
               "#endif\n"
               "  return EXIT_SUCCESS;\n"
               "}\n");

#ifdef DEBUG
  optimize_finalize();
  strings_finalize();
  token_finalize_all();
  alloc_finalize();
#endif

  return EXIT_SUCCESS;
}