// THE CORONA HOLD'S PROOF (2026-10-03): the shipping shader (ivt_corona_ps.h) drawn by the shipping draw
// (transport/corona_pass.h, CorHoldDraw), the shipping arithmetic (CorUnjitter, CorSameRows, CorBestPhase), and THE
// GAME'S OWN CORONA TEST run on the GPU against them, with no game. tools/test-steady and tools/test-rain are the
// pattern.
//
// THE GAME'S TEST is FusionFix's corona vertex shader (deferred_lighting, technique corona, VS11): twelve POINT
// samples of the depth on the first vertex sampler, within a pixel of the corona's projected centre; the brightness
// is the share of them farther than the centre, and at nothing visible the sprite collapses. The tool runs it twice:
// as its own port (co_test_vs.hlsl), and, where FusionFix's sources are at hand (--vs11 <path to
// deferred_lightingVS11.asm>, assembled with the system's d3dx9_43.dll), as THE REAL SHADER. The two must agree.
//
// THE SCENE, of known shape: a far background, a wall's edge, a wire thinner than a pixel, lamp heads a pixel and a
// third wide each with a corona just behind its front surface, coronas beside the wall's edge and behind the wire, one
// in the open and one behind the wall. Drawn as the game draws under the mod: every frame at that frame's sub-pixel
// offset (the jitter sequence itself), into an INTZ depth texture, with the transform carrying the same offset.
//
// THE PROOF:
//   1  THE SNAPSHOT IS THE DEPTH, BIT FOR BIT: after the pass with every pixel written, the held texture is what a
//      sampler reads from the INTZ, on every pixel.
//   2  THE GAME'S TEST READS THE HELD TEXTURE AS IT READS THE DEPTH: on the snapshot's frame, with that frame's
//      transform, every corona's brightness with the held texture on the vertex sampler is its brightness with the
//      INTZ there.
//   3  THE BLINK IS REPRODUCED: with the game's own inputs (the frame's depth, the frame's transform) the brightness
//      of the coronas at lamps, edges and the wire changes over a jitter cycle, and some go out and come back. This is
//      the fault, and the control of proof 4: a scene in which nothing blinked would prove nothing.
//   4  THE HOLD: over a whole cycle and into the next snapshot, frame by frame as the mod makes them (the scene draws
//      its depth at the frame's offset; the corona test reads the held texture with the snapshot frame's transform;
//      the pass then runs, movers alone, or the snapshot again when its phase comes round): every corona's brightness
//      on every frame is what it was on the snapshot's frame; the held texture is bit for bit the snapshot after
//      every pass (with nothing moving, the movers' pass writes nothing, and the next cycle's snapshot is the same
//      picture); and every frame's transform, its jitter taken out, is the snapshot's (CorSameRows).
//      AND WITH THE GAME'S FAR CLIP MOVING while the camera stands still (weather, the hour): the depth is then encoded
//      with another pair every frame, and so is the corona's own (c227). The hold gives the test the snapshot frame's
//      pair with its rows, and the brightness still never changes; the camera comparison leaves out the rows' third
//      element, which is all the far clip changes in them.
//   5  THE MOVERS' PASS WRITES THE MOVERS AND NOTHING ELSE: with a thing that moves in the frame (vectors), exactly the
//      pixels whose vector is at or above the threshold change, each to the frame's depth pushed farther by the push;
//      every other pixel is bit for bit what it was. A vector below the threshold is not a mover.
//   6  WHAT THE PUSH IS FOR: a corona on a mover that drives away, its centre five centimetres in front of the surface,
//      tested against the mover's depth of the frame before (a third of a metre nearer): it shows. A corona fifteen
//      metres behind the mover stays hidden.
//   7  THE SIGNATURE: the walk the mod classifies a vertex shader with (CorCountTexldl) finds twelve texldl in
//      FusionFix's corona shader and in the port, all twelve on s0; and in FusionFix's waterFx shader (VS16, read from
//      beside VS11) twelve, NONE on s0, which is why the count of all would not do.
// NEGATIVE CONTROLS, each of which must FAIL the proof named: the held texture read with the FRAME's transform instead
// of the snapshot's (proof 4: the brightness changes again); another size's texel in the pass (proof 1); vectors that
// are not trusted (proof 5: nothing is written); no push (proof 6: the corona on the mover is out); a camera turned by
// a hundredth of a pixel, and one moved two millimetres sideways (proof 4's comparison: CorSameRows says each is
// another camera); with the far clip moving, the held depth and rows with the FRAME's depth pair (proof 4: it changes).
//
//     tools\test-corona\build.ps1                          builds co.exe (x86, as the game)
//     co.exe --d3d9 <path> [--vs11 <asm>] [--timing]       <path>: DXVK's d3d9, a game folder's vulkan.dll
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
#include <string>
#include <algorithm>
#include "../../source/shaders/ivt_fs_vs.h"
#include "../../source/shaders/ivt_corona_ps.h"
#include "../../source/transport/corona_pass.h"
#include "sd_load_ps.h"                        // tools/test-steady's loader (an INTZ cannot be written from the CPU), compiled by build.ps1 into %TEMP%
#include "co_test_vs.h"                        // the port of the game's test, and the reader of its answer: likewise
#include "co_out_ps.h"

static int gFails = 0;
static void Proof(bool ok, const char* fmt, ...)
{
    char b[1400]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); if (!ok) gFails++;
}
static void Control(bool failsAsItMust, const char* fmt, ...)
{
    char b[1400]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    printf("  %s  CONTROL (%s)%s\n", failsAsItMust ? "PASS" : "FAIL", b, failsAsItMust ? ": fails the proof, as it must" : ": THE PROOF CANNOT SEE THIS FAULT");
    if (!failsAsItMust) gFails++;
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

// ---- the camera, the scene, the coronas ----------------------------------------------------------------------------
static const double kNear = 0.1, kFar = 1500.0;
struct Cam { double C[3], R[3], U[3], F[3], p00, p11, farZ; float rows[16]; float invNear, invLogRatio; };
// A camera somewhere in a world of the game's size, looking somewhere: `turn` radians more of yaw and `side` metres
// to its right (the controls); `farZ` its far plane (the game moves it with the weather).
static Cam MakeCam(int w, int h, double turn = 0.0, double side = 0.0, double farZ = kFar)
{
    Cam c; const double a = 0.7 + turn, b = 0.2;
    c.farZ = farZ;
    c.C[0] = 1234.5 + side * cos(a); c.C[1] = -876.25 - side * sin(a); c.C[2] = 45.5;
    c.F[0] = sin(a) * cos(b); c.F[1] = cos(a) * cos(b); c.F[2] = sin(b);
    c.R[0] = cos(a); c.R[1] = -sin(a); c.R[2] = 0.0;
    c.U[0] = c.R[1] * c.F[2] - c.R[2] * c.F[1]; c.U[1] = c.R[2] * c.F[0] - c.R[0] * c.F[2]; c.U[2] = c.R[0] * c.F[1] - c.R[1] * c.F[0];
    c.p11 = 1.0 / tan(0.5 * 0.9); c.p00 = c.p11 * (double)h / (double)w;
    const double A = farZ / (farZ - kNear), B = -kNear * farZ / (farZ - kNear);
    const double cr = c.C[0] * c.R[0] + c.C[1] * c.R[1] + c.C[2] * c.R[2], cu = c.C[0] * c.U[0] + c.C[1] * c.U[1] + c.C[2] * c.U[2],
                 cf = c.C[0] * c.F[0] + c.C[1] * c.F[1] + c.C[2] * c.F[2];
    for (int r = 0; r < 3; r++)
    {
        c.rows[r * 4 + 0] = (float)(c.R[r] * c.p00); c.rows[r * 4 + 1] = (float)(c.U[r] * c.p11);
        c.rows[r * 4 + 2] = (float)(c.F[r] * A);     c.rows[r * 4 + 3] = (float)c.F[r];
    }
    c.rows[12] = (float)(-cr * c.p00); c.rows[13] = (float)(-cu * c.p11); c.rows[14] = (float)(-cf * A + B); c.rows[15] = (float)(-cf);
    c.invNear = (float)(1.0 / kNear); c.invLogRatio = (float)(1.0 / log2(farZ / kNear));
    return c;
}
// THE JITTER GOES INTO THE ROWS AS hkSetVsConstF PUTS IT: each register's x and y gain its w times the offset.
static void Jitter(const float rows[16], float jnx, float jny, float out[16])
{
    for (int r = 0; r < 4; r++)
    {
        const float w = rows[r * 4 + 3];
        const float px = jnx * w, py = jny * w;
        out[r * 4 + 0] = rows[r * 4 + 0] + px; out[r * 4 + 1] = rows[r * 4 + 1] + py;
        out[r * 4 + 2] = rows[r * 4 + 2];      out[r * 4 + 3] = w;
    }
}
struct Rect { double x0, y0, x1, y1, Z; double mvx, mvy; };
struct Corona { double x, y, Z; char kind; };
struct Pic { std::vector<float> d, mvx, mvy; };
static float LogDepth(double Z, double farZ) { return (float)(log2(Z / kNear) / log2(farZ / kNear)); }
// The frame as the game draws it at an offset of (jx, jy) pixels: texel T shows the scene's point T + 0.5 - j.
static Pic Shot(const std::vector<Rect>& rs, int w, int h, double jx, double jy, double farZ = kFar)
{
    Pic p; p.d.resize((size_t)w * h); p.mvx.assign((size_t)w * h, 0.0f); p.mvy.assign((size_t)w * h, 0.0f);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x)
    {
        const double px = x + 0.5 - jx, py = y + 0.5 - jy;
        double Z = 600.0, mx = 0.0, my = 0.0;
        for (const Rect& r : rs) if (px >= r.x0 && px < r.x1 && py >= r.y0 && py < r.y1 && r.Z < Z) { Z = r.Z; mx = r.mvx; my = r.mvy; }
        const size_t i = (size_t)y * w + x;
        p.d[i] = LogDepth(Z, farZ); p.mvx[i] = (float)mx; p.mvy[i] = (float)my;
    }
    return p;
}
static uint32_t gRng = 0x13579BDFu;
static double Rnd() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFFu) / 16777216.0; }
static void Scene(int w, int h, std::vector<Rect>& rs, std::vector<Corona>& cs)
{
    rs.clear(); cs.clear(); gRng = 0x13579BDFu;
    const double edge = floor(0.30 * w) + 0.37, wire = floor(0.75 * w);
    rs.push_back({ 0.0, 0.0, edge, (double)h, 60.0, 0, 0 });                         // the wall
    rs.push_back({ wire + 0.3, floor(0.5 * h), wire + 0.7, (double)h, 80.0, 0, 0 }); // the wire, 0.4 of a pixel wide
    for (int k = 0; k < 16; k++)
    {   // lamp heads a pixel and a third wide, each with a corona 0.3 m behind its front surface
        const double x = floor(0.40 * w) + 20.0 * (k % 8) + Rnd(), y = floor(0.30 * h) + 24.0 * (k / 8) + Rnd();
        rs.push_back({ x - 0.65, y - 0.65, x + 0.65, y + 0.65, 150.0, 0, 0 });
        cs.push_back({ x, y, 150.3, 'A' });
    }
    const double offs[6] = { -1.3, -0.7, -0.2, 0.3, 0.8, 1.4 };
    for (int i = 0; i < 6; i++) cs.push_back({ edge + offs[i], floor(0.55 * h) + 16.0 * i + 0.4, 200.0, 'B' });   // beside the wall's edge
    for (int i = 0; i < 3; i++) cs.push_back({ wire + 0.1 + 0.4 * i, floor(0.55 * h) + 16.0 * i + 0.6, 200.0, 'C' });   // behind the wire
    cs.push_back({ floor(0.90 * w) + 0.3, floor(0.20 * h) + 0.6, 100.0, 'D' });       // in the open
    cs.push_back({ floor(0.10 * w) + 0.4, floor(0.30 * h) + 0.2, 200.0, 'E' });       // behind the wall
}

// ---- the device ----------------------------------------------------------------------------------------------------
struct Vtx { float p[3]; DWORD col; float uv[2]; float n[3]; };
struct Dev
{
    IDirect3DDevice9* d = nullptr; IDirect3DVertexShader9 *fsVs = nullptr, *portVs = nullptr, *realVs = nullptr;
    IDirect3DVertexDeclaration9 *fsDecl = nullptr, *corDecl = nullptr;
    IDirect3DPixelShader9 *corPs = nullptr, *load = nullptr, *outPs = nullptr;
    IDirect3DTexture9 *up = nullptr, *mv = nullptr;                          // uploads: the frame's depth as the scene draws it, its vectors
    IDirect3DTexture9 *z = nullptr; IDirect3DSurface9* zSurf = nullptr;      // the scene's own depth (INTZ)
    IDirect3DTexture9 *held = nullptr; IDirect3DSurface9* heldSurf = nullptr;// what the pass writes (R32F)
    IDirect3DTexture9 *scr = nullptr; IDirect3DSurface9* scrSurf = nullptr;  // the tool's: where the coronas' answers are drawn, and the INTZ read out
    IDirect3DSurface9* sys = nullptr;
    IDirect3DQuery9* qFreq = nullptr; IDirect3DQuery9* qDis = nullptr;
    int w = 0, h = 0;
};
static const float kQuad[6][4] = { { -1, -1, 0, 1 }, { -1, 1, 0, 1 }, { 1, 1, 0, 1 }, { -1, -1, 0, 1 }, { 1, 1, 0, 1 }, { 1, -1, 0, 1 } };
static const D3DFORMAT kIntz = (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z');
static const float kStill2 = 0.05f * 0.05f;       // steady_depth_pass.h, kSdStillPx squared: what the mod passes

static void Size(Dev& v, int w, int h)
{
    for (IUnknown* u : { (IUnknown*)v.up, (IUnknown*)v.mv, (IUnknown*)v.zSurf, (IUnknown*)v.z, (IUnknown*)v.heldSurf, (IUnknown*)v.held,
                         (IUnknown*)v.scrSurf, (IUnknown*)v.scr, (IUnknown*)v.sys }) if (u) u->Release();
    v.w = w; v.h = h;
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &v.up, nullptr));
    CK(v.d->CreateTexture(w, h, 1, 0, D3DFMT_G16R16F, D3DPOOL_MANAGED, &v.mv, nullptr));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_DEPTHSTENCIL, kIntz, D3DPOOL_DEFAULT, &v.z, nullptr));           CK(v.z->GetSurfaceLevel(0, &v.zSurf));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &v.held, nullptr));  CK(v.held->GetSurfaceLevel(0, &v.heldSurf));
    CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &v.scr, nullptr));   CK(v.scr->GetSurfaceLevel(0, &v.scrSurf));
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
    d->SetVertexShader(v.fsVs); d->SetVertexDeclaration(v.fsDecl);
    const float zero[9][4] = {}; d->SetPixelShaderConstantF(0, &zero[0][0], 9);
    for (int t = 0; t < 3; t++)
    {
        d->SetSamplerState(t, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(t, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        d->SetSamplerState(t, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        d->SetSamplerState(t, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(t, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    }
}
static void Upload(Dev& v, const Pic& p)
{
    D3DLOCKED_RECT lr; CK(v.up->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < v.h; ++y) memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, &p.d[(size_t)y * v.w], sizeof(float) * v.w);
    v.up->UnlockRect(0);
    CK(v.mv->LockRect(0, &lr, nullptr, 0));
    for (int y = 0; y < v.h; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
        for (int x = 0; x < v.w; ++x) { row[2 * x] = ToHalf(p.mvx[(size_t)y * v.w + x]); row[2 * x + 1] = ToHalf(p.mvy[(size_t)y * v.w + x]); } }
    v.mv->UnlockRect(0);
}
static std::vector<float> Read(Dev& v, IDirect3DSurface9* s)
{
    CK(v.d->GetRenderTargetData(s, v.sys));
    std::vector<float> out((size_t)v.w * v.h); D3DLOCKED_RECT lr; CK(v.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < v.h; ++y) memcpy(&out[(size_t)y * v.w], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, sizeof(float) * v.w);
    v.sys->UnlockRect(); return out;
}
// The tool's loader draws texture `from` into a colour target, or into the INTZ as depth.
static void Blit(Dev& v, IDirect3DTexture9* from, IDirect3DSurface9* colour, bool toDepth)
{
    Common(v);
    const float c0[4] = { 1.0f / (float)v.w, 1.0f / (float)v.h, 0.0f, 0.0f };
    const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f };
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
// THE SCENE DRAWS ITS DEPTH: the frame's picture into the INTZ.
static void SceneDraws(Dev& v, const Pic& p) { Upload(v, p); Blit(v, v.up, v.scrSurf, true); }
static std::vector<float> ReadIntz(Dev& v) { Blit(v, v.z, v.scrSurf, false); return Read(v, v.scrSurf); }

// THE MOD'S PASS. `fault`: 0 none (CorHoldDraw itself), 1 another size's texel.
static void Pass(Dev& v, const Cam& cam, bool all, bool trust, float push, int fault = 0)
{
    Common(v);
    if (!fault) CK(CorHoldDraw(v.d, v.z, v.mv, v.heldSurf, v.corPs, (UINT)v.w, (UINT)v.h, all, trust, cam.invNear, cam.invLogRatio, push, kStill2, kQuad, sizeof(kQuad[0])));
    else
    {   // CorHoldDraw's own calls, with the one fault
        const float c[2][4] = { { 1.0f / (float)(v.w + 3), 1.0f / (float)(v.h + 2), all ? 1.0f : 0.0f, kStill2 }, { cam.invNear, cam.invLogRatio, push, trust ? 1.0f : 0.0f } };
        const D3DVIEWPORT9 vp = { 0, 0, (DWORD)v.w, (DWORD)v.h, 0.0f, 1.0f };
        v.d->SetRenderTarget(0, v.heldSurf); v.d->SetViewport(&vp); v.d->SetTexture(0, v.z); v.d->SetTexture(1, v.mv);
        v.d->SetPixelShaderConstantF(0, &c[0][0], 2); v.d->SetPixelShader(v.corPs);
        CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kQuad, sizeof(kQuad[0])));
    }
    v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); v.d->SetRenderTarget(0, v.scrSurf);
}

// THE GAME'S CORONA DRAW: every corona a sprite of two triangles whose four vertices carry the centre in the normal,
// drawn with the corona vertex shader `vs`, the depth `depthTex` on the first vertex sampler (POINT), the transform
// `rows` in c8..c11. Returns each corona's brightness as the shader computed it: 0 where the sprite collapsed.
static const float kPoisson[12][2] = { { -0.326f, -0.406f }, { -0.840f, -0.074f }, { -0.696f, 0.457f }, { -0.203f, 0.621f }, { 0.962f, -0.195f }, { 0.473f, -0.480f },
                                       { 0.519f, 0.767f }, { 0.185f, -0.893f }, { 0.507f, 0.064f }, { 0.896f, 0.412f }, { -0.322f, -0.933f }, { -0.792f, -0.598f } };
// `lgOf`: the camera whose depth pair goes to c227 (the frame's own, or the snapshot's).
static std::vector<float> Test(Dev& v, IDirect3DVertexShader9* vs, IDirect3DBaseTexture9* depthTex, const float rows[16], const Cam& cam, const std::vector<Corona>& cs,
                               const Cam* lgOf = nullptr)
{
    if (!lgOf) lgOf = &cam;
    const int w = v.w, h = v.h;
    std::vector<Vtx> vb; std::vector<uint16_t> ib;
    for (const Corona& c : cs)
    {
        const double nx = 2.0 * c.x / w - 1.0, ny = 1.0 - 2.0 * c.y / h;
        const double X = nx * c.Z / cam.p00, Y = ny * c.Z / cam.p11;
        double P[3]; for (int k = 0; k < 3; k++) P[k] = cam.C[k] + X * cam.R[k] + Y * cam.U[k] + c.Z * cam.F[k];
        const double sx = 3.0 * 2.0 * c.Z / (w * cam.p00), sy = 3.0 * 2.0 * c.Z / (h * cam.p11);     // three pixels to each side
        const uint16_t base = (uint16_t)vb.size();
        for (int q = 0; q < 4; q++)
        {
            const double a = (q & 1) ? 1.0 : -1.0, b = (q & 2) ? 1.0 : -1.0;
            Vtx t; for (int k = 0; k < 3; k++) { t.p[k] = (float)(P[k] + a * sx * cam.R[k] + b * sy * cam.U[k]); t.n[k] = (float)P[k]; }
            t.col = ((q & 2) ? 0xFF000000u : 0u) | 0x00FFFFFFu; t.uv[0] = (q & 1) ? 1.0f : 0.0f; t.uv[1] = 1.0f;
            vb.push_back(t);
        }
        const uint16_t idx[6] = { 0, 1, 2, 2, 1, 3 }; for (int k = 0; k < 6; k++) ib.push_back((uint16_t)(base + idx[k]));
    }
    IDirect3DDevice9* d = v.d;
    Common(v);
    const D3DVIEWPORT9 vp = { 0, 0, (DWORD)w, (DWORD)h, 0.0f, 1.0f };
    d->SetRenderTarget(0, v.scrSurf); d->SetViewport(&vp);
    d->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.0f, 0);
    d->SetVertexShader(vs); d->SetVertexDeclaration(v.corDecl); d->SetPixelShader(v.outPs);
    const float ident[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    const float wv[4][4] = { { (float)cam.R[0], (float)cam.U[0], (float)cam.F[0], 0 }, { (float)cam.R[1], (float)cam.U[1], (float)cam.F[1], 0 },
                             { (float)cam.R[2], (float)cam.U[2], (float)cam.F[2], 0 }, { 0, 0, 0, 1 } };
    const float vi[4][4] = { { (float)cam.R[0], (float)cam.R[1], (float)cam.R[2], 0 }, { (float)cam.U[0], (float)cam.U[1], (float)cam.U[2], 0 },
                             { (float)cam.F[0], (float)cam.F[1], (float)cam.F[2], 0 }, { (float)cam.C[0], (float)cam.C[1], (float)cam.C[2], 1 } };
    const float scr[4] = { (float)w, (float)h, 1.0f / (float)w, 1.0f / (float)h };
    const float lg[4] = { lgOf->invNear, lgOf->invLogRatio, (float)(lgOf->farZ / kNear), (float)kNear };
    const float zero[2][4] = {};
    float po[12][4] = {}; for (int k = 0; k < 12; k++) { po[k][0] = kPoisson[k][0]; po[k][1] = kPoisson[k][1]; }
    d->SetVertexShaderConstantF(0, &ident[0][0], 4); d->SetVertexShaderConstantF(4, &wv[0][0], 4);
    d->SetVertexShaderConstantF(8, rows, 4);         d->SetVertexShaderConstantF(12, &vi[0][0], 4);
    d->SetVertexShaderConstantF(44, scr, 1);         d->SetVertexShaderConstantF(208, &po[0][0], 12);
    d->SetVertexShaderConstantF(227, lg, 1);         d->SetVertexShaderConstantF(235, &zero[0][0], 2);       // c236.y = 0: the fog branch is off
    d->SetTexture(D3DVERTEXTEXTURESAMPLER0, depthTex);
    d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    CK(d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, (UINT)vb.size(), (UINT)ib.size() / 3, ib.data(), D3DFMT_INDEX16, vb.data(), sizeof(Vtx)));
    d->SetTexture(D3DVERTEXTEXTURESAMPLER0, nullptr);
    const std::vector<float> img = Read(v, v.scrSurf);
    std::vector<float> out(cs.size());
    for (size_t k = 0; k < cs.size(); k++) out[k] = img[(size_t)floor(cs[k].y) * w + (size_t)floor(cs[k].x)];
    return out;
}
// the frame's offset, in pixels and as the transform carries it
struct Jit { double px, py; float nx, ny; };
static Jit JitOf(int phase, int w, int h)
{
    Jit j; const float fx = CorHalton(phase + 1, 2) - 0.5f, fy = CorHalton(phase + 1, 3) - 0.5f;
    j.px = fx; j.py = fy; j.nx = 2.0f * fx / (float)w; j.ny = -2.0f * fy / (float)h;
    return j;
}
static bool SameBits(const std::vector<float>& a, const std::vector<float>& b) { return a.size() == b.size() && memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0; }

// ---- proofs 1 to 4 for one cycle length and one corona shader ----------------------------------------------------------
// `drift`: by how much of itself the far clip grows every frame (0: it stands).
static void Hold(Dev& v, int phases, IDirect3DVertexShader9* vs, const char* which, std::vector<float>* answers, double drift = 0.0)
{
    const int w = v.w, h = v.h;
    std::vector<Rect> rs; std::vector<Corona> cs; Scene(w, h, rs, cs);
    const Cam cam = MakeCam(w, h);                // the snapshot frame's camera
    const float push = kCorMoverSpeed / 120.0f;
    const int best = CorBestPhase(phases);
    // the snapshot's frame
    const Jit js = JitOf(best, w, h);
    SceneDraws(v, Shot(rs, w, h, js.px, js.py));
    float rowsS[16], uS[16]; Jitter(cam.rows, js.nx, js.ny, rowsS); CorUnjitter(rowsS, js.nx, js.ny, uS);
    const std::vector<float> ownS = Test(v, vs, v.z, rowsS, cam, cs);
    Pass(v, cam, true, true, push);
    const std::vector<float> snap = Read(v, v.heldSurf), intz = ReadIntz(v);
    long notDepth = 0; for (size_t i = 0; i < snap.size(); i++) if (memcmp(&snap[i], &intz[i], 4) != 0) notDepth++;
    const std::vector<float> heldS = Test(v, vs, v.held, rowsS, cam, cs);
    long readsOther = 0; for (size_t k = 0; k < cs.size(); k++) if (heldS[k] != ownS[k]) readsOther++;
    // the cycle, and the snapshot's phase once more at its end
    std::vector<float> lo = ownS, hi = ownS, wlo, whi, plo, phi;
    long holdBad = 0, heldChanged = 0, rowsOther = 0, frames = 0;
    for (int step = 1; step <= phases; step++)
    {
        const int p = (best + step) % phases;
        const Jit j = JitOf(p, w, h);
        const Cam camF = MakeCam(w, h, 0.0, 0.0, kFar * (1.0 + drift * step));     // the same camera; the far clip has moved
        SceneDraws(v, Shot(rs, w, h, j.px, j.py, camF.farZ));
        float rowsF[16], uF[16]; Jitter(camF.rows, j.nx, j.ny, rowsF); CorUnjitter(rowsF, j.nx, j.ny, uF);
        if (!CorSameRows(uF, uS)) rowsOther++;
        const std::vector<float> own = Test(v, vs, v.z, rowsF, camF, cs);         // the game's own inputs: the fault
        const std::vector<float> hold = Test(v, vs, v.held, rowsS, cam, cs);      // the hold: the snapshot's depth, rows and depth pair
        const std::vector<float> wrong = Test(v, vs, v.held, rowsF, cam, cs);     // the control: the held depth with the frame's transform
        const std::vector<float> pair = Test(v, vs, v.held, rowsS, cam, cs, &camF);   // the control: the held depth and rows with the FRAME's depth pair
        if (wlo.empty()) { wlo = wrong; whi = wrong; plo = pair; phi = pair; }
        for (size_t k = 0; k < cs.size(); k++)
        {
            lo[k] = std::min(lo[k], own[k]); hi[k] = std::max(hi[k], own[k]);
            wlo[k] = std::min(wlo[k], wrong[k]); whi[k] = std::max(whi[k], wrong[k]);
            plo[k] = std::min(plo[k], pair[k]); phi[k] = std::max(phi[k], pair[k]);
            if (hold[k] != ownS[k]) holdBad++;
        }
        const bool again = p == best;
        Pass(v, camF, again, true, push);                                          // the body: the movers alone, or the snapshot again
        // the movers' pass with nothing moving writes nothing. The next snapshot is the same picture where the far
        // clip stood, and that frame's depth, bit for bit, where it moved.
        const std::vector<float> now = Read(v, v.heldSurf);
        if (!again || drift == 0.0) { if (!SameBits(now, snap)) heldChanged++; }
        else if (!SameBits(now, ReadIntz(v))) heldChanged++;
        frames++;
    }
    long blink = 0, out = 0, wrongBlink = 0, calmWrong = 0, pairBlink = 0;
    for (size_t k = 0; k < cs.size(); k++)
    {
        const bool edgeKind = cs[k].kind == 'A' || cs[k].kind == 'B' || cs[k].kind == 'C';
        if (hi[k] != lo[k]) { if (edgeKind) blink++; else calmWrong++; }
        if (hi[k] > 0.0f && lo[k] == 0.0f && edgeKind) out++;
        if (whi[k] != wlo[k]) wrongBlink++;
        if (phi[k] != plo[k]) pairBlink++;
    }
    char tag[96]; _snprintf_s(tag, sizeof(tag), _TRUNCATE, drift > 0.0 ? "%s, the far clip moving" : "%s", which);
    which = tag;
    Proof(notDepth == 0, "1  [%s, a cycle of %d] the snapshot is the depth, bit for bit: %ld of %ld pixels of the held texture are not what a sampler reads from the INTZ", which, phases, notDepth, (long)snap.size());
    Proof(readsOther == 0, "2  [%s, %d] the game's test reads the held texture as it reads the depth: on the snapshot's frame %ld of %ld coronas differ between the two", which, phases, readsOther, (long)cs.size());
    Proof(blink >= 5 && out >= 1 && calmWrong == 0, "3  [%s, %d] the blink, reproduced with the game's own inputs: %ld of 25 coronas at lamps, edges and the wire change brightness over the cycle, %ld of them go out and come back;"
          " the one in the open and the one behind the wall never change (%ld did)", which, phases, blink, out, calmWrong);
    Proof(holdBad == 0 && heldChanged == 0 && rowsOther == 0,
          "4  [%s, %d] the hold, %ld frames: %ld corona-frames differ from the snapshot frame's brightness (of %ld); the held texture differs from the snapshot after %ld passes (the movers' pass with nothing moving, and the next snapshot);"
          " the transform without its jitter is another camera's on %ld frames", which, phases, frames, holdBad, frames * (long)cs.size(), heldChanged, rowsOther);
    Control(wrongBlink > 0, "[%s, %d] the held depth read with the frame's own transform: %ld coronas change brightness again", which, phases, wrongBlink);
    if (drift > 0.0) Control(pairBlink > 0, "[%s, %d] the held depth and rows with the frame's own depth pair (c227): %ld coronas change brightness", which, phases, pairBlink);
    if (answers) *answers = ownS;
}

static double Timed(Dev& v, const Cam& cam, bool all, int frames)
{
    for (int i = 0; i < 8; ++i) Pass(v, cam, all, true, 0.33f);
    UINT64 f = 0; BOOL dis = FALSE;
    IDirect3DQuery9 *t0 = nullptr, *t1 = nullptr;
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t0); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &t1);
    if (!t0 || !t1 || !v.qFreq || !v.qDis) return -1.0;
    v.qDis->Issue(D3DISSUE_BEGIN); v.qFreq->Issue(D3DISSUE_END);
    t0->Issue(D3DISSUE_END);
    for (int i = 0; i < frames; ++i) { Common(v); CorHoldDraw(v.d, v.z, v.mv, v.heldSurf, v.corPs, (UINT)v.w, (UINT)v.h, all, true, cam.invNear, cam.invLogRatio, 0.33f, kStill2, kQuad, sizeof(kQuad[0])); }
    t1->Issue(D3DISSUE_END); v.qDis->Issue(D3DISSUE_END);
    UINT64 a = 0, b = 0;
    while (t0->GetData(&a, sizeof(a), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (t1->GetData(&b, sizeof(b), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (v.qFreq->GetData(&f, sizeof(f), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    while (v.qDis->GetData(&dis, sizeof(dis), D3DGETDATA_FLUSH) == S_FALSE) Sleep(0);
    t0->Release(); t1->Release();
    v.d->SetTexture(0, nullptr); v.d->SetTexture(1, nullptr); v.d->SetRenderTarget(0, v.scrSurf);
    if (dis || !f) return -1.0;
    return (double)(b - a) * 1000.0 / (double)f / (double)frames;
}

static void Proofs(Dev& v, int w, int h)
{
    printf(" at %dx%d\n", w, h);
    Size(v, w, h);
    const int cycles[3] = { 8, 18, 32 };
    for (int c = 0; c < 3; c++)
    {
        std::vector<float> port, real;
        Hold(v, cycles[c], v.portVs, "the port", &port);
        if (v.realVs)
        {
            Hold(v, cycles[c], v.realVs, "FusionFix's own shader", &real);
            long differ = 0; for (size_t k = 0; k < port.size(); k++) if (fabsf(port[k] - real[k]) > 1.0e-4f) differ++;
            Proof(differ == 0, "   [a cycle of %d] the port and FusionFix's own shader give the same brightness on the snapshot's frame for every corona (%ld of %ld differ)", cycles[c], differ, (long)port.size());
        }
    }
    // the far clip a fifth of a percent farther every frame, a cycle of 18
    Hold(v, 18, v.portVs, "the port", nullptr, 0.002);
    if (v.realVs) Hold(v, 18, v.realVs, "FusionFix's own shader", nullptr, 0.002);
    const Cam cam = MakeCam(w, h);
    {   // the comparison's own control: a camera turned by a hundredth of a pixel
        const double turn = 0.01 * 2.0 / ((double)w * cam.p00);
        const Cam cam2 = MakeCam(w, h, turn);
        const Jit j = JitOf(3, w, h); float r[16], u[16]; Jitter(cam.rows, j.nx, j.ny, r); CorUnjitter(r, j.nx, j.ny, u);
        Proof(CorSameRows(u, cam.rows), "4  the transform with a jitter put in and taken out is the camera's own (CorSameRows)");
        Control(!CorSameRows(u, cam2.rows), "a camera turned by a hundredth of a pixel (%.2e radians): CorSameRows says it is another camera", turn);
        const Cam cam3 = MakeCam(w, h, 0.0, 0.002);
        Control(!CorSameRows(u, cam3.rows), "a camera moved two millimetres sideways, 1500 m from the world's origin: CorSameRows says it is another camera (apart by %.1e of the element; the same camera: up to %.0e)",
                (double)CorRowsApart(u, cam3.rows), (double)kCorSame);
        const Cam cam4 = MakeCam(w, h, 0.0, 0.0, kFar * 1.3);
        Proof(CorSameRows(u, cam4.rows), "4  the same camera with its far clip moved by a third is the same camera (the rows' third element is not compared)");
    }
    std::vector<Rect> rs; std::vector<Corona> cs; Scene(w, h, rs, cs);
    const float push = kCorMoverSpeed / 120.0f;
    {   // another size's texel: proof 1's control
        const Jit j = JitOf(0, w, h);
        SceneDraws(v, Shot(rs, w, h, j.px, j.py));
        Pass(v, cam, true, true, push, 1);
        const std::vector<float> got = Read(v, v.heldSurf), intz = ReadIntz(v);
        long bad = 0; for (size_t i = 0; i < got.size(); i++) if (memcmp(&got[i], &intz[i], 4) != 0) bad++;
        Control(bad > 0, "another size's texel in the pass: %ld pixels of the held texture are not the depth", bad);
    }
    {   // proof 5: the movers' pass
        const Jit j0 = JitOf(0, w, h), j1 = JitOf(5, w, h);
        SceneDraws(v, Shot(rs, w, h, j0.px, j0.py));
        Pass(v, cam, true, true, push);
        const std::vector<float> before = Read(v, v.heldSurf);
        std::vector<Rect> rm = rs;
        const double mx = floor(0.50 * w), my = floor(0.75 * h);
        rm.push_back({ mx, my, mx + 40.0, my + 20.0, 30.0, -5.0, 0.0 });                  // a thing that moves five pixels a frame
        rm.push_back({ mx + 60.0, my, mx + 80.0, my + 20.0, 30.0, 0.03, 0.0 });           // a thing whose vector is under the threshold
        const Pic pm = Shot(rm, w, h, j1.px, j1.py);
        SceneDraws(v, pm);
        const std::vector<float> intz = ReadIntz(v);
        Pass(v, cam, false, false, push);
        const bool untrusted = SameBits(Read(v, v.heldSurf), before);
        Pass(v, cam, false, true, push);
        const std::vector<float> after = Read(v, v.heldSurf);
        long movers = 0, moverWrong = 0, staticChanged = 0, slow = 0, slowChanged = 0; double worst = 0.0;
        for (size_t i = 0; i < after.size(); i++)
        {
            const float v2 = pm.mvx[i] * pm.mvx[i] + pm.mvy[i] * pm.mvy[i];
            if (v2 >= kStill2)
            {
                movers++;
                const double wv = exp2((double)intz[i] / (double)cam.invLogRatio) / (double)cam.invNear;
                const double want = log2((wv + (double)push) * (double)cam.invNear) * (double)cam.invLogRatio;
                const double err = fabs((double)after[i] - want); worst = std::max(worst, err);
                if (err > 2.0e-5 || !(after[i] > intz[i])) moverWrong++;
            }
            else
            {
                if (v2 > 0.0f) slow++;
                if (memcmp(&after[i], &before[i], 4) != 0) { staticChanged++; if (v2 > 0.0f) slowChanged++; }
            }
        }
        Proof(movers > 500 && moverWrong == 0 && staticChanged == 0 && slow > 300 && slowChanged == 0,
              "5  the movers' pass writes the movers and nothing else: %ld pixels of the thing that moves hold the frame's depth pushed farther by %.3f m (worst %.1e from the arithmetic; %ld are not);"
              " %ld other pixels changed (0 wanted), among them %ld of the %ld pixels whose vector is under the threshold", movers, (double)push, worst, moverWrong, staticChanged, slowChanged, slow);
        Control(untrusted, "vectors that are not trusted: the movers' pass %s", untrusted ? "writes nothing" : "WROTE");
    }
    {   // proof 6: what the push is for
        const Jit j = JitOf(CorBestPhase(8), w, h);
        std::vector<Rect> rm = rs;
        const double mx = floor(0.50 * w), my = floor(0.75 * h);
        rm.push_back({ mx, my, mx + 40.0, my + 20.0, 30.0, 0.0, -1.0 });                  // a car a frame ago: 30 m, and it drives away
        std::vector<Corona> cm;
        cm.push_back({ mx + 12.3, my + 10.4, 30.25, 'M' });        // its tail light now: the surface is at 30.3 m, the corona 5 cm in front
        cm.push_back({ mx + 28.6, my + 10.7, 45.0, 'N' });         // a lamp 15 m behind it
        float rowsS[16]; Jitter(cam.rows, j.nx, j.ny, rowsS);
        SceneDraws(v, Shot(rm, w, h, j.px, j.py));
        Pass(v, cam, true, true, push);
        const std::vector<float> with = Test(v, v.portVs, v.held, rowsS, cam, cm);
        Pass(v, cam, true, true, 0.0f);
        const std::vector<float> without = Test(v, v.portVs, v.held, rowsS, cam, cm);
        Proof(with[0] > 0.99f && with[1] == 0.0f, "6  what the push is for: against the mover's depth of the frame before, pushed by %.3f m, the corona on the mover has brightness %.3f (1 wanted) and the lamp 15 m behind it %.3f (0 wanted)",
              (double)push, (double)with[0], (double)with[1]);
        Control(without[0] == 0.0f, "no push: the corona on the mover has brightness %.3f", (double)without[0]);
    }
}

struct ID3DXBufferLite : public IUnknown { virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0; virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0; };
typedef HRESULT (WINAPI* PFN_ASM)(LPCSTR, UINT, const void*, void*, DWORD, ID3DXBufferLite**, ID3DXBufferLite**);
// FusionFix's own corona vertex shader, assembled from its source text with the system's D3DX. nullptr if either is missing.
static IDirect3DVertexShader9* RealShader(IDirect3DDevice9* d, const char* path, bool quiet = false)
{
    FILE* f = nullptr; if (fopen_s(&f, path, "rb") != 0 || !f) { if (!quiet) printf("  (FusionFix's shader source was not found at %s: the port alone is run)\n", path); return nullptr; }
    std::string src; char buf[4096]; size_t n; while ((n = fread(buf, 1, sizeof(buf), f)) > 0) src.append(buf, n); fclose(f);
    HMODULE x = LoadLibraryA("d3dx9_43.dll");
    PFN_ASM as = x ? (PFN_ASM)GetProcAddress(x, "D3DXAssembleShader") : nullptr;
    if (!as) { printf("  (d3dx9_43.dll is not on this machine: the port alone is run)\n"); return nullptr; }
    ID3DXBufferLite *code = nullptr, *err = nullptr;
    const HRESULT hr = as(src.data(), (UINT)src.size(), nullptr, nullptr, 0, &code, &err);
    if (FAILED(hr) || !code) { printf("  FAIL  FusionFix's shader did not assemble (0x%08lX): %s\n", (unsigned long)hr, err ? (const char*)err->GetBufferPointer() : ""); gFails++; return nullptr; }
    IDirect3DVertexShader9* vs = nullptr;
    if (FAILED(d->CreateVertexShader((const DWORD*)code->GetBufferPointer(), &vs))) { printf("  FAIL  FusionFix's shader was refused by the device\n"); gFails++; vs = nullptr; }
    code->Release(); if (err) err->Release();
    return vs;
}

int main(int argc, char** argv)
{
    const char* dll = nullptr; const char* vs11 = nullptr; bool timing = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i];
        else if (!strcmp(argv[i], "--vs11") && i + 1 < argc) vs11 = argv[++i];
        else if (!strcmp(argv[i], "--timing")) timing = true;
    }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The corona hold's proof: the shipping shader, draw and arithmetic, and the game's corona test on the GPU, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "coproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("coproof", "co", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);
    Dev v;
    D3DPRESENT_PARAMETERS pp = {}; pp.Windowed = TRUE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.BackBufferWidth = 64; pp.BackBufferHeight = 64;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8; pp.hDeviceWindow = hw;
    CK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hw, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &pp, &v.d));
    CK(v.d->CreateVertexShader((const DWORD*)kIvtFsVS, &v.fsVs));
    CK(v.d->CreateVertexShader((const DWORD*)kCoTestVS, &v.portVs));
    D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(el, &v.fsDecl));
    D3DVERTEXELEMENT9 ce[] = { { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
                               { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, { 0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 }, D3DDECL_END() };
    CK(v.d->CreateVertexDeclaration(ce, &v.corDecl));
    CK(v.d->CreatePixelShader((const DWORD*)kIvtCoronaPS, &v.corPs));
    CK(v.d->CreatePixelShader((const DWORD*)kSdLoadPS, &v.load));
    CK(v.d->CreatePixelShader((const DWORD*)kCoOutPS, &v.outPs));
    if (vs11) v.realVs = RealShader(v.d, vs11);
    else printf("  (no --vs11: FusionFix's own shader is not run, the port alone is)\n");
    {   // proof 7: the signature
        auto count = [](IDirect3DVertexShader9* s, int* onS0) -> int
        {
            UINT size = 0; *onS0 = 0;
            if (!s || FAILED(s->GetFunction(nullptr, &size)) || size < 8) return -1;
            std::vector<DWORD> tok(size / 4 + 1);
            if (FAILED(s->GetFunction(tok.data(), &size))) return -1;
            return CorCountTexldl(tok.data(), size / 4, onS0);
        };
        int p0 = 0, r0 = 0, w0 = 0;
        const int pAll = count(v.portVs, &p0);
        Proof(pAll == 12 && p0 == 12, "7  the signature, the port: %d texldl, %d of them on s0 (12 and 12 wanted)", pAll, p0);
        if (v.realVs)
        {
            const int rAll = count(v.realVs, &r0);
            Proof(rAll == 12 && r0 == 12, "7  the signature, FusionFix's corona shader (VS11): %d texldl, %d of them on s0 (12 and 12 wanted)", rAll, r0);
            std::string water(vs11); const size_t at = water.rfind("VS11");
            IDirect3DVertexShader9* ws = nullptr;
            if (at != std::string::npos) { water.replace(at, 4, "VS16"); ws = RealShader(v.d, water.c_str(), true); }
            if (ws)
            {
                const int wAll = count(ws, &w0);
                Control(wAll >= 12 && w0 < 12, "FusionFix's waterFx shader (VS16): %d texldl, %d of them on s0: the count of all would take it for a corona, the count on s0 does not", wAll, w0);
                ws->Release();
            }
            else printf("  (FusionFix's waterFx shader was not found beside VS11: its control is not run)\n");
        }
    }
    v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &v.qFreq); v.d->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &v.qDis);
    CK(v.d->BeginScene());

    Proof(CorBestPhase(8) == 0 && CorBestPhase(18) == 0 && CorBestPhase(32) == 21,
          "   the phase with the smallest jitter: %d of 8, %d of 18, %d of 32 (0, 0 and 21 wanted: a sixth of a pixel, and a tenth)", CorBestPhase(8), CorBestPhase(18), CorBestPhase(32));
    Proofs(v, 427, 240);
    Proofs(v, 1707, 960);

    if (timing)
    {
        const int sizes[3][2] = { { 3840, 2160 }, { 2560, 1440 }, { 1920, 1080 } };
        for (int i = 0; i < 3; ++i)
        {
            const int w = sizes[i][0], h = sizes[i][1];
            Size(v, w, h);
            const Cam cam = MakeCam(w, h);
            std::vector<Rect> rs; std::vector<Corona> cs; Scene(w, h, rs, cs);
            double ms[4] = {};
            const double shares[3] = { 0.0, 0.02, 0.10 };
            for (int k = 0; k < 3; ++k)
            {
                std::vector<Rect> rm = rs;
                if (shares[k] > 0.0) { const double side = sqrt(shares[k] * (double)w * (double)h); rm.push_back({ 0.4 * w, 0.2 * h, 0.4 * w + side, 0.2 * h + side, 30.0, -5.0, 0.0 }); }
                SceneDraws(v, Shot(rm, w, h, 0.0, 0.0));
                if (k == 0) ms[3] = Timed(v, cam, true, 400);
                ms[k] = Timed(v, cam, false, 400);
            }
            printf("  TIMING at %dx%d on this card's own clock (ms, the pass alone): the snapshot (every pixel, once a jitter cycle) %.4f; the movers' pass (every other frame of a hold) with nothing moving %.4f,"
                   " with movers on 2 percent of the screen %.4f, on 10 percent %.4f; the held texture is %.1f MiB\n", w, h, ms[3], ms[0], ms[1], ms[2], 4.0 * (double)w * (double)h / 1048576.0);
        }
    }
    v.d->EndScene();
    printf("THE CORONA HOLD'S PROOF: %s\n", gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
