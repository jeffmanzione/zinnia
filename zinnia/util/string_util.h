// string_util.h
//
// Created on: Jan 7, 2021
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_JEFF_VM_UTIL_STRING_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_JEFF_VM_UTIL_STRING_H_

int escape(const char str[], char **target);
int unescape(const char str[], char **target);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_JEFF_VM_UTIL_STRING_H_ */