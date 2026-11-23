#ifndef RE5_CONVENIENCE_CONFIG_H
#define RE5_CONVENIENCE_CONFIG_H
#include <windows.h>
extern SRWLOCK code_lock;
int option(const char *name, int fallback);
BOOL ui_init(HINSTANCE, DWORD, LPVOID);
BOOL coop_init(HINSTANCE, DWORD, LPVOID);
BOOL skip_init(HINSTANCE, DWORD, LPVOID);
#endif
