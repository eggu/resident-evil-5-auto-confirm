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
// re5dx9.exe is wrapped in Enigma Protector: it decrypts the game while it starts and checks the image, and a write
// into the image in that window quits the game with "Program integrity check failed". No feature worker starts (so
// nothing is patched) until the game's entry point has run (CRT security cookie 0x11ac150 initialised, its complement
// next to it), its own window (class MTFramework, this process) exists and 3 s have passed. Each worker still checks
// the decrypted code before patching. Never on a timeout. Same gate as Battle Buddy / RE3Controls.
static int entry_point_passed(void) {
    const DWORD *cookie = (const DWORD *)0x11ac150;
    MEMORY_BASIC_INFORMATION m;
    return VirtualQuery(cookie, &m, sizeof m) && m.State == MEM_COMMIT
        && !(m.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        && (DWORD)cookie + 8 <= (DWORD)m.BaseAddress + m.RegionSize
        && cookie[0] != 0xbb40e64e && cookie[1] == ~cookie[0];
}
static BOOL CALLBACK find_game_window(HWND window, LPARAM found) {
    DWORD pid = 0;
    char name[32];
    GetWindowThreadProcessId(window, &pid);
    if (pid == GetCurrentProcessId() && GetClassNameA(window, name, sizeof name) && !strcmp(name, "MTFramework")) {
        *(HWND *)found = window;
        return FALSE;
    }
    return TRUE;
}
static int game_window_up(void) {
    HWND window = NULL;
    EnumWindows(find_game_window, (LPARAM)&window);
    return window != NULL;
}
static int wait_for(int (*ready)(void), DWORD timeout) {
    for (DWORD start = GetTickCount(); !ready(); Sleep(20))
        if (GetTickCount() - start > timeout) return 0;
    return 1;
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
    if (!wait_for(entry_point_passed, 120000)) {
        report("Game entry point not reached; nothing changed (no patch before the integrity check).");
        return 1;
    }
    if (!wait_for(game_window_up, 300000)) {
        report("Game window did not appear; nothing changed.");
        return 1;
    }
    Sleep(3000);
    report("Game running; patching now.");
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
