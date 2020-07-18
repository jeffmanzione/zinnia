// string.h
//
// Created on: Jun 6, 2020
//     Author: Jeff Manzione

#ifndef UTIL_STRING_H_
#define UTIL_STRING_H_

#include <stdbool.h>

bool ends_with(const char *str, const char *suffix);
bool starts_with(const char *str, const char *prefix);
bool contains_char(const char str[], char c);

#endif /* UTIL_STRING_H_ */