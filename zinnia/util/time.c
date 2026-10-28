/*
 * time.c
 *
 *  Created on: Nov 12, 2022
 *      Author: Jeff
 */

// Exposes timegm() and gmtime_r()/localtime_r() on glibc.
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "zinnia/util/time.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <winnt.h>
// Difference between the Windows epoch (1601-01-01) and the Unix epoch.
#define DIFF_FROM_EPOCH_USEC (11644473600LL * 1000 * 1000)
#else
#include <sys/time.h>
#endif

static Timestamp tm_to_timestamp_(const struct tm *tm, uint16_t millisecond) {
  Timestamp ts = {.year = tm->tm_year + 1900,
                  .month = tm->tm_mon + 1,
                  .day_of_month = tm->tm_mday,
                  .hour = tm->tm_hour,
                  .minute = tm->tm_min,
                  .second = tm->tm_sec,
                  .millisecond = millisecond};
  return ts;
}

int64_t current_usec_since_epoch() {
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  ULARGE_INTEGER hundred_nanos = {.LowPart = ft.dwLowDateTime,
                                  .HighPart = ft.dwHighDateTime};
  return (int64_t)(hundred_nanos.QuadPart / 10) - DIFF_FROM_EPOCH_USEC;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000 * 1000 + tv.tv_usec;
#endif
}

// Interprets ts as UTC.
int64_t timestamp_to_micros(const Timestamp *ts) {
  struct tm tm = {.tm_year = ts->year - 1900,
                  .tm_mon = ts->month - 1,
                  .tm_mday = ts->day_of_month,
                  .tm_hour = ts->hour,
                  .tm_min = ts->minute,
                  .tm_sec = ts->second};
#ifdef _WIN32
  int64_t seconds = _mkgmtime(&tm);
#else
  int64_t seconds = timegm(&tm);
#endif
  return (seconds * 1000 + ts->millisecond) * 1000;
}

// Returns a UTC timestamp.
Timestamp micros_to_timestamp(int64_t micros_since_epoch) {
  int64_t millis_since_epoch = micros_since_epoch / 1000;
  int64_t millis = millis_since_epoch % 1000;
  if (millis < 0) {
    millis += 1000;
  }
  time_t seconds_since_epoch = (time_t)((millis_since_epoch - millis) / 1000);
  struct tm tm;
#ifdef _WIN32
  gmtime_s(&tm, &seconds_since_epoch);
#else
  gmtime_r(&seconds_since_epoch, &tm);
#endif
  return tm_to_timestamp_(&tm, (uint16_t)millis);
}

Timestamp current_local_timestamp() {
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
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_t t = tv.tv_sec;
  struct tm tm;
  localtime_r(&t, &tm);
  return tm_to_timestamp_(&tm, (uint16_t)(tv.tv_usec / 1000));
#endif
}

Timestamp current_gmt_timestamp() {
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
#else
  return micros_to_timestamp(current_usec_since_epoch());
#endif
}

TimezoneOffset create_timezone_offset(int8_t hours, int8_t minutes) {
  TimezoneOffset tz_offset = {.hours = hours, .minutes = minutes};
  return tz_offset;
}

void timestamp_to_iso8601(const Timestamp *ts, const TimezoneOffset *tz_offset,
                          char *buffer) {
  const int chars_written = sprintf(
      buffer, "%04u-%02u-%02uT%02u:%02u:%02u.%03u", ts->year, ts->month,
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
