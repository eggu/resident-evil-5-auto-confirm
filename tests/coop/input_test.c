#include <assert.h>
#include <stdio.h>
#include "../../src/coop/input.h"

static uint8_t bio[0x700], keys[0x2400], system_data[0x10440];
static int notices, native_calls, fail_native;
static uint8_t *system_state(void) { return system_data; }
static uint8_t *keyboard_state(void) { return keys; }
static void record_join(const char *input) { (void)input; notices++; }
static void put(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static uint8_t THISCALL other(uint8_t *b, int *slot) {
    native_calls++;
    if (b[0x5dc] || fail_native) return 0;
    for (int i = 0; i < 2; i++)
        if (i != (int)word(b + 0x5d8) && (word(b + i * 0x2c0 + 0x198) & 8)) { *slot = i; return 1; }
    return 0;
}
static uint8_t THISCALL same(uint8_t *b, int *slot) {
    native_calls++;
    if (b[0x5dc] || b[0x5f0] || fail_native) return 0;
    *slot = (int)word(b + 0x5d8);
    return (word(b + *slot * 0x2c0 + 0x198) & 8) != 0;
}
static uint8_t THISCALL keyboard(uint8_t *b) {
    native_calls++;
    return !fail_native && !b[0x5dc] && b[0x5f0]
        && (word(keys + word(b + 0x5e8) * 0x8c0 + 0x120) & (1u << 27));
}
static void reset(void) {
    memset(bio, 0, sizeof bio); memset(keys, 0, sizeof keys); memset(system_data, 0, sizeof system_data);
    notices = native_calls = fail_native = 0;
    put(system_data + 0x10438, 0x1000);
    bio[0x38] = bio[0x3b] = bio[0x2f8] = bio[0x2fb] = 1;
    original_other = other; original_same = same; original_keyboard = keyboard;
}
static void key_chord(void) {
    put(keys + 0x120, (1u << 16) | (1u << 27));
    put(keys + 0x160, 1u << 27);
}
int main(void) {
    int slot = -1;
    reset(); put(bio + 0x198, 8); put(bio + 0x458, 8);
    assert(!join_other(bio, &slot) && !join_same(bio, &slot));
    assert(native_calls == 0 && notices == 0);
    put(bio + 0x450, 0x100);
    assert(join_other(bio, &slot) && slot == 1 && notices == 1);
    reset(); bio[0x5f0] = 1; put(bio + 0x190, 0x100); put(bio + 0x198, 8);
    assert(join_same(bio, &slot) && slot == 0 && bio[0x5f0] == 1);
    fail_native = 1; assert(!join_same(bio, &slot) && bio[0x5f0] == 1);
    fail_native = 0; bio[0x5dc] = 1; assert(!join_same(bio, &slot));
    bio[0x5dc] = 0; put(bio + 0x198, 0); assert(!join_same(bio, &slot)); // held Start never repeats
    put(bio + 0x198, 8); put(system_data + 0x10438, 0); assert(!join_same(bio, &slot));
    reset(); put(keys + 0x120, 1u << 27); put(keys + 0x160, 1u << 27);
    assert(!join_keyboard(bio)); // Esc alone is never a join request
    key_chord(); assert(join_keyboard(bio) && bio[0x5f0] == 0);
    fail_native = 1; assert(!join_keyboard(bio) && bio[0x5f0] == 0); fail_native = 0;
    put(keys + 0x160, 0); assert(!join_keyboard(bio)); // held Esc never repeats
    key_chord(); bio[0x3b] = 0; assert(!join_keyboard(bio)); // disconnected current pad
    bio[0x3b] = 1; bio[0x5dc] = 1; assert(!join_keyboard(bio)); bio[0x5dc] = 0;
    put(system_data + 0x10438, 0); assert(!join_keyboard(bio));
    reset(); key_chord(); put(keys + 0x120, 1u << 27); put(keys + 0x134, 1); assert(join_keyboard(bio));
    put(keys + 0x134, 2); assert(join_keyboard(bio)); // either physical Shift
    put(bio + 0x5d8, 0xffffffff); assert(!join_keyboard(bio));
    put(bio + 0x5d8, 0); put(bio + 0x5e8, 1); assert(!join_keyboard(bio));
    reset(); put(bio + 0x5e8, 1); put(keys + 0x8c0 + 0x120, (1u << 16) | (1u << 27));
    put(keys + 0x8c0 + 0x160, 1u << 27); assert(!join_keyboard(bio));
    puts("input checks passed");
}
