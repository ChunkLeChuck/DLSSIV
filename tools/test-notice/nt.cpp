// THE ON-SCREEN NOTICE'S PROOF (2026-10-10): the shipping picture (transport/notice_pass.h, NoticeRaster), the shipping
// shader (ivt_notice_ps.h) and the shipping draw (NoticeUpload, NoticeDraw) on the GPU, with no game, over a frame
// whose every pixel is known. tools/test-mask is the pattern.
//
// THE PROOF, at 1920x1080 and at 3840x2160 (the picture's size follows the screen), on an 8-bit and on an FP16 target
// (the back buffer with HDR on):
//   1  THE PICTURE IS MADE: a box wider than tall, inside 60 percent of the screen's width, its first line's height
//      as the rule says; it holds text (pixels of full coverage) and box (pixels of none), and nothing else: every
//      pixel's alpha is the box's or above, every colour is at or under its alpha (premultiplied).
//   2  NOTHING OUTSIDE THE BOX CHANGES: every pixel of the frame outside the box's rectangle is the same bits after
//      the draw as before it.
//   3  INSIDE, THE BLEND AS WRITTEN: each pixel is the picture's colour plus the frame times (1 - the picture's
//      alpha), to one step of the target; so a box pixel is the frame darkened to 38 percent and a text pixel is white.
//   4  THE BOX STANDS WHERE THE RULE PUTS IT: centred, 3 percent of the height from the top; texel for pixel (the
//      first and last column and row of the picture land on the box's first and last column and row).
//   5  HALF SHOWN IS HALF: with show = 0.5 each pixel is half the picture's colour plus the frame times (1 - half its
//      alpha). And show = 0 changes nothing at all.
//   6  THE TARGET'S ALPHA IS NOT WRITTEN.
//   7  THE FOUR BLEND STATES THE DRAW BORROWS ARE BACK as they were (set to odd values first).
// NEGATIVE CONTROLS, each of which must FAIL the proof named: the blend left off (proof 3), the box's place moved
// by one pixel in the shader's constants (proofs 3 and 4), the picture not premultiplied (proof 3).
//
//     tools\test-notice\build.ps1
//     nt.exe --d3d9 <DXVK's d3d9.dll> [--out <folder>]
// --out writes the picture and the drawn frames as raw files (bgra-WxH.raw) for tools\test-notice\picture.py, which
// lays the notice over a frame of the game for a person to look at.
// Exit 0 when every proof passed. It changes nothing on the machine. Its window is 64 by 64 and is never shown.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "../../source/shaders/ivt_fs_vs.h"
#include "../../source/shaders/ivt_notice_ps.h"
#include "../../source/transport/notice_pass.h"

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
static void Control(bool failsAsItMust, const char* what)
{
    printf("  %s  CONTROL (%s)%s\n", failsAsItMust ? "PASS" : "FAIL", what, failsAsItMust ? ": fails the proof, as it must" : ": THE PROOF CANNOT SEE THIS FAULT");
    if (!failsAsItMust) gFails++;
}
#define CK(x) do { HRESULT hr_ = (x); if (FAILED(hr_)) { printf("  FAIL  %s: 0x%08lX (line %d)\n", #x, (unsigned long)hr_, __LINE__); exit(2); } } while (0)

static uint32_t gRng = 0x2468ACE1u;
static uint32_t Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return gRng; }

struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr; IDirect3DPixelShader9* ps = nullptr;
    IDirect3DTexture9* rt = nullptr; IDirect3DSurface9* rtSurf = nullptr; IDirect3DSurface9* sys = nullptr;
    int w = 0, h = 0; bool fp16 = false;
};
static void Size(Dev& v, int w, int h, bool fp16)
{
    for (IUnknown* u : { (IUnknown*)v.rtSurf, (IUnknown*)v.rt, (IUnknown*)v.sys }) if (u) u->Release();
    v.w = w; v.h = h; v.fp16 = fp16;
    const D3DFORMAT f = fp16 ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8;
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, f, D3DPOOL_DEFAULT, &v.rt, nullptr)); CK(v.rt->GetSurfaceLevel(0, &v.rtSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, f, D3DPOOL_SYSTEMMEM, &v.sys, nullptr));
}
// THE COMMON STATE OF A PASS OF THE MOD'S (IvtSetupPass, DlssIvTransport.h)
static void Common(Dev& v)
{
    IDirect3DDevice9* d = v.d;
    d->SetRenderTarget(1, nullptr); d->SetDepthStencilSurface(nullptr);
    d->SetRenderState(D3DRS_ZENABLE, FALSE); d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    d->SetRenderState(D3DRS_FOGENABLE, FALSE); d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    d->SetVertexShader(v.vs); d->SetVertexDeclaration(v.decl);
    const float zero[9][4] = {}; d->SetPixelShaderConstantF(0, &zero[0][0], 9);
    for (int t = 0; t < 3; t++)
    {
        d->SetSamplerState(t, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(t, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        d->SetSamplerState(t, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        d->SetSamplerState(t, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(t, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    }
}

// IEEE half (tools/test-rain's)
static uint16_t ToHalf(float f)
{
    uint32_t x; memcpy(&x, &f, 4);
    const uint32_t sign = (x >> 16) & 0x8000u; const uint32_t ex = (x >> 23) & 0xFFu; uint32_t man = x & 0x7FFFFFu;
    if (ex == 0xFF) return (uint16_t)(sign | 0x7C00u | (man ? 0x200u : 0u));
    int e = (int)ex - 127 + 15;
    if (e >= 31) return (uint16_t)(sign | 0x7C00u);
    if (e <= 0)
    {
        if (e < -10) return (uint16_t)sign;
        man |= 0x800000u; const int shift = 14 - e;
        uint32_t hh = man >> shift; const uint32_t rem = man & ((1u << shift) - 1u), half = 1u << (shift - 1);
        if (rem > half || (rem == half && (hh & 1u))) hh++;
        return (uint16_t)(sign | hh);
    }
    uint32_t hh = ((uint32_t)e << 10) | (man >> 13); const uint32_t rem = man & 0x1FFFu;
    if (rem > 0x1000u || (rem == 0x1000u && (hh & 1u))) hh++;
    return (uint16_t)(sign | hh);
}
static float FromHalf(uint16_t hh)
{
    const uint32_t sign = (uint32_t)(hh & 0x8000u) << 16; const uint32_t ex = (hh >> 10) & 0x1Fu; const uint32_t man = hh & 0x3FFu;
    uint32_t x;
    if (ex == 0) { if (!man) x = sign; else { int e = -1; uint32_t m = man; do { e++; m <<= 1; } while (!(m & 0x400u)); x = sign | ((uint32_t)(127 - 15 - e) << 23) | ((m & 0x3FFu) << 13); } }
    else if (ex == 31) x = sign | 0x7F800000u | (man << 13);
    else x = sign | ((ex + 127 - 15) << 23) | (man << 13);
    float f; memcpy(&f, &x, 4); return f;
}

// A frame as four floats a pixel, r g b a, each 0 to 1 (what an 8-bit target can hold exactly: n / 255)
typedef std::vector<float> Img;
static Img Frame(int w, int h)
{
    Img p((size_t)w * h * 4);
    for (size_t i = 0; i < p.size(); i++) p[i] = (float)(Rnd() & 255u) / 255.0f;
    return p;
}
static void Put(Dev& v, const Img& p)
{
    D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, 0));
    for (int y = 0; y < v.h; y++)
    {
        BYTE* row = (BYTE*)lr.pBits + (size_t)y * lr.Pitch;
        for (int x = 0; x < v.w; x++)
        {
            const float* s = &p[((size_t)y * v.w + x) * 4];
            if (v.fp16) { uint16_t* d = (uint16_t*)row + (size_t)x * 4; for (int c = 0; c < 4; c++) d[c] = ToHalf(s[c]); }
            else { BYTE* d = row + (size_t)x * 4; d[2] = (BYTE)(s[0] * 255.0f + 0.5f); d[1] = (BYTE)(s[1] * 255.0f + 0.5f); d[0] = (BYTE)(s[2] * 255.0f + 0.5f); d[3] = (BYTE)(s[3] * 255.0f + 0.5f); }
        }
    }
    v.sys->UnlockRect();
    CK(v.d->UpdateSurface(v.sys, nullptr, v.rtSurf, nullptr));
}
static Img Get(Dev& v)
{
    CK(v.d->GetRenderTargetData(v.rtSurf, v.sys));
    Img p((size_t)v.w * v.h * 4); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; y++)
    {
        const BYTE* row = (const BYTE*)lr.pBits + (size_t)y * lr.Pitch;
        for (int x = 0; x < v.w; x++)
        {
            float* d = &p[((size_t)y * v.w + x) * 4];
            if (v.fp16) { const uint16_t* s = (const uint16_t*)row + (size_t)x * 4; for (int c = 0; c < 4; c++) d[c] = FromHalf(s[c]); }
            else { const BYTE* s = row + (size_t)x * 4; d[0] = s[2] / 255.0f; d[1] = s[1] / 255.0f; d[2] = s[0] / 255.0f; d[3] = s[3] / 255.0f; }
        }
    }
    v.sys->UnlockRect(); return p;
}

// What a run found. kind: 0 the shipping draw; 1 the blend left off; 2 the box's place off by one in the constants;
// 3 the picture's colour not premultiplied (the text's white written whole where the coverage is partial)
struct Res { long outsideChanged, insideWrong, insideN, textPx, boxPx, edgeWrong, alphaChanged; double worst; bool statesBack; };
static Res Run(Dev& v, const NoticeBitmap& bm, float show, int kind, const char* outDir)
{
    Res r = {}; IDirect3DDevice9* d = v.d;
    Img before = Frame(v.w, v.h);
    if (v.fp16) for (size_t i = 0; i < before.size(); i++) before[i] = FromHalf(ToHalf(before[i]));   // as the target holds it
    Put(v, before);
    NoticeBitmap use = bm; std::vector<unsigned char> spoiled;
    if (kind == 3)
    {
        spoiled.assign(bm.bgra, bm.bgra + (size_t)bm.w * bm.h * 4);
        for (size_t i = 0; i < (size_t)bm.w * bm.h; i++) if (spoiled[i * 4]) spoiled[i * 4] = spoiled[i * 4 + 1] = spoiled[i * 4 + 2] = 255;
        use.bgra = spoiled.data();
    }
    IDirect3DTexture9* tex = nullptr; CK(NoticeUpload(d, &use, &tex));
    Common(v);
    // the three states the draw borrows, set to values no pass of the mod's leaves, and a separate alpha blend ON: the draw
    // leaves that one alone (it writes no alpha), so the picture must come out right with it on and it must still be on after
    d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR); d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
    d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX); d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    unsigned x0, y0; NoticePlace((unsigned)v.w, (unsigned)v.h, bm.w, &x0, &y0);
    if (kind == 0 || kind == 3) CK(NoticeDraw(d, v.rtSurf, tex, v.ps, (unsigned)v.w, (unsigned)v.h, bm.w, bm.h, show));
    else
    {   // the same draw written out, with one thing wrong
        const float l = 2.0f * (float)x0 / (float)v.w - 1.0f, rr = 2.0f * (float)(x0 + bm.w) / (float)v.w - 1.0f;
        const float t = 1.0f - 2.0f * (float)y0 / (float)v.h, b = 1.0f - 2.0f * (float)(y0 + bm.h) / (float)v.h;
        const float quad[6][4] = { { l, b, 0, 1 }, { l, t, 0, 1 }, { rr, t, 0, 1 }, { l, b, 0, 1 }, { rr, t, 0, 1 }, { rr, b, 0, 1 } };
        const float c[2][4] = { { (float)x0 + (kind == 2 ? 1.0f : 0.0f), (float)y0, 1.0f / (float)bm.w, 1.0f / (float)bm.h }, { show, 0, 0, 0 } };
        const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f };
        d->SetRenderTarget(0, v.rtSurf); d->SetViewport(&vp);
        d->SetRenderState(D3DRS_ALPHABLENDENABLE, kind == 1 ? FALSE : TRUE);
        d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD); d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        d->SetRenderState(D3DRS_COLORWRITEENABLE, 7);
        d->SetTexture(0, tex); d->SetPixelShaderConstantF(0, &c[0][0], 2); d->SetPixelShader(v.ps);
        CK(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quad, sizeof(quad[0])));
        d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR); d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
        d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX); d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    }
    DWORD s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    d->GetRenderState(D3DRS_SRCBLEND, &s0); d->GetRenderState(D3DRS_DESTBLEND, &s1); d->GetRenderState(D3DRS_BLENDOP, &s2); d->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &s3);
    r.statesBack = s0 == (DWORD)D3DBLEND_DESTCOLOR && s1 == (DWORD)D3DBLEND_SRCCOLOR && s2 == (DWORD)D3DBLENDOP_MAX && s3 == (DWORD)TRUE;
    d->SetTexture(0, nullptr);
    const Img after = Get(v);
    tex->Release();
    // one step of the target: 1/255 on eight bits; on FP16 the half float's own step near 1 is far finer, so a step of
    // the PICTURE (it is eight bits) is the bound there too
    const double step = 1.0 / 255.0 + 1e-6;
    for (int y = 0; y < v.h; y++) for (int x = 0; x < v.w; x++)
    {
        const size_t i = ((size_t)y * v.w + x) * 4;
        const bool in = x >= (int)x0 && x < (int)(x0 + bm.w) && y >= (int)y0 && y < (int)(y0 + bm.h);
        if (after[i + 3] != before[i + 3]) r.alphaChanged++;
        if (!in) { if (memcmp(&after[i], &before[i], 3 * sizeof(float)) != 0) r.outsideChanged++; continue; }
        const unsigned char* p = bm.bgra + ((size_t)(y - (int)y0) * bm.w + (x - (int)x0)) * 4;
        const double a = show * p[3] / 255.0;
        bool wrong = false;
        for (int c = 0; c < 3; c++)
        {
            const double want = show * p[2 - c] / 255.0 + before[i + c] * (1.0 - a);
            const double e = fabs((double)after[i + c] - want);
            if (e > r.worst) r.worst = e;
            if (e > step) wrong = true;
        }
        r.insideN++;
        if (wrong) { r.insideWrong++; if (x == (int)x0 || x == (int)(x0 + bm.w - 1) || y == (int)y0 || y == (int)(y0 + bm.h - 1)) r.edgeWrong++; }
        if (p[0] == 255) r.textPx++; else if (p[0] == 0) r.boxPx++;
    }
    if (outDir && kind == 0 && show == 1.0f && !v.fp16)
    {
        char path[MAX_PATH]; FILE* f = nullptr;
        _snprintf_s(path, sizeof(path), _TRUNCATE, "%s\\notice-%ux%u-for-%dx%d.raw", outDir, bm.w, bm.h, v.w, v.h);
        if (!fopen_s(&f, path, "wb") && f) { fwrite(bm.bgra, 1, (size_t)bm.w * bm.h * 4, f); fclose(f); printf("  written: %s (b g r a, premultiplied)\n", path); }
    }
    return r;
}

static const wchar_t* kLine1 = L"DLSS-IV could not start, so the game runs without it.";
static const wchar_t* kLine2 = L"The reason is in DLSS-IV-DID-NOT-START.txt, next to GTAIV.exe";

static void Proofs(Dev& v, int w, int h, bool fp16, const char* outDir)
{
    Size(v, w, h, fp16);
    printf("%dx%d, %s target\n", w, h, fp16 ? "an FP16 (the back buffer with HDR on)" : "an 8-bit");
    NoticeBitmap bm;
    const bool made = NoticeRaster(kLine1, kLine2, (unsigned)w, (unsigned)h, &bm);
    Proof(made, "1  the picture is made");
    if (!made) return;
    long full = 0, none = 0, bad = 0;
    for (size_t i = 0; i < (size_t)bm.w * bm.h; i++)
    {
        const unsigned char* p = bm.bgra + i * 4;
        if (p[0] == 255 && p[3] == 255) full++;
        if (p[0] == 0 && p[3] == NOTICE_BOX_ALPHA) none++;
        if (p[3] < NOTICE_BOX_ALPHA || p[0] > p[3] || p[1] != p[0] || p[2] != p[0]) bad++;
    }
    const unsigned px = NoticeFontPx((unsigned)h);
    Proof(bm.w > bm.h && bm.w <= (unsigned)w * 6 / 10 && bm.h >= 2 * px && bm.h <= 8 * px && full > 200 && none > (long)(bm.w * bm.h / 3) && bad == 0,
          "1  it is %ux%u for a first line of %u pixels: wider than tall, inside 60 percent of the width; %ld pixels of text at full coverage, %ld of bare box, and none that is neither premultiplied nor at least the box (%ld)",
          bm.w, bm.h, px, full, none, bad);
    unsigned x0, y0; NoticePlace((unsigned)w, (unsigned)h, bm.w, &x0, &y0);
    Proof(x0 == ((unsigned)w - bm.w) / 2 && y0 == ((unsigned)h * 30 + 500) / 1000 && x0 + bm.w <= (unsigned)w, "4  the box stands centred (left edge %u) and 3 percent from the top (row %u)", x0, y0);

    const Res r = Run(v, bm, 1.0f, 0, outDir);
    Proof(r.outsideChanged == 0, "2  nothing outside the box changes: %ld pixels of %ld differ", r.outsideChanged, (long)w * h - r.insideN);
    Proof(r.insideWrong == 0 && r.insideN == (long)(bm.w * bm.h), "3  inside, every one of %ld pixels is the picture plus the frame times (1 - its alpha), to one step (the worst is off by %.5f)", r.insideN, r.worst);
    Proof(r.edgeWrong == 0 && r.insideWrong == 0, "4  texel for pixel: the picture's first and last columns and rows are the box's");
    Proof(r.alphaChanged == 0, "6  the target's alpha is not written (%ld pixels changed)", r.alphaChanged);
    Proof(r.statesBack, "7  the three blend states the draw borrows are back as they were, and the separate alpha blend was left on");
    const Res h5 = Run(v, bm, 0.5f, 0, nullptr);
    Proof(h5.insideWrong == 0 && h5.outsideChanged == 0, "5  half shown is half of the picture over the frame (the worst is off by %.5f)", h5.worst);
    const Res z = Run(v, bm, 0.0f, 0, nullptr);
    {   // show = 0: the frame as it was, inside too (Run's own expectation is the frame there)
        Proof(z.insideWrong == 0 && z.outsideChanged == 0 && z.worst < 1e-6, "5  shown at 0 changes nothing at all (the worst is off by %.7f)", z.worst);
    }
    Control(Run(v, bm, 1.0f, 1, nullptr).insideWrong > 0, "the blend left off");
    const Res c2 = Run(v, bm, 1.0f, 2, nullptr);
    Control(c2.insideWrong > 0, "the box's place off by one pixel in the shader's constants");
    Control(Run(v, bm, 1.0f, 3, nullptr).insideWrong > 0, "the picture not premultiplied");
    free(bm.bgra);
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; const char* outDir = nullptr;
    for (int i = 1; i < argc; ++i) { if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i]; else if (!strcmp(argv[i], "--out") && i + 1 < argc) outDir = argv[++i]; }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The on-screen notice's proof: the shipping picture, shader and draw over a known frame, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "ntproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("ntproof", "nt", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);   // never shown
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtNoticePS, &v.ps));
    CK(v.d->BeginScene());
    Proofs(v, 1920, 1080, false, outDir);
    Proofs(v, 3840, 2160, false, outDir);
    Proofs(v, 1920, 1080, true, nullptr);
    Proofs(v, 1280, 720, false, outDir);
    v.d->EndScene();
    printf(gFails ? "FAIL: %d\n" : "PASS\n", gFails);
    return gFails ? 1 : 0;
}
