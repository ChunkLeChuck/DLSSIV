// THE STEADY DEPTH'S PROOF (2026-10-03): the shipping shaders (ivt_steady_ps.h, ivt_steady_home_ps.h, and the
// development control ivt_steady_probe_ps.h) drawn by the shipping draws (transport/steady_depth_pass.h: SdResolve,
// SdHome, SdControl) on the GPU, with no game, against depth pictures whose right answer is known. tools/test-dedither
// is the pattern.
//
// THE PICTURES are what a jittered renderer leaves in a depth buffer: a scene of analytic shapes (a slanted box, a
// disc, a wire a pixel and a half wide, a wire 0.4 of a pixel wide) at one depth over a background at another, sampled
// once per pixel at the pixel's centre plus the frame's jitter. The jitter is a Halton (2, 3) cycle, as the mod's is.
// A frame here is what a frame is in the game: the scene writes the depth texture the post chain will read, then the
// mod resolves what it holds and writes it home. WHAT IS MEASURED IS THAT TEXTURE, read back as the post chain reads
// it.
//
// THE PROOF:
//   1  A STILL SCENE STANDS STILL. Over a whole jitter cycle, after settling: the depth that goes in flips by the whole
//      gap between the two surfaces on every pixel an edge crosses; the depth that comes out moves by a small part of
//      it, from one frame to the next by less. For cycles of 8, 18 and 32 phases, and by what crosses the pixel: a
//      solid edge, the wide wire, THE THIN WIRE, which is in no sample at all on some frames and is held all the same
//      (it is remembered). With the camera said to be moving it is not: pinned.
//   2  THE NEARER SURFACE WINS: a pixel that any sample of the cycle finds the nearer surface in stays close to it on
//      every frame of the cycle. (The first version kept the mean, and a thin wire then read as what is behind it: the
//      owner saw wires go thin and broken under the blur.)
//   3  INSIDE A SURFACE NOTHING IS TOUCHED: every pixel whose neighbourhood has never held an edge, flat or on a slope
//      below the edge threshold, is bit for bit what the scene drew, on every frame.
//   4  WHAT MOVES KEEPS ITS EDGE, AND LEAVES NO TRAIL. The shapes move three pixels a frame in front of a camera that
//      stands still, and the vectors say so: their edges, followed with them, are as steady as when still; and every
//      pixel with nothing but background around it NOW holds the background's depth exactly, in the same frame (a
//      mover's edges are written with its vector, so nothing of it is remembered).
//   5  A SEED CHANGES NOTHING: with a share of 1 the texture is bit for bit what the scene drew, whatever is held.
//   6  THE HOME, A COLOUR TEXTURE: it writes exactly the pixels that then differ from what the scene drew (its draw is
//      counted by an occlusion query, which is also how a development build counts what the pass did), and the
//      control counts every pixel of a depth picture and none of a blank one.
//   7  THE HOME, A DEPTH TEXTURE (INTZ): the same with the scene's own depth as what is read and written, and the
//      device's depth state is left as the common setup made it, the compare function as it was found.
//   8  A THING THAT GOES WHILE NOTHING MOVES IS LET GO: the shapes pop out of a still scene with no vector to say so;
//      what was remembered of their edges is gone within a bounded number of frames.
// NEGATIVE CONTROLS, each of which must FAIL the proof named: a share of 1 (nothing held: proof 1), an edge threshold
// wider than the gap (nothing is an edge: proof 1), another size's texel (proof 1), vectors of zero under motion (a
// mover is then remembered: a trail, proof 4) and vectors of the wrong sign (proof 4).
//
//     tools\test-steady\build.ps1             builds sd.exe (x86, as the game)
//     sd.exe --d3d9 <path> [--timing] [--explore]
//         <path>: DXVK's d3d9, a game folder's vulkan.dll.  --explore prints proof 1's numbers for other shares (how
//         the shipped one was chosen).
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
#include "../../source/shaders/ivt_steady_ps.h"
#include "../../source/shaders/ivt_steady_home_ps.h"
#include "../../source/shaders/ivt_steady_probe_ps.h"
#include "../../source/transport/steady_depth_pass.h"
#include "sd_load_ps.h"                        // the proof's own loader, compiled by build.ps1 into %TEMP%

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

// IEEE half, round to nearest even (tools/test-dedither's)
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
static uint32_t gRng = 0x12345678u;
static float Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0f; }

// ---- the scene ---------------------------------------------------------------------------------------------------
static const float kFar = 0.90f, kNear = 0.30f, kGap = kFar - kNear;
static float Halton(int i, int base) { float f = 1.0f, r = 0.0f; while (i > 0) { f /= (float)base; r += f * (float)(i % base); i /= base; } return r; }
// Which shape is at (x, y), in pixels of a w by h picture: 0 none, 1 the box, 2 the disc, 3 the thin wire, 4 the wide
// one. The shapes are placed by the picture's size and moved by (ox, oy).
static int Inside(float x, float y, int w, int h, float ox, float oy)
{
    x -= ox; y -= oy;
    const float W = (float)w, H = (float)h;
    {   // a box, turned 27 degrees
        const float cx = 0.30f * W + 0.37f, cy = 0.35f * H + 0.21f, hx = 0.11f * W, hy = 0.16f * H, a = 0.4712389f;
        const float dx = x - cx, dy = y - cy, u = dx * cosf(a) + dy * sinf(a), v = -dx * sinf(a) + dy * cosf(a);
        if (fabsf(u) < hx && fabsf(v) < hy) return 1;
    }
    {   // a disc
        const float cx = 0.68f * W + 0.13f, cy = 0.36f * H + 0.71f, r = 0.13f * H;
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < r * r) return 2;
    }
    for (int k = 0; k < 2; ++k)
    {   // two wires: 0.4 of a pixel wide, and 1.5
        const float x0 = 0.10f * W, y0 = (0.70f + 0.12f * (float)k) * H + 0.3f, x1 = 0.85f * W, y1 = (0.62f + 0.12f * (float)k) * H + 0.6f, half = k ? 0.75f : 0.2f;
        const float vx = x1 - x0, vy = y1 - y0; float t = ((x - x0) * vx + (y - y0) * vy) / (vx * vx + vy * vy); t = t < 0 ? 0 : t > 1 ? 1 : t;
        const float px = x0 + t * vx - x, py = y0 + t * vy - y;
        if (px * px + py * py < half * half) return 3 + k;
    }
    return 0;
}
struct Pic { int w = 0, h = 0; std::vector<float> d; std::vector<float> mvx, mvy; std::vector<uint8_t> shape; };
// One frame of the scene: every pixel sampled at its centre plus (jx, jy). `ramp`: the right-hand eighth is a slope.
static Pic Shot(int w, int h, float jx, float jy, float ox, float oy, float mvxObj)
{
    Pic p; p.w = w; p.h = h; p.d.resize((size_t)w * h); p.mvx.assign((size_t)w * h, 0.0f); p.mvy.assign((size_t)w * h, 0.0f); p.shape.assign((size_t)w * h, 0);
    const int rampX = w - w / 8;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    {
        const float sx = (float)x + 0.5f + jx, sy = (float)y + 0.5f + jy;
        const size_t i = (size_t)y * w + x;
        // a slope, well away from the shapes: a neighbourhood's range on it is 0.002, below the edge threshold
        if (x >= rampX) { p.d[i] = 0.40f + 0.0006f * std::min(sx - (float)rampX, 300.0f) + 0.0004f * std::min(sy, 600.0f); continue; }
        const int in = Inside(sx, sy, w, h, ox, oy);
        p.d[i] = in ? kNear : kFar; p.shape[i] = (uint8_t)in;
        if (in) p.mvx[i] = mvxObj;
    }
    return p;
}

// ---- the device ----------------------------------------------------------------------------------------------------
struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DPixelShader9 *ps = nullptr, *home = nullptr, *load = nullptr, *probe = nullptr;
    IDirect3DTexture9 *up = nullptr, *mv = nullptr;                      // uploads: the frame's depth as the scene draws it, its vectors
    IDirect3DTexture9 *t = nullptr; IDirect3DSurface9* tSurf = nullptr;  // the texture the post chain reads: a colour copy of the depth (R32F), as FusionFix's is
    IDirect3DTexture9 *z = nullptr; IDirect3DSurface9* zSurf = nullptr;  // or the scene's own depth (INTZ)
    IDirect3DTexture9 *h[2] = {}; IDirect3DSurface9* hs[2] = {};         // what the resolve holds: this frame's and the last frame's, in turn
    IDirect3DTexture9 *scr = nullptr; IDirect3DSurface9* scrSurf = nullptr;
    IDirect3DSurface9* sys = nullptr;
    IDirect3DQuery9* qFreq = nullptr; IDirect3DQuery9* qDis = nullptr;
    int w = 0, h_ = 0, cur = 0;
};
static const float kQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };
static const D3DFORMAT kIntz = (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');

static void Size(Dev& v, int w, int h)
{
    for (IUnknown* u : { (IUnknown*)v.up, (IUnknown*)v.mv, (IUnknown*)v.tSurf, (IUnknown*)v.t, (IUnknown*)v.zSurf, (IUnknown*)v.z, (IUnknown*)v.hs[0], (IUnknown*)v.h[0],
                         (IUnknown*)v.hs[1], (IUnknown*)v.h[1], (IUnknown*)v.scrSurf, (IUnknown*)v.scr, (IUnknown*)v.sys }) if (u) u->Release();
    v.w = w; v.h_ = h; v.cur = 0;
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &v.up, nullptr));
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_G16R16F, D3DPOOL_MANAGED, &v.mv, nullptr));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &v.t, nullptr));  CK(v.t->GetSurfaceLevel(0, &v.tSurf));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_DEPTHSTENCIL, kIntz, D3DPOOL_DEFAULT, &v.z, nullptr));        CK(v.z->GetSurfaceLevel(0, &v.zSurf));
    for (int i = 0; i < 2; ++i)
    {
        CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, D3DPOOL_DEFAULT, &v.h[i], nullptr)); CK(v.h[i]->GetSurfaceLevel(0, &v.hs[i]));
        v.d->SetRenderTarget(0, v.hs[i]); v.d->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);    // nothing held yet
    }
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &v.scr, nullptr)); CK(v.scr->GetSurfaceLevel(0, &v.scrSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, D3DFMT_R32F, D3DPOOL_SYSTEMMEM, &v.sys, nullptr));
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
static void Upload(Dev& v, const std::vector<float>& d)
{
    D3DLOCKED_RECT lr; CK(v.up->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < v.h_; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &d[(size_t)y * v.w], sizeof(float) * v.w);
    v.up->UnlockRect(0);
}
static void UploadMv(Dev& v, const Pic& p)
{
    D3DLOCKED_RECT lr; CK(v.mv->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < v.h_; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < v.w; ++x) { row[2 * x] = ToHalf(p.mvx[(size_t)y * v.w + x]); row[2 * x + 1] = ToHalf(p.mvy[(size_t)y * v.w + x]); } }
    v.mv->UnlockRect(0);
}
static std::vector<float> Read(Dev& v, IDirect3DSurface9* s)
{
    CK(v.d->GetRenderTargetData(s, v.sys));
    std::vector<float> out((size_t)v.w * v.h_); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h_; ++y) memcpy(&out[(size_t)y * v.w], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(float) * v.w);
    v.sys->UnlockRect(); return out;
}
// The proof's own loader (sd_load_ps.hlsl) draws texture `from` into a colour target, or into the INTZ as depth.
static void Blit(Dev& v, IDirect3DTexture9* from, IDirect3DSurface9* colour, bool toDepth)
{
    Common(v);
    const float c0[4] = { 1.0f / (float)v.w, 1.0f / (float)v.h_, 0.0f, 0.0f };
    const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h_, 0.0f, 1.0f };
    v.d->SetRenderTarget(0, colour); v.d->SetViewport(&vp);
    v.d->SetTexture(0, from); v.d->SetPixelShaderConstantF(0, c0, 1); v.d->SetPixelShader(v.load);
    if (toDepth)
    {
        v.d->SetDepthStencilSurface(v.zSurf);
        v.d->SetRenderState(D3DRS_ZENABLE, TRUE); v.d->SetRenderState(D3DRS_ZWRITEENABLE, TRUE); v.d->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    }
    CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
    v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F); v.d->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    v.d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE); v.d->SetRenderState(D3DRS_ZENABLE, FALSE);
    v.d->SetDepthStencilSurface(nullptr); v.d->SetTexture(0, nullptr);
}
// THE SCENE WRITES THE DEPTH THE POST CHAIN WILL READ: FusionFix's blit into its colour copy, or the scene's own depth.
static void SceneWrites(Dev& v, bool depthKind) { Blit(v, v.up, depthKind ? v.scrSurf : v.tSurf, depthKind); }
// WHAT THE POST CHAIN READS NOW: the colour copy itself, or the INTZ through a sampler.
static std::vector<float> Result(Dev& v, bool depthKind)
{
    if (!depthKind) return Read(v, v.tSurf);
    Blit(v, v.z, v.scrSurf, false);
    return Read(v, v.scrSurf);
}
// ONE FRAME AS THE MOD DRAWS IT: the resolve, then the home unless the frame seeds. `wrongTexel`: the control, another
// size's texel. Afterwards the texture the post chain reads holds the steadied depth.
static float gDecay = 0.0f;       // SdDecay of the cycle the run is for (set by each run)
static void Mod(Dev& v, bool depthKind, float share, float edge, bool camStill, bool wrongTexel = false)
{
    Common(v);
    IDirect3DTexture9* const now = depthKind ? v.z : v.t;
    if (!wrongTexel) CK(SdResolve(v.d, now, v.h[v.cur ^ 1], v.mv, v.hs[v.cur], v.ps, (UINT)v.w, (UINT)v.h_, share, edge, gDecay, camStill, kQuad, sizeof(kQuad[0])));
    else
    {   // SdResolve's own calls with the texel of another size and this size's viewport
        const float c[2][4] = { { 2.0f / (float)v.w, 2.0f / (float)v.h_, share, edge }, { camStill ? 1.0f : 0.0f, kSdStillPx * kSdStillPx, gDecay, 0.0f } };
        const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h_, 0.0f, 1.0f };
        v.d->SetRenderTarget(0, v.hs[v.cur]); v.d->SetViewport(&vp);
        v.d->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
        v.d->SetTexture(0, now); v.d->SetTexture(1, v.h[v.cur ^ 1]); v.d->SetTexture(2, v.mv);
        v.d->SetPixelShaderConstantF(0, &c[0][0], 2); v.d->SetPixelShader(v.ps);
        CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
    }
    if (share < 1.0f)
        CK(SdHome(v.d, v.h[v.cur], depthKind ? nullptr : v.tSurf, depthKind ? v.zSurf : nullptr, v.hs[v.cur ^ 1], v.home, (UINT)v.w, (UINT)v.h_, kQuad, sizeof(kQuad[0])));
    v.d->SetRenderTarget(0, v.scrSurf);
    v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); v.d->SetTexture(2, nullptr);
    v.cur ^= 1;
}

// ---- proofs 1 to 3: a still scene -----------------------------------------------------------------------------------
// The pixels by what crosses them: [0] a solid edge (the box, the disc), [1] the wide wire, [2] the thin wire.
//   hit    pixels that at least one sample of the cycle finds the nearer surface in, and at least one the farther
//   swing  the most any of them moves over the cycle; step: from one frame to the next
//   away   the furthest any of them is from the surface that wins, at its worst in the cycle
//   inSwing  what goes in: 1 where the pixel's own sample flips by the whole gap
struct Still { long hit[3] = {}; double swing[3] = {}, step[3] = {}, away[3] = {}, inSwing = 0;
               long calm = 0, calmWrong = 0, slope = 0, slopeWrong = 0;
               long own = 0, ownWrong = 0; };     // pixels beside an edge whose own samples never change, and those of them that were changed
// Runs the still scene until what is held has settled (64 frames at least, whole cycles) and measures the next cycle.
// `wrong`: 0 none, 1 a share of 1, 2 another size's texel.
static Still RunStill(Dev& v, int phases, float share, float edge, bool depthKind, int wrong, bool camStill = true)
{
    const int w = v.w, h = v.h_; const size_t n = (size_t)w * h;
    gDecay = SdDecay(phases);
    const int settle = std::max(3, (64 + phases - 1) / phases);
    std::vector<float> lo(n, 9.0f), hi(n, -9.0f), prev, inLo(n, 9.0f), inHi(n, -9.0f), stepMax(n, 0.0f);
    std::vector<uint8_t> differs(n, 0);
    std::vector<uint8_t> seen(n, 0), everEdge(n, 0);              // which shapes the cycle's samples of this pixel hit; was its neighbourhood ever not flat
    Still r;
    const float tol = depthKind ? 1.0f / 8388608.0f : 0.0f;       // the INTZ holds 24 bits
    const int rampX = w - w / 8;
    for (int f = 0; f < (settle + 1) * phases; ++f)
    {
        const int ph = f % phases;
        const Pic p = Shot(w, h, Halton(ph + 1, 2) - 0.5f, Halton(ph + 1, 3) - 0.5f, 0.0f, 0.0f, 0.0f);
        Upload(v, p.d); if (f == 0) UploadMv(v, p);
        SceneWrites(v, depthKind);
        const float sh = (f == 0) ? 1.0f : (wrong == 1 ? 1.0f : share);
        Mod(v, depthKind, sh, edge, camStill, wrong == 2);
        const bool measure = f >= settle * phases;
        std::vector<float> o;
        if (measure || f == settle * phases - 1) o = Result(v, depthKind);
        for (int y = 1; y < h - 1; ++y) for (int x = 1; x < w - 1; ++x)
        {
            const size_t i = (size_t)y * w + x;
            bool flat = true; float nlo = p.d[i], nhi = p.d[i];
            for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx)
            { const float q = p.d[(size_t)(y + dy) * w + (x + dx)]; if (q != p.d[i]) flat = false; nlo = std::min(nlo, q); nhi = std::max(nhi, q); }
            if (nhi - nlo > edge) everEdge[i] = 1;
            if (!measure) continue;
            lo[i] = std::min(lo[i], o[i]); hi[i] = std::max(hi[i], o[i]);
            inLo[i] = std::min(inLo[i], p.d[i]); inHi[i] = std::max(inHi[i], p.d[i]);
            seen[i] |= (uint8_t)(p.shape[i] ? 1u << p.shape[i] : 0u);
            stepMax[i] = std::max(stepMax[i], fabsf(o[i] - prev[i]));
            if (fabsf(o[i] - p.d[i]) > tol) differs[i] = 1;
            // proof 3: a pixel whose neighbourhood has never held an edge leaves as it came: flat, or on the slope
            if (everEdge[i]) continue;
            if (flat) { r.calm++; if (fabsf(o[i] - p.d[i]) > tol) r.calmWrong++; }
            else if (x > rampX + 1) { r.slope++; if (fabsf(o[i] - p.d[i]) > tol) r.slopeWrong++; }
        }
        if (!o.empty()) prev = o;
    }
    for (int y = 1; y < h - 1; ++y) for (int x = 1; x < rampX - 2; ++x)
    {
        const size_t i = (size_t)y * w + x;
        // BESIDE AN EDGE, NOT ON IT: every sample of the cycle finds the same surface in this pixel, so there is nothing
        // of its own to steady and it must leave as it came (a pass that spread the winning surface by a pixel fails here)
        if (everEdge[i] && inHi[i] == inLo[i]) { r.own++; if (differs[i]) r.ownWrong++; }
        if (!seen[i] || inHi[i] - inLo[i] < 0.5f * kGap) continue;   // the pixels an edge crosses: the input flips by the whole gap
        const int c = (seen[i] & (1u << 3)) ? 2 : (seen[i] & (1u << 4)) ? 1 : 0;
        r.hit[c]++;
        r.inSwing = std::max(r.inSwing, (double)(inHi[i] - inLo[i]) / kGap);
        r.swing[c] = std::max(r.swing[c], (double)(hi[i] - lo[i]) / kGap);
        r.step[c]  = std::max(r.step[c], (double)stepMax[i] / kGap);
        r.away[c]  = std::max(r.away[c], (double)(hi[i] - kNear) / kGap);
    }
    return r;
}

// ---- proof 4: what moves ----------------------------------------------------------------------------------------------
struct Moving { long edge[3] = {}; double swing[3] = {}; long trail = 0; long behind = 0; };
// The shapes move `speed` pixels a frame to the right in front of a camera that stands still. `mvMode`: 0 the true
// vectors, 1 zero, 2 the wrong sign.
static Moving RunMoving(Dev& v, int phases, float share, float edge, int mvMode)
{
    const int w = v.w, h = v.h_; const size_t n = (size_t)w * h; const int speed = 3;
    gDecay = SdDecay(phases);
    const int first = std::max(2, (48 + phases - 1) / phases) * phases, frames = first + phases;
    std::vector<float> lo(n, 9.0f), hi(n, -9.0f), inLo(n, 9.0f), inHi(n, -9.0f);
    std::vector<uint8_t> seen(n, 0);
    Moving r;
    const int rampX = w - w / 8, lastX = rampX - 2 - speed * frames;      // the shapes' own pixels that stay left of the slope for the whole run
    for (int f = 0; f < frames; ++f)
    {
        const int ph = f % phases; const float ox = (float)(speed * f);
        Pic p = Shot(w, h, Halton(ph + 1, 2) - 0.5f, Halton(ph + 1, 3) - 0.5f, ox, 0.0f, (float)-speed);
        if (mvMode == 1) std::fill(p.mvx.begin(), p.mvx.end(), 0.0f);
        if (mvMode == 2) for (float& m : p.mvx) m = -m;
        Upload(v, p.d); UploadMv(v, p);
        SceneWrites(v, false);
        Mod(v, false, f == 0 ? 1.0f : share, edge, true);
        if (f < first) continue;
        const std::vector<float> o = Result(v, false);
        for (int y = 2; y < h - 2; ++y) for (int x = 2; x < rampX - 2; ++x)
        {
            const size_t i = (size_t)y * w + x;
            // NO TRAIL: a pixel whose eight neighbours and itself are background in this frame's depth holds the background
            bool bg = true;
            for (int dy = -1; dy <= 1 && bg; ++dy) for (int dx = -1; dx <= 1; ++dx) if (p.d[(size_t)(y + dy) * w + (x + dx)] != kFar) { bg = false; break; }
            if (bg) { r.behind++; if (o[i] != kFar) r.trail++; }
            // THE EDGE, FOLLOWED: the pixel as it stood before the shapes moved at all
            const int sx = x - speed * f;
            if (sx < 2 || sx >= lastX) continue;
            const size_t k = (size_t)y * w + sx;
            lo[k] = std::min(lo[k], o[i]); hi[k] = std::max(hi[k], o[i]);
            inLo[k] = std::min(inLo[k], p.d[i]); inHi[k] = std::max(inHi[k], p.d[i]);
            seen[k] |= (uint8_t)(p.shape[i] ? 1u << p.shape[i] : 0u);
        }
    }
    for (size_t k = 0; k < n; ++k)
    {
        if (inHi[k] - inLo[k] < 0.5f * kGap) continue;
        const int c = (seen[k] & (1u << 3)) ? 2 : (seen[k] & (1u << 4)) ? 1 : 0;
        r.edge[c]++; r.swing[c] = std::max(r.swing[c], (double)(hi[k] - lo[k]) / kGap);
    }
    return r;
}

// ---- proof 8: a thing that goes while nothing moves ---------------------------------------------------------------
// The still scene, settled; then the shapes are gone (no vector says so: a thing that pops out). Returns how many
// frames pass until no pixel is further than a tenth of the gap from the background, or -1 if it never happens.
static int RunVanish(Dev& v, int phases, float share, float edge, int limit)
{
    const int w = v.w, h = v.h_;
    gDecay = SdDecay(phases);
    for (int f = 0; f < 8 * phases; ++f)
    {
        const Pic p = Shot(w, h, Halton(f % phases + 1, 2) - 0.5f, Halton(f % phases + 1, 3) - 0.5f, 0.0f, 0.0f, 0.0f);
        Upload(v, p.d); if (f == 0) UploadMv(v, p);
        SceneWrites(v, false);
        Mod(v, false, f == 0 ? 1.0f : share, edge, true);
    }
    for (int f = 0; f < limit; ++f)
    {
        const Pic p = Shot(w, h, Halton(f % phases + 1, 2) - 0.5f, Halton(f % phases + 1, 3) - 0.5f, -100000.0f, 0.0f, 0.0f);   // the shapes far off the picture
        Upload(v, p.d);
        SceneWrites(v, false);
        Mod(v, false, share, edge, true);
        const std::vector<float> o = Result(v, false);
        bool gone = true; const int rampX = w - w / 8;
        for (int y = 1; y < h - 1 && gone; ++y) for (int x = 1; x < rampX - 2; ++x) if (o[(size_t)y * w + x] < kFar - 0.1f * kGap) { gone = false; break; }
        if (gone) return f + 1;
    }
    return -1;
}

// WHAT THE PROOFS HOLD THE SHIPPED CONSTANTS TO (transport/steady_depth_pass.h has them), in parts of the gap between
// the two surfaces, for [0] a solid edge, [1] a wire a pixel and a half wide, [2] a wire 0.4 of a pixel wide: the most a
// pixel may move over a whole cycle, from one frame to the next, and how far it may be from the surface that wins at
// its worst. kSdMove: the first of these for a solid edge and the wide wire that move.
static const double kSdSwing[3] = { 0.15, 0.15, 0.15 }, kSdStep[3] = { 0.15, 0.15, 0.15 }, kSdAway[3] = { 0.15, 0.15, 0.15 };
static const double kSdMove[2] = { 0.15, 0.15 };
static const int kSdGone = 20;                           // proof 8: frames until a thing that popped out is let go
static bool StillOk(const Still& s)
{
    for (int c = 0; c < 3; ++c)
        if (s.hit[c] < 20 || s.swing[c] > kSdSwing[c] || s.step[c] > kSdStep[c] || s.away[c] > kSdAway[c]) return false;
    return true;
}

static void Proofs(Dev& v, int w, int h, bool full)
{
    Size(v, w, h);
    printf(" at %dx%d\n", w, h);
    const float edge = kSdEdgeLog;
    const int cycles[3] = { 8, 18, 32 };
    for (int ci = 0; ci < (full ? 3 : 1); ++ci)
    {
        const int N = cycles[ci]; const float share = SdShare(N);
        const Still s = RunStill(v, N, share, edge, false, 0);
        Proof(s.inSwing > 0.99 && StillOk(s) && s.own > 500 && s.ownWrong == 0,
              "1  a still scene, a cycle of %d: going in, every pixel an edge crosses flips by the whole gap. Coming out, the most any moves over the cycle / from one frame to the next, in parts of the gap:"
              " a solid edge %.3f / %.3f (%ld pixels; allowed %.2f / %.2f), a wire 1.5 pixels wide %.3f / %.3f (%ld; %.2f / %.2f), a wire 0.4 wide %.3f / %.3f (%ld; %.2f / %.2f)."
              " Beside the edges, %ld pixels whose own samples never change leave as they came (%ld do not)",
              N, s.swing[0], s.step[0], s.hit[0], kSdSwing[0], kSdStep[0], s.swing[1], s.step[1], s.hit[1], kSdSwing[1], kSdStep[1], s.swing[2], s.step[2], s.hit[2], kSdSwing[2], kSdStep[2], s.own, s.ownWrong);
        Proof(s.away[0] <= kSdAway[0] && s.away[1] <= kSdAway[1] && s.away[2] <= kSdAway[2],
              "2  the nearer surface wins: a pixel that any sample of the cycle finds it in is never further from it than: a solid edge %.3f of the gap (allowed %.2f), the wide wire %.3f (%.2f), the thin wire %.3f (%.2f)",
              s.away[0], kSdAway[0], s.away[1], kSdAway[1], s.away[2], kSdAway[2]);
        Proof(s.calm > 1000 && s.calmWrong == 0 && s.slope > 1000 && s.slopeWrong == 0,
              "3  inside a surface: %ld pixel-frames whose neighbourhood never held an edge are bit for bit what the scene drew (%ld are not), and %ld on a slope below the edge threshold (%ld are not)",
              s.calm, s.calmWrong, s.slope, s.slopeWrong);
        if (ci == 0)
        {
            const Still c1 = RunStill(v, N, share, edge, false, 1);
            Control(c1.swing[0] > kSdSwing[0] && c1.swing[1] > kSdSwing[1], "a share of 1, nothing held: a solid edge moves by %.3f of the gap, the wide wire by %.3f", c1.swing[0], c1.swing[1]);
            const Still c3 = RunStill(v, N, share, 1.0f, false, 0);
            Control(c3.swing[0] > kSdSwing[0] && c3.swing[1] > kSdSwing[1], "an edge threshold wider than the gap, nothing is an edge: a solid edge moves by %.3f of the gap, the wide wire by %.3f", c3.swing[0], c3.swing[1]);
            const Still c2 = RunStill(v, N, share, edge, false, 2);
            Control(!StillOk(c2), "another size's texel: a solid edge moves by %.3f of the gap and is as far as %.3f from the nearer surface", c2.swing[0], c2.away[0]);
            // WHILE THE CAMERA MOVES nothing is remembered on a flat frame: pinned, so that it is known and seen if it changes
            const Still m = RunStill(v, N, share, edge, false, 0, false);
            Proof(m.swing[2] > 0.9 && m.swing[1] <= kSdSwing[1], "1  the same with the camera said to be moving (nothing remembered on a flat frame): the thin wire flips by %.3f of the gap, as before this pass existed (PINNED: it is not held then);"
                  " the wide wire moves by %.3f, a solid edge by %.3f (the tip of a corner is not held either)", m.swing[2], m.swing[1], m.swing[0]);
        }
    }
    const float share = SdShare(8);
    gDecay = SdDecay(8);
    {   // proof 4
        const Moving m = RunMoving(v, 8, share, edge, 0);
        Proof(m.edge[0] > 100 && m.edge[1] > 20 && m.swing[0] <= kSdMove[0] && m.swing[1] <= kSdMove[1] && m.trail == 0,
              "4  moving three pixels a frame, the camera still: followed, a solid edge moves by at most %.3f of the gap over a cycle (%ld pixels; allowed %.2f), the wide wire %.3f (%ld; %.2f), the thin wire %.3f (%ld; not held: what it is written with is its vector);"
              " behind them %ld of %ld background pixel-frames hold anything but the background",
              m.swing[0], m.edge[0], kSdMove[0], m.swing[1], m.edge[1], kSdMove[1], m.swing[2], m.edge[2], m.trail, m.behind);
        const Moving z = RunMoving(v, 8, share, edge, 1);
        Control(z.trail > 0, "vectors of zero under motion, so what a mover leaves is remembered: %ld background pixel-frames behind it hold something else (a trail); a solid edge moves by %.3f of the gap", z.trail, z.swing[0]);
        const Moving n = RunMoving(v, 8, share, edge, 2);
        Control(n.swing[0] > kSdMove[0], "vectors of the wrong sign: a solid edge moves by %.3f of the gap", n.swing[0]);
    }
    {   // proof 5: a seed over a history of noise changes nothing
        std::vector<float> noise((size_t)w * h), junk((size_t)w * h);
        for (float& f : noise) f = Rnd();
        for (float& f : junk) f = Rnd();
        Upload(v, junk); SceneWrites(v, false); Mod(v, false, 1.0f, edge, true); Mod(v, false, share, edge, true);     // what is held now holds junk, and flags
        Upload(v, noise); SceneWrites(v, false);
        Mod(v, false, 1.0f, edge, true);
        const std::vector<float> o = Result(v, false);
        long bad = 0; for (size_t i = 0; i < o.size(); ++i) if (memcmp(&o[i], &noise[i], 4) != 0) bad++;
        Proof(bad == 0, "5  a seed (a share of 1) over held junk: the texture the post chain reads is bit for bit what the scene drew (%ld pixels are not, of %ld)", bad, (long)o.size());
    }
    for (int kind = 0; kind < 2; ++kind)
    {   // proofs 6 and 7: the home, a colour texture and a depth texture, on one settled frame of the still scene
        const bool depthKind = kind == 1;
        Pic p;
        for (int f = 0; f < 23; ++f)
        {
            p = Shot(w, h, Halton(f % 8 + 1, 2) - 0.5f, Halton(f % 8 + 1, 3) - 0.5f, 0.0f, 0.0f, 0.0f);
            Upload(v, p.d); if (f == 0) UploadMv(v, p);
            SceneWrites(v, depthKind);
            Mod(v, depthKind, f == 0 ? 1.0f : share, edge, true);
        }
        // the last frame by hand: the home inside an occlusion query, and what the texture held before and after it
        p = Shot(w, h, Halton(8, 2) - 0.5f, Halton(8, 3) - 0.5f, 0.0f, 0.0f, 0.0f);
        Upload(v, p.d); SceneWrites(v, depthKind);
        const std::vector<float> before = Result(v, depthKind);
        Common(v);
        IDirect3DTexture9* const now = depthKind ? v.z : v.t;
        CK(SdResolve(v.d, now, v.h[v.cur ^ 1], v.mv, v.hs[v.cur], v.ps, (UINT)w, (UINT)h, share, edge, gDecay, true, kQuad, sizeof(kQuad[0])));
        v.d->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);           // something the depth home must put back as it found it
        IDirect3DQuery9* q = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q));
        CK(q->Issue(D3DISSUE_BEGIN));
        CK(SdHome(v.d, v.h[v.cur], depthKind ? nullptr : v.tSurf, depthKind ? v.zSurf : nullptr, v.hs[v.cur ^ 1], v.home, (UINT)w, (UINT)h, kQuad, sizeof(kQuad[0])));
        CK(q->Issue(D3DISSUE_END));
        DWORD homed = 0; while (q->GetData(&homed, sizeof(homed), D3DGETDATA_FLUSH) == S_FALSE) {}
        q->Release();
        DWORD ze = 9, zw = 9, zf = 9, cw = 9; IDirect3DSurface9* ds = nullptr;
        v.d->GetRenderState(D3DRS_ZENABLE, &ze); v.d->GetRenderState(D3DRS_ZWRITEENABLE, &zw); v.d->GetRenderState(D3DRS_ZFUNC, &zf); v.d->GetRenderState(D3DRS_COLORWRITEENABLE, &cw);
        v.d->GetDepthStencilSurface(&ds); if (ds) ds->Release();
        v.d->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        // the control's count, as the mod draws it
        IDirect3DQuery9* qc = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_OCCLUSION, &qc));
        DWORD ctl = 0, blank = 0;
        for (int k = 0; k < 2; ++k)
        {
            Common(v); v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
            CK(qc->Issue(D3DISSUE_BEGIN));
            CK(SdControl(v.d, k ? nullptr : now, v.hs[v.cur ^ 1], v.probe, (UINT)w, (UINT)h, kQuad, sizeof(kQuad[0])));
            CK(qc->Issue(D3DISSUE_END));
            while (qc->GetData(k ? &blank : &ctl, sizeof(DWORD), D3DGETDATA_FLUSH) == S_FALSE) {}
            v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
        }
        qc->Release();
        v.d->SetRenderTarget(0, v.scrSurf); v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); v.d->SetTexture(2, nullptr);
        v.cur ^= 1;
        const std::vector<float> after = Result(v, depthKind);
        long changed = 0, wild = 0;
        for (size_t i = 0; i < after.size(); ++i)
        {
            if (after[i] != before[i]) changed++;
            if (after[i] < kNear - 0.001f || after[i] > kFar + 0.001f) wild++;
        }
        if (!depthKind)
        {
            Proof(changed > 0 && (long)homed == changed && wild == 0, "6  the home, a colour texture: it writes %lu pixels, and %ld pixels of the texture the post chain reads differ from what the scene drew (every other one is bit for bit the scene's; %ld outside the two surfaces)",
                  (unsigned long)homed, changed, wild);
            Proof((long)ctl == (long)after.size() && blank == 0, "6  the control: %lu of %ld pixels of the depth texture hold a depth, %lu of a blank one", (unsigned long)ctl, (long)after.size(), (unsigned long)blank);
        }
        else
        {
            Proof(changed > 0 && (long)homed >= changed && wild == 0, "7  the home, a depth texture (INTZ): it writes %lu pixels as depth, and %ld pixels read through a sampler differ from what the scene drew (%ld outside the two surfaces)",
                  (unsigned long)homed, changed, wild);
            Proof(ze == FALSE && zw == FALSE && zf == D3DCMP_GREATER && cw == 0x0F && ds == nullptr, "7  the device after it: no depth test, no depth write, the compare function as found, colour writes on, no depth-stencil bound (%lu %lu %lu 0x%lX %p)",
                  (unsigned long)ze, (unsigned long)zw, (unsigned long)zf, (unsigned long)cw, (void*)ds);
        }
    }
    {   // proof 7, the rest: the scene's own depth texture as what the resolve reads and writes
        const Pic p = Shot(w, h, 0.21f, -0.13f, 0.0f, 0.0f, 0.0f);
        Upload(v, p.d); UploadMv(v, p); SceneWrites(v, true);
        const std::vector<float> o = Result(v, true);
        long bad = 0; double worst = 0;
        for (size_t i = 0; i < o.size(); ++i) { const double e = fabs((double)o[i] - (double)p.d[i]); worst = std::max(worst, e); if (e > 1.0 / 16777215.0) bad++; }
        Proof(bad == 0, "7  an INTZ depth texture read through a sampler is the picture drawn into it, to its 24 bits (%ld pixels are not, the furthest by %.2e)", bad, worst);
        const Still s = RunStill(v, 8, share, edge, true, 0);
        Proof(StillOk(s) && s.calm > 1000 && s.calmWrong == 0 && s.slopeWrong == 0,
              "7  the INTZ as the depth the resolve reads and writes, a cycle of 8: a solid edge moves by at most %.3f of the gap over the cycle, the wide wire %.3f, the thin wire %.3f; %ld calm pixel-frames and %ld on the slope are what the scene drew to 24 bits (%ld and %ld are not)",
              s.swing[0], s.swing[1], s.swing[2], s.calm, s.slope, s.calmWrong, s.slopeWrong);
    }
    if (full)
    {   // proof 8: a thing that goes while nothing moves is let go
        const int gone = RunVanish(v, 8, share, edge, 2000);
        Proof(gone > 0 && gone <= kSdGone, "8  the shapes pop out of a still scene (no vector says so): after %d frame(s) no pixel is further than a tenth of the gap from the background (allowed %d: two cycles and a little). Until then their outline reads nearer than it is", gone, kSdGone);
    }
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; bool timing = false, explore = false;
    for (int i = 1; i < argc; ++i) { if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i]; else if (!strcmp(argv[i], "--timing")) timing = true; else if (!strcmp(argv[i], "--explore")) explore = true; }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The steady depth's proof: the shipping shaders and draws against depth pictures of known answer, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "sdproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("sdproof", "sd", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtSteadyPS, &v.ps));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtSteadyHomePS, &v.home));
    CK(v.d->CreatePixelShader((const DWORD*)kSdLoadPS, &v.load));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtSteadyProbePS, &v.probe));
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &v.qFreq); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &v.qDis);

    if (explore)
    {   // how the shipped share was chosen: proof 1's numbers for other shares, as 1 over (k times the cycle)
        Size(v, 427, 240);
        printf(" EXPLORE at 427x240, in parts of the gap. Each cell: the most a pixel moves over a cycle / from one frame to the next / its farthest from the nearer surface, for a solid edge | the wide wire | the thin wire\n");
        const int ks[4] = { 2, 4, 8, 16 }; const int cycles[3] = { 8, 18, 32 };
        for (int k : ks)
        {
            printf("   a share of 1 over %d cycles%s:", k, k == 8 ? " (what ships)" : "");
            for (int N : cycles) { const Still s = RunStill(v, N, 1.0f / (float)(k * N), kSdEdgeLog, false, 0);
                printf("\n      cycle %2d: %.3f / %.3f / %.3f | %.3f / %.3f / %.3f | %.3f / %.3f / %.3f", N, s.swing[0], s.step[0], s.away[0], s.swing[1], s.step[1], s.away[1], s.swing[2], s.step[2], s.away[2]); }
            const Moving mv = RunMoving(v, 8, 1.0f / (float)(k * 8), kSdEdgeLog, 0);
            const int gone = RunVanish(v, 8, 1.0f / (float)(k * 8), kSdEdgeLog, 4000);
            printf("\n      moving, a cycle of 8: %.3f | %.3f | %.3f (trail %ld);  shapes that pop out are let go after %d frames\n", mv.swing[0], mv.swing[1], mv.swing[2], mv.trail, gone);
        }
    }

    Proofs(v, 427, 240, true);
    Proofs(v, 1707, 960, false);

    const int kSizes[4][2] = { { 3840, 2160 }, { 2560, 1440 }, { 1920, 1080 }, { 1280, 720 } };
    if (timing) for (int si = 0; si < 4; ++si)
    {   // the two draws' own GPU time by this card's timestamps, 20 frames a sample, the median of 30, at four render sizes
        const int TW = kSizes[si][0], TH = kSizes[si][1];
        Size(v, TW, TH);
        const Pic p = Shot(TW, TH, 0.21f, -0.13f, 0.0f, 0.0f, 0.0f);
        std::vector<float> noise((size_t)TW * TH); for (float& f : noise) f = Rnd();
        long edges = 0;                                            // the scene's pixels that count as an edge
        for (int y = 1; y < TH - 1; ++y) for (int x = 1; x < TW - 1; ++x)
        {
            float lo = 9.0f, hi = -9.0f;
            for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) { const float q = p.d[(size_t)(y + dy) * TW + (x + dx)]; lo = std::min(lo, q); hi = std::max(hi, q); }
            if (hi - lo > kSdEdgeLog) edges++;
        }
        UploadMv(v, p);
        // what: 0 the scene, the camera still (every flat pixel asks what is remembered); 1 the scene, the camera moving (a flat
        // pixel asks nothing); 2 noise, EVERY pixel an edge (the most it can cost); 3 the scene with the INTZ read and written;
        // 4 a seed (the resolve alone, nothing read from the frame before, no home)
        auto Time = [&](int what) -> double {
            IDirect3DQuery9 *q0 = nullptr, *q1 = nullptr; v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &q1);
            if (!q0 || !q1 || !v.qDis || !v.qFreq) return -1.0;
            v.qDis->Issue(D3DISSUE_BEGIN); q0->Issue(D3DISSUE_END);
            gDecay = SdDecay(8);
            for (int i = 0; i < 20; ++i) Mod(v, what == 3, what == 4 ? 1.0f : SdShare(8), kSdEdgeLog, what != 1);
            q1->Issue(D3DISSUE_END); v.qDis->Issue(D3DISSUE_END); v.qFreq->Issue(D3DISSUE_END);
            UINT64 a0 = 0, a1 = 0, f = 0; BOOL dis = TRUE;
            while (q1->GetData(&a1, sizeof(a1), D3DGETDATA_FLUSH) == S_FALSE) {}
            q0->GetData(&a0, sizeof(a0), D3DGETDATA_FLUSH); v.qFreq->GetData(&f, sizeof(f), D3DGETDATA_FLUSH); v.qDis->GetData(&dis, sizeof(dis), D3DGETDATA_FLUSH);
            q0->Release(); q1->Release();
            return (dis || !f) ? -1.0 : (double)(a1 - a0) * 1000.0 / (double)f / 20.0; };
        auto Median = [&](int what) -> double { std::vector<double> s; for (int i = 0; i < 5; ++i) Time(what); for (int i = 0; i < 30; ++i) { const double t = Time(what); if (t > 0) s.push_back(t); }
            std::sort(s.begin(), s.end()); return s.empty() ? -1.0 : s[s.size() / 2]; };
        Upload(v, p.d); SceneWrites(v, false); SceneWrites(v, true);
        const double still = Median(0), moving = Median(1), intz = Median(3), seed = Median(4);
        Upload(v, noise); SceneWrites(v, false);
        const double worst = Median(2);
        printf("  TIMING at %dx%d on this card's own clock (ms a frame, the resolve and the home): the scene with the camera still %.4f, with the camera moving %.4f (%.1f percent of its pixels are edges), with the INTZ read and written %.4f;"
               " every pixel an edge (noise, the most it can cost) %.4f; a seed %.4f; its two targets hold %.1f MiB\n",
               TW, TH, still, moving, 100.0 * (double)edges / ((double)TW * TH), intz, worst, seed, 2.0 * 4.0 * (double)TW * (double)TH / 1048576.0);
    }
    printf("%s\n", gFails ? "THE STEADY DEPTH'S PROOF: FAIL" : "THE STEADY DEPTH'S PROOF: PASS");
    return gFails ? 1 : 0;
}
