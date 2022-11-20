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
  int64_t year;
  int64_t month;
  int64_t day_of_month;
  int64_t hour;
  int64_t minute;
  int64_t second;
  int64_t millisecond;
} Timestamp;

int64_t current_usec_since_epoch();
Timestamp current_timestamp();
int64_t timestamp_to_micros(Timestamp *ts);
Timestamp micros_to_timestamp(int64_t micros_since_epoch);

#endif /* UTIL_TIME_H_ */