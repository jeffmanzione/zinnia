/*
 * time.h
 *
 *  Created on: Nov 12, 2022
 *      Author: Jeff
 */

#ifndef UTIL_TIME_H_
#define UTIL_TIME_H_

#include <stdint.h>
#include <time.h>

typedef struct {
  uint16_t year;
  uint8_t month;
  uint8_t day_of_month;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
} Timestamp;

typedef struct {
  int8_t hours;
  int8_t minutes;
} TimezoneOffset;

int64_t current_usec_since_epoch();
Timestamp current_local_timestamp();
Timestamp current_gmt_timestamp();
int64_t timestamp_to_micros(const Timestamp *ts);
Timestamp micros_to_timestamp(int64_t micros_since_epoch);

TimezoneOffset create_timezone_offset(int8_t hours, int8_t minutes);

void timestamp_to_iso8601(const Timestamp *ts, const TimezoneOffset *tz_offset,
                          char *buffer);

#endif /* UTIL_TIME_H_ */