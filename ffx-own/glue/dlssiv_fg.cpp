// OUR OWN FILE (not AMD's): dlssiv-fg.dll's seven functions (dlssiv_fg.h), over the internal C interface of AMD's frame
// interpolation and optical flow (2026-10-07, built and proved 2026-10-10, the own build's stage 2; docs/DECISIONS.md, "PHASE 3: OUR OWN BUILD OF AMD'S
// FRAME GENERATION", sections 3 and 4; docs/reports/fg-3/own-build-2026-10-06/reader-integration.md, "Shape (b)").
//
// WHAT IT STANDS IN FOR. AMD's library reaches these two effects through its provider layer
// (framegeneration/fsr3/internal/ffx_provider_fsr3framegeneration.cpp, 787 lines), which is not compiled into this
// library. This file does what that layer does FOR THE PATH dlssiv-helper.exe TAKES and nothing else; each block below
// names the lines of AMD's file it follows, at tag v2.3.0:
//     the create        88 to 189: two backends (one of one context for the images the two effects share, one of two for
//                       the effects), optical flow at the display's size, frame interpolation, then the eight shared
//                       images in AMD's order and by AMD's names (two of optical flow's, and TWO SETS of the prepare's
//                       three: the provider keeps a second set that nothing reads, and so does this file, in this stage;
//                       the product's cut fi-one-shared-set makes one)
//     the destroy       191 to 216
//     the memory        516 to 544 (what a context will take, from the sizes alone) and 495 to 515 (what one holds)
//     the prepare       679 to 773: the set of the three images is switched FIRST, then AMD's prepare writes the new one
//     the frame id      561 to 568: a dispatch resets when it is told to, when its pair's prepare was told to, or when its
//                       id is not the last one announced, or not one more than the last one interpolated. The provider
//                       announces an id at its configure; this interface has no configure, and the prepare announces it
//                       (the helper configures and prepares every pair with one id, so the two are the same call)
//     optical flow      570 to 588: on the HUD-less image where there is one
//     interpolate       590 to 671: block 8 and one over the display's size, as the provider writes them
// NOT HERE, because the helper's path never comes to it: the swapchain's callbacks and its configure (the helper presents
// its own frames), the distortion field, the first version of the prepare's description, async workloads, AMD's two
// reserved dispatch flags.
//
// AMD'S FILES ARE NOT TOUCHED BY ANYTHING HERE. What this file adds stands in front of AMD's backend, in the table of
// function pointers every effect calls it through (api/internal/ffx_interface.h):
//   - fpCreatePipeline, ALWAYS: AMD's two effects ask the device for 18 pipelines and throw each answer away
//     (ffx_frameinterpolation.cpp 296 to 343, ffx_opticalflow.cpp 212 to 239), so "the create answered OK" does not say
//     the pipelines exist. Here every answer is counted, and a create with a refused pipeline is refused.
//   - THE STAMPS, only on a context created with DLSSIV_FG_CREATE_STAMPS (a development switch): the effects' jobs are
//     held here instead of in AMD's backend, and at AMD's execute each is handed to the backend and executed ALONE, with
//     a GPU timestamp behind it. AMD's own loop (backend/dx12/ffx_dx12.cpp 3999 to 4039) does nothing between two jobs
//     but an event marker that is off, and the backend's pending barriers live in its own state across calls of its
//     execute, so the commands a job records are the ones it records in AMD's loop.
//     WITHOUT THE SWITCH THOSE FOUR POINTERS ARE AMD'S OWN: the list a call records is the list AMD's library records.
//
// THE BARRIER BEFORE A STAMP (host/fg_cost.h says why): a timestamp behind a compute pass need not wait for the pass
// when nothing makes it. A UAV barrier on no resource does. One stands before every stamp that follows recorded work,
// AND BEFORE A CALL'S HEAD STAMP, so that the two ends of every bracket wait alike (2026-10-10);
// DLSSIV_FG_CREATE_STAMPS_NO_BARRIER leaves them all out, so that what they change is measured and not assumed.
// A job that is only a barrier records nothing by itself (AMD's backend keeps it for the next job), so the stamp behind
// it has no barrier and its bracket is an empty one, which must read about zero.
#include "dlssiv_fg.h"

#include "backend/dx12/ffx_dx12.h"
#include "framegeneration/fsr3/include/ffx_frameinterpolation.h"
#include "framegeneration/fsr3/include/ffx_opticalflow.h"
#include "upscalers/fsr3/include/gpu/fsr3/ffx_fsr3_resources.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static_assert(DLSSIV_FG_TRANSFER_SRGB == (uint32_t)FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SRGB && DLSSIV_FG_TRANSFER_PQ == (uint32_t)FFX_API_BACKBUFFER_TRANSFER_FUNCTION_PQ &&
              DLSSIV_FG_TRANSFER_SCRGB == (uint32_t)FFX_API_BACKBUFFER_TRANSFER_FUNCTION_SCRGB, "the transfer function's numbers are AMD's");
static_assert(sizeof(DlssivFgMark) == 48, "a mark is 48 bytes");

// THE CUTS (the own build's stage 3, 2026-10-10). build-ffx-own.ps1 compiles this file twice: once as stage 2 did (THE
// REFERENCE: its bytes are stage 2's record's), and once for THE PRODUCT, from a copy of AMD's source the cuts were
// applied to. What tells the two apart is a FILE, not an option of the compiler (the two compiles have one command
// line, token for token): dlssiv_fg_cuts.h, which
// tools\ffx-own\cuts.py writes into the product build's temp copy of this folder and which is never committed. It says
// how many cuts are in, the first eight digits of their digest, and one DLSSIV_FG_CUT_ name for each cut that is a
// change of THIS file. Nothing below may differ between the two compiles outside such a name.
#if __has_include("dlssiv_fg_cuts.h")
#include "dlssiv_fg_cuts.h"
#define DLSSIV_FG_CUTS 1
#define DLSSIV_FG_STAGE_BUILT 3u
#else
#define DLSSIV_FG_STAGE_BUILT DLSSIV_FG_STAGE
#endif

// THE POSITION (2026-10-17, the cut fi-position; dlssiv_fg.h, "INTERFACE 2"). A library built without that cut is interface 1
// as it always was, BYTE FOR BYTE: it takes only interface 1's frame description and says 1 (the reference, and every
// frozen variant library, keep their records). With the cut it says 2 and takes either size.
static_assert(DLSSIV_FG_FRAME_SIZE_1 == 80 && sizeof(DlssivFgFrameDesc) == 88, "interface 1's frame description is 80 bytes, interface 2's ends with the position");
// THE STORE APART (2026-10-18, the cut fi-store-apart; dlssiv_fg.h, "INTERFACE 3") stands on the position: a library with it says 3,
// one with the position alone says 2, as it did (the number is written here, not taken from the header, which says 3).
#if defined(DLSSIV_FG_CUT_STORE_APART) && !defined(DLSSIV_FG_CUT_POSITION)
#error "the cut fi-store-apart is interface 3, which has the position: build it with fi-position"
#endif
#ifdef DLSSIV_FG_CUT_POSITION
#ifdef DLSSIV_FG_CUT_STORE_APART
#define DLSSIV_FG_INTERFACE_BUILT 3u
#else
#define DLSSIV_FG_INTERFACE_BUILT 2u
#endif
#define DLSSIV_FG_FRAME_SIZE_BAD(s) ((s) != sizeof(DlssivFgFrameDesc) && (s) != DLSSIV_FG_FRAME_SIZE_1)
#else
#define DLSSIV_FG_INTERFACE_BUILT 1u
#define DLSSIV_FG_FRAME_SIZE_BAD(s) ((s) != DLSSIV_FG_FRAME_SIZE_1)
#endif
// THE GUIDES' IMAGES AT THE CAP (2026-10-25, the cut fi-guide-images-at-cap, a picture cut) stands on v-guide-cap: the passes after the
// prepare work at its cap, and this makes the images they work in at the cap too.
#if defined(DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP) && !defined(DLSSIV_FG_CUT_GUIDE_CAP)
#error "the cut fi-guide-images-at-cap makes the guides' images at v-guide-cap's cap: build it with v-guide-cap"
#endif

namespace
{
const uint32_t kMagic = 0x67664944u;       // what a context of this library begins with
enum { kSlotsMost = 64, kStampsASlot = DLSSIV_FG_STAMPS_A_SLOT };

// THE STAMPS' STATE: made only for a context that asked for them.
struct Stamps
{
    ID3D12QueryHeap*           heap;
    ID3D12Resource*            readback;
    uint32_t                   slots;
    bool                       barrier;
    ID3D12GraphicsCommandList* list;       // the list of the call being recorded; null outside a call: nothing is stamped then
    uint32_t                   slot;
    uint8_t                    call;
    uint32_t                   count[kSlotsMost], lost[kSlotsMost];
    DlssivFgMark*              marks;      // slots * kStampsASlot
    FfxGpuJobDescription*      jobs;       // FFX_MAX_GPU_JOBS: the effects' jobs, held until AMD's execute
    uint32_t                   jobCount;
};

// What stands IN FRONT OF the memory AMD's backend keeps its state in (its "scratch buffer"): every function of the
// backend's table is handed the interface, the interface holds that memory's address, and so a function of this file
// that stands in the table finds this from it. No static, no lookup.
struct alignas(64) Own
{
    FfxInterface real;                     // AMD's table as ffxGetInterfaceDX12 filled it
    uint32_t     pipelinesAsked, pipelinesRefused;
    Stamps*      stamps;                   // or null
};

Own* OwnOf(FfxInterface* i) { return (Own*)((uint8_t*)i->scratchBuffer - sizeof(Own)); }

struct PrepareRecord                       // what the provider keeps of a pair's prepare for its dispatch (two, by the id's last bit)
{
    bool     reset;
    uint32_t renderWidth, renderHeight;
    float    frameTimeMs, cameraNear, cameraFar, viewSpaceToMeters, cameraFovY;
};

struct Context
{
    uint32_t     magic;
    bool         check;
    FfxInterface shared, effects;          // the two backends' tables (the provider's backendInterfaceShared and backendInterfaceFi)
    void*        sharedMemory;
    void*        effectsMemory;
    uint32_t     sharedId;
    bool         haveShared, haveFlow, haveInterpolation;
    FfxResourceInternal images[FFX_FSR3_RESOURCE_IDENTIFIER_COUNT];
    uint32_t     set;                      // which of the two sets of the prepare's three images is the current one
    PrepareRecord prepared[2];
    uint64_t     lastId, announcedId;
    Stamps*      stamps;
    FfxOpticalflowContext        flow;
    FfxFrameInterpolationContext interpolation;
#ifdef DLSSIV_FG_CUT_GUIDE_CAP
    uint32_t     displayWidth, displayHeight;   // (the variant v-guide-cap: the cap is two thirds of these)
#endif
#ifdef DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP
    FfxApiDimensions2D guidesAt;           // (the cut fi-guide-images-at-cap) the size the guides' images were made at where it is the cap plus one texel; 0 by 0 where they are the guides' own
#endif
#ifdef DLSSIV_FG_CUT_ENV
    FfxResourceInternal env[4];            // (the variant v-env: the envelopes' four images, on the shared backend. [0] and [1] the brightness's, [2] and [3] the tint's; of each two the first is the one the last optical flow call wrote)
#endif
#ifdef DLSSIV_FG_CUT_STORE_APART
    bool         open;                     // (the cut fi-store-apart) the last interpolate was told NO_STORE: its pair, openId, has not kept its picture yet
    bool         openReset;                // and that dispatch reset
    uint64_t     openId;
#endif
};

Context* ContextOf(DlssivFgContext* h)
{
    Context* c = (Context*)h;
    return (c && c->magic == kMagic) ? c : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// IN FRONT OF AMD'S BACKEND
// ---------------------------------------------------------------------------------------------------------------------
FfxErrorCode OwnCreatePipeline(FfxInterface* i, FfxShaderBlob* blob, const FfxPipelineDescription* desc, FfxUInt32 id, FfxPipelineState* out)
{
    Own* const o = OwnOf(i);
    const FfxErrorCode rc = o->real.fpCreatePipeline(i, blob, desc, id, out);
    o->pipelinesAsked++;
    if (rc != FFX_OK) o->pipelinesRefused++;
    return rc;
}

void Stamp(Stamps* s, uint8_t kind, uint8_t job, bool wait, const wchar_t* label, const char* what)
{
    if (!s->list) return;
    uint32_t& n = s->count[s->slot];
    if (n >= kStampsASlot) { s->lost[s->slot]++; return; }
    const bool waited = wait && s->barrier;
    if (waited)
    {   // every unordered access before this point completes before anything behind it: the stamp waits for the job
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        s->list->ResourceBarrier(1, &b);
    }
    s->list->EndQuery(s->heap, D3D12_QUERY_TYPE_TIMESTAMP, s->slot * kStampsASlot + n);
    DlssivFgMark& m = s->marks[s->slot * kStampsASlot + n];
    m.call = s->call; m.kind = kind; m.job = job; m.waited = waited ? 1 : 0;
    size_t at = 0;
    if (label && label[0]) { for (; at + 1 < sizeof(m.name) && label[at]; at++) m.name[at] = (label[at] >= 32 && label[at] < 127 && label[at] != L',') ? (char)label[at] : '?'; }
    else { for (; at + 1 < sizeof(m.name) && what[at]; at++) m.name[at] = what[at]; }
    for (; at < sizeof(m.name); at++) m.name[at] = 0;
    n++;
}

FfxErrorCode OwnSchedule(FfxInterface* i, const FfxGpuJobDescription* job)
{
    Stamps* const s = OwnOf(i)->stamps;
    if (s->jobCount >= FFX_MAX_GPU_JOBS) return FFX_ERROR_OUT_OF_RANGE;
    s->jobs[s->jobCount++] = *job;
    return FFX_OK;
}

FfxErrorCode OwnQueryNext(FfxInterface* i, FfxGpuJobDescription** out)
{
    Stamps* const s = OwnOf(i)->stamps;
    if (s->jobCount >= FFX_MAX_GPU_JOBS) return FFX_ERROR_OUT_OF_RANGE;
    *out = &s->jobs[s->jobCount++];
    return FFX_OK;
}

FfxErrorCode OwnExecute(FfxInterface* i, FfxCommandList list, FfxUInt32 id)
{
    static const char* const kKind[5] = { "(clear)", "(copy)", "(compute)", "(barrier)", "(discard)" };
    Own* const o = OwnOf(i);
    Stamps* const s = o->stamps;
    FfxErrorCode rc = FFX_OK;
    for (uint32_t k = 0; k < s->jobCount && rc == FFX_OK; k++)
    {
        FfxGpuJobDescription* const job = &s->jobs[k];
        rc = o->real.fpScheduleGpuJob(i, job);
        if (rc == FFX_OK) rc = o->real.fpExecuteGpuJobs(i, list, id);
        const uint32_t type = (uint32_t)job->jobType;
        Stamp(s, DLSSIV_FG_MARK_JOB, (uint8_t)type, job->jobType != FFX_GPU_JOB_BARRIER, job->jobLabel, type < 5 ? kKind[type] : "(job)");
    }
    s->jobCount = 0;
    return rc;
}

FfxErrorCode OwnUnregister(FfxInterface* i, FfxCommandList list, FfxUInt32 id)
{
    Own* const o = OwnOf(i);
    const FfxErrorCode rc = o->real.fpUnregisterResources(i, list, id);
    Stamp(o->stamps, DLSSIV_FG_MARK_END, 0xFF, true, nullptr, "(end: the last barriers)");
    return rc;
}

// A backend as AMD's CreateBackend makes one (backend/dx12/ffx_backends_dx12.cpp 39 to 61: the device, the size AMD's
// backend asks for so many contexts, zeroed memory, AMD's table), with this file's own block in front of the memory.
FfxErrorCode MakeBackend(ID3D12Device* device, size_t contexts, FfxInterface* table, void** memory)
{
    const size_t scratch = ffxGetScratchMemorySizeDX12(contexts);
    uint8_t* const p = (uint8_t*)_aligned_malloc(sizeof(Own) + scratch, alignof(Own));
    if (!p) return FFX_ERROR_OUT_OF_MEMORY;
    memset(p, 0, sizeof(Own) + scratch);
    const FfxErrorCode rc = ffxGetInterfaceDX12(table, ffxGetDeviceDX12(device), p + sizeof(Own), scratch, contexts);
    if (rc != FFX_OK) { _aligned_free(p); return rc; }
    Own* const o = (Own*)p;
    o->real = *table;
    table->fpCreatePipeline = OwnCreatePipeline;
    *memory = p;
    return FFX_OK;
}

void FreeStamps(Stamps* s)
{
    if (!s) return;
    if (s->readback) s->readback->Release();
    if (s->heap) s->heap->Release();
    free(s->marks);
    free(s->jobs);
    free(s);
}

Stamps* MakeStamps(ID3D12Device* device, uint32_t slots, bool barrier)
{
    Stamps* const s = (Stamps*)calloc(1, sizeof(Stamps));
    if (!s) return nullptr;
    s->slots = slots; s->barrier = barrier;
    s->marks = (DlssivFgMark*)calloc((size_t)slots * kStampsASlot, sizeof(DlssivFgMark));
    s->jobs = (FfxGpuJobDescription*)calloc(FFX_MAX_GPU_JOBS, sizeof(FfxGpuJobDescription));
    D3D12_QUERY_HEAP_DESC qd = {};
    qd.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP; qd.Count = slots * kStampsASlot;
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; rd.Width = (UINT64)slots * kStampsASlot * sizeof(UINT64); rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
    rd.Format = DXGI_FORMAT_UNKNOWN; rd.SampleDesc.Count = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    const bool ok = s->marks && s->jobs
        && SUCCEEDED(device->CreateQueryHeap(&qd, __uuidof(ID3D12QueryHeap), (void**)&s->heap))
        && SUCCEEDED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&s->readback));
    if (!ok) { FreeStamps(s); return nullptr; }
    return s;
}

// A call of AMD's begins and ends: with the stamps on, the call's list, its slot and its head's stamp; and nothing after.
void CallBegins(Context* c, void* list, uint32_t slot, uint8_t call)
{
    Stamps* const s = c->stamps;
    if (!s) return;
    s->list = (slot < s->slots) ? (ID3D12GraphicsCommandList*)list : nullptr;
    s->slot = slot; s->call = call;
    Stamp(s, DLSSIV_FG_MARK_HEAD, 0xFF, true, nullptr, call == DLSSIV_FG_CALL_PREPARE ? "(head: prepare)" : call == DLSSIV_FG_CALL_OPTICAL_FLOW ? "(head: optical flow)" : "(head: interpolate)");
}
void CallEnds(Context* c)
{
    if (c->stamps) c->stamps->list = nullptr;
}

FfxApiResource Image(void* resource)
{
    return ffxApiGetResourceDX12((ID3D12Resource*)resource, FFX_API_RESOURCE_STATE_COMMON);
}
FfxApiResource Shared(Context* c, uint32_t id)
{
    return c->shared.fpGetResource(&c->shared, c->images[id]);
}

// AMD's provider's rule (561 to 568), with the prepare as the announcement.
bool ResetsNow(const Context* c, uint64_t id, bool told)
{
    const bool decreased = id < c->lastId;
    const bool skipped = (id - c->lastId) > 1;
    const bool disjoint = decreased || skipped || id != c->announcedId;
    return told || c->prepared[id % 2].reset || disjoint;
}

uint32_t Destroy(Context* c)
{
    // (the provider's order: the shared images, frame interpolation, optical flow, the shared backend's context, the memory)
    FfxErrorCode rc = FFX_OK, r;
    if (c->haveShared)
        for (uint32_t i = 0; i < FFX_FSR3_RESOURCE_IDENTIFIER_COUNT; i++)
        {
            r = c->shared.fpDestroyResource(&c->shared, c->images[i], c->sharedId);
            if (r != FFX_OK && rc == FFX_OK) rc = r;
        }
#ifdef DLSSIV_FG_CUT_ENV
    if (c->haveShared)
        for (uint32_t i = 0; i < 4; i++)
        {
            r = c->shared.fpDestroyResource(&c->shared, c->env[i], c->sharedId);
            if (r != FFX_OK && rc == FFX_OK) rc = r;
        }
#endif
    if (c->haveInterpolation) { r = ffxFrameInterpolationContextDestroy(&c->interpolation); if (r != FFX_OK && rc == FFX_OK) rc = r; }
    if (c->haveFlow)          { r = ffxOpticalflowContextDestroy(&c->flow); if (r != FFX_OK && rc == FFX_OK) rc = r; }
    if (c->haveShared)        { r = c->shared.fpDestroyBackendContext(&c->shared, c->sharedId); if (r != FFX_OK && rc == FFX_OK) rc = r; }
    FreeStamps(c->stamps);
    if (c->effectsMemory) _aligned_free(c->effectsMemory);
    if (c->sharedMemory) _aligned_free(c->sharedMemory);
    c->magic = 0;
    free(c);
    return (uint32_t)rc;
}

bool GoodCreate(const DlssivFgCreateDesc* d)
{
    return d && d->size == sizeof(DlssivFgCreateDesc) && d->device && d->displayWidth && d->displayHeight && d->maxRenderWidth && d->maxRenderHeight
        && ffxApiGetSurfaceFormatDX12((DXGI_FORMAT)d->backBufferFormat) != FFX_API_SURFACE_FORMAT_UNKNOWN;
}
FfxApiSurfaceFormat SourceFormat(const DlssivFgCreateDesc* d)
{
    const uint32_t hud = d->hudlessFormat ? ffxApiGetSurfaceFormatDX12((DXGI_FORMAT)d->hudlessFormat) : (uint32_t)FFX_API_SURFACE_FORMAT_UNKNOWN;
    return (FfxApiSurfaceFormat)(hud != FFX_API_SURFACE_FORMAT_UNKNOWN ? hud : ffxApiGetSurfaceFormatDX12((DXGI_FORMAT)d->backBufferFormat));
}
#ifdef DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP
// (the cut fi-guide-images-at-cap) WHERE THE GUIDES ARE LARGER THAN v-guide-cap's CAP ON BOTH AXES (DLAA), every pass after the
// prepare works at the cap, so the images those passes work in are made at the cap plus one texel instead of the guides'
// size: AMD's context is told that size as its largest render size, and the three shared images follow it (they are sized
// from the context's descriptions). The one texel more keeps the disocclusion mask's one linear sample (at the last column
// and row of the display) on a texel the region never wrote, as at the guides' size. -> the size, or 0 by 0 where the
// guides are at or under the cap (Quality and below: their own size, as before).
FfxApiDimensions2D GuideImagesAt(uint32_t displayW, uint32_t displayH, uint32_t maxW, uint32_t maxH)
{
    const uint32_t capW = (displayW * 2 + 2) / 3, capH = (displayH * 2 + 2) / 3;
    FfxApiDimensions2D at = { 0, 0 };
    if (capW && capH && maxW > capW + 1 && maxH > capH + 1) { at.width = capW + 1; at.height = capH + 1; }
    return at;
}
#endif
}   // namespace

extern "C"
{
// ---------------------------------------------------------------------------------------------------------------------
// WHAT THE LIBRARY IS, and with a description what a context of it will take (the provider's 516 to 544)
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgInfo(DlssivFgInfo* out, const DlssivFgCreateDesc* forMemory)
{
    if (!out || out->size != sizeof(DlssivFgInfo)) return DLSSIV_FG_E_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->size = sizeof(*out);
    out->iface = DLSSIV_FG_INTERFACE_BUILT; out->stage = DLSSIV_FG_STAGE_BUILT; out->caps = DLSSIV_FG_CAP_STAMPS;
    out->amdId = (0xF6000000ull << 32) | (uint64_t)(FFX_SDK_MAKE_VERSION(FFX_FRAMEINTERPOLATION_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_VERSION_MINOR, FFX_FRAMEINTERPOLATION_VERSION_PATCH) & 0xFFFFFFFFu);
    _snprintf_s(out->amdVersion, sizeof(out->amdVersion), _TRUNCATE, "%d.%d.%d", FFX_FRAMEINTERPOLATION_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_VERSION_MINOR, FFX_FRAMEINTERPOLATION_VERSION_PATCH);
#ifdef DLSSIV_FG_CUTS
    _snprintf_s(out->build, sizeof(out->build), _TRUNCATE, "dlssiv-fg stage 3: AMD FidelityFX SDK 2.3.0 source, %d cut(s) %s", DLSSIV_FG_CUTS_COUNT, DLSSIV_FG_CUTS_ID);
#else
    strcpy_s(out->build, sizeof(out->build), "dlssiv-fg stage 2: AMD FidelityFX SDK 2.3.0 source, the mod's interface 1");
#endif
    if (!forMemory) return DLSSIV_FG_OK;
    if (!GoodCreate(forMemory)) return DLSSIV_FG_E_ARGUMENT;
    FfxApiDimensions2D render = { forMemory->maxRenderWidth, forMemory->maxRenderHeight }, display = { forMemory->displayWidth, forMemory->displayHeight };
#ifdef DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP
    {   // (the cut fi-guide-images-at-cap) the size the create will make the guides' images at
        const FfxApiDimensions2D at = GuideImagesAt(display.width, display.height, render.width, render.height);
        if (at.width) render = at;
    }
#endif
    FfxApiEffectMemoryUsage interpolation = {}, flow = {};
    FfxErrorCode rc = ffxFrameInterpolationGetGpuMemoryUsage(ffxGetDeviceDX12((ID3D12Device*)forMemory->device), &render, &display, SourceFormat(forMemory), &interpolation);
    if (rc == FFX_OK) rc = ffxOpticalflowGetGpuMemoryUsage(ffxGetDeviceDX12((ID3D12Device*)forMemory->device), &display, &flow);
    if (rc != FFX_OK) return (uint32_t)rc;
    out->contextBytes = interpolation.totalUsageInBytes + flow.totalUsageInBytes;
    out->aliasableBytes = interpolation.aliasableUsageInBytes + flow.aliasableUsageInBytes;
    return DLSSIV_FG_OK;
}

// ---------------------------------------------------------------------------------------------------------------------
// THE CREATE (the provider's 88 to 189). On any failure everything that was made is given back and *out stays null.
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgCreate(const DlssivFgCreateDesc* d, DlssivFgContext** out, DlssivFgCreated* created)
{
    if (!out) return DLSSIV_FG_E_ARGUMENT;
    *out = nullptr;
    if (!GoodCreate(d) || (created && created->size != sizeof(DlssivFgCreated))) return DLSSIV_FG_E_ARGUMENT;
    const bool wantStamps = (d->flags & DLSSIV_FG_CREATE_STAMPS) != 0;
    if (wantStamps && (d->stampSlots < 1 || d->stampSlots > kSlotsMost)) return DLSSIV_FG_E_ARGUMENT;
    ID3D12Device* const device = (ID3D12Device*)d->device;
    Context* const c = (Context*)calloc(1, sizeof(Context));
    if (!c) return DLSSIV_FG_E_MEMORY;
    c->magic = kMagic;
    c->check = (d->flags & DLSSIV_FG_CREATE_CHECK) != 0;
#ifdef DLSSIV_FG_CUT_GUIDE_CAP
    c->displayWidth = d->displayWidth; c->displayHeight = d->displayHeight;
#endif
    if (d->message) ffxFrameInterpolationSetGlobalDebugMessage((ffxMessageCallback)d->message, FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS);

    FfxErrorCode rc = MakeBackend(device, 1, &c->shared, &c->sharedMemory);
    if (rc == FFX_OK) rc = MakeBackend(device, 2, &c->effects, &c->effectsMemory);
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
    if (wantStamps)
    {
        c->stamps = MakeStamps(device, d->stampSlots, (d->flags & DLSSIV_FG_CREATE_STAMPS_NO_BARRIER) == 0);
        if (!c->stamps) { Destroy(c); return DLSSIV_FG_E_STAMPS; }
        OwnOf(&c->effects)->stamps = c->stamps;
        c->effects.fpScheduleGpuJob      = OwnSchedule;
        c->effects.fpQueryNextGpuJobDesc = OwnQueryNext;
        c->effects.fpExecuteGpuJobs      = OwnExecute;
        c->effects.fpUnregisterResources = OwnUnregister;
    }

    rc = c->shared.fpCreateBackendContext(&c->shared, FFX_EFFECT_SHAREDAPIBACKEND, nullptr, &c->sharedId);
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
    c->haveShared = true;

    FfxOpticalflowContextDescription flow = {};
    flow.backendInterface  = c->effects;
    flow.resolution.width  = d->displayWidth;
    flow.resolution.height = d->displayHeight;
    rc = ffxOpticalflowContextCreate(&c->flow, &flow);
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
    c->haveFlow = true;

    FfxFrameInterpolationContextDescription fi = { 0 };
    fi.backendInterface = c->effects;
    fi.flags |= (d->flags & DLSSIV_FG_CREATE_DEPTH_INVERTED) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INVERTED : 0;
    fi.flags |= (d->flags & DLSSIV_FG_CREATE_DEPTH_INFINITE) ? FFX_FRAMEINTERPOLATION_ENABLE_DEPTH_INFINITE : 0;
    fi.flags |= (d->flags & DLSSIV_FG_CREATE_HDR) ? FFX_FRAMEINTERPOLATION_ENABLE_HDR_COLOR_INPUT : 0;
    fi.flags |= (d->flags & DLSSIV_FG_CREATE_CHECK) ? FFX_FRAMEINTERPOLATION_ENABLE_DEBUG_CHECKING : 0;
    fi.maxRenderSize.width  = d->maxRenderWidth;
    fi.maxRenderSize.height = d->maxRenderHeight;
#ifdef DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP
    c->guidesAt = GuideImagesAt(d->displayWidth, d->displayHeight, d->maxRenderWidth, d->maxRenderHeight);   // (the cut fi-guide-images-at-cap)
    if (c->guidesAt.width) { fi.maxRenderSize.width = c->guidesAt.width; fi.maxRenderSize.height = c->guidesAt.height; }
#endif
    fi.displaySize.width    = d->displayWidth;
    fi.displaySize.height   = d->displayHeight;
    fi.backBufferFormat     = (FfxApiSurfaceFormat)ffxApiGetSurfaceFormatDX12((DXGI_FORMAT)d->backBufferFormat);
    fi.previousInterpolationSourceFormat = d->hudlessFormat ? SourceFormat(d) : fi.backBufferFormat;
    rc = ffxFrameInterpolationContextCreate(&c->interpolation, &fi);
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
    c->haveInterpolation = true;

    // THE IMAGES THE TWO EFFECTS SHARE, made on the shared backend in the provider's order and by its names
    FfxOpticalflowSharedResourceDescriptions flowImages = {};
    rc = ffxOpticalflowGetSharedResourceDescriptions(&c->flow, &flowImages);
    if (rc == FFX_OK) rc = c->shared.fpCreateResource(&c->shared, &flowImages.opticalFlowVector, c->sharedId, &c->images[FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR]);
    if (rc == FFX_OK) rc = c->shared.fpCreateResource(&c->shared, &flowImages.opticalFlowSCD, c->sharedId, &c->images[FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCD_OUTPUT]);
    FfxFrameInterpolationSharedResourceDescriptions fiImages = {};
    if (rc == FFX_OK) rc = ffxFrameInterpolationGetSharedResourceDescriptions(&c->interpolation, &fiImages);
    wchar_t name[256] = {};
#ifdef DLSSIV_FG_CUT_ONE_SHARED_SET
    // (the cut fi-one-shared-set) ONE SET OF THE THREE, NOT TWO. AMD's provider makes two so that an upscaler on another queue
    // can write frame N+1's while frame generation reads frame N's. Here the prepare and the interpolate are on the helper's
    // one queue in submission order, and the interpolate reads the newest prepare's set and no other: the second was never read
    for (FfxUInt32 i = 0; i < 1 && rc == FFX_OK; i++)
#else
    for (FfxUInt32 i = 0; i < 2 && rc == FFX_OK; i++)
#endif
    {
        FfxCreateResourceDescription depth = fiImages.dilatedDepth;
        swprintf(name, 255, L"%s%d", fiImages.dilatedDepth.name, i);
        depth.name = name;
        rc = c->shared.fpCreateResource(&c->shared, &depth, c->sharedId, &c->images[FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_DEPTH_0 + (i * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT)]);
        if (rc != FFX_OK) break;
        FfxCreateResourceDescription vectors = fiImages.dilatedMotionVectors;
        swprintf(name, 255, L"%s%d", fiImages.dilatedMotionVectors.name, i);
        vectors.name = name;
        rc = c->shared.fpCreateResource(&c->shared, &vectors, c->sharedId, &c->images[FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS_0 + (i * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT)]);
        if (rc != FFX_OK) break;
        FfxCreateResourceDescription previous = fiImages.reconstructedPrevNearestDepth;
        swprintf(name, 255, L"%s%d", fiImages.reconstructedPrevNearestDepth.name, i);
        previous.name = name;
        rc = c->shared.fpCreateResource(&c->shared, &previous, c->sharedId, &c->images[FFX_FSR3_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH_0 + (i * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT)]);
    }
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
#ifdef DLSSIV_FG_CUT_ENV
    // (THE VARIANT v-env, NEVER THE PRODUCT: ffx-own\cuts\v-env.cut) THE FOUR IMAGES OF THE ENVELOPES, made on the shared backend as
    // the images above are, because two effects use them: optical flow's first pass writes them (it has the picture's
    // pixels in hand) and the interpolation reads them. The brightness's: one texel for two by two pixels of the
    // display, two channels of 16-bit float; the tint's: one for four by four, four channels. Nothing reads one
    // before it was written: the pass takes nothing from the frame before in the dispatch of a reset and in the first.
    for (uint32_t i = 0; i < 4 && rc == FFX_OK; i++)
    {
        static const wchar_t* const envNames[4] = { L"DLSSIV_EnvA", L"DLSSIV_EnvB", L"DLSSIV_EnvTintA", L"DLSSIV_EnvTintB" };
        const uint32_t side = i < 2 ? 2u : 4u;
        const FfxCreateResourceDescription env = { FfxResourceHeapPlacementInfo::InitDefault(),
            { FFX_API_RESOURCE_TYPE_TEXTURE2D, (uint32_t)(i < 2 ? FFX_API_SURFACE_FORMAT_R16G16_FLOAT : FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT), (d->displayWidth + side - 1) / side, (d->displayHeight + side - 1) / side, 1, 1, FFX_API_RESOURCE_FLAGS_NONE, FFX_API_RESOURCE_USAGE_UAV },
            FFX_API_RESOURCE_STATE_UNORDERED_ACCESS, envNames[i], 0, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED} };
        rc = c->shared.fpCreateResource(&c->shared, &env, c->sharedId, &c->env[i]);
    }
    if (rc != FFX_OK) { Destroy(c); return (uint32_t)rc; }
#endif

    // WHAT AMD'S CODE DID NOT READ: the device's answers to its 18 pipelines; and what it read of the device
    Own* const o = OwnOf(&c->effects);
    FfxDeviceCapabilities caps = {};
    c->effects.fpGetDeviceCapabilities(&c->effects, &caps);
    FfxApiEffectMemoryUsage a = {}, b = {}, s = {};
    ffxFrameInterpolationContextGetGpuMemoryUsage(&c->interpolation, &a);
    ffxOpticalflowContextGetGpuMemoryUsage(&c->flow, &b);
    ffxSharedContextGetGpuMemoryUsage(&c->shared, &s);
    if (created)
    {
        const uint32_t size = created->size;
        memset(created, 0, sizeof(*created));
        created->size = size;
        created->pipelinesAsked = o->pipelinesAsked; created->pipelinesRefused = o->pipelinesRefused;
        const uint32_t model = (uint32_t)caps.maximumSupportedShaderModel;
        created->shaderModel = model == (uint32_t)FFX_SHADER_MODEL_5_1 ? 0x51u : 0x60u + (model - (uint32_t)FFX_SHADER_MODEL_6_0);
        created->waveLaneMin = caps.waveLaneCountMin; created->waveLaneMax = caps.waveLaneCountMax;
        created->fp16 = caps.fp16Supported ? 1u : 0u;
        // (the test of both effects' createPipelineStates, word for word)
        created->wave64 = (caps.waveLaneCountMin == 32 && caps.waveLaneCountMax == 64 && caps.maximumSupportedShaderModel >= FFX_SHADER_MODEL_6_6) ? 1u : 0u;
        created->stamps = !c->stamps ? 0u : c->stamps->barrier ? 1u : 2u;
        created->contextBytes = a.totalUsageInBytes + b.totalUsageInBytes + s.totalUsageInBytes;
        created->aliasableBytes = a.aliasableUsageInBytes + b.aliasableUsageInBytes + s.aliasableUsageInBytes;
    }
    if (o->pipelinesRefused || o->pipelinesAsked != (uint32_t)FFX_FRAMEINTERPOLATION_PASS_COUNT + (uint32_t)FFX_OPTICALFLOW_PASS_COUNT) { Destroy(c); return DLSSIV_FG_E_PIPELINE; }
    *out = (DlssivFgContext*)c;
    return DLSSIV_FG_OK;
}

__declspec(dllexport) uint32_t dlssivFgDestroy(DlssivFgContext* context)
{
    Context* const c = ContextOf(context);
    if (!c) return DLSSIV_FG_E_CONTEXT;
    return Destroy(c);
}

// ---------------------------------------------------------------------------------------------------------------------
// THE PREPARE (the provider's 679 to 773)
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgPrepare(DlssivFgContext* context, const DlssivFgPrepareDesc* d)
{
    Context* const c = ContextOf(context);
    if (!c) return DLSSIV_FG_E_CONTEXT;
    if (!d || d->size != sizeof(DlssivFgPrepareDesc) || !d->commandList || !d->depth || !d->motionVectors) return DLSSIV_FG_E_ARGUMENT;
#ifdef DLSSIV_FG_CUT_GUIDE_IMAGES_AT_CAP
    // (the cut fi-guide-images-at-cap) the guides' images are the cap plus one texel: a prepare larger than that fits them only
    // where v-guide-cap below takes it down to the cap (its own test, word for word); any other is refused before anything is
    // recorded or kept. The helper's own prepare is always taken down (its guides are its own textures at the render size).
    if (c->guidesAt.width && (d->renderWidth > c->guidesAt.width || d->renderHeight > c->guidesAt.height))
    {
        const uint32_t capW = (c->displayWidth * 2 + 2) / 3, capH = (c->displayHeight * 2 + 2) / 3;
        const D3D12_RESOURCE_DESC dd = ((ID3D12Resource*)d->depth)->GetDesc(), md = ((ID3D12Resource*)d->motionVectors)->GetDesc();
        if (!(capW && capH && d->renderWidth > capW && d->renderHeight > capH && dd.Width == d->renderWidth && dd.Height == d->renderHeight && md.Width == dd.Width && md.Height == dd.Height))
            return DLSSIV_FG_E_ARGUMENT;
    }
#endif
    PrepareRecord& r = c->prepared[d->frameId % 2];
    r.reset = (d->flags & DLSSIV_FG_RESET) != 0;
    r.renderWidth = d->renderWidth; r.renderHeight = d->renderHeight;
    r.frameTimeMs = d->frameTimeMs; r.cameraNear = d->cameraNear; r.cameraFar = d->cameraFar; r.viewSpaceToMeters = d->viewSpaceToMeters; r.cameraFovY = d->cameraFovY;
    c->announcedId = d->frameId;
#ifndef DLSSIV_FG_CUT_ONE_SHARED_SET
    c->set = (c->set + 1) & 1;
#endif

    FfxFrameInterpolationPrepareDescription p = { 0 };
    p.flags = 0;
    p.commandList = ffxGetCommandListDX12((ID3D12CommandList*)d->commandList);
    p.renderSize.width = d->renderWidth; p.renderSize.height = d->renderHeight;
    p.jitterOffset.x = d->jitterX; p.jitterOffset.y = d->jitterY;
    p.motionVectorScale.x = d->motionVectorScaleX; p.motionVectorScale.y = d->motionVectorScaleY;
    p.frameTimeDelta = d->frameTimeMs;
    p.cameraNear = d->cameraNear; p.cameraFar = d->cameraFar;
    p.viewSpaceToMetersFactor = d->viewSpaceToMeters;
    p.cameraFovAngleVertical = d->cameraFovY;
    p.depth = Image(d->depth);
    p.motionVectors = Image(d->motionVectors);
    p.frameID = d->frameId;
#ifdef DLSSIV_FG_CUT_GUIDE_CAP
    // (THE VARIANT v-guide-cap, NEVER THE PRODUCT: ffx-own\cuts\v-guide-cap.cut) AMD's code is told that the guides have two
    // thirds of the display on each axis; its prepare pass reads the game's depth and vectors at their own size. The
    // vector scale and the jitter are in the guides' pixels and AMD divides both by the size it is told: scaled with it.
    // Only where the game's two textures are exactly the size the mod said: the pass reads the size from the depth.
    {
        const uint32_t capW = (c->displayWidth * 2 + 2) / 3, capH = (c->displayHeight * 2 + 2) / 3;
        const D3D12_RESOURCE_DESC dd = ((ID3D12Resource*)d->depth)->GetDesc(), md = ((ID3D12Resource*)d->motionVectors)->GetDesc();
        if (capW && capH && d->renderWidth > capW && d->renderHeight > capH && dd.Width == d->renderWidth && dd.Height == d->renderHeight && md.Width == dd.Width && md.Height == dd.Height)
        {
            const float sx = (float)capW / (float)d->renderWidth, sy = (float)capH / (float)d->renderHeight;
            r.renderWidth = capW; r.renderHeight = capH;
            p.renderSize.width = capW; p.renderSize.height = capH;
            p.jitterOffset.x *= sx; p.jitterOffset.y *= sy;
            p.motionVectorScale.x *= sx; p.motionVectorScale.y *= sy;
        }
    }
#endif
    p.dilatedDepth           = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_DEPTH_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    p.dilatedMotionVectors   = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    p.reconstructedPrevDepth = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    memcpy(p.cameraPosition, d->cameraPosition, sizeof(FfxFloat32x3));
    memcpy(p.cameraUp, d->cameraUp, sizeof(FfxFloat32x3));
    memcpy(p.cameraRight, d->cameraRight, sizeof(FfxFloat32x3));
    memcpy(p.cameraForward, d->cameraForward, sizeof(FfxFloat32x3));
    if (c->check)
    {
        static const FfxFloat32x3 zero = { 0.f, 0.f, 0.f };
        if (memcmp(d->cameraPosition, zero, sizeof(zero)) == 0 && memcmp(d->cameraUp, zero, sizeof(zero)) == 0 && memcmp(d->cameraRight, zero, sizeof(zero)) == 0 && memcmp(d->cameraForward, zero, sizeof(zero)) == 0)
            FFX_PRINT_MESSAGE(FFX_API_MESSAGE_TYPE_WARNING, L"Camera view matrix parameters (cameraPosition, cameraUp, cameraRight, cameraForward) are all zero vectors, indicating they remain at their default initialized values. These parameters must be properly set by the application for optimal MLFI quality.");
    }
    CallBegins(c, d->commandList, d->stampSlot, DLSSIV_FG_CALL_PREPARE);
    const FfxErrorCode rc = ffxFrameInterpolationPrepare(&c->interpolation, &p);
    CallEnds(c);
    return (uint32_t)rc;
}

// ---------------------------------------------------------------------------------------------------------------------
// OPTICAL FLOW (the provider's 570 to 588)
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgOpticalFlow(DlssivFgContext* context, const DlssivFgFrameDesc* d)
{
    Context* const c = ContextOf(context);
    if (!c) return DLSSIV_FG_E_CONTEXT;
    if (!d || DLSSIV_FG_FRAME_SIZE_BAD(d->size) || !d->commandList || !d->present) return DLSSIV_FG_E_ARGUMENT;
    FfxOpticalflowDispatchDescription o{};
    o.commandList = ffxGetCommandListDX12((ID3D12CommandList*)d->commandList);
    o.color = Image(d->present);
    if (d->hudless) o.color = Image(d->hudless);
    o.reset = ResetsNow(c, d->frameId, (d->flags & DLSSIV_FG_RESET) != 0);
    o.backbufferTransferFunction = (int)d->transfer;
    o.minMaxLuminance.x = d->minLuminance;
    o.minMaxLuminance.y = d->maxLuminance;
    o.opticalFlowVector = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR);
    o.opticalFlowSCD    = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCD_OUTPUT);
#ifdef DLSSIV_FG_CUT_ENV
    // (the variant v-env) the envelopes' images change places, each two: what the call before wrote is the frame before's now
    {
        const FfxResourceInternal brightness = c->env[0], tint = c->env[2];
        c->env[0] = c->env[1]; c->env[1] = brightness;
        c->env[2] = c->env[3]; c->env[3] = tint;
    }
    o.envCurrent      = c->shared.fpGetResource(&c->shared, c->env[0]);
    o.envPrevious     = c->shared.fpGetResource(&c->shared, c->env[1]);
    o.envTintCurrent  = c->shared.fpGetResource(&c->shared, c->env[2]);
    o.envTintPrevious = c->shared.fpGetResource(&c->shared, c->env[3]);
    o.envFrameTimeMs  = c->prepared[d->frameId % 2].frameTimeMs;
#endif
    CallBegins(c, d->commandList, d->stampSlot, DLSSIV_FG_CALL_OPTICAL_FLOW);
    const FfxErrorCode rc = ffxOpticalflowContextDispatch(&c->flow, &o);
    CallEnds(c);
    return (uint32_t)rc;
}

// ---------------------------------------------------------------------------------------------------------------------
// INTERPOLATE (the provider's 590 to 671)
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgInterpolate(DlssivFgContext* context, const DlssivFgFrameDesc* d)
{
    Context* const c = ContextOf(context);
    if (!c) return DLSSIV_FG_E_CONTEXT;
#ifdef DLSSIV_FG_CUT_STORE_APART
    // (the cut fi-store-apart, interface 3) a call that only keeps the pair needs no output; the two flags together are refused
    if (!d || DLSSIV_FG_FRAME_SIZE_BAD(d->size) || !d->commandList || !d->present) return DLSSIV_FG_E_ARGUMENT;
    const bool noStore = (d->flags & DLSSIV_FG_NO_STORE) != 0, storeOnly = (d->flags & DLSSIV_FG_STORE_ONLY) != 0;
    if ((!d->output && !storeOnly) || (noStore && storeOnly)) return DLSSIV_FG_E_ARGUMENT;
#else
    if (!d || DLSSIV_FG_FRAME_SIZE_BAD(d->size) || !d->commandList || !d->present || !d->output) return DLSSIV_FG_E_ARGUMENT;
#endif
#ifdef DLSSIV_FG_CUT_POSITION
    // (the cut fi-position) WHERE THE GENERATED FRAME STANDS: interface 1's description and 0 are the middle; another value is
    // held within the two bounds; one that is not a number is refused before anything is recorded
    float position = (d->size == sizeof(DlssivFgFrameDesc)) ? d->position : 0.0f;
    if (position != position) return DLSSIV_FG_E_ARGUMENT;
#ifdef DLSSIV_FG_CUT_POSITION_CONTROL
    if (position == 0.0f) position = 0.4f;   // (THE CONTROL control-position, NEVER THE PRODUCT: the middle asked, 0.4 told)
#endif
    position = (position == 0.0f) ? 0.5f : (position < DLSSIV_FG_POSITION_LEAST) ? DLSSIV_FG_POSITION_LEAST : (position > DLSSIV_FG_POSITION_MOST) ? DLSSIV_FG_POSITION_MOST : position;
#endif
    const PrepareRecord& r = c->prepared[d->frameId % 2];
    FfxFrameInterpolationDispatchDescription f{ 0 };
    f.commandList = ffxGetCommandListDX12((ID3D12CommandList*)d->commandList);
    f.currentBackBuffer = Image(d->present);
    f.displaySize.width  = f.currentBackBuffer.description.width;
    f.displaySize.height = f.currentBackBuffer.description.height;
    if (d->hudless) f.currentBackBuffer_HUDLess = Image(d->hudless);
    f.reset = ResetsNow(c, d->frameId, (d->flags & DLSSIV_FG_RESET) != 0);
#ifdef DLSSIV_FG_CUT_STORE_APART
    // (the cut fi-store-apart) A PAIR LEFT OPEN by NO_STORE and not closed: the picture kept is not this pair's previous one, so
    // this dispatch resets. A later frame of the open pair (the same id) does not count the pair again (AGAIN)
    // A later frame of a pair whose first dispatch reset resets too: every frame of a pair is made as a single dispatch at its
    // position would make it
    const bool again = c->open && d->frameId == c->openId;
    if (c->open && !again) f.reset = true;
    if (again && c->openReset) f.reset = true;
    f.storeApart = (noStore ? FFX_FRAMEINTERPOLATION_STORE_APART_NO_STORE : 0u) | (storeOnly ? FFX_FRAMEINTERPOLATION_STORE_APART_STORE_ONLY : 0u)
                 | (again ? FFX_FRAMEINTERPOLATION_STORE_APART_AGAIN : 0u);
#ifdef DLSSIV_FG_CUT_STORE_FIRST_CONTROL
    f.storeApart &= ~FFX_FRAMEINTERPOLATION_STORE_APART_NO_STORE;   // (THE CONTROL control-store-first, NEVER THE PRODUCT: NO_STORE asked, the store made)
#endif
#ifdef DLSSIV_FG_CUT_COUNT_AGAIN_CONTROL
    f.storeApart &= ~FFX_FRAMEINTERPOLATION_STORE_APART_AGAIN;      // (THE CONTROL control-count-again, NEVER THE PRODUCT: every dispatch of a pair counted)
#endif
#endif
    f.renderSize.width  = r.renderWidth;
    f.renderSize.height = r.renderHeight;
    f.output = Image(d->output);
    f.opticalFlowVector = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_VECTOR);
    f.opticalFlowSceneChangeDetection = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_OPTICAL_FLOW_SCD_OUTPUT);
    f.opticalFlowBlockSize = 8;
    f.opticalFlowScale = { 1.f / f.displaySize.width, 1.f / f.displaySize.height };
    f.frameTimeDelta = r.frameTimeMs;
    f.cameraNear = r.cameraNear;
    f.cameraFar = r.cameraFar;
    f.viewSpaceToMetersFactor = r.viewSpaceToMeters;
    f.cameraFovAngleVertical = r.cameraFovY;
    f.dilatedDepth           = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_DEPTH_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    f.dilatedMotionVectors   = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_DILATED_MOTION_VECTORS_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    f.reconstructedPrevDepth = Shared(c, FFX_FSR3_RESOURCE_IDENTIFIER_RECONSTRUCTED_PREVIOUS_NEAREST_DEPTH_0 + (c->set * FFX_FSR3_RESOURCE_IDENTIFIER_UPSCALED_COUNT));
    if (d->rectHeight == 0 && d->rectWidth == 0)
    {
        f.interpolationRect.left   = 0;
        f.interpolationRect.top    = 0;
        f.interpolationRect.width  = f.currentBackBuffer.description.width;
        f.interpolationRect.height = f.currentBackBuffer.description.height;
    }
    else
    {
        f.interpolationRect.top    = d->rectTop;
        f.interpolationRect.left   = d->rectLeft;
        f.interpolationRect.width  = d->rectWidth;
        f.interpolationRect.height = d->rectHeight;
    }
    if (d->flags & DLSSIV_FG_DEBUG_TEAR_LINES)       f.flags |= FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_TEAR_LINES;
    if (d->flags & DLSSIV_FG_DEBUG_RESET_INDICATORS) f.flags |= FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_RESET_INDICATORS;
    if (d->flags & DLSSIV_FG_DEBUG_VIEW)             f.flags |= FFX_FRAMEINTERPOLATION_DISPATCH_DRAW_DEBUG_VIEW;
    f.backBufferTransferFunction = (FfxApiBackbufferTransferFunction)d->transfer;
    f.minMaxLuminance[0] = d->minLuminance;
    f.minMaxLuminance[1] = d->maxLuminance;
    f.frameID = d->frameId;
#ifdef DLSSIV_FG_CUT_POSITION
    f.interpolationPosition = position;
#endif
#ifdef DLSSIV_FG_CUT_ENV
    f.envCurrent     = c->shared.fpGetResource(&c->shared, c->env[0]);   // (the variant v-env) as this pair's optical flow call wrote them
    f.envTintCurrent = c->shared.fpGetResource(&c->shared, c->env[2]);
#endif
    CallBegins(c, d->commandList, d->stampSlot, DLSSIV_FG_CALL_INTERPOLATE);
    const FfxErrorCode rc = ffxFrameInterpolationDispatch(&c->interpolation, &f);
    CallEnds(c);
    if (rc != FFX_OK) return (uint32_t)rc;
#ifdef DLSSIV_FG_CUT_STORE_APART
    c->open = noStore; c->openId = d->frameId; c->openReset = f.reset;
#endif
    c->lastId = d->frameId;
    return DLSSIV_FG_OK;
}

// ---------------------------------------------------------------------------------------------------------------------
// THE STAMPS (development): a slot begins, is resolved into the list, and is read once the list has run
// ---------------------------------------------------------------------------------------------------------------------
__declspec(dllexport) uint32_t dlssivFgStamps(DlssivFgContext* context, DlssivFgStampsDesc* d)
{
    Context* const c = ContextOf(context);
    if (!c) return DLSSIV_FG_E_CONTEXT;
    Stamps* const s = c->stamps;
    if (!s) return DLSSIV_FG_E_STAMPS;
    if (!d || d->size != sizeof(DlssivFgStampsDesc) || d->slot >= s->slots) return DLSSIV_FG_E_ARGUMENT;
    const uint32_t first = d->slot * kStampsASlot;
    if (d->op == DLSSIV_FG_STAMPS_BEGIN)
    {
        s->count[d->slot] = 0; s->lost[d->slot] = 0;
        return DLSSIV_FG_OK;
    }
    if (d->op == DLSSIV_FG_STAMPS_RESOLVE)
    {
        if (!d->commandList) return DLSSIV_FG_E_ARGUMENT;
        if (s->count[d->slot])
            ((ID3D12GraphicsCommandList*)d->commandList)->ResolveQueryData(s->heap, D3D12_QUERY_TYPE_TIMESTAMP, first, s->count[d->slot], s->readback, (UINT64)first * sizeof(UINT64));
        return DLSSIV_FG_OK;
    }
    if (d->op == DLSSIV_FG_STAMPS_READ)
    {
        const uint32_t n = s->count[d->slot];
        d->count = n; d->lost = s->lost[d->slot];
        if (!n) return DLSSIV_FG_OK;
        if (!d->ticks || !d->marks || d->capacity < n) return DLSSIV_FG_E_ARGUMENT;
        const D3D12_RANGE range = { (SIZE_T)first * sizeof(UINT64), ((SIZE_T)first + n) * sizeof(UINT64) };
        void* p = nullptr;
        if (FAILED(s->readback->Map(0, &range, &p)) || !p) return DLSSIV_FG_E_STAMPS;
        memcpy(d->ticks, (const uint8_t*)p + range.Begin, (size_t)n * sizeof(UINT64));
        const D3D12_RANGE none = { 0, 0 };
        s->readback->Unmap(0, &none);
        memcpy(d->marks, s->marks + first, (size_t)n * sizeof(DlssivFgMark));
        return DLSSIV_FG_OK;
    }
    return DLSSIV_FG_E_ARGUMENT;
}
}   // extern "C"
