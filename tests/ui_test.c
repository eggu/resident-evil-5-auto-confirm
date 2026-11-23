// Synthetic image: test the retained UI patches and ensure menu code stays untouched.
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static int save, confirm;
int option(const char *name, int fallback) {
    if (!strcmp(name,"ShortSaveMessages")) return save;
    if (!strcmp(name,"AutoConfirmOK")) return confirm;
    return fallback;
}
#define Sleep(ms) ((void)(ms))
#include "../src/ui/main.c"
static volatile BYTE image_space[0x1900000];
int main(int argc, char **argv) {
    image_space[0]=0;
    assert(argc==2);
    int bad_save=!strcmp(argv[1],"unsupported") || !strcmp(argv[1],"confirm-only-save-conflict");
    int bad_slot=!strcmp(argv[1],"slot-conflict") || !strcmp(argv[1],"save-only-slot-conflict");
    save=!strcmp(argv[1],"save") || !strcmp(argv[1],"all") || !strcmp(argv[1],"unsupported") || bad_slot;
    confirm=!strcmp(argv[1],"confirm") || !strcmp(argv[1],"all") || !strcmp(argv[1],"unsupported") || !strcmp(argv[1],"slot-conflict") || !strcmp(argv[1],"confirm-only-save-conflict");
    DWORD old;
    const DWORD pages[]={0x7a0000,0x9e0000,0xa10000,0xa30000,0xa40000,0xa50000,0xf90000};
    for(unsigned i=0;i<sizeof pages/sizeof *pages;i++)
        assert(VirtualProtect((void *)pages[i],0x10000,PAGE_EXECUTE_READWRITE,&old));
    void *slot_before=bad_slot ? (void *)0x12345678 : UPDATE_ORIGINAL;
    *UPDATE_SLOT=slot_before;
    for(unsigned i=0;i<3;i++) memcpy((void *)save_message_sites[i],save_message_code,8);
    if(bad_save) *(BYTE *)save_message_sites[1] ^= 1;
    BYTE save_before[3][8];
    for(unsigned i=0;i<3;i++) memcpy(save_before[i],(void *)save_message_sites[i],8);
    const DWORD untouched[]={0xa4a6c4,0xa4ea4c,0xa5201c,0xa5432e,0xa39892,0xa39961,0xa39c54,0xa39f58,0xa3af9f,0xa3bc92,0xa3c4ad,0xa3c660,0xa3cfce,0xa3d3d8,0xa3d83d,0xa3e133,0xa3e3de,0xa3e512,0xa3e84a,0xa3ebe8,0xa3f1f0,0xa4754b,0xa479b3,0xa47ee6,0xa480a7,0xa48581,0xa48fba,0xa492e9,0xa4996d,0xa49b03,0xa49f1f,0xa4d6d3,0xa4e093,0xa4f061,0xa51664,0xa51bd8,0xa52612,0xa52de1,0xa53470,0xa5387f,0xa53b43,0xa540c7,0xa59bfa,0xa580c2,0xa57981,0xa39994,0xa39c87,0xa3e890,0xa171f0,0xa17280};
    for(unsigned i=0;i<sizeof untouched/sizeof *untouched;i++) memset((void *)untouched[i],0xa5,8);
    for(unsigned i=0;i<sizeof pages/sizeof *pages;i++)
        assert(VirtualProtect((void *)pages[i],0x10000,PAGE_EXECUTE_READ,&old));
    int supported=!(save && bad_save) && !(confirm && bad_slot);
    assert(ui_supported()==supported);
    assert(install(NULL)==(supported ? 0u : 1u));
    assert((*UPDATE_SLOT!=slot_before)==(confirm && supported));
    for(unsigned i=0;i<3;i++)
        assert((memcmp((void *)save_message_sites[i],save_before[i],8)!=0)==(save && supported));
    for(unsigned i=0;i<sizeof untouched/sizeof *untouched;i++)
        for(unsigned j=0;j<8;j++) assert(((BYTE *)untouched[i])[j]==0xa5);
    for(unsigned i=0;i<sizeof pages/sizeof *pages;i++) {
        MEMORY_BASIC_INFORMATION m;
        assert(VirtualQuery((void *)pages[i],&m,sizeof m));
        assert(m.Protect==PAGE_EXECUTE_READ);
    }
    puts("UI installer: retained switches, conflicts, unchanged menus and page protections checked");
    return 0;
}
