/*
 * time.c
 *
 *  Created on: Nov 12, 2022
 *      Author: Jeff
 */

#include "util/time.h"

#include <stdio.h>

#ifdef linux
#include <time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
#define DIFF_FROM_EPOCH_USEC 116444736000000000LL / 10
#endif

int64_t current_usec_since_epoch() {
  int64_t usec = -1;
#ifdef linux
  struct timespec tms;
  if (clock_gettime(CLOCK_REALTIME, &tms)) {
    return -1;
  }
  usec = tms.tv_sec * USEC_PER_SEC + tms.tv_nsec / 1000;
  if (tms.tv_nsec % 1000 >= 500) {
    ++usec;
  }
#endif

#ifdef _WIN32
  FILETIME ft_now;
  GetSystemTimeAsFileTime(&ft_now);
  // Micros since Jan 1, 1601.
  usec = ((LONGLONG)ft_now.dwLowDateTime +
          ((LONGLONG)(ft_now.dwHighDateTime) << 32LL)) /
             10 -
         DIFF_FROM_EPOCH_USEC;
#endif
  return usec;
}

Timestamp timestamp_from_millis(int64_t millis_since_epoch) {
#ifdef linux
  char time_str[127];
  double fractional_seconds;
  int milliseconds;
  struct tm tm;

  memset(&tm, 0, sizeof(struct tm));
  sprintf(time_str, "%lld UTC", millis_since_epoch / 1000);
  strptime(time_str, "%s %U", &tm);

  time_t seconds_since_epoch = mktime(&tm);

  Timestamp ts = {.year = tm.tm_year,
                  .month = tm.tm_mon,
                  .day_of_month = tm.tm_mday,
                  .hour = tm.tm_hour,
                  .minute = tm.tm_sec,
                  .millisecond =
                      millis_since_epoch - (seconds_since_epoch * 1000)};
  return ts;
#endif

#ifdef _WIN32
  // TODO: Implement for Windows.
  Timestamp ts;
  return ts;
#endif
}