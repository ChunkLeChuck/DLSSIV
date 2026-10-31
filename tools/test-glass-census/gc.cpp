// THE GLASS CENSUS'S PROOF (2026-10-10; roadmap item 5). Not shipped. The census's own text, cut from source\DLSS-IV.cpp by
// prove.py into gc_cut.h, compiled here against stand-ins for the five things of the mod's it touches (the log, the path
// beside GTAIV.exe, the frame counter, the bound second targets, the seam's flag), and run on a real Direct3D 9 device
// with pixel shaders made from the game's own bytecode (prove.py takes three fragments out of the game's .fxc files).
// It plays 1801 frames of five kinds of draw and exits; prove.py reads the table the census wrote and the files it dumped.
//
//     gc.exe --d3d9 <d3d9.dll> --side <folder> --log <file> --ps <a.bin> --ps <b.bin> --ps <c.bin> [--mutant 1|2]
//
// A hidden window of 64 by 64, never shown. Mutant 1 draws the second kind with the first kind's depth write (two kinds
// must then become one); mutant 2 never changes the third kind's alpha reference (its mark for "differed" must stay off).
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d9.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile LONG frames = 0;
static IDirect3DSurface9* curRT123[3] = { nullptr, nullptr, nullptr };
static bool seamThisFrame = false;
static bool gcOn = false;
static char g_side[MAX_PATH];
static FILE* g_log = nullptr;
static void LogRecord(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap); fputc('\n', g_log);
}
static bool IvtSideFilePath(char* path, bool, const char* name)
{
    return _snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\%s", g_side, name) >= 0;
}
static void GlassCensusInit();
static void GlassCensus(IDirect3DDevice9* dev, int kind, UINT prims);
#include "gc_cut.h"

static IDirect3DPixelShader9* Load(IDirect3DDevice9* d, const char* path)
{
    FILE* f = fopen(path, "rb"); if (!f) { printf("FAIL cannot open %s\n", path); exit(2); }
    static BYTE buf[65536]; const size_t n = fread(buf, 1, sizeof(buf), f); fclose(f);
    IDirect3DPixelShader9* ps = nullptr;
    if (n < 8 || FAILED(d->CreatePixelShader((const DWORD*)buf, &ps))) { printf("FAIL CreatePixelShader %s\n", path); exit(2); }
    return ps;
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; const char* logp = nullptr; const char* psp[3] = {}; int nps = 0, mutant = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i];
        else if (!strcmp(argv[i], "--side") && i + 1 < argc) strcpy_s(g_side, argv[++i]);
        else if (!strcmp(argv[i], "--log") && i + 1 < argc) logp = argv[++i];
        else if (!strcmp(argv[i], "--ps") && i + 1 < argc && nps < 3) psp[nps++] = argv[++i];
        else if (!strcmp(argv[i], "--mutant") && i + 1 < argc) mutant = atoi(argv[++i]);
    }
    if (!dll || !logp || !g_side[0] || nps != 3) { printf("FAIL arguments\n"); return 2; }
    g_log = fopen(logp, "w"); if (!g_log) { printf("FAIL log\n"); return 2; }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = LoadLibraryA(dll);
    if (!m) { printf("FAIL could not load %s\n", dll); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("FAIL Direct3DCreate9\n"); return 2; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "gcproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("gcproof", "gc", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);   // never shown
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    IDirect3DDevice9* d = nullptr;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &d))) { printf("FAIL CreateDevice\n"); return 2; }
    IDirect3DPixelShader9* ps[3]; for (int i = 0; i < 3; i++) ps[i] = Load(d, psp[i]);

    // THE SWITCH: no folder, no census; the folder, the census.
    GlassCensusInit();
    fprintf(g_log, "HARNESS without the folder: gcOn %d\n", gcOn ? 1 : 0);
    char dir[MAX_PATH]; IvtSideFilePath(dir, false, "DLSSIV-glass"); CreateDirectoryA(dir, nullptr);
    GlassCensusInit();
    fprintf(g_log, "HARNESS with the folder: gcOn %d\n", gcOn ? 1 : 0);
    if (!gcOn) { fclose(g_log); return 3; }

    auto over = [&](DWORD dst, DWORD zw) {
        d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); d->SetRenderState(D3DRS_DESTBLEND, dst);
        d->SetRenderState(D3DRS_ZWRITEENABLE, zw); };
    for (LONG f = 0; f <= 1800; f++)
    {
        frames = f;
        // A: the first shader, blended over, depth writes off, alpha test on at 100; indexed, 10 primitives
        d->SetPixelShader(ps[0]); over(D3DBLEND_INVSRCALPHA, FALSE);
        d->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); d->SetRenderState(D3DRS_ALPHAREF, 100); d->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        curRT123[0] = nullptr; seamThisFrame = false;
        GlassCensus(d, 0, 10);
        // B: the same shader with depth writes ON: a kind of its own; drawn twice, 7 primitives each
        over(D3DBLEND_INVSRCALPHA, mutant == 1 ? FALSE : TRUE);
        GlassCensus(d, 0, 7); GlassCensus(d, 0, 7);
        // C: the second shader, added, alpha reference 0 on even frames and 8 on odd ones (it "differed"); unindexed
        d->SetPixelShader(ps[1]); over(D3DBLEND_ONE, FALSE);
        d->SetRenderState(D3DRS_ALPHAREF, (mutant != 2 && (f & 1)) ? 8 : 0);
        GlassCensus(d, 1, 3);
        // D: the third shader, not blended, with a second target bound; user-pointer draw
        d->SetPixelShader(ps[2]); d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); d->SetRenderState(D3DRS_ALPHAREF, 0);   // the reference back, or this kind and the next would "differ" too
        curRT123[0] = (IDirect3DSurface9*)(ULONG_PTR)1;
        GlassCensus(d, 2, 2);
        // E: no pixel shader, after the seam; indexed user-pointer draw
        d->SetPixelShader(nullptr); curRT123[0] = nullptr; seamThisFrame = true;
        GlassCensus(d, 3, 1);
    }
    fprintf(g_log, "HARNESS done\n");
    fclose(g_log);
    printf("gc.exe: 1801 frames played\n");
    return 0;
}
