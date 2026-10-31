// =====================================================================================================================
// THE FREEZE CASE: --fake-fg=30 (2026-10-06; docs/reports/fg-2/freeze-repro/REPORT.md). ALL OF THIS MODE IS IN THIS FILE.
// Its only lines in fgpipe.cpp are the include above main and the first lines of main.
//
// WHY. On 2026-10-06 the owner's legs p2h (the helper held 8 s in a prepare by --fghold_q) and p2z (the helper suspended)
// FROZE THE GAME FOR GOOD: the watch ended the helper after 4 s as designed, the queue ran again, one Present that had
// blocked for 4.4 s returned, and the next Present never did (DXVK's Present asleep on a condition variable, a thread
// that DXVK started asleep inside the NVIDIA driver, the render thread spinning on DXVK's device lock). Modes 7, 8 and 9
// of this program prove the queued wait and its release on real fences, but NONE OF THEM PRESENTS while the wait is
// queued, and they submit on a Vulkan device of their own. This mode is the game as far as the queue and the swap chain
// see it:
//   - A REAL D3D9 DEVICE THROUGH STOCK DXVK (the same d3d9.dll the game runs as vulkan.dll, loaded by its path), a visible
//     borderless window, a windowed swap chain with BackBufferCount 2 (three images), D3DCREATE_MULTITHREADED as the game
//     asks (so DXVK's device lock exists), the present interval the run says.
//   - THE HELPER'S FENCES AND IMAGES ARE IMPORTED INTO DXVK'S OWN VkDevice by the mod's import code, and every raw submit
//     goes onto DXVK's own queue under its submission lock (ID3D9VkInteropDevice), as every raw submit of the mod's does.
//     No second Vulkan device exists in this process.
//   - A PRESENTING THREAD plays the game's render thread and its worker in one: per frame the scene's D3D9 calls (a clear),
//     FlushRenderingCommands, the wait-only batch behind fgPrep when the last prepare has not retired (FgxPrepWait, the
//     client's own line), the injection's body (a real command buffer, the frame's in-fence in its submit), the frame
//     message, the same-frame home (a batch that waits on the frame's out-fence), 'Q', the two copies with fgIn, 'G',
//     DXVK's Present, and the frame-end marker (an empty submit under the lock after every Present). A limiter holds it
//     at 59 frames a second, as FusionFix held the game.
//   - A WATCH THREAD does what the mod's watch does when its time is up, IN THE SAME ORDER (fg_client.h, FgxWatchFire):
//     TerminateProcess on the helper, a wait of up to 2 s for it, the CPU signal on the frame's out-fence, FgxPrepRelease,
//     the flag; then both signals again twice a second. The presenting thread turns the flag into the transport's stop
//     at its next frame, as IvtTick does, and goes on presenting plain frames.
//   - THE MAIN THREAD owns the window, pumps its messages and is the watchdog. After the threads start it never calls
//     Direct3D or Vulkan: it reads what the other two wrote and the clock.
// WHAT IT MEASURES: how many Present calls returned before the hold, during it and after the release, how long the
// longest took, and whether the presenting thread is still inside one call a given time after the release (THE FREEZE).
// It prints the verdict as one line, "freeze-result", then "freeze-snapshot-wanted" when frozen and waits to be
// snapshotted and killed by run.py (tools\hangstacks.py reads every thread of this process).
// A FREEZE HERE IS A RESULT, NOT A FAILURE: the proofs of this mode are the instrument's own (the device is DXVK's, the
// session ran, the hold was made, the watch's order ran). FROZEN and RECOVERED are what it measures.
//
// ITS OPTIONS (each run of run.py is one combination; every other argument goes to the helper):
//     --fake-pres-dxvk=PATH      DXVK's d3d9.dll, by its full path (required)
//     --fake-pres-interval=N     1 D3DPRESENT_INTERVAL_ONE (vsync, the default), 0 D3DPRESENT_INTERVAL_IMMEDIATE
//     --fake-pres-hold=N         how the helper stops answering: 1 its own lever (it must be started with --fghold_q=AT:
//                                its serve thread sleeps 8 s before the prepare of that 'Q'; the default), 2 NtSuspendProcess
//                                at frame AT, 0 it does not stop (the helper is simply ended at frame AT: leg p2x),
//                                3 NOTHING STOPS AT ALL (a cost run: no hold, no watch, nobody ended; frame AT only marks
//                                where the counting begins, and the verdict of a session that kept presenting is RAN)
//     --fake-pres-end=N          what the watch does when its time is up: 1 its whole order, the helper ended first (the
//                                default); 2 the two signals alone, THE HELPER LEFT ALIVE (a control: a long stall and no death)
//     --fake-pres-stall=MS       the watch's time, from the hold (4400)
//     --fake-pres-fg=N           1 frame generation's wire runs and its wait is queued (the default); 0 no set, no 'Q', no
//                                'G': the only wait across the process boundary is the frame's own (the shipped path)
//     --fake-pres-at=N           the frame of the hold (300; 1 or later). AT 1 THE HELPER HAS ANSWERED LITTLE OR NOTHING: with
//                                hold=2 it is suspended BEFORE the presenting thread's first frame, so every fence still
//                                reads what it was created with; with hold=1 its lever (--fghold_q=1) sleeps before the
//                                connection's first prepare, the first frame answered and fgPrep still as created
//     --fake-pres-fps=N          the limiter (59; 0 none)
//     --fake-pres-after=MS       how long after the release the verdict is taken (10000)
//     --fake-pres-hdr=N          1 an A16B16G16R16F back buffer and scRGB, as the game's HDR sessions have (0)
//     --fake-pres-win=WxH        the window's size (the primary monitor's when absent, as the game's borderless window)
//     --fake-pres-top=N          1 the window is topmost (the default), 0 it is not
//     --fake-pres-gate=N         THE GATE BEFORE PRESENT: 0 none (the default: the mod as it was on 2026-10-06);
//                                2 WEAK, THE PRODUCT'S: THE MOD'S ONE GATE (source/transport/present_gate.h, IvtGateCore: the
//                                release's core, which frame generation calls too since the fold of 2026-10-10; until then
//                                fg_xport.h's FgxPresentGate), called where the product's worker calls it, just before DXVK's
//                                Present: a wait ON THE CPU, outside DXVK, for what batches queued before the PREVIOUS present
//                                wait for (at most one present behind an outstanding wait). Its counts in the result line are
//                                IvtGate's.
//                                CALLED BY THE PRODUCT'S OWN RULES (fg_product.h, FgP::PresentGate): only under vsync (with
//                                --fake-pres-interval=0 it does not look, and its stored needs stay 0), the out-fence's
//                                need exact in either order, and quiet after this side's transport has stopped;
//                                1 STRONG, NOT THE PRODUCT'S: test code of this file alone, kept for the comparison: the same
//                                wait for EVERY wait this side has queued (no present behind an outstanding wait at all)
//     --fake-pres-order=N        the injection's order: 0 THE SAME FRAME (the default: the frame message first, the body, then
//                                the home, a batch that waits for THIS frame's answer); 1 PIPELINED (the body's own submit
//                                waits for the answer of the frame BEFORE it, the message goes after the submit, and there
//                                is no home batch): the order every session of the game starts in
//     --fake-pres-quiet=N        1 A QUIET WINDOW: shown without being activated, the foreground never asked for, in the
//                                bottom right corner of the primary monitor (the owner's screen stays theirs); 0 the
//                                window is centred and takes the foreground, as the game's does (the default)
//     --fake-pres-marker=N       1 the frame-end marker after every Present (the default), 0 none
//     --fake-pres-home=N         1 the same-frame home's wait on the frame's out-fence (the default), 0 none
// =====================================================================================================================
#pragma once
#include <d3d9.h>
#include "../../source/dxvk/dxvk_interop_min.h"   // the mod's own transcription of DXVK's interop interfaces. By its path from
                                                  // this file: other tools (test-fg-cost, fg-replay) compile fgpipe.cpp too, with their own command lines
#pragma comment(lib, "user32.lib")

static int pQuiet = 0;   // --fake-pres-quiet (a quiet window: see the options above)
static int pGate = 0;    // --fake-pres-gate (the gate before Present: see the options above)
static IvtGate pGateSt;  // gate 2, THE MOD'S ONE GATE: its state and its counts (present_gate.h; the mod has one object for every caller, ivtGate)
static volatile bool pGateDown;   // and the flag its core is handed for "going down" (the mod hands it `dead`): raised with pQuit, THE RUN'S END, so the
                         // presenting thread can leave a hold when it is asked to. NOT this side's stop of its transport: a held present waits that out by the
                         // fences' own values (PStop and the watch signal both past everything queued), as fg_client.h's FgxB::GateDown says of the mod's
static volatile LONG pGateCalls, pGateNoLook; static LONG pGateQuick; static LONGLONG pGateTicks, pGateTicksMax;   // gate 2: its calls; the presents it did not look at
                         // (vsync off, the product's rule); the calls that did not hold, with the clock ticks spent inside them and the longest (this program's own measure
                         // of what a call costs). The holds themselves are the gate's own count now (IvtGate: every hold together and the longest, in the same clock's ticks)
static volatile LONG pGateFrames, pGateLate; static double pGateMaxMs, pGateTotalMs;   // gate 1 (this file's test code): frames it held at all; gates that gave up after 8 s
struct PCfg
{
    int interval = 1, hold = 1, end = 1, stallMs = 4400, fg = 1, at = 300, fps = 59, afterMs = 10000, hdr = 0;
    int w = 0, h = 0, top = 1, marker = 1, home = 1, order = 0;
    const char* dxvk = nullptr;
};
static PCfg pc;

// ID3D9VkExtSwapchain, transcribed from source/dxvk/d3d9_interfaces.h (the five methods after IUnknown, in its order),
// as source/hdr_encode.h transcribes it for the mod. Used only with --fake-pres-hdr=1.
struct PExtSwapchain : public IUnknown
{
    virtual BOOL    __stdcall CheckColorSpaceSupport(int) = 0;
    virtual HRESULT __stdcall SetColorSpace(int) = 0;
    virtual HRESULT __stdcall SetHDRMetaData(const void*) = 0;
    virtual HRESULT __stdcall GetCurrentOutputDesc(float*) = 0;
    virtual void    __stdcall UnlockAdditionalFormats() = 0;
};
static const GUID kPIidExtSwapchain = { 0x13776e93, 0x4aa9, 0x430a, { 0xa4, 0xec, 0xfe, 0x9e, 0x28, 0x11, 0x81, 0xd5 } };

static IDirect3D9* pD3d; static IDirect3DDevice9* pDev; static ID3D9VkInteropDevice* pIDev; static HWND pWnd;
static VkSemaphore pLocal, pEnd;                    // this side's own timelines: every submit's, and the frame-end marker's
static uint64_t pLocalV;                            // the main thread during the setup, the presenting thread afterwards
static VkCommandPool pPool; static VkCommandBuffer pCbOnce, pCbBody, pCbP, pCbH;
static LONGLONG pQpf, pQ0;

static LONGLONG PQpc() { LARGE_INTEGER q; QueryPerformanceCounter(&q); return q.QuadPart; }
static double PMs() { return (double)(PQpc() - pQ0) * 1000.0 / (double)pQpf; }       // milliseconds since this mode began
static LONGLONG PUsOf(double ms) { const LONGLONG u = (LONGLONG)(ms * 1000.0); return u ? u : 1; }   // (0 means "not yet")
static void PDie(int rc)
{
    if (gHelper) TerminateProcess(gHelper, 9);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), (UINT)rc);    // never ExitProcess: DXVK's threads may be asleep for good, and a library's detach could wait for them
}
#define PNEED(x, what) do { if (!(x)) { Say("  FAIL  %s (%s:%d): the instrument could not go on", what, __FILE__, __LINE__); Say("FAIL"); PDie(3); } } while (0)

// ---------------------------------------------------------------- what the two threads leave for the main thread
struct PEv { double t, dur; int frame; char text[900]; };
enum { P_EVS = 512, P_PRES = 16384 };
static PEv pEv[P_EVS]; static volatile LONG pEvN;
static void PEvent(int frame, double t, double dur, const char* fmt, ...)
{
    const LONG i = _InterlockedIncrement(&pEvN) - 1;
    if (i >= P_EVS) return;
    pEv[i].t = t; pEv[i].dur = dur; pEv[i].frame = frame;
    va_list a; va_start(a, fmt); _vsnprintf_s(pEv[i].text, sizeof(pEv[i].text), _TRUNCATE, fmt, a); va_end(a);
}
struct PPres { double tb, te; int frame; HRESULT hr; };
static PPres pPres[P_PRES]; static volatile LONG pPresBegun, pPresDone;
static volatile LONGLONG pLastRetUs;                // the last Present's return, whether or not the table still had room for it
static volatile LONG pOverFailed; static double pOverLongest;   // of the presents past the table: how many failed, and the longest
enum { PS_IDLE = 0, PS_SCENE, PS_FLUSH, PS_PREPWAIT, PS_LOCK, PS_SUBMIT, PS_PIPE, PS_COPY, PS_PRESENT, PS_MARK, PS_GATE };
static const char* const kPState[] = { "nothing (between calls, or its limiter)", "the scene's D3D9 calls", "FlushRenderingCommands", "FgxPrepWait (the lock and the wait-only submit)",
                                       "LockSubmissionQueue (the injection)", "vkQueueSubmit (the injection)", "a write to the pipe", "a copy's lock and submit", "Present", "the frame-end marker (the lock and an empty submit)", "THE GATE before Present (a CPU wait of its own, outside DXVK)" };
static volatile LONG pState, pFrame, pSent, pFired, pStopped, pQuit, pWatchState, pGo;
static volatile LONGLONG pStateSince;               // QPC at the last change of pState
static volatile LONGLONG pHoldUs, pReleaseUs, pStopUs;   // microseconds since the threads' start; 0 = not yet
static volatile LONG pWaits, pWaitRefused, pAnswered, pDispatch, pNosource, pQWritten, pHoldFrame;
static uint64_t pHoldOut, pHoldPrep, pHoldFgOut;    // the helper's three answers as read at the hold (written before pHoldUs)
static void PSet(LONG s) { InterlockedExchange64(&pStateSince, PQpc()); _InterlockedExchange(&pState, s); }
struct PStep        // one call of the presenting thread: where it is while it lasts, and an event when it took long
{
    const char* what; double t0; int frame;
    PStep(const char* w, LONG s, int f) : what(w), t0(PMs()), frame(f) { PSet(s); }
    ~PStep() { const double d = PMs() - t0; PSet(PS_IDLE); if (d >= 50.0) PEvent(frame, t0, d, "%s took %.1f ms", what, d); }
};

// ---------------------------------------------------------------- the binding of this mode: DXVK's queue, under its lock
struct PresB
{
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
    // as the mod's binding (fg_client.h, FgxB::QueueSubmit): one submit on DXVK's queue, under its submission lock
    static VkResult QueueSubmit(const VkSubmitInfo* si)
    {
        pIDev->LockSubmissionQueue();
        const VkResult r = vkQueueSubmit_(gQ, 1, si, VK_NULL_HANDLE);
        pIDev->ReleaseSubmissionQueue();
        return r;
    }
    static VkResult Signal(VkSemaphore s, uint64_t v)
    {
        VkSemaphoreSignalInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO }; si.semaphore = s; si.value = v;
        return vkSignalSemaphore_(gDev, &si);
    }
    // (GateOff() stood here until the fold of 2026-10-10: the gate's core is handed pGateDown as a pointer now)
};
// One submit of this side's: an optional command buffer, an optional wait on a timeline, an optional signal of one, and
// always this side's own timeline (so that the setup can wait for a submit without ever asking the queue to go idle).
static VkResult PSubmit(VkCommandBuffer cb, VkSemaphore waitSem, uint64_t waitVal, VkSemaphore sig, uint64_t sigVal, bool lockHeld)
{
    VkSemaphore sigs[2]; uint64_t sv[2]; uint32_t ns = 0;
    if (sig != VK_NULL_HANDLE) { sigs[ns] = sig; sv[ns] = sigVal; ns++; }
    sigs[ns] = pLocal; sv[ns] = ++pLocalV; ns++;
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.signalSemaphoreValueCount = ns; tl.pSignalSemaphoreValues = sv;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.pNext = &tl;
    if (waitSem != VK_NULL_HANDLE)
    {
        tl.waitSemaphoreValueCount = 1; tl.pWaitSemaphoreValues = &waitVal;
        si.waitSemaphoreCount = 1; si.pWaitSemaphores = &waitSem; si.pWaitDstStageMask = &stage;
    }
    if (cb != VK_NULL_HANDLE) { si.commandBufferCount = 1; si.pCommandBuffers = &cb; }
    si.signalSemaphoreCount = ns; si.pSignalSemaphores = sigs;
    if (!lockHeld) pIDev->LockSubmissionQueue();
    const VkResult r = vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE);
    if (!lockHeld) pIDev->ReleaseSubmissionQueue();
    return r;
}
// The frame-end marker (DlssIvTransport.h, LatEndSignal): an empty submit that signals its own timeline with the frame's
// number, under the lock, on the thread that called Present, after it returned.
static VkResult PMarker(uint64_t n)
{
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.signalSemaphoreValueCount = 1; tl.pSignalSemaphoreValues = &n;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.pNext = &tl; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &pEnd;
    pIDev->LockSubmissionQueue();
    const VkResult r = vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE);
    pIDev->ReleaseSubmissionQueue();
    return r;
}
// The frame's out-fence signalled from the CPU past what was sent (DlssIvTransport.h, IvtSignalOutTo): one signaller at a
// time, and a value is never signalled twice.
static SRWLOCK pSigLock; static uint64_t pSignalledTo;
static VkResult PSignalOutTo(uint64_t top, bool* did)
{
    VkResult r = VK_SUCCESS; *did = false;
    AcquireSRWLockExclusive(&pSigLock);
    if (top > pSignalledTo)
    {
        VkSemaphoreSignalInfo si = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO }; si.semaphore = gSemOut; si.value = top;
        r = vkSignalSemaphore_(gDev, &si);
        if (r == VK_SUCCESS) pSignalledTo = top;
        *did = true;
    }
    ReleaseSRWLockExclusive(&pSigLock);
    return r;
}
static bool PSendF(uint64_t n)
{
    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.reset = n == 1 ? 1u : 0u; f.fm.frame_time_ms = 16.95f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
    return ::PipeWrite(&f, sizeof(f));
}

// ---------------------------------------------------------------- the transport's stop, on the presenting thread
// What IvtFail does about the queue (DlssIvTransport.h): the frame's out-fence signalled past everything sent, the
// prepare's fence released, and nothing is injected or written again. The pipe is left open (the helper of the control
// that leaves it alive must not be ended by a closed pipe); this run ends the helper itself at its end.
static void PStop(int n, const char* why)
{
    if (pStopped) return;
    const double t = PMs();
    bool did = false, did2 = false; VkResult r2 = VK_SUCCESS;
    const VkResult r = PSignalOutTo((uint64_t)(ULONG)pSent + 1, &did);
    if (pc.fg) r2 = FgxPrepRelease<PresB>(&gSet, &gWaits, &did2);
    InterlockedExchange64(&pStopUs, PUsOf(t));
    _InterlockedExchange(&pStopped, 1);
    PEvent(n, t, PMs() - t, "THE TRANSPORT STOPS on the presenting thread (%s): the frame's out-fence %s, the prepare's fence %s; from here it presents plain frames",
           why, !did ? "already released" : r == VK_SUCCESS ? "signalled from the CPU" : "COULD NOT BE SIGNALLED",
           !pc.fg ? "not in this run" : !did2 ? "had nothing to signal" : r2 == VK_SUCCESS ? "signalled from the CPU" : "COULD NOT BE SIGNALLED");
}

// ---------------------------------------------------------------- the presenting thread: the game's frame, as the queue sees it
static DWORD WINAPI PPresenter(void*)
{
    const double period = pc.fps > 0 ? 1000.0 / (double)pc.fps : 0.0;
    const HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, 0x00000002u /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */, TIMER_ALL_ACCESS);
    double next = PMs();
    uint64_t lastQ = 0, lastDispatch = 0; bool gap = false; int n = 0;
    uint64_t outWait = 0;                           // the gate: the out-fence value the last batch this side queued behind it waits for (the home's)
    while (!pGo && !pQuit) Sleep(0);                // the watch thread's word (a run that stops the helper before the first frame makes it wait)
    while (!pQuit)
    {
        // THE LIMITER (FusionFix's, on the game's render thread): the frame begins on a 1/fps grid, never in a burst after a stall
        if (period > 0.0)
        {
            next += period;
            double now = PMs();
            if (next < now) next = now;
            while (!pQuit && (now = PMs()) < next)
            {
                const double left = next - now;
                if (timer && left > 0.3) { LARGE_INTEGER due; due.QuadPart = -(LONGLONG)(left * 10000.0); SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE); WaitForSingleObject(timer, 100); }
                else if (left > 1.5) Sleep(1);
                else YieldProcessor();
            }
        }
        n++;
        _InterlockedExchange(&pFrame, (LONG)n);
        // IvtTick: the watch's flag becomes the transport's stop at the next frame
        if (pFired && !pStopped) PStop(n, "the watch fired");
        {
            PStep s("the scene's D3D9 calls", PS_SCENE, n);
            const int k = n & 255;
            pDev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(k, 255 - k, 96), 1.0f, 0);
            pDev->BeginScene(); pDev->EndScene();
        }
        if (!pStopped)
        {
            const uint64_t un = (uint64_t)(ULONG)n;
            if (n > 1 && FeedVkTimelineValue(&gVk, gSemOut) >= un - 1) _InterlockedIncrement(&pAnswered);   // the frame before was answered before this one began
            // BEFORE THE INJECTION (fg_machine.h, InjectObs and ShadowBeforeInject, chained ahead of the injection's own note):
            // when the prepare of the last 'Q' has not retired, one wait-only batch goes onto DXVK's queue behind fgPrep,
            // by the client's own line
            if (pc.fg && lastQ)
            {
                const uint64_t prep = FeedVkTimelineValue(&gVk, gSet.semPrep);
                if (prep < lastQ)
                {
                    const double t = PMs(); bool q;
                    { PStep s("FgxPrepWait (the lock and the wait-only submit)", PS_PREPWAIT, n); q = FgxPrepWait<PresB>(&gSet, &gWaits, lastQ); }
                    if (q) { const LONG c = _InterlockedIncrement(&pWaits); PEvent(n, t, PMs() - t, "the wait-only batch number %ld is queued behind fgPrep for pair %llu (fgPrep reads %llu), ahead of this frame's body", (long)c, (unsigned long long)lastQ, (unsigned long long)prep); }
                    else _InterlockedIncrement(&pWaitRefused);
                }
            }
            // THE INJECTION (DlssIvTransport.h, the same-frame order on the native path): the frame message goes out FIRST,
            // then everything D3D9 recorded so far goes to the driver (FlushRenderingCommands), then, under one hold of the
            // lock, the body with the frame's in-fence in its submit and the home, a batch that waits for THIS frame's answer
            // IN THE PIPELINED ORDER (--fake-pres-order=1; the same function's other arm, "carried"): the body's own submit
            // waits for the answer of the frame BEFORE this one, once there is one; the frame message goes out after the
            // submit, inside the lock; and there is no home batch. The order every session of the game starts in.
            VkResult ra, rb = VK_SUCCESS; bool sent = true;
            const bool pipelined = pc.order == 1, carried = pipelined && n > 1;
            if (!pipelined) { PStep s("the frame message's write", PS_PIPE, n); sent = PSendF(un); }
            { PStep s("FlushRenderingCommands (the injection)", PS_FLUSH, n); pIDev->FlushRenderingCommands(); }
            { PStep s("LockSubmissionQueue (the injection)", PS_LOCK, n); pIDev->LockSubmissionQueue(); }
            { PStep s("the body's submit", PS_SUBMIT, n); ra = PSubmit(pCbBody, carried ? gSemOut : VK_NULL_HANDLE, carried ? un - 1 : 0, gSemIn, un, true); }
            if (pipelined && ra == VK_SUCCESS) { PStep s("the frame message's write (after the submit)", PS_PIPE, n); sent = PSendF(un); }
            if (!pipelined && ra == VK_SUCCESS && sent && pc.home) { PStep s("the home's submit", PS_SUBMIT, n); rb = PSubmit(VK_NULL_HANDLE, gSemOut, un, VK_NULL_HANDLE, 0, true); }
            pIDev->ReleaseSubmissionQueue();
            if (ra == VK_SUCCESS && rb == VK_SUCCESS && sent) _InterlockedExchange(&pSent, (LONG)n);
            // what the last batch this side queued behind the out-fence waits for: this frame's number in the same-frame
            // order (the home), the frame before it in the pipelined one (0 for the first: its body carries no wait)
            if (ra == VK_SUCCESS && rb == VK_SUCCESS && sent) { if (pipelined) outWait = un - 1; else if (pc.home) outWait = un; }
            if (ra != VK_SUCCESS || rb != VK_SUCCESS) PStop(n, "a submit of the injection's was refused");
            else if (!sent) PStop(n, "the frame message could not be written: the pipe is broken");
            else if (pc.fg)
            {
                // THE HAND (fg_machine.h, ShadowHand): the last DISPATCH must be answered or this pair copies nothing; 'Q' first
                // (SINCE LEVER 2, 2026-10-10, the product writes 'Q' behind the injection's body, which in this stand-in is
                // this very place: the body's and the home's submits are made, the lock is released, and nothing of the
                // frame lies between them and the hand. The order on the pipe and on the queue is the product's in both
                // of its orders; the read of fgOut two lines down is the hand's and sits behind the write in the game.)
                bool nosource = false;
                if (lastDispatch) { if (FeedVkTimelineValue(&gVk, gSet.semOut) < 2 * lastDispatch) nosource = true; else lastDispatch = 0; }
                const uint32_t qf = !lastQ ? FEED_FG_Q_RESET_BUILD : gap ? FEED_FG_Q_RESET_GAP : 0u;
                const LONG qc = _InterlockedIncrement(&pQWritten);
                const bool held = pc.hold == 1 && qc == pc.at;
                if (held) { pHoldOut = FeedVkTimelineValue(&gVk, gSemOut); pHoldPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); pHoldFgOut = FeedVkTimelineValue(&gVk, gSet.semOut); }
                const double tq = PMs(); bool okq, okg = true;
                { PStep s("the write of 'Q'", PS_PIPE, n); okq = FgxWriteQ<PresB>(&gWaits, un, qf, 16.95f, 0.05f, 0.7853981f, 0.0f, 0.0f, nullptr); }   // (wire 2: no jitter, no camera)
                lastQ = un; gap = false;
                if (held)
                {
                    _InterlockedExchange(&pHoldFrame, (LONG)n);
                    PEvent(n, tq, 0, "THE HOLD: 'Q' %d is written, the %ldth of the connection: the helper's lever sleeps 8 s before its prepare. Before it the out-fence read %llu, fgPrep %llu, fgOut %llu",
                           n, (long)qc, (unsigned long long)pHoldOut, (unsigned long long)pHoldPrep, (unsigned long long)pHoldFgOut);
                    InterlockedExchange64(&pHoldUs, PUsOf(tq));
                }
                if (okq)
                {
                    uint32_t kind = FEED_FG_G_NOSOURCE;
                    if (!nosource)
                    {   // THE HAND'S ONE SPAN, AS THE PRODUCT MAKES IT SINCE PHASE 3'S LEVERS 3 AND 1 (fg_machine.h, ShadowSpan):
                        // ONE flush, then the lock, the copy into PRESENT and the submit, with fgIn = n IN THAT SUBMIT. (Until
                        // then: two flushes and two submits, the second copy into HUDLESS carrying fgIn.)
                        VkResult r1;
                        { PStep s("FlushRenderingCommands (the hand's one span)", PS_FLUSH, n); pIDev->FlushRenderingCommands(); }
                        { PStep s("the copy into PRESENT with fgIn (lock and submit)", PS_COPY, n); r1 = PSubmit(pCbP, VK_NULL_HANDLE, 0, gSet.semIn, un, false); }
                        if (r1 == VK_SUCCESS) kind = FEED_FG_G_DISPATCH;      // DISPATCH only when the submit that signals fgIn was taken
                    }
                    { PStep s("the write of 'G'", PS_PIPE, n); okg = FgxWriteG<PresB>(un, kind | ((qf & FEED_FG_Q_RESET_MASK) << FEED_FG_G_RESET_SHIFT), 800.0f); }   // (wire 2: the pair's 'G' carries its 'Q''s reasons, as the product's)
                    if (kind == FEED_FG_G_DISPATCH) { lastDispatch = un; _InterlockedIncrement(&pDispatch); }
                    else { gap = true; _InterlockedIncrement(&pNosource); }
                }
                if (!okq || !okg) PStop(n, "frame generation's message to the helper could not be written: the pipe is broken");
            }
        }
        // THE GATE BEFORE PRESENT, 2 (WEAK): THE MOD'S ONE GATE (present_gate.h's core), where the product's worker calls it (fg_machine.h,
        // WorkerPresent) AND BY THE PRODUCT'S OWN RULES (fg_product.h, FgP::PresentGate, which this block follows line for
        // line): just before DXVK's Present, no lock held. ONLY UNDER VSYNC: with the present interval IMMEDIATE it does
        // not look, and puts its stored needs back to 0. The out-fence's value is what the last batch this side queued
        // behind it waits for, exact in either order; the prepare fence's top is FgxWaits' own. AFTER THIS SIDE'S STOP
        // it is handed no value and no top, so from the next present there is no need and no call of the driver's.
        // A hold of a quarter second or more is said as the product says it, the same words, each fence as read after it.
        if (pGate == 2 && !pc.interval)
        {
            if (pGateSt.prevOut) pGateSt.prevOut = 0;
            if (pGateSt.prevPrep) pGateSt.prevPrep = 0;
            _InterlockedIncrement(&pGateNoLook);
        }
        else if (pGate == 2)
        {
            const uint64_t needOut = (uint64_t)(ULONG)pGateSt.prevOut, needPrep = (uint64_t)(ULONG)pGateSt.prevPrep;
            const LONG gave0 = pGateSt.gaveUp, held0 = pGateSt.held;
            const bool stopped = pStopped != 0;
            const double g0 = PMs(); DWORD ms; LONGLONG dq;
            {
                PStep s("THE GATE before Present (the mod's one gate)", PS_GATE, n);
                const LONGLONG q0 = PQpc();
                // (the core, handed what the product hands it: the out-fence and what the last batch behind it waits for, the set's prepare fence and the waits' top or 0, the flag)
                ms = IvtGateCore(&pGateSt, PresB::Vk(), gSemOut, stopped ? 0 : outWait, gSet.semPrep, (pc.fg && !stopped) ? (uint64_t)(ULONG)gWaits.prepTop : 0, &pGateDown);
                dq = PQpc() - q0;
            }
            // (what a call costs is measured over the calls that did NOT hold: a hold is the gate's work, not its price,
            // and the gate counts its holds itself)
            _InterlockedIncrement(&pGateCalls);
            if (pGateSt.held == held0) { pGateQuick++; pGateTicks += dq; if (dq > pGateTicksMax) pGateTicksMax = dq; }
            if (ms >= 250)
                PEvent(n, g0, (double)ms, "frame generation: present held %lu ms for the host (frame fence %llu of %llu, prepare fence %llu of %llu)%s", (unsigned long)ms,
                       (unsigned long long)FeedVkTimelineValue(&gVk, gSemOut), (unsigned long long)needOut,
                       (unsigned long long)(pc.fg ? FeedVkTimelineValue(&gVk, gSet.semPrep) : 0), (unsigned long long)needPrep, pGateSt.gaveUp != gave0 ? " GAVE UP" : "");
        }
        // THE GATE, 1 (STRONG): THIS FILE'S OWN TEST CODE, NOT THE PRODUCT'S. The same CPU wait, for every wait this side has
        // queued (this frame's too), so that no present at all is issued behind an outstanding one. Kept for the comparison
        // the report makes; its counts are its own (pGateFrames and the three beside it).
        if (pGate == 1)
        {
            const uint64_t prepTop = pc.fg ? (uint64_t)(ULONG)gWaits.prepTop : 0;
            const uint64_t needOut = outWait, needPrep = prepTop;
            if (!pStopped && (needOut || needPrep))
            {
                const double g0 = PMs(); bool waited = false, late = false;
                {
                    PStep s("THE GATE before Present", PS_GATE, n);
                    for (;;)
                    {
                        const bool outOk = !needOut || FeedVkTimelineValue(&gVk, gSemOut) >= needOut;
                        const bool prepOk = !needPrep || FeedVkTimelineValue(&gVk, gSet.semPrep) >= needPrep;
                        if ((outOk && prepOk) || pQuit) break;
                        if (PMs() - g0 > 8000.0) { late = true; break; }
                        waited = true;
                        if (!outOk) FeedVkWaitTimeline(&gVk, gSemOut, needOut, 2); else FeedVkWaitTimeline(&gVk, gSet.semPrep, needPrep, 2);
                    }
                }
                const double gd = PMs() - g0;
                if (waited) { _InterlockedIncrement(&pGateFrames); pGateTotalMs += gd; if (gd > pGateMaxMs) pGateMaxMs = gd; }
                if (late) { _InterlockedIncrement(&pGateLate); PEvent(n, g0, gd, "THE STRONG GATE GAVE UP after %.0f ms (the out-fence wanted %llu, fgPrep %llu): the present goes out behind the wait", gd, (unsigned long long)needOut, (unsigned long long)needPrep); }
            }
        }
        // DXVK'S PRESENT, then the frame-end marker on the same thread
        // (a run with no limiter makes more presents than the table holds: those past it are counted, with the last return,
        // the longest and how many failed, so the verdict reads the whole run)
        const LONG pi = pPresBegun;
        const double tb = PMs();
        if (pi < P_PRES) { pPres[pi].tb = tb; pPres[pi].te = 0.0; pPres[pi].frame = n; pPres[pi].hr = 0; }
        _InterlockedIncrement(&pPresBegun);
        HRESULT hr;
        { PStep s("Present", PS_PRESENT, n); hr = pDev->Present(nullptr, nullptr, nullptr, nullptr); }
        const double te = PMs();
        if (pi < P_PRES) { pPres[pi].te = te; pPres[pi].hr = hr; }
        else { if (te - tb > pOverLongest) pOverLongest = te - tb; if (FAILED(hr)) _InterlockedIncrement(&pOverFailed); }
        InterlockedExchange64(&pLastRetUs, PUsOf(te));
        _InterlockedIncrement(&pPresDone);
        if (pc.marker && !pStopped) { PStep s("the frame-end marker (lock and empty submit)", PS_MARK, n); PMarker((uint64_t)(ULONG)n); }
    }
    return 0;
}

// ---------------------------------------------------------------- the watch thread: the hold where this side makes it, then the mod's order
static DWORD WINAPI PWatch(void*)
{
    // BEFORE THE HELPER'S FIRST ANSWER OF THE CONNECTION (--fake-pres-hold=2 with --fake-pres-at=1; 2026-10-06, the review of
    // the gate): the helper is suspended BEFORE the presenting thread's first frame, so no frame is ever answered and both
    // of its fences still read what they were created with. The presenting thread waits for this thread's word (pGo); in
    // every other run it has it at once.
    const bool atStart = pc.hold == 2 && pc.at <= 1;
    if (!atStart) _InterlockedExchange(&pGo, 1);
    if (pc.hold != 1)
    {   // the hold is this side's to make, at frame AT: the helper suspended, or (no hold at all) simply ended
        while (!atStart && !pQuit && pFrame < pc.at) Sleep(1);
        if (pQuit) return 0;
        const double t = PMs(); const int f = (int)pFrame;
        if (pc.hold == 3)
        {   // A COST RUN: nothing is held, nobody is ended and no watch fires. Frame AT is only where the counting begins
            // (the "hold" and the "release" are one mark in time), and the run goes on for --fake-pres-after
            PEvent(f, t, 0, "A COST RUN: nothing stops. The counting begins here, and the session runs on for %d ms", pc.afterMs);
            _InterlockedExchange(&pHoldFrame, (LONG)f);
            InterlockedExchange64(&pHoldUs, PUsOf(t));
            InterlockedExchange64(&pReleaseUs, PUsOf(t));
            _InterlockedExchange(&pWatchState, 5);
            return 0;
        }
        if (pc.hold == 2)
        {
            // (what the three fences read at the hold, as the lever's runs say it: read before the helper is stopped)
            pHoldOut = FeedVkTimelineValue(&gVk, gSemOut); pHoldPrep = pc.fg ? FeedVkTimelineValue(&gVk, gSet.semPrep) : 0; pHoldFgOut = pc.fg ? FeedVkTimelineValue(&gVk, gSet.semOut) : 0;
            const bool ok = SuspendHelper();
            PEvent(f, t, PMs() - t, "THE HOLD: the helper is SUSPENDED%s (NtSuspendProcess %s). Before it the out-fence read %llu, fgPrep %llu, fgOut %llu",
                   atStart ? " BEFORE THE FIRST FRAME OF THE CONNECTION: it has answered nothing" : "", ok ? "ok" : "FAILED",
                   (unsigned long long)pHoldOut, (unsigned long long)pHoldPrep, (unsigned long long)pHoldFgOut);
            if (atStart) _InterlockedExchange(&pGo, 1);
        }
        else
        {   // leg p2x: nothing is held and nothing is released by a watch; the presenting thread finds the pipe broken
            _InterlockedExchange(&pWatchState, 2);
            const bool ended = TerminateProcess(gHelper, 0) != 0;
            const DWORD w = WaitForSingleObject(gHelper, 2000);
            gHelperEndedHere = true;
            PEvent(f, t, PMs() - t, "THE HELPER IS ENDED where it stands, nothing held (TerminateProcess %s, gone %s): no watch signals anything; the next write to the pipe must fail",
                   ended ? "ok" : "FAILED", w == WAIT_OBJECT_0 ? "yes" : "NOT WITHIN 2 s");
            _InterlockedExchange(&pHoldFrame, (LONG)f);
            InterlockedExchange64(&pHoldUs, PUsOf(t));
            InterlockedExchange64(&pReleaseUs, PUsOf(PMs()));
            _InterlockedExchange(&pWatchState, 5);
            return 0;
        }
        _InterlockedExchange(&pHoldFrame, (LONG)f);
        InterlockedExchange64(&pHoldUs, PUsOf(t));
    }
    else while (!pQuit && !pHoldUs) Sleep(1);
    _InterlockedExchange(&pWatchState, 1);
    const double fireAt = (double)pHoldUs / 1000.0 + (double)pc.stallMs;
    while (!pQuit && PMs() < fireAt) Sleep(1);
    if (pQuit) return 0;
    // ITS ORDER (fg_client.h, FgxWatchFire; DlssIvTransport.h, IvtHostWatch): the helper ended FIRST and waited for, the
    // frame's out-fence signalled from the CPU past what was sent, the prepare's wait released, then the flag
    const uint64_t want = (uint64_t)(ULONG)pSent;
    const uint64_t prep0 = pc.fg ? FeedVkTimelineValue(&gVk, gSet.semPrep) : 0, in0 = FeedVkTimelineValue(&gVk, gSemIn), out0 = FeedVkTimelineValue(&gVk, gSemOut);
    const double tk = PMs(); bool ended = false; DWORD w = WAIT_FAILED;
    _InterlockedExchange(&pWatchState, 2);
    if (pc.end == 1) { ended = TerminateProcess(gHelper, 0) != 0; w = WaitForSingleObject(gHelper, 2000); gHelperEndedHere = true; }
    const double td = PMs();
    _InterlockedExchange(&pWatchState, 3);
    bool did = false, did2 = false; VkResult r2 = VK_SUCCESS;
    const VkResult r = PSignalOutTo(want + 1, &did);
    const double ts = PMs();
    _InterlockedExchange(&pWatchState, 4);
    if (pc.fg) r2 = FgxPrepRelease<PresB>(&gSet, &gWaits, &did2);
    const double tr = PMs();
    _InterlockedExchange(&pFired, 1);
    InterlockedExchange64(&pReleaseUs, PUsOf(tr));
    _InterlockedExchange(&pWatchState, 5);
    PEvent((int)pFrame, tk, tr - tk, "THE WATCH'S ORDER, %.0f ms after the hold (frame %llu sent, the last 'Q' %ld, the lowest pair a wait stands for %ld, wait-only batches queued %ld; before it fgPrep read %llu, the in-fence %llu, the out-fence %llu):"
           " %s; the frame's out-fence signalled to %llu from the CPU: %s in %.1f ms; FgxPrepRelease: %s in %.1f ms; after it fgPrep reads 0x%llX and the out-fence 0x%llX",
           tk - (double)pHoldUs / 1000.0, (unsigned long long)want, (long)gWaits.lastQ, (long)gWaits.waitLow, (long)pWaits, (unsigned long long)prep0, (unsigned long long)in0, (unsigned long long)out0,
           pc.end != 1 ? "THE HELPER IS LEFT ALIVE (this run's control)" : !ended ? "TerminateProcess FAILED" : w == WAIT_OBJECT_0 ? "the helper ended and gone" : "the helper ended, NOT GONE WITHIN 2 s",
           (unsigned long long)(want + 1), !did ? "already released" : r == VK_SUCCESS ? "ok" : "FAILED", ts - td,
           !pc.fg ? "not in this run" : !did2 ? "it had nothing to signal (the fence already stood at or past the top)" : r2 == VK_SUCCESS ? "signalled to one past the top" : "ITS SIGNAL FAILED", tr - ts,
           (unsigned long long)(pc.fg ? FeedVkTimelineValue(&gVk, gSet.semPrep) : 0), (unsigned long long)FeedVkTimelineValue(&gVk, gSemOut));
    // it keeps signalling twice a second until the transport has stopped, because one more frame may have been sent
    while (!pQuit && !pStopped)
    {
        Sleep(500);
        bool d = false;
        PSignalOutTo((uint64_t)(ULONG)pSent + 1, &d);
        if (pc.fg) FgxPrepRelease<PresB>(&gSet, &gWaits, &d);
    }
    return 0;
}

// ---------------------------------------------------------------- the engine name, as the mod gives it to the driver
// THE GAME DOES NOT RUN DXVK UNDER THE NAME "DXVK" (source/DLSS-IV.cpp, HdrInstallEngineNameHook): NVIDIA's Vulkan driver
// keys its present path on VkApplicationInfo::pEngineName. For exactly "DXVK" a window is presented on the driver's own
// composited path; under the mod's default on NVIDIA, "DXVK_NvRemix", it is presented layered on DXGI (independent flip,
// G-Sync in a borderless window, HDR). Which path presents is the heart of this case, so this program renames DXVK's
// instance by the mod's own method: vulkan-1.dll's vkGetInstanceProcAddr export is rewritten in memory BEFORE DXVK is
// loaded, DXVK resolves it with GetProcAddress and asks it for vkCreateInstance, and receives the function below.
// Nothing on disk is touched. --fake-pres-engine=DXVK leaves the name alone (a control: the driver's other path).
static PFN_vkGetInstanceProcAddr pOrigGipa; static PFN_vkCreateInstance pOrigCreateInstance;
static const char* pEngine = "DXVK_NvRemix"; static int pRenamed; static char pEngineWas[64];
static VkResult VKAPI_CALL PCreateInstance(const VkInstanceCreateInfo* ci, const VkAllocationCallbacks* a, VkInstance* out)
{
    if (!pOrigCreateInstance) return VK_ERROR_INITIALIZATION_FAILED;
    if (!ci || !ci->pApplicationInfo) return pOrigCreateInstance(ci, a, out);
    VkApplicationInfo app = *ci->pApplicationInfo;
    strncpy_s(pEngineWas, app.pEngineName ? app.pEngineName : "(none)", _TRUNCATE);
    app.pEngineName = pEngine;
    VkInstanceCreateInfo ci2 = *ci; ci2.pApplicationInfo = &app;
    const VkResult r = pOrigCreateInstance(&ci2, a, out);
    if (r == VK_SUCCESS) pRenamed++;
    return r;
}
// THE SPY (--fake-pres-spy=1; off unless asked, so that the plain runs have nothing of this program's between DXVK and the
// driver). The same patched export hands DXVK this program's vkGetDeviceProcAddr, which wraps three of the driver's
// functions and passes every other one through untouched: vkQueuePresentKHR (with the present id DXVK gave it),
// vkWaitForPresentKHR and vkAcquireNextImageKHR. Each call is recorded with its thread, when it was made, when it
// returned and what it returned. It turns "a thread DXVK started sleeps inside the driver" into "this call, for this
// present id, made then, never returned".
static bool pRename; static int pSpy;
static PFN_vkGetDeviceProcAddr pOrigGdpa;
static PFN_vkQueuePresentKHR pOrigQueuePresent; static PFN_vkAcquireNextImageKHR pOrigAcquire;
typedef VkResult (VKAPI_PTR* PWaitForPresent_t)(VkDevice, VkSwapchainKHR, uint64_t, uint64_t);
static PWaitForPresent_t pOrigWaitForPresent;
struct PSpyRec { uint64_t id; double tb, te; LONG r; DWORD tid; };
static PSpyRec pSpyQ[P_PRES], pSpyW[P_PRES], pSpyA[P_PRES]; static volatile LONG pSpyQn, pSpyWn, pSpyAn;
static PSpyRec* PSpyBegin(PSpyRec* ring, volatile LONG* n, uint64_t id)
{
    const LONG i = _InterlockedIncrement(n) - 1;
    if (i >= P_PRES) return nullptr;
    ring[i].id = id; ring[i].te = 0.0; ring[i].r = 0x7FFFFFFF; ring[i].tid = GetCurrentThreadId(); ring[i].tb = PMs();
    return &ring[i];
}
static VkResult VKAPI_CALL PSpyQueuePresent(VkQueue q, const VkPresentInfoKHR* info)
{
    uint64_t id = 0;
    for (const VkBaseInStructure* p = info ? (const VkBaseInStructure*)info->pNext : nullptr; p; p = p->pNext)
        if (p->sType == VK_STRUCTURE_TYPE_PRESENT_ID_KHR) { const VkPresentIdKHR* pid = (const VkPresentIdKHR*)p; if (pid->swapchainCount && pid->pPresentIds) id = pid->pPresentIds[0]; }
    PSpyRec* const rec = PSpyBegin(pSpyQ, &pSpyQn, id);
    const VkResult r = pOrigQueuePresent(q, info);
    if (rec) { rec->r = (LONG)r; rec->te = PMs(); }
    return r;
}
static VkResult VKAPI_CALL PSpyWaitForPresent(VkDevice dev, VkSwapchainKHR sc, uint64_t id, uint64_t timeout)
{
    PSpyRec* const rec = PSpyBegin(pSpyW, &pSpyWn, id);
    const VkResult r = pOrigWaitForPresent(dev, sc, id, timeout);
    if (rec) { rec->r = (LONG)r; rec->te = PMs(); }
    return r;
}
static VkResult VKAPI_CALL PSpyAcquire(VkDevice dev, VkSwapchainKHR sc, uint64_t timeout, VkSemaphore sem, VkFence fence, uint32_t* index)
{
    PSpyRec* const rec = PSpyBegin(pSpyA, &pSpyAn, 0);
    const VkResult r = pOrigAcquire(dev, sc, timeout, sem, fence, index);
    if (rec) { rec->id = index ? *index : 0; rec->r = (LONG)r; rec->te = PMs(); }
    return r;
}
static PFN_vkVoidFunction VKAPI_CALL PGetDeviceProcAddr(VkDevice dev, const char* name)
{
    const PFN_vkVoidFunction f = pOrigGdpa(dev, name);
    if (!f || !name) return f;
    if (!strcmp(name, "vkQueuePresentKHR"))     { pOrigQueuePresent = (PFN_vkQueuePresentKHR)f; return (PFN_vkVoidFunction)&PSpyQueuePresent; }
    if (!strcmp(name, "vkWaitForPresentKHR"))   { pOrigWaitForPresent = (PWaitForPresent_t)f; return (PFN_vkVoidFunction)&PSpyWaitForPresent; }
    if (!strcmp(name, "vkAcquireNextImageKHR")) { pOrigAcquire = (PFN_vkAcquireNextImageKHR)f; return (PFN_vkVoidFunction)&PSpyAcquire; }
    return f;
}
static PFN_vkVoidFunction VKAPI_CALL PGetInstanceProcAddr(VkInstance inst, const char* name)
{
    if (pRename && name && !strcmp(name, "vkCreateInstance"))
    {
        if (!pOrigCreateInstance) pOrigCreateInstance = (PFN_vkCreateInstance)pOrigGipa(inst, name);
        return (PFN_vkVoidFunction)&PCreateInstance;
    }
    if (pSpy && name && !strcmp(name, "vkGetDeviceProcAddr"))
    {
        const PFN_vkGetDeviceProcAddr f = (PFN_vkGetDeviceProcAddr)pOrigGipa(inst, name);
        if (!f) return nullptr;
        pOrigGdpa = f;
        return (PFN_vkVoidFunction)&PGetDeviceProcAddr;
    }
    return pOrigGipa(inst, name);
}
static bool PPatchExport(HMODULE mod, const char* name, void* fn)      // one entry of a loaded module's export address table, in memory
{
    BYTE* base = (BYTE*)mod;
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    const IMAGE_DATA_DIRECTORY& dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dd.VirtualAddress) return false;
    const IMAGE_EXPORT_DIRECTORY* ex = (const IMAGE_EXPORT_DIRECTORY*)(base + dd.VirtualAddress);
    const DWORD* names = (const DWORD*)(base + ex->AddressOfNames);
    const WORD* ords = (const WORD*)(base + ex->AddressOfNameOrdinals);
    DWORD* funcs = (DWORD*)(base + ex->AddressOfFunctions);
    for (DWORD i = 0; i < ex->NumberOfNames; ++i)
    {
        if (strcmp((const char*)(base + names[i]), name) != 0) continue;
        DWORD* slot = &funcs[ords[i]]; DWORD old = 0;
        if (!VirtualProtect(slot, sizeof(DWORD), PAGE_READWRITE, &old)) return false;
        *slot = (DWORD)((BYTE*)fn - base);
        VirtualProtect(slot, sizeof(DWORD), old, &old);
        return true;
    }
    return false;
}

static LRESULT CALLBACK PWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_CLOSE) return 0;
    return DefWindowProcA(h, m, w, l);
}
static void PPump() { MSG m; while (PeekMessageA(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); } }
static bool PArg(const char* a, const char* key, int* v)
{
    const size_t n = strlen(key);
    if (strncmp(a, key, n) != 0) return false;
    *v = atoi(a + n); return true;
}
static void PClear(VkCommandBuffer cb, VkImage img, float r, float g, float b, float a)
{
    VkClearColorValue c; c.float32[0] = r; c.float32[1] = g; c.float32[2] = b; c.float32[3] = a;
    VkImageSubresourceRange rg = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdClearColorImage_(cb, img, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &rg);
}
// A command buffer recorded ONCE that may be pending many times over (SIMULTANEOUS_USE): the presenting thread submits it
// every frame and never has to wait for a ring slot, so nothing of this side's can hold it but the calls being measured.
static bool PRecordClear(VkCommandBuffer cb, VkImage img, float r, float g, float b)
{
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    if (vkBeginCommandBuffer_(cb, &bi) != VK_SUCCESS) return false;
    PClear(cb, img, r, g, b, 1.0f);
    return vkEndCommandBuffer_(cb) == VK_SUCCESS;
}
// A submit of the setup's, on the main thread before the two threads exist: one command buffer, and the wait for it on
// this side's own timeline (never vkQueueWaitIdle: the queue is DXVK's).
static bool POnceSubmit()
{
    if (vkEndCommandBuffer_(pCbOnce) != VK_SUCCESS) return false;
    if (PSubmit(pCbOnce, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0, false) != VK_SUCCESS) return false;
    return FeedVkWaitTimeline(&gVk, pLocal, pLocalV, 10000);
}
static bool POnceBegin()
{
    vkResetCommandBuffer_(pCbOnce, 0);
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    return vkBeginCommandBuffer_(pCbOnce, &bi) == VK_SUCCESS;
}

static int PresMain(int argc, char** argv)
{
    Say("the fake client: %u-bit, pid %lu, THE FREEZE CASE (--fake-fg=30)", (unsigned)(8 * sizeof(void*)), (unsigned long)GetCurrentProcessId());
    Proof(sizeof(void*) == 4, "this program is 32-bit, as the game is");
    const DWORD me = GetCurrentProcessId();
    char cmd[4096]; int at = sprintf_s(cmd, "\"%s\" %lu --log=1 --sync=0 --logpath=\"%s\"", argv[1], (unsigned long)me, argv[2]);
    int wantFg = -1, dummy = 0;
    for (int i = 3; i < argc; i++)
    {
        const char* a = argv[i];
        if (PArg(a, "--fake-fg=", &dummy)) continue;
        if (PArg(a, "--fake-want-fg=", &wantFg)) continue;
        if (PArg(a, "--fake-pres-interval=", &pc.interval) || PArg(a, "--fake-pres-hold=", &pc.hold) || PArg(a, "--fake-pres-end=", &pc.end)
         || PArg(a, "--fake-pres-stall=", &pc.stallMs) || PArg(a, "--fake-pres-fg=", &pc.fg) || PArg(a, "--fake-pres-at=", &pc.at)
         || PArg(a, "--fake-pres-fps=", &pc.fps) || PArg(a, "--fake-pres-after=", &pc.afterMs) || PArg(a, "--fake-pres-hdr=", &pc.hdr)
         || PArg(a, "--fake-pres-top=", &pc.top) || PArg(a, "--fake-pres-quiet=", &pQuiet) || PArg(a, "--fake-pres-gate=", &pGate) || PArg(a, "--fake-pres-marker=", &pc.marker) || PArg(a, "--fake-pres-home=", &pc.home) || PArg(a, "--fake-pres-order=", &pc.order)
         || PArg(a, "--fake-pres-spy=", &pSpy)) continue;
        if (strncmp(a, "--fake-pres-win=", 16) == 0) { sscanf_s(a + 16, "%dx%d", &pc.w, &pc.h); continue; }
        if (strncmp(a, "--fake-pres-dxvk=", 17) == 0) { pc.dxvk = a + 17; continue; }
        if (strncmp(a, "--fake-pres-engine=", 19) == 0) { pEngine = a + 19; continue; }
        at += sprintf_s(cmd + at, sizeof(cmd) - at, " %s", a);
    }
    // (the hold's frame was 8 or later until the review of the gate: the floors of "the session ran as a game's" are AT less
    // eight, so a hold before frame 8 has no such floor. A hold at the first frame is the run that needs none: it asks what
    // happens when the helper has answered NOTHING yet)
    PNEED(pc.dxvk && pc.at >= 1 && pc.stallMs >= 0 && pc.afterMs >= 1000, "--fake-pres-dxvk=PATH is required, the hold's frame is 1 or later, and the times are not negative");
    Say("  freeze-config interval=%d hold=%d end=%d stall_ms=%d fg=%d at=%d fps=%d after_ms=%d hdr=%d top=%d marker=%d home=%d order=%d (%s) spy=%d engine=%s", pc.interval, pc.hold, pc.end, pc.stallMs, pc.fg, pc.at, pc.fps, pc.afterMs, pc.hdr, pc.top, pc.marker, pc.home,
        pc.order, pc.order == 1 ? "PIPELINED" : "the same frame", pSpy, pEngine);
    {
        char e1[512] = "", e2[512] = "", e3[512] = "";
        GetEnvironmentVariableA("DXVK_CONFIG", e1, sizeof(e1)); GetEnvironmentVariableA("DXVK_CONFIG_FILE", e2, sizeof(e2)); GetEnvironmentVariableA("DXVK_LOG_PATH", e3, sizeof(e3));
        Say("  DXVK's environment as this process has it: DXVK_CONFIG \"%s\", DXVK_CONFIG_FILE \"%s\", DXVK_LOG_PATH \"%s\"", e1, e2, e3);
    }

    // ---- the window: borderless, the primary monitor's size unless told, visible; the main thread owns it and pumps it
    SetProcessDPIAware();
    WNDCLASSA wc = {}; wc.lpfnWndProc = PWndProc; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "dlssiv-fgpipe-freeze"; wc.hCursor = LoadCursorA(nullptr, (LPCSTR)IDC_ARROW);
    PNEED(RegisterClassA(&wc) != 0, "RegisterClass");
    const int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    const int ww = pc.w > 0 ? pc.w : sw, wh = pc.h > 0 ? pc.h : sh;
    const int wx = pQuiet ? sw - ww - 48 : (sw - ww) / 2, wy = pQuiet ? sh - wh - 96 : (sh - wh) / 2;
    pWnd = CreateWindowExA((pc.top ? WS_EX_TOPMOST : 0) | (pQuiet ? WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW : 0), wc.lpszClassName, "DLSS-IV fake client: the freeze case", WS_POPUP, wx, wy, ww, wh, nullptr, nullptr, wc.hInstance, nullptr);
    PNEED(pWnd != nullptr, "CreateWindowEx");
    if (pQuiet) ShowWindow(pWnd, SW_SHOWNOACTIVATE); else { ShowWindow(pWnd, SW_SHOW); SetForegroundWindow(pWnd); }
    PPump();

    // ---- the engine name first, before DXVK exists in this process (see above PCreateInstance)
    QueryPerformanceFrequency((LARGE_INTEGER*)&pQpf); pQ0 = PQpc();
    const bool rename = strcmp(pEngine, "DXVK") != 0;
    pRename = rename;
    if (rename || pSpy)
    {
        const HMODULE vk1 = LoadLibraryA("vulkan-1.dll"); PNEED(vk1 != nullptr, "vulkan-1.dll (32-bit) is not loadable");
        pOrigGipa = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk1, "vkGetInstanceProcAddr");
        PNEED(pOrigGipa && PPatchExport(vk1, "vkGetInstanceProcAddr", (void*)&PGetInstanceProcAddr), "vulkan-1.dll's vkGetInstanceProcAddr export could not be patched");
    }

    // ---- Direct3D 9 through DXVK, loaded by its path; the device as the game asks for it
    const HMODULE dx = LoadLibraryA(pc.dxvk); PNEED(dx != nullptr, "DXVK's d3d9.dll did not load from the path given");
    typedef IDirect3D9* (WINAPI* Create9_t)(UINT);
    const Create9_t create9 = (Create9_t)GetProcAddress(dx, "Direct3DCreate9"); PNEED(create9 != nullptr, "Direct3DCreate9");
    pD3d = create9(D3D_SDK_VERSION); PNEED(pD3d != nullptr, "Direct3DCreate9 gave no interface");
    if (pc.hdr)
    {   // as source/hdr.h: the unlock on DXVK's interface BEFORE CreateDevice, or the FP16 back buffer is refused
        ID3D9VkExtInterfaceMin* ext = nullptr;
        PNEED(SUCCEEDED(pD3d->QueryInterface(__uuidof(ID3D9VkExtInterfaceMin), (void**)&ext)) && ext, "no ID3D9VkExtInterface: the d3d9.dll loaded is not DXVK's");
        ext->UnlockAdditionalFormats(); ext->Release();
    }
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = (UINT)ww; pp.BackBufferHeight = (UINT)wh; pp.BackBufferFormat = pc.hdr ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8;
    pp.BackBufferCount = 2; pp.MultiSampleType = D3DMULTISAMPLE_NONE; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.hDeviceWindow = pWnd; pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = FALSE; pp.PresentationInterval = pc.interval ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    HRESULT hr = pD3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &pp, &pDev);
    char dxPath[MAX_PATH] = ""; GetModuleFileNameA(dx, dxPath, MAX_PATH);
    if (!Proof(SUCCEEDED(hr) && pDev, "a D3D9 device through the d3d9.dll at %s: CreateDevice 0x%08X, %dx%d %s, windowed, BackBufferCount 2 (three images), SwapEffect DISCARD, flags 0x44 (hardware vertex processing, MULTITHREADED), present interval %s",
               dxPath, (unsigned)hr, ww, wh, pc.hdr ? "A16B16G16R16F" : "A8R8G8B8", pc.interval ? "ONE (vsync)" : "IMMEDIATE")) { Say("FAIL"); PDie(1); }
    const bool isDxvk = SUCCEEDED(pDev->QueryInterface(__uuidof(ID3D9VkInteropDevice), (void**)&pIDev)) && pIDev;
    if (!Proof(isDxvk, "the device answers ID3D9VkInteropDevice: it is DXVK's")) { Say("FAIL"); PDie(1); }
    if (rename) Proof(pRenamed >= 1 && strcmp(pEngineWas, "DXVK") == 0, "DXVK's Vulkan instance carries the engine name \"%s\" for the driver, as the game's does by the mod's own hook (%d instance(s) renamed from \"%s\")", pEngine, pRenamed, pEngineWas);
    else Say("  (this run leaves DXVK's engine name alone: \"DXVK\", the driver's other present path)");
    if (pSpy) Proof(pOrigQueuePresent && pOrigWaitForPresent && pOrigAcquire, "THE SPY is between DXVK and the driver for vkQueuePresentKHR, vkWaitForPresentKHR and vkAcquireNextImageKHR (DXVK asked this program's vkGetDeviceProcAddr for all three)");
    pIDev->GetVulkanHandles(&gInst, &gPhys, &gDev);
    uint32_t qIndex = 0; pIDev->GetSubmissionQueue(&gQ, &qIndex, &gFam);
    const char* miss = nullptr;
    const bool loaded = gDev && gQ && FeedVkLoad(&gVk, gDev, &miss);
    if (!Proof(loaded && FeedVkHasTimelineQueries(&gVk), "the mod's own loader took its entry points ON DXVK'S DEVICE %p (queue %p, family %u, index %u)%s%s: stock DXVK enables the two win32 external extensions itself",
               (void*)gDev, (void*)gQ, gFam, qIndex, miss ? "; missing: " : "", miss ? miss : "")) { Say("FAIL"); PDie(1); }
    vkGetDeviceProcAddr_ = gVk.GetDeviceProcAddr;
#define PVKD(n) n##_ = (PFN_##n)vkGetDeviceProcAddr_(gDev, #n); PNEED(n##_, #n)
    PVKD(vkCreateCommandPool); PVKD(vkAllocateCommandBuffers); PVKD(vkBeginCommandBuffer); PVKD(vkEndCommandBuffer); PVKD(vkResetCommandBuffer);
    PVKD(vkQueueSubmit); PVKD(vkCmdClearColorImage); PVKD(vkSignalSemaphore);
    char devName[256] = "";
    {   // the adapter's LUID, from Vulkan and never from D3D9 (DXVK spoofs the D3D9 identity), as the mod reads it
        const HMODULE vl = GetModuleHandleA("vulkan-1.dll"); PNEED(vl != nullptr, "vulkan-1.dll is not loaded although DXVK made a device");
        const PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)GetProcAddress(vl, "vkGetInstanceProcAddr"); PNEED(gipa != nullptr, "vkGetInstanceProcAddr");
        const PFN_vkGetPhysicalDeviceProperties2 props2 = (PFN_vkGetPhysicalDeviceProperties2)gipa(gInst, "vkGetPhysicalDeviceProperties2"); PNEED(props2 != nullptr, "vkGetPhysicalDeviceProperties2");
        VkPhysicalDeviceIDProperties idp = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
        VkPhysicalDeviceProperties2 p2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }; p2.pNext = &idp;
        props2(gPhys, &p2);
        PNEED(idp.deviceLUIDValid, "the Vulkan device has no LUID");
        memcpy(gLuid, idp.deviceLUID, 8); strcpy_s(devName, p2.properties.deviceName);
    }
    Say("  Vulkan: DXVK renders on %s", devName);

    // ---- a few plain frames first: DXVK makes its swap chain at the first Present
    for (int i = 0; i < 4; i++) { pDev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(16, 16, 16), 1.0f, 0); pDev->BeginScene(); pDev->EndScene(); hr = pDev->Present(nullptr, nullptr, nullptr, nullptr); PPump(); }
    Proof(SUCCEEDED(hr), "four plain frames were presented before the session (Present 0x%08X): DXVK has made its swap chain, and its own log beside this run says which present mode it took", (unsigned)hr);
    if (pc.hdr)
    {   // as source/hdr_encode.h: scRGB on the swap chain once the presenter exists
        IDirect3DSwapChain9* sc = nullptr; PExtSwapchain* x = nullptr; HRESULT hc = E_FAIL; BOOL can = FALSE;
        if (SUCCEEDED(pDev->GetSwapChain(0, &sc)) && sc && SUCCEEDED(sc->QueryInterface(kPIidExtSwapchain, (void**)&x)) && x)
        {
            can = x->CheckColorSpaceSupport(1000104002);        // VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT
            if (can) hc = x->SetColorSpace(1000104002);
            x->Release();
        }
        if (sc) sc->Release();
        Proof(can && SUCCEEDED(hc), "HDR as the game's: the surface offers scRGB (%d) and SetColorSpace answered 0x%08X", (int)can, (unsigned)hc);
        for (int i = 0; i < 4; i++) { pDev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(16, 16, 16), 1.0f, 0); pDev->BeginScene(); pDev->EndScene(); pDev->Present(nullptr, nullptr, nullptr, nullptr); PPump(); }
    }
    Say("  the window: %dx%d at (%d, %d), %s%s, %s", ww, wh, wx, wy, pc.top ? "topmost" : "not topmost", pQuiet ? ", QUIET (never activated)" : "", GetForegroundWindow() == pWnd ? "the foreground window" : "NOT the foreground window");

    // ---- this side's pool, command buffers and timelines, on DXVK's device
    VkCommandPoolCreateInfo pci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; pci.queueFamilyIndex = gFam; pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PNEED(vkCreateCommandPool_(gDev, &pci, nullptr, &pPool) == VK_SUCCESS, "vkCreateCommandPool on DXVK's device");
    VkCommandBuffer cbs[4];
    VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = pPool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 4;
    PNEED(vkAllocateCommandBuffers_(gDev, &cai, cbs) == VK_SUCCESS, "vkAllocateCommandBuffers");
    pCbOnce = cbs[0]; pCbBody = cbs[1]; pCbP = cbs[2]; pCbH = cbs[3];
    HANDLE none = nullptr;
    PNEED(FeedVkCreateTimeline(&gVk, false, &pLocal, &none) >= 0 && pLocal != VK_NULL_HANDLE, "this side's own timeline");
    PNEED(FeedVkCreateTimeline(&gVk, false, &pEnd, &none) >= 0 && pEnd != VK_NULL_HANDLE, "the frame-end marker's timeline");

    // ---- the helper, started as the game starts it; the hello with DXVK's adapter
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION helper = {};
    PNEED(CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &helper), "the helper did not start");
    gHelper = helper.hProcess;
    Say("  freeze-helper pid=%lu", (unsigned long)helper.dwProcessId);
    char name[128]; sprintf_s(name, FEED_PIPE_FMT, (unsigned long)me);
    for (int i = 0; i < 400 && gPipe == INVALID_HANDLE_VALUE; i++)
    {
        gPipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
        if (gPipe == INVALID_HANDLE_VALUE) { if (WaitForSingleObject(helper.hProcess, 0) == WAIT_OBJECT_0) break; Sleep(50); PPump(); }
    }
    if (!Proof(gPipe != INVALID_HANDLE_VALUE, "the helper made its pipe for this pid and the connect succeeded")) { Say("FAIL"); PDie(1); }
    FeedHello hello = {}; hello.magic = FEED_IPC_MAGIC; hello.version = FEED_IPC_VERSION; hello.pid = me; hello.client_kind = FEED_CLIENT_VULKAN;
    hello.self_process = 0; memcpy(hello.luid, gLuid, 8); strcpy_s(hello.build, "unknown-source (the fake client)");
    FeedHelloAck hack = {};
    bool ok = ::PipeWrite(&hello, sizeof(hello)) && PipeRead(&hack, sizeof(hack), 30000);
    Proof(ok && hack.magic == FEED_IPC_MAGIC && hack.version == FEED_IPC_VERSION && !(hack.caps & (FEED_CAP_REFUSED | FEED_CAP_BUILD_MISMATCH)), "the hello is answered: version %u, caps 0x%llX, the session not refused", hack.version, (unsigned long long)hack.caps);
    if (wantFg >= 0)
    {
        const unsigned gotFg = ((hack.caps & FEED_CAP_FG_PROBED) ? 1u : 0u) | ((hack.caps & FEED_CAP_FG) ? 2u : 0u);
        Proof(ok && gotFg == (unsigned)wantFg, "the handshake says of frame generation what this run must (%u, it must be %d: 0 not asked, 3 asked and available)", gotFg, wantFg);
    }
    if (!ok || (pc.fg && !(hack.caps & FEED_CAP_FG))) { Say("  FAIL  the session cannot run as this case needs it"); Say("FAIL"); PDie(1); }
    PPump();

    // ---- 'B': a transport build at the back buffer's size, FP16; the imports, by the mod's code, INTO DXVK'S DEVICE
    const UINT bw = (UINT)ww, bh = (UINT)wh;
    FeedBuild b = {}; b.width = bw; b.height = bh; b.color_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT; b.output_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1; b.transport = 1; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    FeedBuildAck back = {}; const uint8_t tagB = 'B';
    ok = ::PipeWrite(&tagB, 1) && ::PipeWrite(&b, sizeof(b)) && PipeRead(&back, sizeof(back), 60000) && back.ok == 1 && back.fence_in && back.fence_out;
    for (int i = 0; i < FEED_SLOTS; i++) ok = ok && back.tex[i] != 0;
    if (!Proof(ok, "the transport build at %ux%u is answered ok with two fences and four textures", bw, bh)) { Say("FAIL"); PDie(1); }
    gSemIn = FeedVkImportFence(&gVk, (HANDLE)(uintptr_t)back.fence_in); gSemOut = FeedVkImportFence(&gVk, (HANDLE)(uintptr_t)back.fence_out);
    static Img im[FEED_SLOTS]; bool imported = gSemIn != VK_NULL_HANDLE && gSemOut != VK_NULL_HANDLE;
    const VkFormat fm[FEED_SLOTS] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT };
    for (int i = 0; i < FEED_SLOTS && imported; i++)
    {
        im[i].w = bw; im[i].h = bh;
        imported = FeedVkImportImage(&gVk, (HANDLE)(uintptr_t)back.tex[i], bw, bh, fm[i], i == FEED_OUTPUT, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, true, &im[i].img, &im[i].mem);
    }
    if (!Proof(imported, "the helper's two fences and four textures import INTO DXVK'S DEVICE by the mod's own code (FeedVkImportFence, FeedVkImportImage)")) { Say("FAIL"); PDie(1); }
    gIm = im;
    ok = POnceBegin();
    if (ok)
    {
        VkImage all[FEED_SLOTS]; for (int i = 0; i < FEED_SLOTS; i++) all[i] = im[i].img;
        FeedVkBarrierN(&gVk, pCbOnce, all, FEED_SLOTS, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        for (int i = 0; i < FEED_SLOTS; i++) PClear(pCbOnce, im[i].img, 0.0f, 0.0f, 0.0f, 0.0f);
        ok = POnceSubmit();
    }
    if (!Proof(ok, "the first layouts and a clear of the four images: one submit ON DXVK'S QUEUE under its submission lock, retired (waited on this side's own timeline, never on the queue)")) { Say("FAIL"); PDie(1); }

    // ---- frame generation's set, through the client's own lines over this mode's binding (DXVK's queue, its lock)
    if (pc.fg)
    {
        const DXGI_FORMAT pf = pc.hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;       // the presented format is the back buffer's
        // THE PRODUCT'S SET SINCE LEVER 1 (2026-10-07, phase 3): asked WITHOUT the HUD-less image, two images
        const int x = FgxAcquire<PresB>(&gSet, bw, bh, pf, bw, bh, 0, FEED_FG_BUILD_DEADLINE_MS, FEED_FG_BUILD_NO_HUDLESS);
        ok = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut && gSet.img[FEED_FG_HUDLESS] == VK_NULL_HANDLE;
        if (ok) { ok = POnceBegin(); if (ok) { FeedVkBarrierN(&gVk, pCbOnce, gSet.all, gSet.nAll, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); for (uint32_t k = 0; k < gSet.nAll; k++) PClear(pCbOnce, gSet.all[k], 0.0f, 0.0f, 0.0f, 1.0f); ok = POnceSubmit(); } }
        if (!Proof(ok, "frame generation's set is CREATED through the client's own lines on DXVK's device (status %d, AMD's create %d, the warm-up %d; %ux%u dxgi %d), its images laid out", gSet.status, gSet.ffxRc, gSet.warmRc, bw, bh, (int)pf))
        { Say("FAIL"); PDie(1); }
    }
    ok = PRecordClear(pCbBody, im[FEED_COLOR].img, 0.25f, 0.5f, 0.75f);
    if (pc.fg) ok = ok && PRecordClear(pCbP, gSet.img[FEED_FG_PRESENT], 0.25f, 0.5f, 0.75f);   // (one copy a pair: the set has no HUD-less image)
    (void)pCbH;
    PNEED(ok, "the per-frame command buffers could not be recorded");
    PPump();

    // ---- the two threads; from here the main thread calls neither Direct3D nor Vulkan
    PSet(PS_IDLE);
    const double tThreads = PMs();
    const HANDLE tp = CreateThread(nullptr, 0, PPresenter, nullptr, 0, nullptr);
    const HANDLE tw = CreateThread(nullptr, 0, PWatch, nullptr, 0, nullptr);
    PNEED(tp && tw, "the two threads did not start");
    const double holdBy = tThreads + (double)pc.at * (pc.fps > 0 ? 1000.0 / (double)pc.fps : 20.0) + 30000.0;       // the hold must have come by then
    double endAt = 0.0; bool noHold = false, noRelease = false;
    for (;;)
    {
        PPump(); Sleep(5);
        const double now = PMs();
        if (!pHoldUs && now > holdBy) { noHold = true; break; }
        if (pHoldUs && !pReleaseUs && now > (double)pHoldUs / 1000.0 + (double)pc.stallMs + 20000.0) { noRelease = true; break; }
        if (pReleaseUs && !endAt) endAt = (double)pReleaseUs / 1000.0 + (double)pc.afterMs;
        if (endAt && now >= endAt) break;
    }

    // ---- what was measured (the two threads may still run, or stand for good: everything below is read, nothing is called)
    const double now = PMs();
    const double hold = pHoldUs ? (double)pHoldUs / 1000.0 : 0.0, rel = pReleaseUs ? (double)pReleaseUs / 1000.0 : 0.0, stop = pStopUs ? (double)pStopUs / 1000.0 : 0.0;
    const LONG begun = pPresBegun, gateCalls = pGateCalls, gateNoLook = pGateNoLook, done = pPresDone;   // (the presenting thread may still run: the gate's calls are read beside the presents they go with)
    const LONG st = pState; const double since = (double)(PQpc() - InterlockedCompareExchange64(&pStateSince, 0, 0)) * 1000.0 / (double)pQpf;
    int nBefore = 0, nDuring = 0, nAfter = 0, failed = 0, longFrame = 0; double longest = 0.0, longAt = 0.0, longAfter = 0.0, lastRet = 0.0;
    for (LONG i = 0; i < done && i < P_PRES; i++)
    {
        const PPres& p = pPres[i]; const double d = p.te - p.tb;
        if (FAILED(p.hr)) failed++;
        if (!hold || p.te < hold) nBefore++; else if (!rel || p.te < rel) nDuring++; else { nAfter++; if (d > longAfter) longAfter = d; }
        if (d > longest) { longest = d; longFrame = p.frame; longAt = p.tb; }
        lastRet = p.te;
    }
    if (done > P_PRES)
    {   // the presents past the table (a run with no limiter): each returned after the last one the table holds
        nAfter += (int)(done - P_PRES); failed += (int)pOverFailed;
        if (pOverLongest > longAfter) longAfter = pOverLongest;
        if (pOverLongest > longest) { longest = pOverLongest; longFrame = -1; longAt = hold; }
    }
    if ((double)pLastRetUs / 1000.0 > lastRet) lastRet = (double)pLastRetUs / 1000.0;
    const LONG evN = pEvN < P_EVS ? pEvN : P_EVS;
    static int order[P_EVS];
    for (LONG i = 0; i < evN; i++) order[i] = (int)i;
    for (LONG i = 1; i < evN; i++) { const int k = order[i]; LONG j = i - 1; while (j >= 0 && pEv[order[j]].t > pEv[k].t) { order[j + 1] = order[j]; j--; } order[j + 1] = k; }
    for (LONG i = 0; i < evN; i++) { const PEv& e = pEv[order[i]]; Say("  freeze-event %+10.1f ms (%+9.1f from the hold), frame %d: %s", e.t, hold ? e.t - hold : 0.0, e.frame, e.text); }
    for (LONG i = 0; i < done && i < P_PRES; i++)
    {   // every Present from five before the hold to twelve after the release, and the one in flight
        const PPres& p = pPres[i];
        if (!hold || p.frame < (int)pHoldFrame - 5) continue;
        if (rel && p.tb > rel + 250.0 && p.te - p.tb < 50.0) continue;
        Say("  freeze-present frame %d: called %+9.1f ms from the hold, returned after %8.1f ms (0x%08X)", p.frame, p.tb - hold, p.te - p.tb, (unsigned)p.hr);
    }
    if (begun > done && done < P_PRES) Say("  freeze-present frame %d: called %+9.1f ms from the hold, NOT RETURNED after %.1f ms", pPres[done].frame, pPres[done].tb - hold, now - pPres[done].tb);
    if (pSpy)
    {   // the driver's three functions as DXVK called them, from just before the hold on: every call that was long, late or never returned
        auto rows = [&](const char* fn, const char* what, const PSpyRec* ring, LONG count)
        {
            const LONG c = count < P_PRES ? count : P_PRES; int open = 0;
            for (LONG i = 0; i < c; i++)
            {
                const PSpyRec& r = ring[i];
                if (r.te == 0.0) open++;
                if (!hold || r.tb < hold - 60.0) continue;
                if (r.te != 0.0 && rel && r.tb > rel + 300.0 && r.te - r.tb < 50.0) continue;
                if (r.te != 0.0) Say("  freeze-spy %s (%s %llu) on thread %lu: called %+9.1f ms from the hold, returned VkResult %ld after %8.1f ms", fn, what, (unsigned long long)r.id, (unsigned long)r.tid, r.tb - hold, (long)r.r, r.te - r.tb);
                else             Say("  freeze-spy %s (%s %llu) on thread %lu: called %+9.1f ms from the hold, NOT RETURNED after %.1f ms", fn, what, (unsigned long long)r.id, (unsigned long)r.tid, r.tb - hold, now - r.tb);
            }
            Say("  freeze-spy-total %s calls=%ld not_returned=%d", fn, (long)c, open);
        };
        rows("vkQueuePresentKHR", "present id", pSpyQ, pSpyQn);
        rows("vkWaitForPresentKHR", "present id", pSpyW, pSpyWn);
        rows("vkAcquireNextImageKHR", "image", pSpyA, pSpyAn);
    }
    const bool stuck = st != PS_IDLE && since >= 3000.0;
    const char* verdict = noHold ? "NO-HOLD" : noRelease ? "NO-RELEASE" : stuck ? "FROZEN" : (nAfter >= 30 && now - lastRet < 1000.0) ? (pc.hold == 3 ? "RAN" : "RECOVERED") : "UNCLEAR";
    static const char* const kWatch[] = { "waiting for the hold", "waiting for its time", "ending the helper and waiting for it", "inside the CPU signal on the frame's out-fence", "inside FgxPrepRelease", "done" };
    // (WITH NO LIMITER the frames are not a game's: this side runs as fast as the queue lets it, so a frame may begin before
    // the one before it was answered and a pair may find its predecessor unanswered. The two floors below are a limited
    // run's; an unlimited one says its numbers and is held to the presents alone)
    const bool paced = pc.fps > 0;
    Proof(nBefore >= pc.at - 8 && failed == 0 && (!paced || (LONG)pAnswered >= (LONG)(pc.at * 9 / 10) - 8),
          "BEFORE THE HOLD the session ran%s: %d Present calls returned (%d failed in the whole run), %ld frames were answered by the helper before the next began%s",
          paced ? " as a game's" : " (NO LIMITER: as fast as the queue allows, and the answered count is a fact here, not a floor)", nBefore, failed, (long)pAnswered, pc.fg ? "" : " (no frame generation in this run)");
    if (pc.fg) Proof(!paced || (LONG)pDispatch >= (LONG)(pc.at * 9 / 10) - 8, "and frame generation's wire ran on DXVK's queue: %ld 'Q' written, %ld pairs dispatched (both copies submitted, fgIn in the second), %ld with no source%s; at the hold the out-fence read %llu, fgPrep %llu, fgOut %llu",
                     (long)pQWritten, (long)pDispatch, (long)pNosource, paced ? "" : " (no limiter: facts, not floors)", (unsigned long long)pHoldOut, (unsigned long long)pHoldPrep, (unsigned long long)pHoldFgOut);
    if (pGate == 2 && pc.interval) Proof(gateCalls >= begun && gateCalls <= begun + 2 && gateNoLook == 0, "THE GATE IS THE MOD'S ONE GATE (present_gate.h's IvtGateCore on this mode's fences), called once before every Present: %ld call(s) for %ld Present(s) begun (read one after the other while the thread may run: the same, or the call of a frame whose Present has not begun)", (long)gateCalls, (long)begun);
    if (pGate == 2 && !pc.interval) Proof(gateCalls == 0 && gateNoLook >= begun && gateNoLook <= begun + 2 && pGateSt.held == 0 && pGateSt.prevOut == 0 && pGateSt.prevPrep == 0,
                                          "WITH VSYNC OFF THE GATE DOES NOT LOOK, as the product's does not (fg_product.h, FgP::PresentGate): %ld call(s) of the core for %ld Present(s) begun, %ld present(s) passed with no look, nothing held, and its stored needs are 0 and 0",
                                          (long)gateCalls, (long)begun, (long)gateNoLook);
    if (pc.hold == 1 && pc.fg) Proof(pWaits >= 1, "THE WAIT WAS QUEUED AS THE GAME QUEUES IT: %ld wait-only batch(es) behind fgPrep by FgxPrepWait, each with the next injection's body behind it (%ld refused)", (long)pWaits, (long)pWaitRefused);
    Proof(!noHold && (noRelease || pWatchState == 5), "the hold was made and the watch thread did its part (it is %s)", kWatch[pWatchState < 6 ? pWatchState : 5]);
    // THE GATE'S COUNTS: gate 2's are the gate's own (IvtGate: held, every hold together and the longest in its own ticks,
    // given up, unread), with this program's measure of a call that did not hold beside them; gate 1's are this file's test
    // code's (it has no "unread")
    const bool own = pGate == 2;
    const double gateUsMean = pGateQuick ? (double)pGateTicks * 1e6 / (double)pQpf / (double)pGateQuick : 0.0, gateUsMax = (double)pGateTicksMax * 1e6 / (double)pQpf;
    const double gateHoldMs = (double)InterlockedCompareExchange64(&pGateSt.sumT, 0, 0) * 1000.0 / (double)pQpf, gateHoldMaxMs = (double)InterlockedCompareExchange64(&pGateSt.maxT, 0, 0) * 1000.0 / (double)pQpf;
    Say("  freeze-result verdict=%s presents_before=%d presents_during=%d presents_after=%d longest_ms=%.1f longest_frame=%d longest_called_from_hold_ms=%.1f longest_after_ms=%.1f"
        " waits_queued=%ld hold_frame=%ld stall_ms=%.1f stop_from_release_ms=%.1f verdict_from_release_ms=%.1f in_flight=%d state=%ld state_ms=%.1f last_return_ago_ms=%.1f frames=%ld"
        " answered=%ld q_written=%ld dispatched=%ld nosource=%ld"
        " gate=%d gate_held=%ld gate_hold_total_ms=%.3f gate_hold_max_ms=%.3f gate_gave_up=%ld gate_unread=%ld gate_calls=%ld gate_no_look=%ld gate_us_mean=%.2f gate_us_max=%.1f",
        verdict, nBefore, nDuring, nAfter, longest, longFrame, longAt - hold, longAfter, (long)pWaits, (long)pHoldFrame, rel ? rel - hold : -1.0, stop && rel ? stop - rel : -1.0, rel ? now - rel : -1.0,
        (int)(begun - done), (long)st, since, now - lastRet, (long)pFrame, (long)pAnswered, (long)pQWritten, (long)pDispatch, (long)pNosource,
        pGate, (long)(own ? pGateSt.held : pGateFrames), own ? gateHoldMs : pGateTotalMs, own ? gateHoldMaxMs : pGateMaxMs, (long)(own ? pGateSt.gaveUp : pGateLate), (long)(own ? pGateSt.unread : 0),
        (long)gateCalls, (long)gateNoLook, gateUsMean, gateUsMax);
    // (the first three numbers and the last two are the ones the product's count line prints, in its words)
    if (own) Say("  THE GATE (the mod's one gate): %ld call(s), %ld present(s) not looked at (vsync off); presents held for the host %ld (%.1f ms in all, longest %.2f ms, given up %ld, unread %ld);"
                 " the %ld call(s) that did not hold took %.2f us on average and %.1f us at most by this program's clock",
                 (long)gateCalls, (long)gateNoLook, (long)pGateSt.held, gateHoldMs, gateHoldMaxMs, (long)pGateSt.gaveUp, (long)pGateSt.unread, (long)pGateQuick, gateUsMean, gateUsMax);
    Say("  THE VERDICT: %s. %d Present calls returned before the hold, %d between the hold and the release, %d after the release; the longest took %.1f ms (frame %d). %.1f s after the release the presenting thread is inside %s, and has been for %.1f ms%s",
        verdict, nBefore, nDuring, nAfter, longest, longFrame, rel ? (now - rel) / 1000.0 : 0.0, kPState[st <= PS_GATE ? st : 0], since,
        noHold ? "; THE HOLD NEVER CAME" : noRelease ? "; THE WATCH THREAD DID NOT FINISH ITS ORDER" : "");
    Say("%d proofs passed, %d failed", gPasses, gFails);
    if (stuck || noRelease || noHold)
    {   // the frozen process is left as it stands for run.py to read (tools\hangstacks.py), then it is ended by whoever comes first
        Say("  freeze-snapshot-wanted pid=%lu", (unsigned long)me);
        Say(gFails ? "FAIL" : "PASS");
        for (int i = 0; i < 9000; i++) { PPump(); Sleep(5); }
        PDie(gFails ? 1 : 0);
    }
    // ---- recovered: the presenting thread is asked to stop, the helper is ended if this run left it alive
    pGateDown = true;
    _InterlockedExchange(&pQuit, 1);
    const bool joined = WaitForSingleObject(tp, 5000) == WAIT_OBJECT_0;
    Say("  (a fact, not a proof: asked to stop, the presenting thread %s)", joined ? "returned from its loop within 5 s" : "DID NOT RETURN within 5 s");
    Say(gFails ? "FAIL" : "PASS");
    PDie(gFails ? 1 : 0);
    return 0;
}
// ====================================================================================== END OF THE FREEZE CASE (--fake-fg=30)
