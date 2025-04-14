#ifndef UTIL_CODEGEN_H_
#define UTIL_CODEGEN_H_

#include <stdbool.h>
#include <stdio.h>

void print_string_as_var_default(const char var_name[], const char content[],
                                 FILE *target);

void print_string_as_var(const char var_name[], const char content[],
                         int loose_literal_upper_bound,
                         int loose_literal_precat_upper_bound,
                         bool avoid_splitting_tokens, FILE *target);

#endif /* UTIL_CODEGEN_H_ */