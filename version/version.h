#ifndef VERSION_VERSION_H_
#define VERSION_VERSION_H_

// Returns a string of the version, e.g., 1.0.0, of the binary.
//
// The returned char* is a literal and does not require cleanup.
const char *version_string();

// Returns a ISO 8601 timestamp string of the binary creation.
//
// The returned char* is a literal and does not require cleanup.
const char *version_timestamp_string();

#endif /* VERSION_VERSION_H_ */