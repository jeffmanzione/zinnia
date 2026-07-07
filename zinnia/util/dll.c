#include "zinnia/util/dll.h"

#include <stdio.h>

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "zinnia/util/platform.h"

bool load_dynamic_library(const char file_name[], void **dl_library_ptr,
                          char *error_buf) {
#ifdef OS_WINDOWS
  HMODULE dl_handle = LoadLibrary(TEXT(file_name));
  if (!dl_handle) {
    const int error_code = GetLastError();
    sprintf(error_buf,
            "Error opening dynamically-loaded library '%s'. Code: %d",
            file_name, error_code);
    return false;
  }
#else
  void *dl_handle = dlopen(file_name, RTLD_LAZY | RTLD_GLOBAL);
  if (!dl_handle) {
    sprintf(error_buf,
            "Error opening dynamically-loaded library '%s'. Message: %s",
            file_name, dlerror());
    return false;
  }
#endif
  *dl_library_ptr = dl_handle;
  return true;
}

bool load_dynamic_function(void *dl_handle, const char fn_name[],
                           void **dynamic_fn_ptr, char *error_buf) {
#ifdef OS_WINDOWS
  void *fn_ptr = GetProcAddress(dl_handle, fn_name);
  if (!fn_ptr) {
    const int error_code = GetLastError();
    sprintf(
        error_buf,
        "Error opening dynamically-loaded function '%s' from library. Code: %d",
        fn_name, error_code);
    return false;
  }
#else
  void *fn_ptr = dlsym(dl_handle, fn_name);
  if (!fn_ptr) {
    sprintf(error_buf,
            "Error opening dynamically-loaded function '%s' from library. "
            "Message: %s",
            fn_name, dlerror());
    return false;
  }
#endif
  *dynamic_fn_ptr = fn_ptr;
  return true;
}

bool load_dynamic_library_function(const char file_name[], const char fn_name[],
                                   void **dynamic_fn_ptr, char *error_buf) {
  void *dl_handle;
  const bool loaded_lib =
      load_dynamic_library(file_name, &dl_handle, error_buf);
  if (!loaded_lib) {
    return false;
  }
  return load_dynamic_function(dl_handle, fn_name, dynamic_fn_ptr, error_buf);
}