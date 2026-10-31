// THE RAIN LAYER'S PROOF (2026-10-03): the shipping shader (ivt_rain_ps.h) drawn by the shipping draw
// (transport/rain_pass.h, RainLay) on the GPU, with no game, against pictures whose right answer is known.
// tools/test-steady is the pattern.
//
// THE PICTURES: a "picture the upscaler made" (FP16, every channel random, alpha included) and a "rain layer" (FP16,
// black but for streaks of a few pixels, as a rain draw into a cleared target leaves it). The mod's pass adds the
// second to the first.
//
// THE PROOF:
//   1  THE RAIN IS ADDED, EXACTLY, AND ONLY WHERE THERE IS RAIN. At the picture's own size: every pixel whose layer is
//      black is bit for bit what it was, in all four channels; every other pixel's red, green and blue are the
//      picture's plus the layer's, to one step of a half float; and the picture's ALPHA is bit for bit what it was
//      everywhere.
//   2  IT WRITES THE DROPS AND NOTHING ELSE. "Bit for bit what it was" would also be true of a pixel the pass WROTE with
//      the picture plus nothing, so the picture carries a witness: MINUS ZERO in the red of every seventh pixel. Minus
//      zero plus zero is plus zero: a pixel the pass writes loses the sign, a pixel it discards keeps it. Every seed
//      under black layer keeps its sign. (An occlusion query was the first instrument here and it counted every pixel
//      of the quad once the driver's pipeline was warm: with no depth test in the pass it counts ahead of the shader's
//      discard. That is also why the mod's log gives no pixel count for this pass.)
//   3  THE DEVICE IS LEFT AS IT WAS FOUND: the two blend factors and the blend operation as they were (the game's
//      own: SRCALPHA and INVSRCALPHA here, and REVSUBTRACT, an operation under which nothing would be added if the
//      pass did not set its own), blending off and all four channels written, as the mod's common setup has them. And with the
//      game's SEPARATE alpha blend left on with factors that would wipe a target's alpha, the picture's alpha is still
//      untouched (proof 1 runs in that state).
//   4  A LARGER PICTURE (a render scale: the layer at two thirds of the picture's size): every pixel is the picture
//      plus the layer filtered to it, and where the filtered layer is black nothing is written.
//   5  NO RAIN, NOTHING: under a black layer the picture is bit for bit what it was, every seed's sign with it.
// NEGATIVE CONTROLS, each of which must FAIL the proof named: the draw with blending left off (the layer REPLACES the
// picture: proof 1), with another size's texel (proof 1), with all four channels written under the wiping alpha
// factors (the alpha goes: proof 1), with the blend operation left as the device had it (the layer is SUBTRACTED:
// proof 1), and a layer that is nowhere black, by the least a half float holds above the
// shader's threshold (every pixel is written: the seeds lose their sign, proof 2).
//
//     tools\test-rain\build.ps1             builds rl.exe (x86, as the game)
//     rl.exe --d3d9 <path> [--timing]       <path>: DXVK's d3d9, a game folder's vulkan.dll
// Exit 0 when every proof passed. It changes nothing on the machine. Close the game first: it wants the graphics card.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "../../source/shaders/ivt_fs_vs.h"
#include "../../source/shaders/ivt_rain_ps.h"
#include "../../source/transport/rain_pass.h"

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
static void Control(bool failsAsItMust, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  CONTROL (%s)%s\n", failsAsItMust ? "PASS" : "FAIL", b, failsAsItMust ? ": fails the proof, as it must" : ": THE PROOF CANNOT SEE THIS FAULT");
    if (!failsAsItMust) gFails++;
}
#define CK(x) do { HRESULT hr_ = (x); if (FAILED(hr_)) { printf("  FAIL  %s: 0x%08lX (line %d)\n", #x, (unsigned long)hr_, __LINE__); exit(2); } } while (0)

// IEEE half, round to nearest even (tools/test-dedither's), and back
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
        uint32_t h = man >> shift; const uint32_t rem = man & ((1u << shift) - 1u), half = 1u << (shift - 1);
        if (rem > half || (rem == half && (h & 1u))) h++;
        return (uint16_t)(sign | h);
    }
    uint32_t h = ((uint32_t)e << 10) | (man >> 13); const uint32_t rem = man & 0x1FFFu;
    if (rem > 0x1000u || (rem == 0x1000u && (h & 1u))) h++;
    return (uint16_t)(sign | h);
}
static float FromHalf(uint16_t h)
{
    const uint32_t sign = (uint32_t)(h & 0x8000u) << 16; const uint32_t ex = (h >> 10) & 0x1Fu; const uint32_t man = h & 0x3FFu;
    uint32_t x;
    if (ex == 0) { if (!man) x = sign; else { int e = -1; uint32_t m = man; do { e++; m <<= 1; } while (!(m & 0x400u)); x = sign | ((uint32_t)(127 - 15 - e) << 23) | ((m & 0x3FFu) << 13); } }
    else if (ex == 31) x = sign | 0x7F800000u | (man << 13);
    else x = sign | ((ex + 127 - 15) << 23) | (man << 13);
    float f; memcpy(&f, &x, 4); return f;
}
static uint32_t gRng = 0x2468ACE1u;
static float Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0f; }

typedef std::vector<uint16_t> Img;             // four halves a pixel: r, g, b, a

struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DPixelShader9* ps = nullptr;
    IDirect3DTexture9* home = nullptr; IDirect3DSurface9* homeSurf = nullptr; IDirect3DSurface9* homeSys = nullptr;
    IDirect3DTexture9* layer = nullptr; IDirect3DSurface9* layerSurf = nullptr; IDirect3DSurface9* layerSys = nullptr;
    IDirect3DQuery9* qFreq = nullptr; IDirect3DQuery9* qDis = nullptr;
    int w = 0, h = 0, lw = 0, lh = 0;
};
static const float kQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };
static const D3DFORMAT kFmt = D3DFMT_A16B16G16R16F;

static void Size(Dev& v, int w, int h, int lw, int lh)
{
    for (IUnknown* u : { (IUnknown*)v.homeSurf, (IUnknown*)v.home, (IUnknown*)v.homeSys, (IUnknown*)v.layerSurf, (IUnknown*)v.layer, (IUnknown*)v.layerSys }) if (u) u->Release();
    v.w = w; v.h = h; v.lw = lw; v.lh = lh;
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, kFmt, D3DPOOL_DEFAULT, &v.home, nullptr));    CK(v.home->GetSurfaceLevel(0, &v.homeSurf));
    CK(v.d->CreateTexture(lw, lh, 1, D3DUSAGE_RENDERTARGET, kFmt, D3DPOOL_DEFAULT, &v.layer, nullptr)); CK(v.layer->GetSurfaceLevel(0, &v.layerSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, kFmt, D3DPOOL_SYSTEMMEM, &v.homeSys, nullptr));
    CK(v.d->CreateOffscreenPlainSurface(lw, lh, kFmt, D3DPOOL_SYSTEMMEM, &v.layerSys, nullptr));
}
// THE COMMON STATE OF A FULL-SCREEN PASS OF THE MOD'S (IvtSetupPass, DlssIvTransport.h)
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
// THE GAME'S OWN BLEND STATE as it may stand when the mod's pass begins: its alpha blend's factors, an operation that
// is not ADD, and the separate alpha blend on with factors that would wipe a target's alpha. The mod's common setup
// turns blending itself off.
static void GameBlend(Dev& v)
{
    v.d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); v.d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    v.d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
    v.d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    v.d->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO); v.d->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
}
static void Put(Dev& v, IDirect3DSurface9* sys, IDirect3DSurface9* rt, const Img& img, int w, int h)
{
    D3DLOCKED_RECT lr; CK(sys->LockRect(&lr, nullptr, 0));
    for (int y = 0; y < h; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &img[(size_t)y * w * 4], sizeof(uint16_t) * 4 * w);
    sys->UnlockRect();
    CK(v.d->UpdateSurface(sys, nullptr, rt, nullptr));
}
static Img Get(Dev& v, IDirect3DSurface9* rt, IDirect3DSurface9* sys, int w, int h)
{
    CK(v.d->GetRenderTargetData(rt, sys));
    Img out((size_t)w * h * 4); D3DLOCKED_RECT lr; CK(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < h; ++y) memcpy(&out[(size_t)y * w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(uint16_t) * 4 * w);
    sys->UnlockRect(); return out;
}
// The picture: every channel random, 0 to 2 (the scene buffer is not held to 1), the alpha random too
static Img Picture(int w, int h)
{
    Img p((size_t)w * h * 4);
    for (size_t i = 0; i < p.size(); ++i) p[i] = ToHalf(2.0f * Rnd());
    for (size_t px = 0; px < (size_t)w * h; px += 7) p[px * 4] = 0x8000u;      // THE WITNESS: minus zero in the red
    return p;
}
// The layer as a rain draw into a cleared target leaves it: black, with streaks. `share`: about how much of it they
// cover. The layer's alpha is whatever the game's blend left there: random on the streaks, zero elsewhere.
static Img Rain(int w, int h, float share)
{
    Img l((size_t)w * h * 4, 0);
    const int streaks = std::max(1, (int)(share * (float)w * (float)h / 9.0f));
    for (int s = 0; s < streaks; ++s)
    {
        int x = (int)(Rnd() * (float)w), y = (int)(Rnd() * (float)h); const int len = 4 + (int)(Rnd() * 10.0f);
        const float r = 0.05f + 0.5f * Rnd(), g = 0.05f + 0.5f * Rnd(), b = 0.05f + 0.5f * Rnd();
        for (int k = 0; k < len && y < h; ++k, ++y)
        {
            if (x < 0 || x >= w) break;
            const size_t i = ((size_t)y * w + x) * 4;
            l[i] = ToHalf(r); l[i + 1] = ToHalf(g); l[i + 2] = ToHalf(b); l[i + 3] = ToHalf(Rnd());
            if ((k & 3) == 3) x += (Rnd() < 0.5f) ? 1 : 0;
        }
    }
    return l;
}
static bool Black(const Img& l, size_t px) { return FromHalf(l[px * 4]) <= 1.0e-6f && FromHalf(l[px * 4 + 1]) <= 1.0e-6f && FromHalf(l[px * 4 + 2]) <= 1.0e-6f; }

// THE MOD'S DRAW. `fault`: 0 none (RainLay itself), 1 blending left off, 2 another size's texel, 3 all four channels
// written, 4 the blend operation left as the device has it.
static void Lay(Dev& v, int fault)
{
    Common(v);
    const bool filter = v.lw != v.w || v.lh != v.h;
    if (!fault) CK(RainLay(v.d, v.layer, v.homeSurf, v.ps, (UINT)v.w, (UINT)v.h, filter, kQuad, sizeof(kQuad[0])));
    else
    {   // RainLay's own calls, with the one fault
        const float c0[4] = { (fault == 2 ? 2.0f : 1.0f) / (float)v.w, (fault == 2 ? 2.0f : 1.0f) / (float)v.h, 0.0f, 0.0f };
        const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f };
        v.d->SetRenderTarget(0, v.homeSurf); v.d->SetViewport(&vp); v.d->SetTexture(0, v.layer);
        v.d->SetPixelShaderConstantF(0, c0, 1); v.d->SetPixelShader(v.ps);
        if (fault != 4) v.d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        v.d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); v.d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        v.d->SetRenderState(D3DRS_COLORWRITEENABLE, fault == 3 ? 0x0F : 0x07);
        v.d->SetRenderState(D3DRS_ALPHABLENDENABLE, fault == 1 ? FALSE : TRUE);
        CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
        v.d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
        v.d->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);     // as GameBlend left it, for the checks that follow
    }
    v.d->SetTexture(0, nullptr);
}

struct Same { long rainPx = 0, blackPx = 0, blackChanged = 0, rainWrong = 0, alphaChanged = 0, seeds = 0, seedsGone = 0; int worst = 0; };
// Proofs 1 and 2 at the picture's own size, in the game's blend state. `fault` as Lay's. `faint`: the layer's black is
// replaced by the least a half float holds above the shader's threshold (the control of proof 2).
static Same RunSame(Dev& v, float share, int fault, bool faint = false)
{
    const int w = v.w, h = v.h;
    const Img pic = Picture(w, h); Img rain = Rain(w, h, share);
    std::vector<uint8_t> black((size_t)w * h);
    for (size_t px = 0; px < (size_t)w * h; ++px) black[px] = Black(rain, px) ? 1 : 0;
    if (faint) for (size_t px = 0; px < (size_t)w * h; ++px) if (black[px]) rain[px * 4] = ToHalf(2.0e-6f);
    Put(v, v.homeSys, v.homeSurf, pic, w, h); Put(v, v.layerSys, v.layerSurf, rain, w, h);
    GameBlend(v);
    Same r; Lay(v, fault);
    const Img out = Get(v, v.homeSurf, v.homeSys, w, h);
    for (size_t px = 0; px < (size_t)w * h; ++px)
    {
        if (out[px * 4 + 3] != pic[px * 4 + 3]) r.alphaChanged++;
        if (black[px])
        {
            r.blackPx++; if (memcmp(&out[px * 4], &pic[px * 4], 8) != 0) r.blackChanged++;
            if (pic[px * 4] == 0x8000u) { r.seeds++; if (out[px * 4] != 0x8000u) r.seedsGone++; }
            continue;
        }
        r.rainPx++;
        for (int c = 0; c < 3; ++c)
        {
            const int want = (int)ToHalf(FromHalf(pic[px * 4 + c]) + FromHalf(rain[px * 4 + c]));
            const int off = abs((int)out[px * 4 + c] - want);
            r.worst = std::max(r.worst, off);
            if (off > 1) { r.rainWrong++; break; }
        }
    }
    return r;
}

static double Timed(Dev& v, int frames)
{
    // the pass alone, on the card's own clock, over `frames` draws
    for (int i = 0; i < 8; ++i) Lay(v, 0);
    UINT64 f = 0; BOOL dis = FALSE;
    IDirect3DQuery9 *t0 = nullptr, *t1 = nullptr;
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t1);
    if (!t0 || !t1 || !v.qFreq || !v.qDis) return -1.0;
    v.qDis->Issue(D3DISSUE_BEGIN); v.qFreq->Issue(D3DISSUE_END);
    t0->Issue(D3DISSUE_END);
    const bool filter = v.lw != v.w || v.lh != v.h;
    for (int i = 0; i < frames; ++i) { Common(v); RainLay(v.d, v.layer, v.homeSurf, v.ps, (UINT)v.w, (UINT)v.h, filter, kQuad, sizeof(kQuad[0])); }
    t1->Issue(D3DISSUE_END); v.qDis->Issue(D3DISSUE_END);
    UINT64 a = 0, b = 0;
    while (t0->GetData(&a, sizeof(a), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (t1->GetData(&b, sizeof(b), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (v.qFreq->GetData(&f, sizeof(f), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (v.qDis->GetData(&dis, sizeof(dis), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    t0->Release(); t1->Release();
    if (dis || !f) return -1.0;
    return (double)(b - a) * 1000.0 / (double)f / (double)frames;
}

static void Proofs(Dev& v, int w, int h)
{
    printf(" at %dx%d\n", w, h);
    Size(v, w, h, w, h);
    {
        const Same s = RunSame(v, 0.02f, 0);
        Proof(s.rainPx > 200 && s.blackChanged == 0 && s.rainWrong == 0 && s.alphaChanged == 0,
              "1  the rain is added, and only where there is rain: %ld pixels of the layer hold rain and each is the picture plus the layer to %d step(s) of a half float (%ld are not);"
              " %ld pixels of black layer are bit for bit what the picture was (%ld are not); the picture's alpha changed on %ld pixels, with the game's separate alpha blend left on to wipe it",
              s.rainPx, s.worst, s.rainWrong, s.blackPx, s.blackChanged, s.alphaChanged);
        Proof(s.seeds > 1000 && s.seedsGone == 0, "2  it writes the drops and nothing else: of %ld pixels under black layer that hold MINUS ZERO, %ld lost the sign (a pixel the pass writes becomes picture plus zero, which is plus zero)",
              s.seeds, s.seedsGone);
        DWORD sb = 0, db = 0, ab = 1, cw = 0, bo = 0;
        v.d->GetRenderState(D3DRS_SRCBLEND, &sb); v.d->GetRenderState(D3DRS_DESTBLEND, &db);
        v.d->GetRenderState(D3DRS_ALPHABLENDENABLE, &ab); v.d->GetRenderState(D3DRS_COLORWRITEENABLE, &cw);
        v.d->GetRenderState(D3DRS_BLENDOP, &bo);
        Proof(sb == D3DBLEND_SRCALPHA && db == D3DBLEND_INVSRCALPHA && ab == FALSE && cw == 0x0F && bo == D3DBLENDOP_REVSUBTRACT,
              "3  the device is left as it was found: source blend %lu and destination blend %lu (the game's 5 and 6), the blend operation %lu (the game's 3, REVSUBTRACT, under which the pass still added), blending %lu (off), colour write 0x%lX (all four)",
              (unsigned long)sb, (unsigned long)db, (unsigned long)bo, (unsigned long)ab, (unsigned long)cw);
        const Same c1 = RunSame(v, 0.02f, 1);
        Control(c1.rainWrong > 0, "blending left off: the layer replaces the picture on %ld of %ld rain pixels", c1.rainWrong, c1.rainPx);
        const Same c2 = RunSame(v, 0.02f, 2);
        Control(c2.rainWrong > 0 || c2.blackChanged > 0, "another size's texel: %ld rain pixels wrong, %ld black ones changed", c2.rainWrong, c2.blackChanged);
        const Same c3 = RunSame(v, 0.02f, 3);
        Control(c3.alphaChanged > 0, "all four channels written under the wiping alpha factors: the picture's alpha changed on %ld pixels", c3.alphaChanged);
        const Same c5 = RunSame(v, 0.02f, 4);
        Control(c5.rainWrong > 0, "the blend operation left as the device had it (REVSUBTRACT): %ld of %ld rain pixels are not the picture plus the layer", c5.rainWrong, c5.rainPx);
        const Same c4 = RunSame(v, 0.02f, 0, true);
        Control(c4.seedsGone == c4.seeds && c4.seeds > 1000, "a layer that is nowhere black (2e-6 where it was): %ld of %ld minus zeros lost the sign, so the witness does see a write", c4.seedsGone, c4.seeds);
    }
    {   // proof 5: no rain
        const Img pic = Picture(w, h); const Img none((size_t)w * h * 4, 0);
        Put(v, v.homeSys, v.homeSurf, pic, w, h); Put(v, v.layerSys, v.layerSurf, none, w, h);
        GameBlend(v);
        Lay(v, 0);
        const Img out = Get(v, v.homeSurf, v.homeSys, w, h);
        long seeds = 0; for (size_t px = 0; px < (size_t)w * h; ++px) if (pic[px * 4] == 0x8000u) seeds++;
        Proof(out == pic && seeds > 1000, "5  no rain, nothing: under a black layer the picture is %s, its %ld minus zeros with it", out == pic ? "bit for bit what it was" : "CHANGED", seeds);
    }
    {   // proof 4: the picture one and a half times the layer
        const int lw = (w * 2) / 3, lh = (h * 2) / 3;
        Size(v, w, h, lw, lh);
        const Img pic = Picture(w, h), rain = Rain(lw, lh, 0.02f);
        Put(v, v.homeSys, v.homeSurf, pic, w, h); Put(v, v.layerSys, v.layerSurf, rain, lw, lh);
        GameBlend(v);
        Lay(v, 0);
        const Img out = Get(v, v.homeSurf, v.homeSys, w, h);
        long lit = 0, wrong = 0, darkChanged = 0, dark = 0, edge = 0; double worst = 0.0;
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        {
            // the layer filtered to this pixel's centre: bilinear over the four texels round it, clamped at the border
            const float fx = ((float)x + 0.5f) * (float)lw / (float)w - 0.5f, fy = ((float)y + 0.5f) * (float)lh / (float)h - 0.5f;
            const int x0 = (int)floorf(fx), y0 = (int)floorf(fy); const float ax = fx - (float)x0, ay = fy - (float)y0;
            float want[3] = {}; bool any = false;
            for (int j = 0; j < 2; ++j) for (int i = 0; i < 2; ++i)
            {
                const int sx = std::min(std::max(x0 + i, 0), lw - 1), sy = std::min(std::max(y0 + j, 0), lh - 1);
                const float wgt = (i ? ax : 1.0f - ax) * (j ? ay : 1.0f - ay);
                const size_t li = ((size_t)sy * lw + sx) * 4;
                for (int c = 0; c < 3; ++c) want[c] += wgt * FromHalf(rain[li + c]);
                if (!Black(rain, (size_t)sy * lw + sx)) any = true;
            }
            const size_t px = (size_t)y * w + x;
            const float m = std::max(want[0], std::max(want[1], want[2]));
            if (!any) { dark++; if (memcmp(&out[px * 4], &pic[px * 4], 8) != 0) darkChanged++; continue; }
            if (m < 2.0e-3f) { edge++; continue; }             // at the filter's own rounding of nearly nothing: either answer
            lit++;
            for (int c = 0; c < 3; ++c)
            {
                const double got = (double)FromHalf(out[px * 4 + c]) - (double)FromHalf(pic[px * 4 + c]);
                const double err = fabs(got - (double)want[c]);
                worst = std::max(worst, err);
                if (err > 0.01 + 0.01 * (double)want[c]) { wrong++; break; }
            }
        }
        Proof(lit > 200 && wrong == 0 && darkChanged == 0,
              "4  a picture one and a half times the layer (%dx%d over %dx%d): %ld pixels the filtered layer lights are the picture plus it (worst %.4f; %ld are not);"
              " %ld pixels it does not reach are bit for bit the picture, minus zeros and all (%ld are not); %ld at the filter's edge of nearly nothing are not judged",
              w, h, lw, lh, lit, worst, wrong, dark, darkChanged, edge);
    }
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; bool timing = false;
    for (int i = 1; i < argc; ++i) { if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i]; else if (!strcmp(argv[i], "--timing")) timing = true; }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The rain layer's proof: the shipping shader and draw against pictures of known answer, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "rlproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("rlproof", "rl", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtRainPS, &v.ps));
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &v.qFreq); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &v.qDis);
    CK(v.d->BeginScene());

    Proofs(v, 427, 240);
    Proofs(v, 1707, 960);

    if (timing)
    {
        const int sizes[4][4] = { { 3840, 2160, 3840, 2160 }, { 3840, 2160, 2560, 1440 }, { 2560, 1440, 2560, 1440 }, { 1920, 1080, 1920, 1080 } };
        for (int i = 0; i < 4; ++i)
        {
            Size(v, sizes[i][0], sizes[i][1], sizes[i][2], sizes[i][3]);
            const Img pic = Picture(sizes[i][0], sizes[i][1]);
            Put(v, v.homeSys, v.homeSurf, pic, sizes[i][0], sizes[i][1]);
            double ms[3] = {};
            const float shares[3] = { 0.0f, 0.02f, 0.10f };
            for (int k = 0; k < 3; ++k)
            {
                const Img rain = shares[k] > 0.0f ? Rain(sizes[i][2], sizes[i][3], shares[k]) : Img((size_t)sizes[i][2] * sizes[i][3] * 4, 0);
                Put(v, v.layerSys, v.layerSurf, rain, sizes[i][2], sizes[i][3]);
                ms[k] = Timed(v, 400);
            }
            printf("  TIMING, a picture of %dx%d with the layer at %dx%d, on this card's own clock (ms a frame, the pass alone): no rain %.4f, rain on about 2 percent of the layer %.4f, on about 10 percent %.4f; the layer holds %.1f MiB\n",
                   sizes[i][0], sizes[i][1], sizes[i][2], sizes[i][3], ms[0], ms[1], ms[2], 8.0 * (double)sizes[i][2] * (double)sizes[i][3] / 1048576.0);
        }
    }
    v.d->EndScene();
    printf("THE RAIN LAYER'S PROOF: %s\n", gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
