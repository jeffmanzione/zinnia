/*
 * time.c
 *
 *  Created on: Nov 12, 2022
 *      Author: Jeff
 */

#include "util/time.h"

#include <stdbool.h>
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
  Timestamp ts = current_local_timestamp();
  return timestamp_to_micros(&ts);
}

int64_t timestamp_to_micros(const Timestamp *ts) {
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

Timestamp current_local_timestamp() {
#ifdef linux
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
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

Timestamp current_gmt_timestamp() {
#ifdef linux
  time_t t = time(NULL);
  struct tm tm = *gmtime(&t);
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
  GetSystemTime(&st);
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

TimezoneOffset create_timezone_offset(int8_t hours, int8_t minutes) {
  TimezoneOffset tz_offset = {.hours = hours, .minutes = minutes};
  return tz_offset;
}

void timestamp_to_iso8601(const Timestamp *ts, const TimezoneOffset *tz_offset,
                          char *buffer) {
  const int chars_written = sprintf(
      buffer, "%04u-%02u-%02uT%02u:%02u:%02u.%03lu", ts->year, ts->month,
      ts->day_of_month, ts->hour, ts->minute, ts->second, ts->millisecond);
  buffer += chars_written;

  if (tz_offset == NULL || (tz_offset->hours == 0 && tz_offset->minutes == 0)) {
    sprintf(buffer, "Z");
  } else {
    sprintf(buffer, "%s%02d:%02d",
            (tz_offset->hours >= 0 && tz_offset->minutes >= 0) ? "+" : "",
            tz_offset->hours, abs(tz_offset->minutes));
  }
}
