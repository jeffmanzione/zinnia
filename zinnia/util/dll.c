#include "zinnia/util/dll.h"

#include <stdio.h>

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "zinnia/util/platform.h"

bool open_dl(const char file_name[], DlHandle *handle_ptr, char *error_buf) {
#ifdef OS_WINDOWS
  HMODULE handle = LoadLibrary(TEXT(file_name));
  if (!handle) {
    const int error_code = GetLastError();
    sprintf(error_buf,
            "Error opening dynamically-loaded library '%s'. Error code: %d",
            file_name, error_code);
    return false;
  }
#else
  void *handle = dlopen(file_name, RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    sprintf(error_buf,
            "Error opening dynamically-loaded library '%s'. Message: %s",
            file_name, dlerror());
    return false;
  }
#endif
  *handle_ptr = handle;
  return true;
}

bool open_dl_sym(DlHandle handle, const char fn_name[],
                 DlFnHandle *fn_handle_ptr, char *error_buf) {
#ifdef OS_WINDOWS
  void *fn_ptr = GetProcAddress(handle, fn_name);
  if (!fn_ptr) {
    const int error_code = GetLastError();
    sprintf(error_buf,
            "Error opening dynamically-loaded function '%s' from library. "
            "Error code: %d",
            fn_name, error_code);
    return false;
  }
#else
  void *fn_ptr = dlsym(handle, fn_name);
  if (!fn_ptr) {
    sprintf(error_buf,
            "Error opening dynamically-loaded function '%s' from library. "
            "Message: %s",
            fn_name, dlerror());
    return false;
  }
#endif
  *fn_handle_ptr = fn_ptr;
  return true;
}

bool open_dl_sym_fn(const char file_name[], const char fn_name[],
                    DlFnHandle *fn_handle_ptr, char *error_buf) {
  void *handle;
  const bool loaded_lib = open_dl(file_name, &handle, error_buf);
  if (!loaded_lib) {
    return false;
  }
  return open_dl_sym(handle, fn_name, fn_handle_ptr, error_buf);
}

bool close_dl(DlHandle handle, char *error_buf) {
  if (!handle) {
    // Should this be an error?
    return true;
  }
#ifdef OS_WINDOWS
  if (!FreeLibrary(handle)) {
    const int error_code = GetLastError();
    sprintf(error_buf, "Failed to close dynamic library. Error code: %d",
            error_code);
    return false;
  }
  return true;
#else
  if (dlclose(handle) != 0) {
    sprintf(error_buf, "Failed to close dynamic library. Message: %s",
            dlerror());
    return false;
  }
  return true;
#endif
}