// THE REPLAY: A RUN THE DEVELOPMENT HELPER CAPTURED IN THE GAME, MADE AGAIN AT A DESK THROUGH ANY BUILD OF THE MOD'S OWN FRAME
// GENERATION LIBRARY (DLSS-IV, 2026-10-10, phase 3 of the frame generation revival, after first light; host\fg_cap.h is
// the capture, docs\reports\fg-3\after-firstlight-2026-10-10\BRIEF-capture.md the brief).
//
// WHAT IT DOES. It reads a run's index.txt and its raw pictures, makes a Direct3D 12 device with no window and no swap
// chain, loads ONE library by its full path, and drives it through the mod's own interface (ffx-own\glue\dlssiv_fg.h)
// with THE VERY CALLS AND NUMBERS THE HELPER MADE: one create with the run's sizes, format and flags; the helper's own
// warm-up pair; then for every frame of the run a prepare in a list of its own (the frame's depth and vectors, every
// number of its record, bit for bit) and optical flow and interpolate in the next list (the frame's presented picture,
// its HUD-less picture where one was bound, the record's reset flag, transfer function and luminance pair). After
// every frame the generated image is read back whole.
//
// THE CONTROL THAT MAKES IT AN INSTRUMENT. Through the library the game ran, EVERY GENERATED IMAGE OF THE RUN MUST EQUAL THE
// ONE THE GAME'S HELPER SAVED, byte for byte: each frame's line says both SHA-256 and EQUAL, or how many pixels differ,
// by how many steps of the format at most, and where the first one is. The last line is PASS or FAIL.
//     --expect=equal     (the default) PASS when every compared image is equal
//     --expect=differ    PASS when at least one differs: the controls' own word (a planted fault must be SEEN)
//     --plant=mvscale    A PLANTED FAULT: the prepare's motion vector scale negated in every frame (one number of the prepare)
//     --plant=pixel      ANOTHER: one pixel of the LAST frame's presented picture changed before it is uploaded
//     --start=K          the replay begins at frame K of the run, with a reset there whatever the record says: frames
//                        before K are not replayed. How many frames later the images are the saved ones again is the
//                        depth of the library's history, measured
//     --warmup=0         no warm-up pair before the run (the helper makes one at every set: 1, the default)
//     --debug=BITS       the mod's interface's debug bits for every interpolate (0x400: AMD's debug view; 0x100 tear
//                        lines; 0x200 reset indicators). The image is then AMD's drawing: it is dumped and NOT compared
//     --dump=K           the replayed images of frames K and later are written beside the result (fNN-gen-replay.raw),
//                        for the sheets. -1 (the default): none
// IT TAKES ANY BUILD OF THE LIBRARY BY PATH (--lib), and says which by its size and SHA-256, beside the SHA-256 of the
// library the capture ran: the reference, the product with its cuts, a build with a variant.
//
//     fgreplay --run=<a run's folder> --lib=<dlssiv-fg.dll> --out=<folder> [the options above]
//
// It opens no window. It is a GPU user: tools\fg-replay\run.py builds it as fgpipe-replay.exe, so that every proof's
// "one GPU user at a time" (which looks for fgpipe*.exe) sees it, and waits for the card itself.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include "../../ffx-own/glue/dlssiv_fg.h"

static FILE* g_out = nullptr;
static void Say(const char* fmt, ...)
{
    char b[2048];
    va_list ap; va_start(ap, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, ap); va_end(ap);
    printf("%s\n", b); fflush(stdout);
    if (g_out) { fprintf(g_out, "%s\n", b); fflush(g_out); }
}
static int Die(const char* fmt, ...)
{
    char b[2048];
    va_list ap; va_start(ap, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, ap); va_end(ap);
    Say("fg-replay: STOPPED: %s", b);
    Say("FAIL");
    return 1;
}

// ---------------------------------------------------------------------------------------------------------------- files
static bool ReadAll(const std::string& path, std::vector<uint8_t>* out)
{
    const HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    bool ok = GetFileSizeEx(f, &size) != 0;
    if (ok) out->resize((size_t)size.QuadPart);
    size_t at = 0;
    while (ok && at < out->size())
    {
        const DWORD want = (DWORD)((out->size() - at) < (64u << 20) ? (out->size() - at) : (64u << 20)); DWORD got = 0;
        ok = ReadFile(f, out->data() + at, want, &got, nullptr) && got == want;
        at += got;
    }
    CloseHandle(f);
    return ok;
}
static bool WriteAll(const std::string& path, const uint8_t* p, size_t n)
{
    const HANDLE f = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    for (size_t at = 0; at < n && ok; )
    {
        const DWORD want = (DWORD)((n - at) < (64u << 20) ? (n - at) : (64u << 20)); DWORD w = 0;
        ok = WriteFile(f, p + at, want, &w, nullptr) && w == want;
        at += w;
    }
    CloseHandle(f);
    return ok;
}
static std::string Sha256(const uint8_t* p, size_t n)
{
    BCRYPT_ALG_HANDLE alg = nullptr; BCRYPT_HASH_HANDLE hh = nullptr; uint8_t d[32] = {}; char hex[65] = "";
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return "?";
    if (BCryptCreateHash(alg, &hh, nullptr, 0, nullptr, 0, 0) == 0)
    {
        for (size_t at = 0; at < n; ) { const ULONG c = (ULONG)((n - at) < (256u << 20) ? (n - at) : (256u << 20)); BCryptHashData(hh, (PUCHAR)(p + at), c, 0); at += c; }
        BCryptFinishHash(hh, d, 32, 0);
        BCryptDestroyHash(hh);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    for (int i = 0; i < 32; i++) _snprintf_s(hex + 2 * i, 3, _TRUNCATE, "%02x", d[i]);
    return hex;
}

// ---------------------------------------------------------------------------------------------------------------- the index
struct FileEnt { std::string name, kind; int frame = 0; uint32_t w = 0, h = 0, fmt = 0, bpp = 0; uint64_t bytes = 0; };
struct FrameRec { std::map<std::string, std::string> kv; };
struct Index
{
    std::map<std::string, std::string> kv;
    std::vector<FrameRec> frames;
    std::vector<FileEnt> files;
    bool ended = false;
};
static uint32_t Hex32(const std::string& s) { return (uint32_t)strtoul(s.c_str(), nullptr, 16); }
static float FloatOf(const std::string& s) { const uint32_t u = Hex32(s); float f; memcpy(&f, &u, 4); return f; }
static std::vector<std::string> Split(const std::string& s, char by)
{
    std::vector<std::string> out; size_t a = 0;
    for (;;) { const size_t b = s.find(by, a); out.push_back(s.substr(a, b == std::string::npos ? b : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return out;
}
static bool ParseIndex(const std::string& path, Index* ix)
{
    std::vector<uint8_t> raw;
    if (!ReadAll(path, &raw)) return false;
    const std::string text((const char*)raw.data(), raw.size());
    size_t a = 0;
    while (a < text.size())
    {
        size_t b = text.find('\n', a); if (b == std::string::npos) b = text.size();
        std::string line = text.substr(a, b - a); a = b + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line == "end") { ix->ended = true; continue; }
        if (line.rfind("frame: ", 0) == 0)
        {
            FrameRec f;
            for (const std::string& tok : Split(line.substr(7), ' ')) { const size_t e = tok.find('='); if (e != std::string::npos) f.kv[tok.substr(0, e)] = tok.substr(e + 1); }
            ix->frames.push_back(f);
        }
        else if (line.rfind("file: ", 0) == 0)
        {
            const std::vector<std::string> t = Split(line.substr(6), ' ');
            if (t.size() != 8) return false;
            FileEnt e; e.name = t[0]; e.kind = t[1]; e.frame = atoi(t[2].c_str()); e.w = (uint32_t)strtoul(t[3].c_str(), nullptr, 10); e.h = (uint32_t)strtoul(t[4].c_str(), nullptr, 10);
            e.fmt = (uint32_t)strtoul(t[5].c_str(), nullptr, 10); e.bpp = (uint32_t)strtoul(t[6].c_str(), nullptr, 10); e.bytes = _strtoui64(t[7].c_str(), nullptr, 10);
            ix->files.push_back(e);
        }
        else
        {
            const size_t c = line.find(": ");
            if (c != std::string::npos && line.rfind("frame_plain: ", 0) != 0) ix->kv[line.substr(0, c)] = line.substr(c + 2);
        }
    }
    return true;
}
static const FileEnt* FindFile(const Index& ix, int frame, const char* kind)
{
    for (const FileEnt& e : ix.files) if (e.frame == frame && e.kind == kind) return &e;
    return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------- Direct3D 12
struct Gpu
{
    ID3D12Device* dev = nullptr; ID3D12CommandQueue* q = nullptr; ID3D12CommandAllocator* alloc = nullptr; ID3D12GraphicsCommandList* list = nullptr;
    ID3D12Fence* fence = nullptr; UINT64 fv = 0; HANDLE ev = nullptr;
    ID3D12Resource* up = nullptr; UINT64 upBytes = 0; ID3D12Resource* rb = nullptr; UINT64 rbBytes = 0;
    char adapter[160] = "";
};
static bool GpuMake(Gpu* g, UINT64 largest)
{
    IDXGIFactory6* fac = nullptr; IDXGIAdapter1* ad = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void**)&fac))) return false;
    for (UINT i = 0; fac->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter1), (void**)&ad) == S_OK; i++)
    {
        DXGI_ADAPTER_DESC1 d = {}; ad->GetDesc1(&d);
        if (!(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && SUCCEEDED(D3D12CreateDevice(ad, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&g->dev)))
        {
            WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, g->adapter, (int)sizeof(g->adapter) - 1, nullptr, nullptr);
            ad->Release(); break;
        }
        ad->Release(); ad = nullptr;
    }
    fac->Release();
    if (!g->dev) return false;
    D3D12_COMMAND_QUEUE_DESC qd = {}; qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    bool ok = SUCCEEDED(g->dev->CreateCommandQueue(&qd, __uuidof(ID3D12CommandQueue), (void**)&g->q))
           && SUCCEEDED(g->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&g->alloc))
           && SUCCEEDED(g->dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g->alloc, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&g->list))
           && SUCCEEDED(g->dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&g->fence));
    if (!ok) return false;
    g->list->Close();
    g->ev = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    D3D12_RESOURCE_DESC bd = {};
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = largest; bd.Height = 1; bd.DepthOrArraySize = 1; bd.MipLevels = 1; bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES hu = {}; hu.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_HEAP_PROPERTIES hr = {}; hr.Type = D3D12_HEAP_TYPE_READBACK;
    ok = SUCCEEDED(g->dev->CreateCommittedResource(&hu, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&g->up))
      && SUCCEEDED(g->dev->CreateCommittedResource(&hr, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&g->rb));
    g->upBytes = g->rbBytes = largest;
    return ok && g->ev;
}
static bool Begin(Gpu* g) { return SUCCEEDED(g->alloc->Reset()) && SUCCEEDED(g->list->Reset(g->alloc, nullptr)); }
static bool End(Gpu* g)
{
    if (FAILED(g->list->Close())) return false;
    ID3D12CommandList* l[] = { g->list };
    g->q->ExecuteCommandLists(1, l);
    const UINT64 v = ++g->fv;
    if (FAILED(g->q->Signal(g->fence, v)) || FAILED(g->fence->SetEventOnCompletion(v, g->ev))) return false;
    return WaitForSingleObject(g->ev, 60000) == WAIT_OBJECT_0 && SUCCEEDED(g->dev->GetDeviceRemovedReason());
}
// A texture as the helper makes its shared ones: simultaneous access, COMMON, unordered access where it is written.
static ID3D12Resource* MakeTex(Gpu* g, UINT w, UINT h, DXGI_FORMAT fmt, bool uav)
{
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; rd.Width = w; rd.Height = h; rd.DepthOrArraySize = 1; rd.MipLevels = 1; rd.Format = fmt; rd.SampleDesc.Count = 1;
    rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS | (uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);
    ID3D12Resource* t = nullptr;
    if (FAILED(g->dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COMMON, nullptr, __uuidof(ID3D12Resource), (void**)&t))) return nullptr;
    return t;
}
static void Footprint(Gpu* g, ID3D12Resource* t, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* fp, UINT* rows, UINT64* rowBytes, UINT64* total)
{
    const D3D12_RESOURCE_DESC d = t->GetDesc();
    g->dev->GetCopyableFootprints(&d, 0, 1, 0, fp, rows, rowBytes, total);
}
// The file's rows (no padding) into the texture, whole. Null data: zeros (the warm-up's blank images).
static bool Upload(Gpu* g, ID3D12Resource* t, const uint8_t* data, size_t n)
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {}; UINT rows = 0; UINT64 rowBytes = 0, total = 0;
    Footprint(g, t, &fp, &rows, &rowBytes, &total);
    if (total > g->upBytes || (data && n != rowBytes * rows)) return false;
    void* p = nullptr; const D3D12_RANGE none = { 0, 0 };
    if (FAILED(g->up->Map(0, &none, &p)) || !p) return false;
    for (UINT y = 0; y < rows; y++)
    {
        if (data) memcpy((uint8_t*)p + (size_t)y * fp.Footprint.RowPitch, data + (size_t)y * rowBytes, (size_t)rowBytes);
        else memset((uint8_t*)p + (size_t)y * fp.Footprint.RowPitch, 0, (size_t)rowBytes);
    }
    g->up->Unmap(0, nullptr);
    if (!Begin(g)) return false;
    D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
    dst.pResource = t; dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.pResource = g->up; src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; src.PlacedFootprint = fp;
    g->list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    return End(g);
}
static bool Readback(Gpu* g, ID3D12Resource* t, std::vector<uint8_t>* out)
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {}; UINT rows = 0; UINT64 rowBytes = 0, total = 0;
    Footprint(g, t, &fp, &rows, &rowBytes, &total);
    if (total > g->rbBytes || !Begin(g)) return false;
    D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
    dst.pResource = g->rb; dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dst.PlacedFootprint = fp;
    src.pResource = t; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    g->list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    if (!End(g)) return false;
    void* p = nullptr; const D3D12_RANGE all = { 0, (SIZE_T)total };
    if (FAILED(g->rb->Map(0, &all, &p)) || !p) return false;
    out->resize((size_t)(rowBytes * rows));
    for (UINT y = 0; y < rows; y++) memcpy(out->data() + (size_t)y * rowBytes, (const uint8_t*)p + (size_t)y * fp.Footprint.RowPitch, (size_t)rowBytes);
    const D3D12_RANGE none = { 0, 0 }; g->rb->Unmap(0, &none);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------- comparing
// Two images of one format: how many pixels differ in any byte, the largest difference of one channel (for FP16 in steps of
// the half float: its bits in order, sign and magnitude as one signed whole number; else in codes), and the first place.
struct Diff { uint64_t px = 0; int32_t most = 0; uint32_t x = 0, y = 0; };
static Diff Compare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, uint32_t w, uint32_t h, uint32_t bpp, bool fp16)
{
    Diff d;
    if (a.size() != b.size() || a.size() != (size_t)w * h * bpp) { d.px = (uint64_t)w * h; d.most = -1; return d; }
    if (memcmp(a.data(), b.data(), a.size()) == 0) return d;
    for (uint32_t y = 0; y < h; y++)
    {
        const uint8_t* ra = a.data() + (size_t)y * w * bpp; const uint8_t* rb = b.data() + (size_t)y * w * bpp;
        if (memcmp(ra, rb, (size_t)w * bpp) == 0) continue;
        for (uint32_t x = 0; x < w; x++)
        {
            if (memcmp(ra + (size_t)x * bpp, rb + (size_t)x * bpp, bpp) == 0) continue;
            if (!d.px) { d.x = x; d.y = y; }
            d.px++;
            if (fp16)
                for (uint32_t c = 0; c < bpp / 2; c++)
                {
                    uint16_t u, v; memcpy(&u, ra + (size_t)x * bpp + 2 * c, 2); memcpy(&v, rb + (size_t)x * bpp + 2 * c, 2);
                    const int32_t su = (u & 0x8000) ? -(int32_t)(u & 0x7FFF) : (int32_t)(u & 0x7FFF), sv = (v & 0x8000) ? -(int32_t)(v & 0x7FFF) : (int32_t)(v & 0x7FFF);
                    const int32_t e = su > sv ? su - sv : sv - su;
                    if (e > d.most) d.most = e;
                }
            else
                for (uint32_t c = 0; c < bpp; c++)
                {
                    const int32_t e = abs((int32_t)ra[(size_t)x * bpp + c] - (int32_t)rb[(size_t)x * bpp + c]);
                    if (e > d.most) d.most = e;
                }
        }
    }
    return d;
}

static std::string Arg(int argc, char** argv, const char* name, const char* dflt)
{
    const size_t n = strlen(name);
    for (int i = 1; i < argc; i++) if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return dflt;
}

int main(int argc, char** argv)
{
    const std::string run = Arg(argc, argv, "--run", ""), libPath = Arg(argc, argv, "--lib", ""), outDir = Arg(argc, argv, "--out", "");
    const std::string expect = Arg(argc, argv, "--expect", "equal"), plant = Arg(argc, argv, "--plant", "none");
    const int startAt = atoi(Arg(argc, argv, "--start", "0").c_str()), warmup = atoi(Arg(argc, argv, "--warmup", "1").c_str()), dumpFrom = atoi(Arg(argc, argv, "--dump", "-1").c_str());
    const uint32_t debugBits = (uint32_t)strtoul(Arg(argc, argv, "--debug", "0").c_str(), nullptr, 0) & (DLSSIV_FG_DEBUG_TEAR_LINES | DLSSIV_FG_DEBUG_RESET_INDICATORS | DLSSIV_FG_DEBUG_VIEW);
    if (run.empty() || libPath.empty() || outDir.empty() || (expect != "equal" && expect != "differ") || (plant != "none" && plant != "mvscale" && plant != "pixel"))
    {
        printf("fgreplay --run=<a run's folder> --lib=<the library, a full path> --out=<folder> [--expect=equal|differ] [--plant=mvscale|pixel] [--start=K] [--warmup=0|1] [--debug=BITS] [--dump=K]\nFAIL\n");
        return 2;
    }
    CreateDirectoryA(outDir.c_str(), nullptr);
    fopen_s(&g_out, (outDir + "\\replay.txt").c_str(), "w");

    // ---- the run
    Index ix;
    if (!ParseIndex(run + "\\index.txt", &ix)) return Die("%s\\index.txt could not be read or is not a capture's index", run.c_str());
    if (!ix.ended || ix.kv["format"] != "1") return Die("the index has no end line or is not format 1: the run is not whole");
    const int N = atoi(ix.kv["frames"].c_str());
    if (N < 2 || (int)ix.frames.size() != N) return Die("the index says %d frames and holds %d frame lines", N, (int)ix.frames.size());
    if (startAt < 0 || startAt >= N) return Die("--start=%d is not a frame of this run (0 to %d)", startAt, N - 1);
    const bool hud = ix.kv["hudless"] == "1";
    const FileEnt* fPres = FindFile(ix, 0, "present"); const FileEnt* fGen = FindFile(ix, 0, "gen"); const FileEnt* fDepth = FindFile(ix, 0, "depth"); const FileEnt* fMv = FindFile(ix, 0, "mv");
    if (!fPres || !fGen || !fDepth || !fMv || (hud && !FindFile(ix, 0, "hud"))) return Die("the index names no presented, generated, depth or vectors file for frame 0");
    const uint32_t createFlags = (uint32_t)strtoul(ix.kv["create_flags"].c_str(), nullptr, 0);
    Say("fg-replay: the run %s: %d frames, the presented image %ux%u format %u, guides %ux%u (depth format %u, vectors format %u), %s; captured by the helper %s, mv_glass %s, sharpness %s",
        run.c_str(), N, fPres->w, fPres->h, fPres->fmt, fDepth->w, fDepth->h, fDepth->fmt, fMv->fmt, hud ? "with the HUD-less image" : "no HUD-less image", ix.kv["helper_build"].c_str(),
        ix.kv["mv_glass"].c_str(), ix.kv["sharpness"].c_str());
    Say("fg-replay: the setting, in the mod's own words: %s", ix.kv["mod_cfg_line"].c_str());

    // ---- the library, by its path, and which one it is
    std::vector<uint8_t> libBytes;
    if (!ReadAll(libPath, &libBytes)) return Die("the library %s could not be read", libPath.c_str());
    const std::string libSha = Sha256(libBytes.data(), libBytes.size());
    const HMODULE mod = LoadLibraryA(libPath.c_str());
    if (!mod) return Die("the library %s would not load (%lu)", libPath.c_str(), GetLastError());
    const DlssivFgInfoFn fnInfo = (DlssivFgInfoFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_INFO);
    const DlssivFgCreateFn fnCreate = (DlssivFgCreateFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_CREATE);
    const DlssivFgDestroyFn fnDestroy = (DlssivFgDestroyFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_DESTROY);
    const DlssivFgPrepareFn fnPrepare = (DlssivFgPrepareFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_PREPARE);
    const DlssivFgOpticalFlowFn fnFlow = (DlssivFgOpticalFlowFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_OPTICAL_FLOW);
    const DlssivFgInterpolateFn fnInterp = (DlssivFgInterpolateFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_INTERPOLATE);
    if (!fnInfo || !fnCreate || !fnDestroy || !fnPrepare || !fnFlow || !fnInterp) return Die("the library %s lacks a function of the mod's interface", libPath.c_str());
    DlssivFgInfo said = {}; said.size = sizeof(said);
    if (fnInfo(&said, nullptr) != DLSSIV_FG_OK || (said.iface < 1u || said.iface > DLSSIV_FG_INTERFACE)) return Die("the library speaks interface %u, this tool %u", said.iface, DLSSIV_FG_INTERFACE);
    said.build[sizeof(said.build) - 1] = 0; said.amdVersion[sizeof(said.amdVersion) - 1] = 0;
    const std::string capSha = ix.kv["library_sha256"];
    Say("fg-replay: the library %s, %llu bytes, sha256 %s: interface %u, stage %u, \"%s\", AMD \"%s\"; the capture's library was sha256 %s: %s",
        libPath.c_str(), (unsigned long long)libBytes.size(), libSha.c_str(), said.iface, said.stage, said.build, said.amdVersion, capSha.empty() ? "(not named)" : capSha.c_str(),
        capSha == libSha ? "THE SAME FILE" : "ANOTHER FILE");

    // ---- the device and the five textures
    Gpu g;
    const uint64_t largest = ((uint64_t)fPres->w * fPres->bpp + 255) / 256 * 256 * fPres->h + 4096;
    if (!GpuMake(&g, largest)) return Die("no Direct3D 12 device, queue or buffers");
    Say("fg-replay: the device: %s; no window, no swap chain", g.adapter);
    ID3D12Resource* tPres = MakeTex(&g, fPres->w, fPres->h, (DXGI_FORMAT)fPres->fmt, false);
    ID3D12Resource* tGen = MakeTex(&g, fGen->w, fGen->h, (DXGI_FORMAT)fGen->fmt, true);
    ID3D12Resource* tHud = hud ? MakeTex(&g, fPres->w, fPres->h, (DXGI_FORMAT)fPres->fmt, false) : nullptr;
    ID3D12Resource* tDepth = MakeTex(&g, fDepth->w, fDepth->h, (DXGI_FORMAT)fDepth->fmt, false);
    ID3D12Resource* tMv = MakeTex(&g, fMv->w, fMv->h, (DXGI_FORMAT)fMv->fmt, false);
    if (!tPres || !tGen || !tDepth || !tMv || (hud && !tHud)) return Die("a texture could not be made");

    // ---- the create, as the helper's own arm makes it (host\fg_own.h, FgoCreateDesc): sizes, format, flags; no stamps
    DlssivFgCreateDesc cd = {};
    cd.size = sizeof(cd); cd.flags = createFlags & (DLSSIV_FG_CREATE_DEPTH_INVERTED | DLSSIV_FG_CREATE_DEPTH_INFINITE | DLSSIV_FG_CREATE_HDR);
    cd.device = g.dev; cd.displayWidth = fPres->w; cd.displayHeight = fPres->h; cd.maxRenderWidth = fDepth->w; cd.maxRenderHeight = fDepth->h; cd.backBufferFormat = fPres->fmt;
    DlssivFgContext* ctx = nullptr; DlssivFgCreated made = {}; made.size = sizeof(made);
    const uint32_t crc = fnCreate(&cd, &ctx, &made);
    if (crc != DLSSIV_FG_OK || !ctx) return Die("the library's create answered 0x%08X", crc);
    Say("fg-replay: created: flags 0x%X, pipelines %u asked, %u refused; shader model %u.%u, AMD's %s", cd.flags, made.pipelinesAsked, made.pipelinesRefused, made.shaderModel >> 4, made.shaderModel & 15u,
        made.wave64 ? "WAVE64 variants" : "base variants");
    const bool hdr = (createFlags & DLSSIV_FG_CREATE_HDR) != 0;

    auto prepare = [&](uint64_t id, bool reset, const FrameRec* f) -> uint32_t
    {
        DlssivFgPrepareDesc d = {};
        d.size = sizeof(d); d.flags = reset ? DLSSIV_FG_RESET : 0u; d.commandList = g.list; d.depth = tDepth; d.motionVectors = tMv; d.frameId = id;
        if (f)
        {
            const std::vector<std::string> rs = Split(f->kv.at("render"), 'x'), jt = Split(f->kv.at("jitter"), ','), ms = Split(f->kv.at("mvscale"), ','), cam = Split(f->kv.at("cam"), ',');
            d.renderWidth = (uint32_t)atoi(rs[0].c_str()); d.renderHeight = (uint32_t)atoi(rs[1].c_str());
            d.jitterX = FloatOf(jt[0]); d.jitterY = FloatOf(jt[1]); d.motionVectorScaleX = FloatOf(ms[0]); d.motionVectorScaleY = FloatOf(ms[1]);
            d.frameTimeMs = FloatOf(f->kv.at("frame_ms")); d.cameraNear = FloatOf(f->kv.at("near")); d.cameraFar = FloatOf(f->kv.at("far")); d.cameraFovY = FloatOf(f->kv.at("fov"));
            d.viewSpaceToMeters = FloatOf(f->kv.at("v2m"));
            for (int k = 0; k < 3; k++) { d.cameraPosition[k] = FloatOf(cam[k]); d.cameraUp[k] = FloatOf(cam[3 + k]); d.cameraRight[k] = FloatOf(cam[6 + k]); d.cameraForward[k] = FloatOf(cam[9 + k]); }
            if (plant == "mvscale") { d.motionVectorScaleX = -d.motionVectorScaleX; d.motionVectorScaleY = -d.motionVectorScaleY; }
        }
        else
        {   // the helper's warm-up (host\fg_host.h, FgBuild and FgPrepareSubmit): 16.67 ms, the identity camera, scale 1
            d.renderWidth = fDepth->w; d.renderHeight = fDepth->h; d.motionVectorScaleX = 1.0f; d.motionVectorScaleY = 1.0f; d.frameTimeMs = 16.67f;
            d.cameraNear = 3.402823466e+38f; d.cameraFar = 0.05f; d.cameraFovY = 0.7853981f; d.viewSpaceToMeters = 1.0f;
            d.cameraUp[1] = 1.0f; d.cameraRight[0] = 1.0f; d.cameraForward[2] = 1.0f;
        }
        return fnPrepare(ctx, &d);
    };
    auto dispatch = [&](uint64_t id, bool reset, const FrameRec* f, bool withHud) -> uint32_t
    {
        DlssivFgFrameDesc d = {};
        d.size = (uint32_t)DLSSIV_FG_FRAME_SIZE_1; d.flags = (reset ? DLSSIV_FG_RESET : 0u) | debugBits; d.commandList = g.list; d.present = tPres; d.hudless = withHud ? tHud : nullptr; d.output = tGen; d.frameId = id;
        if (f)
        {
            const std::vector<std::string> lum = Split(f->kv.at("lum"), ','), rc = Split(f->kv.at("rect"), ',');
            d.rectLeft = atoi(rc[0].c_str()); d.rectTop = atoi(rc[1].c_str()); d.rectWidth = atoi(rc[2].c_str()); d.rectHeight = atoi(rc[3].c_str());
            d.transfer = (uint32_t)atoi(f->kv.at("transfer").c_str()); d.minLuminance = FloatOf(lum[0]); d.maxLuminance = FloatOf(lum[1]);
            // (a run captured with the helper's own --fgdebug holds AMD's drawing in its saved images: the same bits, in the interface's own numbers)
            const uint32_t capDbg = (uint32_t)strtoul(f->kv.at("dbg").c_str(), nullptr, 0);
            d.flags |= ((capDbg & 1u) ? DLSSIV_FG_DEBUG_TEAR_LINES : 0u) | ((capDbg & 2u) ? DLSSIV_FG_DEBUG_RESET_INDICATORS : 0u) | ((capDbg & 4u) ? DLSSIV_FG_DEBUG_VIEW : 0u);
        }
        else
        {
            d.rectWidth = (int32_t)fPres->w; d.rectHeight = (int32_t)fPres->h; d.transfer = hdr ? DLSSIV_FG_TRANSFER_SCRGB : DLSSIV_FG_TRANSFER_SRGB; d.minLuminance = 0.0f; d.maxLuminance = 1000.0f;
        }
        uint32_t rc2 = fnFlow(ctx, &d);
        if (rc2 == DLSSIV_FG_OK) rc2 = fnInterp(ctx, &d);
        return rc2;
    };

    // ---- the warm-up pair, as every set of the helper's has one: blank images, the guides blank too, reset
    if (warmup)
    {
        if (!Upload(&g, tPres, nullptr, 0) || !Upload(&g, tGen, nullptr, 0) || (hud && !Upload(&g, tHud, nullptr, 0)) || !Upload(&g, tDepth, nullptr, 0) || !Upload(&g, tMv, nullptr, 0)) return Die("the blank images could not be uploaded");
        if (!Begin(&g)) return Die("a command list would not begin");
        uint32_t rc = prepare(1, true, nullptr);
        if (rc != DLSSIV_FG_OK || !End(&g)) return Die("the warm-up's prepare answered 0x%08X or did not run", rc);
        if (!Begin(&g)) return Die("a command list would not begin");
        rc = dispatch(1, true, nullptr, hud);
        if (rc != DLSSIV_FG_OK || !End(&g)) return Die("the warm-up's dispatch answered 0x%08X or did not run", rc);
    }
    Say("fg-replay: %s; plant: %s; debug bits 0x%X; it begins at frame %d%s; expected: every compared image %s", warmup ? "the helper's warm-up pair ran first" : "NO warm-up pair (--warmup=0)", plant.c_str(), debugBits,
        startAt, startAt ? " WITH A RESET THERE (--start)" : "", expect == "equal" ? "EQUAL to the saved one" : "and at least one DIFFERS (a control)");

    // ---- the frames
    int compared = 0, equalN = 0, firstEqualAfter = -1; bool allEqualFrom = true;
    std::vector<uint8_t> bPres, bHud, bDepth, bMv, bGen, bOut;
    const bool fp16 = fGen->fmt == (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    for (int i = startAt; i < N; i++)
    {
        const FrameRec& f = ix.frames[i];
        const FileEnt* e;
        char name[64];
        _snprintf_s(name, sizeof(name), _TRUNCATE, "frame %02d", i);
        if (!(e = FindFile(ix, i, "present")) || !ReadAll(run + "\\" + e->name, &bPres) || bPres.size() != e->bytes) return Die("%s: its presented picture is missing or is not the size the index says", name);
        if (!(e = FindFile(ix, i, "depth")) || !ReadAll(run + "\\" + e->name, &bDepth) || bDepth.size() != e->bytes) return Die("%s: its depth is missing or is not the size the index says", name);
        if (!(e = FindFile(ix, i, "mv")) || !ReadAll(run + "\\" + e->name, &bMv) || bMv.size() != e->bytes) return Die("%s: its vectors are missing or are not the size the index says", name);
        if (!(e = FindFile(ix, i, "gen")) || !ReadAll(run + "\\" + e->name, &bGen) || bGen.size() != e->bytes) return Die("%s: its generated image is missing or is not the size the index says", name);
        const bool fHud = hud && f.kv.at("hud") == "1";
        if (fHud && (!(e = FindFile(ix, i, "hud")) || !ReadAll(run + "\\" + e->name, &bHud) || bHud.size() != e->bytes)) return Die("%s: its HUD-less picture is missing or is not the size the index says", name);
        if (plant == "pixel" && i == N - 1)
        {   // one pixel of the last presented picture, at its middle: every byte of it turned over
            uint8_t* px = bPres.data() + ((size_t)(fPres->h / 2) * fPres->w + fPres->w / 2) * fPres->bpp;
            for (uint32_t k = 0; k < fPres->bpp; k++) px[k] = (uint8_t)(fp16 ? ((k & 1) ? 0x3C : 0x00) ^ (px[k] == ((k & 1) ? 0x3C : 0x00) ? 0x04 : 0x00) : px[k] ^ 0xFF);
        }
        if (!Upload(&g, tPres, bPres.data(), bPres.size()) || !Upload(&g, tDepth, bDepth.data(), bDepth.size()) || !Upload(&g, tMv, bMv.data(), bMv.size()) || (fHud && !Upload(&g, tHud, bHud.data(), bHud.size())))
            return Die("%s: a picture could not be uploaded", name);
        const uint64_t id = _strtoui64(f.kv.at("id").c_str(), nullptr, 10);
        const bool pReset = f.kv.at("prep_reset") == "1" || i == startAt, dReset = f.kv.at("disp_reset") == "1" || i == startAt;
        if (!Begin(&g)) return Die("a command list would not begin");
        uint32_t rc = prepare(id, pReset, &f);
        if (rc != DLSSIV_FG_OK || !End(&g)) return Die("%s: the prepare answered 0x%08X or did not run", name, rc);
        if (!Begin(&g)) return Die("a command list would not begin");
        rc = dispatch(id, dReset, &f, fHud);
        if (rc != DLSSIV_FG_OK || !End(&g)) return Die("%s: the dispatch answered 0x%08X or did not run", name, rc);
        if (!Readback(&g, tGen, &bOut)) return Die("%s: the generated image could not be read back", name);
        if (dumpFrom >= 0 && i >= dumpFrom)
        {
            char p[64]; _snprintf_s(p, sizeof(p), _TRUNCATE, "\\f%02d-gen-replay.raw", i);
            if (!WriteAll(outDir + p, bOut.data(), bOut.size())) return Die("%s: its replayed image could not be written", name);
        }
        const std::string shaSaved = Sha256(bGen.data(), bGen.size()), shaNow = Sha256(bOut.data(), bOut.size());
        const int since = atoi(f.kv.at("since").c_str());
        if (debugBits)
        {
            Say("%s pair %s%s, %d since the reset: replayed WITH DEBUG DRAWING (not compared): sha256 %s", name, f.kv.at("n").c_str(), dReset ? " RESET" : "", since, shaNow.c_str());
            continue;
        }
        const Diff d = Compare(bGen, bOut, fGen->w, fGen->h, fGen->bpp, fp16);
        compared++;
        if (!d.px)
        {
            equalN++;
            if (firstEqualAfter < 0) firstEqualAfter = i;
            Say("%s pair %s%s, %d since the reset: saved sha256 %s, replayed sha256 %s: EQUAL", name, f.kv.at("n").c_str(), dReset ? " RESET" : "", since, shaSaved.c_str(), shaNow.c_str());
        }
        else
        {
            firstEqualAfter = -1; if (i > startAt) allEqualFrom = false;
            Say("%s pair %s%s, %d since the reset: saved sha256 %s, replayed sha256 %s: DIFFERS: %llu of %llu pixels, the largest difference %d %s, the first at %u,%u", name, f.kv.at("n").c_str(),
                dReset ? " RESET" : "", since, shaSaved.c_str(), shaNow.c_str(), (unsigned long long)d.px, (unsigned long long)fGen->w * fGen->h, d.most, fp16 ? "step(s) of the half float" : "code(s)", d.x, d.y);
        }
    }
    fnDestroy(ctx);
    if (debugBits)
    {
        Say("fg-replay: %d frames replayed with debug drawing; nothing was compared", N - startAt);
        Say("PASS");
        return 0;
    }
    Say("fg-replay: %d of %d generated images EQUAL the saved ones%s", equalN, compared,
        (startAt && firstEqualAfter >= 0) ? (std::string("; from frame ") + std::to_string(firstEqualAfter) + " on every one is equal: " + std::to_string(firstEqualAfter - startAt) + " frame(s) after the reset at frame "
                                             + std::to_string(startAt)).c_str() : (startAt ? "; NO frame from which every later one is equal" : ""));
    (void)allEqualFrom;
    const bool pass = expect == "equal" ? (compared > 0 && equalN == compared) : (compared > 0 && equalN < compared);
    Say(pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
