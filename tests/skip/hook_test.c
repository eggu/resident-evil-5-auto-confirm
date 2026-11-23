// Execute every actual ASI CALL replacement against the original native helper in isolation.
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/skip/signatures.h"
static volatile BYTE image_space[0x1900000];
static BYTE bio[0x700], event[0x158], system_data[0x10440], session[0x800], network[0x1900];
static uint32_t __attribute__((naked)) run_gate(BYTE *e __attribute__((unused)), unsigned frame __attribute__((unused))) {
    __asm__("push %ebp; push %ebx; push %esi; sub $0x20,%esp; mov 0x30(%esp),%esi; mov 0x34(%esp),%ebp; xor %ebx,%ebx; mov 0x154(%esi),%eax; test %eax,%eax; mov $0x7b2966,%edx; jmp *%edx");
}
static void put(BYTE *p,DWORD value) { memcpy(p,&value,4); }
static uint32_t __attribute__((thiscall,naked)) run_skip(BYTE *b __attribute__((unused)), int s __attribute__((unused)), uintptr_t site __attribute__((unused))) {
    __asm__("pop %edx; pop %eax; xchg %edx,(%esp); push %eax; jmp *%edx");
}
static unsigned logo_releases;
static void __attribute__((thiscall)) logo_release(BYTE *logo) {
    assert(logo); logo_releases++;
}
static void __attribute__((thiscall,naked)) finish_logo(BYTE *logo __attribute__((unused))) {
    __asm__("xor %eax,%eax; mov $0x84c45a,%edx; jmp *%edx");
}
static const BYTE native_logo_wait[]={0x56,0x8b,0xf1,0x8b,0x46,0x0c,0x80,0x78,0x3c,0x01,0x75,0x22,0x85,0xc0,0x74,0x18,0x81,0x60,0x04,0xff,0xdf,0xff,0xff,0x8b,0x4e,0x0c,0x8b,0x01,0x8b,0x50,0x30,0xff,0xd2,0xc7,0x46,0x0c,0x00,0x00,0x00,0x00,0x80,0x46,0x05,0x01,0x5e,0xc3,0x0f,0xb6,0x46,0x04,0x83,0xf8,0x03,0x0f,0x86,0xd8,0x00,0x00,0x00,0x8b,0x0d,0xc4,0x20,0x1b,0x01,0x53,0x57,0x6a,0x00,0x6a,0x00,0x32,0xdb,0xe8,0xf2,0x9b,0xbb,0xff,0x33,0xff,0x55,0x8b,0x0d,0x40,0x9c,0x24,0x01,0x8b,0xa9,0xb4,0x05,0x00,0x00,0x57,0xe8,0x8d,0xf0,0xb9,0xff,0x85,0xc5,0x75,0x54,0x8b,0x0d,0xc4,0x20,0x1b,0x01,0x6a,0x00,0x6a,0x01,0x6a,0x00,0xe8,0x78,0xae,0xbb,0xff,0x84,0xc0,0x75,0x3f,0x8b,0x0d,0x40,0x9c,0x24,0x01,0x6a,0xff,0xe8,0x57,0x07,0xba,0xff,0x84,0xc0,0x75,0x2e,0x8b,0x0d,0xc4,0x20,0x1b,0x01,0x6a,0x01,0xe8,0x46,0x9f,0xbb,0xff,0x84,0xc0,0x75,0x1d,0x8b,0x0d,0x40,0x9c,0x24,0x01,0x6a,0x00,0x6a,0x02,0xe8,0x43,0xf5,0xb9,0xff,0x84,0xc0,0x75,0x0a,0x83,0xc7,0x01,0x83,0xff,0x02,0x7c,0x98,0xeb,0x02,0xb3,0x01,0x8b,0x0d,0x40,0x9c,0x24,0x01,0x6a,0xff,0xe8,0x16,0x07,0xba,0xff,0x84,0xc0,0x5d,0x75,0x04,0x84,0xdb,0x74,0x3e,0x8b,0x46,0x0c,0x81,0x60,0x04,0xff,0xf7,0xff,0xff,0x8b,0x46,0x0c,0x85,0xc0,0x74,0x18,0x81,0x60,0x04,0xff,0xdf,0xff,0xff,0x8b,0x4e,0x0c,0x8b,0x11,0x8b,0x42,0x30,0xff,0xd0,0xc7,0x46,0x0c,0x00,0x00,0x00,0x00,0x8b,0x0d,0x04,0x88,0x1e,0x01,0x8b,0x11,0x8b,0x42,0x28,0x6a,0x20,0x6a,0x00,0xff,0xd0,0x80,0x46,0x05,0x01,0x5f,0x5b,0x5e,0xc3};
int main(int argc, char **argv) {
    int buttons = 1, unlock = 0, movies = 1;
    int defaults = argc > 1 && !strcmp(argv[1], "default");
    if (defaults) movies = 0;
    int unsupported = argc > 1 && !strcmp(argv[1], "unsupported");
    int previous_name = argc > 1 && !strcmp(argv[1], "previous-name");
    int conflict = previous_name || (argc > 1 && !strcmp(argv[1], "conflict"));
    const char *legacy = previous_name ? "RE5.ConvenienceTweaks.asi" : "RE5.CutsceneSkip.asi";
    if (argc > 1 && !strcmp(argv[1], "off")) buttons = unlock = movies = 0;
    if (argc > 1 && !strcmp(argv[1], "buttons")) { buttons=1; unlock=movies=0; }
    if (argc > 1 && !strcmp(argv[1], "unlock")) { unlock=1; buttons=movies=0; }
    if (argc > 1 && !strcmp(argv[1], "movies")) { movies=1; buttons=unlock=0; }
    if (argc > 1 && !strcmp(argv[1], "all")) buttons=unlock=movies=1;
    const char *ini=".\\RE5.AutoConfirm.ini";
    const char *disabled[]={"ShortSaveMessages","AutoConfirmOK","CoopInput"};
    for(unsigned i=0;i<3;i++) assert(WritePrivateProfileStringA("Features",disabled[i],"0",ini));
    assert(WritePrivateProfileStringA("Features","ExtraSkipButtons",buttons?"1":"0",ini));
    assert(WritePrivateProfileStringA("Features","UnlockSkipRestrictions",unlock?"1":"0",ini));
    assert(WritePrivateProfileStringA("Features","SkipStartupMovies",defaults ? NULL : (movies?"1":"0"),ini));
    image_space[0]=0;
    DWORD old;
    const uintptr_t pages[]={0x710000,0x720000,0x740000,0x790000,0x7b0000,0x840000,0xf60000,0xbf0000,0x11e0000,0x1230000,0x1240000};
    for(unsigned i=0;i<sizeof pages/sizeof *pages;i++)
        assert(VirtualProtect((void *)pages[i],0x10000,PAGE_EXECUTE_READWRITE,&old));
    for(unsigned i=0;i<sizeof signatures/sizeof *signatures;i++)
        memcpy((void *)signatures[i].at,signatures[i].code,signatures[i].size);
    memcpy((void *)0xbfa730,native_logo_wait,sizeof native_logo_wait);
    const uintptr_t sites[]={0x71d572,0x71facf,0x72301c,0x746eac,0x74d5ec,0x7b2b59,0x7b2b96,0xbfbb96,0xbfbc2c,0xbfbcc2,0xbfbd5d,0xbfb919,0xbfba33,0xbfac74};
    BYTE originals[14][5];
    for(unsigned i=0;i<14;i++) memcpy(originals[i],(void *)sites[i],5);
    BYTE logo_original[2][5];
    memcpy(logo_original[0],(void *)0xbfb20d,5);
    memcpy(logo_original[1],(void *)0xbfb2d9,5);
    BYTE movie_original[5];
    memcpy(movie_original,(void *)0xbfb627,5);
    *(BYTE **)0x12340a4=system_data;
    *(BYTE **)(system_data+0x1042c)=session;
    *(BYTE **)0x1249c1c=network;
    const BYTE gate_return[]={0x0f,0xb6,0x44,0x24,0x10,0x83,0xc4,0x20,0x5e,0x5b,0x5d,0xc3};
    memcpy((void *)0x7b29d2,gate_return,sizeof gate_return);
    put(event+0x2c,2); put(event+0x154,100);
    assert(run_gate(event,99)==1 && run_gate(event,100)==0);
    put(event+0x2c,0x11); assert(run_gate(event,0)==0); // reproduce native unskippable event
    if (unsupported) {
        *(BYTE **)0x1234578=bio;
        *(BYTE **)0x11e8804=event;
        *(BYTE *)0x799820 ^= 1;
        assert(LoadLibraryA("RE5.AutoConfirm.asi"));
        Sleep(400);
        for(unsigned i=0;i<14;i++) assert(!memcmp((void *)sites[i],originals[i],5));
        puts("Unsupported input helper: all scene/startup patches rejected");
        return 0;
    }
    if (conflict || (!buttons && !unlock && !movies)) {
        if (conflict) {
            HANDLE f=CreateFileA(legacy,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
            assert(f!=INVALID_HANDLE_VALUE); CloseHandle(f);
        }
        assert(LoadLibraryA("RE5.AutoConfirm.asi")); Sleep(500);
        for(unsigned i=0;i<14;i++) assert(!memcmp((void *)sites[i],originals[i],5));
        assert(!memcmp((void *)0xbfb627,movie_original,5));
        assert(!memcmp((void *)0x7b2966,event_gate,sizeof event_gate));
        if(conflict) assert(DeleteFileA(legacy));
        puts("Disabled/conflicting features: native bytes preserved"); return 0;
    }
    *(BYTE **)0x1234578=NULL;
    *(BYTE **)0x11e8804=NULL;
    DWORD loaded_at=GetTickCount();
    assert(LoadLibraryA("RE5.AutoConfirm.asi"));
    Sleep(200);
    for(unsigned i=0;i<14;i++) assert(!memcmp((void *)sites[i],originals[i],5));
    *(BYTE **)0x1234578=bio;
    *(BYTE **)0x11e8804=event;
    Sleep(300);
    assert(GetTickCount()-loaded_at < 2000); // No fixed five-second delay.
    for(unsigned i=0;i<14;i++) assert((memcmp((void *)sites[i],originals[i],5) != 0) == (i < 7 && buttons));
    for(unsigned i=0;i<sizeof signatures/sizeof *signatures;i++) {
        const struct Signature *s=signatures+i;
        for(unsigned j=0;j<s->size;j++) {
            int displacement=0;
            for(unsigned k=0;k<7;k++) if(buttons && s->at+j>sites[k] && s->at+j<sites[k]+5) displacement=1;
            if((s->at+j>=0x7b2976 && s->at+j<0x7b2980)||(s->at+j>=0x7b29b4 && s->at+j<0x7b29be)) {
                assert(((BYTE *)s->at)[j]==(unlock ? 0x90 : s->code[j])); continue;
            }
            const uintptr_t extra[]={0xbfb627,0xbfb20d,0xbfb2d9};
            for(unsigned k=0;movies && k<3;k++)
                if(s->at+j>extra[k] && s->at+j<extra[k]+5) displacement=1;
            if(!displacement) {
                if(((BYTE *)s->at)[j]!=s->code[j]) fprintf(stderr,"signature mismatch at %lx: %02x != %02x\n",(unsigned long)(s->at+j),((BYTE *)s->at)[j],s->code[j]);
                assert(((BYTE *)s->at)[j]==s->code[j]);
            } // native permission/skip branches unchanged
        }
    }
    if (unlock) {
    put(event+0x2c,2); put(event+0x154,100); network[0x186c]=0x5a;
    assert(run_gate(event,99)==1 && run_gate(event,100)==1 && run_gate(event,1000)==1);
    assert(network[0x186c]==0x5a); // skip votes survive the previously closed window
    put(event+0x2c,0x11); assert(run_gate(event,0)==1 && run_gate(event,130)==1);
    assert(*(DWORD *)(event+0x13c)==0xb); // native QTE/result transition still executes
    put(event+0x13c,0); put(event+0x2c,0x1b); assert(run_gate(event,320)==1);
    assert(*(DWORD *)(event+0x13c)==0xb);
    }
    // Startup and title CALLs must stay native, including after the repair poll.
    if (buttons) {
    memcpy((void *)sites[0], originals[0], 5);
    for(int i=0;i<20 && !memcmp((void *)sites[0],originals[0],5);i++) Sleep(100);
    assert(memcmp((void *)sites[0],originals[0],5));
    }
    for(unsigned i=7;i<14;i++) assert(!memcmp((void *)sites[i],originals[i],5));
    assert((memcmp((void *)0xbfb627,movie_original,5)!=0)==movies);
    assert((memcmp((void *)0xbfb20d,logo_original[0],5)!=0)==movies);
    assert((memcmp((void *)0xbfb2d9,logo_original[1],5)!=0)==movies);
    if(movies) {
        *(void **)0xf6d510=(void *)logo_release;
        const uintptr_t logo_sites[]={0xbfb20d,0xbfb2d9};
        typedef void (__attribute__((thiscall)) *LogoFn)(BYTE *);
        for(unsigned k=0;k<2;k++) {
            BYTE owner[0x20]={0}, logo[0x50]={0};
            put(logo,0xf6d4e0); put(logo+0x40,k); logo[6]=1;logo[0x4c]=255;
            put(owner+0xc,(DWORD)logo);owner[4]=3;
            *(BYTE *)(logo_sites[k]+5)=0xc3;
            ((LogoFn)logo_sites[k])(owner);
            assert(logo[0x14]==5 && !logo[0x4c] && !logo[0x3c]);
            finish_logo(logo); // Actual original completion instructions set +3c.
            assert(logo[0x3c]==1);
            unsigned before=logo_releases;
            ((LogoFn)logo_sites[k])(owner);
            assert(logo_releases==before+1 && !*(DWORD *)(owner+0xc) && owner[5]==1);
        }
        puts("Windows ASI: native CAPCOM/Dolby completion and owner cleanup passed");
        // Stub only the native query; execute the actual patched CALL with its thiscall ABI.
        const BYTE native_query[]={0xc6,0x41,0x18,0x5a,0x8a,0x41,0x19,0xc3};
        memcpy((void *)0xbfa8e0,native_query,sizeof native_query);
        *(BYTE *)0xbfb62c=0xc3;
        typedef BYTE (__attribute__((thiscall)) *MovieFn)(BYTE *);
        MovieFn query=(MovieFn)0xbfb627;
        const DWORD types[]={0xf52c2c,0xf52c44,0xf9db1c};
        for(unsigned t=0;t<3;t++) for(DWORD id=0;id<4;id++)
            for(unsigned state=0;state<14;state++) {
                BYTE scene[0x1c]={0}; put(scene,types[t]); put(scene+0x10,id);
                scene[4]=1; scene[5]=state;
                int eligible=state==3 && ((t==0 && id<=1)||(t==1 && id==2));
                assert(query(scene)==eligible && scene[0x18]==0x5a);
                assert(scene[0x14]==0 && scene[5]==state);
                scene[0x19]=7;
                assert(query(scene)==(eligible ? 1 : 7));
            }
        puts("Windows ASI: opt-in movie CALL preserves native query and exact scene filter");
    }
    for(unsigned i=0;i<7;i++) *(BYTE *)(sites[i]+5)=0xc3;

    const DWORD inputs[]={0,1,0x4000,0x8000,0xc000,0x2000,8,0x100};
    for(unsigned site=0;site<7;site++) for(int slot=0;slot<2;slot++)
        for(unsigned i=0;i<sizeof inputs/sizeof *inputs;i++) {
            DWORD bits=inputs[i]; memcpy(bio+slot*0x2c0+0x198,&bits,4);
            DWORD expected=bits|((buttons && (bits&0xc000))?1u:0u);
            assert(run_skip(bio,slot,sites[site])==expected);
            assert(!memcmp(bio+slot*0x2c0+0x198,&bits,4));
        }
    bio[0x5dc]=1;
    for(unsigned i=0;i<7;i++) assert(!run_skip(bio,0,sites[i]));
    puts("Windows ASI: 7 scene calls, startup/title unchanged, partial scene restoration passed");
}
