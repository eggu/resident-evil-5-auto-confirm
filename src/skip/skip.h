#include <stdint.h>
#if defined(_WIN32) && defined(__i386__)
#define THISCALL __attribute__((thiscall))
#else
#define THISCALL
#endif

typedef uint32_t (THISCALL *ButtonsFn)(uint8_t *, int);
static ButtonsFn original_buttons = (ButtonsFn)(uintptr_t)0x799820;

// Only the native cutscene/event skip calls use this result; no Back event is sent to the map.
static uint32_t THISCALL skip_buttons(uint8_t *bio, int slot) {
    uint32_t buttons = original_buttons(bio, slot);
    return buttons | ((buttons & 0xc000) ? 1u : 0u); // native A=4000, X=8000 (both before/after remapping)
}
