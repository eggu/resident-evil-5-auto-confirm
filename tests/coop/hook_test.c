// Synthetic Windows process: execute the built ASI against frozen native predicates at their real VAs.
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/coop/signatures.h"

static BYTE bio[0x700], keyboard[0x2400], system_data[0x10440];
static void put(BYTE *p, DWORD value) { memcpy(p, &value, 4); }
static volatile BYTE image_space[0x1900000];
static void page(uintptr_t at) {
    DWORD old;
    assert(VirtualProtect((void *)at, 0x10000, PAGE_EXECUTE_READWRITE, &old));
}
// Enter a CALL site with the same argument/return stack shape as its native caller.
static BYTE __attribute__((thiscall,naked)) other(BYTE *b __attribute__((unused)), int *s __attribute__((unused))) {
    __asm__("pop %edx; pop %eax; push %edx; push %eax; mov $0x723559,%eax; jmp *%eax");
}
static BYTE __attribute__((thiscall,naked)) same(BYTE *b __attribute__((unused)), int *s __attribute__((unused))) {
    __asm__("pop %edx; pop %eax; push %edx; push %eax; mov $0x72358d,%eax; jmp *%eax");
}
static BYTE __attribute__((thiscall,naked)) key(BYTE *b __attribute__((unused))) {
    __asm__("mov $0x7235ed,%eax; jmp *%eax");
}
int main(int argc, char **argv) {
    image_space[0] = 0;
    assert(argc == 2 || argc == 3);
    int disabled_mode=argc==3 && !strcmp(argv[2],"off");
    int unsupported=argc==3 && !strcmp(argv[2],"unsupported");
    const char *ini = ".\\RE5.AutoConfirm.ini";
    const char *disabled[] = {"ShortSaveMessages", "AutoConfirmOK", "ExtraSkipButtons", "UnlockSkipRestrictions", "SkipStartupMovies"};
    for (unsigned i=0;i<5;i++) assert(WritePrivateProfileStringA("Features",disabled[i],"0",ini));
    assert(WritePrivateProfileStringA("Features","CoopInput",disabled_mode?"0":"1",ini));
    const uintptr_t pages[] = {0x710000,0x720000,0x760000,0x790000,0x1230000,0x1240000};
    for (unsigned i=0; i<sizeof pages/sizeof *pages; i++) page(pages[i]);
    for (unsigned i=0; i<sizeof signatures/sizeof *signatures; i++)
        memcpy((void *)signatures[i].at, signatures[i].code, signatures[i].size);
    const uintptr_t calls[] = {0x723559,0x72358d,0x7235ed};
    const BYTE originals[][5] = {{0xe8,0x42,0x78,7,0},{0xe8,0x8e,0x78,7,0},{0xe8,0xee,0x8b,7,0}};
    for (unsigned i=0; i<3; i++) {
        memcpy((void *)calls[i], originals[i], 5);
        *(BYTE *)(calls[i]+5) = 0xc3;
    }
    memcpy((void *)0x76a410, "\xb0\x01\xc3", 3);
    *(BYTE **)0x12340a4 = system_data;
    *(BYTE **)0x1249c74 = keyboard;
    *(BYTE **)0x1249c40 = bio;
    if(unsupported) *(BYTE *)0x79ada0 ^= 1;
    HMODULE asi = LoadLibraryA(argv[1]); assert(asi);
    if(disabled_mode || unsupported) {
        Sleep(6000);
        for(unsigned i=0;i<3;i++) assert(!memcmp((void *)calls[i],originals[i],5));
        puts("Disabled/unsupported co-op: native calls preserved"); return 0;
    }
    for (int i=0; i<80 && !memcmp((void *)calls[0], originals[0], 5); i++) Sleep(100);
    for (unsigned i=0; i<3; i++) assert(memcmp((void *)calls[i], originals[i], 5));
    put(system_data+0x10438, 0x1000);
    bio[0x38]=bio[0x3b]=bio[0x2f8]=bio[0x2fb]=1;
    put(bio+0x198, 8); put(bio+0x458, 8);
    int slot=-1;
    assert(!other(bio,&slot) && !same(bio,&slot));
    put(bio+0x450,0x100); assert(other(bio,&slot) && slot==1);
    put(bio+0x190,0x100); bio[0x5f0]=1;
    assert(same(bio,&slot) && slot==0 && bio[0x5f0]==1);
    put(keyboard+0x120,1u<<27); put(keyboard+0x160,1u<<27);
    assert(!key(bio));
    put(keyboard+0x120,(1u<<27)|(1u<<16)); bio[0x5f0]=0;
    assert(key(bio) && bio[0x5f0]==0);
    put(keyboard+0x160,0); assert(!key(bio));
    put(system_data+0x10438,0); assert(!same(bio,&slot) && !other(bio,&slot));
    puts("Windows ASI hook + native predicate checks passed");
    return 0;
}
