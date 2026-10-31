// THE MASK'S PROOF (2026-10-10; roadmap item 5): the shipping shader (ivt_mask_ps.h) drawn by the shipping calls
// (transport/mask_pass.h: MaskKeep, the copy that keeps the reference, and MaskDraw) on the GPU, with no game, against
// pictures whose right answer is known. tools/test-rain is the pattern.
//
// THE PICTURES: a "scene right after the fog quad" (FP16, every channel random, 0 to 8: the scene buffer is not held to
// 1) and a "finished scene": the same bits, but for patches where something was laid over it (a pane: the scene times
// a tint plus a reflection; light added; smoke: the scene times a fraction). The reference and the mask are the scene's
// size divided by 1, 2 or 4 in each axis; every proof runs at each.
//
// THE PROOF:
//   1  NOTHING WHERE NOTHING WAS LAID: every texel of the mask whose part of the scene is the same bits on both sides
//      is exactly 0. At a fraction of the size this is the proof that the copy and the pass read the scene through
//      the same filter at the same places.
//   2  THE DIFFERENCE, AS WRITTEN: on every other texel the mask is min(ceiling, strength x |a/(1+a) - b/(1+b)|) of the
//      two luminances, each of the scene filtered to that texel, to one step of the target's eight bits.
//   3  THE CEILING HOLDS: something very bright laid over a dark scene gives the ceiling and not more, and a scene
//      value of 60,000 beside 0 gives the ceiling too.
//   4  WHAT IS NOT A PICTURE DOES NOT BECOME A MASK: a not-a-number, an infinity and a negative value on either side
//      give a mask in 0..ceiling.
//   5  THE SQUEEZE, MEASURED (a fact printed, with a bound): half as much light again over a scene of 0.02 and over a
//      scene of 6.0. Unsqueezed the two differences are 0.01 and 3.0, 300 to 1; squeezed they must be within 8 to 1.
//      THE DARK SCENE STILL GETS THE WEAKER MASK: a night street will show whether this measure is enough there.
//   6  A SIZE THAT DOES NOT DIVIDE (427 by 240 at a half): the untouched texels are still 0, to one step.
// NEGATIVE CONTROLS, each of which must FAIL the proof named: another size's texel (proof 1), the reference left off
// stage 1 (proofs 1 and 2), the strength negated (proof 2), the ceiling at 1 (proof 3), and, at a fraction of the size,
// the scene read POINT where the copy was filtered (proof 1: untouched texels get a mask).
//
//     tools\test-mask\build.ps1                 builds mk.exe (x86, as the game)
//     mk.exe --d3d9 <path> [--timing]           <path>: DXVK's d3d9 (external\dxvk\x32\d3d9.dll, or a game folder's vulkan.dll)
// --timing prints what a frame's two steps cost on this card at each size and fraction: the copy and the pass.
// Exit 0 when every proof passed. It changes nothing on the machine. Its window is 64 by 64 and is never shown.
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
#include "../../source/shaders/ivt_mask_ps.h"
#include "../../source/transport/mask_pass.h"

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

// IEEE half, round to nearest even (tools/test-rain's), and back
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
static uint32_t gRng = 0x13579BDFu;
static float Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0f; }

typedef std::vector<uint16_t> Img;             // four halves a pixel: r, g, b, a

struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DPixelShader9* ps = nullptr;
    IDirect3DTexture9* now = nullptr;  IDirect3DSurface9* nowSurf = nullptr;      // the scene buffer, w by h
    IDirect3DTexture9* ref = nullptr;  IDirect3DSurface9* refSurf = nullptr;      // the reference, rw by rh
    IDirect3DTexture9* mask = nullptr; IDirect3DSurface9* maskSurf = nullptr;     // the mask, rw by rh
    IDirect3DSurface9* sys = nullptr;  IDirect3DSurface9* maskSys = nullptr;
    IDirect3DQuery9* qFreq = nullptr; IDirect3DQuery9* qDis = nullptr;
    D3DFORMAT maskFmt = D3DFMT_A8R8G8B8; int maskBytes = 4;
    int w = 0, h = 0, rw = 0, rh = 0, div = 1;
};
static const float kQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };
static const D3DFORMAT kFmt = D3DFMT_A16B16G16R16F;
static const float kStrength = 2.0f, kCeiling = 0.75f;

static void Size(Dev& v, int w, int h, int div)
{
    for (IUnknown* u : { (IUnknown*)v.nowSurf, (IUnknown*)v.now, (IUnknown*)v.refSurf, (IUnknown*)v.ref, (IUnknown*)v.maskSurf, (IUnknown*)v.mask, (IUnknown*)v.sys, (IUnknown*)v.maskSys }) if (u) u->Release();
    v.w = w; v.h = h; v.div = div; v.rw = (w + div - 1) / div; v.rh = (h + div - 1) / div;
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, kFmt, D3DPOOL_DEFAULT, &v.now, nullptr));  CK(v.now->GetSurfaceLevel(0, &v.nowSurf));
    CK(v.d->CreateTexture(v.rw, v.rh, 1, D3DUSAGE_RENDERTARGET, kFmt, D3DPOOL_DEFAULT, &v.ref, nullptr));  CK(v.ref->GetSurfaceLevel(0, &v.refSurf));
    CK(v.d->CreateTexture(v.rw, v.rh, 1, D3DUSAGE_RENDERTARGET, v.maskFmt, D3DPOOL_DEFAULT, &v.mask, nullptr)); CK(v.mask->GetSurfaceLevel(0, &v.maskSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, kFmt, D3DPOOL_SYSTEMMEM, &v.sys, nullptr));
    CK(v.d->CreateOffscreenPlainSurface(v.rw, v.rh, v.maskFmt, D3DPOOL_SYSTEMMEM, &v.maskSys, nullptr));
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
static void PutScene(Dev& v, const Img& img)
{
    D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, 0));
    for (int y = 0; y < v.h; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &img[(size_t)y * v.w * 4], sizeof(uint16_t) * 4 * v.w);
    v.sys->UnlockRect();
    CK(v.d->UpdateSurface(v.sys, nullptr, v.nowSurf, nullptr));
}
// the mask as the card wrote it, one value a texel in 0..255 (the red or only channel)
static std::vector<uint8_t> GetMask(Dev& v)
{
    CK(v.d->GetRenderTargetData(v.maskSurf, v.maskSys));
    std::vector<uint8_t> out((size_t)v.rw * v.rh); D3DLOCKED_RECT lr; CK(v.maskSys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.rh; ++y) for (int x = 0; x < v.rw; ++x)
        out[(size_t)y * v.rw + x] = ((const BYTE*)lr.pBits)[(size_t)y * lr.Pitch + (size_t)x * v.maskBytes + (v.maskBytes == 4 ? 2 : 0)];
    v.maskSys->UnlockRect(); return out;
}
static double Squeeze(double l) { if (!(l > 0.0)) l = 0.0; if (l > 65504.0) l = 65504.0; return l / (1.0 + l); }

// THE SCENE FILTERED TO ONE TEXEL OF THE MASK, as a bilinear read at that texel's centre gives it: the four texels round
// the point and their weights. At the scene's own size it is the one texel.
struct Foot { size_t px[4]; double wgt[4]; };
static Foot FootOf(const Dev& v, int X, int Y)
{
    Foot f;
    const double fx = ((double)X + 0.5) * (double)v.w / (double)v.rw - 0.5, fy = ((double)Y + 0.5) * (double)v.h / (double)v.rh - 0.5;
    const int x0 = (int)floor(fx), y0 = (int)floor(fy); const double ax = fx - (double)x0, ay = fy - (double)y0;
    int k = 0;
    for (int j = 0; j < 2; ++j) for (int i = 0; i < 2; ++i, ++k)
    {
        const int sx = std::min(std::max(x0 + i, 0), v.w - 1), sy = std::min(std::max(y0 + j, 0), v.h - 1);
        f.px[k] = (size_t)sy * v.w + sx; f.wgt[k] = (i ? ax : 1.0 - ax) * (j ? ay : 1.0 - ay);
    }
    return f;
}
// its luminance; `stored`: each channel is rounded to a half float first, as the reference holds it
static double LumAt(const Img& p, const Foot& f, bool stored)
{
    double c[3] = {};
    for (int k = 0; k < 4; ++k) if (f.wgt[k] > 0.0) for (int ch = 0; ch < 3; ++ch) c[ch] += f.wgt[k] * (double)FromHalf(p[f.px[k] * 4 + ch]);
    if (stored) for (int ch = 0; ch < 3; ++ch) c[ch] = (double)FromHalf(ToHalf((float)c[ch]));
    return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
}
static bool SameAt(const Img& a, const Img& b, const Foot& f)
{
    for (int k = 0; k < 4; ++k) if (f.wgt[k] > 0.0 && memcmp(&a[f.px[k] * 4], &b[f.px[k] * 4], 6) != 0) return false;
    return true;
}

// The reference: every channel random, 0 to `top`
static Img Scene(int w, int h, float top)
{
    Img p((size_t)w * h * 4);
    for (size_t i = 0; i < p.size(); ++i) p[i] = ToHalf(top * Rnd());
    return p;
}
// Things laid over it, in patches: kind 0 a pane (the scene times a tint, plus a reflection), 1 light added, 2 smoke
// (the scene times a fraction), 3 something very bright.
static void Lay(Img& p, int w, int h, int patches, int kindOnly = -1)
{
    for (int s = 0; s < patches; ++s)
    {
        const int kind = kindOnly >= 0 ? kindOnly : s % 3;
        const int pw = 8 + (int)(Rnd() * 40.0f), ph = 8 + (int)(Rnd() * 30.0f);
        const int x0 = (int)(Rnd() * (float)(w - pw)), y0 = (int)(Rnd() * (float)(h - ph));
        const float t = 0.3f + 0.6f * Rnd(), add = 0.05f + 0.6f * Rnd();
        for (int y = y0; y < y0 + ph; ++y) for (int x = x0; x < x0 + pw; ++x)
        {
            const size_t i = ((size_t)y * w + x) * 4;
            for (int c = 0; c < 3; ++c)
            {
                const float s0 = FromHalf(p[i + c]);
                p[i + c] = ToHalf(kind == 0 ? s0 * t + add * 0.3f : kind == 1 ? s0 + add : kind == 2 ? s0 * t : 4000.0f);
            }
        }
    }
}

// THE MOD'S TWO STEPS. The scene buffer holds `ref`, the copy keeps it (MaskKeep); the scene buffer then holds `now`,
// and the pass is drawn. `fault`: 0 none, 1 another size's texel, 2 the strength negated, 3 the ceiling at 1, 4 the
// reference left off stage 1, 5 the scene read POINT where the copy was filtered.
static void Steps(Dev& v, const Img& ref, const Img& now, int fault)
{
    const bool filtered = v.div > 1;
    PutScene(v, ref);
    CK(MaskKeep(v.d, v.nowSurf, v.refSurf, filtered));
    PutScene(v, now);
    Common(v);
    if (!fault) CK(MaskDraw(v.d, v.now, v.ref, v.maskSurf, v.ps, (UINT)v.rw, (UINT)v.rh, filtered, kStrength, kCeiling, kQuad, sizeof(kQuad[0])));
    else
    {   // MaskDraw's own calls, with the one fault
        const float tex = fault == 1 ? 2.0f : 1.0f;
        const float c0[4] = { tex / (float)v.rw, tex / (float)v.rh, fault == 2 ? -kStrength : kStrength, fault == 3 ? 1.0f : kCeiling };
        const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.rw, (DWORD)v.rh, 0.0f, 1.0f };
        v.d->SetRenderTarget(0, v.maskSurf); v.d->SetViewport(&vp);
        v.d->SetTexture(0, v.now); v.d->SetTexture(1, fault == 4 ? nullptr : v.ref);
        if (filtered && fault != 5) { v.d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR); v.d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR); }
        v.d->SetPixelShaderConstantF(0, c0, 1); v.d->SetPixelShader(v.ps);
        CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
    }
    v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr);
}

struct Res { long same = 0, sameWrong = 0, sameWorst = 0, laid = 0, laidWrong = 0; double worst = 0.0; long top = 0, topWrong = 0; };
static Res Run(Dev& v, int fault, int kindOnly = -1, float sceneTop = 8.0f, double ceiling = kCeiling)
{
    const Img ref = Scene(v.w, v.h, sceneTop); Img now = ref;
    Lay(now, v.w, v.h, 60, kindOnly);
    Steps(v, ref, now, fault);
    const std::vector<uint8_t> m = GetMask(v);
    Res r;
    for (int Y = 0; Y < v.rh; ++Y) for (int X = 0; X < v.rw; ++X)
    {
        const Foot f = FootOf(v, X, Y); const int got8 = m[(size_t)Y * v.rw + X]; const double got = (double)got8 / 255.0;
        if (SameAt(now, ref, f)) { r.same++; if (got8 != 0) r.sameWrong++; r.sameWorst = std::max(r.sameWorst, (long)got8); continue; }
        r.laid++;
        const double want = std::min(ceiling, (double)kStrength * fabs(Squeeze(LumAt(now, f, false)) - Squeeze(LumAt(ref, f, true))));
        const double err = fabs(got - want);
        r.worst = std::max(r.worst, err);
        if (err > 1.6 / 255.0) r.laidWrong++;                     // one step of eight bits, and the half floats' own rounding
        if (kindOnly == 3 && want >= ceiling) { r.top++; if (fabs(got - kCeiling) > 0.6 / 255.0) r.topWrong++; }
    }
    return r;
}

static double Timed(Dev& v, int frames, bool copy)
{
    // one step alone, on the card's own clock, over `frames` goes
    const bool filtered = v.div > 1;
    for (int i = 0; i < 8; ++i) { MaskKeep(v.d, v.nowSurf, v.refSurf, filtered); Common(v); MaskDraw(v.d, v.now, v.ref, v.maskSurf, v.ps, (UINT)v.rw, (UINT)v.rh, filtered, kStrength, kCeiling, kQuad, sizeof(kQuad[0])); v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); }
    UINT64 f = 0; BOOL dis = FALSE;
    IDirect3DQuery9 *t0 = nullptr, *t1 = nullptr;
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t1);
    if (!t0 || !t1 || !v.qFreq || !v.qDis) return -1.0;
    v.qDis->Issue(D3DISSUE_BEGIN); v.qFreq->Issue(D3DISSUE_END);
    t0->Issue(D3DISSUE_END);
    for (int i = 0; i < frames; ++i)
    {
        if (copy) MaskKeep(v.d, v.nowSurf, v.refSurf, filtered);
        else { Common(v); MaskDraw(v.d, v.now, v.ref, v.maskSurf, v.ps, (UINT)v.rw, (UINT)v.rh, filtered, kStrength, kCeiling, kQuad, sizeof(kQuad[0])); v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); }
    }
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

static void Proofs(Dev& v, int w, int h, int div)
{
    Size(v, w, h, div);
    printf(" the scene %dx%d, the reference and the mask %dx%d (a %s of it in each axis)\n", w, h, v.rw, v.rh, div == 1 ? "whole" : div == 2 ? "half" : "quarter");
    {
        const Res r = Run(v, 0);
        Proof(r.same > 1000 && r.sameWrong == 0, "1  nothing where nothing was laid: %ld texels whose part of the scene is the same bits on both sides are exactly 0 (%ld are not)", r.same, r.sameWrong);
        Proof(r.laid > 500 && r.laidWrong == 0, "2  the difference, as written: %ld texels something was laid over are min(%.2f, %.0f x the difference of the squeezed brightnesses), worst %.4f of a budget of %.4f (%ld are not)",
              r.laid, kCeiling, kStrength, r.worst, 1.6 / 255.0, r.laidWrong);
        const Res c1 = Run(v, 1);
        Control(c1.sameWrong > 0, "another size's texel: %ld untouched texels get a mask", c1.sameWrong);
        const Res c4 = Run(v, 4);
        Control(c4.sameWrong > 0 || c4.laidWrong > 0, "the reference left off stage 1: %ld untouched texels get a mask, %ld laid ones are wrong", c4.sameWrong, c4.laidWrong);
        const Res c2 = Run(v, 2);
        Control(c2.laidWrong > 0, "the strength negated: %ld of %ld laid texels are not the difference", c2.laidWrong, c2.laid);
        if (div > 1) { const Res c5 = Run(v, 5); Control(c5.sameWrong > 0, "the scene read POINT where the copy was filtered: %ld untouched texels get a mask", c5.sameWrong); }
    }
    {
        const Res r = Run(v, 0, 3, 0.05f);
        Proof(r.top > 300 && r.topWrong == 0, "3  the ceiling holds: %ld texels where 4000 was laid over a dark scene give %.2f and not more (%ld do not)", r.top, kCeiling, r.topWrong);
        const Res c3 = Run(v, 3, 3, 0.05f);
        Control(c3.topWrong > 0, "the ceiling at 1: %ld of those texels are not held to %.2f", c3.topWrong, kCeiling);
        // 60,000 beside 0, and what is not a picture: whole pictures of one pair, so every fraction reads one value
        const uint16_t big = ToHalf(60000.0f), nanv = 0x7E00u, inf = 0x7C00u, neg = ToHalf(-3.0f), one = ToHalf(1.0f);
        const uint16_t pairs[6][2] = { { big, 0 }, { 0, big }, { nanv, one }, { one, nanv }, { inf, one }, { neg, one } };
        long bigWrong = 0, odd = 0; const int cap8 = (int)(kCeiling * 255.0f + 0.5f);
        for (int k = 0; k < 6; ++k)
        {
            Img ref((size_t)w * h * 4, 0), now((size_t)w * h * 4, 0);
            for (size_t px = 0; px < (size_t)w * h; ++px) for (int c = 0; c < 3; ++c) { now[px * 4 + c] = pairs[k][0]; ref[px * 4 + c] = pairs[k][1]; }
            Steps(v, ref, now, 0);
            const std::vector<uint8_t> m = GetMask(v);
            for (size_t i = 0; i < m.size(); ++i) { if (k < 2) { if (abs((int)m[i] - cap8) > 1) bigWrong++; } else if ((int)m[i] > cap8 + 1) odd++; }
        }
        Proof(bigWrong == 0, "3  60,000 beside 0, either way round, gives the ceiling (%ld texels do not)", bigWrong);
        Proof(odd == 0, "4  what is not a picture does not become a mask: a not-a-number, an infinity and a negative value on either side give a value in 0..%.2f (%ld texels do not)", kCeiling, odd);
    }
    {   // proof 5: a dark scene and a bright one, half as much light again laid over each
        Img refD((size_t)w * h * 4), refB((size_t)w * h * 4), nowD((size_t)w * h * 4), nowB((size_t)w * h * 4);
        for (size_t i = 0; i < refD.size(); ++i) { refD[i] = ToHalf(0.02f); refB[i] = ToHalf(6.0f); nowD[i] = ToHalf(0.03f); nowB[i] = ToHalf(9.0f); }
        Steps(v, refD, nowD, 0); const int d = GetMask(v)[0];
        Steps(v, refB, nowB, 0); const int b = GetMask(v)[0];
        Proof(d > 0 && b > 0 && d <= 8 * b && b <= 8 * d, "5  the squeeze, measured: half as much light again over a scene of 0.02 gives %d of 255, over a scene of 6.0 gives %d (unsqueezed the differences are 300 to 1): the dark scene gets the weaker mask", d, b);
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
    printf("The mask's proof: the shipping shader and calls against pictures of known answer, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "mkproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("mkproof", "mk", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);   // never shown
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    // ONE CHANNEL WHERE THE RUNTIME HAS A ONE-CHANNEL TARGET (L8), else four: the wire's image is one channel of eight bits
    const bool l8 = SUCCEEDED(d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_L8));
    v.maskFmt = l8 ? D3DFMT_L8 : D3DFMT_A8R8G8B8; v.maskBytes = l8 ? 1 : 4;
    printf("  the mask's target: %s\n", l8 ? "L8, one channel (the runtime takes it as a render target)" : "A8R8G8B8 (the runtime has no one-channel eight-bit render target)");
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtMaskPS, &v.ps));
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &v.qFreq); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &v.qDis);
    CK(v.d->BeginScene());

    for (int div = 1; div <= 4; div *= 2) { Proofs(v, 428, 240, div); Proofs(v, 1708, 960, div); }
    {   // proof 6: a size that does not divide
        Size(v, 427, 241, 2);
        const Res r = Run(v, 0);
        Proof(r.same > 1000 && r.sameWorst <= 1, "6  a size that does not divide (427x241 at a half, %dx%d): %ld untouched texels are 0 to one step of 255 (the largest is %ld; %ld are not exactly 0)", v.rw, v.rh, r.same, r.sameWorst, r.sameWrong);
    }

    if (timing)
    {
        const int sizes[4][2] = { { 3840, 2160 }, { 2560, 1440 }, { 1920, 1080 }, { 1280, 720 } };
        for (int i = 0; i < 4; ++i) for (int div = 1; div <= 4; div *= 2)
        {
            Size(v, sizes[i][0], sizes[i][1], div);
            const Img ref = Scene(sizes[i][0], sizes[i][1], 8.0f); Img now = ref; Lay(now, sizes[i][0], sizes[i][1], 200);
            Steps(v, ref, now, 0);
            const double pass = Timed(v, 400, false), copy = Timed(v, 400, true);
            printf("  TIMING, a render size of %dx%d, the reference and the mask %dx%d (ms a frame on this card's own clock): the copy %.4f, the pass %.4f, both %.4f; the reference holds %.1f MiB, the mask %.1f\n",
                   sizes[i][0], sizes[i][1], v.rw, v.rh, copy, pass, copy + pass, 8.0 * (double)v.rw * (double)v.rh / 1048576.0, (double)v.maskBytes * (double)v.rw * (double)v.rh / 1048576.0);
        }
    }
    v.d->EndScene();
    printf("THE MASK'S PROOF: %s\n", gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
