// dt_fake_ring.h: the harness's fake of DXVK's swapchain ring, command stream, queue and locks, and the binding frame
// generation's machine runs against in the harness (2026-09-25, phase 1C, stage S1).
//
// WHAT THIS IS. source/fg/fg_machine.h is a template over a BINDING whose static functions are every seam to DXVK, Vulkan,
// the transport and the latency instrument. The mod's binding (fg_rig.h) calls the real ones; this file's binding,
// FgHarnessBinding, calls a model of them, so the machine's ORDER can be proved on real threads before any mod code
// exists (docs/reports/fg-1c/map-final.md, section 3 "S1"). What the rig can get wrong is almost all ordering: which image
// a copy lands in after DXVK rotated its ring, whether a raw Vulkan submit overtakes work still in DXVK's command stream,
// and what DXVK's device lock does when it is taken twice. So the fake models exactly those, and nothing about pixels:
// every image carries one DWORD, its TAG, and a copy, a fill or a present moves or reads tags.
//
// THE ONE RULE OF THIS FILE: every behaviour cites the DXVK 3.1 line it follows (DXVK 3.1's src folder,
// github.com/doitsujin/dxvk at tag v3.1). A behaviour with no line to follow is said to be the fake's own choice, with why.
// A fake that is kinder than DXVK proves nothing: the one-image hazard, the stream order and the lock's single guard slot
// are each reproduced here so that dt_fake_ring_test.cpp can show the hazard happening, not just its absence.
//
// HOW IT SITS ON THE EXISTING FAKE, WITH NO LINE OF dt_fake.h OR dt_fake_dev.h CHANGED. dt_test.cpp must go on proving
// what it proves, so this file adds a layer instead of editing one:
//   - FakeRingSC derives from dt_fake.h's FakeSC. The harness puts one in FakeDev::sc before DtInstall, so the device
//     thread hooks ITS vtable (dt_install.h:348) and dtScPresentO is FakeRingSC::Present. FakeSC's own `bb` member is
//     the ring's surface 0, the stable object GetBackBuffer(0) hands out, so FakeDev::GetBackBuffer and FakeDev's default
//     render target (FakeDev::Defaults) need no change to name the ring's back buffer.
//   - RingWrapDevice() runs AFTER DtInstall and wraps dtDx[3..118], the device slots the worker replays through and every
//     direct path calls (dt_core.h:58), in thunks that take the fake device lock, check the submission-queue rule, call
//     FakeDev's own method and then model the tag work of ColorFill (35), StretchRect (34), Clear (43) and the draws
//     (81 to 84). The game's own ColorFill and StretchRect, replayed by the worker through the real device thread, reach
//     FakeDev through these thunks and write tags into the ring's images; dt_test.cpp never calls RingWrapDevice, so it
//     sees FakeDev exactly as before.
//
// INCLUDE ORDER: devthread.h, dt_fake.h, dt_fake_dev.h, fg_machine.h, then this file. The binding calls dtScPresent
// (dt_res.h) and dtDx (dt_core.h) and returns the machine's own enums, so all four come first; it reads renderTid and
// latPresFrame, which the includer defines for the device thread anyway (dt_test.cpp's preamble, copied as it is).
//
// ONE CORRECTION TO THE MAP, READ FROM DXVK. map-final.md 2.8 and 2.9 and fg_machine.h's FgSurf comment say a D3D9 call
// naming the back buffer resolves to the image current WHEN THE COMMAND STREAM RUNS (citing d3d9_device.cpp:1233-1240).
// Those lines say the opposite: StretchRect reads the surface's texture and its image ON THE CALLING THREAD, at the call
// (1232-1233 GetCommonTexture, 1239-1240 GetImage), and the command it emits carries that image by value (1420-1429);
// ColorFill does the same (1493, 1512), and a draw or a Clear binds the framebuffer at the call (BindFramebuffer, 6695;
// the draw's test of the dirty flag, 7403), which is why the rotation marks the framebuffer dirty (d3d9_swapchain.cpp:954).
// WHEN THE TWO READINGS DIFFER (corrected 2026-09-25 after the verifier's review). PresentImage's flush (839-840) comes
// before its rotation (951-952), but that flush only HANDS the chunk to DXVK's CS thread and does not wait for it
// (Flush 6176-6179 is ExecuteFlush(false), whose one wait, 6167-6168, is only for Synchronize9On12). So the CS thread
// may run work recorded before a Present after the rotation, and then "the image current when the list runs" is not the
// image the command carries. The capture reading is DXVK's, so the fake follows it: the image is taken at the call, and
// RingStats::csResolveDiffer counts the CS items that ran after their image stopped being the current one. That count is
// a FACT, not a fault: it says where the map's reading would have been wrong. The self-test shows it at 0 when the CS
// thread keeps up with the Present and at 1 when it does not (gRing.presentCs, below).
#pragma once
#include <deque>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <utility>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

// ------------------------------------------------------------------ the counts, and the rules they hold the machine to
//
// THE VIOLATIONS MUST BE 0 in every clean harness run; the self-test makes each one happen on purpose once, to show the
// detector is live (the instrument rule: a counter nobody has seen move proves nothing).
struct RingStats
{
    // D3D9 calls made by the thread HOLDING the submission queue. DXVK's interop header: "While the submission queue is
    // locked, no D3D9 methods must be called from the locking thread, or otherwise a deadlock might occur"
    // (d3d9/d3d9_interfaces.h:185-196). The map's H14.
    volatile LONG qD3D9;
    volatile LONG qD3D9Foreign;     // D3D9 calls by OTHER threads while it was held: allowed by DXVK, counted
    volatile LONG qRecursive;       // a second LockSubmissionQueue by its holder: m_mutexQueue is a plain mutex (dxvk/dxvk_queue.cpp:115-116)
    volatile LONG rawNoQLock;       // a RecSubmit made without the queue held (the mod submits under it, map 2.7.3)
    volatile LONG recNoBegin;       // a Rec* or RecSubmit with no command buffer begun
    volatile LONG cbUnderLock;      // CbTake with the interop lock held by the caller: "every command buffer taken before the lock" (H14, O-S4)
    volatile LONG cbUntaken;        // RecBegin with no command buffer left in its span's reservation (see CbTake)
    volatile LONG deadImage;        // a raw copy that reached the queue naming a back-buffer image a Reset destroyed (see RingConfigure), or an image the defragmenter's planted move left (kRingMove)
    volatile LONG moves;            // (2026-10-24) the defragmenter's planted moves made (kRingMove): not a violation, a case's own count
    volatile LONG tlBackwards;      // a timeline signalled at or below its value: Vulkan requires a timeline signal to increase
    volatile LONG pipeReentry;      // WriteQ or WriteG entered while another write was inside (G6)
    volatile LONG badHandle;        // a raw copy named a handle that is no image of the fake's
    volatile LONG badSurface;       // a rig call named an FgSurf or FgImg out of range
    // THE HELPER'S SET AND THE QUEUE'S ORDER (2026-10-10, phase 3, stage 6; "THE HELPER'S SET" below says each rule and
    // dt_fg_shadow.h's cases FK1 to FK8 plant each one). The set's rules are live only while the wire's fake runs
    // (gRing.set.on): the rig's images are its own and none of them is the helper's.
    volatile LONG setDead;          // a raw copy that reached the queue naming an image of a set a build released
    volatile LONG setVirgin;        // a raw copy OUT of a set image nothing has written since the build that made it
    volatile LONG setAbsent;        // a raw copy naming a slot the set in hand has no image for (HUDLESS of a two-image set)
    volatile LONG setNotAcquired;   // a raw copy into or out of a set image the game had not acquired in that buffer
    volatile LONG setAcquireOrder;  // an acquire of an image already acquired, a release of one that was not, or a buffer
                                    // submitted with an image still acquired (the helper could never take it back)
    volatile LONG genBusy;          // a raw copy out of GEN while the helper was inside the dispatch that writes it
    volatile LONG genUnanswered;    // a raw copy out of GEN while fgOut did not read EXACTLY twice the pair the worker has
                                    // open: another pair's image, an answer with no frame, no answer yet, a helper gone
    volatile LONG genReset;         // a raw copy out of GEN while it held the picture of a dispatch that reset AMD's history
    volatile LONG csPastWait;       // a batch of the command stream that RAN with a wait queued before it still pending
                                    // (the gate below holds every one: only its planted leak makes this move)
    volatile LONG presentAfterRefused;   // a Present made under the interop lock by a thread one of whose submits was
                                    // refused since it took that lock: a fire that shows what it did not copy in
    // THE BACK BUFFER'S SAVE (2026-10-10, phase 3, stage 7; "THE BACK BUFFER, WRITTEN OR NOT" below; live only while a
    // merged case switches it on, gRing.back.on)
    volatile LONG backLost;         // a Present made under the interop lock (a fire's) while the back buffer held a write of
                                    // the game's since the mark, with no copy of it into SAVE recorded in that span: the
                                    // game's half-drawn frame is gone
    // FACTS, not violations
    volatile LONG inBusy;           // a raw copy INTO PRESENT or HUDLESS while the helper was inside the dispatch that reads
                                    // them (G5's reason; lever 4's key does it on purpose, behind the real queue's order)
    volatile LONG csHeld;           // batches of the command stream held behind a queued wait
    volatile LONG refusedInSpan;    // submits refused while their caller held the interop lock (inside a span or a fire)
    volatile LONG setBuilds, setAcquires, setReleases;
    volatile LONG csResolveDiffer;  // CS items whose captured back-buffer image was not the current one when they ran (see the head of the file)
    volatile LONG devContended;     // internal lock acquisitions that had to wait for another owner
    volatile LONG interopNestRelease;   // LockDevice by the interop slot's own owner: the held lock was RELEASED (d3d9_interop.cpp:248-250)
    volatile LONG latEndDropped;    // LatEnd at or below FG_TL_END's value: LatEndSignal's guard drops it (map 2.5, R2 5.2)
    volatile LONG cbUnused;         // command buffers a span reserved and never began: dropped at the span's end (see CbTake)
    volatile LONG presents, presentsFailed, blitsRun, flushes, presentFlushes, submits, samplesRaw, samplesD3D9, d3d9Calls, resets;
};

// ------------------------------------------------------------------ the model's state
// RING_IMG_MAX: every image the fake ever made is kept (a raw write into a dead one is the deadImage rule), so the pool only
// grows: each Fresh() and each Reset makes the ring's images anew. 512 held S1 to S5's cases; S6's sixteen, with their many
// fresh rigs, ran it out on 2026-09-28 (every case after read image 0 for a new one), so 4,096. (2026-10-19, HOLD2, H1: the whole run with
// FH1 to FH12 made 3,953 of the 4,096, and the merges behind it add more cases: 8,192, a vector reserved once, 24 bytes an image)
// THE D3D9 SURFACES of the rig's old shape D and THE SAMPLES' kinds and classes (fg_machine.h's until round 9's S9, when the
// machine stopped naming them): the fake's own proof (dt_fake_ring_test) and the HUD cases' spare surfaces use them.
// FG_SURF_BB resolves to the current image when DXVK's stream runs.
enum FgSurf { FG_SURF_BB = 0, FG_SURF_HOLD = 1, FG_SURF_SAVET = 2, FG_SURF_N };
enum FgSampleKind { FG_SMP_GEN = 0, FG_SMP_REAL = 1, FG_SMP_HAND = 2, FG_SMP_PRESENTED = 3, FG_SMP_RESTORE = 4, FG_SMP_STANDBY = 5, FG_SMP_N };
enum FgSampleClass { FG_CLS_LIT = 0, FG_CLS_BLACK = 1, FG_CLS_DARK = 2, FG_CLS_TAG = 3, FG_CLS_NOTAG = 4, FG_CLS_N };
enum { RING_IMG_MAX = 8192, RING_SMP_TARGETS = 16, RING_SURF_MAX = 4 };
enum { RCS_FILL = 1, RCS_COPY = 2, RCS_BLIT = 3, RCS_MOVE = 4 };
// THE DEFRAGMENTER'S MOVE (2026-10-24, the SDR menu crash; fg_hud_rule.h, FgHudSrcTake): DXVK moves a texture that is not mapped, shared
// or of a stable address to NEW STORAGE, a new VkImage, at a point of its own command stream (dxvk_image.cpp, relocateStorage and
// assignStorage), and frees the old image behind its own work alone. A case plants one by a ColorFill of the colour kRingMove on the
// surface: in the stream's order (it rides the call the worker replays), the surface's image becomes a NEW image holding the old one's
// tag, and THE OLD ONE IS DEAD: a raw copy naming it that reaches the queue is counted (deadImage) and not made, as the driver would
// fault on it. Not a ring surface (a back buffer moves at a Reset alone).
static const DWORD kRingMove = 0x0E0E0E0Eu;
enum { RING_CS_LAZY = 0, RING_CS_PRE = 1, RING_CS_ALL = 2 };
enum { RRAW_COPY = 1, RRAW_PATCH = 2, RRAW_SAMPLE = 3, RRAW_SIGNAL = 4, RRAW_ACQUIRE = 5, RRAW_RELEASE = 6 };
struct RingImg      { DWORD tag; ULONGLONG handle; bool dead; bool gone; };   // dead: a back-buffer image a Reset destroyed (RingConfigure); gone: an image of a set a build released (RingSetBuild)
struct RingCsItem   { int op, dst, src; DWORD tag; LONG seq; bool dstBb, srcBb; LONG ord; DWORD tid; };
struct RingRawOp    { int op, dst, src, tl, kind; ULONGLONG v; LONG p, f; };
struct RingPresented{ DWORD tag; DWORD tid; LONG ord; int img; };
struct RingSample   { int kind; LONG p, f; int cls; DWORD tag; bool raw; };
struct RingPendSmp  { int kind; LONG p, f; int target; LONG seq; };
struct RingPipe     { char type; LONG p; DWORD flags; DWORD tid; };
// THE GATE BEFORE DXVK'S PRESENT (2026-10-06; fg_machine.h, WorkerPresent's B::PresentGate): two logs, each entry with its
// thread and the OTHER log's length at that moment, so "one call of the binding's before every entry of the Present, on
// the same thread" can be read off them. (Not the SyncFrameLatency stand-in below, which this file has always called
// "the gate": that one holds a Present inside DXVK; this one is the binding's call ahead of it.)
struct RingEntered  { DWORD tid; LONG noted; };    // the fake Present was ENTERED: by which thread, after how many calls a binding noted
struct RingPreNote  { DWORD tid; LONG entered; };  // a binding's call before a present: on which thread, after how many entries of the Present
// THE HELPER'S SET (2026-10-10, phase 3, stage 6): what the fake knows of the three images that are the helper's memory
// (FG_IMG_PRESENT, FG_IMG_GEN, FG_IMG_HUDLESS: gRing.rigImg[0..2] while `on`), and the hooks the wire's fake gives it.
// (2026-10-18, round 3, G3: and FG_IMG_GEN2, rigImg[3], the fourth place of wire 3, where hasSlot names it)
// (round 8's deep strip: the second input set's two slots went with the true fill; slot s is the machine's image s)
enum { RING_SET_SLOTS = 4 };
static_assert(FG_IMG_GEN2 == 3 && FG_IMG_SAVE >= 4, "the set's four places are the machine's first four images (fg_machine.h, FgImg)");
static int RingSlotImg(int s) { return s; }
struct RingSetModel
{
    volatile LONG on = 0;                    // the wire's fake runs (dt_fg_shadow.h, ShFresh): the set's rules are live
    LONG          bgen = 0;                  // the set's generation: 1 at a case's first build, one more at each build
    // (2026-10-18, round 3, G3: FOUR SLOTS, the set's places of wire 3: PRESENT, GEN, HUDLESS and GEN2, the second store's, which hasSlot names
    // only for a case whose set has it; a case before dt_fg_fill.h's has no GEN2, and its three read as they did)
    LONG          wrote[RING_SET_SLOTS] = {};   // per slot: the generation its image was last written in (0: not since its build)
    volatile LONG owner[RING_SET_SLOTS] = {};   // per slot: acquired by the game in the buffer now running (0: the helper's)
    volatile LONG genN = 0;                  // the pair GEN holds the picture of (0: none of this set)
    volatile LONG genResetNow = 0;           // that dispatch reset AMD's history
    volatile LONG genBusyNow = 0;            // the helper is inside a dispatch: it writes GEN
    volatile LONG inBusyNow = 0;             // and reads PRESENT and HUDLESS (round 5, H: 1 the first set's, 2 the second's)
    LONG        (*pairNow)() = nullptr;      // the pair the worker has open on the wire, its injection's number (0: none)
    bool        (*hasSlot)(int) = nullptr;   // does the set in hand have an image for this slot
    volatile LONG atOrAbove = 0;             // A CONTROL'S: the answer read "at or above 2n" where the rule is equality
    volatile LONG allowReset = 0;            // the day the owner decides a reset dispatch's picture may be shown
};

struct FakeRingSC;
static struct RingModel
{
    bool             ready = false;
    FakeDev*         dev = nullptr;
    FakeRingSC*      sc = nullptr;
    CRITICAL_SECTION data;                   // the model's own bookkeeping: never held across a modelled lock's wait
    CRITICAL_SECTION q;                      // THE SUBMISSION QUEUE (LockSubmissionQueue)
    volatile LONG    qOwner = 0, qDepth = 0;
    // the images: the ring's, the rig's Vulkan-only ones, the rig's D3D9 surfaces', the sample targets', any other surface's
    std::vector<RingImg> img;
    std::unordered_map<const void*, int> surfImg;   // a surface that is NOT the ring's: its one image, made on first use
    // THE RING (d3d9_swapchain.h:245-255, d3d9_swapchain.cpp:1021-1022, 951-952)
    D3DSWAPEFFECT    effect = D3DSWAPEFFECT_DISCARD; UINT bbc = 2; BOOL windowed = TRUE;
    int              n = 0;                  // images in the ring
    FakeSurf*        surf[RING_SURF_MAX] = {};   // surf[0] is sc->bb, the others the fake's own
    int              under[RING_SURF_MAX] = {};  // the image under each surface NOW (D3D9Subresource::m_texture)
    int              rigImg[FG_IMG_N] = {};  // the rig's Vulkan images (FG_IMG_*; 2026-10-19, HOLD2: and the merged path's hold slots past them; round 5, H: and PRESENT2, HUDLESS2)
    FakeSurf*        rigSurf[FG_SURF_N] = {};    // the rig's D3D9 surfaces; [FG_SURF_BB] unused (it is surf[0])
    FakeSurf*        smpSurf[RING_SMP_TARGETS] = {};
    int              smpNext = 0;
    // THE COMMAND STREAM: deferred D3D9 work, run at a flush
    std::vector<RingCsItem> cs; LONG csSeq = 0, csRunSeq = 0;
    // HOW FAR DXVK'S CS THREAD GOT DURING A PRESENT (revised 2026-09-25 after the verifier's review; it was a bool,
    // presentFlushRuns, true by default). PresentImage hands two things to the CS thread: the work recorded before the
    // Present, with its own Flush (d3d9_swapchain.cpp:839-840, which is ExecuteFlush(false), d3d9_device.cpp:6132-6170,
    // FlushCsChunk at 6160 and no wait), and the blit, emitted at 881 and handed over by FlushCsChunk at 915. The calling
    // thread waits for neither, so when Present returns the CS thread may have run none of it, the first part, or both.
    // All three are DXVK; the fake cannot know which the game will meet, so the harness picks:
    //   RING_CS_LAZY (the default): nothing ran; both wait for the next flush. The latest DXVK allows, so a raw submit made
    //     after the Present overtakes the blit AND the pre-Present work, and a D3D9 sample recorded before the Present has
    //     not reached the queue until a flush (map 2.8's "a flush first").
    //   RING_CS_PRE: the pre-Present work ran inside the Present; the blit did not (the old default).
    //   RING_CS_ALL: both ran inside the Present, before the rotation: no raw submit after the Present can overtake anything.
    // The old default was the kindest of the three to a sample and the harshest to nobody, so it hid a sample read too
    // early; LAZY is the default now, and a harness case whose answer depends on this runs under all three.
    int              presentCs = 0;              // RING_CS_*
    // THE PRESENT
    LONG             ord = 0;                    // Presents that reached the blit
    std::vector<RingPresented> presented;
    std::vector<RingEntered> entered; std::vector<RingPreNote> preNoted;   // every entry of the Present; every call a binding noted ahead of one
    volatile LONG    gateArmed = 0, gateIn = 0;  // the SyncFrameLatency stand-in
    HANDLE           gateEv = nullptr;
    // THE RAW QUEUE: one command buffer being recorded at a time (the rig's are worker-only)
    std::vector<RingRawOp> cb; bool cbOpen = false; volatile LONG cbReserved = 0; volatile LONG cbFailNext = 0; volatile LONG submitFailNext = 0;
    // WHICH SUBMIT IS REFUSED (2026-10-10, phase 3, stage 6): submitFailNext refuses the next submits; with submitFailSkip
    // that many are let through first, so a case can refuse the SECOND or the THIRD submit of a fire (its restore) and
    // not only its first (its copy-in). 0, as every case before it ran.
    volatile LONG    submitFailSkip = 0;
    // THE COMMAND STREAM'S GATE (stage 6). Work in DXVK's command stream reaches the Vulkan queue when the stream runs, and
    // a queue starts its batches in order: what reaches it behind a batch that waits stands behind that wait, a
    // Present's blit like anything else (check-machine.md, R2). The fake's raw submits act at once, so a binding that
    // models a queued wait (dt_fg_shadow.h, ShB) gives this hook: called by RingRunCs with the submission queue held
    // and the list not yet run; TRUE: the hook took the list (it runs it, in order, when its wait is over). Null: the
    // stream runs at once, as it always did.
    bool           (*csGate)() = nullptr;
    RingSetModel     set;                    // THE HELPER'S SET (stage 6)
    // THE BACK BUFFER, WRITTEN OR NOT (stage 7). A fire of the merged path saves the back buffer before its write and puts
    // it back after its Present ONLY WHEN THE GAME HAS WRITTEN IT since the pair was armed (fg_machine.h, B::BackWritten:
    // the binding's answer; the product's device thread will know it from the calls it replays, and here the fake is
    // what knows). on: a merged case runs and the rule below is live. dirty: a D3D9 call that writes the back buffer was
    // made since the mark (set at the CALL, on the thread that replays it, which is the thread that fires). THE RULE:
    // a Present made under the interop lock while dirty, with no copy into SAVE recorded in that span, loses the
    // game's picture: counted (RingStats::backLost).
    struct { volatile LONG on = 0, dirty = 0; } back;
    void           (*signalInside)(int tl, ULONGLONG v) = nullptr;   // one shot: runs inside a submit's signal, before the value moves
    // THE TIMELINES
    SRWLOCK          tlLock = SRWLOCK_INIT; CONDITION_VARIABLE tlCv = CONDITION_VARIABLE_INIT;
    ULONGLONG        tl[FG_TL_N] = {};
    // THE SAMPLES
    std::vector<RingPendSmp> pend; std::deque<RingSample> done;
    DWORD            genTag = 0xFFFF00FFu, blackTag = 0xFF000000u, darkTag = 0xFF010101u, drawTag = 0;
    // THE PIPE
    volatile LONG    pipeIn = 0; std::vector<RingPipe> pipe; void (*pipeInside)() = nullptr;
} gRing;
static RingStats gRingSt;
static void* gRingDxOrig[119];               // dtDx as DtInstall left it, before RingWrapDevice
static bool  gRingWrapped = false;
static thread_local LONG tRingLastSeq = -1;  // the CS seq the calling thread's last modelled call recorded
static thread_local LONG tRingRefused = 0;   // a submit of the calling thread's was refused since it took the interop lock (stage 6)
static thread_local LONG tRingBackSaved = 0; // the calling thread recorded a copy of the back buffer into SAVE since it took the interop lock (stage 7)
static void RingBackMark()  { _InterlockedExchange(&gRing.back.dirty, 0); }
static bool RingBackDirty() { return gRing.back.dirty != 0; }
static const ULONGLONG kRingHandle0 = 0x00DC000000000000ull;

// ------------------------------------------------------------------ the internal device lock and the interop slot
//
// DXVK'S DEVICE LOCK, EXACTLY (d3d9/d3d9_multithread.h, .cpp). One owner word. AcquireLock: compare-exchange 0 to the
// caller's id, and a guard that unlocks; the OWNER'S re-entry returns an EMPTY guard that unlocks nothing and "relies
// entirely on proper scoping" (66-80); anyone else spins 2000 pauses then yields, over and over (d3d9_multithread.cpp:5-12,
// util/sync/sync_spinlock.h:21-37). The guard's move-assignment UNLOCKS what it held before taking the other's (26-32,
// 96-98). Nearly every D3D9 device method takes it on entry (e.g. d3d9_device.cpp:1219 StretchRect, 1486 ColorFill, 1905
// Clear; the exceptions are listed at RingSlotLocks), and the swapchain's Present holds it for its whole length (d3d9_swapchain.cpp:113). D3D9DeviceEx is created with it
// only for D3DCREATE_MULTITHREADED, which GTA IV sets (FakeDev::GetCreationParameters), so it is always on here.
static volatile LONG gRingOwner = 0;         // D3D9Multithread::m_owner
struct RingGuard
{
    bool held = false;
    RingGuard() {}
    explicit RingGuard(bool h) : held(h) {}
    RingGuard(RingGuard&& o) : held(o.held) { o.held = false; }
    RingGuard& operator=(RingGuard&& o) { Unlock(); held = o.held; o.held = false; return *this; }   // d3d9_multithread.h:26-32
    ~RingGuard() { Unlock(); }
    void Unlock() { if (held) { held = false; _InterlockedExchange(&gRingOwner, 0); } }               // 96-98, then 90-92
    RingGuard(const RingGuard&) = delete; RingGuard& operator=(const RingGuard&) = delete;
};
static RingGuard RingDevAcquire()
{
    const LONG me = (LONG)GetCurrentThreadId();
    const LONG was = _InterlockedCompareExchange(&gRingOwner, me, 0);
    if (was == 0) return RingGuard(true);
    if (was == me) return RingGuard();                          // the owner's re-entry: an empty guard (d3d9_multithread.h:74-77)
    _InterlockedIncrement(&gRingSt.devContended);
    for (;;)
    {
        for (int i = 1; i < 2000; i++) { if (_InterlockedCompareExchange(&gRingOwner, me, 0) == 0) return RingGuard(true); _mm_pause(); }
        SwitchToThread();                                       // dxvk::this_thread::yield (sync_spinlock.h:36)
    }
}
// THE INTEROP SLOT (d3d9/d3d9_interop.cpp:248-254). ID3D9VkInteropDevice keeps ONE guard member for the whole device, not
// one per thread (d3d9_interop.h:141, `D3D9DeviceLock m_lock`): LockDevice is `m_lock = m_device->LockDevice()` and UnlockDevice is `m_lock = D3D9DeviceLock()`. The
// right-hand side is evaluated first, so a second LockDevice by the owner gets an EMPTY guard (the owner's re-entry) and
// the assignment then unlocks the held one: the lock is RELEASED while its owner believes it holds it. A recursive or
// counting fake would hide exactly the nesting bug the machine's depth counter exists for, so this is modelled as it is.
// ONE THREAD ONLY (noted 2026-09-25 after the verifier's review). The slot's assignment is not synchronised: the move
// clears the owner word first and writes `held` after (RingGuard's operator=), so a second thread taking the slot in
// that window would find its guard overwritten. DXVK's m_lock is the same plain member with the same assignment
// (d3d9_interop.h:141, d3d9_multithread.h:26-32), so this is not the fake being kinder or harsher; it holds because the
// rig's worker is the only caller of LockDevice (fg_machine.h's Lock and Unlock). A harness case must not take the
// interop slot from two threads and read anything into what follows.
static RingGuard gRingInterop;
static void RingInteropLock()
{
    const bool ownerHeld = gRingInterop.held && gRingOwner == (LONG)GetCurrentThreadId();
    RingGuard g = RingDevAcquire();                             // evaluated first, as `m_device->LockDevice()` is
    if (ownerHeld && !g.held) _InterlockedIncrement(&gRingSt.interopNestRelease);
    gRingInterop = std::move(g);                                // then assigned: the old guard unlocks
}
static void RingInteropUnlock() { gRingInterop = RingGuard(); }
static bool RingInteropHeldByMe() { return gRingInterop.held && gRingOwner == (LONG)GetCurrentThreadId(); }

// THE SUBMISSION QUEUE. LockSubmissionQueue is lockSubmission: synchronize() (wait for DXVK's queued submissions to be
// made) then m_mutexQueue.lock() (dxvk/dxvk_device.h:688-691, dxvk_queue.cpp:93-99, 115-116). DXVK's own command lists
// reach vkQueueSubmit on its submission thread under that same mutex, so while the rig holds it DXVK's work waits and the
// rig's raw submits land in between. The fake takes gRing.q for the rig's submits AND to run the CS list (RingRunCs), so
// the order in which the two reach the images is the order they would reach the queue. Only a real LockSubmissionQueue
// (the rig's QLock, LatEndSignal's own) is a second take when its holder makes it; the fake's own ordering takes are not.
static void RingQEnter(bool lockCall)
{
    const LONG me = (LONG)GetCurrentThreadId();
    if (lockCall && gRing.qOwner == me) _InterlockedIncrement(&gRingSt.qRecursive);
    EnterCriticalSection(&gRing.q);
    gRing.qOwner = me; gRing.qDepth++;
}
static void RingQLeave()
{
    if (--gRing.qDepth == 0) gRing.qOwner = 0;
    LeaveCriticalSection(&gRing.q);
}
// Every D3D9 call the fake models goes through here first, before its lock.
static void RingD3D9Entry()
{
    _InterlockedIncrement(&gRingSt.d3d9Calls);
    const LONG q = gRing.qOwner;
    if (!q) return;
    if (q == (LONG)GetCurrentThreadId()) _InterlockedIncrement(&gRingSt.qD3D9);
    else _InterlockedIncrement(&gRingSt.qD3D9Foreign);
}

// ------------------------------------------------------------------ images and surfaces
static int RingNewImg()
{
    const int i = (int)gRing.img.size();
    if (i >= RING_IMG_MAX) { Err("the ring fake ran out of images"); return 0; }
    gRing.img.push_back({ 0u, kRingHandle0 + 0x100ull * (ULONGLONG)(i + 1) });
    return i;
}
// The image a surface names NOW (call with gRing.data held). The ring's surfaces answer the image under them this moment,
// as D3D9Subresource::GetCommonTexture does (d3d9_subresource.h:126-131 swaps m_texture under the stable object); any other
// surface has one image for life.
static int RingImgOfSurfLocked(const void* s, bool* isBb)
{
    if (isBb) *isBb = false;
    if (!s) return -1;
    for (int i = 0; i < gRing.n; i++) if (gRing.surf[i] == s) { if (isBb) *isBb = i == 0; return gRing.under[i]; }
    auto it = gRing.surfImg.find(s);
    if (it != gRing.surfImg.end()) return it->second;
    const int k = RingNewImg();
    gRing.surfImg[s] = k;
    return k;
}
// A handle names the image it was issued for for ever, dead or alive: that a dead one is used is the queue's to count
// (RecSubmit), because a copy recorded while the image lived is only wrong if it reaches the queue after the Reset.
static int RingImgOfHandleLocked(ULONGLONG h)
{
    const ULONGLONG k = (h - kRingHandle0) / 0x100ull;
    if (h <= kRingHandle0 || (h - kRingHandle0) % 0x100ull || k > gRing.img.size()) { _InterlockedIncrement(&gRingSt.badHandle); return -1; }
    return (int)k - 1;
}
static int RingCurrent() { EnterCriticalSection(&gRing.data); const int i = gRing.under[0]; LeaveCriticalSection(&gRing.data); return i; }
static int RingUnder(int s) { EnterCriticalSection(&gRing.data); const int i = (s >= 0 && s < gRing.n) ? gRing.under[s] : -1; LeaveCriticalSection(&gRing.data); return i; }
static DWORD RingTag(int img) { EnterCriticalSection(&gRing.data); const DWORD t = (img >= 0 && img < (int)gRing.img.size()) ? gRing.img[img].tag : 0xDEADDEADu; LeaveCriticalSection(&gRing.data); return t; }
static void RingSetTag(int img, DWORD t) { EnterCriticalSection(&gRing.data); if (img >= 0 && img < (int)gRing.img.size()) gRing.img[img].tag = t; LeaveCriticalSection(&gRing.data); }
static ULONGLONG RingHandle(int img) { EnterCriticalSection(&gRing.data); const ULONGLONG h = (img >= 0 && img < (int)gRing.img.size()) ? gRing.img[img].handle : 0; LeaveCriticalSection(&gRing.data); return h; }
static int RingImgOfHandle(ULONGLONG h) { EnterCriticalSection(&gRing.data); const int i = RingImgOfHandleLocked(h); LeaveCriticalSection(&gRing.data); return i; }
static int RingRig(int img) { return (img >= 0 && img < FG_IMG_N) ? gRing.rigImg[img] : (_InterlockedIncrement(&gRingSt.badSurface), -1); }

// THE CLASSES (map 2.8), BY THE SAMPLE'S KIND (revised 2026-09-25 after the verifier's review: the tag alone gave only
// TAG, BLACK or LIT, so a tag sample that missed counted as LIT and FG_CLS_DARK and FG_CLS_NOTAG could never occur). The
// harness declares three tags. The TAG KINDS (PRESENTED, RESTORE, STANDBY) look for the 16x16 magenta patch by an exact
// match (D3DTEXF_POINT, map 2.8), so they read TAG or NOTAG and nothing else: fault 16's hit is a PRESENTED sample that
// reads NOTAG. The CLASSIFIER KINDS (GEN, REAL, HAND) read BLACK (the declared black: a black centre AND a dark whole
// frame), DARK (the declared dark: a black centre in a frame that is not dark, map 2.8's "a dark centre is counted"), or
// LIT; and TAG when they meet the patch itself, which is what a GEN sample reads in TAG mode and what the HANDOFF sample
// (HAND in TAG mode) looks for.
static bool RingTagKind(int kind) { return kind == FG_SMP_PRESENTED || kind == FG_SMP_RESTORE || kind == FG_SMP_STANDBY; }
static int RingClassify(int kind, DWORD tag)
{
    if (RingTagKind(kind)) return tag == gRing.genTag ? FG_CLS_TAG : FG_CLS_NOTAG;
    return tag == gRing.genTag ? FG_CLS_TAG : tag == gRing.blackTag ? FG_CLS_BLACK : tag == gRing.darkTag ? FG_CLS_DARK : FG_CLS_LIT;
}
static void RingDeclareTags(DWORD gen, DWORD black, DWORD dark = 0xFF010101u) { gRing.genTag = gen; gRing.blackTag = black; gRing.darkTag = dark; }

// ------------------------------------------------------------------ the ring's shape
//
// HOW MANY IMAGES, AND WHETHER THEY ROTATE. SwapWithFrontBuffer() is false for COPY, and false for DISCARD with one back
// buffer in a window, which DXVK's tests found behaves as COPY (d3d9_swapchain.h:245-255); true otherwise. The ring is
// BackBufferCount images plus one when it swaps with the front buffer (d3d9_swapchain.cpp:1021-1022; the other term there,
// d3d9.extraFrontbuffer, defaults to false, d3d9_options.cpp:79, and is not modelled). The rotation walks
// i = 1 .. count-1 (951-952), so one image never rotates and every larger ring does. The harness's four shapes:
// (DISCARD, 1, windowed) 1 image; (FLIP, 1) 2; (DISCARD, 2) 3, the owner's; (DISCARD, 3) 4.
static bool RingSwapsWithFront(D3DSWAPEFFECT e, UINT bbc, BOOL windowed)
{
    if (e == D3DSWAPEFFECT_COPY) return false;
    if (e == D3DSWAPEFFECT_DISCARD && bbc == 1 && windowed) return false;
    return true;
}
// A RESET'S NEW BACK BUFFERS (revised 2026-09-25 after the verifier's review: the ring had no Reset, and this stand-in
// kept every old image usable and dropped a pending blit, so a restore aimed at a pre-Reset handle landed silently and
// the map's H9 could not fail). What DXVK 3.1 does, for a non-Ex device like GTA IV's:
//   - Reset takes the device lock (d3d9_device.cpp:449) and destroys the back buffers at once (DestroyBackBuffers, 467;
//     the swapchain's CreateBackBuffers destroys them again first, d3d9_swapchain.cpp:1017-1019, via ResetSwapChain 520
//     and the swapchain's own Reset 662), then makes new ones (1021-1060). So every image is NEW and every old one is
//     GONE: a Vulkan handle queried before the Reset names a destroyed VkImage. The fake marks the old images dead, and a
//     raw copy that reaches the queue naming one is counted (RingStats::deadImage) and not applied.
//   - The new images are initialised "to black" (the comment at 1061-1062, InjectCs of initImage at 1069-1073; initImage's
//     own body was not read). The fake gives them the declared black tag.
//   - WORK ALREADY IN THE COMMAND STREAM IS NOT DROPPED. Each command carries its images by reference (StretchRect's
//     lambda, d3d9_device.cpp:1420-1429; the blit's cSrcView, d3d9_swapchain.cpp:886), and DestroyBackBuffers only lets go
//     of the surfaces' own references (984-989), so a pending copy or blit still runs, on the old image, and a blit still
//     presents it. A successful Reset then flushes and waits for the CS thread (ExecuteFlush(true) and
//     SynchronizeCsThread(SynchronizeAll), d3d9_device.cpp:529-530). The fake keeps its list; RingReset runs it at the end.
// THE FAKE'S OWN CHOICES: the surface objects stay the same (DXVK makes new D3D9Surface objects at 1048-1060; the rig asks
// for GetBackBuffer(0) fresh each time and releases HOLD, SAVE_T and the sample targets at the Reset barrier, map 2.15, so
// nothing the rig does can hold one across a Reset); the rig's own images and surfaces do not die (they are the rig's to
// release, and the fake has no reason to model that); a FAILED Reset, which in DXVK has already destroyed the back
// buffers (467 comes before both failure returns, 513-518 and 520-527), is not modelled, because FakeDev::Reset always
// succeeds. Called directly (RingInstall, and the harness's quiescent shape changes), it is the same Reset with the
// flush left to the caller.
static void RingConfigure(D3DSWAPEFFECT e, UINT bbc, BOOL windowed)
{
    EnterCriticalSection(&gRing.data);
    gRing.effect = e; gRing.bbc = bbc; gRing.windowed = windowed;
    for (int i = 0; i < gRing.n; i++) gRing.img[gRing.under[i]].dead = true;
    gRing.n = (int)bbc + (RingSwapsWithFront(e, bbc, windowed) ? 1 : 0);
    if (gRing.n < 1 || gRing.n > RING_SURF_MAX) { Err("the ring fake models 1 to %d images, not %d", RING_SURF_MAX, gRing.n); gRing.n = 1; }
    for (int i = 0; i < gRing.n; i++) { gRing.under[i] = RingNewImg(); gRing.img[gRing.under[i]].tag = gRing.blackTag; }
    LeaveCriticalSection(&gRing.data);
}
static bool RingImgDead(int img) { EnterCriticalSection(&gRing.data); const bool d = img >= 0 && img < (int)gRing.img.size() && gRing.img[img].dead; LeaveCriticalSection(&gRing.data); return d; }

// ------------------------------------------------------------------ the command stream
//
// DXVK DEFERS EVERY D3D9 CALL THAT DRAWS. The call records a command (EmitCs) that the CS thread runs later, and the work
// reaches the Vulkan queue when a command list is flushed: at FlushRenderingCommands (d3d9_interop.cpp:235-238, Flush then
// SynchronizeCsThread) and at the Present's own EndFrame and Flush (d3d9_swapchain.cpp:839-840), which dispatches without
// waiting (gRing.presentCs says how far the CS thread got). A raw Vulkan submit made in between reaches the queue FIRST:
// that is the 2026-09-07 ordering and the one-image hazard. The fake keeps the list and runs it at a flush, and at the
// Present as far as gRing.presentCs says, under the queue (RingQEnter), applying each item to the images in record order.
static LONG RingCsRecord(int op, int dst, int src, DWORD tag, bool dstBb, bool srcBb, LONG ord)
{
    RingCsItem it = { op, dst, src, tag, 0, dstBb, srcBb, ord, GetCurrentThreadId() };
    if (dstBb) _InterlockedExchange(&gRing.back.dirty, 1);      // (stage 7: the game wrote the back buffer, as of this call)
    EnterCriticalSection(&gRing.data);
    it.seq = ++gRing.csSeq;
    gRing.cs.push_back(it);
    LeaveCriticalSection(&gRing.data);
    tRingLastSeq = it.seq;
    return it.seq;
}
// One list of the stream's work, applied in record order. Call with the submission queue and gRing.data held. (A
// function of its own since stage 6: a binding that held a list behind a queued wait runs it through here too.)
static void RingApplyCsLocked(const std::vector<RingCsItem>& list)
{
    for (const RingCsItem& it : list)
    {
        // THE RESOLUTION CHECK (the head of the file): an item that named the back buffer carries the image current at its
        // call; if that is no longer the image under surface 0, DXVK's reading and the map's would have disagreed here. An
        // item whose image a Reset destroyed is left out: DXVK destroyed the surface too, so the map's reading names nothing.
        if ((it.dstBb && !gRing.img[it.dst].dead && it.dst != gRing.under[0]) || (it.srcBb && !gRing.img[it.src].dead && it.src != gRing.under[0]))
            _InterlockedIncrement(&gRingSt.csResolveDiffer);
        switch (it.op)
        {
        case RCS_FILL: gRing.img[it.dst].tag = it.tag; break;
        case RCS_COPY: gRing.img[it.dst].tag = gRing.img[it.src].tag; break;
        case RCS_MOVE:   // (kRingMove) the surface that names it is given a new image with its tag; the old one is dead
            for (auto& kv : gRing.surfImg)
                if (kv.second == it.dst)
                {
                    const int k = RingNewImg();
                    gRing.img[k].tag = gRing.img[it.dst].tag;
                    gRing.img[it.dst].dead = true;
                    kv.second = k;
                    _InterlockedIncrement(&gRingSt.moves);
                    break;
                }
            break;
        case RCS_BLIT:   // the present blit reads the image captured at the Present, when it runs (d3d9_swapchain.cpp:848, 881-915)
            gRing.presented.push_back({ gRing.img[it.src].tag, it.tid, it.ord, it.src });
            _InterlockedIncrement(&gRingSt.blitsRun);
            break;
        }
        gRing.csRunSeq = it.seq;
    }
}
static void RingRunCs()
{
    RingQEnter(false);
    if (gRing.csGate && gRing.csGate()) { RingQLeave(); return; }   // (stage 6) the list stands behind a queued wait: its holder runs it
    EnterCriticalSection(&gRing.data);
    RingApplyCsLocked(gRing.cs);
    gRing.cs.clear();
    LeaveCriticalSection(&gRing.data);
    RingQLeave();
}

// ------------------------------------------------------------------ THE HELPER'S SET (2026-10-10, phase 3, stage 6)
//
// WHAT THE MERGED PATH WILL BE PROVED AGAINST (docs/reports/fg-3/map-2026-10-06: machine.md's machine-15, proofs-gates-
// kit.md's pgk-13, check-machine.md's X8 and R19). Until this stage the three images of the helper's set were three of
// the rig's images like any other: they lived for the whole run, anything could be copied into or out of them at any
// time, and "the generated image" held whatever PRESENT held. A machine that SHOWS the generated image can go wrong in
// ways that model could not count. Now, while the wire's fake runs (set.on):
//   THE SET DIES AT A BUILD (RingSetBuild: the game's side of a build, where its imports go). The three slots get NEW
//     images holding their sentinels, and the old ones are GONE: a raw copy that reaches the queue naming one is
//     counted (setDead) and not made, as a copy naming a back buffer a Reset destroyed is. And each slot carries THE
//     GENERATION IT WAS LAST WRITTEN IN: a copy OUT of a slot nothing has written since its build (wrote != bgen) is
//     counted (setVirgin), and made: it lands the sentinel, which is what a fire of an old set's pair would show.
//   THE SET HAS THE SLOTS IT HAS (hasSlot): a copy naming one it lacks is counted (setAbsent) and not made. The
//     product's line returns silently there; a machine that asked for it must be seen.
//   OWNERSHIP. A set image is the helper's memory: the game ACQUIRES it in the buffer that copies and RELEASES it in
//     that same buffer (fg_product.h, RecCopyToImg). The raw queue has the two as commands (RecAcquire, RecRelease). A
//     copy into or out of a set image not acquired is counted (setNotAcquired); an acquire of one already acquired, a
//     release of one that was not, and a buffer submitted with one still acquired are counted (setAcquireOrder).
//   GEN IS THE HELPER'S WHILE IT DISPATCHES (genBusyNow, set by the fake helper from the start of a dispatch to just
//     before its answer): a copy out of GEN meanwhile is counted (genBusy). A copy INTO an input meanwhile is a FACT
//     (inBusy): it is what G5 forbids, and what lever 4's key does behind the real queue's order, which this fake
//     does not model.
//   THE ANSWER IS READ BY EQUALITY (critic.md, X2). A copy out of GEN is counted (genUnanswered) unless fgOut reads
//     EXACTLY twice the pair the worker has open (pairNow): so GEN of the pair before, an answer with no frame, an
//     answer not in yet and A HELPER THAT IS GONE (its fences at the top) are each seen at the copy itself, whatever
//     a placer or a stamper believed. atOrAbove is that rule's control.
//   A RESET DISPATCH'S PICTURE IS FED, NEVER SHOWN (critic.md, X4): a copy out of GEN while it holds one is counted
//     (genReset), unless allowReset.
// Call these with gRing.data held where the name says Locked.
static int RingSetSlotOfLocked(int img)
{
    if (!gRing.set.on || img < 0) return -1;
    for (int s = 0; s < RING_SET_SLOTS; s++) if (gRing.rigImg[RingSlotImg(s)] == img) return s;
    return -1;
}
// A raw copy dst <- src, as it reaches the queue. False: the copy is not made.
static bool RingSetCopyLocked(int dst, int src)
{
    RingSetModel& m = gRing.set;
    if (!m.on) return true;
    if (gRing.img[dst].gone || gRing.img[src].gone) { _InterlockedIncrement(&gRingSt.setDead); return false; }
    const int sd = RingSetSlotOfLocked(dst), ss = RingSetSlotOfLocked(src);
    for (int k = 0; k < 2; k++)
    {
        const int s = k ? ss : sd;
        if (s < 0) continue;
        if (m.hasSlot && !m.hasSlot(s)) { _InterlockedIncrement(&gRingSt.setAbsent); return false; }
        if (!m.owner[s]) _InterlockedIncrement(&gRingSt.setNotAcquired);
    }
    if (sd >= 0)
    {
        // (a copy INTO PRESENT or HUDLESS while the helper reads them)
        if ((sd == FG_IMG_PRESENT || sd == FG_IMG_HUDLESS) && m.inBusyNow == 1) _InterlockedIncrement(&gRingSt.inBusy);
        m.wrote[sd] = m.bgen;
    }
    if (ss >= 0)
    {
        if (m.wrote[ss] != m.bgen) _InterlockedIncrement(&gRingSt.setVirgin);
        if (ss == FG_IMG_GEN || ss == FG_IMG_GEN2)                // (round 3, G3: the second store's image is a generated frame too, read by the same rules)
        {
            if (m.genBusyNow) _InterlockedIncrement(&gRingSt.genBusy);
            AcquireSRWLockShared(&gRing.tlLock); const ULONGLONG out = gRing.tl[FG_TL_OUT]; ReleaseSRWLockShared(&gRing.tlLock);
            const ULONGLONG n2 = 2ull * (ULONGLONG)(ULONG)(m.pairNow ? m.pairNow() : 0);
            if (!(n2 && (m.atOrAbove ? out >= n2 : out == n2))) _InterlockedIncrement(&gRingSt.genUnanswered);
            if (m.genResetNow && !m.allowReset) _InterlockedIncrement(&gRingSt.genReset);
        }
    }
    return true;
}
static void RingSetOwnLocked(int img, bool acquire)
{
    const int s = RingSetSlotOfLocked(img);
    if (s < 0) return;
    RingSetModel& m = gRing.set;
    if (acquire) { if (m.owner[s]) _InterlockedIncrement(&gRingSt.setAcquireOrder); m.owner[s] = 1; _InterlockedIncrement(&gRingSt.setAcquires); }
    else { if (!m.owner[s]) _InterlockedIncrement(&gRingSt.setAcquireOrder); m.owner[s] = 0; _InterlockedIncrement(&gRingSt.setReleases); }
}
// A buffer's end: an image still acquired stays the game's for ever as far as the helper knows. Counted, and given back.
static void RingSetBufferEndLocked()
{
    RingSetModel& m = gRing.set;
    if (!m.on) return;
    for (int s = 0; s < RING_SET_SLOTS; s++) if (m.owner[s]) { _InterlockedIncrement(&gRingSt.setAcquireOrder); m.owner[s] = 0; }
}
// A CASE'S FIRST SET: the rig's three images as a fresh rig left them (their sentinels), now the helper's.
static void RingSetFirst(LONG (*pairNow)(), bool (*hasSlot)(int))
{
    EnterCriticalSection(&gRing.data);
    RingSetModel& m = gRing.set;
    m.bgen = 1; m.genN = 0; m.genResetNow = 0; m.genBusyNow = 0; m.inBusyNow = 0; m.atOrAbove = 0; m.allowReset = 0;
    for (int s = 0; s < RING_SET_SLOTS; s++) { m.wrote[s] = 0; m.owner[s] = 0; }
    m.pairNow = pairNow; m.hasSlot = hasSlot;
    m.on = 1;
    LeaveCriticalSection(&gRing.data);
}
// A BUILD, ON THE GAME'S SIDE: the imports of the old set go and a new set is held. sentinel0: slot s starts at
// sentinel0 + 0x100 * s, as a fresh rig's images do.
static void RingSetBuild(DWORD sentinel0)
{
    EnterCriticalSection(&gRing.data);
    RingSetModel& m = gRing.set;
    if (m.on)
    {
        // (2026-10-19, round 3, G3 at SW's merge: THE FOURTH PLACE ONLY WHERE THE SET HAS IT. A build made four new images for every set,
        // GEN2's too where the case's set has none, and the pool, which only grows (RING_IMG_MAX), ran out in the whole run's FB5 and FQ6
        // with SW's cases in: a case whose set has no GEN2 makes the three images it always made)
        for (int s = 0; s < RING_SET_SLOTS; s++)
        {
            if (s == FG_IMG_GEN2 && !(m.hasSlot && m.hasSlot(s))) { m.wrote[s] = 0; m.owner[s] = 0; continue; }
            const int ri = RingSlotImg(s);
            gRing.img[gRing.rigImg[ri]].gone = true;
            gRing.rigImg[ri] = RingNewImg();
            gRing.img[gRing.rigImg[ri]].tag = sentinel0 + 0x100u * (DWORD)ri;   // (by the machine's number: the first four as before)
            m.wrote[s] = 0; m.owner[s] = 0;
        }
        m.bgen++; m.genN = 0; m.genResetNow = 0;
        _InterlockedIncrement(&gRingSt.setBuilds);
    }
    LeaveCriticalSection(&gRing.data);
}
static void RingSetOff() { gRing.set.on = 0; gRing.set.pairNow = nullptr; gRing.set.hasSlot = nullptr; gRing.set.atOrAbove = 0; gRing.set.allowReset = 0; }
// The raw queue's completion of D3D9 samples: the readback of a D3D9 sample target is a raw copy the rig records in its
// NEXT command buffer after a flush (map 2.8, "The readback, never a wait"), so a sample whose StretchRect has run is
// read at the next RecSubmit, from the target's tag as it is then. Call with gRing.data held.
static void RingCompleteD3D9SamplesLocked()
{
    size_t k = 0;
    for (; k < gRing.pend.size() && gRing.pend[k].seq <= gRing.csRunSeq; k++)
    {
        const RingPendSmp& s = gRing.pend[k];
        const int ti = gRing.surfImg[gRing.smpSurf[s.target]];
        const DWORD tag = gRing.img[ti].tag;
        gRing.done.push_back({ s.kind, s.p, s.f, RingClassify(s.kind, tag), tag, false });
        _InterlockedIncrement(&gRingSt.samplesD3D9);
    }
    gRing.pend.erase(gRing.pend.begin(), gRing.pend.begin() + k);
}

// ------------------------------------------------------------------ the timelines
static void RingTlSignal(int t, ULONGLONG v, bool latEnd)
{
    if (!latEnd) if (void (*f)(int, ULONGLONG) = gRing.signalInside) { gRing.signalInside = nullptr; f(t, v); }
    AcquireSRWLockExclusive(&gRing.tlLock);
    if (v > gRing.tl[t]) gRing.tl[t] = v;
    else _InterlockedIncrement(latEnd ? &gRingSt.latEndDropped : &gRingSt.tlBackwards);
    ReleaseSRWLockExclusive(&gRing.tlLock);
    WakeAllConditionVariable(&gRing.tlCv);
}

// ------------------------------------------------------------------ the device thunks (the D3D9 device, as DXVK's)
//
// WHAT EACH THUNK DOES, IN ORDER: the submission-queue rule; DXVK's device lock for the whole call, on the slots whose
// D3D9DeviceEx method opens with `D3D9DeviceLock lock = LockDevice()` (RingSlotLocks, below); FakeDev's own method (its log and its answer, unchanged); then,
// on success, the tag work the four drawing kinds do in DXVK's command stream. Only calls on the ring's device are
// locked and modelled; another device's calls pass straight through, as they have their own lock in DXVK.
static void RingModelOf(std::integral_constant<int, 34>, IDirect3DDevice9*, IDirect3DSurface9* s, const RECT*, IDirect3DSurface9* t, const RECT*, D3DTEXTUREFILTERTYPE)
{
    // StretchRect: both images taken at the call (d3d9_device.cpp:1232-1240), the copy emitted with them (1420-1429).
    bool sb, tb;
    EnterCriticalSection(&gRing.data);
    const int si = RingImgOfSurfLocked(s, &sb), ti = RingImgOfSurfLocked(t, &tb);
    LeaveCriticalSection(&gRing.data);
    if (si >= 0 && ti >= 0) RingCsRecord(RCS_COPY, ti, si, 0, tb, sb, 0);
}
static void RingModelOf(std::integral_constant<int, 35>, IDirect3DDevice9*, IDirect3DSurface9* s, const RECT*, D3DCOLOR c)
{
    // ColorFill: the image taken at the call (1493, 1512). The colour IS the tag: a partial rect fills the whole tag (the
    // fake has one value an image), which is what the sample-rect fill of fault 32 needs.
    bool b;
    EnterCriticalSection(&gRing.data);
    const int i = RingImgOfSurfLocked(s, &b);
    LeaveCriticalSection(&gRing.data);
    if (i >= 0) RingCsRecord(c == kRingMove && !b ? RCS_MOVE : RCS_FILL, i, -1, c, b, false, 0);   // (kRingMove: the defragmenter's move, planted)
}
static void RingFillRt0(IDirect3DDevice9* d, DWORD tag)
{
    // A Clear or a draw writes the bound colour target, bound at the call (BindFramebuffer, d3d9_device.cpp:6695; the draw's
    // dirty test, 7403); after a rotation the framebuffer is marked dirty (d3d9_swapchain.cpp:951), so the next one binds
    // the new image. FakeDev keeps the bound target in st.rt[0], the ring's surface 0 after a Reset (FakeDev::Defaults).
    IDirect3DSurface9* rt = ((FakeDev*)d)->st.rt[0];
    bool b;
    EnterCriticalSection(&gRing.data);
    const int i = RingImgOfSurfLocked(rt, &b);
    LeaveCriticalSection(&gRing.data);
    if (i >= 0) RingCsRecord(RCS_FILL, i, -1, tag, b, false, 0);
}
static void RingModelOf(std::integral_constant<int, 43>, IDirect3DDevice9* d, DWORD, const D3DRECT*, DWORD f, D3DCOLOR c, float, DWORD)
{ if (f & D3DCLEAR_TARGET) RingFillRt0(d, c); }
// Reset (16), after FakeDev's own (which logs it and rebinds the ring's surface 0 as the render target, as ResetSwapChain
// binds the new back buffer, d3d9_device.cpp:8967-8968): the new ring (RingConfigure says what DXVK does), in the shape
// the parameters ask for, BackBufferCount 0 counting as 1 (NormalizePresentParameters, d3d9_swapchain.cpp:817), then the
// Reset's own flush and CS wait (d3d9_device.cpp:529-530), so every command recorded before it has run, on the old
// images, when Reset returns. It runs inside the thunk, under the device lock Reset holds (449). gFkOnReset
// (dt_fake_dev.h:33) is left to the harness: dt_test.cpp uses it for its own assertions, and a dt_fg_test case may too.
static void RingModelOf(std::integral_constant<int, 16>, IDirect3DDevice9*, D3DPRESENT_PARAMETERS* pp)
{
    _InterlockedIncrement(&gRingSt.resets);
    D3DSWAPEFFECT e = gRing.effect; UINT bbc = gRing.bbc; BOOL w = gRing.windowed;
    if (pp) { e = pp->SwapEffect; bbc = pp->BackBufferCount ? pp->BackBufferCount : 1; w = pp->Windowed; }
    RingConfigure(e, bbc, w);
    RingRunCs();
}
// Every other slot: nothing to model, except that a draw (81 to 84) writes gRing.drawTag into the bound target when the
// harness has set one (0: draws leave tags alone, which is what a test of ColorFill and StretchRect wants).
template <int S, class... A> static void RingModelOf(std::integral_constant<int, S>, IDirect3DDevice9* d, A...)
{ if constexpr (S >= 81 && S <= 84) { if (gRing.drawTag) RingFillRt0(d, gRing.drawTag); } }
// THE REFUSALS DXVK MAKES BEFORE THE FAKE'S METHOD IS REACHED (added 2026-09-25 after the verifier's review: FakeDev's
// GetBackBuffer, dt_fake_dev.h:37, answers surface 0 for any index, which dt_test.cpp relies on and this file must not
// change). The device's GetBackBuffer clears the out pointer and refuses a swapchain other than 0 (d3d9_device.cpp:558-561),
// then forwards to the swapchain's, which refuses a null pointer and an index at or past BackBufferCount (the COUNT the
// game asked for, not the ring's size: d3d9_swapchain.cpp:480-486). D3D_OK means go on to FakeDev.
static HRESULT RingRefuseOf(std::integral_constant<int, 18>, IDirect3DDevice9*, UINT sc, UINT i, D3DBACKBUFFER_TYPE, IDirect3DSurface9** pp)
{
    if (pp) *pp = nullptr;
    if (sc != 0 || !pp || i >= gRing.bbc) return D3DERR_INVALIDCALL;
    return D3D_OK;
}
template <int S, class... A> static HRESULT RingRefuseOf(std::integral_constant<int, S>, IDirect3DDevice9*, A...) { return D3D_OK; }

// WHICH SLOTS TAKE THE LOCK. Read method by method from d3d9_device.cpp (a scan of each D3D9DeviceEx method's body for
// LockDevice, 2026-09-25): these open WITHOUT it and do not forward to anything that takes it: the queries
// GetAvailableTextureMem, EvictManagedResources, GetDirect3D, GetDeviceCaps, GetDisplayMode, GetCreationParameters,
// GetNumberOfSwapChains (237-288, 441), GetTextureStageState (2812-2827), GetRasterStatus and SetDialogBoxMode (567-577,
// whose swapchain methods take none either, d3d9_swapchain.cpp:500, 796), the creates (415, 599-938, 1623, 3406, 3523,
// 3882, 4104; whether their internals take it was NOT read, and the rig makes none on the worker), the patches
// (4068-4094), and Present, which takes none itself (539-550, 4237) and reaches the swapchain's, which does (113). Every
// other slot takes it, directly or through the setter or swapchain method it forwards to (SetTransform 4652, SetTexture
// 4598, SetSamplerState 4556, SetTextureStageState 4673-4683, GetBackBuffer via d3d9_swapchain.cpp:478, the gamma ramps
// 700 and 739).
static constexpr bool RingSlotLocks(int s)
{
    switch (s)
    {
    case 4: case 5: case 6: case 7: case 8: case 9: case 13: case 15: case 17: case 19: case 20:
    case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 36: case 66: case 86: case 91: case 106:
    case 115: case 116: case 117: case 118: return false;
    default: return true;
    }
}
// The thunk is a template over the method's own return and argument types, deduced by RingThunkOf from the member pointer
// (MSVC does not match a __stdcall member pointer in a class partial specialization, but it deduces one in a call).
template <int SLOT, class R, class... A> struct RingThunk
{
    static R __stdcall Fn(IDirect3DDevice9* d, A... a)
    {
        typedef R(__stdcall* Orig)(IDirect3DDevice9*, A...);
        const bool mine = gRing.ready && d == (IDirect3DDevice9*)gRing.dev;
        if (mine) RingD3D9Entry();
        RingGuard g;
        if (mine && RingSlotLocks(SLOT)) g = RingDevAcquire();
        if constexpr (std::is_void_v<R>) { ((Orig)gRingDxOrig[SLOT])(d, a...); }
        else
        {
            if constexpr (std::is_same_v<R, HRESULT>) { if (mine) { const HRESULT no = RingRefuseOf(std::integral_constant<int, SLOT>(), d, a...); if (FAILED(no)) return no; } }
            const R r = ((Orig)gRingDxOrig[SLOT])(d, a...);
            if constexpr (std::is_same_v<R, HRESULT>) { if (mine && SUCCEEDED(r)) RingModelOf(std::integral_constant<int, SLOT>(), d, a...); }
            return r;
        }
    }
};
template <int SLOT, class R, class... A> static void* RingThunkOf(R (__stdcall IDirect3DDevice9::*)(A...)) { return (void*)&RingThunk<SLOT, R, A...>::Fn; }
#define RING_DEV_SLOTS(X) \
    X(3, TestCooperativeLevel) X(4, GetAvailableTextureMem) X(5, EvictManagedResources) X(6, GetDirect3D) X(7, GetDeviceCaps) \
    X(8, GetDisplayMode) X(9, GetCreationParameters) X(10, SetCursorProperties) X(11, SetCursorPosition) X(12, ShowCursor) \
    X(13, CreateAdditionalSwapChain) X(14, GetSwapChain) X(15, GetNumberOfSwapChains) X(16, Reset) X(17, Present) \
    X(18, GetBackBuffer) X(19, GetRasterStatus) X(20, SetDialogBoxMode) X(21, SetGammaRamp) X(22, GetGammaRamp) \
    X(23, CreateTexture) X(24, CreateVolumeTexture) X(25, CreateCubeTexture) X(26, CreateVertexBuffer) X(27, CreateIndexBuffer) \
    X(28, CreateRenderTarget) X(29, CreateDepthStencilSurface) X(30, UpdateSurface) X(31, UpdateTexture) X(32, GetRenderTargetData) \
    X(33, GetFrontBufferData) X(34, StretchRect) X(35, ColorFill) X(36, CreateOffscreenPlainSurface) X(37, SetRenderTarget) \
    X(38, GetRenderTarget) X(39, SetDepthStencilSurface) X(40, GetDepthStencilSurface) X(41, BeginScene) X(42, EndScene) \
    X(43, Clear) X(44, SetTransform) X(45, GetTransform) X(46, MultiplyTransform) X(47, SetViewport) X(48, GetViewport) \
    X(49, SetMaterial) X(50, GetMaterial) X(51, SetLight) X(52, GetLight) X(53, LightEnable) X(54, GetLightEnable) \
    X(55, SetClipPlane) X(56, GetClipPlane) X(57, SetRenderState) X(58, GetRenderState) X(59, CreateStateBlock) \
    X(60, BeginStateBlock) X(61, EndStateBlock) X(62, SetClipStatus) X(63, GetClipStatus) X(64, GetTexture) X(65, SetTexture) \
    X(66, GetTextureStageState) X(67, SetTextureStageState) X(68, GetSamplerState) X(69, SetSamplerState) X(70, ValidateDevice) \
    X(71, SetPaletteEntries) X(72, GetPaletteEntries) X(73, SetCurrentTexturePalette) X(74, GetCurrentTexturePalette) \
    X(75, SetScissorRect) X(76, GetScissorRect) X(77, SetSoftwareVertexProcessing) X(78, GetSoftwareVertexProcessing) \
    X(79, SetNPatchMode) X(80, GetNPatchMode) X(81, DrawPrimitive) X(82, DrawIndexedPrimitive) X(83, DrawPrimitiveUP) \
    X(84, DrawIndexedPrimitiveUP) X(85, ProcessVertices) X(86, CreateVertexDeclaration) X(87, SetVertexDeclaration) \
    X(88, GetVertexDeclaration) X(89, SetFVF) X(90, GetFVF) X(91, CreateVertexShader) X(92, SetVertexShader) \
    X(93, GetVertexShader) X(94, SetVertexShaderConstantF) X(95, GetVertexShaderConstantF) X(96, SetVertexShaderConstantI) \
    X(97, GetVertexShaderConstantI) X(98, SetVertexShaderConstantB) X(99, GetVertexShaderConstantB) X(100, SetStreamSource) \
    X(101, GetStreamSource) X(102, SetStreamSourceFreq) X(103, GetStreamSourceFreq) X(104, SetIndices) X(105, GetIndices) \
    X(106, CreatePixelShader) X(107, SetPixelShader) X(108, GetPixelShader) X(109, SetPixelShaderConstantF) \
    X(110, GetPixelShaderConstantF) X(111, SetPixelShaderConstantI) X(112, GetPixelShaderConstantI) \
    X(113, SetPixelShaderConstantB) X(114, GetPixelShaderConstantB) X(115, DrawRectPatch) X(116, DrawTriPatch) \
    X(117, DeletePatch) X(118, CreateQuery)
// WRAP THE DEVICE, AFTER DtInstall. Each thunk's argument list is the method's own, taken from d3d9.h through the member
// pointer's type, so the compiler checks every signature; the name list is checked against dt_core.h's dtDevName, which
// is the slot order the device thread replays by. QueryInterface, AddRef and Release (0 to 2) take no lock in DXVK and
// are left alone.
static bool RingWrapDevice()
{
    if (gRingWrapped) return true;
    void* th[119] = {};
    const char* nm[119] = {};
#define RING_FILL(n, name) th[n] = RingThunkOf<n>(&IDirect3DDevice9::name); nm[n] = #name;
    RING_DEV_SLOTS(RING_FILL)
#undef RING_FILL
    for (int i = 3; i < 119; i++)
    {
        if (!th[i] || strcmp(nm[i], dtDevName[i])) { Err("the ring's thunk for slot %d is %s, the device thread's is %s", i, nm[i] ? nm[i] : "missing", dtDevName[i]); return false; }
        if (!dtDx[i]) { Err("dtDx[%d] is empty: RingWrapDevice runs after DtInstall", i); return false; }
    }
    for (int i = 0; i < 119; i++) gRingDxOrig[i] = dtDx[i];
    for (int i = 3; i < 119; i++) dtDx[i] = th[i];
    gRingWrapped = true;
    return true;
}

// ------------------------------------------------------------------ the swapchain
struct FakeRingSC : FakeSC
{
    // PRESENT, AS D3D9SwapChainEx::Present AND PresentImage DO IT, in this order:
    //   the device lock for the whole call (d3d9_swapchain.cpp:113);
    //   a lost device answers at once and nothing else happens (117-118): the base FakeSC::Present's answer, which the
    //     harness sets through gFkPresentHr, stands for it, and its log entry (slot 1803) is the base's, unchanged; no
    //     flush, no blit, no rotation (counted as presentsFailed);
    //   the Present's own flush, EndFrame and Flush (839-840), handed to the CS thread without a wait: whether the work
    //     recorded before the Present runs here is gRing.presentCs's (RING_CS_PRE and RING_CS_ALL run it, LAZY leaves it);
    //   the blit, a CS item holding the image current NOW (swapImage, 848, taken before the command is emitted) that
    //     appends that image's tag, as it is when the item runs, to the PRESENTED log; it is emitted at 881 and handed to
    //     the CS thread by FlushCsChunk at 915, INSIDE the Present, so "the next flush" is only the LATEST it can run:
    //     RING_CS_ALL runs it here, before the rotation, and the other two leave it for the next flush, where a raw submit
    //     made after the Present and before that flush overtakes it. So a harness case may show that overtaking happen,
    //     but must never assert that a blit has NOT reached the queue as the only outcome DXVK allows;
    //   SyncFrameLatency (925, 1142-1145): the gate, where the harness can hold the calling thread with the lock held;
    //   the rotation (951-952): each i = 1 .. n-1 swaps the images under surfaces i and i-1, so the surfaces stay the same
    //     objects and surface 0 ends up over what was surface 1's image; then the framebuffer is marked dirty (954).
    // NOT MODELLED: the GDI fallback for a partial copy (184-186) and a failed window context (153-154, an early return
    // before the flush), neither of which the rig's swapchain reaches; the front-buffer blit of d3d9.extraFrontbuffer
    // (164-176, 942-946), off by default; no back buffers after a failed Reset (125-126), which the fake never has.
    STDMETHOD(Present)(const RECT* s, const RECT* t, HWND w, const RGNDATA* r, DWORD f) override
    {
        // ENTERED, before anything of DXVK's (its lock included): the fake's own log for the gate before the present
        EnterCriticalSection(&gRing.data);
        gRing.entered.push_back({ GetCurrentThreadId(), (LONG)gRing.preNoted.size() });
        LeaveCriticalSection(&gRing.data);
        RingD3D9Entry();
        // A PRESENT AFTER A REFUSED SUBMIT OF THE SAME SPAN (stage 6; check-machine.md, X1): the caller holds the interop
        // lock (a fire's present is made under the fire's own) and one of its submits was refused since it took it. What
        // it presents is then not what it meant to copy in: the back buffer as it stood.
        if (tRingRefused && RingInteropHeldByMe()) _InterlockedIncrement(&gRingSt.presentAfterRefused);
        // A FIRE'S PRESENT OVER A BACK BUFFER THE GAME HAS WRITTEN, WITH NO SAVE OF IT (stage 7): the picture is lost
        if (gRing.back.on && gRing.back.dirty && !tRingBackSaved && RingInteropHeldByMe()) _InterlockedIncrement(&gRingSt.backLost);
        RingGuard g = RingDevAcquire();
        const HRESULT hr = FakeSC::Present(s, t, w, r, f);
        if (FAILED(hr)) { _InterlockedIncrement(&gRingSt.presentsFailed); return hr; }
        _InterlockedIncrement(&gRingSt.presents);
        if (gRing.presentCs != RING_CS_LAZY) { _InterlockedIncrement(&gRingSt.presentFlushes); RingRunCs(); }
        EnterCriticalSection(&gRing.data);
        const int cur = gRing.under[0];
        const LONG ord = ++gRing.ord;
        LeaveCriticalSection(&gRing.data);
        RingCsRecord(RCS_BLIT, -1, cur, 0, false, false, ord);
        if (gRing.presentCs == RING_CS_ALL) RingRunCs();
        if (_InterlockedExchange(&gRing.gateArmed, 0))
        {
            _InterlockedExchange(&gRing.gateIn, 1);
            WaitForSingleObject(gRing.gateEv, INFINITE);
            _InterlockedExchange(&gRing.gateIn, 0);
        }
        EnterCriticalSection(&gRing.data);
        for (int i = 1; i < gRing.n; i++) std::swap(gRing.under[i], gRing.under[i - 1]);
        LeaveCriticalSection(&gRing.data);
        return hr;
    }
    // GetBackBuffer (473-497): the lock (478); a null pointer (480-481) and an index at or past BackBufferCount (483-486),
    // which is not the ring's size, are refused; surface i itself, the stable object (495), never an image.
    STDMETHOD(GetBackBuffer)(UINT i, D3DBACKBUFFER_TYPE, IDirect3DSurface9** pp) override
    {
        RingD3D9Entry();
        RingGuard g = RingDevAcquire();
        if (!pp) return D3DERR_INVALIDCALL;
        if (i >= gRing.bbc || (int)i >= gRing.n) return D3DERR_INVALIDCALL;
        *pp = gRing.surf[i]; (*pp)->AddRef();
        return D3D_OK;
    }
    STDMETHOD(GetPresentParameters)(D3DPRESENT_PARAMETERS* p) override
    {
        memset(p, 0, sizeof(*p));
        p->BackBufferWidth = 1920; p->BackBufferHeight = 1080; p->BackBufferFormat = D3DFMT_X8R8G8B8;
        p->BackBufferCount = gRing.bbc; p->SwapEffect = gRing.effect; p->Windowed = gRing.windowed;
        return D3D_OK;
    }
};
// The harness holds the next Present to reach the gate until RingGateRelease (the ObsGate pattern, dt_test.cpp:691-721).
static void RingGateHoldNext() { _InterlockedExchange(&gRing.gateArmed, 1); }
static void RingGateRelease()  { SetEvent(gRing.gateEv); }
static bool RingGateHeld()     { return gRing.gateIn != 0; }

// INSTALL THE MODEL over a device and its ring swapchain, BEFORE DtInstall (so the device thread hooks the ring's vtable),
// then RingWrapDevice after it. The rig's Vulkan images, its D3D9 surfaces and the 16 sample targets are made here; the
// mod makes the D3D9 ones on the render thread (map 2.5 step 9), which the fake has no reason to model.
static void RingInstall(FakeDev* dev, FakeRingSC* sc, D3DSWAPEFFECT e = D3DSWAPEFFECT_DISCARD, UINT bbc = 2, BOOL windowed = TRUE)
{
    InitializeCriticalSection(&gRing.data);
    InitializeCriticalSection(&gRing.q);
    gRing.gateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    gRing.img.reserve(RING_IMG_MAX);
    gRing.dev = dev; gRing.sc = sc;
    gRing.surf[0] = &sc->bb;
    for (int i = 1; i < RING_SURF_MAX; i++) gRing.surf[i] = new FakeSurf(1920, 1080, nullptr, D3DUSAGE_RENDERTARGET);
    for (int i = 0; i < FG_IMG_N; i++) gRing.rigImg[i] = RingNewImg();
    EnterCriticalSection(&gRing.data);
    for (int i = 1; i < FG_SURF_N; i++) { gRing.rigSurf[i] = new FakeSurf(1920, 1080, nullptr, D3DUSAGE_RENDERTARGET); RingImgOfSurfLocked(gRing.rigSurf[i], nullptr); }
    for (int i = 0; i < RING_SMP_TARGETS; i++) { gRing.smpSurf[i] = new FakeSurf(8, 8, nullptr, D3DUSAGE_RENDERTARGET); RingImgOfSurfLocked(gRing.smpSurf[i], nullptr); }
    LeaveCriticalSection(&gRing.data);
    gRing.ready = true;
    RingConfigure(e, bbc, windowed);
}
static IDirect3DSurface9* RingSurf(int s)
{
    if (s == FG_SURF_BB) return gRing.surf[0];
    if (s > 0 && s < FG_SURF_N) return gRing.rigSurf[s];
    _InterlockedIncrement(&gRingSt.badSurface);
    return nullptr;
}
static int RingImgOfSurf(const void* s) { EnterCriticalSection(&gRing.data); const int i = RingImgOfSurfLocked(s, nullptr); LeaveCriticalSection(&gRing.data); return i; }
// The logs the harness reads, and a clean slate between cases (the stats and the modelled locks are not touched).
static std::vector<RingPresented> RingPresentedLog() { EnterCriticalSection(&gRing.data); auto v = gRing.presented; LeaveCriticalSection(&gRing.data); return v; }
static std::vector<RingPipe> RingPipeLog() { EnterCriticalSection(&gRing.data); auto v = gRing.pipe; LeaveCriticalSection(&gRing.data); return v; }
static size_t RingCsPending() { EnterCriticalSection(&gRing.data); const size_t n = gRing.cs.size(); LeaveCriticalSection(&gRing.data); return n; }
static size_t RingSamplesPending() { EnterCriticalSection(&gRing.data); const size_t n = gRing.pend.size(); LeaveCriticalSection(&gRing.data); return n; }
static void RingClearLogs()
{
    EnterCriticalSection(&gRing.data);
    gRing.presented.clear(); gRing.pipe.clear(); gRing.done.clear(); gRing.pend.clear(); gRing.entered.clear(); gRing.preNoted.clear();
    LeaveCriticalSection(&gRing.data);
}
// THE GATE BEFORE DXVK'S PRESENT: a binding's PresentGate notes its call here (the product's shape, dt_fg_shadow.h), and
// the two logs as they stand.
static void RingPreNoteCall()
{
    EnterCriticalSection(&gRing.data);
    gRing.preNoted.push_back({ GetCurrentThreadId(), (LONG)gRing.entered.size() });
    LeaveCriticalSection(&gRing.data);
}
static std::vector<RingEntered> RingEnteredLog() { EnterCriticalSection(&gRing.data); auto v = gRing.entered; LeaveCriticalSection(&gRing.data); return v; }
static std::vector<RingPreNote> RingPreNotedLog() { EnterCriticalSection(&gRing.data); auto v = gRing.preNoted; LeaveCriticalSection(&gRing.data); return v; }

// ------------------------------------------------------------------ THE BINDING (fg_machine.h's contract, every function)
//
// Facts the machine reads from the mod's globals are plain statics here, which the harness writes by hand. Every seam
// that is DXVK goes to the model above; every seam that is an event, a timer or a pipe is a real one, so the machine's
// threads really wait and really wake.
struct FgHarnessBinding
{
    // THE THREE TRAITS (fg_machine.h, "THE THREE TRAITS"): the harness's binding is the rig's shape, an instrument that
    // arms, on phase 1's stub
    static constexpr bool kRig = true, kEngage = true, kWire = false;
    // ---- the facts the harness sets
    static inline LONG frames = 0, camJumps = 0, hdrEncodes = 0, paceSleptN = 0, sentN = 0;
    static inline bool hdrOn = false, hdrView = false, limiter = false;
    static inline ULONGLONG refreshPacked = 0;
    static inline int ffCap = 0, ffRel = 0, dxvkCap = 0;
    static inline LONG latOrd[64] = {};
    // ---- the clock: real QPC, or a fake one the harness steps for the placer's unit cases
    static inline bool fakeClock = false;
    static inline LONGLONG clockNow = 0;
    // ---- the chained observers (map 2.4), each settable; each call is counted
    static inline void (*prevPresentObs)(LONG) = nullptr;
    static inline void (*prevInjectObs)(DWORD, const DWORD*, DWORD) = nullptr;
    static inline void (*prevArmObs)() = nullptr;
    static inline void (*prevDisarmObs)() = nullptr;
    static inline void (*prevObs82)(const DWORD*) = nullptr;
    static inline volatile LONG nPrevPresent = 0, nPrevInject = 0, nPrevArm = 0, nPrevDisarm = 0, nPrev82 = 0;
    // ---- the events, the pace timer, and counts
    static inline HANDLE evPlacer = nullptr, evPace = nullptr, paceTimer = nullptr;
    static inline volatile LONG nWakePlacer = 0, nWakePace = 0, nOrigPresent = 0, nDxvkPresent = 0;
    static inline int lines = 0;
    // ---- origSwapPresent (the harness's own variable, map 2.4): the machine's Install reads it and puts FgSwapPresent in
    //      its place. FgHarnessInit points it at dtScPresent, which is what the mod's holds when the rig installs.
    static inline void* origSwapPresent = nullptr;

    // STATE AND CLOCK
    static FgState& S()
    {
        // THE RING MEMORY IS VirtualAlloc'd, as the mod's is (fg_machine.h, "THE MACHINE'S STATE"): FgStateBytes() of zeroed,
        // page-aligned memory, attached once. FgHarnessReset() gives a fresh state between cases.
        static FgState s;
        static void* mem = nullptr;
        if (!mem)
        {
            mem = VirtualAlloc(nullptr, FgStateBytes(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!mem) { Err("VirtualAlloc of the machine's rings failed"); static BYTE spare[1 << 16]; mem = spare; }
            FgStateAttach(s, mem);
        }
        return s;
    }
    static LONGLONG Now() { if (fakeClock) return clockNow; LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
    static LONGLONG Qpf() { static LONGLONG f = 0; if (!f) { LARGE_INTEGER q; QueryPerformanceFrequency(&q); f = q.QuadPart; } return f; }
    static void Line(const char* fmt, ...)
    {
        if (lines++ >= 400) return;
        va_list a; va_start(a, fmt); printf("  [fg] "); vprintf(fmt, a); printf("\n"); va_end(a);
    }

    // RENDER-THREAD FACTS
    static LONG Frames() { return frames; }
    static LONG CamJumps() { return camJumps; }
    static LONG HdrEncodes() { return hdrEncodes; }
    static bool HdrOn() { return hdrOn; }
    static bool HdrView() { return hdrView; }
    static ULONGLONG RefreshPacked() { return refreshPacked; }
    static int FfCap() { return ffCap; }
    static int FfRel() { return ffRel; }
    static int DxvkCap() { return dxvkCap; }
    static bool Limiter() { return limiter; }
    static LONG PaceSleptN() { return paceSleptN; }
    // The saved origSwapPresent is the device thread's dtScPresent (dt_res.h:533-580): it records DT_OP_PRESENT with
    // dtScPresentO as the callee, which the rig's install replaces with FgWorkerPresent, and waits DT_R_PACE for the one before.
    static HRESULT OrigSwapPresent(IDirect3DSwapChain9* sc, const RECT* s, const RECT* t, HWND w, const RGNDATA* r, DWORD f)
    { _InterlockedIncrement(&nOrigPresent); return dtScPresent(sc, s, t, w, r, f); }
    // The binding's render-thread tick (stage S4): the mod retries its D3D9 targets there; the fake has none.
    static void RenderTick() {}
    // THE GATE BEFORE DXVK'S PRESENT (fg_machine.h's list): nothing, as the rig's does nothing. The product's shape notes
    // its calls (dt_fg_shadow.h, ShB::PresentGate), and L9 reads them against the entries of the fake Present.
    static void PresentGate() {}
    // DXVK'S BARE PRESENT FOR THE MACHINE'S INSTALL (fg_machine.h, Install; the fold of 2026-10-10): the mod's bindings hand
    // back DXVK's own pointer when the transport's gate stands in dtScPresentO as a thunk. No gate stands there in this
    // harness, so the pointer comes back as it is; dt_fg_shadow.h's L20 puts the mod's own thunk there and has a binding
    // that takes it out as the product's does.
    static void* DxvkBare(void* p) { return p; }
    // FgPace's own timer (map 2.11): a real high-resolution waitable timer and the pace-wake event, the event first so a
    // wake that is already set wins. With the fake clock the wait is not real: the clock steps to the due time, and only a
    // wake already set counts.
    static int PaceWait(LONGLONG dueQpc)
    {
        if (fakeClock)
        {
            if (WaitForSingleObject(evPace, 0) == WAIT_OBJECT_0) return 1;
            if (dueQpc > clockNow) clockNow = dueQpc;
            return 0;
        }
        LONGLONG left = dueQpc - Now();
        if (left < 0) left = 0;
        LARGE_INTEGER due; due.QuadPart = -(LONGLONG)((double)left * 1e7 / (double)Qpf());
        if (!SetWaitableTimer(paceTimer, &due, 0, nullptr, nullptr, FALSE)) { Err("SetWaitableTimer failed (%lu)", GetLastError()); return 0; }
        const HANDLE h[2] = { evPace, paceTimer };
        const DWORD r = WaitForMultipleObjects(2, h, FALSE, INFINITE);
        if (r == WAIT_OBJECT_0) { CancelWaitableTimer(paceTimer); return 1; }
        return 0;
    }

    // WORKER FACTS
    static LONG SentN() { return sentN; }
    static LONG LatOrd(LONG n) { return latOrd[n & 63]; }

    // WORKER SPANS
    // CbTake: a slot of the rig's 32 not retired within 2 s makes the span withhold, BEFORE any lock (O-S4). The fake's
    // slots always retire; the harness makes one fail with gRing.cbFailNext. Taking one under the interop lock is H14's
    // fault and is counted.
    // A RESERVATION BELONGS TO ITS SPAN (revised 2026-09-25 after the verifier's review). It was one running total, so the
    // first span that took more than it began left a surplus that hid every later begin with nothing taken: the machine's
    // Z N fire took 3 and began 2 (FirePresent's CbTake against its two RecBegins), D's FG_HUDLESS took 2 and began 1
    // (SpanHudless's), until 30231f20 matched each reservation to its span, and after either the check was blind for the
    // rest of the run (the line numbers this cited moved with that commit). Now a CbTake REPLACES the reservation, and
    // the span's end (DevUnlock, the interop lock's release, or the next CbTake when a span takes no lock) drops what is
    // left; what is dropped is counted as cbUnused, a fact, because taking more than a span uses is wasteful, not wrong.
    static void CbDrop()
    {
        const LONG left = _InterlockedExchange(&gRing.cbReserved, 0);
        if (left > 0) _InterlockedExchangeAdd(&gRingSt.cbUnused, left);
    }
    static bool CbTake(int n)
    {
        if (RingInteropHeldByMe()) _InterlockedIncrement(&gRingSt.cbUnderLock);
        CbDrop();
        if (gRing.cbFailNext > 0) { _InterlockedDecrement(&gRing.cbFailNext); return false; }
        _InterlockedExchange(&gRing.cbReserved, n);
        return true;
    }
    static void DevLock() { RingInteropLock(); tRingRefused = 0; tRingBackSaved = 0; }
    static void DevUnlock() { RingInteropUnlock(); CbDrop(); tRingRefused = 0; tRingBackSaved = 0; }
    // FlushRenderingCommands: m_device->Flush(), which takes the device lock (d3d9_device.cpp:6176-6179), then
    // SynchronizeCsThread(SynchronizeAll) (d3d9_interop.cpp:235-238): every D3D9 command recorded so far has reached the
    // queue when it returns. The fake runs its CS list.
    static void Flush()
    {
        RingD3D9Entry();
        RingGuard g = RingDevAcquire();
        _InterlockedIncrement(&gRingSt.flushes);
        RingRunCs();
    }
    // A FRESH query of the back buffer's image, THE WAY THE MOD MAKES IT (revised 2026-09-25 after the verifier's review;
    // it was exempt from the queue rule because GetVulkanImageInfo takes no device lock, d3d9_interop.cpp:106-151). The
    // mod's query is IvtBackbufferInfo (DlssIvTransport.h:2746-2777, map 2.7.3 step 2): dev->GetBackBuffer, which takes
    // the device lock (d3d9_swapchain.cpp:478), QueryInterface, GetVulkanImageInfo, then Release. That is a D3D9 call,
    // under d3d9_interfaces.h:192-193's rule like any other, so the fake makes it as the mod does: GetBackBuffer(0)
    // through dtDx[18] (the thunk: the queue rule and the lock), the image under the surface it returned, and Release.
    // A query moved inside QLock is now counted, as the rule requires.
    static ULONGLONG BBImage()
    {
        if (!gRingWrapped) { Err("BBImage before RingWrapDevice"); return 0; }
        IDirect3DSurface9* bb = BbSurface();
        if (!bb) return 0;
        EnterCriticalSection(&gRing.data);
        const int i = RingImgOfSurfLocked(bb, nullptr);
        const ULONGLONG h = i >= 0 ? gRing.img[i].handle : 0;
        LeaveCriticalSection(&gRing.data);
        bb->Release();
        return h;
    }
    static void QLock() { RingQEnter(true); }
    static void QUnlock() { RingQLeave(); }
    static void RecBegin()
    {
        if (gRing.cbReserved <= 0) _InterlockedIncrement(&gRingSt.cbUntaken);   // nothing left in this span's reservation
        else _InterlockedDecrement(&gRing.cbReserved);
        if (gRing.cbOpen) _InterlockedIncrement(&gRingSt.recNoBegin);   // a second begin with one open: the first is lost
        gRing.cb.clear(); gRing.cbOpen = true;
    }
    static void RingRec(const RingRawOp& o) { if (!gRing.cbOpen) { _InterlockedIncrement(&gRingSt.recNoBegin); return; } gRing.cb.push_back(o); }
    // The raw copies name the back buffer by HANDLE (the VkImage the query returned): a copy into a handle writes THAT image
    // even if DXVK rotated another one under the surface since (d3d9_swapchain.cpp:951-952 moves images, not handles).
    static void RecCopyToImg(ULONGLONG bb, int img) { if (img == FG_IMG_SAVE) tRingBackSaved = 1; RingRec({ RRAW_COPY, RingRig(img), RingImgOfHandle(bb), 0, 0, 0, 0, 0 }); }
    static void RecCopyFromImg(int img, ULONGLONG bb) { RingRec({ RRAW_COPY, RingImgOfHandle(bb), RingRig(img), 0, 0, 0, 0, 0 }); }
    static void RecCopyImg(int src, int dst) { RingRec({ RRAW_COPY, RingRig(dst), RingRig(src), 0, 0, 0, 0, 0 }); }
    static void RecPatch(int img) { RingRec({ RRAW_PATCH, RingRig(img), -1, 0, 0, 0, 0, 0 }); }
    // THE ACQUIRE AND THE RELEASE OF A SET IMAGE (stage 6; "THE HELPER'S SET"): commands of the buffer, in its order. NOT
    // seams of the machine's: a binding's copy seams make them round their copies (dt_fg_shadow.h, ShB), as the
    // product's make the external transfers. For an image that is not the helper's they do nothing.
    static void RecAcquire(int img) { RingRec({ RRAW_ACQUIRE, RingRig(img), -1, 0, 0, 0, 0, 0 }); }
    static void RecRelease(int img) { RingRec({ RRAW_RELEASE, RingRig(img), -1, 0, 0, 0, 0, 0 }); }
    static void RecSampleRaw(int img, LONG p, LONG f, int kind) { RingRec({ RRAW_SAMPLE, -1, RingRig(img), 0, kind, 0, p, f }); }
    static void RecSignal(int tl, LONG v)
    {
        if (tl < 0 || tl >= FG_TL_N) { _InterlockedIncrement(&gRingSt.badSurface); return; }
        RingRec({ RRAW_SIGNAL, -1, -1, tl, 0, (ULONGLONG)(LONG)v, 0, 0 });
    }
    // THE SUBMIT ACTS AT ONCE, in submission order: the D3D9 samples whose StretchRect has already reached the queue are read
    // first (their readback heads the rig's next buffer, map 2.8), then this buffer's copies, patches and raw samples in
    // order, then its signals (a Vulkan submit signals its semaphores after its commands). Work still in the CS list is NOT
    // touched: it has not reached the queue, and this submit overtakes it. A copy that names a back-buffer image a Reset
    // destroyed is counted (deadImage) and does nothing: in DXVK it would name a freed VkImage (see RingConfigure).
    // THE RETIRE WAIT'S TWO COUNTS (fg_machine.h BeforeReset, 2026-10-06): every submit is ASKED a value; only one that is
    // taken ever has it signalled, and here a taken submit retires at once. RetireWait asks for the last value TAKEN; a
    // control's binding asks for the last value ASKED (the rig's line until that day) and waits out its bound after one
    // refused submit.
    static inline volatile LONG subAsked = 0, subTook = 0;
    // THE CARD HELD (2026-10-28, HU14: a Reset with the hand's HUD-less copy still on the card). Here a taken submit retires at
    // once; WHILE cardHold STANDS a taken submit is applied as before but has NOT RETIRED: the retire wait waits for it (subRet,
    // the last value retired), and every image its copies name is ON THE CARD (OnCard) until the hold is let go (CardRelease,
    // from any thread: the card finishing). cardHold 0, as every case before ran: subRet follows subTook at each submit.
    static inline volatile LONG cardHold = 0, subRet = 0;
    static inline std::vector<int> onCard;                    // (under gRing.data) the images the held submits' copies name
    static void CardRelease()
    {
        _InterlockedExchange(&cardHold, 0);
        EnterCriticalSection(&gRing.data); onCard.clear(); LeaveCriticalSection(&gRing.data);
        _InterlockedExchange(&subRet, subTook);
    }
    static int OnCard(int img)
    {
        int n = 0;
        EnterCriticalSection(&gRing.data); for (int x : onCard) if (x == img) n++; LeaveCriticalSection(&gRing.data);
        return n;
    }
    static bool RetireWaitFor(LONG v, DWORD ms)
    {
        const ULONGLONG end = GetTickCount64() + ms;
        while (subRet < v) { if (GetTickCount64() >= end) return false; Sleep(1); }
        return true;
    }
    static bool RetireWait(DWORD ms) { return RetireWaitFor(subTook, ms); }
    static bool RecSubmit()
    {
        if (!gRing.cbOpen) { _InterlockedIncrement(&gRingSt.recNoBegin); return false; }
        gRing.cbOpen = false;
        if (gRing.qOwner != (LONG)GetCurrentThreadId()) _InterlockedIncrement(&gRingSt.rawNoQLock);
        const LONG asked = _InterlockedIncrement(&subAsked);
        if (gRing.submitFailNext > 0)
        {
            if (gRing.submitFailSkip > 0) _InterlockedDecrement(&gRing.submitFailSkip);   // (stage 6: this one is let through)
            else
            {
                _InterlockedDecrement(&gRing.submitFailNext);
                if (RingInteropHeldByMe()) { tRingRefused = 1; _InterlockedIncrement(&gRingSt.refusedInSpan); }
                return false;
            }
        }
        std::vector<std::pair<int, ULONGLONG>> sig;
        RingQEnter(false);
        EnterCriticalSection(&gRing.data);
        RingCompleteD3D9SamplesLocked();
        for (const RingRawOp& o : gRing.cb)
        {
            switch (o.op)
            {
            case RRAW_COPY:
                if (o.dst < 0 || o.src < 0) break;
                if (cardHold) { onCard.push_back(o.src); onCard.push_back(o.dst); }   // (HU14: on the card until the hold goes)
                if (gRing.img[o.dst].dead || gRing.img[o.src].dead) { _InterlockedIncrement(&gRingSt.deadImage); break; }
                if (!RingSetCopyLocked(o.dst, o.src)) break;      // (stage 6: the helper's set has rules of its own)
                gRing.img[o.dst].tag = gRing.img[o.src].tag;
                break;
            case RRAW_ACQUIRE: if (o.dst >= 0) RingSetOwnLocked(o.dst, true); break;
            case RRAW_RELEASE: if (o.dst >= 0) RingSetOwnLocked(o.dst, false); break;
            case RRAW_PATCH: if (o.dst >= 0) gRing.img[o.dst].tag = gRing.genTag; break;
            case RRAW_SAMPLE:
                if (o.src >= 0) { const DWORD t = gRing.img[o.src].tag; gRing.done.push_back({ o.kind, o.p, o.f, RingClassify(o.kind, t), t, true }); _InterlockedIncrement(&gRingSt.samplesRaw); }
                break;
            case RRAW_SIGNAL: sig.push_back({ o.tl, o.v }); break;
            }
        }
        RingSetBufferEndLocked();
        gRing.cb.clear();
        LeaveCriticalSection(&gRing.data);
        RingQLeave();
        for (const auto& s : sig) RingTlSignal(s.first, s.second, false);
        _InterlockedIncrement(&gRingSt.submits);
        if (asked > subTook) _InterlockedExchange(&subTook, asked);
        if (!cardHold && asked > subRet) _InterlockedExchange(&subRet, asked);   // (HU14: not held, retired at once as before)
        return true;
    }
    // THE RIG'S D3D9 CALLS go through dtDx, straight to DXVK as the mod's do (map 2.9, dt_core.h:55-58), so here they pass
    // through the device thunks: the lock, the queue rule, FakeDev's log (slots 18, 34, 35) and the CS list. FG_SURF_BB is
    // asked for fresh each time through the device's GetBackBuffer (a PASS call in the mod, dt_device_sync.h:439).
    static IDirect3DSurface9* BbSurface()
    {
        IDirect3DSurface9* s = nullptr;
        typedef HRESULT(__stdcall* GetBb_t)(IDirect3DDevice9*, UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9**);
        ((GetBb_t)dtDx[18])((IDirect3DDevice9*)gRing.dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &s);
        return s;
    }
    static IDirect3DSurface9* SurfFor(int surf, bool* release)
    {
        *release = surf == FG_SURF_BB;
        return surf == FG_SURF_BB ? BbSurface() : RingSurf(surf);
    }
    static void D3D9Stretch(int srcSurf, int dstSurf)
    {
        if (!gRingWrapped) { Err("D3D9Stretch before RingWrapDevice"); return; }
        bool rs, rd;
        IDirect3DSurface9* s = SurfFor(srcSurf, &rs); IDirect3DSurface9* d = SurfFor(dstSurf, &rd);
        if (s && d) ((dtT_StretchRect)dtDx[34])((IDirect3DDevice9*)gRing.dev, s, nullptr, d, nullptr, D3DTEXF_NONE);
        if (rs && s) s->Release();
        if (rd && d) d->Release();
    }
    // The black ColorFill of fault 32: the colour is the harness's declared black tag.
    static void D3D9Fill(int dstSurf)
    {
        if (!gRingWrapped) { Err("D3D9Fill before RingWrapDevice"); return; }
        bool rd;
        IDirect3DSurface9* d = SurfFor(dstSurf, &rd);
        const RECT rc = { 896, 476, 1024, 604 };
        if (d) ((dtT_ColorFill)dtDx[35])((IDirect3DDevice9*)gRing.dev, d, &rc, gRing.blackTag);
        if (rd && d) d->Release();
    }
    // A SAMPLE through DXVK's stream (map 2.8): a StretchRect from the back buffer into one of 16 8x8 targets, CS-ordered, so
    // it reads what the blit would read at that point; its readback is collected by a later RecSubmit (see above).
    static void D3D9Sample(int kind, LONG p, LONG f)
    {
        if (!gRingWrapped) { Err("D3D9Sample before RingWrapDevice"); return; }
        IDirect3DSurface9* bb = BbSurface();
        EnterCriticalSection(&gRing.data);
        const int t = gRing.smpNext++ % RING_SMP_TARGETS;
        LeaveCriticalSection(&gRing.data);
        tRingLastSeq = -1;
        const RECT rc = { 896, 476, 1024, 604 };
        const HRESULT hr = ((dtT_StretchRect)dtDx[34])((IDirect3DDevice9*)gRing.dev, bb, &rc, gRing.smpSurf[t], nullptr, D3DTEXF_POINT);
        if (bb) bb->Release();
        if (FAILED(hr) || tRingLastSeq < 0) { Err("a D3D9 sample's StretchRect was not recorded (hr 0x%08lX)", hr); return; }
        EnterCriticalSection(&gRing.data);
        gRing.pend.push_back({ kind, p, f, t, tRingLastSeq });
        LeaveCriticalSection(&gRing.data);
    }
    // fgDxvkPresent: DXVK's own Present, the swapchain's original method called directly, past every hook. NO LONGER A SEAM
    // (fg_machine.h's contract, revised 2026-09-25): the machine calls the dtScPresentO its Install saved, which in the
    // harness is FakeRingSC::Present because DtInstall hooked the ring's vtable. The same method either way; kept here for
    // the self-test and for a harness case that presents without the machine.
    static HRESULT DxvkPresent(IDirect3DSwapChain9* sc, const RECT* s, const RECT* t, HWND w, DWORD flags)
    {
        _InterlockedIncrement(&nDxvkPresent);
        if (sc != (IDirect3DSwapChain9*)gRing.sc) { Err("DxvkPresent on a swapchain that is not the ring's"); return D3DERR_INVALIDCALL; }
        return gRing.sc->FakeRingSC::Present(s, t, w, nullptr, flags);
    }
    // LatEndSignal(f): a raw signal of the transport's end timeline, submitted under the submission queue; its guard drops a
    // value at or below the last (map 2.5, "LatEndSignal's guard").
    static void LatEnd(LONG f)
    {
        RingQEnter(true);
        RingTlSignal(FG_TL_END, (ULONGLONG)(LONG)f, true);
        RingQLeave();
    }
    static bool SampleCollect(int* kind, LONG* p, LONG* f, int* cls)
    {
        EnterCriticalSection(&gRing.data);
        const bool have = !gRing.done.empty();
        if (have) { const RingSample s = gRing.done.front(); gRing.done.pop_front(); *kind = s.kind; *p = s.p; *f = s.f; *cls = s.cls; }
        LeaveCriticalSection(&gRing.data);
        return have;
    }

    // TIMELINES: several threads can wait on one; a signal wakes every waiter.
    // THE TIME-OUT IS COARSE (noted 2026-09-25 after the verifier's review): the deadline is GetTickCount64, which steps in
    // about 15.6 ms, so a 60 ms time-out took 61.7 to 63.4 ms in the self-test. That is fine for a time-out, and a wake by a
    // signal is not delayed by it. But a stamp must be taken from Now() by the stamper after TlWait returns, as StampStep
    // does (fg_machine.h), never from when TlWait's own deadline passed: H6's 0.1 ms bound would not survive the tick.
    static ULONGLONG TlValue(int tl)
    {
        if (tl < 0 || tl >= FG_TL_N) return 0;
        AcquireSRWLockShared(&gRing.tlLock); const ULONGLONG v = gRing.tl[tl]; ReleaseSRWLockShared(&gRing.tlLock);
        return v;
    }
    // THE STAMPER MADE LATE (round 9's S9, H6, in place of the fault key's 2): with tlInLate set, a wait for an fgSemIn value
    // that is a multiple of 100 comes back 1 ms after the value came and says it timed out, so the stamper's next step finds
    // the value already there and stamps it late, as a stamper busy elsewhere would.
    static inline volatile LONG tlInLate = 0;
    static bool TlWait(int tl, ULONGLONG v, DWORD ms)
    {
        if (tl < 0 || tl >= FG_TL_N) return false;
        const ULONGLONG end = GetTickCount64() + ms;
        AcquireSRWLockExclusive(&gRing.tlLock);
        while (gRing.tl[tl] < v)
        {
            const ULONGLONG now = GetTickCount64();
            if (now >= end) break;
            SleepConditionVariableSRW(&gRing.tlCv, &gRing.tlLock, (DWORD)(end - now), 0);
        }
        const bool ok = gRing.tl[tl] >= v;
        ReleaseSRWLockExclusive(&gRing.tlLock);
        if (ok && tlInLate && tl == FG_TL_IN && (v % 100) == 0) { const LONGLONG t0 = Now(); while (Now() - t0 < Qpf() / 1000) Sleep(0); return false; }
        return ok;
    }

    // THE CHAINED OBSERVERS
    static void PrevPresentObs(LONG k) { _InterlockedIncrement(&nPrevPresent); if (prevPresentObs) prevPresentObs(k); }
    static void PrevInjectObs(DWORD kind, const DWORD* payload, DWORD bytes) { _InterlockedIncrement(&nPrevInject); if (prevInjectObs) prevInjectObs(kind, payload, bytes); }
    static void PrevArmObs() { _InterlockedIncrement(&nPrevArm); if (prevArmObs) prevArmObs(); }
    static void PrevDisarmObs() { _InterlockedIncrement(&nPrevDisarm); if (prevDisarmObs) prevDisarmObs(); }
    static void PrevObs82(const DWORD* c) { _InterlockedIncrement(&nPrev82); if (prevObs82) prevObs82(c); }

    // THE PIPE'S STUBS (G6): each write is logged with its thread; a write entered while another is inside is counted.
    // gRing.pipeInside, one shot, runs inside a write so the self-test can show the re-entry counter move.
    static void PipeWrite(char type, LONG p, DWORD flags)
    {
        if (_InterlockedExchange(&gRing.pipeIn, 1)) _InterlockedIncrement(&gRingSt.pipeReentry);
        if (void (*f)() = gRing.pipeInside) { gRing.pipeInside = nullptr; f(); }
        EnterCriticalSection(&gRing.data);
        gRing.pipe.push_back({ type, p, flags, GetCurrentThreadId() });
        LeaveCriticalSection(&gRing.data);
        _InterlockedExchange(&gRing.pipeIn, 0);
    }
    static void WriteQ(LONG p) { PipeWrite('Q', p, 0); }
    static void WriteG(LONG p, DWORD flags) { PipeWrite('G', p, flags); }

    // EVENTS: auto-reset, as a wake that nobody consumed yet must not be counted twice.
    static void WakePlacer() { _InterlockedIncrement(&nWakePlacer); SetEvent(evPlacer); }
    static void WakePace() { _InterlockedIncrement(&nWakePace); SetEvent(evPace); }
    // The exact raise's poll (fg_machine.h PlacerRun, 2026-10-04): the placer's event, taken if it is set, as the mod's does.
    static bool PlacerPoll() { return WaitForSingleObject(evPlacer, 0) == WAIT_OBJECT_0; }

    // THE SEAMS THE CONTRACT GAINED WHILE THE MACHINE WAS WRITTEN (fg_machine.h, 2026-09-25): the chained observers and the
    // two present pointers are no longer seams (the machine saves them from the device thread's own tables, which the
    // harness has), so the Prev* functions above are kept only for a machine that still calls them; these three replace them.
    static void*& OrigSwapPresentSlot() { return origSwapPresent; }
    static DWORD RenderTid() { return renderTid; }
    // (PresFrame, Spacing (W13's input), GpuTickNs (the present end's tick) and MarkLockTicks (the marker's lock time) went with
    // the instrument's records in round 9's S9: the machine calls none of them)
};
// The binding's own objects. Call once, before the machine runs.
static void FgHarnessInit()
{
    typedef FgHarnessBinding B;
    B::evPlacer = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    B::evPace = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    B::paceTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!B::paceTimer) B::paceTimer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    B::origSwapPresent = (void*)&dtScPresent;
    B::S();
}
// A fresh machine state between cases: the rings zeroed and attached again, the keys to their defaults.
static void FgHarnessReset()
{
    FgState& s = FgHarnessBinding::S();
    FgHarnessBinding::subAsked = 0; FgHarnessBinding::subTook = 0;
    FgHarnessBinding::cardHold = 0; FgHarnessBinding::subRet = 0; FgHarnessBinding::onCard.clear();   // (HU14's card hold)
    void* mem = s.pub;
    memset(mem, 0, FgStateBytes());
    memset(&s, 0, sizeof(s));
    FgStateAttach(s, mem);
}
