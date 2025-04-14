// string_util.h
//
// Created on: Jan 7, 2021
//     Author: Jeff Manzione

#ifndef JEFF_VM_UTIL_STRING_H_
#define JEFF_VM_UTIL_STRING_H_

int escape(const char str[], char **target);
int unescape(const char str[], char **target);

#endif /* JEFF_VM_UTIL_STRING_H_ */