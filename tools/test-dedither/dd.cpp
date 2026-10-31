// THE DITHER PASS'S PICTURE PROOF (2026-10-03): the shipping shaders (ivt_dedither_ps.h, ivt_dedither_home_ps.h) drawn
// by the shipping draws (transport/dedither_pass.h, DedInPlace) on the GPU, with no game, against pictures whose right
// answer is known. G9's harness (tools/test-hdr-skip/g9.cpp) is the pattern.
//
// THE PASS IS TWO DRAWS SINCE 2026-10-03 LATE: the first writes its own target only where a mark reaches, the second
// asks the mark again and puts the pixels the first changed into the SCENE BUFFER itself, which is what the upscaler
// is then given. So the "scene buffer" here is a render target the picture is loaded into before every run of the
// pass, and every proof reads THAT back. The pass's target is never cleared by the mod: here it is filled with
// MAGENTA AND AN ALPHA OF 1 before every run, so a pixel the second draw read where the first had not written this
// frame would turn magenta in the scene buffer, in any proof.
//
// THE PICTURES are built as the game builds them: FusionFix's deferred passes leave pixels out by the parity of the
// pixel's x and y (thresholds 0.2 even-even, 0.6 even-odd, 0.8 odd-even, 0.4 odd-odd; a pixel is drawn when the alpha
// is at or above its threshold) and mark the pixels they drew in the alpha of the third G-buffer target when the alpha
// is below 0.8. So an alpha of 0.3 draws one pixel in four, 0.5 two, 0.7 three.
//
// THE PROOF, at 427x240 and at 3840x2160 (where one texel is a small number and the taps have to land just as exactly):
//   1  THE THREE LEVELS. An object of one colour over a background of another, at 0.3, 0.5 and 0.7: every pixel inside
//      (three pixels in from the object's edge, where the taps reach outside) leaves as the true blend, a quarter, a
//      half, three quarters of the way from the background to the object, in mode 1 and in mode 2.
//   2  NO MARK, NO CHANGE. A frame of noise up to six times white with no mark anywhere leaves bit for bit as it came,
//      and so does every pixel of picture 1 that is further than three pixels from a marked one.
//   3  A MARK IS NOT ENOUGH. With the mark set on EVERY pixel: a flat colour, a hard vertical edge, a hard horizontal
//      edge and a ramp all leave bit for bit as they came (the test finds no odd pixel in them).
//   4  WHAT THE RULE COSTS, pinned: one bright pixel alone on a marked flat surface IS taken for dither, as FusionFix's
//      filter takes it. Mode 1 writes the surroundings there, mode 2 a quarter of the way to the pixel.
//   5  THE VIEW paints exactly the pixels the pass moves: magenta inside the three objects, the green tint on a marked
//      flat surface (nothing moves there), and nothing at all without a mark.
//   6  WHICH PIXELS ONE MARK REACHES: a single marked pixel, at each of the four parities, makes the pass look at
//      exactly four pixels: itself, the one to its left, the one above and the one diagonally above. Read from the
//      view's green tint on a flat picture. (The mark is read with one filtered tap; this is what holds it to the
//      four pixels it stands for.)
//   7  THE COUNTER (development builds: ivt_dedither_probe_ps.h inside an occlusion query, colour writes off) counts
//      what is there: every pixel of a texture that holds colour and none of a blank one (its control), exactly the
//      pixels a mark reaches, and the pixels the pass moved (counted here from the pass's own output), on picture 1.
//   8  THE COUNT THAT LETS THE PASS REST (DedCount, ivt_dedither_count_ps.h: one texel in sixteen inside an occlusion
//      query, colour writes off) answers with exactly the number of texels (4x, 4y) that hold the mark, zero on a
//      picture with none, and leaves the target it is drawn on untouched. Every dithered object of the pictures here is
//      seen by it. Its control: the count asked with another size's texel does not give that number.
//   9  THE TWO DRAWS GIVE THE FIRST FORM'S PICTURE, BIT FOR BIT. The first form (dd_ref_ps.hlsl, kept here as the
//      reference) wrote a whole copy of the scene with the changed pixels in it. On the three levels and on noise with
//      scattered marks, in both modes, the scene buffer after the two draws is that copy on every pixel and in all four
//      channels, with the pass's target full of magenta beforehand. And THE TARGET IS WRITTEN ONLY WHERE A MARK
//      REACHES: every other pixel of it is still the magenta it was filled with.
//      Its controls: the second draw with another size's texel puts the pixels in the wrong places; the second draw
//      WITHOUT its test of the mark (a blank mark given to it: nothing comes home) and with a mark that is set
//      everywhere (the magenta comes home) both leave the scene buffer wrong.
// FOUR NEGATIVE CONTROLS must FAIL proof 1: the scene sampled POINT (a tap is then one pixel, not a block's mean), no
// mark given (a blank texture on stage 1), a texel constant for another size, and the mark sampled POINT.
//
//     tools\test-dedither\build.ps1           builds dd.exe (x86, as the game)
//     dd.exe --d3d9 <path> [--timing]         <path>: DXVK's d3d9, a game folder's vulkan.dll (the game's own translation)
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
#include "../../source/shaders/ivt_dedither_ps.h"
#include "../../source/shaders/ivt_dedither_home_ps.h"
#include "dd_ref_ps.h"                         // the pass's first form, compiled by build.ps1 into %TEMP%: proof 9's reference
#include "../../source/shaders/ivt_dedither_view_ps.h"
#include "../../source/shaders/ivt_dedither_probe_ps.h"
#include "../../source/shaders/ivt_dedither_count_ps.h"
#include "../../source/transport/dedither_pass.h"

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
#define CK(x) do { HRESULT hr_ = (x); if (FAILED(hr_)) { printf("  FAIL  %s: 0x%08lX (line %d)\n", #x, (unsigned long)hr_, __LINE__); exit(2); } } while (0)

// IEEE half, round to nearest even (g9.cpp's)
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
    const uint32_t sign = (uint32_t)(h & 0x8000u) << 16; const uint32_t e = (h >> 10) & 0x1Fu, m = h & 0x3FFu; uint32_t x;
    if (e == 0) { if (!m) x = sign; else { int k = 0; uint32_t mm = m; while (!(mm & 0x400u)) { mm <<= 1; k++; } x = sign | ((uint32_t)(113 - k) << 23) | ((mm & 0x3FFu) << 13); } }
    else if (e == 31) x = sign | 0x7F800000u | (m << 13);
    else x = sign | ((e + 112u) << 23) | (m << 13);
    float f; memcpy(&f, &x, 4); return f;
}
static uint32_t gRng = 0x12345678u;
static float Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0f; }

struct Px { float r, g, b; };
struct Pic { int w = 0, h = 0; std::vector<Px> px; std::vector<uint8_t> mark; };
static Pic Blank(int w, int h, Px bg) { Pic p; p.w = w; p.h = h; p.px.assign((size_t)w * h, bg); p.mark.assign((size_t)w * h, 0); return p; }
// THE GAME'S PATTERN, as FusionFix's deferred pass draws it
static float Threshold(int x, int y) { return (x & 1) ? ((y & 1) ? 0.4f : 0.8f) : ((y & 1) ? 0.6f : 0.2f); }
static void Dither(Pic& p, int x0, int y0, int x1, int y1, float alpha, Px obj)
{
    for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x)
        if (alpha >= Threshold(x, y)) { p.px[(size_t)y * p.w + x] = obj; p.mark[(size_t)y * p.w + x] = (alpha < 0.8f) ? 255 : 0; }
}

struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DPixelShader9 *ps = nullptr, *view = nullptr, *probe = nullptr, *count = nullptr, *home = nullptr, *ref = nullptr;
    IDirect3DTexture9 *scene = nullptr, *mark = nullptr, *blank = nullptr, *full = nullptr, *rt = nullptr; IDirect3DSurface9 *rtSurf = nullptr, *sys = nullptr;
    // THE SCENE BUFFER AS THE MOD MEETS IT: a render target. `up` holds the loaded picture (system memory), `keep` a copy
    // of it on the card (the timing puts the buffer back from it between passes), `out` is what Read reads: the scene
    // buffer after the shipping pass, the pass's own target after the view or the reference.
    IDirect3DTexture9 *buf = nullptr, *keep = nullptr; IDirect3DSurface9 *bufSurf = nullptr, *keepSurf = nullptr, *up = nullptr, *out = nullptr;
    IDirect3DQuery9* qFreq = nullptr; IDirect3DQuery9* qDis = nullptr;
    int w = 0, h = 0;
};
static const float kQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };

static void Size(Dev& v, int w, int h)
{
    for (IUnknown* u : { (IUnknown*)v.scene, (IUnknown*)v.mark, (IUnknown*)v.blank, (IUnknown*)v.rtSurf, (IUnknown*)v.rt, (IUnknown*)v.sys,
                         (IUnknown*)v.bufSurf, (IUnknown*)v.buf, (IUnknown*)v.keepSurf, (IUnknown*)v.keep, (IUnknown*)v.up, (IUnknown*)v.full }) if (u) u->Release();
    v.w = w; v.h = h; v.out = nullptr;
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &v.buf, nullptr));  CK(v.buf->GetSurfaceLevel(0, &v.bufSurf));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &v.keep, nullptr)); CK(v.keep->GetSurfaceLevel(0, &v.keepSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &v.up, nullptr));
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &v.full, nullptr));
    { D3DLOCKED_RECT lr; CK(v.full->LockRect(0, &lr, nullptr, 0)); for (int y = 0; y < h; ++y) memset((BYTE*)lr.pBits + (size_t)y * lr.Pitch, 0xFF, (size_t)w * 4); v.full->UnlockRect(0); }
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_MANAGED, &v.scene, nullptr));
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &v.mark, nullptr));
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &v.blank, nullptr));
    { D3DLOCKED_RECT lr; CK(v.blank->LockRect(0, &lr, nullptr, 0)); for (int y = 0; y < h; ++y) memset((BYTE*)lr.pBits + (size_t)y * lr.Pitch, 0, (size_t)w * 4); v.blank->UnlockRect(0); }
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &v.rt, nullptr));
    CK(v.rt->GetSurfaceLevel(0, &v.rtSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &v.sys, nullptr));
}
static void Load(Dev& v, const Pic& p)
{
    D3DLOCKED_RECT lr; CK(v.scene->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < p.h; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < p.w; ++x) { const Px& q = p.px[(size_t)y * p.w + x]; row[4 * x] = ToHalf(q.r); row[4 * x + 1] = ToHalf(q.g); row[4 * x + 2] = ToHalf(q.b); row[4 * x + 3] = ToHalf(1.0f); } }
    v.scene->UnlockRect(0);
    // and the same picture for the scene buffer: system memory, then a copy on the card
    CK(v.up->LockRect(&lr, nullptr, 0));
    for (int y = 0; y < p.h; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < p.w; ++x) { const Px& q = p.px[(size_t)y * p.w + x]; row[4 * x] = ToHalf(q.r); row[4 * x + 1] = ToHalf(q.g); row[4 * x + 2] = ToHalf(q.b); row[4 * x + 3] = ToHalf(1.0f); } }
    v.up->UnlockRect();
    CK(v.d->UpdateSurface(v.up, nullptr, v.keepSurf, nullptr));
    CK(v.mark->LockRect(0, &lr, nullptr, 0));
    // the other three channels carry what the game keeps there (anything): the pass must read the alpha alone
    for (int y = 0; y < p.h; ++y) { DWORD* row = (DWORD*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < p.w; ++x) row[x] = ((DWORD)p.mark[(size_t)y * p.w + x] << 24) | 0x00C08040u; }
    v.mark->UnlockRect(0);
}
// THE COMMON STATE OF A FULL-SCREEN PASS OF THE MOD'S (IvtSetupPass, DlssIvTransport.h), then the pass's own draw.
// `wrong`: 0 the shipping draw; 1 the scene sampled POINT; 2 a blank texture for the mark; 3 another size's texel;
// 4 the mark sampled POINT.
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
// THE COUNTER'S DRAW, as the mod makes it: colour writes off, inside an occlusion query, through DedDraw
static DWORD Count(Dev& v, int which, bool blankMark = false)
{
    IDirect3DQuery9* q = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q));
    Common(v);
    v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    CK(q->Issue(D3DISSUE_BEGIN));
    CK(DedDraw(v.d, v.scene, blankMark ? v.blank : v.mark, v.rtSurf, v.probe, (UINT)v.w, (UINT)v.h, 1, kQuad, sizeof(kQuad[0]), (float)which));
    CK(q->Issue(D3DISSUE_END));
    DWORD n = 0; while (q->GetData(&n, sizeof(n), D3DGETDATA_FLUSH) == S_FALSE) {}
    q->Release();
    v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
    return n;
}
// THE COUNT THAT LETS THE PASS REST, as the mod asks it (DedCount). `wrongSize`: the control, another size's texel.
static DWORD Lattice(Dev& v, bool wrongSize = false)
{
    IDirect3DQuery9* q = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q));
    Common(v);
    CK(DedCount(v.d, v.mark, v.rtSurf, v.count, (UINT)(wrongSize ? v.w / 2 : v.w), (UINT)(wrongSize ? v.h / 2 : v.h), kQuad, sizeof(kQuad[0]), q));
    DWORD n = 0; while (q->GetData(&n, sizeof(n), D3DGETDATA_FLUSH) == S_FALSE) {}
    q->Release();
    return n;
}
static long LatticeTruth(const Pic& p)
{
    long n = 0;
    for (int y = 0; y < p.h; y += 4) for (int x = 0; x < p.w; x += 4) if (p.mark[(size_t)y * p.w + x]) n++;
    return n;
}
// ONE RUN OF A PASS. The shipping pass (v.ps): the scene buffer is loaded with the picture, then the mod's two draws,
// and Read reads the SCENE BUFFER. The view and the reference (the first form) write a whole copy into the pass's own
// target from the picture, and Read reads that. `wrong` 1 to 4: the shipping pass with its first draw wrong;
// 5: its second draw with another size's texel; 6: its second draw given a blank mark; 7: its second draw given a
// mark that is set on every pixel.
static void Draw(Dev& v, IDirect3DPixelShader9* ps, int mode, int wrong = 0)
{
    IDirect3DDevice9* d = v.d;
    Common(v);
    const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f };
    if (ps != v.ps)
    {   // a whole copy, as the first form made it
        CK(DedDraw(d, v.scene, v.mark, v.rtSurf, ps, (UINT)v.w, (UINT)v.h, mode, kQuad, sizeof(kQuad[0])));
        d->SetTexture(0, nullptr); d->SetTexture(1, nullptr);
        v.out = v.rtSurf; return;
    }
    CK(d->UpdateSurface(v.up, nullptr, v.bufSurf, nullptr));       // the scene buffer as the scene left it
    v.out = v.bufSurf;
    // THE PASS'S TARGET HOLDS ANOTHER PICTURE, written everywhere: magenta, alpha 1 (the mod never clears it)
    d->SetRenderTarget(0, v.rtSurf); d->SetViewport(&vp);
    d->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 255, 0, 255), 1.0f, 0);
    if (wrong == 0) { CK(DedInPlace(d, v.buf, v.bufSurf, v.mark, v.rt, v.rtSurf, v.ps, v.home, (UINT)v.w, (UINT)v.h, mode, kQuad, sizeof(kQuad[0]))); return; }
    // THE WRONG DRAWS: DedInPlace's own calls with one thing changed
    if (wrong <= 4)
    {
        const float c0[4] = { 1.0f / (float)(wrong == 3 ? v.w / 2 : v.w), 1.0f / (float)(wrong == 3 ? v.h / 2 : v.h), (float)mode, 0.0f };
        d->SetTexture(0, v.buf); d->SetTexture(1, wrong == 2 ? v.blank : v.mark);
        const DWORD f = wrong == 1 ? D3DTEXF_POINT : D3DTEXF_LINEAR, fm = wrong == 4 ? D3DTEXF_POINT : D3DTEXF_LINEAR;
        d->SetSamplerState(0, D3DSAMP_MAGFILTER, f); d->SetSamplerState(0, D3DSAMP_MINFILTER, f);
        d->SetSamplerState(1, D3DSAMP_MAGFILTER, fm); d->SetSamplerState(1, D3DSAMP_MINFILTER, fm);
        d->SetPixelShaderConstantF(0, c0, 1); d->SetPixelShader(ps);
        CK(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
        CK(DedHome(d, v.rt, wrong == 2 ? v.blank : v.mark, v.bufSurf, v.home, (UINT)v.w, (UINT)v.h, kQuad, sizeof(kQuad[0])));
    }
    else if (wrong >= 6)
    {   // the first draw as shipped, the second given another mark than the first read
        CK(DedDraw(d, v.buf, v.mark, v.rtSurf, v.ps, (UINT)v.w, (UINT)v.h, mode, kQuad, sizeof(kQuad[0])));
        CK(DedHome(d, v.rt, wrong == 6 ? v.blank : v.full, v.bufSurf, v.home, (UINT)v.w, (UINT)v.h, kQuad, sizeof(kQuad[0])));
    }
    else
    {   // the first draw as shipped, the second with another size's texel
        CK(DedDraw(d, v.buf, v.mark, v.rtSurf, v.ps, (UINT)v.w, (UINT)v.h, mode, kQuad, sizeof(kQuad[0])));
        const float c0[4] = { 1.0f / (float)(v.w / 2), 1.0f / (float)(v.h / 2), 0.0f, 0.0f };
        d->SetTexture(0, nullptr);
        d->SetRenderTarget(0, v.bufSurf); d->SetViewport(&vp);
        d->SetTexture(0, v.rt); d->SetTexture(1, v.mark);
        d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        d->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR); d->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        d->SetPixelShaderConstantF(0, c0, 1); d->SetPixelShader(v.home);
        d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x07);
        CK(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
        d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
        d->SetTexture(0, nullptr); d->SetTexture(1, nullptr);
    }
    d->SetRenderTarget(0, v.rtSurf);
}
static std::vector<uint16_t> ReadSurf(Dev& v, IDirect3DSurface9* s)
{
    CK(v.d->GetRenderTargetData(s, v.sys));
    std::vector<uint16_t> out((size_t)v.w * v.h * 4); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; ++y) memcpy(&out[(size_t)y * v.w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(uint16_t) * v.w * 4);
    v.sys->UnlockRect(); return out;
}
static std::vector<uint16_t> Read(Dev& v)
{
    CK(v.d->GetRenderTargetData(v.out ? v.out : v.rtSurf, v.sys));
    std::vector<uint16_t> out((size_t)v.w * v.h * 4); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; ++y) memcpy(&out[(size_t)y * v.w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(uint16_t) * v.w * 4);
    v.sys->UnlockRect(); return out;
}
static bool Same(const std::vector<uint16_t>& o, const Pic& p, int x, int y)   // bit for bit what went in
{
    const Px& q = p.px[(size_t)y * p.w + x]; const uint16_t* t = &o[4 * ((size_t)y * p.w + x)];
    return t[0] == ToHalf(q.r) && t[1] == ToHalf(q.g) && t[2] == ToHalf(q.b) && t[3] == ToHalf(1.0f);
}
static bool Near(const std::vector<uint16_t>& o, int w, int x, int y, Px want)   // within two FP16 steps of the answer
{
    const uint16_t* t = &o[4 * ((size_t)y * w + x)]; const float got[3] = { FromHalf(t[0]), FromHalf(t[1]), FromHalf(t[2]) }, exp[3] = { want.r, want.g, want.b };
    for (int i = 0; i < 3; ++i) if (fabsf(got[i] - exp[i]) > 0.002f * fabsf(exp[i]) + 1e-5f) return false;
    return true;
}
static Px Mix(Px a, Px b, float t) { return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t }; }

// PICTURE 1: three objects over one background, and where they are
struct Box { int x0, y0, x1, y1; float alpha; int drawn; };
static const Px kBg = { 0.30f, 0.45f, 0.90f }, kObj = { 2.50f, 0.20f, 0.05f };
static Pic Levels(int w, int h, Box box[3])
{
    Pic p = Blank(w, h, kBg);
    const int bw = w / 5, bh = h / 2;
    const float al[3] = { 0.3f, 0.5f, 0.7f };
    for (int i = 0; i < 3; ++i)
    {
        box[i] = { w / 10 + i * (bw + w / 20) + 1, h / 4 + 1, w / 10 + i * (bw + w / 20) + 1 + bw, h / 4 + 1 + bh, al[i], i + 1 };   // odd corners: the pattern's phase is not the box's
        Dither(p, box[i].x0, box[i].y0, box[i].x1, box[i].y1, al[i], kObj);
    }
    return p;
}
// how many pixels inside the three objects are NOT the true blend
static long LevelsWrong(const std::vector<uint16_t>& o, int w, const Box box[3])
{
    long bad = 0;
    for (int i = 0; i < 3; ++i) for (int y = box[i].y0 + 3; y < box[i].y1 - 3; ++y) for (int x = box[i].x0 + 3; x < box[i].x1 - 3; ++x)
        if (!Near(o, w, x, y, Mix(kBg, kObj, box[i].drawn / 4.0f))) bad++;
    return bad;
}
// every pixel within r of a marked one, once a picture (rows, then columns)
static std::vector<uint8_t> NearMap(const Pic& p, int r)
{
    std::vector<uint8_t> a((size_t)p.w * p.h, 0), o((size_t)p.w * p.h, 0);
    for (int y = 0; y < p.h; ++y) for (int x = 0; x < p.w; ++x) if (p.mark[(size_t)y * p.w + x])
        for (int xx = std::max(0, x - r); xx <= std::min(p.w - 1, x + r); ++xx) a[(size_t)y * p.w + xx] = 1;
    for (int y = 0; y < p.h; ++y) for (int x = 0; x < p.w; ++x) if (a[(size_t)y * p.w + x])
        for (int yy = std::max(0, y - r); yy <= std::min(p.h - 1, y + r); ++yy) o[(size_t)yy * p.w + x] = 1;
    return o;
}

static void Proofs(Dev& v, int w, int h)
{
    printf(" at %dx%d\n", w, h);
    Size(v, w, h);
    Box box[3]; const Pic lev = Levels(w, h, box);
    long inside = 0; for (int i = 0; i < 3; ++i) inside += (long)(box[i].x1 - box[i].x0 - 6) * (box[i].y1 - box[i].y0 - 6);
    Load(v, lev);
    const std::vector<uint8_t> nearMark = NearMap(lev, 3);
    for (int mode = 1; mode <= 2; ++mode)
    {
        Draw(v, v.ps, mode); const auto o = Read(v);
        const long bad = LevelsWrong(o, w, box);
        Proof(bad == 0, "1  mode %d: the three levels (one, two, three pixels in four) leave as the true blend on %ld of %ld pixels inside the objects", mode, inside - bad, inside);
        long away = 0, farBad = 0;
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (!nearMark[(size_t)y * w + x]) { away++; if (!Same(o, lev, x, y)) farBad++; }
        Proof(farBad == 0 && away > 0, "2  mode %d: every pixel further than three from a mark leaves bit for bit as it came (%ld pixels, %ld moved)", mode, away, farBad);
    }
    // THE COUNTER on the same picture
    {
        Draw(v, v.ps, 1); const auto o = Read(v);
        long reach = 0, moved = 0;
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        {
            const int x1 = std::min(x + 1, w - 1), y1 = std::min(y + 1, h - 1);
            if (lev.mark[(size_t)y * w + x] | lev.mark[(size_t)y * w + x1] | lev.mark[(size_t)y1 * w + x] | lev.mark[(size_t)y1 * w + x1]) reach++;
            const Px& q = lev.px[(size_t)y * w + x]; const uint16_t* t = &o[4 * ((size_t)y * w + x)];
            const float in[3] = { FromHalf(ToHalf(q.r)), FromHalf(ToHalf(q.g)), FromHalf(ToHalf(q.b)) };
            bool m = false;
            for (int i = 0; i < 3; ++i) if (fabsf(FromHalf(t[i]) - in[i]) - 0.01f * (fabsf(in[i]) + 0.01f) > 0.0f) m = true;
            if (m) moved++;
        }
        const DWORD c0 = Count(v, 0), c0b = Count(v, 0, true), c1 = Count(v, 1), c1b = Count(v, 1, true), c2 = Count(v, 2);
        Proof(c0 == (DWORD)(w * h) && c0b == 0, "7  the counter's control: %lu of %d pixels of a texture that holds colour, %lu of a blank one", (unsigned long)c0, w * h, (unsigned long)c0b);
        Proof(c1 == (DWORD)reach && c1b == 0 && reach > 0, "7  the counter counts the pixels a mark reaches: %lu, the picture has %ld (a blank mark: %lu)", (unsigned long)c1, reach, (unsigned long)c1b);
        Proof(moved > 0 && labs((long)c2 - moved) <= moved / 10000 + 2, "7  the counter counts the pixels the pass moves: %lu, the pass's own output moved %ld", (unsigned long)c2, moved);
    }
    // THE COUNT THAT LETS THE PASS REST, on the same picture
    {
        Draw(v, v.ps, 1); const auto before = ReadSurf(v, v.rtSurf);
        const DWORD got = Lattice(v); const long want = LatticeTruth(lev);
        const auto after = ReadSurf(v, v.rtSurf);
        Proof((long)got == want && want > 0 && before == after, "8  the count: %lu of the one-in-sixteen texels hold the mark, the picture has %ld, and the target it was drawn on is untouched", (unsigned long)got, want);
        long seen = 0;
        for (int i = 0; i < 3; ++i)
        {   // each object alone: is it seen?
            Pic one = Blank(w, h, kBg); Dither(one, box[i].x0, box[i].y0, box[i].x1, box[i].y1, box[i].alpha, kObj);
            Load(v, one); if (Lattice(v) > 0) seen++;
        }
        Proof(seen == 3, "8  each of the three levels alone is seen by the count (%ld of 3): the texels it reads are drawn at every level", seen);
        Load(v, lev);
        const DWORD wrongN = Lattice(v, true);
        printf("  %s  CONTROL (the count with another size's texel): %lu, the picture has %ld%s\n", (long)wrongN != want ? "PASS" : "FAIL", (unsigned long)wrongN, want,
               (long)wrongN != want ? ", as it must differ" : ": PROOF 8 CANNOT SEE THIS FAULT");
        if ((long)wrongN == want) gFails++;
    }
    // THE TWO DRAWS AGAINST THE FIRST FORM, on the three levels and on noise with scattered marks
    {
        Pic sp = Blank(w, h, kBg); for (auto& q : sp.px) q = { 6.0f * Rnd(), 6.0f * Rnd(), 6.0f * Rnd() };
        for (size_t i = 0; i < sp.mark.size(); ++i) if ((int)(Rnd() * 50.0f) == 0) sp.mark[i] = 255;
        long differ = 0, pixels = 0, written = 0, astray = 0, untouched = 0;
        const Pic* pics[2] = { &lev, &sp };
        const uint16_t kMag[4] = { ToHalf(1.0f), ToHalf(0.0f), ToHalf(1.0f), ToHalf(1.0f) };       // what the target was filled with
        for (int pi = 0; pi < 2; ++pi)
        {
            Load(v, *pics[pi]);
            std::vector<uint8_t> reach((size_t)w * h, 0);
            for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
            {
                const int x1 = std::min(x + 1, w - 1), y1 = std::min(y + 1, h - 1); const Pic& p = *pics[pi];
                reach[(size_t)y * w + x] = (p.mark[(size_t)y * w + x] | p.mark[(size_t)y * w + x1] | p.mark[(size_t)y1 * w + x] | p.mark[(size_t)y1 * w + x1]) ? 1 : 0;
            }
            for (int mode = 1; mode <= 2; ++mode)
            {
                Draw(v, v.ref, mode); const auto first = Read(v);
                Draw(v, v.ps, mode);  const auto now = Read(v); const auto held = ReadSurf(v, v.rtSurf);
                for (size_t i = 0; i < (size_t)w * h; ++i)
                {
                    pixels++;
                    if (memcmp(&first[4 * i], &now[4 * i], 8) != 0) differ++;
                    const bool mag = memcmp(&held[4 * i], kMag, 8) == 0;
                    if (reach[i]) { written++; if (mag) astray++; }           // a pixel a mark reaches was written (no longer the magenta)
                    else          { untouched++; if (!mag) astray++; }        // every other one is still the magenta
                }
            }
        }
        Load(v, lev);
        Proof(differ == 0 && pixels > 0, "9  the two draws give the first form's picture: %ld of %ld pixels of the scene buffer differ from the copy the first form wrote (all four channels, both modes, two pictures; the pass's target full of magenta beforehand)", differ, pixels);
        Proof(written > 0 && untouched > 0 && astray == 0,
              "9  the target is written only where a mark reaches: %ld pixels of it were written (%.2f percent), %ld are still the magenta it was filled with (%ld astray)",
              written, 100.0 * (double)written / (double)pixels, untouched, astray);
        Draw(v, v.ref, 1); const auto first = Read(v);
        const char* kHome[3] = { "another size's texel", "a blank mark (nothing comes home)", "a mark set on every pixel (the magenta comes home)" };
        for (int k = 0; k < 3; ++k)
        {
            Draw(v, v.ps, 1, 5 + k); const auto bad = Read(v);
            long off = 0; for (size_t i = 0; i < (size_t)w * h; ++i) if (memcmp(&first[4 * i], &bad[4 * i], 8) != 0) off++;
            printf("  %s  CONTROL (the second draw with %s): %ld pixels differ from the first form's picture%s\n", off > 0 ? "PASS" : "FAIL", kHome[k], off,
                   off > 0 ? ", as it must differ" : ": PROOF 9 CANNOT SEE THIS FAULT");
            if (off == 0) gFails++;
        }
    }
    // THE CONTROLS, on the same picture: each must leave the objects' insides wrong
    const char* kWrong[5] = { "", "the scene sampled POINT", "no mark given", "another size's texel", "the mark sampled POINT" };
    for (int wrong = 1; wrong <= 4; ++wrong)
    {
        Draw(v, v.ps, 1, wrong); const auto o = Read(v);
        const long bad = LevelsWrong(o, w, box);
        const bool seen = bad > inside / 8;
        printf("  %s  CONTROL (%s): %ld of %ld pixels inside the objects are not the blend%s\n", seen ? "PASS" : "FAIL", kWrong[wrong], bad, inside,
               seen ? ", as it must be" : ": THE PROOF CANNOT SEE THIS FAULT");
        if (!seen) gFails++;
    }
    // ONE MARK, FOUR PIXELS: the view's green tint on a flat picture, a single marked pixel at each parity
    {
        long wrongReach = 0, tinted = 0;
        for (int k = 0; k < 4; ++k)
        {
            Pic one = Blank(w, h, kBg); const int mx = w / 2 + (k & 1), my = h / 2 + (k >> 1); one.mark[(size_t)my * w + mx] = 255;
            Load(v, one); Draw(v, v.view, 1); const auto o = Read(v);
            const Px tint = Mix(kBg, { 0.0f, 2.0f, 0.0f }, 0.5f);
            for (int y = my - 4; y <= my + 4; ++y) for (int x = mx - 4; x <= mx + 4; ++x)
            {
                const bool reach = (x == mx || x == mx - 1) && (y == my || y == my - 1);
                const bool is = Near(o, w, x, y, tint);
                if (is) tinted++;
                if (is != reach || (!reach && !Same(o, one, x, y))) wrongReach++;
            }
        }
        Proof(wrongReach == 0 && tinted == 16, "6  one marked pixel reaches exactly four pixels (itself, left, above, diagonally above), at each of the four parities (%ld tinted of 16, %ld wrong)", tinted, wrongReach);
        Load(v, lev);
    }
    // THE VIEW on the same picture
    {
        Draw(v, v.view, 1); const auto o = Read(v);
        long magenta = 0, other = 0, farBad = 0;
        for (int i = 0; i < 3; ++i) for (int y = box[i].y0 + 3; y < box[i].y1 - 3; ++y) for (int x = box[i].x0 + 3; x < box[i].x1 - 3; ++x)
        { if (Near(o, w, x, y, { 4.0f, 0.0f, 4.0f })) magenta++; else other++; }
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (!nearMark[(size_t)y * w + x] && !Same(o, lev, x, y)) farBad++;
        Proof(other == 0 && farBad == 0, "5  the view: magenta on %ld of %ld pixels inside the objects, and nothing painted away from a mark (%ld)", magenta, inside, farBad);
    }
    // NO MARK AT ALL: noise up to six times white
    {
        Pic n = Blank(w, h, kBg); for (auto& q : n.px) q = { 6.0f * Rnd(), 6.0f * Rnd(), 6.0f * Rnd() };
        Load(v, n);
        long moved = 0;
        for (int mode = 1; mode <= 2; ++mode) { Draw(v, v.ps, mode); const auto o = Read(v); for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (!Same(o, n, x, y)) moved++; }
        Proof(moved == 0, "2  noise with no mark anywhere: bit for bit as it came, both modes (%ld pixels moved of %ld)", moved, 2L * w * h);
        {
            const DWORD none0 = Lattice(v);
            Pic sp = n; for (size_t i = 0; i < sp.mark.size(); ++i) if ((int)(Rnd() * 500.0f) == 0) sp.mark[i] = 255;   // scattered single marks
            Load(v, sp);
            const DWORD got = Lattice(v); const long want = LatticeTruth(sp);
            Proof(none0 == 0 && (long)got == want && want > 0, "8  the count: zero on a picture with no mark; on noise with scattered single marks %lu, the picture has %ld", (unsigned long)got, want);
            Load(v, n);
        }
        Draw(v, v.view, 1); const auto o = Read(v); long painted = 0; for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (!Same(o, n, x, y)) painted++;
        Proof(painted == 0, "5  the view of it: nothing painted (%ld)", painted);
    }
    // A MARK EVERYWHERE AND NO DITHER: flat, a vertical edge, a horizontal edge, a ramp, four bands
    {
        Pic m = Blank(w, h, kBg); std::fill(m.mark.begin(), m.mark.end(), (uint8_t)255);
        for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
        {
            Px& q = m.px[(size_t)y * w + x];
            if      (y < h / 4)     q = kBg;                                                      // flat
            else if (y < h / 2)     q = (x < w / 2) ? Px{ 0.1f, 0.1f, 0.1f } : Px{ 3.0f, 2.0f, 1.0f };   // a vertical edge
            else if (y < 3 * h / 4) q = { 0.5f + 0.01f * (float)(x % 200), 0.25f + 0.005f * (float)(x % 200), 0.1f };   // a ramp (it restarts every 200 pixels: an edge too)
            else                    q = { 1.2f, 0.7f, 0.2f };                                     // and below, another flat: the band boundaries are horizontal edges
        }
        Load(v, m);
        long moved = 0;
        for (int mode = 1; mode <= 2; ++mode) { Draw(v, v.ps, mode); const auto o = Read(v); for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (!Same(o, m, x, y)) moved++; }
        Proof(moved == 0, "3  the mark on every pixel of a picture with no dither (flat, edges both ways, a ramp): bit for bit as it came, both modes (%ld moved of %ld)", moved, 2L * w * h);
        Draw(v, v.view, 1); const auto o = Read(v);
        const int x = w / 3, y = h / 8;
        Proof(Near(o, w, x, y, Mix(kBg, { 0.0f, 2.0f, 0.0f }, 0.5f)), "5  the view of it: the green tint on a marked flat surface, where nothing moves");
        // ONE BRIGHT PIXEL ALONE on the marked flat band
        Pic s = m; const int sx = w / 3 + 1, sy = h / 8; const Px bright = { 5.0f, 5.0f, 5.0f }; s.px[(size_t)sy * w + sx] = bright;
        Load(v, s);
        Draw(v, v.ps, 1); const auto o1 = Read(v); Draw(v, v.ps, 2); const auto o2 = Read(v);
        Proof(Near(o1, w, sx, sy, kBg) && Near(o2, w, sx, sy, Mix(kBg, bright, 0.25f)),
              "4  one bright pixel alone on a marked flat surface is taken for dither (the rule's cost, FusionFix's too): mode 1 writes the surroundings, mode 2 a quarter of the way to it");
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
    printf("The dither pass's picture proof: the shipping shader and draw against pictures of known answer, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "ddproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("ddproof", "dd", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtDeditherPS, &v.ps));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtDeditherViewPS, &v.view));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtDeditherProbePS, &v.probe));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtDeditherCountPS, &v.count));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtDeditherHomePS, &v.home));
    CK(v.d->CreatePixelShader((const DWORD*)kDdRefPS, &v.ref));
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &v.qFreq); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &v.qDis);

    Proofs(v, 427, 240);
    Proofs(v, 3840, 2160);

    const int kSizes[3][2] = { { 3840, 2160 }, { 2560, 1440 }, { 1280, 720 } };
    if (timing) for (int si = 0; si < 3; ++si)
    {   // the pass's own GPU time by this card's timestamps, 20 draws a sample, the median of 30, at three render sizes
        const int TW = kSizes[si][0], TH = kSizes[si][1];
        Size(v, TW, TH);
        // WHAT IS TIMED, as the first form was on 2026-10-03: twenty passes between two timestamps, the median of thirty such
        // samples. `between`: another draw between two passes (the count, on a small target of its own), which is how a
        // frame runs: back to back, a pass finds what the pass before it left in the card's caches.
        // `what`: 1 the pass (its two draws, mode 1); 2 the same in mode 2; 3 the pass's FIRST FORM (a whole copy);
        // 0 nothing but the draw between.
        // THE SCENE BUFFER IS NOT WRITTEN HERE: the second draw goes home into a buffer of its own (the same format and
        // size), so every pass meets the picture as the scene left it and nothing has to be put back.
        IDirect3DTexture9* smallT = nullptr; IDirect3DSurface9* smallS = nullptr;
        CK(v.d->CreateTexture((TW + 3) / 4, (TH + 3) / 4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &smallT, nullptr)); CK(smallT->GetSurfaceLevel(0, &smallS));
        auto Time = [&](int what, bool between) -> double {
            IDirect3DQuery9 *q0 = nullptr, *q1 = nullptr, *oq = nullptr; v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q1);
            v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &oq);
            if (!q0 || !q1 || !oq || !v.qDis || !v.qFreq) return -1.0;
            v.qDis->Issue(D3DISSUE_BEGIN); q0->Issue(D3DISSUE_END);
            for (int k = 0; k < 20; ++k)
            {
                Common(v);
                if (between) DedCount(v.d, v.mark, smallS, v.count, (UINT)v.w, (UINT)v.h, kQuad, sizeof(kQuad[0]), oq);
                if (what == 1 || what == 2) DedInPlace(v.d, v.buf, v.keepSurf, v.mark, v.rt, v.rtSurf, v.ps, v.home, (UINT)v.w, (UINT)v.h, what, kQuad, sizeof(kQuad[0]));
                if (what == 3) DedDraw(v.d, v.buf, v.mark, v.rtSurf, v.ref, (UINT)v.w, (UINT)v.h, 1, kQuad, sizeof(kQuad[0]));
            }
            q1->Issue(D3DISSUE_END); v.qDis->Issue(D3DISSUE_END); v.qFreq->Issue(D3DISSUE_END);
            UINT64 a0 = 0, a1 = 0, f = 0; BOOL dis = TRUE;
            while (q1->GetData(&a1, sizeof(a1), D3DGETDATA_FLUSH) == S_FALSE) {}
            q0->GetData(&a0, sizeof(a0), D3DGETDATA_FLUSH); v.qFreq->GetData(&f, sizeof(f), D3DGETDATA_FLUSH); v.qDis->GetData(&dis, sizeof(dis), D3DGETDATA_FLUSH);
            q0->Release(); q1->Release(); oq->Release();
            v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr);
            return (dis || !f) ? -1.0 : (double)(a1 - a0) * 1000.0 / (double)f / 20.0; };
        auto Median = [&](int what, bool between) -> double { std::vector<double> s; for (int k = 0; k < 5; ++k) Time(what, between);
            for (int k = 0; k < 30; ++k) { const double x = Time(what, between); if (x > 0) s.push_back(x); }
            std::sort(s.begin(), s.end()); return s.empty() ? -1.0 : s[s.size() / 2]; };
        Pic none = Blank(TW, TH, kBg); for (auto& q : none.px) q = { 2.0f * Rnd(), 2.0f * Rnd(), 2.0f * Rnd() };
        Pic some = none; Dither(some, TW / 10, TH / 8, TW / 10 + (int)(TW * 0.3164f), TH / 8 + (int)(TH * 0.3162f), 0.5f, kObj);                     // a tenth of the screen dithered
        Pic thin = none; Dither(thin, TW / 10, TH / 8, TW / 10 + (int)(TW * 0.0707f), TH / 8 + (int)(TH * 0.0707f), 0.5f, kObj);                     // half a percent of it
        Pic all = none; Dither(all, 0, 0, TW, TH, 0.5f, kObj);
        const Pic* pics[4] = { &none, &thin, &some, &all }; const char* names[4] = { "no mark", "half a percent of the screen dithered", "a tenth of it", "every pixel" };
        for (int pi = 0; pi < 4; ++pi)
        {
            Load(v, *pics[pi]);
            CK(v.d->UpdateSurface(v.up, nullptr, v.bufSurf, nullptr));
            const double b1 = Median(1, false), b2 = Median(2, false), b3 = Median(3, false);
            const double gap = Median(0, true), f1 = Median(1, true) - gap, f2 = Median(2, true) - gap, f3 = Median(3, true) - gap;
            printf("  TIMING at %dx%d on this card's own clock (ms a pass), %s. BACK TO BACK: the pass %.4f (mode 2 %.4f), its first form %.4f."
                   " WITH ANOTHER DRAW BETWEEN TWO PASSES, as a frame runs (that draw's own %.4f taken off): the pass %.4f (mode 2 %.4f), its first form %.4f\n",
                   TW, TH, names[pi], b1, b2, b3, gap, f1, f2, f3);
        }
        Load(v, none);
        { IDirect3DQuery9* oq = nullptr; v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &oq);
          auto TimeC = [&]() -> double {
              IDirect3DQuery9 *q0 = nullptr, *q1 = nullptr; v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q1);
              if (!q0 || !q1 || !oq || !v.qDis || !v.qFreq) return -1.0;
              v.qDis->Issue(D3DISSUE_BEGIN); q0->Issue(D3DISSUE_END);
              for (int k = 0; k < 20; ++k) { Common(v); DedCount(v.d, v.mark, v.rtSurf, v.count, (UINT)v.w, (UINT)v.h, kQuad, sizeof(kQuad[0]), oq); }
              q1->Issue(D3DISSUE_END); v.qDis->Issue(D3DISSUE_END); v.qFreq->Issue(D3DISSUE_END);
              UINT64 a0 = 0, a1 = 0, f = 0; BOOL dis = TRUE;
              while (q1->GetData(&a1, sizeof(a1), D3DGETDATA_FLUSH) == S_FALSE) {}
              q0->GetData(&a0, sizeof(a0), D3DGETDATA_FLUSH); v.qFreq->GetData(&f, sizeof(f), D3DGETDATA_FLUSH); v.qDis->GetData(&dis, sizeof(dis), D3DGETDATA_FLUSH);
              q0->Release(); q1->Release();
              return (dis || !f) ? -1.0 : (double)(a1 - a0) * 1000.0 / (double)f / 20.0; };
          std::vector<double> s; for (int k = 0; k < 5; ++k) TimeC(); for (int k = 0; k < 30; ++k) { const double x = TimeC(); if (x > 0) s.push_back(x); }
          std::sort(s.begin(), s.end()); const double cNone = s.empty() ? -1.0 : s[s.size() / 2];
          printf("  TIMING at %dx%d, THE COUNT (on the pass's own target, colour writes off): %.4f. WHILE NOTHING IS DITHERED the mod draws the count alone on one frame in four: %.4f ms a frame on average\n",
                 TW, TH, cNone, cNone / 4.0);
          if (oq) oq->Release(); }
        smallS->Release(); smallT->Release();
    }
    printf("%s\n", gFails ? "THE DITHER PASS'S PICTURE PROOF: FAIL" : "THE DITHER PASS'S PICTURE PROOF: PASS");
    return gFails ? 1 : 0;
}
