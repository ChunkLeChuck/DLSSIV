// G10'S PICTURE PROOF (step 5, 2026-09-29): the HDR encode that sharpens at its own sample (hdr_encode_g10_ps.h and
// hdr_encode_cga_g10_ps.h, hdr_encode_ps.hlsl under HDR_G10) against the pipeline it replaces, on the GPU, pixel for pixel,
// with no game. G9's harness (tools/test-hdr-skip/g9.cpp) is the pattern: the same frame, states, draws and pass rule.
//
// THE PIPELINE IT REPLACES, drawn with the shipping shaders: the frame before the HUD (B) is sharpened by the shipping RCAS
// (kRcasPS) into a full-precision target, S; the pre-HUD copy is S; the HUD then draws over part of it, F; the shipping encode
// (kHdrEncodePS, or kHdrEncodeCgaPS under the absorb) encodes F against S. G10: the pre-HUD copy is B, the HUD draws over B
// (F'), and the G10 encode encodes F' against B, sharpening at its sample and comparing the UNSHARPENED sample for the mask.
// The target is FP32, so the stored S carries every bit RCAS computed: the two paths differ only by how fxc scheduled the two
// shaders' arithmetic, not by an FP16 rounding the game's own pass would add (G10 skips that rounding; it is not what is proved).
//
// THE PROOF, in every state (G9's decodes, roll-off, restoration, the HUD mask, a menu, console gamma, source 1, the LUT path,
// the absorbed encode with its dither), at three sharpening strengths and both ceilings:
//   1  AWAY FROM THE HUD (every pixel whose five RCAS taps miss it) G10's output equals the pipeline's within 64 float32 steps,
//      NaN where it has NaN, and its FP16 picture differs only by single FP16 steps, as often as those float32 steps can cross
//      an FP16 rounding edge (G9's derivation). The 64 was set before the first run, for two separately compiled RCAS feeding
//      the encode's power curve; the plain encode's states came out BIT-EXACT (0 steps), so it is slack, and every negative
//      control lands at tens of millions of steps. THE ABSORBED STATES are held to their own COMPILE NOISE instead, measured
//      first in the same state: G10 at strength 0 (RCAS then returns its centre exactly) against the shipping absorbed encode
//      on the same frame, where only the two shaders' scheduling differs. The Xbox curve comes out a few float32 steps apart
//      in the two absorbed compiles (the PS3 curve exactly), and source 1's restoration magnifies it to hundreds; the sharpened
//      comparison may be no worse than twice that noise, single FP16 steps only. (Measured 2026-09-29: noise 44, 557, 530 and
//      35 steps against G10's 44, 699, 616 and 45.)
//   2  THE MASK IS EXACT: on every pixel the HUD did not touch (its neighbours included, whose sharpened value the HUD's edge
//      moves), G10's mask acts exactly where the shipping encode's mask acts on the same frame without sharpening. AT UI LEVEL
//      0.15, NOT 1 (review of 6fb08430): at level 1 the mask cannot change a pixel below white (HdrShip multiplies by
//      lerp(1, level, coverage)), so the first version proved it on bright pixels only.
//   3  AND ON THE HUD TOO, where the mask matters: G10 at strength 0 against the shipping encode on the same frame, mask on at
//      0.15, over EVERY pixel: bit-exact on the plain encode and on the absorbed PS3 curve.
// ON THE HUD the sharpened pictures differ ON PURPOSE (G10 sharpens the HUD; the owner judges that in the game): printed.
// THE REFERENCE KEEPS S AT FULL PRECISION where the game's back buffer is FP16, so "bit-exact" is against that idealised pipeline:
// G10 also skips the half-FP16-step rounding of the sharpened frame, which is not measured here.
//
// SEVEN NEGATIVE CONTROLS must FAIL: the encode without sharpening against the pipeline (1), plain and absorbed; a G10 whose
// ceiling is always 1 (1, where values between white and the ceiling meet an edge); a G10 that reads its attenuation from the
// ceiling's lane (1); a G10 whose mask compares the SHARPENED sample (2, plain and absorbed); an absorbed G10 that puts the curve
// on the centre before sharpening (1, under the absorbed rule, which the plain controls cannot exercise).
//
//     tools\test-hdr-sharpen\build.ps1        builds g10.exe (x86, as the game) and the five wrong encodes (%TEMP%\g10gen)
//     g10.exe --d3d9 <path>                   <path>: DXVK's d3d9, the game folder's vulkan.dll (native D3D9 refuses this draw,
//                                             as it does G9's)
// Exit 0 when every proof passed. It changes nothing on the machine.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdarg>
#include <limits>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "../../source/shaders/hdr_encode_ps.h"
#include "../../source/shaders/hdr_encode_cga_ps.h"
#include "../../source/shaders/hdr_encode_g10_ps.h"
#include "../../source/shaders/hdr_encode_cga_g10_ps.h"
#include "../../source/shaders/rcas_ps.h"
#include "../../source/shaders/ivt_fs_vs.h"
#include "../../source/hdr_xbox_gamma.h"
#include "g10_wrong_mask.h"            // build.ps1 generates these in %TEMP%\g10gen: the five wrong encodes (negative controls)
#include "g10_wrong_maskcga.h"
#include "g10_wrong_ceiling.h"
#include "g10_wrong_lanes.h"
#include "g10_wrong_order.h"
#include "g10_oldline.h"               // the shipping encode before the Xbox curve's fix (check 5's control), plain
#include "g10_oldlinecga.h"            // and absorbed
#include "hdr_encode_rline_ps.h"       // the encode before the restoration's switch (build.ps1 generates it; B3 switch 14's control
#include "hdr_encode_cga_rline_ps.h"   // until Stage F): 567f1bba's encode, the mask fixed and the exact ratio on the old line (checks 5, 6)

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
#define CK(x) do { HRESULT hr_ = (x); if (FAILED(hr_)) { printf("  FAIL  %s: 0x%08lX (line %d)\n", #x, (unsigned long)hr_, __LINE__); exit(2); } } while (0)

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
static const int W = 1024, H = 512;
// THE HUD: rows 128 to 255 of the left quarter, where the frame after the HUD differs from the frame before it (G9's region).
static bool InHud(int x, int y) { return y >= 128 && y < 256 && x < W / 4; }
static bool NearHud(int x, int y) { return y >= 127 && y <= 256 && x <= W / 4; }   // on it, or one of its RCAS taps is

// THE FRAME: G9's bands (gradients, emitters, noise below and across white, warps, the specials: zero, negatives, 65504, +Inf,
// NaN), which put edges at every level, above white included.
static std::vector<Px> MakeFrame(int w, int h)
{
    std::vector<Px> f((size_t)w * h);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    {
        Px& p = f[(size_t)y * w + x]; const float u = (x + 0.5f) / w;
        const int band = y / 64;
        switch (band)
        {
        case 0: p = { u, (y % 64) / 63.0f, 0.5f }; break;
        case 1: p = (y % 2) ? Px{ 4.0f * u, 0.05f, 0.05f } : Px{ 3.0f * u, 3.0f * u, 3.0f * u }; break;
        case 2: case 3: p = { Rnd(), Rnd(), Rnd() }; break;
        case 4: case 5: p = { 2.0f * Rnd(), 2.0f * Rnd(), 2.0f * Rnd() }; break;
        case 6:
        {
            p = { 0.9f * Rnd(), 0.9f * Rnd(), 0.9f * Rnd() };
            const int bx = x / 8, by = (y % 64) / 4;
            if (((bx + by) & 1) && (x % 8) == 3 && (y % 4) == 2) p.r = 1.5f;
            break;
        }
        default:
        {
            const int k = x / 64; const float kn = powf(0.85f + 0.10f * (x % 64) / 63.0f, 1.0f / 2.2f);
            const float inf = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
            const Px sp[16] = { {1, 1, 1}, {FromHalf(0x3C01), 0.5f, 0.5f}, {0.9995f, 0.9995f, 0.9995f}, {0, 0, 0},
                                {-0.001f, 0.2f, 0.2f}, {100, 100, 100}, {65504, 1, 1}, {inf, 0.5f, 0.5f},
                                {nan, 0.5f, 0.5f}, {kn, kn * 0.5f, kn * 0.2f}, {kn, kn, kn}, {0.5f, kn, 0.1f},
                                {1, 0, 0}, {FromHalf(0x3C01), FromHalf(0x3C01), FromHalf(0x3C01)}, {0.02f, 0.03f, 0.04f}, {1.2f, 0.9f, 0.3f} };
            p = sp[k & 15];
            break;
        }
        }
    }
    return f;
}

struct Dev
{
    IDirect3DDevice9* d = nullptr;
    IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DTexture9* tex[12] = {};
    IDirect3DTexture9* rt32 = nullptr; IDirect3DTexture9* rt16 = nullptr;
    IDirect3DSurface9* sys32 = nullptr; IDirect3DSurface9* sys16 = nullptr;
    std::vector<Px> after, before;        // the frame after the HUD (F', s0) and before it (B, s11), as FP16 holds them
    int w = 0, h = 0;
};

static IDirect3DTexture9* MakeHalfTex(IDirect3DDevice9* d, int w, int h, const std::vector<Px>& px)
{
    IDirect3DTexture9* t = nullptr; CK(d->CreateTexture(w, h, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_MANAGED, &t, nullptr));
    D3DLOCKED_RECT lr; CK(t->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < h; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < w; ++x) { const Px& p = px[(size_t)y * w + x]; row[4 * x] = ToHalf(p.r); row[4 * x + 1] = ToHalf(p.g); row[4 * x + 2] = ToHalf(p.b); row[4 * x + 3] = ToHalf(1.0f); } }
    t->UnlockRect(0); return t;
}
static IDirect3DTexture9* MakeF32Tex(IDirect3DDevice9* d, int w, int h, const std::vector<float>& rgba)
{
    IDirect3DTexture9* t = nullptr; CK(d->CreateTexture(w, h, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &t, nullptr));
    D3DLOCKED_RECT lr; CK(t->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < h; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &rgba[(size_t)y * w * 4], sizeof(float) * 4 * w);
    t->UnlockRect(0); return t;
}
static IDirect3DTexture9* MakeR32Tex(IDirect3DDevice9* d, int w, const float* v)
{
    IDirect3DTexture9* t = nullptr; CK(d->CreateTexture(w, 1, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &t, nullptr));
    D3DLOCKED_RECT lr; CK(t->LockRect(0, &lr, nullptr, 0)); memcpy(lr.pBits, v, sizeof(float) * w); t->UnlockRect(0); return t;
}

static void Setup(Dev& v, IDirect3D9* d3d, HWND hw, int w, int h)
{
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    IDirect3DDevice9* d = v.d; v.w = w; v.h = h;
    CK(d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.vs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(d->CreateVertexDeclaration(el, &v.decl));
    std::vector<Px> frame = MakeFrame(w, h), scene((size_t)w * h), pre = frame, bloom((size_t)(w / 4) * (h / 4));
    for (auto& p : scene) p = { 6.0f * Rnd(), 6.0f * Rnd(), 6.0f * Rnd() };
    for (auto& p : bloom) p = { Rnd(), Rnd(), Rnd() };
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) if (InHud(x, y)) { Px& p = pre[(size_t)y * w + x]; p = { p.r * 0.3f, p.g * 0.3f, p.b * 0.3f }; }   // under the HUD, before it drew
    auto rounded = [](std::vector<Px> f) { for (auto& p : f) p = { FromHalf(ToHalf(p.r)), FromHalf(ToHalf(p.g)), FromHalf(ToHalf(p.b)) }; return f; };
    v.after = rounded(frame); v.before = rounded(pre);
    v.tex[0] = MakeHalfTex(d, w, h, frame);
    v.tex[1] = MakeHalfTex(d, w, h, scene);
    const float lum = 0.25f; v.tex[2] = MakeR32Tex(d, 1, &lum);
    v.tex[3] = MakeHalfTex(d, w / 4, h / 4, bloom);
    {
        CK(d->CreateTexture(1024, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &v.tex[4], nullptr));
        D3DLOCKED_RECT lr; CK(v.tex[4]->LockRect(0, &lr, nullptr, 0));
        for (int y = 0; y < 32; ++y) { DWORD* row = (DWORD*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
            for (int x = 0; x < 1024; ++x) { const DWORD r = (x % 32) * 255 / 31, g = y * 255 / 31, b = (x / 32) * 255 / 31; row[x] = 0xFF000000u | (r << 16) | (g << 8) | b; } }
        v.tex[4]->UnlockRect(0);
    }
    v.tex[9] = MakeR32Tex(d, 256, kHdrXboxGamma);
    float prof[256]; for (int i = 0; i < 256; ++i) prof[i] = powf(i / 255.0f, 0.8f); v.tex[10] = MakeR32Tex(d, 256, prof);
    v.tex[11] = MakeHalfTex(d, w, h, pre);
    CK(d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &v.rt32, nullptr));
    CK(d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &v.rt16, nullptr));
    CK(d->CreateOffscreenPlainSurface(w, h, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM, &v.sys32, nullptr));
    CK(d->CreateOffscreenPlainSurface(w, h, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &v.sys16, nullptr));
}

struct State { char name[96]; float c[9][4]; bool cga; float att; bool aw; };
static const float kIvtQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };

static void Draw(Dev& v, IDirect3DPixelShader9* ps, const float (&cIn)[9][4], IDirect3DTexture9* rt, IDirect3DTexture9* const (&tx)[12])
{
    IDirect3DDevice9* d = v.d; IDirect3DSurface9* surf = nullptr; CK(rt->GetSurfaceLevel(0, &surf));
    CK(d->SetRenderTarget(0, surf)); surf->Release();
    D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f }; d->SetViewport(&vp);
    d->SetRenderState(D3DRS_ZENABLE, FALSE); d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); d->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF); d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    for (int i = 0; i < 12; ++i)
    {
        d->SetTexture(i, tx[i]);
        if (!tx[i]) continue;
        d->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_POINT); d->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        d->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP); d->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, FALSE);
    }
    d->SetVertexShader(v.vs); d->SetVertexDeclaration(v.decl); d->SetPixelShader(ps);
    d->SetPixelShaderConstantF(0, &cIn[0][0], 9);
    CK(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kIvtQuad, sizeof(kIvtQuad[0])));
}
template <class T> static std::vector<T> Read(Dev& v, IDirect3DTexture9* rt, IDirect3DSurface9* sys)
{
    IDirect3DSurface9* surf = nullptr; CK(rt->GetSurfaceLevel(0, &surf)); CK(v.d->GetRenderTargetData(surf, sys)); surf->Release();
    std::vector<T> out((size_t)v.w * v.h * 4); D3DLOCKED_RECT lr; CK(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; ++y) memcpy(&out[(size_t)y * v.w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(T) * v.w * 4);
    sys->UnlockRect(); return out;
}
struct Out { std::vector<float> f32; std::vector<uint16_t> f16; };
static Out Render(Dev& v, IDirect3DPixelShader9* ps, const float (&c)[9][4], IDirect3DTexture9* const (&tx)[12])
{
    Out o;
    Draw(v, ps, c, v.rt32, tx); o.f32 = Read<float>(v, v.rt32, v.sys32);
    Draw(v, ps, c, v.rt16, tx); o.f16 = Read<uint16_t>(v, v.rt16, v.sys16);
    return o;
}
static int64_t Ulps(float a, float b)
{
    int32_t x, y; memcpy(&x, &a, 4); memcpy(&y, &b, 4);
    if (x < 0) x = (int32_t)0x80000000 - x; if (y < 0) y = (int32_t)0x80000000 - y;
    return std::llabs((int64_t)x - (int64_t)y);
}
struct Cmp { int64_t maxUlp = 0; size_t diff32 = 0, nanMismatch = 0, diff16 = 0, n = 0; int maxHalfStep = 0; float worstA = 0, worstB = 0; int wx = 0, wy = 0; };
template <class Sel> static Cmp Compare(const Dev& v, const Out& A, const Out& B, Sel sel)
{
    Cmp r;
    for (int y = 0; y < v.h; ++y) for (int x = 0; x < v.w; ++x)
    {
        if (!sel(x, y)) continue;
        const size_t i = (size_t)y * v.w + x; r.n++;
        bool d32 = false, d16 = false;
        for (int c = 0; c < 3; ++c)
        {
            const float a = A.f32[4 * i + c], b = B.f32[4 * i + c];
            if (std::isnan(a) || std::isnan(b)) { if (std::isnan(a) != std::isnan(b)) { r.nanMismatch++; d32 = true; } }
            else { const int64_t u = Ulps(a, b); if (u) d32 = true; if (u > r.maxUlp) { r.maxUlp = u; r.worstA = a; r.worstB = b; r.wx = x; r.wy = y; } }
            const uint16_t ha = A.f16[4 * i + c], hb = B.f16[4 * i + c];
            if (ha != hb)
            {
                d16 = true;
                const bool na = (ha & 0x7C00u) == 0x7C00u && (ha & 0x3FFu), nb = (hb & 0x7C00u) == 0x7C00u && (hb & 0x3FFu);
                if (!(na && nb)) r.maxHalfStep = std::max(r.maxHalfStep, (int)std::abs((int)(ha & 0x7FFFu) - (int)(hb & 0x7FFFu)) + (((ha ^ hb) & 0x8000u) ? 1000 : 0));
                else d16 = false;
            }
        }
        r.diff32 += d32; r.diff16 += d16;
    }
    return r;
}
// THE PASS RULE: G9's, with the float32 bound this file's header argues (64 steps), the FP16 flips derived from it as G9 does.
static const int64_t kMaxUlp = 64;
static bool Within(const Cmp& r, int64_t maxUlp = kMaxUlp)
{
    const double expect = 3.0 * (double)r.diff32 * 2.0 * (double)r.maxUlp / 8192.0 * 3.0;
    return r.maxUlp <= maxUlp && r.nanMismatch == 0 && (double)r.diff16 <= std::max(10.0, expect) && r.maxHalfStep <= 1;
}
static const char* Tell(const Cmp& r)
{
    static char b[320];
    _snprintf_s(b, sizeof(b), _TRUNCATE, "%zu pixels: FP32 %zu differ, at most %lld step(s); NaN mismatches %zu; FP16 %zu differ, at most %d step(s)",
                r.n, r.diff32, (long long)r.maxUlp, r.nanMismatch, r.diff16, r.maxHalfStep);
    return b;
}

static void Base(State& s, const char* name, bool cga, float att, bool aw)
{
    memset(&s, 0, sizeof(s)); strncpy_s(s.name, name, _TRUNCATE); s.cga = cga; s.att = att; s.aw = aw;
    s.c[0][0] = 203.0f; s.c[0][1] = 800.0f; s.c[0][2] = 2.2f; s.c[0][3] = 1.0f;
    s.c[1][0] = 0.90f;  s.c[1][1] = 0.0f;
    s.c[1][2] = 1.0f / W; s.c[1][3] = 1.0f / H;
    s.c[2][0] = 0.80f;  s.c[2][3] = 0.0f;
    s.c[3][0] = 1.0f; s.c[3][1] = 1.2f; s.c[3][2] = 0.25f; s.c[3][3] = 0.8f;
    s.c[4][0] = 1.0f; s.c[4][1] = 1.0f; s.c[4][2] = 1.0f; s.c[4][3] = 0.0f;
    s.c[5][0] = 0.5f; s.c[5][1] = 0.5f; s.c[5][2] = 0.5f; s.c[5][3] = 1.0f;
    s.c[6][0] = 0.2f; s.c[6][1] = 0.1f; s.c[6][2] = 0.05f; s.c[6][3] = 0.0f;
    s.c[7][0] = 1.0f; s.c[7][1] = 1.0f; s.c[7][2] = 1920.0f; s.c[7][3] = 1080.0f;
    s.c[8][0] = 1.0f; s.c[8][1] = kHdrXboxGammaTopSlope; s.c[8][2] = 0.0f; s.c[8][3] = 0.0f;
}
// ShpCeiling (DLSS-IV.cpp), the CPU's side: six times white in the buffer's encoding by the decode in c0.z, or 1.
static float Ceiling(const State& s)
{
    if (!s.aw) return 1.0f;
    const float z = s.c[0][2];
    return z == 0.0f ? 1.055f * powf(6.0f, 1.0f / 2.4f) - 0.055f : powf(6.0f, 1.0f / z);
}

struct Shaders { IDirect3DPixelShader9 *enc, *encC, *g10, *g10C, *rcas, *wMask, *wMaskC, *wCeil, *wLanes, *wOrder; };

// The pipeline G10 replaces, for one state: S = RCAS(B) at full precision; F = the HUD over S; the shipping encode of F
// against S.
static Out Pipeline(Dev& v, const Shaders& sh, const State& s, IDirect3DPixelShader9* encOverride = nullptr)
{
    float rc[9][4] = {}; rc[0][0] = s.att; rc[0][1] = Ceiling(s); rc[0][2] = 1.0f / v.w; rc[0][3] = 1.0f / v.h;
    IDirect3DTexture9* t0[12] = {}; t0[0] = v.tex[11];
    Draw(v, sh.rcas, rc, v.rt32, t0);
    const std::vector<float> S = Read<float>(v, v.rt32, v.sys32);
    std::vector<float> F = S;
    for (int y = 0; y < v.h; ++y) for (int x = 0; x < v.w; ++x) if (InHud(x, y))
    { const size_t i = (size_t)y * v.w + x; const Px& p = v.after[i]; F[4 * i] = p.r; F[4 * i + 1] = p.g; F[4 * i + 2] = p.b; F[4 * i + 3] = 1.0f; }
    IDirect3DTexture9* tF = MakeF32Tex(v.d, v.w, v.h, F); IDirect3DTexture9* tS = MakeF32Tex(v.d, v.w, v.h, S);
    IDirect3DTexture9* tx[12]; memcpy(tx, v.tex, sizeof(tx)); tx[0] = tF; tx[11] = tS;
    const Out o = Render(v, encOverride ? encOverride : (s.cga ? sh.encC : sh.enc), s.c, tx);
    tF->Release(); tS->Release();
    return o;
}
static void G10Const(float (&c)[9][4], const State& s) { c[2][1] = Ceiling(s); c[2][2] = s.att; }   // hdr_encode.h's two lanes
static Out G10(Dev& v, IDirect3DPixelShader9* ps, const State& s, float uiOn)
{
    float c[9][4]; memcpy(c, s.c, sizeof(c));
    G10Const(c, s);
    c[1][1] = uiOn;
    return Render(v, ps, c, v.tex);
}
static Out Ui(Dev& v, IDirect3DPixelShader9* ps, const State& s, float uiOn)   // the shipping encode on the frame as G10 sees it
{
    float c[9][4]; memcpy(c, s.c, sizeof(c));
    c[1][1] = uiOn;
    return Render(v, ps, c, v.tex);
}
// WHERE A MASK ACTS: the pixels whose output with the mask on differs from the output with it off (beyond 4 float32 steps, or
// NaN on one side only). Under G10 the mask compares the unsharpened sample, so on the pixels the HUD did not touch it must
// act exactly where the shipping encode's mask acts on the same frame without any sharpening.
static std::vector<uint8_t> Acts(const Dev& v, const Out& on, const Out& off)
{
    std::vector<uint8_t> a((size_t)v.w * v.h, 0);
    for (size_t i = 0; i < a.size(); ++i) for (int c = 0; c < 3; ++c)
    {
        const float x = on.f32[4 * i + c], y = off.f32[4 * i + c];
        if (std::isnan(x) || std::isnan(y)) { if (std::isnan(x) != std::isnan(y)) a[i] = 1; }
        else if (Ulps(x, y) > 4) a[i] = 1;
    }
    return a;
}

int main(int argc, char** argv)
{
    const char* dll = nullptr;
    for (int i = 1; i < argc; ++i) if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i];
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("G10's picture proof: the HDR encode that sharpens at its sample against the sharpen pass then the encode, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "g10proof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("g10proof", "g10", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v; Setup(v, d3d, hw, W, H);
    Shaders sh = {};
    CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodePS, &sh.enc));      CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeCgaPS, &sh.encC));
    CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeG10PS, &sh.g10));   CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeCgaG10PS, &sh.g10C));
    CK(v.d->CreatePixelShader((const DWORD*)kRcasPS, &sh.rcas));
    CK(v.d->CreatePixelShader((const DWORD*)kG10WrongMaskPS, &sh.wMask)); CK(v.d->CreatePixelShader((const DWORD*)kG10WrongMaskCgaPS, &sh.wMaskC));
    CK(v.d->CreatePixelShader((const DWORD*)kG10WrongCeilingPS, &sh.wCeil)); CK(v.d->CreatePixelShader((const DWORD*)kG10WrongLanesPS, &sh.wLanes));
    CK(v.d->CreatePixelShader((const DWORD*)kG10WrongOrderPS, &sh.wOrder));
    auto away = [](int x, int y) { return !NearHud(x, y); };
    auto untouched = [](int x, int y) { return !InHud(x, y); };
    auto onHud = [](int x, int y) { return InHud(x, y); };
    // THE COMPILE NOISE IS READ WHERE RCAS AT STRENGTH 0 RETURNS ITS CENTRE UNCHANGED: every tap finite and not negative.
    // Beside a +Inf texel RCAS computes 0 x Inf = NaN at any strength, and it clamps a negative channel to 0 (the shipping
    // sharpen pass does both, which is why the pipeline and G10 agree there exactly); the shipping encode the noise is measured
    // against has no RCAS, so those few pixels would read as noise of any size and make the limit vacuous. The first two runs
    // read 47, then 17 FP16 steps, and 864 million float32 steps where the dither met the clamped -0.001 near zero.
    std::vector<uint8_t> finiteTaps((size_t)W * H, 1);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
    {
        const int tx[5] = { x, x, x - 1, x + 1, x }, ty[5] = { y, y - 1, y, y, y + 1 };
        for (int k = 0; k < 5; ++k)
        {
            const Px& p = v.after[(size_t)std::min(std::max(ty[k], 0), H - 1) * W + std::min(std::max(tx[k], 0), W - 1)];
            if (!std::isfinite(p.r) || !std::isfinite(p.g) || !std::isfinite(p.b) || p.r < 0.0f || p.g < 0.0f || p.b < 0.0f) finiteTaps[(size_t)y * W + x] = 0;
        }
    }
    auto awayFinite = [&](int x, int y) { return !NearHud(x, y) && finiteTaps[(size_t)y * W + x]; };

    std::vector<State> st; State s;
    Base(s, "source 0, 2.2, sharpness 1, above white shown", false, 1.0f, true); st.push_back(s);
    Base(s, "source 0, 2.2, sharpness 0.5, ceiling 1", false, 0.5f, false); st.push_back(s);
    Base(s, "source 0, 2.2, sharpness 0 (attenuation 0.25), above white", false, 0.25f, true); st.push_back(s);
    Base(s, "source 0, the sRGB decode, above white shown", false, 1.0f, true); s.c[0][2] = 0.0f; st.push_back(s);
    Base(s, "source 0, 2.4, hard clip, above white shown", false, 1.0f, true); s.c[0][2] = 2.4f; s.c[2][0] = 0.0f; st.push_back(s);
    Base(s, "source 0, restoration off, ceiling 1", false, 1.0f, false); s.c[0][3] = 0.0f; st.push_back(s);
    // THE MASK ON, NOT ABSORBED: console gamma none. HdrUiCoverage puts FusionFix's curve on the pre-HUD copy before comparing,
    // because in the game FusionFix has drawn it into the frame by then; this harness's frame carries no curve, so with a curve
    // named the mask would act on every pixel in both paths (the first run's failure here was that, not the shader).
    Base(s, "source 0, the HUD mask on (level 1)", false, 1.0f, true); s.c[1][1] = 1.0f; s.c[8][0] = 0.0f; st.push_back(s);
    Base(s, "source 0, a game menu (level -1)", false, 1.0f, true); s.c[1][1] = -1.0f; st.push_back(s);
    Base(s, "source 0, console gamma PS3", false, 1.0f, true); s.c[8][0] = 2.0f; st.push_back(s);
    Base(s, "source 1, the exact ratio", false, 1.0f, false); s.c[2][3] = 1.0f; st.push_back(s);
    Base(s, "source 1, the LUT path", false, 1.0f, false); s.c[2][3] = 1.0f; s.c[6][3] = 1.0f; s.c[8][2] = 0.5f; s.c[8][3] = 1.2f; st.push_back(s);
    Base(s, "absorbed: the Xbox curve, dither on", true, 1.0f, true); st.push_back(s);
    Base(s, "absorbed: the PS3 curve, dither off, the HUD mask on", true, 0.5f, true); s.c[7][0] = 2.0f; s.c[7][1] = 0.0f; s.c[8][0] = 2.0f; s.c[1][1] = 1.0f; st.push_back(s);
    Base(s, "absorbed: source 1, ceiling 1", true, 1.0f, false); s.c[2][3] = 1.0f; st.push_back(s);
    Base(s, "absorbed: source 1, ceiling 1, dither off", true, 1.0f, false); s.c[2][3] = 1.0f; s.c[7][1] = 0.0f; st.push_back(s);
    Base(s, "absorbed: the Xbox curve, dither off", true, 1.0f, true); s.c[7][1] = 0.0f; st.push_back(s);

    // THE ABSORBED ENCODE'S RULE (the first runs: the Xbox curve came out a few float32 steps apart in the two absorbed compiles,
    // the PS3 curve exact, and source 1's restoration magnifies it). At strength 0 G10's RCAS returns its centre exactly (lobe 0,
    // 1 / (4 * 0 + 1) = 1), so G10 against the shipping absorbed encode on the same frame differs by how fxc scheduled the two
    // shaders' shared arithmetic and nothing else. The sharpened comparison may be no worse than twice that, and single FP16
    // steps only. That the noise is only scheduling is the explanation, not a proof; what bounds it is the absorbed negative
    // controls below, which this rule must refuse.
    auto absorbedOk = [&](const State& t, const Cmp& r, bool say) -> bool {
        State z = t; z.att = 0.0f;
        const Cmp noise = Compare(v, Ui(v, sh.encC, t, t.c[1][1]), G10(v, sh.g10C, z, t.c[1][1]), awayFinite);
        const int64_t lim = std::max<int64_t>(kMaxUlp, 2 * noise.maxUlp);
        const double flips = std::max(10.0, 2.0 * (double)noise.diff16);
        if (say) printf("        its compile noise (strength 0 against the shipping absorbed encode, finite taps): %s; the limit here: %lld steps, %.0f FP16 flips\n",
                        Tell(noise), (long long)lim, flips);
        return r.maxUlp <= lim && r.nanMismatch == 0 && r.maxHalfStep <= 1 && (double)r.diff16 <= flips; };
    printf("  1: away from the HUD, G10 against the pipeline it replaces (the rule: %lld float32 steps, G9's FP16 rule; absorbed\n"
           "     states: no worse than twice their own COMPILE NOISE, measured first and printed)\n", (long long)kMaxUlp);
    for (const State& t : st)
    {
        const Out ref = Pipeline(v, sh, t);
        const Out g = G10(v, t.cga ? sh.g10C : sh.g10, t, t.c[1][1]);
        const Cmp r = Compare(v, ref, g, away);
        // THE ABSORBED ENCODE'S COMPILE NOISE (the first runs: the Xbox curve came out a few float32 steps apart in the two
        // absorbed compiles, the PS3 curve exact, and source 1's restoration magnifies it). At strength 0 G10's RCAS returns its
        // centre exactly (lobe 0, 1 / (4 * 0 + 1) = 1), so G10 against the shipping absorbed encode on the same frame differs by
        // how fxc scheduled the two shaders' shared arithmetic and nothing else. The sharpened comparison may be no worse than
        // twice that, and single FP16 steps only.
        const bool ok = t.cga ? absorbedOk(t, r, true) : Within(r);
        Proof(ok, "%-58s %s", t.name, Tell(r));
        if (r.maxUlp) printf("        the largest: pipeline %.9g, G10 %.9g at (%d, %d)\n", r.worstA, r.worstB, r.wx, r.wy);
        const Cmp hud = Compare(v, ref, g, onHud);
        printf("        on the HUD (sharpened by G10 on purpose, not held): %zu of %zu pixels differ in the FP16 picture\n", hud.diff16, hud.n);
    }

    // Not a plain frame with a console gamma named: in the game FusionFix has drawn its curve into such a frame before the
    // encode, and this harness's frame carries none, so both masks would act on nearly every pixel for a reason the game never
    // has (the first run measured that, and where a mask acts also depends on the sample it restores, which G10 sharpens).
    // The absorbed encode applies the curve itself, so there the model is the game's.
    const float kLevel = 0.15f;       // the UI level the mask checks run at: below 1, so the mask moves sub-white pixels too
    // THE REFERENCE: the shipping encode on the same frame, where the two compiles agree exactly (plain; the PS3 curve). Under the
    // Xbox curve the absorbed compiles differ by a few float32 steps (check 1's compile noise), which moved 2 of 18,013 pixels
    // across the mask's threshold on the first run at level 0.15; there the reference is G10 ITSELF AT STRENGTH 0: the same
    // compiled shader, so the comparison isolates what sharpening does to the mask (nothing, if the mask reads sM), and check 3
    // ties strength 0 to the shipping encode where the compiles agree.
    printf("  2: the mask is exact: on every pixel the HUD did not touch, G10's mask acts exactly where the reference mask acts on\n"
           "     the same frame without sharpening, at UI level %.2f (the shipping encode: plain, console gamma none, and absorbed,\n"
           "     the PS3 curve; G10 itself at strength 0: absorbed, the Xbox curve with dither)\n", kLevel);
    struct MaskCase { const char* name; bool cga; float gamma; float dither; bool selfRef; };
    const MaskCase mc[3] = { { "plain, console gamma none", false, 0.0f, 0.0f, false },
                             { "absorbed, the Xbox curve, dither on", true, 1.0f, 1.0f, true }, { "absorbed, the PS3 curve, dither off", true, 2.0f, 0.0f, false } };
    auto maskMismatch = [&](IDirect3DPixelShader9* g, const MaskCase& k, size_t* refActs) -> size_t {
        State t; Base(t, "mask", k.cga, 1.0f, true); t.c[8][0] = k.gamma; t.c[7][0] = k.gamma; t.c[7][1] = k.dither;
        const std::vector<uint8_t> ag = Acts(v, G10(v, g, t, kLevel), G10(v, g, t, 0.0f));
        std::vector<uint8_t> ar;
        if (k.selfRef) { State z = t; z.att = 0.0f; ar = Acts(v, G10(v, k.cga ? sh.g10C : sh.g10, z, kLevel), G10(v, k.cga ? sh.g10C : sh.g10, z, 0.0f)); }
        else { IDirect3DPixelShader9* enc = k.cga ? sh.encC : sh.enc; ar = Acts(v, Ui(v, enc, t, kLevel), Ui(v, enc, t, 0.0f)); }
        size_t mis = 0, n = 0, band[8] = {};
        for (int y = 0; y < v.h; ++y) for (int x = 0; x < v.w; ++x) if (!InHud(x, y)) { const size_t i = (size_t)y * v.w + x; n += ar[i]; band[y / 64] += ar[i]; mis += ag[i] != ar[i]; }
        if (refActs)
        {
            *refActs = n;
            if (n) printf("        where the reference mask acts with no HUD there, by band of the frame (0 gradients, 1 emitters, 2-3 noise,"
                          " 4-5 noise across white, 6 warps, 7 specials): %zu %zu %zu %zu %zu %zu %zu %zu\n",
                          band[0], band[1], band[2], band[3], band[4], band[5], band[6], band[7]);
        }
        return mis; };
    for (const MaskCase& k : mc)
    {
        size_t acts = 0; const size_t mis = maskMismatch(k.cga ? sh.g10C : sh.g10, k, &acts);
        Proof(mis == 0, "%-38s the shipping mask acts on %zu untouched pixels; G10's differs from it on %zu", k.name, acts, mis);
    }
    {
        // AND THE MASK DOES ACT ON THE HUD: there, mask on against off must differ (the comparison can see the mask).
        State a; Base(a, "mask", false, 1.0f, true); a.c[8][0] = 0.0f;
        const Cmp r = Compare(v, G10(v, sh.g10, a, kLevel), G10(v, sh.g10, a, 0.0f), onHud);
        Proof(r.diff32 > 20000, "CONTROL: on the HUD the mask changes %zu of %zu pixels (it is live in this harness)", r.diff32, r.n);
    }
    printf("  3: the mask on the HUD itself: G10 at strength 0 against the shipping encode on the same frame, mask on at %.2f,\n"
           "     every pixel whose taps are finite and not negative, the HUD included\n", kLevel);
    {
        auto all = [&](int x, int y) { return finiteTaps[(size_t)y * W + x] != 0; };   // RCAS at strength 0 returns its centre there
        State a; Base(a, "hud", false, 0.0f, true); a.c[8][0] = 0.0f;
        Cmp r = Compare(v, Ui(v, sh.enc, a, kLevel), G10(v, sh.g10, a, kLevel), all);
        Proof(r.diff32 == 0 && r.nanMismatch == 0 && r.diff16 == 0, "plain, console gamma none:           %s", Tell(r));
        State b; Base(b, "hud", true, 0.0f, true); b.c[7][0] = 2.0f; b.c[7][1] = 0.0f; b.c[8][0] = 2.0f;
        r = Compare(v, Ui(v, sh.encC, b, kLevel), G10(v, sh.g10C, b, kLevel), all);
        Proof(r.diff32 == 0 && r.nanMismatch == 0 && r.diff16 == 0, "absorbed, the PS3 curve, dither off:  %s", Tell(r));
    }

    // 4. THE ABSORBED MASK VIEW: DROPPED AT STAGE F (2026-09-30). The view (hdr_debug_cga_ps: hdr_view=14 keeping the absorb in
    // development builds) was built for sitting 19's M2 lead, which was answered and fixed (DECISIONS 2026-09-29, "THE XBOX
    // CURVE'S MASK LEAD SEEN IN THE GAME AND FIXED" and "SITTING 20 READ"); it left the source with the strip and its check
    // went with it. Both are at the tag: git show step5-instruments:src/DLSS-IV/tools/test-hdr-sharpen/g10.cpp.
    printf("  4: DROPPED at Stage F (2026-09-30): the absorbed mask view it checked left the source with the strip (its lead was\n"
           "     answered and fixed, DECISIONS 2026-09-29); the check and the view are at the tag step5-instruments\n");

    // 5. THE XBOX 360 CURVE ABOVE WHITE (the fix, 2026-09-29): the mask's model of FusionFix's Xbox curve was a straight line
    // above white and is FusionFix's own formula now (hdr_common.hlsli, ConsoleGammaExact). Against the encode as it was (built
    // from the same source with the line put back, build.ps1): (a) under the absorbed Xbox curve the mask no longer acts on the
    // untouched pixels it used to, and the old encode still does (the check can see the fault); (b), THE MASK FIX ALONE (the
    // encode with the mask fixed and the exact ratio still on the line, generated by build.ps1) against the old one, nothing
    // else moved: in
    // every state of check 1 without the Xbox curve the two encodes agree bit for bit on every pixel, mask on and off; under it
    // they agree on every untouched pixel where NEITHER mask acts (the first run compared where the old mask did not act, and the
    // plain Xbox states, a frame with no curve on it under a model of one, which the game never has, put pixels over the dead
    // zone for the new formula that the line had left just under it: the mask's decision, not anything else, moved there).
    // "NEITHER ACTS" IS Acts()'s 4 float32 steps, so under the Xbox curve a pixel either mask moves by up to 4 steps counts as
    // untouched and may differ by that much between the two: the rule there is at most 4 float32 steps and no half-float step
    // (the second run: 65 pixels of 49,407 at most 4 steps, none in FP16; with the fix confined to the mask, 63 at most 4 steps,
    // 30 of them one half-float step: values on a rounding boundary, where 4 float32 steps flip the half). Without the Xbox curve
    // the rule stays bit-exact.
    printf("  5: the Xbox 360 curve above white is FusionFix's formula in the mask (the fix), against the encode as it was\n");
    {
        IDirect3DPixelShader9 *oldP = nullptr, *oldC = nullptr, *rlP = nullptr, *rlC = nullptr;
        CK(v.d->CreatePixelShader((const DWORD*)kOldLinePS, &oldP)); CK(v.d->CreatePixelShader((const DWORD*)kOldLineCgaPS, &oldC));
        CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeRLinePS, &rlP)); CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeCgaRLinePS, &rlC));
        for (float dither : { 1.0f, 0.0f })
        {
            State t; Base(t, "fix", true, 1.0f, true); t.c[8][0] = 1.0f; t.c[7][0] = 1.0f; t.c[7][1] = dither;
            const std::vector<uint8_t> aNew = Acts(v, Ui(v, sh.encC, t, kLevel), Ui(v, sh.encC, t, 0.0f));
            const std::vector<uint8_t> aOld = Acts(v, Ui(v, oldC, t, kLevel), Ui(v, oldC, t, 0.0f));
            size_t nNew = 0, nOld = 0;
            for (int y = 0; y < v.h; ++y) for (int x = 0; x < v.w; ++x) if (!InHud(x, y)) { const size_t i = (size_t)y * v.w + x; nNew += aNew[i]; nOld += aOld[i]; }
            Proof(nNew <= 16 && nOld > 1000, "absorbed, the Xbox curve, dither %s: the mask acts on %zu untouched pixels (the old line: %zu)",
                  dither > 0.5f ? "on: " : "off:", nNew, nOld);
        }
        size_t statesExact = 0, statesXbox = 0; bool allOk = true; const char* worstName = "";
        for (const State& s0 : st)
            for (float lvl : { kLevel, 1.0f, 0.0f })
            {
                const bool xbox = s0.c[8][0] > 0.5f && s0.c[8][0] < 1.5f;
                const Out a = Ui(v, s0.cga ? rlC : rlP, s0, lvl), b = Ui(v, s0.cga ? oldC : oldP, s0, lvl);
                if (!xbox)
                {
                    const Cmp r = Compare(v, a, b, [](int, int) { return true; });
                    const bool ok = r.diff32 == 0 && r.nanMismatch == 0 && r.diff16 == 0;
                    if (!ok && allOk) { allOk = false; worstName = s0.name; printf("        %s, level %.2f: %s\n", s0.name, lvl, Tell(r)); }
                    statesExact += ok;
                }
                else
                {
                    const std::vector<uint8_t> aOld = Acts(v, Ui(v, s0.cga ? oldC : oldP, s0, kLevel), Ui(v, s0.cga ? oldC : oldP, s0, 0.0f));
                    const std::vector<uint8_t> aNew = Acts(v, Ui(v, s0.cga ? rlC : rlP, s0, kLevel), Ui(v, s0.cga ? rlC : rlP, s0, 0.0f));
                    const Cmp r = Compare(v, a, b, [&](int x, int y) { const size_t i = (size_t)y * v.w + x; return !InHud(x, y) && !aOld[i] && !aNew[i]; });
                    const bool ok = r.maxUlp <= 4 && r.nanMismatch == 0 && r.maxHalfStep <= 1;
                    if (!ok && allOk) { allOk = false; worstName = s0.name; printf("        %s, level %.2f (untouched, neither mask acting): %s\n", s0.name, lvl, Tell(r)); }
                    statesXbox += ok;
                }
            }
        Proof(allOk, "nothing else moved: %zu state-and-level runs without the Xbox curve bit-exact on every pixel, %zu with it within 4"
              " float32 steps and one half-float step on every untouched pixel where neither mask acts%s%s", statesExact, statesXbox,
              allOk ? "" : "; FIRST DIFFERENCE IN ", worstName);
        oldP->Release(); oldC->Release();

        // 6. THE RESTORATION'S XBOX CURVE (the owner, 2026-09-29 evening: "yes switch it"): the exact ratio (source 1) puts its
        // unclipped value through FusionFix's formula above white, not the line. Today's encode against the control build.ps1
        // generates (the same with the ratio on the line; B3 switch 14's control until Stage F). Without the Xbox curve every pixel must be bit-exact. With it a pixel may differ ONLY
        // WHERE THE CONTROL'S RESTORATION ACTED (its output differs from the control with restoration off, c0.w = 0): the ratio
        // changes only for a composite with a channel above white, where the line already made it above 1. The size is the two
        // curves' own: the formula over the line at x times white is +3.3 percent at 3, +5.8 at 4.55, +11.5 at 10 (this frame's
        // specials reach that far; the game's highlights read up to 4.55); so the most brightened pixel must stay under kBrighter.
        // "ACTED" IS OUTPUT EQUALITY, so a restoration small enough to round to the unrestored value counts as not acting: a
        // handful may differ outside it (the second run: 2 of 1,675,913). DARKER BY A HAIR IS THE CURVES TOO: just above white the
        // formula climbs at 1.125 where the line was drawn at the table's 1.130 (the second run: 5,067 pixels, at most 0.030 percent).
        // THE FIRST RUN, with a rule that also asked for no change below paper white, failed on this frame's own inconsistency: its
        // composite is not the frame's, so pixels below white were restored (400,074 of them), which the game cannot do.
        printf("  6: the restoration's exact ratio uses FusionFix's Xbox formula above white (the owner's switch), against the encode as it was\n");
        {
            const double kBrighter = 0.13;
            size_t runsExact = 0, runsXbox = 0, outside = 0, darker = 0, nDiff = 0, nExactRatioDiff = 0; double maxUp = 0.0, maxDown = 0.0; bool ok6 = true;
            for (const State& s0 : st)
                for (float lvl : { kLevel, 1.0f, 0.0f })
                {
                    const bool xbox = s0.c[8][0] > 0.5f && s0.c[8][0] < 1.5f;
                    const Out a = Ui(v, s0.cga ? sh.encC : sh.enc, s0, lvl), b = Ui(v, s0.cga ? rlC : rlP, s0, lvl);
                    if (!xbox)
                    {
                        const Cmp r = Compare(v, a, b, [](int, int) { return true; });
                        if (r.diff32 || r.nanMismatch || r.diff16) { ok6 = false; printf("        %s, level %.2f: %s\n", s0.name, lvl, Tell(r)); }
                        runsExact++;
                        continue;
                    }
                    runsXbox++;
                    State off = s0; off.c[0][3] = 0.0f;
                    const Out bOff = Ui(v, s0.cga ? rlC : rlP, off, lvl);
                    size_t here = 0;
                    for (size_t i = 0; i < (size_t)v.w * v.h; ++i)
                    {
                        bool d = false, acted = false;
                        for (int ch = 0; ch < 3; ++ch)
                        {
                            const float x = a.f32[4 * i + ch], y = b.f32[4 * i + ch], z = bOff.f32[4 * i + ch];
                            if (!(x == y || (std::isnan(x) && std::isnan(y)))) d = true;
                            if (!(y == z || (std::isnan(y) && std::isnan(z)))) acted = true;
                        }
                        if (!d) continue;
                        nDiff++; here++; if (!strcmp(s0.name, "source 1, the exact ratio")) nExactRatioDiff++;
                        if (!acted) outside++;
                        const double ya = 0.2126 * a.f32[4 * i] + 0.7152 * a.f32[4 * i + 1] + 0.0722 * a.f32[4 * i + 2];
                        const double yb = 0.2126 * b.f32[4 * i] + 0.7152 * b.f32[4 * i + 1] + 0.0722 * b.f32[4 * i + 2];
                        if (yb > 0.0 && std::isfinite(ya) && std::isfinite(yb))
                        {
                            const double q = ya / yb - 1.0;
                            maxUp = std::max(maxUp, q);
                            if (q < -1e-6) { darker++; maxDown = std::max(maxDown, -q); }
                        }
                    }
                    if (here) printf("        %-52s level %.2f: %zu pixels differ\n", s0.name, lvl, here);
                }
            Proof(ok6 && outside <= 8 && maxUp <= kBrighter && maxDown <= 0.005 && nExactRatioDiff > 100,
                  "%zu runs without the Xbox curve bit-exact; %zu with it: %zu pixels differ (%zu in the exact-ratio state), outside what the"
                  " restoration touched %zu; brightened at most %.2f percent (bound %.0f), darker %zu by at most %.3f percent (bound 0.5)",
                  runsExact, runsXbox, nDiff, nExactRatioDiff, outside, 100.0 * maxUp, 100.0 * kBrighter, darker, 100.0 * maxDown);
        }
        rlP->Release(); rlC->Release();
    }

    printf("  NEGATIVE CONTROLS, each must FAIL:\n");
    {
        State a; Base(a, "nc", false, 1.0f, true);
        const Out ref = Pipeline(v, sh, a);
        Cmp r = Compare(v, ref, Render(v, sh.enc, a.c, v.tex), away);
        Proof(!Within(r), "the encode WITHOUT sharpening against the pipeline:        %s", Tell(r));
        float cw[9][4]; memcpy(cw, a.c, sizeof(cw)); G10Const(cw, a);
        r = Compare(v, ref, Render(v, sh.wCeil, cw, v.tex), away);
        Proof(!Within(r), "a G10 whose ceiling is always 1:                            %s", Tell(r));
        r = Compare(v, ref, Render(v, sh.wLanes, cw, v.tex), away);
        Proof(!Within(r), "a G10 that reads its attenuation from the ceiling's lane:   %s", Tell(r));
        Proof(maskMismatch(sh.wMask, mc[0], nullptr) > 0, "a G10 whose mask compares the SHARPENED sample: its mask differs from the shipping one on %zu pixels", maskMismatch(sh.wMask, mc[0], nullptr));
        Proof(maskMismatch(sh.wMaskC, mc[2], nullptr) > 0, "the same, absorbed: on %zu pixels", maskMismatch(sh.wMaskC, mc[2], nullptr));
        // THE ABSORBED RULE CAN FAIL: under it, the shipping absorbed encode (no sharpening) and an absorbed G10 that puts the
        // curve on the centre before sharpening, each against the pipeline, in the dithered Xbox state.
        State b; Base(b, "nc", true, 1.0f, true);
        const Out refC = Pipeline(v, sh, b);
        r = Compare(v, refC, Render(v, sh.encC, b.c, v.tex), away);
        Proof(!absorbedOk(b, r, false), "the absorbed encode WITHOUT sharpening, under the absorbed rule: %s", Tell(r));
        float co[9][4]; memcpy(co, b.c, sizeof(co)); G10Const(co, b);
        r = Compare(v, refC, Render(v, sh.wOrder, co, v.tex), away);
        Proof(!absorbedOk(b, r, false), "an absorbed G10 that curves before it sharpens, same rule: %s", Tell(r));
    }
    printf("%s\n", gFails ? "G10 PICTURE PROOF: FAIL" : "G10 PICTURE PROOF: PASS");
    return gFails ? 1 : 0;
}
