// THE PROOF OF quad_uv (2026-10-04): the hook's own reading and correcting code (source/transport/quad_uv.h) on the
// GPU, with no game, against a stand-in for the game's distance pass. tools/test-rain is the pattern.
//
// THE PASS, as a RenderDoc capture of the game shows it (docs/DECISIONS.md, "THE FAINT LINES UNDER A RENDER SCALE"): a
// quad of two triangles, six vertices of the game's 36-byte immediate vertex appended to a dynamic vertex buffer, a
// vertex shader that passes position and coordinate straight on, the scene read with a LINEAR filter FROM THE TARGET
// BEING WRITTEN, and the half texel of the size the game believes baked into the coordinates. Here the pixel shader
// multiplies by a gain, which is enough to see a pixel that was read after it had been written.
//
// THREE DRAWS OF IT:
//   NATIVE    the game believes the real size (no render scale): the tap is the pixel's own texel
//   AS 1.0.8  the game believes a larger size (a render scale), the quad drawn as the game made it
//   THE FIX   the same, drawn as RsQuadDraw draws it: QuvRead from the buffer, QuvFix, DrawPrimitiveUP, stream 0 back
//
// THE PROOF:
//   1  QuvFix puts the half texel right: every coordinate of the game's quad becomes 0 or 1 plus half a texel of the
//      REAL size, bit for bit, and no other byte of the vertices changes. A list of six and a strip of four.
//   2  QuvFix leaves everything else alone: a quad that already carries the real half texel, a corner that is not at
//      plus or minus one, a coordinate that is neither, another size's half texel, equal sizes. Each is refused and
//      not one byte changes.
//   3  QuvRead gives back what the game wrote: six vertices appended to a DYNAMIC, WRITEONLY, default-pool buffer at
//      an offset, read through the real Direct3D 9 layer, are the bytes written. A managed buffer and a static one are
//      refused (their lock would make the device thread wait).
//   4  THE FIX IS THE NATIVE DRAW: at each pair of sizes, on a flat picture and on a random one, the fixed draw's
//      result is bit for bit the native draw's.
//   5  The device is left with the game's buffer on stream 0, and a draw from it right after is the native draw.
//   6  QuvVsStraight, the gate on which draws are read back at all: true for this tool's pass-through shader and false
//      for one that transforms its position; and, where FusionFix's sources are at hand (--ff <its win32_30_nv8 folder>,
//      assembled with the system's d3dx9_43.dll), true for the game's own full-screen shaders (rage_postfx VS0 and VS1,
//      gta_im VS3, rage_im VS2, deferred_lighting VS0, VS19 and VS20) and false for the ones the coronas, the sky, the
//      clouds and the light volumes go through.
// CONTROLS, each of which must FAIL as named:
//   A  THE FAULT ITSELF: the quad as the game made it at a render scale, on a FLAT picture. A flat picture read off
//      centre is still flat, so every pixel that differs was read AFTER a neighbour had been written. They exist, and
//      they sit on the quad's seam and on columns that are multiples of 1024.
//   B  the same quad on a random picture is not the native draw (the off-centre tap mixes the neighbours in).
//   C  the hook told a wrong believed size corrects nothing: its result is the 1.0.8 draw, not the native one.
//   D  a user-pointer draw without the rebind leaves stream 0 empty (so the rebind in the hook is needed).
//
//     tools\test-quad\build.ps1             builds qv.exe (x86, as the game)
//     qv.exe --d3d9 <path> [--ff <dir>] [--timing]       <path>: DXVK's d3d9, a game folder's vulkan.dll
// Exit 0 when every proof passed. It changes nothing on the machine.
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
#include <string>
#include <algorithm>
#include "qv_pass_vs.h"
#include "qv_dist_ps.h"
#include "qv_wvp_vs.h"
#include "../../source/transport/quad_uv.h"

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1400]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
static void Control(bool failsAsItMust, const char* fmt, ...)
{
    char b[1400]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  CONTROL (%s)%s\n", failsAsItMust ? "PASS" : "FAIL", b, failsAsItMust ? ": fails, as it must" : ": THE PROOF CANNOT SEE THIS FAULT");
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
static uint32_t gRng = 0x13572468u;
static float Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0f; }

typedef std::vector<uint16_t> Img;             // four halves a pixel: r, g, b, a

// THE GAME'S IMMEDIATE VERTEX, as the capture's declaration has it: a FLOAT3 position at 0, a colour at 24, a FLOAT2
// coordinate at 28, 36 bytes.
#pragma pack(push, 1)
struct GV { float x, y, z; float nx, ny, nz; DWORD col; float u, v; };
#pragma pack(pop)
static_assert(sizeof(GV) == 36, "the game's immediate vertex is 36 bytes");
static const UINT kUvOff = 28, kVbBytes = 64 * 1024;
static const float kGain = 1.25f;

// The quad as the game builds it for a target it believes is bw x bh (the capture's order and values)
static void GameQuad(GV q[6], UINT bw, UINT bh)
{
    const float hu = 0.5f / (float)bw, hv = 0.5f / (float)bh;
    const float c[6][4] = { { -1, 1, 0, 0 }, { -1, -1, 0, 1 }, { 1, 1, 1, 0 }, { -1, -1, 0, 1 }, { 1, -1, 1, 1 }, { 1, 1, 1, 0 } };
    for (int i = 0; i < 6; i++) { q[i] = GV{ c[i][0], c[i][1], 0.1f, 0, 0, 0, 0xFFFFFFFFu, c[i][2] + hu, c[i][3] + hv }; }
}

struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9* vs = nullptr; IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DPixelShader9* ps = nullptr; IDirect3DVertexBuffer9* vb = nullptr; UINT vbAt = 0;
    IDirect3DTexture9* scene = nullptr; IDirect3DSurface9* sceneSurf = nullptr; IDirect3DSurface9* sys = nullptr;
    int w = 0, h = 0;
};
static const D3DFORMAT kFmt = D3DFMT_A16B16G16R16F;

static void Size(Dev& v, int w, int h)
{
    for (IUnknown* u : { (IUnknown*)v.sceneSurf, (IUnknown*)v.scene, (IUnknown*)v.sys }) if (u) u->Release();
    v.w = w; v.h = h;
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, kFmt, D3DPOOL_DEFAULT, &v.scene, nullptr)); CK(v.scene->GetSurfaceLevel(0, &v.sceneSurf));
    CK(v.d->CreateOffscreenPlainSurface(w, h, kFmt, D3DPOOL_SYSTEMMEM, &v.sys, nullptr));
}
static void Put(Dev& v, const Img& img)
{
    D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, 0));
    for (int y = 0; y < v.h; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &img[(size_t)y * v.w * 4], sizeof(uint16_t) * 4 * v.w);
    v.sys->UnlockRect();
    CK(v.d->UpdateSurface(v.sys, nullptr, v.sceneSurf, nullptr));
}
static Img Get(Dev& v)
{
    CK(v.d->GetRenderTargetData(v.sceneSurf, v.sys));
    Img out((size_t)v.w * v.h * 4); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; ++y) memcpy(&out[(size_t)y * v.w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(uint16_t) * 4 * v.w);
    v.sys->UnlockRect(); return out;
}
// The distance pass's state: the scene buffer is the target AND the texture, read with a linear filter
static void State(Dev& v)
{
    IDirect3DDevice9* d = v.d;
    d->SetRenderTarget(0, v.sceneSurf); d->SetDepthStencilSurface(nullptr);
    d->SetRenderState(D3DRS_ZENABLE, FALSE); d->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); d->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); d->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    d->SetVertexShader(v.vs); d->SetVertexDeclaration(v.decl); d->SetPixelShader(v.ps);
    const float g[4] = { kGain, 0, 0, 0 }; d->SetPixelShaderConstantF(0, g, 1);
    d->SetTexture(0, v.scene);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR); d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    d->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}
// The game appends to its dynamic buffer: NOOVERWRITE while there is room, DISCARD when it wraps
static UINT Append(Dev& v, const GV* q, UINT n)
{
    const UINT bytes = n * sizeof(GV); void* p = nullptr;
    if (v.vbAt + bytes > kVbBytes) { v.vbAt = 0; CK(v.vb->Lock(0, bytes, &p, D3DLOCK_DISCARD)); }
    else CK(v.vb->Lock(v.vbAt, bytes, &p, D3DLOCK_NOOVERWRITE));
    memcpy(p, q, bytes); v.vb->Unlock();
    const UINT start = v.vbAt / sizeof(GV); v.vbAt += bytes; return start;
}
enum { AS_GAME, AS_HOOK };
// One distance pass over the scene buffer. bw x bh: what the game believes. AS_HOOK: drawn as RsQuadDraw draws it, told
// that the believed size is hw x hh (the target's original size in the mod's table). `rebind`: the hook's last line.
// Returns whether the quad was corrected.
static bool Pass(Dev& v, UINT bw, UINT bh, int how, UINT hw = 0, UINT hh = 0, bool rebind = true)
{
    GV q[6]; GameQuad(q, bw, bh);
    const UINT start = Append(v, q, 6);
    CK(v.d->SetStreamSource(0, v.vb, 0, sizeof(GV)));
    State(v);
    if (how == AS_GAME) { CK(v.d->DrawPrimitive(D3DPT_TRIANGLELIST, start, 2)); return false; }
    BYTE buf[kQuvMaxVerts * kQuvMaxStride];
    const UINT n = QuvVerts(D3DPT_TRIANGLELIST);
    if (!QuvRead(v.vb, 0 + start * (UINT)sizeof(GV), n * (UINT)sizeof(GV), buf)) { printf("  FAIL  QuvRead refused the game's buffer\n"); exit(2); }
    if (!QuvFix(buf, n, sizeof(GV), kUvOff, hw, hh, (UINT)v.w, (UINT)v.h)) { CK(v.d->DrawPrimitive(D3DPT_TRIANGLELIST, start, 2)); return false; }
    CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, buf, sizeof(GV)));
    if (rebind) CK(v.d->SetStreamSource(0, v.vb, 0, sizeof(GV)));
    return true;
}
static Img Run(Dev& v, const Img& pic, UINT bw, UINT bh, int how, UINT hw = 0, UINT hh = 0)
{
    Put(v, pic); Pass(v, bw, bh, how, hw, hh); return Get(v);
}
static Img Flat(int w, int h) { Img p((size_t)w * h * 4, ToHalf(1.0f)); return p; }
static Img Random(int w, int h) { Img p((size_t)w * h * 4); for (size_t i = 0; i < p.size(); ++i) p[i] = ToHalf(0.25f + 1.5f * Rnd()); return p; }

struct FlatStat { long bad = 0, onCol = 0, onSeam = 0, other = 0; double worst = 0.0; };
// On a flat picture every pixel should be the picture times the gain. One that is not was read after a neighbour had
// been written. Where are they: on a column that is a multiple of 1024, within two pixels of the quad's seam (bottom
// left to top right), or elsewhere.
static FlatStat FlatRead(const Img& out, int w, int h)
{
    FlatStat s; const uint16_t want = ToHalf(kGain);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    {
        const uint16_t got = out[((size_t)y * w + x) * 4 + 1];
        if (got == want) continue;
        s.bad++; s.worst = std::max(s.worst, fabs((double)FromHalf(got) / (double)kGain - 1.0));
        const double seamY = (double)h - ((double)x + 0.5) * (double)h / (double)w;
        if (x && (x % 1024) == 0) s.onCol++;
        else if (fabs(((double)y + 0.5) - seamY) <= 2.0) s.onSeam++;
        else s.other++;
    }
    return s;
}
// Mean relative distance of a result from "every pixel is its own texel times the gain"
static double OwnTexelErr(const Img& out, const Img& pic)
{
    double sum = 0.0; const size_t n = out.size() / 4;
    for (size_t px = 0; px < n; ++px)
    {
        const double want = (double)FromHalf(pic[px * 4 + 1]) * (double)kGain;
        sum += fabs((double)FromHalf(out[px * 4 + 1]) - want) / want;
    }
    return sum / (double)n;
}
static long Differ(const Img& a, const Img& b) { long n = 0; for (size_t px = 0; px < a.size() / 4; ++px) if (memcmp(&a[px * 4], &b[px * 4], 8)) n++; return n; }

static void CpuProofs()
{
    {   // 1
        bool ok = true; long checked = 0;
        const UINT pairs[4][4] = { { 3840, 2160, 2560, 1440 }, { 3840, 2160, 1920, 1080 }, { 1920, 1080, 1280, 720 }, { 2560, 1440, 1707, 960 } };
        for (int p = 0; p < 4; ++p) for (UINT n : { 6u, 4u })
        {
            GV q[6], was[6]; GameQuad(q, pairs[p][0], pairs[p][1]);
            if (n == 4) { q[3] = q[4]; }            // a strip: top left, bottom left, top right, bottom right
            memcpy(was, q, sizeof(q));
            const bool did = QuvFix((BYTE*)q, n, sizeof(GV), kUvOff, pairs[p][0], pairs[p][1], pairs[p][2], pairs[p][3]);
            ok = ok && did;
            const float nu = 0.5f / (float)pairs[p][2], nv = 0.5f / (float)pairs[p][3];
            for (UINT i = 0; i < n; ++i)
            {
                const float wu = (was[i].u > 0.5f ? 1.0f : 0.0f) + nu, wv = (was[i].v > 0.5f ? 1.0f : 0.0f) + nv;
                ok = ok && memcmp(&q[i].u, &wu, 4) == 0 && memcmp(&q[i].v, &wv, 4) == 0 && memcmp(&q[i], &was[i], kUvOff) == 0; checked++;
            }
            for (UINT i = n; i < 6; ++i) ok = ok && memcmp(&q[i], &was[i], sizeof(GV)) == 0;     // nothing past the n it was given
        }
        Proof(ok && checked == 40, "1  QuvFix puts the half texel right: %ld vertices over four pairs of sizes, a list of six and a strip of four, each coordinate 0 or 1 plus half a texel"
              " of the REAL size bit for bit, and no other byte of a vertex changed", checked);
    }
    {   // 2
        int refused = 0, untouched = 0; const int cases = 6;
        for (int c = 0; c < cases; ++c)
        {
            GV q[6], was[6]; UINT ow = 3840, oh = 2160, nw = 2560, nh = 1440;
            GameQuad(q, 3840, 2160);
            if (c == 0) GameQuad(q, 2560, 1440);                 // already the real half texel
            if (c == 1) q[2].x = 0.5f;                           // a corner that is not at one
            if (c == 2) q[4].u = 0.25f;                          // a coordinate that is neither 0 nor 1 plus the half texel
            if (c == 3) GameQuad(q, 1920, 1080);                 // another size's half texel
            if (c == 4) { nw = ow; nh = oh; }                    // equal sizes: nothing to correct
            if (c == 5) q[1].v = 1.0f;                           // a quad with no half texel at all
            memcpy(was, q, sizeof(q));
            if (!QuvFix((BYTE*)q, 6, sizeof(GV), kUvOff, ow, oh, nw, nh)) refused++;
            if (memcmp(q, was, sizeof(q)) == 0) untouched++;
        }
        Proof(refused == cases && untouched == cases, "2  QuvFix leaves everything else alone: %d of %d other quads refused (the real half texel already, a corner off one, a stray coordinate,"
              " another size's half texel, equal sizes, no half texel), %d of them without a byte changed", refused, cases, untouched);
    }
}

// FusionFix's own vertex shaders, assembled from their source text with the system's D3DX (tools/test-corona's way)
struct ID3DXBufferLite : public IUnknown { virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0; virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0; };
typedef HRESULT (WINAPI* PFN_ASM)(LPCSTR, UINT, const void*, void*, DWORD, ID3DXBufferLite**, ID3DXBufferLite**);
// 1 straight, 0 not, -1 the file or the assembler is missing or the text did not assemble
static int StraightOf(const std::string& path)
{
    FILE* f = nullptr; if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return -1;
    std::string src; char buf[4096]; size_t n; while ((n = fread(buf, 1, sizeof(buf), f)) > 0) src.append(buf, n); fclose(f);
    static HMODULE x = LoadLibraryA("d3dx9_43.dll");
    static PFN_ASM as = x ? (PFN_ASM)GetProcAddress(x, "D3DXAssembleShader") : nullptr;
    if (!as) return -1;
    ID3DXBufferLite *code = nullptr, *err = nullptr;
    if (FAILED(as(src.data(), (UINT)src.size(), nullptr, nullptr, 0, &code, &err)) || !code) { if (err) err->Release(); return -1; }
    const int r = QuvVsStraight((const DWORD*)code->GetBufferPointer(), code->GetBufferSize() / 4) ? 1 : 0;
    code->Release(); if (err) err->Release();
    return r;
}
static void StraightProofs(const char* ff)
{
    const bool own = QuvVsStraight((const DWORD*)kQvPassVS, sizeof(kQvPassVS) / 4), wvp = QuvVsStraight((const DWORD*)kQvWvpVS, sizeof(kQvWvpVS) / 4);
    const DWORD junk[4] = { 0xFFFF0300u, 0x0000FFFFu, 0, 0 };                 // a PIXEL shader's version token: not a vertex shader at all
    Proof(own && !wvp && !QuvVsStraight(junk, 4) && !QuvVsStraight(nullptr, 0),
          "6  QuvVsStraight: this tool's pass-through vertex shader is %s, the one that transforms its position is %s, and what is not a vertex shader is turned away",
          own ? "straight" : "NOT STRAIGHT", wvp ? "STRAIGHT" : "not");
    if (!ff) { printf("  (no --ff: FusionFix's own shaders are not classified)\n"); return; }
    const char* yes[] = { "rage_postfx\\rage_postfxVS0.asm", "rage_postfx\\rage_postfxVS1.asm", "gta_im\\gta_imVS3.asm", "rage_im\\rage_imVS2.asm",
                          "deferred_lighting\\deferred_lightingVS0.asm", "deferred_lighting\\deferred_lightingVS19.asm", "deferred_lighting\\deferred_lightingVS20.asm" };
    const char* no[]  = { "deferred_lighting\\deferred_lightingVS11.asm", "deferred_lighting\\deferred_lightingVS15.asm", "deferred_lighting\\deferred_lightingVS17.asm",
                          "gta_atmoscatt_clouds\\gta_atmoscatt_cloudsVS7.asm", "gta_im\\gta_imVS0.asm", "gta_im\\gta_imVS1.asm", "gta_im\\gta_imVS2.asm",
                          "rage_im\\rage_imVS0.asm", "rage_im\\rage_imVS1.asm", "gta_rmptfx_litsprite\\gta_rmptfx_litspriteVS0.asm", "gta_default\\gta_defaultVS0.asm" };
    int yesOk = 0, noOk = 0, missing = 0; std::string wrong;
    for (const char* s : yes) { const int r = StraightOf(std::string(ff) + "\\" + s); if (r == 1) yesOk++; else if (r < 0) missing++; else wrong += std::string(" ") + s; }
    for (const char* s : no)  { const int r = StraightOf(std::string(ff) + "\\" + s); if (r == 0) noOk++;  else if (r < 0) missing++; else wrong += std::string(" ") + s; }
    Proof(yesOk == (int)(sizeof(yes) / sizeof(yes[0])) && noOk == (int)(sizeof(no) / sizeof(no[0])),
          "6  QuvVsStraight on FusionFix's own shaders: %d of %d full-screen ones are straight (rage_postfx VS0 is the distance pass's), %d of %d that transform the position are not; %d could not be read or assembled%s%s",
          yesOk, (int)(sizeof(yes) / sizeof(yes[0])), noOk, (int)(sizeof(no) / sizeof(no[0])), missing, wrong.empty() ? "" : "; WRONG:", wrong.c_str());
}

static void GpuProofs(Dev& v, UINT bw, UINT bh, int w, int h)
{
    printf(" the game believes %ux%u, the target is %dx%d\n", bw, bh, w, h);
    Size(v, w, h);
    const Img flat = Flat(w, h), rnd = Random(w, h);
    const Img nativeFlat = Run(v, flat, (UINT)w, (UINT)h, AS_GAME), nativeRnd = Run(v, rnd, (UINT)w, (UINT)h, AS_GAME);
    const Img oldFlat = Run(v, flat, bw, bh, AS_GAME), oldRnd = Run(v, rnd, bw, bh, AS_GAME);
    Put(v, flat); const bool did1 = Pass(v, bw, bh, AS_HOOK, bw, bh); const Img fixFlat = Get(v);
    Put(v, rnd);  const bool did2 = Pass(v, bw, bh, AS_HOOK, bw, bh); const Img fixRnd = Get(v);
    const FlatStat sn = FlatRead(nativeFlat, w, h), so = FlatRead(oldFlat, w, h), sf = FlatRead(fixFlat, w, h);
    const double en = OwnTexelErr(nativeRnd, rnd), eo = OwnTexelErr(oldRnd, rnd), ef = OwnTexelErr(fixRnd, rnd);
    Control(so.bad > 0 && so.onCol + so.onSeam > 0,
            "A  the quad as the game made it, on a flat picture: %ld pixels were read after a neighbour had been written (up to %.1f percent off): %ld on columns that are multiples of 1024,"
            " %ld within two pixels of the quad's seam, %ld elsewhere. The native draw: %ld",
            so.bad, 100.0 * so.worst, so.onCol, so.onSeam, so.other, sn.bad);
    Control(Differ(oldRnd, nativeRnd) > (long)w * h / 2 && eo > 4.0 * en,
            "B  the quad as the game made it, on a random picture: %ld of %ld pixels are not the native draw's; mean distance from 'its own texel times the gain' %.2f percent against the native %.2f",
            Differ(oldRnd, nativeRnd), (long)w * h, 100.0 * eo, 100.0 * en);
    Proof(did1 && did2 && Differ(fixFlat, nativeFlat) == 0 && Differ(fixRnd, nativeRnd) == 0 && sf.bad == sn.bad,
          "4  the fix is the native draw: corrected %s; a flat picture differs from the native draw on %ld pixels and a random one on %ld; pixels read after a neighbour was written %ld (native %ld);"
          " mean distance from 'its own texel times the gain' %.2f percent (native %.2f)",
          (did1 && did2) ? "both" : "NOT BOTH", Differ(fixFlat, nativeFlat), Differ(fixRnd, nativeRnd), sf.bad, sn.bad, 100.0 * ef, 100.0 * en);
    {   // control C: the hook told another believed size corrects nothing
        Put(v, rnd); const bool did = Pass(v, bw, bh, AS_HOOK, bw + 64, bh + 36); const Img out = Get(v);
        // (not "bit for bit the 1.0.8 draw": that draw reads pixels it is writing, and is not the same twice)
        Control(!did && Differ(out, nativeRnd) > (long)w * h / 2,
                "C  the hook told a wrong believed size (%ux%u): corrected %s, and %ld of %ld pixels are not the native draw's (%ld differ from the earlier 1.0.8 draw: that draw is not the same twice)",
                bw + 64, bh + 36, did ? "YES" : "nothing", Differ(out, nativeRnd), (long)w * h, Differ(out, oldRnd));
    }
    {   // 5 and control D: stream 0
        IDirect3DVertexBuffer9* sb = nullptr; UINT off = 99, str = 99;
        Put(v, rnd); Pass(v, bw, bh, AS_HOOK, bw, bh, false);
        v.d->GetStreamSource(0, &sb, &off, &str);
        Control(sb == nullptr, "D  a user-pointer draw with no rebind: stream 0 holds %s afterwards", sb ? "A BUFFER" : "nothing");
        if (sb) sb->Release();
        sb = nullptr;
        Put(v, rnd); Pass(v, bw, bh, AS_HOOK, bw, bh, true);
        v.d->GetStreamSource(0, &sb, &off, &str);
        const bool back = sb == v.vb && off == 0 && str == sizeof(GV);
        if (sb) sb->Release();
        // and the game's next draw from its buffer, with no SetStreamSource of its own, is what it would have been
        Put(v, rnd);
        GV q[6]; GameQuad(q, (UINT)w, (UINT)h); const UINT start = Append(v, q, 6);
        State(v); CK(v.d->DrawPrimitive(D3DPT_TRIANGLELIST, start, 2));
        const Img next = Get(v);
        Proof(back && Differ(next, nativeRnd) == 0, "5  the device is left with the game's buffer on stream 0 (%s), and the game's next draw from it is the native draw (%ld pixels differ)",
              back ? "its buffer, offset 0, stride 36" : "NOT RESTORED", Differ(next, nativeRnd));
    }
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; const char* ff = nullptr; bool timing = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i];
        else if (!strcmp(argv[i], "--ff") && i + 1 < argc) ff = argv[++i];
        else if (!strcmp(argv[i], "--timing")) timing = true;
    }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The proof of quad_uv: the game's full-screen quad at a render scale, read and corrected by the hook's own code, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "qvproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("qvproof", "qv", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kQvPassVS, &v.vs));
    CK(v.d->CreatePixelShader((const DWORD*)kQvDistPS, &v.ps));
    D3DVERTEXELEMENT9 el[] = { { 0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
                               { 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
                               { 0, 28, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.decl));
    CK(v.d->CreateVertexBuffer(kVbBytes, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &v.vb, nullptr));
    CK(v.d->BeginScene());

    CpuProofs();
    StraightProofs(ff);
    {   // 3: QuvRead through the real layer
        GV q[6]; BYTE got[6 * sizeof(GV)]; bool same = true; UINT at = 0;
        for (int round = 0; round < 40; ++round)
        {
            GameQuad(q, 3840, 2160); for (int i = 0; i < 6; ++i) { q[i].z = Rnd(); q[i].col = (DWORD)round * 0x01010101u + (DWORD)i; }
            const UINT start = Append(v, q, 6); at = start;
            same = same && QuvRead(v.vb, start * (UINT)sizeof(GV), sizeof(q), got) && memcmp(got, q, sizeof(q)) == 0;
        }
        IDirect3DVertexBuffer9 *man = nullptr, *stat = nullptr; BYTE scratch[sizeof(q)];
        CK(v.d->CreateVertexBuffer(4096, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &man, nullptr));
        CK(v.d->CreateVertexBuffer(4096, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &stat, nullptr));
        const bool refMan = !QuvRead(man, 0, sizeof(q), scratch), refStat = !QuvRead(stat, 0, sizeof(q), scratch);
        const bool refEnd = !QuvRead(v.vb, kVbBytes - 100, sizeof(q), scratch);
        // the copy itself, at every start within a line: QuvCopy reads whole 64-byte lines and must hand back exactly the bytes asked for
        __declspec(align(64)) static BYTE mem[512]; for (int i = 0; i < 512; ++i) mem[i] = (BYTE)(i * 37 + 11);
        for (int at0 = 64; at0 < 128; ++at0) for (UINT len : { 1u, 36u, 144u, 216u })
        {
            BYTE o[216]; memset(o, 0xEE, sizeof(o)); QuvCopy(o, mem + at0, len);
            same = same && memcmp(o, mem + at0, len) == 0 && (len == sizeof(o) || o[len] == 0xEE);
        }
        man->Release(); stat->Release();
        Proof(same && refMan && refStat && refEnd, "3  QuvRead gives back what the game wrote: 40 quads appended to a DYNAMIC, WRITEONLY, default-pool buffer are %s (the last at vertex %u);"
              " a managed buffer is %s, a static one %s, a read past the buffer's end %s",
              same ? "read back byte for byte" : "NOT WHAT WAS WRITTEN", at, refMan ? "refused" : "READ", refStat ? "refused" : "READ", refEnd ? "refused" : "READ");
    }
    GpuProofs(v, 3840, 2160, 2560, 1440);
    GpuProofs(v, 3840, 2160, 1920, 1080);
    GpuProofs(v, 1920, 1080, 1280, 720);

    if (timing)
    {
        GV q[6]; GameQuad(q, 3840, 2160); const UINT start = Append(v, q, 6);
        BYTE buf[kQuvMaxVerts * kQuvMaxStride]; LARGE_INTEGER f, a, b; QueryPerformanceFrequency(&f);
        const int n = 200000; long fixed = 0;
        QueryPerformanceCounter(&a);
        for (int i = 0; i < n; ++i)
            if (QuvRead(v.vb, start * (UINT)sizeof(GV), 6 * (UINT)sizeof(GV), buf) && QuvFix(buf, 6, sizeof(GV), kUvOff, 3840, 2160, 2560, 1440)) fixed++;
        QueryPerformanceCounter(&b);
        printf("  TIMING, on the CPU: reading the game's six vertices back and correcting them takes %.0f nanoseconds a quad (%d quads, %ld corrected). The game draws three such quads a frame.\n",
               (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n, n, fixed);
        // where the time goes: the description, the lock and unlock, the copy out of the buffer's memory, the arithmetic
        D3DVERTEXBUFFER_DESC d; void* ptr = nullptr; double ns[4];
        QueryPerformanceCounter(&a); for (int i = 0; i < n; ++i) v.vb->GetDesc(&d); QueryPerformanceCounter(&b);
        ns[0] = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;
        QueryPerformanceCounter(&a); for (int i = 0; i < n; ++i) { v.vb->Lock(start * (UINT)sizeof(GV), 216, &ptr, D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE); v.vb->Unlock(); } QueryPerformanceCounter(&b);
        ns[1] = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;
        v.vb->Lock(start * (UINT)sizeof(GV), 216, &ptr, D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE); v.vb->Unlock();
        volatile BYTE sink = 0;
        QueryPerformanceCounter(&a); for (int i = 0; i < n; ++i) { memcpy(buf, ptr, 216); sink = (BYTE)(sink + buf[i % 216]); } QueryPerformanceCounter(&b);
        ns[2] = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;
        QueryPerformanceCounter(&a); for (int i = 0; i < n; ++i) { memcpy(buf, q, 216); QuvFix(buf, 6, sizeof(GV), kUvOff, 3840, 2160, 2560, 1440); } QueryPerformanceCounter(&b);
        ns[3] = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;
        QueryPerformanceCounter(&a); for (int i = 0; i < n; ++i) { QuvCopy(buf, (const BYTE*)ptr, 216); sink = (BYTE)(sink + buf[i % 216]); } QueryPerformanceCounter(&b);
        const double nsStream = (double)(b.QuadPart - a.QuadPart) * 1e9 / (double)f.QuadPart / (double)n;
        printf("  TIMING, the parts (nanoseconds): the buffer's description %.0f, the lock and unlock %.0f, the arithmetic %.0f; the copy of 216 bytes out of the buffer's memory %.0f as QuvRead makes it"
               " (streaming loads: %s) against %.0f with memcpy\n", ns[0], ns[1], ns[3], nsStream, QuvSse41() ? "this processor has SSE4.1" : "NO SSE4.1 HERE, so this is memcpy too", ns[2]);
    }
    v.d->EndScene();
    printf("THE PROOF OF quad_uv: %s\n", gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
