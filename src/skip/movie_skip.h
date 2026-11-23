#include <string.h>

typedef uint8_t (THISCALL *MovieInputFn)(uint8_t *);
static MovieInputFn original_movie_input = (MovieInputFn)(uintptr_t)0xbfa8e0;

static uint8_t THISCALL skip_startup_movie(uint8_t *scene) {
    uint8_t result = original_movie_input(scene); // Preserve native prompt/input side effects.
    uint32_t type, movie;
    memcpy(&type, scene, 4);
    memcpy(&movie, scene + 0x10, 4);
    if (scene[4] == 1 && scene[5] == 3 &&
        ((type == 0xf52c2c && movie <= 1) || (type == 0xf52c44 && movie == 2)))
        return 1; // The caller performs the native fade, stop and cleanup.
    return result;
}

typedef void (THISCALL *LogoWaitFn)(uint8_t *);
static LogoWaitFn original_logo_wait = (LogoWaitFn)(uintptr_t)0xbfa730;

static void THISCALL skip_startup_logo(uint8_t *scene) {
    original_logo_wait(scene); // Keep the native ownership, completion and cleanup checks.
    uint8_t *logo;
    memcpy(&logo, scene + 0xc, sizeof logo);
    if (!logo) return;
    uint32_t type, kind;
    memcpy(&type, logo, 4);
    memcpy(&kind, logo + 0x40, 4);
    if (type == 0xf6d4e0 && kind <= 1 && logo[6] == 1 && logo[0x14] < 5) {
        logo[0x14] = 5; // The next native Update signals completion; the owner frees the task.
        logo[0x4c] = 0;
    }
}
