// X2, THE PIPE SMOKE OF THE HELPER'S XeSS ARM (2026-10-28; docs/reports/xess-plan-2026-10-17, PLAN.md X2's proofs).
//
// A DESK TOOL, NEVER IN THE SHIPPED SHAPE: a stand-in for the game against the BUILT helper, over the real pipe, with no
// game and no window. It starts the helper with its own pid (as the game does), speaks the real protocol (feed_ipc.h, the
// only file of the mod it includes: an instrument that shared code with the helper would agree with its mistakes), asks ONE
// upscaling build, and then runs frames through the helper's shared set: colour, depth and motion written here, the in-fence
// signalled here, the out-fence waited for here, and the Output read back here.
//
// WHY NOT tools\test-fg-pipe\fgpipe.cpp: that fake client asks TRANSPORT builds only (the helper copies Color to Output and no
// upscaler runs), so it cannot see an upscaler's arm. This one asks the build the game asks (transport 0, a target size), so
// DLSS, FSR and XeSS each run for real. It holds the set through Direct3D 12 (OpenSharedHandle) where the game holds it
// through Vulkan: the helper cannot tell (it duplicates NT handles into whatever process the pid names, and the hello says
// "a Vulkan client", the only kind it serves).
//
// WHAT IT PRINTS (one line each, "  PASS  " or "  FAIL  ", the last line PASS or FAIL), and tools\test-xess\pipe.py reads:
//   CAPS 0x...           the hello ack's capability word
//   ACK ok N flags 0x... output_fmt N
//   BLACK BEFORE mean    the Output read back BEFORE the first frame: blank, so the reader below is shown to read black as
//                        black (THE CONTROL of "the output is not black")
//   OUTPUT mean ...      the Output after the last frame: the red channel's mean, its least and most, and the true pattern's
//                        mean over the same pixels
//   FRAMES answered N of M
//
//     pipe_smoke.exe <helper.exe> <log> [--frames=N] [--render=WxH] [--output=WxH] [--rebuild=WxH] [--sdr] [--expect-refused] [--refused-then-ok] [helper args...]
// --refused-then-ok (fg18-fix, F6): the first build must be refused, and the same build asked again must be answered, with its frames.
// --rebuild: a second build at that render size and the same output, its own frames and reads (the frames' numbers go on).
// Every argument it does not know goes to the helper as it is (--backend=2, --gpu_probe=1, ...).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "feed_ipc.h"

static int gFails = 0, gPasses = 0;
static void Say(const char* fmt, ...) { va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); printf("\n"); fflush(stdout); }
static bool Proof(bool ok, const char* fmt, ...)
{
    char b[1024]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", b); fflush(stdout);
    (ok ? gPasses : gFails)++;
    return ok;
}

static HANDLE gPipe = INVALID_HANDLE_VALUE, gHelper = nullptr;
static bool PipeWrite(const void* p, DWORD n) { DWORD put = 0; return WriteFile(gPipe, p, n, &put, nullptr) && put == n; }
// a read with a deadline: the pipe is peeked until the bytes are there, the helper ends, or the time is out
static bool PipeRead(void* p, DWORD n, DWORD ms)
{
    const ULONGLONG end = GetTickCount64() + ms;
    for (;;)
    {
        DWORD avail = 0;
        if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr)) return false;
        if (avail >= n) { DWORD got = 0; return ReadFile(gPipe, p, n, &got, nullptr) && got == n; }
        if (GetTickCount64() > end) return false;
        if (WaitForSingleObject(gHelper, 0) == WAIT_OBJECT_0 && avail < n) { Sleep(50); if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr) || avail < n) return false; }
        Sleep(2);
    }
}

// ------------------------------------------------------------------------------------------------- Direct3D 12, this side's
static ID3D12Device* gDev = nullptr; static ID3D12CommandQueue* gQ = nullptr; static ID3D12CommandAllocator* gAl = nullptr;
static ID3D12GraphicsCommandList* gL = nullptr; static ID3D12Fence* gF = nullptr; static UINT64 gFv = 0; static HANDLE gEv = nullptr;
static void Flush()
{
    gL->Close(); ID3D12CommandList* l[] = { gL }; gQ->ExecuteCommandLists(1, l);
    gQ->Signal(gF, ++gFv);
    if (gF->GetCompletedValue() < gFv) { gF->SetEventOnCompletion(gFv, gEv); WaitForSingleObject(gEv, INFINITE); }
    gAl->Reset(); gL->Reset(gAl, nullptr);
}
static ID3D12Resource* Buffer(UINT64 size, D3D12_HEAP_TYPE t)
{
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = t;
    D3D12_RESOURCE_DESC d = {}; d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; d.Width = size; d.Height = 1; d.DepthOrArraySize = 1;
    d.MipLevels = 1; d.SampleDesc.Count = 1; d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* r = nullptr;
    return SUCCEEDED(gDev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &d, t == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ,
                                                   nullptr, IID_PPV_ARGS(&r))) ? r : nullptr;
}
static uint16_t ToHalf(float f)
{
    uint32_t x; memcpy(&x, &f, 4);
    const uint32_t s = (x >> 16) & 0x8000u; const int32_t e = (int32_t)((x >> 23) & 0xFF) - 127 + 15; const uint32_t m = x & 0x7FFFFFu;
    if (e <= 0) return (uint16_t)s;
    if (e >= 31) return (uint16_t)(s | 0x7C00u);
    return (uint16_t)(s | ((uint32_t)e << 10) | (m >> 13));
}
static float FromHalf(uint16_t h)
{
    const uint32_t s = (uint32_t)(h & 0x8000u) << 16, e = (h >> 10) & 0x1F, m = h & 0x3FF; uint32_t x;
    if (e == 0) { float v = ldexpf((float)m, -24); return (h & 0x8000u) ? -v : v; }
    if (e == 31) x = s | 0x7F800000u | (m << 13); else x = s | ((e - 15 + 127) << 23) | (m << 13);
    float f; memcpy(&f, &x, 4); return f;
}
// THE PATTERN, in output pixels: smooth stripes, values 0.1 to 0.9, the same function for the input and the truth
static float Pattern(double x, double y)
{
    const double k = 6.283185307179586;
    double v = 0.5 + 0.2 * sin(k * x / 37.0) * cos(k * y / 29.0) + 0.15 * sin(k * (x + y) / 113.0);
    return (float)(v < 0.1 ? 0.1 : v > 0.9 ? 0.9 : v);
}

// one texture written from the CPU: an upload buffer laid out by its footprint, then one copy
static bool Upload(ID3D12Resource* tex, const D3D12_RESOURCE_DESC& d, void (*fill)(uint8_t* row, UINT y, UINT w, DXGI_FORMAT f, double sx, double sy), double sx, double sy)
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {}; UINT rows = 0; UINT64 rowBytes = 0, total = 0;
    gDev->GetCopyableFootprints(&d, 0, 1, 0, &fp, &rows, &rowBytes, &total);
    ID3D12Resource* up = Buffer(total, D3D12_HEAP_TYPE_UPLOAD);
    if (!up) return false;
    uint8_t* p = nullptr; D3D12_RANGE none = { 0, 0 };
    if (FAILED(up->Map(0, &none, (void**)&p))) { up->Release(); return false; }
    for (UINT y = 0; y < d.Height; y++) fill(p + fp.Offset + (UINT64)y * fp.Footprint.RowPitch, y, (UINT)d.Width, d.Format, sx, sy);
    up->Unmap(0, nullptr);
    D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = tex; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = up; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = fp;
    gL->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    Flush();
    up->Release();
    return true;
}
static void FillColor(uint8_t* row, UINT y, UINT w, DXGI_FORMAT f, double sx, double sy)
{
    for (UINT x = 0; x < w; x++)
    {
        const float v = Pattern((x + 0.5) * sx, (y + 0.5) * sy);
        if (f == DXGI_FORMAT_R16G16B16A16_FLOAT) { uint16_t* q = (uint16_t*)row + 4 * x; q[0] = ToHalf(v); q[1] = ToHalf(v * 0.9f); q[2] = ToHalf(v * 0.8f); q[3] = ToHalf(1.0f); }
        else { uint8_t* q = row + 4 * x; const uint8_t r = (uint8_t)lround(v * 255.0), g = (uint8_t)lround(v * 0.9 * 255.0), b = (uint8_t)lround(v * 0.8 * 255.0);
               if (f == DXGI_FORMAT_B8G8R8A8_UNORM) { q[0] = b; q[1] = g; q[2] = r; } else { q[0] = r; q[1] = g; q[2] = b; } q[3] = 255; }
    }
}
static void FillDepth(uint8_t* row, UINT, UINT w, DXGI_FORMAT, double, double) { for (UINT x = 0; x < w; x++) ((float*)row)[x] = 0.5f; }
static void FillMv(uint8_t* row, UINT, UINT w, DXGI_FORMAT, double, double) { for (UINT x = 0; x < w; x++) { ((uint16_t*)row)[2 * x] = 0; ((uint16_t*)row)[2 * x + 1] = 0; } }

// the Output read back: the red channel's mean, least and most over the inner pixels, NaN counted, and the truth's mean
struct Read { double mean, lo, hi, truth, err; long long nan; bool ok; };
static Read ReadOutput(ID3D12Resource* tex, const D3D12_RESOURCE_DESC& d)
{
    Read R = { 0, 1e30, -1e30, 0, 0, 0, false };
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {}; UINT rows = 0; UINT64 rowBytes = 0, total = 0;
    gDev->GetCopyableFootprints(&d, 0, 1, 0, &fp, &rows, &rowBytes, &total);
    ID3D12Resource* rb = Buffer(total, D3D12_HEAP_TYPE_READBACK);
    if (!rb) return R;
    D3D12_TEXTURE_COPY_LOCATION src = {}; src.pResource = tex; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION dst = {}; dst.pResource = rb; dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dst.PlacedFootprint = fp;
    gL->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    Flush();
    uint8_t* p = nullptr; D3D12_RANGE all = { 0, (SIZE_T)total };
    if (FAILED(rb->Map(0, &all, (void**)&p))) { rb->Release(); return R; }
    double sum = 0, tsum = 0, esum = 0; long long n = 0;
    const UINT W = (UINT)d.Width, H = d.Height, b = 32;
    for (UINT y = b; y + b < H; y += 2)
    {
        const uint8_t* row = p + fp.Offset + (UINT64)y * fp.Footprint.RowPitch;
        for (UINT x = b; x + b < W; x += 2)
        {
            float r;
            if (d.Format == DXGI_FORMAT_R16G16B16A16_FLOAT) r = FromHalf(((const uint16_t*)row)[4 * x]);
            else if (d.Format == DXGI_FORMAT_B8G8R8A8_UNORM) r = row[4 * x + 2] / 255.0f;
            else r = row[4 * x] / 255.0f;
            if (r != r) { R.nan++; continue; }
            const float t = Pattern(x + 0.5, y + 0.5);
            sum += r; tsum += t; esum += fabs((double)r - t); n++;
            if (r < R.lo) R.lo = r;
            if (r > R.hi) R.hi = r;
        }
    }
    D3D12_RANGE none = { 0, 0 }; rb->Unmap(0, &none); rb->Release();
    R.ok = n > 0; R.mean = n ? sum / n : 0; R.truth = n ? tsum / n : 0; R.err = n ? esum / n : 0;
    return R;
}
static float Halton(int i, int b) { float f = 1, r = 0; while (i > 0) { f /= (float)b; r += f * (float)(i % b); i /= b; } return r; }

int main(int argc, char** argv)
{
    if (argc < 3) { Say("usage: pipe_smoke.exe <helper.exe> <log> [--frames=N] [--render=WxH] [--output=WxH] [--rebuild=WxH] [--sdr] [--expect-refused] [--refused-then-ok] [helper args]"); Say("FAIL"); return 2; }
    UINT rw = 2560, rh = 1440, ow = 3840, oh = 2160, rw2 = 0, rh2 = 0; int frames = 60; bool sdr = false, expectRefused = false, thenOk = false;
    char extra[1024] = "";
    for (int i = 3; i < argc; i++)
    {
        if      (!strncmp(argv[i], "--frames=", 9))  frames = atoi(argv[i] + 9);
        else if (!strncmp(argv[i], "--render=", 9))  sscanf_s(argv[i] + 9, "%ux%u", &rw, &rh);
        else if (!strncmp(argv[i], "--output=", 9))  sscanf_s(argv[i] + 9, "%ux%u", &ow, &oh);
        else if (!strcmp(argv[i], "--sdr"))          sdr = true;
        else if (!strncmp(argv[i], "--rebuild=", 10)) sscanf_s(argv[i] + 10, "%ux%u", &rw2, &rh2);   // a second build at this render size, the same output
        else if (!strcmp(argv[i], "--expect-refused")) expectRefused = true;
        else if (!strcmp(argv[i], "--refused-then-ok")) { expectRefused = true; thenOk = true; }   // (fg18-fix, F6: the first build refused, the same build asked again answered)
        else { strcat_s(extra, " "); strcat_s(extra, argv[i]); }
    }
    // ---- this side's device: the adapter DXGI prefers for performance, as the helper's own first guess
    IDXGIFactory6* fac = nullptr; IDXGIAdapter1* ad = nullptr; DXGI_ADAPTER_DESC1 desc = {};
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&fac))) || FAILED(fac->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&ad)))
        || FAILED(D3D12CreateDevice(ad, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&gDev)))) { Say("  FAIL  no Direct3D 12 device here"); Say("FAIL"); return 1; }
    ad->GetDesc1(&desc);
    D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    gDev->CreateCommandQueue(&qd, IID_PPV_ARGS(&gQ)); gDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gAl));
    gDev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gAl, nullptr, IID_PPV_ARGS(&gL));
    gDev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gF)); gEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Say("  the stand-in game: pid %lu, %ls (vendor 0x%04X)", GetCurrentProcessId(), desc.Description, desc.VendorId);

    // ---- the helper, started as the game starts it
    const DWORD me = GetCurrentProcessId();
    char cmd[4096]; sprintf_s(cmd, "\"%s\" %lu --log=2 --sync=0 --logpath=\"%s\"%s", argv[1], (unsigned long)me, argv[2], extra);
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) { Say("  FAIL  the helper did not start (%lu)", GetLastError()); Say("FAIL"); return 1; }
    gHelper = pi.hProcess;
    char name[128]; sprintf_s(name, FEED_PIPE_FMT, (unsigned long)me);
    for (int i = 0; i < 600 && gPipe == INVALID_HANDLE_VALUE; i++)
    {
        gPipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
        if (gPipe == INVALID_HANDLE_VALUE) { if (WaitForSingleObject(gHelper, 0) == WAIT_OBJECT_0) break; Sleep(50); }
    }
    if (!Proof(gPipe != INVALID_HANDLE_VALUE, "the helper made its pipe for this pid and the connect succeeded")) { TerminateProcess(gHelper, 9); Say("FAIL"); return 1; }

    FeedHello hello = {}; hello.magic = FEED_IPC_MAGIC; hello.version = FEED_IPC_VERSION; hello.pid = me; hello.client_kind = FEED_CLIENT_VULKAN;
    memcpy(hello.luid, &desc.AdapterLuid, 8); strcpy_s(hello.build, "unknown-source (the desk pipe smoke)");
    FeedHelloAck hack = {};
    bool ok = PipeWrite(&hello, sizeof(hello)) && PipeRead(&hack, sizeof(hack), 30000);
    Proof(ok && hack.magic == FEED_IPC_MAGIC && hack.version == FEED_IPC_VERSION && !(hack.caps & (FEED_CAP_REFUSED | FEED_CAP_BUILD_MISMATCH)),
          "the hello is answered and the session is not refused (version %u)", hack.version);
    Say("CAPS 0x%llX", (unsigned long long)hack.caps);
    if (!ok) { TerminateProcess(gHelper, 9); Say("FAIL"); return 1; }

    // ---- 'B': the build the game asks (transport 0, a target size), FP16 or the SDR capture's BGRA8. Run once, and again
    // at another render size with --rebuild= (the helper's ReleaseSet then gives the first context back and makes a second).
    // The frames' numbers go on across builds, as the game's do: the two fences are the connection's.
    UINT64 nNext = 1;
    auto build = [&](UINT rw, UINT rh, int frames)
    {
    const DXGI_FORMAT cf = sdr ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT;
    FeedBuild b = {}; b.width = rw; b.height = rh; b.color_fmt = (uint32_t)cf; b.output_fmt = (uint32_t)cf; b.hdr = sdr ? 0 : 1;
    b.depth_inverted = 1; b.flags_override = -1; b.transport = 0; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    b.target_width = (ow == rw && oh == rh) ? 0 : ow; b.target_height = (ow == rw && oh == rh) ? 0 : oh;
    FeedBuildAck back = {}; const uint8_t tagB = 'B';
    const ULONGLONG tb0 = GetTickCount64();
    const bool ok = PipeWrite(&tagB, 1) && PipeWrite(&b, sizeof(b)) && PipeRead(&back, sizeof(back), 60000);
    Say("ACK %ux%u ok %d flags 0x%X output_fmt %u result 0x%X (%llu ms)", rw, rh, ok ? back.ok : -1, back.flags, back.output_fmt, back.ngx_result, (unsigned long long)(GetTickCount64() - tb0));
    if (expectRefused)
    {
        Proof(ok && back.ok == 0, "the build is answered and REFUSED, as this run expects");
        bool none = true; for (int i = 0; i < FEED_SLOTS; i++) none = none && back.tex[i] == 0;
        Proof(ok && none, "a refused build hands over no texture handle");
    }
    else
    {
        Proof(ok && back.ok == 1, "the build %ux%u is answered ok", rw, rh);
        bool all = ok && back.fence_in && back.fence_out; for (int i = 0; i < FEED_SLOTS; i++) all = all && back.tex[i] != 0;
        Proof(all, "the ack carries two fences and four texture handles");
        if (ok && back.ok == 1 && all)
        {
            // ---- the set, opened on this side's device
            ID3D12Fence* fin = nullptr; ID3D12Fence* fout = nullptr; ID3D12Resource* t[FEED_SLOTS] = {};
            bool opened = SUCCEEDED(gDev->OpenSharedHandle((HANDLE)(uintptr_t)back.fence_in, IID_PPV_ARGS(&fin)))
                       && SUCCEEDED(gDev->OpenSharedHandle((HANDLE)(uintptr_t)back.fence_out, IID_PPV_ARGS(&fout)));
            for (int i = 0; i < FEED_SLOTS; i++) opened = opened && SUCCEEDED(gDev->OpenSharedHandle((HANDLE)(uintptr_t)back.tex[i], IID_PPV_ARGS(&t[i])));
            Proof(opened, "the two fences and four textures open on this side's device");
            if (opened)
            {
                const D3D12_RESOURCE_DESC dc = t[FEED_COLOR]->GetDesc(), dd = t[FEED_DEPTH]->GetDesc(), dm = t[FEED_MV]->GetDesc(), dout = t[FEED_OUTPUT]->GetDesc();
                Say("  the set: colour %llux%u fmt %u, depth fmt %u, motion fmt %u, output %llux%u fmt %u", dc.Width, dc.Height, (unsigned)dc.Format,
                    (unsigned)dd.Format, (unsigned)dm.Format, dout.Width, dout.Height, (unsigned)dout.Format);
                const double sx = (double)ow / rw, sy = (double)oh / rh;
                const bool up = Upload(t[FEED_COLOR], dc, FillColor, sx, sy) && Upload(t[FEED_DEPTH], dd, FillDepth, 1, 1) && Upload(t[FEED_MV], dm, FillMv, 1, 1);
                Proof(up, "colour (a smooth pattern, 0.1 to 0.9), depth (0.5) and motion (0) written into the set");
                const Read r0 = ReadOutput(t[FEED_OUTPUT], dout);
                Say("BLACK BEFORE mean %.4f least %.4f most %.4f", r0.mean, r0.lo, r0.hi);
                Proof(r0.ok && r0.hi <= 0.001, "THE CONTROL: before any frame the Output reads black (most %.4f): the reader reads black as black", r0.hi);
                int answered = 0;
                const int phases = 32;
                for (int k = 1; k <= frames; k++)
                {
                    const UINT64 n = nNext++;
                    fin->Signal(n);                                       // this side's copies are done (they were flushed above)
                    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.reset = k == 1 ? 1u : 0u;
                    const int ph = (int)((n - 1) % phases) + 1;
                    f.fm.jitter_x = Halton(ph, 2) - 0.5f; f.fm.jitter_y = Halton(ph, 3) - 0.5f;
                    f.fm.frame_time_ms = 8.33f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
                    if (!PipeWrite(&f, sizeof(f))) break;
                    if (fout->GetCompletedValue() < n) { fout->SetEventOnCompletion(n, gEv); if (WaitForSingleObject(gEv, 5000) != WAIT_OBJECT_0) break; }
                    answered++;
                }
                Say("FRAMES answered %d of %d", answered, frames);
                Proof(answered == frames, "every frame was answered in its turn (%d of %d)", answered, frames);
                const Read r1 = ReadOutput(t[FEED_OUTPUT], dout);
                Say("OUTPUT mean %.4f least %.4f most %.4f nan %lld truth %.4f error %.4f", r1.mean, r1.lo, r1.hi, r1.nan, r1.truth, r1.err);
                Proof(r1.ok && r1.nan == 0 && r1.mean > 0.2 && r1.hi > 0.5, "the Output after the last frame is NOT black: mean %.4f, most %.4f, no NaN", r1.mean, r1.hi);
                Proof(r1.ok && fabs(r1.mean - r1.truth) < 0.1 && r1.err < 0.15, "and it is the picture that went in: mean %.4f against the pattern's %.4f, error %.4f a pixel", r1.mean, r1.truth, r1.err);
                for (int i = 0; i < FEED_SLOTS; i++) if (t[i]) t[i]->Release();
                fin->Release(); fout->Release();
            }
        }
    }
    };
    build(rw, rh, frames);
    if (thenOk) { expectRefused = false; build(rw, rh, frames); }   // (fg18-fix, F6: the game asks the build again after a refusal, as IvtBuild does)
    if (rw2 && rh2 && !expectRefused) build(rw2, rh2, frames);
    // ---- the end: the game closes the pipe, the helper ends by itself
    CloseHandle(gPipe);
    const bool ended = WaitForSingleObject(gHelper, 20000) == WAIT_OBJECT_0;
    DWORD ec = 99; GetExitCodeProcess(gHelper, &ec);
    if (!ended) TerminateProcess(gHelper, 9);
    Proof(ended && ec == 0, "the helper ended by itself within 20 s of the pipe closing (exit code %lu)", (unsigned long)ec);
    Say("%d proofs passed, %d failed", gPasses, gFails);
    Say(gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
