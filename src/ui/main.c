// RE5 UI Speed Tweaks: auto-confirm OK-only system messages (save/load complete etc.).
// The alert unit (uIdAlert00, vtable 0xf9db1c) waits for a button in manager mode 2. After the
// configured delay we take the same path as the game's OK press: clear the button prompt, arm the
// post-press timer and switch the manager to mode 3, which sets the result and closes the window.
#include <windows.h>
#include <stdio.h>
#include "../config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define UPDATE_SLOT ((void **)(0xf9db1c + 6 * 4))
#define UPDATE_ORIGINAL ((void *)0x9e2c10)
#define MANAGER (*(BYTE **)0x11b2158)
#define BUTTON_PROMPT (*(void **)0x11b20c4)
#define MESSAGE_TABLE 0x1121760 // 32-byte rows, button type word at +0x12 (1 = yes/no, 2 = OK only)
#define MESSAGE_COUNT 0x28
#define MODE_WAIT_INPUT 2
#define MODE_CLOSING 3

typedef void(__attribute__((thiscall)) * UpdateFn)(BYTE *);
typedef void(__attribute__((thiscall)) * PromptClearFn)(void *, int, int);
typedef void(__attribute__((thiscall)) * SetModeFn)(BYTE *, int);

static UpdateFn original;
static char path[MAX_PATH];
static DWORD delay_ms = 300;
static ULONGLONG wait_start;
static int logged_id = -1;
static float save_message_seconds = 0.1f;
// movss xmm0, [0xf554ec] (1.0 s) that arms the minimum "saving/loading" message time in the save manager
static const DWORD save_message_sites[] = {0x7a69c5, 0x7a6a37, 0x7a6d2d};
static const BYTE save_message_code[] = {0xf3, 0x0f, 0x10, 0x05, 0xec, 0x54, 0xf5, 0x00};

static void log_line(const char *text) {
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "%s\n", text); fclose(f); }
}

static int button_type(DWORD id) {
    return id < MESSAGE_COUNT ? *(short *)(MESSAGE_TABLE + id * 32 + 0x12) : -1;
}

static void __attribute__((thiscall)) alert_update(BYTE *alert) {
    BYTE *manager = MANAGER;
    DWORD id = *(DWORD *)(alert + 0x40);
    static DWORD last_mode = ~0u, last_id = ~0u;
    DWORD mode = manager ? *(DWORD *)(manager + 0x3974c) : ~0u;
    if (mode != last_mode || id != last_id) { // ponytail: diagnostic trace, remove once verified
        char line[96];
        snprintf(line, sizeof line, "[%lu] mode=%lu id=%lu type=%d", GetTickCount(), mode, id, button_type(id));
        log_line(line);
        last_mode = mode; last_id = id;
    }
    if (!manager || *(DWORD *)(manager + 0x3974c) != MODE_WAIT_INPUT || button_type(id) != 2) {
        wait_start = 0;
        original(alert);
        return;
    }
    ULONGLONG now = GetTickCount64();
    if (!wait_start) wait_start = now;
    if (now - wait_start >= delay_ms) {
        wait_start = 0;
        ((PromptClearFn)0x7b43c0)(BUTTON_PROMPT, 1, 0);
        *(float *)(alert + 0x58) = 0;
        ((SetModeFn)0x7a72b0)(manager, MODE_CLOSING);
        if (logged_id != (int)id) {
            char line[64];
            snprintf(line, sizeof line, "auto-confirmed message %lu", id);
            log_line(line);
            logged_id = (int)id;
        }
    }
    original(alert);
}

static void read_config(void) {
    char ini[MAX_PATH], value[32];
    strcpy(ini, path);
    strcpy(strrchr(ini, '.'), ".ini");
    GetPrivateProfileStringA("Timing", "AutoConfirmSeconds", "0.1", value, sizeof value, ini);
    double seconds = atof(value);
    if (!isfinite(seconds) || seconds < 0 || seconds > 30) seconds = 0.1;
    delay_ms = (DWORD)(seconds * 1000);
    GetPrivateProfileStringA("Timing", "SaveMessageSeconds", "0.1", value, sizeof value, ini);
    seconds = atof(value);
    save_message_seconds = !isfinite(seconds) || seconds < 0 || seconds > 30 ? 0.1f : (float)seconds;
}

// Point each "movss xmm0, [constant]" at our own float.
static int patch_constant(const DWORD *sites, unsigned count, const BYTE *expect, float *value) {
    int patched = 0;
    for (unsigned i = 0; i < count; i++) {
        BYTE *code = (BYTE *)sites[i];
        DWORD old, tmp;
        if (memcmp(code, expect, 8)) continue;
        if (!VirtualProtect(code + 4, 4, PAGE_EXECUTE_READWRITE, &old)) continue;
        *(float **)(code + 4) = value;
        if (!VirtualProtect(code + 4, 4, old, &tmp)) log_line("WARNING: timing page protection restoration failed");
        FlushInstructionCache(GetCurrentProcess(), code, 8);
        patched++;
    }
    return patched;
}

static int matches(DWORD at, const void *bytes, size_t size) {
    MEMORY_BASIC_INFORMATION m;
    return VirtualQuery((void *)at, &m, sizeof m) && m.State == MEM_COMMIT
        && !(m.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        && at + size <= (DWORD)m.BaseAddress + m.RegionSize
        && !memcmp((void *)at, bytes, size);
}
static int ui_supported(void) {
    if (option("AutoConfirmOK", 1)) {
        void *expected = UPDATE_ORIGINAL;
        if (!matches((DWORD)UPDATE_SLOT, &expected, 4)) return 0;
    }
    if (option("ShortSaveMessages", 1))
        for (unsigned i = 0; i < sizeof save_message_sites / sizeof *save_message_sites; i++)
            if (!matches(save_message_sites[i], save_message_code, sizeof save_message_code)) return 0;
    return 1;
}

static DWORD WINAPI install(void *unused) {
    (void)unused;
    Sleep(5000); // the protected startup restores its tables before this point
    int i;
    for (i = 0; i < 600 && !ui_supported(); i++) Sleep(100);
    if (i == 600) { log_line("UI code/alert slot mismatch; unsupported build or conflicting mod, nothing changed"); return 1; }
    if (option("AutoConfirmOK", 1)) {
        DWORD old, tmp;
        if (!VirtualProtect(UPDATE_SLOT, 4, PAGE_READWRITE, &old)) { log_line("VirtualProtect failed"); return 1; }
        original = (UpdateFn)*UPDATE_SLOT;
        InterlockedExchangePointer(UPDATE_SLOT, (void *)alert_update);
        if (!VirtualProtect(UPDATE_SLOT, 4, old, &tmp)) { log_line("WARNING: alert page protection restoration failed"); return 1; }
    }
    int saves = option("ShortSaveMessages", 1)
        ? patch_constant(save_message_sites, 3, save_message_code, &save_message_seconds) : 0;
    char line[160];
    snprintf(line, sizeof line, "installed UI: AutoConfirmOK=%d AutoConfirmSeconds=%.3f ShortSaveMessages=%d SaveMessageSeconds=%.3f (%d/3 sites)",
             option("AutoConfirmOK", 1), delay_ms / 1000.0,
             option("ShortSaveMessages", 1), save_message_seconds, saves);
    log_line(line);
    return 0;
}

BOOL ui_init(HINSTANCE h, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH || GetModuleHandleA(NULL) != (HMODULE)0x400000) return TRUE;
    DisableThreadLibraryCalls(h);
    DWORD n = GetModuleFileNameA(h, path, sizeof path);
    char *ext = strrchr(path, '.');
    if (!n || n >= sizeof path || !ext) return TRUE;
    strcpy(ext, ".log");
    read_config();
    HANDLE t = CreateThread(NULL, 0, install, NULL, 0, NULL);
    if (t) CloseHandle(t);
    else log_line("Failed to create UI worker; nothing changed.");
    return TRUE;
}
