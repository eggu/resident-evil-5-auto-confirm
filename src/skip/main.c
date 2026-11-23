#include <windows.h>
#include <stdio.h>
#include "../config.h"
#include "skip.h"
#include "movie_skip.h"
static char log_path[MAX_PATH];

static void log_line(const char *input) {
    FILE *out = fopen(log_path, "a");
    if (out) { fprintf(out, "[%lu] %s\n", GetTickCount(), input); fclose(out); }
}

struct Patch { uintptr_t at; uint8_t expected[5]; void *target; };
static const struct Patch patches[] = {
    {0x71d572, {0xe8, 0xa9, 0xc2, 0x07, 0x00}, (void *)skip_buttons},
    {0x71facf, {0xe8, 0x4c, 0x9d, 0x07, 0x00}, (void *)skip_buttons},
    {0x72301c, {0xe8, 0xff, 0x67, 0x07, 0x00}, (void *)skip_buttons},
    {0x746eac, {0xe8, 0x6f, 0x29, 0x05, 0x00}, (void *)skip_buttons},
    {0x74d5ec, {0xe8, 0x2f, 0xc2, 0x04, 0x00}, (void *)skip_buttons},
    {0x7b2b59, {0xe8, 0xc2, 0x6c, 0xfe, 0xff}, (void *)skip_buttons},
    {0x7b2b96, {0xe8, 0x85, 0x6c, 0xfe, 0xff}, (void *)skip_buttons},
    {0xbfb627, {0xe8, 0xb4, 0xf2, 0xff, 0xff}, (void *)skip_startup_movie},
    {0xbfb20d, {0xe8, 0x1e, 0xf5, 0xff, 0xff}, (void *)skip_startup_logo},
    {0xbfb2d9, {0xe8, 0x52, 0xf4, 0xff, 0xff}, (void *)skip_startup_logo},
};
static unsigned patch_count = 10;
static int extra_buttons, unlock_scenes, movies;
static int selected(unsigned i) { return i < 7 ? extra_buttons : movies; }
#include "signatures.h"

static int readable(const void *p, size_t n) {
    MEMORY_BASIC_INFORMATION m;
    return VirtualQuery(p, &m, sizeof m) && m.State == MEM_COMMIT
        && !(m.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        && (uintptr_t)p + n <= (uintptr_t)m.BaseAddress + m.RegionSize;
}

static int supported(void) {
    // Wait for native game managers, not just visible decrypted bytes.
    const uintptr_t managers[] = {0x12340a4, 0x1234578, 0x11e8804};
    for (unsigned i = 0; i < sizeof managers / sizeof *managers; i++) {
        if (!readable((void *)managers[i], sizeof(void *))) return 0;
        void *manager = *(void **)managers[i];
        if (!manager || !readable(manager, 4)) return 0;
    }
    int changed = 0;
    for (unsigned i = 0; i < patch_count; i++) {
        if (!selected(i)) continue;
        const struct Patch *p = patches + i;
        uint8_t own[5] = {0xe8};
        int32_t relative = (intptr_t)p->target - (p->at + 5);
        memcpy(own + 1, &relative, 4);
        if (!readable((void *)p->at, 5)) return 0;
        if (!memcmp((void *)p->at, p->expected, 5)) changed = 1;
        else if (memcmp((void *)p->at, own, 5)) return 0;
    }
    for (unsigned i = 0; i < sizeof signatures / sizeof *signatures; i++) {
        const struct Signature *s = signatures + i;
        if (!readable((void *)s->at, s->size)) return 0;
        for (unsigned j = 0; j < s->size; j++) {
            uintptr_t at = s->at + j;
            uint8_t actual = *(uint8_t *)at;
            if (actual == s->code[j]) continue;
            if (((at >= 0x7b2976 && at < 0x7b2980) ||
                 (at >= 0x7b29b4 && at < 0x7b29be)) && unlock_scenes && actual == 0x90) continue;
            unsigned k;
            for (k = 0; k < patch_count; k++)
                if (selected(k) && at > patches[k].at && at < patches[k].at + 5) break;
            if (k == patch_count) return 0;
        }
    }
    if (unlock_scenes) for (unsigned i = 0; i < 10; i++)
        if (*(uint8_t *)(0x7b2976 + i) != 0x90 || *(uint8_t *)(0x7b29b4 + i) != 0x90) changed = 1;
    return changed ? 1 : 2;
}

static DWORD WINAPI install(void *unused) {
    (void)unused;
    movies = option("SkipStartupMovies", 0);
    extra_buttons = option("ExtraSkipButtons", 1);
    unlock_scenes = option("UnlockSkipRestrictions", 0);
    log_line(movies ? "startup movie and CAPCOM/Dolby logo skip enabled; title and save dialogs remain native"
                    : "startup movie skip disabled; title and save dialogs remain native");
    int installed = 0; // Reapply if startup protection restores any owned patch to its original bytes.
    for (unsigned i = 0; i < 600; i++) {
        AcquireSRWLockExclusive(&code_lock);
        if (supported() == 1) {
            const uintptr_t pages[] = {0x71d000,0x71f000,0x723000,0x746000,0x74d000,0x7b2000,0xbfb000};
            unsigned page_count = movies ? 7 : 6;
            DWORD old[7], ignored;
            unsigned ready = 0;
            for (; ready < page_count; ready++) {
                if (!VirtualProtect((void *)pages[ready], 0x1000, PAGE_EXECUTE_READWRITE, old + ready)) break;
            }
            if (ready != page_count) {
                while (ready) { --ready; VirtualProtect((void *)pages[ready],0x1000,old[ready],&ignored); }
                log_line("installation failed: VirtualProtect; nothing changed");
                ReleaseSRWLockExclusive(&code_lock);
                return 1;
            }
            for (unsigned j = 0; j < patch_count; j++) {
                if (!selected(j)) continue;
                uint8_t *call = (uint8_t *)patches[j].at;
                int32_t relative = (intptr_t)patches[j].target - ((intptr_t)call + 5);
                memcpy(call + 1, &relative, sizeof relative);
            }
            // Keep skip voting open past the native cutoff and in event 0x11; use the original completion path.
            if (unlock_scenes) {
                memset((void *)0x7b2976, 0x90, 10);
                memset((void *)0x7b29b4, 0x90, 10);
            }
            int restored = 1;
            for (unsigned j = 0; j < page_count; j++) {
                if (!VirtualProtect((void *)pages[j],0x1000,old[j],&ignored)) restored = 0;
                FlushInstructionCache(GetCurrentProcess(),(void *)pages[j],0x1000);
            }
            log_line(restored ? "installed selected scene/startup options; title and save dialogs unchanged"
                             : "installed; WARNING: code page protection restoration failed");
            if (!restored) { ReleaseSRWLockExclusive(&code_lock); return 1; }
            installed = 1;
        }
        ReleaseSRWLockExclusive(&code_lock);
        Sleep(100);
    }
    if (installed) return 0;
    log_line("unsupported code or another mod owns skip calls; nothing changed");
    return 1;
}

BOOL skip_init(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH || GetModuleHandleA(NULL) != (HMODULE)0x400000) return TRUE;
    DisableThreadLibraryCalls(instance);
    DWORD n = GetModuleFileNameA(instance, log_path, sizeof log_path);
    if (!n || n >= sizeof log_path) return TRUE;
    char *ext = strrchr(log_path, '.');
    if (!ext || ext - log_path > MAX_PATH - 5) return TRUE;
    strcpy(ext, ".log");
    log_line("loaded; checking game code (executable name independent)");
    HANDLE thread = CreateThread(NULL, 0, install, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
    else log_line("Failed to create skip worker; nothing changed.");
    return TRUE;
}
