// Simulate what the startup gate in src/main.c waits for: the game's CRT security cookie initialised at 0x11ac150
// (complement next to it) and a top-level MTFramework window of this process. The gate then waits another 3 s.
#include <windows.h>
#include <assert.h>
#define GATE_MS 3000
static void game_running(void) {
    DWORD *cookie = (DWORD *)0x11ac150, old;
    assert(VirtualProtect(cookie, 8, PAGE_READWRITE, &old));
    cookie[0] = 0x1234abcd; cookie[1] = ~cookie[0];
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA; wc.lpszClassName = "MTFramework";
    assert(RegisterClassA(&wc));
    assert(CreateWindowA("MTFramework", "", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL));
}
