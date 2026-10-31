// dt_fg_law.h: THE LAW, THE ARMED PRODUCT'S OWN DRIVER, IN THE HARNESS (2026-10-17, phase 5, package P1; docs/reports/fg-5/brief-plan/PLAN.md,
// section 1.B P1, and docs/reports/fg-5/brief-W/BRIEF-P1-law.md).
//
// Included by dt_fg_test.cpp behind dt_fg_place.h, with ONE line; its cases are LW1 to LW16.
//
// WHAT A CASE HERE IS. One fake game (dt_fg_rates.h's simulation) whose want is written ONCE A WINDOW OF 120 FRAMES by what the product's
// latency waiter runs: THE READER (what the conditions are handed as the real rate) and the conditions (source/fg/fg_cond.h, as the product compiles it),
// and from LW1 on the law itself. The window's numbers are the simulation's own: the median interval between the window's frames'
// entries (the waiter's median period), the game's own cost (what the fake game was given), and the pacer's law E as the machine
// computed it this window (FgRenderLine::lawEUs, read once at the window's end as the waiter reads it).
#pragma once
#include "../../source/fg/fg_cond.h"
#include "../../source/fg/fg_ffwatch.h"   // (round 2: FusionFix's limiter changed in its own menu, LW14)
// P2'S PACER (head 1.0) IS MERGED (fg-revival, 2026-10-17, phase 5): LW12 judges R_on within 1.5 of the game's own rate
#define LW_P2_MERGED 1
// (round 3, G4) THE PRODUCT'S READER of R_free, as fg_arm_life.h's FgpLawFreeX10 calls it
#define LW_PRODUCT_FREE(med, per, floor, ms, alone) FgCondFreeX10(med, per, floor, ms, alone)

// ---------------------------------------------------------------- the window's numbers
// The median interval between the entries of frames [a, b), and the median of the costs the fake game was given there, in ms.
static double LwMedEntryMs(int a, int b)
{
    std::vector<LONGLONG> iv;
    for (int i = a + 1; i < b && i < (int)gRt.pairs.size(); i++) iv.push_back(gRt.pairs[i].tEntry - gRt.pairs[i - 1].tEntry);
    if (iv.empty()) return 0.0;
    std::nth_element(iv.begin(), iv.begin() + iv.size() / 2, iv.end());
    return (double)iv[iv.size() / 2] * 1000.0 / (double)B::Qpf();
}
static double LwMedCostMs(int a, int b)
{
    std::vector<LONGLONG> c;
    for (int i = a; i < b && i < (int)gRt.pairs.size(); i++) c.push_back(gRt.pairs[i].cost);
    if (c.empty()) return 0.0;
    std::nth_element(c.begin(), c.begin() + c.size() / 2, c.end());
    return (double)c[c.size() / 2] * 1000.0 / (double)B::Qpf();
}
// THE SESSION AS THE CONDITIONS SEE IT IN THESE CASES: the product's facts of the owner's session (a window, FLIP with two back buffers,
// FP16 and HDR on, variable refresh, the marker and the saved Present the mod's own), the panel the simulation runs at, LOCK 1015.
static FgCondIn LwSession(const RtPar& par)
{
    FgCondIn in = {};
    in.presentOurs = 1; in.markerOk = 1; in.dxvk31 = -1;
    in.refreshNum = (int)(ULONG)(par.refresh >> 32); in.refreshDen = (int)(ULONG)par.refresh; in.lockPm = 1015;
    in.swapEffect = 2; in.backBuffers = 2; in.windowed = 1; in.bbFp16 = 1; in.hdrOn = 1; in.vrr = 1;
    return in;
}
// THE READER. LW_NOW is the product's (fg_arm_life.h's FgpCondRead since phase 5, through fg_cond.h's FgCondRateX10): standing by the
// window's median real frame time, engaged R_free from the pacer's law E. LW_OLD is THE READER BEFORE PHASE 5: the window's median real
// frame time, engaged or not, and the pair's rate never above C (fg_cond.h's FgCondPairFpsX10, gone with phase 5). Kept here, written out,
// as the control of LW2.
// LW_E is THE BRIEF'S FIRST FORM, built and replaced the same day: R_free as the pacer's law E (FgRenderLine::lawEUs), which leaves out the
// time inside the original Present, where a card that is the limit holds the game (fg_cond.h, FgCondFreeX10, says why; LW11 shows it).
enum { LW_OLD = 1, LW_NOW = 0, LW_E = 2 };
// THE WINDOW'S R_FREE AS THE PRODUCT WORKS IT OUT (fg_arm_life.h, FgpLawFreeX10): FgPace's own period and E from the render line against the
// window's median. LwFreeBegin at a run's start; LwWindowFree once a window, before the read.
static int gLwFree = 0, gLwPerUs = 0;
static void LwFreeBegin() { gLwFree = 0; gLwPerUs = 0; }
static int gLwLawPerUs = 0, gLwMedUs = 0;
// (round 3, G4, 2026-10-18) THE READER OF BEFORE, written out as the controls' (FX16, LW2): under the pacer's hold at the floor, R_free the rate
// of the law's median sample, which reads the render thread's own part alone (fg_cond.h, FgCondFreeX10, says why). gLwFreeBefore 1: the
// window's R_free by it; 0: the product's.
static int gLwFreeBefore = 0;
static int LwFreeBeforeX10(int lawMedUs, int periodUs, int floorUs, double medMs)
{
    if (periodUs <= 0 || floorUs <= 0 || medMs <= 0.1) return 0;
    const bool atFloor = (double)periodUs <= (double)floorUs * 1.01, held = medMs * 1000.0 <= (double)periodUs * 1.03;
    if (!(atFloor && held)) return (int)(10000.0 / medMs);
    return lawMedUs > 0 ? (int)(10000000LL / lawMedUs) : 0;
}
// (round 3, G4) A STAGE THAT RUNS IN THE PACER'S SLEEP: the game's frame as two stages, the render thread's own part and a stage one frame
// ahead of it that starts its frame when the render thread starts the one before (GTA IV's update thread's shape; the worker's replay and
// the card behave alike). The fake game is given, for the frame that begins now, the render thread's own part, or longer where it must wait
// for the stage ahead: running free its interval is the longer stage's, and held by the pacer the stage ahead finishes in the sleep, so the
// sample is the render thread's part alone, as the evening's fixed120 leg read (dt_fg_fixed.h, FX16).
struct LwStages { LONGLONG beginPrev, uEnd; int have; };
static double LwStageCostMs(LwStages& st, double frameMs, double renderMs)
{
    const LONGLONG begin = B::clockNow;
    const LONGLONG uStart = !st.have ? begin : (st.uEnd > st.beginPrev ? st.uEnd : st.beginPrev);
    st.uEnd = uStart + RtMs(frameMs); st.beginPrev = begin; st.have = 1;
    const double waitMs = (double)(st.uEnd - begin) * 1000.0 / (double)B::Qpf();
    return waitMs > renderMs ? waitMs : renderMs;
}
// aloneX10: the game's own rate as the law last read it where nothing held it (fg_cond.h, FgDriveMem::aloneX10), as fg_arm_life.h hands it
static void LwWindowFree(bool engaged, LONG eUs, double medMs, const FgCondIn& session, int aloneX10)
{
    (void)eUs;
    const FgState& s = B::S();
    gLwPerUs = (int)(s.r.period * 1000.0);
    // (round 3, as fg_arm_life.h's FgpLawFreeX10: the law's median of the game's own cost, and its own period before the room floor)
    gLwMedUs = s.law ? FgCondRingMedUs(s.law->fast) : 0;
    gLwLawPerUs = s.law ? (int)(s.law->per * 1000.0) : 0;
    gLwFree = !engaged ? 0 : gLwFreeBefore ? LwFreeBeforeX10(gLwMedUs, gLwPerUs, FgCondFloorUs(session), medMs)
                                           : LW_PRODUCT_FREE(gLwMedUs, gLwPerUs, FgCondFloorUs(session), medMs, aloneX10);
}
static int LwFreeX10(int how, bool engaged, LONG eUs)
{
    if (!engaged) return 0;
    if (how == LW_E) return eUs > 0 ? (int)(10000000 / eUs) : 0;
    return gLwFree;
}
// THE READING INTO THE CONDITIONS' FACTS, as fg_arm_life.h's FgpCondRead fills them: the measured rate (realFpsX10: standing by the game's
// own, engaged R_on, SLOW's), R_free engaged (freeFpsX10, FAST's) and, standing by, the rate predicted with frame generation (predFpsX10,
// FGC_ONSLOW's; pred: the law's memory and numbers, null for none). LW_OLD fills the reader before phase 5: the median, never above C.
// (round 8, P: stamps 1, FGC_ONSLOW asks the rate with K the least c outside the fixed mode, as FgpCondRead calls FgpLawPredOnX10 with
// FgCondFixedMode of the session; 0, the prediction of before, the control. The session's facts must be in `in` before this call.)
static void LwFill(FgCondIn& in, int how, bool engaged, double medMs, LONG eUs, const FgDriveMem* pm = nullptr, const FgDriveK* pk = nullptr, int card = -1, int hi = 1, int stamps = 1)
{
    in.engaged = engaged ? 1 : 0;
    in.realFpsX10 = FgCondRateX10(medMs, 0);
    in.freeFpsX10 = 0; in.predFpsX10 = 0;
    if (how == LW_OLD) { const int c = FgCondCFpsX10(in); if (c > 0 && in.realFpsX10 > c) in.realFpsX10 = c; return; }
    in.freeFpsX10 = LwFreeX10(how, engaged, eUs);
    // (phase 5 round 2, S2: the product's FgpLawPredX10 asks the rate with c's margin, FgDrivePredHiX10; hi 0, the prediction of before)
    if (!engaged && pm && pk) in.predFpsX10 = !hi ? FgDrivePredX10(*pm, in.realFpsX10, card)
                                            : stamps ? FgDrivePredOnX10(*pm, in.realFpsX10, card, FgCondFixedMode(in) ? 1 : 0) : FgDrivePredHiX10(*pm, in.realFpsX10, card);
}

// ---------------------------------------------------------------- LW2, a game that gets lighter while engaged
// WHAT IT SHOWS. The 120 Hz panel (C 59.1). A game engaged at 62.5 frames a second by itself (16 ms) for five windows, then lighter, window
// by window, to 90 (11.1 ms) over nineteen, and held there. Once a window the reader and the conditions; the want PASS where a row stands.
// THE LAW'S DEMAND (the plan's LW2): FG stands down within three windows of the game's own rate passing 70 (the FAST row's 67 and its 3 of
// hysteresis while engaged), and not before it passes 67.
// CONTROL, TODAY'S READER (LW_OLD): the window's median real frame time is the pacer's own period while engaged (59.1), and the pair's rate
// is never above C: FAST cannot fire while engaged, and the game is held at 59 with generation for the rest of the run. IT MUST FAIL.
// (ROUND 3 OF THE ROUND, G4, 2026-10-18: WHAT MOVED, AND WHY. The evening's fixed120 leg showed that while the pacer holds the game the law's
// samples are the render thread's own part alone (fg_cond.h, FgCondFreeX10; dt_fg_fixed.h, FX16): R_free from them stood the fixed mode down
// at 293.7 for a game of 93 by itself, and at the variable panel's floor it read 180 to 300 for a game of about 100 (fill95). This fake game had
// no part of its frame that runs in the pacer's sleep, so its samples were its cost and the climb was seen; the game's are not. NOW, held at
// the floor, R_free is the game's own rate as last read standing by, never over the samples' bound, and the climb is NOT SEEN while held: FG
// stays engaged at the floor to the end (told, not judged). WHAT STILL HOLDS AND IS JUDGED: FG is never stood down before the game passes 67,
// at 3 and 10 percent of spread and with A STAGE IN THE PACER'S SLEEP (LwStages, the render thread's own part 5 ms): the reader of before
// stands that one down at once, in the first window engaged, for a game at 62.5 (THE CONTROL). The true fill takes the hold away (its pattern
// paces the game at its own cost): the climb is the fill's to see.)
struct Lw2Leg { std::vector<double> game, med; std::vector<int> rX10, row; std::vector<LONG> want, eUs; int w67, w70, stood, early; int order, n, real1; std::string first; };
static Lw2Leg Lw2Run(int how, double spread = 0.03, double renderMs = 0.0)
{
    Lw2Leg g = {};
    RtPar par;
    RtBegin(par);
    FgState& s = B::S();
    const FgCondK k = FgCondDefaults();
    gRtSeed = 20261017u;
    LwFreeBegin();
    g.w67 = g.w70 = g.stood = g.early = -1;
    int alone = 0;                                            // (G4: the game's own rate in the last window nothing was engaged, as the law keeps it)
    LwStages st = {};                                         // (G4: renderMs over 0, the stage in the pacer's sleep)
    for (int win = 0; win < 34; win++)
    {
        const double cost = win < 5 ? 16.0 : win < 24 ? 16.0 - (16.0 - 11.1) * (double)(win - 5) / 19.0 : 11.1;
        const int first = (int)gRt.pairs.size();
        const bool engaged = s.wt.want != FG_WANT_PASS;
        std::vector<double> own;
        for (int i = 0; i < 120; i++)
        {
            const double f = cost * RtSpread(spread);
            own.push_back(f);
            RtFrame(RtMs(renderMs > 0.0 ? LwStageCostMs(st, f, renderMs * RtSpread(0.10)) : f));
        }
        const int last = (int)gRt.pairs.size();
        std::nth_element(own.begin(), own.begin() + own.size() / 2, own.end());
        const double med = LwMedEntryMs(first, last), game = renderMs > 0.0 ? own[own.size() / 2] : LwMedCostMs(first, last);
        const LONG eUs = s.r.lawEUs;
        FgCondIn in = LwSession(par);
        LwWindowFree(engaged, eUs, med, in, alone);
        if (!engaged) alone = FgCondRateX10(med, 0);
        LwFill(in, how, engaged, med, eUs);
        const int row = FgCondFirst(in, k);
        const LONG want = row ? FG_WANT_PASS : FG_WANT_ON;
        g.game.push_back(game); g.med.push_back(med); g.rX10.push_back(FgCondFastRateX10(in)); g.row.push_back(row); g.want.push_back(want); g.eUs.push_back(eUs);
        if (g.w67 < 0 && game > 0.0 && 1000.0 / game > 67.0) g.w67 = win;
        if (g.w70 < 0 && game > 0.0 && 1000.0 / game > 70.0) g.w70 = win;
        if (engaged && want == FG_WANT_PASS && g.stood < 0) { g.stood = win; if (g.w67 < 0 || win < g.w67) g.early = win; }
        s.wt.want = want;
    }
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    s.wt.want = FG_WANT_ON;
    RtEnd("LW2");
    return g;
}
static void Lw2Print(const char* what, const Lw2Leg& g)
{
    printf("  %s\n  window   the game by itself   the window's median   E (us)   the rate read   the row that stands   the want next\n", what);
    for (size_t i = 0; i < g.game.size(); i++)
        printf("  %4d     %6.1f               %6.1f               %6ld   %6.1f          %-20s  %s\n", (int)i, g.game[i] > 0 ? 1000.0 / g.game[i] : 0.0, g.med[i] > 0 ? 1000.0 / g.med[i] : 0.0,
            g.eUs[i], g.rX10[i] / 10.0, FgCondName(g.row[i]), g.want[i] == FG_WANT_PASS ? "PASS" : "ON");
    printf("  the game passed 67 by itself in window %d and 70 in window %d; FG stood down behind window %d; pictures out of place %d; real frames shown once %d of %d\n",
        g.w67, g.w70, g.stood, g.order, g.real1, g.n);
}
static void CaseLW2()
{
    const char* const name = "LW2 a game climbing from 60 to 90 while held at the floor is never stood down before 67, its render thread's part no reading of its rate (2026-10-17, phase 5; round 3, G4)";
    CaseBegin(name);
    const Lw2Leg g = Lw2Run(LW_NOW);
    Lw2Print("THE READER NOW:", g);
    if (g.order || g.real1 != g.n) Err("LW2: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", g.order, g.first.c_str(), g.real1, g.n);
    if (g.early >= 0) Err("LW2: FG stood down behind window %d, before the game passed 67 by itself (window %d)", g.early, g.w67);
    printf("  TOLD (round 3, G4): held at the floor the climb is not read: the game passed 70 by itself in window %d, FG stood down behind window %d (-1: never)\n", g.w70, g.stood);
    const Lw2Leg s = Lw2Run(LW_NOW, 0.10);
    Lw2Print("THE SAME CLIMB, THE GAME'S COST SPREAD 10 PERCENT:", s);
    if (s.order || s.real1 != s.n) Err("LW2 at a 10 percent spread: %d picture(s) out of place, %d of %d real frames shown exactly once", s.order, s.real1, s.n);
    if (s.early >= 0) Err("LW2 at a 10 percent spread: FG stood down behind window %d, before the true median passed 67 (window %d)", s.early, s.w67);
    // (round 3, G4: THE GAME'S SHAPE, a stage in the pacer's sleep: the render thread's own part 5 ms)
    const Lw2Leg t = Lw2Run(LW_NOW, 0.03, 5.0);
    Lw2Print("THE SAME CLIMB WITH A STAGE IN THE PACER'S SLEEP (the render thread's own part 5 ms), THE READER NOW:", t);
    if (t.order || t.real1 != t.n) Err("LW2 with the stage: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", t.order, t.first.c_str(), t.real1, t.n);
    if (t.early >= 0) Err("LW2 with the stage: FG stood down behind window %d, before the game passed 67 by itself (window %d)", t.early, t.w67);
    const RingStats stc = gRingSt;
    gLwFreeBefore = 1; const Lw2Leg c = Lw2Run(LW_NOW, 0.03, 5.0); gLwFreeBefore = 0;
    ViolRestore(stc);
    Lw2Print("CONTROL, THE READER OF BEFORE (the rate of the law's median sample under the hold), THE SAME STAGE:", c);
    if (c.early < 0) Err("LW2 control: the reader of before did not stand FG down before the game passed 67 (window %d; stood down behind %d): the control shows nothing", c.w67, c.stood);
    Ctl("the reader of before with the stage in the sleep: stood down behind window %d, the game past 67 only in window %d", c.stood, c.w67);
    const RingStats sto = gRingSt;
    const Lw2Leg o = Lw2Run(LW_OLD);
    ViolRestore(sto);
    printf("  TOLD, THE READER OF PHASE 4 (the window's median, never above C): stood down behind window %d\n", o.stood);
    CaseEnd(name);
}

// ================================================================ THE LAW IN THE SIMULATION (LW1, LW3 to LW8)
// WHAT fg_arm_life.h's FgpLawWindowStep READS, read here from the same machine, once a window of 120 frames: the reader (LW_NOW), the
// conditions with the case's session, the band, the generated and real frames the machine presented (FgState::c.presentsI, presentsN),
// the places of the generated frame on the card (the machine's ring, FgArmedWorker::placeV and placeN, fed by the simulation's shown
// instants where RtPar::place is on), the camera cut count (the binding's camJumps), the simulation's clock in ms; and the law itself
// (fg_cond.h's FgDrive*, unchanged), whose want the case writes. THE CUT POLL after every frame, as the product's waiter polls it.
// ONE DIFFERENCE, SAID: the product's present path stands by at the very present a row stands (FgPA::MayEngage reads the row a present);
// this machine's binding has no such word, so here a row stands frame generation down at the window's step, at most a window later. The
// cases judge the law's states and the frames, not that instant.
struct LwLaw
{
    FgDriveMem mem; FgDriveK k; FgCondK ck;
    LONG presI, presN, place;
    int how, nvCap, ffCap;
    int card;                                     // the card's reading standing by, as the case gives it (1 the limit, 0 idle room, -1 cannot tell)
    int engageFrame;                              // the frame the last engage came at (-1: none)
    std::vector<double> turnsMs;                  // the simulation's clock at every engage and stand-down
    std::vector<double> changesMs;                // (S2 round 2) ... at every change of state of any kind
    double waitFromMs, waitToMs;                  // the last wait for a cut: the clock at its start and at the engage that ended it
    int waitFromWin;                              // ... and the window behind which it started
    int noPred;                                   // 1: THE LAW WITHOUT THE PREDICTION (a control): no FGC_ONSLOW, whatever the cost
    int noLawPer;                                 // 1: c with the law's period not taken out, the round-2 rule (LW15's control)
    int devPass;                                  // (phase 5 round 2, S2) 1: THE ARMED DEVELOPMENT BUILD'S WINDOW ORDER: the shadow driver's step writes the
                                                  // want PASS ahead of the conditions' read (DLSS-IV.cpp, the latency waiter's window: FgpDevWindowStep, then
                                                  // FgpArmWindowStep; fg_product.h wrote PASS there at fg_shadow 0 until this round), the law writes it back after
    int oldRead;                                  // (S2) 1: THE CONDITIONS' "ENGAGED" AS IT WAS READ UNTIL THIS ROUND, from the want word (LW19's control)
    int cardEngaged;                              // (S2) the card's reading the latency waiter gives in a window that ran engaged (-2: the case's own, card;
                                                  // the lawon leg's: 0, "idle room", the waiter's in-flight median at 0 behind frame generation's pacer)
    int cpuAt, win;                               // (S2) the window from which the CPU placement has answered (0: at once), and the windows so far
    FgDriveCardH cardH;                           // (S2) the product's kept readings (fg_cond.h, FgDriveCardTake)
    int noMargin;                                 // (S2) 1: FGC_ONSLOW asks the rate on c's mean alone, the law of before (LW20's control)
    int noOnslowBand;                             // (S2 round 2) 1: FGC_ONSLOW with no band in the wait for a cut, the table of before (LW21's control)
    int oldShadow;                                // (S2 round 2) 1: the shadow driver's step writes PASS at fg_shadow 0, as before this round (LW19's arms)
    int passOnRead;                               // (S2 round 2) windows the law ran ON whose want the conditions read as PASS
    const signed char* cardSeries; int cardSeriesN; // (S2 round 2) the card's reading standing by window by window (null: card)
    int noStamps;                                 // (round 8, P) 1: THE LAW OF BEFORE, no K: FgDriveIn::kUs 0 and FGC_ONSLOW on c and its margin alone (the control)
    LONG kAt[FG_K_BINS];                          // (round 8, P) the placer's kFull at the last window not ON, as fg_arm_life.h's fgpLawKAt
};
struct LwRec { double game, med; int rX10, rFree, rE, rPred, cUs, row, band, state, prev, why; int shownGen, shownN, cardMed, cardN, frames; LONG want; };
static unsigned LwNowMs() { return (unsigned)(B::clockNow * 1000 / B::Qpf()); }
// THE SCENES' FLOOR (2026-10-27, the owner's floor of 38 in and 35 out; DECISIONS, "THE FLOOR: 38 IN, 35 OUT"). The scenes of LW9, LW12, LW13,
// LW15, LW17, LW20 to LW22, LW46 to LW50, LW52 and LW53 were set BY NUMBER at the floor of 40 (out under 37): a busy card at R_on 36, a
// prediction of 38.3 to 39.6, a game hovering at 40. They hold the law's mechanics at the floor's edge, not the floor's value, so they run on
// the floor they were set at, unkeyed (a 60 Hz panel keeps its 30). The table's own value is held by tools/test-fg-cond (its oracle, the
// floor's cases and the policy's rows) and by SH3 here.
static FgCondK LwSceneK() { FgCondK k = FgCondDefaults(); k.minRealFpsX10 = 400; return k; }
static void LwLawBegin(LwLaw& L, const FgDriveK& k, const FgCondK& ck, int how = LW_NOW)
{
    FgState& s = B::S();
    L = LwLaw(); L.k = k; L.ck = ck; L.how = how; L.engageFrame = -1; L.card = -1; L.cardEngaged = -2;
    FgDriveInit(L.mem, L.k);
    s.wt.want = FG_WANT_PASS;                                 // (the law starts standing by: only it engages)
    L.presI = s.c.presentsI; L.presN = s.c.presentsN; L.place = ShArmB::a.w.placeN;
    LwFreeBegin();
}
static void LwLawPoll(LwLaw& L)
{
    if (!FgDriveCut(L.mem, L.k, LwNowMs(), (int)B::camJumps, 0)) return;
    B::S().wt.want = FG_WANT_ON; L.engageFrame = (int)gRt.pairs.size(); L.turnsMs.push_back((double)LwNowMs()); L.waitToMs = (double)LwNowMs();
    L.changesMs.push_back((double)LwNowMs());
}
// THE CONDITIONS' "ENGAGED" AS fg_arm_life.h's FgpCondRead READS IT (phase 5 round 2, S2), behind the development build's PASS where the case
// plants it (LwLaw::devPass).
// (REVIEW ROUND 2, the reviewer's third: THE ORDER OF THE WRITERS AS THE PRODUCT RUNS IT. The development build's shadow driver step, which
// came first, wrote nothing at fg_shadow 0, and went with that driver in round 8's deep strip; LwLaw::oldShadow writes PASS there as that
// step did before round 2. Then the conditions' read, then the law's own write behind it, as in LwLawWindow.)
static bool LwEngagedRead(LwLaw& L)
{
    if (L.devPass && L.oldShadow) B::S().wt.want = FG_WANT_PASS;   // (the PASS of before; no step writes the want ahead of the law now)
    const bool wantOn = B::S().wt.want != FG_WANT_PASS;
    if (L.devPass && L.mem.state == FGDR_ON && !wantOn) L.passOnRead++;   // (the present path would have stood by between the two writes)
    if (L.oldRead) return wantOn;                             // (the reading of before: the want word)
    return FgCondEngagedOf(1, FgDriveWantsOn(L.mem) ? 1 : 0, wantOn ? 1 : 0, 0) != 0;   // (fg_arm_life.h, FgpLawEngagedNow: the law drives every case here)
}
// THE CARD'S READING THE LAW IS HANDED (S2), as fg_arm_life.h's FgpLawFreeX10 takes it from the waiter's reading of the window that ran: the
// product's FgDriveCardTake (taken only where the law held nothing, standing over three); the reading of before took every window's.
static int LwCardRead(LwLaw& L)
{
    const int stand = (L.cardSeries && L.win < L.cardSeriesN) ? L.cardSeries[L.win] : L.card;   // (review round 2: the leg's own readings, window by window)
    const int reading = (L.cardEngaged != -2 && L.mem.state == FGDR_ON) ? L.cardEngaged : stand;
    if (L.oldRead) return reading;
    return FgDriveCardTake(L.cardH, reading, FgDriveHolds(L.mem) ? 1 : 0);
}
static LwRec LwLawWindow(LwLaw& L, const RtPar& par, int first, int last, int cleared = 0)
{
    FgState& s = B::S();
    LwRec r = {};
    r.frames = last - first; r.med = LwMedEntryMs(first, last); r.game = LwMedCostMs(first, last);
    const bool engaged = LwEngagedRead(L);
    const int card = LwCardRead(L);
    const LONG eUs = s.r.lawEUs;
    FgCondIn in = LwSession(par);
    LwWindowFree(engaged, eUs, r.med, in, L.mem.aloneX10);
    LwFill(in, L.how, engaged, r.med, eUs, L.noPred ? nullptr : &L.mem, &L.k, card, (L.oldRead || L.noMargin) ? 0 : 1, L.noStamps ? 0 : 1);
    in.engaging = (!L.oldRead && !L.noOnslowBand && L.mem.state == FGDR_ENGAGING) ? 1 : 0;   // (S2 round 2: as fg_arm_life.h's FgpLawEngagingNow)
    in.nvCap = L.nvCap; in.ffCap = L.ffCap;
    r.rX10 = in.realFpsX10; r.rFree = LwFreeX10(LW_NOW, engaged, eUs); r.rE = LwFreeX10(LW_E, engaged, eUs); r.rPred = in.predFpsX10; r.cUs = FgDriveCUs(L.mem, card, nullptr);
    FgDriveIn di = {};
    di.realX10 = in.realFpsX10; di.card = card; di.lawPerUs = (engaged && !L.noLawPer) ? gLwLawPerUs : 0;
    di.floorUs = (engaged && !L.oldRead && !L.noLawPer) ? FgCondFloorUs(in) : 0;   // (S2: as fg_arm_life.h's FgpLawWindowStep; LW15's control takes neither)
    di.perUs = (engaged && !L.oldRead && !L.noLawPer) ? gLwPerUs : 0;              // (S2: the period in force, FgPace's own, as FgpLawFreeX10 reads it)
    if (engaged)
    {   // (G4's round 3: the probe's two readings, through fg_cond.h's functions as fg_arm_life.h's FgpLawWindowStep calls them; the rig has no fill)
        di.heldUpX10 = FgDriveHeldUpX10(gLwMedUs, gLwPerUs, FgCondFloorUs(in), r.med);
        di.fastX10 = FgDriveFastX10(in, L.ck);
    }
    in.lift |= FgDriveWarmLift(L.mem);                        // (round 5: a stint's warm-up lifts the rate rows, as fg_arm_life.h's FgpCondRead)
    di.row = FgCondFirst(in, L.ck); di.band = FgDriveBandOf(in, L.ck);
    { const LONG pi = s.c.presentsI, pn = s.c.presentsN; di.shownGen = (int)(pi - L.presI); di.shownN = (int)(pn - L.presN); L.presI = pi; L.presN = pn; }
    { const LONG pl = ShArmB::a.w.placeN; di.cardN = FgDrivePlaceMed(ShArmB::a.w.placeV, pl, L.place, &di.cardMed); L.place = pl; }
    di.cleared = cleared; di.cpuReady = L.win >= L.cpuAt ? 1 : 0; L.win++;
    if (cleared) L.cardH = FgDriveCardH();                    // (S2 review round 2: as fg_arm_life.h's FgpLawWindowStep at a Reset or a build)
    {   // (round 8, P: K, the stint's median over its full dispatches, as fg_arm_life.h's FgpLawKUs reads the placer's kFull)
        const bool onK = L.mem.state == FGDR_ON && L.mem.warmLeft == 0; long d[FG_K_BINS];   // (from the warm-up's end, as FgpLawKUs is called)
        for (int i = 0; i < FG_K_BINS; i++) { const LONG v = ShArmB::a.pl.kFull[i]; d[i] = v - L.kAt[i]; if (!onK || cleared) L.kAt[i] = v; }
        di.kUs = (onK && !cleared && !L.noStamps) ? FgDriveKMedUs(d, FG_K_BINS, FG_K_BIN_US) : 0;
    }
    const int was = L.mem.state;
    if (FgDriveStep(L.mem, di, L.k, LwNowMs(), (int)B::camJumps, 0))
    {
        if (L.mem.state == FGDR_ON) { L.engageFrame = last; L.waitToMs = (double)LwNowMs(); }
        if (L.mem.state == FGDR_ENGAGING) { L.waitFromMs = (double)LwNowMs(); L.waitFromWin = (int)(gRt.frameN / 120) - 1; }
        if (L.mem.state == FGDR_ON || was == FGDR_ON) L.turnsMs.push_back((double)LwNowMs());
        L.changesMs.push_back((double)LwNowMs());                 // (S2 round 2: every change of state, ENGAGING's entries and exits among them)
    }
    s.wt.want = FgDriveWantsOn(L.mem) ? FG_WANT_ON : FG_WANT_PASS;
    r.row = di.row; r.band = di.band; r.state = L.mem.state; r.prev = was; r.why = L.mem.why; r.want = s.wt.want;
    r.shownGen = di.shownGen; r.shownN = di.shownN; r.cardMed = di.cardMed; r.cardN = di.cardN;
    return r;
}
// The most of a list's instants in any five minutes of the simulation's clock.
static int LwIn5Min(const std::vector<double>& t)
{
    int most = 0;
    for (size_t i = 0; i < t.size(); i++) { int n = 0; for (size_t j = i; j < t.size() && t[j] - t[i] < 300000.0; j++) n++; if (n > most) most = n; }
    return most;
}
// The most engages and stand-downs in any five minutes of the simulation's clock.
static int LwTurnsIn5Min(const LwLaw& L) { return LwIn5Min(L.turnsMs); }
// (S2 round 2, the reviewer's second) THE MOST CHANGES OF STATE of any kind in any five minutes: each entry to the wait for a cut holds the
// latency ladder at rung 1 and parks the refresh guard, and each exit gives them back, so ENGAGING's flips count as the ON edges do.
static int LwChangesIn5Min(const LwLaw& L) { return LwIn5Min(L.changesMs); }
// A run's table: every window where the state changed, the first three, and every tenth (all where `all`).
static void LwPrint(const char* what, const std::vector<LwRec>& w, bool all = false)
{
    printf("  %s\n  window  game  median  real  R_free  E rate  pred   c us  row               band  state      shown      card (n)\n", what);
    for (size_t i = 0; i < w.size(); i++)
    {
        const LwRec& r = w[i];
        if (!all && i >= 3 && (i % 10) != 0 && r.state == r.prev && i + 1 != w.size()) continue;
        printf("  %5d  %5.1f  %5.1f  %5.1f  %5.1f   %5.1f   %5.1f  %4d  %-16s  %-4s  %-9s  %4d/%-4d  %4d (%d)%s%s\n", (int)i, r.game > 0 ? 1000.0 / r.game : 0.0, r.med > 0 ? 1000.0 / r.med : 0.0,
            r.rX10 / 10.0, r.rFree / 10.0, r.rE / 10.0, r.rPred / 10.0, r.cUs, FgCondEnumName(r.row), FgDriveBandName(r.band), FgDriveStateName(r.state), r.shownGen, r.shownN, r.cardMed, r.cardN,
            r.state != r.prev ? "   <- " : "", r.state != r.prev ? FgDriveWhyName(r.why) : "");
    }
}
// Every real frame shown exactly once, in order (FQ4's assertions), over the whole run.
struct LwFrames { int n, real1, gen, order; std::string first; };
static LwFrames LwFramesRead()
{
    LwFrames f = {};
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { f.n++; if (gRt.pairs[i].real == 1) f.real1++; if (gRt.pairs[i].gen == 1) f.gen++; }
    f.order = gRt.order; f.first = gRt.firstBad;
    return f;
}
static double LwShown(int a, int b)
{
    int n = 0, g = 0;
    for (int i = a; i < b && i < (int)gRt.pairs.size() - 1; i++) { n++; if (gRt.pairs[i].gen == 1) g++; }
    return n ? 100.0 * g / n : 0.0;
}

// ---------------------------------------------------------------- LW1, steady at the lock at 70's shape
// WHAT IT SHOWS. RL8's leg (t70.2 of 2026-10-17 morning: the lock at 300 MHz, the card's times pair by pair from the leg, the armed build's keys
// of today: the prepare's wait and the placement by the card), run from a machine STANDING BY under the law for 150 windows. THE PLAN'S LW1:
// it engages within the dwell (three windows, then the wait for a cut, which no cut ends here: ten seconds), and makes at most one
// transition in the 150 windows. AND THE BRIEF'S STOP RULE: THE ENGAGE'S FIRST SECOND (its first 50 pairs) at 84 of 100 or better.
// THE TOP MOVED OUT OF THE WAY, AND WHY (found by this case's first run, 2026-10-17): STANDING BY, THE GAME AT THAT LOCK IS NOT IN THE BAND.
// The leg's game alone ran at 74.5 (t70.1 of pair 13d5772a), and the fake standing by runs at about 108 (its card queue holds no frame
// that generates nothing); either is over the top of 67, so the law stands by with FGC_FAST for good, as it does for any game that fast
// by itself (the plan's D6). THE PLAN'S LEG 2 ("lock70 engages by itself") CANNOT HAPPEN AT THE TABLE'S TOP: the report of P1 says so.
// So the first run moves the top with its development key to 240 (fg_law_fast, FgCondFastKey), the way that leg would have to, and the
// second run, at the table's own top, is held to standing by with FGC_FAST in every window.
static RlLeg LwRl8() { return kRl[7]; }
static void LwShapeFrames(const RtPar& par, const RlLeg& l, int from, int count)
{
    const bool shape = l.lsA && l.lsAN > 0 && l.lsB && l.lsBN > 0;
    const double lsDry = (double)(par.tailUs + par.upUs + par.postUs) - 4000.0 * (1.0 - par.inj);
    for (int i = from; i < from + count; i++)
    {
        if (shape && lsDry > 1000.0)
        {
            const int j = i / kRlHold; const double fr = (double)(i % kRlHold) / (double)kRlHold;
            const double ls = (1.0 - fr) * (double)l.lsB[j % l.lsBN] + fr * (double)l.lsB[(j + 1) % l.lsBN];
            double k = ls / lsDry; if (k < 0.3) k = 0.3; if (k > 3.0) k = 3.0;
            gRt.par.tailUs = (LONG)(par.tailUs * (1.0 + (k - 1.0) * kRlShTail)); gRt.par.postUs = (LONG)(par.postUs * (1.0 + (k - 1.0) * kRlShPost)); gRt.par.upUs = (LONG)(par.upUs * (1.0 + (k - 1.0) * kRlShUp));
        }
        RtFrame(RtMs(gRlCostMs * RtSpread(0.25)));
    }
}
struct Lw1Leg { std::vector<LwRec> w; LwFrames f; int engageWin, engageFrame, turns, turns5, fastWins, waitWin; double first1s, after, waitMs; };
static Lw1Leg Lw1Run(int windows, int topFps)
{
    Lw1Leg g = {};
    const RlLeg l = LwRl8();
    gRlNow = RL_NOW_ALL; const RtPar par = RlPar(l, 0); gRlNow = 0;
    RtBegin(par);
    gRtSeed = 20261014u;
    FgCondK ck = FgCondDefaults(); FgCondFastKey(ck, topFps);
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), ck);
    g.engageWin = -1;
    for (int win = 0; win < windows; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { LwShapeFrames(par, l, first + i, 1); LwLawPoll(L); }
        g.w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (g.engageWin < 0 && L.mem.state == FGDR_ON) g.engageWin = win;
        if (g.w.back().row == FGC_FAST && g.w.back().state == FGDR_STANDBY) g.fastWins++;
    }
    g.engageFrame = L.engageFrame; g.turns = L.mem.engages + L.mem.downs; g.turns5 = LwTurnsIn5Min(L);
    g.waitWin = -1; g.waitMs = -1;
    for (size_t i = 0; i < g.w.size(); i++) if (g.w[i].state == FGDR_ENGAGING && g.w[i].prev == FGDR_STANDBY) { g.waitWin = (int)i; break; }
    if (g.engageFrame >= 0 && g.waitWin >= 0) g.waitMs = L.waitToMs - L.waitFromMs;
    g.f = LwFramesRead();
    // (the engage's first second: its first 50 pairs, as RL8's own reading of it; and every pair behind them)
    const int e = g.engageFrame;
    if (e >= 0) { g.first1s = LwShown(e, e + 50); g.after = LwShown(e + 50, (int)gRt.pairs.size()); }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW1");
    return g;
}
static void CaseLW1()
{
    const char* const name = "LW1 steady at the lock at 70's shape (RL8's leg) under the law: it engages within the dwell, one transition in 150 windows, the first second at 84 of 100 or better (2026-10-17, phase 5)";
    CaseBegin(name);
    const Lw1Leg g = Lw1Run(150, 240);
    LwPrint("THE LAW ON RL8's SHAPE, from standing by, THE TOP AT 240 (fg_law_fast):", g.w);
    printf("  the wait for a cut began behind window %d and the engage came %.0f ms later, at frame %d (no cut comes in this run: the wait is 10,000 ms); transitions %d in all, at most %d in five minutes;"
           " THE ENGAGE'S FIRST SECOND %.1f of 100 shown, behind it %.1f; %d frames, real frames shown once %d, out of place %d\n",
        g.waitWin, g.waitMs, g.engageFrame, g.turns, g.turns5, g.first1s, g.after, g.f.n, g.f.real1, g.f.order);
    if (g.f.order || g.f.real1 != g.f.n) Err("LW1: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", g.f.order, g.f.first.c_str(), g.f.real1, g.f.n);
    // (the dwell: the third window in band starts the wait; the wait: 10 s with no cut, the engage at the first frame past it)
    if (g.waitWin != 2 || g.waitMs < 10000.0 || g.waitMs > 10050.0) Err("LW1: the wait began behind window %d and lasted %.0f ms (window 2, and 10,000 ms to the frame, wanted)", g.waitWin, g.waitMs);
    if (g.turns > 1) Err("LW1: %d transitions in 150 windows (one, the engage, wanted)", g.turns);
    if (g.engageFrame >= 0 && g.first1s < 84.0) Err("LW1: THE ENGAGE'S FIRST SECOND %.1f of 100 shown (84 or more wanted: THE BRIEF'S STOP RULE)", g.first1s);
    const Lw1Leg t = Lw1Run(40, 0);
    printf("  AT THE TABLE'S TOP (67): %d of 40 windows stood by with FGC_FAST (the game by itself at %.1f in the last), engaged behind window %d\n",
        t.fastWins, t.w.empty() || t.w.back().game <= 0 ? 0.0 : 1000.0 / t.w.back().game, t.engageWin);
    if (t.fastWins != 40 || t.engageWin >= 0) Err("LW1: at the table's top %d of 40 windows stood by with FGC_FAST, engaged behind window %d (40 and never wanted: a game that fast by itself is the plan's D6)", t.fastWins, t.engageWin);
    if (t.f.order || t.f.real1 != t.f.n) Err("LW1 at the table's top: %d picture(s) out of place, %d of %d real frames shown exactly once", t.f.order, t.f.real1, t.f.n);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW9, LW12, LW13: THE ADDED TIME ON THE CARD (the main session's round 2)
// THE MODEL (fg_cond.h, FgDrivePredX10): R_on = 1 / (1/R + c), c in ms a real frame, measured by every stint over its windows 2 to 6, filed by
// what the card was standing by at the engage (the limit, idle room, or cannot tell), remembered until a Reset or a build; none measured is
// optimistic (c = 0). WHAT THE CASES LAY ON THE GAME: frame generation's cost as time added to every engaged frame (a stand-in, said: the
// fake's card does not slow the game standing by, LW1 says why, and the law reads nothing but the rates); and THE CARD'S READING STANDING BY
// as the case's own (the product reads it from the latency waiter's median in flight and the latency pacer's sleeps, fg_cond.h, FgDriveCardOf,
// which this harness does not run). Five minutes of the simulation's clock, the law from standing by.
struct Lw9Leg { std::vector<LwRec> w; LwFrames f; int turns, engages, downs, endState, onslowWins, turnsAfter2; double lastOn, mins; int cLimUs, cRoomUs, lastLawPerUs; };
// (ROUND 3: FRAME GENERATION'S COST IS NOW TIME WHERE A CARD THAT IS THE LIMIT HOLDS THE GAME, not CPU time of the frame: the device thread's
// one-frame-ahead wait inside the original Present (devthread/dt_res.h, DT_R_PACE: dtScPresent calls LatOnPresent right behind it). This
// harness's LatOnPresent hook advances the simulation's clock by addMs there, on engaged presents alone. So the pacer's law sees what it sees
// on a real busy card, the game's own CPU frame with the card's wait left out (its free sample takes the time inside Present out), the real
// frame runs at the game's own plus addMs, and the generated frames are shown as ever. A first form laid the time on the fake's card queue
// (its post chain): it starved every generated frame, which no leg showed (t70.2 showed 99 of 100), and was dropped. addMs 0: a free card.)
static double gLwAddMs = 0.0;
static void LwAddInPresent()
{
    if (gLwAddMs <= 0.0 || !gRt.on || B::S().wt.want == FG_WANT_PASS) return;
    const LONGLONG until = B::clockNow + RtMs(gLwAddMs);
    RtRun(until, 2);
    if (until > B::clockNow) B::clockNow = until;
}
static RtPar Lw9Par(double aloneMs, double addMs)
{
    (void)aloneMs; gLwAddMs = addMs; gLatOnPresentHook = addMs > 0.0 ? &LwAddInPresent : nullptr;
    RtPar p;
    return p;
}
static void Lw9ParEnd() { gLwAddMs = 0.0; gLatOnPresentHook = nullptr; }

static Lw9Leg Lw9Run(double aloneMs, double addMs, int card, int noMemory, double spread = 0.03)
{
    Lw9Leg g = {};
    const RtPar par = Lw9Par(aloneMs, addMs);
    RtBegin(par);
    gRtSeed = 20261017u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = card;
    const unsigned t0 = LwNowMs();
    while (LwNowMs() - t0 < 300000u && g.w.size() < 400)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            RtFrame(RtMs(aloneMs * RtSpread(spread)));
            LwLawPoll(L);
        }
        if (noMemory) { L.mem.cLimUs = -1; L.mem.cRoomUs = -1; }   // (THE CONTROL: a law that learns nothing from a stint)
        g.w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (g.w.back().row == FGC_ONSLOW) g.onslowWins++;
        if (g.w.back().state == FGDR_ON && g.w.back().med > 0) { g.lastOn = 1000.0 / g.w.back().med; g.lastLawPerUs = gLwLawPerUs; }
        if (L.mem.engages + L.mem.downs > 2 && g.w.back().state != g.w.back().prev) g.turnsAfter2++;
    }
    g.mins = (LwNowMs() - t0) / 60000.0;
    g.turns = L.mem.engages + L.mem.downs; g.engages = L.mem.engages; g.downs = L.mem.downs; g.endState = L.mem.state; g.cLimUs = L.mem.cLimUs; g.cRoomUs = L.mem.cRoomUs;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW9");
    return g;
}
static void Lw9Say(const char* what, const Lw9Leg& g)
{
    LwPrint(what, g.w);
    printf("  %.1f minutes, %d windows: transitions %d (engages %d, stand-downs %d), the state at the end %s, windows standing by for FGC_ONSLOW %d, R_on in the last engaged window %.1f; c measured: on a card that was the limit %d us, with room %d us; %d frames, real frames shown once %d, out of place %d\n",
        g.mins, (int)g.w.size(), g.turns, g.engages, g.downs, FgDriveStateName(g.endState), g.onslowWins, g.lastOn, g.cLimUs, g.cRoomUs, g.f.n, g.f.real1, g.f.order);
}
static bool Lw9FramesOk(const char* tag, const Lw9Leg& g)
{
    if (g.f.order || g.f.real1 != g.f.n) { Err("%s: %d picture(s) out of place, %d of %d real frames shown exactly once", tag, g.f.order, g.f.real1, g.f.n); return false; }
    return true;
}
// LW9: A BUSY CARD WITH c = 6.5 ms (t70.2's: 74.5 by itself, 50.1 engaged). By itself at 52: R_on 38.9, over SLOW's 37: the optimistic first
// engage stays; at most 1 transition. By itself at 60: R_on 43.2: engages and stays.
static void CaseLW9()
{
    const char* const name = "LW9 a busy card adding 6.5 ms a real frame: by itself at 52 at most 1 transition in 5 minutes; at 60 it engages and stays (2026-10-17, phase 5, round 2)";
    CaseBegin(name);
    const Lw9Leg a = Lw9Run(1000.0 / 52.0, 6.5, 1, 0);
    Lw9Say("BY ITSELF AT 52, THE CARD THE LIMIT, 6.5 ms ADDED ENGAGED:", a);
    // ("engage once, then hold off" is allowed (round 2): 0 or 1 transitions, or 1 engage and 1 stand-down and nothing behind them. Which of
    // the two happens on this base turns on the pacer's headroom: P2's head of 1.0 keeps R_on over 37, the head of 1.06 here takes it under)
    if (a.mins < 5.0 || !(a.turns <= 1 || (a.engages == 1 && a.downs == 1 && a.turnsAfter2 == 0 && a.endState != FGDR_ON))) Err("LW9: by itself at 52, %d transitions (%d engages, %d stand-downs, %d behind them) in %.1f minutes (at most 1, or engage once and then hold off, in 5 wanted)", a.turns, a.engages, a.downs, a.turnsAfter2, a.mins);
    Lw9FramesOk("LW9 at 52", a);
    const Lw9Leg b = Lw9Run(1000.0 / 60.0, 6.5, 1, 0);
    Lw9Say("BY ITSELF AT 60, THE CARD THE LIMIT, 6.5 ms ADDED ENGAGED:", b);
    if (b.engages != 1 || b.turns != 1 || b.endState != FGDR_ON || b.lastOn < 37.0) Err("LW9: by itself at 60, %d engage(s), %d transition(s), %s at the end, R_on %.1f (1, 1, ON and over SLOW's 37 wanted: 43.2 with P2's pacer, about 39.5 with this base's headroom)", b.engages, b.turns, FgDriveStateName(b.endState), b.lastOn);
    if (b.cLimUs < 6000 || b.cLimUs > 9500) Err("LW9: at 60 the stint measured c %d us on a card that was the limit (6,000 to 9,500 wanted: 6,500 laid on)", b.cLimUs);   // (round 4: restored; round 3's rule read 3,807)
    Lw9FramesOk("LW9 at 60", b);
    CaseEnd(name);
}
// LW12: A FREE CARD (idle room standing by; frame generation's work fits in the idle: nothing added). By itself at 45, 50 and 55: each
// engages and stays, R_on within 1.5 of the game's own rate. THE 1.5 IS JUDGED WITH P2'S PACER (head 1.0); ON THIS BASE THE PACER'S HEAD OF
// 1.06 TAKES ABOUT 6 PERCENT, so it is TOLD here and judged where LW_P2_MERGED is defined (the main session sets it at the merge).
static void CaseLW12()
{
    const char* const name = "LW12 a free card: by itself at 45, 50 and 55 frame generation engages and stays, R_on within 1.5 of the game's own (2026-10-17, phase 5, round 2)";
    CaseBegin(name);
    static const double fps[3] = { 45.0, 50.0, 55.0 };
    for (int i = 0; i < 3; i++)
    {
        char what[96]; _snprintf_s(what, sizeof(what), _TRUNCATE, "BY ITSELF AT %.0f, THE CARD WITH IDLE ROOM, NOTHING ADDED:", fps[i]);
        const Lw9Leg g = Lw9Run(1000.0 / fps[i], 0.0, 0, 0, 0.02);
        Lw9Say(what, g);
        if (g.engages != 1 || g.turns != 1 || g.endState != FGDR_ON) Err("LW12: by itself at %.0f, %d engage(s), %d transition(s), %s at the end (1, 1, ON wanted)", fps[i], g.engages, g.turns, FgDriveStateName(g.endState));
#if LW_P2_MERGED
        // (ROUND 3: what frame generation may cost on a free card is held against THE PACER'S OWN CHOICE, not the game's rate: at 120 Hz P2's law
        // paces a game between the floor and the free range's start AT that start (20 ms: 50 a second), the gap the owner's eye decides (leg 7),
        // and that is not frame generation's cost. So R_on within 1.5 of the lower of the game's own rate and the law's own period's rate.)
        const double allow = g.lastLawPerUs > 0 && 1000000.0 / g.lastLawPerUs < fps[i] ? 1000000.0 / g.lastLawPerUs : fps[i];
        printf("  by itself at %.0f: R_on %.1f; the law's own period %d us (%.1f a second): frame generation may run it at %.1f\n", fps[i], g.lastOn, g.lastLawPerUs, g.lastLawPerUs > 0 ? 1000000.0 / g.lastLawPerUs : 0.0, allow);
        if (fabs(g.lastOn - allow) > 1.5) Err("LW12: by itself at %.0f, R_on %.1f against %.1f, the lower of its own and the law's period's (within 1.5 wanted)", fps[i], g.lastOn, allow);
        if (g.cRoomUs > 300) Err("LW12: by itself at %.0f, c with room %d us (300 at the most wanted: frame generation fits in the idle)", fps[i], g.cRoomUs);
#else
        printf("  TOLD (judged with P2's pacer, LW_P2_MERGED): by itself at %.0f, R_on %.1f, %.1f under\n", fps[i], g.lastOn, fps[i] - g.lastOn);
#endif
        Lw9FramesOk("LW12", g);
    }
    CaseEnd(name);
}
// LW13: NO STAMPS AND NO CARD READING (the mod cannot tell), by itself at 52 on a busy card that adds 8.5 ms (R_on 36, under SLOW's 37). The
// optimistic first engage, SLOW stands it down, the stint's c is remembered, and FGC_ONSLOW keeps it by: 1 engage, 1 stand-down, then 0
// transitions for the rest of the five minutes. CONTROL, A LAW THAT LEARNS NOTHING FROM A STINT: it engages again behind every rest and
// back-off, the flap the player sees.
static void CaseLW13()
{
    const char* const name = "LW13 no stamps, by itself at 52 on a busy card (R_on 36): 1 engage, 1 stand-down, then 0 transitions in 5 minutes, c remembered (2026-10-17, phase 5, round 2)";
    CaseBegin(name);
    const Lw9Leg g = Lw9Run(1000.0 / 52.0, 8.5, -1, 0);
    Lw9Say("BY ITSELF AT 52, THE CARD CANNOT BE READ, 8.5 ms ADDED ENGAGED:", g);
    if (g.engages != 1 || g.downs != 1 || g.turnsAfter2 != 0 || g.endState == FGDR_ON || g.mins < 5.0) Err("LW13: %d engage(s), %d stand-down(s), %d transitions behind them, %s at the end, %.1f minutes (1, 1, 0, not ON, 5 wanted)", g.engages, g.downs, g.turnsAfter2, FgDriveStateName(g.endState), g.mins);
    if (g.cLimUs < 7000 || g.cLimUs > 11000) Err("LW13: c remembered %d us (about 8,500 wanted)", g.cLimUs);
    Lw9FramesOk("LW13", g);
    const RingStats stc = gRingSt;
    const Lw9Leg c = Lw9Run(1000.0 / 52.0, 8.5, -1, 1);
    ViolRestore(stc);
    Lw9Say("CONTROL, A LAW THAT LEARNS NOTHING FROM A STINT:", c);
    if (c.turns <= 2) Err("LW13 control: a law that learns nothing made %d transition(s) in %.1f minutes (more than two wanted: the flap): the control shows nothing", c.turns, c.mins);
    Ctl("a law that learns nothing: %d transitions in %.1f minutes, engaged at 36 and stood down for FGC_SLOW by turns", c.turns, c.mins);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW14, FusionFix's limiter changed in its own menu (round 2, H(b))
// WHAT IT SHOWS. THE WATCH ITSELF (source/fg/fg_ffwatch.h, the product's code) on real files in a folder of %TEMP%: a FusionFix cfg (5.1.0's
// numbering) and its ini, the limiter OFF; a game of 62 by itself, the law from standing by. Once a window the watch asks the two write times
// and the conditions are handed its copy (fg_arm_life.h, FgpLawFfApply). In window 20 THE PLAYER SETS CUSTOM 58 (the cfg's preset to Custom, the
// ini's FpsLimit to 58), and FusionFix's limiter is faked in the game's present from then on: FGC_FFCAP and the stand-down within 2 windows.
// In window 40 THE LIMIT IS LIFTED (FpsLimit 0): engaged again behind the dwell and the wait for a cut. And the rule's table, held to
// LatReadLimiterCfg's list for both numberings.
static bool Lw14Write(const char* path, const char* text)
{
    FILE* f = nullptr; if (fopen_s(&f, path, "wb") || !f) return false;
    fputs(text, f); fclose(f); return true;
}
static void CaseLW14()
{
    const char* const name = "LW14 FusionFix's limiter changed in its own menu: written at 58 it stands the law by within 2 windows, lifted it engages again behind its dwell (2026-10-17, phase 5, round 2)";
    CaseBegin(name);
    // (the rule's table, both numberings, and a Custom limit with and without its ini)
    {
        struct { int preset, ff51, seen, limit, cap, rel; } t[] = { { 0, 1, 0, 0, 0, 0 }, { 2, 1, 0, 0, 30, 0 }, { 7, 1, 0, 0, 100, 0 }, { 12, 1, 0, 0, 240, 0 }, { 99, 1, 0, 0, 240, 0 },
            { 1, 1, 1, 58, 58, 0 }, { 1, 1, 1, 0, 0, 0 }, { 1, 1, 1, -3, 0, 3 }, { 1, 1, 0, 0, 0, 1 }, { 2, 0, 0, 0, 0, 0 }, { 3, 0, 1, 58, 58, 0 }, { 9, 0, 0, 0, 100, 0 }, { -1, 1, 0, 0, 0, 0 } };
        int bad = 0;
        for (auto& x : t) { int c = 0, r = 0; FgFfFromPreset(x.preset, x.ff51, x.seen, x.limit, &c, &r); if (c != x.cap || r != x.rel) { bad++; printf("  THE RULE: preset %d (5.1 %d), ini %d at %d: %d fps, Custom -%d (%d and %d wanted)\n", x.preset, x.ff51, x.seen, x.limit, c, r, x.cap, x.rel); } }
        if (bad) Err("LW14: the rule's table is not LatReadLimiterCfg's in %d place(s)", bad);
    }
    char dir[MAX_PATH]; GetTempPathA(MAX_PATH, dir); strcat_s(dir, "dlssiv-lw14\\"); CreateDirectoryA(dir, nullptr);
    FgFfWatch w = {};
    _snprintf_s(w.cfg, MAX_PATH, _TRUNCATE, "%sGTAIV.EFLC.FusionFix.cfg", dir); _snprintf_s(w.ini, MAX_PATH, _TRUNCATE, "%sGTAIV.EFLC.FusionFix.ini", dir); w.ff51 = 1;
    if (!Lw14Write(w.cfg, "[FRAMELIMIT]\r\nFpsLimitPreset=0\r\n") || !Lw14Write(w.ini, "[FRAMELIMIT]\r\nFpsLimit=0\r\n")) { Err("LW14: the files could not be written in %s", dir); CaseEnd(name); return; }
    const int started = FgFfWatchStart(w, 0, 0);
    // (ROUND 3: A READ TAKEN WHILE THE PLAYER'S MENU WRITES IS NOT TAKEN: the write times taken as the watch's step takes them, the ini written,
    // then the read: refused, the times forgotten; the next step reads again and takes it. Then the files as they were, and the watch's times)
    int tornRefused = 0, tornForgot = 0, tornRetaken = 0;
    {
        FgFfWatch t = w;
        FgFfTimeMoved(t.cfg, &t.cfgT, &t.haveCfgT); FgFfTimeMoved(t.ini, &t.iniT, &t.haveIniT);
        Sleep(20); Lw14Write(t.ini, "[FRAMELIMIT]\r\nFpsLimit=77\r\n");
        const LONG before = t.packed; const int changes0 = t.changes;
        tornRefused = FgFfRead(t) == 0 && t.packed == before && t.changes == changes0;
        tornForgot = !t.haveCfgT && !t.haveIniT;
        FgFfWatchStep(t); tornRetaken = t.haveCfgT && t.haveIniT;
        Sleep(20); Lw14Write(w.ini, "[FRAMELIMIT]\r\nFpsLimit=0\r\n");
        FgFfWatchStep(w);
    }
    printf("  a read taken while the ini was written: refused %s, its write times forgotten %s, taken at the next step %s\n", tornRefused ? "yes" : "NO", tornForgot ? "yes" : "NO", tornRetaken ? "yes" : "NO");
    if (!tornRefused || !tornForgot || !tornRetaken) Err("LW14: a read taken mid-write: refused %d, forgotten %d, retaken %d (each 1 wanted)", tornRefused, tornForgot, tornRetaken);
    RtPar par;
    RtBegin(par);
    gRtSeed = 20261017u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.card = 0;
    std::vector<LwRec> rec; int standAt = -1, ffcapAt = -1, backAt = -1, onBefore = 0, lastChange = 0;
    for (int win = 0; win < 70; win++)
    {
        if (win == 20) { Sleep(20); Lw14Write(w.cfg, "[FRAMELIMIT]\r\nFpsLimitPreset=1\r\n"); Lw14Write(w.ini, "[FRAMELIMIT]\r\nFpsLimit=58\r\n"); }
        if (win == 40) { Sleep(20); Lw14Write(w.ini, "[FRAMELIMIT]\r\nFpsLimit=0\r\n"); }
        // (THE WATCH'S STEP, as the law's window step runs it ahead of the next conditions' step)
        if (FgFfWatchStep(w)) lastChange = win;
        { const LONG p = w.packed; L.ffCap = (int)(p & 0xFFFF); }   // (round 3: the published word, as FgpLawFfApply reads it)
        B::limiter = L.ffCap > 0; B::ffCap = L.ffCap;
        const LONGLONG capTicks = L.ffCap > 0 ? RtMs(1000.0 / L.ffCap) : 0;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            LONGLONG cost = RtMs(1000.0 / 62.0 * RtSpread(0.03));
            if (capTicks && !gRt.pairs.empty()) { const LONGLONG due = gRt.pairs.back().tEntry + capTicks, at = B::clockNow + cost; if (due > at) cost += due - at; }
            RtFrame(cost); LwLawPoll(L);
        }
        rec.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        const LwRec& r = rec.back();
        if (win < 20 && r.state == FGDR_ON) onBefore++;
        if (win >= 20 && ffcapAt < 0 && r.row == FGC_FFCAP) ffcapAt = win;
        if (win >= 20 && standAt < 0 && r.prev == FGDR_ON && r.state != FGDR_ON) standAt = win;
        if (win >= 40 && backAt < 0 && r.state == FGDR_ON) backAt = win;
    }
    const LwFrames f = LwFramesRead();
    B::limiter = false; B::ffCap = 0;
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW14");
    LwPrint("THE WATCH ON FILES IN %TEMP%: OFF, CUSTOM 58 IN WINDOW 20, LIFTED IN WINDOW 40:", rec);
    printf("  the watch started %s; changes read %d, the last in window %d; engaged windows before the write %d; FGC_FFCAP from window %d, the stand-down behind window %d; engaged again behind window %d; %d frames, real frames shown once %d, out of place %d\n",
        started ? "yes" : "NO", w.changes, lastChange, onBefore, ffcapAt, standAt, backAt, f.n, f.real1, f.order);
    if (!started || w.changes != 2) Err("LW14: the watch started %d and read %d change(s) (yes and 2 wanted)", started, w.changes);
    if (onBefore < 5 || ffcapAt < 20 || ffcapAt > 21 || standAt < 20 || standAt > 21) Err("LW14: engaged windows before the write %d, FGC_FFCAP from window %d, the stand-down behind window %d (5 or more, and 20 or 21 for both: within 2 windows of the write)", onBefore, ffcapAt, standAt);
    // (lifted: the dwell's three windows in band, then the wait for a cut, 10 s with none: at 62 about five windows)
    if (backAt < 0 || backAt > 52) Err("LW14: engaged again behind window %d (by window 52 wanted: the dwell and the wait behind the lift in window 40)", backAt);
    if (f.order || f.real1 != f.n) Err("LW14: %d picture(s) out of place, %d of %d real frames shown exactly once", f.order, f.real1, f.n);
    DeleteFileA(w.cfg); DeleteFileA(w.ini); RemoveDirectoryA(dir);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW15, a stint paced over the game's own rate leaves c near 0 (round 3)
// WHAT IT SHOWS. The 120 Hz panel, a free card (Lw9Par's idle room), a game of 55 by itself whose frames spread by 10 percent: the law paces
// it at its dearest frames (L, the 98th of the last 4 s: about 19.9 ms), so the real rate engaged is about 50 while frame generation costs
// the card nothing. THE REVIEWER'S FINDING (round 3, with the old gap's 20 ms in the law's place): the stint filed c = 1,442 us "with room",
// the pacer's choice taken for frame generation's cost, and a later scene at 42 predicted 39.6 and was refused for the session. NOW c IS ONLY
// THE TIME BEYOND THE LAW'S OWN PERIOD: near 0. Then NVIDIA's cap for two windows (a settings row: standing by at once) and the game at 42:
// engaged again. (Round 9's LW: the old gap, the key fg_test_free_min_us at 20000, went with the key; the law's own period over the game's
// median is the spread's, so the case keeps a pacer's choice for the control to take.)
// CONTROL, THE ROUND-2 RULE (the law's period not taken out): c about 1,600 us (1,581 on round 9's run), the scene at 42 refused with
// FGC_ONSLOW.
struct Lw15Leg { int cRoom, engages, onslow42, on42; std::vector<LwRec> w; LwFrames f; };
static Lw15Leg Lw15Run(int noLawPer)
{
    Lw15Leg g = {};
    RtPar par = Lw9Par(1000.0 / 55.0, 0.0);
    RtBegin(par);
    gRtSeed = 20261017u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = 0; L.noLawPer = noLawPer;
    for (int win = 0; win < 64; win++)
    {
        const double fps = win < 30 ? 55.0 : 42.0;
        L.nvCap = (win == 30 || win == 31) ? 116 : 0;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(1000.0 / fps * RtSpread(win < 30 ? 0.10 : 0.02))); LwLawPoll(L); }
        g.w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (win == 29) g.cRoom = L.mem.cRoomUs;
        if (win >= 34 && g.w.back().row == FGC_ONSLOW) g.onslow42++;
        if (win >= 34 && g.w.back().state == FGDR_ON) g.on42++;
    }
    g.engages = L.mem.engages; g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW15");
    return g;
}
static void CaseLW15()
{
    const char* const name = "LW15 a stint paced over the game's own rate leaves c near 0, and a later scene at 42 engages; the round-2 rule refused it (2026-10-17, phase 5, round 3)";
    CaseBegin(name);
    const Lw15Leg g = Lw15Run(0);
    LwPrint("55 BY ITSELF ON A FREE CARD, A SPREAD OF 10 PERCENT (PACED AT ABOUT 50), NVIDIA'S CAP FOR TWO WINDOWS, THEN 42:", g.w);
    printf("  c measured with room %d us; engages %d; windows at 42 standing by for FGC_ONSLOW %d, engaged %d; %d frames, real frames shown once %d, out of place %d\n",
        g.cRoom, g.engages, g.onslow42, g.on42, g.f.n, g.f.real1, g.f.order);
    if (g.cRoom < 0 || g.cRoom > 300) Err("LW15: the stint at the free range's start filed c %d us with room (300 at the most wanted: the pacer's choice is never cost)", g.cRoom);
    if (g.engages != 2 || g.onslow42 || g.on42 < 20) Err("LW15: engages %d, FGC_ONSLOW windows at 42 %d, engaged windows at 42 %d (2, 0 and 20 or more wanted)", g.engages, g.onslow42, g.on42);
    if (g.f.order || g.f.real1 != g.f.n) Err("LW15: %d picture(s) out of place, %d of %d real frames shown exactly once", g.f.order, g.f.real1, g.f.n);
    const RingStats stc = gRingSt;
    const Lw15Leg c = Lw15Run(1);
    ViolRestore(stc);
    LwPrint("CONTROL, THE ROUND-2 RULE (the law's period not taken out of c):", c.w);
    if (c.cRoom < 1000 || c.onslow42 < 20) Err("LW15 control: the round-2 rule filed c %d us and stood the scene at 42 by for FGC_ONSLOW in %d windows (about 1,600 and 20 or more wanted): the control shows nothing", c.cRoom, c.onslow42);
    Ctl("the round-2 rule: c %d us with room, the scene at 42 refused with FGC_ONSLOW in %d windows", c.cRoom, c.onslow42);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW16, an engage at a menu: nothing generated is not c (round 3)
// WHAT IT SHOWS. A free card, a game of 45 by itself (paced at its own 22.2 ms: no gap to take for cost). At the engage A MENU COMES UP FOR NINE
// WINDOWS (the present path stands by: the binding's mayEngage at 0, nothing generated) and runs at 40; then the game again. The stint's
// windows that measure c all fall in the menu (round 5: its windows 4 to 8, behind the warm-up; nine windows of menu, six before it): NO c IS LEARNT (none, not the menu's 25 ms against gameplay's 22.2). CONTROL, THE ROUND-2 RULE (any
// window a sample, FgDriveK::costNeedShown 0): c about 2,800 us from the menu's rate.
struct Lw16Leg { int cRoom, menuWins; std::vector<LwRec> w; LwFrames f; };
static Lw16Leg Lw16Run(int needShown)
{
    Lw16Leg g = {};
    RtPar par = Lw9Par(1000.0 / 45.0, 0.0);
    RtBegin(par);
    gRtSeed = 20261017u;
    FgDriveK k = FgDriveDefaults(); k.costNeedShown = needShown;
    LwLaw L; LwLawBegin(L, k, FgCondDefaults()); L.card = 0;
    int menuLeft = -1;
    for (int win = 0; win < 40; win++)
    {
        if (menuLeft < 0 && L.mem.state == FGDR_ON) menuLeft = 9;
        const bool menu = menuLeft > 0;
        _InterlockedExchange(&ShB::mayEngage, menu ? 0 : 1);
        if (menu) g.menuWins++;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs((menu ? 25.0 : 1000.0 / 45.0) * RtSpread(0.02))); LwLawPoll(L); if (menuLeft < 0 && L.mem.state == FGDR_ON) { menuLeft = 9; _InterlockedExchange(&ShB::mayEngage, 0); } }
        g.w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (menuLeft > 0) menuLeft--;
    }
    _InterlockedExchange(&ShB::mayEngage, 1);
    g.cRoom = L.mem.cRoomUs; g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW16");
    return g;
}
static void CaseLW16()
{
    const char* const name = "LW16 an engage at a menu: the stint's windows that generated nothing are not c (2026-10-17, phase 5, round 3)";
    CaseBegin(name);
    const Lw16Leg g = Lw16Run(1);
    LwPrint("45 BY ITSELF ON A FREE CARD, A MENU AT 40 FOR NINE WINDOWS FROM THE ENGAGE:", g.w);
    printf("  menu windows %d; c with room %d us (-1: none learnt); %d frames, real frames shown once %d, out of place %d\n", g.menuWins, g.cRoom, g.f.n, g.f.real1, g.f.order);
    if (g.menuWins < 8 || g.cRoom != -1) Err("LW16: %d menu window(s), c with room %d us (8 or more whole windows of it, and none learnt from a menu, wanted)", g.menuWins, g.cRoom);
    if (g.f.order || g.f.real1 != g.f.n) Err("LW16: %d picture(s) out of place, %d of %d real frames shown exactly once", g.f.order, g.f.real1, g.f.n);
    const RingStats stc = gRingSt;
    const Lw16Leg c = Lw16Run(0);
    ViolRestore(stc);
    LwPrint("CONTROL, THE ROUND-2 RULE (any window of the stint a sample):", c.w);
    if (c.cRoom < 1500) Err("LW16 control: the round-2 rule learnt c %d us from the menu's windows (about 2,800 wanted): the control shows nothing", c.cRoom);
    Ctl("the round-2 rule: c %d us learnt from a menu's windows", c.cRoom);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW11, R_free on a card that is the limit
// WHAT IT SHOWS. RL8's shape ENGAGED (the law put in ON at the run's start, as an engage would have: the fake standing by is not this leg's
// game, LW1 says why), at the table's own top, for 30 windows. The card holds the game at 43 to 51 real frames a second; the pacer's due
// has passed on nearly every present, so the law reads the measured rate: in band, nothing stands, no stand-down.
// CONTROL, THE BRIEF'S FIRST FORM (R_free as the pacer's law E alone, LW_E): E leaves out the time inside the original Present, where the
// card holds the game, and reads the CPU's frame: about 90 a second, over the top's 70 engaged, and the law stands the busy card down.
struct Lw11Leg { std::vector<LwRec> w; LwFrames f; int downs, downWin, downRow; int perUs; };
static Lw11Leg Lw11Run(int how)
{
    Lw11Leg g = {};
    const RlLeg l = LwRl8();
    gRlNow = RL_NOW_ALL; const RtPar par = RlPar(l, 0); gRlNow = 0;
    RtBegin(par);
    gRtSeed = 20261014u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults(), how);
    FgDriveOn(L.mem, L.k, FGDRW_AT_ONCE); B::S().wt.want = FG_WANT_ON;   // (engaged from the run's start)
    g.downWin = -1; g.downRow = FGC_OK;
    for (int win = 0; win < 30; win++)
    {
        const int first = (int)gRt.pairs.size();
        LwShapeFrames(par, l, first, 120);
        g.w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        g.perUs = gLwPerUs;
        if (g.downWin < 0 && g.w.back().prev == FGDR_ON && g.w.back().state != FGDR_ON) { g.downWin = win; g.downRow = g.w.back().row; }
        if (L.mem.state != FGDR_ON) break;
    }
    g.downs = L.mem.downs; g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW11");
    return g;
}
static void CaseLW11()
{
    const char* const name = "LW11 R_free on a card that is the limit: RL8's shape engaged reads its measured rate and stays engaged; E alone would stand it down (2026-10-17, phase 5)";
    CaseBegin(name);
    const Lw11Leg g = Lw11Run(LW_NOW);
    LwPrint("R_FREE AS BUILT (E where the pacer holds the game, the measured rate where the game holds itself):", g.w);
    printf("  stand-downs %d; FgPace's own period at the end %d us; %d frames, real frames shown once %d, out of place %d\n", g.downs, g.perUs, g.f.n, g.f.real1, g.f.order);
    if (g.downs != 0 || g.w.size() != 30) Err("LW11: %d stand-down(s), the first behind window %d for %s (none in 30 windows wanted)", g.downs, g.downWin, FgCondEnumName(g.downRow));
    if (g.f.order || g.f.real1 != g.f.n) Err("LW11: %d picture(s) out of place, %d of %d real frames shown exactly once", g.f.order, g.f.real1, g.f.n);
    for (size_t i = 2; i < g.w.size(); i++) if (g.w[i].rFree > 600) { Err("LW11: window %d read R_free %.1f for a game running %.1f real", (int)i, g.w[i].rFree / 10.0, g.w[i].med > 0 ? 1000.0 / g.w[i].med : 0.0); break; }
    const RingStats stc = gRingSt;
    const Lw11Leg c = Lw11Run(LW_E);
    ViolRestore(stc);
    LwPrint("CONTROL, R_FREE AS E ALONE (the brief's first form):", c.w);
    if (c.downs < 1 || c.downRow != FGC_FAST) Err("LW11 control: E alone stood the busy card down %d time(s) (%s) (once, for FGC_FAST, wanted): the control shows nothing", c.downs, FgCondEnumName(c.downRow));
    Ctl("E alone read %.1f for a game running %.1f real and stood it down behind window %d for %s", c.w.empty() ? 0.0 : c.w.back().rE / 10.0, c.w.empty() || c.w.back().med <= 0 ? 0.0 : 1000.0 / c.w.back().med, c.downWin, FgCondEnumName(c.downRow));
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW3, a route that crosses the floor both ways
// WHAT IT SHOWS. The 120 Hz panel, the law from standing by, a game of 5 percent spread on the simulation's clock for five and a half minutes:
// 62 by itself for ten windows (an engage, which measures the cost factor: the pacer holds it at 59.1), then heavier, window by window, to
// 35 over twenty, held there twenty, lighter to 50 over twenty, held to the end. THE PLAN'S LW3: at most 4 transitions in any five
// minutes, never a lost or doubled real frame (FQ4's: every real frame shown once, in order); and the law's states where they belong:
// engaged at 62, standing by while the game by itself sits at 35, engaged again at 50.
static void CaseLW3()
{
    const char* const name = "LW3 a route crossing the floor both ways: at most 4 transitions in five minutes, every real frame once and in order (2026-10-17, phase 5)";
    CaseBegin(name);
    RtPar par;
    RtBegin(par);
    gRtSeed = 20261017u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults());
    std::vector<LwRec> w; std::vector<double> ms;
    const unsigned t0 = LwNowMs();
    int onAt35 = 0, win = 0;
    while (LwNowMs() - t0 < 330000u && win < 400)
    {
        const double fps = win < 10 ? 62.0 : win < 30 ? 62.0 - (62.0 - 35.0) * (win - 10) / 20.0 : win < 50 ? 35.0 : win < 70 ? 35.0 + 15.0 * (win - 50) / 20.0 : 50.0;
        ms.push_back(fps);
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(1000.0 / fps * RtSpread(0.05))); LwLawPoll(L); }
        w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (win >= 32 && win < 50 && w.back().state == FGDR_ON) onAt35++;
        win++;
    }
    const LwFrames f = LwFramesRead(); const int turns5 = LwTurnsIn5Min(L), turns = L.mem.engages + L.mem.downs, endState = L.mem.state, onAt62 = w.size() > 9 && w[9].state == FGDR_ON;
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW3");
    LwPrint("THE ROUTE (62, down to 35, held, up to 50, held):", w);
    printf("  %d windows; transitions %d in all, at most %d in five minutes; engaged in window 9 at 62: %s; engaged windows while the game sat at 35: %d; at the end %s; %d frames, real frames shown once %d, out of place %d\n",
        (int)w.size(), turns, turns5, onAt62 ? "yes" : "NO", onAt35, FgDriveStateName(endState), f.n, f.real1, f.order);
    if (turns5 > 4) Err("LW3: %d transitions in five minutes (4 at the most wanted)", turns5);
    if (f.order || f.real1 != f.n) Err("LW3: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", f.order, f.first.c_str(), f.real1, f.n);
    if (!onAt62 || onAt35 || endState != FGDR_ON) Err("LW3: engaged at 62 %s, engaged windows at 35 %d, at the end %s (yes, 0 and ON wanted)", onAt62 ? "yes" : "no", onAt35, FgDriveStateName(endState));
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW4, the health exit: a helper late on every pair
// WHAT IT SHOWS. A game of 62 by itself, the cut wait at 0 (the engage at the dwell's window: this case is about what follows), and THE
// HELPER'S ANSWER 9 MS BEHIND EVERY HANDOFF (RtPar::kUs; over the plan's bound of 8 ms, so no plan reaches it; 30 ms stalled the fake helper, whose events are not built for an answer later than a frame): every generated frame is late and withheld, the shown share far under 90 of 100.
// The law stands down three windows into each stint (FGDRW_SHOWN), rests, returns, and the back-off DOUBLES AND CAPS: the windows from each
// stand-down to the next standing by, 70, 130, 250, 310, 310 (rest 10 and the back-off 60, 120, 240, 300, 300). The resting windows run 12
// frames each (said: nothing in them is generated, and the law reads only their band; the engaged and standing-by windows run 120).
// (ROUND 8, P, 2026-10-25: THE LAW RUNS WITHOUT K HERE, LwLaw::noStamps. With it the first stint reads K 9,125 us, the card cannot be read,
// and FGC_ONSLOW predicts 39.5 for a game of 62: it stays by behind the first rest (70) to the end and never cycles, which is the better
// outcome and not this case's mechanism. The back-off's doubling is proved on the law of before; LW46 to LW50 prove K.)
static void CaseLW4()
{
    const char* const name = "LW4 the health exit: a helper late on every pair stands down, rests, returns; the back-off doubles and caps at 300 (2026-10-17, phase 5)";
    CaseBegin(name);
    RtPar par; par.kUs = 9000;
    RtBegin(par);
    gRtSeed = 20261017u;
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;
    LwLaw L; LwLawBegin(L, k, FgCondDefaults()); L.noStamps = 1;   // (round 8, P: the law of before, said above)
    std::vector<int> rests, whys; int restRun = 0, guard = 0, sickShownMost = 0;
    std::vector<LwRec> w;
    while ((int)rests.size() < 5 && guard++ < 3000)
    {
        const bool resting = L.mem.state == FGDR_REST;
        const int first = (int)gRt.pairs.size(), n = resting ? 12 : 120;
        for (int i = 0; i < n; i++) { RtFrame(RtMs(1000.0 / 62.0 * RtSpread(0.03))); LwLawPoll(L); }
        const int was = L.mem.state;
        w.push_back(LwLawWindow(L, par, first, (int)gRt.pairs.size()));
        if (w.back().state == FGDR_ON && w.back().shownN > 0) { const int pc = 100 * w.back().shownGen / w.back().shownN; if (pc > sickShownMost) sickShownMost = pc; }
        if (was == FGDR_ON && L.mem.state == FGDR_REST) { whys.push_back(L.mem.why); restRun = 0; }
        if (L.mem.state == FGDR_REST) restRun++;
        if (was == FGDR_REST && L.mem.state == FGDR_STANDBY) rests.push_back(restRun);
    }
    const LwFrames f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW4");
    LwPrint("A HELPER LATE ON EVERY PAIR:", w);
    printf("  the rests, stand-down to standing by again:");
    for (size_t i = 0; i < rests.size(); i++) printf(" %d (%s)", rests[i], i < whys.size() ? FgDriveWhyName(whys[i]) : "?");
    printf("; the most generated frames shown of 100 in an engaged window %d; %d frames, real frames shown once %d, out of place %d\n", sickShownMost, f.n, f.real1, f.order);
    static const int want[5] = { 70, 130, 250, 310, 310 };
    bool ok = rests.size() == 5;
    for (size_t i = 0; ok && i < 5; i++) ok = rests[i] == want[i] && whys[i] == FGDRW_SHOWN;
    if (!ok) Err("LW4: the rests were not 70, 130, 250, 310, 310 windows, each behind FGDRW_SHOWN (see the line above)");
    if (f.order || f.real1 != f.n) Err("LW4: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", f.order, f.first.c_str(), f.real1, f.n);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW5, a Reset mid-rest clears the rest
// WHAT IT SHOWS. The product's reader of "a Reset or a build" is the layer's arm count (dtArmCount): it is read here across A REAL RESET of
// the harness's device (the device thread disarms, and arms again behind 300 presents), and the law, put in REST by a short stint's health
// exit, is stepped with that reading: STANDBY, FGDRW_CLEARED, the rest, the back-off and the back-off's memory gone.
// CONTROL: the same reading with no Reset between: still REST.
static void CaseLW5()
{
    const char* const name = "LW5 a Reset mid-rest clears the rest: the arm count read across a real Reset of the device, the law in REST goes to STANDBY (2026-10-17, phase 5)";
    CaseBegin(name);
    FgDriveK k = FgDriveDefaults();
    auto restNow = [&](FgDriveMem& m)
    {
        FgDriveInit(m, k);
        FgDriveIn in = {}; in.row = FGC_OK; in.band = FGDB_IN; in.cpuReady = 1;
        unsigned now = 1000;
        for (int i = 0; i < 3; i++) { now += 2000; FgDriveStep(m, in, k, now, 0, 0); }
        FgDriveCut(m, k, now + 16, 1, 0);
        in.shownN = 100; in.shownGen = 10;
        for (int i = 0; i < 6; i++) { now += 2000; FgDriveStep(m, in, k, now, 1, 0); }   // (round 5: the warm-up's 3, then the health exit's 3)
        return now;
    };
    Fresh(); Warm();
    FgDriveMem m; unsigned now = restNow(m);
    const LONG arm0 = dtArmCount;
    HANDLE h = Spawn(ResetFrom); Join(h, "LW5's Reset");
    const bool rearmed = EnsureArmed();
    const LONG arm1 = dtArmCount;
    FgDriveIn in = {}; in.row = FGC_OK; in.band = FGDB_IN; in.cpuReady = 1; in.cleared = arm1 != arm0 ? 1 : 0;
    const int restBefore = m.restLeft, backBefore = m.backoffLeft;
    FgDriveStep(m, in, k, now + 2000, 1, 0);
    printf("  in REST with %d window(s) of rest and %d of back-off left; the arm count %ld before the Reset and %ld behind it (re-armed: %s); the law: %s, %s, rest %d, back-off %d, the next %d\n",
        restBefore, backBefore, (long)arm0, (long)arm1, rearmed ? "yes" : "NO", FgDriveStateName(m.state), FgDriveWhyName(m.why), m.restLeft, m.backoffLeft, m.backoffNext);
    if (!rearmed || arm1 == arm0) Err("LW5: the Reset did not move the arm count (%ld, %ld; re-armed %d): the product's reader would not see it", (long)arm0, (long)arm1, rearmed ? 1 : 0);
    if (restBefore != 10 || backBefore != 60) Err("LW5: the law was not in its rest (rest %d, back-off %d: 10 and 60 wanted)", restBefore, backBefore);
    if (m.state != FGDR_STANDBY || m.why != FGDRW_CLEARED || m.restLeft || m.backoffLeft || m.backoffNext != 60) Err("LW5: behind the Reset %s, %s, rest %d, back-off %d, next %d (STANDBY, FGDRW_CLEARED, 0, 0, 60 wanted)", FgDriveStateName(m.state), FgDriveWhyName(m.why), m.restLeft, m.backoffLeft, m.backoffNext);
    FgDriveMem c; now = restNow(c);
    const LONG a0 = dtArmCount; Frame(); Drain(); const LONG a1 = dtArmCount;
    in.cleared = a1 != a0 ? 1 : 0;
    FgDriveStep(c, in, k, now + 2000, 1, 0);
    if (c.state != FGDR_REST) Err("LW5 control: with no Reset the law went to %s (REST wanted): the control shows nothing", FgDriveStateName(c.state));
    Ctl("no Reset: the arm count stood (%ld), the law stayed in REST (%d window(s) of rest left)", (long)a1, c.restLeft);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW6, the stand-down with the worker held in the next body and a ride pending
// WHAT IT SHOWS, by hand, from PW2's shape (dt_fg_place.h): ride pair P, then pair A with its dispatch HELD (the helper in it) and its order
// wait standing; the worker meets A's position in frame B's calls with A's answer not in, and WAITS THERE, frame B queued behind it. WHILE IT
// IS HELD the law stands down: the want written PASS from another thread, as the latency waiter writes it (memory: a drained proof hides an
// ordering). Then the helper lets go (or never does: the wait's bound), and frames C and D come standing by.
// WHAT MUST HOLD: A's real frame once, B's once, C's and D's once, in that order; A's generated frame, if shown, ahead of A's real frame and
// never twice; frame B within a second (no freeze); the stand-down landed while the worker waited (its wait counted, the helper in A).
// (ROUND 3, THE REVIEWER: the stand-down goes THROUGH THE PRODUCT'S TWO SEAMS, not a write of the want: (1) FgpArmWant, the run event first
// and then the want, AS fg_arm_life.h WRITES IT (FgThreadsRun's rule over this harness's binding, the run event of dt_fg_threads.h; copied
// here line for line because FgpArmWant is the product's own and this program does not compile fg_arm_life.h); (2) A ROW: the refusal the
// present path reads (FgPA::MayEngage's: here the binding's mayEngage), the want left ON, as a row that stands does it between two window
// steps; and THE CONTROL, no stand-down at all, which shows the generated frames of C and D that the two take away.)
static volatile LONG gLw6Wait0 = 0, gLw6Wrote = 0, gLw6Held = 0; static LONG gLw6Pass = 0; static int gLw6Mode = 0;
static void Lw6ArmWant(LONG want)
{
    if (!ThS::runEv) ThS::runEv = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (FgThreadsRun<ShArmB>(&gLw6Pass, want)) SetEvent(ThS::runEv); else ResetEvent(ThS::runEv);
    FgState& s = B::S();
    if (s.wt.want != want) WriteRelease(&s.wt.want, want);
}
static void Lw6StandDown()
{
    _InterlockedExchange(&gLw6Held, gH.inDispatch != 0 ? 1 : 0);
    if (gLw6Mode == 1) Lw6ArmWant(FG_WANT_PASS);
    else if (gLw6Mode == 2) _InterlockedExchange(&ShB::mayEngage, 0);
    _InterlockedExchange(&gLw6Wrote, ShArmB::a.w.posWaits - gLw6Wait0);
}
static void Lw6StandDownLetGo() { Lw6StandDown(); Sleep(5); ShRelease(); }
struct Lw6Leg { int genA, realA, realB, realC, realD, genCD, inOrder; LONG waits, got; DWORD ms; int wroteInWait, heldAtWrite; LONG want; };
static Lw6Leg Lw6Run(int mode, int letGo)
{
    Lw6Leg g = {};
    gLw6Mode = mode; gLw6Pass = 0;
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.rideEarly = 1; a.k.rideWaitAns = 1; a.k.prepWait = 1; a.k.place = 1;
    a.k.ord = 2; a.k.ordMaxUs = 4500; LaOn();
    _InterlockedExchange(&ShArmB::placeOn, 1);
    ShWarm<MA>(); MgDueN();
    ClockAdd(16.7); Pw2Calls(8); ShInject(true); Frame<MA>(); Drain();
    if (!ShSettle()) Err("LW6: the fake helper did not read pair P's messages");
    ClockAdd(16.7); Pw2Calls(8);
    _InterlockedExchange(&gH.holdDispatch, 1);
    ShInject(true);
    const DWORD tagA = Frame<MA>();
    Drain();
    WaitFor([] { return gH.inDispatch != 0; }, 3000);
    const size_t shown0 = MgShownN();
    gLw6Wait0 = a.w.posWaits; _InterlockedExchange(&gLw6Wrote, -1); _InterlockedExchange(&gLw6Held, 0);
    const LONG got0 = a.w.posWaitGot;
    // (the stand-down lands once the worker is waiting at the position, frame B queued behind it; then, where it is let go, the helper is released)
    void (*then)() = letGo ? &Lw6StandDownLetGo : &Lw6StandDown;
    Later l = { [] { return ShArmB::a.w.posWaits > gLw6Wait0; }, then, 3000, 0 };
    HANDLE h = Spawn(LaterMain, &l);
    const DWORD t0 = GetTickCount();
    ClockAdd(16.7); Pw2Calls(8); ShInject(true);
    const DWORD tagB = Frame<MA>();
    Drain();
    g.ms = GetTickCount() - t0;
    Join(h, "LW6's stand-down");
    if (!letGo) ShRelease();
    if (!ShSettle()) Err("LW6: the fake helper did not read frame B's messages");
    g.want = B::S().wt.want;
    ClockAdd(16.7); Pw2Calls(8); ShInject(true); const DWORD tagC = Frame<MA>(); Drain();
    ClockAdd(16.7); Pw2Calls(8); ShInject(true); const DWORD tagD = Frame<MA>(); Drain();
    MgDueN();
    ShArmB::OrdSignal(0x7FFFFF); Drain();
    const auto sh = Shown(); const DWORD genA = ShGenTag(tagA, false), genC = ShGenTag(tagC, false), genD = ShGenTag(tagD, false);
    int iG = -1, iA = -1, iB = -1, iC = -1, iD = -1;
    for (size_t i = shown0; i < sh.size(); i++)
    {
        if (sh[i].tag == genA) { g.genA++; if (iG < 0) iG = (int)i; }
        if (sh[i].tag == genC || sh[i].tag == genD) g.genCD++;
        if (sh[i].tag == tagA) { g.realA++; if (iA < 0) iA = (int)i; }
        if (sh[i].tag == tagB) { g.realB++; if (iB < 0) iB = (int)i; }
        if (sh[i].tag == tagC) { g.realC++; if (iC < 0) iC = (int)i; }
        if (sh[i].tag == tagD) { g.realD++; if (iD < 0) iD = (int)i; }
    }
    g.inOrder = (iG < 0 || iG < iA) && iA >= 0 && iA < iB && iB < iC && iC < iD ? 1 : 0;
    g.waits = a.w.posWaits - gLw6Wait0; g.got = a.w.posWaitGot - got0;
    g.wroteInWait = gLw6Wrote; g.heldAtWrite = gLw6Held;
    _InterlockedExchange(&ShB::mayEngage, 1);
    _InterlockedExchange(&ShArmB::placeOn, 0);
    MgClean("LW6");
    MgEnd();
    return g;
}
static void CaseLW6()
{
    const char* const name = "LW6 the stand-down through FgpArmWant and through a row, the worker held in the next body and a ride pending: every real frame once and in order, no freeze (2026-10-17, phase 5, round 3)";
    CaseBegin(name);
    static const char* const kMode[3] = { "CONTROL, NO STAND-DOWN", "THROUGH FgpArmWant (the run event, then the want)", "THROUGH A ROW (the refusal the present path reads, the want left ON)" };
    int ctlGen = 0;
    for (int mode = 0; mode < 3; mode++)
        for (int letGo = 1; letGo >= 0; letGo--)
        {
            if (mode == 0 && !letGo) continue;
            const Lw6Leg g = Lw6Run(mode, letGo);
            const char* tag = mode == 0 ? "control" : mode == 1 ? "FgpArmWant" : "a row";
            printf("  %s, %s: the stand-down written with the worker waiting at the position (waits counted then %d, the helper in A's dispatch %s); waits %ld, the answer came %ld;"
                   " A's generated frame %d, real frames A %d, B %d, C %d, D %d, generated frames of C and D shown %d, in order %s; frame B took %lu ms; the want behind it %s\n",
                kMode[mode], letGo ? "the helper lets go" : "the helper never lets go (the wait's bound)", g.wroteInWait, g.heldAtWrite ? "yes" : "NO",
                g.waits, g.got, g.genA, g.realA, g.realB, g.realC, g.realD, g.genCD, g.inOrder ? "yes" : "NO", (unsigned long)g.ms, g.want == FG_WANT_PASS ? "PASS" : "ON");
            if (g.wroteInWait < 1 || !g.heldAtWrite) Err("LW6 (%s): the stand-down did not land while the worker waited and the helper was held (waits %d, held %d): the case proved nothing", tag, g.wroteInWait, g.heldAtWrite);
            if (g.realA != 1 || g.realB != 1 || g.realC != 1 || g.realD != 1 || g.genA > 1 || !g.inOrder) Err("LW6 (%s): A's generated frame %d, real frames %d %d %d %d, in order %d (at most 1, each 1, yes wanted)", tag, g.genA, g.realA, g.realB, g.realC, g.realD, g.inOrder);
            if (g.ms > 1000) Err("LW6 (%s): frame B took %lu ms (a second at the most wanted: a freeze)", tag, (unsigned long)g.ms);
            if (mode == 1 && g.want != FG_WANT_PASS) Err("LW6 (FgpArmWant): the want behind the stand-down is not PASS");
            if (mode > 0 && g.genCD != 0) Err("LW6 (%s): %d generated frame(s) of C and D shown behind the stand-down (none wanted)", tag, g.genCD);
            if (mode == 0) ctlGen = g.genCD;
        }
    if (ctlGen < 1) Err("LW6 control: with no stand-down %d generated frame(s) of C and D shown (some wanted): the control shows nothing", ctlGen);
    Ctl("no stand-down: %d generated frame(s) of C and D shown, which both stand-downs take away", ctlGen);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW7, FusionFix's limiter at 58, 100 and 120
// WHAT IT SHOWS. A game of 60 by itself (58: the limiter binds standing by) and of 62 (100 and 120), the law from standing by for 20
// windows, FUSIONFIX'S LIMITER FAKED IN THE GAME'S PRESENT (a present no sooner than 1/cap behind the one before: what it sleeps is added to
// the frame, and counted). AT 58, at or under 1.03 C: FGC_FFCAP stands in every window, nothing is engaged. AT 100 AND 120: engaged; FgPace
// owns the pacing (the machine's PaceOwner, 1) and holds the game at 59.1, so the limiter SLEEPS 0 in every engaged window.
struct Lw7Leg { int engaged, ffcapWins, owner1, ownerOther; double sleptOnMs, sleptOffMs; LwFrames f; };
static Lw7Leg Lw7Run(int cap, double fps)
{
    Lw7Leg g = {};
    RtPar par;
    RtBegin(par);
    gRtSeed = 20261017u;
    B::limiter = true; B::ffCap = cap;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.ffCap = cap;
    const LONGLONG capTicks = RtMs(1000.0 / cap);
    for (int win = 0; win < 20; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            LONGLONG cost = RtMs(1000.0 / fps * RtSpread(0.03));
            const bool on = B::S().wt.want != FG_WANT_PASS;
            if (!gRt.pairs.empty())
            {
                const LONGLONG due = gRt.pairs.back().tEntry + capTicks, at = B::clockNow + cost;
                if (due > at) { cost += due - at; (on ? g.sleptOnMs : g.sleptOffMs) += (double)(due - at) * 1000.0 / (double)B::Qpf(); }
            }
            RtFrame(cost);
            LwLawPoll(L);
            if (on) { if (B::S().r.paceOwner == 1) g.owner1++; else g.ownerOther++; }
        }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        if (r.row == FGC_FFCAP) g.ffcapWins++;
        if (r.state == FGDR_ON) g.engaged++;
    }
    g.f = LwFramesRead();
    B::limiter = false; B::ffCap = 0;
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW7");
    return g;
}
static void CaseLW7()
{
    const char* const name = "LW7 FusionFix's limiter: at 58 the law stands by (FGC_FFCAP); at 100 and 120 it engages, FgPace owns the pacing and the limiter sleeps 0 (2026-10-17, phase 5)";
    CaseBegin(name);
    static const int caps[3] = { 58, 100, 120 };
    for (int c = 0; c < 3; c++)
    {
        const Lw7Leg g = Lw7Run(caps[c], caps[c] == 58 ? 60.0 : 62.0);
        printf("  FusionFix at %d: engaged windows %d of 20, windows with FGC_FFCAP %d; engaged presents FgPace owned %d, another owner %d; the limiter slept %.1f ms engaged, %.1f standing by; %d frames, real frames shown once %d, out of place %d\n",
            caps[c], g.engaged, g.ffcapWins, g.owner1, g.ownerOther, g.sleptOnMs, g.sleptOffMs, g.f.n, g.f.real1, g.f.order);
        if (g.f.order || g.f.real1 != g.f.n) Err("LW7 at %d: %d picture(s) out of place, %d of %d real frames shown exactly once", caps[c], g.f.order, g.f.real1, g.f.n);
        if (caps[c] == 58) { if (g.engaged || g.ffcapWins != 20 || g.sleptOffMs <= 0.0) Err("LW7 at 58: engaged windows %d, FGC_FFCAP windows %d, the limiter's sleep standing by %.1f ms (0, 20 and some wanted)", g.engaged, g.ffcapWins, g.sleptOffMs); }
        else if (g.engaged < 5 || g.ownerOther || g.owner1 <= 0 || g.sleptOnMs > 0.0) Err("LW7 at %d: engaged windows %d, presents owned by FgPace %d and by another %d, the limiter's sleep engaged %.1f ms (5 or more, some, 0 and 0 wanted)", caps[c], g.engaged, g.owner1, g.ownerOther, g.sleptOnMs);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW8, a foreign limiter: NVIDIA's Max Frame Rate
// WHAT IT SHOWS. A game of 62 by itself, the law from standing by for 12 windows, NVIDIA's cap for the game as the read-only probe would hand
// it (FgCondIn::nvCap). AT 116 (under the 121.7 presents a pair makes at 120 Hz with 3 percent): FGC_NVCAP stands in every window and nothing
// is engaged, with its line. CONTROL, AT 130: nothing stands, and it engages.
// (RTSS IS NOT A ROW: its module is in the game whether it limits or not, and its Vulkan layer even with RTSS closed: DECISIONS, 2026-10-11,
// "RTSS CANNOT BE SEEN AS A LIMITER". The brief's form is not built; the report of P1 says why and what would.)
static int Lw8Run(int nv, int* engaged)
{
    RtPar par;
    RtBegin(par);
    gRtSeed = 20261017u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.nvCap = nv;
    int rows = 0; *engaged = 0;
    for (int win = 0; win < 12; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(1000.0 / 62.0 * RtSpread(0.03))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        if (r.row == FGC_NVCAP) rows++;
        if (r.state == FGDR_ON) (*engaged)++;
    }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW8");
    return rows;
}
static void CaseLW8()
{
    const char* const name = "LW8 a foreign limiter stands frame generation by: NVIDIA's Max Frame Rate at 116 on a 120 Hz panel (FGC_NVCAP, its line); at 130 it engages (2026-10-17, phase 5)";
    CaseBegin(name);
    int on = 0; const int rows = Lw8Run(116, &on);
    FgCondIn in = LwSession(RtPar()); in.nvCap = 116; char b[320]; FgCondLine(FGC_NVCAP, in, FgCondDefaults(), b, sizeof(b));
    printf("  NVIDIA's cap at 116: windows with FGC_NVCAP %d of 12, engaged windows %d; the line: %s\n", rows, on, b);
    if (rows != 12 || on) Err("LW8: at 116 FGC_NVCAP stood in %d of 12 windows and %d were engaged (12 and 0 wanted)", rows, on);
    int onC = 0; const int rowsC = Lw8Run(130, &onC);
    printf("  CONTROL, NVIDIA's cap at 130: windows with FGC_NVCAP %d, engaged windows %d\n", rowsC, onC);
    if (rowsC || onC < 3) Err("LW8 control: at 130 FGC_NVCAP stood in %d window(s) and %d were engaged (0 and 3 or more wanted): the control shows nothing", rowsC, onC);
    Ctl("NVIDIA's cap at 130, over the pair's presents: nothing stands, engaged in %d windows", onC);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW17, A STINT'S WARM-UP (round 5, the owner's F1C lawon leg on pair 9415cf8c)
// WHAT IT SHOWS. THE LEG'S OWN WINDOWS REPLAYED INTO THE LAW AND THE TABLE (fg_cond.h, FgDriveStep and FgCondFirst with the warm-up's lift,
// FgDriveWarmLift, as fg_arm_life.h's FgpCondRead ORs it in): the card locked at 750 MHz, the game about 60 by itself and the card the limit;
// windows 1 to 4 waiting for the CPU placement, the dwell 5 to 7, the wait for a cut 8 to 12 (two seconds a window: FGDRW_WAITED behind
// window 12), then ON. ITS FIRST THREE ON WINDOWS SHOW 1 OF 107, 0 OF 119 AND 62 OF 120 GENERATED FRAMES (on the card 303 of 1, none, 423 of
// 51), then 114 to 120 of 120 (95 to 100 of 100: the health exit's share is of the real frames presented) for the rest of five minutes, R_on 43 a second there (6.6 ms added on a card that is the limit). Two shapes of
// the warm-up's real rate: (a) THE LEG'S, 54.5, 52.4 and 43.3; (b) THE MAIN SESSION'S HARDER ONE, 36, 35 and 34.5, under SLOW's 37 all three.
// THE LAW'S DEMAND: each stays ON for the five minutes, one engage and no stand-down, and c is measured behind the warm-up (its windows 4 to 8
// at 43 against 60: 6,300 to 6,900 us; the warm-up's rates would have given more). CONTROL, ROUND 4'S LAW (no warm-up, FgDriveK::warmWin 0):
// shape (a) stands down behind window 15 for FGDRW_SHOWN, as the game did; shape (b) behind window 13 for FGC_SLOW.
struct Lw17Leg { std::vector<LwRec> w; int engages, downs, endState, firstDownWin, firstDownWhy, firstDownRow, cLim, liftWins; };
static Lw17Leg Lw17Run(int warmWin, int hard)
{
    Lw17Leg g = {}; g.firstDownWin = -1;
    FgDriveK k = FgDriveDefaults(); k.warmWin = warmWin;
    const FgCondK ck = LwSceneK();
    FgDriveMem m; FgDriveInit(m, k);
    unsigned now = 1000;
    static const double warmA[3] = { 54.5, 52.4, 43.3 }, warmB[3] = { 36.0, 35.0, 34.5 };
    static const int gen[3] = { 1, 0, 62 }, n[3] = { 107, 119, 120 }, cMed[3] = { 303, 0, 423 }, cN[3] = { 1, 0, 51 };
    int onW = 0;
    for (int win = 1; win <= 150; win++)
    {
        now += 2000;
        const bool on = m.state == FGDR_ON;
        FgCondIn in = LwSession(RtPar());
        in.engaged = on ? 1 : 0;
        FgDriveIn di = {};
        double r = 60.0 + ((win * 7) % 5 - 2) * 0.3;
        if (on)
        {
            onW++;
            r = onW <= 3 ? (hard ? warmB[onW - 1] : warmA[onW - 1]) : 43.0 + ((win * 3) % 5 - 2) * 0.2;
            di.shownGen = onW <= 3 ? gen[onW - 1] : 114 + (win % 7); di.shownN = onW <= 3 ? n[onW - 1] : 120;
            di.cardMed = onW <= 3 ? cMed[onW - 1] : 440 + (win % 30); di.cardN = onW <= 3 ? cN[onW - 1] : 100;
            di.lawPerUs = 16920;                                   // (the law's period at the panel's floor: the card holds the game, c = real - alone)
        }
        in.realFpsX10 = (int)(r * 10.0 + 0.5);
        const unsigned plain = (unsigned)FgCondFirst(in, ck);
        in.lift |= FgDriveWarmLift(m);
        di.row = FgCondFirst(in, ck); di.band = FgDriveBandOf(in, ck);
        if (on && di.row != (int)plain) g.liftWins++;
        di.realX10 = in.realFpsX10; di.card = 1; di.cpuReady = win > 4 ? 1 : 0;
        LwRec rec = {}; rec.prev = m.state;
        FgDriveStep(m, di, k, now, 0, 0);
        rec.rX10 = di.realX10; rec.row = di.row; rec.band = di.band; rec.state = m.state; rec.why = m.why; rec.cUs = FgDriveCUs(m, 1, nullptr);
        rec.shownGen = di.shownGen; rec.shownN = di.shownN; rec.cardMed = di.cardMed; rec.cardN = di.cardN; rec.frames = 120;
        g.w.push_back(rec);
        if (rec.prev == FGDR_ON && m.state != FGDR_ON && g.firstDownWin < 0) { g.firstDownWin = win; g.firstDownWhy = m.why; g.firstDownRow = m.whyRow; }
    }
    g.engages = m.engages; g.downs = m.downs; g.endState = m.state; g.cLim = m.cLimUs;
    return g;
}
static void Lw17Say(const char* what, const Lw17Leg& g)
{
    printf("  %s\n  window  real  row               state      shown      card (n)   c us\n", what);
    for (size_t i = 0; i < g.w.size(); i++)
    {
        const LwRec& r = g.w[i];
        if (i >= 20 && (i % 20) != 0 && r.state == r.prev && i + 1 != g.w.size()) continue;
        printf("  %5d  %5.1f  %-16s  %-9s  %4d/%-4d  %4d (%d)  %5d%s%s\n", (int)i + 1, r.rX10 / 10.0, FgCondEnumName(r.row), FgDriveStateName(r.state), r.shownGen, r.shownN, r.cardMed, r.cardN, r.cUs,
            r.state != r.prev ? "   <- " : "", r.state != r.prev ? FgDriveWhyName(r.why) : "");
    }
    printf("  150 windows: engages %d, stand-downs %d, the state at the end %s, the first stand-down behind window %d (%s, %s); c measured on a card that was the limit %d us; ON windows whose row the warm-up lifted %d\n",
        g.engages, g.downs, FgDriveStateName(g.endState), g.firstDownWin, g.firstDownWin > 0 ? FgDriveWhyName(g.firstDownWhy) : "none", g.firstDownWin > 0 ? FgCondEnumName(g.firstDownRow) : "none", g.cLim, g.liftWins);
}
static void CaseLW17()
{
    const char* const name = "LW17 a stint's warm-up: the owner's F1C lawon leg (1, 0 and 62 of 120 shown in its first three ON windows) stays ON; round 4's law stood down behind window 15 (2026-10-17, phase 5, round 5)";
    CaseBegin(name);
    const int wW = FgDriveDefaults().warmWin;
    for (int hard = 0; hard < 2; hard++)
    {
        const char* tag = hard ? "LW17 (b)" : "LW17 (a)";
        const Lw17Leg g = Lw17Run(wW, hard);
        Lw17Say(hard ? "(b) THE WARM-UP AT 36, 35 AND 34.5, UNDER SLOW'S 37, THE WARM-UP OF 3:" : "(a) THE LEG'S OWN WARM-UP, 54.5, 52.4 AND 43.3, THE WARM-UP OF 3:", g);
        if (g.engages != 1 || g.downs != 0 || g.endState != FGDR_ON) Err("%s: %d engage(s), %d stand-down(s), %s at the end (1, 0, ON wanted)", tag, g.engages, g.downs, FgDriveStateName(g.endState));
        if (g.w.size() > 12 && (g.w[11].state != FGDR_ON || g.w[11].why != FGDRW_WAITED)) Err("%s: behind window 12 the state %s, %s (ON, FGDRW_WAITED wanted: the leg's engage)", tag, FgDriveStateName(g.w[11].state), FgDriveWhyName(g.w[11].why));
        if (g.cLim < 6300 || g.cLim > 6900) Err("%s: c %d us (6,300 to 6,900 wanted: 43 engaged against 60 by itself, its windows 4 to 8 alone, never the warm-up's)", tag, g.cLim);
        if (hard && g.liftWins != 3) Err("%s: the warm-up lifted the row in %d ON window(s) (3 wanted: FGC_SLOW in each of its three)", tag, g.liftWins);
        const RingStats stc = gRingSt;
        const Lw17Leg c = Lw17Run(0, hard);
        ViolRestore(stc);
        Lw17Say(hard ? "CONTROL (b), ROUND 4'S LAW, NO WARM-UP:" : "CONTROL (a), ROUND 4'S LAW, NO WARM-UP:", c);
        const int wantWin = hard ? 13 : 15, wantWhy = hard ? FGDRW_ROW : FGDRW_SHOWN;
        if (c.firstDownWin != wantWin || c.firstDownWhy != wantWhy) Err("%s control: the first stand-down behind window %d, %s (behind %d, %s wanted, as the game did): the control shows nothing", tag, c.firstDownWin, c.firstDownWin > 0 ? FgDriveWhyName(c.firstDownWhy) : "none", wantWin, FgDriveWhyName(wantWhy));
        Ctl("%s round 4's law: stood down behind window %d for %s (%s)", tag, c.firstDownWin, c.firstDownWin > 0 ? FgDriveWhyName(c.firstDownWhy) : "none", c.firstDownWin > 0 ? FgCondEnumName(c.firstDownRow) : "none");
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW19, THE MORNING'S lawon LEG REPLAYED (phase 5 round 2, S2, 2026-10-18)
// WHAT THE LEG SHOWED (docs/reports/fg-5/sitting-2026-10-18, F1C-lawon: 750 MHz, DLAA, the game about 58 to 61 by itself): ON in 17 of 176
// windows. Each stint stood down by FGC_SLOW at R_on 38.9 and 39.4, OVER the engaged out-rule of 37, with the line "the game runs at about 38
// fps BY ITSELF, under the 40 it needs", and R_free and the pacer's E read "none" in every ON window: the conditions read an engaged session as
// standing by. WHY: in the armed development build the latency waiter's window runs the shadow driver's step (fg_product.h, FgpDevWindowStep)
// ahead of the armed product's (DLSS-IV.cpp, the window's tail); at fg_shadow 0 that step wrote the want PASS every window, the conditions read
// "engaged" from the want word a moment later (fg_arm_life.h, FgpCondRead), and the law wrote ON back behind them. So engaged, SLOW stood at the
// standing-by floor (40, not 37), c had no law period (fixed120's 6,421 us from the pacer's own hold: dt_fg_fixed.h, FX14) and R_free was
// never read. THE CASE: the leg's own scene, window by window (the game's own frame time standing by; while ON the leg's real frame time less
// its c, 8,120 us), 8.12 ms laid on every engaged frame where a card that is the limit holds the game (LwAddInPresent), the card the limit
// standing by, the cut wait 10 s, the law from its first window, IN THE DEVELOPMENT BUILD'S WINDOW ORDER (LwLaw::devPass). HELD, THE
// PRODUCT'S READING: no stand-down by FGC_SLOW at an R_on of 37 or more; R_free read in every window that ran engaged; at most 4 engages and
// stand-downs in any 5 minutes (W's lawflap); every real frame once and in order. CONTROL, THE READING OF BEFORE (the want word): a stand-down
// by FGC_SLOW at 37 or more, as the leg's. (A stand-in, said: c is laid on as one constant; the leg's two stints measured 8,120 and 6,361 us.)
static const short kLw19Scene[176] = {   // (hundredths of a millisecond; tools: the session scratchpad's lawon_series.py over the leg's log)
    1672, 1629, 1629, 1681, 1672, 1667, 1672, 1582, 1580, 1626, 1667, 1637, 1580, 1676, 1627, 1609, 1558, 1621, 1759, 1786,
    1832, 1795, 1629, 1613, 1623, 1639, 1645, 1709, 1701, 1701, 1692, 1656, 1712, 1736, 1767, 1727, 1661, 1681, 1718, 1675,
    1553, 1553, 1572, 1730, 1890, 1776, 1730, 1684, 1658, 1709, 1689, 1681, 1590, 1634, 1718, 1751, 1761, 1761, 1748, 1727,
    1757, 1887, 1855, 1931, 1883, 1938, 1912, 1855, 1842, 1869, 1736, 1724, 1661, 1767, 1802, 1748, 1658, 1650, 1678, 1647,
    1661, 1642, 1686, 1698, 1681, 1739, 1681, 1613, 1730, 1802, 1767, 1689, 1605, 1669, 1629, 1642, 1603, 1656, 1664, 1736,
    1733, 1739, 1653, 1650, 1698, 1733, 1715, 1623, 1639, 1637, 1650, 1645, 1681, 1650, 1730, 1569, 1524, 1541, 1482, 1487,
    1386, 1476, 1592, 1592, 1621, 1726, 1709, 1692, 1692, 1597, 1587, 1669, 1631, 1629, 1650, 1639, 1748, 1754, 1727, 1709,
    1709, 1724, 1845, 1773, 1689, 1650, 1672, 1653, 1672, 1783, 1789, 1745, 1748, 1757, 1773, 1783, 1767, 1757, 1776, 1869,
    1845, 1789, 1848, 1832, 1832, 1898, 1862, 1957, 1905, 1848, 2037, 1862, 1799, 1786, 1802, 1835 };
// (REVIEW ROUND 2: THE LEG'S OWN CARD READINGS standing by, window by window, from its law lines: 1 "is the limit", 0 "has idle room". They flip:
// windows 47 to 49, 138 to 144 and 155 to 156 read idle room standing by among readings of the limit. The leg's own ON windows, 13 to 19 and
// 115 to 126, read the engaged reading that says nothing of the card (FgDriveCardTake); they are put as the limit their neighbours read)
static const signed char kLw19Card[176] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
struct Lw19Leg { std::vector<LwRec> w; LwFrames f; int engages, downs, slowOver, slowOverX10, slowUnder, engagedWins, rFreeNone, turns5, endState; int cLimUs, cLimErrUs; double mins; int changes5, changes, passOnRead, waitOnslow; };
// (ROUND 9, L2, 2026-10-26: THE CHANGES' BOUND, CHANGED BY NAME FROM 4 TO 6. Behind the stint's stand-down by FGC_SLOW (window 63) the quick
// come-back rests one window, not 70, so the scene's lightest windows reach the wait: window 117 by itself 64.6, the prediction 40.2 over the
// floor, the wait entered; window 118 by itself 67.6, over the top, FGC_FAST, the wait left. Two changes more in the same 5 minutes, and none
// into or out of ON. What the reviewer's bound was for stays held by its own count: the wait is never left on FGC_ONSLOW's edge, and the
// engages and stand-downs stay at 4 at the most in any 5 minutes)
enum { LW19_CHANGES_5MIN = 6 };
static Lw19Leg Lw19Run(int oldRead, int devPass, int oldShadow, const short* scene = kLw19Scene, int wins = 176, double addMs = 8.12)
{
    Lw19Leg g = {};
    const RtPar par = Lw9Par(16.4, addMs);
    RtBegin(par);
    gRtSeed = 20261018u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.card = 1; L.oldRead = oldRead; L.devPass = devPass; L.oldShadow = oldShadow;
    L.cardEngaged = 0; L.cpuAt = 4;                           // (the leg's: "idle room" read in every ON window; the CPU placement answered behind window 4)
    L.cardSeries = kLw19Card; L.cardSeriesN = 176;            // (review round 2: the leg's standing-by readings, flips and all)
    const unsigned t0 = LwNowMs();
    for (int win = 0; win < wins; win++)
    {
        const double alone = scene[win] / 100.0;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.03))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (r.prev == FGDR_ON)
        {
            g.engagedWins++;
            if (r.rFree <= 0) g.rFreeNone++;
            if (r.state != FGDR_ON && r.why == FGDRW_ROW && r.row == FGC_SLOW) { if (r.rX10 >= 370) { g.slowOver++; if (!g.slowOverX10) g.slowOverX10 = r.rX10; } else g.slowUnder++; }
        }
        if (r.prev == FGDR_ENGAGING && r.state == FGDR_STANDBY && r.row == FGC_ONSLOW) g.waitOnslow++;   // (round 9, L2: the wait left on FGC_ONSLOW's edge)
    }
    g.mins = (LwNowMs() - t0) / 60000.0;
    g.engages = L.mem.engages; g.downs = L.mem.downs; g.endState = L.mem.state; g.turns5 = LwTurnsIn5Min(L);
    g.cLimUs = L.mem.cLimUs; g.cLimErrUs = L.mem.cLimErrUs; g.changes5 = LwChangesIn5Min(L); g.changes = (int)L.changesMs.size(); g.passOnRead = L.passOnRead;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW19");
    return g;
}
static void Lw19Say(const char* what, const Lw19Leg& g)
{
    LwPrint(what, g.w);
    printf("  %.1f minutes, %d windows: engages %d, stand-downs %d (by FGC_SLOW at an R_on of 37 or more %d, the first at %.1f; under 37 %d), at most %d in any 5 minutes, the state at the end %s;"
           " windows that ran engaged %d, R_free none in %d of them; c on a card that was the limit %d us, its margin %d us at the end; changes of state of any kind %d, at most %d in any 5 minutes; %d frames, real frames shown once %d, out of place %d\n",
        g.mins, (int)g.w.size(), g.engages, g.downs, g.slowOver, g.slowOverX10 / 10.0, g.slowUnder, g.turns5, FgDriveStateName(g.endState), g.engagedWins, g.rFreeNone, g.cLimUs, g.cLimErrUs, g.changes, g.changes5, g.f.n, g.f.real1, g.f.order);
}
static void CaseLW19()
{
    const char* const name = "LW19 the morning's lawon leg replayed in the development build's window order: no engaged stand-down by FGC_SLOW at 37 or more, R_free read engaged, at most 4 turns in 5 minutes (2026-10-18, phase 5 round 2, S2)";
    CaseBegin(name);
    const Lw19Leg a = Lw19Run(0, 1, 0);
    Lw19Say("THE LEG'S SCENE AND CARD READINGS, THE PRODUCT'S WRITERS IN THE DEVELOPMENT BUILD'S ORDER, THE PRODUCT'S READING:", a);
    printf("  ON windows whose want the conditions read as PASS: %d\n", a.passOnRead);
    if (a.passOnRead) Err("LW19: %d ON window(s) whose want read PASS at the conditions' step (none wanted: no step writes the want ahead of the law)", a.passOnRead);
    {   // (review round 2: the reading stands with the shadow driver's PASS put back too: the law's own state is what it reads)
        const Lw19Leg b = Lw19Run(0, 1, 1);
        Lw19Say("THE SAME, WITH THE SHADOW DRIVER'S PASS OF BEFORE PUT BACK (the reading alone must hold):", b);
        if (!b.passOnRead || b.slowOver || b.rFreeNone || b.changes5 > LW19_CHANGES_5MIN || b.waitOnslow) Err("LW19 (b): with the PASS of before, %d ON window(s) read PASS, %d stand-down(s) by FGC_SLOW at 37 or more, R_free none in %d, %d changes in 5 minutes, the wait left on FGC_ONSLOW %d time(s) (PASS read in some, then 0, 0, %d at the most and 0 wanted)", b.passOnRead, b.slowOver, b.rFreeNone, b.changes5, b.waitOnslow, (int)LW19_CHANGES_5MIN);
    }
    if (a.engages < 1) Err("LW19: the law never engaged (one engage at least wanted: the leg's first stint)");
    if (a.slowOver) Err("LW19: %d stand-down(s) by FGC_SLOW at an R_on of 37 or more engaged, the first at %.1f (none wanted: the engaged out-rule is 37)", a.slowOver, a.slowOverX10 / 10.0);
    if (a.rFreeNone) Err("LW19: R_free none in %d of %d windows that ran engaged (none wanted: the conditions read the session as engaged)", a.rFreeNone, a.engagedWins);
    if (a.turns5 > 4) Err("LW19: %d engages and stand-downs in 5 minutes (4 at the most wanted: W's lawflap)", a.turns5);
    // (S2 ROUND 2, the reviewer's first and second: the game's pass through FAST at windows 118 to 120 cleared c's margin, and the wait for a cut
    // was entered and left at 124/125 and 134/136, each entry the ladder held and the guard parked. One stint's cycle is four changes)
    if (a.changes5 > LW19_CHANGES_5MIN) Err("LW19: %d changes of state of any kind in 5 minutes (%d at the most wanted: one stint's STANDBY, ENGAGING, ON, REST and back, and one wait entered and left behind the quick come-back, round 9's L2)", a.changes5, (int)LW19_CHANGES_5MIN);
    if (a.waitOnslow) Err("LW19: the wait for a cut left on FGC_ONSLOW's edge %d time(s) (none wanted)", a.waitOnslow);
    if (a.downs > 0 && a.cLimErrUs <= 0) Err("LW19: c's margin %d us at the end, behind a stint that filed one and no stint since (kept wanted: a passing band move is not the game elsewhere)", a.cLimErrUs);
    if (a.f.order || a.f.real1 != a.f.n) Err("LW19: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const RingStats stc = gRingSt;
    const Lw19Leg c = Lw19Run(1, 1, 1);
    ViolRestore(stc);
    Lw19Say("CONTROL, THE READING OF BEFORE (the want word), THE SAME ORDER:", c);
    if (!c.slowOver || !c.rFreeNone) Err("LW19 control: the reading of before stood down by FGC_SLOW at 37 or more %d time(s) and read R_free none in %d window(s) (both wanted, as the leg did): the control shows nothing", c.slowOver, c.rFreeNone);
    Ctl("the reading of before: %d engage(s), stand-downs by FGC_SLOW at 37 or more %d (the first at %.1f), R_free none in %d of %d engaged windows", c.engages, c.slowOver, c.slowOverX10 / 10.0, c.rFreeNone, c.engagedWins);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW20, C'S MARGIN: A MARGINAL GAME STAYS BY, ONE THAT CLEARLY GAINS ENGAGES AT ONCE (S2)
// WHAT IT SHOWS (the brief's A; fg_cond.h, FgDrivePredHiX10). THE LEG'S FIRST STINT AS ITS SCENE: by itself at 61.1 (16.37 ms), 8.12 ms laid on
// every engaged frame on a card that is the limit, and while ON the game's own frame time window by window so that R_on runs as the leg's
// first stint ran (41.8, 40.2 and 41.0 in the warm-up, then 41.3, 42.2, 41.1, 38.9 and 40.5: its c windows, a spread of about 0.7 ms), then 41
// for twelve windows, then A HEAVIER SCENE (19.5 ms by itself, R_on 36: FGC_SLOW stands it down, behind its cost windows: a short stint, the
// back-off). Standing by, the game is at 61.1 again until window 150, then at 65 (15.38 ms) to the end. HELD, THE LAW WITH C'S MARGIN: one
// engage and one stand-down before window 150, FGC_ONSLOW standing in 30 or more windows at 61.1 behind the stint (R_on 40.8 on c's mean, 39.7
// with its spread), and engaged again within 10 windows of the move to 65 (41.3 with the spread: a game that gets faster engages again); every
// real frame once and in order. CONTROL, THE LAW OF BEFORE (FGC_ONSLOW on c's mean alone): it engages again at 61.1 before window 150, the
// second identical cycle.
struct Lw20Leg { std::vector<LwRec> w; LwFrames f; int engBefore, downBefore, onslow61, engAfterAt, moveWin; int cLimUs, cLimErrUs; };
static Lw20Leg Lw20Run(int noMargin)
{
    Lw20Leg g = {}; g.engAfterAt = -1; g.moveWin = 150;
    const RtPar par = Lw9Par(16.37, 8.12);
    RtBegin(par);
    gRtSeed = 20261018u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = 1; L.noMargin = noMargin;
    static const double onRon[8] = { 41.8, 40.2, 41.0, 41.3, 42.2, 41.1, 38.9, 40.5 };
    int onIdx = 0;
    for (int win = 0; win < 175; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        const double by = win < g.moveWin ? 16.37 : 15.38;
        double alone = by;
        if (on && win < g.moveWin) alone = onIdx <= 8 ? 1000.0 / onRon[onIdx - 1] - 8.12 : onIdx <= 20 ? 1000.0 / 41.0 - 8.12 : 19.5;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (win < g.moveWin)
        {
            g.engBefore = L.mem.engages; g.downBefore = L.mem.downs;   // (the law's own counts: the cut poll engages inside a window)
            if (g.downBefore > 0 && r.row == FGC_ONSLOW && r.prev != FGDR_ON) g.onslow61++;
            g.cLimUs = L.mem.cLimUs; g.cLimErrUs = L.mem.cLimErrUs;     // (what the stint behind it filed, before the move)
        }
        else if (g.engAfterAt < 0 && r.state == FGDR_ON) g.engAfterAt = win;
    }
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW20");
    return g;
}
static void Lw20Say(const char* what, const Lw20Leg& g)
{
    LwPrint(what, g.w);
    printf("  before window %d: engages %d, stand-downs %d, windows standing by on FGC_ONSLOW behind the stint %d; behind the move to 65 engaged at window %d; c %d us, its margin %d us; %d frames, real frames shown once %d, out of place %d\n",
        g.moveWin, g.engBefore, g.downBefore, g.onslow61, g.engAfterAt, g.cLimUs, g.cLimErrUs, g.f.n, g.f.real1, g.f.order);
}
static void CaseLW20()
{
    const char* const name = "LW20 c's margin: behind a stint that ran over the floor only on its mean a game at 61 stays by, one at 65 engages at once (2026-10-18, phase 5 round 2, S2)";
    CaseBegin(name);
    // (engages are counted at a window whose state turned ON in a step; the cut poll's engage mid-window shows as the next window's ON)
    const Lw20Leg a = Lw20Run(0);
    Lw20Say("THE LEG'S FIRST STINT, THEN A HEAVIER SCENE; BY ITSELF AT 61.1, THEN AT 65 FROM WINDOW 150; THE LAW WITH C'S MARGIN:", a);
    if (a.engBefore != 1 || a.downBefore != 1) Err("LW20: before the move to 65, %d engage(s) and %d stand-down(s) (1 and 1 wanted: no second identical cycle)", a.engBefore, a.downBefore);
    if (a.onslow61 < 30) Err("LW20: FGC_ONSLOW stood in %d window(s) at 61.1 behind the stint (30 or more wanted: the rate with c's spread is under the floor)", a.onslow61);
    if (a.engAfterAt < 0 || a.engAfterAt > a.moveWin + 10) Err("LW20: behind the move to 65 engaged at window %d (by window %d wanted: a game that clearly gains engages at once)", a.engAfterAt, a.moveWin + 10);
    if (a.cLimErrUs < 400 || a.cLimErrUs > 1500) Err("LW20: c's margin %d us (400 to 1,500 wanted: the leg's stint's spread, about 700)", a.cLimErrUs);
    if (a.f.order || a.f.real1 != a.f.n) Err("LW20: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const RingStats stc = gRingSt;
    const Lw20Leg c = Lw20Run(1);
    ViolRestore(stc);
    Lw20Say("CONTROL, THE LAW OF BEFORE (FGC_ONSLOW on c's mean):", c);
    if (c.engBefore < 2) Err("LW20 control: the law of before engaged %d time(s) before the move to 65 (2 or more wanted, the second identical cycle): the control shows nothing", c.engBefore);
    Ctl("the law of before: %d engages and %d stand-downs at 61.1 before window %d", c.engBefore, c.downBefore, c.moveWin);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW21, FGC_ONSLOW'S BAND IN THE WAIT FOR A CUT (S2, review round 2, the reviewer's second)
// WHAT IT SHOWS. Each entry to the wait for a cut holds the latency ladder at rung 1 and parks the refresh guard, and each exit gives them back.
// A game whose rate with frame generation hovers at the floor entered and left the wait window by window. THE SCENE: by itself at 61.1 with
// 8.12 ms laid on engaged frames on a card that is the limit, one stint to learn c and its margin, a heavier scene that stands it down (19.5
// ms by itself), then standing by AT THE FLOOR'S EDGE, read from the law's own c and margin each window: three windows at a predicted 40.6,
// one at 39.4, by turns, to the end. HELD, THE TABLE WITH THE BAND (37 in the wait): behind the stint's rest, the wait for a cut is never left
// on FGC_ONSLOW, and the law engages again from it; every real frame once and in order. CONTROL, THE TABLE OF BEFORE (no band): the wait is
// entered and left on FGC_ONSLOW three times or more.
struct Lw21Leg { std::vector<LwRec> w; LwFrames f; int engagingExits, engagesAfter, downs1At; };
static Lw21Leg Lw21Run(int noBand)
{
    Lw21Leg g = {}; g.downs1At = -1;
    const RtPar par = Lw9Par(16.37, 8.12);
    RtBegin(par);
    gRtSeed = 20261018u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = 1; L.noOnslowBand = noBand;
    int onIdx = 0, edge = 0;
    for (int win = 0; win < 200; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        double alone = 16.37;
        if (on && g.downs1At < 0 && onIdx > 12) alone = 19.5;                     // (the first stint: twelve windows, then the heavier scene)
        else if (g.downs1At >= 0 && !on)
        {   // (standing by at the edge: the rate with frame generation 40.6 three windows, 39.4 the fourth, from the law's own c and margin)
            const int c = FgDriveCUs(L.mem, 1, nullptr) + FgDriveCErrUs(L.mem, 1);
            alone = 1000.0 / ((edge++ % 4) == 3 ? 39.4 : 40.6) - c / 1000.0;
        }
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.005))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (r.prev == FGDR_ON && r.state != FGDR_ON && g.downs1At < 0) g.downs1At = win;
        if (g.downs1At >= 0 && win > g.downs1At)
        {
            if (r.prev == FGDR_ENGAGING && r.state == FGDR_STANDBY && r.row == FGC_ONSLOW) g.engagingExits++;
            if (r.state == FGDR_ON && r.prev != FGDR_ON) g.engagesAfter++;
        }
    }
    if (g.downs1At >= 0 && L.mem.engages > 1) g.engagesAfter = L.mem.engages - 1;   // (the law's own count: the cut poll engages inside a window)
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW21");
    return g;
}
static void CaseLW21()
{
    const char* const name = "LW21 FGC_ONSLOW's band in the wait for a cut: a game at the floor's edge never leaves the wait on FGC_ONSLOW, and engages (2026-10-18, phase 5 round 2, S2, review round 2)";
    CaseBegin(name);
    const Lw21Leg a = Lw21Run(0);
    LwPrint("ONE STINT, THEN STANDING BY AT THE EDGE (40.6, 40.6, 40.6, 39.4 by turns); THE TABLE WITH THE BAND:", a.w);
    printf("  the first stand-down behind window %d; behind it, the wait left on FGC_ONSLOW %d time(s), engages %d; %d frames, real frames shown once %d, out of place %d\n", a.downs1At, a.engagingExits, a.engagesAfter, a.f.n, a.f.real1, a.f.order);
    if (a.downs1At < 0) Err("LW21: the first stint never stood down (the heavier scene must stand it down)");
    if (a.engagingExits) Err("LW21: the wait for a cut left on FGC_ONSLOW %d time(s) (none wanted: the band is 37 there)", a.engagingExits);
    if (a.engagesAfter < 1) Err("LW21: no engage behind the stint's rest (one wanted: at a predicted 40.6 it enters the wait and stays)");
    if (a.f.order || a.f.real1 != a.f.n) Err("LW21: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const RingStats stc = gRingSt;
    const Lw21Leg c = Lw21Run(1);
    ViolRestore(stc);
    printf("  CONTROL, THE TABLE OF BEFORE (no band): the wait left on FGC_ONSLOW %d time(s), engages %d behind the stint\n", c.engagingExits, c.engagesAfter);
    if (c.engagingExits < 3) Err("LW21 control: the table of before left the wait on FGC_ONSLOW %d time(s) (3 or more wanted): the control shows nothing", c.engagingExits);
    Ctl("the table of before: the wait left on FGC_ONSLOW %d time(s)", c.engagingExits);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW22, A FREE CARD BEHIND A HEAVY SCENE IS NOT KEPT OUT (S2, review round 3)
// WHAT IT SHOWS. Round 2's rule (fg_cond.h, FgDriveCUs: a card with idle room and no c of its own takes the limit's c) kept a marginal game from
// a second optimistic engage on a flip of the card's reading, and could keep a FREE card out all session: c measured 8 ms in a heavy scene,
// then a light scene with idle room at 60 by itself predicts 1/(16.7 + 8 + margin) = 39.4, FGC_ONSLOW stands, and a c with room cannot be
// learnt without an engage. THE SCENE: by itself at 61.1 on a card that is the limit, 8.8 ms laid on engaged frames: ONE LONG STINT (65
// windows: no back-off behind it), c filed, a heavier scene stands it down (19.5 ms by itself); then, behind that stand-down, A LIGHT SCENE:
// by itself at 60 on a card with idle room, nothing laid on (frame generation's work fits in the idle), to window 220. HELD: engaged again
// within 40 windows of the light scene's start (the rest of 10, the idle run of FG_DRIVE_IDLE_PRIOR_WIN, the dwell and the cut wait), ON
// from there to the end, at most 4 changes of state behind the light scene's start; every real frame once and in order. A LIGHT SCENE ON A
// BUSY CARD (the limit read, 8.8 ms laid on, by itself at 60: 1 / (16.67 + 8.8 + margin) = 39.3) stays by.
struct Lw22Leg { std::vector<LwRec> w; LwFrames f; int downs1At, engAt, endState, changesAfter, lightAt, cLim; };
static Lw22Leg Lw22Run(int lightIdle)
{
    Lw22Leg g = {}; g.downs1At = -1; g.engAt = -1; g.lightAt = -1;
    const RtPar par = Lw9Par(16.37, 8.8);
    RtBegin(par);
    gRtSeed = 20261018u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = 1;
    int onIdx = 0; double lightMs = 0.0;
    for (int win = 0; win < 220; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        double alone = 16.37;
        if (g.downs1At >= 0) { alone = 16.67; L.card = lightIdle ? 0 : 1; gLwAddMs = lightIdle ? 0.0 : 8.8; }
        else if (on && onIdx > 65) alone = 19.5;                                   // (the heavy stint: 65 windows, a long one, then heavier)
        if (g.downs1At >= 0 && g.lightAt < 0) { g.lightAt = win; lightMs = (double)LwNowMs(); }
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (r.prev == FGDR_ON && r.state != FGDR_ON && g.downs1At < 0) g.downs1At = win;
        if (g.lightAt >= 0 && g.engAt < 0 && r.state == FGDR_ON) g.engAt = win;
    }
    for (double t : L.changesMs) if (t >= lightMs) g.changesAfter++;
    g.endState = L.mem.state; g.cLim = L.mem.cLimUs;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW22");
    return g;
}
static void CaseLW22()
{
    const char* const name = "LW22 a free card behind a heavy scene: by itself at 60 with idle room it engages again within 40 windows and stays, no cycle (2026-10-18, phase 5 round 2, S2, review round 3)";
    CaseBegin(name);
    const Lw22Leg a = Lw22Run(1);
    LwPrint("A LONG HEAVY STINT ON A BUSY CARD (8.8 ms laid on), THEN, BEHIND ITS STAND-DOWN, A LIGHT SCENE AT 60 WITH IDLE ROOM:", a.w);
    printf("  the heavy stint stood down behind window %d (c %d us); the light scene from window %d: engaged at window %d, %s at the end, %d change(s) of state behind its start; %d frames, real frames shown once %d, out of place %d\n",
        a.downs1At, a.cLim, a.lightAt, a.engAt, FgDriveStateName(a.endState), a.changesAfter, a.f.n, a.f.real1, a.f.order);
    if (a.downs1At < 0) Err("LW22: the heavy stint never stood down");
    if (a.engAt < 0 || a.engAt > a.lightAt + 40) Err("LW22: the light scene with idle room engaged at window %d (by window %d wanted, 40 behind its start: a free card is not kept out)", a.engAt, a.lightAt + 40);
    if (a.endState != FGDR_ON) Err("LW22: %s at the end (ON wanted: no cycle on a free card)", FgDriveStateName(a.endState));
    if (a.changesAfter > 4) Err("LW22: %d changes of state behind the light scene's start (4 at the most wanted)", a.changesAfter);
    if (a.f.order || a.f.real1 != a.f.n) Err("LW22: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const RingStats stc = gRingSt;
    const Lw22Leg b = Lw22Run(0);
    ViolRestore(stc);
    printf("  THE SAME LIGHT SCENE ON A BUSY CARD (the limit read, 8.8 ms laid on): engaged at window %d, %s at the end\n", b.engAt, FgDriveStateName(b.endState));
    if (b.engAt >= 0) Err("LW22 (b): the light scene on a busy card engaged at window %d (none wanted: 60 by itself with c 8 ms and its margin is under the floor, LW20)", b.engAt);
    CaseEnd(name);
}

// (THE TRUE FILL'S LAW's cases, LW23 to LW28, LW30, LW31 and LW34 to LW45, went with the fill in round 8's deep strip. LW29, LW32 and
// LW33, the doubling's card and probe, written beside them by G4's round 3, stay.)
// ---------------------------------------------------------------- LW29, THE CARD STANDING BY WITH VSYNC ON (the reviewer's third)
// THE LEGS' OWN LATENCY LINES standing by (one every five windows: the window, in flight, the latency pacer's share asleep, the present block, the
// period), through the product's reading and taking of the card (fg_cond.h, FgDriveCardOf and FgDriveCardTake, as fg_arm_life.h's
// FgpLawFreeX10 calls them): fill95 (the card the limit at 1455 MHz, DXVK holding the worker 3.6 to 6.4 ms a 9.4 to 11.1 ms frame), lawmarg
// (750 MHz) and flw (a free card). HELD: fill95 and lawmarg read the limit, flw idle room. CONTROL, THE READING OF BEFORE (no block): fill95
// reads idle room, as the leg's law did in 47 of 60 windows.
struct Lw29Row { int win, inFlight, sleptPc, blockUs, periodUs; };
static const Lw29Row kLw29Fill95[] = { { 5, 0, 0, 5950, 9580 }, { 15, 0, 0, 850, 16900 }, { 20, 0, 0, 5870, 10260 }, { 30, 0, 0, 4970, 10710 }, { 35, 0, 0, 4890, 9710 }, { 45, 0, 0, 5780, 10240 },
                                       { 50, 0, 0, 6400, 10080 }, { 60, 0, 0, 6020, 9370 }, { 65, 0, 0, 5700, 10410 }, { 75, 0, 0, 3640, 10100 }, { 85, 0, 0, 490, 16930 }, { 90, 0, 0, 5180, 11110 } };
static const Lw29Row kLw29Lawmarg[] = { { 5, 1, 0, 12400, 16210 }, { 40, 0, 0, 1160, 27180 }, { 45, 1, 0, 11170, 16920 }, { 50, 1, 0, 12490, 17820 }, { 55, 1, 0, 13490, 18530 },
                                        { 60, 1, 0, 13880, 18440 }, { 65, 1, 0, 12920, 16690 }, { 70, 1, 0, 11770, 16910 }, { 75, 1, 0, 10540, 16610 }, { 80, 1, 0, 12940, 17080 } };
static const Lw29Row kLw29Flw[] = { { 5, 0, 0, 0, 8350 }, { 10, 0, 0, 0, 8340 }, { 15, 0, 0, 10, 16870 }, { 20, 0, 0, 10, 16850 }, { 25, 0, 0, 10, 16910 }, { 30, 0, 0, 10, 16890 }, { 35, 0, 0, 10, 16910 },
                                    { 40, 0, 0, 10, 16910 }, { 45, 0, 0, 10, 16900 }, { 50, 0, 0, 10, 16940 }, { 55, 0, 0, 10, 16960 }, { 60, 0, 0, 10, 16890 } };
// the card the law takes over a leg's rows, one a window in their order (the lines are one in five windows: a sample of the leg's readings, not
// its run of them; a switch's own window, fill95's 15 and 85 at 0.05 and 0.03, is one reading among the limit's); before: the reading of before.
// Returns the windows taken as idle room, of n
static int Lw29Run(const Lw29Row* rows, int count, int before, int* n)
{
    FgDriveCardH h = {}; int idle = 0; *n = 0;
    for (int i = 0; i < count; i++)
        {
            const Lw29Row& x = rows[i];
            const int reading = FgDriveCardOf(x.inFlight * 10, x.sleptPc * 120 / 100, 120, before ? -1 : x.blockUs, before ? 0 : x.periodUs);
            if (FgDriveCardTake(h, reading, 0) == 0) idle++;
            (*n)++;
        }
    return idle;
}
static void CaseLW29()
{
    const char* const name = "LW29 the card standing by with vsync on is read from the present's block: fill95's card was the limit, flw's free (2026-10-18, G4's round 3)";
    CaseBegin(name);
    int nf = 0, nm = 0, nw = 0, nc = 0;
    const int f = Lw29Run(kLw29Fill95, 12, 0, &nf), m = Lw29Run(kLw29Lawmarg, 10, 0, &nm), w = Lw29Run(kLw29Flw, 12, 0, &nw), c = Lw29Run(kLw29Fill95, 12, 1, &nc);
    printf("  the windows taken as idle room: fill95 %d of %d, lawmarg %d of %d, flw %d of %d; CONTROL, the reading of before (no block): fill95 %d of %d\n", f, nf, m, nm, w, nw, c, nc);
    if (f || m) Err("LW29: fill95 read idle room in %d of %d windows and lawmarg in %d of %d (none wanted: the card was the limit)", f, nf, m, nm);
    if (w < nw - 2) Err("LW29: flw's free card read idle room in %d of %d windows (all but the first two, which the take's three readings need, wanted)", w, nw);
    if (c < nc / 2) Err("LW29 control: the reading of before took fill95's card as idle in %d of %d windows (most wanted, as the leg's law: the check cannot see the fault)", c, nc);
    Ctl("the reading of before: fill95's card idle in %d of %d windows", c, nc);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW32, THE PROBE: a game that gets lighter while held is seen (the reviewer's first)
// THE LATCH, confirmed by trace: held at the floor R_free is the rate read standing by, never over the samples' bound, so FAST engaged could never
// fire for a game that got lighter. THE CASE, THE LAW OVER THE SIMULATION (LwLawWindow, the product's reading through fg_cond.h's functions)
// WITH A STAGE IN THE PACER'S SLEEP (LwStages: the render thread's own part a third of the frame, so the held samples read about three times
// the game's rate, the legs' worst being 3.14): on the 120 Hz variable display a game at 62.5 by itself engages; from window 10 it gets lighter,
// to 90 by window 30, and stays there; five minutes. HELD: it is stood down (FGDRW_PROBE, then standing by with FGC_FAST: FGDRW_PROBED, or
// FGDRW_BAND where the probe's window moved the band, the law's band rule coming first) within 70 windows of reaching 90 and is standing by at
// the end, every real frame once and in order. (G4's round 4) A GAME IN BAND NEVER PROBES: at 64 throughout, no probe in five minutes, ON at the
// end, one engage. CONTROL, no probe (probeWin 0, the law before): ON to the end at the floor's 59.
struct Lw32Leg { int engagedAt, probeAt, offAt, offWhy, offRow, endState, turns, probes, held; LwFrames f; };
// the lognormal spread of a frame's cost, mean 1 (sigma: the spread of its logarithm), by Box and Muller on the simulation's own seed
static double LwLogSpread(double sigma)
{
    const double u1 = (RtSpread(1.0) + 1e-9) / 2.0000001, u2 = RtSpread(1.0) / 2.0;   // (RtSpread(1.0) is uniform on 0 to 2)
    const double z = sqrt(-2.0 * log(u1 < 1e-12 ? 1e-12 : u1)) * cos(6.283185307179586 * u2);
    return exp(sigma * z - sigma * sigma / 2.0);
}
// probe: the probe asked (probeWin as the law's, 0 none); climb: 1 the climb to 90, 0 the game at 64; wins; renderShare: the render thread's own
// part of the frame; logSigma: over 0 the lognormal spread of the frame (else 3 percent uniform); overPm: probeOverPm (0 the law's)
static Lw32Leg Lw32Run(int probe, int climb, int wins = 150, double renderShare = 1.0 / 3.0, double logSigma = 0.0, int overPm = 0)
{
    Lw32Leg g = {}; g.engagedAt = g.offAt = g.probeAt = -1;
    RtPar par; RtBegin(par);
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0; if (!probe) k.probeWin = 0; if (overPm > 0) k.probeOverPm = overPm;
    LwLaw L; LwLawBegin(L, k, FgCondDefaults()); L.card = 0;
    gRtSeed = 20261018u; LwStages st = {};
    for (int win = 0; win < wins; win++)
    {
        const double cost = !climb ? 15.6 : win < 10 ? 16.0 : win < 30 ? 16.0 - (16.0 - 11.1) * (double)(win - 10) / 20.0 : 11.1;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            const double f = cost * (logSigma > 0.0 ? LwLogSpread(logSigma) : RtSpread(0.03));
            RtFrame(RtMs(LwStageCostMs(st, f, f * renderShare * RtSpread(0.10)))); LwLawPoll(L);
        }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        if (r.state == FGDR_ON && g.engagedAt < 0) g.engagedAt = win;
        if (r.prev == FGDR_ON && r.state == FGDR_ON && FgDriveHeldUpX10(gLwMedUs, gLwPerUs, FgCondFloorUs(LwSession(par)), r.med) > 0) g.held++;   // (the window as the probe read it)
        // (the probe's window standing by: FGDRW_PROBED with the row it read, or FGDRW_BAND where the band moved there, the law's band rule first)
        if (r.state == FGDR_REST && r.why == FGDRW_PROBE) { g.probes++; if (g.probeAt < 0) g.probeAt = win; }
        if (g.probeAt >= 0 && win > g.probeAt && r.state == FGDR_STANDBY && g.offAt < 0) { g.offAt = win; g.offWhy = r.why; g.offRow = r.row; }
    }
    g.endState = L.mem.state; g.turns = LwTurnsIn5Min(L); g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    RtEnd("LW32");
    return g;
}
static void CaseLW32()
{
    const char* const name = "LW32 the probe: a game that gets lighter while held at the floor is seen and stood down; one in band stays (2026-10-18, G4's round 3)";
    CaseBegin(name);
    const Lw32Leg a = Lw32Run(1, 1), b = Lw32Run(1, 0), c = Lw32Run(0, 1);
    printf("  62.5 then 90 by window 30: engaged at window %d, the probe behind window %d, standing by behind window %d (%s, %s), %s at the end, %d engages and stand-downs in five minutes; %d of %d real frames shown once, out of place %d\n",
        a.engagedAt, a.probeAt, a.offAt, FgDriveWhyName(a.offWhy), FgCondEnumName(a.offRow), FgDriveStateName(a.endState), a.turns, a.f.real1, a.f.n, a.f.order);
    printf("  at 64 throughout: engaged at %d, %d probe(s), %s at the end, %d engages and stand-downs in five minutes; CONTROL, no probe: %s at the end\n",
        b.engagedAt, b.probes, FgDriveStateName(b.endState), b.turns, FgDriveStateName(c.endState));
    if (a.engagedAt < 0 || a.offAt < 0 || a.offAt > 30 + 70 || a.offRow != FGC_FAST || a.endState == FGDR_ON) Err("LW32: engaged at %d, off at %d by %s, %s at the end (off by FGC_FAST within 70 windows of reaching 90 at window 30, not ON at the end, wanted)", a.engagedAt, a.offAt, FgCondEnumName(a.offRow), FgDriveStateName(a.endState));
    if (a.f.order || a.f.real1 != a.f.n) Err("LW32: %d picture(s) out of place (%s), %d of %d real frames shown once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    if (b.endState != FGDR_ON || b.probes || b.turns > 1) Err("LW32: the game in band: %d probe(s), %s at the end, %d engages and stand-downs (none, ON, the one engage wanted)", b.probes, FgDriveStateName(b.endState), b.turns);
    if (c.endState != FGDR_ON) Err("LW32 control: with no probe the lighter game ended %s (ON wanted, the latch: the check cannot see it)", FgDriveStateName(c.endState));
    Ctl("no probe: the lighter game %s at the end", FgDriveStateName(c.endState));
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW33, A GAME IN BAND NEVER PROBES (G4's round 4, the main session's)
// (a) LAWEDGE'S OWN WINDOWS (sitting-2026-10-18b, the game alone about 64 to 74, ON 89 windows): each ON window's R_on, E and the pacing line's
// period and the panel's floor, through the product's readings of the probe (fg_cond.h, FgDriveHeldUpX10 with E's period standing for the law's
// median, FgDriveFastX10) and the law's step from ON: no probe over the whole leg (no window held at the floor: the card was the limit, the
// period 18.25 to 24.5 ms). (b) TWENTY MINUTES OF THE SIMULATION at 64 by itself WITH THAT LEG'S SPREAD (its standing-by windows' frames: a
// window's longest about twice its mean, 27 to 36 ms against 13.5 to 15.2: a lognormal spread of 0.33) and the render thread's part the worst
// share the legs showed under the hold (the bound 3.14 times the game's rate): no probe, ON at the end (TOLD: with that spread the law's own
// period follows the game's tail over the floor in most windows, so few are held at all). (c) THE WORST CASE FOR THE BOUND, the same twenty
// minutes with a spread of 3 percent, held at the floor nearly every window: no probe. CONTROL, ROUND 3'S RULE (the bound over FAST's bound by
// nothing) on (c): it probes.
struct Lw33Row { int win, onX10, freeX10, eX10, perUs, floorUs; };
static const Lw33Row kLw33Edge[] = {
    { 23, 417, 417, 1306, 24000, 16916 }, { 24, 497, 497, 1407, 20000, 16916 }, { 25, 489, 489, 1277, 21000, 16916 }, { 26, 458, 458, 1181, 22000, 16916 }, { 27, 458, 458, 1065, 21750, 16916 },
    { 28, 456, 456, 899, 22500, 16916 }, { 29, 446, 446, 740, 22250, 16916 }, { 30, 450, 450, 620, 20500, 16916 }, { 31, 489, 489, 657, 18250, 16916 }, { 32, 471, 471, 1334, 21250, 16916 }, { 33, 509, 509, 1334, 20250, 16916 },
    { 34, 493, 493, 1209, 22250, 16916 }, { 35, 450, 450, 1346, 20500, 16916 }, { 36, 494, 494, 1384, 19750, 16916 }, { 37, 491, 491, 1426, 20500, 16916 }, { 38, 479, 479, 1233, 21250, 16916 }, { 39, 469, 469, 1190, 22000, 16916 },
    { 40, 450, 450, 1257, 21500, 16916 }, { 41, 480, 480, 1403, 20500, 16916 }, { 42, 480, 480, 1441, 22000, 16916 }, { 43, 455, 455, 1546, 21250, 16916 }, { 44, 485, 485, 1609, 20500, 16916 }, { 45, 539, 539, 1587, 18500, 16916 },
    { 46, 473, 473, 1491, 23250, 16916 }, { 47, 425, 425, 1117, 23500, 16916 }, { 48, 456, 456, 1117, 19750, 16916 }, { 49, 471, 471, 1074, 21250, 16916 }, { 50, 459, 459, 1046, 22000, 16916 }, { 51, 465, 465, 894, 21500, 16916 },
    { 52, 469, 469, 1003, 21250, 16916 }, { 53, 491, 491, 950, 19750, 16916 }, { 54, 508, 508, 1097, 19750, 16916 }, { 55, 501, 501, 914, 22250, 16916 }, { 56, 453, 453, 816, 21750, 16916 }, { 57, 460, 460, 1160, 22500, 16916 },
    { 58, 445, 445, 1366, 20500, 16916 }, { 59, 493, 493, 1377, 20250, 16916 }, { 60, 464, 464, 1299, 22250, 16916 }, { 61, 446, 446, 1328, 22000, 16916 }, { 62, 462, 462, 1215, 22500, 16916 }, { 63, 450, 450, 1170, 21250, 16916 },
    { 64, 470, 470, 1247, 23000, 16916 }, { 65, 434, 434, 1240, 23750, 16916 }, { 66, 421, 421, 1518, 21250, 16916 }, { 67, 473, 473, 1584, 20000, 16916 }, { 68, 484, 484, 1389, 21000, 16916 }, { 69, 476, 476, 1283, 21000, 16916 },
    { 70, 485, 485, 1381, 20250, 16916 }, { 71, 506, 506, 1387, 20000, 16916 }, { 72, 484, 484, 1295, 20750, 16916 }, { 73, 494, 494, 1071, 20250, 16916 }, { 74, 479, 479, 1110, 21500, 16916 }, { 75, 459, 459, 1058, 22500, 16916 },
    { 76, 449, 449, 1312, 20500, 16916 }, { 77, 489, 489, 1415, 23000, 16916 }, { 78, 435, 435, 1450, 20250, 16916 }, { 79, 508, 508, 1436, 19750, 16916 }, { 80, 492, 492, 1226, 20750, 16916 }, { 81, 477, 477, 1177, 21250, 16916 },
    { 82, 474, 474, 1225, 23250, 16916 }, { 83, 431, 431, 1466, 19250, 16916 }, { 84, 520, 520, 1386, 20250, 16916 }, { 85, 489, 489, 1381, 20500, 16916 }, { 86, 505, 505, 1412, 19250, 16916 }, { 87, 461, 461, 1127, 21750, 16916 },
    { 88, 461, 461, 1196, 21750, 16916 }, { 89, 442, 442, 1295, 22750, 16916 }, { 90, 504, 504, 1310, 19750, 16916 }, { 91, 508, 508, 1141, 21250, 16916 }, { 92, 470, 470, 1148, 20500, 16916 }, { 93, 486, 486, 1224, 20750, 16916 },
    { 94, 481, 481, 1356, 21000, 16916 }, { 95, 461, 461, 1499, 22000, 16916 }, { 96, 446, 446, 1532, 22500, 16916 }, { 97, 464, 464, 1376, 21000, 16916 }, { 98, 477, 477, 1406, 20000, 16916 }, { 99, 416, 416, 1360, 24250, 16916 },
    { 100, 515, 515, 1396, 19250, 16916 }, { 101, 517, 517, 1396, 19500, 16916 }, { 102, 512, 512, 1555, 19500, 16916 }, { 103, 511, 511, 1456, 20000, 16916 }, { 104, 477, 477, 1449, 21000, 16916 }, { 105, 535, 535, 1476, 18500, 16916 },
    { 106, 511, 511, 1386, 19750, 16916 }, { 107, 503, 503, 1396, 20250, 16916 }, { 108, 411, 411, 1491, 24500, 16916 }, { 109, 440, 440, 1380, 22750, 16916 }, { 110, 425, 425, 1211, 24000, 16916 } };
static void CaseLW33()
{
    const char* const name = "LW33 a game in band never probes: lawedge's whole leg and twenty minutes at 64 with its spread (2026-10-19, G4's round 4)";
    CaseBegin(name);
    // (a) lawedge's windows through the law from ON, as fg_arm_life.h hands them (the table's FAST bound engaged at 120 Hz on a variable display)
    FgCondIn s = {}; s.refreshNum = 120; s.refreshDen = 1; s.lockPm = 1015; s.vrr = 1;
    const int fast = FgDriveFastX10(s, FgCondDefaults());
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;
    FgDriveMem m; FgDriveInit(m, k);
    FgDriveIn sb = {}; sb.row = FGC_OK; sb.band = FGDB_IN; sb.cpuReady = 1; sb.realX10 = 650; sb.card = 1;
    unsigned now = 0;
    for (int w = 0; w < 3; w++) { FgDriveStep(m, sb, k, now, 0, 0); now += 2000; }
    const int n = (int)(sizeof(kLw33Edge) / sizeof(kLw33Edge[0]));
    int held = 0, probes = 0;
    for (int i = 0; i < n; i++)
    {
        const Lw33Row& x = kLw33Edge[i];
        FgDriveIn d = sb; d.realX10 = x.onX10; d.shownN = 120; d.shownGen = 118; d.perUs = x.perUs; d.lawPerUs = x.perUs; d.floorUs = x.floorUs;
        d.heldUpX10 = FgDriveHeldUpX10(x.eX10 > 0 ? (int)(10000000LL / x.eX10) : 0, x.perUs, x.floorUs, 10000.0 / x.onX10);
        d.fastX10 = fast;
        if (d.heldUpX10) held++;
        FgDriveStep(m, d, k, now, 0, 0); now += 2000;
        if (m.state == FGDR_REST && m.why == FGDRW_PROBE) probes++;
    }
    printf("  (a) lawedge's %d ON windows: held at the floor in %d, probes %d, %s at the end (FAST's bound engaged %d.%d, the probe's %d.%d)\n", n, held, probes, FgDriveStateName(m.state), fast / 10, fast % 10, fast * k.probeOverPm / 10000, (fast * k.probeOverPm / 1000) % 10);
    if (probes || m.state != FGDR_ON) Err("LW33 (a): lawedge's windows made %d probe(s), %s at the end (none, ON, wanted)", probes, FgDriveStateName(m.state));
    // (b) twenty minutes at 64 with the leg's spread, the render thread's part the worst share under the hold
    const Lw32Leg b = Lw32Run(1, 0, 600, 1.0 / 3.14, 0.33), w = Lw32Run(1, 0, 600, 1.0 / 3.14, 0.0), c = Lw32Run(1, 0, 600, 1.0 / 3.14, 0.0, 1000);
    printf("  (b) twenty minutes at 64, the leg's spread (0.33), the render thread's part 1/3.14 of the frame: held in %d windows, %d probe(s), %s at the end, %d of %d real frames shown once, out of place %d\n",
        b.held, b.probes, FgDriveStateName(b.endState), b.f.real1, b.f.n, b.f.order);
    printf("  (c) the same with a spread of 3 percent: held in %d windows, %d probe(s), %s at the end; CONTROL, round 3's rule: %d probe(s)\n", w.held, w.probes, FgDriveStateName(w.endState), c.probes);
    if (b.probes || b.endState != FGDR_ON) Err("LW33 (b): %d probe(s), %s at the end (none, ON, wanted)", b.probes, FgDriveStateName(b.endState));
    if (b.f.order || b.f.real1 != b.f.n) Err("LW33 (b): %d picture(s) out of place, %d of %d real frames shown once", b.f.order, b.f.real1, b.f.n);
    if (w.probes || w.endState != FGDR_ON || w.held < 500) Err("LW33 (c): held in %d windows, %d probe(s), %s at the end (500 or more held, none, ON, wanted)", w.held, w.probes, FgDriveStateName(w.endState));
    if (c.probes < 1) Err("LW33 control: round 3's rule made %d probe(s) in twenty minutes (1 or more wanted, as LW32's game at 64 did: the check cannot see it)", c.probes);
    Ctl("round 3's rule: %d probe(s) in twenty minutes at 64", c.probes);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW46 to LW50, K: THE HELPER'S ANSWER ON THE CARD, THE LEAST c (round 8, P, 2026-10-25)
// WHAT THEY SHOW (fg_cond.h, FgDrivePredOnX10 and FgDriveKMedUs; fg_arm_life.h, FgpLawKUs and FgpLawPredOnX10). The rates read c only from a
// stint that ran whole or stood down by FGC_SLOW, and as 0 where the pacer's period holds the real frame; K, the helper's answer behind every
// handoff over full dispatches (the placer's kFull, this rig's machine fills it as the product's does), is read from every window a stint
// runs. FGC_ONSLOW asks the larger of c with its margin and K where the card is the limit or cannot be read, outside the fixed mode.
// THE SCENE OF LW46 TO LW48 AND LW50: the rig's variable 120 Hz panel, the floor 40 (out under 37). A FIRST STINT THE RATES CANNOT READ: by
// itself at 50 the cold engage (nothing read: optimistic, as before, and the same in both arms), then from the stint's fourth window the
// scene lightens to 85, held at the top: the rates file a c far under what is laid on (the pacer's hold hides it, the legs' 0); in its
// fourteenth window NVIDIA's cap is set for one window (FGC_NVCAP, a settings row: standing by at once, no rest). THEN THE SCENE: by
// itself at R, the card as the case says. LAID ON: frame generation's cost as time in every engaged present (LW9's stand-in, gLwAddMs) and
// the helper's answer that long behind every handoff (RtPar::kUs), equal, the product's model (K a part of c; the rig has no hand's span and
// no writes, so here K is all of it). THE CONTROL OF EACH: the law of before (LwLaw::noStamps), or the one number its edge stands on.
// SAID: the rig proves the law's logic and its reading of the placer's counts, not that K is a part of c on a real card: that is the legs'.
struct LwKLeg { std::vector<LwRec> w; LwFrames f; int engages, downs, fastAt, sceneAt, engAfter, onslowAfter, engagingExits, changesAfter, endState, cStamp, cLim, cLimFirst; double lastOn; };
static LwKLeg LwKRun(double sceneFps, double addMs, LONG kUs, int card, int noStamps, int edge = 0, int noBand = 0)
{
    LwKLeg g = {}; g.fastAt = -1; g.sceneAt = -1;
    RtPar par = Lw9Par(20.0, addMs); par.kUs = kUs; if (kUs + 500 > par.planUs) par.planUs = kUs + 500;   // (the plan over the answer: shown, as the product's plan learns)
    RtBegin(par);
    gRtSeed = 20261025u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = card; L.noStamps = noStamps; L.noOnslowBand = noBand;
    int onIdx = 0, e = 0; double sceneMs = 0.0;
    for (int win = 0; win < 110; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        double alone = 20.0;
        L.nvCap = 0;
        if (g.fastAt < 0 && on && onIdx > 3) alone = 1000.0 / 85.0;               // (the first stint from its fourth window: lighter, held at the top)
        if (g.fastAt < 0 && on && onIdx == 14) L.nvCap = 100;                     // (its fourteenth: NVIDIA's cap set for a window, a settings row)
        else if (g.fastAt >= 0)
        {
            if (g.sceneAt < 0) { g.sceneAt = win; sceneMs = (double)LwNowMs(); g.cLimFirst = L.mem.cLimUs; }   // (c as the first stint left it)
            alone = 1000.0 / sceneFps;
            // (LW50: standing by AT THE FLOOR'S EDGE from the law's own K: 40.6 three windows, 39.4 the fourth, LW21's pattern)
            if (edge && !on) alone = 1000.0 / ((e++ % 4) == 3 ? 39.4 : 40.6) - L.mem.cStampUs / 1000.0;
        }
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (g.fastAt < 0 && r.prev == FGDR_ON && r.state != FGDR_ON) g.fastAt = win;
        if (g.sceneAt >= 0 && win >= g.sceneAt)
        {
            if (r.row == FGC_ONSLOW && r.state != FGDR_ON) g.onslowAfter++;
            if (r.prev == FGDR_ENGAGING && r.state == FGDR_STANDBY && r.row == FGC_ONSLOW) g.engagingExits++;
        }
        if (r.state == FGDR_ON && r.med > 0) g.lastOn = 1000.0 / r.med;
    }
    for (double t : L.changesMs) if (t >= sceneMs && g.sceneAt >= 0) g.changesAfter++;
    g.engages = L.mem.engages; g.downs = L.mem.downs; g.engAfter = L.mem.engages - 1; g.endState = L.mem.state; g.cStamp = L.mem.cStampUs; g.cLim = L.mem.cLimUs;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW46");
    return g;
}
static void LwKSay(const char* what, const LwKLeg& g)
{
    printf("  %s: the first stint stood down behind window %d, the scene from window %d; engages %d (%d behind the scene's start), stand-downs %d, %s at the end;"
           " windows standing by on FGC_ONSLOW behind the scene's start %d, the wait for a cut left on it %d, changes of state behind it %d; K as the law keeps it %d us,"
           " c with the limit %d us behind the first stint, %d us at the end; R_on in the last engaged window %.1f; %d frames, real frames shown once %d, out of place %d\n",
           what, g.fastAt, g.sceneAt, g.engages, g.engAfter, g.downs, FgDriveStateName(g.endState), g.onslowAfter, g.engagingExits, g.changesAfter, g.cStamp, g.cLimFirst, g.cLim, g.lastOn, g.f.n, g.f.real1, g.f.order);
}
static bool LwKFirstOk(const char* tag, const LwKLeg& g)
{
    bool ok = true;
    if (g.fastAt < 0 || g.sceneAt < 0) { Err("%s: the first stint never stood down (FGC_NVCAP in its fourteenth window wanted)", tag); ok = false; }
    if (g.cLimFirst >= 1500) { Err("%s: the first stint filed c %d us from the rates (under 1,500 wanted: the pacer's hold hides what is laid on)", tag, g.cLimFirst); ok = false; }
    if (g.f.order || g.f.real1 != g.f.n) { Err("%s: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", tag, g.f.order, g.f.first.c_str(), g.f.real1, g.f.n); ok = false; }
    return ok;
}
// LW46: A WEAK CARD NEAR THE FLOOR, BY ITSELF AT 45, c 3 ms: K read in the first stint (3,125 us, its bin's middle) predicts 39.4, under the
// floor: FGC_ONSLOW stands to the end and no engage follows the cold one. CONTROL, THE LAW OF BEFORE: c as the hold let the rates read it,
// it predicts over the floor and engages into a real rate of about 39.6, under it (the base's behaviour: it FAILS this case).
static void CaseLW46()
{
    const char* const name = "LW46 K the least c: a weak card by itself at 45 with c 3 ms, the rates blind, predicts about 39.4 and does not engage (round 8, P, 2026-10-25)";
    CaseBegin(name);
    const LwKLeg a = LwKRun(45.0, 3.0, 3000, 1, 0);
    LwKSay("BY ITSELF AT 45, THE CARD THE LIMIT, c 3 ms, K READ", a);
    LwKFirstOk("LW46", a);
    if (a.cStamp < 2900 || a.cStamp > 3400) Err("LW46: K kept %d us (about 3,125 wanted: 3,000 behind every handoff, its bin's middle)", a.cStamp);
    if (a.engAfter != 0 || a.endState == FGDR_ON || a.onslowAfter < 20) Err("LW46: %d engage(s) behind the scene's start, %s at the end, %d window(s) on FGC_ONSLOW (0, not ON, 20 or more wanted)", a.engAfter, FgDriveStateName(a.endState), a.onslowAfter);
    const RingStats stc = gRingSt;
    const LwKLeg c = LwKRun(45.0, 3.0, 3000, 1, 1);
    ViolRestore(stc);
    LwKSay("CONTROL, THE LAW OF BEFORE (no K)", c);
    if (c.engAfter < 1) Err("LW46 control: the law of before made %d engage(s) behind the scene's start (1 or more wanted: it engages into about 39.6): the control shows nothing", c.engAfter);
    else Ctl("the law of before engaged %d time(s) behind the scene's start, R_on %.1f in its last engaged window", c.engAfter, c.lastOn);
    CaseEnd(name);
}
// LW47: THE SAME CARD BY ITSELF AT 50: K predicts 43.2, over the floor: it engages and stays, R_on over the 37 out. CONTROL, K 6 ms (c 6
// ms): 38.3 predicted, FGC_ONSLOW stands (the case turns on K; the law of before engages there too and FAILS the control).
static void CaseLW47()
{
    const char* const name = "LW47 K the least c: the same card by itself at 50 engages again and stays (about 43.2 predicted) (round 8, P, 2026-10-25)";
    CaseBegin(name);
    const LwKLeg a = LwKRun(50.0, 3.0, 3000, 1, 0);
    LwKSay("BY ITSELF AT 50, THE CARD THE LIMIT, c 3 ms, K READ", a);
    LwKFirstOk("LW47", a);
    if (a.engAfter != 1 || a.endState != FGDR_ON || a.lastOn < 37.0) Err("LW47: %d engage(s) behind the scene's start, %s at the end, R_on %.1f (1, ON, 37 or more wanted)", a.engAfter, FgDriveStateName(a.endState), a.lastOn);
    const RingStats stc = gRingSt;
    const LwKLeg c = LwKRun(50.0, 6.0, 6000, 1, 0);
    ViolRestore(stc);
    LwKSay("CONTROL, K AND c 6 ms", c);
    if (c.engAfter != 0 || c.onslowAfter < 20) Err("LW47 control: K 6 ms made %d engage(s) behind the scene's start, %d window(s) on FGC_ONSLOW (0 and 20 or more wanted: 38.3 predicted): the control shows nothing", c.engAfter, c.onslowAfter);
    else Ctl("K %d us: no engage behind the scene's start, %d window(s) on FGC_ONSLOW", c.cStamp, c.onslowAfter);
    CaseEnd(name);
}
// LW48: A CARD WITH IDLE ROOM keeps the law of before: by itself at 41, nothing laid on (its work fits the idle), K 3 ms: K is neither filed
// (the stint engaged on idle room) nor read (idle room standing by): it engages again and stays. CONTROL, THE CARD THE LIMIT with the same K
// and c 3 ms: 36.3 predicted, FGC_ONSLOW stands (the law of before engages there and FAILS the control).
static void CaseLW48()
{
    const char* const name = "LW48 K the least c: a card with idle room by itself at 41 engages again as before; the same K on a card that is the limit stays by (round 8, P, 2026-10-25)";
    CaseBegin(name);
    const LwKLeg a = LwKRun(41.0, 0.0, 3000, 0, 0);
    LwKSay("BY ITSELF AT 41, THE CARD WITH IDLE ROOM, NOTHING LAID ON, K 3 ms", a);
    LwKFirstOk("LW48", a);
    if (a.cStamp != 0) Err("LW48: K kept %d us from a stint that engaged on idle room (none wanted)", a.cStamp);
    if (a.engAfter != 1 || a.endState != FGDR_ON) Err("LW48: %d engage(s) behind the scene's start, %s at the end (1 and ON wanted: the law of before)", a.engAfter, FgDriveStateName(a.endState));
    const RingStats stc = gRingSt;
    const LwKLeg c = LwKRun(41.0, 3.0, 3000, 1, 0);
    ViolRestore(stc);
    LwKSay("CONTROL, THE CARD THE LIMIT, c AND K 3 ms", c);
    if (c.engAfter != 0 || c.onslowAfter < 20) Err("LW48 control: on a card that is the limit %d engage(s) behind the scene's start, %d window(s) on FGC_ONSLOW (0 and 20 or more wanted): the control shows nothing", c.engAfter, c.onslowAfter);
    else Ctl("the card the limit: no engage behind the scene's start, %d window(s) on FGC_ONSLOW", c.onslowAfter);
    CaseEnd(name);
}
// LW49: THE FIXED MODE IS UNCHANGED (its rate is half the refresh): through the reader as fg_arm_life.h's FgpCondRead calls it (LwFill,
// FgCondFixedMode of the session), a fixed 120 Hz screen with vsync (FGC_ONSLOW asks the mode's floor, half the refresh), standing by at 62
// with K 3,125 us kept and no c: the prediction is the law of before's to the tenth (62.0) and FGC_ONSLOW does not hold, where K would have
// read 51.9 and held it. CONTROL, THE VARIABLE PANEL at 41.5 with the same memory: 36.7, FGC_ONSLOW holds (the law of before does not:
// it FAILS the control).
static void CaseLW49()
{
    const char* const name = "LW49 K the least c: the fixed mode is unchanged, its prediction the law of before's (round 8, P, 2026-10-25)";
    CaseBegin(name);
    const RtPar par;
    const FgDriveK k = FgDriveDefaults(); const FgCondK ck = FgCondDefaults();
    FgDriveMem m; FgDriveInit(m, k); m.cStampUs = 3125;
    FgCondIn fx = LwSession(par); fx.vrr = 0; fx.flipQuant = 0; fx.fixedKey = 1; fx.vsync = 1;
    LwFill(fx, LW_NOW, false, 1000.0 / 62.0, 0, &m, &k, 1, 1, 1);
    const int before = FgDrivePredHiX10(m, fx.realFpsX10, 1), withK = FgDrivePredOnX10(m, fx.realFpsX10, 1, 0);
    const bool fxHolds = FgCondHolds(FGC_ONSLOW, fx, ck);
    FgCondIn fk = fx; fk.predFpsX10 = withK; const bool kHolds = FgCondHolds(FGC_ONSLOW, fk, ck);
    printf("  THE FIXED MODE (%s, the floor %d): by itself %d.%d, predicted %d.%d (the law of before %d.%d; with K %d.%d, which %s), FGC_ONSLOW %s\n", FgCondFixedMode(fx) ? "on" : "NOT ON",
           FgCondFloorX10(fx, ck) / 10, fx.realFpsX10 / 10, fx.realFpsX10 % 10, fx.predFpsX10 / 10, fx.predFpsX10 % 10, before / 10, before % 10, withK / 10, withK % 10,
           kHolds ? "would hold it" : "would not hold it", fxHolds ? "HOLDS" : "does not hold");
    if (!FgCondFixedMode(fx)) Err("LW49: the session is not the fixed mode (the case's own setting is wrong)");
    if (fx.predFpsX10 != before || fxHolds) Err("LW49: in the fixed mode predicted %d against the law of before's %d, FGC_ONSLOW %s (equal and not holding wanted)", fx.predFpsX10, before, fxHolds ? "holds" : "does not hold");
    if (!kHolds) Err("LW49: K read in the fixed mode would not hold FGC_ONSLOW either (it must, or the case shows nothing)");
    FgCondIn vr = LwSession(par);
    LwFill(vr, LW_NOW, false, 1000.0 / 41.5, 0, &m, &k, 1, 1, 1);
    const bool vrHolds = FgCondHolds(FGC_ONSLOW, vr, ck);
    printf("  CONTROL, THE VARIABLE PANEL AT 41.5: predicted %d.%d, FGC_ONSLOW %s\n", vr.predFpsX10 / 10, vr.predFpsX10 % 10, vrHolds ? "HOLDS" : "does not hold");
    if (!vrHolds || vr.predFpsX10 >= 415) Err("LW49 control: on the variable panel predicted %d, FGC_ONSLOW %s (under 415 and holding wanted): the control shows nothing", vr.predFpsX10, vrHolds ? "holds" : "does not hold");
    else Ctl("the variable panel: %d.%d predicted, FGC_ONSLOW holds", vr.predFpsX10 / 10, vr.predFpsX10 % 10);
    CaseEnd(name);
}
// LW50: NO FLAP AT THE FLOOR'S EDGE WITH K: behind the first stint, standing by at a predicted 40.6 three windows and 39.4 the fourth, from
// the law's own K (LW21's pattern). K is only read engaged, so standing by nothing of it moves: the wait for a cut is never left on
// FGC_ONSLOW, it engages and stays, at most 4 changes of state behind the scene's start. CONTROL, THE TABLE WITHOUT THE BAND IN THE WAIT
// (LW21's control): the wait is left on FGC_ONSLOW.
static void CaseLW50()
{
    const char* const name = "LW50 K the least c: no flap at the floor's edge, the wait for a cut never left on FGC_ONSLOW (round 8, P, 2026-10-25)";
    CaseBegin(name);
    const LwKLeg a = LwKRun(46.5, 3.0, 3000, 1, 0, 1);
    LwKSay("AT THE FLOOR'S EDGE FROM K (40.6, 40.6, 40.6, 39.4)", a);
    LwKFirstOk("LW50", a);
    if (a.cStamp < 2900 || a.cStamp > 3400) Err("LW50: K kept %d us (about 3,125 wanted)", a.cStamp);
    if (a.engagingExits || a.engAfter != 1 || a.endState != FGDR_ON || a.changesAfter > 4) Err("LW50: the wait left on FGC_ONSLOW %d time(s), %d engage(s) behind the scene's start, %s at the end, %d change(s) of state (0, 1, ON, 4 at the most wanted)", a.engagingExits, a.engAfter, FgDriveStateName(a.endState), a.changesAfter);
    const RingStats stc = gRingSt;
    const LwKLeg c = LwKRun(46.5, 3.0, 3000, 1, 0, 1, 1);
    ViolRestore(stc);
    LwKSay("CONTROL, NO BAND IN THE WAIT", c);
    if (c.engagingExits < 1) Err("LW50 control: without the band the wait was left on FGC_ONSLOW %d time(s) (1 or more wanted): the control shows nothing", c.engagingExits);
    else Ctl("without the band the wait was left on FGC_ONSLOW %d time(s)", c.engagingExits);
    CaseEnd(name);
}
// LW51: A SHORT, HITCHY STINT FILES NO K THAT KEEPS FRAME GENERATION OFF (round 8, P, the review of c8821d7b, 2026-10-25). By itself at 60 on
// a card that is the limit, 1 ms laid on and the helper's answer 1 ms behind every handoff; THE FIRST STINT HITCHES: every answer of its
// windows 12 ms late from the first it starts ON in (RtSim::kNext, a hitch of the helper's own), and NVIDIA's cap set in its second such
// window, the warm-up's last (the engage's window, ON from the cut poll, is the warm-up's first), stands it down (a
// settings row) inside its warm-up. Then by itself at 60 again. HELD: K is never filed from a stint that did not leave its warm-up (fg_cond.h,
// FgDriveStep), so the law engages again and stays ON. ON c8821d7b IT FAILS: K 12,125 us from the warm-up, 34.7 predicted, FGC_ONSLOW to
// the end, frame generation kept off for the session. CONTROL, THE HITCHES BEHIND THE WARM-UP (six windows of them, the cap in the sixth):
// K is filed from the windows behind it as before, and the law stays by (the gate takes out the warm-up, not K).
struct Lw51Leg { std::vector<LwRec> w; LwFrames f; int engages, endState, cStamp, onslowAfter, downAt; };
static Lw51Leg Lw51Run(int hitchWins)
{
    Lw51Leg g = {}; g.downAt = -1;
    RtPar par = Lw9Par(16.67, 1.0); par.kUs = 1000;
    RtBegin(par);
    gRtSeed = 20261025u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.card = 1;
    int onIdx = 0;
    for (int win = 0; win < 70; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        const bool hitch = g.downAt < 0 && on && onIdx <= hitchWins;
        L.nvCap = (g.downAt < 0 && on && onIdx == hitchWins) ? 100 : 0;          // (the stint's last hitchy window: NVIDIA's cap, a settings row)
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { if (hitch) gRt.kNext = 12000; RtFrame(RtMs(16.67 * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (g.downAt < 0 && r.prev == FGDR_ON && r.state != FGDR_ON) g.downAt = win;
        if (g.downAt >= 0 && win > g.downAt && r.row == FGC_ONSLOW && r.state != FGDR_ON) g.onslowAfter++;
    }
    g.engages = L.mem.engages; g.endState = L.mem.state; g.cStamp = L.mem.cStampUs;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW51");
    return g;
}
static void CaseLW51()
{
    const char* const name = "LW51 K the least c: a short, hitchy stint files no K that keeps frame generation off (round 8, P, the review of c8821d7b)";
    CaseBegin(name);
    const Lw51Leg a = Lw51Run(2);
    printf("  THE STINT HITCHES IN ITS WARM-UP AND ENDS IN IT: stood down behind window %d; K as the law keeps it %d us; engages %d, %s at the end, windows on FGC_ONSLOW behind it %d; %d frames, real frames shown once %d, out of place %d\n",
           a.downAt, a.cStamp, a.engages, FgDriveStateName(a.endState), a.onslowAfter, a.f.n, a.f.real1, a.f.order);
    if (a.downAt < 0) Err("LW51: the hitchy stint never stood down (FGC_NVCAP in the warm-up's last window wanted)");
    if (a.cStamp > 2000 || a.engages < 2 || a.endState != FGDR_ON || a.onslowAfter > 0) Err("LW51: K %d us, %d engage(s), %s at the end, %d window(s) on FGC_ONSLOW (2,000 at the most, 2 or more, ON and none wanted: the warm-up's hitches kept frame generation off)", a.cStamp, a.engages, FgDriveStateName(a.endState), a.onslowAfter);
    if (a.f.order || a.f.real1 != a.f.n) Err("LW51: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const RingStats stc = gRingSt;
    const Lw51Leg c = Lw51Run(6);
    ViolRestore(stc);
    printf("  CONTROL, THE HITCHES BEHIND THE WARM-UP (six windows): stood down behind window %d; K %d us; engages %d, %s at the end, windows on FGC_ONSLOW behind it %d\n",
           c.downAt, c.cStamp, c.engages, FgDriveStateName(c.endState), c.onslowAfter);
    if (c.cStamp < 10000 || c.engages != 1 || c.onslowAfter < 20) Err("LW51 control: hitches behind the warm-up filed K %d us, %d engage(s), %d window(s) on FGC_ONSLOW (over 10,000, 1 and 20 or more wanted): the control shows nothing", c.cStamp, c.engages, c.onslowAfter);
    else Ctl("hitches behind the warm-up: K %d us filed, FGC_ONSLOW %d window(s)", c.cStamp, c.onslowAfter);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW52, THE QUICK COME-BACK: A HEAVY STRETCH, THEN A LIGHT ONE (round 9, L2, 2026-10-26)
// THE OWNER'S WORD: "it should come back as soon as possible I think, waiting 2 minutes is nonsense. as soon as the frames allow it to be a
// better experience it should come back". THE SITTING: rel-flw and rel-edge stood down by FGC_SLOW behind windows 41 and 39 and rested 70
// windows (rest 10, back-off 60), longer than either leg. NOW (fg_cond.h, FgDriveExit): behind a stand-down by a rate row the rest is one
// window and there is no back-off; the law's own prediction (FGC_ONSLOW) and the dwell bring it back. THE SCENE: a busy card (the limit
// read, 6.5 ms laid on every engaged frame, LwAddInPresent), by itself at 61 (16.4 ms): engaged at 43.7; from the stint's 21st window a
// HEAVY STRETCH, 45 by itself (22.0 ms): R_on 35, FGC_SLOW stands it down behind its warm-up and cost windows (c filed, about 6.5 ms); eight
// windows of it standing by (the prediction 34.8: FGC_ONSLOW), then THE LIGHT STRETCH, 61 by itself again, to window 120. The cut wait as
// the product has it (10 s). HELD: the stand-down by FGC_SLOW; no engage in the heavy stretch; ON again within 10 windows of the light
// stretch's start (the rest, the dwell's 3 and the cut wait); ON at the end; every real frame once and in order. CONTROL, THE LAW OF BEFORE
// (quickRestWin 0: the rest 10 and the back-off 60): not back within 10 windows. FAIL FIRST ON THE BASE (af718127's fg_cond.h, which has no
// quick come-back: this case's own run is the law of before there): not back within 10 windows.
struct Lw52Leg { std::vector<LwRec> w; LwFrames f; int downAt, downRow, lightAt, backAt, heavyOn, endState, cLim; double backS; };
static Lw52Leg Lw52Run(int quick)
{
    Lw52Leg g = {}; g.downAt = g.lightAt = g.backAt = -1;
    const RtPar par = Lw9Par(16.4, 6.5);
    RtBegin(par);
    gRtSeed = 20261026u;
    FgDriveK k = FgDriveDefaults();
#if defined(FG_DRIVE_QUICK)
    if (!quick) k.quickRestWin = 0;                           // (the law of before: the control)
#else
    (void)quick;
#endif
    LwLaw L; LwLawBegin(L, k, LwSceneK()); L.card = 1;
    int onIdx = 0; double lightMs = 0.0;
    for (int win = 0; win < 120; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        double alone = 16.4;
        if (g.downAt < 0 && on && onIdx > 20) alone = 22.0;                          // (the heavy stretch, engaged)
        else if (g.downAt >= 0 && win <= g.downAt + 8) alone = 22.0;                 // (and eight windows of it standing by)
        if (g.downAt >= 0 && win > g.downAt + 8 && g.lightAt < 0) { g.lightAt = win; lightMs = (double)LwNowMs(); }
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (g.downAt < 0 && r.prev == FGDR_ON && r.state != FGDR_ON) { g.downAt = win; g.downRow = r.row; g.cLim = L.mem.cLimUs; }
        if (g.downAt >= 0 && g.lightAt < 0 && L.mem.state == FGDR_ON) g.heavyOn++;
        if (g.lightAt >= 0 && g.backAt < 0 && L.mem.state == FGDR_ON) { g.backAt = win; g.backS = ((double)LwNowMs() - lightMs) / 1000.0; }
    }
    g.endState = L.mem.state;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW52");
    return g;
}
static void CaseLW52()
{
    const char* const name = "LW52 the quick come-back: behind a heavy stretch's stand-down by FGC_SLOW, a light stretch engages again within 10 windows (2026-10-26, round 9, L2)";
    CaseBegin(name);
    const Lw52Leg a = Lw52Run(1);
    LwPrint("61 BY ITSELF ON A BUSY CARD (6.5 ms laid on), A HEAVY STRETCH AT 45 FROM THE STINT'S 21st WINDOW, THEN 61 AGAIN:", a.w);
    printf("  the stand-down behind window %d (%s, c %d us); engaged windows in the heavy stretch standing by %d; the light stretch from window %d: ON again at window %d, %.1f s behind its start; %s at the end; %d frames, real frames shown once %d, out of place %d\n",
           a.downAt, FgCondEnumName(a.downRow), a.cLim, a.heavyOn, a.lightAt, a.backAt, a.backS, FgDriveStateName(a.endState), a.f.n, a.f.real1, a.f.order);
    if (a.downAt < 0 || a.downRow != FGC_SLOW) Err("LW52: the heavy stretch's stand-down behind window %d by %s (one by FGC_SLOW wanted)", a.downAt, FgCondEnumName(a.downRow));
    if (a.heavyOn) Err("LW52: %d window(s) ON in the heavy stretch standing by (none wanted: the prediction is under the floor there)", a.heavyOn);
    if (a.backAt < 0 || a.backAt > a.lightAt + 10) Err("LW52: the light stretch from window %d engaged again at window %d (within 10 windows wanted: the rest, the dwell and the cut wait)", a.lightAt, a.backAt);
    if (a.endState != FGDR_ON) Err("LW52: %s at the end (ON wanted)", FgDriveStateName(a.endState));
    if (a.f.order || a.f.real1 != a.f.n) Err("LW52: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
#if defined(FG_DRIVE_QUICK)
    const RingStats stc = gRingSt;
    const Lw52Leg c = Lw52Run(0);
    ViolRestore(stc);
    printf("  CONTROL, THE LAW OF BEFORE (the rest 10 and the back-off 60): the stand-down behind window %d, the light stretch from window %d, ON again at window %d\n", c.downAt, c.lightAt, c.backAt);
    if (c.backAt >= 0 && c.backAt <= c.lightAt + 10) Err("LW52 control: the law of before engaged again at window %d, within 10 of the light stretch's start (%d): the control shows nothing", c.backAt, c.lightAt);
    else Ctl("the law of before: ON again at window %d, the light stretch from %d", c.backAt, c.lightAt);
#endif
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW53, NO CYCLING: A GAME HOVERING AT THE FLOOR, AND THE GUARD (round 9, L2)
// THE OWNER'S OLDER WORD: no cycling. (A) A GAME HOVERING AT THE FLOOR: 60 by itself on a busy card that adds 9.5 and 10.6 ms by turns (R_on
// 38.2 and 36.7: the second under the engaged out-rule of 37). The optimistic first engage, FGC_SLOW behind the warm-up, c filed from the
// stint (about 10 ms and its spread), and FGC_ONSLOW keeps it by on the prediction (about 37, under 40): 1 engage, 1 stand-down, then no
// transition for the rest of 5 minutes. The quick come-back does not come back where the frames do not allow it. (B) THE GUARD, A SCENE
// THAT SWINGS: 62.5 by itself (16.0 ms) for 14 windows, then 40 (25.0 ms) for 4 (about 12 s), by turns, 6.5 ms laid on engaged (R_on 44.4 and
// 31.7), the cut wait 0 (so the guard, not the cut wait, is what the gaps show): each light stretch engages and files c behind its warm-up
// from light windows alone, each heavy one stands it down by FGC_SLOW after a short stint. (A first form swung 14 and 14: its first stint's
// cost windows took a heavy one, c read 8.8 ms, and FGC_ONSLOW kept it by for good: no cycle to guard.) HELD (fg_cond.h, FgDriveExit): two
// stand-downs or more, behind the second short stint running the next engage 30 s or more behind the stand-down, behind the third and later
// 60 s or more; every real frame once and in order. CONTROL, THE GUARD OFF (flapHoldMs 0): it engages again behind every heavy stretch, more
// often than the guard lets it. FAIL FIRST ON THE BASE (af718127's law): (B) FAILS THERE TOO, which the base's back-off was believed to hold:
// the game's own band moves with the swing (40 by itself is under the floor: SLOW; 62.5 IN), each move clears the rest and the back-off
// (FGDRW_BAND), and the law of before engaged behind every heavy stretch from the second on (53.6 s, then 14.8 s each, 6 engages in 5
// minutes). The guard's hold is not cleared by a band move (FgDriveExit), so it holds where the back-off did not.
struct Lw53Leg { std::vector<LwRec> w; LwFrames f; int engages, downs, endState, turnsAfter2; double mins; std::vector<double> gapsS; };
static Lw53Leg Lw53Run(int swing, int guard)
{
    Lw53Leg g = {};
    const RtPar par = Lw9Par(16.67, swing ? 6.5 : 9.5);
    RtBegin(par);
    gRtSeed = 20261026u;
    FgDriveK k = FgDriveDefaults();
#if defined(FG_DRIVE_QUICK)
    if (!guard) k.flapHoldMs = 0;                             // (the guard off: the control)
#else
    (void)guard;
#endif
    if (swing) k.cutWaitMs = 0;
    LwLaw L; LwLawBegin(L, k, LwSceneK()); L.card = 1;
    const unsigned t0 = LwNowMs();
    double downMs = -1.0;
    for (int win = 0; win < 400 && LwNowMs() - t0 < 300000u; win++)
    {
        double alone = 16.67;
        if (swing) alone = (win % 18) >= 14 ? 25.0 : 16.0;
        else gLwAddMs = (win & 1) ? 10.6 : 9.5;
        const int was = L.mem.state;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(alone * RtSpread(0.01))); LwLawPoll(L); }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r);
        if (r.prev == FGDR_ON && r.state != FGDR_ON) downMs = (double)LwNowMs();
        if (was != FGDR_ON && L.mem.state == FGDR_ON && downMs >= 0.0) { g.gapsS.push_back(((double)LwNowMs() - downMs) / 1000.0); downMs = -1.0; }
        if (L.mem.engages + L.mem.downs > 2 && r.state != r.prev) g.turnsAfter2++;
    }
    g.mins = (LwNowMs() - t0) / 60000.0;
    g.engages = L.mem.engages; g.downs = L.mem.downs; g.endState = L.mem.state;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW53");
    return g;
}
static std::string Lw53Gaps(const Lw53Leg& g) { std::string s; char b[32]; for (double x : g.gapsS) { _snprintf_s(b, sizeof(b), _TRUNCATE, "%s%.1f", s.empty() ? "" : ", ", x); s += b; } return s.empty() ? "none" : s; }
static void CaseLW53()
{
    const char* const name = "LW53 no cycling: a game hovering at the floor engages once and stays by; a swinging scene is held 30 s behind its second short stint, 60 s from the third (2026-10-26, round 9, L2)";
    CaseBegin(name);
    const Lw53Leg a = Lw53Run(0, 1);
    LwPrint("(A) 60 BY ITSELF, A BUSY CARD ADDING 9.5 AND 10.6 ms BY TURNS (R_on 38.2 AND 36.7):", a.w);
    printf("  (A) %.1f minutes: engages %d, stand-downs %d, changes of state behind them %d, %s at the end; %d frames, real frames shown once %d, out of place %d\n",
           a.mins, a.engages, a.downs, a.turnsAfter2, FgDriveStateName(a.endState), a.f.n, a.f.real1, a.f.order);
    if (a.engages != 1 || a.downs != 1 || a.turnsAfter2 != 0 || a.endState == FGDR_ON || a.mins < 5.0) Err("LW53 (A): %d engage(s), %d stand-down(s), %d change(s) behind them, %s at the end in %.1f minutes (1, 1, 0, not ON, 5 wanted: no cycling at the floor)", a.engages, a.downs, a.turnsAfter2, FgDriveStateName(a.endState), a.mins);
    if (a.f.order || a.f.real1 != a.f.n) Err("LW53 (A): %d picture(s) out of place (%s), %d of %d real frames shown exactly once", a.f.order, a.f.first.c_str(), a.f.real1, a.f.n);
    const Lw53Leg b = Lw53Run(1, 1);
    LwPrint("(B) A SWINGING SCENE: 62.5 BY ITSELF 14 WINDOWS, 40 FOR 4, BY TURNS, 6.5 ms LAID ON ENGAGED, THE CUT WAIT 0:", b.w);
    printf("  (B) %.1f minutes: engages %d, stand-downs %d; the seconds from each stand-down to the next engage: %s; %d frames, real frames shown once %d, out of place %d\n",
           b.mins, b.engages, b.downs, Lw53Gaps(b).c_str(), b.f.n, b.f.real1, b.f.order);
    {
        bool ok = b.gapsS.size() >= 2;
        for (size_t i = 1; ok && i < b.gapsS.size(); i++) ok = b.gapsS[i] >= (i == 1 ? 30.0 : 60.0);
        if (!ok) Err("LW53 (B): the seconds from each stand-down to the next engage were %s (2 or more, the second 30 or more and the later ones 60 or more wanted)", Lw53Gaps(b).c_str());
    }
    if (b.f.order || b.f.real1 != b.f.n) Err("LW53 (B): %d picture(s) out of place (%s), %d of %d real frames shown exactly once", b.f.order, b.f.first.c_str(), b.f.real1, b.f.n);
#if defined(FG_DRIVE_QUICK)
    const RingStats stc = gRingSt;
    const Lw53Leg c = Lw53Run(1, 0);
    ViolRestore(stc);
    printf("  CONTROL, THE GUARD OFF: %.1f minutes, engages %d, stand-downs %d; the seconds from each stand-down to the next engage: %s\n", c.mins, c.engages, c.downs, Lw53Gaps(c).c_str());
    if (c.engages <= b.engages) Err("LW53 control: with the guard off %d engage(s), with it %d (more wanted off: the control shows nothing)", c.engages, b.engages);
    else Ctl("the guard off: %d engages in %.1f minutes against %d with it", c.engages, c.mins, b.engages);
#endif
    CaseEnd(name);
}
