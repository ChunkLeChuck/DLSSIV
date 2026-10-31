// X0, THE HEADLESS XeSS SMOKE TEST (2026-10-27; docs/reports/xess-plan-2026-10-17, PLAN.md X0 and UPDATE-2026-10-27.md).
//
// A DESK TOOL, NEVER IN THE SHIPPED SHAPE: no game, no pipe, no window. It makes a Direct3D 12 device on the machine's first
// hardware adapter, loads Intel's libxess.dll BY ITS FULL PATH (LoadLibrary and GetProcAddress, no import library, as the
// helper's arm will), and answers what the plan's X0 asks before any production arm is written:
//   1. THE FILE LOADS AND CREATES on this device; its version; whether the driver is the one it prefers.
//   2. THE QUALITY SETTINGS' INPUT RANGES at the build's output size (the table X2's PickXessQuality picks from).
//   3. THE COLOUR FORMATS the game sends: R16G16B16A16_FLOAT (HDR, scRGB) and B8G8R8A8_UNORM (SDR), with R8G8B8A8_UNORM (the
//      guide's own example) beside them. Each: Init, four executes, the output not black.
//   4. THE MOTION VECTORS' SIGN AND UNITS, by two arms on an analytic pattern moving at a known rate (render pixels): A hands
//      XeSS the vector as the mod's guides write it, previous minus current, in render pixels (ivt_guides_ps.hlsl); B the
//      same negated. The true picture is known exactly at the output size, so each arm's error is measured, not judged.
//   5. THE JITTER'S SIGN, the same way on a still pattern: each input pixel samples the pattern at its centre MINUS (Jx, Jy),
//      which is where the mod's projection puts the sample (DLSS-IV.cpp, "WHERE THE JITTER GOES": jx_ndc = +2 Jx / w,
//      jy_ndc = -2 Jy / h); A hands XeSS (Jx, Jy) as the mod hands DLSS and FSR, B the pair negated.
//   6. THE PER-EXECUTE GPU TIME on this card at the chosen setting (GPU timestamps around xessD3D12Execute; median, p99).
// One text file of verdict lines (--out); exit 0 when every question got an answer, 1 when one did not, 2 on a usage error.
// tools/test-xess/gen.py builds and runs it and holds the verdicts; B in 4 and 5 is the control each proof must beat.
//
// Build (gen.py does it): cl /O2 /EHsc /MT /std:c++17 /I<external\xess\inc> xess_smoke.cpp d3d12.lib dxgi.lib
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include "xess/xess.h"
#include "xess/xess_d3d12.h"

// ------------------------------------------------------------------------------------------------------------ the report
static FILE* gOut = nullptr;
static void Say(const char* fmt, ...)
{
    char b[2048]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
    fputs(b, stdout); fputc('\n', stdout);
    if (gOut) { fputs(b, gOut); fputc('\n', gOut); fflush(gOut); }
}
static int gUnanswered = 0;

// ------------------------------------------------------------------------------------------------------------ Intel's entry points
#define XFN(n) static decltype(&n) p_##n = nullptr
XFN(xessGetVersion); XFN(xessD3D12CreateContext); XFN(xessD3D12BuildPipelines); XFN(xessD3D12Init); XFN(xessD3D12Execute);
XFN(xessDestroyContext); XFN(xessGetOptimalInputResolution); XFN(xessGetProperties); XFN(xessIsOptimalDriver);
XFN(xessSetVelocityScale); XFN(xessSetJitterScale); XFN(xessSetLoggingCallback); XFN(xessGetPipelineBuildStatus);
static int gXessErrors = 0;
static void XessLog(const char* m, xess_logging_level_t lv)
{
    if (lv >= XESS_LOGGING_LEVEL_WARNING) { Say("  libxess %s: %s", lv == XESS_LOGGING_LEVEL_ERROR ? "error" : "warning", m); if (lv == XESS_LOGGING_LEVEL_ERROR) gXessErrors++; }
}
static const char* XRes(xess_result_t r)
{
    switch (r)
    {
    case XESS_RESULT_SUCCESS: return "SUCCESS";
    case XESS_RESULT_WARNING_NONEXISTING_FOLDER: return "WARNING_NONEXISTING_FOLDER";
    case XESS_RESULT_WARNING_OLD_DRIVER: return "WARNING_OLD_DRIVER";
    case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: return "ERROR_UNSUPPORTED_DEVICE";
    case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: return "ERROR_UNSUPPORTED_DRIVER";
    case XESS_RESULT_ERROR_UNINITIALIZED: return "ERROR_UNINITIALIZED";
    case XESS_RESULT_ERROR_INVALID_ARGUMENT: return "ERROR_INVALID_ARGUMENT";
    case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY: return "ERROR_DEVICE_OUT_OF_MEMORY";
    case XESS_RESULT_ERROR_DEVICE: return "ERROR_DEVICE";
    case XESS_RESULT_ERROR_NOT_IMPLEMENTED: return "ERROR_NOT_IMPLEMENTED";
    case XESS_RESULT_ERROR_INVALID_CONTEXT: return "ERROR_INVALID_CONTEXT";
    case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS: return "ERROR_OPERATION_IN_PROGRESS";
    case XESS_RESULT_ERROR_UNSUPPORTED: return "ERROR_UNSUPPORTED";
    case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY: return "ERROR_CANT_LOAD_LIBRARY";
    case XESS_RESULT_ERROR_WRONG_CALL_ORDER: return "ERROR_WRONG_CALL_ORDER";
    default: return "ERROR_UNKNOWN";
    }
}
static bool LoadXess(const wchar_t* path)
{
    HMODULE m = LoadLibraryExW(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!m) { Say("LOAD: FAILED, LoadLibrary error %lu (126 is a missing dependency: the Visual C++ runtime)", GetLastError()); return false; }
    bool ok = true;
#define XGET(n) do { p_##n = (decltype(p_##n))GetProcAddress(m, #n); if (!p_##n) { Say("LOAD: no entry point %s", #n); ok = false; } } while (0)
    XGET(xessGetVersion); XGET(xessD3D12CreateContext); XGET(xessD3D12BuildPipelines); XGET(xessD3D12Init); XGET(xessD3D12Execute);
    XGET(xessDestroyContext); XGET(xessGetOptimalInputResolution); XGET(xessGetProperties); XGET(xessIsOptimalDriver);
    XGET(xessSetVelocityScale); XGET(xessSetJitterScale); XGET(xessSetLoggingCallback); XGET(xessGetPipelineBuildStatus);
#undef XGET
    return ok;
}

// ------------------------------------------------------------------------------------------------------------ Direct3D 12
static ID3D12Device* gDev = nullptr;
static ID3D12CommandQueue* gQ = nullptr;
static ID3D12CommandAllocator* gAlloc = nullptr;
static ID3D12GraphicsCommandList* gList = nullptr;
static ID3D12Fence* gFence = nullptr;
static UINT64 gFenceV = 0;
static HANDLE gEvt = nullptr;
static ID3D12QueryHeap* gTs = nullptr;
static ID3D12Resource* gTsBuf = nullptr;
static UINT64 gTsFreq = 0;

static void Flush()
{
    gQ->Signal(gFence, ++gFenceV);
    if (gFence->GetCompletedValue() < gFenceV) { gFence->SetEventOnCompletion(gFenceV, gEvt); WaitForSingleObject(gEvt, INFINITE); }
}
static ID3D12Resource* MakeBuffer(UINT64 size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_STATES st)
{
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = heap;
    D3D12_RESOURCE_DESC d = {}; d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; d.Width = size; d.Height = 1; d.DepthOrArraySize = 1;
    d.MipLevels = 1; d.SampleDesc.Count = 1; d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* r = nullptr;
    return SUCCEEDED(gDev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &d, st, nullptr, IID_PPV_ARGS(&r))) ? r : nullptr;
}
struct Tex
{
    ID3D12Resource* res = nullptr; ID3D12Resource* up = nullptr; ID3D12Resource* rb = nullptr;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {}; UINT64 bytes = 0; UINT w = 0, h = 0, bpp = 0; DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    D3D12_RESOURCE_STATES st = D3D12_RESOURCE_STATE_COMMON; uint8_t* upMap = nullptr;
    void Free() { if (upMap) up->Unmap(0, nullptr); if (res) res->Release(); if (up) up->Release(); if (rb) rb->Release(); *this = Tex(); }
};
static UINT Bpp(DXGI_FORMAT f) { return f == DXGI_FORMAT_R16G16B16A16_FLOAT ? 8 : 4; }
static bool MakeTex(Tex& t, UINT w, UINT h, DXGI_FORMAT fmt, bool uav, bool upload, bool readback, D3D12_RESOURCE_STATES st)
{
    t.w = w; t.h = h; t.fmt = fmt; t.bpp = Bpp(fmt); t.st = st;
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC d = {}; d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; d.Width = w; d.Height = h; d.DepthOrArraySize = 1;
    d.MipLevels = 1; d.Format = fmt; d.SampleDesc.Count = 1; d.Flags = uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
    if (FAILED(gDev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &d, st, nullptr, IID_PPV_ARGS(&t.res)))) return false;
    UINT rows = 0; UINT64 rowBytes = 0;
    gDev->GetCopyableFootprints(&d, 0, 1, 0, &t.fp, &rows, &rowBytes, &t.bytes);
    if (upload)
    {
        if (!(t.up = MakeBuffer(t.bytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ))) return false;
        D3D12_RANGE none = { 0, 0 }; if (FAILED(t.up->Map(0, &none, (void**)&t.upMap))) return false;
    }
    if (readback && !(t.rb = MakeBuffer(t.bytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST))) return false;
    return true;
}
static void Barrier(Tex& t, D3D12_RESOURCE_STATES to)
{
    if (t.st == to) return;
    D3D12_RESOURCE_BARRIER b = {}; b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; b.Transition.pResource = t.res;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES; b.Transition.StateBefore = t.st; b.Transition.StateAfter = to;
    gList->ResourceBarrier(1, &b); t.st = to;
}
static void CopyUp(Tex& t)
{
    Barrier(t, D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = t.res; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = t.up; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = t.fp;
    gList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}
static void CopyDown(Tex& t)
{
    Barrier(t, D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = t.res; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = t.rb; dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dst.PlacedFootprint = t.fp;
    gList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}
static uint8_t* RowUp(Tex& t, UINT y) { return t.upMap + t.fp.Offset + (UINT64)y * t.fp.Footprint.RowPitch; }

// half floats, enough for values in [0, 4] and vectors of a few pixels
static uint16_t ToHalf(float f)
{
    uint32_t x; memcpy(&x, &f, 4);
    const uint32_t sign = (x >> 16) & 0x8000u; int32_t e = (int32_t)((x >> 23) & 0xFF) - 127 + 15; uint32_t m = x & 0x7FFFFFu;
    if (e <= 0) return (uint16_t)sign;
    if (e >= 31) return (uint16_t)(sign | 0x7C00u);
    uint32_t h = sign | ((uint32_t)e << 10) | (m >> 13);
    if (m & 0x1000u) h++;   // round half up
    return (uint16_t)h;
}
static float FromHalf(uint16_t h)
{
    const uint32_t sign = (uint32_t)(h & 0x8000u) << 16; const uint32_t e = (h >> 10) & 0x1F; const uint32_t m = h & 0x3FF;
    uint32_t x;
    if (e == 0) { if (!m) x = sign; else { float v = ldexpf((float)m, -24); return (h & 0x8000u) ? -v : v; } }
    else if (e == 31) x = sign | 0x7F800000u | (m << 13);
    else x = sign | ((e - 15 + 127) << 23) | (m << 13);
    float f; memcpy(&f, &x, 4); return f;
}

// ------------------------------------------------------------------------------------------------------------ the scene
// THE PATTERN, in OUTPUT pixels, continuous: three products of sines with periods from 3.3 to 23 output pixels (the finest
// is above the render size's Nyquist rate at a scale of 1.5, so only a reconstruction that places the samples right can show
// it), in [0.1, 0.9]. The same function makes the input samples and the true picture.
static float Pattern(double x, double y)
{
    const double k = 6.283185307179586;
    double v = 0.5 + 0.16 * sin(k * x / 3.3) * sin(k * y / 4.1) + 0.12 * sin(k * (x + y) / 9.7) + 0.12 * sin(k * x / 23.0) * cos(k * y / 17.0);
    return (float)std::min(0.9, std::max(0.1, v));
}
static float Halton(int index, int base)
{
    float f = 1.0f, r = 0.0f;
    while (index > 0) { f /= (float)base; r += f * (float)(index % base); index /= base; }
    return r;
}

struct Arm
{
    const char* name; DXGI_FORMAT fmt; bool ldr;
    double velX, velY;          // the pattern's motion per frame, RENDER pixels (0 for the jitter test)
    double mvSign;              // the vector handed: mvSign * (previous - current) in render pixels; A +1, B -1
    double jitterSign;          // the jitter handed: jitterSign * (Jx, Jy); A +1, B -1; 0 with jitter off
    bool jitter;
    int frames, scoreFrom;      // frames run; the error is the mean over frames scoreFrom..frames-1
};
struct ArmResult { bool ok = false; double err = 0.0; double mean = 0.0; double gpuMs = 0.0; };

static UINT gRw = 2560, gRh = 1440, gOw = 3840, gOh = 2160;
static xess_quality_settings_t gQuality = XESS_QUALITY_SETTING_QUALITY;

static void FillColor(Tex& c, double t, double jx, double jy, const Arm& a)
{
    const double s = (double)gOw / gRw, sy = (double)gOh / gRh;
    for (UINT y = 0; y < c.h; y++)
    {
        uint8_t* row = RowUp(c, y);
        for (UINT x = 0; x < c.w; x++)
        {
            // the sample lands at the pixel's centre MINUS the jitter (the mod's projection), and the pattern has moved by t * vel
            const double rx = x + 0.5 - jx - t * a.velX, ry = y + 0.5 - jy - t * a.velY;
            const float v = Pattern(rx * s, ry * sy);
            if (c.fmt == DXGI_FORMAT_R16G16B16A16_FLOAT)
            {
                uint16_t* p = (uint16_t*)row + x * 4; const uint16_t hv = ToHalf(v);
                p[0] = hv; p[1] = ToHalf(v * 0.9f); p[2] = ToHalf(v * 0.8f); p[3] = ToHalf(1.0f);
            }
            else
            {
                uint8_t* p = row + x * 4; const uint8_t r = (uint8_t)lround(v * 255.0), g = (uint8_t)lround(v * 0.9 * 255.0), b = (uint8_t)lround(v * 0.8 * 255.0);
                if (c.fmt == DXGI_FORMAT_B8G8R8A8_UNORM) { p[0] = b; p[1] = g; p[2] = r; } else { p[0] = r; p[1] = g; p[2] = b; }
                p[3] = 255;
            }
        }
    }
}
static void FillConst(Tex& m, float a, float b)
{
    for (UINT y = 0; y < m.h; y++)
    {
        uint8_t* row = RowUp(m, y);
        for (UINT x = 0; x < m.w; x++)
        {
            if (m.fmt == DXGI_FORMAT_R16G16_FLOAT) { uint16_t* p = (uint16_t*)row + x * 2; p[0] = ToHalf(a); p[1] = ToHalf(b); }
            else { float* p = (float*)row + x; *p = a; }
        }
    }
}
// THE ERROR against the true picture: the mean absolute difference of the red channel (the pattern itself), over the output
// with a 48-pixel border left out, and the output's mean.
static void Score(Tex& o, double t, const Arm& a, double* err, double* mean)
{
    uint8_t* map = nullptr; D3D12_RANGE all = { 0, (SIZE_T)o.bytes };
    if (FAILED(o.rb->Map(0, &all, (void**)&map))) { *err = -1.0; return; }
    const double s = (double)gOw / gRw, sy = (double)gOh / gRh;
    double e = 0.0, m = 0.0; long long n = 0;
    for (UINT y = 48; y + 48 < o.h; y++)
    {
        const uint8_t* row = map + o.fp.Offset + (UINT64)y * o.fp.Footprint.RowPitch;
        for (UINT x = 48; x + 48 < o.w; x++)
        {
            float r;
            if (o.fmt == DXGI_FORMAT_R16G16B16A16_FLOAT) r = FromHalf(((const uint16_t*)row)[x * 4]);
            else if (o.fmt == DXGI_FORMAT_B8G8R8A8_UNORM) r = row[x * 4 + 2] / 255.0f;
            else r = row[x * 4] / 255.0f;
            // the true picture at the output pixel's centre, the pattern moved by t * vel (render pixels, so scaled)
            const float tr = Pattern((x + 0.5) - t * a.velX * s, (y + 0.5) - t * a.velY * sy);
            e += fabs((double)r - tr); m += r; n++;
        }
    }
    D3D12_RANGE none = { 0, 0 }; o.rb->Unmap(0, &none);
    *err = n ? e / n : -1.0; *mean = n ? m / n : 0.0;
}

static xess_quality_settings_t PickQuality(xess_context_handle_t ctx, bool print)
{
    const xess_2d_t out = { gOw, gOh };
    static const struct { xess_quality_settings_t q; const char* n; } qs[] = {
        { XESS_QUALITY_SETTING_ULTRA_PERFORMANCE, "Ultra Performance" }, { XESS_QUALITY_SETTING_PERFORMANCE, "Performance" },
        { XESS_QUALITY_SETTING_BALANCED, "Balanced" }, { XESS_QUALITY_SETTING_QUALITY, "Quality" },
        { XESS_QUALITY_SETTING_ULTRA_QUALITY, "Ultra Quality" }, { XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS, "Ultra Quality Plus" },
        { XESS_QUALITY_SETTING_AA, "Native Anti-Aliasing" } };
    xess_quality_settings_t pick = (xess_quality_settings_t)0; long best = LONG_MAX;
    for (auto& q : qs)
    {
        xess_2d_t opt = {}, mn = {}, mx = {};
        const xess_result_t r = p_xessGetOptimalInputResolution(ctx, &out, q.q, &opt, &mn, &mx);
        if (print) Say("SETTING %d %-20s %s: optimal %ux%u, range %ux%u to %ux%u (ratio %.3f)", (int)q.q, q.n, XRes(r), opt.x, opt.y, mn.x, mn.y, mx.x, mx.y,
                       opt.x ? (double)gOw / opt.x : 0.0);
        if (r != XESS_RESULT_SUCCESS) continue;
        if (gRw >= mn.x && gRw <= mx.x && gRh >= mn.y && gRh <= mx.y)
        {
            const long d = labs((long)opt.x - (long)gRw) + labs((long)opt.y - (long)gRh);
            if (d < best) { best = d; pick = q.q; }
        }
    }
    return pick;
}

// ONE ARM: a context of its own, Init at the arm's format, the arm's frames, its error. bench > 0: that many executes on the
// last inputs with GPU timestamps around each, no readback (the cost).
static ArmResult RunArm(const Arm& a, int bench, std::vector<double>* times)
{
    ArmResult R;
    xess_context_handle_t ctx = nullptr;
    xess_result_t r = p_xessD3D12CreateContext(gDev, &ctx);
    if (r != XESS_RESULT_SUCCESS) { Say("  %s: xessD3D12CreateContext %s", a.name, XRes(r)); return R; }
    p_xessSetLoggingCallback(ctx, XESS_LOGGING_LEVEL_WARNING, XessLog);
    uint32_t flags = XESS_INIT_FLAG_INVERTED_DEPTH | (a.ldr ? XESS_INIT_FLAG_LDR_INPUT_COLOR : 0);
    r = p_xessD3D12BuildPipelines(ctx, nullptr, true, flags);
    if (r != XESS_RESULT_SUCCESS) Say("  %s: xessD3D12BuildPipelines %s", a.name, XRes(r));
    xess_d3d12_init_params_t ip = {}; ip.outputResolution = { gOw, gOh }; ip.qualitySetting = gQuality; ip.initFlags = flags;
    r = p_xessD3D12Init(ctx, &ip);
    if (r != XESS_RESULT_SUCCESS) { Say("  %s: xessD3D12Init %s", a.name, XRes(r)); p_xessDestroyContext(ctx); return R; }
    p_xessSetVelocityScale(ctx, 1.0f, 1.0f); p_xessSetJitterScale(ctx, 1.0f, 1.0f);

    Tex col, mv, dep, out;
    bool made = MakeTex(col, gRw, gRh, a.fmt, false, true, false, D3D12_RESOURCE_STATE_COPY_DEST)
             && MakeTex(mv, gRw, gRh, DXGI_FORMAT_R16G16_FLOAT, false, true, false, D3D12_RESOURCE_STATE_COPY_DEST)
             && MakeTex(dep, gRw, gRh, DXGI_FORMAT_R32_FLOAT, false, true, false, D3D12_RESOURCE_STATE_COPY_DEST)
             && MakeTex(out, gOw, gOh, a.fmt, true, false, true, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    if (!made) { Say("  %s: a texture could not be made", a.name); col.Free(); mv.Free(); dep.Free(); out.Free(); p_xessDestroyContext(ctx); return R; }
    // the vector: previous minus current, render pixels; the pattern moves by +vel a frame, so previous - current = -vel
    FillConst(mv, (float)(a.mvSign * -a.velX), (float)(a.mvSign * -a.velY));
    FillConst(dep, 0.5f, 0.0f);

    const int phases = std::max(8, (int)ceil(8.0 * ((double)gOw / gRw) * ((double)gOh / gRh)));
    double errSum = 0.0, meanSum = 0.0; int scored = 0; bool execOk = true;
    const int total = a.frames + (bench > 0 ? bench : 0);
    for (int f = 0; f < total; f++)
    {
        const bool benchFrame = f >= a.frames;
        const int ph = f % phases;
        const double jx = a.jitter ? (Halton(ph + 1, 2) - 0.5) : 0.0, jy = a.jitter ? (Halton(ph + 1, 3) - 0.5) : 0.0;
        gAlloc->Reset(); gList->Reset(gAlloc, nullptr);
        if (!benchFrame)
        {
            FillColor(col, (double)f, jx, jy, a);
            CopyUp(col);
            if (f == 0) { CopyUp(mv); CopyUp(dep); }
        }
        Barrier(col, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); Barrier(mv, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Barrier(dep, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); Barrier(out, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        xess_d3d12_execute_params_t ep = {};
        ep.pColorTexture = col.res; ep.pVelocityTexture = mv.res; ep.pDepthTexture = dep.res; ep.pOutputTexture = out.res;
        ep.jitterOffsetX = (float)(a.jitterSign * jx); ep.jitterOffsetY = (float)(a.jitterSign * jy);
        ep.exposureScale = 1.0f; ep.resetHistory = f == 0 ? 1u : 0u; ep.inputWidth = gRw; ep.inputHeight = gRh;
        gList->EndQuery(gTs, D3D12_QUERY_TYPE_TIMESTAMP, 0);
        r = p_xessD3D12Execute(ctx, gList, &ep);
        gList->EndQuery(gTs, D3D12_QUERY_TYPE_TIMESTAMP, 1);
        gList->ResolveQueryData(gTs, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, gTsBuf, 0);
        if (r != XESS_RESULT_SUCCESS) { if (execOk) Say("  %s: xessD3D12Execute %s at frame %d", a.name, XRes(r), f); execOk = false; }
        const bool score = !benchFrame && f >= a.scoreFrom;
        if (score) CopyDown(out);
        gList->Close();
        ID3D12CommandList* l[] = { gList }; gQ->ExecuteCommandLists(1, l); Flush();
        if (score) { double e = 0, m = 0; Score(out, (double)f, a, &e, &m); errSum += e; meanSum += m; scored++; }
        if (benchFrame && times)
        {
            uint64_t* ts = nullptr; D3D12_RANGE rr = { 0, 16 };
            if (SUCCEEDED(gTsBuf->Map(0, &rr, (void**)&ts))) { times->push_back((double)(ts[1] - ts[0]) * 1000.0 / (double)gTsFreq); D3D12_RANGE none = { 0, 0 }; gTsBuf->Unmap(0, &none); }
        }
        if (!execOk) break;
    }
    R.ok = execOk && scored > 0; R.err = scored ? errSum / scored : -1.0; R.mean = scored ? meanSum / scored : 0.0;
    col.Free(); mv.Free(); dep.Free(); out.Free();
    p_xessDestroyContext(ctx);
    return R;
}

static const char* FmtName(DXGI_FORMAT f)
{
    return f == DXGI_FORMAT_R16G16B16A16_FLOAT ? "R16G16B16A16_FLOAT" : f == DXGI_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" : "R8G8B8A8_UNORM";
}

int wmain(int argc, wchar_t** argv)
{
    std::wstring lib, outPath; int benchN = 300;
    for (int i = 1; i < argc; i++)
    {
        if (!wcsncmp(argv[i], L"--lib=", 6)) lib = argv[i] + 6;
        else if (!wcsncmp(argv[i], L"--out=", 6)) outPath = argv[i] + 6;
        else if (!wcsncmp(argv[i], L"--bench=", 8)) benchN = _wtoi(argv[i] + 8);
        else if (!wcsncmp(argv[i], L"--render=", 9)) swscanf_s(argv[i] + 9, L"%ux%u", &gRw, &gRh);
        else if (!wcsncmp(argv[i], L"--output=", 9)) swscanf_s(argv[i] + 9, L"%ux%u", &gOw, &gOh);
        else { fwprintf(stderr, L"unknown argument %s\n", argv[i]); return 2; }
    }
    if (lib.empty() || outPath.empty()) { fwprintf(stderr, L"usage: xess_smoke --lib=<full path of libxess.dll> --out=<file> [--bench=N] [--render=WxH] [--output=WxH]\n"); return 2; }
    if (_wfopen_s(&gOut, outPath.c_str(), L"w") != 0) { fwprintf(stderr, L"cannot write %s\n", outPath.c_str()); return 2; }
    Say("XESS SMOKE (X0): render %ux%u, output %ux%u", gRw, gRh, gOw, gOh);

    // 1. the device: the first hardware adapter (the helper rebinds to the game's; here there is one card)
    IDXGIFactory4* fac = nullptr; IDXGIAdapter1* ad = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&fac)))) { Say("DEVICE: no DXGI factory"); return 1; }
    for (UINT i = 0; fac->EnumAdapters1(i, &ad) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC1 d; ad->GetDesc1(&d);
        if (!(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) { char n[128]; WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, n, sizeof(n), nullptr, nullptr);
                                                       Say("DEVICE: %s, vendor 0x%04X, %llu MB", n, d.VendorId, (unsigned long long)(d.DedicatedVideoMemory >> 20)); break; }
        ad->Release(); ad = nullptr;
    }
    if (!ad || FAILED(D3D12CreateDevice(ad, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gDev)))) { Say("DEVICE: no Direct3D 12 device"); return 1; }
    D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    gDev->CreateCommandQueue(&qd, IID_PPV_ARGS(&gQ));
    gDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gAlloc));
    gDev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gAlloc, nullptr, IID_PPV_ARGS(&gList)); gList->Close();
    gDev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)); gEvt = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    D3D12_QUERY_HEAP_DESC th = {}; th.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; th.Count = 2;
    gDev->CreateQueryHeap(&th, IID_PPV_ARGS(&gTs)); gTsBuf = MakeBuffer(16, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    gQ->GetTimestampFrequency(&gTsFreq);
    for (DXGI_FORMAT f : { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM })
    {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT fs = { f };
        gDev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fs, sizeof(fs));
        Say("FORMAT SUPPORT %s: typed UAV store %s, typed UAV load %s", FmtName(f), (fs.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) ? "yes" : "no",
            (fs.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) ? "yes" : "no");
    }

    // 1. the file
    if (!LoadXess(lib.c_str())) { Say("VERDICT LOAD: FAILED"); return 1; }
    xess_version_t v = {}; p_xessGetVersion(&v);
    Say("VERDICT LOAD: libxess %u.%u.%u loaded by its full path, every entry point found", v.major, v.minor, v.patch);
    {
        xess_context_handle_t ctx = nullptr; const xess_result_t r = p_xessD3D12CreateContext(gDev, &ctx);
        Say("VERDICT CREATE: xessD3D12CreateContext %s", XRes(r));
        if (r != XESS_RESULT_SUCCESS) return 1;
        const xess_result_t od = p_xessIsOptimalDriver(ctx);
        Say("DRIVER: xessIsOptimalDriver %s", XRes(od));
        // 2. the settings' ranges at this output, and the one the render size falls in
        gQuality = PickQuality(ctx, true);
        xess_properties_t pr = {}; const xess_2d_t o = { gOw, gOh };
        if (p_xessGetProperties(ctx, &o, &pr) == XESS_RESULT_SUCCESS)
            Say("PROPERTIES: descriptors %u, temporary buffers %.1f MB, temporary textures %.1f MB", pr.requiredDescriptorCount, pr.tempBufferHeapSize / 1048576.0, pr.tempTextureHeapSize / 1048576.0);
        p_xessDestroyContext(ctx);
        if (!gQuality) { Say("VERDICT SETTING: NO SETTING HOLDS %ux%u at %ux%u", gRw, gRh, gOw, gOh); return 1; }
        Say("VERDICT SETTING: %ux%u to %ux%u is setting %d", gRw, gRh, gOw, gOh, (int)gQuality);
    }

    // 3. the colour formats
    for (DXGI_FORMAT f : { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM })
    {
        const bool ldr = f != DXGI_FORMAT_R16G16B16A16_FLOAT;
        Arm a = { FmtName(f), f, ldr, 0.0, 0.0, 1.0, 1.0, true, 6, 2 };
        gXessErrors = 0;
        const ArmResult r = RunArm(a, 0, nullptr);
        Say("VERDICT FORMAT %s (%s): %s, the output's mean %.3f (the pattern's about 0.5), error %.4f, libxess errors %d", FmtName(f), ldr ? "LDR flag" : "HDR",
            (r.ok && r.mean > 0.2 && gXessErrors == 0) ? "RUNS" : "DOES NOT RUN", r.mean, r.err, gXessErrors);
    }

    // 4. the vectors' sign and units: the pattern moving 3.25 and 1.75 render pixels a frame
    {
        const Arm A = { "motion A", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 3.25, 1.75, +1.0, 1.0, true, 40, 24 };
        const Arm B = { "motion B", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 3.25, 1.75, -1.0, 1.0, true, 40, 24 };
        const Arm Z = { "motion 0", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 3.25, 1.75, 0.0, 1.0, true, 40, 24 };
        const ArmResult a = RunArm(A, 0, nullptr), b = RunArm(B, 0, nullptr), z = RunArm(Z, 0, nullptr);
        Say("MOTION: error with previous minus current %.4f, negated %.4f, no vector %.4f", a.err, b.err, z.err);
        const bool ok = a.ok && b.ok && z.ok && a.err > 0 && b.err > a.err * 1.25 && z.err > a.err * 1.25;
        if (!ok) gUnanswered++;
        Say("VERDICT MOTION: %s", ok ? "previous minus current, in render pixels, velocity scale 1: XeSS takes the mod's guides as they are (both controls worse by a quarter or more)"
                                     : "NOT ANSWERED: the controls do not separate");
    }
    // 5. the jitter's sign: a still pattern, the sample at the centre minus (Jx, Jy)
    {
        const Arm A = { "jitter A", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 0.0, 0.0, 1.0, +1.0, true, 40, 24 };
        const Arm B = { "jitter B", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 0.0, 0.0, 1.0, -1.0, true, 40, 24 };
        const Arm Z = { "jitter 0", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 0.0, 0.0, 1.0, 0.0, true, 40, 24 };
        const ArmResult a = RunArm(A, 0, nullptr), b = RunArm(B, 0, nullptr), z = RunArm(Z, 0, nullptr);
        Say("JITTER: error with (Jx, Jy) as the mod hands DLSS and FSR %.4f, negated %.4f, handed as 0 %.4f", a.err, b.err, z.err);
        const bool ok = a.ok && b.ok && z.ok && a.err > 0 && b.err > a.err * 1.25 && z.err > a.err * 1.1;
        if (!ok) gUnanswered++;
        Say("VERDICT JITTER: %s", ok ? "(Jx, Jy) as the mod hands DLSS and FSR, jitter scale 1: the same pair (the negated control worse by a quarter or more)"
                                     : "NOT ANSWERED: the controls do not separate");
    }
    // 6. the cost
    if (benchN > 0)
    {
        std::vector<double> t;
        const Arm A = { "bench", DXGI_FORMAT_R16G16B16A16_FLOAT, false, 0.0, 0.0, 1.0, 1.0, true, 4, 1000 };
        const ArmResult r = RunArm(A, benchN, &t);
        if (r.ok || !t.empty())
        {
            std::sort(t.begin(), t.end());
            const double med = t[t.size() / 2], p99 = t[std::min(t.size() - 1, (size_t)(t.size() * 0.99))];
            Say("VERDICT COST: %zu executes at setting %d, %ux%u to %ux%u, FP16: median %.3f ms, p99 %.3f ms, least %.3f ms (GPU timestamps around xessD3D12Execute)",
                t.size(), (int)gQuality, gRw, gRh, gOw, gOh, med, p99, t.front());
        }
        else { Say("VERDICT COST: NOT MEASURED"); gUnanswered++; }
    }
    Say("END: %d question(s) without an answer", gUnanswered);
    fclose(gOut);
    return gUnanswered ? 1 : 0;
}
