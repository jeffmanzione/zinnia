// platform.h
//
// Created on: Dec 28, 2020
//     Author: Jeff Manzione

#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_PLATFORM_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_PLATFORM_H_

#ifdef __unix__
#define OS_LINUX
#elif defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#define OS_WINDOWS
#endif

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_PLATFORM_H_ */