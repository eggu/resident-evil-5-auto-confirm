#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

SRWLOCK code_lock = SRWLOCK_INIT;
static char ini[MAX_PATH], log_path[MAX_PATH];
int option(const char *name, int fallback) {
    return GetPrivateProfileIntA("Features", name, fallback, ini) == 1;
}
static void report(const char *message) {
    FILE *f = fopen(log_path, "a");
    if (f) { fprintf(f, "%s\n", message); fclose(f); }
}
static int legacy_in(const char *directory) {
    const char *names[] = {"RE5.UISpeedTweaks.asi", "RE5.CoopInput.asi", "RE5.CutsceneSkip.asi", "RE5.ConvenienceTweaks.asi"};
    char candidate[MAX_PATH];
    for (unsigned i = 0; i < sizeof names / sizeof *names; i++) {
        int n = snprintf(candidate, sizeof candidate, "%s\\%s", directory, names[i]);
        if (GetModuleHandleA(names[i]) || n < 0 || n >= MAX_PATH ||
            GetFileAttributesA(candidate) != INVALID_FILE_ATTRIBUTES) return 1;
    }
    return 0;
}
static DWORD WINAPI initialize(void *module) {
    char directory[MAX_PATH], executable[MAX_PATH], child[MAX_PATH];
    strcpy(directory, ini);
    char *slash = strrchr(directory, '\\');
    if (!slash) return 1;
    *slash = 0;
    DWORD n = GetModuleFileNameA(NULL, executable, sizeof executable);
    if (!n || n >= sizeof executable || !(slash = strrchr(executable, '\\'))) return 1;
    *slash = 0;
    int conflict = legacy_in(directory) || legacy_in(executable);
    const char *folders[] = {"scripts", "plugins"};
    for (unsigned i = 0; i < 2; i++) {
        int length = snprintf(child, sizeof child, "%s\\%s", executable, folders[i]);
        if (length < 0 || length >= MAX_PATH || legacy_in(child)) conflict = 1;
    }
    if (conflict) {
        report("Legacy ASI detected: remove RE5.UISpeedTweaks.asi, RE5.CoopInput.asi RE5.CutsceneSkip.asi and RE5.ConvenienceTweaks.asi; no integrated features started.");
        return 1;
    }
    HINSTANCE h = (HINSTANCE)module;
    if (option("ShortSaveMessages", 1) || option("AutoConfirmOK", 1)) ui_init(h, DLL_PROCESS_ATTACH, NULL);
    if (option("CoopInput", 1)) coop_init(h, DLL_PROCESS_ATTACH, NULL);
    if (option("ExtraSkipButtons", 1) || option("UnlockSkipRestrictions", 0) || option("SkipStartupMovies", 0)) skip_init(h, DLL_PROCESS_ATTACH, NULL);
    report("Selected feature workers started; see subsequent installation results. Restart the game after INI changes.");
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH || GetModuleHandleA(NULL) != (HMODULE)0x400000) return TRUE;
    DisableThreadLibraryCalls(h);
    DWORD n = GetModuleFileNameA(h, ini, sizeof ini);
    char *ext = strrchr(ini, '.');
    if (!n || n >= sizeof ini || !ext || ext - ini > MAX_PATH - 5) return TRUE;
    strcpy(ext, ".ini");
    strcpy(log_path, ini);
    strcpy(strrchr(log_path, '.'), ".log");
    HANDLE thread = CreateThread(NULL, 0, initialize, h, 0, NULL);
    if (thread) CloseHandle(thread);
    else report("Failed to create initialization thread; nothing changed.");
    return TRUE;
}
