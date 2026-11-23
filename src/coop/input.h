#include <stdint.h>
#include <string.h>

#if defined(_WIN32) && defined(__i386__)
#define THISCALL __attribute__((thiscall))
#else
#define THISCALL
#endif

typedef uint8_t (THISCALL *PadJoinFn)(uint8_t *, int *);
typedef uint8_t (THISCALL *KeyboardJoinFn)(uint8_t *);
static PadJoinFn original_other = (PadJoinFn)(uintptr_t)0x79ada0;
static PadJoinFn original_same = (PadJoinFn)(uintptr_t)0x79ae20;
static KeyboardJoinFn original_keyboard = (KeyboardJoinFn)(uintptr_t)0x79c1e0;
static uint8_t *system_state(void);
static uint8_t *keyboard_state(void);
static void record_join(const char *input);

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

static int paused(void) {
    uint8_t *system = system_state();
    return system && (word(system + 0x10438) & 0x1000);
}

static int pad_available(const uint8_t *bio, int slot) {
    return slot >= 0 && slot < 2 && bio[slot * 0x2c0 + 0x38] && bio[slot * 0x2c0 + 0x3b];
}

static int pad_chord(const uint8_t *bio, int slot) {
    return pad_available(bio, slot) && (word(bio + slot * 0x2c0 + 0x190) & 0x100)
        && (word(bio + slot * 0x2c0 + 0x198) & 8);
}

static uint8_t THISCALL join_other(uint8_t *bio, int *slot) {
    if (!paused() || (!pad_chord(bio, 0) && !pad_chord(bio, 1))) return 0;
    uint8_t result = original_other(bio, slot);
    if (!result || !pad_chord(bio, *slot)) return 0;
    record_join("LB+Start (other pad)");
    return 1;
}

static uint8_t THISCALL join_same(uint8_t *bio, int *slot) {
    int current = (int)word(bio + 0x5d8);
    if (!paused() || !pad_chord(bio, current)) return 0;
    // A held modifier changes device mode; the explicit chord requests the native keyboard/pad split.
    uint8_t mode = bio[0x5f0];
    bio[0x5f0] = 0;
    uint8_t result = original_same(bio, slot);
    bio[0x5f0] = mode;
    if (result) record_join("LB+Start (keyboard/pad split)");
    return result;
}

static uint8_t THISCALL join_keyboard(uint8_t *bio) {
    uint8_t *keyboard = keyboard_state();
    unsigned bank = word(bio + 0x5e8);
    int current = (int)word(bio + 0x5d8);
    if (!paused() || !keyboard || bank != 0 || !pad_available(bio, current)) return 0;
    uint8_t *keys = keyboard + bank * 0x8c0;
    int shift = (word(keys + 0x120) & (1u << 16)) || (word(keys + 0x134) & 3);
    if (!shift || !(word(keys + 0x160) & (1u << 27))) return 0;
    uint8_t mode = bio[0x5f0];
    bio[0x5f0] = 1;
    uint8_t result = original_keyboard(bio);
    bio[0x5f0] = mode;
    if (result) record_join("Shift+Esc");
    return result;
}
