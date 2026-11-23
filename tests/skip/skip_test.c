#include <assert.h>
#include <stdio.h>
#include "../../src/skip/skip.h"
#include "../../src/skip/movie_skip.h"
static uint32_t native_bits;
static uint8_t *expected_bio;
static int expected_slot, calls;
static uint32_t THISCALL native_buttons(uint8_t *bio, int slot) {
    assert(bio == expected_bio && slot == expected_slot);
    calls++;
    return native_bits;
}
static uint8_t movie_result;
static unsigned movie_calls;
static uint8_t THISCALL native_movie_input(uint8_t *scene) {
    movie_calls++;
    scene[0x18]++; // Native side effects must survive the adapter.
    return movie_result;
}
static void movie_tests(void) {
    original_movie_input = native_movie_input;
    const uint32_t types[] = {0xf52c2c, 0xf52c44, 0xf9db1c, 0};
    for (unsigned t=0;t<4;t++) for (uint32_t id=0;id<4;id++)
        for (unsigned state=0;state<14;state++) for (unsigned phase=0;phase<3;phase++)
            for (unsigned input=0;input<256;input++) {
                uint8_t scene[0x1c]={0};
                memcpy(scene,types+t,4); memcpy(scene+0x10,&id,4);
                scene[4]=phase; scene[5]=state; movie_result=input;
                unsigned before=movie_calls;
                int eligible=phase==1 && state==3 && ((t==0 && id<=1)||(t==1 && id==2));
                assert(skip_startup_movie(scene)==(eligible ? 1 : input));
                assert(movie_calls==before+1 && scene[0x18]==1);
                assert(scene[4]==phase && scene[5]==state && scene[0x14]==0);
            }
    puts("Startup movie adapter: native call preserved; exact scene/id/state filtering passed");
}
static unsigned logo_calls;
static void THISCALL native_logo_wait(uint8_t *scene) {
    uint8_t *logo;
    memcpy(&logo,scene+0xc,sizeof logo);
    logo_calls++;
    if (logo && logo[0x3c]) {
        logo=NULL;
        memcpy(scene+0xc,&logo,sizeof logo);
    }
}
static void logo_tests(void) {
    original_logo_wait=native_logo_wait;
    for (uint32_t kind=0;kind<7;kind++) {
        uint8_t scene[0x20]={0}, logo[0x50]={0}, expected[0x50];
        uint8_t *ptr=logo;
        uint32_t type=0xf6d4e0;
        memcpy(logo,&type,4); memcpy(logo+0x40,&kind,4); memcpy(scene+0xc,&ptr,sizeof ptr);
        logo[0x4c]=255; memcpy(expected,logo,sizeof logo);
        skip_startup_logo(scene);
        assert(!memcmp(logo,expected,sizeof logo)); // Init has not completed; do not change it.
        logo[6]=1; memcpy(expected,logo,sizeof logo);
        if(kind<=1) {expected[0x14]=5;expected[0x4c]=0;}
        unsigned before=logo_calls;
        skip_startup_logo(scene);
        assert(logo_calls==before+1 && !memcmp(logo,expected,sizeof logo));
        assert(!logo[0x3c]); // Completion/cleanup is still the native task's responsibility.
        logo[0x3c]=1; memcpy(expected,logo,sizeof logo);
        skip_startup_logo(scene);
        memcpy(&ptr,scene+0xc,sizeof ptr);
        assert(!ptr && !memcmp(logo,expected,sizeof logo)); // Do not use a released task.
    }
    uint8_t scene[0x20]={0}, foreign[0x50]={0}, before[0x50];
    uint8_t *ptr=foreign;
    foreign[6]=1;memcpy(scene+0xc,&ptr,sizeof ptr);memcpy(before,foreign,sizeof foreign);
    skip_startup_logo(scene);assert(!memcmp(foreign,before,sizeof foreign));
    puts("CAPCOM/Dolby logo checks passed; Init, notices, foreign tasks and native cleanup preserved");
}
int main(void) {
    movie_tests();
    logo_tests();
    uint8_t bio = 0;
    expected_bio = &bio;
    original_buttons = native_buttons;
    for (int slot=0; slot<2; slot++) {
        expected_slot=slot;
        for (uint32_t bits=0; bits<0x10000; bits++) {
            native_bits=bits;
            uint32_t result=skip_buttons(&bio,slot);
            assert((result & ~1u)==(bits & ~1u));
            assert((result & 1u)==((bits & (0xc000|1)) ? 1u : 0u));
            assert(native_bits==bits); // the shared input state is never mutated
        }
    }
    assert(calls == 2*0x10000);
    puts("Back/X/A skip checks passed; other buttons and input state preserved");
}
