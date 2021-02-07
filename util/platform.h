// platform.h
//
// Created on: Dec 28, 2020
//     Author: Jeff Manzione

#ifndef UTIL_PLATFORM_H_
#define UTIL_PLATFORM_H_

#ifdef __unix__         
#define OS_LINUX
#elif defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) 
#define OS_WINDOWS
#endif

#endif /* UTIL_PLATFORM_H_ */