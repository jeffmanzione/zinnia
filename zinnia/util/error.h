#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ERROR_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ERROR_H_

// FATALF(fmt, ...)
//
// Fatally terminates the program after writing the specified formatted message
// to stderr.
//
// Usage:
//   FATALF("Oh no fatal error, value=%d.", value);

#define FATALF(fmt, ...)                                        \
  do {                                                          \
    errorf__(__LINE__, __func__, __FILE__, fmt, ##__VA_ARGS__); \
  } while (0)

void errorf__(int line_num, const char func_name[], const char file_name[],
              const char fmt[], ...);

// DEBUGF(fmt, ...)
//
// Outputs a message with the given [fmt] just like pritnf but with some
// additional debug infor such as line number, function, and file and
// immediately flushes the output.
//
// Usage:
//   DEBUGF("Oh no, value=%d.", value);
#ifdef DEBUG

#define ASSERT(exp)                                                           \
  do {                                                                        \
    if (!(exp)) {                                                             \
      FATALF("Assertion failed. Expression( %s ) evaluated to false.", #exp); \
    }                                                                         \
  } while (0)

#define DEBUGF(fmt, ...) \
  debugf__(__LINE__, __func__, __FILE__, fmt, ##__VA_ARGS__)

void debugf__(int line_num, const char func_name[], const char file_name[],
              const char fmt[], ...);

#else

#define ASSERT(exp) \
  do {              \
    (void)(exp);    \
  } while (0)

#define DEBUGF(fmt, ...) \
  do {                   \
  } while (0)

#endif

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_ERROR_H_ */