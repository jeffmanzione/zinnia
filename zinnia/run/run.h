// run.h
//
// Created on: Oct 30, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_RUN_RUN_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_RUN_RUN_H_

#include "c-data-structures/arraylike.h"
#include "zinnia/util/args/commandline.h"
#include "zinnia/util/void_array.h"

typedef struct {
  enum { FP_FILE_SOURCE, FP_FILE_SEED } type;
  union {
    struct {
      const char **parts;
      size_t num_parts;
    } source;
    struct {
      const unsigned char *content_bytes;
      size_t content_size;
    } seed_data;
  };
} FileParts;

DEFINE_ARRAYLIKE(FilePartsArray, FileParts);

void run(const SourceNameSet *source_files, ArgStore *store);
void run_files(const CharPtrArray *source_file_names,
               const FilePartsArray *source_contents,
               const VoidPtrArray *init_fns, ArgStore *store);

int zinnia(int argc, const char *argv[]);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_RUN_RUN_H_ */