// THE NOTICE'S FRAME SIDE, PROVEN (2026-10-11). Not shipped. source\notice.h WHOLE (the queue, the rules, the state machine
// that waits for the world, has the picture made on a thread, shows it and ends it), compiled against stand-ins for what
// it uses of the mod (the world's two flags, the pass's save, setup and restore, the log) and against A CLOCK OF THE
// HARNESS'S OWN (NTC_NOW), and run on a real Direct3D 9 device through DXVK with a back buffer of 1280 by 720.
// Each frame the back buffer is cleared to one grey, the mod's frame-end call is made, and the buffer is read back.
//
//     nf.exe --d3d9 <d3d9.dll> --case alive|failed|unarmed|blind|seen|off|wine
//
// THE WORLD IS WHAT THE HARNESS SAYS IT IS: each frame it adds a number of world matrices to the tracker's count
// (m4bWorldSerial) and one to the frame counter, as the mod's hooks do; 20 a frame is a menu, 1500 the world.
// WHAT THIS CANNOT SHOW, and the owner's first run did: whether the mod's own count is the right one to read. The first
// build read HDR's count, which stands still with hdr=0, and every proof here passed. tools\test-notice-rules now holds
// the source to it as text (the count is the tracker's, made outside every HDR gate); the game is the proof.
//
//   alive   a warning's notice: nothing while the world is not on the screen, nothing for the settle time, then the box
//           at the top centre and not one pixel outside it, fading in, whole, fading out, gone at ten seconds, and the
//           second notice after it; the device's blend states and the sRGB read as the game left them
//   failed  the upscaling could not start but the hooks see (the usual "could not start"): nothing while the world is
//           not on the screen, however long; then the notice for 15 seconds
//   unarmed the hooks are not armed: nothing for two minutes, then the notice
//   blind   the hooks are dead and never saw the world: nothing for 20 seconds, then 15 seconds of the notice
//   seen    the hooks died after they had seen the world: at once (after the settle time)
//   off     notice=0: nothing, ever, and nothing left waiting          wine: the same under Wine
// THE CONTROLS, each of which must end FAIL: --control noworld (case alive with the world never on the screen: the notice
// must not begin), --control early (case blind with the blind time cut to nothing by the harness's clock: "nothing for
// the first 20 seconds" must fail).
// A window of 64 by 64, never shown. Exit 0 when every proof of the case passes.
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d9.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ULONGLONG g_now = 5000000;
#define NTC_NOW() g_now
static bool dead = false, m4bHaveWorld = false, ivtWine = false, ntcOff = false, ivtInOwnDraw = false;
static unsigned __int64 m4bWorldSerial = 0;      // the tracker's count of world matrices
static volatile LONG toldVerdict = 0, frames = 0;
static unsigned g_uploads = 0;                   // world matrices the stand-in game uploads a frame
enum { MENU = 20, WORLD = 1500 };
#define IVT_CFG_COMPLAINTS 8
static char ivtCfgBad[IVT_CFG_COMPLAINTS][96] = {};
static int  ivtCfgBadN = 0, ivtCfgBadSeen = 0;
static char g_said[8][256]; static int g_saidN = 0;
static void LogRecord(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    if (g_saidN < 8) vsnprintf(g_said[g_saidN++], sizeof(g_said[0]), fmt, ap);
    va_end(ap);
}
struct RsOwnCall { RsOwnCall() {} ~RsOwnCall() {} };
struct RsOwnCreateCall { RsOwnCreateCall() {} ~RsOwnCreateCall() {} };
#include "shaders/ivt_fs_vs.h"
static IDirect3DVertexShader9* ivtFsVS = nullptr;
static IDirect3DVertexDeclaration9* ivtFsDecl = nullptr;
static bool IvtEnsureFsQuad(IDirect3DDevice9* dev)
{
    if (!ivtFsVS && FAILED(dev->CreateVertexShader((const DWORD*)kIvtFsVS, &ivtFsVS))) return false;
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    if (!ivtFsDecl && FAILED(dev->CreateVertexDeclaration(el, &ivtFsDecl))) return false;
    return true;
}
// The pass's bracket, as much of it as the notice leans on: what it saves here is what the mod's own saves too.
struct IvtSavedState { IDirect3DSurface9* rt; IDirect3DVertexShader9* vs; IDirect3DPixelShader9* ps; IDirect3DVertexDeclaration9* decl; IDirect3DBaseTexture9* tex; D3DVIEWPORT9 vp; DWORD ab, cw, z; };
static void IvtSaveState(IDirect3DDevice9* dev, IvtSavedState* o)
{
    dev->GetRenderTarget(0, &o->rt); dev->GetVertexShader(&o->vs); dev->GetPixelShader(&o->ps); dev->GetVertexDeclaration(&o->decl); dev->GetTexture(0, &o->tex);
    dev->GetViewport(&o->vp); dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &o->ab); dev->GetRenderState(D3DRS_COLORWRITEENABLE, &o->cw); dev->GetRenderState(D3DRS_ZENABLE, &o->z);
}
static void IvtSetupPass(IDirect3DDevice9* dev, const IvtSavedState*, const float c[9][4])
{
    dev->SetRenderState(D3DRS_ZENABLE, FALSE); dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F); dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    dev->SetVertexShader(ivtFsVS); dev->SetVertexDeclaration(ivtFsDecl); dev->SetPixelShaderConstantF(0, &c[0][0], 9);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT); dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT); dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}
static void IvtRestoreState(IDirect3DDevice9* dev, IvtSavedState* o)
{
    dev->SetRenderTarget(0, o->rt); dev->SetViewport(&o->vp); dev->SetVertexShader(o->vs); dev->SetPixelShader(o->ps); dev->SetVertexDeclaration(o->decl); dev->SetTexture(0, o->tex);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, o->ab); dev->SetRenderState(D3DRS_COLORWRITEENABLE, o->cw); dev->SetRenderState(D3DRS_ZENABLE, o->z);
    if (o->rt) o->rt->Release(); if (o->vs) o->vs->Release(); if (o->ps) o->ps->Release(); if (o->decl) o->decl->Release(); if (o->tex) o->tex->Release();
}
#include "notice.h"

enum { W = 1280, H = 720, GREY = 0x40 };
static IDirect3DDevice9* d = nullptr; static IDirect3DSurface9* bb = nullptr; static IDirect3DSurface9* sys = nullptr;
static unsigned char px[W * H * 4];
static int g_fail = 0;
static void Proof(bool ok, const char* what) { printf("  %s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) g_fail++; }
// One frame: `ms` later on the harness's clock, the back buffer cleared to the grey, the mod's frame-end call, the read-back.
static void Frame(unsigned ms)
{
    g_now += ms;
    frames++; m4bWorldSerial += g_uploads;
    d->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(GREY, GREY, GREY), 1.0f, 0);
    NoticeFrame(d);
    d->GetRenderTargetData(bb, sys);
    D3DLOCKED_RECT lr; sys->LockRect(&lr, nullptr, D3DLOCK_READONLY);
    for (int y = 0; y < H; y++) memcpy(px + (size_t)y * W * 4, (unsigned char*)lr.pBits + (size_t)y * lr.Pitch, W * 4);
    sys->UnlockRect();
}
static bool AllGrey() { for (size_t i = 0; i < (size_t)W * H; i++) if (px[i * 4] != GREY || px[i * 4 + 1] != GREY || px[i * 4 + 2] != GREY) return false; return true; }
// The box as the rule places it, and what the frame holds: is anything outside it not the grey; the box's top-left pixel
// (the box's own darkening, no text there); and how many pixels inside are brighter than the grey (the text).
struct Seen { bool outsideClean; int corner; int bright; unsigned x0, y0; };
static Seen Look()
{
    Seen s = {}; NoticePlace(W, H, ntcBw, &s.x0, &s.y0);
    s.outsideClean = true;
    for (unsigned y = 0; y < H; y++) for (unsigned x = 0; x < W; x++)
    {
        const unsigned char* p = px + ((size_t)y * W + x) * 4;
        const bool in = x >= s.x0 && x < s.x0 + ntcBw && y >= s.y0 && y < s.y0 + ntcBh;
        if (!in) { if (p[0] != GREY || p[1] != GREY || p[2] != GREY) s.outsideClean = false; }
        else if (p[1] > GREY + 40) s.bright++;
    }
    s.corner = px[((size_t)s.y0 * W + s.x0) * 4 + 1];
    return s;
}
// Frames of 16 ms until the picture's thread is done and the notice has begun (state 2), at most 5 real seconds.
static bool UntilShown()
{
    for (int i = 0; i < 500 && ntcState != 2; i++) { Sleep(10); Frame(16); }
    return ntcState == 2;
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; const char* which = "alive"; const char* control = "";
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i];
        else if (!strcmp(argv[i], "--case") && i + 1 < argc) which = argv[++i];
        else if (!strcmp(argv[i], "--control") && i + 1 < argc) control = argv[++i];
    }
    if (!strcmp(control, "noworld")) which = "alive"; else if (!strcmp(control, "early")) which = "blind";
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : nullptr;
    if (!m) { printf("  FAIL  could not load the Direct3D 9 library\n"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "nfproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("nfproof", "nf", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);   // never shown
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = W; pp.BackBufferHeight = H;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &d))) { printf("  FAIL  CreateDevice\n"); return 2; }
    d->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    d->CreateOffscreenPlainSurface(W, H, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, nullptr);
    d->BeginScene();
    printf("The notice's frame side, case %s, through %s\n", which, dll);
    static const char kWarnA[] = "Another mod (x.asi) replaced DLSS-IV's Direct3D hook as the game started: HDR and the render scale start only after you change the resolution once";
    static const char kWarnB[] = "DXVK could not end a state block recording when the device thread checked it at the game's start, so the game may render wrongly";

    if (!strcmp(which, "alive"))
    {
        toldVerdict = 1; g_uploads = MENU;
        for (int i = 0; i < 3; i++) Frame(16);
        Proof(AllGrey() && ntcState == 0, "nothing posted: the frame is untouched");
        Proof(!NoticeDrewThisFrame(), "and NoticeDrewThisFrame says no (the name frame generation asks, 2026-10-11)");
        NoticeFromWarn(kWarnA); NoticeFromWarn(kWarnB); NoticeFromWarn("sharpness=0.50 is set, but the sharpening pass could not be created");
        bool clean = true; for (int i = 0; i < 60; i++) { Frame(500); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0 && ntcPending == 1, "two notices wait (the third warning is not one); 30 seconds of a menu's few world matrices: nothing is drawn");
        g_uploads = NTC_WORLD_UPLOADS - 1;
        clean = true; for (int i = 0; i < 60; i++) { Frame(100); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0, "one world matrix a frame short of the mark, for six seconds: still nothing");
        g_uploads = !strcmp(control, "noworld") ? MENU : WORLD;
        clean = true; for (int i = 0; i < 14; i++) { Frame(100); clean = clean && AllGrey(); }     // 1.4 s of world
        Proof(clean && ntcState == 0, "the world is on the screen: nothing for the first second and a half");
        // the game's own states, set to values no pass leaves, to be found again after every frame from here
        d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR); d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR); d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
        d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
        Frame(200);
        Proof(ntcState == 1 || ntcState == 2, "after it, the notice is taken and its picture is being made (off this thread)");
        Proof(UntilShown(), "the picture arrives and the notice begins");
        Proof(g_saidN == 1 && !strcmp(g_said[0], "  notice: on screen for 10 s (1500 world matrices this frame): DLSS-IV: Another mod got in the way at start. Change the resolution once, or restart the game."), "the log says which, once, with the frame's count");
        const ULONGLONG t0 = ntcT0;
        g_now = t0 + 250 - 16; Frame(16);
        Seen full = Look();
        Proof(ntcBw > 300 && ntcBh > 30 && full.x0 == (W - ntcBw) / 2 && full.y0 == NoticeTop(H), "the box is at the top centre, 3 percent down");
        Proof(full.outsideClean, "whole: not one pixel outside the box is changed");
        Proof(NoticeDrewThisFrame(), "a notice was drawn into this frame: NoticeDrewThisFrame says yes");
        Proof(full.corner >= 23 && full.corner <= 26 && full.bright > 500, "whole: the box darkens the frame to 38 percent and the text is in it");
        g_now = t0 + 125 - 16; Frame(16);
        Seen half = Look();
        Proof(half.outsideClean && half.corner >= 43 && half.corner <= 46, "half faded in: half of the darkening");
        g_now = t0 + 5000 - 16; Frame(16);
        Seen mid = Look();
        Proof(mid.corner == full.corner && mid.bright == full.bright && mid.outsideClean, "five seconds in: the same picture");
        g_now = t0 + 9800 - 16; Frame(16);
        Seen out = Look();
        Proof(out.corner >= 43 && out.corner <= 46 && out.outsideClean, "200 ms before its end: half faded out");
        DWORD s0 = 0, s1 = 0, s2 = 0, ab = 1, srgb = 0;
        d->GetRenderState(D3DRS_SRCBLEND, &s0); d->GetRenderState(D3DRS_DESTBLEND, &s1); d->GetRenderState(D3DRS_BLENDOP, &s2); d->GetRenderState(D3DRS_ALPHABLENDENABLE, &ab);
        d->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &srgb);
        Proof(s0 == (DWORD)D3DBLEND_DESTCOLOR && s1 == (DWORD)D3DBLEND_SRCCOLOR && s2 == (DWORD)D3DBLENDOP_MAX && ab == FALSE && srgb == TRUE, "the game's blend states and its sRGB read are as it left them");
        g_now = t0 + 10000 - 16; Frame(16);
        Proof(AllGrey() && ntcState == 0 && ntcTex == nullptr, "at ten seconds: nothing is drawn and the picture is given back");
        Proof(!NoticeDrewThisFrame(), "and NoticeDrewThisFrame says no again, on the first frame without it");
        Frame(16);
        Proof(UntilShown() && g_saidN == 2 && strstr(g_said[1], "DLSS-IV: The game may render wrongly. Set threaded=0 in DLSS-IV.cfg."), "the second notice follows, said in the log");
        g_now = ntcT0 + 10000; Frame(16); Frame(16);
        bool quiet = true; for (int i = 0; i < 20; i++) { Frame(1000); quiet = quiet && AllGrey(); }
        Proof(quiet && ntcState == 0 && ntcPending == 0, "after it nothing waits and nothing is drawn");
        NoticeFromWarn(kWarnA);
        quiet = true; for (int i = 0; i < 20; i++) { Frame(1000); quiet = quiet && AllGrey(); }
        Proof(quiet && ntcPending == 0, "the same warning again in the same session: no second notice");
    }
    else if (!strcmp(which, "failed") || !strcmp(which, "unarmed"))
    {
        const bool armed = !strcmp(which, "failed");
        dead = false; toldVerdict = armed ? 1 : 0; g_uploads = armed ? MENU : WORLD;
        NoticeFatal(false, false);
        Frame(16);
        const ULONGLONG first = g_now;
        bool clean = true; for (int i = 0; i < 118; i++) { Frame(1000); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0 && ntcPending == 1, armed ? "the upscaling failed, the hooks see: 118 seconds of loading and menus, nothing is drawn (no 20-second rule here)"
                                                               : "the hooks are not armed: nothing for 118 seconds, whatever the game uploads");
        if (armed) g_uploads = WORLD; else { g_now = first + NTC_UNARMED_MS - 16; Frame(16); }
        clean = true; for (int i = 0; i < 14; i++) { Frame(100); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0, armed ? "the world comes on the screen: nothing for the settle time" : "at two minutes: nothing for the settle time");
        Frame(200);
        Proof(UntilShown(), "then the notice begins");
        Proof(g_saidN == 1 && strstr(g_said[0], "on screen for 15 s (") && strstr(g_said[0], "): DLSS-IV could not start, so the game runs without it."), "it is \"could not start\", for 15 seconds");
        const ULONGLONG t0 = ntcT0;
        g_now = t0 + 12000 - 16; Frame(16);
        Seen s = Look();
        Proof(s.outsideClean && s.bright > 500 && s.corner >= 23 && s.corner <= 26, "twelve seconds in it still shows whole, and nothing outside the box is changed");
        g_now = t0 + 15000 - 16; Frame(16);
        Proof(AllGrey() && ntcState == 0, "at fifteen seconds it is gone");
    }
    else if (!strcmp(which, "blind") || !strcmp(which, "seen"))
    {
        const bool seen = !strcmp(which, "seen");
        dead = true; toldVerdict = 1; m4bHaveWorld = seen; g_uploads = 0;
        NoticeFatal(seen, false);
        Frame(16);
        if (!strcmp(control, "early")) g_now += 30000;      // the control: the harness's clock jumps the blind time
        const ULONGLONG first = g_now;
        if (!seen)
        {
            bool clean = true; for (int i = 0; i < 39; i++) { Frame(500); clean = clean && AllGrey(); }       // to 19.5 s
            Proof(clean && ntcState == 0, "the mod never saw the world: nothing for the first 20 seconds");
            g_now = first + 20000 - 16; Frame(16);
        }
        bool clean = true; for (int i = 0; i < 14; i++) { Frame(100); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0, "then nothing for the settle time of a second and a half");
        Frame(200);
        Proof(UntilShown(), "then the notice begins");
        Proof(g_saidN == 1 && strstr(g_said[0], "on screen for 15 s (") && strstr(g_said[0], seen ? "): DLSS-IV stopped, so the game now runs without it." : "): DLSS-IV could not start, so the game runs without it."), "it is the right one of the two, for 15 seconds");
        const ULONGLONG t0 = ntcT0;
        g_now = t0 + 12000 - 16; Frame(16);
        Seen s = Look();
        Proof(s.outsideClean && s.bright > 500 && s.corner >= 23 && s.corner <= 26, "twelve seconds in it still shows whole, and nothing outside the box is changed");
        g_now = t0 + 15000 - 16; Frame(16);
        Proof(AllGrey() && ntcState == 0, "at fifteen seconds it is gone");
    }
    else if (!strcmp(which, "off") || !strcmp(which, "wine"))
    {
        if (!strcmp(which, "off")) ntcOff = true; else ivtWine = true;
        toldVerdict = 1; g_uploads = WORLD;
        NoticeFromWarn(kWarnA); NoticeFatal(false, false);
        Proof(ntcPending == 1, "two notices were posted (control)");
        bool clean = true; for (int i = 0; i < 60; i++) { Frame(500); clean = clean && AllGrey(); }
        Proof(clean && ntcState == 0 && ntcPending == 0 && g_saidN == 0, "30 seconds with the world on the screen: nothing is drawn, nothing is said, nothing waits");
    }
    else { printf("  FAIL  unknown case\n"); g_fail++; }
    d->EndScene();
    printf(g_fail ? "FAIL: %d\n" : "PASS\n", g_fail);
    return g_fail ? 1 : 0;
}
