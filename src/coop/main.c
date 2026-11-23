#include <windows.h>
#include <stdio.h>
#include "../config.h"
#include "input.h"

static char log_path[MAX_PATH];
static uint8_t *system_state(void) { return *(uint8_t **)0x12340a4; }
static uint8_t *keyboard_state(void) { return *(uint8_t **)0x1249c74; }
static void record_join(const char *input) {
    FILE *out = fopen(log_path, "a");
    if (out) { fprintf(out, "[%lu] %s\n", GetTickCount(), input); fclose(out); }
}

struct Patch { uintptr_t at; uint8_t expected[5]; void *target; };
static const struct Patch patches[] = {
    {0x723559, {0xe8, 0x42, 0x78, 0x07, 0}, (void *)join_other},
    {0x72358d, {0xe8, 0x8e, 0x78, 0x07, 0}, (void *)join_same},
    {0x7235ed, {0xe8, 0xee, 0x8b, 0x07, 0}, (void *)join_keyboard},
};
#include "signatures.h"

static int readable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION m;
    return VirtualQuery(p, &m, sizeof m) && m.State == MEM_COMMIT
        && !(m.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        && (uintptr_t)p + n <= (uintptr_t)m.BaseAddress + m.RegionSize;
}

static int supported(void) {
    for (unsigned i = 0; i < sizeof patches / sizeof *patches; i++) {
        const struct Patch *p = patches + i;
        if (!readable((void *)p->at, 5) || memcmp((void *)p->at, p->expected, 5)) return 0;
    }
    for (unsigned i = 0; i < sizeof signatures / sizeof *signatures; i++) {
        const struct Signature *s = signatures + i;
        if (!readable((void *)s->at, s->size) || memcmp((void *)s->at, s->code, s->size)) return 0;
    }
    return 1;
}

static DWORD WINAPI install(void *unused) {
    (void)unused;
    Sleep(5000); // The game protector restores code during startup.
    for (unsigned i = 0; i < 600; i++) {
        AcquireSRWLockExclusive(&code_lock);
        if (supported()) {
            DWORD old, ignored;
            if (!VirtualProtect((void *)0x723000, 0x1000, PAGE_EXECUTE_READWRITE, &old)) {
                record_join("installation failed: VirtualProtect; nothing changed");
                ReleaseSRWLockExclusive(&code_lock);
                return 1;
            }
            for (unsigned j = 0; j < sizeof patches / sizeof *patches; j++) {
                uint8_t *call = (uint8_t *)patches[j].at;
                int32_t relative = (intptr_t)patches[j].target - ((intptr_t)call + 5);
                memcpy(call + 1, &relative, sizeof relative);
            }
            int restored = VirtualProtect((void *)0x723000, 0x1000, old, &ignored);
            FlushInstructionCache(GetCurrentProcess(), (void *)0x723000, 0x1000);
            record_join(restored ? "installed: Shift+Esc / LB+Start; pause screen only"
                                 : "installed; WARNING: code page protection restoration failed");
            ReleaseSRWLockExclusive(&code_lock);
            return restored ? 0 : 1;
        }
        ReleaseSRWLockExclusive(&code_lock);
        Sleep(100);
    }
    record_join("unsupported code or another mod owns join calls; nothing changed");
    return 1;
}

BOOL coop_init(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH || GetModuleHandleA(NULL) != (HMODULE)0x400000) return TRUE;
    DisableThreadLibraryCalls(instance);
    DWORD n = GetModuleFileNameA(instance, log_path, sizeof log_path);
    if (!n || n >= sizeof log_path) return TRUE;
    char *ext = strrchr(log_path, '.');
    if (!ext || ext - log_path > MAX_PATH - 5) return TRUE;
    strcpy(ext, ".log");
    record_join("loaded; checking game code (executable name independent)");
    HANDLE thread = CreateThread(NULL, 0, install, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
    else record_join("Failed to create co-op worker; nothing changed.");
    return TRUE;
}
