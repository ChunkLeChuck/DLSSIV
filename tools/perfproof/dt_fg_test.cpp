// dt_fg_test.cpp: frame generation's machine (source/fg/fg_machine.h) on the REAL device thread, against the harness's fake
// of DXVK (dt_fake_ring.h): every case of the phase 1C map's appendix, each with a negative control (2026-09-25, phase 1C,
// stage S1).
//
//   python run_proof.py dt_fg_test [threaded]      (prove-all runs it at threaded=2 and at threaded=1)
//
// WHAT IT PROVES. The rig presents every real frame twice from the device thread's worker, I (a copy of N) and then N, and
// what it can get wrong is almost all ORDER: which image a raw copy lands in after DXVK rotated its ring, whether a raw
// submit overtakes work still in DXVK's command stream, what DXVK's device lock does when it is taken twice, whether a
// fire, a note, a Reset or a disarm on one thread lands inside a span on another. So the machine runs here unchanged, as
// FgMachine<FgHarnessBinding>, over the device thread unchanged (the same prelude dt_test.cpp uses), with dt_fake_ring.h
// standing in for DXVK. Each case is the appendix of docs/reports/fg-1c/map-final.md, named as it is there.
//
// HOW EVERY ORDERING CASE IS BUILT (dt_test.cpp:691-721, the ObsGate pattern): the worker is HELD, by a note observer (the
// harness's gate, note kind 15) or inside the fake Present (the SyncFrameLatency stand-in, RingGateHoldNext), WITH A
// COMMAND QUEUED BEHIND IT, and the case looks while it is held. A case that drains before it tests cannot see an ordering
// bug (the black sky hid behind a green harness for weeks, 2026-09-19). The render thread may block behind the hold (at
// threaded=2 the validator drains after every recorded Present, DtFrameEnd), so the look and the release are made by a
// helper thread (Later, below) and the render thread asserts on what it saw once it returns.
//
// EVERY CASE HAS A NEGATIVE CONTROL that switches the protection off and must SEE the fault; a control that sees nothing
// fails the case, because then the case proves nothing. A control is a harness flag, a key the machine already has for
// the purpose (its fault bits), or a MUTATED COPY OF A SEAM: a binding derived from FgHarnessBinding that overrides one
// static function, over which FgMachine is instantiated a second time (same state, B::S(), different seam). Never an
// edit to fg_machine.h: a protection the harness cannot switch off from outside is said to be structural, and why.
//
// THE PLACER AND THE STAMPERS run two ways, and each case says which: as REAL THREADS the harness starts (a placer loop
// calling PlacerStep and arming a high-resolution timer for the instant it returns, three stamper loops calling
// StampStep), or ON THE FAKE CLOCK (B::fakeClock), where the harness steps the time, stamps and calls PlacerStep itself,
// for the cases whose answer depends on the placer's timing. Where a case needs only a due, the harness writes it with
// the machine's own DueWrite, standing in for a placer that decided it, and says so.
//
// WHAT IS NOT HERE, AND WHY. G4 (fgPrep) and G8 (no GPU wait on fgOut) are phase 2's: the machine has no fgPrep path and
// the binding contract has no wait seam, so both are written as documented stubs that do not count toward PASS. The idle
// hook (fire point e) is not in fg_machine.h either (it is DtWorkerIdle's one DEVSTAT line, stage S7); H8's idle fire is
// therefore made by a harness note that publishes dtDone exactly as an idle worker has, then calls the machine's Fire.
// FgRigOnResetBarrier (map 2.15) is fg_rig.h's, not the machine's; H9 stands in for its stood-aside line with the
// machine's own Kill.
//
// THE CASES AND THEIR TAGS, the appendix's names (`dt_fg_test.exe 1 H11` runs one alone, for a rerun after a fix; with no
// tag every case runs, as prove-all runs it): G1 to G8, the plan's guarantees (G4 and G8 the stubs above); H1 to H15, the
// critics'; R1 to R8, "the rest, carried": R1 a foreign Reset with N pending, R2 FG_FIRE refused across a disarm, R3 the
// sleeper count, R4 stray callers, R5 a fire's failed Present, R6 the rect copy, R7 FgInjectObs exactly once, R8 the census
// over 10,000 random frames; and V1, not the appendix's, added after the verifier's review (a fire publishes the executing
// command's index, map 2.7.2); and FK1 to FK8 (2026-10-10, phase 3's stage 6, in dt_fg_shadow.h): what the fakes learned
// for the merged path, one rule a case, each with its planted violation; and M1 to M12 (stage 7, in dt_fg_merged.h): THE
// MERGED PATH, a binding that arms on the product's wire, each in both orders of 'Q'. Each prints what it saw and what its control saw, then a PASS or FAIL line (N/A where a
// control cannot bite at this setting); the table at the end repeats them, and the last line is the run's verdict.
//
// WHAT IT FOUND IN fg_machine.h ON ITS FIRST RUNS (2026-09-25), each fixed in a commit of its own that names the case: pair 0
// signalled a timeline with its starting value (every case); X0 wrote a 'Q' (H5); a warm-up pair moved the pad (H12); the
// live table's overflow answered a pair twice (H4 b); a placer eight publications behind wedged for the session (H4 c); the
// sleep returned its timer's lateness past the due, with no slack and no spin (H11); a stop's N fired at the injection, a
// drain or a hand was counted early or late, not at the stop (R7); an N whose command buffer was refused was counted twice (R8).
#include <windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <map>

// The layer's log and its latency and classifier stubs, as dt_test.cpp has them. TWO DIFFERENCES, both for the rig: the
// latency instrument's switches are ON, so dtScPresent writes latPresFrame (the frame each Present ends, which the
// harness's LatPresentObs stand-in signals) and calls LatOnPresent (whose hook stands for LatPace's sleep in the sleeper
// case); and the frame counter is the harness's to step.
static bool  telemetryOn = true;
static DWORD renderTid   = 0;
static int   rsOwnCreate = 0;
static int   gLayerLines = 0;
static void LogRecord(const char* fmt, ...)
{
    if (gLayerLines++ >= 300) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
static void LogHealth(const char* fmt, ...)
{
    if (gLayerLines++ >= 300) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
static void LogDetail(const char* fmt, ...) { (void)fmt; }
#define LOGMARK ""
#ifndef DEVSTAT
#define DEVSTAT(...) __VA_ARGS__
#endif
#ifndef DLSSIV_HOT
#define DLSSIV_HOT
#endif
static void IvtWarn(const char* fmt, ...) { (void)fmt; }
static bool gNoteRecheck = true;
#define DT_NOTE_RECHECK() gNoteRecheck
static bool gSyncOrdered = true;
#define DT_SYNC_ORDERED_ON() gSyncOrdered
enum { LAT_RING = 64 };
static bool latOn = true, latPacerOn = true;
static volatile LONG latPresFrame[LAT_RING];
DEVSTAT(alignas(8) static volatile LONGLONG latFlipRelQpc[LAT_RING]; static volatile float latFlipRelBlk[LAT_RING]; static volatile LONG latFlipRelN[LAT_RING];)
static volatile LONG frames = 0;
static void (*gLatOnPresentHook)() = nullptr;
static void LatOnPresent(LONG, LONGLONG) { if (gLatOnPresentHook) gLatOnPresentHook(); }
// THE REFRESH GUARD (next, 2026-09-26; merged into frame generation 2026-09-28): dt_res.h tests it after the pacing
// wait. Never armed here, as the rig parks it in the mod (FgRigParkGuard): a no-op, as dt_test's is.
static bool latGuard = false;
static void RgOnPresent() {}
// THE SAVE'S QUESTION (2026-10-12; source/devthread/dt_bbs.h) IS COMPILED INTO THIS HARNESS AS A BUILD WHOSE PRODUCT ARMS COMPILES
// IT: its sites in the layer's wrappers (DT_BBS_ON), its entry points ahead of the layer and its bodies behind it, as the
// mod reads them at the top and at the end of its file. dt_fg_saves.h has the cases (SV1 to SV5). A SITE SWITCHED OFF IS
// THE HARNESS'S ALONE: gBbsBlind names one (a kind DT_BBS_*, or one of dt_bbs.h's four), and every case's control sets it.
// THE LYING BUILD (-DDT_BBS_LIE=1: `python run_proof.py dt_fg_test -DDT_BBS_LIE=1 2 SV1`) answers "not written", always:
// every SV case must then FAIL at the picture.
// (gBbsLie: SV5's control, the same lie for one segment of the census, whose fresh machine puts gBbsBlind back)
#define DT_BBS_ON 1
static volatile LONG gBbsBlind = -1, gBbsLie = 0;
#ifdef DT_BBS_LIE
#define DT_BBS_BLIND(what) ((what) == 103 || gBbsBlind == (LONG)(what))
#else
#define DT_BBS_BLIND(what) (gBbsBlind == (LONG)(what) || (gBbsLie && (what) == 103))
#endif
#include "../../source/devthread/dt_bbs_top.h"
#include "../../source/devthread/devthread.h"
#include "../../source/devthread/dt_bbs.h"
static_assert(DT_BBS_B_ALL == 103, "the lying build's switch names dt_bbs.h's DT_BBS_B_ALL by its value");
// THE FAKE ASKS WHICH THREAD IS THE WORKER (DLSS-IV 1.0.8, folded in 2026-10-03): dt_fake.h counts the three device calls
// Windows' compatibility layer hooks whenever the worker makes one. The device thread's own id, as dt_test's is without
// its stand-in for that layer.
static DWORD FkWorkerTid() { return dtWorkerTid; }
static bool WriteSlot(void** vtbl, int slot, void* fn)
{
    DWORD old;
    if (!VirtualProtect(&vtbl[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    vtbl[slot] = fn;
    VirtualProtect(&vtbl[slot], sizeof(void*), old, &old);
    return vtbl[slot] == fn;
}
#include "dt_fake.h"
#include "dt_fake_dev.h"
// THE GATE BEFORE DXVK'S PRESENT, SWITCHED OFF BY SITE (fg_machine.h, WorkerPresent's two calls of B::PresentGate; L9's two
// controls in dt_fg_shadow.h): bit 0 the call ahead of a present passed through, bit 1 the one ahead of the inline present.
static volatile LONG gPresentGateOff = 0;
#define FG_PRESENT_GATE_ON(site) (!(gPresentGateOff & (1 << (site))))
// THREE GUARDS OF THE MERGED PATH'S FIRE, SWITCHED OFF ONE BY ONE (fg_machine.h, ArmedFirePresent; stage 7c): bit 0 the
// machine's state read again behind the gate, bit 1 the answer read again behind the gate, bit 2 the stand-down at a
// lost restore. The controls of M15, M16 and M20 in dt_fg_merged.h.
static volatile LONG gArmedGuardOff = 0;
#define FG_ARMED_GUARD_ON(which) (!(gArmedGuardOff & (1 << (which))))
// THE EARLY RULE C1 SWITCHED OFF (fg_machine.h, FG_C1_ON; round 9's S8): a pair still armed at the next injection is not
// forced there, so a case can hold it armed across the injection (what the development key fg_test_early = 3 did). Fresh
// clears it.
static volatile LONG gC1Off = 0;
#define FG_C1_ON() (!gC1Off)
// THE STUB'S MODELLED ANSWER MOVED (fg_machine.h, FG_STUB_GEN_US; round 9's S9): us after the handoff's retire at which the
// rig's placer models the answer's arrival (what the development key fg_test_gen_us did). P2's oracle and control, PD1 and PD5
// set it; Fresh puts back 1,500 us, the machine's own.
static volatile LONG gStubGenUs = 1500;
#define FG_STUB_GEN_US() ((LONG)gStubGenUs)
#include "../../source/fg/fg_machine.h"
#include "dt_fake_ring.h"

typedef FgHarnessBinding B;
typedef FgMachine<B> M;

// THE SPLIT'S COMPILE PROOF (2026-10-05, phase 2 of the revival, stage 8; fg_machine.h, "THE THREE TRAITS"), ONE SHAPE SINCE ROUND
// 9'S STRIP (2026-10-26). Until then a binding of the PRODUCT'S SHADOW SHAPE (false to kRig and kEngage, true to kWire) was
// compiled here beside the merged one; that shape went from the machine, which now refuses any binding that cannot arm
// (fg_machine.h's static_assert). What stays: THE MERGED PATH'S SEAM LIST AND NOTHING MORE (2026-10-10, phase 3, stage 7;
// the build map's machine-7 "COMPILE" and pgk-14). A binding that arms on the wire and is NOT an instrument's: the shape
// the product compiles (FgPA). It has the wire's seams and the ones the merged path adds (its own state, "is this image
// there", the back buffer's mark and question, the restore's copy, FgPace's wait and its sleeper count, the frame end, the
// pacer's and the worker's wakes, the placer's poll), and LACKS EVERY INSTRUMENT'S SEAM ON PURPOSE (the plants, the
// samples, the tag and its patch, shape D's stretches and fill, the marker's lock ticks, the spacing, the GPU tick): an
// instrument's statement left unfenced in the merged path, or in the armed lines the merged path shares with the rig
// (FgSwapPresent's tail, FgPace, the frame end), does not compile here. Its seams do nothing and no case runs it:
// dt_fg_merged.h's cases run the same machine over ShArmB, whose seams are the fake's. tools/test-fg-split.py compiles this
// file, reads its object for what the compiler instantiated over it, and proves the proof with two controls, each of
// which MUST NOT COMPILE: this binding saying FALSE to kEngage (FG_SPLIT_CONTROL 1: the machine's static_assert refuses a
// binding that cannot arm) and FALSE to kWire (3: the stub's hand writes 'Q' with ONE argument, the pair's ordinal, and this
// shape has the wire's two-argument 'Q' alone, so the trait fences the stub off it). (TRUE to kRig, 4, went in round 9's S9
// with the instrument's records: no instrument's seam is left for this shape to lack.)
struct FgArmedShape
{
#if defined(FG_SPLIT_CONTROL) && FG_SPLIT_CONTROL == 1
    static constexpr bool kRig = false, kEngage = false, kWire = true;    // THE CONTROL: a binding that cannot arm must not compile (the machine's static_assert)
#elif defined(FG_SPLIT_CONTROL) && FG_SPLIT_CONTROL == 3
    static constexpr bool kRig = false, kEngage = true, kWire = false;    // THE CONTROL: the wire's seams WITHOUT the wire's trait must not compile (the stub's one-argument 'Q')
#else
    static constexpr bool kRig = false, kEngage = true, kWire = true;
#endif
    static FgState& S() { static FgState s; return s; }
    static LONGLONG Now() { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
    static LONGLONG Qpf() { return 10000000; }
    static void Line(const char* fmt, ...) { (void)fmt; }
    // render-thread facts
    static LONG Frames() { return 0; }
    static LONG CamJumps() { return 0; }
    static LONG HdrEncodes() { return 0; }
    static bool HdrOn() { return false; }
    static bool HdrView() { return false; }
    static ULONGLONG RefreshPacked() { return 0; }
    static int  FfCap() { return 0; }
    static bool Limiter() { return false; }
    static void RenderTick() {}
    static DWORD RenderTid() { return 0; }
    static void*& OrigSwapPresentSlot() { static void* slot = nullptr; return slot; }
    // worker facts and spans
    static LONG SentN() { return 0; }
    static LONG LatOrd(LONG n) { (void)n; return 0; }
    static bool CbTake(int n) { (void)n; return true; }
    static void DevLock() {}
    static void DevUnlock() {}
    static void Flush() {}
    static ULONGLONG BBImage() { return 0; }
    static void QLock() {}
    static void QUnlock() {}
    static void RecBegin() {}
    static void RecCopyToImg(ULONGLONG bb, int img) { (void)bb; (void)img; }
    static void RecSignal(int tl, LONG v) { (void)tl; (void)v; }
    static bool RecSubmit() { return true; }
    static bool RetireWait(DWORD ms) { (void)ms; return true; }
    // timelines, the pipe, events
    static ULONGLONG TlValue(int tl) { (void)tl; return 0; }
    static bool TlWait(int tl, ULONGLONG v, DWORD ms) { (void)tl; (void)v; (void)ms; return false; }
    static void WriteQ(LONG n, DWORD flags) { (void)n; (void)flags; }
    static void WriteG(LONG n, DWORD kind) { (void)n; (void)kind; }
    static void WakePlacer() {}
    // the wire's own (stage 9)
    static FgShadowState& X() { static FgShadowState x; return x; }
    static bool MayEngage() { return false; }
    static bool PrepGuard(LONG n) { (void)n; return false; }
    static bool HasHudless() { return false; }                    // phase 3, lever 1: the set has no HUD-less image (the game's shape)
    static bool Pipelined() { return false; }
    static bool QAtHand() { return false; }                       // phase 3, lever 2: the development key (off): 'Q' rides behind the injection's body
    static void PresentGate() {}                                  // the gate before DXVK's Present (every binding's, since the freeze of phase 2's first sitting)
    static void* DxvkBare(void* p) { return p; }                  // DXVK's bare Present for the install (every binding's, since the fold of 2026-10-10)
    static void InjectSeen(const DWORD* payload, DWORD bytes) { (void)payload; (void)bytes; }
    static LONGLONG WorkT() { return 0; }
    static void WorkAdd(LONGLONG t0) { (void)t0; }
    static FgArmedState& A() { static FgArmedState a; return a; }
    static bool HomeWaited(LONG n) { (void)n; return false; }     // 2026-10-14: the four seams of "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT"
    static void CardClass(int up) { (void)up; }
    static bool OrdWait(LONG n) { (void)n; return false; }
    static void OrdSignal(LONG n) { (void)n; }
    static bool PlaceStamps() { return false; }                   // 2026-10-17, stage 3: the writes' GPU stamps (none here)
    static bool ImgOk(int img) { (void)img; return false; }
    static void BackMark() {}
    static bool BackWritten() { return true; }
    static int  HudWant() { return 0; }                           // phase 3, stage 15: the HUD-less picture's three seams (the render thread's)
    static void HudDraw() {}
    static void HudUndo() {}
    static bool HudSrc() { return false; }                        // 2026-10-11, the second-target form: its two seams on the worker
    static void RecCopyHudToImg(int img) { (void)img; }
    static void RecCopyFromImg(int img, ULONGLONG bb) { (void)img; (void)bb; }
    static LONG PaceSleptN() { return 0; }
    static int  PaceWait(LONGLONG dueQpc) { (void)dueQpc; return 0; }
    static void LatEnd(LONG f) { (void)f; }
    static void WakePace() {}
    static bool PlacerPoll() { return false; }
};
typedef FgMachine<FgArmedShape> MAS;
static void* volatile gSplitSink;
static void* const kArmedShapeEntries[] = {
    (void*)&MAS::Install, (void*)&MAS::Unchained, (void*)&MAS::Kill, (void*)&MAS::Stop, (void*)&MAS::SwapPresent, (void*)&MAS::WorkerPresent,
    (void*)&MAS::NoteHand, (void*)&MAS::NoteHudless, (void*)&MAS::NoteFire, (void*)&MAS::PresentObs, (void*)&MAS::InjectObs, (void*)&MAS::OnArm, (void*)&MAS::OnDisarm,
    (void*)&MAS::StampStep, (void*)&MAS::BeforeRelease, (void*)&MAS::BeforeBuild, (void*)&MAS::PlacerRun, (void*)&MAS::BeforeReset, (void*)&MAS::OwnsEnd, (void*)&MAS::EndFor };
static int ArmedEntries()
{
    int n = 0;
    for (void* e : kArmedShapeEntries) { gSplitSink = e; if (gSplitSink) n++; }
    return n;
}
// THE HARNESS'S NOTE KINDS: the gate (moved from 6 to 15 in dt_test.cpp for the same reason, 6 is the rig's FG_HUDLESS),
// and a note that runs a harness function ON THE WORKER at its place in the stream (H8's idle fire, stray callers, a stop
// between two notes). Neither is a kind the mod registers.
enum : DWORD { kHarnessRun = 14, kHarnessGate = 15 };
static_assert(kHarnessRun > DT_NOTE_FG_FIRE && kHarnessGate < DT_NOTE_N, "the harness's notes sit above the rig's and below DT_NOTE_N");

static int gCfg = 2;
static FakeDev* gFd; static IDirect3DDevice9* D; static FakeRingSC* gSc; static IDirect3DSwapChain9* SC;
static IDirect3DSurface9* gBb = nullptr;         // the ring's surface 0: the stable object the game draws into
static IDirect3DSurface9* gOtherRt = nullptr;    // a render target that is not the ring's
static void* gDxvkPresent0 = nullptr;            // dtScPresentO as DtInstall left it: FakeRingSC::Present, DXVK's own
static const DWORD kGen = 0xFFFF00FFu, kBlack = 0xFF000000u, kSentinel = 0xDEAD0001u;
enum : LONG { kHarnessWhy = 99 };              // the reason an answer the HARNESS wrote carries (AnswerAs, below)

// ------------------------------------------------------------------ the result table
struct Row { std::string name, result, control; };
static std::vector<Row> gRows;
static int gErr0 = 0;
static RingStats gSt0;
static std::string gCtl;          // what this case's control saw, for its summary line
static bool gStub = false;
// A ROW THAT CANNOT BE A PROOF AT THIS SETTING (2026-09-25, after the verifier's review of G2): its assertions still run and an
// error still fails the row and the run, but a clean row reads N/A, not PASS, because its control cannot bite here.
static bool gNa = false;
static char gBuf[512];
static const char* Fmt(const char* fmt, ...) { va_list a; va_start(a, fmt); vsnprintf(gBuf, sizeof(gBuf), fmt, a); va_end(a); return gBuf; }
// THE FAKE'S RULES (dt_fake_ring.h, RingStats): each must stay 0 in every clean case; a control that breaks one on purpose
// puts it back with ViolRestore, so what is left at a case's end is a fault nobody asked for.
// (stage 6 of phase 3, 2026-10-10: the helper's set and the queue's order have rules of their own, counted apart so a
// case's line can say which; they are held to 0 like the rest)
static LONG ViolSet(const RingStats& s)
{ return s.setDead + s.setVirgin + s.setAbsent + s.setNotAcquired + s.setAcquireOrder + s.genBusy + s.genUnanswered + s.genReset + s.csPastWait + s.presentAfterRefused + s.backLost; }
static LONG Viol(const RingStats& s)
{ return s.qD3D9 + s.qRecursive + s.rawNoQLock + s.recNoBegin + s.cbUnderLock + s.cbUntaken + s.tlBackwards + s.pipeReentry + s.badHandle + s.badSurface + s.deadImage + ViolSet(s); }
static void ViolRestore(const RingStats& s)
{
    gRingSt.qD3D9 = s.qD3D9; gRingSt.qRecursive = s.qRecursive; gRingSt.rawNoQLock = s.rawNoQLock; gRingSt.recNoBegin = s.recNoBegin;
    gRingSt.cbUnderLock = s.cbUnderLock; gRingSt.cbUntaken = s.cbUntaken; gRingSt.tlBackwards = s.tlBackwards; gRingSt.pipeReentry = s.pipeReentry;
    gRingSt.badHandle = s.badHandle; gRingSt.badSurface = s.badSurface; gRingSt.deadImage = s.deadImage;
    gRingSt.setDead = s.setDead; gRingSt.setVirgin = s.setVirgin; gRingSt.setAbsent = s.setAbsent; gRingSt.setNotAcquired = s.setNotAcquired;
    gRingSt.setAcquireOrder = s.setAcquireOrder; gRingSt.genBusy = s.genBusy; gRingSt.genUnanswered = s.genUnanswered; gRingSt.genReset = s.genReset;
    gRingSt.csPastWait = s.csPastWait; gRingSt.presentAfterRefused = s.presentAfterRefused; gRingSt.backLost = s.backLost;
}
// (a suffix for a case that is run a second time in another arm: lever 2 runs the product's cases in both orders of 'Q')
static const char* gCaseArm = "";
static void CaseBegin(const char* name) { gErr0 = gErr; gSt0 = gRingSt; gCtl.clear(); gStub = false; gNa = false; printf("\n  -- %s%s\n", name, gCaseArm); }
static void CaseEnd(const char* name)
{
    const RingStats& s = gRingSt;
    if (Viol(s) != Viol(gSt0))
        Err("%s: the fake's rules were broken outside a control: queue %ld, recursive %ld, unlocked submit %ld, no begin %ld, taken under lock %ld,"
            " untaken %ld, backwards %ld, pipe %ld, handle %ld, surface %ld, dead image %ld", name,
            s.qD3D9 - gSt0.qD3D9, s.qRecursive - gSt0.qRecursive, s.rawNoQLock - gSt0.rawNoQLock, s.recNoBegin - gSt0.recNoBegin,
            s.cbUnderLock - gSt0.cbUnderLock, s.cbUntaken - gSt0.cbUntaken, s.tlBackwards - gSt0.tlBackwards, s.pipeReentry - gSt0.pipeReentry,
            s.badHandle - gSt0.badHandle, s.badSurface - gSt0.badSurface, s.deadImage - gSt0.deadImage);
    if (ViolSet(s) != ViolSet(gSt0))
        Err("%s: the rules of the helper's set and of the queue's order were broken outside a control: an image of a released set %ld, a set image read before"
            " anything wrote it %ld, a slot the set lacks %ld, not acquired %ld, acquire and release out of order %ld, GEN read while the helper wrote it %ld,"
            " GEN read with no answer of its pair's %ld, a reset dispatch's picture read %ld, the stream past a queued wait %ld, a Present after a refused submit %ld,"
            " the game's half-drawn back buffer lost at a fire %ld", name,
            s.setDead - gSt0.setDead, s.setVirgin - gSt0.setVirgin, s.setAbsent - gSt0.setAbsent, s.setNotAcquired - gSt0.setNotAcquired,
            s.setAcquireOrder - gSt0.setAcquireOrder, s.genBusy - gSt0.genBusy, s.genUnanswered - gSt0.genUnanswered, s.genReset - gSt0.genReset,
            s.csPastWait - gSt0.csPastWait, s.presentAfterRefused - gSt0.presentAfterRefused, s.backLost - gSt0.backLost);
    const bool pass = gErr == gErr0;
    const char* res = gStub ? "STUB" : !pass ? "FAIL" : gNa ? "N/A" : "PASS";
    gRows.push_back({ std::string(name) + gCaseArm, res, gCtl });
    printf("  %s: %s\n", res, gCtl.c_str());
}
static void Ctl(const char* fmt, ...) { char b[512]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a); if (!gCtl.empty()) gCtl += "; "; gCtl += b; }

// ------------------------------------------------------------------ threads the harness starts
static HANDLE Spawn(LPTHREAD_START_ROUTINE f, void* a = nullptr) { return CreateThread(nullptr, 0, f, a, 0, nullptr); }
// A JOIN THAT CANNOT HANG THE RUN (dt_fake_ring_test.cpp's): a helper still running after 10 s is stuck on something
// nobody will release, and every later case would stall behind it.
static void Join(HANDLE h, const char* what = "a helper thread")
{
    if (WaitForSingleObject(h, 10000) != WAIT_OBJECT_0) { printf("  HUNG: %s did not finish within 10 s\nFAILED\n", what); ExitProcess(3); }
    CloseHandle(h);
}
// (2026-10-24, round 5 round 2, F: THE WAITS' OWN TIME BY TAG, real milliseconds and counts, for a desk's look at where a slow run's time goes:
// WaitForSaid prints them. Tag 0 every untagged wait)
static volatile LONGLONG gWfTicks[8]; static volatile LONG gWfN[8];
template <class P> static bool WaitFor(P ready, DWORD ms, int tag = 0)
{
    LARGE_INTEGER q0; QueryPerformanceCounter(&q0);
    const ULONGLONG t = GetTickCount64() + ms;
    bool ok = true;
    while (!ready()) { if (GetTickCount64() >= t) { ok = false; break; } Sleep(0); }
    LARGE_INTEGER q1; QueryPerformanceCounter(&q1);
    const int i = tag & 7; InterlockedExchangeAdd64(&gWfTicks[i], q1.QuadPart - q0.QuadPart); _InterlockedIncrement(&gWfN[i]);
    return ok;
}
static void WaitForSaid(const char* what)
{
    LARGE_INTEGER f; QueryPerformanceFrequency(&f);
    printf("  THE HARNESS'S WAITS (%s), real ms and count by tag:", what);
    for (int i = 0; i < 8; i++) if (gWfN[i]) printf(" [%d] %.0f ms / %ld", i, (double)gWfTicks[i] * 1000.0 / (double)f.QuadPart, gWfN[i]);
    printf("\n");
}
// LATER: the look-and-release helper of every held case. It waits for `until` (the worker is held), waits `afterMs` more
// so the render thread has recorded what it records behind the hold, runs `then` (the look, then the release), and
// says whether the hold was ever reached.
struct Later { bool (*until)(); void (*then)(); DWORD waitMs, afterMs; volatile LONG done; bool met; };
static DWORD WINAPI LaterMain(void* a)
{
    Later* l = (Later*)a;
    l->met = WaitFor([l] { return l->until(); }, l->waitMs);
    Sleep(l->afterMs);
    l->then();
    _InterlockedExchange(&l->done, 1);
    return 0;
}

// ------------------------------------------------------------------ the gate, and a harness function run on the worker
static HANDLE gGateEv = nullptr;
static volatile LONG gGateIn = 0;
static void ObsGate(DWORD, const DWORD*, DWORD) { _InterlockedExchange(&gGateIn, 1); WaitForSingleObject(gGateEv, INFINITE); _InterlockedExchange(&gGateIn, 0); }
static bool Gate() { const DWORD x = 0x6A7Eu; const bool ok = DtNote(kHarnessGate, &x, 4); DtPublishFenced(); return ok; }
// A RUN NOTE: [fn, arg]. The observer hands the function the note's own index and the index after it, which is where an
// idle worker's dtDone would stand once it had run the note and found nothing more (DtWorkerMain: DtWorkerDone(r), then
// DtWorkerIdle(r)).
typedef void (*RunFn)(DWORD own, DWORD next, DWORD arg);
static void ObsRun(DWORD, const DWORD* p, DWORD bytes)
{
    const DWORD own = (DWORD)((p - 2) - dtRing);
    DWORD next = own + 2 + (bytes + 3) / 4;
    if (next >= dtRingDw) next = 0;
    ((RunFn)(UINT_PTR)p[0])(own, next, p[1]);
}
static bool Run(RunFn f, DWORD arg = 0) { const DWORD pl[2] = { (DWORD)(UINT_PTR)f, arg }; const bool ok = DtNote(kHarnessRun, pl, 8); DtPublishFenced(); return ok; }

// ------------------------------------------------------------------ what the rig chains to: stand-ins for the mod's observers
// Install saves these as "previous" (map 2.4 steps 4 to 6), so every case proves the chaining, not a stub of it.
//   PrevInject   IvtInjectNoteW: its scope guard's decrement is "the whole contract" (DlssIvTransport.h:5505-5511), so it
//                decrements the pending count the harness raised when it posted the note, and counts its calls.
//   PrevPresent  LatPresentObs, whose first statement is LatEndSignal (DlssIvTransport.h:3460-3463): the frame end of a
//                present the rig does not own.
//   PrevArm, PrevDisarm, PrevObs82: TrkOnArm, TrkOnDisarm, TrkObsDraw, counted.
static volatile LONG gInjPending = 0, gInjCalls = 0, gLatObsCalls = 0, gArmCalls = 0, gDisarmCalls = 0, gObs82Calls = 0;
// THE INJECTION'S BODY (2026-10-10, phase 3, lever 2). In the mod the saved observer IS the transport's body: it writes the
// frame message, makes the frame's submits and moves the sent count, all on the worker. The product's shadow cases
// (dt_fg_shadow.h) install a body here that does the same, so that what the machine does BEHIND the saved observer
// (it writes 'Q' there) is run as the game runs it. Null for every other case: the saved observer only counts.
static void (*gInjBody)(const DWORD* payload, DWORD bytes) = nullptr;
static void PrevInject(DWORD, const DWORD* payload, DWORD bytes) { _InterlockedIncrement(&gInjCalls); _InterlockedDecrement(&gInjPending); if (gInjBody) gInjBody(payload, bytes); }
// (gPrevLatEnd, 2026-10-13: dt_fg_rates.h's simulation of the card's queue takes the frame end of a present the machine does
// not own as it takes the machine's own, to signal it when that present is shown. Null for every other case.)
static void (*gPrevLatEnd)(LONG f) = nullptr;
static void PrevPresent(LONG k) { _InterlockedIncrement(&gLatObsCalls); const LONG f = latPresFrame[k & (LAT_RING - 1)]; if (gPrevLatEnd) gPrevLatEnd(f); else B::LatEnd(f); }
static void PrevArm() { _InterlockedIncrement(&gArmCalls); }
static void PrevDisarm() { _InterlockedIncrement(&gDisarmCalls); }
static void PrevObs82(const DWORD*) { _InterlockedIncrement(&gObs82Calls); }

template <class MM> static void InstallRig()
{
    dtNoteObs[DT_NOTE_INJECT] = PrevInject; dtPresentObs = PrevPresent; dtArmObs = PrevArm; dtDisarmObs = PrevDisarm; dtObs[82] = PrevObs82;
    dtNoteObs[kHarnessGate] = ObsGate; dtNoteObs[kHarnessRun] = ObsRun;
    B::origSwapPresent = (void*)&dtScPresent;
    if (!MM::Install()) Err("the rig's Install refused on a fresh state");
}
// Only between cases, with the worker idle and no harness thread running: the harness's own, not the machine's (the rig
// is never uninstalled within a session, map 2.4). It puts back what the install replaced so the next Install on a fresh
// state saves the harness's stand-ins as "previous", never the rig's own observers (O-N3's recursion).
static void UninstallRig()
{
    dtNoteObs[DT_NOTE_FG_HAND] = dtNoteObs[DT_NOTE_FG_HUDLESS] = dtNoteObs[DT_NOTE_FG_FIRE] = nullptr;
    dtNoteObs[DT_NOTE_INJECT] = nullptr; dtPresentObs = nullptr; dtArmObs = nullptr; dtDisarmObs = nullptr;
    static const int ck[] = { 34, 35, 37, 43, 81, 82, 83, 84 };
    for (int i : ck) dtObs[i] = nullptr;
    dtScPresentO = gDxvkPresent0;
    B::origSwapPresent = (void*)&dtScPresent;
}

// ------------------------------------------------------------------ the placer and the stampers as real threads
static volatile LONG gThrQuit = 0;
static HANDLE gThrQuitEv = nullptr;
static HANDLE gThr[6] = {};                      // (five since stage 7: the merged machine's four stampers and its placer)
static int gThrN = 0;
// THE ANSWER LOG: every answer the placer publishes, seen by the thread that calls PlacerStep right after the step (the
// machine's answer ring is 8 deep, which is its own design for G5's read of p-1; a log is what "exactly once" needs).
// A SLOT THAT MOVED BY MORE THAN ONE LAP between two scans (p to p + 16) means the log missed p + 8's answer: counted, so
// a case can say its log was complete (the census leans on the placer's own counters where it was not).
static std::vector<std::pair<LONG, LONG>> gAnsLog;
static LONG gAnsSlotSeen[FG_RING][3];
static LONG gAnsLogMissed = 0;
static void AnsLogReset() { gAnsLog.clear(); gAnsLogMissed = 0; for (int i = 0; i < FG_RING; i++) { gAnsSlotSeen[i][0] = -1; gAnsSlotSeen[i][1] = -1; gAnsSlotSeen[i][2] = -1; } }
static void AnsLogScan()
{
    FgState& s = B::S();
    for (int i = 0; i < FG_RING; i++)
    {
        const LONG q = ReadAcquire(&s.ans[i].seq);
        if (q < 0) continue;
        FgAns a = {};
        if (!M::Read(s.ans[i], q, &a)) continue;
        if (a.p == gAnsSlotSeen[i][0] && a.val == gAnsSlotSeen[i][1] && a.why == gAnsSlotSeen[i][2]) continue;
        if (gAnsSlotSeen[i][0] >= 0 && a.p > gAnsSlotSeen[i][0] + FG_RING) gAnsLogMissed++;
        gAnsSlotSeen[i][0] = a.p; gAnsSlotSeen[i][1] = a.val; gAnsSlotSeen[i][2] = a.why;
        if (a.why != kHarnessWhy) gAnsLog.push_back({ a.p, a.val });      // the placer's answers only
    }
}
// How many times the placer answered p, by the log.
static int AnsTimes(LONG p) { int n = 0; for (const auto& e : gAnsLog) n += e.first == p; return n; }
static LONG AnsVal(LONG p) { LONG v = -1; for (const auto& e : gAnsLog) if (e.first == p) v = e.second; return v; }
template <class MM> static DWORD WINAPI StamperMain(void* a)
{
    const int tl = (int)(INT_PTR)a;
    LONG last = (LONG)B::TlValue(tl);
    while (!gThrQuit) { if (MM::StampStep(tl, &last, 250) && tl == FG_TL_IN) SetEvent(B::evPlacer); }   // the IN stamper wakes the placer (map 2.6)
    return 0;
}
template <class MM> static DWORD WINAPI PlacerMain(void*)
{
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!timer) timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    const HANDLE h[3] = { B::evPlacer, timer, gThrQuitEv };
    for (;;)
    {
        WaitForMultipleObjects(3, h, FALSE, 250);
        if (gThrQuit) break;
        // THE EXACT RAISE (2026-10-04): the machine's own pass (the step, a spin to a raise that is near, the instant to set
        // the timer for), as fg_rig.h's placer thread runs it
        const LONGLONG next = MM::PlacerRun();
        AnsLogScan();
        if (next)
        {
            LONGLONG left = next - B::Now(); if (left < 0) left = 0;
            LARGE_INTEGER due; due.QuadPart = -(LONGLONG)((double)left * 1e7 / (double)B::Qpf()); if (due.QuadPart == 0) due.QuadPart = -1;
            SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
        }
    }
    CloseHandle(timer);
    return 0;
}
template <class MM> static void StartThreads(bool placer, bool stampers)
{
    gThrQuit = 0; ResetEvent(gThrQuitEv); gThrN = 0;
    if (stampers) for (int tl = FG_TL_IN; tl <= FG_TL_END; tl++) gThr[gThrN++] = Spawn(&StamperMain<MM>, (void*)(INT_PTR)tl);
    if (placer) gThr[gThrN++] = Spawn(&PlacerMain<MM>);
}
static void StopThreads()
{
    if (!gThrN) return;
    _InterlockedExchange(&gThrQuit, 1); SetEvent(gThrQuitEv);
    for (int i = 0; i < gThrN; i++) Join(gThr[i], "the placer or a stamper");
    gThrN = 0;
}
// ON THE FAKE CLOCK the harness stamps and steps: every timeline value not yet stamped is stamped now, at clockNow.
static LONG gLast[FG_TL_N];
template <class MM = M> static void StampNow() { for (int tl = FG_TL_IN; tl <= FG_TL_END; tl++) while (MM::StampStep(tl, &gLast[tl], 0)) {} }
template <class MM = M> static LONGLONG PlaceNow() { const LONGLONG n = MM::PlacerStep(B::Now()); AnsLogScan(); return n; }
static void ClockAdd(double ms) { B::clockNow += (LONGLONG)(ms * (double)B::Qpf() / 1000.0); }

// ------------------------------------------------------------------ the game's side
static DWORD gTag = 0x00A00000u;
static DWORD NewTag() { return ++gTag; }
// A frame's facts as hkEndScene and the transport leave them (map 2.5 step 3): frames moves at EndScene, and "sent" means
// the injection of the frame this present ends was sent (latOrd[ivtSentN & 63] == fEnd).
static void Facts(bool sent = true)
{
    frames++; B::frames = frames;
    // (with a body installed in the saved injection observer the sent count is the WORKER's, moved inside that body as the
    // transport moves it: the product's shadow cases, since lever 2)
    if (sent && !gInjBody) { B::sentN++; B::latOrd[B::sentN & 63] = frames - 1; }
}
static void Fill(DWORD tag) { D->ColorFill(gBb, nullptr, tag); }
template <class MM = M> static HRESULT Present(const RECT* s = nullptr, const RECT* t = nullptr) { return MM::SwapPresent(SC, s, t, nullptr, nullptr, 0); }
template <class MM = M> static DWORD Frame(bool sent = true) { Facts(sent); const DWORD t = NewTag(); Fill(t); Present<MM>(); return t; }
static void Drain() { D->TestCooperativeLevel(); }
// The render thread's own notes, built exactly as FgSwapPresent builds them, for the cases that need one without the rest
// (G1's control, H4's FG_HAND with no arm, H13's stop between the notes and the present).
static bool RecordDrain()
{
    FgState& s = B::S();
    if (!s.r.lastEngaged) return true;
    FgFirePayload fp = { FG_PT_D, s.r.lastP, s.r.lastGen };
    s.r.lastEngaged = false;
    return DtNote(DT_NOTE_FG_FIRE, &fp, sizeof(fp));
}
static bool RecordHand(DWORD flags = 0)
{
    FgState& s = B::S();
    FgHandPayload h = {};
    h.k = dtPresRecN + 1; h.gen = dtArmCount; h.p = s.r.nextP; h.fEnd = B::frames - 1; h.flags = flags; h.tEntry = B::Now();
    if (!DtNote(DT_NOTE_FG_HAND, &h, sizeof(h))) return false;
    s.r.lastP = h.p; s.r.lastGen = h.gen; s.r.lastEngaged = true; s.r.nextP++;
    return true;
}
static bool RecordHudless() { FgState& s = B::S(); FgPairPayload pp = { dtPresRecN + 1, s.r.lastGen, s.r.lastP }; return DtNote(DT_NOTE_FG_HUDLESS, &pp, sizeof(pp)); }
static HRESULT RecordPresent(const RECT* s = nullptr, const RECT* t = nullptr) { return dtScPresent(SC, s, t, nullptr, nullptr, 0); }
// FG_FIRE(PLACER) as FgPace records it when the placer wakes it (point b), and published at once (DtPublishFenced).
static bool FireNote(LONG why = FG_PT_B)
{
    FgState& s = B::S();
    FgFirePayload fp = { why, s.r.lastP, s.r.lastGen };
    const bool ok = DtNote(DT_NOTE_FG_FIRE, &fp, sizeof(fp));
    DtPublishFenced();
    return ok;
}
// THE HARNESS STANDING IN FOR THE PLACER, where a case needs a decision and not the placer's timing: an answer record and
// the machine's own DueWrite, with the open pair's generation. The answer is written with the machine's Publish and a
// reason of its own (kHarnessWhy), NOT through Answer, so the placer's counters and the answer log count the placer alone:
// "every p answered exactly once" is a claim about PlacerStep, and a harness answer must never be mistaken for one.
static void AnswerAs(LONG p, bool withheld = false) { FgAns a = {}; a.p = p; a.val = withheld ? 2 * p + 1 : 2 * p; a.why = kHarnessWhy; M::Publish(B::S().ans[p & (FG_RING - 1)], p, a); }
static void DueI(LONG p, LONG verdict, DWORD why = 0) { FgState& s = B::S(); M::DueWrite(s.dueI, p, dtArmCount, verdict, why, B::Now()); }
static void DueN(LONG p) { FgState& s = B::S(); M::DueWrite(s.dueN, p, dtArmCount, FG_DUE_FIRE, 0, B::Now()); }
// THE PRESENTED LOG: every blit that has RUN, in the order it ran; a flush first, so every Present made so far has run.
static std::vector<RingPresented> Shown() { B::Flush(); return RingPresentedLog(); }
static std::string TagList(const std::vector<RingPresented>& v, size_t from)
{
    std::string s;
    for (size_t i = from; i < v.size(); i++) { char b[16]; snprintf(b, sizeof(b), "%s%lX", s.empty() ? "" : " ", v[i].tag); s += b; }
    return s.empty() ? "(none)" : s;
}
static bool ShownHas(const std::vector<RingPresented>& v, size_t from, DWORD tag) { for (size_t i = from; i < v.size(); i++) if (v[i].tag == tag) return true; return false; }
static int ShownAt(const std::vector<RingPresented>& v, size_t from, DWORD tag) { for (size_t i = from; i < v.size(); i++) if (v[i].tag == tag) return (int)i; return -1; }

// ------------------------------------------------------------------ a fresh rig for every case
// (the rig's keys a case set here, shape, mode, probe, kfix, gen, spacing, hold, abba, abbaPace and fault, went with them in
// round 9's S9: images is the fake swapchain's own shape; the modelled answer is gStubGenUs)
struct Opt
{
    int images = 3;
    bool fakeClock = false;
};
static void RingShape(int images, D3DSWAPEFFECT* e, UINT* bbc)
{
    switch (images)
    {
    case 1: *e = D3DSWAPEFFECT_DISCARD; *bbc = 1; break;   // 1 image: DISCARD, one back buffer, windowed (map 2.14)
    case 2: *e = D3DSWAPEFFECT_FLIP; *bbc = 1; break;      // 2
    case 4: *e = D3DSWAPEFFECT_DISCARD; *bbc = 3; break;   // 4
    default: *e = D3DSWAPEFFECT_DISCARD; *bbc = 2; break;  // 3, the owner's
    }
}
static D3DPRESENT_PARAMETERS ResetParams()
{
    D3DPRESENT_PARAMETERS p = {}; p.BackBufferWidth = 1920; p.BackBufferHeight = 1080; p.BackBufferFormat = D3DFMT_X8R8G8B8;
    p.BackBufferCount = gRing.bbc; p.SwapEffect = gRing.effect; p.Windowed = gRing.windowed;
    return p;
}
// THE LAYER ARMS after 300 presents that worked (DtMaybeArm); every case runs armed unless it says otherwise.
static bool EnsureArmed()
{
    // Through the rig once it is installed (unarmed, FgSwapPresent passes straight to dtScPresent); before, the vtable's.
    for (int i = 0; i < 400 && !dtArmed; i++) { Facts(false); if (B::S().ptr.installed) Present(); else SC->Present(nullptr, nullptr, nullptr, nullptr, 0); }
    return dtArmed;
}
// A CASE'S OWN WORK BETWEEN THE CLEAN STATE AND THE MACHINE'S INSTALL (2026-10-10, L20): the layer is armed, nothing of a
// machine is installed and dtScPresentO is the fake's own Present. Null for every case but the one that sets it.
static void (*gFreshBeforeInstall)() = nullptr;
template <class MM = M> static void Fresh(const Opt& o = Opt())
{
    StopThreads();
    if (!EnsureArmed()) Err("the layer did not arm");
    Drain();
    UninstallRig();
    B::fakeClock = false;
    gRing.presentCs = RING_CS_LAZY; gRing.cbFailNext = 0; gRing.submitFailNext = 0; gRing.signalInside = nullptr; gRing.drawTag = 0; gRing.pipeInside = nullptr;
    gRing.submitFailSkip = 0; gRing.csGate = nullptr; RingSetOff();        // (stage 6: no gate on the stream and no set's rules until a case of the wire's asks)
    gRing.back.on = 0;                                                     // (stage 7: nor the back buffer's save rule, until a merged case asks)
    B::Flush();                                                            // anything left in the command stream runs now
    D3DSWAPEFFECT e; UINT bbc; RingShape(o.images, &e, &bbc);
    RingConfigure(e, bbc, TRUE);                                           // a quiescent shape change (dt_fake_ring.h)
    RingClearLogs(); { EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); LeaveCriticalSection(&gFkLock); }
    // THE TIMELINES START AGAIN WITH THE STATE: p restarts at 1 in a fresh FgState, and a timeline signal must exceed the
    // value (the fake counts one that does not), so the fake's semaphores are new too, as the mod's are per session.
    AcquireSRWLockExclusive(&gRing.tlLock); for (int t = 0; t < FG_TL_N; t++) gRing.tl[t] = 0; ReleaseSRWLockExclusive(&gRing.tlLock);
    for (int t = 0; t < FG_TL_N; t++) gLast[t] = 0;
    FgHarnessReset();
    FgState& s = B::S();
    gC1Off = 0; gStubGenUs = 1500; B::tlInLate = 0;
    frames = 10; B::frames = frames; B::sentN = 0; memset((void*)B::latOrd, 0, sizeof(B::latOrd));
    B::camJumps = 0; B::hdrEncodes = 0; B::hdrOn = false; B::hdrView = false; B::refreshPacked = 0; B::ffCap = 0; B::limiter = false; B::paceSleptN = 0;
    gInjPending = 0; AnsLogReset();
    for (int i = 0; i < FG_IMG_N; i++) RingSetTag(gRing.rigImg[i], kSentinel + 0x100u * (DWORD)i);
    if (gFreshBeforeInstall) gFreshBeforeInstall();
    InstallRig<MM>();
    s.wt.want = FG_WANT_ON;
    if (o.fakeClock) { LARGE_INTEGER q; QueryPerformanceCounter(&q); B::clockNow = q.QuadPart; B::fakeClock = true; }
}
// A first pair, answered, so the next pair's G5 test finds its predecessor's answer (map 2.5) and is not nosource.
template <class MM = M> static void Warm() { Frame<MM>(); Drain(); AnswerAs(B::S().r.lastP); }

// ON THE FAKE CLOCK, a pair 3.6 ms after the last: the frame, its IN stamp, the placer's take, then 1.6 ms (past the
// modelled 2p arrival, 1.5 ms) and the placer's next step: the pair answered and its I verdict raised.
// With fill false the frame draws nothing, so the first fire point its commands reach is FgSwapPresent's DRAIN (a fill
// is a ColorFill, whose replay is a checkpoint, point a).
static void FakePair(bool fill = true)
{
    ClockAdd(2.0);
    if (fill) Frame(); else { Facts(); Present(); }
    Drain(); StampNow(); PlaceNow(); ClockAdd(1.6); PlaceNow();
}
// n of them: the render thread's facts settle (four periods in its ring, the engage's cut behind it), and every pair is
// answered before the next FG_HAND, as the real placer answers it at its arrival.
static void FakeFrames(int n) { for (int i = 0; i < n; i++) FakePair(); }

// ---------------------------------------------------------------- G1, the handoff reads N's final back buffer
// WHAT IT SHOWS (plan 3.3, map 2.5 step 3). FG_HAND is recorded by FgSwapPresent, after everything the game drew for the
// frame, so the worker's copy BB -> FG_PRESENT reads the final image and never an intermediate one. The worker is held by
// the gate between the scene fill and the final fill, with the final fill, EndScene and FgSwapPresent's notes queued
// behind it: held, FG_PRESENT has not been written; released, it holds the final fill. CONTROL: FG_HAND recorded before
// the last fill (the render thread's order mutated by hand) copies the scene. Real threads: none; no due is needed.
static volatile DWORD gG1HeldTag = 0;
static void CaseG1()
{
    CaseBegin("G1 the handoff reads N's final back buffer");
    Fresh();
    FgState& s = B::S();
    Warm();
    RingSetTag(gRing.rigImg[FG_IMG_PRESENT], kSentinel);
    Facts();
    const DWORD scene = NewTag(), fin = NewTag();
    Fill(scene);
    if (!Gate()) Err("G1: the gate note was refused");
    Fill(fin);
    D->EndScene();
    Later l = { [] { return gGateIn != 0; }, [] { gG1HeldTag = RingTag(gRing.rigImg[FG_IMG_PRESENT]); SetEvent(gGateEv); }, 3000, 40 };
    HANDLE h = Spawn(LaterMain, &l);
    Present();
    Join(h, "G1's release");
    Drain();
    const DWORD after = RingTag(gRing.rigImg[FG_IMG_PRESENT]);
    if (!l.met) Err("G1: the worker never reached the gate");
    if (gG1HeldTag != kSentinel) Err("G1: FG_PRESENT held %lX while the worker was held before the final fill: the handoff ran early", gG1HeldTag);
    if (after != fin) Err("G1: FG_PRESENT holds %lX after the release, not the final back buffer %lX (the scene was %lX)", after, fin, scene);
    printf("  held: FG_PRESENT %lX (unset); released: %lX (final %lX, scene %lX)\n", gG1HeldTag, after, fin, scene);
    // CONTROL
    AnswerAs(s.r.lastP);
    Facts();
    const DWORD scene2 = NewTag(), fin2 = NewTag();
    Fill(scene2);
    RecordDrain();                       // FgSwapPresent's step 1: the last present was engaged
    if (!RecordHand()) Err("G1 control: FG_HAND was refused");
    Fill(fin2);
    RecordPresent();
    Drain();
    const DWORD ctl = RingTag(gRing.rigImg[FG_IMG_PRESENT]);
    if (ctl != scene2) Err("G1 control: FG_HAND recorded before the last fill left FG_PRESENT at %lX, not the scene %lX: the control shows nothing", ctl, scene2);
    Ctl("FG_HAND before the last fill copied %s", ctl == scene2 ? "the SCENE" : "something else");
    CaseEnd("G1 the handoff reads N's final back buffer");
}

// ---------------------------------------------------------------- G3, N comes after I
// WHAT IT SHOWS (plan 3.4, map 2.7.3 "I first"). The worker is held INSIDE I's fake Present (the SyncFrameLatency stand-in,
// holding DXVK's lock) with the N fire and the whole next frame queued behind it; released, the pair's two presents are
// shown and I's end (its marker's signal) comes before N's (its frame end), each read where the fire makes it by a binding
// that logs the two (the stub's I is N's own copy, so the two show one tag: the TAG mode that told them apart on the screen,
// and then the fire records, went with the rig's mode key and the instrument's records in round 9's S9). The withheld case: a WITHHOLD
// verdict (the placer's 2p+1) resolves I and N fires on time, alone. A stale FIRE_I for a closed pair presents nothing and
// is counted (with H7). And both dues current at ONE fire point (the last leg, added after the verifier's review) still
// read I then N. The placer is the harness (DueWrite).
// CONTROL: the stale FIRE_I again, but keyed to the OPEN pair, as a due with no pair identity would be: it shows an I the
// placer never raised for that pair. The order itself is structural (one worker fires I then N inside one routine, and
// the fake Present holds the device lock across the hold), so the control is on the due's identity.
static volatile LONG gG3HeldShown = -1;
// THE ORDER OF A PAIR'S TWO ENDS, logged on the worker as the fire makes them: I's marker signal (FG_TL_MARKI, its pair) and N's
// frame end (LatEnd, its frame). Read only once the worker is drained.
static std::vector<std::pair<char, LONG>> gG3Order;
struct G3B : FgHarnessBinding
{
    static void RecSignal(int tl, LONG v) { if (tl == FG_TL_MARKI) gG3Order.push_back({ 'I', v }); FgHarnessBinding::RecSignal(tl, v); }
    static void LatEnd(LONG f) { gG3Order.push_back({ 'N', f }); FgHarnessBinding::LatEnd(f); }
};
typedef FgMachine<G3B> MG3;
static bool IThenN(LONG p, LONG fEnd)
{
    int i = -1, n = -1;
    for (int k = 0; k < (int)gG3Order.size(); k++)
    {
        if (i < 0 && gG3Order[k].first == 'I' && gG3Order[k].second == p) i = k;
        if (n < 0 && gG3Order[k].first == 'N' && gG3Order[k].second == fEnd) n = k;
    }
    return i >= 0 && n > i;
}
static void CaseG3()
{
    CaseBegin("G3 N comes after I");
    Fresh<MG3>(); gG3Order.clear();
    FgState& s = B::S();
    Warm<MG3>();
    const DWORD fN = Frame<MG3>(); Drain();
    const LONG p = s.r.lastP, fe = B::frames - 1;
    if (s.w.openPhase != FG_PAIR_ARMED) Err("G3: the pair did not arm (phase %ld)", s.w.openPhase);
    AnswerAs(p);
    const size_t base = Shown().size();
    DueI(p, FG_DUE_SHOW);
    RingGateHoldNext();
    FireNote();
    Later l = { [] { return RingGateHeld(); }, [] { gG3HeldShown = (LONG)RingPresentedLog().size(); RingGateRelease(); }, 3000, 60 };
    HANDLE h = Spawn(LaterMain, &l);
    if (!WaitFor([] { return RingGateHeld(); }, 3000)) Err("G3: I's Present never reached the gate");
    DueN(p);
    FireNote();                          // the N fire, queued behind the held I
    const DWORD next = Frame<MG3>();     // and the whole next frame
    Join(h, "G3's release");
    Drain();
    auto v = Shown();
    const bool order = IThenN(p, fe);
    printf("  held inside I's Present with N and the next frame queued; after the release the log reads %s (N %lX), I's end before N's %d\n", TagList(v, base).c_str(), fN, order);
    if (!order || ShownAt(v, base, fN) != (int)base) Err("G3: the pair's ends do not read I then N, or the log does not show N's frame first: %s", TagList(v, base).c_str());
    if (v.size() - base != 2) Err("G3: %zu presents for the pair, not 2", v.size() - base);
    // THE WITHHELD CASE: the next frame's pair, answered 2p+1, I withheld, N on time and alone
    const LONG p2 = s.r.lastP;
    AnswerAs(p2, true);
    const LONG onTime0 = s.c.nOnTime, shown0 = s.c.iShown;
    DueI(p2, FG_DUE_WITHHOLD, FG_W_CUT); DueN(p2);
    FireNote(); Drain();
    v = Shown();
    const size_t b2 = base + 2;
    printf("  withheld pair: the log adds %s (N %lX)\n", TagList(v, b2).c_str(), next);
    if (v.size() != b2 + 1 || v[b2].tag != next) Err("G3: the withheld pair presented %s, not N alone", TagList(v, b2).c_str());
    if (s.c.nOnTime != onTime0 + 1 || s.c.iShown != shown0) Err("G3: the withheld pair counted %ld on time and %ld I shown", s.c.nOnTime - onTime0, s.c.iShown - shown0);
    // A STALE FIRE_I for the closed pair p2, then the next pair armed and a fire point reached
    DueI(p2, FG_DUE_SHOW);
    const LONG stale0 = s.c.staleDues;
    const DWORD f3 = Frame<MG3>(); Drain();
    const LONG p3 = s.r.lastP;
    FireNote(); Drain();
    v = Shown();
    if (s.c.staleDues <= stale0) Err("G3: a stale FIRE_I was not counted");
    if (v.size() != b2 + 1) Err("G3: a stale FIRE_I presented something: %s", TagList(v, b2 + 1).c_str());
    if (s.w.openPhase != FG_PAIR_ARMED || s.w.openP != p3) Err("G3: the pair after the stale due is not still armed");
    printf("  stale FIRE_I for closed pair %ld: %ld stale due(s) counted, nothing presented, pair %ld still armed\n", p2, s.c.staleDues - stale0, p3);
    // CONTROL: the same due, keyed to the open pair
    DueI(p3, FG_DUE_SHOW);
    FireNote(); Drain();
    v = Shown();
    const bool ctlSaw = v.size() > b2 + 1 && ShownHas(v, b2 + 1, f3);
    if (!ctlSaw) Err("G3 control: a FIRE_I keyed to the open pair presented nothing: the control shows nothing");
    Ctl("the stale FIRE_I keyed to the open pair %s", ctlSaw ? "PRESENTED an I the placer never raised for it" : "presented nothing");
    // BOTH DUES CURRENT AT ONE FIRE POINT (2026-09-25, added after the verifier's review: every leg above writes FIRE_N only
    // once I is held or resolved, so a fire routine that fired N FIRST whenever both dues were current, withholding I as
    // "N first", passed all 31 cases). The placer raises both before the worker reaches a point whenever that point comes
    // after tN - w_N: OFFSET's H = 0, a worker a frame behind, the idle hook off. Map 2.7.3 says I first, then N, from the
    // one routine. So both dues are written before ONE FG_FIRE, the fire is held inside the first Present it makes with the
    // next frame queued behind it, and after the release the log must read I then N: I shown once, N on time once, two
    // presents. Its control is the leg above (a FIRE_I with the wrong identity presents an I): the order itself is one
    // routine's, which the harness cannot switch off from outside, so the verifier's N-first mutant is what it was run against.
    AnswerAs(p3);
    const DWORD f4 = Frame<MG3>(); Drain();   // its DRAIN fires the control's N(p3); pair p4 arms
    const LONG p4 = s.r.lastP, fe4 = B::frames - 1;
    if (s.w.openPhase != FG_PAIR_ARMED || s.w.openP != p4) Err("G3: pair %ld did not arm for the both-dues leg", p4);
    AnswerAs(p4);
    const size_t b4 = Shown().size();
    const LONG sh4 = s.c.iShown, on4 = s.c.nOnTime;
    DueI(p4, FG_DUE_SHOW); DueN(p4);
    RingGateHoldNext();
    FireNote();                          // ONE fire point, both dues current
    Later l4 = { [] { return RingGateHeld(); }, [] { RingGateRelease(); }, 3000, 60 };
    HANDLE h4 = Spawn(LaterMain, &l4);
    if (!WaitFor([] { return RingGateHeld(); }, 3000)) Err("G3: the both-dues fire never reached its first Present");
    Frame<MG3>();                        // the next frame, queued behind the held Present
    Join(h4, "G3's both-dues release");
    Drain();
    v = Shown();
    const bool order4 = IThenN(p4, fe4);
    printf("  both dues current at one fire point, held inside its first Present with the next frame queued: the log adds %s (N %lX), I's end before N's %d, I shown %ld, N on time %ld\n",
        TagList(v, b4).c_str(), f4, order4, s.c.iShown - sh4, s.c.nOnTime - on4);
    if (!order4 || ShownAt(v, b4, f4) != (int)b4 || v.size() - b4 != 2) Err("G3: with both dues current at one point the log reads %s and the ends I then N %d, not I then N (%lX)", TagList(v, b4).c_str(), order4, f4);
    if (s.c.iShown != sh4 + 1 || s.c.nOnTime != on4 + 1) Err("G3: with both dues current at one point, I shown %ld and N on time %ld, not 1 and 1", s.c.iShown - sh4, s.c.nOnTime - on4);
    CaseEnd("G3 N comes after I");
}

// ---------------------------------------------------------------- G5, nosource
// WHAT IT SHOWS (O-M2, map 2.5 "the G5 test"). Pair p-1 armed and unanswered: pair p is NOSOURCE, so FG_HAND copies
// nothing (FG_PRESENT keeps p-1's image), FG_HUDLESS restores nothing, FgWorkerPresent presents inline, and what is shown
// is N's final; the pair is published INLINE, answered 2p+1 (PlacerStep on the real clock, called once by the harness), and
// its 'G' stub is written with the nosource flag. CONTROL: the first map's behaviour, skip the copies and STAY ARMED (a
// wrapper FG_HUDLESS observer that clears the nosource flag after the machine's own), presents FG_PRESENT, a stale tag.
static void Hudless_FirstMap(DWORD kind, const DWORD* p, DWORD bytes) { M::NoteHudless(kind, p, bytes); B::S().w.openNosource = false; }
static void CaseG5()
{
    CaseBegin("G5 nosource");
    Fresh();
    FgState& s = B::S();
    Warm();
    const DWORD e1 = Frame(); Drain();             // pair p-1: armed, left unanswered
    const size_t base = Shown().size();
    RingSetTag(gRing.rigImg[FG_IMG_HUDLESS], kSentinel);
    const LONG in0 = s.c.pubInline[FG_IN_NOSOURCE];
    const DWORD f = Frame(); Drain();
    const LONG p = s.r.lastP;
    auto v = Shown();
    printf("  pair %ld after an unanswered pair: FG_PRESENT %lX (p-1's %lX), FG_HUDLESS %lX, shown %s\n", p, RingTag(gRing.rigImg[FG_IMG_PRESENT]), e1,
        RingTag(gRing.rigImg[FG_IMG_HUDLESS]), TagList(v, base).c_str());
    if (s.c.pubInline[FG_IN_NOSOURCE] != in0 + 1) Err("G5: the pair was not published INLINE(nosource)");
    if (RingTag(gRing.rigImg[FG_IMG_PRESENT]) != e1) Err("G5: FG_HAND copied into FG_PRESENT for a nosource pair");
    if (RingTag(gRing.rigImg[FG_IMG_HUDLESS]) != kSentinel) Err("G5: FG_HUDLESS copied for a nosource pair");
    if (v.empty() || v.back().tag != f) Err("G5: the last present shows %lX, not N's final %lX", v.empty() ? 0 : v.back().tag, f);
    if (ShownAt(v, base, e1) < 0) Err("G5: pair p-1's N was never presented (the drain)");
    PlaceNow();
    FgAns a = {};
    if (!M::Read(s.ans[p & (FG_RING - 1)], p, &a) || a.val != 2 * p + 1) Err("G5: pair %ld was not answered 2p+1", p);
    bool g = false; for (const RingPipe& r : RingPipeLog()) if (r.type == 'G' && r.p == p && (r.flags & 2u)) g = true;
    if (!g) Err("G5: no 'G' stub with the nosource flag for pair %ld", p);
    // CONTROL: pair p+1 after an unanswered armed pair p' again, with the first map's FG_HUDLESS
    const DWORD e3 = Frame(); Drain();            // p+1: armed (its predecessor p was answered above), left unanswered
    dtNoteObs[DT_NOTE_FG_HUDLESS] = Hudless_FirstMap;
    const size_t b2 = Shown().size();
    const DWORD f4 = Frame(); Drain();
    dtNoteObs[DT_NOTE_FG_HUDLESS] = &M::NoteHudless;
    const LONG p4 = s.r.lastP;
    DueN(p4); FireNote(); Drain();
    v = Shown();
    int e3n = 0; for (size_t i = b2; i < v.size(); i++) e3n += v[i].tag == e3;
    const bool stale = e3n >= 2 && !ShownHas(v, b2, f4);     // p+1's N at the drain, then p+1's image AGAIN for p+2, whose own is lost
    if (!stale) Err("G5 control: skipping the copies and staying armed presented %s, not the stale %lX twice: the control shows nothing", TagList(v, b2).c_str(), e3);
    Ctl("skip the copies and stay armed presented %s (N was %lX): %s", TagList(v, b2).c_str(), f4, stale ? "a STALE tag" : "nothing stale");
    CaseEnd("G5 nosource");
}
// ---------------------------------------------------------------- G2, I(N-1,N) comes after N-1
// WHAT IT SHOWS (O-M3; map 2.5 step 1). The worker is held by the gate BEFORE frame N-1's notes and Present, while the
// render thread records frame N: FgSwapPresent's step 1 records the DRAIN for N-1 by a fact the render thread owns (its
// last present was engaged), so the worker fires N(N-1) before FG_HAND(N) overwrites FG_PRESENT, and the screen never
// steps back. N-1's answer is written by the harness before the hold (the placer would have written it at N-1's arrival),
// so pair N copies and the hazard is real. CONTROLS: (a) the first map's rule, "record the drain only if N-1 is still
// pending", read on the render thread, where the held worker has not even armed N-1: no drain is recorded, and the
// machine's own backstop (H15, FG_HAND closes an open pair) fires N(N-1) and counts "hand found open", the fault that must
// be 0 in every counted window; (b) that rule AND the first map's FG_HAND, which forgot an open pair instead of closing
// it (a wrapper observer): FG_HAND(N) overwrites FG_PRESENT first and N-1's frame is never shown. AT threaded=2 the
// validator drains after every recorded Present (DtFrameEnd), so the worker is never a frame behind and neither control
// can bite; the case says so rather than failing a race that cannot exist there, and its row reads N/A there, not PASS
// (2026-09-25, after the verifier's review: a PASS row whose control cannot bite reads as a proof it is not). Its
// assertions still run at 2; a missing drain is caught at 2 by H2 and R8.
static void Hand_FirstMap(DWORD kind, const DWORD* p, DWORD bytes) { FgWorkerLine& w = B::S().w; if (w.openPhase == FG_PAIR_ARMED) w.openPhase = FG_PAIR_NONE; M::NoteHand(kind, p, bytes); }
static void G2Run(int mode, DWORD* a, DWORD* b, size_t* base, LONG* handOpen, std::vector<RingPresented>* v)
{
    FgState& s = B::S();
    Fresh();
    Warm();
    Facts(); *a = NewTag(); Fill(*a);
    AnswerAs(s.r.nextP);                                   // N-1's answer, as the placer writes it at N-1's arrival
    *base = Shown().size();
    const LONG ho0 = s.c.handOpen;
    if (!Gate()) Err("G2: the gate note was refused");
    Later l = { [] { return gGateIn != 0; }, [] { SetEvent(gGateEv); }, 3000, 80 };
    HANDLE h = Spawn(LaterMain, &l);
    Present();                                             // N-1's notes and Present, all behind the gate
    const LONG nm1 = s.r.lastP;
    if (mode >= 1 && !(s.w.openPhase == FG_PAIR_ARMED && s.w.openP == nm1)) s.r.lastEngaged = false;   // the first map's test, on this thread
    if (mode == 2) dtNoteObs[DT_NOTE_FG_HAND] = Hand_FirstMap;
    Facts(); *b = NewTag(); Fill(*b); Present();           // frame N: [DRAIN(N-1)], FG_HAND(N), FG_HUDLESS(N), the Present (waits for N-1's)
    Join(h, "G2's release");
    Drain();
    dtNoteObs[DT_NOTE_FG_HAND] = &M::NoteHand;
    if (!l.met) Err("G2: the worker never reached the gate");
    Frame(); Drain();                                      // the next frame's drain fires N(N)
    *handOpen = s.c.handOpen - ho0;
    *v = Shown();
}
static void CaseG2()
{
    CaseBegin("G2 I(N-1,N) comes after N-1");
    DWORD a, b; size_t base; LONG ho; std::vector<RingPresented> v;
    G2Run(0, &a, &b, &base, &ho, &v);
    const int ia = ShownAt(v, base, a), ib = ShownAt(v, base, b);
    bool back = false; for (size_t i = base + 1; i < v.size(); i++) if (v[i].tag < v[i - 1].tag) back = true;
    printf("  held before N-1's present while N was recorded: shown %s (N-1 %lX, N %lX), hand found open %ld\n", TagList(v, base).c_str(), a, b, ho);
    if (ia < 0 || ib < 0 || ia > ib) Err("G2: N-1 (%lX) was not shown before N (%lX): %s", a, b, TagList(v, base).c_str());
    if (back) Err("G2: the screen stepped back: %s", TagList(v, base).c_str());
    if (ho != 0) Err("G2: FG_HAND found the pair open %ld time(s) with the drain recorded", ho);
    // CONTROL (a)
    G2Run(1, &a, &b, &base, &ho, &v);
    const bool sawA = ho == 1 && ShownAt(v, base, a) >= 0;
    // CONTROL (b)
    DWORD a2, b2; size_t base2; LONG ho2; std::vector<RingPresented> v2;
    G2Run(2, &a2, &b2, &base2, &ho2, &v2);
    const bool sawB = !ShownHas(v2, base2, a2) && ShownHas(v2, base2, b2);
    printf("  control (a), the first map's rule: hand found open %ld, shown %s; (b) with its FG_HAND too: shown %s (N-1 %lX)\n", ho, TagList(v, base).c_str(), TagList(v2, base2).c_str(), a2);
    if (gCfg == 1)
    {
        if (!sawA) Err("G2 control (a): without the render thread's drain FG_HAND did not find the pair open (%ld): the control shows nothing", ho);
        if (!sawB) Err("G2 control (b): N-1's frame (%lX) was still shown: the control shows nothing", a2);
        Ctl("(a) no drain: 'hand found open' %ld (the backstop's fault count); (b) and no close: N-1 %s", ho, sawB ? "NEVER SHOWN, FG_PRESENT overwritten first" : "shown");
    }
    else
    {
        gNa = true;
        Ctl("N/A at threaded=2, a proof only at threaded=1: every recorded Present is drained (DtFrameEnd's validation), the worker is never"
            " a frame behind, and neither control can bite: (a) hand found open %ld, (b) N-1 %s", ho, sawB ? "lost" : "shown");
    }
    CaseEnd("G2 I(N-1,N) comes after N-1");
}

// ---------------------------------------------------------------- G4, fgPrep: IN dt_fg_shadow.h SINCE PHASE 2'S STAGE 9
// The appendix: "a held fgPrep makes a counted wait-only batch before the stub body; IvtFail releases it". It was a
// documented stub while the machine had no fgPrep path. The path is the product's shape's (ShadowBeforeInject and the
// binding's PrepGuard), and the case runs that shape over a fake helper held inside a prepare: dt_fg_shadow.h, CaseG4.
// The release on a stop is L7's there.

// ---------------------------------------------------------------- G6, pairing and threads
// WHAT IT SHOWS (map 2.5, 2.15; R1 5.4). The pipe stub ('Q' at FG_HAND for a sent frame, 'G' at FG_HUDLESS or at the pair's
// close): no re-entry (the fake counts it), 'Q'(p) then exactly one 'G'(p), and each written by the worker or at a drained
// point (the disarm tail runs with the ring drained, on whichever thread disarms). Over plain frames, then with a pair
// OPEN ('Q' written, 'G' not) across a Reset from another thread with the render thread idle (the barrier disarms on the
// Reset's thread) and across a Reset while the render thread records (the render thread answers the ask), each followed
// by a re-arm. The drained test is made inside the stub by a derived binding (G6B), so it sees the thread and the ring
// at the instant of the write. CONTROL: the disarm observer run on another thread while the layer is ARMED (a disarm tail
// with no drain behind it): its 'G' is written with the ring live, and the stub's test sees it.
struct G6B : FgHarnessBinding
{
    static inline volatile LONG undrained = 0, writes = 0;
    static void Check()
    {
        _InterlockedIncrement(&writes);
        const bool ok = GetCurrentThreadId() == dtWorkerTid || (!dtArmed && (DWORD)dtDone == dtW && (DWORD)dtPub == dtW);
        if (!ok) _InterlockedIncrement(&undrained);
    }
    static void WriteQ(LONG p) { Check(); FgHarnessBinding::WriteQ(p); }
    static void WriteG(LONG p, DWORD f) { Check(); FgHarnessBinding::WriteG(p, f); }
};
typedef FgMachine<G6B> M6;
static DWORD WINAPI ResetFrom(void*) { D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp); return 0; }
static volatile LONG gBusyGo = 0;
static DWORD WINAPI ResetBusy(void*) { while (!gBusyGo) Sleep(0); Sleep(5); D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp); return 0; }
static DWORD WINAPI DisarmObsElsewhere(void*) { M6::OnDisarm(); return 0; }
// 'Q'(p) then exactly one 'G'(p), no 'G' without its 'Q'; returns the number of pairs checked.
static int PipePairs(const std::vector<RingPipe>& log, std::string* bad)
{
    std::map<LONG, int> q, g; std::map<LONG, size_t> qAt;
    for (size_t i = 0; i < log.size(); i++)
    {
        const RingPipe& r = log[i];
        if (r.type == 'Q') { q[r.p]++; qAt[r.p] = i; }
        else if (r.type == 'G') { g[r.p]++; if (!q.count(r.p)) { if (bad->empty()) *bad = Fmt("'G'(%ld) with no 'Q'", r.p); } }
    }
    for (const auto& e : q)
        if (e.second != 1 || g[e.first] != 1) { if (bad->empty()) *bad = Fmt("pair %ld: %d 'Q' and %d 'G'", e.first, e.second, g[e.first]); }
    return (int)q.size();
}
static void CaseG6()
{
    CaseBegin("G6 pairing and threads");
    Fresh<M6>();
    FgState& s = B::S();
    G6B::undrained = 0; G6B::writes = 0;
    Warm<M6>();
    for (int i = 0; i < 6; i++) { Frame<M6>(); Drain(); AnswerAs(s.r.lastP); }
    // A RESET FROM ANOTHER THREAD, the render thread idle, with a pair OPEN
    Facts(); Fill(NewTag()); RecordDrain(); RecordHand(); Drain();
    if (s.w.openPhase != FG_PAIR_OPEN || !s.w.openQ || s.w.openG) Err("G6: the pair was not open with its 'Q' and no 'G' before the Reset");
    const LONG disarms0 = dtDisarms;
    HANDLE h = Spawn(ResetFrom); Join(h, "G6's foreign Reset");
    if (dtDisarms != disarms0 + 1 || dtArmed) Err("G6: the foreign Reset did not disarm the layer");
    if (!EnsureArmed()) Err("G6: no re-arm after the foreign Reset");
    Warm<M6>();
    // A RESET WHILE THE RENDER THREAD RECORDS, with a pair OPEN
    Facts(); Fill(NewTag()); RecordDrain(); RecordHand(); Drain();
    if (s.w.openPhase != FG_PAIR_OPEN || !s.w.openQ) Err("G6: the pair was not open before the recording Reset");
    gBusyGo = 0;
    h = Spawn(ResetBusy); gBusyGo = 1;
    const ULONGLONG t0 = GetTickCount64(); unsigned n = 0;
    while (dtArmed && GetTickCount64() - t0 < 3000) { D->SetRenderState((D3DRENDERSTATETYPE)(7 + n % 200), n); n++; }
    Join(h, "G6's recording Reset");
    if (dtArmed) Err("G6: a Reset did not unarm a recording render thread");
    if (!EnsureArmed()) Err("G6: no re-arm after the recording Reset");
    Warm<M6>(); Frame<M6>(); Drain();
    std::string bad;
    const auto log = RingPipeLog();
    const int pairs = PipePairs(log, &bad);
    printf("  %d pairs with a 'Q' over plain frames and two Resets with a pair open; %ld stub writes, %ld not on the worker and not drained; %s\n",
        pairs, G6B::writes, G6B::undrained, bad.empty() ? "every 'Q' has exactly one 'G'" : bad.c_str());
    if (!bad.empty()) Err("G6: %s", bad.c_str());
    if (pairs < 8) Err("G6: only %d pairs wrote a 'Q'", pairs);
    if (G6B::undrained) Err("G6: %ld stub write(s) off the worker at an undrained point", G6B::undrained);
    // the two closes at the Resets were written off the worker: by the Reset's thread and by the render thread
    int offWorker = 0; for (const RingPipe& r : log) offWorker += r.tid != dtWorkerTid;
    if (offWorker < 2) Err("G6: only %d stub write(s) came from a disarm tail; the Resets did not close the open pairs", offWorker);
    // CONTROL
    Facts(); Fill(NewTag()); RecordDrain(); RecordHand(); Drain();
    const LONG u0 = G6B::undrained;
    h = Spawn(DisarmObsElsewhere); Join(h, "G6's control");
    const LONG seen = G6B::undrained - u0;
    M6::OnArm();                                           // the harness's clean-up: the machine's own arm observer clears the quiet flag
    if (seen < 1) Err("G6 control: a 'G' written by a disarm tail with the layer armed was not seen: the control shows nothing");
    Ctl("a disarm observer run elsewhere with the layer armed wrote %ld 'G' at an UNDRAINED point", seen);
    CaseEnd("G6 pairing and threads");
}

// ---------------------------------------------------------------- G7, no generated pixel reaches an image the game draws into
// WHAT IT SHOWS (plan 3.4; map 2.8, 2.9; R4 3.1, 3.2). At 1, 2, 3 and 4 images, three pairs, I and N each fired on its own
// by a due the harness writes and FG_FIRE(PLACER), with the game's next frame already begun in the back buffer (its partial
// image, g): after EVERY fire the image current (the one the game draws into next) holds g, never the fire's picture (the
// pair's frame, which the stub's I and N both present) nor any other. CONTROL: the restore dropped (NoRestoreB, a binding whose
// raw SAVE -> BB does nothing): at 2 or more images the game's image after a fire does not hold g. (Shape D's StretchRects, TAG's
// magenta generated image and its samples, and the faults 16 and 1, the post-present flush and a restore skipped, went with
// the rig's keys in round 9's S9: the merged path's own save and restore are M11's and M12's, with their reruns.)
// H9's last leg: a binding whose wait before a Reset asks for the last value ASKED of a submit (the rig's until 2026-10-06)
struct AskedB : FgHarnessBinding { static bool RetireWait(DWORD ms) { return RetireWaitFor(subAsked, ms); } };
typedef FgMachine<AskedB> MAK;
struct NoRestoreB : FgHarnessBinding
{
    static void RecCopyFromImg(int img, ULONGLONG bb) { if (img == FG_IMG_SAVE) return; FgHarnessBinding::RecCopyFromImg(img, bb); }
};
typedef FgMachine<NoRestoreB> MNR;
static void RawEmpty() { B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSubmit(); B::QUnlock(); }
struct G7Res { int fires = 0, bad = 0, fireInGame = 0; std::string first; };
template <class MM> static void G7Pairs(int pairs, G7Res& r)
{
    FgState& s = B::S();
    for (int i = 0; i < pairs; i++)
    {
        const DWORD f = Frame<MM>(); Drain();
        const LONG p = s.r.lastP;
        if (s.w.openPhase != FG_PAIR_ARMED) { Err("G7: pair %ld did not arm", p); return; }
        AnswerAs(p);
        const DWORD g = NewTag();
        for (int part = 0; part < 2; part++)
        {
            Fill(g);                                       // the game's next frame, begun in the back buffer
            if (part == 0) DueI(p, FG_DUE_SHOW); else DueN(p);
            FireNote(); Drain(); B::Flush();
            r.fires++;
            const DWORD cur = RingTag(RingCurrent());
            if (cur == f) r.fireInGame++;
            if (cur != g) { r.bad++; if (r.first.empty()) r.first = Fmt("pair %ld after %s: the game's image holds %lX, not %lX", p, part ? "N" : "I", cur, g); }
        }
    }
}
static void CaseG7()
{
    CaseBegin("G7 no generated pixel reaches an image the game draws into");
    std::string ctl;
    for (int n = 1; n <= 4; n++)
    {
        Opt o; o.images = n;
        Fresh(o); Warm();
        const size_t base = Shown().size();
        G7Res r; G7Pairs<M>(3, r);
        const auto v = Shown();
        printf("  %d image(s): %d fires, the game's image right after each: %d wrong, %d holding the fire's picture; %zu presents\n", n, r.fires, r.bad, r.fireInGame, v.size() - base);
        if (r.bad) Err("G7 %d: %s", n, r.first.c_str());
        // the six fires the harness raised, and the warm pair's N, which the first pair's drain fired
        if (v.size() - base != 7) Err("G7 %d: %zu presents for three pairs, not 7", n, v.size() - base);
        // THE CONTROL, at 2 images and more
        if (n >= 2)
        {
            Fresh<MNR>(o); Warm<MNR>();
            G7Res rc; G7Pairs<MNR>(3, rc);
            if (!rc.bad) Err("G7 control, %d: with the restore dropped the game's image still held its frame after every fire: the control shows nothing", n);
            ctl += Fmt("%s%d %d/%d", ctl.empty() ? "" : ", ", n, rc.bad, rc.fires);
        }
    }
    Ctl("restore dropped, the game's image NOT its frame after (fires) %s", ctl.c_str());
    CaseEnd("G7 no generated pixel reaches an image the game draws into");
}

// ---------------------------------------------------------------- G8, no GPU wait on fgOut (phase 2): a documented stub
// The appendix: "the submit stub's wait list never names fgOut". fgOut is phase 2's (the helper's output image), and the
// binding contract at this commit has NO seam through which a rig submit can wait on anything (RecSignal is the only
// semaphore operation; dt_fake_ring.h's author notes the same). So no wait can be named and none can be logged; the case is
// a stub that does not count, and the day a wait seam is added, its submit stub logs the list and this case reads it.
// SINCE PHASE 2'S STAGE 9 the case is real and in dt_fg_shadow.h (CaseG8): the product's shape has fgOut (a timeline it only
// READS, at the next hand) and one wait seam (PrepGuard, behind fgPrep), and the case holds a fake helper inside a
// dispatch and shows the worker runs on.
// (H1, the samples read what the blit reads, went with the rig's probe and its black-fill fault in round 9's S9: the samples
// were the probe's alone, an instrument's record no build of the product takes)

// ---------------------------------------------------------------- H2, the placer never wedges
// WHAT IT SHOWS (O-M1; map 2.6). The placer ON THE FAKE CLOCK (the harness stamps and steps it): it raises FIRE_I for pair
// a at a's arrival, and the worker withholds I(a) at the next frame's DRAIN (point d); it raises FIRE_I(b), and an
// injection note withholds I(b) and fires N(b) early (point c, rule C1); pair c's I then fires on time at FIRE_I(c), and
// the stampers keep stamping: nothing the placer does waits on a timeline, so a withheld I, which never signals its marker,
// blocks nothing. CONTROL: the first map's SEQUENTIAL placer, which after raising FIRE_I(a) waited for fgSemMarkI to reach
// a before looking at anything else: it would wait for ever (the harness's wait times out at 300 ms), and b and c would
// never be placed.
static void CaseH2()
{
    CaseBegin("H2 the placer never wedges");
    Opt o; o.fakeClock = true; Fresh(o);
    FgState& s = B::S();
    FakeFrames(6);                                        // the render thread's facts settle: no cut, four periods, every pair answered
    FgDue d = {};
    FakePair(); const LONG a = s.r.lastP;
    const bool raisedA = M::Read(*s.dueI, a, &d) && d.verdict == FG_DUE_SHOW;
    const LONG drain0 = s.c.iWithheld[10], early0 = s.c.iWithheld[9], shown0 = s.c.iShown, pinj0 = gInjCalls;
    FakePair(false); const LONG b = s.r.lastP;            // no fill: its DRAIN is the first point, and withholds I(a) (point d); b arms, is placed
    // THE CONTROL, here, where the first map's placer would be: still waiting for I(a)'s marker
    const bool wedged = !B::TlWait(FG_TL_MARKI, (ULONGLONG)a, 300);
    const bool raisedB = M::Read(*s.dueI, b, &d) && d.verdict == FG_DUE_SHOW;
    const DWORD inj[4] = { 1, 2, 3, 4 };
    _InterlockedIncrement(&gInjPending);
    if (!DtNote(DT_NOTE_INJECT, inj, sizeof(inj))) Err("H2: the injection note was refused");
    DtPublishFenced(); Drain();                           // point c: N(b) early, I(b) withheld
    FakePair(); const LONG c = s.r.lastP;
    const bool raisedC = M::Read(*s.dueI, c, &d) && d.verdict == FG_DUE_SHOW;
    const LONGLONG raisedAt = B::clockNow;                // the instant the placer raised FIRE_I(c): its step's clock
    FireNote(); Drain(); StampNow();                      // FgPace's FG_FIRE at the placer's wake: point b
    LONGLONG tm = 0;
    const bool markC = M::ReadStamp(s.st[FG_TL_MARKI], c, &tm);
    // (the fire's own record went with the instrument's records in round 9's S9: its marker's stamp, on the clock that has not
    // moved since the raise, says when I fired)
    printf("  I(a=%ld) raised %d and withheld at the drain %ld; I(b=%ld) raised %d and withheld early %ld; I(c=%ld) raised %d, shown %ld, fire start - the instant it was raised %.3f ms;"
           " stamps: IN %ld, MARKI %ld (c's %s); the injection's observer called %ld\n",
        a, raisedA, s.c.iWithheld[10] - drain0, b, raisedB, s.c.iWithheld[9] - early0, c, raisedC, s.c.iShown - shown0,
        markC ? M::TicksToMs(tm - raisedAt) : -1.0, s.c.stamped[FG_TL_IN], s.c.stamped[FG_TL_MARKI], markC ? "stamped" : "MISSING", gInjCalls - pinj0);
    if (!raisedA || !raisedB || !raisedC) Err("H2: the placer did not raise every FIRE_I (a %d, b %d, c %d)", raisedA, raisedB, raisedC);
    if (s.c.iWithheld[10] - drain0 != 1 || s.c.iWithheld[9] - early0 != 1) Err("H2: I(a) and I(b) were not withheld at the drain and early");
    if (s.c.iShown - shown0 != 1 || !markC) Err("H2: pair c's I did not fire or was not stamped");
    if (!markC || tm != raisedAt) Err("H2: pair c's I did not fire at the instant it was raised");
    if (s.c.stamped[FG_TL_IN] < 9) Err("H2: the IN stamper stopped stamping");
    if (gInjPending != 0) Err("H2: the injection's observer was not called exactly once");
    if (!wedged) Err("H2 control: fgSemMarkI reached a withheld pair: the control shows nothing");
    Ctl("a sequential placer waiting for I(a)'s marker %s", wedged ? "WEDGED (300 ms, never signalled)" : "went on");
    CaseEnd("H2 the placer never wedges");
}

// ---------------------------------------------------------------- H3, the ordinal race
// WHAT IT SHOWS (O-N4; map 2.5). A disarm ask answered inside FgSwapPresent, at the two places it can be:
//  (a) between FG_HAND and FG_HUDLESS: FG_HUDLESS is made a driving note for the case, so its post fetches the body's
//      state, and the fake's GetTexture starts a Reset from another thread and returns once its ask is up; the post's
//      Release answers it (NoteAcrossDisarm's shape, dt_test.cpp:1188-1228). FG_HAND is in; FG_HUDLESS is refused;
//  (b) between the notes and the record: a derived binding raises the ask from PaceSleptN, the one seam FgSwapPresent
//      calls after its notes and before dtScPresent, which then answers it.
// Either way the disarm tail finds the pair OPEN and publishes it CLOSED(disarm), FgWorkerPresent is reached on the render
// thread and passes through, the placer answers the pair 2p+1, and the census closes. Then the generation: after the
// re-arm, the first recorded Present has the lost pair's ordinal again (the unarmed one was never recorded), and a
// one-shot left pointing at it with the old generation (the harness puts it back) is REJECTED by the generation.
// CONTROLS: (1) the same one-shot with the generation made current: FgWorkerPresent arms a pair that was closed, and it is
// published a second time (the census's out-of-order count sees it); (2) (a) with the device thread's re-test off: the
// refused note is instead left in a ring nobody publishes, and replayed after the re-arm.
static HANDLE gH3Reset = nullptr;
static void H3OnGetTexture()
{
    if (gH3Reset) return;
    gH3Reset = Spawn(ResetFrom);
    const ULONGLONG t0 = GetTickCount64();
    while (!dtDisarmAsk && dtArmed && GetTickCount64() - t0 < 3000) Sleep(0);
}
static IDirect3DTexture9* gH3Tex = nullptr;
static void MakeTexturesUnknown()             // every texture entry unknown, so a driving note's post fetches slot 0 (dt_test.cpp, NoteAcrossDisarm)
{
    if (!gH3Tex) D->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &gH3Tex, nullptr);
    D->SetTexture(0, gH3Tex);
    IDirect3DStateBlock9* sb = nullptr; D->CreateStateBlock(D3DSBT_ALL, &sb);
    sb->Capture(); sb->Apply(); sb->Release();
    Drain();
}
struct H3B : FgHarnessBinding
{
    static inline volatile LONG ask = 0;
    static LONG PaceSleptN()
    {
        if (_InterlockedExchange(&ask, 0)) { strncpy_s(dtAskWhy, sizeof(dtAskWhy), "the harness, between FgSwapPresent's notes and its record", _TRUNCATE); _InterlockedExchange(&dtDisarmAsk, 1); }
        return FgHarnessBinding::PaceSleptN();
    }
};
typedef FgMachine<H3B> MH3;
static int H3HudlessRuns = 0;
static void H3Hudless(DWORD k, const DWORD* p, DWORD n) { H3HudlessRuns++; M::NoteHudless(k, p, n); }
// (a); recheck off is control (2). Returns whether the note was orphaned.
static bool H3a(bool recheck, LONG* p, LONG* closed, LONG* passed, LONG* refused)
{
    FgState& s = B::S();
    Fresh(); Warm();
    MakeTexturesUnknown();
    dtNoteObs[DT_NOTE_FG_HUDLESS] = H3Hudless; H3HudlessRuns = 0;
    dtNoteDrives[DT_NOTE_FG_HUDLESS] = true; dtBodySampFree = 0x1FFFFFu & ~1u;
    gNoteRecheck = recheck; gH3Reset = nullptr; gFkOnGetTexture = H3OnGetTexture;
    const LONG c0 = s.c.pubClosed[FG_CL_DISARM], pp0 = s.c.presentsPass, r0 = s.c.noteRefused;
    Facts(); Fill(NewTag()); Present();
    gFkOnGetTexture = nullptr; gNoteRecheck = true;
    if (gH3Reset) { Join(gH3Reset, "H3's foreign Reset"); gH3Reset = nullptr; } else Err("H3 (a): the post never fetched a texture");
    const bool orphan = dtW != (DWORD)dtPub || (DWORD)dtDone != (DWORD)dtPub;
    *p = s.r.lastP; *closed = s.c.pubClosed[FG_CL_DISARM] - c0; *passed = s.c.presentsPass - pp0; *refused = s.c.noteRefused - r0;
    if (dtArmed) Err("H3 (a): the layer was still armed after the other thread's Reset");
    if (!EnsureArmed()) Err("H3 (a): no re-arm");
    Drain();
    dtNoteDrives[DT_NOTE_FG_HUDLESS] = false; dtBodySampFree = 0;
    dtNoteObs[DT_NOTE_FG_HUDLESS] = &M::NoteHudless;
    PlaceNow();
    return orphan;
}
static void CaseH3()
{
    CaseBegin("H3 the ordinal race");
    FgState& s = B::S();
    LONG p, closed, passed, refused;
    const bool orphan = H3a(true, &p, &closed, &passed, &refused);
    const LONG ansA = AnsVal(p);
    printf("  (a) ask answered between FG_HAND and FG_HUDLESS: pair %ld published CLOSED(disarm) %ld, FgWorkerPresent passed through %ld,"
           " notes refused %ld, orphan %d, answered %ld\n", p, closed, passed, refused, orphan, ansA);
    if (closed != 1 || passed < 1 || orphan || ansA != 2 * p + 1 || AnsTimes(p) != 1) Err("H3 (a): the pair was not closed, passed through and answered 2p+1 once");
    if (H3HudlessRuns != 0) Err("H3 (a): FG_HUDLESS reached the worker after a disarm refused it");
    // (b)
    Fresh<MH3>(); Warm<MH3>();
    const LONG c0 = s.c.pubClosed[FG_CL_DISARM], pp0 = s.c.presentsPass, gen0 = dtArmCount, rec0 = dtPresRecN;
    H3B::ask = 1;
    Facts(); Fill(NewTag()); Present<MH3>();
    const LONG pb = s.r.lastP, k = rec0 + 1;
    const bool closedB = s.c.pubClosed[FG_CL_DISARM] - c0 == 1, passB = s.c.presentsPass - pp0 >= 1, unrec = dtPresRecN == rec0;
    // the generation: re-arm with nothing engaged, then the lost pair's one-shot put back, with the OLD generation
    s.wt.want = FG_WANT_PASS;
    if (!EnsureArmed()) Err("H3 (b): no re-arm");
    Drain();
    const bool sameK = dtPresRecN + 1 == k;
    FgWorkerLine& w = s.w;
    w.openPhase = FG_PAIR_OPEN; w.openP = pb; w.openGen = gen0; w.openHudlessSeen = true; w.handFor = dtPresRecN + 1; w.handGen = gen0;
    const LONG pp1 = s.c.presentsPass, arm1 = s.c.pubArmed;
    Facts(); Fill(NewTag()); Present<MH3>(); Drain();
    const bool rejected = s.c.presentsPass == pp1 + 1 && s.c.pubArmed == arm1;
    PlaceNow();
    printf("  (b) ask answered between the notes and the record: CLOSED(disarm) %d, passed through %d, not recorded %d; after the re-arm the next ordinal"
           " is the lost pair's again %d, and its one-shot with the old generation was rejected %d; answered %ld\n", closedB, passB, unrec, sameK, rejected, AnsVal(pb));
    if (!closedB || !passB || !unrec) Err("H3 (b): the pair was not closed and passed through");
    if (!rejected) Err("H3 (b): a one-shot of the old generation armed a pair after the re-arm");
    if (AnsVal(pb) != 2 * pb + 1 || AnsTimes(pb) != 1) Err("H3 (b): pair %ld was not answered 2p+1 exactly once", pb);
    // CONTROL (1): the same one-shot, the generation made current
    w.openPhase = FG_PAIR_OPEN; w.openP = pb; w.openGen = dtArmCount; w.openHudlessSeen = true; w.handFor = dtPresRecN + 1; w.handGen = dtArmCount;
    const LONG order0 = s.c.pubOrder;
    Facts(); Fill(NewTag()); Present<MH3>(); Drain();
    const LONG twice = s.c.pubOrder - order0;
    s.wt.want = FG_WANT_ON;
    // CONTROL (2)
    const bool orphan2 = H3a(false, &p, &closed, &passed, &refused);
    printf("  control (1): a closed pair's one-shot with the current generation was published again %ld time(s); (2) re-test off: orphaned %d,"
           " replayed after the re-arm %d time(s)\n", twice, orphan2, H3HudlessRuns);
    if (twice < 1) Err("H3 control (1): the one-shot with a current generation did not arm the closed pair: the control shows nothing");
    if (!orphan2 || H3HudlessRuns != 1) Err("H3 control (2): with the re-test off the note was not orphaned and replayed: the control shows nothing");
    Ctl("(1) generation current: the closed pair PUBLISHED AGAIN %ld; (2) re-test off: FG_HUDLESS ORPHANED and replayed %d after the re-arm", twice, H3HudlessRuns);
    CaseEnd("H3 the ordinal race");
}

// ---------------------------------------------------------------- H4, publications
// WHAT IT SHOWS (O-M4; map 2.5, 2.6). ON THE FAKE CLOCK, the placer not run until the end. (a) The appendix's shape: two
// arms (the pair before each answered by the harness, as the placer would have at its arrival, so G5 lets the second
// arm), then one FG_HAND with no arm (recorded with no Present), then a pair whose FG_HAND closes it: the placer's first
// run takes all four in p order and answers each exactly once. (b) Added: four arms with the first one answered AND
// still live when the table fills (the live table is three; map 2.6 "asserted, counted beyond"): the overflow may retire
// it but must not answer it again. (c) Added: nine publications before the placer runs (the first armed, the eight behind it
// INLINE as nosource, which no answer holds back): the ring is eight, so the ninth overwrites the first before it is read; the placer must neither
// stop there nor lose an answer. CONTROL: the first map's single arm slot and auto-reset event (a take that reads only
// the latest publication) loses the others.
static std::set<LONG> FirstMapTake()
{
    std::set<LONG> got; FgState& s = B::S();
    const LONG last = ReadAcquire(&s.sh.pubP);
    FgPub v = {};
    if (M::Read(s.pub[last & (FG_RING - 1)], last, &v)) got.insert(last);
    return got;
}
static void CaseH4()
{
    CaseBegin("H4 publications");
    FgState& s = B::S();
    Opt o; o.fakeClock = true;
    // (a)
    Fresh(o);
    Frame(); Drain(); const LONG a = s.r.lastP; AnswerAs(a);
    Frame(); Drain(); const LONG b = s.r.lastP; AnswerAs(b);
    Facts(); Fill(NewTag()); RecordDrain(); RecordHand(); Drain(); const LONG c = s.r.lastP;
    Frame(); Drain(); const LONG d = s.r.lastP;
    const std::set<LONG> ctl = FirstMapTake();
    StampNow(); PlaceNow(); ClockAdd(3.0); PlaceNow();
    int once = 0; std::string times;
    for (LONG p = a; p <= d; p++) { const int t = AnsTimes(p); once += t == 1; times += Fmt("%s%ld:%d", times.empty() ? "" : " ", p, t); }
    printf("  (a) arms %ld and %ld, FG_HAND with no arm %ld, then %ld: taken up to %ld, answers (p:times) %s; published armed %ld, closed(unarmed) %ld, out of order %ld\n",
        a, b, c, d, s.pl.seen, times.c_str(), s.c.pubArmed, s.c.pubClosed[FG_CL_UNARMED], s.c.pubOrder);
    if (once != 4 || s.pl.seen != d || s.c.pubOrder) Err("H4 (a): not every p taken in order and answered exactly once (%s)", times.c_str());
    if (s.c.pubClosed[FG_CL_UNARMED] != 1) Err("H4 (a): the FG_HAND with no arm was not published CLOSED(unarmed)");
    const bool lost = ctl.size() == 1 && !ctl.count(a) && !ctl.count(b);
    if (!lost) Err("H4 control: the first map's single slot did not lose a publication: the control shows nothing");
    // (b)
    Fresh(o);
    Frame(); Drain(); const LONG a2 = s.r.lastP;
    StampNow(); PlaceNow(); ClockAdd(2.0); PlaceNow();          // a2 taken, arrived, ANSWERED by the placer, not yet resolved
    Frame(); Drain(); AnswerAs(s.r.lastP);                        // b2 (the drain resolves a2), c2, d2 armed before the placer looks again
    Frame(); Drain(); AnswerAs(s.r.lastP);
    Frame(); Drain();
    StampNow(); PlaceNow();
    const int a2n = AnsTimes(a2);
    printf("  (b) four arms, the first answered and still live when the table filled: live overflow %ld, pair %ld answered %d time(s)\n", s.c.liveOverflow, a2, a2n);
    if (a2n != 1) Err("H4 (b): the live table's overflow answered pair %ld %d times", a2, a2n);
    // (c) (nine pairs no placer answered: the first arms and the drains fire it, the eight behind it go inline as nosource,
    // their predecessor unanswered; X0's inline pairs did the same until the rig's mode key went in round 9's S9)
    Fresh(o);
    for (int i = 0; i < 9; i++) { Frame(); Drain(); }
    PlaceNow();
    for (int i = 0; i < 3; i++) { Frame(); Drain(); PlaceNow(); }
    s.wt.want = FG_WANT_PASS; Frame(); Drain(); PlaceNow();      // (the last pair armed is fired at the disengage's drain and answered)
    // BY THE PLACER'S OWN COUNTS: its first step answers nine pairs through an answer ring of eight, so the harness's log
    // cannot see them all (it would need the machine to stop between two answers); the counts see every Answer call.
    int twice = 0; for (LONG p = 1; p <= s.r.lastP; p++) twice += AnsTimes(p) > 1;
    const LONG answers = s.c.answers2p + s.c.answers2p1;
    printf("  (c) nine publications before the placer ran: taken up to %ld of %ld, %ld answers for %ld pairs, %d answered twice\n", s.pl.seen, s.r.lastP, answers, s.r.lastP, twice);
    if (s.pl.seen != s.r.lastP || answers != s.r.lastP || twice) Err("H4 (c): after nine publications the placer took up to %ld of %ld and gave %ld answers: it stopped at the overwritten slot", s.pl.seen, s.r.lastP, answers);
    Ctl("the first map's single slot took only %zu publication(s) of 4: LOST %s", ctl.size(), lost ? "the other three" : "none");
    CaseEnd("H4 publications");
}

// (H5, the inline modes, went with the rig's mode key in round 9's S9: X0's every pair inline was the key's own. The frame
// end it read, owned, stamped, found by the ring route, and its control, the present observer's suppression switched off,
// are T3's over the merged machine, the shape that ships, and F64's, below.)
// THE SUPPRESSION SKIPPED FOR ONE PAIR (round 9's S9, in place of the fault key's 64): a present observer standing in front of
// the machine's own that, for the ordinal the machine owns while pair p mod 100 = 20 is armed, forgets the ownership first,
// so that frame's end is signalled at the arm. Counted. F64's leg, T3's control. Swapped in after the engage's pointer check.
static volatile LONG gSkipSuppressed = 0;
template <class MM> static void PresentObs_SkipAt20(LONG k)
{
    FgState& s = B::S();
    if (k == s.w.lastOwnedK && s.w.openPhase == FG_PAIR_ARMED && (s.w.openP % 100) == 20) { s.w.lastOwnedK = -1; _InterlockedIncrement(&gSkipSuppressed); }
    MM::PresentObs(k);
}
static void SpinMs(double ms) { const LONGLONG t = B::Now() + (LONGLONG)(ms * (double)B::Qpf() / 1000.0); while (B::Now() < t) _mm_pause(); }
static bool SettlePlacer(DWORD ms)        // the real placer has taken and answered everything published
{
    FgState& s = B::S();
    return WaitFor([&s] { return s.pl.seen == ReadAcquire(&s.sh.pubP) && s.pl.liveN == 0; }, ms);
}
// ---------------------------------------------------------------- H6, stamps
// WHAT IT SHOWS (C-F3, C-F28, C-F8; map 2.6). The three stampers as REAL THREADS, the placer running beside them: an I
// marker signalled (a raw submit, as FirePresent's marker is) is stamped within 0.1 ms, sixty times (see inside for the
// rule that tells the OS's preemption from the stamper's); the END stamper
// counts a skipped value (f, then f + 2); a stamper that comes back from its wait 1 ms after the value came (the fake's
// tlInLate, in place of the fault key's 2 since round 9's S9) finds values 100 and 200 of fgSemIn already there and stamps
// them late, at least 1 ms after they were signalled, and counts them. CONTROL: value 300 with the stamper's wait honest is
// stamped on time and not counted late.
static void RawSignal(int tl, LONG v) { B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSignal(tl, v); B::RecSubmit(); B::QUnlock(); }
static bool StampOf(int tl, LONG v, LONGLONG* t, DWORD ms) { FgState& s = B::S(); return WaitFor([&] { return M::ReadStamp(s.st[tl], v, t); }, ms); }
static void CaseH6()
{
    CaseBegin("H6 stamps");
    FgState& s = B::S();
    Fresh(); Drain();
    StartThreads<M>(true, true);
    Sleep(20);
    // THE 0.1 ms BOUND UNDER LOAD (revised 2026-09-25 after the verifier's review: 1 run in 20 failed with one marker stamped
    // 0.49 ms late while a compile and another harness shared the machine; 80 runs alone, the worst was 0.053 ms). The bound
    // measures how fast the OS wakes the stamper thread, and one preemption is not the machine's. It is NOT widened. What
    // changed: 60 markers, not 20; a marker over the bound is measured again at once with a fresh value, and that one must be
    // within it (the stamper recovered, so it was a wake and not a stamper that polls or waits a slice); at most 2 of the 60
    // may need it, so a lateness that is the rule cannot hide behind the second measurements; and the median must be within
    // the bound. A stamper stamping from the placer's loop, or at its 250 ms slice, fails all three.
    std::vector<double> d, again;
    LONG val = 0;
    auto markOnce = [&](double* ms) -> bool
    {
        Sleep(2);
        const LONGLONG t0 = B::Now();
        RawSignal(FG_TL_MARKI, ++val);
        LONGLONG t = 0;
        if (!StampOf(FG_TL_MARKI, val, &t, 500)) { Err("H6: marker %ld was never stamped", val); return false; }
        *ms = M::TicksToMs(t - t0);
        return true;
    };
    for (int i = 0; i < 60; i++)
    {
        double ms = 0;
        if (!markOnce(&ms)) continue;
        d.push_back(ms);
        double ms2 = 0;
        if (ms > 0.1 && markOnce(&ms2)) again.push_back(ms2);
    }
    std::sort(d.begin(), d.end());
    const double worst = d.empty() ? -1 : d.back(), med = d.empty() ? -1 : d[d.size() / 2];
    const int over = (int)std::count_if(d.begin(), d.end(), [](double x) { return x > 0.1; });
    const int againOver = (int)std::count_if(again.begin(), again.end(), [](double x) { return x > 0.1; });
    const double worstAgain = again.empty() ? 0 : *std::max_element(again.begin(), again.end());
    B::LatEnd(1000); LONGLONG te = 0; StampOf(FG_TL_END, 1000, &te, 500);   // the first END value: a skip from the stamper's start, not the case's
    const LONG skip0 = s.c.endSkipped;
    B::LatEnd(1002); StampOf(FG_TL_END, 1002, &te, 500);
    const LONG skipped = s.c.endSkipped - skip0;
    B::tlInLate = 1;
    double lateMs[2] = {}; bool lateOk = true;
    const LONG late0 = s.c.stampedLate[FG_TL_IN];
    for (int k = 0; k < 2; k++)
    {
        const LONG v = 100 * (k + 1);
        RawSignal(FG_TL_IN, v - 1); Sleep(5);
        const LONGLONG t0 = B::Now();
        RawSignal(FG_TL_IN, v);
        LONGLONG t = 0;
        if (!StampOf(FG_TL_IN, v, &t, 500)) { Err("H6: IN %ld never stamped", v); lateOk = false; continue; }
        lateMs[k] = M::TicksToMs(t - t0);
        if (!s.st[FG_TL_IN][v & 63].late || lateMs[k] < 1.0) lateOk = false;
    }
    const LONG lateN = s.c.stampedLate[FG_TL_IN] - late0;
    B::tlInLate = 0;
    RawSignal(FG_TL_IN, 299); Sleep(5);
    const LONGLONG t3 = B::Now(); RawSignal(FG_TL_IN, 300);
    LONGLONG t = 0; const bool got300 = StampOf(FG_TL_IN, 300, &t, 500);
    const double ms300 = M::TicksToMs(t - t3); const bool late300 = s.st[FG_TL_IN][300 & 63].late != 0;
    StopThreads();
    printf("  marker stamps: median %.4f ms, worst %.4f ms over %zu, %d over 0.1 ms, measured again at once: %zu, worst %.4f ms; END f then f+2: %ld skip(s)"
           " counted; the stamper's wait made late: values 100 and 200 stamped %.3f and %.3f ms late, %ld counted late; control, 300 without it: %.4f ms, late %d\n",
        med, worst, d.size(), over, again.size(), worstAgain, skipped, lateMs[0], lateMs[1], lateN, ms300, late300);
    if (d.size() != 60) Err("H6: only %zu of 60 markers were stamped", d.size());
    if (med > 0.1) Err("H6: the median marker was stamped %.4f ms after its signal, over the 0.1 ms bound", med);
    if (over > 2) Err("H6: %d of 60 markers were stamped over the 0.1 ms bound (worst %.4f ms); at most 2 may be the OS's", over, worst);
    if (againOver || (int)again.size() != over) Err("H6: %d marker(s) measured again after one over the bound were over it too (worst %.4f ms), or not measured (%zu of %d)",
        againOver, worstAgain, again.size(), over);
    if (skipped != 1) Err("H6: the END stamper counted %ld skips for f, f + 2", skipped);
    if (!lateOk || lateN < 2) Err("H6: a stamper back late from its wait did not stamp 100 and 200 at least 1 ms late and count them");
    if (!got300 || late300 || ms300 > 0.5) Err("H6 control: value 300 with the stamper's wait honest was stamped late (%d) or slowly (%.3f ms): the control shows nothing", late300, ms300);
    Ctl("the stamper's wait honest: value 300 stamped in %.4f ms, not late", ms300);
    CaseEnd("H6 stamps");
}

// ---------------------------------------------------------------- H7, stale dues
// WHAT IT SHOWS (O-M8; map 2.6 "the dues"). The placer ON THE FAKE CLOCK takes pair a before its tI; the next frame's DRAIN
// is replayed before tI (I withheld, N fired); the placer's own next step retires a and raises nothing for it. A placer
// that read a as open an instant before the resolution landed raises FIRE_I(a) anyway (written by the harness, which is
// the one way it happens): no I is presented, at the next fire point of the next pair the stale due is counted, and pair
// b is untouched. CONTROL: the same due keyed to the open pair (a due with no pair identity): it presents an I.
static void CaseH7()
{
    CaseBegin("H7 stale dues");
    Opt o; o.fakeClock = true; Fresh(o);                     // (an I presented is read by the worker's count of I presents: the TAG mode went in round 9's S9)
    FgState& s = B::S();
    FakeFrames(6);
    ClockAdd(2.0); Frame(); Drain(); const LONG a = s.r.lastP;
    StampNow(); PlaceNow();                                   // taken, tS; the clock is before a's arrival and tI
    ClockAdd(0.5); Frame(); Drain(); const LONG b = s.r.lastP;   // the DRAIN, before tI: I(a) withheld, N(a) fired; b is nosource (a unanswered)
    StampNow(); PlaceNow();                                   // the placer retires a (answered 2a+1) and answers b's INLINE at once
    FgDue d = {};
    const bool none = !M::Read(*s.dueI, a, &d);
    DueI(a, FG_DUE_SHOW);                                     // the stale FIRE_I: a placer that read a as open just before its resolution
    ClockAdd(2.0); Frame(); Drain(); const LONG c = s.r.lastP;   // c arms (b answered)
    const LONG stale0 = s.c.staleDues, shown0 = s.c.iShown, pi0 = s.c.presentsI;
    FireNote(); Drain();
    const bool noI = s.c.presentsI == pi0;
    const bool cArmed = s.w.openPhase == FG_PAIR_ARMED && s.w.openP == c;
    printf("  the placer raised nothing for closed pair %ld %d; pair %ld inline (nosource) %d; the stale FIRE_I(%ld) at pair %ld's fire point: %ld stale due(s) counted,"
           " I shown %ld, an I presented %d; pair %ld still armed %d\n", a, none, b, AnsVal(b) == 2 * b + 1, a, c, s.c.staleDues - stale0, s.c.iShown - shown0, !noI, c, cArmed);
    if (!none) Err("H7: the placer raised a due for a pair its resolution had closed");
    if (s.c.staleDues <= stale0 || !noI || s.c.iShown != shown0) Err("H7: the stale FIRE_I was not counted, or presented an I");
    if (!cArmed) Err("H7: pair c was disturbed by the stale due");
    DueI(c, FG_DUE_SHOW); FireNote(); Drain();
    const bool saw = s.c.presentsI == pi0 + 1;
    if (!saw) Err("H7 control: the due keyed to the open pair presented nothing: the control shows nothing");
    Ctl("the due keyed to the open pair %s", saw ? "PRESENTED an I" : "presented nothing");
    CaseEnd("H7 stale dues");
}

// ---------------------------------------------------------------- H8, the lock
// WHAT IT SHOWS (O-M6, G-M3, C-F9; map 2.7.3, 2.7.5). An idle fire: the worker has run everything, published dtDone and
// found nothing (a harness note stands for the idle hook, which fg_machine.h does not have yet: it publishes dtDone past
// itself, as DtWorkerMain does before DtWorkerIdle, and calls the machine's Fire at point e). The fire is held INSIDE its
// fake Present; the render thread opens a DT_DIRECT scope, whose drain returns at once (the worker published it was done),
// and makes a D3D9 call: it must not reach the fake device until the fire ends. Where it can land is after the Present,
// before the restore: a hook inside the fire's marker submit waits there for it (150 ms). CONTROL 1: without the lock
// (NoLockB, DevLock and DevUnlock that do nothing): the call reaches the device inside the fire. CONTROL 2: a nested take
// (the idle hook taking the RAW interop lock, as the first map's did, and the fire taking it again): DXVK's one guard slot
// RELEASES the lock, and the call reaches the device inside the fire. And the machine's own depth count taken twice: the
// nested take is counted and the lock kept.
struct NoLockB : FgHarnessBinding { static void DevLock() {} static void DevUnlock() { CbDrop(); } };
typedef FgMachine<NoLockB> MNL;
static volatile LONG gH8Returned = 0, gH8DuringFire = -1, gH8AtGate = -1, gH8LockHeld = -1, gH8FireDone = 0;
template <class MM> static void H8IdleFire(DWORD, DWORD next, DWORD mode)
{
    // idle: everything run, nothing more. A CAUGHT-UP publish, as DtWorkerMain's own before DtWorkerIdle (since the fold of
    // DLSS-IV 1.0.8, 2026-10-03: DtWorkerDone takes the worker's count of dwords run and whether it has caught up). The count
    // is DtWorkerMain's local, which a note cannot see, so it is left as last published (the machine's WorkerAt says why).
    DtWorkerDone(next, (DWORD)dtDoneDw, true);
    if (mode == 2) B::DevLock();                      // control 2: the idle hook takes the raw interop lock
    if (mode == 3) MM::Lock();                        // the machine's own count, taken twice
    MM::Fire(FG_PT_E, next);
    if (mode == 3) MM::Unlock();
    if (mode == 2) B::DevUnlock();
    _InterlockedExchange(&gH8FireDone, 1);
}
// THE WINDOW'S LENGTH FOLLOWS DXVK'S LOCK WORD (revised 2026-09-25 after one control run in eight missed its 150 ms window on a
// loaded machine). Held by this thread (the fire's lock): 150 ms, in which the call must NOT land. Free (no lock, or a nested
// take that released it): nothing keeps the call out, so the hook waits up to 2 s for the call that MUST land. The main case's
// window is unchanged; only the controls stop depending on the scheduler.
static void H8Hook(int, ULONGLONG)
{
    gH8LockHeld = gRingOwner == (LONG)GetCurrentThreadId() ? 1 : 0;
    const ULONGLONG t = GetTickCount64() + (gH8LockHeld ? 150 : 2000);
    while (!gH8Returned && GetTickCount64() < t) Sleep(1);
    gH8DuringFire = gH8Returned;
}
template <class MM> static bool H8Run(int mode, LONG* nest)
{
    Fresh<MM>(); Warm<MM>();
    FgState& s = B::S();
    Frame<MM>(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p); DueI(p, FG_DUE_SHOW);
    gH8Returned = 0; gH8DuringFire = -1; gH8AtGate = -1; gH8LockHeld = -1; gH8FireDone = 0;
    const LONG nest0 = s.w.nest;
    gRing.signalInside = H8Hook;
    RingGateHoldNext();
    Run(&H8IdleFire<MM>, (DWORD)mode);
    if (!WaitFor([] { return RingGateHeld(); }, 3000)) { Err("H8: the idle fire never reached its Present"); RingGateRelease(); return false; }
    Later l = { [] { return true; }, [] { gH8AtGate = gH8Returned; RingGateRelease(); }, 10, 60 };
    HANDLE h = Spawn(LaterMain, &l);
    { DT_DIRECT(); D->ColorFill(gOtherRt, nullptr, 0x0088u); }
    _InterlockedExchange(&gH8Returned, 1);
    Join(h, "H8's release");
    // THE FIRE MUST HAVE ENDED BEFORE ITS HOOK'S VERDICT IS READ (2026-09-25, found by the fixer's repeat runs: the controls
    // failed 5 times in 39 full runs, once in 33 at a5bb8e81 beside it, and never in 30 runs of H8 alone). An idle fire published dtDone
    // PAST itself (that is what idle means), so Drain returns at once while it still runs: the call could return, the
    // render thread read gH8DuringFire (still -1) before the hook woke from its Sleep(1) to write 1, and the control said the
    // call never landed. The hook's own trace showed it: the call returned 63 ms after the hold and the verdict was unset.
    // So wait for the harness note that made the fire to finish, bounded (the hook's longest window is 2 s).
    if (!WaitFor([] { return gH8FireDone != 0; }, 5000)) Err("H8 (mode %d): the idle fire did not end within 5 s", mode);
    Drain();
    if (gRing.signalInside) { gRing.signalInside = nullptr; Err("H8: the fire's marker submit never ran the hook"); }
    *nest = s.w.nest - nest0;
    if (gH8AtGate != 0) Err("H8 (mode %d): the scope's call reached the device while the fire was held INSIDE its Present", mode);
    if ((mode == 0 || mode == 3) && gH8LockHeld != 1) Err("H8 (mode %d): DXVK's lock was not held by the fire between its Present and its restore", mode);
    return gH8DuringFire == 1;
}
static void CaseH8()
{
    CaseBegin("H8 the lock");
    LONG nest;
    const bool leak0 = H8Run<M>(0, &nest);
    // THE TWO NESTED SHAPES take the lock BEFORE the fire, so the fire's command buffers are taken under it: the span-order
    // rule the map gives FgRigIdle (it never calls the interop lock, O-M6) is broken by construction here, counted, put back.
    const LONG cbu0 = gRingSt.cbUnderLock;
    LONG nest3;
    const bool leak3 = H8Run<M>(3, &nest3);
    LONG nest1; const bool leak1 = H8Run<MNL>(1, &nest1);
    LONG nest2; const bool leak2 = H8Run<M>(2, &nest2);
    const LONG cbUnder = gRingSt.cbUnderLock - cbu0; gRingSt.cbUnderLock = cbu0;
    const LONG nr = gRingSt.interopNestRelease;
    printf("  the scope's call reached the device inside the fire: with the lock %d, the machine's count taken twice %d (nest counted %ld); control 1, no lock %d;"
           " control 2, the raw lock taken twice %d (%ld interop releases so far); command buffers taken under the lock by the two nested shapes %ld\n", leak0, leak3, nest3, leak1, leak2, nr, cbUnder);
    if (leak0) Err("H8: a DT_DIRECT's call reached the device inside an idle fire");
    if (leak3 || nest3 != 1) Err("H8: the machine's nested take lost the lock (%d) or was not counted (%ld)", leak3, nest3);
    if (!leak1) Err("H8 control 1: without the lock the call still did not reach the device inside the fire: the control shows nothing");
    if (!leak2) Err("H8 control 2: a raw nested take did not release the lock: the control shows nothing");
    Ctl("1: no lock, the call LANDED inside the fire %d; 2: raw nested take, LANDED %d", leak1, leak2);
    CaseEnd("H8 the lock");
}
// ---------------------------------------------------------------- H9, a stood-aside Reset during a checkpoint fire
// WHAT IT SHOWS (O-S1; map 2.15). A fire at a checkpoint (the game's next ColorFill, replayed, observed at dtObs[35]) is
// held inside its fake Present for 1.3 s. Another thread Resets: DtResetBarrier waits its 1 s and STANDS ASIDE (the worker
// never finishes), so the rig moves to DEAD (FgRigOnResetBarrier is fg_rig.h's, not the machine's: the harness stands in
// for its line with the machine's Kill), waits the fire out (the machine's fireBusy, the flag every span holds from before
// its lock to after its unlock, bounded at 1 s as the map's barrier is) and only then calls DXVK's Reset. So the fire's
// restore lands on the image it was aimed at before the Reset destroys it (the fake counts a raw copy that reaches a dead
// image), and nothing is presented from the Reset's thread. A hook inside the fire's marker submit (after the Present,
// before the restore) waits 200 ms for the Reset, which is where a Reset with nothing in its way would land. CONTROL: the
// first map's barrier (no fireBusy wait) AND no lock (NoLockB): the Reset lands between the Present and the restore and
// the restore's raw copy names a destroyed image. (Either one alone keeps it out: the interop lock holds DXVK's device lock
// across the restore, and the fireBusy wait holds the Reset itself.)
// EACH PROTECTION ALONE (2026-09-25, added after the verifier's review, which found that a fire span that never set
// fireBusy passed every case: the lock alone kept the Reset out, so the wait the appendix names, "the barrier's wait covers
// the restore's submit", was never shown to do anything on its own). In the mod the wait is the one that matters: the
// lock orders CPU calls, and only the wait holds a Reset back while a restore's raw submit may still be in flight (map
// 2.15). So two more legs: THE WAIT ALONE (NoLockB, the barrier's wait on), which must reach no destroyed image, and THE
// LOCK ALONE (the machine's lock, no wait), which must not either. In the wait-alone leg the hook's window is the short one
// even though nothing holds DXVK's lock: the barrier's wait is bounded at 1 s (map 2.15), so a 2 s window would outlast
// it by design, and the leg would measure the bound rather than the wait. The main run's control (neither) is the one that
// shows the window is real.
static volatile LONG gH9ResetDone = 0, gH9Waited = 0, gH9ResetTid = 0, gH9InWindow = 0;
static const char* volatile gH9How = "";
static bool gH9Wait = true;
static bool gH9Short = false;            // the wait-alone leg: the hook's 200 ms window with nothing holding DXVK's lock
static DWORD WINAPI H9Reset(void*)
{
    gH9ResetTid = (LONG)GetCurrentThreadId();
    DWORD ms = 0;
    gH9How = DtResetBarrier(&ms);
    if (!strcmp(gH9How, "stood aside")) M::Kill("a Reset stood aside (the harness, standing in for FgRigOnResetBarrier)");
    // THE WINDOW, made deterministic: the Reset is made once the fire is between its Present and its restore (the hook below
    // says so), which is where one with nothing in its way would land; otherwise which thread took DXVK's lock first when
    // the held Present returned would decide the control.
    { const ULONGLONG t = GetTickCount64() + 3000; while (!gH9InWindow && GetTickCount64() < t) Sleep(0); }
    if (gH9Wait) { const LONGLONG t0 = B::Now(); while (InterlockedCompareExchange(&B::S().sh.fireBusy, 0, 0) && B::Now() - t0 < B::Qpf()) Sleep(0); gH9Waited = 1; }
    D3DPRESENT_PARAMETERS pp = ResetParams();
    typedef HRESULT(__stdcall* Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    ((Reset_t)dtDx[16])(D, &pp);
    _InterlockedExchange(&gH9ResetDone, 1);
    return 0;
}
// As H8's hook: 200 ms while the fire holds DXVK's lock (the Reset must not get in), up to 2 s when nothing holds it (the
// control's Reset will get in, and the scheduler must not decide whether it is seen to). The wait-alone leg asks for the
// short window (gH9Short), for the reason in the case's comment.
static void H9Hook(int, ULONGLONG)
{
    _InterlockedExchange(&gH9InWindow, 1);
    const ULONGLONG t = GetTickCount64() + (gH9Short || gRingOwner == (LONG)GetCurrentThreadId() ? 200 : 2000);
    while (!gH9ResetDone && GetTickCount64() < t) Sleep(1);
}
template <class MM> static void H9Run(bool wait, LONG* dead, bool* fromReset, LONG* state, bool* stood)
{
    Fresh<MM>(); Warm<MM>();
    FgState& s = B::S();
    Frame<MM>(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p); DueI(p, FG_DUE_SHOW);
    const RingStats st0 = gRingSt;
    gH9ResetDone = 0; gH9Waited = 0; gH9Wait = wait; gH9How = ""; gH9InWindow = 0;
    gRing.signalInside = H9Hook;
    RingGateHoldNext();
    Fill(NewTag()); DtPublishFenced();                        // the checkpoint: its replay fires I, held in the Present
    if (!WaitFor([] { return RingGateHeld(); }, 3000)) { Err("H9: the checkpoint fire never reached its Present"); RingGateRelease(); return; }
    HANDLE hr = Spawn(H9Reset);
    Later l = { [] { return true; }, [] { RingGateRelease(); }, 10, 1300 };
    HANDLE h = Spawn(LaterMain, &l);
    Join(h, "H9's release"); Join(hr, "H9's Reset");
    *dead = gRingSt.deadImage - st0.deadImage;
    gRingSt.deadImage = st0.deadImage;                        // put back: the case asserts on it itself, below
    *stood = !strcmp(gH9How, "stood aside");
    *state = s.sh.state;
    const auto v = Shown();
    *fromReset = false; for (const RingPresented& e : v) if ((LONG)e.tid == gH9ResetTid) *fromReset = true;
    Drain();                                                  // the render thread answers the ask the barrier left up: the layer disarms
    if (gRing.signalInside) { gRing.signalInside = nullptr; Err("H9: the fire's marker submit never ran the hook"); }
}
static void CaseH9()
{
    CaseBegin("H9 a stood-aside Reset during a checkpoint fire");
    LONG dead, state; bool fromReset, stood;
    H9Run<M>(true, &dead, &fromReset, &state, &stood);
    printf("  the barrier %s; the rig %s; the fire's restore reached a destroyed image %ld time(s); presented from the Reset's thread %d\n",
        stood ? "stood aside" : "did NOT stand aside", state == FG_DEAD ? "DEAD" : "not dead", dead, fromReset);
    if (!stood) Err("H9: the barrier did not stand aside (%s), so the case is not the one the appendix names", gH9How);
    if (state != FG_DEAD) Err("H9: the rig is not DEAD after a stood-aside Reset");
    if (dead) Err("H9: the fire's restore reached an image the Reset had destroyed");
    if (fromReset) Err("H9: something was presented from the Reset's thread");
    // EACH PROTECTION ALONE (see the case's comment)
    LONG deadW, stateW; bool fromResetW, stoodW;
    gH9Short = true; H9Run<MNL>(true, &deadW, &fromResetW, &stateW, &stoodW); gH9Short = false;
    LONG deadL, stateL; bool fromResetL, stoodL;
    H9Run<M>(false, &deadL, &fromResetL, &stateL, &stoodL);
    printf("  the fireBusy wait alone (no lock): the barrier %s, the restore reached a destroyed image %ld time(s), presented from the Reset's thread %d;"
           " the lock alone (no wait): the barrier %s, %ld time(s), %d\n", stoodW ? "stood aside" : "did NOT stand aside", deadW, fromResetW,
           stoodL ? "stood aside" : "did NOT stand aside", deadL, fromResetL);
    if (!stoodW || !stoodL) Err("H9: the barrier did not stand aside in the one-protection legs (wait alone %d, lock alone %d)", stoodW, stoodL);
    if (deadW) Err("H9: with the lock off, the barrier's fireBusy wait alone did not keep the Reset off the restore's image (%ld)", deadW);
    if (deadL) Err("H9: with the wait off, the interop lock alone did not keep the Reset off the restore's image (%ld)", deadL);
    if (fromResetW || fromResetL) Err("H9: something was presented from the Reset's thread in a one-protection leg");
    (void)stateW; (void)stateL;
    LONG deadC, stateC; bool fromResetC, stoodC;
    H9Run<MNL>(false, &deadC, &fromResetC, &stateC, &stoodC);
    printf("  control: no fireBusy wait and no lock: the restore reached a destroyed image %ld time(s)\n", deadC);
    if (!deadC) Err("H9 control: with neither the wait nor the lock the restore still missed the Reset: the control shows nothing");
    Ctl("no fireBusy wait and no lock: the restore hit a DESTROYED image %ld time(s)", deadC);
    // A REFUSED SUBMIT BEFORE A RESET (the lifecycle map's fault 18, fixed 2026-10-06; FgMachine::BeforeReset). The wait before
    // a Reset asks for the last value a submit TOOK, so with one submit refused it returns at once and nothing dies. CONTROL:
    // a binding that asks for the last value ASKED (fg_rig.h's line until that day): it waits its whole 2 s, at every
    // Reset from then on, and the machine goes DEAD on the time-out.
    {
        Fresh(); Warm(); Frame(); Drain();
        _InterlockedExchange(&gRing.submitFailNext, 1);
        RawEmpty();                                           // one submit of the binding's, refused
        const LONG asked = B::subAsked, took = B::subTook;
        ULONGLONG t0 = GetTickCount64();
        const int r1 = M::BeforeReset(false, "the harness");
        const ULONGLONG ms1 = GetTickCount64() - t0;
        const LONG st1 = B::S().sh.state;
        t0 = GetTickCount64();
        const int r2 = MAK::BeforeReset(false, "the harness");
        const ULONGLONG ms2 = GetTickCount64() - t0;
        const LONG st2 = B::S().sh.state;
        printf("  a refused submit before a Reset (asked %ld, taken %ld): the wait for what was TAKEN returned %d in %llu ms and the machine is %s; the wait for what was ASKED returned %d in %llu ms and the machine is %s\n",
            asked, took, r1, (unsigned long long)ms1, st1 == FG_LIVE ? "live" : "NOT LIVE", r2, (unsigned long long)ms2, st2 == FG_DEAD ? "DEAD" : "not dead");
        if (asked != took + 1) Err("H9: the refused submit left asked %ld and taken %ld; the leg needs one value nothing signals", asked, took);
        if (r1 != 0 || ms1 > 500 || st1 != FG_LIVE) Err("H9: after a refused submit the wait before a Reset returned %d in %llu ms (0 at once wanted) and the machine is %s", r1, (unsigned long long)ms1, st1 == FG_LIVE ? "live" : "not live");
        if (r2 != 2 || ms2 < 1900 || st2 != FG_DEAD) Err("H9 control: a wait for the last value ASKED returned %d in %llu ms: it did not run out its 2 s, so the control shows nothing", r2, (unsigned long long)ms2);
        Ctl("a wait for the last value ASKED after a refused submit: its whole %llu ms, and the machine DEAD", (unsigned long long)ms2);
    }
    CaseEnd("H9 a stood-aside Reset during a checkpoint fire");
}

// ---------------------------------------------------------------- H10, a disarm between FIRE_I and FIRE_N, then a re-arm
// WHAT IT SHOWS (O-S6; map 2.15). Pair p armed, its I fired (the placer's FIRE_I, written by the harness); then the render
// thread's own Reset disarms (the disarm tail drops N: no present from a foreign thread, and a Reset is a hitch anyway);
// then the placer's FIRE_N for p arrives (it had not seen the disarm), and the layer re-arms (300 presents). No stale fire:
// the next pair's fire points count the due stale and N(p) is never presented after the disarm; p is closed, and the
// placer (on the real clock, called by the harness) answers it exactly once. CONTROL: the disarm observer switched off
// (the pair survives the disarm, as it would with no rig observer in the tail): after the re-arm the old pair is fired
// into the new images, a stale fire.
static void H10Run(bool control, LONG* stale, bool* nAfter, int* answered, LONG* p)
{
    Fresh(); Warm();
    FgState& s = B::S();
    const DWORD f = Frame(); Drain();
    *p = s.r.lastP;
    AnswerAs(*p);
    DueI(*p, FG_DUE_SHOW); FireNote(); Drain();
    const LONG gen0 = dtArmCount;
    if (control) dtDisarmObs = PrevDisarm;
    const size_t base = Shown().size();
    D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp);
    if (control) dtDisarmObs = &M::OnDisarm;
    { FgDue d = {}; d.p = *p; d.gen = gen0; d.verdict = FG_DUE_FIRE; M::Publish(*s.dueN, *p, d); }   // the placer's FIRE_N, raised before it saw the disarm
    const LONG stale0 = s.c.staleDues;
    if (!EnsureArmed()) Err("H10: no re-arm");
    Frame(); Drain();
    FireNote(); Drain();
    PlaceNow();
    *stale = s.c.staleDues - stale0;
    *nAfter = ShownHas(Shown(), base, f);
    *answered = AnsTimes(*p);
}
static void CaseH10()
{
    CaseBegin("H10 a disarm between FIRE_I and FIRE_N, then a re-arm");
    LONG stale, p; bool nAfter; int ans;
    H10Run(false, &stale, &nAfter, &ans, &p);
    printf("  pair %ld: after the disarm and re-arm, stale dues counted %ld, N presented after the disarm %d, answered %d time(s)\n", p, stale, nAfter, ans);
    if (stale < 1) Err("H10: the stale FIRE_N was not counted");
    if (nAfter) Err("H10: pair %ld's N was presented after the disarm", p);
    if (ans != 1) Err("H10: pair %ld was answered %d times", p, ans);
    LONG staleC, pC; bool nAfterC; int ansC;
    H10Run(true, &staleC, &nAfterC, &ansC, &pC);
    printf("  control: the disarm observer off: N presented after the disarm %d\n", nAfterC);
    if (!nAfterC) Err("H10 control: with the disarm observer off the old pair was not fired after the re-arm: the control shows nothing");
    Ctl("the disarm observer off: the old pair's N %s after the re-arm", nAfterC ? "WAS PRESENTED into the new images" : "was not presented");
    CaseEnd("H10 a disarm between FIRE_I and FIRE_N, then a re-arm");
}

// ---------------------------------------------------------------- H11, FgRigSleepUntil
// WHAT IT SHOWS (O-S2, C-F23; map 2.11 item 5). The machine's sleep (SleepUntil, over the binding's PaceWait: the rig's own
// high-resolution timer and the pace-wake event), 8 ms long, woken twice by the placer's event (2 and 4 ms in): each wake
// records FG_FIRE(PLACER) and the sleep goes on; a sleep whose timer woke within its 0.6 ms slack returns within 0.1 ms of
// its due (a timer that woke past the slack is the OS's lateness, which the machine counts and no sleep can take back: see
// inside); and only the one timer wake is counted as timer lateness, never an event wake. Five sleeps within the slack,
// the worst reported. On the render thread, armed and LIVE. CONTROL: the first map's sleep, LatSleepUntil's shape as the
// first map used it, which ended at the first early wake: it returns milliseconds before its due.
static DWORD WINAPI WakeTwice(void*) { SpinMs(2.0); B::WakePace(); SpinMs(2.0); B::WakePace(); return 0; }
static void CaseH11()
{
    CaseBegin("H11 FgRigSleepUntil");
    Fresh(); Warm();
    FgState& s = B::S();
    // WHICH SLEEPS THE 0.1 ms BOUND IS FOR (2026-09-25, after 125 sleeps measured one at a time): with the slack and the spin,
    // every sleep whose timer woke within its 0.6 ms slack returned within 0.0004 ms of its due, and the only late returns
    // (0.10 and 0.23 ms) were sleeps whose TIMER woke past the slack (0.70 and 0.83 ms late), which no sleep routine can take
    // back (LatSleepUntil's own comment) and which the machine counts itself (timerPastSlack). So the bound is held on the
    // sleeps whose timer woke within the slack, at least five of them (up to fifteen tries on a loaded machine), and every
    // past-slack wake must have been counted past the slack by the machine, with a timer lateness over the slack.
    double worst = 0, worstEarly = 0; int bad = 0, inSlack = 0, past = 0, countWrong = 0, tries = 0;
    for (; tries < 15 && inSlack < 5; tries++)
    {
        const LONG w0 = s.r.wakes, n0 = s.r.timerN, ps0 = s.r.timerPastSlack; const LONGLONG l0 = s.r.timerLateSum;
        const LONG fires0 = s.c.noteRefused;
        ResetEvent(B::evPace);
        const LONGLONG due = B::Now() + B::Qpf() * 8 / 1000;
        HANDLE h = Spawn(WakeTwice);
        M::SleepUntil(due);
        const LONGLONG t1 = B::Now();
        Join(h, "H11's waker");
        const double late = M::TicksToMs(t1 - due);
        const double counted = M::TicksToMs(s.r.timerLateSum - l0);
        // the one timer wake's lateness against the instant the timer was set for: never negative, and never the distance
        // of an event wake from the due (2 and 4 ms here), which is what counting an event wake would add
        if (s.r.wakes - w0 != 2 || s.r.timerN - n0 != 1 || counted < 0.0 || counted > 3.0 || s.c.noteRefused != fires0) bad++;
        if (s.r.timerPastSlack == ps0)
        {
            inSlack++;
            if (late > worst) worst = late;
            if (-late > worstEarly) worstEarly = -late;
            if (counted > 0.6) countWrong++;                  // past the slack by its lateness, yet not counted past it
        }
        else { past++; if (counted <= 0.6) countWrong++; }  // counted past the slack, yet within it by its lateness
        Drain();
    }
    // CONTROL
    ResetEvent(B::evPace);
    const LONGLONG due = B::Now() + B::Qpf() * 8 / 1000;
    HANDLE h = Spawn(WakeTwice);
    B::PaceWait(due);                                         // the first map: one wait, ended by the first wake
    const double early = M::TicksToMs(due - B::Now());
    Join(h, "H11's control waker"); SpinMs(10.0); ResetEvent(B::evPace);
    printf("  %d sleeps of 8 ms woken twice each; %d with the timer within its slack: returned at most %.4f ms after the due (at most %.4f ms before it);"
           " %d timer wakes past the slack, counted by the machine (the count disagreeing with the lateness: %d); runs with the wrong wake or lateness counts %d;"
           " control: the first map's sleep returned %.3f ms early\n", tries, inSlack, worst, worstEarly, past, countWrong, bad, early);
    if (inSlack < 5) Err("H11: only %d of %d sleeps had their timer wake within the slack: the bound was not measured", inSlack, tries);
    if (worst > 0.1 || worstEarly > 0.0) Err("H11: a sleep whose timer woke within its slack returned %.4f ms after or %.4f ms before its due, outside [0, 0.1] ms", worst, worstEarly);
    if (countWrong) Err("H11: %d timer wake(s) whose past-the-slack count disagreed with their lateness", countWrong);
    if (bad) Err("H11: %d run(s) did not count exactly 2 event wakes and 1 timer wake, or counted an event wake as timer lateness", bad);
    if (early < 1.0) Err("H11 control: the first map's sleep did not end early: the control shows nothing");
    Ctl("the first map's sleep ended %.3f ms EARLY at the first wake", early);
    CaseEnd("H11 FgRigSleepUntil");
}

// (H12, readiness and the pad, went with the rig's fixed K and its planted late answers in round 9's S9: at readiness, the only
// stub placer left, I is never late and the stub calls no pad. The merged placer's pad is P1's, a late answer moving it by one
// and a pair with no frame not at all; the pad's own law, CapLate and CapOnTime against A5's, is equiv_fgpace's.)

// ---------------------------------------------------------------- H13, CLOSING
// WHAT IT SHOWS (O-S5; map 2.4, 2.15). A stop with a pair armed: the rig is CLOSING, the present observer is unchained at
// once (so LatWaiterStop finds its own at the head), the next fire point (the game's next draw, a checkpoint) fires N once
// and withholds I ("closed at stop"), the checkpoint observers are removed, and a second point finds nothing to fire. A pair
// whose FG_HAND and FG_HUDLESS ran before the stop and whose Present comes after it (the stop is a harness note on the
// worker between them, as IvtFail can run there) is presented inline by FgWorkerPresent. CONTROL: the stop made DEAD
// instead of CLOSING (the machine's Kill): the armed pair's N is never fired and the real frame is lost.
static void StopOnWorker(DWORD, DWORD, DWORD) { M::Stop("the harness, on the worker between a pair's notes and its Present"); }
static void CaseH13()
{
    CaseBegin("H13 CLOSING");
    Fresh(); Warm();
    FgState& s = B::S();
    const DWORD f = Frame(); Drain();
    const size_t base = Shown().size();
    const LONG atStop0 = s.c.nAtStop, cl0 = s.c.iWithheld[12];
    M::Stop("the harness");
    const bool unchained = dtPresentObs == PrevPresent;
    Fill(NewTag()); Drain();                                  // the next fire point: a checkpoint
    const bool ckGone = dtObs[35] == nullptr && dtObs[82] == PrevObs82;
    Fill(NewTag()); FireNote(FG_PT_D); Drain();               // a second point: nothing left
    auto v = Shown();
    int fN = 0; for (size_t i = base; i < v.size(); i++) fN += v[i].tag == f;
    printf("  stop with pair armed: state %ld, the present observer unchained %d; N fired at the next point %d time(s), closed at stop %ld, I withheld (closing) %ld;"
           " checkpoint observers removed %d\n", s.sh.state, unchained, fN, s.c.nAtStop - atStop0, s.c.iWithheld[12] - cl0, ckGone);
    if (s.sh.state != FG_CLOSING || !unchained) Err("H13: the stop did not move to CLOSING and unchain the present observer");
    if (fN != 1 || s.c.nAtStop - atStop0 != 1 || s.c.iWithheld[12] - cl0 != 1) Err("H13: N was not fired exactly once with I withheld at the stop");
    if (!ckGone) Err("H13: the checkpoint observers were not removed");
    // the pair whose Present comes after the stop
    Fresh(); Warm();
    Facts(); const DWORD g = NewTag(); Fill(g);
    RecordDrain(); RecordHand(); RecordHudless();
    Run(StopOnWorker);
    const size_t b2 = Shown().size();
    const LONG in0 = s.c.pubInline[FG_IN_CLOSING];
    RecordPresent(); Drain();
    v = Shown();
    printf("  a pair's Present after a stop on the worker: published INLINE(closing) %ld, shown %s (N %lX)\n", s.c.pubInline[FG_IN_CLOSING] - in0, TagList(v, b2).c_str(), g);
    if (s.c.pubInline[FG_IN_CLOSING] - in0 != 1 || !ShownHas(v, b2, g)) Err("H13: FgWorkerPresent did not present the pair inline after the stop");
    // CONTROL
    Fresh(); Warm();
    const DWORD fc = Frame(); Drain();
    const size_t b3 = Shown().size();
    M::Kill("the harness's control: a stop straight to DEAD");
    Fill(NewTag()); Drain(); Frame(); Drain();
    v = Shown();
    const bool lost = !ShownHas(v, b3, fc);
    if (!lost) Err("H13 control: with the stop made DEAD the armed pair's N was still shown: the control shows nothing");
    Ctl("a stop straight to DEAD: the armed pair's real frame %s", lost ? "was LOST (never presented)" : "was shown");
    CaseEnd("H13 CLOSING");
}

// ---------------------------------------------------------------- H14, span order
// WHAT IT SHOWS (G-N3, O-S4; map 2.7.3 "the span's order"). Pairs with every span kind (FG_HAND's, FG_HUDLESS's, I's and
// N's fires) at 1, 2, 3 and 4 images: no fake D3D9 call is made while the fake submission queue is held by its holder
// (dt_fake_ring.h counts it: d3d9_interfaces.h:185-196), and every span takes its command buffers before the interop lock
// (counted too); both 0. (Shape D, and the probe's and TAG's samples, went with the rig's keys in round 9's S9: the stub's
// spans are shape Z's alone.) CONTROLS: a binding whose RecSubmit makes the handle query
// first (a D3D9 call under the queue) and a binding whose CbTake defers the real take to DevLock (a take under the lock):
// each is counted.
struct QCallB : FgHarnessBinding { static bool RecSubmit() { (void)FgHarnessBinding::BBImage(); return FgHarnessBinding::RecSubmit(); } };
struct CbLateB : FgHarnessBinding
{
    static inline int want = 0;
    static bool CbTake(int n) { want = n; return true; }
    static void DevLock() { FgHarnessBinding::DevLock(); FgHarnessBinding::CbTake(want); }
};
typedef FgMachine<QCallB> MQC;
typedef FgMachine<CbLateB> MCL;
template <class MM> static void H14Pairs(int pairs)
{
    FgState& s = B::S();
    for (int i = 0; i < pairs; i++)
    {
        Frame<MM>(); Drain();
        const LONG p = s.r.lastP;
        AnswerAs(p);
        Fill(NewTag()); DueI(p, FG_DUE_SHOW); DueN(p); FireNote(); Drain();
    }
    B::Flush(); RawEmpty();
}
static void CaseH14()
{
    CaseBegin("H14 span order");
    const RingStats s0 = gRingSt;
    int runs = 0;
    for (int n = 1; n <= 4; n++)
    {
        Opt o; o.images = n;
        Fresh(o); Warm(); H14Pairs<M>(3); runs++;
    }
    printf("  %d runs of 3 pairs at 1 to 4 images: D3D9 calls under the queue by its holder %ld, command buffers taken under the lock %ld,"
           " begun with none taken %ld; %ld D3D9 calls by other threads while it was held (allowed)\n",
        runs, gRingSt.qD3D9 - s0.qD3D9, gRingSt.cbUnderLock - s0.cbUnderLock, gRingSt.cbUntaken - s0.cbUntaken, gRingSt.qD3D9Foreign - s0.qD3D9Foreign);
    if (gRingSt.qD3D9 != s0.qD3D9 || gRingSt.cbUnderLock != s0.cbUnderLock || gRingSt.cbUntaken != s0.cbUntaken) Err("H14: a span broke its order");
    // CONTROLS
    const RingStats c0 = gRingSt;
    Fresh<MQC>(); Warm<MQC>(); H14Pairs<MQC>(1);
    const LONG q = gRingSt.qD3D9 - c0.qD3D9;
    Fresh<MCL>(); Warm<MCL>(); H14Pairs<MCL>(1);
    const LONG cb = gRingSt.cbUnderLock - c0.cbUnderLock;
    ViolRestore(c0);
    if (!q) Err("H14 control: a D3D9 call inside the queue was not counted: the control shows nothing");
    if (!cb) Err("H14 control: a command buffer taken under the lock was not counted: the control shows nothing");
    Ctl("a query inside the queue: %ld COUNTED; a take deferred into the lock: %ld COUNTED", q, cb);
    CaseEnd("H14 span order");
}

// ---------------------------------------------------------------- H15, FG_HAND closes an open pair
// WHAT IT SHOWS (O-M3; map 2.5 "close anything still open", 2.7.1). With the DRAIN note refused (the render thread's step 1
// finds nothing to drain: the harness clears the fact it is recorded by, as a refused DtNote leaves it), FG_HAND(p+1) finds
// pair p armed, fires N(p) through the fire routine as a drain (I withheld) and counts "hand found open"; then pair p+1
// arms, its checkpoint observers are installed a second time, and FgObs82 is NOT chained to itself: a replayed draw calls
// the tracker's observer exactly once.
// REVISED 2026-09-25 after the verifier's review, which found this case's control a tautology: it assigned the first map's
// value by hand, compared it and put it back, and no machine code ran (a mutant that saved prevObs82 at every checkpoint
// install, as the first map did, passed every case). Two things protect the second half: prevObs82 is written in one
// place, Install, behind the once-flag (O-N3), never by CheckpointsInstall; and CheckpointsInstall returns at once while
// installed (the ckInstalled guard). Every close removes the observers before the next arm installs them, so an arm alone
// never reaches the guard. So the case now exercises the machine's own code, not a hand-made value: the second arm (after a
// close), AND a checkpoint install made AGAIN WHILE INSTALLED (on the worker, at its place in the stream), each followed by
// a replayed draw that must call the tracker's observer exactly once. The first map's install, run as a mutant of the
// machine (prevObs82 saved at every install, no guard), fails that second leg (2026-09-25); the harness has no seam to
// switch the guard off from outside, so this half has no in-harness control, said here as the header's rule asks.
// CONTROL, for the first half: the first map's FG_HAND, which forgot an open pair instead of closing it (G2's wrapper),
// with the DRAIN refused again: FG_HAND(p+1) overwrites FG_PRESENT and pair p's frame is never shown.
static void CaseH15()
{
    CaseBegin("H15 FG_HAND closes an open pair");
    Fresh(); Warm();
    FgState& s = B::S();
    const DWORD f = Frame(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p);
    const size_t base = Shown().size();
    const LONG ho0 = s.c.handOpen, wh0 = s.c.iWithheld[11];
    s.r.lastEngaged = false;                                  // the DRAIN note refused
    Frame(); Drain();
    const LONG q = s.r.lastP;
    auto v = Shown();
    const bool nShown = ShownHas(v, base, f);
    const bool armed2 = s.w.openPhase == FG_PAIR_ARMED && s.w.openP == q;
    const LONG c0 = gObs82Calls;
    D->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1); Drain();
    const LONG calls = gObs82Calls - c0;
    printf("  the DRAIN refused: hand found open %ld, I withheld (hand open) %ld, N(%ld) shown %d; pair %ld armed %d; dtObs[82] is FgObs82 %d, its previous is the"
           " tracker's %d; one replayed draw called the tracker's observer %ld time(s)\n", s.c.handOpen - ho0, s.c.iWithheld[11] - wh0, p, nShown, q, armed2,
        dtObs[82] == &M::Obs82, s.ptr.prevObs82 == PrevObs82, calls);
    if (s.c.handOpen - ho0 != 1 || s.c.iWithheld[11] - wh0 != 1 || !nShown) Err("H15: FG_HAND did not close the open pair by firing its N");
    if (!armed2 || dtObs[82] != &M::Obs82 || s.ptr.prevObs82 != PrevObs82 || calls != 1) Err("H15: the second arm chained FgObs82 wrongly (calls %ld)", calls);
    // A CHECKPOINT INSTALL MADE AGAIN WHILE INSTALLED, on the worker at its place in the stream, then a replayed draw.
    // Looked at BEFORE the draw is replayed: a previous observer that IS FgObs82 would recurse until the stack ran out, and
    // the run would end with no row (the first map's install, as a mutant of the machine, did exactly that before this test).
    Run([](DWORD, DWORD, DWORD) { M::CheckpointsInstall(); });
    Drain();
    const bool selfChained = s.ptr.prevObs82 == &M::Obs82;
    const LONG c1 = gObs82Calls;
    if (!selfChained) { D->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1); Drain(); }
    const LONG calls2 = selfChained ? -1 : gObs82Calls - c1;
    printf("  a checkpoint install made again while installed: dtObs[82] is FgObs82 %d, its previous is the tracker's %d, is FgObs82 itself %d; one replayed"
           " draw called the tracker's observer %ld time(s)\n", dtObs[82] == &M::Obs82, s.ptr.prevObs82 == PrevObs82, selfChained, calls2);
    if (selfChained) Err("H15: a checkpoint install made again chained FgObs82 to ITSELF (a replayed draw would recurse; none was replayed)");
    else if (dtObs[82] != &M::Obs82 || s.ptr.prevObs82 != PrevObs82 || calls2 != 1) Err("H15: a checkpoint install made again chained FgObs82 wrongly (calls %ld)", calls2);
    // CONTROL (the first half): the first map's FG_HAND, with the DRAIN refused again
    AnswerAs(q);
    const DWORD fc = Frame(); Drain();                        // its DRAIN fires N(q); pair r arms with frame fc
    const LONG pr = s.r.lastP;
    AnswerAs(pr);
    const size_t bc = Shown().size();
    s.r.lastEngaged = false;                                  // the DRAIN note refused
    dtNoteObs[DT_NOTE_FG_HAND] = Hand_FirstMap;
    Frame(); Drain();
    dtNoteObs[DT_NOTE_FG_HAND] = &M::NoteHand;
    v = Shown();
    const bool lost = !ShownHas(v, bc, fc);
    printf("  control, the first map's FG_HAND with the DRAIN refused: after pair %ld's frame %lX the log adds %s\n", pr, fc, TagList(v, bc).c_str());
    if (!lost) Err("H15 control: the first map's FG_HAND still showed pair %ld's frame: the control shows nothing", pr);
    Ctl("the first map's FG_HAND (forgets the open pair), the DRAIN refused: pair %ld's frame %s; the no-self-chaining half has no in-harness control (see the case)",
        pr, lost ? "NEVER SHOWN" : "shown");
    CaseEnd("H15 FG_HAND closes an open pair");
}

// ---------------------------------------------------------------- V1, a fire publishes where the worker is (map 2.7.2)
// NOT AN APPENDIX CASE: added 2026-09-25 after the verifier's review, whose mutant that dropped the fire span's
// DtWorkerDone(own) passed every case. WHAT IT SHOWS (map 2.7.2, and its DECIDED line against "the next command's
// index"). A fire can block for a refresh inside its Present, and a non-driving note or observer does not publish dtDone
// first (dt_core.h, DtWorkerMain), so every span begins with DtWorkerDone(own), own being the command BEING EXECUTED. Two
// consequences, one leg each, with the fire at a checkpoint (the game's ColorFill, replayed, observed at dtObs[35]) held
// inside its fake Present and a SetRenderState recorded just before that ColorFill in the same publication: (A) a wait
// aimed at the earlier command (DtWaitPast, as a query's GetData makes it) returns while the fire is held; (B) a wait
// aimed at the checkpointed command itself does NOT return until the fire ends, which is why own is the executing
// command's index and never the next one (with the next, a drain aimed at dtW could return while the fire runs).
// CONTROL: the same two commands with the harness's gate note in place of the ColorFill: a hold that publishes nothing,
// which is the fire without its DtWorkerDone. The wait aimed at the earlier command sits out the whole hold. The worker
// also publishes dtDone after every 64th command it runs, so one run in 64 the publish lands between the two commands
// and the control cannot bite; the control is therefore made up to three times and must bite once. The protection itself
// is a line of the machine, with no seam to switch it off from outside; the two mutants (the line dropped, and own made
// the next index) were run against this case, 2026-09-25.
static volatile LONG gV1Released = 0;
static void V1Release() { _InterlockedExchange(&gV1Released, 1); RingGateRelease(); }
static void V1GateRelease() { _InterlockedExchange(&gV1Released, 1); SetEvent(gGateEv); }
// Records the earlier command at *x and the holding command at *a, published together. Near the ring's end the ring is
// first walked round, so neither index is a wrap marker's.
static bool V1Record(bool gate, DWORD* x, DWORD* a)
{
    for (int i = 0; i < 20000 && dtRingDw - (DWORD)dtW < 4096; i++) D->SetRenderState(D3DRS_ZENABLE, (DWORD)(i & 1));
    Drain();
    const DWORD x0 = (DWORD)dtW;
    D->SetRenderState(D3DRS_ZENABLE, FALSE);
    const DWORD a0 = (DWORD)dtW;
    bool ok = true;
    if (gate) { const DWORD g = 0x6A7Eu; ok = DtNote(kHarnessGate, &g, 4); }
    else Fill(NewTag());
    DtPublishFenced();
    *x = x0; *a = a0;
    return ok && a0 > x0;
}
static void CaseV1()
{
    CaseBegin("V1 a fire publishes the executing command's index (map 2.7.2)");
    Fresh(); Warm();
    FgState& s = B::S();
    Frame(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p); DueI(p, FG_DUE_SHOW);
    const LONG shown0 = s.c.iShown;
    gV1Released = 0;
    RingGateHoldNext();
    DWORD x = 0, a = 0;
    if (!V1Record(false, &x, &a)) Err("V1: the two commands were not recorded in order");
    Later l = { [] { return RingGateHeld(); }, V1Release, 3000, 300 };
    HANDLE h = Spawn(LaterMain, &l);
    if (!WaitFor([] { return RingGateHeld(); }, 3000)) Err("V1: the checkpoint fire never reached its Present");
    DtWaitPast(x, DT_R_DIRECT);
    const LONG relA = gV1Released;
    DtWaitPast(a, DT_R_DIRECT);
    const LONG relB = gV1Released;
    Join(h, "V1's release");
    Drain();
    printf("  fire held inside its Present at the checkpoint (index %lu): the wait for the command before it (index %lu) returned %s the release;"
           " the wait for the checkpointed command itself returned %s it; I shown %ld\n", a, x, relA ? "AFTER" : "before", relB ? "after" : "BEFORE", s.c.iShown - shown0);
    if (s.c.iShown != shown0 + 1) Err("V1: the checkpoint did not fire I (%ld)", s.c.iShown - shown0);
    if (relA) Err("V1: a wait aimed at a command the worker had run sat out the fire: the span did not publish where the worker was");
    if (!relB) Err("V1: a wait aimed at the command being executed returned while its fire was held: the span published past it");
    // CONTROL: a hold that publishes nothing (the harness's gate), up to three times (see above)
    int tries = 0; bool bit = false;
    for (; tries < 3 && !bit; tries++)
    {
        gV1Released = 0;
        DWORD xc = 0, ac = 0;
        if (!V1Record(true, &xc, &ac)) { Err("V1 control: the gate was refused"); break; }
        Later lc = { [] { return gGateIn != 0; }, V1GateRelease, 3000, 300 };
        HANDLE hc = Spawn(LaterMain, &lc);
        if (!WaitFor([] { return gGateIn != 0; }, 3000)) Err("V1 control: the worker never reached the gate");
        DtWaitPast(xc, DT_R_DIRECT);
        bit = gV1Released != 0;
        Join(hc, "V1's control release");
        Drain();
    }
    if (!bit) Err("V1 control: with a hold that publishes nothing the wait for the earlier command still returned early, three times: the control shows nothing");
    Ctl("a hold that publishes nothing: the wait for the command before it %s (try %d of 3)", bit ? "SAT OUT THE HOLD" : "returned early", tries);
    CaseEnd("V1 a fire publishes the executing command's index (map 2.7.2)");
}
// ---------------------------------------------------------------- the rest, carried (map-final.md's appendix, last list)

// A FOREIGN RESET WITH N PENDING (map 2.15; plan:555). Pair p armed, its N not fired; another thread Resets with the render
// thread idle, so the barrier disarms on the Reset's thread: the disarm tail drops the pair's N and I (counted), publishes
// nothing new (the pair was published ARMED) and the placer (real clock, called by the harness) retires and answers it
// once; nothing is presented from the Reset's thread. CONTROL: a disarm observer that fires the open pair's N before the
// machine's (the tempting "don't lose the frame"): a present from the Reset's thread, onto images about to be destroyed.
static void Disarm_FiresFirst() { M::Fire(FG_PT_CLOSING, (DWORD)dtDone); M::OnDisarm(); }
static volatile LONG gForeignResetTid = 0;
static DWORD WINAPI ResetFromTid(void*) { gForeignResetTid = (LONG)GetCurrentThreadId(); D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp); return 0; }
static void ForeignResetRun(bool control, LONG* dropped, bool* fromReset, int* answered, LONG* p)
{
    Fresh(); Warm();
    FgState& s = B::S();
    Frame(); Drain();
    *p = s.r.lastP;
    const LONG d0 = s.c.nDropped;
    if (control) dtDisarmObs = Disarm_FiresFirst;
    HANDLE h = Spawn(ResetFromTid); Join(h, "the foreign Reset");
    if (control) dtDisarmObs = &M::OnDisarm;
    *dropped = s.c.nDropped - d0;
    const auto v = Shown();
    *fromReset = false; for (const RingPresented& e : v) if ((LONG)e.tid == gForeignResetTid) *fromReset = true;
    PlaceNow();
    *answered = AnsTimes(*p);
    if (!EnsureArmed()) Err("the foreign Reset case: no re-arm");
}
static void CaseForeignReset()
{
    CaseBegin("R1 a foreign Reset with N pending");
    LONG dropped, p; bool fromReset; int ans;
    ForeignResetRun(false, &dropped, &fromReset, &ans, &p);
    printf("  pair %ld armed, a Reset from another thread: N dropped %ld, presented from the Reset's thread %d, answered %d time(s)\n", p, dropped, fromReset, ans);
    if (dropped != 1 || fromReset || ans != 1) Err("R1: the pair was not dropped, counted and answered once, or a present came from the Reset's thread");
    LONG dc, pc; bool fc; int ac;
    ForeignResetRun(true, &dc, &fc, &ac, &pc);
    if (!fc) Err("R1 control: a disarm observer that fires first did not present from the Reset's thread: the control shows nothing");
    Ctl("a disarm observer that fires N first: %s", fc ? "a PRESENT FROM THE RESET'S THREAD" : "nothing");
    CaseEnd("R1 a foreign Reset with N pending");
}

// FG_FIRE REFUSED ACROSS A DISARM (the NoteAcrossDisarm shape, dt_test.cpp:1188-1228). FG_FIRE is made a driving note for
// the case so its post fetches, and the fake's GetTexture starts a Reset from another thread and returns once the ask is
// up; the post's Release answers it. With the device thread's re-test the note is REFUSED (FgPace's sleep counts it and
// goes on): nothing is left in the ring, the worker never sees it. CONTROL: the re-test off: the note is committed into a
// ring nobody publishes and replayed after the re-arm, a fire request from a generation that no longer exists.
static int gFireRuns = 0;
static void FireCount(DWORD k, const DWORD* p, DWORD n) { gFireRuns++; M::NoteFire(k, p, n); }
static bool FireAcrossDisarm(bool recheck, bool* posted, int* replayed)
{
    Fresh(); Warm();
    MakeTexturesUnknown();
    dtNoteObs[DT_NOTE_FG_FIRE] = FireCount; gFireRuns = 0;
    dtNoteDrives[DT_NOTE_FG_FIRE] = true; dtBodySampFree = 0x1FFFFFu & ~1u;
    gNoteRecheck = recheck; gH3Reset = nullptr; gFkOnGetTexture = H3OnGetTexture;
    *posted = FireNote();
    gFkOnGetTexture = nullptr; gNoteRecheck = true;
    if (gH3Reset) { Join(gH3Reset, "the foreign Reset"); gH3Reset = nullptr; } else Err("FG_FIRE across a disarm: the post never fetched a texture");
    const bool orphan = dtW != (DWORD)dtPub || (DWORD)dtDone != (DWORD)dtPub;
    if (!EnsureArmed()) Err("FG_FIRE across a disarm: no re-arm");
    Drain();
    dtNoteDrives[DT_NOTE_FG_FIRE] = false; dtBodySampFree = 0;
    dtNoteObs[DT_NOTE_FG_FIRE] = &M::NoteFire;
    *replayed = gFireRuns;
    return orphan;
}
static void CaseFireAcrossDisarm()
{
    CaseBegin("R2 FG_FIRE refused across a disarm");
    bool posted; int replayed;
    const bool orphan = FireAcrossDisarm(true, &posted, &replayed);
    printf("  with the re-test: posted %d, orphaned %d, run by the worker %d time(s)\n", posted, orphan, replayed);
    if (posted || orphan || replayed) Err("R2: FG_FIRE across a disarm was not refused cleanly");
    bool postedC; int replayedC;
    const bool orphanC = FireAcrossDisarm(false, &postedC, &replayedC);
    printf("  control, the re-test off: posted %d, orphaned %d, replayed after the re-arm %d time(s)\n", postedC, orphanC, replayedC);
    // FgPace publishes every FG_FIRE it records (DtPublishFenced, SleepUntil), and so does FireNote: a note the re-test
    // would have refused is therefore not left in the ring, it is RUN by the worker at once, on a layer already disarmed,
    // beside the other thread's Reset. That, not the orphan dt_test's injection note shows, is this note's hazard.
    if (!postedC || replayedC != 1) Err("R2 control: with the re-test off the note was not posted and run after the disarm: the control shows nothing");
    Ctl("re-test off: FG_FIRE POSTED into a disarmed layer and RUN by the worker %d time(s) (orphaned %d)", replayedC, orphanC);
    CaseEnd("R2 FG_FIRE refused across a disarm");
}

// THE SLEEPER COUNT (M12; map 2.11). With a refresh rate the rig's pacing owns (2 ms: C = 4.06 ms), FgPace sleeps each
// engaged present; the count taken from inside the sleepers says one sleeper a present, never two. CONTROL: both sleepers
// forced: LatOnPresent's hook (called inside dtScPresent, where LatPace sleeps) moves latPaceSleptN, standing for a LatPace
// that was not parked: the count sees 2 on every present that FgPace also slept.
static void LatPaceSleeps() { B::paceSleptN++; }
static void SleeperRun(bool both, LONG* s1, LONG* s2, int frames)
{
    Fresh(); Warm();
    FgState& s = B::S();
    B::refreshPacked = (500ull << 32) | 1ull;
    for (int i = 0; i < 4; i++) { Frame(); Drain(); AnswerAs(s.r.lastP); }
    const LONG a = s.r.sleeps1, b = s.r.sleeps2;
    if (both) gLatOnPresentHook = LatPaceSleeps;
    for (int i = 0; i < frames; i++) { Frame(); Drain(); AnswerAs(s.r.lastP); }
    gLatOnPresentHook = nullptr;
    *s1 = s.r.sleeps1 - a; *s2 = s.r.sleeps2 - b;
}
static void CaseSleepers()
{
    CaseBegin("R3 the sleeper count");
    LONG s1, s2; SleeperRun(false, &s1, &s2, 20);
    printf("  20 engaged presents at C = 4.06 ms: one sleeper %ld, two sleepers %ld\n", s1, s2);
    if (s2 || s1 < 15) Err("R3: the count saw %ld presents with one sleeper and %ld with two", s1, s2);
    LONG c1, c2; SleeperRun(true, &c1, &c2, 20);
    printf("  control, LatPace forced to sleep too: one sleeper %ld, two sleepers %ld\n", c1, c2);
    if (c2 < 15) Err("R3 control: both sleepers forced and the count saw 2 only %ld times: the control shows nothing", c2);
    Ctl("both sleepers forced: the count saw 2 on %ld of 20 presents", c2);
    CaseEnd("R3 the sleeper count");
}

// STRAY CALLERS PASS THROUGH (plan:222; map 2.4). FgWorkerPresent called on the render thread (dtScPresent's unarmed and
// dirty-region paths call the callee directly) passes to DXVK's Present, arming nothing; FgSwapPresent called on the worker
// (a harness note) and on another thread passes to dtScPresent and records nothing. The render-thread call is made with
// the pair OPEN and its one-shot pointing at the very ordinal it would take, so the thread test is the only one that can
// refuse it. CONTROL: the device thread's own worker id pointed at the render thread for that one call (the worker idle):
// the same call ARMS the pair from the render thread and presents nothing, the frame the game asked for vanishes.
static void SwapOnWorker(DWORD, DWORD, DWORD) { M::SwapPresent(SC, nullptr, nullptr, nullptr, nullptr, 0); }
static DWORD WINAPI SwapElsewhere(void*) { M::SwapPresent(SC, nullptr, nullptr, nullptr, nullptr, 0); return 0; }
static bool StrayWorkerPresent(bool control, LONG* pass, LONG* armed, bool* shown)
{
    Fresh(); Warm();
    FgState& s = B::S();
    Facts(); const DWORD f = NewTag(); Fill(f);
    RecordDrain(); RecordHand(); RecordHudless(); Drain();    // the pair OPEN, FG_HUDLESS seen, its one-shot on the next ordinal
    const bool ready = s.w.openPhase == FG_PAIR_OPEN && s.w.handFor == dtPresDone + 1 && s.w.handGen == dtArmCount;
    const size_t base = Shown().size();
    const LONG p0 = s.c.presentsPass, a0 = s.c.pubArmed;
    const DWORD keep = dtWorkerTid;
    if (control) dtWorkerTid = GetCurrentThreadId();
    typedef HRESULT(__stdcall* P_t)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);
    ((P_t)dtScPresentO)(SC, nullptr, nullptr, nullptr, nullptr, 0);
    dtWorkerTid = keep;
    *pass = s.c.presentsPass - p0; *armed = s.c.pubArmed - a0;
    *shown = ShownHas(Shown(), base, f);
    return ready;
}
static void CaseStray()
{
    CaseBegin("R4 stray callers pass through");
    FgState& s = B::S();
    LONG pass, armed; bool shown;
    const bool ready = StrayWorkerPresent(false, &pass, &armed, &shown);
    printf("  FgWorkerPresent on the render thread (every other test would admit it %d): passed through %ld, armed %ld, the frame shown %d\n", ready, pass, armed, shown);
    if (!ready || pass != 1 || armed || !shown) Err("R4: FgWorkerPresent on the render thread did not pass through");
    // FgSwapPresent on the worker and on another thread
    Fresh(); Warm();
    const LONG n0 = s.r.nextP, rec0 = dtPresRecN, pass0 = s.c.presentsPass, fp0 = dtForeignPresents;
    const size_t base = Shown().size();
    Run(SwapOnWorker); Drain();
    const auto v = Shown();
    const bool fromWorker = v.size() > base && v.back().tid == dtWorkerTid;
    HANDLE h = Spawn(SwapElsewhere); Join(h, "the foreign FgSwapPresent");
    printf("  FgSwapPresent on the worker: notes recorded %ld, presents recorded %ld, passed through %ld, presented from the worker %d; on another thread:"
           " counted foreign %ld\n", s.r.nextP - n0, dtPresRecN - rec0, s.c.presentsPass - pass0, fromWorker, dtForeignPresents - fp0);
    if (s.r.nextP != n0 || dtPresRecN != rec0 || !fromWorker) Err("R4: FgSwapPresent on the worker acted");
    if (dtForeignPresents - fp0 != 1) Err("R4: FgSwapPresent on another thread did not pass to dtScPresent as a foreign present");
    LONG passC, armedC; bool shownC;
    StrayWorkerPresent(true, &passC, &armedC, &shownC);
    printf("  control, the worker's id pointed at the render thread: passed through %ld, armed %ld, the frame shown %d\n", passC, armedC, shownC);
    if (!armedC || shownC) Err("R4 control: without the thread test the render thread's call did not arm the pair: the control shows nothing");
    Ctl("the thread test defeated: the render thread's call ARMED the pair (%ld) and its frame %s", armedC, shownC ? "was shown" : "VANISHED");
    CaseEnd("R4 stray callers pass through");
}

// A FIRE'S FAILED PRESENT REACHES THE GAME EXACTLY ONCE, through dtPresHr (R2 F4; map 2.7.3). The N fire's Present is made
// to fail (the fake's lost device); the game's next Presents (FgSwapPresent's own return, which is dtScPresent's) see
// D3DERR_DEVICELOST exactly once. CONTROL: dtPresHr cleared behind the fire (a machine that did not fold its failure in):
// the game never hears of the lost device.
static int FailedFireRun(bool control)
{
    Fresh(); Warm();
    FgState& s = B::S();
    Frame(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p);
    gFkPresentHr = (LONG)D3DERR_DEVICELOST;
    DueN(p); FireNote(); Drain();
    if (gFkPresentHr) { gFkPresentHr = 0; Err("the failed present case: the fire made no Present"); }
    if (control) _InterlockedExchange(&dtPresHr, 0);
    int lost = 0;
    for (int i = 0; i < 4; i++) { Facts(); Fill(NewTag()); if (Present() == D3DERR_DEVICELOST) lost++; Drain(); AnswerAs(s.r.lastP); }
    return lost;
}
static void CaseFailedPresent()
{
    CaseBegin("R5 a fire's failed Present reaches the game once");
    const int lost = FailedFireRun(false);
    printf("  the N fire's Present failed: the game's next four Presents returned the lost device %d time(s)\n", lost);
    if (lost != 1) Err("R5: the fire's failed Present reached the game %d times, not once", lost);
    const int lostC = FailedFireRun(true);
    if (lostC) Err("R5 control: with dtPresHr cleared the game still heard of it: the control shows nothing");
    Ctl("dtPresHr not folded: the game heard of the lost device %d time(s), NEVER", lostC);
    CaseEnd("R5 a fire's failed Present reaches the game once");
}

// FgWorkerPresent'S RECT COPY SURVIVES THE RING SLOT BEING OVERWRITTEN (R1 finding 2; map 2.5). A game Present with a
// source and a destination rect, armed; the render thread then records enough (25,000 Sets, 75,000 dwords against a
// 65,536-dword ring) that the ring wraps over the Present's command; then N fires: the fake's Present receives the
// original rects (its log keeps them). CONTROL: the pointer the ring handed FgWorkerPresent, kept by a wrapper observer
// installed after the engage (so the machine's pointer check has not yet looked): it now points at something else, which
// a present that had kept the pointer would have sent.
static const RECT* volatile gRectSrcPtr = nullptr;
static HRESULT __stdcall WorkerPresent_Keep(IDirect3DSwapChain9* sc, const RECT* s, const RECT* t, HWND w, const RGNDATA* r, DWORD f)
{ if (s) gRectSrcPtr = s; return M::WorkerPresent(sc, s, t, w, r, f); }
static void CaseRects()
{
    CaseBegin("R6 FgWorkerPresent's rect copy survives the ring slot being overwritten");
    Fresh(); Warm();
    FgState& s = B::S();
    const RECT src = { 11, 22, 1911, 1072 }, dst = { 5, 6, 1285, 726 };
    dtScPresentO = (void*)&WorkerPresent_Keep;
    gRectSrcPtr = nullptr;
    Facts(); Fill(NewTag()); Present(&src, &dst); Drain();
    dtScPresentO = (void*)&M::WorkerPresent;
    const LONG p = s.r.lastP;
    AnswerAs(p);
    const bool armed = s.w.openPhase == FG_PAIR_ARMED && s.w.openP == p;
    for (int i = 0; i < 25000; i++) D->SetRenderState(D3DRS_STENCILREF, (DWORD)i);
    Drain();
    RECT seen = {}; if (gRectSrcPtr) memcpy(&seen, (const void*)gRectSrcPtr, sizeof(seen));
    const bool overwritten = gRectSrcPtr && memcmp(&seen, &src, sizeof(RECT)) != 0;
    EnterCriticalSection(&gFkLock); gFk.clear(); LeaveCriticalSection(&gFkLock);
    DueN(p); FireNote(); Drain();
    RECT gs = {}, gd = {}; bool found = false;
    EnterCriticalSection(&gFkLock);
    for (size_t i = gFk.size(); i-- > 0;) if (gFk[i].slot == 1803 && gFk[i].data.size() == 2 * sizeof(RECT)) { memcpy(&gs, gFk[i].data.data(), sizeof(RECT)); memcpy(&gd, gFk[i].data.data() + sizeof(RECT), sizeof(RECT)); found = true; break; }
    LeaveCriticalSection(&gFkLock);
    printf("  armed %d; after the ring wrapped the Present's slot reads {%ld,%ld,%ld,%ld}; the fire presented src {%ld,%ld,%ld,%ld} dst {%ld,%ld,%ld,%ld}\n", armed,
        seen.left, seen.top, seen.right, seen.bottom, gs.left, gs.top, gs.right, gs.bottom, gd.left, gd.top, gd.right, gd.bottom);
    if (!armed || !found || memcmp(&gs, &src, sizeof(RECT)) || memcmp(&gd, &dst, sizeof(RECT))) Err("R6: the fire did not present the original rects");
    if (!overwritten) Err("R6 control: the ring slot still held the rect after the wrap: the control shows nothing");
    Ctl("the ring's own pointer, kept, would have sent {%ld,%ld,%ld,%ld}: %s", seen.left, seen.top, seen.right, seen.bottom, overwritten ? "OVERWRITTEN" : "unchanged");
    CaseEnd("R6 FgWorkerPresent's rect copy survives the ring slot being overwritten");
}

// FgInjectObs CALLS THE SAVED OBSERVER EXACTLY ONCE ON EVERY PATH, the dead rig included (map 2.7.1). The harness raises a
// pending count as it posts each injection note, and the saved observer (IvtInjectNoteW's stand-in) takes it down, as the
// injection's scope guard does. Paths: nothing open; a pair armed, LIVE, rule C1 (N fires early); a pair armed with rule
// C3 (no early fire); CLOSING with a pair armed; DEAD with a pair armed. After each the count is 0 and the observer was
// called once. CONTROL: an injection observer that returns early on a dead rig: the count stays up, a wait at every seam.
static void Inject_DeadReturns(DWORD k, const DWORD* p, DWORD n) { if (B::S().sh.state == FG_DEAD) return; M::InjectObs(k, p, n); }
static bool InjectOnce(const char* what, std::string* line)
{
    const LONG c0 = gInjCalls;
    const DWORD pay[4] = { 7, 8, 9, 10 };
    _InterlockedIncrement(&gInjPending);
    if (!DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) { _InterlockedDecrement(&gInjPending); Err("the injection note was refused (%s)", what); return false; }
    DtPublishFenced(); Drain();
    const LONG calls = gInjCalls - c0;
    *line += Fmt("%s%s: %ld call, pending %ld", line->empty() ? "" : "; ", what, calls, gInjPending);
    return calls == 1 && gInjPending == 0;
}
static void CaseInjectOnce()
{
    CaseBegin("R7 FgInjectObs calls the saved observer exactly once");
    FgState& s = B::S();
    std::string line; bool ok = true;
    Fresh(); Warm(); Frame(); Drain(); AnswerAs(s.r.lastP); FireNote(FG_PT_D); Drain();
    ok &= InjectOnce("nothing open", &line);
    Frame(); Drain(); AnswerAs(s.r.lastP);
    const LONG e0 = s.c.nEarly;
    ok &= InjectOnce("armed, C1", &line);
    const bool c1 = s.c.nEarly == e0 + 1;
    Opt o3; Fresh(o3); gC1Off = 1; Warm(); Frame(); Drain(); AnswerAs(s.r.lastP);
    const LONG e3 = s.c.nEarly;
    ok &= InjectOnce("armed, C3", &line);
    const bool c3 = s.c.nEarly == e3 && s.w.openPhase == FG_PAIR_ARMED;
    Fresh(); Warm(); Frame(); Drain();
    M::Stop("the harness");
    const LONG st0 = s.c.nAtStop;
    ok &= InjectOnce("CLOSING, armed", &line);
    const bool closing = s.c.nAtStop == st0 + 1;
    Fresh(); Warm(); Frame(); Drain();
    M::Kill("the harness");
    ok &= InjectOnce("DEAD, armed", &line);
    printf("  %s; C1 fired N early %d, C3 did not %d, CLOSING fired N %d\n", line.c_str(), c1, c3, closing);
    if (!ok || !c1 || !c3 || !closing) Err("R7: the saved observer was not called exactly once on every path, or a path fired wrongly");
    // CONTROL
    Fresh(); Warm(); Frame(); Drain();
    M::Kill("the harness");
    dtNoteObs[DT_NOTE_INJECT] = Inject_DeadReturns;
    std::string cl; InjectOnce("DEAD, returning early", &cl);
    const LONG stuck = gInjPending;
    dtNoteObs[DT_NOTE_INJECT] = &M::InjectObs;
    gInjPending = 0;
    if (stuck != 1) Err("R7 control: an observer that returns early on a dead rig left the count at %ld: the control shows nothing", stuck);
    Ctl("an observer returning early on a dead rig: the pending count STAYED %ld (a wait at every seam)", stuck);
    CaseEnd("R7 FgInjectObs calls the saved observer exactly once");
}

// ---------------------------------------------------------------- THE CENSUS CLOSES over 10,000 random frames
// WHAT IT SHOWS (map 2.5, 2.6: every FG_HAND the worker executes is published exactly once, in p order; every published
// pair gets exactly one answer; every armed or inline pair one I resolution and one N resolution; every 'Q' its one 'G').
// Segments of up to 1,000 frames until 10,000 game frames have run (a disarm's re-arm presents are not counted, and a
// stop ends its segment early, so it takes eleven), each a fresh rig with 1 to 4 images drawn at random (the rig's keys the
// census drew beside them, shape, mode, a fixed K, the faults 4 and 8 and the probe, went in round 9's S9: their draws are
// still made, so every segment's stream of events is the one it was). The placer and the three stampers run as
// REAL THREADS on the real clock, and FgPace sleeps (C = 2.03 ms at a 1000 Hz refresh), so fire point b is real. Inside a
// segment, at random: a frame not sent (noinject), a camera cut, the HDR view, a game draw (checkpoints), an injection
// note (point c), a command-buffer refusal (a withheld I or a dropped N), a failed Present, a disarm (the render thread's
// own Reset, 300 presents to re-arm), and a stop (CLOSING, the segment ends). At each segment's end the rig is
// disengaged, the placer settles, and the census must close. The seed is fixed, so a failure repeats.
static unsigned gSeed = 0x2F6E2B1u;
static unsigned Rnd() { gSeed ^= gSeed << 13; gSeed ^= gSeed >> 17; gSeed ^= gSeed << 5; return gSeed; }
// the census's state block counts, kept for the run's last lines (main): Applies, on the recorded path, drained
static LONG gCensusSb[3] = { 0, 0, 0 };
struct CensusTotals { LONG frames = 0, pairs = 0, armed = 0, inl = 0, closed = 0, shown = 0, disarms = 0, stops = 0, injects = 0, cbFails = 0, lost = 0, onTime = 0; int bad = 0;
                      LONG sbApplies = 0, sbRecorded = 0, sbDrained = 0; };   // C1 beside the rig (CensusSegment says what they count)
// THE CONTROL'S SEAM: an FG_HAND observer that loses one note in 150 (the worker never opens, never publishes that pair).
static int gHandSkip = 0;
static void Hand_LosesOne(DWORD k, const DWORD* p, DWORD n) { if (++gHandSkip % 150 == 0) return; M::NoteHand(k, p, n); }
static std::string CensusSegment(int seg, int frames, CensusTotals& t, bool control = false)
{
    FgState& s = B::S();
    Opt o;
    (void)Rnd(); o.images = 1 + Rnd() % 4;
    for (int i = 0; i < 5; i++) (void)Rnd();                  // (the retired keys' draws: mode, a fixed K, the faults 4 and 8, the probe)
    Fresh(o);
    B::refreshPacked = (1000ull << 32) | 1ull;
    StartThreads<M>(true, true);
    const RingStats st0 = gRingSt;
    const LONG disarms0 = dtDisarms;
    // C1 BESIDE THE RIG (the fold of DLSS-IV 1.0.8, 2026-10-03). Since 1.0.8 the Windows mod records a state block's Apply into
    // the ring instead of draining it (C1, dtSbRecOn: this harness's third argument), so the game's Applies sit in the same
    // stream as the rig's notes and the worker runs them between the rig's spans. Every segment therefore carries a RECORDED
    // block (Begin, one render state, End) which the stream sets away from and Applies on about a quarter of its frames,
    // chosen from the frame's own draw: no extra draw of the seed, so the segments are the ones they were. What must hold: the
    // census still closes; the device ends on what the stream last did to that state; at threaded=2 the layer's own
    // validation finds the shadow equal to the device (main, at the end); and at B the Applies take the recorded path (no
    // drain, the answer "would take it") while at A every one drains (CaseCensus counts both, so each run is the other's control).
    const DWORD kSbX = D3DRS_STENCILREF, kSbVal = 0x51u;
    IDirect3DStateBlock9* sb = nullptr; DWORD sbExpect = 0; bool sbTouched = false;
    auto sbMake = [&]() { if (!sb && dtArmed) { D->BeginStateBlock(); D->SetRenderState((D3DRENDERSTATETYPE)kSbX, kSbVal); D->EndStateBlock(&sb); } };
    sbMake();
    const LONG sbFail0 = dtSbRecApplyFailed;                  // the worker's own count of recorded Applies whose replay failed
    if (control) { Frame(); gHandSkip = 0; dtNoteObs[DT_NOTE_FG_HAND] = Hand_LosesOne; }   // after the engage's pointer check
    bool stopped = false;
    for (int i = 0; i < frames && !stopped; i++)
    {
        const unsigned r = Rnd() % 1000;
        if (r < 3 && dtArmed)                                  // a disarm: the render thread's own Reset
        {
            if (sb) { sb->Release(); sb = nullptr; }           // a Reset wants every state block released
            sbTouched = false;                                 // and puts the state back to its default
            D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp); t.disarms++;
            if (!EnsureArmed()) { Err("census: no re-arm"); break; }
            sbMake();                                          // recorded again, armed (C1 learns a recording only while armed)
            continue;
        }
        if (r >= 3 && r < 4 && i > frames / 2) { M::Stop("the census"); stopped = true; t.stops++; break; }
        if (r >= 10 && r < 25) B::camJumps++;
        B::hdrView = (r >= 25 && r < 35);
        if (r >= 35 && r < 45) { _InterlockedExchange(&gRing.cbFailNext, 1); t.cbFails++; }
        if (r >= 45 && r < 50) gFkPresentHr = (LONG)D3DERR_DEVICELOST;
        if (sb && dtArmed && (r & 3) == 1)                     // C1 beside the rig: away from the block's value, then its Apply
        {
            D->SetRenderState((D3DRENDERSTATETYPE)kSbX, 0x10u + (r & 7));
            // RECORDED is the layer's own count of recorded Applies moving by one (dtSbRecApplies, dt_res.h), not its last
            // answer: that is written only inside DtSbApplyTry, so an Apply that went round it would read the one before
            // (the fold's review). DRAINED is a wait counted for the Apply, which is counted on entry.
            const LONG d0 = dtWaitN[DT_R_SBAPPLY], rec0 = dtSbRecApplies;
            sb->Apply();
            t.sbApplies++;
            if (dtWaitN[DT_R_SBAPPLY] != d0) t.sbDrained++;
            else if (dtSbRecApplies == rec0 + 1) t.sbRecorded++;
            sbExpect = kSbVal; sbTouched = true;
        }
        Facts(!(r >= 50 && r < 90));
        Fill(NewTag());
        if (r % 3 == 0) D->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1);
        if (r >= 90 && r < 150) { const DWORD pay[4] = { 1, 2, 3, 4 }; _InterlockedIncrement(&gInjPending); if (DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) t.injects++; else _InterlockedDecrement(&gInjPending); }
        Present();
        t.frames++;
    }
    // the end of the segment: disengage, so the last pair is drained, and let the placer take and answer everything
    s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 3; i++) { Facts(); Fill(NewTag()); Present(); }
    if (sb) { sb->Release(); sb = nullptr; }
    Drain();
    const DWORD sbDev = gFd->st.rs[kSbX];                       // what the device holds once everything queued has run
    if (control) dtNoteObs[DT_NOTE_FG_HAND] = &M::NoteHand;
    _InterlockedExchange(&gRing.cbFailNext, 0); gFkPresentHr = 0;
    SpinMs(30.0);
    const bool settled = SettlePlacer(5000);
    StopThreads();
    // THE CENSUS
    const LONG pairs = s.r.nextP - 1;
    LONG inl = 0, closed = 0; for (int i = 0; i < FG_IN_N; i++) inl += s.c.pubInline[i]; for (int i = 0; i < FG_CL_N; i++) closed += s.c.pubClosed[i];
    const LONG pub = s.c.pubArmed + inl + closed;
    LONG iRes = s.c.iShown; for (int i = 0; i < FG_W_N; i++) iRes += s.c.iWithheld[i];
    const LONG nRes = s.c.nOnTime + s.c.nEarly + s.c.nLate + s.c.nAtStop + s.c.nDropped;
    int once = 0, twice = 0; for (LONG p = 1; p <= pairs; p++) { const int k = AnsTimes(p); once += k == 1; twice += k > 1; }
    const LONG answers = s.c.answers2p + s.c.answers2p1;
    std::string bad;
    const auto log = RingPipeLog(); const int qpairs = PipePairs(log, &bad);
    std::string why;
    if (!settled) why += " the placer did not settle;";
    // every recorded Apply RAN: at threaded=1 only the segment's last one is read back from the device above, so the worker's
    // own count of failed replays must not have moved (the fold's review: 2,460 of 2,478 were otherwise unchecked there)
    if (dtSbRecApplyFailed != sbFail0) why += Fmt(" %ld recorded Applies failed on the worker;", (long)(dtSbRecApplyFailed - sbFail0));
    if (sbTouched && sbDev != sbExpect) why += Fmt(" the device's render state %lu holds 0x%lX after the recorded blocks, not 0x%lX;", (unsigned long)kSbX, (unsigned long)sbDev, (unsigned long)sbExpect);
    if (s.sh.pubP != pairs || pub != pairs || s.c.pubOrder) why += Fmt(" published %ld of %ld pairs (last %ld, out of order %ld);", pub, pairs, s.sh.pubP, s.c.pubOrder);
    if (s.pl.seen != pairs) why += Fmt(" the placer took %ld of %ld;", s.pl.seen, pairs);
    if (answers != pairs || twice) why += Fmt(" %ld answers for %ld pairs, %d answered more than once;", answers, pairs, twice);
    if (once != pairs && !gAnsLogMissed) why += Fmt(" the log saw %d of %ld pairs answered once;", once, pairs);
    if (iRes != s.c.pubArmed + inl) why += Fmt(" %ld I resolutions for %ld armed and inline pairs;", iRes, s.c.pubArmed + inl);
    // THE 6c COUNTERS (2026-09-27, the review of 6c's fixes: "the only proof of the machine's counters is the next game run").
    // Every nosource pair has exactly one cause; and the 2p+1 answers are the inline and closed pairs,
    // the pairs closed before their arrival or answered late, and at most one more for each lost publication, live overflow
    // and pair a render fact or noinject withheld (those are answered 2p+1 at their arrival unless retired first).
    {
        const LONG nsBy = s.c.nosourceBy[FG_NS_UNANSWERED] + s.c.nosourceBy[FG_NS_NOBUFFER];
        if (nsBy != s.c.pubInline[FG_IN_NOSOURCE]) why += Fmt(" %ld nosource causes for %ld nosource inline pairs;", nsBy, s.c.pubInline[FG_IN_NOSOURCE]);
    }
    {
        // exact: every 2p+1 answer is counted at its path (a fact at the arrival, a retirement before or after the arrival, stale)
        const LONG want = inl + closed + s.c.closedBeforeArrival + s.c.answeredLate + s.c.answeredFact + s.c.answeredStale;
        if (s.c.answers2p1 != want)
            why += Fmt(" %ld answers 2p+1 for %ld by path (inline %ld, closed %ld, closed before arrival %ld, answered late %ld, by fact %ld, stale %ld);",
                       s.c.answers2p1, want, inl, closed, s.c.closedBeforeArrival, s.c.answeredLate, s.c.answeredFact, s.c.answeredStale);
    }
    if (nRes != s.c.pubArmed || s.c.presentsInline != inl) why += Fmt(" %ld N resolutions for %ld armed pairs, %ld inline presents for %ld inline pairs;", nRes, s.c.pubArmed, s.c.presentsInline, inl);
    if (!bad.empty() || s.c.qWritten != s.c.gWritten) why += Fmt(" the pipe: %s ('Q' %ld, 'G' %ld);", bad.empty() ? "counts differ" : bad.c_str(), s.c.qWritten, s.c.gWritten);
    if (s.w.openPhase != FG_PAIR_NONE) why += " a pair is still open;";
    if (gInjPending) why += Fmt(" %ld injection(s) pending;", gInjPending);
    if (Viol(gRingSt) != Viol(st0)) why += " the fake's rules were broken;";
    printf("  segment %d: %d image(s)%s: %ld pairs (armed %ld, inline %ld, closed %ld), I shown %ld, N on time %ld early %ld late %ld"
           " at stop %ld dropped %ld, stale dues %ld, hand open %ld, 'Q' pairs %d, disarms %ld; %s\n", seg, o.images,
        stopped ? ", stopped" : "", pairs, s.c.pubArmed, inl, closed, s.c.iShown, s.c.nOnTime, s.c.nEarly, s.c.nLate, s.c.nAtStop, s.c.nDropped, s.c.staleDues, s.c.handOpen,
        qpairs, dtDisarms - disarms0, why.empty() ? "the census closes" : ("OPEN:" + why).c_str());
    B::refreshPacked = 0; B::camJumps = 0; B::hdrView = false;
    if (control) return why;
    if (!why.empty()) { t.bad++; Err("census segment %d:%s", seg, why.c_str()); }
    t.pairs += pairs; t.armed += s.c.pubArmed; t.inl += inl; t.closed += closed; t.shown += s.c.iShown; t.lost += gAnsLogMissed; t.onTime += s.c.nOnTime;
    return why;
}
static void CaseCensus()
{
    CaseBegin("R8 the census closes over 10,000 random frames");
    CensusTotals t;
    int seg = 0;
    while (t.frames < 10000 && seg < 20) CensusSegment(seg++, 1000, t);
    printf("  %ld frames, %ld pairs (armed %ld, inline %ld, closed %ld), %ld I shown, %ld disarms, %ld stops, %ld injections, %ld command-buffer refusals;"
           " %d segment(s) open; answers the log missed by a lap %ld\n", t.frames, t.pairs, t.armed, t.inl, t.closed, t.shown, t.disarms, t.stops, t.injects, t.cbFails, t.bad, t.lost);
    if (t.frames < 10000) Err("census: only %ld game frames ran", t.frames);
    // C1 BESIDE THE RIG: the recorded blocks' Applies by the path they took. At B (the Windows mod) they must take the recorded
    // path, at A every one must drain; each run is the other's control, and a run with no Apply proves nothing.
    printf("  state blocks beside the rig (C1 at %s): %ld Applies, %ld on the recorded path, %ld drained\n", DT_SB_REC_ON() ? "B" : "A", t.sbApplies, t.sbRecorded, t.sbDrained);
    gCensusSb[0] = t.sbApplies; gCensusSb[1] = t.sbRecorded; gCensusSb[2] = t.sbDrained;
    if (t.sbApplies < 1000) Err("census: %ld state block Applies in the stream, 1000 wanted: C1 beside the rig was not exercised", t.sbApplies);
    else if (DT_SB_REC_ON() && t.sbRecorded * 10 < t.sbApplies * 9) Err("census at B: %ld of %ld Applies took the recorded path (%ld drained), nine in ten wanted: C1 does not run beside the rig as it does in the mod", t.sbRecorded, t.sbApplies, t.sbDrained);
    else if (!DT_SB_REC_ON() && (t.sbRecorded || t.sbDrained * 10 < t.sbApplies * 9)) Err("census at A: %ld of %ld Applies drained and %ld were recorded: the drained Apply is not what ran", t.sbDrained, t.sbApplies, t.sbRecorded);
    // CONTROL: a segment of 600 frames whose FG_HAND observer loses one note in 150 (a mutated seam: the worker never opens
    // those pairs, so it never publishes them). The census must not close, or it proves nothing.
    CensusTotals tc;
    const std::string open = CensusSegment(seg, 600, tc, true);
    if (open.empty()) Err("census control: a worker that loses FG_HAND notes still closed the census: the control shows nothing");
    Ctl("an FG_HAND observer losing 1 note in 150: the census does NOT close:%s", open.c_str());
    CaseEnd("R8 the census closes over 10,000 random frames");
}

// ================================================================ STAGE S6 (docs/reports/fg-1c/s6-plan.md section 5)
// The calibration modes OFFSET and SLOPE, the early rule's inputs (fg_test_early = 2, the placer's plan record, C3's
// accounting at the injection), the CAP state and the ownership table, the route moved into the machine, the per-present
// export's read, W12's chain and bins, and fault 64. Each with its negative control, as every case above.

static double TickMs(LONGLONG t) { return (double)t * 1000.0 / (double)B::Qpf(); }
static LONGLONG MsTick(double ms) { return (LONGLONG)(ms * (double)B::Qpf() / 1000.0 + 0.5); }
// An END stamp written as the END stamper writes one (value, stamp, value).
static void StampEnd(LONG f, LONGLONG t)
{
    FgStamp& e = B::S().st[FG_TL_END][f & (FG_STAMPS - 1)];
    WriteRelease(&e.v1, -1); e.t = t; e.late = 0; WriteRelease(&e.v2, f); WriteRelease(&e.v1, f);
}

// (STAGE S6's calibration modes went with the rig's mode, hold and abba_pace keys in round 9's S9, and their cases by name:
// S6a (OFFSET's placer), S6b (OFFSET on the worker), S6c (SLOPE's hold), S6h (SLOPE in FusionFix's frame), S6d (C3 in SLOPE),
// S6e (the ABBA windows, WindowStep's one reader), S6g (the census in OFFSET and SLOPE) and R3b (the CAP of the pass-through
// half). What still guards what ships beside them: G3's both dues at one point (I then N in one call), H16 (C1's early
// fire), R8 (the census), T2 (CAP, ON and PASS as the merged path's wants), R3c (who paces).)

// R3c, WHO PACES (map 2.11 item 6; s6-plan.md 3.14): the ownership table at a 120 Hz refresh and lock 1.015 (C 16.92 ms, 59.1
// fps) with FusionFix's limiter: a cap of 58 is FusionFix's, 60 ambiguous (within 1.03 of C), 100 and none FgPace's, and no
// limiter FgPace's whatever the cap. CONTROL: a copy of the table, faithful at 1.03 (it answers as the machine does), with
// the band changed to 1.0: it answers FgPace at 60, so the band is what makes 60 ambiguous.
static int OwnerCopy(double cFps, int f, bool limiter, double band)
{
    if (limiter && f > 0 && cFps > 0.0) { if ((double)f <= cFps) return 2; if ((double)f <= band * cFps) return 3; }
    return cFps > 0.0 ? 1 : 0;
}
static void CaseOwner()
{
    CaseBegin("R3c who paces: the ownership table at 120 Hz");
    Fresh();
    FgState& s = B::S();
    B::refreshPacked = (120ull << 32) | 1ull;
    // 59 and 61 (the harness's review): 59 is under C (59.11) so FusionFix's, which pins the <= edge's side; 61 is over 1.03 C
    // (60.89) so FgPace's, which pins the band's width (a band of 1.05 would read 61 ambiguous)
    const int caps[6] = { 58, 59, 60, 61, 100, 0 }, want[6] = { 2, 2, 3, 1, 1, 1 };
    const double cFps = 1000.0 / M::CPeriodMs();
    std::string line; bool ok = true, faithful = true;
    for (int i = 0; i < 6; i++)
    {
        B::limiter = true; B::ffCap = caps[i];
        const int o = M::PaceOwner();
        ok &= o == want[i]; faithful &= OwnerCopy(cFps, caps[i], true, 1.03) == o;
        line += Fmt("%scap %d: %d", i ? ", " : "", caps[i], o);
    }
    B::limiter = false; B::ffCap = 58;
    const int noLim = M::PaceOwner(); ok &= noLim == 1;
    printf("  C %.3f fps: %s; no limiter at cap 58: %d (2 FusionFix, 3 ambiguous, 1 FgPace)\n", cFps, line.c_str(), noLim);
    if (!ok) Err("R3c: the ownership table is not 58 and 59 FusionFix, 60 ambiguous, 61, 100 and none FgPace, and FgPace with no limiter");
    const int ctl = OwnerCopy(cFps, 60, true, 1.0);
    if (!faithful || ctl != 1) Err("R3c control: the copy was not faithful at 1.03, or at 1.0 it did not answer FgPace at 60: the control shows nothing");
    Ctl("the copy, faithful at 1.03, with the band at 1.0: cap 60 answers %d (FgPace), NOT ambiguous", ctl);
    B::refreshPacked = 0; B::limiter = false; B::ffCap = 0;
    CaseEnd("R3c who paces: the ownership table at 120 Hz");
}

// H16, THE EARLY RULE'S (b) READS DT_R_BODY (s6-plan.md 3.9; DECISIONS "PHASE 1C, S6", D4). Under C1 an N fired early at the
// injection runs inside the injection's driving note; while its Present is held (the fake's SyncFrameLatency stand-in) the
// render thread asks for an entry the shadow cannot answer (a render target made unknown, as a skipped post leaves it) and
// waits DT_R_BODY for the note: the wait grows by at least the time the Present was held. (Whether it can go down, rule (b)'s
// negative deltas, is not this case's: dtWaitT has one writer, the render thread that reads it here, so the "went down" flag
// below cannot bite and is printed only; W9 counts negative deltas in the field.) CONTROL: C3 at
// the same injection: nothing fires there, the note ends at once, and the wait does not grow by the hold.
static volatile LONG gH16Hit0 = 0; static LONGLONG gH16HitT = 0, gH16RelT = 0; static bool gH16Met = false;
static DWORD WINAPI H16Helper(void*)
{
    gH16Met = WaitFor([] { return dtWaitHit[DT_R_BODY] != gH16Hit0; }, 1000);
    gH16HitT = B::Now();
    Sleep(30);
    gH16RelT = B::Now();
    if (RingGateHeld()) RingGateRelease();
    return 0;
}
static void H16Run(int early, double* waitedMs, double* heldMs, bool* held, LONG* fired, bool* down)
{
    Opt o; Fresh(o); gC1Off = early == 3 ? 1 : 0; Warm();   // (C3: the early rule switched off, the harness's own seam)
    FgState& s = B::S();
    Frame(); Drain(); AnswerAs(s.r.lastP);
    const LONG e0 = s.c.nEarly;
    dtNoteDrives[DT_NOTE_INJECT] = true;
    const LONGLONG w0 = dtWaitT[DT_R_BODY];
    gH16Hit0 = dtWaitHit[DT_R_BODY];
    RingGateHoldNext();
    const DWORD pay[4] = { 5, 6, 7, 8 };
    _InterlockedIncrement(&gInjPending);
    if (!DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) { _InterlockedDecrement(&gInjPending); Err("H16: the injection note was refused"); }
    DtPublishFenced();
    *held = WaitFor([] { return RingGateHeld(); }, early == 1 ? 3000 : 100);
    dtSh.rtOk[0] = false;                                     // an entry the shadow cannot answer beside the note
    HANDLE h = Spawn(H16Helper);
    IDirect3DSurface9* rt = nullptr; D->GetRenderTarget(0, &rt); if (rt) rt->Release();
    const LONGLONG w1 = dtWaitT[DT_R_BODY];
    Join(h, "H16's helper");
    Drain();
    _InterlockedExchange(&gRing.gateArmed, 0); ResetEvent(gRing.gateEv);   // a hold nobody took, and a release nobody waited for
    dtNoteDrives[DT_NOTE_INJECT] = false;
    *waitedMs = TickMs(w1 - w0); *heldMs = gH16Met ? TickMs(gH16RelT - gH16HitT) : 0.0; *fired = s.c.nEarly - e0; *down = w1 < w0 || dtWaitT[DT_R_BODY] < w1;
}
static void CaseH16()
{
    CaseBegin("H16 the early rule's (b): DT_R_BODY carries an early fire's hold");
    double waited, heldMs; bool held, down; LONG fired;
    H16Run(1, &waited, &heldMs, &held, &fired, &down);
    printf("  C1: N fired early %ld, its Present held %d for %.2f ms; the render thread's fetch waited DT_R_BODY %.2f ms; went down %d\n", fired, held, heldMs, waited, down);
    if (fired != 1 || !held || heldMs < 25.0 || waited + 0.2 < heldMs || down) Err("H16: DT_R_BODY did not grow by the early fire's hold, or it went down");
    double wC, hC; bool heldC, downC; LONG firedC;
    H16Run(3, &wC, &hC, &heldC, &firedC, &downC);
    printf("  control, C3: fired %ld, a Present held %d; DT_R_BODY grew %.2f ms\n", firedC, heldC, wC);
    if (firedC || heldC || wC >= heldMs / 2.0 || downC) Err("H16 control: under C3 the fetch still waited %.2f ms: the control shows nothing", wC);
    Ctl("C3 at the same injection: nothing fired, DT_R_BODY grew only %.3f ms", wC);
    CaseEnd("H16 the early rule's (b): DT_R_BODY carries an early fire's hold");
}

// THE ROUTE, MOVED INTO THE MACHINE (s6-plan.md 3.13; fg_machine.h's Route). RT1, THE REPLAY: fg_rig.h's FgRigSample routing as
// 47cd88e0 had it, copied here verbatim but for its sink (OldSample), and the machine's Route, side by side over one recorded
// sequence of 4,000 waiter iterations: random frames, owned or not, a valid begin or not, their END stamps back 0 to 7
// iterations later, never, or dated before the begin. Every sample either takes, every return and every "not back" must be
// the same, and the new route's "end before begin" must equal the sequence's own count of such ends. CONTROL: a mutant of the
// old route that ends a sample not back at tOut (its own fallback: te = b + 1 ms) instead of dropping it: the replay tells
// it apart.
struct OldPend { LONG fe; LONGLONG b; float p, h, q, l; int iters; };
struct OldRouteSt { OldPend pend[16]; int pendN; LONG notBack; };
struct Took { LONG fe; LONGLONG b, te; float p, h, q, l; bool operator==(const Took& o) const { return fe == o.fe && b == o.b && te == o.te && p == o.p && h == o.h && q == o.q && l == o.l; } };
static std::vector<Took> gOldTook, gNewTook;
static int OldSample(OldRouteSt& wt, LONG fe, bool valid, LONGLONG b, float p, float h, float q, float l, bool mutant)
{
    for (int i = 0; i < wt.pendN; )
    {
        OldPend& e = wt.pend[i];
        LONGLONG te = 0;
        const bool back = M::EndFor(e.fe, &te);
        if (back || ++e.iters > 4)
        {
            if (back && te > e.b) gOldTook.push_back({ e.fe, e.b, te, e.p, e.h, e.q, e.l });
            else if (!back) { if (mutant) gOldTook.push_back({ e.fe, e.b, e.b + MsTick(1.0), e.p, e.h, e.q, e.l }); else wt.notBack++; }
            wt.pend[i] = wt.pend[--wt.pendN];
            continue;
        }
        i++;
    }
    if (!valid || !M::OwnsEnd(fe)) return 0;
    LONGLONG te = 0;
    if (M::EndFor(fe, &te))
    {
        if (te > b) gOldTook.push_back({ fe, b, te, p, h, q, l });
        return 1;
    }
    if (wt.pendN < 16) { OldPend& e = wt.pend[wt.pendN++]; e.fe = fe; e.b = b; e.p = p; e.h = h; e.q = q; e.l = l; e.iters = 0; }
    else wt.notBack++;
    return 1;
}
// THE NEW ROUTE CARRIES fp TOO (the S row's begin frame): each take's fp is checked against the frame the sequence gave it
static LONG gNewFpBad = 0;
struct NewSink { void Take(const FgPend& e, LONGLONG te) { gNewTook.push_back({ e.fe, e.b, te, e.p, e.h, e.q, e.l }); if (e.fp != e.fe - 1 - (e.fe % 3)) gNewFpBad++; } };
struct ReplayRes { size_t oldN, newN; int firstDiff; LONG oldNotBack, newNotBack, beforeBegin, wantBefore, fpBad; int retDiff; };
// (MM and fresh, since phase 3's stage 7d: T3 in dt_fg_merged.h runs this replay over the merged machine, on a state it made)
template <class MM = M> static ReplayRes Replay(bool mutant, bool fresh = true)
{
    if (fresh) Fresh();
    gOldTook.clear(); gNewTook.clear(); gNewFpBad = 0;
    OldRouteSt ow = {}; FgRoute nw = {}; NewSink sink;
    unsigned seed = 0x5E6A11u;
    auto rnd = [&seed]() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; };
    const int N = 4000;
    std::vector<int> delay(N); std::vector<LONGLONG> te(N), b(N); std::vector<bool> owned(N), valid(N);
    const LONGLONG t0 = B::Now();
    ReplayRes r = {};
    for (int i = 0; i < N; i++)
    {
        const unsigned x = rnd();
        owned[i] = (x % 10) < 7; valid[i] = (x >> 8) % 20 != 0;
        b[i] = t0 + MsTick(16.9 * i);
        const unsigned y = rnd() % 100;
        delay[i] = y < 5 ? -1 : (int)(rnd() % 8);             // -1: never back
        te[i] = y >= 5 && y < 8 ? b[i] - MsTick(1.0) : b[i] + MsTick(5.0 + (double)(rnd() % 1000) / 100.0);
    }
    for (int i = 0; i < N; i++)
    {
        const LONG fe = 100000 + i;
        MM::OwnWrite(fe, owned[i] ? 1 : 0, i);
        for (int j = i - 7; j <= i; j++) if (j >= 0 && delay[j] >= 0 && j + delay[j] == i) StampEnd(100000 + j, te[j]);
        // h, q and l vary per sample and fp is not fe (the harness's review: constants hid a Route that swapped or dropped them)
        const float h = (float)(i % 7) + 0.25f, q = (float)(i % 11) + 0.5f, l = (float)(i % 13) + 0.75f;
        const int ro = OldSample(ow, fe, valid[i], b[i], (float)i, h, q, l, mutant);
        const int rn = MM::Route(nw, fe, fe - 1 - (fe % 3), valid[i], b[i], (float)i, h, q, l, sink);
        if (ro != rn) r.retDiff++;
        // the route sees a stamp that comes back by its fifth look after the deferral (delay 5 at most); a later one is "not back"
        if (valid[i] && owned[i] && delay[i] >= 0 && delay[i] <= 5 && te[i] <= b[i]) r.wantBefore++;
    }
    r.oldN = gOldTook.size(); r.newN = gNewTook.size(); r.firstDiff = -1;
    for (size_t k = 0; k < r.oldN || k < r.newN; k++) if (k >= r.oldN || k >= r.newN || !(gOldTook[k] == gNewTook[k])) { r.firstDiff = (int)k; break; }
    r.oldNotBack = ow.notBack; r.newNotBack = nw.notBack; r.beforeBegin = nw.beforeBegin; r.fpBad = gNewFpBad;
    return r;
}
static void CaseRouteReplay()
{
    CaseBegin("RT1 the route in the machine replays fg_rig.h's");
    const ReplayRes r = Replay(false);
    printf("  4,000 iterations: taken old %zu new %zu (first difference %d), returns differing %d, not back old %ld new %ld, end before begin %ld (the sequence's %ld)\n",
        r.oldN, r.newN, r.firstDiff, r.retDiff, r.oldNotBack, r.newNotBack, r.beforeBegin, r.wantBefore);
    if (r.firstDiff >= 0 || r.retDiff || r.oldNotBack != r.newNotBack || r.oldN < 1000 || r.oldNotBack < 20) Err("RT1: the machine's route did not replay the old routing exactly (or the sequence exercised too little)");
    if (r.fpBad) Err("RT1: %ld takes carried the wrong begin frame (fp)", r.fpBad);
    if (r.beforeBegin != r.wantBefore || r.wantBefore < 20) Err("RT1: end before begin counted %ld, the sequence has %ld", r.beforeBegin, r.wantBefore);
    const ReplayRes c = Replay(true);
    printf("  control, the old routing ending a sample not back at tOut: taken %zu against %zu, first difference %d\n", c.oldN, c.newN, c.firstDiff);
    if (c.firstDiff < 0) Err("RT1 control: a route that ends samples at tOut replayed the same: the control shows nothing");
    Ctl("a route ending not-back samples at tOut: the replay DIFFERS at take %d (%zu takes against %zu)", c.firstDiff, c.oldN, c.newN);
    CaseEnd("RT1 the route in the machine replays fg_rig.h's");
}

// RT2, NOT BACK AND BEFORE ITS BEGIN, WITH THE WORKER HELD (s6-plan.md 3.13). The worker is held by the gate with N's fire queued
// behind it, so the END value of the armed pair's frame is withheld; the route defers that frame's sample at its first look
// and drops it "not back" exactly at the fifth look after (the sixth call), not before; released, the END is signalled and
// stamped, and the dropped sample never comes back. Then an END stamp dated before its begin: dropped and counted "end before
// begin". CONTROL: the old routing's mutant (tOut) over the same looks takes the sample instead.
static int RT2Looks(bool mutant, LONG* notBackAt, LONG* tookAt)
{
    Opt o; o.fakeClock = true; Fresh(o); Warm();
    FgState& s = B::S();
    Frame(); Drain(); AnswerAs(s.r.lastP);
    const LONG p = s.r.lastP, fe = s.w.openFEnd;
    StampNow();                                               // everything signalled so far is stamped
    // THE STAMPERS RUN AS REAL THREADS (the harness's review: with none running, the END stayed unstamped whether or not the
    // gate held, so the gate proved nothing): only the held worker now keeps N's END from them
    StartThreads<M>(false, true);
    Gate();                                                   // the worker held...
    DueN(p); FireNote();                                      // ...with N's fire (its END) queued behind it
    const bool held = WaitFor([] { return gGateIn != 0; }, 3000);
    FgRoute rt = {}; NewSink sink; OldRouteSt ow = {};
    gOldTook.clear(); gNewTook.clear();
    const LONGLONG b = B::Now() - MsTick(5.0);
    *notBackAt = -1; *tookAt = -1;
    int calls = 0;
    for (int look = 1; look <= 8; look++)
    {
        Sleep(5);                                             // the stampers' time to stamp anything that came back
        const LONG cand = look == 1 ? fe : fe + 1000 + look;  // after the first, candidates the rig does not own
        if (mutant) OldSample(ow, cand, true, b, 1.0f, 1.0f, 1.0f, 1.0f, true); else M::Route(rt, cand, cand, true, b, 1.0f, 1.0f, 1.0f, 1.0f, sink);
        calls++;
        if (*notBackAt < 0 && (mutant ? ow.notBack : rt.notBack) > 0) *notBackAt = look;
        if (*tookAt < 0 && (mutant ? !gOldTook.empty() : !gNewTook.empty())) *tookAt = look;
    }
    SetEvent(gGateEv); Drain();                               // released: N fires and its END is signalled; a stamper stamps it
    LONGLONG te = 0; const bool back = held && WaitFor([fe] { LONGLONG x = 0; return M::EndFor(fe, &x); }, 2000) && M::EndFor(fe, &te);
    StopThreads();
    M::Route(rt, fe + 2000, fe + 2000, true, b, 1.0f, 1.0f, 1.0f, 1.0f, sink);   // one more look: the dropped sample stays dropped
    if (!mutant && !gNewTook.empty() && *tookAt < 0) *tookAt = 9;
    return back ? calls : -calls;
}
static void CaseRouteHeld()
{
    CaseBegin("RT2 not back at the fifth look, end before begin, the worker held");
    FgState& s = B::S();
    LONG nb, took;
    const int calls = RT2Looks(false, &nb, &took);
    printf("  the END withheld behind the gate: deferred at call 1, not back at call %ld, taken at %ld; the END signalled once released %d\n", nb, took, calls > 0);
    if (nb != 6 || took >= 0 || calls <= 0) Err("RT2: the withheld sample was not dropped not-back exactly at the fifth look after its deferral (the sixth call), or it was taken");
    // END BEFORE BEGIN: an owned frame whose END stamp is dated before its begin
    FgRoute rt = {}; NewSink sink; gNewTook.clear();
    const LONG f2 = 900000; const LONGLONG b2 = B::Now();
    M::OwnWrite(f2, 1, 1); StampEnd(f2, b2 - MsTick(1.0));
    const int ret = M::Route(rt, f2, f2, true, b2, 1.0f, 1.0f, 1.0f, 1.0f, sink);
    printf("  an END dated 1 ms before its begin: returned %d, taken %zu, end before begin %ld\n", ret, gNewTook.size(), rt.beforeBegin);
    if (ret != 1 || !gNewTook.empty() || rt.beforeBegin != 1) Err("RT2: an END before its begin was not dropped and counted");
    (void)s;
    LONG nbC, tookC;
    RT2Looks(true, &nbC, &tookC);
    printf("  control, the old routing's tOut mutant: not back at call %ld, taken at call %ld\n", nbC, tookC);
    if (tookC < 0 || nbC >= 0) Err("RT2 control: the tOut mutant did not take the withheld sample: the control shows nothing");
    Ctl("the tOut mutant TOOK the withheld sample at call %ld (never not back)", tookC);
    CaseEnd("RT2 not back at the fifth look, end before begin, the worker held");
}

// (X1, the per-present export's read, X2, its flags, J1, W12's chain and bins, MP1, the present end's map, AF1, the map to the
// aim, and H17, C1's early lead from the fire record, went with the instrument's records in round 9's S9: each read a record
// no build of the product writes.)
// F64, THE FOURTH FAULT (s6-plan.md 3.18; FG_F_SKIPSUPPRESS, bit 64, until the fault key went in round 9's S9: now the
// harness's own present observer in front of the machine's, PresentObs_SkipAt20): on pair 20 the present observer does not
// suppress the armed ordinal, so the frame end is signalled at the arm and N's own signal later is dropped by LatEndSignal's
// guard (the fake's latEndDropped): the END stamper's stamp of pair 20's frame precedes N's fire, which age-gpuend must see.
// CONTROL: the machine's own observer alone: no suppression skipped, nothing dropped, and the END stamp is not before N's fire.
static bool F64Run(bool fault, LONG* skipped, LONG* dropped, double* endMinusN)
{
    Opt o; o.fakeClock = true;
    Fresh(o);
    FgState& s = B::S();
    B::refreshPacked = (120ull << 32) | 1ull; s.r.appliedUs = 16917;
    const LONG sk0 = gSkipSuppressed, dr0 = gRingSt.latEndDropped;
    FakePair();                                               // the engage: FgSwapPresent's pointer check looks here, before the swap
    if (fault) dtPresentObs = &PresentObs_SkipAt20<M>;
    for (int i = 1; i < 22; i++) FakePair();
    if (fault) dtPresentObs = &M::PresentObs;
    // (pair 20's frame, by its publication, and N's fire start, by its resolution: the fire record that gave both went with the
    // instrument's records in round 9's S9)
    FgPub pb = {}; FgRes rs = {};
    const bool hp = M::Read(s.pub[20 & (FG_RING - 1)], 20, &pb) && M::Read(s.res[20 & (FG_RING - 1)], 20, &rs) && rs.nStart;
    LONGLONG te = 0; const bool he = hp && M::ReadStamp(s.st[FG_TL_END], pb.fEnd, &te);
    *skipped = gSkipSuppressed - sk0; *dropped = gRingSt.latEndDropped - dr0;
    *endMinusN = he ? TickMs(te - rs.nStart) : 1e9;
    B::refreshPacked = 0;
    return hp && he;
}
static void CaseFault64()
{
    CaseBegin("F64 fault 64: the frame end at the arm, N's own dropped");
    LONG sk, dr; double d;
    const bool ok = F64Run(true, &sk, &dr, &d);
    printf("  the suppression skipped at pair 20, over 22 pairs: suppression skipped %ld, ends dropped by the guard %ld; pair 20's END stamp %.3f ms from its N's fire start\n", sk, dr, d);
    if (!ok || sk != 1 || dr != 1 || d > -1.0) Err("F64: pair 20's frame end was not signalled at the arm (END before N by more than 1 ms) with N's own dropped once");
    LONG skC, drC; double dC;
    const bool okC = F64Run(false, &skC, &drC, &dC);
    printf("  control, the machine's own present observer alone: skipped %ld, dropped %ld; pair 20's END %.3f ms from N's fire start\n", skC, drC, dC);
    if (!okC || skC || drC || dC < 0.0) Err("F64 control: with the machine's own observer alone an end was still skipped, dropped or before N: the control shows nothing");
    Ctl("the machine's own observer alone: nothing skipped or dropped, pair 20's END %.3f ms AFTER N's start", dC);
    CaseEnd("F64 fault 64: the frame end at the arm, N's own dropped");
}


// PF1, THE PERIOD IN FORCE (design 4.1): PublishApplied keeps A5's max for HITCH (appliedUs) and publishes FgPace's own period as
// the period in force; the placer halves the pair's published period, not a live read. On the fake clock: a pair published
// with period 16.900 ms (after a downward move from 17.300), then the live period moved to 16.500 before the pair was timed.
// CONTROLS: A5's max would give H 8.65, and a live read 8.25; the pair's own gives 8.45.
static void CasePeriod()
{
    CaseBegin("PF1 the period in force: the pair's own, not A5's max, not a live read");
    Opt o; o.fakeClock = true; Fresh(o);
    FgState& s = B::S();
    B::refreshPacked = (120ull << 32) | 1ull;
    s.r.period = 16.9; s.r.prevPeriod = 17.3;                  // (FgPace owns: a 120 Hz panel, no limiter)
    M::PublishApplied();
    printf("  after a downward move 17.3 -> 16.9: applied (A5, for HITCH) %ld us, the period in force %ld us\n", s.r.appliedUs, s.r.periodUs);
    if (s.r.appliedUs != 17300 || s.r.periodUs != 16900) Err("PF1: PublishApplied did not keep A5's max in appliedUs and FgPace's own in periodUs");
    s.w.wIUs = 0; s.w.wNUs = 0;
    const LONG p = 1; const LONGLONG tS = B::Now();
    { FgHandPayload h = {}; h.p = p; h.gen = 1; h.k = p; h.fEnd = p; h.periodUs = s.r.periodUs; M::PubRecord(p, FG_PUB_ARMED, 0, &h, tS, 1);
      FgStamp& e = s.st[FG_TL_IN][p & (FG_STAMPS - 1)]; WriteRelease(&e.v1, -1); e.t = tS; e.late = 0; WriteRelease(&e.v2, p); WriteRelease(&e.v1, p); }
    s.r.period = 16.5; s.r.appliedUs = 16500;                               // a move after the pair's present, before its timing
    PlaceNow(); ClockAdd(1.6); PlaceNow();
    FgPlan pn = {}; const bool hp = M::Read(s.plan[p & (FG_RING - 1)], p, &pn);
    const double hMs = hp ? (double)pn.hUs / 1000.0 : -1.0;
    printf("  the pair's H %.3f ms from its period %ld us (A5's max would give 8.650, a live read 8.250)\n", hMs, hp ? pn.periodUs : -1);
    if (!hp || M::Absd(hMs - 8.45) > 0.001 || pn.periodUs != 16900) Err("PF1: the placer did not halve the pair's own period in force (8.450 ms)");
    B::refreshPacked = 0;
    Ctl("A5's max would put H at 8.650 ms and a live read at 8.250: the pair's own gives %.3f", hMs);
    CaseEnd("PF1 the period in force: the pair's own, not A5's max, not a live read");
}

// ================================================================== THE PACING DESIGN (2026-10-04)
// docs/reports/fg-1c/pacing-design-2026-10-04 (6-critic.txt R1, R3, R5, R7): the exact raise, the one-sided grid, N's shift
// and the halves that take it, the immediate present, and the records the export's B and L rows are written from.

// A pair fed to the placer as the worker and the IN stamper feed it (H12's way): the IN stamp, then the publication with
// its wake. tS is the handoff's stamp; flags the hand's.
static void PdFeed(LONG p, LONGLONG tS, DWORD flags, LONG periodUs, DWORD withhold = 0)
{
    FgState& s = B::S();
    FgStamp& e = s.st[FG_TL_IN][p & (FG_STAMPS - 1)];
    WriteRelease(&e.v1, -1); e.t = tS; e.late = 0; WriteRelease(&e.v2, p); WriteRelease(&e.v1, p);
    FgHandPayload h = {}; h.p = p; h.gen = 1; h.k = p; h.fEnd = p; h.flags = flags; h.periodUs = periodUs; h.withhold = withhold; h.tEntry = tS - 1000;
    M::PubRecord(p, FG_PUB_ARMED, 0, &h, tS, 1);
}
static void PdResolve(LONG p)
{
    FgState& s = B::S();
    FgRes x = {}; x.p = p; x.gen = 1; x.iRes = FG_RES_SHOWN; x.nRes = FG_RES_FIRED; x.iStart = x.nStart = B::Now();
    M::Publish(s.res[p & (FG_RING - 1)], p, x);
}
static double PdMedian(std::vector<double> v) { if (v.empty()) return -1.0; std::sort(v.begin(), v.end()); return v[v.size() / 2]; }

// PD1, THE EXACT RAISE (PlacerRun). The placer as a REAL THREAD on the real clock, running the machine's own pass as the
// mod's thread does: 24 pairs, each with a generation time of 3 ms, so I's raise is TIMED (3 ms after the stamp) and N's
// follows half a period later. Each raise's lateness is the placer's own clock at the raise against the instant it was
// wanted (the raise record the export's L row is written from). The timer is set 0.6 ms short and the rest is spun: the
// median lateness must be under 0.03 ms. CONTROL: THE BARE TIMER, the harness's own thread waiting the same 3 ms and 8.45 ms on
// the same high-resolution waitable timer the placer's thread sets (what a raise on the timer alone was before the spin, and
// what the development key fg_test_spin=0 ran until it went with the placer's keys in round 9's S8): N's wait (8.45 ms, the
// game's case) must be at least 0.05 ms late in its median and three times the exact raise's (the game's own runs read 0.25 to
// 0.34 ms), I's three times the exact one's. WHAT BITES: N's floor of 0.05 ms. The poll that ends a spin early (a publication or
// an IN stamp inside a spin) is not driven here.
static double gPdSpinMeanMs = 0.0;                 // the mean length of a spin in the last PdRaises run (ms)
static void PdRaises(int n, double* medI, double* medN, double* worst, LONG* spins, int* got)
{
    Opt o; Fresh(o); gStubGenUs = 3000;                       // (the stub's modelled answer at 3 ms: FG_STUB_GEN_US, the key's 3000 until round 9's S9)
    FgState& s = B::S();
    B::refreshPacked = (120ull << 32) | 1ull;
    s.w.wIUs = 0; s.w.wNUs = 0;
    StartThreads<M>(true, false);
    std::vector<double> li, ln; *worst = 0; *got = 0;
    for (LONG p = 1; p <= n; p++)
    {
        PdFeed(p, B::Now(), p == 1 ? FG_HF_FIRST : 0u, 16900);
        FgRaise rz = {};
        if (!WaitFor([&s, p, &rz] { return M::Read(s.raise[p & (FG_RING - 1)], p, &rz); }, 2000)) { Err("PD1: pair %ld's raises were not recorded within 2 s", p); break; }
        const double a = M::TicksToMs(rz.tIRaised - rz.atI), b = M::TicksToMs(rz.tNRaised - rz.atN);
        if (a < 0.0 || b < 0.0) Err("PD1: a raise was made BEFORE its instant (I %.4f ms, N %.4f ms)", a, b);
        li.push_back(a); ln.push_back(b); (*got)++;
        if (a > *worst) *worst = a; if (b > *worst) *worst = b;
        PdResolve(p); SetEvent(B::evPlacer);
        SpinMs(2.0);
    }
    StopThreads();
    *medI = PdMedian(li); *medN = PdMedian(ln); *spins = s.pl.spins;
    gPdSpinMeanMs = s.pl.spins ? M::TicksToMs(s.pl.spinTicks) / (double)s.pl.spins : 0.0;
    B::refreshPacked = 0;
}
// THE CONTROL'S BARE TIMER: n waits of waitMs each on a high-resolution waitable timer set as PlacerMain sets it, the median of
// how late each wait returned, ms
static double PdBareTimer(int n, double waitMs)
{
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!timer) timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    std::vector<double> late;
    for (int i = 0; i < n; i++)
    {
        const LONGLONG at = B::Now() + (LONGLONG)(waitMs * (double)B::Qpf() / 1000.0);
        LONGLONG left = at - B::Now(); if (left < 0) left = 0;
        LARGE_INTEGER due; due.QuadPart = -(LONGLONG)((double)left * 1e7 / (double)B::Qpf()); if (due.QuadPart == 0) due.QuadPart = -1;
        SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
        WaitForSingleObject(timer, 2000);
        late.push_back(M::TicksToMs(B::Now() - at));
    }
    CloseHandle(timer);
    return PdMedian(late);
}
static void CasePD1()
{
    CaseBegin("PD1 the exact raise: the placer's timed raises land on their instant");
    double i1, n1, w1; LONG sp1; int g1;
    PdRaises(24, &i1, &n1, &w1, &sp1, &g1);
    const double spinMean = gPdSpinMeanMs;
    const double i0 = PdBareTimer(24, 3.0), n0 = PdBareTimer(24, 8.45);
    printf("  the exact raise: %d pairs, lateness of the raise ms median I %.4f N %.4f (worst %.4f), %ld spins of %.3f ms each on average\n", g1, i1, n1, w1, sp1, spinMean);
    printf("  control, the bare timer (24 waits each): median lateness of a 3 ms wait %.4f ms, of an 8.45 ms wait %.4f ms\n", i0, n0);
    if (g1 < 20) Err("PD1: fewer than 20 pairs were raised (%d)", g1);
    if (i1 > 0.03 || n1 > 0.03) Err("PD1: with the spin the median lateness is %.4f (I) and %.4f (N) ms, over 0.03", i1, n1);
    // two timed raises a pair, each met by a spin, but for a raise whose timer woke past its 0.6 ms slack (met at once, late
    // by the excess: 1 of 48 on the case's first run, 0.027 ms): nine in ten must have spun. And a spin is the timer's slack
    // and no more (the review of 2026-10-05: a pass that spun the whole wait, 3 and 8 ms here, would have landed its raises
    // as exactly)
    if (sp1 * 10 < 2 * g1 * 9) Err("PD1: %ld spins for %d pairs: fewer than nine in ten of the timed raises were met by a spin", sp1, g1);
    if (spinMean > 0.9 || spinMean <= 0.0) Err("PD1: a spin lasted %.3f ms on average: the timer's slack is 0.6 ms, and a spin over 0.9 is the whole wait spun", spinMean);
    if (n0 < 0.05 || n0 < 3.0 * n1 || i0 < 3.0 * i1) Err("PD1 control: the bare timer's waits were not late (median 3 ms %.4f, 8.45 ms %.4f): the control shows nothing", i0, n0);
    Ctl("the bare timer's waits are LATE: median %.3f ms (3 ms) and %.3f ms (8.45 ms), against the exact raise's %.4f and %.4f", i0, n0, i1, n1);
    CaseEnd("PD1 the exact raise: the placer's timed raises land on their instant");
}


// PD5, THE RECORDS BEHIND THE EXPORT'S NEW ROWS. (a) THE PACER'S (the B row): FgPace owning pacing on the fake clock at
// 120 Hz: the first engaged present resyncs (no sleep); the next ones sleep, each given a due exactly one step after the
// one before; the record holds the present's entry, that due, the release and FG_PB_OWNER. CONTROL: with FusionFix
// owning (its limiter under C) the record carries no owner bit and no due. (b) THE PLACER'S (the L row): the raise record
// holds the instant each raise was wanted and the placer's clock when it made it: stepped exactly to those instants on the
// fake clock, the two are equal; stepped 0.2 ms after, the record says 0.2 ms late.
static void CasePD5()
{
    CaseBegin("PD5 the records behind the export's B and L rows");
    FgState& s = B::S();
    {
        Opt o; o.fakeClock = true; Fresh(o);
        B::refreshPacked = (120ull << 32) | 1ull;                // (FgPace owns: a 120 Hz panel, no limiter)
        LONGLONG due[6] = {}; LONG fl[6] = {}; int bad = 0;
        for (int i = 0; i < 6; i++)
        {
            ClockAdd(4.0); Frame(); Drain();
            const LONG f = B::frames - 1;
            const FgPresRec& e = s.pres[f & (FG_STAMPS - 1)];
            if (e.f1 != f || e.f2 != f) { Err("PD5: frame %ld has no present record", f); continue; }
            due[i] = e.due; fl[i] = e.pflags;
            if (!(e.pflags & FG_PB_OWNER) || !e.tEntry || e.tPost < e.tEntry || !e.due || e.periodUs < 16000) bad++;
            AnswerAs(s.r.lastP);
        }
        const LONGLONG step = (LONGLONG)(s.r.period * (double)B::Qpf() / 1000.0);
        int stepped = 0, slept = 0;
        for (int i = 2; i < 6; i++) { if (due[i] - due[i - 1] == step) stepped++; if (fl[i] & FG_PB_SLEPT) slept++; }
        printf("  FgPace owning: 6 presents, records off %d; the first resynced (bits 0x%lx); of the last 4, %d were given a due one step (%.4f ms) after the one before and %d slept\n",
               bad, (unsigned long)fl[0], stepped, M::TicksToMs(step), slept);
        if (bad) Err("PD5: %d present record(s) lacked the owner bit, the entry, the due, the release or the period", bad);
        if (!(fl[0] & FG_PB_RESYNC) || (fl[0] & FG_PB_SLEPT)) Err("PD5: the first engaged present did not resync without a sleep (bits 0x%lx)", (unsigned long)fl[0]);
        if (stepped != 4 || slept != 4) Err("PD5: of 4 presents %d were given a due one step after the one before and %d slept", stepped, slept);
        // CONTROL: FusionFix owns
        B::limiter = true; B::ffCap = 50;
        ClockAdd(4.0); Frame(); Drain();
        const LONG f = B::frames - 1; const FgPresRec& e = s.pres[f & (FG_STAMPS - 1)];
        printf("  control, FusionFix's limiter at 50 owning: bits 0x%lx, due %lld\n", (unsigned long)e.pflags, (long long)e.due);
        if ((e.pflags & FG_PB_OWNER) || e.due) Err("PD5 control: with FusionFix owning the record still says FgPace owned or gave a due");
        B::limiter = false; B::ffCap = 0; B::refreshPacked = 0; AnswerAs(s.r.lastP);
    }
    {
        Opt o; o.fakeClock = true; Fresh(o); gStubGenUs = 3000;
        B::refreshPacked = (120ull << 32) | 1ull; s.w.wIUs = 200; s.w.wNUs = 300;
        double lateI[2] = {}, lateN[2] = {}; bool exact[2] = {};
        for (LONG p = 1; p <= 2; p++)
        {
            ClockAdd(16.9);
            const LONGLONG tS = B::Now();
            PdFeed(p, tS, 0u, 16900); PlaceNow();
            FgPlan pn = {}; if (!M::Read(s.plan[p & (FG_RING - 1)], p, &pn)) { Err("PD5: pair %ld has no plan", p); continue; }
            const LONGLONG atI = pn.tI - M::UsToTicks(200), atN = pn.tN - M::UsToTicks(300), slip = p == 2 ? M::UsToTicks(200) : 0;
            B::clockNow = atI + slip; PlaceNow();                    // I's instant is the arrival here (tI - w_I): answered and raised in one step
            B::clockNow = atN + slip; PlaceNow();
            FgRaise rz = {}; if (!M::Read(s.raise[p & (FG_RING - 1)], p, &rz)) { Err("PD5: pair %ld has no raise record", p); continue; }
            exact[p - 1] = rz.atI == atI && rz.atN == atN;
            lateI[p - 1] = M::TicksToMs(rz.tIRaised - rz.atI); lateN[p - 1] = M::TicksToMs(rz.tNRaised - rz.atN);
            if (pn.wIUs != 200 || pn.wNUs != 300 || pn.tEntry != tS - 1000 || pn.tReady != pn.tI) Err("PD5: pair %ld's plan does not carry the service times, the hand's entry or the readiness", p);
            PdResolve(p); PlaceNow();
        }
        printf("  the raise record: stepped to the instants, I %.4f and N %.4f ms late; control, stepped 0.2 ms after them: %.4f and %.4f\n", lateI[0], lateN[0], lateI[1], lateN[1]);
        if (!exact[0] || !exact[1]) Err("PD5: the raise record's wanted instants are not tI - w_I and tN - w_N");
        if (lateI[0] != 0.0 || lateN[0] != 0.0) Err("PD5: a raise made at its instant is recorded late");
        if (M::Absd(lateI[1] - 0.2) > 0.001 || M::Absd(lateN[1] - 0.2) > 0.001) Err("PD5 control: a raise made 0.2 ms late is not recorded 0.2 ms late: the record shows nothing");
        B::refreshPacked = 0;
    }
    Ctl("FusionFix owning leaves no owner bit and no due; a raise 0.2 ms late is recorded 0.200 ms late");
    CaseEnd("PD5 the records behind the export's B and L rows");
}

// PD6, THE HAND'S PERIOD AND ENTRY REACH THE PLAN THROUGH THE WORKER (the review of the pacing batch, 2026-10-05). PF1 proves
// the placer halves the publication's own period, and PD2 that the grid chains on it; both hand a payload straight to
// PubRecord. A pair of the game's is published by FgWorkerPresent, which built its payload afresh from the worker's open
// fields and left the period and the entry out: every armed pair's publication said period 0, entry 0, and the placer fell
// back on the live applied period. HERE A REAL FRAME goes the whole way (FgSwapPresent's hand, the note, the worker, the armed
// publication, the placer) on the fake clock with FgPace owning, just after a downward move (period 16.9 ms, the one before
// 17.3): the plan must carry 16,900 us, H 8.450 ms and the QPC of the frame's entry. CONTROL: the applied period at that
// present is A5's max, 17,300 us, which is what the plan carried until this day (H 8.650).
static void CasePD6()
{
    CaseBegin("PD6 the hand's period in force and its entry reach the plan through the worker");
    Opt o; o.fakeClock = true; Fresh(o);
    FgState& s = B::S();
    B::refreshPacked = (120ull << 32) | 1ull;                    // (FgPace owns: a 120 Hz panel, no limiter)
    FakeFrames(6);
    s.r.period = 16.9; s.r.prevPeriod = 17.3; s.r.paceRunning = true;
    ClockAdd(2.0);
    const LONGLONG t0 = B::Now();
    Frame(); Drain(); StampNow(); PlaceNow();
    const LONG p = s.r.lastP;
    FgPub pv = {}; const bool hv = M::Read(s.pub[p & (FG_RING - 1)], p, &pv);
    FgPlan pn = {}; const bool hp = M::Read(s.plan[p & (FG_RING - 1)], p, &pn);
    printf("  pair %ld: published %s, period %ld us, entry %s; the plan: period %ld us, H %.3f ms, entry %s; the applied period then %ld us (A5's max)\n", p,
           hv ? (pv.kind == FG_PUB_ARMED ? "ARMED" : "not armed") : "nothing", hv ? pv.periodUs : -1, hv && pv.tEntry == t0 ? "the frame's" : "NOT the frame's",
           hp ? pn.periodUs : -1, hp ? (double)pn.hUs / 1000.0 : -1.0, hp && pn.tEntry == t0 ? "the frame's" : "NOT the frame's", s.r.appliedUs);
    if (!hv || pv.kind != FG_PUB_ARMED) Err("PD6: the frame's pair was not published ARMED: the case shows nothing");
    else
    {
        if (pv.periodUs != 16900 || pv.tEntry != t0) Err("PD6: the armed publication carries period %ld us and entry %lld, not the hand's 16900 and %lld", pv.periodUs, (long long)pv.tEntry, (long long)t0);
        if (!hp || pn.periodUs != 16900 || M::Absd((double)pn.hUs / 1000.0 - 8.45) > 0.001) Err("PD6: the plan does not halve the hand's own period in force (16900 us, H 8.450 ms)");
        if (!hp || pn.tEntry != t0) Err("PD6: the plan's entry is not the frame's arrival at its Present");
    }
    if (s.r.appliedUs != 17300) Err("PD6 control: the applied period at that present is %ld us, not A5's max 17300: the control shows nothing", s.r.appliedUs);
    B::refreshPacked = 0; AnswerAs(p);
    Ctl("the applied period at that present is %ld us (A5's max): the plan carried THAT until 2026-10-05, H 8.650", s.r.appliedUs);
    CaseEnd("PD6 the hand's period in force and its entry reach the plan through the worker");
}

// PD8, THE PACING LAW IN THE PACER (2026-10-05; source/fg/fg_law.h, whose arithmetic equiv_fglaw proves against its oracle:
// THIS case proves the machine's joins). Real frames through FgSwapPresent on the fake clock, FgPace owning, 120 Hz, the
// keys as every build has them since round 9's LW (the law in every build, no hold band, the free range from the floor).
// (a) 160 frames of 3 ms, ONE of 31 ms, 60 of 3 ms: the period is the floor after every one, no move is counted, and exactly
// that one present's record says HITCH. (b) Then 25 frames of 30 ms, a level that moved: ONE move up, to the level (20 ms or
// over; the present records and the hands say so with their regime bits), and after the frames are 3 ms again the period
// comes back ONTO the floor within 15 s, with no regime bit left. (c) The applied period (the hitch test's) is the old period
// at the present after the last move down and the floor at the one after, where A5's rule kept the old one until the next
// move. NO CONTROL OF ITS OWN since round 9's LW: it was A5's law on the same frames (moving at the single long frame and
// ending above its floor), which went from every build; (b) is the reading's positive (the same reading sees the law's one
// rise), and dt_fg_rates.h's PL1 holds the single long frame on the product's binding, failed first on A5's base.
// THE ROWS FOR THE REPLAY (replay_pd8.py): with DLSSIV_PD8_ROWS naming a file, the case writes every frame's present record
// there as the mod's export wrote its B row (the rig's, until round 9), so the reader can step them through the law's reference.
static void CasePD8()
{
    FILE* rowsF = nullptr;
    { char path[MAX_PATH] = {}; if (GetEnvironmentVariableA("DLSSIV_PD8_ROWS", path, sizeof(path)) && path[0]) fopen_s(&rowsF, path, "wb"); }
    if (rowsF) fprintf(rowsF, "# DLSS-IV fg presents, format 2: build harness, pid %lu, qpf %lld, stage S6\n", GetCurrentProcessId(), (long long)B::Qpf());
    CaseBegin("PD8 the pacing law: one long frame moves nothing, a level that moved gives one rise and a return to the floor");
    FgState& s = B::S();
    const double floorMs = 2.0 * 1.015 * 1000.0 / 120.0;
    struct Arm { LONG movesA, hitchA, ups, downs, regRec, regHand, regLate, figs; int offA; double top, end, backMs; LONG appliedAfter1, appliedAfter2; bool back; } arm[1] = {};
    {
        Arm& a = arm[0];
        Opt o; o.fakeClock = true; Fresh(o);
        B::refreshPacked = (120ull << 32) | 1ull;
        const LONG mv0 = s.r.moves; double prev = 0.0; LONGLONG tSlowEnd = 0, tBack = 0; int sinceDown = -1;
        for (int i = 0; i < 1300; i++)
        {
            const bool slowRun = i >= 221 && i < 246;
            // (NOT WHOLE MICROSECONDS, since the batch's review: i mod 10 ticks more, under a microsecond. With whole ms the
            // law's sample and its cut to whole microseconds were one number, and the replay of these rows, replay_pd8.py,
            // could not have told a machine that handed its law the uncut sample. Every other figure of the case stands.)
            ClockAdd(i == 160 ? 31.0 : slowRun ? 30.0 : 3.0); B::clockNow += i % 10; Frame(); Drain();
            const LONG f = B::frames - 1; const FgPresRec& e = s.pres[f & (FG_STAMPS - 1)];
            const bool rec = e.f1 == f && e.f2 == f;
            if (!rec) { Err("PD8: frame %ld has no present record", f); break; }
            if (rowsF)
                fprintf(rowsF, "B,%ld,%lld,%lld,%lld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", f, (long long)e.tEntry, (long long)e.due, (long long)e.tPost, e.pflags, e.freeUs, e.inOrigUs, e.sleepUs, e.lateUs,
                        e.periodUs, e.lawEUs, e.lawLUs);
            const double per = s.r.period;
            if (i < 221)
            {
                if (M::Absd(per - floorMs) > 0.001) a.offA++;
                if (e.pflags & FG_PB_HITCH) a.hitchA += (i == 160) ? 1 : 100;       // any other frame's is wrong
                if (i >= 200 && e.lawEUs >= 2990 && e.lawEUs <= 3010 && e.lawLUs >= e.lawEUs && e.lawLUs <= 3010) a.figs++;   // E and L of 3 ms frames
                if (i == 220) a.movesA = s.r.moves - mv0;
            }
            else
            {
                if (prev > 0.0 && per > prev + 1e-9) a.ups++;
                if (prev > 0.0 && per < prev - 1e-9) { a.downs++; sinceDown = 0; }
                else if (sinceDown >= 0) sinceDown++;
                if (per > a.top) a.top = per;
                const bool off = per > floorMs + 0.001;
                if (off && (e.pflags & FG_PB_REG2)) a.regRec++;
                if (off != ((e.pflags & FG_PB_REG2) != 0) || (e.pflags & FG_PB_REG1)) a.regLate++;
                FgPub pv = {};
                // the hand of THIS present carries the period the step BEFORE it set: read against that one (armed pairs: an
                // inline publication carries no hand)
                if (M::Read(s.pub[s.r.lastP & (FG_RING - 1)], s.r.lastP, &pv) && pv.kind == FG_PUB_ARMED)
                {
                    const bool offH = pv.periodUs > (LONG)(floorMs * 1000.0) + 1;
                    if (offH && (pv.flags & FG_HF_REG2)) a.regHand++;
                    if (offH != ((pv.flags & FG_HF_REG2) != 0) || (pv.flags & FG_HF_REG1)) a.regLate++;
                }
                if (i == 245) tSlowEnd = B::Now();
                if (i > 245 && !off && !tBack && a.ups) { tBack = B::Now(); a.back = true; a.backMs = M::TicksToMs(tBack - tSlowEnd); }
                // the applied period one and two presents after the LAST move down (the one that reached the floor)
                if (!off && sinceDown == 1) a.appliedAfter1 = s.r.appliedUs;
                if (!off && sinceDown == 2) a.appliedAfter2 = s.r.appliedUs;
            }
            prev = per;
            AnswerAs(s.r.lastP);
        }
        a.end = s.r.period;
        B::refreshPacked = 0;
    }
    if (rowsF) fclose(rowsF);
    const Arm& n = arm[0];
    printf("  the law: through 221 frames (the 161st of 31 ms) %ld moves, off the floor after %d frames, hitch said on %ld record(s); then 25 frames of 30 ms: %ld move(s) up, to %.3f ms at most,"
           " %ld down, back on the floor %s (%.1f s after the last slow frame), %.3f ms at the end\n", n.movesA, n.offA, n.hitchA, n.ups, n.top, n.downs, n.back ? "yes" : "NO", n.backMs / 1000.0, n.end);
    printf("  its regime: said free on %ld present records and %ld hands while off the floor, said wrongly on %ld; the applied period after the last move down: %ld us one present on, %ld us two on (the floor %.0f)\n",
           n.regRec, n.regHand, n.regLate, n.appliedAfter1, n.appliedAfter2, floorMs * 1000.0);
    if (n.movesA != 0 || n.offA != 0) Err("PD8: one 31 ms frame moved the law %ld time(s), off its floor after %d frame(s)", n.movesA, n.offA);
    if (n.hitchA != 1) Err("PD8: the 31 ms frame's record, and no other, must say HITCH (%ld: 100 a wrong frame)", n.hitchA);
    if (n.figs != 21) Err("PD8: the law's E and L are on %ld of 21 records of 3 ms frames (3000 us each wanted)", n.figs);
    if (n.ups != 1) Err("PD8: a level that moved to 30 ms gave %ld moves up, not one", n.ups);
    if (n.top < 20.0 - 0.001 || n.top > 40.0) Err("PD8: the rise went to %.3f ms, not to the level (20 ms or over)", n.top);
    if (!n.back || n.backMs > 15000.0 || M::Absd(n.end - floorMs) > 0.001) Err("PD8: the period did not come back onto the floor within 15 s (back %d, %.1f s, %.3f ms at the end)", n.back ? 1 : 0, n.backMs / 1000.0, n.end);
    if (n.regRec < 100 || n.regHand < 100 || n.regLate != 0) Err("PD8: the regime did not travel: free on %ld records and %ld hands while off the floor, said wrongly on %ld (a record or a hand whose bit and period disagree)", n.regRec, n.regHand, n.regLate);
    if (n.appliedAfter1 <= (LONG)(floorMs * 1000.0) + 100 || labs(n.appliedAfter2 - (LONG)(floorMs * 1000.0)) > 1)
        Err("PD8: the applied period after the last move down is %ld us one present on and %ld two on: the old period, then the floor (%.0f) wanted", n.appliedAfter1, n.appliedAfter2, floorMs * 1000.0);
    CaseEnd("PD8 the pacing law: one long frame moves nothing, a level that moved gives one rise and a return to the floor");
}

// PD12, THE PAD UNDER THE LAW, AS IT IS (the review of the law's batch, 2026-10-05). The pad is the placer's: a late call
// raises it a quarter ms a step, sixty clean ones lower it. PaceLaw adds it to the law's answer and the period moves AT ONCE;
// A5 added it to its target, under its band and its dwell. No leg of the sitting moves the pad (it is called only with a
// fixed K, and the legs run K by readiness): this case says what the join does, so that the day a leg does move it nothing
// is a surprise. Real frames on the fake clock, past the warm-up, at the floor: the pad raised to 2 by hand puts the period
// at the floor plus 0.5 ms on the next step, MOVED on that record and NO regime bit (the regime is the law's own, and the
// law is at its floor); the step after moves nothing; the pad at 0 again, the period is the floor on the next step. (The pad
// joins the law on every binding but the merged path, whose room floor stands in its place: the product never reads it.)
// NO CONTROL OF ITS OWN since round 9's LW: it was A5's law on the same frames (a pad of 2 never moved it), which went from every
// build; the case's own three steps are a positive (the pad moves the period at once) and two negatives (the step after, the floor).
static void CasePD12()
{
    CaseBegin("PD12 the pad under the law: added to the law's answer, at once, the regime still the law's");
    FgState& s = B::S();
    const double floorMs = 2.0 * 1.015 * 1000.0 / 120.0;
    double up[2] = {}, hold[2] = {}, down[2] = {}; LONG flUp[2] = {}, flHold[2] = {}, flDown[2] = {}; LONG mvs[2] = {};
    {
        const int law = 1;                                       // (the arrays' second place: A5's, the first, went with its law in round 9's LW)
        Opt o; o.fakeClock = true; Fresh(o);
        B::refreshPacked = (120ull << 32) | 1ull;
        auto frame = [&s](LONG* fl) { ClockAdd(3.0); Frame(); Drain(); const LONG f = B::frames - 1; const FgPresRec& e = s.pres[f & (FG_STAMPS - 1)]; *fl = (e.f1 == f && e.f2 == f) ? e.pflags : -1; AnswerAs(s.r.lastP); return s.r.period; };
        LONG fl = 0;
        for (int i = 0; i < 200; i++) frame(&fl);
        const LONG mv0 = s.r.moves;
        if (M::Absd(s.r.period - floorMs) > 0.001) Err("PD12: the pacer is not at its floor before the pad is raised (law %d, %.3f ms)", law, s.r.period);
        _InterlockedExchange(&s.sh.padQms, 2);
        up[law] = frame(&flUp[law]); hold[law] = frame(&flHold[law]);
        for (int i = 0; i < 18; i++) hold[law] = frame(&fl);
        _InterlockedExchange(&s.sh.padQms, 0);
        down[law] = frame(&flDown[law]);
        mvs[law] = s.r.moves - mv0;
        B::refreshPacked = 0;
    }
    printf("  the law: the pad at 2: %.3f ms on the next step (bits 0x%lx), %.3f after twenty more; at 0 again: %.3f ms (bits 0x%lx); %ld moves. The floor %.3f\n",
           up[1], (unsigned long)flUp[1], hold[1], down[1], (unsigned long)flDown[1], mvs[1], floorMs);
    if (M::Absd(up[1] - (floorMs + 0.5)) > 1e-6 || !(flUp[1] & FG_PB_MOVED) || (flUp[1] & (FG_PB_REG1 | FG_PB_REG2)) || (flHold[1] & FG_PB_MOVED) || M::Absd(hold[1] - (floorMs + 0.5)) > 1e-6)
        Err("PD12: the pad at 2 under the new law: the period %.3f ms on the next step (the floor plus 0.5 wanted), bits 0x%lx (MOVED and no regime wanted), the step after 0x%lx (no move wanted), %.3f twenty steps on", up[1], (unsigned long)flUp[1], (unsigned long)flHold[1], hold[1]);
    if (M::Absd(down[1] - floorMs) > 1e-6 || !(flDown[1] & FG_PB_MOVED) || mvs[1] != 2) Err("PD12: the pad at 0 again: the period %.3f ms (the floor wanted), bits 0x%lx, %ld moves in all (2 wanted)", down[1], (unsigned long)flDown[1], mvs[1]);
    CaseEnd("PD12 the pad under the law: added to the law's answer, at once, the regime still the law's");
}

// THE ORDER THE CASES RUN IN, and their tags: `dt_fg_test.exe 1 H11` runs one case alone (a rerun after a fix), and with no
// tag every case runs, which is what prove-all does.
static const char* gOnly = nullptr;
// F1, THE COPY FLOOR (stage S5; map S5's proof, C-F16, G-S4; re-derived after F1C-1a, 2026-09-27). fg_machine.h's
// FgCopyClassify, hand-fed: a 10 us reading of a 4K FP16 copy is below the floor (VOID), a 50 us one (1a's fast real copies)
// is OK, a 1 us 1080p 8-bit reading is below and a 40 us one OK, the slow line sits where S5 had it (329 us at 4K FP16). The
// floor's value is checked at both sizes. CONTROLS: a fixed 4K FP16 floor (13.2 us whatever the size) would VOID an honest
// L2-resident 5 us 1080p copy, so the per-size floor is what keeps it OK; and S5's floor (half the DRAM-bound time) would VOID
// 1a's 50 us copies, which is why it was re-derived.
static void CaseF1()
{
    CaseBegin("F1 the copy floor, per size and format (map S5)");
    const double f4k = FgCopyFloorUs(3840, 2160, 8), f1080 = FgCopyFloorUs(1920, 1080, 4);
    if (f4k < 13.1 || f4k > 13.25) Err("F1: the 4K FP16 floor is %.2f us, not 13.17 (0.1 x 2 x 3840 x 2160 x 8 / 1008 GB/s)", f4k);
    if (f1080 < 1.6 || f1080 > 1.7) Err("F1: the 1080p 8-bit floor is %.2f us, not 1.65", f1080);
    if (FgCopyClassify(10.0, 3840, 2160, 8) != FG_COPY_BELOW) Err("F1: a 10 us 4K FP16 reading (an empty bracket's size) is not below the floor");
    if (FgCopyClassify(50.0, 3840, 2160, 8) != FG_COPY_OK) Err("F1: a 50 us 4K FP16 reading (F1C-1a's fast real copies) is not OK");
    if (FgCopyClassify(1.0, 1920, 1080, 4) != FG_COPY_BELOW) Err("F1: a 1 us 1080p 8-bit reading is not below the floor");
    if (FgCopyClassify(40.0, 1920, 1080, 4) != FG_COPY_OK) Err("F1: a 40 us 1080p 8-bit reading is not OK");
    if (FgCopyClassify(700.0, 3840, 2160, 8) != FG_COPY_SLOW) Err("F1: a 700 us 4K FP16 reading is not slow");
    if (FgCopyClassify(320.0, 3840, 2160, 8) != FG_COPY_OK || FgCopyClassify(330.0, 3840, 2160, 8) != FG_COPY_SLOW)
        Err("F1: the slow line at 4K FP16 is not S5's (five of S5's 65.8 us floors, 329 us): 320 us should be OK and 330 us slow");
    if (FgCopyClassify(150.0, 3840, 2160, 8) != FG_COPY_OK) Err("F1: a 150 us 4K FP16 reading (0S's measured copy size) is not OK");
    const bool fixedVoids = 5.0 < f4k;                        // the fixed floor's verdict on an honest L2-resident 1080p copy
    const bool s5Voids = 50.0 < 0.5 * FgCopyDramUs(3840, 2160, 8);   // S5's floor on 1a's fast real copies
    if (!fixedVoids) Err("F1 control: a fixed 4K FP16 floor would NOT void a 5 us 1080p reading; the control proves nothing");
    if (!s5Voids) Err("F1 control: S5's floor (half the DRAM-bound time) would NOT void a 50 us 4K reading; the re-derivation's reason is gone");
    Ctl("floors %.2f us (4K FP16) and %.2f us (1080p 8-bit); controls: a fixed 4K floor would VOID an honest 5 us 1080p copy, and S5's floor would VOID 1a's 50 us 4K copies", f4k, f1080);
    CaseEnd("F1 the copy floor, per size and format (map S5)");
}

// THE PRODUCT'S SHAPE AND ITS CASES (phase 2, stage 9): G4, G8 and L1 to L8
// ------------------------------------------------------------------ THE MOD'S GATE BEFORE DXVK'S PRESENT, WHOLE (the fold of 2026-10-10)
// source/transport/present_gate.h AS THE MOD COMPILES IT: the state, the term and the core, AND the release's wrapper
// (IvtPresentGate), its thunk (IvtGatedPresent) and its wrap (IvtGateWrap), with the one object ivtGate. Nothing of it is
// copied. What the header reads of the transport is given here: a Vulkan table with two fences this harness moves, the
// frame's out-fence, the vsync flag, the stop flag, the last frame sent and how each frame ran. Case L20
// (dt_fg_shadow.h) is its one user: every other case leaves the out-fence null, where neither gate looks.
#pragma warning(push)
#pragma warning(disable: 4505)
#include "../../source/transport/feed_vk.h"
#pragma warning(pop)
static FeedVk ivtVk;
static VkSemaphore ivtSemOut = VK_NULL_HANDLE;
static volatile bool latVsync = false, ivtFailed = false;
static volatile LONG ivtWatchAwait = 0;
static volatile LONG latOrdAsync[LAT_RING];
#include "../../source/transport/present_gate.h"
struct FoldFence { volatile LONGLONG v; volatile LONG reads, waits; };
static FoldFence gFoldOut, gFoldPrep;
static const VkSemaphore kFoldSemOut = (VkSemaphore)0x1001, kFoldSemPrep = (VkSemaphore)0x1002;
static FoldFence* FoldOf(VkSemaphore s) { return s == kFoldSemOut ? &gFoldOut : s == kFoldSemPrep ? &gFoldPrep : nullptr; }
static VkResult VKAPI_CALL FoldCounter(VkDevice, VkSemaphore s, uint64_t* v)
{
    FoldFence* f = FoldOf(s); if (!f) return VK_ERROR_UNKNOWN;
    _InterlockedIncrement(&f->reads);
    *v = (uint64_t)InterlockedCompareExchange64(&f->v, 0, 0);
    return VK_SUCCESS;
}
// (a real wait's shape: it returns when the value is reached or the time is up, whichever is first)
static VkResult VKAPI_CALL FoldWait(VkDevice, const VkSemaphoreWaitInfo* wi, uint64_t timeoutNs)
{
    FoldFence* f = FoldOf(wi->pSemaphores[0]); if (!f) return VK_ERROR_UNKNOWN;
    _InterlockedIncrement(&f->waits);
    const ULONGLONG end = GetTickCount64() + (ULONGLONG)(timeoutNs / 1000000ull);
    for (;;)
    {
        if ((uint64_t)InterlockedCompareExchange64(&f->v, 0, 0) >= wi->pValues[0]) return VK_SUCCESS;
        if (GetTickCount64() >= end) return VK_TIMEOUT;
        Sleep(1);
    }
}

#include "dt_fg_shadow.h"
// THE MERGED PATH AND ITS CASES (phase 3, stage 7): a binding that arms on the product's wire
#include "dt_fg_merged.h"
#include "dt_fg_threads.h"   // (phase 3, stage 8) the product's thread loops, source/fg/fg_threads.h, run over the fakes: case T1
#include "dt_fg_firstlight.h"   // (phase 3, stage 10) first light's copy mode, FgArmedKeys::slotCopy: case FL1
#include "dt_fg_hud.h"          // (phase 3, stage 15) the HUD-less picture: cases HU1 to HU5
#include "dt_fg_saves.h"        // (2026-10-12) the save's question, answered by the device thread's tracker: cases SV1 to SV5
#include "dt_fg_rates.h"        // (2026-10-12, phase 4) a pair held at other real rates than the panel's half, the free band: cases FB1 to FB4
#include "dt_fg_late.h"         // (2026-10-14) the helper's answer on a card that is the limit: cases LA1 to LA8
#include "dt_fg_rideframes.h"   // (2026-10-14, afternoon) every generated frame shown on a busy card: cases RF1 to RF5
#include "dt_fg_replay.h"       // (2026-10-14, evening) the seven legs at the two locks replayed: cases RL1 to RL7
#include "dt_fg_place.h"        // (2026-10-17) stage 3, the generated frame placed by the card and its first mend: cases PW1
#include "dt_fg_law.h"          // (2026-10-17, phase 5, P1) the law, the armed product's own driver: cases LW1 to LW16
#include "dt_fg_fixed.h"        // (2026-10-17, night, phase 5, F1) the fixed-refresh mode on the panel model: cases FX0 to FX6
#include "dt_fg_ramp.h"         // (2026-10-18, phase 5 round 2, S1) the switch-on's ramp and the stand-down: cases RP1 to RP5
#include "dt_fg_ship.h"         // (2026-10-24, round 6, W) the shipped build's wiring (source/fg/fg_ship.h): cases SH1 to SH4
#include "dt_fg_linux.h"        // (2026-10-24, round 7, L) under Wine, the pure rules with Wine's answers faked (source/fg/fg_linux_rule.h): cases WN1 to WN5
#include "dt_fg_corefix.h"      // (2026-10-28, fg19-corefix) two findings of 1.1's core audit: cases HU14 and LW54

#define RUN(tag, fn) do { if (!gOnly || !strcmp(gOnly, tag)) fn(); } while (0)
static void RunCases()
{
    RUN("G1", CaseG1); RUN("G2", CaseG2); RUN("G3", CaseG3); RUN("G5", CaseG5); RUN("G6", CaseG6); RUN("G7", CaseG7);   // (G4 and G8, the shadow shape's, went in round 9's strip)
    RUN("H2", CaseH2); RUN("H3", CaseH3); RUN("H4", CaseH4); RUN("H6", CaseH6); RUN("H7", CaseH7); RUN("H8", CaseH8);   // (H1, H5 and H12, the probe's, X0's and a fixed K's, went in round 9's S9)
    RUN("H9", CaseH9); RUN("H10", CaseH10); RUN("H11", CaseH11); RUN("H13", CaseH13); RUN("H14", CaseH14); RUN("H15", CaseH15);
    RUN("R1", CaseForeignReset); RUN("R2", CaseFireAcrossDisarm); RUN("R3", CaseSleepers); RUN("R4", CaseStray); RUN("R5", CaseFailedPresent);
    RUN("R6", CaseRects); RUN("R7", CaseInjectOnce); RUN("R8", CaseCensus);
    RUN("V1", CaseV1);
    RUN("F1", CaseF1);
    // STAGE S6 (s6-plan.md section 5)
    // (S6f, the key fg_test_early = 2, went in round 9's S8; S6a to S6e, S6g and S6h, the modes OFFSET and SLOPE and the ABBA
    // windows, R3b, the CAP of the pass-through half, and X2, the export's flags of SLOPE and the probe, in round 9's S9)
    RUN("R3c", CaseOwner); RUN("H16", CaseH16);   // (H17, X1, J1, MP1 and AF1, the instrument's records', went in round 9's S9)
    RUN("RT1", CaseRouteReplay); RUN("RT2", CaseRouteHeld); RUN("F64", CaseFault64);
    // S6, Q1 (b) and Q2 (b) (s6-q1q2-design.md section 7)
    RUN("PF1", CasePeriod);
    // (AF2, AL1 and CS1 ran the aim's screen signal, the aim's loop and the call side, development keys no shipped build sets,
    // and went with them in round 9's S8: docs\INSTRUMENTS-FG.md, wave 8)
    // THE PACING DESIGN (2026-10-04; docs/reports/fg-1c/pacing-design-2026-10-04)
    RUN("PD1", CasePD1); RUN("PD5", CasePD5); RUN("PD6", CasePD6); RUN("PD8", CasePD8); RUN("PD12", CasePD12);
    // (PD2, PD3 and PD4 ran the one-sided grid, N's shift with the A/B halves and FORCEIMMEDIATE, development keys no shipped
    // build sets, and went with them in round 9's S8: docs\INSTRUMENTS-FG.md, wave 8)
    // (PD7, PD9, PD10 and PD11 ran the development keys of the floor's hold, the grid and the shift with the law, the planted long
    // frames and the forced period, which no shipped build sets, and went with them in round 9's S7: docs\INSTRUMENTS-FG.md, wave 7)
    // (PHASE 2, STAGE 9 TO PHASE 3, STAGE 6: the product's shadow shape's cases, L1 to L19 with the gate's L9, and the fakes'
    // FK1 to FK7, ran over the binding that could not arm and went with it in round 9's strip, 2026-10-26: docs\INSTRUMENTS-FG.md,
    // wave 5. M9 holds the gate before every present over the merged machine. FK8, a refused submit inside a fire, runs the rig.)
    RUN("FK8", CaseFK8);
    // THE FOLD OF THE RELEASE LINE'S GATE (2026-10-10; dt_fg_shadow.h): ONE GATE ON ONE STATE ACROSS THE MACHINE'S INSTALL. Once:
    // nothing in it writes a 'Q', so the two orders of 'Q' are one case (over the merged machine since round 9's strip)
    RUN("L20", CaseL20);
    // PHASE 3, STAGE 7 (2026-10-10; dt_fg_merged.h): THE MERGED PATH, over the one binding that arms on the product's wire.
    // 7a, the worker: a pair; an unsent frame; the hand that finds a pair armed; fed, never shown; the pending real frame
    // ahead of the guard's wait; what has no hand; a build; a write that was not submitted; the gate before every
    // present; the answer by equality; the back buffer's save; one image. M1r: the first of them over the merged binding
    // that is an instrument's.
    RUN("M1", CaseM1); RUN("M1r", CaseM1r); RUN("M2", CaseM2); RUN("M3", CaseM3); RUN("M4", CaseM4); RUN("M5", CaseM5); RUN("M6", CaseM6);
    RUN("M7", CaseM7); RUN("M8", CaseM8); RUN("M9", CaseM9); RUN("M10", CaseM10); RUN("M11", CaseM11); RUN("M12", CaseM12);
    // 7b, the placer on real answers: one planned slot a pair and the fence read by equality; the real frame's due does
    // not move with the answer (the rig's placer its oracle and its control); no pair judged by which stamper ran first;
    // the machine under its own threads. And THE CENSUS: 10,000 random frames, the screen read as a whole (MCr: 2,000
    // over the merged binding that is an instrument's).
    RUN("P1", CaseP1); RUN("P2", CaseP2); RUN("P3", CaseP3); RUN("P4", CaseP4); RUN("MC", CaseMC); RUN("MCr", CaseMCr);
    // 7c, THE LIFE (2026-10-10): a stop with a pair armed; a disarm and a Reset from another thread, and the first pair
    // behind them; a machine that fires nothing with a pair armed; the helper dead at five more points; a build with
    // the worker held, and the retire wait; the set that ends itself; the mirror's own control; a lost restore.
    RUN("M13", CaseM13); RUN("M14", CaseM14); RUN("M15", CaseM15); RUN("M16", CaseM16); RUN("M17", CaseM17); RUN("M18", CaseM18); RUN("M19", CaseM19); RUN("M20", CaseM20);
    RUN("FK9", CaseFK9); RUN("FK10", CaseFK10);   // (2026-10-26, round 9, the reviewer's R2: FK5's and FK7's plants over the armed binding)
    // 7d, THE FRAME END, THE TAIL, THE WANTS (2026-10-10): a standby present costs what the shadow shape's costs (once: it
    // writes no 'Q'); CAP as a want (SHADOW went in round 9's S8); the frame end at the real frame's fire, with the route and the
    // instrument's fault; the pair's room and the pacer's clamp; lever 4's key refused; the least lead of a generated frame.
    RUN("T1", CaseT1); RUN("T2", CaseT2); RUN("T3", CaseT3); RUN("T4", CaseT4); RUN("T6", CaseT6); RUN("T7", CaseT7);   // (T5, lever 4's key, went in round 9's strip)
    RUN("TL1", CaseTL1);   // (stage 8, dt_fg_threads.h) the product's thread loops over the fakes (TL: 7d took the tags T1 to T6)
    RUN("FL1", CaseFL1);   // (stage 10, dt_fg_firstlight.h) first light's copy mode: the slot shows the pair's real frame
    // STAGE 15 (2026-10-10; dt_fg_hud.h): THE HUD-LESS PICTURE. The plan's order on the hold; an inline pair never shows it;
    // a picture that cannot be taken back; the note refused behind it; a frame with none is fed and never shown.
    RUN("HU1", CaseHU1); RUN("HU2", CaseHU2); RUN("HU3", CaseHU3); RUN("HU4", CaseHU4); RUN("HU5", CaseHU5); RUN("HU6", CaseHU6);
    // THE SECOND-TARGET FORM (2026-10-11; dt_fg_hud.h): the picture from an image of the binding's own in the hand's one span;
    // that image not nameable at the hand; every way such a hand can end; the census in that form.
    RUN("HU7", CaseHU7); RUN("HU8", CaseHU8); RUN("HU9", CaseHU9); RUN("HU10", CaseHU10);
    // THE PHONE (2026-10-12; dt_fg_hud.h): a 3D overlay ahead of the interface, by the mod's own rules (fg_machine.h,
    // FgHudOverlay3dRule; fg_hud_rule.h): TWO COPY POINTS: the encode's copy and its count are a release's, and frame
    // generation's own copy is taken ahead of the overlay, at two shapes and three handsets, the worker held; its
    // controls are the first form's else-if (the encode's copy ahead of the phone) and the question not asked.
    RUN("HU11", CaseHU11);
    // SDR (2026-10-20; dt_fg_hud.h): the HUD-less picture of an 8-bit back buffer by the mod's SDR rules (fg_hud_rule.h): the copy
    // at the interface's first draw or ahead of the phone, FusionFix's curve drawn a second time, the frames with none, a menu.
    RUN("HU12", CaseHU12);
    // THE SECOND TARGET MOVED (2026-10-24, the SDR menu crash; dt_fg_hud.h): DXVK's defragmenter moves the HUD-less target to a new image
    // between the frame's picture and its hand, the worker held: the span names the image of now (fg_hud_rule.h, FgHudSrcTake)
    RUN("HU13", CaseHU13);
    RUN("HU14", CaseHU14);   // (2026-10-28, core audit finding 1: a Reset with the hand's HUD-less copy still on the card)
    // THE GAME'S WEB PAGE (2026-10-13; dt_fg_hud.h; source/fg/fg_web_rule.h): its state found in the game's code by shape from
    // two directions, or refused with its own reason; the rule that a present stands by while it is above zero. Pure:
    // no machine, so it is not run a second time under fg_q_at_hand.
    RUN("WP1", CaseWP1);
    // THE SAVE'S QUESTION (2026-10-12; dt_fg_saves.h): the product's answer, the device thread's tracker, in the merged
    // machine: the replayed writes and the worker's own; the calls beside the worker; the worker held where an order
    // matters; what cannot be followed; the census, held to the fake's own knowledge. Each kind with its site switched
    // off as its control.
    RUN("SV1", CaseSV1); RUN("SV2", CaseSV2); RUN("SV3", CaseSV3); RUN("SV4", CaseSV4); RUN("SV5", CaseSV5);
    // PHASE 4, THE FREE BAND (2026-10-12; dt_fg_rates.h): the merged machine closed over a fake game whose own cost sets the
    // period, the pacer, the placer and the worker together, the screen's log read with the instant of every present:
    // steady at 55, 42 and 30; a cost that wanders; the floor crossed both ways; a 144 Hz and a 240 Hz panel. Each
    // control plants the floor's half (or the 120 Hz panel's free range) and the judge must say so.
    RUN("FB1", CaseFB1); RUN("FB2", CaseFB2); RUN("FB3", CaseFB3); RUN("FB4", CaseFB4);
    // THE PRODUCT'S LAW (round 9, 2026-10-27; dt_fg_rates.h): the product's keys on the product's binding, one long frame, eight, a level
    RUN("PL1", CasePL1);
    // THE RIDE (2026-10-13; dt_fg_rates.h, behind FB4): the same closed machine over a fake with THE CARD'S ONE QUEUE, judged on
    // when each picture was SHOWN: the two locks' shapes of that morning's sitting (FQ1, FQ2), a 240 Hz panel's ceiling
    // (FQ3), what can meet a ride that is pending (FQ4), the ride's place in the worker's order with the worker held
    // in the next body (FQ5). (FB5, the forced period, went with its key in round 9's S7.)
    RUN("FQ1", CaseFQ1); RUN("FQ2", CaseFQ2); RUN("FQ3", CaseFQ3); RUN("FQ4", CaseFQ4); RUN("FQ5", CaseFQ5);
    // THE RIDE HOLDS (the evening of 2026-10-13; dt_fg_rates.h, at its end): the leg t90.2's fault, slow answers with the plan's
    // step and the conditions' rate rule in the loop (FQ6)
    RUN("FQ6", CaseFQ6);
    // THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT (2026-10-14; dt_fg_late.h): one late answer costs one generated frame (LA1, LA2);
    // the legs' locked state, the game first on the card, and its two mends (LA3); the copy over a dispatch in flight
    // behind the home, the helper held (LA4); the order wait's releasers, each with the other held (LA5), and its stops
    // (LA6); what arms it (LA7); the helper's place on the card, asked and given back (LA8)
    RUN("LA1", CaseLA1); RUN("LA2", CaseLA2); RUN("LA3", CaseLA3); RUN("LA4", CaseLA4); RUN("LA5", CaseLA5); RUN("LA6", CaseLA6); RUN("LA7", CaseLA7); RUN("LA8", CaseLA8);
    // EVERY GENERATED FRAME SHOWN ON A BUSY CARD (2026-10-14, afternoon; dt_fg_rideframes.h): the lock at 300 MHz settled, its generated
    // frames still due at the next injection and the period at the ride's wait floor (RF1); its engage's first two windows (RF2);
    // the early show in the worker's order, the worker held (RF3); FgPace's last spin hears the placer's wake (RF4); the worker ahead of the card waits for the answer behind a standing order wait (RF5)
    RUN("RF1", CaseRF1); RUN("RF2", CaseRF2); RUN("RF3", CaseRF3); RUN("RF4", CaseRF4); RUN("RF5", CaseRF5);
    // STAGE 3 (2026-10-17; dt_fg_place.h): the prepare's wait behind the answer, the helper let go, held, killed, a Reset's stop, and its control (PW1)
    RUN("PW1", CasePW1); RUN("PW2", CasePW2);
    // PHASE 5, THE LAW (2026-10-17, P1; dt_fg_law.h): a game that gets lighter while engaged stands down (LW2); a route across the floor (LW3);
    // the health exit, the rest and the back-off (LW4); a Reset mid-rest (LW5); the stand-down with the worker held (LW6); FusionFix's limiter
    // (LW7); NVIDIA's cap (LW8); the busy card the engage takes under the floor (LW9). (LW10, a forced period, went with its key in
    // round 9's S7.)
    RUN("LW2", CaseLW2); RUN("LW3", CaseLW3); RUN("LW4", CaseLW4); RUN("LW5", CaseLW5); RUN("LW6", CaseLW6); RUN("LW7", CaseLW7); RUN("LW8", CaseLW8);
    RUN("LW9", CaseLW9); RUN("LW12", CaseLW12); RUN("LW13", CaseLW13); RUN("LW14", CaseLW14); RUN("LW15", CaseLW15); RUN("LW16", CaseLW16); RUN("LW17", CaseLW17);
    RUN("LW19", CaseLW19);   // (2026-10-18, phase 5 round 2, S2: the morning's lawon leg in the development build's window order)
    RUN("LW20", CaseLW20);   // (S2: c's margin, a marginal game stays by and one that clearly gains engages at once)
    RUN("LW21", CaseLW21);   // (S2 review round 2: FGC_ONSLOW's band in the wait for a cut)
    RUN("LW22", CaseLW22);   // (S2 review round 3: a free card behind a heavy scene is not kept out)
    // (2026-10-18, round 3, G4: THE TRUE FILL'S LAW: the top, C, half the refresh, the CPU part's gate, the health exit, c under the pattern)
    RUN("LW29", CaseLW29);   // (2026-10-18, G4's round 3: the card standing by with vsync on, from the present's block)
    RUN("LW32", CaseLW32);   // (G4's round 3: the probe, a game that gets lighter while held)
    RUN("LW33", CaseLW33);   // (G4's round 4: a game in band never probes: lawedge's leg and twenty minutes with its spread)
    // (2026-10-25, round 8, P: K, the helper's answer on the card, the least c: FGC_ONSLOW on a weak card, a card with room, the fixed mode, the edge)
    RUN("LW46", CaseLW46); RUN("LW47", CaseLW47); RUN("LW48", CaseLW48); RUN("LW49", CaseLW49); RUN("LW50", CaseLW50);
    RUN("LW51", CaseLW51);   // (round 8, P, the review of c8821d7b: a short, hitchy stint files no K that keeps frame generation off)
    RUN("LW52", CaseLW52); RUN("LW53", CaseLW53);   // (2026-10-26, round 9, L2: the quick come-back behind a rate row's stand-down; no cycling, the guard)
    RUN("LW54", CaseLW54);   // (2026-10-28, core audit finding 2: a burst of late handoffs, the floors given back, the stint comes back)
    // AND BY NAME ALONE, as the replays are (each runs RL8's shape: 5.5 and 1.5 minutes at threaded=1): steady at the lock at 70's shape and the
    // engage's first second (LW1); R_free on a card that is the limit (LW11)
    if (gOnly) { RUN("LW1", CaseLW1); RUN("LW11", CaseLW11); }
    // PHASE 5, THE FIXED-REFRESH MODE (2026-10-17, night, F1; dt_fg_fixed.h): a fixed 120 Hz panel at a game of 90 (FX1); a game under half the
    // refresh (FX3); vsync off, and the table at fg_fixed 0 as before (FX4); the freeze proof (FX5); the mode's memory of a quantised display (FX6).
    // BY NAME ALONE: RL9 under the panel model's variable arm (FX0, 2,160 frames twice and a control), the panel at 119.88 for ten minutes (FX2)
    RUN("FX1", CaseFX1); RUN("FX3", CaseFX3); RUN("FX4", CaseFX4); RUN("FX5", CaseFX5); RUN("FX6", CaseFX6);
    // (round 2, 2026-10-18, the reviewer's findings) the game at the engage floor (FX7); the overrun stands down (FX8); the frame queue at 2 and 3 (FX9); the Present's own time (FX10)
    RUN("FX7", CaseFX7); RUN("FX8", CaseFX8); RUN("FX9", CaseFX9); RUN("FX10", CaseFX10);
    // (round 3, 2026-10-18) the way back in relaxes (FX12); a blind window teaches within a bound (FX13)
    RUN("FX12", CaseFX12); RUN("FX13", CaseFX13);
    RUN("FX14", CaseFX14);   // (2026-10-18, phase 5 round 2, S2: the morning's fixed120 leg in the development build's window order)
    RUN("FX15", CaseFX15);   // (S2: the panel's own evidence against NVIDIA's no)
    RUN("FX16", CaseFX16);   // (2026-10-18, round 3, G4: the evening's fixed120 leg, R_free under the fixed mode's hold)
    RUN("FX20", CaseFX20);   // (2026-10-24, round 6, F, the merge's reviewer R6: the player's fg_min_fps holds on a variable display)
    RUN("FX21", CaseFX21);   // (2026-10-24, round 7, F, F1: the fixed mode's overrun by the bulk, never the stutter tail)
    RUN("FX22", CaseFX22);   // (2026-10-24, round 7, F, F4: 165 to 240 Hz on the doubling)
    RUN("FX23", CaseFX23);
    RUN("FX24", CaseFX24);
    RUN("FX28", CaseFX28); RUN("FX29", CaseFX29); RUN("FX30", CaseFX30);   // (FX30: the reviewer's R1, no cycle at the refresh on a fixed 120)   // (FX29: 2026-10-26, round 9, L3b: the fixed screen's first switch-on at once behind the dwell)// (2026-10-26, round 9, L1: the fixed screen's standby reads the rate the screen gets, from the waiter's own intervals; FX27, round 8's B, retired with its count)
    RUN("FX25", CaseFX25); RUN("FX26", CaseFX26);   // (2026-10-25, the review of opt A: the integral's ratchet, a fast integral when the card's wait ends)   // (2026-10-25, the last optimisation's A: the card's wait on the fires' presents, the split and the card's rule)   // (2026-10-24, round 7 review, F: a lasting dip of the held rate stands the fixed mode down at its third window)
    RUN("SH1", CaseSH1); RUN("SH2", CaseSH2); RUN("SH3", CaseSH3); RUN("SH5", CaseSH5);   // (2026-10-24, round 6, W: the shipped build's wiring, fg_ship.h)
    RUN("WN1", CaseWN1); RUN("WN2", CaseWN2); RUN("WN3", CaseWN3); RUN("WN4", CaseWN4); RUN("WN5", CaseWN5);   // (2026-10-24, round 7, L: under Wine, fg_linux_rule.h)
    RUN("SH4", CaseSH4);   // (round 6, W; in the suite since F's hunk took FGC_PANEL's waiver off the fill)
    RUN("SH6", CaseSH6); RUN("SH7", CaseSH7); RUN("SH8", CaseSH8); RUN("SH9", CaseSH9); RUN("SH10", CaseSH10); RUN("SH11", CaseSH11); RUN("SH12", CaseSH12);   // (fg18-fix, the release audit of 2026-10-28: F2, F1, F3/F4/F10)   // (2026-10-24, round 7, W: the VRR active read, the classifier alone, the better refresh)
    // PHASE 5 ROUND 2, THE SWITCH-ON (2026-10-18, S1; dt_fg_ramp.h): lawon's switch-on reproduced and mended (RP1); every panel (RP2); the fixed
    // mode (RP3); the switch's floor and the reader of a window (RP4)
    RUN("RP1", CaseRP1); RUN("RP2", CaseRP2); RUN("RP3", CaseRP3); RUN("RP4", CaseRP4);
    // (S1 round 2, 2026-10-18) the target follows the law's prediction (RP7)
    RUN("RP7", CaseRP7); RUN("RP8", CaseRP8);
    // (round 3's switch-on cases by name; the grid fill's FP and FH cases went with the fill in round 8's deep strip)
    if (gOnly) { RUN("RP5", CaseRP5); RUN("RP6", CaseRP6); RUN("RP9", CaseRP9); }
    RUN("RP10", CaseRP10);   // (2026-10-18, round 3, SW: fill95's switch-ons, the card's queue read directly)   // (by name alone: the switch's cost a present, a timing; the ride's WAIT floor at lawon's shape, told)
    if (gOnly) { RUN("FX0", CaseFX0); RUN("FX2", CaseFX2); RUN("FX11", CaseFX11); }
    // THE SEVEN LEGS AT THE TWO LOCKS REPLAYED (2026-10-14, evening; dt_fg_replay.h): each leg's shape and keys, held to its W's numbers.
    // BY NAME ALONE, NOT IN THE SUITE: the gate they are is open (the header says which legs the simulation does not reach yet)
    if (gOnly) { RUN("RL1", CaseRL1); RUN("RL2", CaseRL2); RUN("RL3", CaseRL3); RUN("RL4", CaseRL4); RUN("RL5", CaseRL5); RUN("RL6", CaseRL6); RUN("RL7", CaseRL7); RUN("RL8", CaseRL8); RUN("RL9", CaseRL9); RUN("RL10", CaseRL10); RUN("RL11", CaseRL11); RUN("PS1", CasePS1); RUN("HR1", CaseHR1); }
    // EVERY CASE OF THE MERGED PATH ONCE MORE UNDER THE DEVELOPMENT KEY fg_q_at_hand, where the worker writes 'Q' at the hand
    // as it did until lever 2 (the shadow shape's sixteen cases ran here too, G4h, G8h and L1h to L14h, until round 9's strip)
    gShAtHand = 1; gCaseArm = " [fg_q_at_hand]";
    // (THE MERGED PATH IN THAT ORDER) (stage 7 carried the early 'Q' into the binding that arms: the key
    // gives the other order, the one the build map was written against)
    RUN("M1h", CaseM1); RUN("M1rh", CaseM1r); RUN("M2h", CaseM2); RUN("M3h", CaseM3); RUN("M4h", CaseM4); RUN("M5h", CaseM5); RUN("M6h", CaseM6);
    RUN("M7h", CaseM7); RUN("M8h", CaseM8); RUN("M9h", CaseM9); RUN("M10h", CaseM10); RUN("M11h", CaseM11); RUN("M12h", CaseM12);
    RUN("P1h", CaseP1); RUN("P2h", CaseP2); RUN("P3h", CaseP3); RUN("P4h", CaseP4); RUN("MCh", CaseMC); RUN("MCrh", CaseMCr);
    RUN("M13h", CaseM13); RUN("M14h", CaseM14); RUN("M15h", CaseM15); RUN("M16h", CaseM16); RUN("M17h", CaseM17); RUN("M18h", CaseM18); RUN("M19h", CaseM19); RUN("M20h", CaseM20);
    RUN("T2h", CaseT2); RUN("T3h", CaseT3); RUN("T4h", CaseT4); RUN("T6h", CaseT6); RUN("T7h", CaseT7);
    RUN("TL1h", CaseTL1);
    RUN("FL1h", CaseFL1);
    RUN("HU1h", CaseHU1); RUN("HU2h", CaseHU2); RUN("HU3h", CaseHU3); RUN("HU4h", CaseHU4); RUN("HU5h", CaseHU5); RUN("HU6h", CaseHU6);
    RUN("HU7h", CaseHU7); RUN("HU8h", CaseHU8); RUN("HU9h", CaseHU9); RUN("HU10h", CaseHU10);
    RUN("HU11h", CaseHU11);
    RUN("HU12h", CaseHU12);
    RUN("HU13h", CaseHU13);
    RUN("SV1h", CaseSV1); RUN("SV2h", CaseSV2); RUN("SV3h", CaseSV3); RUN("SV4h", CaseSV4); RUN("SV5h", CaseSV5);
    RUN("FB1h", CaseFB1); RUN("FB2h", CaseFB2); RUN("FB3h", CaseFB3); RUN("FB4h", CaseFB4);
    RUN("FQ1h", CaseFQ1); RUN("FQ2h", CaseFQ2); RUN("FQ3h", CaseFQ3); RUN("FQ4h", CaseFQ4); RUN("FQ5h", CaseFQ5);
    RUN("FQ6h", CaseFQ6);
    gShAtHand = 0; gCaseArm = "";
}

int main(int argc, char** argv)
{
    // "split" (stage 8 of phase 2): no case runs; the line says the product's shape compiled, and tools/test-fg-split.py
    // reads this program's object for what was instantiated over it
    if (argc > 1 && !strcmp(argv[1], "split"))
    {
        printf("the split: %d entry points of the merged path's shape (kRig false, kEngage true, kWire true) compiled and are in this program\nPASS\n", ArmedEntries());
        return 0;
    }
    gCfg = argc > 1 ? atoi(argv[1]) : 2;
    gOnly = (argc > 2 && strcmp(argv[2], "all")) ? argv[2] : nullptr;   // "all": no case filter, so a third argument can follow
    // C1'S SWITCH (the fold of DLSS-IV 1.0.8, 2026-10-03): a third argument of 1 runs every case with the recorded state-block
    // Apply on (B), which is what the Windows mod runs since 1.0.8 (DLSS-IV.cpp sets dtSbRecOn at InitThread); without it the
    // drained Apply (A), the header's default and what the mod runs under Wine. The gate runs both at both settings.
    dtSbRecOn = argc > 3 && atoi(argv[3]) != 0;
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("dt_fg_test: threaded=%d%s, frame generation's machine on the device thread, against the fake of DXVK\n", gCfg,
           dtSbRecOn ? ", C1 at B (a state block's Apply recorded, the Windows mod's path)" : ", C1 at A (a state block's Apply drained)");
    InitializeCriticalSection(&gFkLock);
    renderTid = GetCurrentThreadId();
    gFd = new FakeDev; gSc = new FakeRingSC; gFd->sc = gSc; gFd->Defaults(); gDev = gFd; D = gFd; SC = gSc;
    RingInstall(gFd, gSc);
    RingDeclareTags(kGen, kBlack);
    if (!DtStart(256u << 10)) { printf("DtStart refused\nFAILED\n"); return 1; }
    void** vt = *(void***)gFd;
    memcpy(dtDx, vt, sizeof(dtDx));
    static const DWORD kVs[] = { 0xFFFE0300u, 0x0000FFFFu }, kPs[] = { 0xFFFF0300u, 0x0000FFFFu };
    if (!DtInstall(gFd, vt, gCfg, kVs, kPs) || !dtArmable) { printf("DtInstall refused: %s\nFAILED\n", dtWhyNot); return 1; }
    if (!RingWrapDevice()) { printf("RingWrapDevice refused\nFAILED\n"); return 1; }
    FgHarnessInit();
    gDxvkPresent0 = dtScPresentO;
    gGateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    gThrQuitEv = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    gBb = gRing.surf[0];
    D->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &gOtherRt, nullptr);
    dtShBodiesEver = true;
    if (!EnsureArmed()) { printf("  the layer did not arm after 400 presents\nFAILED\n"); return 1; }

    RunCases();

    StopThreads();
    printf("\n  the cases, threaded=%d:\n", gCfg);
    int fails = 0, stubs = 0, nas = 0;
    for (const Row& r : gRows)
    {
        printf("  %-4s %-58s control: %s\n", r.result.c_str(), r.name.c_str(), r.control.empty() ? "(none)" : r.control.c_str());
        if (r.result == "FAIL") fails++;
        if (r.result == "STUB") stubs++;
        if (r.result == "N/A") nas++;
    }
    const RingStats& s = gRingSt;
    printf("\n  %zu cases (%d documented stubs and %d row(s) N/A at this setting, which do not count). The fake: %ld presents (%ld failed), %ld blits, %ld flushes, %ld raw submits,"
           " %ld raw and %ld D3D9 samples, %ld resets, %ld contended lock takes, %ld nested interop takes, %ld spare command buffers\n",
        gRows.size(), stubs, nas, s.presents, s.presentsFailed, s.blitsRun, s.flushes, s.submits, s.samplesRaw, s.samplesD3D9, s.resets,
        s.devContended, s.interopNestRelease, s.cbUnused);
    // THE RING FAKE'S POOL, IN THE TAIL (2026-10-19, round 3, G3): it only grows, and a run that reaches RING_IMG_MAX fails its last cases
    printf("  the ring fake made %zu of its %d images\n", gRing.img.size(), (int)RING_IMG_MAX);
    // WHICH ARM RAN, IN THE TAIL (run_proof.py keeps the last 24,000 characters, and the census's own line is far above them)
    printf("  C1 at %s: %ld state block Applies beside the rig in the census, %ld on the recorded path, %ld drained\n", DT_SB_REC_ON() ? "B" : "A", gCensusSb[0], gCensusSb[1], gCensusSb[2]);
    if (dtValidate) printf("  the layer's validation: %ld Gets and %ld whole-shadow compares checked against the fake device, %ld mismatch(es)\n", (long)dtValGets, (long)dtValChecks, (long)dtValMis);
    // THE THREE DEVICE CALLS WINDOWS' COMPATIBILITY LAYER HOOKS (QueryInterface, CreateAdditionalSwapChain, Reset) must never run on
    // the worker: since 1.0.8 the device thread installs over that layer's hooks because it makes them on the calling thread
    // only, and the fold lets the rig run behind the layer for the first time. The fake counts them (dt_fake.h, FkCaller3).
    if (gFkOnWorker3) { printf("  %ld of QueryInterface, CreateAdditionalSwapChain or Reset ran on the worker\n", (long)gFkOnWorker3); gErr++; }
    if (dtValMis) { printf("  the device thread's shadow differed from the device %ld time(s)\n", (long)dtValMis); gErr++; }
    if (gOnly) printf("  ONE CASE ONLY (%s): not every proof\n", gOnly);
    if (gRows.empty()) { printf("  no case ran\n"); gErr++; }
    if (fails || gErr) printf("  FAILED BECAUSE: %d case(s) failed, %d error(s) counted\n", fails, (int)gErr);
    printf("%s\n", (fails || gErr) ? "FAILED" : "PASS");
    return (fails || gErr) ? 1 : 0;
}
