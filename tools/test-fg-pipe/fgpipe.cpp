// THE FAKE CLIENT (2026-10-05, frame generation phase 2, stage 4; docs/DECISIONS.md "PHASE 2: THE BUILD MAP'S ROUND" and
// "PHASE 2, STAGE 0"; docs/reports/fg-2/map-2026-10-05/critic.md, section 3, K4).
//
// A small 32-BIT program that is "the game" to the BUILT helper, with no game: it starts host\dlssiv-helper.exe with its own
// pid, speaks the real protocol over the real pipe, and holds the helper's shared images and fences through VULKAN, with the
// mod's own import code (source/transport/feed_vk.h, included unchanged), as the game's DXVK device does. So the things a
// phase that presents nothing cannot show on a screen are proved at a desk, against the real helper:
//   - what the wire MEANS, not only how long its structs are (pixels in, pixels out);
//   - that a wait is released by the signal it waits for AND BY NOTHING ELSE (the control below);
//   - what an import costs a 32-bit process in address space.
// IT INCLUDES feed_ipc.h, feed_fg.h AND feed_vk.h AND NOTHING OF host\: an instrument that shared code with the helper
// would agree with the helper's mistakes.
//
// STAGE 4 (this file's first form) proves the INSTRUMENT, against the helper as it is, before anything is proved with it:
// hello, a transport build ('B': the helper copies Color to Output), frames ('F') with known pixels in Color read back
// from Output, and THE CONTROL: a frame whose in-fence is NOT signalled must not be answered (the out-fence does not move
// and Output keeps the frame before), and is answered once the fence is signalled. Frame generation's messages come in
// the stages that add them to the helper.
//
// STAGE 5 (negotiation): what the hello ack says of frame generation, on every kind of session. Three options of this
// program's own, which run.py combines into its sessions (every other argument goes to the helper as it is):
//     --fake-want-fg=N     the caps bits 256 and 1024 must read N: 0 neither (not asked), 1 asked and nothing usable
//                          (256 without 1024), 3 asked and available. (1024 since the wire's revision of 2026-10-06; 512
//                          was wire 1's and must NEVER be offered: every session proves that too)
//     --fake-hello-only    the session ends after the hello: the pipe is closed and the helper must end by itself
//     --fake-send-tag=c    after the hello, write the one tag byte c and NOTHING ELSE, and keep the pipe open: the helper
//                          must end the session by itself (a tag it does not serve)
//
// STAGE 6 (the helper's half of the wire): after the run of stage 4, frame generation as the game will speak it.
//     --fake-fg=1          the whole of it: 'g' and its ack, the imports, whole pairs with the generated image read by
//                          pixels, THE HELD 'Q' (the frame's in-fence withheld: fgPrep must not come) and THE HELD 'G'
//                          (fgIn withheld: fgOut must not come, and the next frame waits behind it: one queue, measured),
//                          the kinds with no frame, a repeated 'Q', a 'G' with no 'Q', a refused 'g', a second format,
//                          the release, twenty acquire and release cycles, a rebuild with frame generation alive
//     --fake-fg=2          the raise lever (the helper started with --fgraise_at=2): the second dispatch raises; every
//                          later pair has no frame, the upscaler's frames go on by pixels, the next 'g' is refused
//     --fake-fg=3          the failure lever (--fgfail_at=2): three dispatches fail, the fourth is not tried, a new 'g'
//                          starts afresh
//     --fake-fg=4          the video memory rule forced (--fgvram_refuse=1): 'g' refused, frames go on
//     --fake-fg-stamp      with 1: ask for the three constants ('S', a development helper) and read each image through
//                          this side's imports, where the machine's order says it is
// STAGE 7 (the game's half): THE SET IS ACQUIRED AND RELEASED BY THE CLIENT'S OWN LINES. source/fg/fg_xport.h is the mod's
// code for the 'g' exchange, the imports and the release, written over a binding; this program is its second binding. So
// every set of the runs above is made by FgxAcquire and given back by FgxRelease or FgxDrop, as in the game, and:
//     --fake-fg=5          LATE: the helper sleeps before its ack (--fgslow_g=3000) and the client's deadline is 1 s
//     --fake-fg=6          A BROKEN PIPE: the helper ends on 'g' (--fgdie_g=1), its ack unwritten
// STAGE 10 (the waits across the process boundary, BEFORE a wait is wired in the game): 'Q' and 'G' are written by the
// client's own lines too (FgxWriteQ, FgxWriteG), and the one GPU wait of the game's on a value of the helper's
// (FgxPrepWait: a batch with no command buffer behind fgPrep) is queued on this program's queue with "the next
// injection's body" behind it, and released by the client's own release (FgxPrepRelease), on REAL fences:
//     --fake-fg=7          THE HELPER HELD IN A PREPARE (alive; its queue waits behind a frame's in-fence this side
//                          withholds): the body stays behind the wait (the control: no release, no body), the watch's
//                          term reads the hold as OURS and does not condemn, and the client's release lets the body run.
//                          First, the watch's term over hand-fed rows (a pure function), and since 2026-10-06 the term of
//                          the gate before DXVK's Present (IvtGateStep) over its own
//     --fake-fg=8          THE HELPER SUSPENDED: the watch's term, on real readings and the real clock, condemns it in 4 s;
//                          then the watch's own order, the helper ENDED FIRST and the fence signalled after, and the body runs
//     --fake-fg=9          THE HELPER KILLED with a 'Q' unanswered. MEASURED HERE: the kernel leaves a dead process's
//                          fences at the top, so a wait queued behind fgPrep holds nothing and the client's release
//                          finds nothing to signal. (What needs the release is a helper that is alive: mode 7.)
// PHASE 3, THE WIRE'S ONE REVISION (2026-10-06; docs\reports\fg-3\wire-2026-10-06\specification.md, part 2), on both helpers:
//     --fake-fg=11         LEVER 2 (2026-10-10): 'Q' written right behind 'F' with the helper held ahead of that prepare
//                          (--fghold_q=2): the frame's own answer does not wait for it; and a pair in the old order
//     --fake-fg=10         what wire 2 added, through the client's own lines: a set asked WITHOUT the HUD-less image (two
//                          imports, the ack's third image all zero, a pair generates from PRESENT alone, twenty cycles);
//                          a reset's reason on 'G' ALONE, and on 'Q' and 'G' both (the helper must count each pair's
//                          reason once: run.py reads its count); a 'Q' that says PIPELINED; the jitter on the wire;
//                          and a set WITH the image again, by clearing the bit
// SINCE THAT REVISION EVERY SESSION ABOVE SPEAKS WIRE 2 AS THE PRODUCT DOES: 'Q' is 81 bytes, and a pair's 'G' carries the
// reasons its 'Q' carried (SendG below).
// THE FREEZE CASE (2026-10-06, after the owner's legs p2h and p2z froze the game for good): none of the modes above
// PRESENTS while the wait is queued, and all of them submit on a Vulkan device of this program's own.
//     --fake-fg=30         a real D3D9 device through DXVK, the helper's fences imported into DXVK's device, the wait queued
//                          on DXVK's own queue under its lock, and a thread that keeps presenting while the watch's order
//                          runs. It measures whether Present still returns afterwards. Its own file, fgpipe_freeze.h,
//                          says what it is and lists its options (--fake-pres-...)
// THE COST BENCH (2026-10-06, phase 3 of the revival, its first stage): what one generated frame costs, by its parts.
//     --fake-fg=40         the product's two real copies out of an off-screen back buffer, timed by Vulkan timestamps, a
//                          moving picture, and one arm for each part of a pair, with every part alone on the card and
//                          with the card as the limit. NO WINDOW. Its own file, fgpipe_cost.h, says what it is and lists
//                          its options (--fake-cost-...); tools\test-fg-cost\run.py runs it and reads its numbers
// THE DESK MEASUREMENTS (2026-10-06, phase 3, the build map's stage 1): what AMD's library WRITES, read before anything shows it.
//     --fake-fg=50         one item of eight a session (--fake-desk-item=N): the reset pair and the pairs after it, the
//                          guides smaller than the display, HDR constants by bits, the HUD's rectangle, the held answer,
//                          two depth planes, the helper killed, K. The whole generated image is read every time. NO WINDOW.
//                          Its own file, fgpipe_desk.h, says what each item sends; tools\test-fg-desk\run.py judges
// THE HELPER'S TWO FIXES OWED BEFORE A GENERATED FRAME IS SHOWN (2026-10-10, phase 3; helper-wire-18 and helper-wire-19):
//     --fake-fg=12         an answer the CPU signals never says "a frame": every kind of pair with its answer made by the
//                          CPU, an older answer still queued, and the device removed for real (--fake-fix=1, 2, 3)
//     --fake-fg=13         frame generation's ring never blocks the serve thread: the real hold on a ring of three with the
//                          serve thread's own time, and every slot held with the queue going on (--fake-fix=1, 2)
//                          Their own file, fgpipe_fix.h, says what each case sends and which lever of the helper it needs
//     --fake-fg=14         THE TRUE FILL'S WIRE 3 (2026-10-18): a set of four images, the answer at each count of generated
//                          frames, flow-only, the stock arm's fallback, a 'g' of wire 2 on a helper of wire 3; and where the
//                          helper offers no 2048 (or --fake-wire=2), wire 2 spoken. Its own file, fgpipe_wire3.h, says each
//     --fake-fg=15         THE SECOND INPUT SET (2026-10-20, round 5, H): a set with PRESENT2 and HUDLESS2 beside a helper that offers
//                          4096, pairs by turns, each set's read told apart by pixels with its control, the sets never mixing, a set
//                          without the second. Its own file, fgpipe_wire4.h, says each
//     --fake-fg=16         UNDER WINE AND PROTON (2026-10-24, round 7, L): with --fake-wine=N this program's binding says Wine: the
//                          game's own three fences and their probe, a silent fence planted each way, the bridge. Its own file,
//                          fgpipe_wine.h, says each case
//     --fake-wire=2        every set of the run asked as a client of wire 2 asks it (by default a set is asked at wire 3
//                          wherever the helper offers 2048, as the game asks it)
//
//     fgpipe.exe <helper.exe> <helper log path> [options of this program and extra helper arguments...]
// Every line it prints starts "  PASS  " or "  FAIL  "; the last line is PASS or FAIL. tools\test-fg-pipe\run.py builds it
// 32-bit, runs it against a copy of the helper under %TEMP%, and reads the helper's own log beside it.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cmath>
#pragma warning(push)
#pragma warning(disable: 4505)      // the game's headers: the functions this program does not call
#include "feed_ipc.h"
#include "feed_fg.h"
#include "feed_vk.h"
#include "fg_xport.h"               // THE CLIENT'S OWN LINES for a set of frame generation's (source/fg), over the binding below
#define IVT_GATE_CORE_ONLY          // THE GATE BEFORE DXVK'S PRESENT (source/transport/present_gate.h: the mod's one gate since the fold of
#include "present_gate.h"           // 2026-10-10): its state, its term and its core alone, for the hand-fed rows below and the freeze case
                                    // (fgpipe_freeze.h). Until the fold these were fg_xport.h's FgxGate, FgxGateStep and FgxPresentGate
#pragma warning(pop)

static int gFails = 0, gPasses = 0;
static void Say(const char* fmt, ...)
{
    char b[2048]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    fputs(b, stdout); fputs("\n", stdout); fflush(stdout);
}
static bool Proof(bool ok, const char* fmt, ...)
{
    char b[1536]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
    Say("  %s  %s", ok ? "PASS" : "FAIL", b); if (ok) gPasses++; else gFails++;
    return ok;
}
#define NEED(x, what) do { if (!(x)) { Say("  FAIL  %s (%s:%d): the instrument could not go on", what, __FILE__, __LINE__); Say("FAIL"); ExitProcess(3); } } while (0)

// ---------------------------------------------------------------- Vulkan, loaded by hand (feed_vk.h sets VK_NO_PROTOTYPES)
static PFN_vkGetInstanceProcAddr vkGIPA;
#define VKF(n) static PFN_##n n##_
VKF(vkCreateInstance); VKF(vkEnumeratePhysicalDevices); VKF(vkGetPhysicalDeviceProperties2); VKF(vkGetPhysicalDeviceQueueFamilyProperties);
VKF(vkGetPhysicalDeviceMemoryProperties); VKF(vkCreateDevice); VKF(vkGetDeviceProcAddr); VKF(vkGetDeviceQueue); VKF(vkCreateCommandPool);
VKF(vkAllocateCommandBuffers); VKF(vkBeginCommandBuffer); VKF(vkEndCommandBuffer); VKF(vkResetCommandBuffer); VKF(vkQueueSubmit);
VKF(vkQueueWaitIdle); VKF(vkCreateBuffer); VKF(vkGetBufferMemoryRequirements); VKF(vkBindBufferMemory); VKF(vkMapMemory);
VKF(vkCmdCopyImageToBuffer); VKF(vkCmdClearColorImage); VKF(vkSignalSemaphore);
static VkInstance gInst; static VkPhysicalDevice gPhys; static VkDevice gDev; static VkQueue gQ; static uint32_t gFam = 0;
static FeedVk gVk; static VkCommandPool gPool; static VkCommandBuffer gCb; static VkPhysicalDeviceMemoryProperties gMem;
static unsigned char gLuid[8];

static void Begin()
{
    vkResetCommandBuffer_(gCb, 0);
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer_(gCb, &bi);
}
// Submit the one command buffer, signalling `sig` to `value` in that submit when a semaphore is given (as the game signals
// its in-fence in the submit of its copies), and wait for the queue: this program has one buffer and takes its time.
static bool Submit(VkSemaphore sig, uint64_t value)
{
    vkEndCommandBuffer_(gCb);
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.signalSemaphoreValueCount = 1; tl.pSignalSemaphoreValues = &value;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.commandBufferCount = 1; si.pCommandBuffers = &gCb;
    if (sig != VK_NULL_HANDLE) { si.pNext = &tl; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &sig; }
    if (vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) return false;
    return vkQueueWaitIdle_(gQ) == VK_SUCCESS;
}
static uint32_t MemType(uint32_t bits, VkMemoryPropertyFlags want)
{
    for (uint32_t i = 0; i < gMem.memoryTypeCount; i++) if ((bits & (1u << i)) && (gMem.memoryTypes[i].propertyFlags & want) == want) return i;
    NEED(false, "no host-visible memory type"); return 0;
}

// ---------------------------------------------------------------- the 32-bit address space
static void FreeSpace(SIZE_T* largest, SIZE_T* total)
{
    *largest = 0; *total = 0;
    MEMORY_BASIC_INFORMATION mi; SYSTEM_INFO si; GetSystemInfo(&si);
    for (const char* p = (const char*)si.lpMinimumApplicationAddress; p < (const char*)si.lpMaximumApplicationAddress; )
    {
        if (!VirtualQuery(p, &mi, sizeof(mi))) break;
        if (mi.State == MEM_FREE) { *total += mi.RegionSize; if (mi.RegionSize > *largest) *largest = mi.RegionSize; }
        p = (const char*)mi.BaseAddress + mi.RegionSize;
    }
}

// ---------------------------------------------------------------- the pipe
static HANDLE gPipe = INVALID_HANDLE_VALUE;
static bool PipeWrite(const void* p, DWORD n) { DWORD w = 0; return WriteFile(gPipe, p, n, &w, nullptr) && w == n; }
static bool PipeRead(void* p, DWORD n, DWORD ms)
{
    // the pipe is a byte stream opened without overlapped I/O: poll what has arrived, so a helper that never answers is a
    // FAIL line and not a hung test
    const ULONGLONG t0 = GetTickCount64(); DWORD got = 0;
    while (got < n)
    {
        DWORD avail = 0;
        if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr)) return false;
        if (avail) { DWORD r = 0; if (!ReadFile(gPipe, (char*)p + got, (n - got) < avail ? (n - got) : avail, &r, nullptr)) return false; got += r; continue; }
        if (GetTickCount64() - t0 > ms) return false;
        Sleep(1);
    }
    return true;
}

static const UINT W = 3840, H = 2160;      // the product's largest size: the display's and, in these runs, the guides' too

// ---------------------------------------------------------------- pixels
static uint16_t Half(float f)
{
    uint32_t u; memcpy(&u, &f, 4);
    const uint32_t sign = (u >> 16) & 0x8000u; const int32_t exp = (int32_t)((u >> 23) & 0xFF) - 127 + 15; const uint32_t man = (u >> 13) & 0x3FFu;
    if (exp <= 0) return (uint16_t)sign;
    if (exp >= 31) return (uint16_t)(sign | 0x7C00u);
    return (uint16_t)(sign | ((uint32_t)exp << 10) | man);
}
struct Img { VkImage img = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; UINT w = 0, h = 0; };
static VkBuffer gRb; static VkDeviceMemory gRbMem; static void* gRbPtr;
static void Clear(const Img& i, float r, float g, float b, float a)
{
    VkClearColorValue c; c.float32[0] = r; c.float32[1] = g; c.float32[2] = b; c.float32[3] = a;
    VkImageSubresourceRange rg = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdClearColorImage_(gCb, i.img, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &rg);
}
// How many of the 64x64 FP16 pixels of an image round (cx, cy) are exactly (r, g, b). 4096 is all of them.
// THE TRANSPORT BUILD COPIES THE LEFT HALF OF Color TO Output (the helper's own line says so), so a block in the left half
// shows the copy and a block in the right half must keep whatever Output held before: LEFT and RIGHT below.
static int CountAt(const Img& i, UINT cx, UINT cy, float r, float g, float b, uint16_t* first)
{
    Begin();
    FeedVkBarrierN(&gVk, gCb, &i.img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg = {}; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    rg.imageOffset = { (int32_t)(cx - 32), (int32_t)(cy - 32), 0 }; rg.imageExtent = { 64, 64, 1 };
    vkCmdCopyImageToBuffer_(gCb, i.img, VK_IMAGE_LAYOUT_GENERAL, gRb, 1, &rg);
    if (!Submit(VK_NULL_HANDLE, 0)) return -1;
    const uint16_t* px = (const uint16_t*)gRbPtr; int n = 0;
    const uint16_t hr = Half(r), hg = Half(g), hb = Half(b);
    for (int k = 0; k < 64 * 64; k++) if (px[4 * k] == hr && px[4 * k + 1] == hg && px[4 * k + 2] == hb) n++;
    if (first) memcpy(first, px, 4 * sizeof(uint16_t));
    return n;
}

// THE SHIPPED HELPER HAS NO TRANSPORT COPY (2026-10-31, strip item 11): --fake-real-upscale, which run.py --rel passes, asks a
// real build. Every frame is sent with reset, so no history is in it, and Output is read within a tolerance on the whole block:
// the upscaler writes both halves and does not keep a constant to the bit. It still tells the frames apart: the constants
// differ by 0.25 or more, so the control's read of frame 1 and the read of frame 2 cannot both pass on one picture.
static bool gRealUpscale = false;
static float HalfToFloat(uint16_t h)
{
    const int e = (h >> 10) & 31, m = h & 1023;
    const float v = e == 0 ? (float)m / 16777216.0f : (float)ldexp(1.0 + m / 1024.0, e - 15);
    return (h & 0x8000) ? -v : v;
}
static int OutputAt(const Img& i, UINT cx, UINT cy, float r, float g, float b, uint16_t* first)
{
    if (!gRealUpscale) return CountAt(i, cx, cy, r, g, b, first);
    if (CountAt(i, cx, cy, r, g, b, first) < 0) return -1;   // (its copy leaves the block in the read-back buffer)
    const uint16_t* px = (const uint16_t*)gRbPtr; int n = 0;
    for (int k = 0; k < 64 * 64; k++)
        if (fabsf(HalfToFloat(px[4 * k]) - r) < 0.02f && fabsf(HalfToFloat(px[4 * k + 1]) - g) < 0.02f && fabsf(HalfToFloat(px[4 * k + 2]) - b) < 0.02f) n++;
    return n;
}

// ---------------------------------------------------------------- frame generation's wire, as the game speaks it
static const Img* gIm;                              // the upscaler's four images (the transport build's)
static VkSemaphore gSemIn, gSemOut;                 // and its two fences
static bool PipeReadLate(void* p, DWORD n, DWORD ms, bool* late)   // PipeRead, saying which way it failed
{
    const ULONGLONG t0 = GetTickCount64(); DWORD got = 0; *late = false;
    while (got < n)
    {
        DWORD avail = 0;
        if (!PeekNamedPipe(gPipe, nullptr, 0, nullptr, &avail, nullptr)) return false;
        if (avail) { DWORD r = 0; if (!ReadFile(gPipe, (char*)p + got, (n - got) < avail ? (n - got) : avail, &r, nullptr)) return false; got += r; continue; }
        if (GetTickCount64() - t0 > ms) { *late = true; return false; }
        Sleep(1);
    }
    return true;
}
// (Wine, 2026-10-24, round 7, L: --fake-wine=N, fgpipe_wine.h. Declared here for the binding; 0 is Windows, every other mode)
static int gFakeWine;                // the binding says Wine, and the case N runs
static int gWineWarns;               // what the client's lines put in "the warnings file": how many, and the last
static char gWineWarnLast[400];
static HANDLE FakeHelperProcess();   // the helper's process (fgpipe_wine.h, below gHelper)
static FgxWineSt gWineSt;            // the session's state under Wine (fg_linux.h): in the game the client's heap state holds it
// THIS PROGRAM'S BINDING FOR THE CLIENT'S LINES (fg_xport.h says what a binding is)
struct FakeB
{
    // (Wine: the four a binding that may say Wine has, fg_linux.h; WineFences false outside --fake-fg=16, so every other mode is Windows)
    static bool WineFences() { return gFakeWine != 0; }
    static HANDLE Host() { return FakeHelperProcess(); }
    static bool Bridge() { return gFakeWine == 4; }
    static void Warn(const char* s) { gWineWarns++; strcpy_s(gWineWarnLast, s); Say("  the warnings file would say: %s", s); }
    static FgxWineSt* WineState() { return &gWineSt; }
    static bool PipeWrite(const void* p, DWORD n) { return ::PipeWrite(p, n); }
    static bool PipeReadWithin(void* p, DWORD n, DWORD ms, bool* late) { return PipeReadLate(p, n, ms, late); }
    static FeedVk* Vk() { return &gVk; }
    static VkExternalMemoryHandleTypeFlagBits HandleType() { return VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT; }
    static bool Narrow() { return true; }
    static void Line(const char* fmt, ...)
    {
        char b[1024]; va_list a; va_start(a, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, a); va_end(a);
        Say("  the client's lines say: %s", b);
    }
    // stage 10: one submit on this program's queue (one thread submits at a time: no lock), and a signal from the CPU
    static VkResult QueueSubmit(const VkSubmitInfo* si) { return vkQueueSubmit_(gQ, 1, si, VK_NULL_HANDLE); }
    static VkResult Signal(VkSemaphore s, uint64_t v)
    {
        VkSemaphoreSignalInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO }; si.semaphore = s; si.value = v;
        return vkSignalSemaphore_(gDev, &si);
    }
};
static FgxSet gSet;                                 // the game's side of the set, as the mod holds it
static FgxWaits gWaits;                             // and what its release and its watch read of the per-pair half
static HANDLE gHelper;                              // the helper's process, for the runs that suspend or end it
static bool gHelperEndedHere;                       // one of those runs ended it: the tail must not read that as its own exit
static Img SetImg(int k) { Img i; i.img = gSet.img[k]; i.mem = gSet.mem[k]; i.w = gSet.w; i.h = gSet.h; return i; }
static uint64_t gSeenPrep, gSeenOut; static int gFenceReads; static bool gFenceBack;
// Every read of a fence the helper signals goes through here: NO FENCE MAY EVER READ LOWER THAN BEFORE.
static uint64_t Seen(VkSemaphore s, uint64_t* last)
{
    const uint64_t v = FeedVkTimelineValue(&gVk, s); gFenceReads++;
    if (v < *last) gFenceBack = true; else *last = v;
    return v;
}
static bool SignalNow(VkSemaphore s, uint64_t v)    // from the CPU, for the held cases
{
    VkSemaphoreSignalInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO }; si.semaphore = s; si.value = v;
    return vkSignalSemaphore_(gDev, &si) == VK_SUCCESS;
}
// 'F': a constant into Color, the in-fence signalled in that submit unless it is withheld, then the frame message.
static bool SendF(uint64_t n, float r, float g, float b, bool signalIn)
{
    Begin(); Clear(gIm[FEED_COLOR], r, g, b, 1.0f);
    if (!Submit(signalIn ? gSemIn : VK_NULL_HANDLE, n)) return false;
    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.reset = (n == 1 || gRealUpscale) ? 1u : 0u; f.fm.frame_time_ms = 16.67f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
    return PipeWrite(&f, sizeof(f));
}
// 'Q' AND 'G' THROUGH THE CLIENT'S OWN LINES (stage 10): the messages the game will write are built by fg_xport.h here too,
// so what the helper counts and answers in every run below is what those lines put on the wire.
// WIRE 2 (2026-10-06): 'Q' carries the reasons known at its write, PIPELINED, and the frame's jitter; a pair's 'G' carries
// THE REASONS ITS 'Q' CARRIED, shifted, as the product's machine writes it (fg_machine.h, ShadowHudless), unless it is
// of kind CLOSED. SendQ and SendG are that pair of the product's; SendQx and SendGx write exactly what they are given.
static uint64_t gLastQN; static uint32_t gLastQWhy;
static bool SendQx(uint64_t n, uint32_t flags, float jx, float jy)
{
    gLastQN = n; gLastQWhy = flags & FEED_FG_Q_RESET_MASK;
    return FgxWriteQ<FakeB>(&gWaits, n, flags, 16.67f, 0.05f, 0.7853981f, jx, jy, nullptr);
}
static bool SendGx(uint64_t n, uint32_t kind, uint32_t why) { return FgxWriteG<FakeB>(n, (kind & FEED_FG_G_KIND_MASK) | ((why & FEED_FG_Q_RESET_MASK) << FEED_FG_G_RESET_SHIFT), 800.0f); }
static bool SendQ(uint64_t n, uint32_t flags) { return SendQx(n, flags, 0.0f, 0.0f); }
// 'Q' WITH A CAMERA (the stills mode's): the client's own line again; its camera pointer is what sets FEED_FG_Q_CAMERA
static bool SendQc(uint64_t n, uint32_t flags, const float* cam)
{
    gLastQN = n; gLastQWhy = flags & FEED_FG_Q_RESET_MASK;
    return FgxWriteQ<FakeB>(&gWaits, n, flags, 16.67f, 0.05f, 0.7853981f, 0.0f, 0.0f, cam);
}
static bool SendG(uint64_t n, uint32_t kind)  { return SendGx(n, kind, (kind != FEED_FG_G_CLOSED && n == gLastQN) ? gLastQWhy : 0u); }
// The game's copies for pair n, as one submit: "the back buffer" into PRESENT and, WHERE THE SET HAS THE IMAGE, the HUD-less
// picture into HUDLESS (the same constant: with no motion and equal pictures the generated one must be that constant),
// fgIn = n in the submit unless it is withheld.
static bool Copies(float r, float g, float b, bool signalIn, uint64_t n)
{
    Begin(); Clear(SetImg(FEED_FG_PRESENT), r, g, b, 1.0f);
    if (gSet.img[FEED_FG_HUDLESS] != VK_NULL_HANDLE) Clear(SetImg(FEED_FG_HUDLESS), r, g, b, 1.0f);
    return Submit(signalIn ? gSet.semIn : VK_NULL_HANDLE, n);
}
// WIRE 3 (2026-10-18; feed_ipc.h, FEED_CAP_FG_WIRE3): FEED_FG_BUILD_WIRE3 where the helper offered 2048 and this run does not
// speak as a client of wire 2 (--fake-wire=2): EVERY SET OF EVERY MODE is asked at wire 3 then, as the game asks them, so
// every proof of the modes before wire 3 is also a proof that a set of wire 3 whose pairs ask one frame is wire 2's.
static uint32_t gWireFlags;
// A 'g' by hand, for the one case the client's lines never send: a request with a set still held on this side. Its ack in
// the layout the flags speak (wire 3's 124 bytes, or wire 2's 104), read into the one struct as the client's lines do.
static bool RawExchange(uint32_t w, uint32_t h, uint32_t fmt, uint32_t flags, FeedFgAckIvt* a)
{
    FeedFgBuildIvt b = {}; b.tag = 'g'; b.disp_w = w; b.disp_h = h; b.present_fmt = fmt; b.render_w = W; b.render_h = H; b.shape = 0; b.flags = flags;
    memset(a, 0, sizeof(*a));
    if (flags & FEED_FG_BUILD_WIRE3) return PipeWrite(&b, sizeof(b)) && PipeRead(a, sizeof(*a), FEED_FG_BUILD_DEADLINE_MS);
    FeedFgAck2Ivt a2 = {};
    if (!PipeWrite(&b, sizeof(b)) || !PipeRead(&a2, sizeof(a2), FEED_FG_BUILD_DEADLINE_MS)) return false;
    a->status = a2.status; a->ffx_rc = a2.ffx_rc; for (int k = 0; k < 3; k++) a->img[k] = a2.img[k];
    a->fence_in = a2.fence_in; a->fence_prep = a2.fence_prep; a->fence_out = a2.fence_out; a->ctx_bytes = a2.ctx_bytes; a->provider = a2.provider;
    return true;
}
// A SET THROUGH THE CLIENT'S OWN LINES: FgxAcquire ('g', the ack, the imports; it refuses an ack that names other fences
// than the first), then the images' first layout, which in the game is the first copy's to make. (Wire 3: the layout over
// the images the set holds, FgxSet::all, as the product's first layout takes them.)
static int Acquire(uint32_t fmt, DWORD deadlineMs, uint32_t buildFlags = 0)
{
    const int x = FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)fmt, W, H, 0, deadlineMs, buildFlags | gWireFlags);
    if (x != FGX_CREATED) return x;
    Begin();
    FeedVkBarrierN(&gVk, gCb, gSet.all, gSet.nAll, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    return Submit(VK_NULL_HANDLE, 0) ? FGX_CREATED : -1;
}
// A WHOLE PAIR, nothing withheld: 'F'(n), 'Q'(n), fgPrep = n, the two copies with fgIn = n (for the kinds whose inputs the
// game makes), 'G'(n). Returns what fgOut reads once it is at or past 2n (0 when it never got there in 5 s).
static uint64_t Pair(uint64_t n, uint32_t qflags, uint32_t kind, float r, float g, float b, bool sendF, bool* prepOk)
{
    bool ok = (!sendF || SendF(n, r, g, b, true)) && SendQ(n, qflags);
    *prepOk = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) == n;
    if (kind == FEED_FG_G_DISPATCH || kind == FEED_FG_G_WITHHELD) ok = ok && Copies(r, g, b, true, n);
    ok = ok && SendG(n, kind);
    if (!ok || !FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000)) return 0;
    return Seen(gSet.semOut, &gSeenOut);
}

// ---------------------------------------------------------------- stage 10: the watch's term, and the queued wait
// THE WATCH'S TERM OVER HAND-FED ROWS (fg_xport.h, FgxWatchStep: a pure function of four readings and its own state).
static void WatchRows()
{
    const uint64_t S = 10000000ull;                 // one second, in the clock's 100 ns units
    FgxWatchSt st = {};
    const bool caughtUp = FgxWatchStep(&st, 10, 0, 10, 10, 0) || FgxWatchStep(&st, 10, 0, 10, 10, 100 * S);
    st = {};
    const bool noQ = FgxWatchStep(&st, 0, 0, 0, 5, 0) || FgxWatchStep(&st, 0, 0, 0, 5, 100 * S);
    st = {}; bool ours = false;
    for (uint64_t i = 0; i <= 20; i++) ours = FgxWatchStep(&st, 10, 0, 9, 9, i * S) || ours;
    st = {};
    const bool s0 = FgxWatchStep(&st, 10, 0, 9, 10, 0), s39 = FgxWatchStep(&st, 10, 0, 9, 10, 39 * S / 10), s40 = FgxWatchStep(&st, 10, 0, 9, 10, 4 * S);
    st = {}; bool moving = false;
    for (uint64_t i = 0; i <= 20; i++) moving = FgxWatchStep(&st, 100, 0, 50 + i, 100, i * 3 * S) || moving;
    st = {};
    const bool blind0 = FgxWatchStep(&st, 10, 0, 9, 0, 0), blind4 = FgxWatchStep(&st, 10, 0, 9, 0, 4 * S);
    st = {};
    FgxWatchStep(&st, 10, 0, 9, 10, 0); FgxWatchStep(&st, 10, 0, 9, 10, 3 * S);
    const bool again1 = FgxWatchStep(&st, 11, 0, 9, 10, 3 * S + 1), again2 = FgxWatchStep(&st, 11, 0, 9, 11, 6 * S), again3 = FgxWatchStep(&st, 11, 0, 9, 11, 10 * S);
    Proof(!caughtUp && !noQ, "THE WATCH'S TERM, hand-fed: a helper that has prepared every 'Q' written, and a session with no 'Q' yet, are never condemned (100 s each)");
    Proof(!ours, "a prepare that waits for OUR OWN frame's input (the in-fence below the last 'Q') is never condemned, over 20 s of looks: what is late is ours");
    Proof(!s0 && !s39 && s40, "a prepare that stands still with its frame's input retired here: not at the first look, not at 3.9 s, CONDEMNED at 4.0 s");
    Proof(!moving, "a prepare that is behind but MOVES between looks is never condemned (21 looks, 3 s apart)");
    Proof(!blind0 && blind4, "an in-fence that cannot be read (0) does not blind it: the look goes on without it and condemns at 4 s");
    Proof(!again1 && !again2 && again3, "its clock starts again when the hold becomes ours and then stops being ours (3 s still, a new 'Q' whose input has not retired, then 4 s still: condemned only then)");
    // THE GAME'S OWN STATE (the review of phase 2): the wait for pair 10 is queued, and in the same frame the hand writes
    // 'Q'(11), whose frame's input sits BEHIND the wait and cannot retire. Judged on the wait's own value the helper is
    // condemned; judged on the last 'Q' alone, as the term was, it never is.
    st = {};
    const bool g0 = FgxWatchStep(&st, 11, 10, 9, 10, 0), g39 = FgxWatchStep(&st, 11, 10, 9, 10, 39 * S / 10), g40 = FgxWatchStep(&st, 11, 10, 9, 10, 4 * S);
    st = {}; bool lastOnly = false;
    for (uint64_t i = 0; i <= 20; i++) lastOnly = FgxWatchStep(&st, 11, 0, 9, 10, i * S) || lastOnly;
    st = {}; bool oursLow = false;
    for (uint64_t i = 0; i <= 20; i++) oursLow = FgxWatchStep(&st, 11, 10, 9, 9, i * S) || oursLow;
    st = {}; bool answered = false;
    for (uint64_t i = 0; i <= 20; i++) answered = FgxWatchStep(&st, 12, 10, 10, 10, i * S) || answered;
    Proof(!g0 && !g39 && g40, "AS THE GAME LEAVES IT: a wait queued for pair 10 and 'Q'(11) written behind it (fgPrep 9, the in-fence 10): judged on the wait's own value, not at the first look, not at 3.9 s, CONDEMNED at 4.0 s");
    Proof(!lastOnly, "THE CONTROL: the same readings judged on the last 'Q' alone (no wait's value given) are read as OURS over 20 s of looks and never condemned: the term as it was could not see a prepare that hangs");
    Proof(!oursLow && !answered, "and it still spares us: the wait's own frame not retired here (the in-fence 9) is OURS, and once the wait's pair is answered (fgPrep 10) the term is back on the last 'Q', whose input has not retired");
}
// THE GATE'S TERM OVER HAND-FED ROWS (present_gate.h, IvtGateStep: a pure function of the two needs, the two readings and
// whether each reading succeeded; the gate before DXVK's Present, 2026-10-06). 0 pass, 1 hold on the out-fence, 2 hold on
// the prepare fence, 3 unread. SINCE THE REVIEW OF THE GATE A COUNTER OF 0 IS A VALUE: the helper's fences are created at
// 0, and the first form of the term took 0 for "could not be read" and was blind until the helper's first answer. The
// loop round the term runs on real fences in the freeze case (fgpipe_freeze.h, --fake-pres-gate=2), a helper stopped
// before its first answer among its runs.
static void GateRows()
{
    const uint64_t TOP = UINT64_MAX;
    const bool R = true, X = false;                 // a counter that was READ, and one whose read FAILED
    Proof(IvtGateStep(0, 0, 0, X, 0, X) == 0 && IvtGateStep(0, 0, 7, R, 9, R) == 0 && IvtGateStep(0, 0, 0, R, 0, R) == 0,
          "THE GATE'S TERM, hand-fed: with no need it passes, whatever the two fences read and whether they were read at all (not read; 7 and 9; 0 and 0)");
    Proof(IvtGateStep(10, 8, 10, R, 8, R) == 0 && IvtGateStep(10, 8, 11, R, 9, R) == 0 && IvtGateStep(10, 8, TOP, R, TOP, R) == 0,
          "both needs reached: it passes, at the values exactly (10 and 8), past them, and with both fences at the top (a helper that is gone)");
    Proof(IvtGateStep(10, 0, 9, R, 0, X) == 1 && IvtGateStep(10, 8, 9, R, 8, R) == 1 && IvtGateStep(10, 8, 9, R, 100, R) == 1,
          "the out-fence short of its need (9 of 10) HOLDS ON THE OUT-FENCE: with no need on the prepare fence, with that one reached, and with it far past");
    Proof(IvtGateStep(0, 8, 0, X, 7, R) == 2 && IvtGateStep(10, 8, 10, R, 7, R) == 2 && IvtGateStep(10, 8, 100, R, 1, R) == 2,
          "the prepare fence short of its need (7 of 8) HOLDS ON THE PREPARE FENCE: with no need on the out-fence, with that one reached, and with it far past");
    Proof(IvtGateStep(10, 8, 9, R, 7, R) == 1 && IvtGateStep(10, 8, 1, R, 1, R) == 1 && IvtGateStep(10, 8, 0, R, 0, R) == 1, "both short: THE OUT-FENCE FIRST (at 9 and 7, at 1 and 1, at 0 and 0)");
    Proof(IvtGateStep(5, 0, 0, R, 0, X) == 1 && IvtGateStep(1, 0, 0, R, 0, X) == 1 && IvtGateStep(0, 5, 0, X, 0, R) == 2 && IvtGateStep(6, 5, 5, R, 0, R) == 1 && IvtGateStep(6, 5, 6, R, 0, R) == 2,
          "A COUNTER READ AS 0 IS A VALUE AND HOLDS (a fence nothing has signalled yet: the helper has not answered once): need 5 and need 1 on the out-fence, need 5 on the prepare fence, and the prepare fence at 0 once the out-fence is reached");
    Proof(IvtGateStep(10, 0, 0, X, 0, X) == 3 && IvtGateStep(0, 8, 0, X, 0, X) == 3 && IvtGateStep(10, 8, 0, X, 0, X) == 3 && IvtGateStep(10, 8, 10, R, 0, X) == 3 && IvtGateStep(10, 8, 0, X, 8, R) == 3
          && IvtGateStep(10, 8, 3, X, 2, X) == 3,
          "a need whose read FAILED is UNREAD and is never held on: the out-fence's, the prepare fence's, both, either one beside a fence that is reached, and whatever the failed read left in the value");
    Proof(IvtGateStep(0, 8, 5, R, 8, R) == 0 && IvtGateStep(0, 8, 0, X, 8, R) == 0 && IvtGateStep(10, 0, 10, R, 3, R) == 0 && IvtGateStep(10, 0, 10, R, 0, X) == 0,
          "A NEED OF 0 IS NEVER HELD ON and never unread: its fence may read anything, or not be read at all, and the other fence alone decides");
    Proof(IvtGateStep(10, 8, 9, R, 0, X) == 1 && IvtGateStep(10, 8, 0, X, 7, R) == 2 && IvtGateStep(10, 8, 0, R, 0, X) == 1 && IvtGateStep(10, 8, 0, X, 0, R) == 2,
          "EACH FENCE IS JUDGED BY ITSELF: a fence whose read failed does not blind the gate to the other, which was read and is short, at 9, at 7 or at 0 (it holds on the one it read)");
}
// "THE NEXT INJECTION'S BODY", as far as the queue sees it: a submit of its own behind whatever is queued, which signals
// this program's own timeline when it runs. No command buffer (the one this program has may not be reset while a submit
// of it could be pending) and no wait for the queue: it may be behind the wait on fgPrep.
static VkSemaphore gBodySem;
static bool BodySubmit(uint64_t value)
{
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.signalSemaphoreValueCount = 1; tl.pSignalSemaphoreValues = &value;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.pNext = &tl; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &gBodySem;
    return vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) == VK_SUCCESS;
}
// THE WAIT AND THE BODY, ON A THREAD OF THEIR OWN: with the helper frozen, whether a submit that waits on a fence shared with
// it even RETURNS is not this program's to assume (the transport measured a CPU signal that did not). The top is stored
// before the submit, so the release works either way. Nothing else submits to the queue while this thread may.
static volatile LONG gWtQueued, gWtBody;
static DWORD WINAPI WaitThread(void*)
{
    if (FgxPrepWait<FakeB>(&gSet, &gWaits, 34)) _InterlockedExchange(&gWtQueued, 1);
    if (BodySubmit(1)) _InterlockedExchange(&gWtBody, 1);
    return 0;
}
// The client's release on a thread of its own, for the one place where its signal may not return: a helper that is frozen.
static volatile LONG gRtDid, gRtRes;
static DWORD WINAPI ReleaseThread(void*)
{
    bool did = false;
    const VkResult r = FgxPrepRelease<FakeB>(&gSet, &gWaits, &did);
    _InterlockedExchange(&gRtRes, (LONG)r); _InterlockedExchange(&gRtDid, did ? 1 : 0);
    return 0;
}
static bool SuspendHelper()
{
    typedef LONG (NTAPI* NtSuspendProcess_t)(HANDLE);
    const NtSuspendProcess_t f = (NtSuspendProcess_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess");
    return f && gHelper && f(gHelper) >= 0;
}
static uint64_t Now100() { ULONGLONG t = 0; QueryUnbiasedInterruptTime(&t); return t; }
// THE RUNS OF STAGE 10 (modes 7, 8, 9). Each ends the session: a transport whose queued wait had to be released has stopped.
static void WaitRun(int mode)
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    bool prep = false, did = false;
    if (mode == 7) { WatchRows(); GateRows(); }
    const int x = Acquire(FP16, FEED_FG_BUILD_DEADLINE_MS);
    const uint64_t o33 = x == FGX_CREATED ? Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep) : 0;
    HANDLE none = nullptr;
    const bool sem = FeedVkCreateTimeline(&gVk, false, &gBodySem, &none) >= 0 && gBodySem != VK_NULL_HANDLE;
    if (none) CloseHandle(none);
    if (!Proof(x == FGX_CREATED && prep && o33 == 66 && sem, "a set through the client's own lines, and one whole pair before the hold (fgOut %llu): its 'Q' and its 'G' written by FgxWriteQ and FgxWriteG", (unsigned long long)o33)) return;
    bool ok;
    if (mode == 7)
    {   // THE HELPER HELD IN A PREPARE: alive, its queue behind frame 34's in-fence, which this side withholds
        ok = SendF(34, A[0], A[1], A[2], false) && SendQ(34, 0);
    }
    else
    {   // THE HELPER FROZEN with pair 34's 'Q' unread: frame 34 was given and answered first, so its input HAS retired here
        ok = SendF(34, A[0], A[1], A[2], true) && FeedVkWaitTimeline(&gVk, gSemOut, 34, 5000);
        ok = ok && SuspendHelper() && SendQ(34, 0);
    }
    const bool early = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 1000);
    if (mode == 9)
    {   // KILLED where it stands: its exit never runs, so nothing will ever signal fgPrep
        gHelperEndedHere = TerminateProcess(gHelper, 9) != 0;
        WaitForSingleObject(gHelper, 2000);
    }
    const HANDLE th = CreateThread(nullptr, 0, WaitThread, nullptr, 0, nullptr);
    const bool returned = th && WaitForSingleObject(th, 1500) == WAIT_OBJECT_0;
    const bool ran = FeedVkWaitTimeline(&gVk, gBodySem, 1, returned ? 1500 : 0);
    const uint64_t heldPrep = FeedVkTimelineValue(&gVk, gSet.semPrep), mine = FeedVkTimelineValue(&gVk, gSemIn);
    Say("  (a fact, not a proof: with the helper %s the wait-only submit and the body's submit %s; the batch was %s)",
        mode == 7 ? "alive and held" : mode == 8 ? "suspended" : "killed", returned ? "both returned at once" : "DID NOT RETURN within 1.5 s", gWtQueued ? "taken by the queue" : "not taken yet");
    if (mode == 9)
        Proof(ok && th && !early && returned && ran && heldPrep == UINT64_MAX && mine == UINT64_MAX && gWaits.prepTop == 34 && gWaits.lastQ == 34,
              "THE HELPER KILLED with pair 34's 'Q' unanswered: the kernel leaves a dead helper's fences AT THE TOP (fgPrep reads 0x%llX, the frame's in-fence 0x%llX), so the wait queued behind fgPrep (top stored: %ld) holds nothing: the body %s with no release at all",
              (unsigned long long)heldPrep, (unsigned long long)mine, (long)gWaits.prepTop, ran ? "ran" : "DID NOT RUN");
    else
        Proof(ok && th && !early && !ran && heldPrep == 33 && mine == (mode == 7 ? 33u : 34u) && gWaits.prepTop == 34 && gWaits.lastQ == 34,
              "THE QUEUED WAIT, the helper %s with pair 34's 'Q' unanswered: fgPrep reads %llu, the frame's in-fence %llu, the top was stored (%ld) before the submit, and THE CONTROL holds: with no release the body behind the wait %s, as it must",
              mode == 7 ? "HELD IN ITS PREPARE" : "SUSPENDED", (unsigned long long)heldPrep, (unsigned long long)mine, (long)gWaits.prepTop, ran ? "RAN" : "did not run");
    bool relJoined = true;
    if (mode == 7)
    {   // the watch's term on these REAL readings and a hand-fed clock: the frame's own input has not retired, so the hold is ours
        FgxWatchSt st = {}; bool fired = false;
        for (uint64_t i = 0; i <= 12; i++) fired = FgxWatchStep(&st, (uint64_t)(ULONG)gWaits.lastQ, (uint64_t)(ULONG)gWaits.waitLow, heldPrep, mine, i * 10000000ull) || fired;
        Proof(!fired, "the watch's term reads this hold as OURS and does not condemn the helper over 12 s of looks (the last 'Q' 34, fgPrep %llu, the frame's in-fence %llu: its input has not retired here)",
              (unsigned long long)heldPrep, (unsigned long long)mine);
    }
    else if (mode == 8)
    {   // THE WATCH'S TERM, on real readings and the real clock, looking twice a second as the watch does
        // AS THE GAME DOES (the review of phase 2): in the frame that queued the wait, the hand writes THE NEXT 'Q'. Its frame's
        // input would sit behind the wait and never retire, so from here the last 'Q' is one ahead of the in-fence for good.
        const bool q35 = SendQ(35, 0);
        FgxWatchSt stc = {}; bool firedOld = false;
        for (uint64_t i = 0; i <= 12; i++) firedOld = FgxWatchStep(&stc, (uint64_t)(ULONG)gWaits.lastQ, 0, heldPrep, mine, i * 10000000ull) || firedOld;
        Proof(q35 && gWaits.lastQ == 35 && gWaits.waitLow == 34 && !firedOld,
              "AS IN THE GAME, the next 'Q' (35) is written behind the queued wait (the last 'Q' %ld, the lowest pair a wait stands for %ld, fgPrep %llu, the in-fence %llu). THE CONTROL: judged on the last 'Q' alone these REAL readings are OURS over 12 s of looks, never condemned",
              (long)gWaits.lastQ, (long)gWaits.waitLow, (unsigned long long)heldPrep, (unsigned long long)mine);
        FgxWatchSt st = {}; bool fired = false; const uint64_t t0 = Now100(); uint64_t at = 0; int looks = 0;
        while (!fired && Now100() - t0 < 80000000ull)
        {
            fired = FgxWatchStep(&st, (uint64_t)(ULONG)gWaits.lastQ, (uint64_t)(ULONG)gWaits.waitLow, FeedVkTimelineValue(&gVk, gSet.semPrep), FeedVkTimelineValue(&gVk, gSemIn), Now100());
            looks++;
            if (fired) at = Now100() - t0; else Sleep(500);
        }
        const bool still = !FeedVkWaitTimeline(&gVk, gBodySem, 1, 0);
        Proof(fired && at >= 39000000ull && at <= 57000000ull && still, "THE WATCH'S TERM condemns the suspended helper: after %.1f s and %d looks (4.0 to 5.5 s wanted), with the body still behind the wait",
              (double)at / 1e7, looks);
        // THE RELEASE TRIED WHILE THE HELPER IS STILL FROZEN, on a thread of its own: the transport measured, for the frame's
        // fence, that such a signal does not return until the helper is gone. A fact for the record, whichever way it goes.
        const HANDLE rt = CreateThread(nullptr, 0, ReleaseThread, nullptr, 0, nullptr);
        const bool relReturned = rt && WaitForSingleObject(rt, 1500) == WAIT_OBJECT_0;
        const bool relFreed = FeedVkWaitTimeline(&gVk, gBodySem, 1, relReturned ? 500 : 0);
        Say("  (a fact, not a proof: with the helper still suspended the release's signal %s within 1.5 s, and the body %s)",
            relReturned ? "RETURNED" : "did not return", relFreed ? "RAN" : "did not run");
        // THE WATCH'S OWN ORDER: the helper ended FIRST, then the signal
        gHelperEndedHere = TerminateProcess(gHelper, 0) != 0;
        WaitForSingleObject(gHelper, 2000);
        relJoined = rt && WaitForSingleObject(rt, 5000) == WAIT_OBJECT_0;
        if (rt) CloseHandle(rt);
    }
    else
    {   // KILLED: the game learns it at its next write, which is what stops its transport and makes the release
        const uint8_t tagF = 'F'; bool broke = false;
        for (int i = 0; i < 200 && !broke; i++) { broke = !PipeWrite(&tagF, 1); if (!broke) Sleep(10); }
        Proof(broke, "the pipe is broken under the game's next write: the stop that makes the release");
    }
    // THE RELEASE, as the game's stop and its watch make it: no wait first
    const VkResult r = FgxPrepRelease<FakeB>(&gSet, &gWaits, &did);
    const bool joined = th && WaitForSingleObject(th, 5000) == WAIT_OBJECT_0;
    const bool freed = joined && gWtQueued && gWtBody && FeedVkWaitTimeline(&gVk, gBodySem, 1, 5000);
    const uint64_t after = FeedVkTimelineValue(&gVk, gSet.semPrep);
    if (th) CloseHandle(th);
    if (mode == 7)
        Proof(did && r == VK_SUCCESS && freed && after == 35, "THE CLIENT'S RELEASE, the helper alive: fgPrep signalled from the CPU to one past the top (it reads %llu), and the body behind the wait ran",
              (unsigned long long)after);
    else
        Proof(gHelperEndedHere && r == VK_SUCCESS && relJoined && freed && after >= 35,
              "%s: the body behind the wait ran, and fgPrep reads 0x%llX (%s)",
              mode == 8 ? "THE WATCH'S ORDER, the helper ended first" : "THE HELPER GONE",
              (unsigned long long)after, (did || gRtDid) ? "signalled by the client's release" : "left at the top by the kernel when the helper died: the client's release had nothing to signal");
    bool did2 = true;
    const VkResult r2 = FgxPrepRelease<FakeB>(&gSet, &gWaits, &did2);
    Proof(!did2 && r2 == VK_SUCCESS, "a second release (the stop and the watch can both come) signals nothing: no value is signalled twice");
    if (mode == 7)
    {   // the frame that was announced and never submitted has its in-fence signalled from the CPU, as the game's stop does,
        // so the helper's queue runs out and it can end by itself
        ok = SignalNow(gSemIn, 34) && FeedVkWaitTimeline(&gVk, gSemOut, 34, 5000);
        Say("  (a fact, not a proof: once frame 34's in-fence is signalled the helper's own queued signal of 34 lands behind the release's 35, and fgPrep then reads %llu)",
            (unsigned long long)FeedVkTimelineValue(&gVk, gSet.semPrep));
        Proof(ok, "the held frame's in-fence signalled from the CPU: the helper's queue runs out (frame 34 is answered)");
    }
}

// =====================================================================================================================
// STAGE 12, THE TWO RESTORED TOOLS' MOVING PICTURE: what was --fake-fg=20 (ONE BLOCK, between this banner and the one that
// ends it). ROUND 8'S DEEP STRIP (2026-10-26) TOOK THE MODE ITSELF OUT with the helper's two tools (host\fg_tools.h) and
// tools\test-fg-tools: what stays here is its moving picture with known vectors and the staging round it, which the cost
// bench (fgpipe_cost.h, --fake-fg=40) and the desk measurements (fgpipe_desk.h, --fake-fg=50) are built on. What the mode
// was, as it stood:
//
// The helper's two development tools (host\fg_tools.h: the vectors' self-test, --fgtool_mv=1, and the lag test,
// --fgtool_lag=1) measure pictures and vectors, and every pair the runs below send is of constants: nothing moves, so
// nothing can be measured. This mode feeds A MOVING PICTURE WITH KNOWN VECTORS through the real helper, pair after pair
// as the game would ('F', 'Q', fgPrep, the two copies with fgIn, 'G', fgOut), and LEAVES THE VERDICT TO THE HELPER'S OWN
// LOG LINES, which tools\test-fg-tools\run.py reads. What this program proves itself is only that every pair was
// answered. Ten phases, each on a set of its own (the tools' numbers are a set's):
//     RIGHT     the picture moves, the vectors say so, PRESENT is this frame's picture     vectors near 0, +x+y lowest;
//                                                                                          lag: THIS frame's
//     NEGATED   the same with every vector negated                                         vectors above 1, -x-y lowest
//     ZERO      the picture moves, the vectors are zero                                    exactly 1, a dead instrument
//     STILL     nothing moves                                                              not scored: a still picture
//     BEHIND    as RIGHT, but PRESENT shows the picture of the frame BEFORE                lag: the PREVIOUS frame's
//     FAR       vectors of 400 pixels                                                      not scored: out of the tile
//     TIMES     the right vectors times the render size, and TIMESNEG, those negated       not scored: out of the tile
//               (for the observation below: what the vectors would be were the helper's scale too small)
//     HELD      one pair's in-fence and one pair's fgIn withheld for 4.5 s                 not scored: the wait timed out
//     QUALITY   as RIGHT at a render size of 2560x1440 under a 3840x2160 display           as RIGHT, the vectors scaled
// Nine of the sets are released by the game's word; the last one stays ALIVE when the pipe closes, so the tools' closing
// lines for it are the ones the helper says at the connection's end.
// AND ONE THING IS OBSERVED AND NOT JUDGED: where AMD's generated image sits, phase by phase (TObserve below says why).
// THE PICTURE is a gradient of three waves, the same value in red, green and blue. It is made ONCE, one period larger
// than an image each way, and an image is filled from it at an offset: so a frame's picture is the one before it moved
// by a WHOLE number of pixels, bit for bit, and the right vectors must leave exactly nothing.
// THE VECTORS are what the game writes into FEED_MV (source/shaders/ivt_guides_ps.hlsl): pixels at the render size,
// PREVIOUS MINUS CURRENT, y down. A picture that moved by (+6, +4) has the vectors (-6, -4) at every pixel.
// THIS FILE STILL INCLUDES NOTHING OF host\: the picture, the vectors and the half floats are this program's own.
// =====================================================================================================================
#include <cmath>
static PFN_vkCmdCopyBufferToImage tCopyBufferToImage;
struct TStage { VkBuffer buf = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; const uint16_t* px = nullptr; UINT w = 0, h = 0, period = 0; };
static TStage tPlain, tWide;                        // the picture at one pixel a pixel, and the same picture 3/2 as large
static Img tIm[FEED_SLOTS];                         // the upscaler's four images after this mode's own rebuild

// The picture for images of w x h, enlarged by num/den: its three waves are 96, 64 and 48 pixels long times that, so it
// repeats every 192 times that, and the buffer is one repeat larger than the image each way.
static void TStageMake(TStage* s, UINT w, UINT h, UINT num, UINT den)
{
    s->period = 192u * num / den; s->w = w + s->period; s->h = h + s->period;
    const VkDeviceSize bytes = (VkDeviceSize)s->w * s->h * 8u;
    VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = bytes; bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    NEED(vkCreateBuffer_(gDev, &bi, nullptr, &s->buf) == VK_SUCCESS, "the picture's buffer");
    VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, s->buf, &q);
    VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size;
    mi.memoryTypeIndex = MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &s->mem) == VK_SUCCESS, "the picture's memory");
    NEED(vkBindBufferMemory_(gDev, s->buf, s->mem, 0) == VK_SUCCESS, "vkBindBufferMemory for the picture");
    void* p = nullptr; NEED(vkMapMemory_(gDev, s->mem, 0, bytes, 0, &p) == VK_SUCCESS, "vkMapMemory for the picture");
    const UINT P = s->period; const double tau = 6.283185307179586, k = (double)num / (double)den;
    uint16_t* one = (uint16_t*)malloc((size_t)P * P * sizeof(uint16_t)); NEED(one, "memory for one repeat of the picture");
    for (UINT v = 0; v < P; v++) for (UINT u = 0; u < P; u++)
        one[(size_t)v * P + u] = Half((float)(0.45 + 0.20 * sin(tau * u / (96.0 * k)) + 0.15 * sin(tau * v / (64.0 * k)) + 0.08 * sin(tau * (u + v) / (48.0 * k))));
    const uint16_t a1 = Half(1.0f); uint16_t* px = (uint16_t*)p;
    for (UINT v = 0; v < s->h; v++)
    {
        const uint16_t* row = one + (size_t)(v % P) * P; uint16_t* o = px + (size_t)v * s->w * 4u;
        for (UINT u = 0; u < s->w; u++) { const uint16_t t = row[u % P]; o[4 * u] = t; o[4 * u + 1] = t; o[4 * u + 2] = t; o[4 * u + 3] = a1; }
    }
    free(one);
    s->px = px;
}
// An image filled from the picture after it moved by (sx, sy) in all: what the image shows at (x, y) is the picture at
// (x - sx, y - sy).
static void TPut(const Img& i, const TStage& s, long sx, long sy)
{
    const long P = (long)s.period;
    const UINT ox = (UINT)(((-sx) % P + P) % P), oy = (UINT)(((-sy) % P + P) % P);
    VkBufferImageCopy rg = {}; rg.bufferOffset = ((VkDeviceSize)oy * s.w + ox) * 8u; rg.bufferRowLength = s.w;
    rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { i.w, i.h, 1 };
    tCopyBufferToImage(gCb, s.buf, i.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg);
}
// WHERE THE GENERATED IMAGE SITS: OBSERVED, NEVER JUDGED HERE. The helper's two tools read what frame generation is HANDED.
// What AMD's library MAKES of it is a third question, and this mode is the first desk run with motion in it, so the
// answer is taken while the pictures are known: after a pair is answered, the centre 64x64 of GEN is read and compared
// with four pictures this program can compute, each as a part of the change between the two presented frames (the same
// ratio the helper's tools print: 0 is that picture exactly, 1 is as far as the two frames are from each other):
//     the HALF-WAY picture (the presented one moved back by half its move)     what an interpolation that follows the motion makes
//     THIS frame's presented picture                                           a copy
//     the PREVIOUS frame's presented picture                                   a copy of the frame before
//     the even MIX of the two                                                  a blend with no motion in it
// One line a phase. Read ACROSS the phases it says what the vectors do to AMD's picture at the scale the helper passes
// (the review of stages 5 to 7 left that open): right against negated against zero, and against the same vectors
// multiplied by the render size (the phases TIMES and TIMESNEG), which is what they would have to be if the helper's
// scale of one over the render size were one render size too small.
static float THalfToFloat(uint16_t v)               // this program's own; the picture's values are plain numbers below 1, or zero
{
    const uint32_t e = (v >> 10) & 0x1Fu, m = v & 0x3FFu;
    if (e == 0 || e == 31) return 0.0f;
    const uint32_t bits = ((uint32_t)(v >> 15) << 31) | ((e + 112u) << 23) | (m << 13);
    float f; memcpy(&f, &bits, 4); return f;
}
static double TAt(const TStage& s, long sx, long sy, UINT x, UINT y)   // the picture at an image's pixel after it moved by (sx, sy) in all
{
    const long P = (long)s.period;
    const UINT ox = (UINT)(((-sx) % P + P) % P), oy = (UINT)(((-sy) % P + P) % P);
    return THalfToFloat(s.px[((size_t)(y + oy) * s.w + (x + ox)) * 4u]);
}
struct TObs { double mid = 0, cur = 0, prev = 0, mix = 0, change = 0; int pairs = 0; };
static void TObserve(TObs* o, const TStage& s, long cS, long cT, long pS, long pT)
{
    const Img gen = SetImg(FEED_FG_GEN);
    const UINT x0 = W / 2 - 32, y0 = H / 2 - 32;
    Begin();
    FeedVkBarrierN(&gVk, gCb, &gen.img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg = {}; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    rg.imageOffset = { (int32_t)x0, (int32_t)y0, 0 }; rg.imageExtent = { 64, 64, 1 };
    vkCmdCopyImageToBuffer_(gCb, gen.img, VK_IMAGE_LAYOUT_GENERAL, gRb, 1, &rg);
    if (!Submit(VK_NULL_HANDLE, 0)) return;
    const uint16_t* px = (const uint16_t*)gRbPtr;
    for (UINT y = 0; y < 64; y++) for (UINT x = 0; x < 64; x++)
    {
        const double g = THalfToFloat(px[4 * (y * 64 + x)]);
        const double c = TAt(s, cS, cT, x0 + x, y0 + y), p = TAt(s, pS, pT, x0 + x, y0 + y), m = TAt(s, (cS + pS) / 2, (cT + pT) / 2, x0 + x, y0 + y);
        o->mid += fabs(g - m); o->cur += fabs(g - c); o->prev += fabs(g - p); o->mix += fabs(g - 0.5 * (c + p)); o->change += fabs(c - p);
    }
    o->pairs++;
}
// The upscaler's set built again at another render size, as a resolution change does it: the game's imports go first,
// then 'B', then the new four are imported. The two fences of the connection stay.
static bool TRebuild(UINT rw, UINT rh)
{
    for (int i = 0; i < FEED_SLOTS; i++)
    {
        if (gIm[i].img) gVk.DestroyImage(gDev, gIm[i].img, nullptr);
        if (gIm[i].mem) gVk.FreeMemory(gDev, gIm[i].mem, nullptr);
    }
    FeedBuild b = {}; b.width = rw; b.height = rh; b.color_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT; b.output_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1; b.transport = gRealUpscale ? 0 : 1; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    FeedBuildAck back = {}; const uint8_t tagB = 'B';
    if (!(PipeWrite(&tagB, 1) && PipeWrite(&b, sizeof(b)) && PipeRead(&back, sizeof(back), 60000) && back.ok == 1)) return false;
    const VkFormat fm[FEED_SLOTS] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT };
    bool ok = true;
    for (int i = 0; i < FEED_SLOTS; i++)
    {
        tIm[i] = Img(); tIm[i].w = rw; tIm[i].h = rh;
        ok = ok && back.tex[i] && FeedVkImportImage(&gVk, (HANDLE)(uintptr_t)back.tex[i], rw, rh, fm[i], i == FEED_OUTPUT, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, true, &tIm[i].img, &tIm[i].mem);
        if (back.tex[i]) CloseHandle((HANDLE)(uintptr_t)back.tex[i]);     // the import duplicated it
    }
    if (!ok) return false;
    Begin();
    { VkImage all[FEED_SLOTS]; for (int i = 0; i < FEED_SLOTS; i++) all[i] = tIm[i].img; FeedVkBarrierN(&gVk, gCb, all, FEED_SLOTS, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); }
    Clear(tIm[FEED_DEPTH], 0.0f, 0.0f, 0.0f, 0.0f);
    if (!Submit(VK_NULL_HANDLE, 0)) return false;
    gIm = tIm;
    return true;
}
// ============================================================================== END OF STAGE 12'S BLOCK (what was --fake-fg=20)

// THE COST BENCH (2026-10-06, phase 3's first stage), --fake-fg=40: the product's two real copies out of an off-screen back
// buffer, mode 20's moving picture, and one arm for each part of a pair, idle and with the card as the limit. All of it is
// in the file below (it stands here because it uses mode 20's picture); FgRun only hands over to it.
#include "fgpipe_cost.h"

// THE DESK MEASUREMENTS (2026-10-06, phase 3, the build map's stage 1), --fake-fg=50: the whole generated image read back
// and measured, one item of the stage's list a session. All of it is in the file below (it stands here because it uses
// mode 20's picture and the cost bench's images); FgRun only hands over to it.
#include "fgpipe_desk.h"
// THE HELPER'S TWO FIXES (2026-10-10, phase 3; helper-wire-18 and -19), --fake-fg=12 and --fake-fg=13: an answer the CPU
// signals never says "a frame", and the ring never blocks the serve thread. All of it is in the file below (it stands
// here because it uses mode 20's picture); FgRun only hands over to it.
#include "fgpipe_fix.h"

// THE TRUE FILL'S WIRE, --fake-fg=14 (2026-10-18, package G2): a set of four images, the answer at each count, flow-only, the
// stock arm's fallback, and the two halves of different wires. All of it is in the file below.
#include "fgpipe_wire3.h"

// THE SECOND INPUT SET, --fake-fg=15 (2026-10-20, round 5, H): a set with PRESENT2 and HUDLESS2, pairs by turns, the read of each set
// told apart by pixels with its control, the sets never mixing, a set without the second. All of it is in the file below.
static unsigned long long gCaps;                    // the hello ack's caps, as this run read them
#include "fgpipe_wire4.h"

// UNDER WINE AND PROTON, --fake-fg=16 --fake-wine=N (2026-10-24, round 7, L): the game's own fences, their probe, and what fails closed.
// All of it is in the file below.
#include "fgpipe_wine.h"

// THE RUNS OF STAGES 6 AND 7 (the head of this file says what each mode is). The run of stage 4 used frames 1 to 32.
// =====================================================================================================================
// PHASE 3, THE WIRE'S ONE REVISION, --fake-fg=10 (2026-10-06; specification.md, part 2). What wire 2 added, each through the
// client's own lines against the BUILT helper. The helper's own count of this run is run.py's to read (W2_COUNTS there):
// 22 sets (this one, twenty cycles, the three-image one), seven pairs 33 to 39, cut 2 (pair 35 says it on 'G' alone,
// pair 36 on both: ONE each), build 2, pipelined 1.
// =====================================================================================================================
static void Wire2Run(bool stamp)
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f }, B[3] = { 0.75f, 0.25f, 0.5f };
    bool prep = false, ok = false; uint16_t first[4] = {};
    Proof(sizeof(FeedFgQIvt) == 81 && sizeof(FeedFgGIvt) == 17 && sizeof(FeedFgBuildIvt) == 29 && sizeof(FeedFgAck2Ivt) == 104,
          "WIRE 2's sizes as this program compiled them: 'Q' %u bytes (81), 'G' %u (17), 'g' %u (29), its ack %u (104; wire 3's is %u)",
          (unsigned)sizeof(FeedFgQIvt), (unsigned)sizeof(FeedFgGIvt), (unsigned)sizeof(FeedFgBuildIvt), (unsigned)sizeof(FeedFgAck2Ivt), (unsigned)sizeof(FeedFgAckIvt));
    // ---- A SET ASKED WITHOUT THE HUD-LESS IMAGE
    int x = Acquire(FP16, DL, FEED_FG_BUILD_NO_HUDLESS);
    const bool have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut && gSet.provider != 0;
    Proof(have && gSet.img[FEED_FG_PRESENT] != VK_NULL_HANDLE && gSet.img[FEED_FG_GEN] != VK_NULL_HANDLE && gSet.fmt[FEED_FG_PRESENT] == FP16 && gSet.fmt[FEED_FG_GEN] == FP16
          && gSet.storage[FEED_FG_GEN] && !gSet.storage[FEED_FG_PRESENT]
          && gSet.img[FEED_FG_HUDLESS] == VK_NULL_HANDLE && gSet.mem[FEED_FG_HUDLESS] == VK_NULL_HANDLE && gSet.nt[FEED_FG_HUDLESS] == nullptr,
          "A SET ASKED WITHOUT THE HUD-LESS IMAGE (FEED_FG_BUILD_NO_HUDLESS) is CREATED through the client's own lines (status %d): TWO images imported, GEN alone with storage usage, and THE THIRD ALL ZERO: no handle came for it and nothing was imported", gSet.status);
    if (!have) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    if (stamp)
    {   // ---- which handle is which image, by pixels: the helper stamps the two it has
        const uint8_t tagS = 'S'; uint8_t done = 0;
        ok = PipeWrite(&tagS, 1) && PipeRead(&done, 1, 10000) && done == 1;
        const int c0 = ok ? CountAt(SetImg(FEED_FG_PRESENT), CX, CY, 0.125f, 0.25f, 0.5f, nullptr) : -1;
        const int c1 = ok ? CountAt(SetImg(FEED_FG_GEN),     CX, CY, 0.5f, 0.125f, 0.25f, nullptr) : -1;
        Proof(c0 == 4096 && c1 == 4096, "THE TWO CONSTANTS of a set with no HUD-less image are where the machine's order says: PRESENT %d, GEN %d of 4096 (the helper's 'S' stamps the images it has)", c0, c1);
    }
    // ---- a pair generates from PRESENT alone: two pairs of one constant give exactly that constant
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 34");
    const uint64_t o34 = Pair(34, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    const int g34 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first);
    Proof(prep && o33 == 66 && o34 == 68 && g34 == 4096, "WITH NO HUD-LESS IMAGE A PAIR GENERATES: pairs 33 and 34 (fgOut %llu, %llu), ONE copy each (PRESENT) with fgIn in its submit, and GEN is that constant, exactly (%d of 4096)",
          (unsigned long long)o33, (unsigned long long)o34, g34);
    // ---- A RESET'S REASON ON 'G' ALONE (pair 35), then on 'Q' AND 'G' as the product writes it (pair 36): a frame each
    ok = SendF(35, B[0], B[1], B[2], true) && SendQx(35, 0, 0.25f, -0.375f) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 35, 5000)
      && Copies(B[0], B[1], B[2], true, 35) && SendGx(35, FEED_FG_G_DISPATCH, FEED_FG_Q_RESET_CUT) && FeedVkWaitTimeline(&gVk, gSet.semOut, 70, 5000);
    const uint64_t o35 = Seen(gSet.semOut, &gSeenOut);
    const uint64_t o36 = Pair(36, FEED_FG_Q_RESET_CUT, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    Proof(ok && o35 == 70 && prep && o36 == 72, "A CAMERA CUT SAID ON 'G' ALONE (pair 35, its 'Q' carried no reason and a jitter of 0.25, -0.375) has its frame (fgOut %llu), and so has one said on 'Q' and 'G' both, as the product writes it (pair 36: %llu); the helper must have counted ONE cut for each (its count is run.py's to read)",
          (unsigned long long)o35, (unsigned long long)o36);
    // ---- A 'Q' THAT SAYS PIPELINED
    ok = SendF(37, B[0], B[1], B[2], true) && SendQx(37, FEED_FG_Q_PIPELINED, 0.0f, 0.0f) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 37, 5000)
      && Copies(B[0], B[1], B[2], true, 37) && SendGx(37, FEED_FG_G_DISPATCH, 0) && FeedVkWaitTimeline(&gVk, gSet.semOut, 74, 5000);
    const uint64_t o37 = Seen(gSet.semOut, &gSeenOut);
    Proof(ok && o37 == 74, "A 'Q' THAT SAYS PIPELINED (pair 37) is prepared and dispatched like any other (fgOut %llu); the helper must have counted it (run.py reads that)", (unsigned long long)o37);
    // ---- twenty acquire and release cycles of such a set
    x = FgxRelease<FakeB>(&gSet, DL);
    Proof(x == FGX_RELEASED && !gSet.have, "the two-image set is RELEASED through the client's own lines (status %d)", gSet.status);
    DWORD h2 = 0, h20 = 0; int made = 0, freed = 0, two = 0;
    for (int k = 1; k <= 20; k++)
    {
        if (Acquire(FP16, DL, FEED_FG_BUILD_NO_HUDLESS) == FGX_CREATED) { made++; if (gSet.img[FEED_FG_HUDLESS] == VK_NULL_HANDLE && gSet.nt[FEED_FG_HUDLESS] == nullptr && gSet.img[FEED_FG_GEN] != VK_NULL_HANDLE) two++; }
        if (FgxRelease<FakeB>(&gSet, DL) == FGX_RELEASED) freed++;
        if (k == 2)  GetProcessHandleCount(GetCurrentProcess(), &h2);
        if (k == 20) GetProcessHandleCount(GetCurrentProcess(), &h20);
    }
    Proof(made == 20 && freed == 20 && two == 20 && h20 <= h2 + 2, "TWENTY ACQUIRE AND RELEASE CYCLES of a set with no HUD-less image: %d created and imported (%d with two images and no third handle), %d released; this process's handles %lu after the second, %lu after the twentieth",
          made, two, freed, (unsigned long)h2, (unsigned long)h20);
    // ---- THE IMAGE COMES BACK BY CLEARING THE BIT: three images, and a pair generates
    x = Acquire(FP16, DL);
    bool three = x == FGX_CREATED && gSet.have;
    for (int i = 0; i < FEED_FG_GEN2; i++) three = three && gSet.img[i] != VK_NULL_HANDLE && gSet.nt[i] != nullptr && gSet.fmt[i] == FP16;
    three = three && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE && gSet.nAll == 3;   // (wire 3: no fourth image unless asked)
    const uint64_t o38 = three ? Pair(38, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep) : 0;
    if (three) { Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f); NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 39"); }
    const uint64_t o39 = three ? Pair(39, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep) : 0;
    const int g39 = three ? CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first) : -1;
    Proof(three && o38 == 76 && o39 == 78 && g39 == 4096, "THE IMAGE COMES BACK BY CLEARING THE BIT: the next 'g' is CREATED with THREE images, and its pairs 38 and 39 generate (fgOut %llu, %llu; GEN is the constant on %d of 4096)",
          (unsigned long long)o38, (unsigned long long)o39, g39);
    Proof(!gFenceBack && gFenceReads > 8, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
    // the set stays ALIVE: the game closes the pipe with frame generation up, and the helper must end by itself
}

// =====================================================================================================================
// PHASE 3, LEVER 2, --fake-fg=11 (2026-10-10; docs/reports/fg-3/lever2-2026-10-10/DESIGN.md, section 6). 'Q' RIGHT BEHIND 'F':
// DOES THE PREPARE, NOW ASKED FOR AT THE INJECTION, DELAY THE FRAME'S OWN ANSWER? The helper is started with its lever
// --fghold_q=2: when it has read the connection's second 'Q' it sleeps 8 s BEFORE that pair's prepare. This side writes
// 'F'(34) and, at once, 'Q'(34), which is the game's order since lever 2. THE FRAME'S ANSWER MUST NOT STAND BEHIND THE
// HELD PREPARE: the out-fence reaches 34 and a batch on this queue that waits for it (the frame's home) retires, while
// fgPrep stands at 33 for a whole second. (Mode 7 proves the other direction: a prepare whose frame's input is withheld
// does not run.) Then the pair completes when the helper wakes, and a pair in THE ORDER OF THE DEVELOPMENT KEY ('Q' only
// after the frame's answer, as the hand wrote it until 2026-10-10) generates the same constant: the helper serves both.
// The helper's own count of this run is run.py's to read.
// =====================================================================================================================
static bool HomeSubmit(uint64_t waitOut, uint64_t value)
{
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.waitSemaphoreValueCount = 1; tl.pWaitSemaphoreValues = &waitOut;
    tl.signalSemaphoreValueCount = 1; tl.pSignalSemaphoreValues = &value;
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.pNext = &tl;
    si.waitSemaphoreCount = 1; si.pWaitSemaphores = &gSemOut; si.pWaitDstStageMask = &stage;
    si.signalSemaphoreCount = 1; si.pSignalSemaphores = &gBodySem;
    return vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) == VK_SUCCESS;
}
static void EarlyQRun()
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    bool prep = false; uint16_t first[4] = {};
    const int x = Acquire(FP16, FEED_FG_BUILD_DEADLINE_MS, FEED_FG_BUILD_NO_HUDLESS);
    const uint64_t o33 = x == FGX_CREATED ? Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep) : 0;
    HANDLE none = nullptr;
    const bool sem = FeedVkCreateTimeline(&gVk, false, &gBodySem, &none) >= 0 && gBodySem != VK_NULL_HANDLE;
    if (none) CloseHandle(none);
    if (!Proof(x == FGX_CREATED && prep && o33 == 66 && sem, "the game's set (no HUD-less image) and one whole pair before the hold (fgOut %llu)", (unsigned long long)o33)) return;
    // ---- THE GAME'S ORDER SINCE LEVER 2: 'F'(34), and 'Q'(34) right behind it
    LARGE_INTEGER qf, q0, q1; QueryPerformanceFrequency(&qf);
    bool ok = SendF(34, A[0], A[1], A[2], true);
    QueryPerformanceCounter(&q0);
    ok = ok && SendQ(34, 0);
    const bool outOk = ok && FeedVkWaitTimeline(&gVk, gSemOut, 34, 2000);
    QueryPerformanceCounter(&q1);
    const double outMs = (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart;
    const bool home = outOk && HomeSubmit(34, 1) && FeedVkWaitTimeline(&gVk, gBodySem, 1, 2000);
    const uint64_t prepThen = FeedVkTimelineValue(&gVk, gSet.semPrep);
    const bool prepStood = !FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 1000);
    Proof(ok && outOk && home && prepThen == 33 && prepStood && outMs < 1000.0,
          "'Q'(34) WRITTEN RIGHT BEHIND 'F'(34), the helper held ahead of that prepare: THE FRAME'S OWN ANSWER DID NOT WAIT FOR IT: the out-fence reached 34 %.1f ms after the 'Q' was written and a batch that waits for it (the frame's home) retired, while fgPrep read %llu and %s for a whole second more",
          outMs, (unsigned long long)prepThen, prepStood ? "stood there" : "MOVED");
    // ---- the pair completes when the helper wakes: its prepare, the copy with fgIn, 'G', the generated constant
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 34's dispatch");
    ok = Copies(A[0], A[1], A[2], true, 34) && SendG(34, FEED_FG_G_DISPATCH);
    const bool prep34 = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 12000);
    const bool out34 = prep34 && FeedVkWaitTimeline(&gVk, gSet.semOut, 68, 5000);
    const uint64_t o34 = Seen(gSet.semOut, &gSeenOut);
    const int g34 = out34 ? CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first) : -1;
    Proof(prep34 && out34 && o34 == 68 && g34 == 4096, "the held pair completes when the helper wakes: its prepare (fgPrep 34), then the copy with fgIn and 'G': fgOut %llu, and GEN is the constant on %d of 4096", (unsigned long long)o34, g34);
    // ---- THE ORDER OF THE DEVELOPMENT KEY: 'Q'(35) only after the frame's own answer
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 35");
    ok = SendF(35, A[0], A[1], A[2], true) && FeedVkWaitTimeline(&gVk, gSemOut, 35, 5000);
    const uint64_t prepBefore = FeedVkTimelineValue(&gVk, gSet.semPrep);
    ok = ok && SendQ(35, 0) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 35, 5000) && Copies(A[0], A[1], A[2], true, 35) && SendG(35, FEED_FG_G_DISPATCH)
      && FeedVkWaitTimeline(&gVk, gSet.semOut, 70, 5000);
    const uint64_t o35 = Seen(gSet.semOut, &gSeenOut);
    const int g35 = ok ? CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first) : -1;
    Proof(ok && prepBefore == 34 && o35 == 70 && g35 == 4096, "A PAIR IN THE ORDER OF fg_q_at_hand ('F'(35), its answer, and only then 'Q'(35): fgPrep read %llu when the frame was answered) generates too: fgOut %llu, GEN the constant on %d of 4096",
          (unsigned long long)prepBefore, (unsigned long long)o35, g35);
    Proof(!gFenceBack && gFenceReads >= 3, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}

static void FgRun(int mode, bool stamp)
{
    if (mode == 10) { Wire2Run(stamp); return; }    // phase 3: the wire's one revision (the block above)
    if (mode == 14) { Wire3Run(stamp); return; }    // the true fill: wire 3 (fgpipe_wire3.h)
    if (mode == 15) { Wire4Run(stamp, gCaps); return; }   // round 5, H: the second input set (fgpipe_wire4.h)
    if (mode == 16) { WineRun(); return; }          // round 7, L: under Wine, the game's own fences (fgpipe_wine.h)
    if (mode == 11) { EarlyQRun(); return; }        // phase 3, lever 2: 'Q' right behind 'F' (the block above)
    if (mode == 12 || mode == 13) { FixRun(mode); return; }   // phase 3: the helper's two fixes (fgpipe_fix.h)
    if (mode == 40) { CostRun(); return; }          // phase 3: the cost bench (fgpipe_cost.h)
    if (mode == 50) { DeskRun(); return; }          // phase 3: the desk measurements (fgpipe_desk.h)
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT, BGRA8 = (uint32_t)DXGI_FORMAT_B8G8R8A8_UNORM;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    const UINT CX = W / 2, CY = H / 2, LX = W / 4;
    const float A[3] = { 0.25f, 0.5f, 0.75f }, B[3] = { 0.75f, 0.25f, 0.5f };
    bool prep = false, ok = false; uint16_t first[4] = {};
    SIZE_T big0, tot0, big1, tot1;
    if (mode == 5 || mode == 6)
    {   // ---- the two outcomes that leave the pipe out of step: in the game each stops the transport
        const int x = Acquire(FP16, 1000);
        if (mode == 5) Proof(x == FGX_LATE && !gSet.have && !gSet.semIn, "LATE, through the client's own lines: the helper did not answer 'g' within the 1 s this run gives it (it sleeps 3 s): the lines say LATE and hold nothing");
        else           Proof(x == FGX_PIPE && !gSet.have && !gSet.semIn, "A BROKEN PIPE, through the client's own lines: the helper ended on 'g' with its ack unwritten: the lines say so and hold nothing");
        return;
    }
    FreeSpace(&big0, &tot0);
    int x = Acquire(FP16, DL);
    FreeSpace(&big1, &tot1);
    if (mode == 4)
    {   // ---- the video memory rule, forced: REFUSED through the client's lines, nothing held, and the frames go on
        Proof(x == FGX_REFUSED && gSet.status == FEED_FG_ST_VRAM && !gSet.have, "REFUSED, through the client's own lines: 'g' is refused by the video memory rule (status %d) and nothing is held", gSet.status);
        ok = SendF(33, A[0], A[1], A[2], true) && FeedVkWaitTimeline(&gVk, gSemOut, 33, 5000);
        const int c = ok ? OutputAt(gIm[FEED_OUTPUT], LX, CY, A[0], A[1], A[2], nullptr) : -1;
        Proof(c == 4096, "and the upscaler's frames go on: frame 33 is answered with its pixels (%d of 4096)", c);
        return;
    }
    Proof(x == FGX_CREATED && gSet.status == FEED_FG_ST_CREATED && gSet.warmRc == 0,
          "CREATED, through the client's own lines (status %d, AMD's create %d, the warm-up %d; the create and the warm-up took %u.%u ms; the context is %llu MiB by the provider's figure; provider 0x%llX)",
          gSet.status, gSet.ffxRc, gSet.warmRc, gSet.warmUs / 1000, (gSet.warmUs % 1000) / 100, (unsigned long long)(gSet.ctxBytes >> 20), (unsigned long long)gSet.provider);
    bool have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut && gSet.provider != 0;
    for (int i = 0; i < FEED_FG_GEN2; i++) have = have && gSet.img[i] != VK_NULL_HANDLE && gSet.fmt[i] == FP16;
    have = have && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE;   // (wire 3: the fourth image only where asked)
    Proof(have && gSet.storage[FEED_FG_GEN] && !gSet.storage[FEED_FG_PRESENT] && !gSet.storage[FEED_FG_HUDLESS],
          "the lines hold three fences as timeline semaphores and three images in the presented format, GEN alone imported with storage usage");
    Say("  the imports of 3 fences and 3 images at %ux%u cost %ld KB of free address space (the largest block moved by %ld KB)", W, H,
        (long)((tot0 - tot1) >> 10), (long)(((long long)big0 - (long long)big1) / 1024));
    if (!have) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    if (stamp)
    {   // ---- which handle is which image, by pixels
        const uint8_t tagS = 'S'; uint8_t done = 0;
        ok = PipeWrite(&tagS, 1) && PipeRead(&done, 1, 10000) && done == 1;
        const int c0 = ok ? CountAt(SetImg(FEED_FG_PRESENT), CX, CY, 0.125f, 0.25f, 0.5f, nullptr) : -1;
        const int c1 = ok ? CountAt(SetImg(FEED_FG_GEN),     CX, CY, 0.5f, 0.125f, 0.25f, nullptr) : -1;
        const int c2 = ok ? CountAt(SetImg(FEED_FG_HUDLESS), CX, CY, 0.25f, 0.5f, 0.125f, nullptr) : -1;
        const int cx = ok ? CountAt(SetImg(FEED_FG_PRESENT), CX, CY, 0.5f, 0.125f, 0.25f, nullptr) : -1;
        Proof(c0 == 4096 && c1 == 4096 && c2 == 4096, "THE THREE CONSTANTS are where the machine's order says: PRESENT %d, GEN %d, HUDLESS %d of 4096 (stamped by the helper on 'S', read through the imports the client's lines made)", c0, c1, c2);
        Proof(cx == 0, "and the reader tells them apart: PRESENT holds none of GEN's constant (%d of 4096)", cx);
    }
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN");

    if (mode == 2)
    {   // ---- THE RAISE LEVER: the helper's second dispatch raises inside its wrapper
        const uint64_t o1 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
        const uint64_t o2 = Pair(34, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
        const uint64_t o3 = Pair(35, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
        Proof(o1 == 66 && o2 == 69 && o3 == 71 && prep, "the pair before the raise has its frame (fgOut %llu), the pair that raised and the one after have none (%llu, %llu), and fgPrep still comes",
              (unsigned long long)o1, (unsigned long long)o2, (unsigned long long)o3);
        ok = SendF(36, B[0], B[1], B[2], true) && FeedVkWaitTimeline(&gVk, gSemOut, 36, 5000);
        const int c = ok ? OutputAt(gIm[FEED_OUTPUT], LX, CY, B[0], B[1], B[2], first) : -1;
        Proof(c == 4096, "AFTER THE RAISE THE UPSCALER'S FRAMES GO ON, by pixels: frame 36's constant is in Output (%d of 4096)", c);
        FgxDrop<FakeB>(&gSet);
        x = Acquire(FP16, DL);
        Proof(x == FGX_REFUSED && gSet.status == FEED_FG_ST_RAISED && !gSet.have, "and the next 'g' is REFUSED as raised, through the client's own lines (status %d)", gSet.status);
        return;
    }
    if (mode == 3)
    {   // ---- THE FAILURE LEVER: dispatches 2, 3 and 4 fail before AMD's call
        uint64_t o[5];
        for (int k = 0; k < 5; k++) o[k] = Pair(33 + k, k == 0 ? FEED_FG_Q_RESET_BUILD : 0u, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
        Proof(o[0] == 66 && o[1] == 69 && o[2] == 71 && o[3] == 73 && o[4] == 75,
              "one frame, then three planted failures (%llu, %llu, %llu), and the pair after them is answered with no frame too (%llu): three in a row end frame generation for the set",
              (unsigned long long)o[1], (unsigned long long)o[2], (unsigned long long)o[3], (unsigned long long)o[4]);
        FgxDrop<FakeB>(&gSet);
        x = Acquire(FP16, DL);
        const uint64_t o6 = x == FGX_CREATED ? Pair(38, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep) : 0;
        Proof(x == FGX_CREATED && o6 == 76, "a new 'g' starts afresh: CREATED through the client's lines (the same three fences, or they would refuse it), and its first pair has a frame (fgOut %llu)", (unsigned long long)o6);
        return;
    }

    // ---- two pairs of one constant: the generated image is that constant, exactly
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    Proof(prep && o33 == 66, "pair 33, the first after 'g' (with reset): fgPrep reached 33 behind the prepare and fgOut reads %llu: a frame was generated", (unsigned long long)o33);
    // (GEN is seeded again before the pair whose image is read: pair 33 wrote the same constant, and a pair 34 that
    // dispatched nothing would otherwise read as exact. The review of stages 5 to 7.)
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 34");
    const uint64_t o34 = Pair(34, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    const int g34 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first);
    Proof(prep && o34 == 68 && g34 == 4096, "pair 34: fgOut reads %llu and GEN is the constant that went into PRESENT and HUDLESS, exactly (%d of 4096; first pixel 0x%04X 0x%04X 0x%04X)",
          (unsigned long long)o34, g34, first[0], first[1], first[2]);

    // ---- THE HELD 'Q': frame 35's in-fence is withheld. The prepare is on the queue behind the wait for it: fgPrep must not come.
    ok = SendF(35, A[0], A[1], A[2], false) && SendQ(35, 0);
    const bool earlyQ = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, 35, 1500);
    const uint64_t heldPrep = Seen(gSet.semPrep, &gSeenPrep);
    Proof(ok && !earlyQ && heldPrep == 34, "THE HELD 'Q': frame 35's in-fence was withheld for 1.5 s: fgPrep %s (it reads %llu), as it must",
          earlyQ ? "MOVED" : "did not move", (unsigned long long)heldPrep);
    ok = ok && SignalNow(gSemIn, 35) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 35, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 35, 5000);
    Proof(ok && Seen(gSet.semPrep, &gSeenPrep) == 35, "and once the in-fence is signalled the prepare runs: fgPrep reads 35 and the frame's own out-fence 35");

    // ---- THE HELD 'G': pair 35's fgIn is withheld, and frame 36 is written BEHIND it. One queue: neither may be answered.
    ok = ok && Copies(A[0], A[1], A[2], false, 35) && SendG(35, FEED_FG_G_DISPATCH) && SendF(36, A[0], A[1], A[2], true);
    const bool earlyG = ok && FeedVkWaitTimeline(&gVk, gSet.semOut, 70, 1500);
    const uint64_t heldOut = Seen(gSet.semOut, &gSeenOut), heldUp = FeedVkTimelineValue(&gVk, gSemOut);
    Proof(ok && !earlyG && heldOut == 68 && heldUp == 35,
          "THE HELD 'G': pair 35's fgIn was withheld for 1.5 s: fgOut %s (it reads %llu), and frame 36, written behind it, was not answered either (the out-fence reads %llu): ONE QUEUE, as it must",
          earlyG ? "MOVED" : "did not move", (unsigned long long)heldOut, (unsigned long long)heldUp);
    ok = ok && SignalNow(gSet.semIn, 35) && FeedVkWaitTimeline(&gVk, gSet.semOut, 70, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 36, 5000);
    Proof(ok && Seen(gSet.semOut, &gSeenOut) == 70, "and once fgIn is signalled both go: fgOut reads 70 and frame 36 is answered");

    // ---- a pair with no source: answered at once, no wait on fgIn, GEN not written
    // (GEN is SEEDED first with a value no dispatch makes: PRESENT, HUDLESS and GEN all held the same constant here, and a
    // wrongful dispatch would have written that constant over itself. The review of stages 5 to 7.)
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before the pair with no source");
    const int before = CountAt(SetImg(FEED_FG_GEN), CX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    const uint64_t o36 = Pair(36, 0, FEED_FG_G_NOSOURCE, 0, 0, 0, false, &prep);
    const int after = CountAt(SetImg(FEED_FG_GEN), CX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    Proof(prep && o36 == 73 && before == 4096 && after == 4096, "a pair with NO SOURCE (fgIn never signalled): answered %llu at once, and GEN was not written (the seed is on %d of 4096 before, %d after)",
          (unsigned long long)o36, before, after);

    // ---- the helper resets on its own after a pair with no frame; two pairs of a NEW constant give exactly that constant
    const uint64_t o37 = Pair(37, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);            // seeded again: pair 38's own dispatch must write it
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN before pair 38");
    const uint64_t o38 = Pair(38, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    const int g38 = CountAt(SetImg(FEED_FG_GEN), CX, CY, B[0], B[1], B[2], first);
    Proof(o37 == 74 && o38 == 76 && g38 == 4096, "after it, two pairs of a new constant (fgOut %llu, %llu): GEN is the new constant, exactly (%d of 4096)",
          (unsigned long long)o37, (unsigned long long)o38, g38);

    // ---- a repeated 'Q' is counted and NEVER answered
    ok = SendQ(37, 0); Sleep(400);
    const uint64_t dupPrep = Seen(gSet.semPrep, &gSeenPrep);
    Proof(ok && dupPrep == 38, "a repeated 'Q' (pair 37 again) is not answered: fgPrep still reads %llu", (unsigned long long)dupPrep);

    // ---- a 'G' with no 'Q': no frame, and NO WAIT on fgIn (a wait would hold this answer for good)
    ok = SendG(45, FEED_FG_G_DISPATCH) && FeedVkWaitTimeline(&gVk, gSet.semOut, 91, 5000);
    const uint64_t drift = Seen(gSet.semOut, &gSeenOut);
    Proof(ok && drift == 91, "a 'G' with no 'Q' (pair 45, said to be a dispatch): answered %llu, no frame, with no wait on fgIn", (unsigned long long)drift);
    const uint64_t o46 = Pair(46, FEED_FG_Q_RESET_GAP, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    const uint64_t o47 = Pair(47, 0, FEED_FG_G_WITHHELD, B[0], B[1], B[2], true, &prep);
    Proof(prep && o46 == 92 && o47 == 95, "and the wire goes on: pair 46 has its frame (%llu); pair 47, WITHHELD by the game with its inputs made, has none (%llu)",
          (unsigned long long)o46, (unsigned long long)o47);

    // ---- a 'g' BY HAND with the set still held (the client's lines never send one): refused, and the set before it lives
    FeedFgAckIvt a2; ok = RawExchange(W, H, (uint32_t)DXGI_FORMAT_R32G32B32A32_FLOAT, gWireFlags, &a2);
    const uint64_t o48 = Pair(48, FEED_FG_Q_RESET_GAP, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    Proof(ok && a2.status == FEED_FG_ST_FORMAT && a2.fence_in == gSet.fence[0] && a2.fence_prep == gSet.fence[1] && a2.fence_out == gSet.fence[2] && o48 == 96,
          "a 'g' with a format outside the five, sent by hand, is refused (status %d) with the same three fences, and the set before it still generates (pair 48: %llu)", a2.status, (unsigned long long)o48);

    // ---- a second set in another format, with no build between
    x = Acquire(BGRA8, DL);
    const uint64_t o49 = x == FGX_CREATED ? Pair(49, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep) : 0;
    Proof(x == FGX_CREATED && gSet.fmt[FEED_FG_GEN] == BGRA8 && o49 == 98, "a second 'g' in another format (B8G8R8A8) is CREATED through the client's lines, and its first pair has a frame (%llu)", (unsigned long long)o49);

    // ---- the release, and a pair after it
    x = FgxRelease<FakeB>(&gSet, DL);
    const uint64_t o50 = Pair(50, 0, FEED_FG_G_NOSOURCE, 0, 0, 0, true, &prep);
    Proof(x == FGX_RELEASED && gSet.status == FEED_FG_ST_RELEASED && !gSet.have && prep && o50 == 101,
          "RELEASED, through the client's own lines (status %d); a pair after it still gets fgPrep and is answered with no frame (%llu)", gSet.status, (unsigned long long)o50);

    // ---- REFUSED through the client's own lines: nothing held
    x = Acquire((uint32_t)DXGI_FORMAT_R32G32B32A32_FLOAT, DL);
    Proof(x == FGX_REFUSED && gSet.status == FEED_FG_ST_FORMAT && !gSet.have, "REFUSED, through the client's own lines (a format outside the five: status %d): nothing is held", gSet.status);

    // ---- twenty acquire and release cycles: this side's handles flat, the helper's video memory read from its log by run.py
    DWORD h2 = 0, h20 = 0; int made = 0, freed = 0;
    for (int k = 1; k <= 20; k++)
    {
        if (Acquire(FP16, DL) == FGX_CREATED) made++;
        if (FgxRelease<FakeB>(&gSet, DL) == FGX_RELEASED) freed++;
        if (k == 2)  GetProcessHandleCount(GetCurrentProcess(), &h2);
        if (k == 20) GetProcessHandleCount(GetCurrentProcess(), &h20);
    }
    Proof(made == 20 && freed == 20 && h20 <= h2 + 2, "TWENTY ACQUIRE AND RELEASE CYCLES through the client's own lines: %d created and imported, %d released; this process's handles %lu after the second, %lu after the twentieth",
          made, freed, (unsigned long)h2, (unsigned long)h20);

    // ---- a rebuild of the upscaler's set with frame generation alive: the game drops its imports, then builds, then asks again
    x = Acquire(FP16, DL);
    FgxDrop<FakeB>(&gSet);       // as the build's head does: the game's imports go before the main set's
    FeedBuild b = {}; b.width = W; b.height = H; b.color_fmt = FP16; b.output_fmt = FP16;
    b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1; b.transport = gRealUpscale ? 0 : 1; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    FeedBuildAck back = {}; const uint8_t tagB = 'B';
    ok = x == FGX_CREATED && PipeWrite(&tagB, 1) && PipeWrite(&b, sizeof(b)) && PipeRead(&back, sizeof(back), 60000) && back.ok == 1;
    for (int i = 0; i < FEED_SLOTS; i++) if (back.tex[i]) CloseHandle((HANDLE)(uintptr_t)back.tex[i]);   // the new set is not used by this run
    x = ok ? Acquire(FP16, DL) : -1;
    const uint64_t o51 = x == FGX_CREATED ? Pair(51, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep) : 0;
    Proof(ok && x == FGX_CREATED && o51 == 102, "a REBUILD of the upscaler's set with frame generation alive is answered ok, the next 'g' is CREATED, and its first pair has a frame (%llu)", (unsigned long long)o51);

    Proof(!gFenceBack && gFenceReads > 20, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
    // the set stays ALIVE: the game closes the pipe with frame generation up, and the helper must end by itself
}

// THE FREEZE CASE (2026-10-06), --fake-fg=30: a real D3D9 device through DXVK that keeps PRESENTING while the wait on the
// helper's prepare is queued on DXVK's own queue. All of it is in the file below; main only hands over to it.
#include "fgpipe_freeze.h"

int main(int argc, char** argv)
{
    if (argc < 3) { Say("usage: fgpipe.exe <helper.exe> <helper log path> [extra helper arguments...]"); Say("FAIL"); return 2; }
    for (int i = 3; i < argc; i++) if (strcmp(argv[i], "--fake-fg=30") == 0) return PresMain(argc, argv);   // before any Vulkan of this program's own: that mode's only device is DXVK's
    Say("the fake client: %u-bit, pid %lu", (unsigned)(8 * sizeof(void*)), (unsigned long)GetCurrentProcessId());
    Proof(sizeof(void*) == 4, "this program is 32-bit, as the game is");
    SIZE_T big0, tot0; FreeSpace(&big0, &tot0);

    // ---- Vulkan: the first discrete GPU that reports a LUID, a device with the two win32 external extensions and timeline semaphores
    HMODULE vl = LoadLibraryW(L"vulkan-1.dll"); NEED(vl, "vulkan-1.dll (32-bit)");
    vkGIPA = (PFN_vkGetInstanceProcAddr)GetProcAddress(vl, "vkGetInstanceProcAddr"); NEED(vkGIPA, "vkGetInstanceProcAddr");
    vkCreateInstance_ = (PFN_vkCreateInstance)vkGIPA(nullptr, "vkCreateInstance"); NEED(vkCreateInstance_, "vkCreateInstance");
    VkApplicationInfo ai = { VK_STRUCTURE_TYPE_APPLICATION_INFO }; ai.pApplicationName = "DLSS-IV fake client"; ai.pEngineName = "DLSS-IV fake client"; ai.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ic = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO }; ic.pApplicationInfo = &ai;
    NEED(vkCreateInstance_(&ic, nullptr, &gInst) == VK_SUCCESS, "vkCreateInstance");
#define VKI(n) n##_ = (PFN_##n)vkGIPA(gInst, #n); NEED(n##_, #n)
    VKI(vkEnumeratePhysicalDevices); VKI(vkGetPhysicalDeviceProperties2); VKI(vkGetPhysicalDeviceQueueFamilyProperties);
    VKI(vkGetPhysicalDeviceMemoryProperties); VKI(vkCreateDevice); VKI(vkGetDeviceProcAddr);
    uint32_t np = 8; VkPhysicalDevice pds[8]; vkEnumeratePhysicalDevices_(gInst, &np, pds);
    char devName[256] = "";
    for (int pass = 0; pass < 2 && !gPhys; pass++)
        for (uint32_t i = 0; i < np && !gPhys; i++)
        {
            VkPhysicalDeviceIDProperties idp = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
            VkPhysicalDeviceProperties2 p2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }; p2.pNext = &idp;
            vkGetPhysicalDeviceProperties2_(pds[i], &p2);
            if (!idp.deviceLUIDValid || (pass == 0 && p2.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) continue;
            gPhys = pds[i]; memcpy(gLuid, idp.deviceLUID, 8); strcpy_s(devName, p2.properties.deviceName);
        }
    NEED(gPhys, "no Vulkan device with a LUID");
    vkGetPhysicalDeviceMemoryProperties_(gPhys, &gMem);
    uint32_t nf = 16; VkQueueFamilyProperties fams[16]; vkGetPhysicalDeviceQueueFamilyProperties_(gPhys, &nf, fams);
    for (uint32_t i = 0; i < nf; i++) if (fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { gFam = i; break; }
    const float prio = 1.0f;
    VkDeviceQueueCreateInfo qi = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO }; qi.queueFamilyIndex = gFam; qi.queueCount = 1; qi.pQueuePriorities = &prio;
    const char* exts[] = { "VK_KHR_external_memory_win32", "VK_KHR_external_semaphore_win32" };
    VkPhysicalDeviceTimelineSemaphoreFeatures ts = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES }; ts.timelineSemaphore = VK_TRUE;
    VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO }; dci.pNext = &ts; dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qi;
    dci.enabledExtensionCount = 2; dci.ppEnabledExtensionNames = exts;
    NEED(vkCreateDevice_(gPhys, &dci, nullptr, &gDev) == VK_SUCCESS, "vkCreateDevice");
    const char* miss = nullptr;
    NEED(FeedVkLoad(&gVk, gDev, &miss), "FeedVkLoad, the mod's own loader");
#define VKD(n) n##_ = (PFN_##n)vkGetDeviceProcAddr_(gDev, #n); NEED(n##_, #n)
    VKD(vkGetDeviceQueue); VKD(vkCreateCommandPool); VKD(vkAllocateCommandBuffers); VKD(vkBeginCommandBuffer); VKD(vkEndCommandBuffer);
    VKD(vkResetCommandBuffer); VKD(vkQueueSubmit); VKD(vkQueueWaitIdle); VKD(vkCreateBuffer); VKD(vkGetBufferMemoryRequirements);
    VKD(vkBindBufferMemory); VKD(vkMapMemory); VKD(vkCmdCopyImageToBuffer); VKD(vkCmdClearColorImage); VKD(vkSignalSemaphore);
    vkGetDeviceQueue_(gDev, gFam, 0, &gQ);
    VkCommandPoolCreateInfo pci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; pci.queueFamilyIndex = gFam; pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    NEED(vkCreateCommandPool_(gDev, &pci, nullptr, &gPool) == VK_SUCCESS, "vkCreateCommandPool");
    VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = gPool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 1;
    NEED(vkAllocateCommandBuffers_(gDev, &cai, &gCb) == VK_SUCCESS, "vkAllocateCommandBuffers");
    {   // the readback buffer: 64 x 64 FP16 pixels
        VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = 64 * 64 * 8; bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        NEED(vkCreateBuffer_(gDev, &bi, nullptr, &gRb) == VK_SUCCESS, "vkCreateBuffer");
        VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, gRb, &q);
        VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size;
        mi.memoryTypeIndex = MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &gRbMem) == VK_SUCCESS, "vkAllocateMemory");
        NEED(vkBindBufferMemory_(gDev, gRb, gRbMem, 0) == VK_SUCCESS, "vkBindBufferMemory");
        NEED(vkMapMemory_(gDev, gRbMem, 0, bi.size, 0, &gRbPtr) == VK_SUCCESS, "vkMapMemory");
    }
    Say("  Vulkan: %s, the mod's own loader took its entry points", devName);
    SIZE_T big1, tot1; FreeSpace(&big1, &tot1);

    // ---- the helper, started as the game starts it: its first argument is OUR pid
    const DWORD me = GetCurrentProcessId();
    char cmd[4096]; int at = sprintf_s(cmd, "\"%s\" %lu --log=1 --sync=0 --logpath=\"%s\"", argv[1], (unsigned long)me, argv[2]);
    bool helloOnly = false; int wantFg = -1; int sendTag = 0; int fgMode = 0; bool fgStamp = false; int wire = 3;
    for (int i = 3; i < argc; i++)
    {
        if      (strcmp(argv[i], "--fake-hello-only") == 0)       helloOnly = true;
        else if (strncmp(argv[i], "--fake-want-fg=", 15) == 0)    wantFg = atoi(argv[i] + 15);
        else if (strncmp(argv[i], "--fake-send-tag=", 16) == 0)   sendTag = (unsigned char)argv[i][16];
        else if (strncmp(argv[i], "--fake-fg=", 10) == 0)         fgMode = atoi(argv[i] + 10);
        else if (strcmp(argv[i], "--fake-fg-stamp") == 0)         fgStamp = true;
        else if (strcmp(argv[i], "--fake-real-upscale") == 0)     gRealUpscale = true;
        else if (strncmp(argv[i], "--fake-wire=", 12) == 0)       wire = atoi(argv[i] + 12);   // (wire 3) 2: this run speaks as a client of wire 2, whatever the helper offers
        else if (strncmp(argv[i], "--fake-cost-", 12) == 0)       CostOption(argv[i] + 12);   // the cost bench's own (fgpipe_cost.h)
        else if (strncmp(argv[i], "--fake-desk-", 12) == 0)       DeskOption(argv[i] + 12);   // the desk measurements' own (fgpipe_desk.h)
        else if (strncmp(argv[i], "--fake-fix=", 11) == 0)        xCase = atoi(argv[i] + 11); // which case of the two fixes' modes (fgpipe_fix.h)
        else if (strncmp(argv[i], "--fake-wine=", 12) == 0)       gFakeWine = atoi(argv[i] + 12); // (round 7, L) the binding says Wine: the case of --fake-fg=16 (fgpipe_wine.h)
        else at += sprintf_s(cmd + at, sizeof(cmd) - at, " %s", argv[i]);
    }
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION helper = {};
    NEED(CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &helper), "the helper did not start");
    gHelper = helper.hProcess;
    char name[128]; sprintf_s(name, FEED_PIPE_FMT, (unsigned long)me);
    for (int i = 0; i < 400 && gPipe == INVALID_HANDLE_VALUE; i++)
    {
        gPipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
        if (gPipe == INVALID_HANDLE_VALUE) { if (WaitForSingleObject(helper.hProcess, 0) == WAIT_OBJECT_0) break; Sleep(50); }
    }
    if (!Proof(gPipe != INVALID_HANDLE_VALUE, "the helper made its pipe for this pid and the connect succeeded")) { TerminateProcess(helper.hProcess, 9); Say("FAIL"); return 1; }

    // ---- hello: version 16, a Vulkan client, our adapter's LUID, a build id that proves nothing (so it is not compared)
    FeedHello hello = {}; hello.magic = FEED_IPC_MAGIC; hello.version = FEED_IPC_VERSION; hello.pid = me; hello.client_kind = FEED_CLIENT_VULKAN;
    hello.self_process = 0; memcpy(hello.luid, gLuid, 8); strcpy_s(hello.build, "unknown-source (the fake client)");
    FeedHelloAck hack = {};
    bool ok = PipeWrite(&hello, sizeof(hello)) && PipeRead(&hack, sizeof(hack), 30000);
    Proof(ok && hack.magic == FEED_IPC_MAGIC && hack.version == FEED_IPC_VERSION, "the hello is answered: version %u, caps 0x%llX", hack.version, (unsigned long long)hack.caps);
    Proof(ok && !(hack.caps & (FEED_CAP_REFUSED | FEED_CAP_BUILD_MISMATCH)), "the helper did not refuse the session (same adapter by LUID, an unprovable build id is not compared)");
    Say("  caps: frame generation %s", (hack.caps & FEED_CAP_FG) ? "OFFERED (1024)" : (hack.caps & FEED_CAP_FG_PROBED) ? "asked for, no provider (256 without 1024)" : "not offered (neither bit: this helper was not asked, or does not know how)");
    if (wantFg >= 0)
    {
        const unsigned gotFg = ((hack.caps & FEED_CAP_FG_PROBED) ? 1u : 0u) | ((hack.caps & FEED_CAP_FG) ? 2u : 0u);
        static const char* const kSay[4] = { "not asked (neither bit)", "asked, nothing usable (256 without 1024)", "1024 WITHOUT 256, which no helper may say", "asked and available (256 and 1024)" };
        Proof(ok && gotFg == (unsigned)wantFg, "the handshake says of frame generation what this session must: %s (it must be: %s)", kSay[gotFg], kSay[wantFg & 3]);
        // THE WIRE'S REVISION IS THE BIT (feed_ipc.h): 512 was wire 1's, and a helper of wire 2 never offers it, asked or not
        Proof(ok && !(hack.caps & FEED_CAP_FG_WIRE1), "and it does NOT offer wire 1's bit (512), which a game of the wire before this one would take for its own: caps 0x%llX", (unsigned long long)hack.caps);
    }
    if (!ok) { TerminateProcess(helper.hProcess, 9); Say("FAIL"); return 1; }
    // (wire 3, 2026-10-18) EVERY SET OF THIS RUN IS ASKED AT WIRE 3 where the helper offered 2048 beside 1024 and the run is not
    // told to speak as a client of wire 2: what the game does (fg_client.h, FgxAfterBuild)
    gWireFlags = ((hack.caps & FEED_CAP_FG) && (hack.caps & FEED_CAP_FG_WIRE3) && wire != 2) ? (uint32_t)FEED_FG_BUILD_WIRE3 : 0u;
    gCaps = hack.caps;                              // (round 5, H: --fake-fg=15 asks the second input set only where 4096 was offered)
    if (hack.caps & FEED_CAP_FG)
        Say("  wire: the helper %s 2048 beside 1024; this run speaks wire %d", (hack.caps & FEED_CAP_FG_WIRE3) ? "OFFERS" : "does NOT offer", gWireFlags ? 3 : 2);
    if (sendTag)
    {   // ---- one tag byte the helper must not serve: IT ends the session, with the pipe still open on this side
        const uint8_t t = (uint8_t)sendTag;
        const bool wrote = PipeWrite(&t, 1);
        const bool ended = WaitForSingleObject(helper.hProcess, 20000) == WAIT_OBJECT_0;
        DWORD ec = 99; GetExitCodeProcess(helper.hProcess, &ec);
        if (!ended) TerminateProcess(helper.hProcess, 9);
        Proof(wrote && ended && ec == 0, "the tag '%c' alone ended the session: the helper ended by itself while this side kept the pipe open (exit code %lu)", (char)sendTag, (unsigned long)ec);
        CloseHandle(gPipe);
        Say("%d proofs passed, %d failed", gPasses, gFails);
        Say(gFails ? "FAIL" : "PASS");
        return gFails ? 1 : 0;
    }
    if (helloOnly)
    {   // ---- the session ends here: the game closes the pipe, the helper ends by itself
        CloseHandle(gPipe);
        const bool ended = WaitForSingleObject(helper.hProcess, 20000) == WAIT_OBJECT_0;
        DWORD ec = 99; GetExitCodeProcess(helper.hProcess, &ec);
        if (!ended) TerminateProcess(helper.hProcess, 9);
        Proof(ended && ec == 0, "the helper ended by itself within 20 s of the pipe closing (exit code %lu)", (unsigned long)ec);
        Say("%d proofs passed, %d failed", gPasses, gFails);
        Say(gFails ? "FAIL" : "PASS");
        return gFails ? 1 : 0;
    }

    // ---- 'B': a TRANSPORT build (the helper copies the left half of Color to Output: no upscaler needed), FP16, at the
    // product's largest size so the address-space figure below is the product's
    const UINT LX = W / 4, RX = 3 * W / 4, CY = H / 2;     // the centres of the two blocks that are read
    FeedBuild b = {}; b.width = W; b.height = H; b.color_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT; b.output_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1; b.transport = gRealUpscale ? 0 : 1; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    FeedBuildAck back = {}; const uint8_t tagB = 'B';
    ok = PipeWrite(&tagB, 1) && PipeWrite(&b, sizeof(b)) && PipeRead(&back, sizeof(back), 60000);
    Proof(ok && back.ok == 1, "the build is answered ok (ok %d, result 0x%X, output format %u)", back.ok, back.ngx_result, back.output_fmt);
    bool handles = ok && back.fence_in && back.fence_out;
    for (int i = 0; i < FEED_SLOTS; i++) handles = handles && back.tex[i] != 0;
    Proof(handles, "the ack carries two fences and four texture handles, valid in this process");
    if (!ok || !handles) { CloseHandle(gPipe); WaitForSingleObject(helper.hProcess, 15000); Say("FAIL"); return 1; }

    // ---- the imports, by the mod's own code, and what they cost a 32-bit process
    SIZE_T big2, tot2; FreeSpace(&big2, &tot2);
    const VkSemaphore semIn = FeedVkImportFence(&gVk, (HANDLE)(uintptr_t)back.fence_in), semOut = FeedVkImportFence(&gVk, (HANDLE)(uintptr_t)back.fence_out);
    Proof(semIn != VK_NULL_HANDLE && semOut != VK_NULL_HANDLE, "both D3D12 fences import as Vulkan timeline semaphores (FeedVkImportFence)");
    Img im[FEED_SLOTS]; bool imported = true;
    const VkFormat fm[FEED_SLOTS] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT };
    for (int i = 0; i < FEED_SLOTS; i++)
    {
        im[i].w = W; im[i].h = H;
        imported = imported && FeedVkImportImage(&gVk, (HANDLE)(uintptr_t)back.tex[i], W, H, fm[i], i == FEED_OUTPUT, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, true, &im[i].img, &im[i].mem);
    }
    Proof(imported, "the four shared textures import as Vulkan images (FeedVkImportImage; the Output with storage usage)");
    if (semIn == VK_NULL_HANDLE || semOut == VK_NULL_HANDLE || !imported) { CloseHandle(gPipe); WaitForSingleObject(helper.hProcess, 15000); Say("FAIL"); return 1; }
    SIZE_T big3, tot3; FreeSpace(&big3, &tot3);
    Say("  address space (this 32-bit process): largest free block %lu MB at start, %lu after Vulkan, %lu before the imports, %lu after them; free in all %lu, %lu, %lu, %lu MB",
        (unsigned long)(big0 >> 20), (unsigned long)(big1 >> 20), (unsigned long)(big2 >> 20), (unsigned long)(big3 >> 20),
        (unsigned long)(tot0 >> 20), (unsigned long)(tot1 >> 20), (unsigned long)(tot2 >> 20), (unsigned long)(tot3 >> 20));
    Say("  the imports of 2 fences and 4 images at %ux%u cost %ld KB of free address space (the largest block moved by %ld KB)", W, H,
        (long)((tot2 - tot3) >> 10), (long)(((long long)big2 - (long long)big3) / 1024));

    // ---- the first layouts, and Output seeded with a value no copy of Color can leave there
    Begin();
    { VkImage all[FEED_SLOTS]; for (int i = 0; i < FEED_SLOTS; i++) all[i] = im[i].img; FeedVkBarrierN(&gVk, gCb, all, FEED_SLOTS, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); }
    Clear(im[FEED_OUTPUT], -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the first submit");
    uint16_t first[4] = {};
    const int s0 = CountAt(im[FEED_OUTPUT], LX, CY, -1.0f, -1.0f, -1.0f, first), s1 = CountAt(im[FEED_OUTPUT], RX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    Proof(s0 == 4096 && s1 == 4096, "Output holds the seed in both halves before any frame (%d and %d of 4096; first pixel 0x%04X 0x%04X 0x%04X)", s0, s1, first[0], first[1], first[2]);

    // ---- FRAME 1: a constant into Color with the in-fence signalled IN THAT SUBMIT, 'F', the out-fence, the constant in Output
    auto frame = [&](uint64_t n, float r, float g, float bl, bool signal) -> bool
    {
        Begin(); Clear(im[FEED_COLOR], r, g, bl, 1.0f);
        if (!Submit(signal ? semIn : VK_NULL_HANDLE, n)) return false;
        FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.reset = (n == 1 || gRealUpscale) ? 1u : 0u; f.fm.frame_time_ms = 16.67f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
        return PipeWrite(&f, sizeof(f));
    };
    ok = frame(1, 0.25f, 0.5f, 0.75f, true) && FeedVkWaitTimeline(&gVk, semOut, 1, 5000);
    Proof(ok, "frame 1: the out-fence reached 1 within 5 s of the frame message");
    const char* exactly = gRealUpscale ? "within 0.02, through the upscaler" : "exactly";
    const int n1 = ok ? OutputAt(im[FEED_OUTPUT], LX, CY, 0.25f, 0.5f, 0.75f, first) : -1;
    const int r1 = !ok ? -1 : gRealUpscale ? OutputAt(im[FEED_OUTPUT], RX, CY, 0.25f, 0.5f, 0.75f, nullptr) : CountAt(im[FEED_OUTPUT], RX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    Proof(n1 == 4096, "frame 1: Output's left half is the constant that went into Color, %s (%d of 4096; first pixel 0x%04X 0x%04X 0x%04X)", exactly, n1, first[0], first[1], first[2]);
    Proof(r1 == 4096, gRealUpscale ? "frame 1: Output's right half is that constant too, and not the seed: the upscaler writes the whole image (%d of 4096)"
                                   : "frame 1: Output's right half, which the transport copy does not write, still holds the seed (%d of 4096): the blocks read are where the helper wrote and where it did not", r1);

    // ---- THE CONTROL: frame 2 with the in-fence NOT signalled. The helper must not answer, and Output must keep frame 1.
    ok = frame(2, 0.75f, 0.25f, 0.5f, false);
    const bool early = ok && FeedVkWaitTimeline(&gVk, semOut, 2, 1500);
    const uint64_t outHeld = FeedVkTimelineValue(&gVk, semOut);
    const int kept = OutputAt(im[FEED_OUTPUT], LX, CY, 0.25f, 0.5f, 0.75f, nullptr);
    Proof(ok && !early && outHeld == 1 && kept == 4096,
          "THE CONTROL: frame 2's in-fence was withheld for 1.5 s: the out-fence %s (it reads %llu) and Output %s, as it must",
          early ? "MOVED" : "did not move", (unsigned long long)outHeld, kept == 4096 ? "still holds frame 1" : "CHANGED");
    VkSemaphoreSignalInfo ssi = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO }; ssi.semaphore = semIn; ssi.value = 2;
    ok = ok && vkSignalSemaphore_(gDev, &ssi) == VK_SUCCESS && FeedVkWaitTimeline(&gVk, semOut, 2, 5000);
    const int n2 = ok ? OutputAt(im[FEED_OUTPUT], LX, CY, 0.75f, 0.25f, 0.5f, first) : -1;
    Proof(ok && n2 == 4096, "and once the in-fence is signalled the same frame is answered: Output is frame 2's constant, %s (%d of 4096; first pixel 0x%04X 0x%04X 0x%04X)", exactly, n2, first[0], first[1], first[2]);

    // ---- thirty more, back to back, the constant changing every frame; the last one read
    int answered = 0; float lr = 0, lg = 0, lb = 0;
    for (uint64_t n = 3; n <= 32 && ok; n++)
    {
        lr = (float)(n % 4) * 0.25f; lg = (float)((n + 1) % 4) * 0.25f; lb = (float)((n + 2) % 4) * 0.25f;
        ok = frame(n, lr, lg, lb, true) && FeedVkWaitTimeline(&gVk, semOut, n, 5000);
        if (ok) answered++;
    }
    const int n3 = ok ? OutputAt(im[FEED_OUTPUT], LX, CY, lr, lg, lb, first) : -1;
    Proof(answered == 30 && n3 == 4096, "thirty more frames, each answered in its turn (%d of 30); the last one's constant is in Output, %s (%d of 4096)", answered, exactly, n3);

    // ---- frame generation's wire (stage 6), when asked
    gIm = im; gSemIn = semIn; gSemOut = semOut;
    if (fgMode >= 7 && fgMode <= 9) WaitRun(fgMode);
    else if (fgMode) FgRun(fgMode, fgStamp);

    // ---- the end: the game closes the pipe, the helper ends by itself
    CloseHandle(gPipe);
    const bool ended = WaitForSingleObject(helper.hProcess, 20000) == WAIT_OBJECT_0;
    DWORD ec = 99; GetExitCodeProcess(helper.hProcess, &ec);
    if (!ended) TerminateProcess(helper.hProcess, 9);
    if (fgMode == 6) Proof(ended && ec == 7, "the helper is gone, as its lever ended it (exit code %lu)", (unsigned long)ec);
    else if (gHelperEndedHere) Proof(ended, "the helper is gone, as this run ended it (it wrote no closing line and signalled nothing at its exit)");
    else if (gExitWant) Proof(ended && ec == gExitWant, "the helper ended by itself with the exit code this run is for (%lu; %lu wanted: a removed device)", (unsigned long)ec, (unsigned long)gExitWant);
    else Proof(ended && ec == 0, "the helper ended by itself within 20 s of the pipe closing (exit code %lu)", (unsigned long)ec);
    if (gSet.semPrep != VK_NULL_HANDLE && !gHelperEndedHere)
    {   // frame generation's fences were made in this run: the helper leaves the two it answers on at the top
        const uint64_t p = FeedVkTimelineValue(&gVk, gSet.semPrep), o = FeedVkTimelineValue(&gVk, gSet.semOut);
        Proof(ended && p == UINT64_MAX && o == UINT64_MAX, "at its exit the helper left fgPrep and fgOut at the top (0x%llX, 0x%llX): no wait of the game's is left on a helper that is gone",
              (unsigned long long)p, (unsigned long long)o);
    }
    Say("%d proofs passed, %d failed", gPasses, gFails);
    Say(gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
