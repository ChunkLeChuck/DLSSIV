// equiv_fgpace.cpp: FgPace (source/fg/fg_machine_render.h, Pace and PaceLaw) against A5's FgDelayPresent, quoted verbatim from
// the shelved code, WITH THE PACING LAW IN A5's STEPS 2 AND 3 (2026-09-26, frame generation phase 1C, stage S4:
// docs/reports/fg-1c/map-final.md, section 2.11 and S4's last bullet; PORTED in round 9's LW, 2026-10-27, when the law became
// every build's and A5's target and move went: docs/DECISIONS.md, "THE NEW PACING LAW SHIPS; A5 GOES").
//
//   python run_proof.py equiv_fgpace                                    the proof; its last line is its verdict
//   python run_proof.py equiv_fgpace -DEQUIV_FGPACE_CONTROL=1           control 1: the reference's join without the law's whole-us cut
//   python gen_equiv_fgpace_ctl.py <dir>                                control 2's copy of the header, one constant changed
//   python run_proof.py equiv_fgpace -DEQUIV_FGPACE_CONTROL=2 -I<dir>   control 2: the proof over that copy
//   python check_equiv_fgpace_quote.py                                  the quotes below are A5's lines, byte for byte
// prove-all runs all of them. Each control must FAIL, and at the comparison (exit 1, its verdict naming the control), not
// at the build (90).
//
// WHAT IT PROVES SINCE ROUND 9. FgPace is A5's FgDelayPresent in its steps 1 (the sample) and 4 (the due, no banking), with the
// plan's two changes and one named third, and THE PACING LAW (fg_law.h, FgLawStepForm) in steps 2 and 3 in A5's target and
// move's place, joined as PaceLaw joins it: the sample cut to whole microseconds, the entry's time, no hold band and the free
// range from the floor, the pad added to the law's answer (the binding here is not the merged path), the 40 ms ceiling, a move
// counted where the period changes. So the reference is A5's quoted lines with ONE block replaced: A5's 11943-11964 (the target
// and the move) by RefLawStep below, which runs fg_law.h on an instance of its own. THE LAW'S ARITHMETIC IS NOT WHAT THIS PROVES
// (both sides call the same function: equiv_fglaw holds it against its oracle); THIS PROVES THE MACHINE AROUND IT: that Pace
// hands the law the sample, the time and the floor A5's lines and the joins say, applies its answer as PaceLaw's joins say,
// and that the sample, the start, the engage and the due are A5's, step by step, every output compared. ITS CLAIM, AND NO MORE
// (the verifier's finding 9, 2026-09-26): identical on the compared streams. What differs from A5 by construction, or may, and
// is not compared, is listed in the output under "not compared"; what differs and was found is on a "REPORTED" line (never a
// failure): change 3's due debt, the engage's three named differences, and A5's refusal below 48 Hz. The sides:
//   - FgPace is THE REAL HEADER's: FgMachine<EqBinding>::Pace, CapLate and CapOnTime, included, over the minimal binding
//     below. Nothing of the machine is copied here.
//   - A5 is the shelved code at the tag frame-generation-last (the tag object c9f8e1a8, the commit ad1bf56d), DLSS-IV.cpp,
//     QUOTED: every line between an "A5 QUOTE" marker and its END is that line of that file, unchanged
//     (check_equiv_fgpace_quote.py proves it in the gate), and RefLawStep in the one gap.
//     The wrapping is two functions (FgCapRealFps's refresh from the stream, and FgDelayPresent cut to the pacer's lines),
//     the fake clock, and the sleep's stub.
//
// THE LAW IS MORE THAN THE MAP'S THREE RANGES. S4's bullet cites 9954-9970 (the pad), 11880-11888 (the start at the floor)
// and 11925-11995 (the sample, the target, the move, the due; since round 9 quoted as 11925-11942 and 11965-11995 around the
// gap the law fills). They hold the law's statements, but three more lines take
// part and are quoted too: 9858-9860 (kFgWarmPairs = 150, the warm-up the move waits out), 12000 and 12015 (the post-wait
// clock read that becomes fgPrevPairQpc, the next sample's start) and 12123-12128 (the warm-up's countdown). Map 2.11's own
// heading cites 12000-12015; S4's bullet dropped them. The state's declarations (9717, 9720, 9735, 9772, 11822, 11829-11837,
// 11843), FgCapRealFps's head (11854-11858) and its refusal below 48 Hz (11867-11879), the frequency read (11901) and the
// start's call site (11908-11911) are quoted so the quoted statements run over A5's own names and starting values; A5's
// forget (11846-11852) and the warm-up's re-arm (11720) are quoted for the engage's stream.
//
// THE THREE CHANGES, EACH HANDLED BY CONSTRUCTION AND NAMED IN THE OUTPUT:
//   1. THE SAMPLE EXCLUDES THE TIME INSIDE fgOrigSwapPresent (plan 3.7). The compared streams have none (tInOrig = 0), so
//      the exclusion keeps the whole sample and both sides take the same one. One stream has it (0.2 to 1.5 ms a present)
//      and shows the effect: against A5 on the same timeline FgPace differs from its first sample (expected); against A5
//      fed the sample the exclusion keeps, every output is identical at every step, the sample's side (the law's own period,
//      E, L and warm-up, the period, moves, pad) and the due's side alike: every present of that stream after the first sleeps (checked), and a
//      sleep ends at the due however late the clock was read, so the excluded time can reach the due only by a bug.
//   2. THE SLEEP PRIMITIVE (the rig's own timer loop, woken by the placer). The proof compares the sleep's TARGET, never
//      the primitive: A5's FgWaitUntil is a stub that records its target, and FgPace's target is its due at the decision.
//      Both fakes end a sleep when their clock reads the target, so both sides' next sample starts at the same instant.
//   3. ONE SLEEP IS CAPPED AT 20 ms (C-F30). The compared streams stay under it: a guard lengthens an interval that would
//      leave more than 19 ms to sleep, the same for both sides, and says how often it did; a sleep that still ended before
//      its target fails the proof. Two streams cross the cap on purpose (120 Hz, and 60 Hz where the floor itself is over
//      it): identical up to the first capped sleep, every capped sleep exactly 20 ms long; after it the law's outputs stay
//      identical but the real interval does not, and that is measured and REPORTED.
//
// THE CLOCKS. Each side has its own fake QPC at 10 MHz (Windows' own rate): every read returns the clock and moves it one
// tick, as a real clock moves between reads. A step sets the clock to the previous post-sleep read plus the stream's
// interval; the sample is then exactly the interval on both sides. FgPace's fake timer (PaceWait) wakes exactly at the
// instant it was set for and never on the placer's event, so its sleep loop's note to the worker is never reached and the
// device thread is never started; the header's own spin then reads the clock up to the target.
//
// COVERAGE. A branch the streams never take is a failure of the proof, not a pass. The classifier reads the reference's state
// before and after each step (the reference is A5's lines and the law; on a passing run FgPace's is the same). Where a branch
// leaves a mark of its own (a counter, the pad, the law's hitch and warm-up) the mark is read; the joins (the pad added, the
// 40 ms ceiling) are read where the period differs from the law's own answer, and the period must then be one of the two or
// the classifier is wrong and the proof fails. A5's own target and move branches went with them in round 9; the law's branches
// are equiv_fglaw's.
//
// THE CONTROLS (the gate fails if either ever passes): 1 changes the reference's join (the law handed the sample uncut, where
// PaceLaw cuts it to whole microseconds); 2 includes a copy of the header that gen_equiv_fgpace_ctl.py wrote with PaceLaw's
// pad weight 0.25 ms changed to 0.26, so the header's own join is shown to be what the comparison reads. Neither touches the
// real header on disk. (Until round 9: A5's down-move limit -0.4 to -0.41, and FgPace's up-move dwell 6 to 7, both A5's.)
#include <windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include <algorithm>
#include <string>
#include <vector>

// The device thread's prelude, as dt_fg_test.cpp has it: fg_machine.h is written over the device thread (FgPace's sleep
// loop calls DtNote and DtPublishFenced on a placer wake), so the header is included over the real one, unstarted.
static bool  telemetryOn = true;
static DWORD renderTid   = 0;
static int   rsOwnCreate = 0;
static void LogRecord(const char* fmt, ...) { (void)fmt; }
static void LogHealth(const char* fmt, ...) { (void)fmt; }
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
static bool latOn = false, latPacerOn = false;
static volatile LONG latPresFrame[LAT_RING];
DEVSTAT(alignas(8) static volatile LONGLONG latFlipRelQpc[LAT_RING]; static volatile float latFlipRelBlk[LAT_RING]; static volatile LONG latFlipRelN[LAT_RING];)
static volatile LONG frames = 0;
static void LatOnPresent(LONG, LONGLONG) {}
// THE REFRESH GUARD (next, 2026-09-26; merged into frame generation 2026-09-28): dt_res.h tests it after the pacing
// wait. Never armed here, as the rig parks it in the mod (FgRigParkGuard): a no-op, as dt_test's is.
static bool latGuard = false;
static void RgOnPresent() {}
#include "../../source/devthread/devthread.h"
static bool WriteSlot(void** vtbl, int slot, void* fn)   // devthread.h declares it; never called here (nothing is installed)
{
    DWORD old;
    if (!VirtualProtect(&vtbl[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    vtbl[slot] = fn;
    VirtualProtect(&vtbl[slot], sizeof(void*), old, &old);
    return vtbl[slot] == fn;
}

#ifndef EQUIV_FGPACE_CONTROL
#define EQUIV_FGPACE_CONTROL 0
#endif
#if EQUIV_FGPACE_CONTROL == 2
#include <fg_machine_ctl2.h>      // CONTROL 2: gen_equiv_fgpace_ctl.py's copy, found ONLY through run_proof.py's -I (the
                                  // angle form skips this folder, so a stale copy left here cannot shadow the fresh one)
#else
#include "../../source/fg/fg_machine.h"
#endif

static const LONGLONG kQpf   = 10000000;       // QPC's rate on Windows 10 and later
static const LONGLONG kT0    = 1000000000;     // the first present at 100 s
static const LONGLONG kGuard = 190000;         // 19 ms: the most a compared stream may leave to sleep (change 3's margin)

// ------------------------------------------------------------------ FgPace's side: the real header over a fake binding
static FgState  gS;
static void*    gMem = nullptr;
static LONGLONG gClockF = 0;      // FgPace's clock
static int      gHz = 120;
static LONG     gStep = 0;
static LONG     gTimerSets = 0;   // PaceWait calls: the primitive's, counted, never compared

struct EqBinding
{
    // THE THREE TRAITS (fg_machine.h): the rig's shape, which is the shape FgPace ran in. This binding had none, and needed
    // none while no member of the machine's CLASS asked a trait: since phase 3's stage 5 (2026-10-10) the class holds a
    // constant made of them (kEarlyQ), which every binding the machine is instantiated over must be able to answer, and
    // this proof did not compile from that commit until this line (found 2026-10-10, at stage 7).
    static constexpr bool kRig = true, kEngage = true, kWire = false;
    static FgState& S() { return gS; }
    static LONGLONG Now() { return gClockF++; }
    static LONGLONG Qpf() { return kQpf; }
    static void Line(const char*, ...) {}
    static LONG Frames() { return gStep; }
    static ULONGLONG RefreshPacked() { return ((ULONGLONG)(ULONG)gHz << 32) | 1u; }   // the stream's refresh, as num/1
    static int  FfCap() { return 0; }
    static int  FfRel() { return 0; }
    static int  DxvkCap() { return 0; }
    static bool Limiter() { return false; }
    static LONG PaceSleptN() { return 0; }
    // THE FAKE TIMER: wakes exactly at the instant it was set for, never on the pace event (change 2 is not compared).
    static int  PaceWait(LONGLONG dueQpc) { gTimerSets++; if (gClockF < dueQpc) gClockF = dueQpc; return 0; }
    static void WakePlacer() {}
    static void WakePace() {}
};
typedef FgMachine<EqBinding> M;

static void FgFresh()
{
    if (!gMem) gMem = VirtualAlloc(nullptr, FgStateBytes(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memset(gMem, 0, FgStateBytes());
    memset((void*)&gS, 0, sizeof(gS));
    FgStateAttach(gS, gMem);
}

// ------------------------------------------------------------------ A5's side: the shelved code, quoted
namespace a5
{
static bool telemetryOn = true;                  // A5's followed the log level (fg-last 67, 1532); on, so its counters count
static struct { int fg_cap; } ivtCfg = { -2 };   // fg_cap=-2: the adaptive pacer; 0 (no cap) only while the engage's stream
                                                 // has FgPace stopped, as A5's live fg_cap switch (8555) allowed
static LONGLONG clk = 0;                         // A5's clock: every read returns it and moves it one tick
static LONGLONG waitTarget = 0;                  // change 2: the target the stub below was asked for
static LONG     waitCalls = 0;
static int      hzNow = 120;
static void Qpc(LARGE_INTEGER* p) { p->QuadPart = clk++; }
static void QpfFake(LARGE_INTEGER* p) { p->QuadPart = kQpf; }
#define QueryPerformanceCounter(p)   a5::Qpc(p)
#define QueryPerformanceFrequency(p) a5::QpfFake(p)

// A5 QUOTE fg-last DLSS-IV.cpp:9717-9717
static LONGLONG fgQpcFreq = 0;
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:9720-9720
static LONGLONG fgPrevPairQpc = 0;
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:9735-9735
static double fgRefreshMs = 0.0;
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:9772-9772
static HANDLE fgTimerRender = nullptr;   // render thread: the output cap
// A5 QUOTE END

// CHANGE 2, THE SLEEP PRIMITIVE. A5's FgWaitUntil (fg-last 9784) is the primitive, not the law; what the law decides is
// its TARGET. The stub records the target and ends the wait when its clock reads it, as FgPace's fake timer and the
// header's spin end theirs.
static void FgWaitUntil(LONGLONG targetQpc, HANDLE* timerSlot)
{
    (void)timerSlot;
    waitTarget = targetQpc; waitCalls++;
    if (clk < targetQpc) clk = targetQpc;
    LARGE_INTEGER t; Qpc(&t);
}

// A5 QUOTE fg-last DLSS-IV.cpp:9858-9860
static const int kFgWarmPairs = 150;      // about two and a half seconds at 59/s
static int      fgWarmLeft = kFgWarmPairs;
static UINT64   fgWarmSkips = 0;
// A5 QUOTE END
// FgDelayRelease (fg-last 11716), cut to its warm-up line: A5 re-armed the warm-up at a Reset and a transport stop. The
// engage's stream calls it where FgPace starts again (the verifier's finding 4).
static void FgWarmRearm()
{
// A5 QUOTE fg-last DLSS-IV.cpp:11720-11720
    fgWarmLeft = kFgWarmPairs;
// A5 QUOTE END
}
// A5 QUOTE fg-last DLSS-IV.cpp:9954-9970
// THE ADAPTIVE PACER'S ONLY INPUT FROM THIS THREAD (roadmap A5, fg_cap=-2). The
// presenter says whether each generated frame was ready on time; the pacer
// lengthens the real period by a quarter millisecond per miss, up to 4 ms, and
// takes a quarter back after sixty clean pairs. Interlocked: two threads touch it.
static volatile LONG fgCapPadQms = 0;      // the pad, in quarter milliseconds, 0 to 16
static int           fgCapClean  = 0;      // clean pairs in a row; the presenter thread's alone
static void FgCapOnTime()
{
    if (ivtCfg.fg_cap != -2) return;
    if (++fgCapClean >= 60) { fgCapClean = 0; if (InterlockedCompareExchange(&fgCapPadQms, 0, 0) > 0) InterlockedDecrement(&fgCapPadQms); }
}
static void FgCapLate()
{
    if (ivtCfg.fg_cap != -2) return;
    fgCapClean = 0;
    if (InterlockedCompareExchange(&fgCapPadQms, 0, 0) < 16) InterlockedIncrement(&fgCapPadQms);
}
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:11822-11822
static int      fgCapFps   = 0;        // real frames per second, 0 = no cap
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:11829-11837
static double   fgCapPeriodMs = 0.0, fgCapPrevPeriodMs = 0.0, fgCapFloorMs = 0.0;
static double   fgFreeAvgMs = 0.0, fgFreeHiMs = 0.0;
static int      fgCapDwell = 0;
static UINT64   fgCapMoves = 0;        // report only
static LONGLONG fgCapDue   = 0;        // when the next real frame may go out
static UINT64   fgCapWaits = 0, fgCapResyncs = 0;
// The ordinary "behind the clock, restart it" case, kept apart from the
// four-step resync so the change above can be seen rather than assumed.
static UINT64   fgCapClamps = 0;
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:11843-11843
static bool     fgCapSaid  = false;
// A5 QUOTE END
// A5's forget: at a Reset (10224), a transport stop (11803) and an fg_cap change (8555). The engage's stream calls it where
// FgPace stops (the fg_cap switch to 0) and where it starts again (the switch back, with the warm-up re-armed).
// A5 QUOTE fg-last DLSS-IV.cpp:11846-11852
static void FgCapForget()
{
    fgCapSaid = false; fgRefreshMs = 0.0; fgCapFps = 0;
    fgCapPeriodMs = 0.0; fgCapPrevPeriodMs = 0.0; fgCapFloorMs = 0.0;
    fgFreeAvgMs = 0.0; fgFreeHiMs = 0.0; fgCapDwell = 0; fgCapMoves = 0;
    fgCapClean = 0; InterlockedExchange(&fgCapPadQms, 0);
}
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:11854-11858
static int FgCapRealFps(IDirect3DDevice9* dev)
{
    if (ivtCfg.fg_cap == 0) { fgCapPeriodMs = 0.0; return 0; }                                                // asked for no cap
    if (ivtCfg.fg_cap  > 0) { fgCapPeriodMs = 1000.0 / (double)ivtCfg.fg_cap; return ivtCfg.fg_cap; }    // asked for a specific one
    int hz = 0;
// A5 QUOTE END
    hz = hzNow; (void)dev;               // 11859-11866 ask the display; the stream names its refresh
// A5 QUOTE fg-last DLSS-IV.cpp:11867-11888
    if (hz <= 1) { fgCapPeriodMs = 0.0; return 0; }    // unknown: do not invent a cap
    if (hz < 48)
    {
        // A MODE THE PAIR CANNOT SERVE. The owner's forced 3840x2160 once reported
        // 29 Hz and the cap came out at 13 real frames a second, which read as the
        // game crawling. Below 48 Hz no cap is derived: the game runs free and the
        // line says why. Applies to -1 and -2 alike; 0 and N never reach here.
        static int saidLow = 0;
        if (saidLow++ < 3)
            LogRecord("  fg: the display mode reports %d Hz; a cap derived from it would hold the game to %d real frames"
                      " a second, so NO cap is applied at this mode", hz, (hz > 4 ? hz - 2 : hz) / 2);
        fgCapPeriodMs = 0.0; return 0;
    }
    fgRefreshMs = 1000.0 / (double)hz;                   // the floor the pacer respects
    // THE PANEL'S CEILING AS A PERIOD: two presents per real frame must fit one
    // refresh each, plus 1.5 percent for the driver's rounding of the rate.
    fgCapFloorMs = 2.0 * fgRefreshMs * 1.015;
    if (ivtCfg.fg_cap == -2)                             // adaptive: starts at the ceiling and follows the game
    {
        fgCapPeriodMs = fgCapFloorMs; fgCapPrevPeriodMs = fgCapFloorMs;
        return (int)(1000.0 / fgCapPeriodMs + 0.5);
    }
// A5 QUOTE END
    return -1;                           // not reached at fg_cap=-2: 11889-11892 are the fixed cap's
}

// STEPS 2 AND 3 SINCE ROUND 9: THE LAW, IN A5'S TARGET AND MOVE'S PLACE (11943-11964), joined as fg_machine_render.h's PaceLaw
// joins it, written out here from its rules (not copied): the edges no hold band and the free range from the floor (FgLawReset's
// 0 and 0, at the start: FgCapRealFps's first call, where A5 set its period to the floor), the floor handed in every step
// (FgLawFloor), the sample cut to whole microseconds and the frame's entry as its time, the free band's form at a head of 1.0,
// the pad added, 40 ms at the most, a move counted where the period changes and the previous period kept for that step alone.
// One law per side (the exclusion's stream runs two A5s): the driver points `law` at the side's own and sets `lawFresh` where A5
// starts (its first present, and every start of the engage's stream); `entry` is the frame's entry on that side's clock.
static FgLaw*   law = nullptr;
static bool     lawFresh = true;
static LONGLONG entry = 0;
static void RefLawStep(LONGLONG nowQpc)
{
    FgLaw& w = *law;
    if (lawFresh) { FgLawReset(w, fgCapFloorMs, 0.0, 0.0, 1.0); lawFresh = false; }
    FgLawFloor(w, fgCapFloorMs);
    const double sample = fgPrevPairQpc ? (double)(nowQpc - fgPrevPairQpc) * 1000.0 / (double)fgQpcFreq : 0.0;
#if EQUIV_FGPACE_CONTROL == 1
    const double c = sample;                         // CONTROL 1: the sample handed in uncut; this build must FAIL at the comparison
#else
    const double c = sample > 0.0 ? (double)(LONG)(sample * 1000.0) / 1000.0 : 0.0;
#endif
    double per = FgLawStepForm(w, c, (double)entry * 1000.0 / (double)fgQpcFreq);
    per += (double)InterlockedCompareExchange(&fgCapPadQms, 0, 0) * 0.25;
    if (per > 40.0) per = 40.0;
    if (per != fgCapPeriodMs) { fgCapPrevPeriodMs = fgCapPeriodMs; fgCapPeriodMs = per; if (telemetryOn) fgCapMoves++; }
    else fgCapPrevPeriodMs = fgCapPeriodMs;
}
// FgDelayPresent (fg-last 11895) cut to the pacer's lines, in their order. Between them A5 saves, hands off, presents,
// restores and arms the presenter, none of which the pacer reads.
static void Present()
{
    IDirect3DDevice9* const dev = nullptr;
// A5 QUOTE fg-last DLSS-IV.cpp:11901-11901
    if (!fgQpcFreq) QueryPerformanceFrequency((LARGE_INTEGER*)&fgQpcFreq);
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:11908-11911
    if (!fgCapSaid)
    {
        fgCapSaid = true;
        fgCapFps  = FgCapRealFps(dev);
// A5 QUOTE END
    }                                    // 11912-11923 are the start's log lines
// A5 QUOTE fg-last DLSS-IV.cpp:11925-11942
    if (fgCapFps > 0 && fgQpcFreq)
    {
        LONGLONG now = 0; QueryPerformanceCounter((LARGE_INTEGER*)&now);
        if (ivtCfg.fg_cap == -2 && fgCapPeriodMs > 0.1)
        {
            // 1. THE GAME'S OWN COST, the cap wait excluded by construction: from
            //    last frame's post-wait sample (fgPrevPairQpc, not yet overwritten
            //    below) to this frame's pre-wait now.
            if (fgPrevPairQpc)
            {
                const double freeMs = (double)(now - fgPrevPairQpc) * 1000.0 / (double)fgQpcFreq;
                if (freeMs > 0.2 && freeMs < 100.0)
                {
                    fgFreeAvgMs += (freeMs - fgFreeAvgMs) * 0.0625;
                    if (freeMs > fgFreeHiMs) fgFreeHiMs = freeMs;                 // a new worst is taken at once
                    else fgFreeHiMs += (fgFreeAvgMs - fgFreeHiMs) * 0.02;         // and forgotten slowly
                }
            }
// A5 QUOTE END
            RefLawStep(now);                     // 11943-11964, A5's target and move: the law since round 9 (RefLawStep above)
// A5 QUOTE fg-last DLSS-IV.cpp:11965-11995
        }
        const LONGLONG step = (ivtCfg.fg_cap == -2 && fgCapPeriodMs > 0.1)
                            ? (LONGLONG)(fgCapPeriodMs * (double)fgQpcFreq / 1000.0)
                            : fgQpcFreq / fgCapFps;
        // Four steps of deficit resync the clock, and never more than 120 ms of it
        // (tenet 13): with an adaptive step the four could otherwise mean 160 ms.
        const LONGLONG resync = (step * 4 < (LONGLONG)(fgQpcFreq * 12 / 100)) ? step * 4 : (LONGLONG)(fgQpcFreq * 12 / 100);
        // A limiter must never BANK time: after a hitch, catching up runs several
        // frames uncapped and hands the panel exactly the burst the cap exists
        // to prevent. The comment said that and the code allowed FOUR steps of
        // it, which is not a rounding allowance, it is four whole frames of
        // uncapped output after every hitch.
        //
        // MEASURED. real->gen is metronomic at 8.47 to 8.48 because we schedule
        // it. gen->real, which is the game's next real present and which nothing
        // schedules, ran from 0.43 ms to 31.55 ms in one run. 0.43 ms means the
        // generated frame was on screen for under half a millisecond, which
        // needs a real interval near 8.9 ms, which a 16.95 ms cap is supposed to
        // make impossible. Banked time is how it became possible.
        //
        // So bank nothing. Behind the clock means restart the clock, which makes
        // this a pure minimum-interval limiter.
        if (fgCapDue == 0 || now > fgCapDue + resync) { fgCapDue = now; if (telemetryOn) fgCapResyncs++; }
        else if (now > fgCapDue) { fgCapDue = now; if (telemetryOn) fgCapClamps++; }
        else if (fgCapDue - now > (LONGLONG)(fgQpcFreq / 20000))   // > 0.05 ms left
        {
            FgWaitUntil(fgCapDue, &fgTimerRender); // render thread
            if (telemetryOn) fgCapWaits++;
        }
        fgCapDue += step;
    }
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:12000-12000
    LONGLONG q0 = 0; QueryPerformanceCounter((LARGE_INTEGER*)&q0);
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:12015-12015
    fgPrevPairQpc = q0;
// A5 QUOTE END
// A5 QUOTE fg-last DLSS-IV.cpp:12123-12128
    if (fgWarmLeft > 0)
    {
        // The handoff above already went out, so the host is warming up on real
        // work. Only the present is withheld.
        fgWarmLeft--; fgWarmSkips++;
    }
// A5 QUOTE END
}
#undef QueryPerformanceCounter
#undef QueryPerformanceFrequency
}   // namespace a5

// A5's state, one instance a side: the exclusion's stream runs two A5s, so the quoted statics are loaded and saved around
// every call.
#define A5_STATE(X) \
    X(LONGLONG, clk) X(LONGLONG, fgQpcFreq) X(LONGLONG, fgPrevPairQpc) X(LONGLONG, fgCapDue) X(LONGLONG, waitTarget) \
    X(double, fgRefreshMs) X(double, fgCapPeriodMs) X(double, fgCapPrevPeriodMs) X(double, fgCapFloorMs) \
    X(double, fgFreeAvgMs) X(double, fgFreeHiMs) \
    X(int, fgWarmLeft) X(int, fgCapClean) X(int, fgCapFps) X(int, fgCapDwell) X(int, hzNow) \
    X(UINT64, fgWarmSkips) X(UINT64, fgCapMoves) X(UINT64, fgCapWaits) X(UINT64, fgCapResyncs) X(UINT64, fgCapClamps) \
    X(LONG, fgCapPadQms) X(LONG, waitCalls) X(bool, fgCapSaid) \
    X(FgLaw*, law) X(bool, lawFresh) X(LONGLONG, entry)
// (round 9) the reference's law, one a side: the main A5's and the exclusion's second A5's (123 KB each: not on the stack)
static FgLaw gLawA, gLawAx;
struct A5State
{
#define X(t, n) t n;
    A5_STATE(X)
#undef X
};
static void A5Load(const A5State& s)
{
#define X(t, n) a5::n = s.n;
    A5_STATE(X)
#undef X
}
static void A5Save(A5State& s)
{
#define X(t, n) s.n = a5::n;
    A5_STATE(X)
#undef X
}
static A5State A5Fresh(int hz, FgLaw* lw)
{
    A5State s; memset(&s, 0, sizeof(s));
    s.fgWarmLeft = a5::kFgWarmPairs;   // 9859's starting value
    s.hzNow = hz;
    s.law = lw; s.lawFresh = true;     // (round 9: the side's own law, reset at A5's first present)
    return s;
}

// ------------------------------------------------------------------ what is compared
// EVERY OUTPUT, after every step (and pad and clean after every pad call). The first nine are the sample's side (they depend
// only on the samples and the pad calls); the rest are the due's side (they depend on the clock too). (Round 9: the law's own
// period, E, L and warm-up in the places of A5's mean, worst, dwell and warm-up count.)
enum { O_LPER, O_E, O_PERIOD, O_PREV, O_L, O_LWARM, O_MOVES, O_PAD, O_CLEAN, O_DUE, O_RESYNCS, O_CLAMPS, O_SLEPT, O_TARGET, O_N };
static const int kSampleSide = O_CLEAN + 1;
struct FieldInfo { const char* name; const char* a5; const char* fg; bool isDouble; };
static const FieldInfo kField[O_N] =
{
    { "the law's own period (FgLaw::per)",             "RefLawStep", "PaceLaw (fg_law.h)", true },
    { "the law's E (FgLaw::E)",                        "RefLawStep", "PaceLaw (fg_law.h)", true },
    { "period (fgCapPeriodMs, r.period)",              "RefLawStep", "PaceLaw, the joins", true },
    { "previous period (fgCapPrevPeriodMs, r.prevPeriod)", "RefLawStep", "PaceLaw, the joins", true },
    { "the law's L (FgLaw::L)",                        "RefLawStep", "PaceLaw (fg_law.h)", true },
    { "the law's warm-up left (FgLaw::warm)",          "RefLawStep", "PaceLaw (fg_law.h); Pace's start", false },
    { "moves (fgCapMoves, r.moves)",                   "RefLawStep", "PaceLaw, the joins", false },
    { "pad qms (fgCapPadQms, s.sh.padQms)",            "A5 fg-last 9960-9970",   "CapLate, CapOnTime", false },
    { "clean run (fgCapClean, s.pl.capClean)",         "A5 fg-last 9960-9970",   "CapLate, CapOnTime", false },
    { "due (fgCapDue, r.due)",                         "A5 fg-last 11966-11994", "Pace, 4. THE DUE", false },
    { "resyncs (fgCapResyncs, r.resyncs)",             "A5 fg-last 11987",       "Pace, 4. THE DUE", false },
    { "clamps (fgCapClamps, r.clamps)",                "A5 fg-last 11988",       "Pace, 4. THE DUE", false },
    { "slept (fgCapWaits, r.paceSlept)",               "A5 fg-last 11989-11993", "Pace, 4. THE DUE", false },
    { "sleep target (FgWaitUntil's, SleepUntil's)",    "A5 fg-last 11991",       "Pace, 4. THE DUE (SleepUntil(r.due))", false },
};
struct Out { unsigned long long v[O_N]; };
static unsigned long long Bits(double d) { unsigned long long u; memcpy(&u, &d, 8); return u; }
static double Dbl(unsigned long long u) { double d; memcpy(&d, &u, 8); return d; }
static Out OutF(int slept, LONGLONG target)
{
    const FgRenderLine& r = gS.r; const FgLaw& w = *gS.law;
    Out o;
    o.v[O_LPER] = Bits(w.per); o.v[O_E] = Bits(w.E); o.v[O_PERIOD] = Bits(r.period); o.v[O_PREV] = Bits(r.prevPeriod);
    o.v[O_L] = Bits(w.L); o.v[O_LWARM] = (unsigned long long)(LONGLONG)w.warm;
    o.v[O_MOVES] = (unsigned long long)(LONGLONG)r.moves; o.v[O_PAD] = (unsigned long long)(LONGLONG)gS.sh.padQms;
    o.v[O_CLEAN] = (unsigned long long)(LONGLONG)gS.pl.capClean; o.v[O_DUE] = (unsigned long long)r.due;
    o.v[O_RESYNCS] = (unsigned long long)(LONGLONG)r.resyncs; o.v[O_CLAMPS] = (unsigned long long)(LONGLONG)r.clamps;
    o.v[O_SLEPT] = (unsigned long long)slept; o.v[O_TARGET] = (unsigned long long)target;
    return o;
}
static Out OutA(const A5State& a, int slept, LONGLONG target)
{
    Out o; const FgLaw& w = *a.law;
    o.v[O_LPER] = Bits(w.per); o.v[O_E] = Bits(w.E); o.v[O_PERIOD] = Bits(a.fgCapPeriodMs);
    o.v[O_PREV] = Bits(a.fgCapPrevPeriodMs);
    o.v[O_L] = Bits(w.L); o.v[O_LWARM] = (unsigned long long)(LONGLONG)w.warm;
    o.v[O_MOVES] = (unsigned long long)a.fgCapMoves; o.v[O_PAD] = (unsigned long long)(LONGLONG)a.fgCapPadQms;
    o.v[O_CLEAN] = (unsigned long long)(LONGLONG)a.fgCapClean; o.v[O_DUE] = (unsigned long long)a.fgCapDue;
    o.v[O_RESYNCS] = (unsigned long long)a.fgCapResyncs; o.v[O_CLAMPS] = (unsigned long long)a.fgCapClamps;
    o.v[O_SLEPT] = (unsigned long long)slept; o.v[O_TARGET] = (unsigned long long)target;
    return o;
}
static std::string Val(int f, unsigned long long u)
{
    char b[64];
    if (kField[f].isDouble) sprintf_s(b, "%.17g", Dbl(u));
    else if (f == O_DUE || f == O_TARGET) sprintf_s(b, "%lld (%.4f ms)", (LONGLONG)u, (double)((LONGLONG)u - kT0) * 1000.0 / kQpf);
    else sprintf_s(b, "%lld", (LONGLONG)u);
    return b;
}
static int gMismatch = 0, gFail2 = 0;     // unexpected differences (exit 1); every other failure (exit 2)
static int gReported = 0;                 // divergences REPORTED for the main session, each on a line "  REPORTED": never a failure
static int gNamed = 0;                    // differences the main session DECIDED (DECISIONS "PHASE 1C, S4"), each on a line "  NAMED"
static bool Same(const Out& a, const Out& f, int from, int to) { for (int i = from; i < to; i++) if (a.v[i] != f.v[i]) return false; return true; }
static void Report(const char* tag, int stream, int step, const Out& a, const Out& f, int from, int to)
{
    printf("    %s stream %d step %d:\n", tag, stream, step);
    for (int i = from; i < to; i++)
        if (a.v[i] != f.v[i])
            printf("      %-52s A5 %-28s FgPace %s   (the reference: %s; FgPace: %s)\n", kField[i].name,
                   Val(i, a.v[i]).c_str(), Val(i, f.v[i]).c_str(), kField[i].a5, kField[i].fg);
}

// ------------------------------------------------------------------ the coverage of A5's lines and the law's joins
// (round 9: A5's target, band and move branches went with them; the law's own branches are equiv_fglaw's)
enum Br
{
    BR_PAD_RAISE, BR_PAD_MAX, BR_PAD_BACK, BR_PAD_NOTHING, BR_PAD_CLEAN,
    BR_S_FIRST, BR_S_KEPT, BR_S_HITCH, BR_S_LOW, BR_S_HIGH,
    BR_S_LOW_EXACT, BR_S_HIGH_EXACT, BR_S_NEAR_LOW, BR_S_NEAR_HIGH,
    BR_J_WARM, BR_J_STILL, BR_J_UP, BR_J_DOWN, BR_J_LAWUP, BR_J_LAWDOWN, BR_J_PAD, BR_J_CEIL,
    BR_D_RESYNC0, BR_D_RESYNC, BR_D_RESYNC_W4, BR_D_RESYNC_W120, BR_D_CLAMP, BR_D_SLEEP, BR_D_ONTIME,
    BR_D_EDGE, BR_D_AT_DUE, BR_D_AT_005,
    BR_WIN4, BR_WIN120, BR_N
};
static const char* kBr[BR_N] =
{
    "pad: a miss raises it a quarter ms (9969)", "pad: a miss at its maximum, 16, holds it (9969)",
    "pad: 60 clean pairs take a quarter back (9963)", "pad: 60 clean pairs, nothing to take back (9963)",
    "pad: a clean pair counted (9963)",
    "sample: the first present has none (11933)", "sample: taken by the law, not a hitch (fg_law.h)",
    "sample: a hitch, over 1.5 periods (fg_law.h)", "sample: dropped, 0.2 ms or less (the law's bound)",
    "sample: dropped, 100 ms or more (the law's bound)",
    "sample: dropped, exactly 0.2 ms, 2000 ticks (the law's bound)", "sample: dropped, exactly 100 ms, 1,000,000 ticks (the law's bound)",
    "sample: kept, over 0.2 ms by 0.05 ms or less (the law's bound)", "sample: kept, under 100 ms by 1 ms or less (the law's bound)",
    "join: the law in its warm-up (150 steps from a start)", "join: the period unchanged", "join: the period up, a move counted",
    "join: the period down, a move counted", "join: the law's own period up", "join: the law's own period down",
    "join: the pad added to the law's answer", "join: the 40 ms ceiling over the law's answer and the pad",
    "due: resync, no due yet (11987)", "due: resync, behind by more than the window (11987)",
    "due: resync that only the 4-step window makes (11971)", "due: resync that only the 120 ms window makes (11971)",
    "due: behind the clock, restarted (11988)", "due: sleep to the due (11989-11993)",
    "due: on time, 0.05 ms or less left, no sleep (11989)",
    "due: exactly at the due plus the window, clamped (11987-11988)", "due: exactly at the due, on time (11988-11989)",
    "due: exactly 0.05 ms, 500 ticks, left, no sleep (11989)",
    "window: 4 steps in force (11971)", "window: 120 ms in force (11971)",
};
static LONGLONG gBr[BR_N];
static void Instrument(const char* what, int stream, int step)
{
    if (gFail2 < 20) printf("    COVERAGE INSTRUMENT: stream %d step %d: %s\n", stream, step, what);
    gFail2++;
}
static void ClassifyPad(int kind, const A5State& a0, const A5State& a1, int stream, int step)
{
    if (kind == 1)
    {
        if (a1.fgCapClean != 0) Instrument("a late call left the clean run above 0", stream, step);
        if (a0.fgCapPadQms < 16) { if (a1.fgCapPadQms == a0.fgCapPadQms + 1) gBr[BR_PAD_RAISE]++; else Instrument("a late call under 16 did not raise the pad", stream, step); }
        else { if (a1.fgCapPadQms == 16) gBr[BR_PAD_MAX]++; else Instrument("a late call at 16 moved the pad", stream, step); }
        return;
    }
    if (a0.fgCapClean + 1 < 60) { if (a1.fgCapClean == a0.fgCapClean + 1 && a1.fgCapPadQms == a0.fgCapPadQms) gBr[BR_PAD_CLEAN]++; else Instrument("a clean pair under 60 was not counted", stream, step); return; }
    if (a1.fgCapClean != 0) { Instrument("the 60th clean pair did not restart the run", stream, step); return; }
    if (a0.fgCapPadQms > 0) { if (a1.fgCapPadQms == a0.fgCapPadQms - 1) gBr[BR_PAD_BACK]++; else Instrument("the 60th clean pair did not take a quarter back", stream, step); }
    else { if (a1.fgCapPadQms == 0) gBr[BR_PAD_NOTHING]++; else Instrument("the 60th clean pair moved a pad of 0", stream, step); }
}
// The law's own before a step (the reference's law is one object, stepped in place): its period and warm-up.
struct LawSnap { double per; int warm; };
static void ClassifyStep(const A5State& a0, const A5State& a1, const LawSnap& l0, LONGLONG now, int stream, int step)
{
    const FgLaw& w = *a1.law;
    // THE SAMPLE: its value is the stream's input, the branch is read from the law's state.
    if (!a0.fgPrevPairQpc) gBr[BR_S_FIRST]++;
    else
    {
        const double ms = (double)(now - a0.fgPrevPairQpc) * 1000.0 / (double)kQpf;
        if (ms <= 0.2 || ms >= 100.0)
        {
            if (w.hitch) Instrument("a sample outside 0.2 to 100 ms was called a hitch (the law did not leave it out)", stream, step);
            gBr[ms <= 0.2 ? BR_S_LOW : BR_S_HIGH]++;
            // THE BOUNDS THEMSELVES (the verifier's finding 1): a law with ">=" or "<=" there keeps these.
            if (ms == 0.2) gBr[BR_S_LOW_EXACT]++;
            if (ms == 100.0) gBr[BR_S_HIGH_EXACT]++;
        }
        else
        {
            gBr[w.hitch ? BR_S_HITCH : BR_S_KEPT]++;
            // AND JUST INSIDE THEM: a law whose bounds moved inward (0.25, 99) drops these.
            if (ms <= 0.25) gBr[BR_S_NEAR_LOW]++;
            if (ms >= 99.0) gBr[BR_S_NEAR_HIGH]++;
        }
    }
    // THE JOINS: the period is the law's answer with the pad, never over 40 ms; a move is counted where it changed.
    const double p0 = a0.fgCapPeriodMs, p1 = a1.fgCapPeriodMs;
    const bool moved = a1.fgCapMoves == a0.fgCapMoves + 1;
    if (!moved && a1.fgCapMoves != a0.fgCapMoves) Instrument("moves changed by more than one", stream, step);
    if (moved != (Bits(p1) != Bits(p0)) && a0.fgCapPeriodMs > 0.0) Instrument("a move counted where the period did not change, or none where it did", stream, step);
    if (l0.warm > 0) gBr[BR_J_WARM]++;
    if (a0.fgCapPeriodMs > 0.0) gBr[!moved ? BR_J_STILL : p1 > p0 ? BR_J_UP : BR_J_DOWN]++;
    if (w.per > l0.per) gBr[BR_J_LAWUP]++;
    if (w.per < l0.per && l0.per > 0.0) gBr[BR_J_LAWDOWN]++;
    double withPad = w.per; withPad += (double)a1.fgCapPadQms * 0.25;
    if (Bits(p1) == Bits(withPad) && withPad <= 40.0) { if (a1.fgCapPadQms) gBr[BR_J_PAD]++; }
    else if (Bits(p1) == Bits(40.0) && withPad > 40.0) gBr[BR_J_CEIL]++;
    else Instrument("the period is neither the law's answer with the pad nor the 40 ms ceiling over it", stream, step);
    // THE DUE.
    const UINT64 rs = a1.fgCapResyncs - a0.fgCapResyncs, cl = a1.fgCapClamps - a0.fgCapClamps, sl = a1.fgCapWaits - a0.fgCapWaits;
    if (rs + cl + sl > 1) Instrument("more than one of resync, clamp and sleep in one step", stream, step);
    const LONGLONG base = (rs || cl) ? now : a0.fgCapDue;   // a resync or a clamp sets the due to now, then adds the step
    const LONGLONG stepT = a1.fgCapDue - base;
    const bool w4 = stepT * 4 < kQpf * 12 / 100;
    gBr[w4 ? BR_WIN4 : BR_WIN120]++;
    if (rs)
    {
        if (!a0.fgCapDue) gBr[BR_D_RESYNC0]++;
        else
        {
            const LONGLONG late = now - a0.fgCapDue;
            gBr[BR_D_RESYNC]++;
            if (w4 && late <= kQpf * 12 / 100) gBr[BR_D_RESYNC_W4]++;          // the 120 ms window would have clamped it
            if (!w4 && late <= stepT * 4) gBr[BR_D_RESYNC_W120]++;             // the 4-step window would have clamped it
        }
    }
    else if (cl)
    {
        gBr[BR_D_CLAMP]++;
        // EXACTLY ON THE WINDOW'S EDGE (the verifier's finding 5): a law with ">=" there resyncs this one.
        if (now == a0.fgCapDue + (w4 ? stepT * 4 : kQpf * 12 / 100)) gBr[BR_D_EDGE]++;
    }
    else if (sl) gBr[BR_D_SLEEP]++;
    else
    {
        if (a0.fgCapDue - now > kQpf / 20000) Instrument("more than 0.05 ms left and no sleep", stream, step);
        gBr[BR_D_ONTIME]++;
        // EXACTLY AT THE DUE, AND EXACTLY 0.05 ms BEFORE IT: a law with ">=" at 11988 clamps the first, at 11989 sleeps the
        // second. QPC counts whole ticks, so both happen in game.
        if (now == a0.fgCapDue) gBr[BR_D_AT_DUE]++;
        if (a0.fgCapDue - now == kQpf / 20000) gBr[BR_D_AT_005]++;
    }
}

// ------------------------------------------------------------------ the streams
// pad: 0 none, 1 late (FgCapLate), 2 on time (FgCapOnTime). ivT: the interval in exact ticks, which replaces ivMs when set.
// off: the engage's stream only, a present where FgPace does not run.
struct In { double ivMs, inOrigMs; int pad; LONGLONG ivT; int off; };
struct Rng
{
    unsigned long long s;
    unsigned long long Next() { unsigned long long z = (s += 0x9E3779B97F4A7C15ull); z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull; z = (z ^ (z >> 27)) * 0x94D049BB133111EBull; return z ^ (z >> 31); }
    double U() { return (double)(Next() >> 11) * (1.0 / 9007199254740992.0); }
    double U(double a, double b) { return a + (b - a) * U(); }
};
// A RANDOM STREAM: regimes of 60 to 700 presents whose cost WALKS (each regime 0.7 to 1.4 times the last, between the
// floor less 13 ms and 39 ms), with jitter; spikes, stalls past 100 ms and double presents under 0.2 ms; pad calls after
// the warm-up (as the placer makes them: a warm-up pair is neither late nor clean), late at a regime's own rate. A walk,
// not independent draws: A5 gives a period back only 0.4 ms per 60 presents, so a heavy regime followed at once by a light
// one leaves more than 20 ms to sleep for thousands of presents, and the guard would shape most of the stream.
static std::vector<In> GenRandom(unsigned long long seed, int hz, int steps)
{
    Rng g = { seed };
    const double floorMs = 2.0 * 1.015 * 1000.0 / hz;   // only to shape the stream; the law computes its own
    const double lo = (std::max)(1.5, floorMs - 13.0), hi = 39.0;
    std::vector<In> v; v.reserve(steps);
    int left = 0; double base = g.U(floorMs * 0.4, floorMs * 1.1), jit = 0.0, pLate = 0.0;
    static const double kJit[] = { 0.0, 0.03, 0.12, 0.35 };
    static const double kLate[] = { 0.0, 0.0, 0.0, 0.002, 0.01, 0.05, 0.6 };
    for (int k = 1; k <= steps; k++)
    {
        if (left-- <= 0)
        {
            if (k > 1) base *= g.U(0.7, 1.4);
            base = (std::min)(hi, (std::max)(lo, base));
            left = 60 + (int)(g.Next() % 640);
            jit = kJit[g.Next() % 4]; pLate = kLate[g.Next() % 7];
        }
        In e = { base * (1.0 + g.U(-jit, jit)), 0.0, 0 };
        const double r = g.U();
        if (r < 0.0015)      e.ivMs = base * g.U(1.5, 3.0);   // a spike
        else if (r < 0.0018) e.ivMs = base * g.U(3.0, 6.0);   // a big one
        else if (r < 0.0033) e.ivMs = g.U(100.5, 170.0);      // a stall
        else if (r < 0.0053) e.ivMs = g.U(0.02, 0.19);        // a double present
        if (k > 150 && g.U() < 0.95) e.pad = g.U() < pLate ? 1 : 2;
        v.push_back(e);
    }
    return v;
}
// BY HAND: A STEP. 6 ms, then 22 ms for 700 presents, then 6 ms again: the period climbs in clamped steps of 1.5 ms after
// 6 presents and comes back in steps of 0.4 ms after 60, and the step's first presents are behind the clock.
static std::vector<In> GenStep()
{
    std::vector<In> v;
    for (int k = 1; k <= 2700; k++) { In e = { (k > 400 && k <= 1100) ? 22.0 : 6.0, 0.0, k > 150 ? 2 : 0 }; v.push_back(e); }
    return v;
}
// BY HAND: A RAMP. 4 ms to 45 ms over 2000 presents, then 45 ms: the period follows in unclamped moves to the 40 ms
// ceiling, the window becomes 120 ms, and at 45 ms every present is behind the clock. One 175 ms stall in the hold is a
// resync that only the 120 ms window makes (135 ms behind, under 4 steps of 40 ms).
static std::vector<In> GenRamp()
{
    std::vector<In> v;
    for (int k = 1; k <= 2600; k++)
    {
        In e = { k <= 2000 ? 4.0 + 41.0 * (double)(k - 1) / 1999.0 : 45.0, 0.0, k > 150 ? 2 : 0 };
        if (k == 2300) e.ivMs = 175.0;
        v.push_back(e);
    }
    return v;
}
// BY HAND: A SPIKE. 12 ms with single spikes: 50 ms (kept, a new worst), 130 ms (dropped, and a resync only the 4-step
// window makes), 0.1 ms (dropped), 85 ms (kept: the target reaches its 40 ms ceiling), three of 40 ms; and a burst of 20
// late pairs, which raises the pad to its maximum and holds it there, then clean runs that take it back. NOT A 12 ms STREAM
// THROUGHOUT (the verifier's nit 11): after the 85 ms spike the law asks for more than 19 ms to sleep for about a thousand
// presents, so change 3's guard lengthens those intervals and the stream runs near 21 ms there (its guard share is printed;
// a lengthened interval is the next sample, on both sides alike).
static std::vector<In> GenSpike()
{
    std::vector<In> v;
    for (int k = 1; k <= 2400; k++)
    {
        In e = { 12.0, 0.0, k > 150 ? 2 : 0 };
        if (k == 400) e.ivMs = 50.0;
        if (k == 700) e.ivMs = 130.0;
        if (k == 1000) e.ivMs = 0.1;
        if (k == 1300) e.ivMs = 85.0;
        if (k >= 1800 && k < 1803) e.ivMs = 40.0;
        if (k >= 500 && k < 520) e.pad = 1;
        v.push_back(e);
    }
    return v;
}
// BY HAND: THE BOUNDS, AT EXACT TICKS (the verifier's findings 1 and 5, 2026-09-26). The random streams never land ON a
// bound, so a law with a bound moved or made inclusive passed. All of it is inside the warm-up (140 presents), where the
// period stays at the floor, so the step is known to the tick; 8 ms presents sleep to their due, and a present after a
// sleep starts its interval at the due plus one tick (the post-sleep read):
//   20: step - 1 ticks, so now lands exactly ON the due: on time, no clamp (">=" at 11988 clamps it)
//   40: step - 501 ticks, so exactly 0.05 ms (500 ticks) is left: no sleep (">=" at 11989 sleeps)
//   60: step + resync - 1 ticks, so now is exactly the due plus the window: a clamp, not a resync (">=" at 11987)
//   80 and 120: samples of exactly 0.2 ms and exactly 100 ms, both dropped (11936's bounds made inclusive keep them)
//   90 and 100: samples of 0.22 ms and 99.5 ms, both kept (11936's bounds moved inward, 0.25 or 99, drop them)
// The coverage requires each exact case by its own branch, so a stream that no longer lands on them fails the proof.
static std::vector<In> GenBounds(int hz)
{
    const double floorMs = 2.0 * (1000.0 / (double)hz) * 1.015;     // as 11880-11883 compute it; the proof compares the floor
    const LONGLONG step = (LONGLONG)(floorMs * (double)kQpf / 1000.0);
    const LONGLONG resync = (step * 4 < kQpf * 12 / 100) ? step * 4 : kQpf * 12 / 100;
    std::vector<In> v;
    for (int k = 1; k <= 140; k++)
    {
        In e = { 8.0, 0.0, 0 };
        if (k == 20) e.ivT = step - 1;
        if (k == 40) e.ivT = step - 1 - kQpf / 20000;
        if (k == 60) e.ivT = step + resync - 1;
        if (k == 80) e.ivT = kQpf / 5000;          // 0.2 ms
        if (k == 90) e.ivT = 2200;                 // 0.22 ms
        if (k == 100) e.ivT = 995000;              // 99.5 ms
        if (k == 120) e.ivT = kQpf / 10;           // 100 ms
        v.push_back(e);
    }
    return v;
}
// CHANGE 1'S STREAM: 10 ms with jitter, and 0.2 to 1.5 ms inside fgOrigSwapPresent at every present.
static std::vector<In> GenExclusion()
{
    Rng g = { 0xE1C1u };
    std::vector<In> v;
    for (int k = 1; k <= 1500; k++)
    {
        In e = { 10.0 * (1.0 + g.U(-0.1, 0.1)), g.U(0.2, 1.5), 0 };
        if (k > 150) e.pad = g.U() < 0.05 ? 1 : 2;
        v.push_back(e);
    }
    return v;
}
// THE ENGAGE'S STREAM (the verifier's finding 4, 2026-09-26). Every other stream starts from one fresh engage, so the
// engage's resets wrote zeros over zeros and a law that kept avg, worst, the dwell or the due passed. Here FgPace stops and
// starts again four times, as the ABBA legs make it: after the warm-up with a short stop (2 presents), inside the next
// warm-up (2 presents), after a heavier stretch with a long stop (30 presents, 360 ms), and for a single present. While
// stopped, FgSwapPresent runs its stop line (paceRunning = false, prevPostSleep = Now) and the placer calls no pad; the
// last pad calls before each stop after a warm-up are clean ones, so the clean run is not 0 there.
static std::vector<In> GenEngage()
{
    Rng g = { 0xE6A6Eu };
    std::vector<In> v;
    int since = 0;
    for (int k = 1; k <= 2000; k++)
    {
        const bool off = (k >= 701 && k <= 702) || (k >= 760 && k <= 761) || (k >= 1401 && k <= 1430) || k == 1801;
        In e = { ((k > 1000 && k <= 1400) ? 20.0 : 12.0) * (1.0 + g.U(-0.05, 0.05)), 0.0, 0, 0, off ? 1 : 0 };
        const double r = g.U();
        since = off ? 0 : since + 1;
        if (since > 150) e.pad = r < 0.08 ? 1 : 2;               // the warm-up's pairs are neither late nor clean
        if (e.pad && ((k >= 690 && k <= 700) || (k >= 1390 && k <= 1400) || (k >= 1790 && k <= 1800))) e.pad = 2;
        v.push_back(e);
    }
    return v;
}
// CHANGE 3'S STREAMS. At 120 Hz: 30 ms until the period has followed it, then 4 ms, so more than 20 ms is left to sleep.
// At 60 Hz (the verifier's finding 7): a 5 ms game throughout. There the floor itself, 33.8 ms, is over the cap, so every
// sleep from the second present on is capped.
static std::vector<In> GenCap()
{
    std::vector<In> v;
    for (int k = 1; k <= 900; k++) { In e = { k <= 400 ? 30.0 : 4.0, 0.0, k > 150 ? 2 : 0 }; v.push_back(e); }
    return v;
}
static std::vector<In> GenCapLow()
{
    std::vector<In> v;
    for (int k = 1; k <= 1200; k++) { In e = { 5.0, 0.0, k > 150 ? 2 : 0 }; v.push_back(e); }
    return v;
}

// ------------------------------------------------------------------ one stream
enum { K_COMPARED = 0, K_EXCLUSION = 1, K_CAP = 2, K_ENGAGE = 3 };
static LONGLONG gTargets = 0, gSleepsEndedAtTarget = 0, gCapped = 0, gGuards = 0, gComparedSteps = 0;
static bool gExclLive = false, gExclSampleSame = false;
static int gCapStreams = 0, gCapCrossed = 0;
// THE ENGAGE, SEEN (finding 4): how many engages after the first, and at how many of them each field FgPace resets held
// something else than its reset value (a reset over the value it already had proves nothing), each field the engage keeps
// held something else than A5's forget would leave, and the kept due would have resynced (a long stop) or not (a short one).
// (round 9: the law's 4 s and 12 s memories and its own period in the places of A5's mean, worst and dwell; warm and inWarm the
// law's warm-up)
struct EngageSeen { int n, inWarm, lawFast, lawSlow, lawPer, period, prev, warm, due, pad, clean, moves, shortStop, longStop; LONG padKept; };
static EngageSeen gEng;
// WHAT CHANGE 3 DOES TO THE PACE, per cap stream (the verifier's finding 7): the law's outputs stay identical after a capped
// sleep, so only the achieved real interval shows it. Reported for the main session, never failed: whether to re-anchor
// the due after a capped sleep is its decision (impl-equiv.md, item 1).
struct CapReport { int hz; const char* label; double meanF, meanA, dueAheadF, dueAheadA; int n; };
static std::vector<CapReport> gCapReports;

static void RunStream(int idx, const char* label, int kind, int hz, const std::vector<In>& in)
{
    gHz = hz; FgFresh(); gClockF = 0; gTimerSets = 0;
    A5State A = A5Fresh(hz, &gLawA), Ax = A5Fresh(hz, &gLawAx);
    a5::ivtCfg.fg_cap = -2;
    LONGLONG PF = 0, PA = 0, PAx = 0;
    int guards = 0, mism = 0, firstMism = 0, expDiff = 0, firstExp = 0, dueSideDiff = 0, firstCap = 0, caps = 0, padMax = 0;
    int sampleSideMism = 0, capLenBad = 0, capIvN = 0, offSteps = 0;
    LONGLONG capIvF = 0, capIvA = 0, lastPF = 0, lastPA = 0;   // change 3's streams: the real intervals after the first cap
    LONG keptPad = 0; int keptClean = 0; UINT64 keptMoves = 0;  // the engage's stream: A5's values at the stop, before its forget
    const int steps = (int)in.size();
    for (int k = 1; k <= steps; k++)
    {
        gStep = k;
        const In& e = in[k - 1];
        const bool off = kind == K_ENGAGE && e.off;                         // FgPace does not run at this present
        const bool reengage = kind == K_ENGAGE && !off && k > 1 && !gS.r.paceRunning;
        // 1. THE PAD: the same call on every side, compared at once.
        if (e.pad)
        {
            const A5State a0 = A;
            A5Load(A); if (e.pad == 1) a5::FgCapLate(); else a5::FgCapOnTime(); A5Save(A);
            if (kind == K_EXCLUSION) { A5Load(Ax); if (e.pad == 1) a5::FgCapLate(); else a5::FgCapOnTime(); A5Save(Ax); }
            if (e.pad == 1) M::CapLate(); else M::CapOnTime();
            const Out oa = OutA(A, 0, 0), of = OutF(0, 0);
            if (!Same(oa, of, O_PAD, O_CLEAN + 1))
            {
                if (!mism++) { firstMism = k; Report("MISMATCH (a pad call)", idx, k, oa, of, O_PAD, O_CLEAN + 1); }
            }
            if (kind == K_COMPARED) ClassifyPad(e.pad, a0, A, idx, k);
            if (A.fgCapPadQms > padMax) padMax = A.fgCapPadQms;
        }
        // 2. THE TIME: the previous post-sleep read plus the interval (and, on change 1's stream, the time inside the
        //    original present, which A5's sample holds and FgPace's excludes).
        const LONGLONG ivT = e.ivT > 0 ? e.ivT : (LONGLONG)llround(e.ivMs * (double)kQpf / 1000.0), inT = (LONGLONG)llround(e.inOrigMs * (double)kQpf / 1000.0);
        LONGLONG nowF = kT0, nowA = kT0, nowAx = kT0;
        if (k > 1) { nowF = PF + ivT + inT; nowA = PA + ivT + inT; nowAx = PAx + ivT; }
        // CHANGE 3's GUARD: FgPace must not be left more than 19 ms to sleep outside the cap's own stream. The interval is
        // lengthened by the excess on every side alike, so the samples stay the same on every side.
        // Not while FgPace is stopped, nor at the present that starts it again: neither sleeps, and the due is stale there.
        if (kind != K_CAP && k > 1 && !off && gS.r.paceRunning && gS.r.due && gS.r.due - nowF > kGuard)
        {
            const LONGLONG shift = gS.r.due - nowF - kGuard;
            nowF += shift; nowA += shift; nowAx += shift; guards++;
        }
        // THE ENGAGE, BEFORE IT: what FgPace holds when it starts again, against what its engage resets it to and what A5's
        // forget would leave (the liveness of every reset and every named difference, required at the end).
        if (reengage)
        {
            const FgRenderLine& r = gS.r; const FgLaw& lw = *gS.law; const double fl = M::CPeriodMs();
            const LONGLONG st = (LONGLONG)(fl * (double)kQpf / 1000.0), rw = (st * 4 < kQpf * 12 / 100) ? st * 4 : kQpf * 12 / 100;
            gEng.n++;
            if (lw.warm > 0 && lw.warm < 150) gEng.inWarm++;
            if (lw.fast.n != 0) gEng.lawFast++;
            if (lw.slow.n != 0) gEng.lawSlow++;
            if (Bits(lw.per) != Bits(fl)) gEng.lawPer++;
            if (Bits(r.period) != Bits(fl)) gEng.period++;
            if (Bits(r.prevPeriod) != Bits(fl)) gEng.prev++;
            if (lw.warm != 150) gEng.warm++;
            if (r.due != 0) gEng.due++;
            if (gS.sh.padQms != 0) { gEng.pad++; gEng.padKept = gS.sh.padQms; }
            if (gS.pl.capClean != 0) gEng.clean++;
            if (r.moves != 0) gEng.moves++;
            if (nowF > r.due + rw) gEng.longStop++; else gEng.shortStop++;   // would the kept due have resynced by itself
        }
        // 3. FGPACE: the header's law; or, while it is stopped, its caller's stop line (fg_machine.h, FgSwapPresent's step 7:
        //    "else { r.paceRunning = false; r.prevPostSleep = B::Now(); }", which check_equiv_fgpace_quote.py finds there).
        const LONGLONG dueBefore = gS.r.due;
        gClockF = nowF;
        gS.r.paceSlept = 0;                            // as its caller, FgSwapPresent, clears it before every Pace (step 7)
        gS.r.lastEntry = nowF - inT;                   // (round 9: the frame's entry, which FgSwapPresent sets and PaceLaw hands the law)
        if (off) { gS.r.paceRunning = false; gS.r.prevPostSleep = EqBinding::Now(); offSteps++; }
        else M::Pace(inT);
        const int sleptF = gS.r.paceSlept;
        PF = gS.r.prevPostSleep;
        if (gClockF != PF + 1) { Instrument("FgPace read its clock after its post-sleep read (the fake's accounting is off)", idx, k); }
        const LONGLONG targetF = sleptF ? dueBefore : 0;
        if (sleptF)
        {
            gTargets++;
            const LONGLONG end = PF - 1;               // the read that ended the header's spin
            if (end == targetF) gSleepsEndedAtTarget++;
            else if (end < targetF)
            {
                caps++; gCapped++;
                if (kind == K_CAP)
                {
                    // THE CAP'S LENGTH (the verifier's finding 2): 20 ms exactly, counted from SleepUntil's first read, the
                    // one after Pace's now (the fake clock makes it exact). The 20 ms is C-F30's, under the device thread's
                    // 25 ms settle; nothing else in the gate holds it (dt_fg_test's H11 sleeps 8 ms).
                    if (end - (nowF + 1) != kQpf / 50)
                    {
                        if (!capLenBad++)
                            printf("    MISMATCH stream %d step %d: change 3's capped sleep lasted %lld ticks, not %lld (20 ms: fg_machine.h SleepUntil's cap, C-F30)\n",
                                   idx, k, end - (nowF + 1), kQpf / 50);
                        if (!mism++) firstMism = k;
                    }
                    if (!firstCap)
                    {
                        firstCap = k;
                        printf("    change 3 at step %d: the sleep ended %.3f ms after it began, %.3f ms before its target (A5 sleeps to the target)\n",
                               k, (double)(end - (nowF + 1)) * 1000.0 / kQpf, (double)(targetF - end) * 1000.0 / kQpf);
                    }
                }
                else Instrument("a sleep ended before its target: this stream crossed the 20 ms cap (the guard failed)", idx, k);
            }
            else Instrument("a sleep ended after its target (the fake timer is late)", idx, k);
        }
        // 4. A5: the quoted law, on its own clock.
        //    THE ENGAGE'S STREAM, MAPPED ONTO A5 (provisional: which A5 event an engage is, is the main session's decision,
        //    impl-equiv.md item 2). A stop is A5's live switch to fg_cap=0 (8555 runs FgCapForget), after which A5's presents
        //    still sample the clock (12000, 12015) and never pace. A start is the switch back (FgCapForget again) with the
        //    warm-up re-armed as a Reset re-armed it (11720). THREE NAMED DIFFERENCES are then applied to A5's side, each a
        //    choice FgPace's engage made that A5's forget did not: the pad and the clean run are KEPT (A5 zeroed them), the
        //    moves counter is KEPT (A5 zeroed it), and the due is ZEROED (A5 kept it). Everything else must then be identical.
        if (off && a5::ivtCfg.fg_cap == -2)
        {
            keptPad = A.fgCapPadQms; keptClean = A.fgCapClean; keptMoves = A.fgCapMoves;
            A5Load(A); a5::ivtCfg.fg_cap = 0; a5::FgCapForget(); A5Save(A);
        }
        if (reengage)
        {
            A5Load(A); a5::ivtCfg.fg_cap = -2; a5::FgCapForget(); a5::FgWarmRearm(); A5Save(A);
            A.fgCapPadQms = keptPad; A.fgCapClean = keptClean;           // NAMED DIFFERENCE 1: the pad and the clean run kept
            A.fgCapMoves = keptMoves;                                     // NAMED DIFFERENCE 2: the moves counter kept
            A.fgCapDue = 0;                                               // NAMED DIFFERENCE 3: the due zeroed
            A.lawFresh = true;                                            // (round 9: the law starts again where FgPace does, Pace's FgLawReset)
        }
        A.entry = nowA - inT; Ax.entry = nowAx;                           // (round 9: the frame's entry, the law's time, on each side's clock)
        const LawSnap l0 = { A.lawFresh ? 0.0 : A.law->per, A.lawFresh ? 150 : A.law->warm };
        const A5State a0 = A;
        A5Load(A); a5::clk = nowA; a5::waitTarget = 0; const UINT64 w0 = a5::fgCapWaits; a5::Present(); A5Save(A);
        const int sleptA = (int)(A.fgCapWaits - w0);
        PA = A.fgPrevPairQpc;
        if (kind == K_EXCLUSION)
        {
            A5Load(Ax); a5::clk = nowAx; a5::waitTarget = 0; const UINT64 wx = a5::fgCapWaits; a5::Present(); A5Save(Ax);
            PAx = Ax.fgPrevPairQpc;
            const Out ox = OutA(Ax, (int)(Ax.fgCapWaits - wx), (Ax.fgCapWaits - wx) ? Ax.waitTarget : 0), of = OutF(sleptF, targetF);
            if (!Same(ox, of, 0, kSampleSide))
            {
                if (!sampleSideMism++) Report("MISMATCH (change 1: FgPace against A5 fed the kept sample, the sample's side)", idx, k, ox, of, 0, kSampleSide);
            }
            // THE DUE'S SIDE TOO (the verifier's finding 3). Every present of this stream after the first sleeps, and a sleep
            // ends at the due whatever the clock read before it, so the excluded time cannot reach the due: identical at
            // every step, or the excluded time leaked into the due (the due's clock read as now - tInOrig, or the due
            // advanced by it). The premise is checked, not assumed: a present after the first that did not sleep fails.
            if (!Same(ox, of, kSampleSide, O_N))
            {
                if (!dueSideDiff++) Report("MISMATCH (change 1: FgPace against A5 fed the kept sample, the due's side)", idx, k, ox, of, kSampleSide, O_N);
            }
            if (k > 1 && !sleptF) Instrument("a present of change 1's stream did not sleep, so the excluded time was not absorbed (the stream's premise)", idx, k);
        }
        if (k == 1 && Bits(A.fgCapFloorMs) != Bits(M::CPeriodMs()))
        {
            printf("    MISMATCH stream %d step 1: the floor, A5 %.17g (fg-last DLSS-IV.cpp:11880-11883) FgPace %.17g (fg_machine.h CPeriodMs)\n",
                   idx, A.fgCapFloorMs, M::CPeriodMs());
            mism++;
        }
        // 5. COMPARE.
        const Out oa = OutA(A, sleptA, sleptA ? A.waitTarget : 0), of = OutF(sleptF, targetF);
        const bool same = Same(oa, of, 0, O_N);
        if (kind == K_COMPARED)
        {
            if (!same && !mism++) { firstMism = k; Report("MISMATCH", idx, k, oa, of, 0, O_N); }
            if (same && PF != PA) Instrument("the two sides agree but their sleeps ended apart (the fakes disagree)", idx, k);
            ClassifyStep(a0, A, l0, nowA, idx, k);
        }
        else if (kind == K_EXCLUSION)
        {
            if (!same && !expDiff++) { firstExp = k; Report("expected (change 1: A5 on the same timeline takes the excluded time into its sample)", idx, k, oa, of, 0, O_N); }
        }
        else if (kind == K_ENGAGE)
        {
            // Stopped, neither law runs: only where the next sample starts is compared. Running, everything is.
            if (off)
            {
                if (PF != PA && !mism++)
                {
                    firstMism = k;
                    printf("    MISMATCH stream %d step %d: FgPace stopped, and the next sample starts at %lld for FgPace, %lld for A5\n", idx, k, PF, PA);
                }
            }
            else
            {
                if (!same && !mism++) { firstMism = k; Report(reengage ? "MISMATCH (the engage's stream, at a start)" : "MISMATCH (the engage's stream)", idx, k, oa, of, 0, O_N); }
                if (same && PF != PA) Instrument("the two sides agree but their sleeps ended apart (the fakes disagree)", idx, k);
            }
        }
        else
        {
            if (!same)
            {
                // THE CAPPED STEP ITSELF IS AFTER THE CAP (2026-09-26, the main session's decision on change 3): its due restarts
                // from the cap's end (no banking), so from that step on the due differs from A5's, who slept to the target.
                if (!firstCap || k < firstCap) { if (!mism++) { firstMism = k; Report("MISMATCH (before the cap)", idx, k, oa, of, 0, O_N); } }
                else if (!expDiff++) { firstExp = k; Report("expected (change 3: after a capped sleep)", idx, k, oa, of, 0, O_N); }
            }
            // THE ACHIEVED REAL INTERVAL, post-sleep read to post-sleep read, on each side (finding 7).
            if (firstCap && k > firstCap) { capIvF += PF - lastPF; capIvA += PA - lastPA; capIvN++; }
        }
        lastPF = PF; lastPA = PA;
    }
    gMismatch += mism + sampleSideMism + dueSideDiff;
    if (kind == K_COMPARED) { gGuards += guards; gComparedSteps += steps; }
    char verdict[64];
    if (mism + sampleSideMism + dueSideDiff) sprintf_s(verdict, "DIFFERENT %d times, first at step %d", mism + sampleSideMism + dueSideDiff, firstMism);
    else sprintf_s(verdict, "identical");
    if (kind == K_COMPARED)
        printf("  stream %2d  %-34s %3d Hz %4d steps: %s; moves %llu, resyncs %llu, clamps %llu, sleeps %llu, pad max %d, guard %d (%.0f%%)\n",
               idx, label, hz, steps, verdict, A.fgCapMoves, A.fgCapResyncs, A.fgCapClamps, A.fgCapWaits, padMax, guards, 100.0 * guards / steps);
    else if (kind == K_ENGAGE)
        printf("  stream %2d  %-34s %3d Hz %4d steps: %s at every present FgPace ran, with the named differences applied to A5 at each start; "
               "%d presents stopped (the next sample's start compared); moves %llu, resyncs %llu, clamps %llu, sleeps %llu, pad max %d, guard %d\n",
               idx, label, hz, steps, verdict, offSteps, A.fgCapMoves, A.fgCapResyncs, A.fgCapClamps, A.fgCapWaits, padMax, guards);
    else if (kind == K_EXCLUSION)
    {
        gExclLive = expDiff > 0; gExclSampleSame = sampleSideMism == 0 && dueSideDiff == 0;
        printf("  stream %2d  %-34s %3d Hz %4d steps: change 1. Against A5 on the same timeline: %d steps differ, from step %d (expected). "
               "Against A5 fed the kept sample: the sample's side %s at every step, the due's side %s (every present after the first sleeps, which absorbs the excluded time); guard %d\n",
               idx, label, hz, steps, expDiff, firstExp, sampleSideMism ? "DIFFERS" : "identical", dueSideDiff ? "DIFFERS" : "identical too", guards);
    }
    else
    {
        // WHAT THE CAP LEAVES BEHIND: the due does not read when the sleep ended, so a capped sleep's remainder stays in it.
        gCapStreams++; if (firstCap) gCapCrossed++;
        const double dueF = (double)(gS.r.due - gClockF) * 1000.0 / kQpf, dueA = (double)(A.fgCapDue - A.clk) * 1000.0 / kQpf;
        const double ivF = capIvN ? (double)capIvF * 1000.0 / kQpf / capIvN : 0.0, ivA = capIvN ? (double)capIvA * 1000.0 / kQpf / capIvN : 0.0;
        printf("  stream %2d  %-34s %3d Hz %4d steps: change 3. The law's outputs %s up to the first capped sleep (step %d); after it %d steps differ%s. "
               "Capped sleeps %d, each %s. After the first, a real frame every %.2f ms on average for FgPace, %.2f ms for A5. "
               "At the end the due is %.1f ms ahead of the clock for FgPace, %.1f ms for A5\n",
               idx, label, hz, steps, mism ? "DIFFER" : "are identical", firstCap, expDiff, expDiff ? " (expected)" : "",
               caps, capLenBad ? "NOT 20 ms" : "exactly 20 ms", ivF, ivA, dueF, dueA);
        CapReport cr = { hz, label, ivF, ivA, dueF, dueA, capIvN };
        gCapReports.push_back(cr);
    }
}

// A5 REFUSED TO PACE BELOW 48 Hz (fg-last 11868-11879, quoted in FgCapRealFps above: "the game runs free"); FgPace has no
// such refusal (it stops only at a floor of 0.1 ms or less), and neither has the rig's refusal list (fg_rig.h, FgRigEligible).
// The verifier's finding 8. Not compared, SHOWN: both sides on one stream at a low rate, a 10 ms game. If a side stops
// doing what this says, the named difference has changed and the proof fails until its text is brought up to date.
static void ShowLowRefresh(int hz)
{
    gHz = hz; FgFresh(); gClockF = 0;
    A5State A = A5Fresh(hz, &gLawA);
    a5::ivtCfg.fg_cap = -2;
    LONGLONG PF = 0, PA = 0;
    int sleptF = 0;
    const int steps = 300;
    for (int k = 1; k <= steps; k++)
    {
        const LONGLONG nowF = k > 1 ? PF + kQpf / 100 : kT0, nowA = k > 1 ? PA + kQpf / 100 : kT0;
        gStep = k; gClockF = nowF; gS.r.paceSlept = 0; gS.r.lastEntry = nowF;
        M::Pace(0);
        sleptF += gS.r.paceSlept; PF = gS.r.prevPostSleep;
        A.entry = nowA;
        A5Load(A); a5::clk = nowA; a5::Present(); A5Save(A);
        PA = A.fgPrevPairQpc;
    }
    const bool a5Refused = A.fgCapFps == 0 && A.fgCapWaits == 0 && A.fgCapResyncs == 0;
    const bool fgPaced = sleptF > 0 && gS.r.period > 0.1;
    printf("  below 48 Hz, at %d Hz with a 10 ms game, %d presents: A5 %s; FgPace %s\n", hz, steps,
           a5Refused ? "refused to pace (no cap derived, no sleep: the game ran free)" : "PACED",
           fgPaced ? "paced from its floor" : "DID NOT PACE");
    if (!a5Refused || !fgPaced)
    {
        printf("    FAIL: the named difference below 48 Hz is not what this proof says (A5 refuses, FgPace paces): bring its text up to date\n");
        gFail2++;
        return;
    }
    gNamed++;
    printf("  NAMED (below 48 Hz, decided 2026-09-26): A5 refused to pace below 48 Hz (fg-last 11868-11879); FgPace's law paces there,"
           " from a floor of %.1f ms at %d Hz (%d of %d presents slept), and the rig refuses such a mode before FgPace can run (FgRigEligible:"
           " C under 48 fps, the plan's decision 9)\n", M::CPeriodMs(), hz, sleptF, steps);
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    const DWORD t0 = GetTickCount();
    printf("equiv_fgpace: FgPace (source/fg/fg_machine_render.h, FgMachine<EqBinding>::Pace and PaceLaw, CapLate, CapOnTime) against A5's\n"
           "  FgDelayPresent (tag frame-generation-last, commit ad1bf56d, DLSS-IV.cpp, quoted verbatim) with the pacing law (fg_law.h) in its\n"
           "  target and move's place, joined as PaceLaw joins it (round 9), on the same sample streams\n");
#if EQUIV_FGPACE_CONTROL == 1
    printf("  CONTROL 1: the reference hands the law the sample uncut (PaceLaw cuts it to whole microseconds). This build MUST FAIL at the comparison.\n");
#elif EQUIV_FGPACE_CONTROL == 2
    printf("  CONTROL 2: the header included is gen_equiv_fgpace_ctl.py's copy, PaceLaw's pad weight changed from 0.25 to 0.26 ms. This build MUST FAIL at the comparison.\n");
#else
    printf("  control: none (the real header, A5's lines as quoted, the law's joins as PaceLaw's rules say)\n");
#endif
    printf("  compared after every step:");
    for (int i = 0; i < O_N; i++) printf("%s %s", i ? "," : "", kField[i].name);
    printf("; the pad and the clean run after every pad call; the floor at the start\n");

    // THE 20 RANDOM STREAMS: seeds 1001 to 1020, each at one of six refresh rates.
    static const int kHz[] = { 60, 100, 120, 144, 165, 240 };
    int idx = 0;
    for (int i = 1; i <= 20; i++)
    {
        const unsigned long long seed = 1000 + i; const int hz = kHz[i % 6];
        char label[64]; sprintf_s(label, "random, seed %llu", seed);
        RunStream(++idx, label, K_COMPARED, hz, GenRandom(seed, hz, 3000));
    }
    RunStream(++idx, "by hand: a step (6, 22, 6 ms)", K_COMPARED, 120, GenStep());
    RunStream(++idx, "by hand: a ramp (4 to 45 ms)", K_COMPARED, 120, GenRamp());
    RunStream(++idx, "by hand: a spike (12 ms, spikes)", K_COMPARED, 120, GenSpike());
    RunStream(++idx, "by hand: the bounds, at exact ticks", K_COMPARED, 120, GenBounds(120));
    const int nCompared = idx;
    RunStream(++idx, "change 1: 0.2-1.5 ms in the present", K_EXCLUSION, 120, GenExclusion());
    RunStream(++idx, "the engage: FgPace stops and starts", K_ENGAGE, 120, GenEngage());
    RunStream(++idx, "change 3: 30 ms, then 4 ms", K_CAP, 120, GenCap());
    RunStream(++idx, "change 3 at 60 Hz: a 5 ms game", K_CAP, 60, GenCapLow());
    ShowLowRefresh(30);

    // THE THREE CHANGES, each said.
    printf("  change 1, the sample excludes the time inside fgOrigSwapPresent: %s\n",
           gExclLive && gExclSampleSame ? "handled (compared streams have none; its stream shows the effect is the sample and only the sample)"
                                        : "NOT SHOWN");
    printf("  change 2, the sleep primitive: %lld sleep targets compared, never the primitive; %lld of the fake sleeps ended exactly at"
           " the target, %lld were change 3's capped ones\n", gTargets, gSleepsEndedAtTarget, gCapped);
    if (gSleepsEndedAtTarget + gCapped != gTargets) { printf("    FAIL: %lld sleeps ended neither at the target nor at the cap\n", gTargets - gSleepsEndedAtTarget - gCapped); gFail2++; }
    printf("  change 3, the 20 ms cap: %s; the guard lengthened %lld of the compared streams' %lld intervals\n",
           gCapCrossed == gCapStreams ? "handled (compared streams stay under it; its streams cross it)" : "NOT CROSSED by every one of its streams", gGuards, gComparedSteps);
    if (!gExclLive || !gExclSampleSame) { printf("    FAIL: change 1's stream did not show its effect as expected\n"); gFail2++; }
    if (gCapCrossed != gCapStreams || gCapStreams < 2) { printf("    FAIL: %d of change 3's %d streams crossed the cap\n", gCapCrossed, gCapStreams); gFail2++; }
    // THE ENGAGE (finding 4): every reset seen over a value it changes, every named difference seen changing something, and
    // both kinds of stop; otherwise the engage's stream proved less than it says.
    printf("  the engage: %d starts after the first (%d inside a warm-up; %d short stops, %d long). Before a start, and so reset by it: the law's 4 s memory %d, its 12 s memory %d,"
           " its own period off the floor %d, period off the floor %d, previous period off the floor %d, the law's warm-up not 150 %d, due %d times. Kept by it: pad %d, clean run %d, moves %d times\n",
           gEng.n, gEng.inWarm, gEng.shortStop, gEng.longStop, gEng.lawFast, gEng.lawSlow, gEng.lawPer, gEng.period, gEng.prev, gEng.warm, gEng.due, gEng.pad, gEng.clean, gEng.moves);
    if (!gEng.n || !gEng.inWarm || !gEng.shortStop || !gEng.longStop || !gEng.lawFast || !gEng.lawSlow || !gEng.lawPer || !gEng.period || !gEng.prev
        || !gEng.warm || !gEng.due || !gEng.pad || !gEng.clean || !gEng.moves)
    { printf("    FAIL: the engage's stream left a reset, a named difference or a kind of stop unexercised (a 0 above)\n"); gFail2++; }
    else
    {
        gNamed++;
        printf("  NAMED (the engage, decided 2026-09-26: the pad and the clean run are the placer's, measured over the whole leg): FgPace's engage keeps the pad (%ld quarter ms here) and the clean run"
               " and the moves counter, and zeroes the due; A5's forget (11846-11852) zeroed the first three and kept the due. The proof maps a stop to A5's"
               " fg_cap switch to 0 and a start to the switch back with the warm-up re-armed (11720), and applies these three as named differences\n", gEng.padKept);
    }
    // CHANGE 3, DECIDED 2026-09-26 (the verifier's finding 7): a capped sleep used to leave its remainder in the due, so the due
    // ran seconds ahead and every present slept 20 ms whatever the period became. The due now restarts from the cap's end (no
    // banking): a debt past one period is a FAILURE; the interval the cap forces while the law asks for more is NAMED. Below
    // about 101 Hz the floor alone is over 20 ms, and the rig refuses panels whose C is under 48 fps.
    for (const CapReport& c : gCapReports)
    {
        if (c.n <= 0) { printf("    FAIL: change 3's %d Hz stream measured no real interval after its first capped sleep\n", c.hz); gFail2++; continue; }
        if (c.dueAheadF > c.dueAheadA + 20.0)
        { printf("    FAIL: change 3 at %d Hz: FgPace's due ended %.1f s ahead of its clock: a capped sleep's remainder is banked again\n", c.hz, c.dueAheadF / 1000.0); gFail2++; continue; }
        gNamed++;
        printf("  NAMED (change 3, decided 2026-09-26): at %d Hz (%s) the cap binds: FgPace's real frames came every %.2f ms, A5's every %.2f ms;"
               " FgPace's due ended %.1f s ahead of its clock (no banking)\n", c.hz, c.label, c.meanF, c.meanA, c.dueAheadF / 1000.0);
    }
    if (gTargets == 0) { printf("    FAIL: no sleep target was compared\n"); gFail2++; }

    // WHAT IS NOT COMPARED (the verifier's finding 9): the verdict covers the machine's arithmetic around the law on the streams
    // above, and no more. Each of these differs from A5 or may, by construction, and is said rather than compared.
    printf("  not compared, by construction:\n"
           "    the floor's refresh and LOCK: A5 took an integer Hz from the display, FgPace the rational of the refresh read times FG_LOCK_PM"
           " (map 2.11 item 2); every stream here is num/1, where the floors are bit-identical (checked); at 119.88 Hz they differ\n"
           "    the law's own arithmetic: both sides call fg_law.h's FgLawStepForm (equiv_fglaw holds it against its oracle)\n"
           "    who may sleep: PaceOwner answers FgPace (no limiter, no FusionFix cap); FusionFix's ownership and the ambiguous band are not exercised\n"
           "    the counters: A5's followed the log level; here they always count\n"
           "    the sleep itself (change 2): timer lateness and placer wakes; dt_fg_test's H11 runs the real sleep loop\n"
           "    the merged path's joins (the room floor and the switch's floor in the pad's place): dt_fg_test's T4, RP and PL cases\n"
           "    the mod's own compile of the law: this proof builds 32-bit with /O2 and the default floating point, as build.ps1 does\n");

    // THE COVERAGE, over the compared streams.
    printf("  branches of A5's lines and the law's joins taken, in steps, over the %d compared streams:\n", nCompared);
    int missing = 0;
    for (int b = 0; b < BR_N; b++)
    {
        printf("    %-62s %8lld%s\n", kBr[b], gBr[b], gBr[b] ? "" : "   NEVER TAKEN");
        if (!gBr[b]) missing++;
    }
    if (missing) { printf("    FAIL: %d branches never taken; a branch never taken is not proved\n", missing); gFail2++; }
    printf("  %.1f s\n", (GetTickCount() - t0) / 1000.0);
    // THE REPORTED COUNT, on its own line above the verdict, so prove-all can show it on the step's line.
    printf("  named (decided): %d (each on a NAMED line above)\n", gNamed);
    printf("  reported for the main session: %d (each on a REPORTED line above; none of them fails this proof)\n", gReported);

    if (gMismatch)
    {
        // THE VERDICT LINE SAYS WHICH BUILD FAILED: prove-all reads the last lines only, and a control must fail HERE.
        if (EQUIV_FGPACE_CONTROL)
            printf("EQUIV_FGPACE FAIL: %d unexpected differences between FgPace and A5 (MISMATCH above); control %d failed at the comparison, as it must\n",
                   gMismatch, EQUIV_FGPACE_CONTROL);
        else
            printf("EQUIV_FGPACE FAIL: %d unexpected differences between FgPace and A5 (MISMATCH above)\n", gMismatch);
        return 1;
    }
    if (gFail2) { printf("EQUIV_FGPACE FAIL: %d failures of the proof itself (coverage, the named changes, the fakes)\n", gFail2); return 2; }
    printf("EQUIV_FGPACE PASS\n");
    return 0;
}
