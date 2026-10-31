// dt_fg_threads.h: THE PRODUCT'S THREAD LOOPS, RUN IN THE HARNESS (2026-10-10, phase 3 of the revival, the build map's stage 8;
// docs/reports/fg-3/map-2026-10-06/check-machine.md, R12; lifecycle-safety.md, LS-20; docs/reports/fg-3/merge-2026-10-10/
// READY-FOR-11.md).
//
// Included by dt_fg_test.cpp after dt_fg_merged.h, with ONE line; its case is TL1 (one RUN line in RunCases, twice: the
// two orders of 'Q').
//
// WHAT IS NEW. Until this stage every case that ran "the placer and the stampers as threads" ran loops the harness wrote
// for itself (PlacerMain and StamperMain in dt_fg_test.cpp, ShStamperMain in dt_fg_shadow.h): stand-ins for the
// product's. The checkers said so (R12): "the placer has a timer", "a stamper idles on a dead helper's fence" and "a
// stamper keeps running across the disengage edge" were proved on the stand-ins. source/fg/fg_threads.h now holds the
// loops ONCE, over the machine's binding and a second small binding for what a thread waits on, and the product's
// binding that arms starts its threads on them (source/fg/fg_arm_life.h). HERE THE VERY SAME LOOPS RUN OVER THE FAKES:
// the merged machine (FgMachine<ShArmB>), the fake of DXVK, the fake helper, and waits of the harness's own (ThS).
// The cases that were there keep their stand-ins: moving them over is not this stage's.
//
// THE COMPILE SHAPE (the build map's pgk-14, for the loops): fg_threads.h instantiated over the product's shape that has
// no seam but the contract's (FgArmedShape; the shadow shape's FgProductShape went in round 9's strip) and over waits that
// do nothing. If a loop asked the machine's binding for anything the contract does not list, this would not compile.
#pragma once
#include "../../source/fg/fg_threads.h"

struct FgThrShape
{
    static bool Quit() { return true; }
    static void PlacerWait() {}
    static void PlacerTimer(LONGLONG at) { (void)at; }
    static void PlacerRan() {}
    static void RunWait() {}
    static void GoneWait() {}
    static LONG StampFrom(int tl) { (void)tl; return 0; }
    static void StamperUp(int tl) { (void)tl; }
    static void WakePlacer() {}
};
static void* const kThreadsShapeEntries[] = {
    (void*)&FgThreadPlacer<FgArmedShape, FgThrShape>, (void*)&FgThreadStamper<FgArmedShape, FgThrShape>,
    (void*)&FgThreadsRun<FgArmedShape> };

// ------------------------------------------------------------------ what the loops wait on, in the harness
// The placer's event is the fake's own (B::evPlacer: the machine's B::WakePlacer sets it at every publication, as the
// mod's does). Its timer is a real one, set from the harness's clock: a raise that is 2 ms away on that clock fires 2 ms
// later on the wall, and a pass that finds the harness's clock has not moved yet sets it again. noTimer: A CONTROL'S
// placer, whose timer is never set (the shadow shape's loop, which is right for a binding that times nothing).
struct ThS
{
    static inline HANDLE timer = nullptr, runEv = nullptr;
    static inline volatile LONG up = 0, passes = 0, inWait = 0, noTimer = 0, timersSet = 0;
    static bool Quit() { return gThrQuit != 0; }
    static void PlacerWait()
    {
        const HANDLE h[3] = { B::evPlacer, gThrQuitEv, timer };
        _InterlockedExchange(&inWait, 1);
        WaitForMultipleObjects(3, h, FALSE, INFINITE);
        _InterlockedExchange(&inWait, 0);
    }
    static void PlacerTimer(LONGLONG at)
    {
        if (noTimer) return;
        _InterlockedIncrement(&timersSet);
        LONGLONG left = at - B::Now(); if (left < 0) left = 0;
        LARGE_INTEGER due; due.QuadPart = -(LONGLONG)((double)left * 1e7 / (double)B::Qpf()); if (due.QuadPart == 0) due.QuadPart = -1;
        SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
    }
    static void PlacerRan() { _InterlockedIncrement(&passes); }
    static void RunWait()  { const HANDLE h[2] = { runEv, gThrQuitEv }; WaitForMultipleObjects(2, h, FALSE, INFINITE); }
    static void GoneWait() { WaitForSingleObject(gThrQuitEv, 20); }
    static LONG StampFrom(int tl) { (void)tl; return 0; }         // (Fresh leaves every timeline at 0: a thread that starts late skips nothing)
    static void StamperUp(int tl) { (void)tl; _InterlockedIncrement(&up); }
    static void WakePlacer() { SetEvent(B::evPlacer); }
};
static DWORD WINAPI ThPlacerMain(void*) { return FgThreadPlacer<ShArmB, ThS>(); }
static DWORD WINAPI ThStamperMain(void* a) { return FgThreadStamper<ShArmB, ThS>((int)(INT_PTR)a); }
// The product's five threads over the merged machine: four stampers and the placer, on fg_threads.h's loops. run: the
// stampers' run event as the driver would have it (set: engaged).
static void ThStart(bool run = true)
{
    if (!ThS::timer)
    {
        ThS::timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (!ThS::timer) ThS::timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    }
    if (!ThS::runEv) ThS::runEv = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    gThrQuit = 0; ResetEvent(gThrQuitEv); gThrN = 0; ThS::up = 0; ThS::passes = 0;
    if (run) SetEvent(ThS::runEv); else ResetEvent(ThS::runEv);
    static const int tls[4] = { FG_TL_IN, FG_TL_PREP, FG_TL_OUT, FG_TL_END };
    for (int tl : tls) gThr[gThrN++] = Spawn(ThStamperMain, (void*)(INT_PTR)tl);
    gThr[gThrN++] = Spawn(ThPlacerMain);
    if (!WaitFor([] { return ThS::up == 4; }, 5000)) Err("the shared loops' four stampers did not start within 5 s");
}
// The placer's thread has gone back to its wait and makes no more passes: whatever woke it has been answered.
static bool ThQuiet()
{
    for (int i = 0; i < 300; i++)
    {
        const LONG p0 = ThS::passes;
        Sleep(3);
        if (ThS::passes == p0 && ThS::inWait) return true;
    }
    return false;
}

// ---------------------------------------------------------------- TL1, the product's thread loops over the fakes
// WHAT IT SHOWS, in three legs.
// 1. THE RUN. P4's thirty pairs of an even run, with the placer and the four stampers on THE SHARED LOOPS, and with
//    nothing in the harness waking the placer: P4 sets the placer's event itself at every step of its clock; here the
//    harness only moves its clock. A pair is timed because its publication or its handoff's stamper woke the placer,
//    and its two dues are raised because THE PLACER'S OWN TIMER fired. Every generated frame is shown and then its
//    real frame, and the frame ends are stamped by the fourth stamper.
//    CONTROL: the same loop with a timer that is never set (what the shadow shape's loop has, rightly: it times
//    nothing). Once the wakes of the frame itself have been answered, no due is raised, no generated frame is shown,
//    and every real frame goes out forced, at the next injection.
// 2. THE DISENGAGE EDGE (LS-20). The driver asks fg_threads.h's rule with the want it is about to write, PASS, and the
//    rule says "the stampers keep running": the last engaged pair, armed after that, has its handoff stamped and its
//    frame end stamped. A whole window of standby later, with no pair open, the rule says "rest", and five standby
//    presents add no stamp.
//    CONTROL: the stampers rested AT the edge (the shadow driver's order). The last pair's handoff has no stamp, the
//    placer times it by its own clock, and its frame end has no stamp.
// 3. THE RULE BY ITSELF, at its five answers.
struct T1Leg { int gen, real, order; LONG iShow, nRaised, forced, ends, timers; };
static T1Leg T1Run(bool timer)
{
    T1Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000;
    ShWarm<MA>(); MgDueN();
    ThS::noTimer = timer ? 0 : 1; ThS::timersSet = 0;
    ThStart();
    MgEvenStart();
    const size_t sh = MgShownN();
    const LONG i0 = a.pl.iShow, n0 = a.pl.nRaised, f0 = a.w.iWhy[FG_AW_FORCED];
    std::vector<DWORD> tags; std::vector<LONG> ends;
    for (int i = 0; i < 30; i++)
    {
        const LONG timed0 = a.pl.nTimed;
        const DWORD t = MgEven<MA>(); tags.push_back(t); ends.push_back(B::frames - 1);
        const LONG p = MgP(); const LONGLONG t0 = B::clockNow;
        FgDue d = {};
        // (timed by the placer's thread while the clock still stands at the handoff, AND NOBODY SETS ITS EVENT HERE: the
        // pair's publication does, and its handoff's stamper)
        if (!WaitFor([&] { return a.pl.nTimed > timed0; }, 3000)) { Err("TL1: the placer's thread did not time pair %ld by its own wakes", p); break; }
        if (!timer)
        {   // the control must not be decided by a late wake: the answer's stamp is in and the placer is back in its wait
            FgArmedPair rec = {}; LONGLONG ts = 0;
            MA::Read(a.pair[p & (FG_RING - 1)], p, &rec);
            WaitFor([&] { return MA::ReadStamp(s.st[FG_TL_OUT], 2 * rec.n, &ts); }, 1000);
            if (!ThQuiet()) { Err("TL1 control: the placer's thread did not come to rest behind pair %ld", p); break; }
        }
        B::clockNow = t0 + MA::UsToTicks(2100);
        if (timer) { if (!WaitFor([&] { return MgDue(false, p, &d); }, 3000)) { Err("TL1: the placer's own timer raised no due for pair %ld's generated frame", p); break; } }
        else Sleep(6);
        FireNote(); Drain();
        B::clockNow = t0 + MA::UsToTicks(2100 + kMgPeriodUs / 2);
        if (timer) { if (!WaitFor([&] { return MgDue(true, p, &d); }, 3000)) { Err("TL1: the placer's own timer raised no due for pair %ld's real frame", p); break; } }
        else Sleep(12);
        FireNote(); Drain();
    }
    ShFrame<MA>(); MgDueN();
    const auto shown = Shown();
    size_t at = sh;
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (at < shown.size() && shown[at].tag == ShGenTag(tags[i], false)) { g.gen++; at++; }
        if (at < shown.size() && shown[at].tag == tags[i]) { g.real++; at++; }
    }
    g.order = (int)(shown.size() - at);
    g.iShow = a.pl.iShow - i0; g.nRaised = a.pl.nRaised - n0; g.forced = a.w.iWhy[FG_AW_FORCED] - f0; g.timers = ThS::timersSet;
    LONGLONG te = 0;
    if (!ends.empty()) WaitFor([&] { return MA::EndFor(ends.back(), &te); }, 1000);
    for (LONG f : ends) g.ends += MA::EndFor(f, &te) ? 1 : 0;
    StopThreads();
    ThS::noTimer = 0;
    MgClean("TL1");
    MgEnd();
    return g;
}
struct T1Edge { int ranAtEdge, restLater, inStamp, endStamp, shown; LONG own, restStamps; };
static T1Edge T1EdgeRun(bool rule)
{
    T1Edge e = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000;
    ShWarm<MA>(); MgDueN();
    ThS::noTimer = 0;
    ThStart();
    MgEvenStart();
    LONG passWindows = 0;
    // THE DRIVER'S STEP AT THE EDGE: the rule asked with the want it is about to write, the event by its answer, and only
    // then the want. A present recorded between the two still reads ON: that is the last engaged pair, below.
    const bool run = rule ? FgThreadsRun<ShArmB>(&passWindows, FG_WANT_PASS) : false;
    e.ranAtEdge = run ? 1 : 0;
    if (run) SetEvent(ThS::runEv); else ResetEvent(ThS::runEv);
    Sleep(320);                                               // (a rested stamper has left its 250 ms slice by now and stands in its run wait)
    const LONG own0 = a.pl.tsOwn + a.pl.tsNever;
    const size_t sh = MgShownN();
    const LONG timed0 = a.pl.nTimed;
    const DWORD tag = MgEven<MA>();                           // THE LAST ENGAGED PAIR
    const LONG p = MgP(), fEnd = B::frames - 1; const LONGLONG t0 = B::clockNow;
    WriteRelease(&s.wt.want, FG_WANT_PASS);
    FgArmedPair rec = {}; LONGLONG ts = 0; FgDue d = {};
    MA::Read(a.pair[p & (FG_RING - 1)], p, &rec);
    if (!WaitFor([&] { return a.pl.nTimed > timed0; }, 3000)) Err("TL1 edge: the placer's thread did not time the last pair");
    e.inStamp = WaitFor([&] { return MA::ReadStamp(s.st[FG_TL_IN], rec.n, &ts); }, rule ? 2000 : 400) ? 1 : 0;
    B::clockNow = t0 + MA::UsToTicks(2100);
    if (!WaitFor([&] { return MgDue(false, p, &d); }, 3000)) Err("TL1 edge: no due was raised for the last pair's generated frame");
    FireNote(); Drain();
    B::clockNow = t0 + MA::UsToTicks(2100 + kMgPeriodUs / 2);
    if (!WaitFor([&] { return MgDue(true, p, &d); }, 3000)) Err("TL1 edge: no due was raised for the last pair's real frame");
    FireNote(); Drain();
    e.endStamp = WaitFor([&] { return MA::EndFor(fEnd, &ts); }, rule ? 2000 : 400) ? 1 : 0;
    e.own = a.pl.tsOwn + a.pl.tsNever - own0;
    {
        const auto shown = Shown();
        if (shown.size() >= sh + 2 && shown[sh].tag == ShGenTag(tag, false) && shown[sh + 1].tag == tag) e.shown = 1;
    }
    // A WINDOW OF STANDBY LATER: the first standby present has closed what was open, and the rule is asked again
    ShFrame<MA>();
    const bool run2 = FgThreadsRun<ShArmB>(&passWindows, FG_WANT_PASS);
    e.restLater = run2 ? 0 : 1;
    if (!run2) ResetEvent(ThS::runEv);
    Sleep(320);
    const LONG st0 = s.c.stamped[FG_TL_IN] + s.c.stamped[FG_TL_PREP] + s.c.stamped[FG_TL_OUT] + s.c.stamped[FG_TL_END];
    for (int i = 0; i < 5; i++) ShFrame<MA>();
    Sleep(40);
    e.restStamps = s.c.stamped[FG_TL_IN] + s.c.stamped[FG_TL_PREP] + s.c.stamped[FG_TL_OUT] + s.c.stamped[FG_TL_END] - st0;
    StopThreads();
    MgClean("TL1 edge");
    MgEnd();
    return e;
}
static void CaseTL1()
{
    const char* name = "TL1 the product's thread loops (fg_threads.h) over the fakes: the placer's own timer, four stampers, the disengage edge (stage 8)";
    CaseBegin(name);
    int entries = 0;
    for (void* p : kThreadsShapeEntries) { gSplitSink = p; if (gSplitSink) entries++; }
    // ---- 1. the run
    const T1Leg g = T1Run(true);
    printf("  thirty pairs on the shared loops, nothing waking the placer but the machine and its own timer (set %ld time(s)): it raised %ld generated frames' dues and %ld real frames';"
           " on the screen %d generated frames and %d real ones, %d picture(s) out of place; frame ends stamped %ld of 30; real frames forced first %ld\n",
        g.timers, g.iShow, g.nRaised, g.gen, g.real, g.order - 1, g.ends, g.forced);
    if (g.iShow != 30 || g.nRaised != 30 || g.gen != 30 || g.real != 30 || g.order != 1 || g.forced != 0)
        Err("TL1: the placer's thread raised %ld and %ld dues, and the screen showed %d generated and %d real frames with %d other picture(s) (30, 30, 30, 30 and the one closing frame wanted); forced %ld",
            g.iShow, g.nRaised, g.gen, g.real, g.order, g.forced);
    if (g.timers < 30) Err("TL1: the placer's timer was set %ld time(s) in thirty pairs: the dues were not raised by it", g.timers);
    // (a stamper stamps the value it finds: a frame end it did not look at in time has no stamp, so not every one need be there)
    if (g.ends < 20) Err("TL1: %ld of 30 frame ends were stamped by the frame end's stamper", g.ends);
    const RingStats stc = gRingSt;
    const T1Leg c = T1Run(false);
    ViolRestore(stc);
    if (c.iShow != 0 || c.gen != 0 || c.real != 30 || c.forced < 29 || c.timers != 0)
        Err("TL1 control: with a placer whose timer is never set %ld dues were raised, %d generated frames shown, %d real frames, %ld forced first, the timer set %ld time(s) (0, 0, 30, at least 29 and 0 wanted): the control shows nothing",
            c.iShow, c.gen, c.real, c.forced, c.timers);
    Ctl("the same loop with no timer: %ld dues raised, %d generated frames shown, every real frame forced out at the next injection (%ld), %d shown", c.iShow, c.gen, c.forced, c.real);
    // ---- 2. the disengage edge
    const T1Edge e = T1EdgeRun(true);
    printf("  the disengage edge by the rule: the stampers %s at the edge; the last pair's handoff %s and its frame end %s, its generated frame and its real frame %s;"
           " timed by the placer's own clock %ld; a window later the rule says %s, and five standby presents added %ld stamp(s)\n",
        e.ranAtEdge ? "kept running" : "RESTED", e.inStamp ? "was stamped" : "HAD NO STAMP", e.endStamp ? "was stamped" : "HAD NO STAMP", e.shown ? "were shown in order" : "WERE NOT SHOWN IN ORDER",
        e.own, e.restLater ? "rest" : "RUN", e.restStamps);
    if (!e.ranAtEdge || !e.inStamp || !e.endStamp || !e.shown || !e.restLater || e.restStamps != 0)
        Err("TL1 edge: ran at the edge %d, handoff stamped %d, frame end stamped %d, shown in order %d, rested a window later %d, stamps while rested %ld (1, 1, 1, 1, 1 and 0 wanted)",
            e.ranAtEdge, e.inStamp, e.endStamp, e.shown, e.restLater, e.restStamps);
    const RingStats ste = gRingSt;
    const T1Edge ce = T1EdgeRun(false);
    ViolRestore(ste);
    if (ce.inStamp || ce.endStamp || ce.own < 1)
        Err("TL1 edge control: with the stampers rested at the edge the last pair's handoff stamp is %d, its frame end's %d, and the placer timed %ld pair(s) by its own clock (0, 0 and at least 1 wanted): the control shows nothing",
            ce.inStamp, ce.endStamp, ce.own);
    Ctl("the stampers rested at the edge: the last pair's handoff and its frame end have no stamp, and the placer timed it by its own clock (%ld)", ce.own);
    // ---- 3. the rule by itself
    {
        MgFresh<MA>();
        FgState& s = B::S();
        LONG pw = 5; int bad = 0;
        if (!FgThreadsRun<ShArmB>(&pw, FG_WANT_ON) || pw != 0) bad |= 1;           // engaged: they run, and the count starts again
        if (!FgThreadsRun<ShArmB>(&pw, FG_WANT_PASS) || pw != 1) bad |= 2;         // the edge window: they still run
        _InterlockedExchange(&s.sh.pairOpen, 1);
        if (!FgThreadsRun<ShArmB>(&pw, FG_WANT_PASS)) bad |= 4;                    // a window later with a pair open: they still run
        _InterlockedExchange(&s.sh.pairOpen, 0);
        if (FgThreadsRun<ShArmB>(&pw, FG_WANT_PASS)) bad |= 8;                     // and with none: rest
        if (!FgThreadsRun<ShArmB>(&pw, FG_WANT_ON) || pw != 0) bad |= 16;          // engaged again: up before the first hand
        MgEnd();
        printf("  the rule by itself, five answers: %s; the compile shape: %d instantiation(s) of the loops over the product's two shapes\n", bad ? "WRONG" : "right", entries);
        if (bad) Err("TL1: fg_threads.h's run rule answered wrong (bits %d)", bad);
        if (entries != (int)(sizeof(kThreadsShapeEntries) / sizeof(kThreadsShapeEntries[0]))) Err("TL1: the loops' compile shape has %d entries", entries);
    }
    CaseEnd(name);
}
