#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_

#include <stdbool.h>

bool load_dynamic_library(const char file_name[], void **dl_library_ptr,
                          char *error_buf);
bool load_dynamic_function(void *dl_handle, const char fn_name[],
                           void **dynamic_fn_ptr, char *error_buf);
bool load_dynamic_library_function(const char file_name[], const char fn_name[],
                                   void **dynamic_fn_ptr, char *error_buf);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_ */