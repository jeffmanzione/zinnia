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

int64_t current_usec_since_epoch();

typedef struct {
  uint32_t year, month, day_of_month, hour, minute, second;
  uint64_t millisecond;
} Timestamp;

Timestamp timestamp_from_millis();

#endif /* UTIL_TIME_H_ */