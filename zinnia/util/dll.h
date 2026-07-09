#ifndef COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_
#define COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_

#include <stdbool.h>

typedef void *DlHandle;
typedef void *DlFnHandle;

bool open_dl(const char file_name[], DlHandle *handle_ptr, char *error_buf);
bool open_dl_sym(DlHandle handle, const char fn_name[],
                 DlFnHandle *fn_handle_ptr, char *error_buf);
bool open_dl_sym_fn(const char file_name[], const char fn_name[],
                    DlFnHandle *fn_handle_ptr, char *error_buf);
bool close_dl(DlHandle handle, char *error_buf);

#endif /* COM_GITHUB_JEFFMANZIONE_ZINNIA_UTIL_DLL_H_ */