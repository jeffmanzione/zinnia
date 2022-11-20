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
#define DIFF_FROM_EPOCH_USEC (11644473600LL * 1000 * 1000)
#endif

int64_t current_usec_since_epoch() {
  Timestamp ts = current_timestamp();
  return timestamp_to_micros(&ts);
}

int64_t timestamp_to_micros(Timestamp *ts) {
  struct tm tm = {.tm_year = ts->year - 1900,
                  .tm_mon = ts->month - 1,
                  .tm_mday = ts->day_of_month,
                  .tm_hour = ts->hour,
                  .tm_min = ts->minute,
                  .tm_sec = ts->second,
                  .tm_isdst = -1};
  time_t time = mktime(&tm);
  int64_t time64 = (time * 1000 + ts->millisecond) * 1000;
  return time64;
}

Timestamp micros_to_timestamp(int64_t micros_since_epoch) {
  int64_t seconds_since_epoch = micros_since_epoch / 1000 / 1000;
  struct tm *tm = localtime(&seconds_since_epoch);
  int64_t millis = (micros_since_epoch / 1000) % 1000;
  Timestamp ts = {.year = tm->tm_year + 1900,
                  .month = tm->tm_mon + 1,
                  .day_of_month = tm->tm_mday,
                  .hour = tm->tm_hour,
                  .minute = tm->tm_min,
                  .second = tm->tm_sec,
                  .millisecond = millis};
  return ts;
}

Timestamp current_timestamp() {
#ifdef linux
  time_t T = time(NULL);
  struct tm tm = *localtime(&T);
  Timestamp ts = {.year = tm.tm_year + 1900,
                  .month = tm.tm_mon + 1,
                  .day_of_month = tm.tm_mday,
                  .hour = tm.tm_hour,
                  .minute = tm.tm_min,
                  .second = tm.tm_sec,
                  .millisecond = 0};
  return ts;
#endif
#ifdef _WIN32
  SYSTEMTIME st;
  GetLocalTime(&st);
  Timestamp ts = {.year = st.wYear,
                  .month = st.wMonth,
                  .day_of_month = st.wDay,
                  .hour = st.wHour,
                  .minute = st.wMinute,
                  .second = st.wSecond,
                  .millisecond = st.wMilliseconds};
  return ts;
#endif
}