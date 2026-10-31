// dt_fg_fixed.h: THE FIXED-REFRESH MODE IN THE HARNESS (2026-10-17, night; phase 5, package F1; docs/reports/fg-5/brief-fill/PLAN-fill.md,
// section 0 and C's F1, the design in B: "FIXED REFRESH, VSYNC ON: the vblank grid, whole ratios only").
//
// Included by dt_fg_test.cpp behind dt_fg_law.h with ONE line. IN THE SUITE: FX1 (a fixed 120 at a game of 90), FX3 (a game under half the
// refresh), FX4 (vsync off, and the table with the key at 0 exactly as before), FX5 (the freeze proof), FX6 (the mode's memory of a
// quantised display); ROUND 2 (2026-10-18): FX7 (the game at the engage floor), FX8 (the overrun stands down), FX9 (the frame queue at 2 and
// 3), FX10 (the Present's own time). BY NAME ALONE, as the replays are: FX0 (RL9 under the panel model's variable arm: 2,160 frames), FX2 (the
// panel at 119.88 against 120 read: TEN MINUTES of game, 36,000 frames, and its mirror at 120.12 for three) and FX11 (a game hovering at the
// edge, five minutes twice and the told wander).
//
// WHAT RUNS. The merged machine on the simulation of dt_fg_rates.h, with THE PANEL MODEL of that file (RtPar::panelHz and the rest, off in
// every case before these): a grid of refreshes at the panel's TRUE rate, and DXVK's FIFO hold of depth 1 (a present returns when the one
// ahead of it is shown). The machine is told the refresh as the mod reads it (RtPar::refresh) and the fixed mode's word as the conditions
// would publish it (RtPar::fixOn, fg_arm_life.h's FgpFixPublish); it reads the FIFO's hold where the product does, in its own timed call of
// DXVK's Present (dtPresTicksSum), and paces by it (fg_machine.h, PaceFixed). The card is RL9's (the free card of the owner's flw leg, the
// machine's keys as the armed build hands them today, stage 3's included).
// WHAT IS READ, BY THE PANEL: every present it showed in order, its refresh, the pair it belongs to: every present one refresh after the one
// before (a gap of two refreshes is a refresh shown twice: a REPEAT), generated and real by turns, each pair's two once (FQ4's assertions:
// each real frame once, no generated frame twice, nothing out of place, no real frame dropped).
// THE GATE OF THE PLAN (section D): the panel model must leave RL9 unchanged before a fixed case is judged: FX0, and RL9 itself read the same
// before and after this file (the report has both outputs).
#pragma once

// ------------------------------------------------------------------ what a run of the panel model reads
struct FxRead
{
    int n, real1, realTwice, never, genTwice, gen; int order; LONG dropped;   // FQ4's, over the pairs of the stretch (order: the simulation's own)
    int panelOrder;                                                            // the panel's: a present that was not the one due (a withheld generated frame among them)
    int shown, apart1, repeats, doubles, kMax, firstRep; double dispFps, fps;  // by the panel, over the stretch
    int repAll;                                                                // repeats over the whole run (from its first shown pair)
    int repLate;                                                               // repeats at the run's presents past `repAfter` (FxReadRun)
    int stallGaps;                                                             // (round 3) the planted stalls' own gaps, not counted as repeats
    LONG medUs, ppm, wins, lastUs; LONG holds, bound; double holdUs, holdMaxUs;
    double ageCardMs, agePanelMs;                                              // the real frame's age at its present on the card and on the panel, medians, from its entry
    double waitUs;                                                             // (round 2) the FIFO's wait for the one ahead alone, the Present's own time aside, the mean a present
    LONG held, stalls;                                                         // (round 2) the correction's windows held for the game, the stalls it restarted at
    int card; LONG cardWaits, iPpm; double cardWaitUs;                             // (2026-10-25, FX24) the block read as the card's at the end; the card's waits made, their mean
    std::string first;
};
// The run's pairs [from, n - 1) and the panel's presents from the first of pair `from` to the last of pair n - 2.
static FxRead FxReadRun(int from, int repAfter = 0x40000000)
{
    const RtSim& g = gRt; FxRead o = {};
    const int n = (int)g.pairs.size();
    for (int i = from; i < n - 1; i++)
    {
        const RtPair& q = g.pairs[i]; o.n++;
        if (q.real == 1) o.real1++; else if (q.real > 1) o.realTwice++; else o.never++;
        if (q.gen == 1) o.gen++; else if (q.gen > 1) o.genTwice++;
    }
    o.order = g.order; o.first = g.firstBad; o.dropped = B::S().c.nDropped;
    // by the panel
    const double T = g.vbT;
    int a = -1, b = -1, firstAll = -1;
    for (int i = 0; i < (int)g.disps.size(); i++)
    {
        const int p = g.disps[i].pair;
        if (p >= 1 && firstAll < 0) firstAll = i;
        if (p >= from && a < 0) a = i;
        if (p >= 0 && p <= n - 2) b = i;
    }
    o.firstRep = -1;
    for (int i = (firstAll < 0 ? 0 : firstAll); i + 1 < (int)g.disps.size() && T > 0.0; i++)
    {
        const long long k = llround((double)(g.disps[i + 1].t - g.disps[i].t) / T);
        if (g.disps[i + 1].stall) { o.stallGaps++; continue; }   // (round 3: the stall's own gap, told apart)
        if (k > 1) { o.repAll += (int)(k - 1); if (i > repAfter) o.repLate += (int)(k - 1); }
    }
    if (a >= 0 && b > a && T > 0.0)
    {
        int expectPair = g.disps[a].pair, expectGen = 1;
        for (int i = a; i <= b; i++)
        {
            const RtSim::Disp& d = g.disps[i];
            o.shown++;
            if (d.pair != expectPair || d.gen != expectGen) { if (o.first.empty()) o.first = Fmt("the panel showed pair %d's %s where pair %d's %s was due", d.pair, d.gen ? "generated frame" : "real frame", expectPair, expectGen ? "generated frame" : "real frame"); o.panelOrder++; expectPair = d.pair; expectGen = d.gen; }
            if (expectGen) expectGen = 0; else { expectGen = 1; expectPair++; }
            if (i == b) break;
            const long long k = llround((double)(g.disps[i + 1].t - d.t) / T);
            if (k == 1) o.apart1++; else if (k > 1) { o.repeats += (int)(k - 1); if (o.firstRep < 0) o.firstRep = i - a; } else o.doubles++;
            if (k > o.kMax) o.kMax = (int)k;
        }
        const double span = (double)(g.disps[b].t - g.disps[a].t) / (double)B::Qpf();
        o.dispFps = span > 0.0 ? (double)(b - a) / span : 0.0;
    }
    const RtStat st = RtRead(from, n - 1); o.fps = st.fps;
    {
        std::vector<double> ac, ap; const double q = (double)B::Qpf() / 1000.0;
        for (int i = from; i < n - 1; i++) { const RtPair& x = g.pairs[i]; if (x.real != 1) continue; ac.push_back((double)(x.tReal - x.tEntry) / q); if (x.dReal) ap.push_back((double)(x.dReal - x.tEntry) / q); }
        std::sort(ac.begin(), ac.end()); std::sort(ap.begin(), ap.end());
        o.ageCardMs = ac.empty() ? 0.0 : ac[ac.size() / 2]; o.agePanelMs = ap.empty() ? 0.0 : ap[ap.size() / 2];
    }
    const FgRenderLine& r = B::S().r;
    o.medUs = r.fixMedUs; o.ppm = r.fixPpm; o.wins = r.fixWins; o.lastUs = r.fixLastBlkUs;
    o.holds = g.fifoHolds; o.bound = g.fifoBound;
    o.holdUs = g.fifoHolds ? (double)g.fifoTicks * 1e6 / (double)B::Qpf() / g.fifoHolds : 0.0;
    o.holdMaxUs = (double)g.fifoMax * 1e6 / (double)B::Qpf();
    o.waitUs = g.fifoN > g.fifoN0 ? (double)(g.fifoWaitTicks - g.fifoWait0) * 1e6 / (double)B::Qpf() / (g.fifoN - g.fifoN0) : 0.0;   // (over the stretch: FxRun marks its start)
    o.held = r.fixHeld; o.stalls = r.fixStalls;
    o.card = (r.fixCard >> 2) & 1; o.cardWaits = gRt.cardWaits; o.iPpm = r.fixIPpm;
    o.cardWaitUs = gRt.cardWaits ? (double)gRt.cardWaitTicks * 1e6 / (double)B::Qpf() / gRt.cardWaits : 0.0;
    return o;
}
static const char* FxLine(const FxRead& o)
{
    static char bufs[4][1400]; static int at = 0;
    char* const b = bufs[at++ & 3];
    snprintf(b, sizeof(bufs[0]), "%d pairs: real frames once %d, twice %d, never %d, generated frames twice %d, out of place %d, real frames dropped %ld; THE PANEL: not the one due %d, %d presents shown, %d one refresh after the one before, %d refresh(es) shown twice (the first at the stretch's present %d; %d over the whole run), %d presents on one refresh, the widest gap %d refreshes;"
        " %.3f presents a second on the panel, %.3f real frames a second; THE FIXED PACER: the block a present median %ld us (the last %ld), the correction %+ld ppm after %ld windows; the FIFO held %ld presents, %.0f us on the mean, %.0f at the most, %ld at its bound, its wait alone %.0f us on the mean over the stretch; held for the game %ld window(s), stalls %ld; THE REAL FRAME'S AGE from its entry, medians: on the card %.2f ms, on the panel %.2f ms",
        o.n, o.real1, o.realTwice, o.never, o.genTwice, o.order, o.dropped, o.panelOrder, o.shown, o.apart1, o.repeats, o.firstRep, o.repAll, o.doubles, o.kMax, o.dispFps, o.fps, o.medUs, o.lastUs, o.ppm, o.wins, o.holds, o.holdUs, o.holdMaxUs, o.bound, o.waitUs, o.held, o.stalls, o.ageCardMs, o.agePanelMs);
    return b;
}
// FQ4's assertions on a fixed shape, and the panel's: every present one refresh after the one before, none twice, by turns
static std::string FxJudge(const FxRead& o, bool panel = true)
{
    std::string bad;
    if (o.realTwice || o.never || o.genTwice || o.order || o.dropped) bad += Fmt("real frames twice %d, never %d, generated twice %d, out of place %d (%s), dropped %ld; ", o.realTwice, o.never, o.genTwice, o.order, o.first.c_str(), o.dropped);
    if (o.gen != o.n) bad += Fmt("%d of %d generated frames shown; ", o.gen, o.n);
    if (panel && o.panelOrder) bad += Fmt("the panel: %d present(s) not the one due (%s); ", o.panelOrder, o.first.c_str());
    if (panel && (o.repeats || o.doubles || o.shown < 2 * o.n - 1 || o.apart1 != o.shown - 1)) bad += Fmt("the panel: %d presents, %d one refresh apart, %d repeated refreshes, %d presents on one refresh (every present one refresh after the one before wanted); ", o.shown, o.apart1, o.repeats, o.doubles);
    if (o.bound) bad += Fmt("the FIFO's hold met its 250 ms bound %ld time(s); ", o.bound);
    return bad;
}
// THE FIXED SHAPE: RL9's card and keys with stage 3's (the armed build's today), the panel a grid at `trueHz` behind a FIFO of depth 1, the
// machine told `readHz` and the fixed mode `fixOn`
static RtPar FxPar(double trueHz, ULONGLONG readPacked, int fixOn)
{
    gRlNow = RL_NOW_ALL; RtPar p = RlPar(kRl[8], 0); gRlNow = 0;
    p.refresh = readPacked;
    p.panelHz = trueHz; p.panelVrr = 0; p.fifo = 1; p.panelPhase = 0.37; p.fixOn = fixOn;
    return p;
}
static const ULONGLONG kFx120 = (120ull << 32) | 1ull;
// A run of the fixed shape: `frames` of a game of `costMs` (5 percent of spread), read from pair `from`
// (round 2) A WINDOW OF 120 FRAMES as the run went: the block's median, the correction, the law's L, the conditions' overrun (round 7: FgCondFixShareStep, FxOverStep; before, FgCondFixOverStep
// as fg_arm_life.h steps it, two refreshes as read)
struct FxWin { LONG med, ppm, lUs; int over; };
// (round 7, F1) THE OVERRUN AS fg_arm_life.h's FgpFixPublish STEPS IT: the share of the law's last 12 s over the line (FgCondRingOverPm) and
// its E, FgCondFixShareStep; gFxR6 1: ROUND 6's RULE (c76d58c1: L over the line two windows running), the fail-first control. gFxSharePm: the
// last share read (-1 none)
static int gFxR6 = 0, gFxSharePm = -1, gFxDipRun = 0;   // (gFxDipRun: the held rate's dip, FgCondFixDipStep)
static int FxOverStep(int& run, const FgState& s, int lineUs, bool engaged)
{
    if (gFxR6) { const int l = engaged ? (int)s.r.lawLUs : 0; if (!engaged || l <= 0) run = 0; else run = l > lineUs ? run + 1 : 0; gFxSharePm = -1; return run >= FG_COND_FIX_OVER_WIN ? 1 : 0; }
    gFxSharePm = (engaged && s.law) ? FgCondRingOverPm(s.law->slow, lineUs, FG_COND_FIX_SHARE_MIN_N) : -1;
    return FgCondFixShareStep(run, gFxSharePm, engaged ? (int)s.r.lawEUs : 0, lineUs, engaged ? 1 : 0);
}
static int gFxPlantAt = -1; static LONG gFxPlantIPpm = 0;   // (2026-10-25, the review's FX26: at this frame the integral and its trusted copy are planted)
static FxRead FxRun(const RtPar& p, double costMs, int frames, int from, unsigned seed, std::vector<LONG>* medTrace = nullptr, int repAfter = 0x40000000, std::vector<FxWin>* wins = nullptr)
{
    RtBegin(p);
    gRtSeed = seed;
    int overRun = 0;
    const ULONG num = (ULONG)(p.refresh >> 32), den = (ULONG)p.refresh;
    const int twoUs = num ? (int)(2000000LL * den / num) : 0;
    for (int i = 0; i < frames; i++)
    {
        if (i == from) { gRt.fifoN0 = gRt.fifoN; gRt.fifoWait0 = gRt.fifoWaitTicks; }   // (the stretch's FIFO wait from here: frames and pairs go one for one)
        RtFrame(RtMs(costMs * RtSpread(0.05)));
        if (i == gFxPlantAt) { FgRenderLine& r = B::S().r; r.fixIPpm = gFxPlantIPpm; r.fixITrust = (SHORT)gFxPlantIPpm; }
        if (medTrace && (i % 600) == 599) { medTrace->push_back(B::S().r.fixMedUs); medTrace->push_back(B::S().r.fixPpm); }
        if (wins && (i % 120) == 119) { const FgRenderLine& r = B::S().r; const int over = FxOverStep(overRun, B::S(), twoUs + FG_COND_FIX_OVER_US, true); wins->push_back({ r.fixMedUs, r.fixPpm, r.lawLUs, over }); }
    }
    const FxRead o = FxReadRun(from, repAfter);
    RtEnd("FX");
    return o;
}

// ---------------------------------------------------------------- FX0, the panel model's variable arm leaves RL9 as it was
// RL9 (the owner's flw, the free card) run twice from the same seed: with the panel model off, as RL9 runs, and with its VARIABLE arm on at the
// panel's own 120 Hz. EVERYTHING THE MACHINE READS AND EVERY NUMBER RL9 JUDGES ARE THE SAME, TO THE LAST DIGIT (the arm adds the instants the
// panel showed and touches nothing the machine sees); and RL9's own judge passes on the run with it off. CONTROL (a run): the same arm at a
// panel of 100 Hz, whose fastest refresh is longer than the pair's halves: it holds most presents for its refresh (it can see a thing), and
// the machine's numbers are still the same.
static void CaseFX0()
{
    const char* const name = "FX0 the panel model's variable arm leaves RL9 as it was: every number the machine reads and RL9 judges the same (2026-10-17, night, F1)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const RlLeg& l = kRl[8];
    const RlOut off = RlRun(l, 0);
    gRtPanelHz = 120.0; gRtPanelVrr = 1; gRtPanelFifo = 0;
    const RlOut on = RlRun(l, 0); const LONG heldOn = gRt.vrrHeld; const size_t dispOn = gRt.disps.size();
    const double heldOnUs = heldOn ? (double)gRt.vrrHeldTicks * 1e6 / (double)B::Qpf() / heldOn : 0.0, heldOnMaxUs = (double)gRt.vrrHeldMax * 1e6 / (double)B::Qpf();
    gRtPanelHz = 100.0;
    const RlOut c = RlRun(l, 0); const LONG heldC = gRt.vrrHeld;
    gRtPanelHz = 0.0; gRtPanelVrr = 0;
    ViolRestore(stc);
    RlJudge(l, off, "FX0 (RL9, the model off)");
    auto same = [](const RlOut& a, const RlOut& b) -> std::string
    {
        std::string d;
        if (a.g.gen != b.g.gen || a.g.n != b.g.n || a.g.real1 != b.g.real1) d += Fmt("shown %d of %d and %d once against %d of %d and %d; ", b.g.gen, b.g.n, b.g.real1, a.g.gen, a.g.n, a.g.real1);
        if (a.g.set.fps != b.g.set.fps || a.g.set.perMs != b.g.set.perMs || a.g.set.rmsE != b.g.set.rmsE || a.g.set.maxE != b.g.set.maxE) d += Fmt("rate %.6f, period %.6f, e rms %.6f at worst %.6f against %.6f, %.6f, %.6f, %.6f; ", b.g.set.fps, b.g.set.perMs, b.g.set.rmsE, b.g.set.maxE, a.g.set.fps, a.g.set.perMs, a.g.set.rmsE, a.g.set.maxE);
        if (a.plMedAll != b.plMedAll || a.g.ageMed != b.g.ageMed || a.room != b.room || a.posFires != b.posFires || a.posDue != b.posDue || a.posInj != b.posInj) d += "the card's place, the age, the room floor or the placement; ";
        for (int w = 0; w < 9; w++) if (a.winGen[w] != b.winGen[w] || a.winN[w] != b.winN[w] || a.plMed[w] != b.plMed[w]) { d += Fmt("window %d; ", w + 1); break; }
        for (int k = 0; k < FG_AW_N; k++) if (a.why[k] != b.why[k]) { d += "the reasons a generated frame was not shown; "; break; }
        if (a.g.order != b.g.order || a.g.waits != b.g.waits) d += "the order or the order waits; ";
        return d;
    };
    const std::string dOn = same(off, on), dC = same(off, c);
    printf("  RL9 with the panel model off: %.1f of 100 shown (%d of %d), %.3f real, on the card %ld; WITH ITS VARIABLE ARM AT 120 Hz: %.1f (%d of %d), %.3f real, on the card %ld; the panel showed %zu presents, %ld of them held for its refresh (by %.1f us on the mean, %.1f at the most: TOLD, the panel's fastest refresh against the card's instants, which nothing of the machine reads); %s\n",
        off.shownAll, off.g.gen, off.g.n, off.g.set.fps, off.plMedAll, on.shownAll, on.g.gen, on.g.n, on.g.set.fps, on.plMedAll, dispOn, heldOn, heldOnUs, heldOnMaxUs, dOn.empty() ? "every number the same" : dOn.c_str());
    if (!dOn.empty()) Err("FX0: RL9 under the panel model's variable arm differs: %s", dOn.c_str());
    if (dispOn < 2000) Err("FX0: the panel model showed %zu presents (about 2,160 wanted: the arm did not run)", dispOn);
    printf("  CONTROL, the variable arm at a panel of 100 Hz: %ld presents held for the panel's refresh; the machine's numbers %s\n", heldC, dC.empty() ? "the same" : dC.c_str());
    if (heldC < 500 || !dC.empty()) Err("FX0 control: at 100 Hz the panel held %ld present(s) (500 or more wanted: the model sees nothing) and the machine read %s (the same wanted)", heldC, dC.empty() ? "the same" : dC.c_str());
    Ctl("a panel of 100 Hz under the same run: %ld presents held for its refresh", heldC);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX1, a fixed 120 Hz panel, a game of 90
// THE PLAN'S FX1: the panel a grid at 120.000 Hz behind a FIFO of depth 1, read as 120, the fixed mode on (fg_fixed 1), a game of 90 by itself
// (11.1 ms, 5 percent of spread): PACED TO 60.00, every present one refresh after the one before over 600 pairs behind the correction's run-in
// (pairs 1800 to 2400: the engage leaves the queue about 3 ms deep, and the correction brings it to its target in about ten windows), no
// refresh shown twice from the first pair on, generated and real by turns, FQ4's assertions; the block a present's
// median within 250 us of its target of 500 at the end, the correction within 200 ppm of 0.
// CONTROLS (runs): (1) the fixed mode off on the same panel, the variable pacing at LOCK 1.015 (16.92 ms a pair, a hair over two refreshes):
// refreshes shown twice, every half second or so (the judder the fixed mode exists for); (2) the fixed mode with its correction off (fg_fixed
// 2): the period exactly two refreshes as read: no refresh twice on this panel, but the block a present's median stays where the engage left
// the queue (about 3.8 ms here: a refresh of picture age more), since nothing steers it.
static void CaseFX1()
{
    const char* const name = "FX1 a fixed 120 Hz panel, a game of 90: paced to 60.00, every present one refresh after the one before, none twice, by turns (2026-10-17, night, F1)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const FxRead o = FxRun(FxPar(120.0, kFx120, 1), 11.1, 2401, 1800, 20261017u);
    const FxRead c1 = FxRun(FxPar(120.0, kFx120, 0), 11.1, 1001, 400, 20261017u);
    const FxRead c2 = FxRun(FxPar(120.0, kFx120, 2), 11.1, 1001, 400, 20261017u);
    ViolRestore(stc);
    printf("  THE FIXED MODE: %s\n", FxLine(o));
    const std::string bad = FxJudge(o);
    if (!bad.empty()) Err("FX1: %s", bad.c_str());
    if (o.repAll) Err("FX1: %d refresh(es) shown twice over the whole run (none wanted from the first pair on)", o.repAll);
    if (fabs(o.fps - 60.0) > 0.02 || fabs(o.dispFps - 120.0) > 0.04) Err("FX1: %.3f real frames a second and %.3f presents a second on the panel (60.00 and 120.00 within 0.02 and 0.04 wanted)", o.fps, o.dispFps);
    if (o.medUs < 250 || o.medUs > 750 || labs(o.ppm) > 200 || o.wins < 10) Err("FX1: the block a present's median %ld us, the correction %+ld ppm after %ld windows (250 to 750, within 200 of 0, 10 or more wanted)", o.medUs, o.ppm, o.wins);
    printf("  CONTROL 1, the fixed mode off (the variable pacing at LOCK 1.015): %s\n", FxLine(c1));
    if (c1.repeats < 10) Err("FX1 control 1: with the fixed mode off %d refresh(es) were shown twice over 600 pairs (10 or more wanted: the panel model shows no judder)", c1.repeats);
    Ctl("the fixed mode off: %d refreshes shown twice over 600 pairs", c1.repeats);
    printf("  CONTROL 2, the correction off (fg_fixed 2): %s\n", FxLine(c2));
    if (c2.ppm != 0 || labs(c2.medUs - FG_FIX_TARGET_US) < 1000) Err("FX1 control 2: the correction off read %+ld ppm, a median block of %ld us (0, and a block left where the engage put it, 1,000 us or more from the target, wanted: the check cannot see the correction)", c2.ppm, c2.medUs);
    Ctl("the correction off: %+ld ppm, the block's median %ld us where the engage left it", c2.ppm, c2.medUs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX2, the panel at 119.88 against 120 read: ten minutes
// THE PLAN'S FX2: the panel's true rate 119.88 Hz, the mod's read 120 (the correction must take 1,000 ppm): TEN MINUTES OF GAME (36,000 pairs
// of a game of 90), NO REFRESH SHOWN TWICE over all of it, every present one refresh after the one before behind the first minute, the
// block a present's median within 250 us of its target at the end, the correction within 200 ppm of 1,001. THE MIRROR, three minutes: the
// panel at 120.12 (the correction must take -999 ppm): the pacer starts slow by 700 ppm against the hair it starts fast by, so a refresh
// may be shown twice before the correction has it: TOLD over the first 30 s, and none behind them.
// CONTROL (a run, a minute): the correction off on the 119.88 panel: the pacer 0.1 percent fast, the FIFO a present deeper and the game held
// by it: the block's median climbs to most of a refresh (the "extra slot" the correction exists to keep out).
static void CaseFX2()
{
    const char* const name = "FX2 the panel at 119.88 against 120 read, ten minutes: no refresh shown twice, the block at its target, the correction at 1,000 ppm (2026-10-17, night, F1)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    std::vector<LONG> tr;
    const FxRead o = FxRun(FxPar(119.88, kFx120, 1), 11.1, 36001, 3600, 20261018u, &tr);
    const FxRead m = FxRun(FxPar(120.12, kFx120, 1), 11.1, 10801, 1800, 20261019u);
    const FxRead c = FxRun(FxPar(119.88, kFx120, 2), 11.1, 3601, 1800, 20261018u);
    // (round 3, the reviewer's third: THE MIRROR WITH A STALL OF 100 ms EVERY 20 s, three minutes: no refresh shown twice but the stalls' own
    // gaps, the integral kept whole at each restart; TOLD beside it, round 2's halving at a restart (the guard's bit 15 off), and the reviewer's
    // form of the blind window, no teaching at all (bit 14 off), on the plain mirror)
    RtPar ps = FxPar(120.12, kFx120, 1); ps.panelStallAt = 2400; ps.panelStallMs = 100; ps.panelStallEvery = 2400;
    const FxRead st = FxRun(ps, 11.1, 10801, 600, 20261019u);
    RtPar ph = ps; ph.guardOff = 1 << 15;
    const FxRead sh = FxRun(ph, 11.1, 10801, 600, 20261019u);
    RtPar pb = FxPar(120.12, kFx120, 1); pb.guardOff = 1 << 14;
    const FxRead nb = FxRun(pb, 11.1, 10801, 1800, 20261019u);
    ViolRestore(stc);
    printf("  119.88 Hz against 120, ten minutes, read behind the first minute: %s\n", FxLine(o));
    printf("    the block's median and the correction every ten seconds:"); for (size_t i = 0; i + 1 < tr.size(); i += 2) printf(" %ld/%+ld", tr[i], tr[i + 1]); printf("\n");
    const std::string bad = FxJudge(o);
    if (!bad.empty()) Err("FX2: %s", bad.c_str());
    if (o.repAll) Err("FX2: %d refresh(es) shown twice over the ten minutes (none wanted)", o.repAll);
    if (o.medUs < 250 || o.medUs > 750 || labs(o.ppm - 1001) > 200) Err("FX2: the block a present's median %ld us, the correction %+ld ppm at the end (250 to 750, and 1,001 within 200 wanted)", o.medUs, o.ppm);
    printf("  THE MIRROR, 120.12 Hz against 120, three minutes, read behind the first 30 s: %s\n", FxLine(m));
    printf("  TOLD: the mirror's refreshes shown twice over its whole run (the first 30 s with them) %d\n", m.repAll);
    const std::string badM = FxJudge(m);
    if (!badM.empty()) Err("FX2, the mirror: %s", badM.c_str());
    if (m.medUs < 250 || m.medUs > 750 || labs(m.ppm + 999) > 200) Err("FX2, the mirror: the block a present's median %ld us, the correction %+ld ppm (250 to 750, and -999 within 200 wanted)", m.medUs, m.ppm);
    printf("  THE MIRROR WITH A STALL OF 100 ms EVERY 20 s, three minutes: %d refresh(es) shown twice besides the stalls' own %d gap(s); the correction at the end %+ld ppm, stalls restarted at %ld\n", st.repAll, st.stallGaps, st.ppm, st.stalls);
    printf("  TOLD: the same with round 2's halving at a restart: %d refresh(es) shown twice besides the stalls'; the reviewer's form of a blind window (no teaching) on the plain mirror: %d refresh(es) shown twice, the correction at the end %+ld ppm\n", sh.repAll, nb.repAll, nb.ppm);
    if (st.repAll || st.realTwice || st.never || st.genTwice || st.order || st.dropped) Err("FX2, the mirror with stalls: %d refresh(es) twice besides the stalls', real frames twice %d, never %d, generated twice %d, out of place %d, dropped %ld (all 0 wanted)", st.repAll, st.realTwice, st.never, st.genTwice, st.order, st.dropped);
    printf("  CONTROL, the correction off on the 119.88 panel, a minute: %s\n", FxLine(c));
    if (c.medUs < 3000) Err("FX2 control: the correction off left the block's median at %ld us (3,000 or more wanted: the FIFO a present deeper)", c.medUs);
    Ctl("the correction off at 119.88: the block's median %ld us", c.medUs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- the table's facts for a fixed display
static FgCondIn FxSession(int vrr, int quant, int vsync, int key, int hz)
{
    FgCondIn in = {};
    in.presentOurs = 1; in.markerOk = 1; in.dxvk31 = -1;
    in.refreshNum = hz; in.refreshDen = 1; in.lockPm = 1015;
    in.swapEffect = 1; in.backBuffers = 2; in.windowed = 1; in.bbFp16 = 1; in.hdrOn = 1;
    in.vrr = vrr; in.flipQuant = quant; in.vsync = vsync; in.fixedKey = key;
    return in;
}
static std::string FxCondLine(int c, const FgCondIn& in, const FgCondK& k) { char b[400]; return FgCondLine(c, in, k, b, sizeof(b)) > 0 ? std::string(b) : std::string("(none)"); }

// ---------------------------------------------------------------- FX3, a game under half the refresh stands by
// THE PLAN'S FX3 (rule 5: "2x with the game paced to EXACTLY Hz/2 for R >= Hz/2 x 1.03"; the law: "the floor is Hz/2 exactly; SLOW reads R_on
// against it"). On a fixed 120 Hz display with vsync and the mode asked for: STANDING BY a game of 61.7 by itself stands by for SLOW (its line
// names 61.8), one of 61.8 runs; ENGAGED one at 59.5 with it runs (99 of 100 of 60 is 59.4), one at 59.3 stands down; a game of 116.5 by
// itself stands by for FAST (it shows every refresh itself: the top is 97 of 100 of the refresh), 116.3 runs; C is 60.0 and the floor's
// period 16,666 us. A FIXED 60 Hz DISPLAY: a game of 50 runs (round 5, the owner's floor by display, fg_cond.h FgCondMinX10: 30 real on a
// 60 Hz screen; it stood by for SLOW at the table's floor of 40 before), a game of 30.8 by itself stands by (30.9 the floor), and the same
// with fg_min_fps 30; THE PANEL ROW never stands in the mode (C at 30 is the mode's, not a
// panel under 97 Hz). CONTROL: the same 60 Hz display with the mode not asked for (fg_fixed 0): FGC_PANEL, the row of before.
static void CaseFX3()
{
    const char* const name = "FX3 a game under half the refresh stands by, the floor 61.8 at 120 Hz, a fixed 60 Hz display behind fg_min_fps 30 (2026-10-17, night, F1)";
    CaseBegin(name);
    FgCondK k = FgCondDefaults();
    struct Row { const char* what; int hz, key, engaged, real, free_, minFps, want; };
    static const Row rows[] =
    {
        { "120 Hz, 61.7 by itself",      120, 1, 0,  617,    0, 0, FGC_SLOW },
        { "120 Hz, 61.8 by itself",      120, 1, 0,  618,    0, 0, FGC_OK },
        { "120 Hz, engaged at 59.5",     120, 1, 1,  595,  700, 0, FGC_OK },
        { "120 Hz, engaged at 59.3",     120, 1, 1,  593,  700, 0, FGC_OK },     // (round 7, F1: the game's own 70.0 clears the way in: a dip, not the bulk; FGC_SLOW before)
        { "120 Hz, engaged at 59.3, R_free 61.0", 120, 1, 1, 593, 610, 0, FGC_SLOW },   // (round 7, F1: the game's own under 61.8: the bulk cannot hold it)
        { "120 Hz, engaged at 59.3, R_free none", 120, 1, 1, 593, 0, 0, FGC_SLOW },     // (round 7, F1: unread: as before)
        { "120 Hz, 116.5 by itself",     120, 1, 0, 1165,    0, 0, FGC_FAST },
        { "120 Hz, 116.3 by itself",     120, 1, 0, 1163,    0, 0, FGC_OK },
        { "120 Hz, engaged, R_free 119.5", 120, 1, 1, 600, 1195, 0, FGC_FAST },
        { "60 Hz, 50 by itself",          60, 1, 0,  500,    0, 0, FGC_OK },     // (round 5: the floor 30 on a 60 Hz screen; FGC_SLOW before)
        { "60 Hz, 30.8 by itself",        60, 1, 0,  308,    0, 0, FGC_SLOW },   // (round 5: the floor by display, 30.9)
        { "60 Hz, 50, fg_min_fps 30",     60, 1, 0,  500,    0, 30, FGC_OK },
        { "60 Hz, 30.8, fg_min_fps 30",   60, 1, 0,  308,    0, 30, FGC_SLOW },
        { "60 Hz, 50, the mode not asked", 60, 0, 0, 500,    0, 30, FGC_PANEL },
    };
    int bad = 0;
    for (const Row& r : rows)
    {
        FgCondK kk = k; if (r.minFps) FgCondFloorKey(kk, r.minFps);
        FgCondIn in = FxSession(0, 0, 1, r.key, r.hz);
        in.engaged = r.engaged; in.realFpsX10 = r.real; in.freeFpsX10 = r.free_;
        const int c = FgCondFirst(in, kk);
        printf("  %-32s -> %-12s %s\n", r.what, FgCondEnumName(c), c ? FxCondLine(c, in, kk).c_str() : "");
        if (c != r.want) { bad++; Err("FX3: %s: %s where %s was wanted", r.what, FgCondEnumName(c), FgCondEnumName(r.want)); }
        if (r.key && r.hz == 60 && FgCondHolds(FGC_PANEL, in, kk)) Err("FX3: %s: the panel row stands in the fixed mode", r.what);
    }
    {
        const FgCondIn in = FxSession(0, 0, 1, 1, 120);
        printf("  120 Hz in the mode: C %d tenths, the floor %d tenths, the top %d tenths, the floor's period %d us\n", FgCondCFpsX10(in), FgCondFloorX10(in, k), FgCondTopX10(in, k), FgCondFloorUs(in));
        if (FgCondCFpsX10(in) != 600 || FgCondFloorX10(in, k) != 618 || FgCondTopX10(in, k) != 1164 || FgCondFloorUs(in) != 16666) Err("FX3: C %d, the floor %d, the top %d, the floor's period %d (600, 618, 1164 and 16666 wanted)", FgCondCFpsX10(in), FgCondFloorX10(in, k), FgCondTopX10(in, k), FgCondFloorUs(in));
    }
    Ctl("a fixed 60 Hz display with the mode not asked for: FGC_PANEL, the row of before (C under 48)");
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX4, vsync off stands by with its line; the key at 0 reads as before
// THE PLAN'S FX4 (rule 6: "FIXED REFRESH, VSYNC OFF: stand by with one line saying to turn vsync on"). A display NVIDIA calls fixed, the mode
// asked for, vsync off: FGC_VSYNCOFF, a STANDBY row (so the conditions' step asks it every window and the law engages by itself once vsync is
// on), whose line goes to the warnings file too; the rate rows do not stand in front of it (a game of 90 by itself would read FAST at the
// variable table's top). Vsync on: the mode, no row. A display read QUANTISED where NVIDIA has no answer: the mode the same. A display
// NVIDIA CALLS VARIABLE whose presents read QUANTISED: never the mode, FGC_FIXED with its own line.
// AND THE TABLE WITH THE KEY AT 0 IS THE TABLE OF BEFORE: over NVIDIA's three answers, the memory's two, vsync's two, 120 and 60 Hz and a
// spread of rates, every row and every derived number reads as the table of before did, whose FgCondIn::vrr folded the memory in
// (FgpVrrRead's "if (latFlipDisplay == 2) v = 0"): the old FIXED and VRR_UNKNOWN rows are written out here as they were.
static void CaseFX4()
{
    const char* const name = "FX4 a fixed display with vsync off stands by and says to turn vsync on; with fg_fixed 0 the table reads exactly as before (2026-10-17, night, F1)";
    CaseBegin(name);
    const FgCondK k = FgCondDefaults();
    {
        FgCondIn in = FxSession(0, 0, 0, 1, 120); in.realFpsX10 = 900;
        const int c = FgCondFirst(in, k);
        const std::string line = FxCondLine(c, in, k);
        printf("  NVIDIA says fixed, the mode asked for, vsync off, a game of 90: %s, class %d, to the warnings file %s: %s\n", FgCondEnumName(c), FgCondClassOf(c), FgCondWarns(c) ? "yes" : "NO", line.c_str());
        if (c != FGC_VSYNCOFF || FgCondClassOf(c) != FGCC_STANDBY || !FgCondWarns(c)) Err("FX4: vsync off read %s, class %d, warns %d (FGC_VSYNCOFF, standing by, to the warnings file wanted)", FgCondEnumName(c), FgCondClassOf(c), (int)FgCondWarns(c));
        if (line.find("turn vsync on") == std::string::npos || line.find("120.00 Hz") == std::string::npos) Err("FX4: the vsync line does not say the refresh and to turn vsync on: %s", line.c_str());
        in.vsync = 1;
        const int c2 = FgCondFirst(in, k);
        printf("  the same with vsync on: %s, the mode %s\n", FgCondEnumName(c2), FgCondFixedMode(in) ? "on" : "OFF");
        if (c2 != FGC_OK || !FgCondFixedMode(in)) Err("FX4: vsync on read %s with the mode %d (FGC_OK and the mode wanted)", FgCondEnumName(c2), (int)FgCondFixedMode(in));
    }
    {
        FgCondIn in = FxSession(-1, 1, 1, 1, 120); in.realFpsX10 = 900;
        const int c = FgCondFirst(in, k);
        printf("  no answer from NVIDIA, the memory quantised, vsync on: %s, the mode %s\n", FgCondEnumName(c), FgCondFixedMode(in) ? "on" : "OFF");
        if (c != FGC_OK || !FgCondFixedMode(in)) Err("FX4: no answer and the memory quantised read %s, the mode %d (FGC_OK and the mode wanted)", FgCondEnumName(c), (int)FgCondFixedMode(in));
        FgCondIn y = FxSession(1, 1, 1, 1, 120); y.realFpsX10 = 900;
        const int cy = FgCondFirst(y, k);
        printf("  NVIDIA says variable, the memory quantised, the mode asked for: %s, the mode %s: %s\n", FgCondEnumName(cy), FgCondFixedMode(y) ? "ON" : "off", FxCondLine(cy, y, k).c_str());
        if (cy != FGC_FIXED || FgCondFixedMode(y) || FxCondLine(cy, y, k).find("was read as running variable refresh") == std::string::npos) Err("FX4: NVIDIA's yes with the memory quantised read %s, the mode %d (FGC_FIXED with its own line, never the mode, wanted)", FgCondEnumName(cy), (int)FgCondFixedMode(y));
    }
    // THE KEY AT 0: the table of before, written out
    long checked = 0, differ = 0;
    for (int vrr = -1; vrr <= 1; vrr++) for (int quant = 0; quant <= 1; quant++) for (int vs = 0; vs <= 1; vs++) for (int hz : { 60, 120, 144, 240 })
    for (int real : { 0, 290, 370, 395, 400, 590, 600, 618, 660, 700, 900, 1170 }) for (int eng = 0; eng <= 1; eng++) for (int unk = 0; unk <= 1; unk++)
    {
        FgCondK kk = k; kk.unknownVrrRefused = unk;
        FgCondIn in = FxSession(vrr, quant, vs, 0, hz); in.realFpsX10 = real; in.engaged = eng; in.freeFpsX10 = eng ? real + 50 : 0; in.predFpsX10 = eng ? 0 : real - 20;
        const int vOld = quant ? 0 : vrr;                      // FgpVrrRead's fold, as it was
        for (int c = 1; c < FGC_N; c++)
        {
            bool old;
            if (c == FGC_FIXED) old = vOld == 0;
            else if (c == FGC_VRR_UNKNOWN) old = vOld < 0 && unk != 0;
            else if (c == FGC_VSYNCOFF) old = false;
            else { FgCondIn z = in; z.vrr = vOld; z.flipQuant = 0; z.vsync = 0; old = FgCondHolds(c, z, kk); }
            checked++;
            if (FgCondHolds(c, in, kk) != old) { differ++; if (differ <= 3) Err("FX4: with fg_fixed 0, row %s reads %d where the table of before read %d (vrr %d, the memory %d, vsync %d, %d Hz, rate %d, engaged %d)", FgCondEnumName(c), (int)FgCondHolds(c, in, kk), (int)old, vrr, quant, vs, hz, real, eng); }
        }
        {   // C, the floor's period, the top and the floor, by the formulas of before (round 5: the floor 30 real on a 60 Hz screen where fg_min_fps
            // is not set, the owner's floor by display, on either kind of display: written out here, not read from fg_cond.h)
            const int cf = (int)((long long)FgCondHzX1000(in) * 10 / (2LL * in.lockPm));
            const int top = cf > 0 ? (int)((long long)kk.maxRealFpsX10 * cf / 591) : kk.maxRealFpsX10;
            const int mn = (hz == 60 && !kk.minKeyed) ? 300 : kk.minRealFpsX10;
            const int flo = mn > top - kk.rateHystX10 ? top - kk.rateHystX10 : mn;
            if (FgCondCFpsX10(in) != cf || FgCondFloorUs(in) != (int)(2LL * in.lockPm * 1000000LL / FgCondHzX1000(in)) || FgCondTopX10(in, kk) != top || FgCondFloorX10(in, kk) != flo)
                { differ++; Err("FX4: with fg_fixed 0, C, the floor's period, the top or the floor moved (%d Hz)", hz); }
        }
    }
    printf("  with fg_fixed 0: %ld row readings against the table of before, %ld differ\n", checked, differ);
    // (the control: a check that can fail: the old FIXED row against the new with the key at 1, where they must differ on NVIDIA's "no" and vsync on)
    {
        const FgCondIn in = FxSession(0, 0, 1, 1, 120);
        const bool oldFixed = true, nowFixed = FgCondHolds(FGC_FIXED, in, k);
        printf("  CONTROL: the same check with fg_fixed 1 on NVIDIA's no and vsync on: the row of before %d, now %d\n", (int)oldFixed, (int)nowFixed);
        if (nowFixed == oldFixed) Err("FX4 control: with fg_fixed 1 the FIXED row reads as before (it must not: the check cannot see the mode)");
        Ctl("fg_fixed 1 on NVIDIA's no with vsync: the FIXED row of before 1, now 0");
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX5, the freeze proof: a present held in the FIFO, the worker held
// THE PLAN'S FX5 (and RULES: "with the worker held and a command queued behind it"). TWO SHAPES:
//   (a) THE WORKER HELD INSIDE DXVK'S PRESENT, A FRAME QUEUED BEHIND IT (no simulation: the frames are made by hand, FQ5's pattern). The fixed
//       mode on and the refresh read, pair A armed; the next fake Present (the fire that shows A's generated frame) HOLDS INSIDE DXVK with
//       the device lock held (the fake's SyncFrameLatency stand-in: a FIFO wait that does not end until released); frame B's calls, its hand
//       and its Present are made behind it on the render thread, and the worker is released 300 ms later from another thread. WHAT THE
//       RENDER THREAD MEETS THERE IS DXVK'S: a call that takes DXVK's device lock waits while the worker holds it inside the Present (DXVK
//       does the same: present_gate.h's freeze note); the fixed mode adds no lock and no wait (PaceFixed reads one counter with an interlocked
//       read). So: NO DEADLOCK (frame B comes back: behind the release at threaded=2, before it at threaded=1), PaceFixed ran at B's Present, the time B came back against the
//       release is THE SAME WITH THE MODE OFF (within 50 ms: the comparison run), and A's generated frame once, A's real frame once, ahead
//       of B's, B's once: FQ4's assertions.
//   (b) THE PANEL STALLS 100 ms in the simulation (RtPar::panelStallAt): the FIFO holds the present behind the stalled one that long, inside
//       the machine's own timed call: the run goes on to its end (no freeze), every real frame once and in order, nothing dropped, a
//       generated frame the stall made late withheld (4 at the most), and no refresh shown twice more than 2 s behind the stall.
static volatile LONG gFxReleased = 0;
static void FxRelease() { _InterlockedExchange(&gFxReleased, 1); RingGateRelease(); }
struct Fx5Leg { bool armed, held, back; int aGen, aReal, bReal, aBeforeB; double frameMs, tailMs; LONG since0, since1; };
static Fx5Leg Fx5Run(int fixOn)
{
    Fx5Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S();
    B::refreshPacked = kFx120;
    WriteRelease(&s.r.fixOn, fixOn);
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>(true, false);
    if (!ShSettle()) Err("FX5: the fake helper did not read pair A's messages");
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    g.since0 = s.r.fixSince;
    _InterlockedExchange(&gFxReleased, 0);
    RingGateHoldNext();
    DueI(MgP(), FG_DUE_SHOW); FireNote(); DtPublishFenced();
    g.held = WaitFor([] { return RingGateHeld(); }, 3000);
    Later l = { [] { return false; }, [] { FxRelease(); }, 300, 0 };
    HANDLE h = Spawn(LaterMain, &l);
    LARGE_INTEGER q0, q1, qf; QueryPerformanceFrequency(&qf); QueryPerformanceCounter(&q0);
    ClockAdd(16.7); ShInject(true);
    const DWORD tagB = Frame<MA>();                           // frame B's calls, hand and Present, QUEUED BEHIND THE HELD WORKER
    QueryPerformanceCounter(&q1);
    g.back = true;
    g.frameMs = (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart;
    g.tailMs = g.frameMs - 300.0;
    g.since1 = s.r.fixSince;
    Join(h, "FX5's release");
    Drain();
    if (!ShSettle()) Err("FX5: the fake helper did not settle behind the release");
    MgDueN();
    const auto sh = Shown();
    int iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++)
    {
        if (sh[i].tag == tagA) { g.aReal++; if (iA < 0) iA = (int)i; }
        if (ShIsGen(sh[i].tag) && ShGenPicture(sh[i].tag) == tagA) g.aGen++;
        if (sh[i].tag == tagB) { g.bReal++; if (iB < 0) iB = (int)i; }
    }
    g.aBeforeB = (iA >= 0 && (iB < 0 || iA < iB)) ? 1 : 0;
    WriteRelease(&s.r.fixOn, 0);
    MgClean("FX5");
    MgEnd();
    return g;
}
static void CaseFX5()
{
    const char* const name = "FX5 the freeze proof: a present held in DXVK's FIFO with a frame queued behind it, and a panel that stalls 100 ms: no freeze, every real frame once and in order (2026-10-17, night, F1)";
    CaseBegin(name);
    const Fx5Leg a = Fx5Run(1);
    const Fx5Leg o = Fx5Run(0);
    printf("  (a) THE FIXED MODE: pair A armed %s; the worker held inside DXVK's Present %s; frame B's calls, hand and Present came back %s, %.1f ms behind the release (300 ms; under 0: before it, the device thread's setting lets the render thread past the held worker); PaceFixed ran at B's Present %s;"
           " A's generated frame shown %d time(s), A's real frame %d, ahead of B's %s, B's real frame %d\n",
        a.armed ? "yes" : "NO", a.held ? "yes" : "NO", a.back ? "yes" : "NO", a.tailMs, a.since1 > a.since0 ? "yes" : "NO", a.aGen, a.aReal, a.aBeforeB ? "yes" : "NO", a.bReal);
    printf("  (a) THE SAME WITH THE MODE OFF: frame B came back %.1f ms behind the release; A's generated frame %d, A's real frame %d, ahead of B's %s, B's real frame %d\n", o.tailMs, o.aGen, o.aReal, o.aBeforeB ? "yes" : "NO", o.bReal);
    if (!a.armed || !a.held || !o.armed || !o.held) Err("FX5 (a): the leg did not reach its hold (pair A armed %d and %d, the worker held %d and %d)", (int)a.armed, (int)o.armed, (int)a.held, (int)o.held);
    if (!a.back || a.since1 <= a.since0) Err("FX5 (a): frame B came back %d, PaceFixed ran at its Present %d (both wanted)", (int)a.back, (int)(a.since1 > a.since0));
    if (fabs(a.tailMs - o.tailMs) > 50.0) Err("FX5 (a): frame B came back %.1f ms behind the release with the mode on and %.1f with it off (within 50 ms of each other wanted: the mode adds no wait)", a.tailMs, o.tailMs);
    if (a.aGen != 1 || a.aReal != 1 || !a.aBeforeB || a.bReal != 1) Err("FX5 (a): A's generated frame %d, A's real frame %d, ahead of B's %d, B's real frame %d (1, 1, 1 and 1 wanted)", a.aGen, a.aReal, a.aBeforeB, a.bReal);
    if (o.aGen != 1 || o.aReal != 1 || !o.aBeforeB || o.bReal != 1) Err("FX5 (a), the mode off: A's generated frame %d, A's real frame %d, ahead of B's %d, B's real frame %d (1, 1, 1 and 1 wanted)", o.aGen, o.aReal, o.aBeforeB, o.bReal);
    const RingStats stc = gRingSt;
    // (b) the panel stalls 100 ms at the run's present 1,200
    RtPar p = FxPar(120.0, kFx120, 1); p.panelStallAt = 1200; p.panelStallMs = 100;
    std::vector<FxWin> bw;
    const FxRead b = FxRun(p, 11.1, 2401, 500, 20261020u, nullptr, 1200 + 240, &bw);
    int back = -1;                                            // (round 2: the first window of 120 behind the stall (frame 600) from which the median stays in 250 to 750)
    for (int w = 5; w < (int)bw.size(); w++) { bool ok = true; for (int v = w; v < (int)bw.size(); v++) if (bw[v].med < 250 || bw[v].med > 750) { ok = false; break; } if (ok) { back = w - 5; break; } }
    RtPar pc = p; pc.guardOff = 1 << 13;
    std::vector<FxWin> cw;
    const FxRead bc = FxRun(pc, 11.1, 2401, 500, 20261020u, nullptr, 1200 + 240, &cw);
    ViolRestore(stc);
    Ctl("the same hold with the mode off: frame B back %.1f ms behind the release (the mode on: %.1f)", o.tailMs, a.tailMs);
    printf("  (b) the panel stalled 100 ms at the run's present 1,200, read from pair 500: %s; refreshes shown twice more than 2 s behind the stall %d\n", FxLine(b), b.repLate);
    printf("  (b) TOLD: generated frames withheld around the stall %d (a generated frame whose pair the stall made late is not shown; no real frame is lost), presents on the panel not the one due %d\n", b.n - b.gen, b.panelOrder);
    if (b.realTwice || b.never || b.genTwice || b.order || b.dropped || b.bound || b.n - b.gen > 4) Err("FX5 (b): real frames twice %d, never %d, generated twice %d, out of place %d (%s), dropped %ld, the FIFO at its bound %ld, generated frames withheld %d (all 0, and 4 at the most around the stall, wanted)", b.realTwice, b.never, b.genTwice, b.order, b.first.c_str(), b.dropped, b.bound, b.n - b.gen);
    if (b.holdMaxUs < 90000.0) Err("FX5 (b): the FIFO's longest hold %.0f us (the stall's 100 ms wanted: the stall did not reach the machine)", b.holdMaxUs);
    if (b.repLate) Err("FX5 (b): %d refresh(es) shown twice more than 2 s behind the stall (none behind its recovery wanted)", b.repLate);
    // (ROUND 2, the reviewer's first: BACK AT ITS TARGET WITHIN 10 WINDOWS OF THE STALL, and there to the end. Round 1 read the median 0 and
    // -585 ppm 30 s behind it: its control, the anti-windup off, must not come back)
    printf("  (b) behind the stall (window 5 of the run) the block's median back in 250 to 750 for good from window %d behind it; at the end the median %ld us, the correction %+ld ppm\n", back, b.medUs, b.ppm);
    printf("  (b) CONTROL, round 1's correction (the anti-windup off): at the end the median %ld us, the correction %+ld ppm\n", bc.medUs, bc.ppm);
    if (back < 0 || back > 10 || labs(b.ppm) > 300) Err("FX5 (b): the median came back in 250 to 750 for good %d window(s) behind the stall, the correction %+ld ppm at the end (10 at the most, and within 300 of 0, wanted)", back, b.ppm);
    if (bc.medUs >= 250 && bc.medUs <= 750 && labs(bc.ppm) <= 300) Err("FX5 (b) control: round 1's correction came back too (the median %ld us, %+ld ppm): the check cannot see the windup", bc.medUs, bc.ppm);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX6, the mode's memory of a quantised display (F5's rules)
// THE RULES (fg_cond.h, FgCondFixLatchStep; F5's reading of the owner's kept legs): the classifier's DISPLAY MEMORY is taken only from a
// window where the mod's own pacer does not hold the game; while it does the latch is kept whatever the memory says; a new refresh read, a
// Reset or NVIDIA's yes clears it. THE SELF-CONFIRMATION F5 WARNS OF CANNOT HAPPEN: a variable panel paced by the fixed mode's own pattern
// could read QUANTISED (rule 5 of the classifier comes before its pacer rule), but such a window is a held one, and a held window's memory
// is never taken: below, a stand-by that read FREE, then a hundred engaged windows whose memory reads QUANTISED (the worst case: the
// classifier fooled by the pace), never set the latch, and the table never enters the mode on them. (The classifier itself is not run here:
// it is DLSS-IV.cpp's and tools/flipcls replays it; engaged, the waiter hands it none of the frames the machine ends itself.)
// CONTROL: the same sequence with the hold ignored (every window taken) sets the latch on the first engaged window.
static void CaseFX6()
{
    const char* const name = "FX6 the fixed mode's memory of a quantised display: taken only while standing by, kept while held, cleared by a new refresh, a Reset or NVIDIA's yes; no self-confirmation (2026-10-17, night, F1)";
    CaseBegin(name);
    FgFixLatch L = {};
    const unsigned long long k1 = 0x0000007800000001ull, k2 = 0x0000007800000001ull ^ 2ull;
    struct Step { int mem, held, nvYes; unsigned long long key; int want; const char* what; };
    const Step steps[] =
    {
        { 0, 0, 0, k1, 0, "standing by, no memory yet" },
        { 2, 0, 0, k1, 1, "standing by, QUANTISED" },
        { 0, 1, 0, k1, 1, "engaged, the memory none: kept" },
        { 1, 1, 0, k1, 1, "engaged, the memory FREE: kept" },
        { 0, 0, 0, k1, 0, "standing by, the memory cleared (a contradiction)" },
        { 2, 0, 0, k1, 1, "standing by, QUANTISED again" },
        { 2, 0, 0, k2, 1, "a Reset: cleared, and this stand-by window takes QUANTISED" },
        { 2, 1, 0, k2 ^ 4ull, 0, "a new refresh while held: cleared, and the held window takes nothing" },
        { 2, 0, 1, k2 ^ 4ull, 0, "NVIDIA says variable: cleared, nothing taken" },
        { 1, 0, 0, k2 ^ 4ull, 0, "standing by, FREE" },
    };
    for (const Step& s : steps)
    {
        const int v = FgCondFixLatchStep(L, s.mem, s.held, s.key, s.nvYes);
        printf("  %-62s -> %d\n", s.what, v);
        if (v != s.want) Err("FX6: %s: the latch %d (%d wanted)", s.what, v, s.want);
    }
    // THE SELF-CONFIRMATION: a stand-by that read FREE, then 100 engaged windows whose memory reads QUANTISED, then the table
    FgFixLatch S = {}; int set = 0;
    FgCondFixLatchStep(S, 1, 0, k1, 0);
    for (int w = 0; w < 100; w++) set += FgCondFixLatchStep(S, 2, 1, k1, 0);
    FgCondIn in = FxSession(-1, S.quant, 1, 1, 120); in.realFpsX10 = 900;
    const int c = FgCondFirst(in, FgCondDefaults());
    printf("  a variable panel (no answer from NVIDIA) paced by the fixed mode's pattern: 100 engaged windows reading QUANTISED set the latch %d time(s); the table then: %s, the mode %s\n", set, FgCondEnumName(c), FgCondFixedMode(in) ? "ON" : "off");
    if (set || FgCondFixedMode(in) || c != FGC_VRR_UNKNOWN) Err("FX6: the fixed mode confirmed itself: the latch set %d time(s), the mode %d, the row %s (0, off and FGC_VRR_UNKNOWN wanted)", set, (int)FgCondFixedMode(in), FgCondEnumName(c));
    {   // CONTROL: the hold ignored
        FgFixLatch C = {}; int setC = 0;
        FgCondFixLatchStep(C, 1, 0, k1, 0);
        for (int w = 0; w < 100; w++) setC += FgCondFixLatchStep(C, 2, 0, k1, 0);
        printf("  CONTROL, the same windows taken as if nothing held the game: the latch set in %d of 100\n", setC);
        if (setC != 100) Err("FX6 control: with the hold ignored the latch set in %d of 100 windows (100 wanted: the check cannot see a taken window)", setC);
        Ctl("the hold ignored: the latch set in %d of 100 engaged windows", setC);
    }
    CaseEnd(name);
}

// ================================================================ ROUND 2 (2026-10-18, the night): THE INDEPENDENT REVIEWER'S FINDINGS
// ---------------------------------------------------------------- FX7, the game at the engage floor: the correction does not wind up
// THE REVIEWER'S RUN: a fixed 120 Hz panel, a game of 16.18 ms (the engage floor, 61.8 by itself) with 5 percent of spread: round 1's
// correction read the block's median 0 and ran to -2,424 ppm in 40 s, still falling (a frame past the period leaves nothing queued, and the
// integral took that as a pacer too slow). ROUND 2 (fg_machine.h, PaceFixed): a window whose median is 0 and whose pacer met 3 or more of its
// presents late moves nothing. HELD: over 40 s THE CORRECTION NEVER PASSES -1,000 ppm (the seed of -300, one window's proportional half and
// the integral's few steps where a queue was seen), and the mode either holds the block's median in 250 to 750 at the end OR STEPS ASIDE: the
// conditions' overrun (fg_cond.h; round 7 the share of the law's 12 s, FgCondFixShareStep) stands it down, said with its window. THE SAME AT 16.0 ms: healthy (no
// refresh shown twice over the run, no overrun, the correction within the bound; its median told: the game's slowest frames reach the
// period, so the correction builds no queue ahead of them and stands still). Both: FQ4's assertions. CONTROL (a run): round 1's correction (the guard's bit 13 off): past -1,000 ppm.
static void FxEdge(const char* what, double costMs, int guardOff, LONG* ppmMin, int* overAt, FxRead* out)
{
    RtPar p = FxPar(120.0, kFx120, 1); p.guardOff = guardOff;
    std::vector<FxWin> w;
    *out = FxRun(p, costMs, 2401, 600, 20261018u, nullptr, 0x40000000, &w);
    *ppmMin = 0; *overAt = -1;
    printf("  %s: by window of 120 frames, the block's median / the correction / the law's L / over:", what);
    for (size_t i = 0; i < w.size(); i++) { if (w[i].ppm < *ppmMin) *ppmMin = w[i].ppm; if (w[i].over && *overAt < 0) *overAt = (int)i; printf(" %ld/%+ld/%ld/%d", w[i].med, w[i].ppm, w[i].lUs, w[i].over); }
    printf("\n  %s: %s\n", what, FxLine(*out));
}
static void CaseFX7()
{
    const char* const name = "FX7 the game at the engage floor (16.18 ms on a fixed 120 Hz): the correction never passes -1,000 ppm, and the mode holds its target or steps aside (2026-10-18, F1 round 2)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    LONG mE, mH, mC; int oE, oH, oC; FxRead e, h, c;
    FxEdge("16.18 ms", 16.18, 0, &mE, &oE, &e);
    FxEdge("16.0 ms", 16.0, 0, &mH, &oH, &h);
    FxEdge("CONTROL, 16.18 ms with round 1's correction", 16.18, 1 << 13, &mC, &oC, &c);
    ViolRestore(stc);
    printf("  16.18 ms: the correction at its least %+ld ppm, the overrun at window %d, held for the game %ld window(s); 16.0 ms: at its least %+ld ppm, the overrun at window %d; CONTROL: at its least %+ld ppm\n", mE, oE, e.held, mH, oH, mC);
    if (mE < -1000) Err("FX7: at 16.18 ms the correction reached %+ld ppm (never past -1,000 wanted)", mE);
    // (round 7, F1: OR STAYS BY THE BULK: the share of its frames over the line under 100 of 1000, at most 1 of 100 refreshes shown twice, the
    // legs' own bar (16.18 ms with 5 of 100 of spread: about 40 of 1000 over 16.92 ms, 6 of 3,600 refreshes twice))
    if (oE < 0 && (e.medUs < 250 || e.medUs > 750) && e.repeats * 100 > e.shown) Err("FX7: at 16.18 ms the mode neither stepped aside nor held its target (the median %ld us) and %d of %d refreshes were shown twice (1 of 100 at the most wanted)", e.medUs, e.repeats, e.shown);
    else if (oE < 0 && (e.medUs < 250 || e.medUs > 750)) printf("  TOLD (round 7): at 16.18 ms the mode stays by the bulk, the block's median %ld us, %d of %d refreshes shown twice\n", e.medUs, e.repeats, e.shown);
    if (e.realTwice || e.never || e.genTwice || e.order || e.dropped) Err("FX7: at 16.18 ms real frames twice %d, never %d, generated twice %d, out of place %d, dropped %ld (all 0 wanted)", e.realTwice, e.never, e.genTwice, e.order, e.dropped);
    const std::string bh = FxJudge(h);
    if (!bh.empty() || h.repAll || oH >= 0 || mH < -1000) Err("FX7: at 16.0 ms %s %d refresh(es) twice over the run, the overrun at window %d, the correction at its least %+ld ppm (healthy, none, none and never past -1,000 wanted)", bh.c_str(), h.repAll, oH, mH);
    printf("  TOLD: at 16.0 ms the block's median %ld us at the end: the game's slowest frames reach the period, so no queue is built ahead of them and the correction stands still (held for the game %ld window(s)); no refresh shown twice\n", h.medUs, h.held);
    if (mC >= -1000) Err("FX7 control: round 1's correction stayed within -1,000 ppm (%+ld): the check cannot see the windup", mC);
    Ctl("round 1's correction at 16.18 ms: %+ld ppm at its least", mC);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX8, a game that overruns the period stands down; one that does not, stays
// THE REVIEWER'S RUN: engaged in the fixed mode the pacer holds 16.67 ms, so the window's median real rate stays at about 60 whatever the game
// costs: a game of 16.4 ms (61 by itself) read 60.09, showed 4 refreshes twice in 1,200, and was never stood down. ROUND 2 (fg_cond.h,
// FgCondFixOverStep): THE LAW'S L over two refreshes and a quarter of a millisecond, two windows running, is FGC_SLOW in the mode. (ROUND 7, F1:
// the share of the law's last 12 s over that line past 100 of 1000 with its E over it, two windows running, FgCondFixShareStep: the 16.4 ms game
// reads about 180; the case's bound of 6 windows behind window 8 holds the 12 s turning over. CONTROL 2, gFxR6: round 6's rule, as before)
// WHAT RUNS: the law itself (fg_cond.h's FgDrive*, fg_arm_life.h's reading as dt_fg_law.h's LwLawWindow reads it) over the fixed shape, the
// session the fixed mode's (NVIDIA's no, vsync, fg_fixed 1), the overrun stepped once a window from the machine's own L. HELD: at 16.4 ms the
// law, engaged at 16.0 ms, stands the mode down (ON to REST) within 6 windows of the game going to 16.4 ms (the overrun's 2 windows and the law's 4 s of samples turning over; standing by a game of 16.4 never engages: SLOW at 61.8); at 16.0 ms it stays
// ON to the end of 40 windows; A GAME WANDERING ACROSS THE EDGE (16.0 and 16.5 ms by turns, 30 s each, five minutes): at most 5 engages and
// stand-downs (the law's rest and back-off: the plan's "at most 4 transitions in 5 minutes on a steady route", one more for the wander's own
// first engage). Every real frame once and in order. CONTROL (a run): the overrun not read (round 1's conditions): 16.4 ms never stands down.
static int gFxEntryX10 = 0, gFxWasEng = 0, gFxNoEntry = 0, gFxEntryR2 = 0; static FgFixEntry gFxEntry;   // (gFxEntryR2: round 2's latch, FX12's control)   // (gFxNoEntry: FX11's control, the way back in not asked)                    // (round 2: the way back in, as fg_arm_life.h's FgpFixPublish keeps it)
static LwRec FxLawWindow(LwLaw& L, const RtPar& par, int first, int last, int& overRun, bool readOver)
{
    FgState& s = B::S();
    LwRec r = {};
    r.frames = last - first; r.med = LwMedEntryMs(first, last); r.game = LwMedCostMs(first, last);
    const bool engaged = LwEngagedRead(L);                    // (phase 5 round 2, S2: the product's reading, behind the development build's PASS where planted)
    const int card = LwCardRead(L);
    const LONG eUs = s.r.lawEUs;
    FgCondIn in = LwSession(par);
    in.vrr = 0; in.vsync = 1; in.fixedKey = 1;                // (the fixed mode's session)
    LwWindowFree(engaged, eUs, r.med, in, L.mem.aloneX10);
    LwFill(in, LW_NOW, engaged, r.med, eUs, &L.mem, &L.k, card, L.oldRead ? 0 : 1);
    in.engaging = (!L.oldRead && !L.noOnslowBand && L.mem.state == FGDR_ENGAGING) ? 1 : 0;   // (S2 round 2)
    in.fixLUs = engaged ? (int)s.r.lawLUs : 0;
    const int wasOver = overRun >= FG_COND_FIX_OVER_WIN ? 1 : 0;
    const int over = FxOverStep(overRun, s, FgCondFloorUs(in) + FG_COND_FIX_OVER_US, engaged);
    in.fixSharePm = gFxSharePm;
    in.fixDipFirst = gFxR6 ? 0 : FgCondFixDipStep(gFxDipRun, in);   // (round 7: the dip two windows running; round 6's rule had none)
    const int setX10 = (readOver && over && !wasOver && s.law) ? FgCondFixEntryX10(in.fixLUs, FgCondRingMedUs(s.law->fast), FgCondFloorUs(in)) : 0;
    if (engaged && !gFxWasEng && !over) { gFxEntry.x10 = 0; gFxEntry.n = 0; gFxEntryX10 = 0; }
    // (G4's round 2: the way back in behind a stand-down by SLOW too, as fg_arm_life.h's FgpFixPublish sets it)
    if (!gFxEntryR2 && !engaged && gFxWasEng && !setX10 && gFxEntry.x10 == 0 && L.mem.state == FGDR_REST && L.mem.prev == FGDR_ON && L.mem.why == FGDRW_ROW && L.mem.whyRow == FGC_SLOW)
        FgCondFixEntrySlowSet(gFxEntry, FgCondFloorX10(in, FgCondDefaults()));
    if (gFxEntryR2) { if (setX10) gFxEntryX10 = setX10; }
    else gFxEntryX10 = FgCondFixEntryStep(gFxEntry, setX10, engaged ? 0 : 1, FgCondFloorX10(in, FgCondDefaults()));
    gFxWasEng = engaged ? 1 : 0;
    in.fixOver = readOver ? over : 0; in.fixEntryX10 = (readOver && !gFxNoEntry) ? gFxEntryX10 : 0;
    in.lift |= FgDriveWarmLift(L.mem);
    FgDriveIn di = {};
#if defined(FG_COND_ALONE_FAST)
    di.realX10 = FgCondAloneX10(in); di.card = card; di.lawPerUs = engaged ? gLwLawPerUs : 0;   // (the reviewer's R1: as FgpLawWindowStep)
#else
    di.realX10 = in.realFpsX10; di.card = card; di.lawPerUs = engaged ? gLwLawPerUs : 0;
#endif
    di.floorUs = (engaged && !L.oldRead) ? FgCondFloorUs(in) : 0;   // (S2: as fg_arm_life.h's FgpLawWindowStep)
    di.perUs = (engaged && !L.oldRead) ? gLwPerUs : 0;              // (S2: the period in force, FgPace's own)
    if (engaged) { di.heldUpX10 = FgDriveHeldUpX10(gLwMedUs, gLwPerUs, FgCondFloorUs(in), r.med); di.fastX10 = FgDriveFastX10(in, L.ck); }   // (G4's round 3: the probe's)
    di.row = FgCondFirst(in, L.ck); di.band = FgDriveBandOf(in, L.ck);
    { const LONG pi = s.c.presentsI, pn = s.c.presentsN; di.shownGen = (int)(pi - L.presI); di.shownN = (int)(pn - L.presN); L.presI = pi; L.presN = pn; }
    { const LONG pl = ShArmB::a.w.placeN; di.cardN = FgDrivePlaceMed(ShArmB::a.w.placeV, pl, L.place, &di.cardMed); L.place = pl; }
    di.cpuReady = 1;
#if defined(FG_DRIVE_FIXED_AT_ONCE)
    di.fixed = FgCondFixedMode(in) ? 1 : 0;                        // (round 9, L3b: as fg_arm_life.h's FgpLawWindowStep)
#endif
    const int was = L.mem.state;
    if (FgDriveStep(L.mem, di, L.k, LwNowMs(), (int)B::camJumps, 0))
        if (L.mem.state == FGDR_ON || was == FGDR_ON) L.turnsMs.push_back((double)LwNowMs());
    s.wt.want = FgDriveWantsOn(L.mem) ? FG_WANT_ON : FG_WANT_PASS;
    r.rX10 = in.realFpsX10; r.rFree = LwFreeX10(LW_NOW, engaged, eUs); r.rPred = in.predFpsX10; r.cUs = FgDriveCUs(L.mem, card, nullptr); r.band = di.band;   // (S2: for FX14's table)
    r.row = di.row; r.state = L.mem.state; r.prev = was; r.why = L.mem.why; r.want = s.wt.want;
    return r;
}
struct Fx8Leg { int engagedAt, downAt, turns, order, n, real1, wins; std::string first; };
static Fx8Leg Fx8Run(double costA, double costB, int windows, int switchWin, bool readOver, bool alternate, double hoverMs = 0.0)
{
    Fx8Leg g = {}; g.engagedAt = g.downAt = -1;
    RtPar par = FxPar(120.0, kFx120, 1);
    RtBegin(par);
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;          // (no wait for a cut: the engage at once behind the dwell)
    LwLaw L; LwLawBegin(L, k, FgCondDefaults());
    gRtSeed = 20261018u;
    int overRun = 0; gFxEntryX10 = 0; gFxWasEng = 0; gFxEntry = FgFixEntry(); gFxDipRun = 0;
    for (int win = 0; win < windows; win++)
    {
        double cost = switchWin <= 0 ? costA : alternate ? (((win / switchWin) & 1) ? costB : costA) : (win < switchWin ? costA : costB);
        if (hoverMs > 0.0) cost = costA * RtSpread(hoverMs / costA);   // (a game hovering at the edge: its own cost drawn afresh each window)
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(cost * RtSpread(0.05))); LwLawPoll(L); }
        const LwRec r = FxLawWindow(L, par, first, (int)gRt.pairs.size(), overRun, readOver);
        if (r.state == FGDR_ON && g.engagedAt < 0) g.engagedAt = win;
        if (r.prev == FGDR_ON && r.state != FGDR_ON && g.downAt < 0) g.downAt = win;
    }
    g.turns = (int)L.turnsMs.size(); g.wins = windows;
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("FX8");
    return g;
}
static void CaseFX8()
{
    const char* const name = "FX8 engaged in the fixed mode, a game that overruns two refreshes stands down by the law's L; one that does not, stays (2026-10-18, F1 round 2)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const Fx8Leg a = Fx8Run(16.0, 16.4, 20, 8, true, false);    // (engaged at 16.0, then heavier: 16.4 from window 8; standing by a game of 16.4 never engages, SLOW at 61.8)
    const Fx8Leg b = Fx8Run(16.0, 16.0, 40, 0, true, false);
    const Fx8Leg c = Fx8Run(16.0, 16.4, 20, 8, false, false);
    ViolRestore(stc);
    printf("  16.0 then 16.4 ms from window 8: engaged at window %d, stood down at window %d; 16.0 ms: engaged at window %d, stood down at window %d (of %d); CONTROL, the overrun not read: engaged at %d, stood down at %d\n",
        a.engagedAt, a.downAt, b.engagedAt, b.downAt, b.wins, c.engagedAt, c.downAt);
    if (a.engagedAt < 0 || a.engagedAt >= 8 || a.downAt < 8 || a.downAt > 8 + 6) Err("FX8: engaged at 16.0 ms at window %d and, 16.4 ms from window 8, stood down at %d (engaged before 8, down within 6 windows of 8 wanted)", a.engagedAt, a.downAt);
    if (b.engagedAt < 0 || b.downAt >= 0) Err("FX8: at 16.0 ms engaged at window %d and stood down at %d (engaged, and never down, wanted)", b.engagedAt, b.downAt);
    for (const Fx8Leg* l : { &a, &b }) if (l->order || l->real1 != l->n) Err("FX8: %d picture(s) out of place (%s), %d of %d real frames shown once", l->order, l->first.c_str(), l->real1, l->n);
    if (c.engagedAt < 0 || c.downAt >= 0) Err("FX8 control: with the overrun not read the game going to 16.4 ms engaged at %d and stood down at %d (engaged and never down wanted: the check cannot see the overrun)", c.engagedAt, c.downAt);
    Ctl("the overrun not read: 16.4 ms engaged at window %d and never stood down", c.engagedAt);
    CaseEnd(name);
}
// ---------------------------------------------------------------- FX11 (by name: ten minutes of game), a game at the edge flaps no more than the law allows
// THE REVIEWER'S "NO FLAP AT THE EDGE": A GAME HOVERING AT THE EDGE (16.12 ms, its own cost drawn afresh each window 0.15 ms either way, five
// minutes) makes at most 5 engages and stand-downs (the plan's "at most 4 transitions in 5 minutes on a steady route", one more for the first
// engage). WHAT HOLDS IT: the way back in (fg_cond.h, FgCondFixEntryX10). CONTROL (a run): the way back in not asked: the median's moves across
// 61.8 clear the law's back-off window to window, and it flaps (twenty in round 2's first form). TOLD: a game that really moves, 16.0 and 16.5
// ms by turns of 30 s (each move of its own band clears the law's back-off by design: the game is elsewhere).
static void CaseFX11()
{
    const char* const name = "FX11 a game hovering at the engage edge for five minutes makes at most 5 engages and stand-downs in the fixed mode (2026-10-18, F1 round 2)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const Fx8Leg hv = Fx8Run(16.12, 16.12, 150, 0, true, false, 0.15);
    const Fx8Leg w = Fx8Run(16.0, 16.5, 150, 15, true, true);
    // (round 7, F1: by the bulk the hovering game is never stood down (one engage), so the control is ROUND 6's RULE with the way back in not
    // asked, gFxR6: the flap the way back in exists for, which the check must still see)
    gFxNoEntry = 1; gFxR6 = 1; const Fx8Leg c = Fx8Run(16.12, 16.12, 150, 0, true, false, 0.15); gFxNoEntry = 0; gFxR6 = 0;
    ViolRestore(stc);
    printf("  hovering at the edge (16.12 ms, 0.15 either way window to window), five minutes: %d engage(s) and stand-down(s); TOLD, the game moving 16.0 and 16.5 ms by turns of 30 s: %d; CONTROL, the way back in not asked: %d\n", hv.turns, w.turns, c.turns);
    if (hv.turns > 5) Err("FX11: the game hovering at the edge made %d engage(s) and stand-down(s) in five minutes (5 at the most wanted)", hv.turns);
    for (const Fx8Leg* l : { &hv, &w }) if (l->order || l->real1 != l->n) Err("FX11: %d picture(s) out of place (%s), %d of %d real frames shown once", l->order, l->first.c_str(), l->real1, l->n);
    if (c.turns <= 5) Err("FX11 control: with the way back in not asked the hovering game made %d engage(s) and stand-down(s) (more than 5 wanted: the check cannot see a flap)", c.turns);
    Ctl("round 6's rule with the way back in not asked: %d engages and stand-downs in five minutes", c.turns);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX9, DXVK's frame queue at 2 and 3
// THE REVIEWER'S THIRD: with d3d9.maxFrameLatency at d a present waits only once d presents stand unshown, so the block the correction holds at
// 500 us is the d-th's: the queue is d deep, (d - 1) refreshes of picture age more than at 1 (the mod asks for 1 where responsiveness is on, its
// default; a player's responsiveness 0 or own deeper value leaves DXVK's). NOTHING SHALLOWER CAN BE SEEN FROM INSIDE THE PRESENT (a queue under
// d deep never waits), so the shallowest the mode can hold with no refresh shown twice is d deep at its target. HELD at d = 2 and 3 on the
// fixed 120 Hz panel, a game of 90: every present one refresh after the one before, none twice, the block's median in 250 to 750; TOLD: the real
// frame's age on the panel against d = 1's (FX1's shape), about a refresh more a slot.
static void CaseFX9()
{
    const char* const name = "FX9 DXVK's frame queue at 2 and 3: no refresh shown twice, the block at its target with the queue as shallow as a Present can see, the age told (2026-10-18, F1 round 2)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    FxRead r[3];
    for (int d = 1; d <= 3; d++)
    {
        RtPar p = FxPar(120.0, kFx120, 1); p.fifo = d; std::vector<FxWin> w;
        r[d - 1] = FxRun(p, 11.1, 2401, 1800, 20261017u, nullptr, 0x40000000, &w);
        printf("  the frame queue %d by window, the block's median / the correction:", d); for (const FxWin& x : w) printf(" %ld/%+ld", x.med, x.ppm); printf("\n");
    }
    ViolRestore(stc);
    for (int d = 1; d <= 3; d++)
    {
        printf("  the frame queue %d: %s\n", d, FxLine(r[d - 1]));
        const std::string bad = FxJudge(r[d - 1]);
        if (!bad.empty()) Err("FX9: the frame queue %d: %s", d, bad.c_str());
        if (r[d - 1].repAll || r[d - 1].medUs < 250 || r[d - 1].medUs > 750) Err("FX9: the frame queue %d: %d refresh(es) twice over the run, the median %ld us (none, and 250 to 750, wanted)", d, r[d - 1].repAll, r[d - 1].medUs);
    }
    printf("  TOLD: the real frame's age on the panel, medians: %.2f ms at a queue of 1, %.2f at 2, %.2f at 3\n", r[0].agePanelMs, r[1].agePanelMs, r[2].agePanelMs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX10, a Present whose own time is 500 us is not read as a hold
// THE REVIEWER'S THIRD, ITS SECOND HALF: a slow CPU's Present takes time with no queue at all, and the correction would take it for the queue's
// slack: holding 500 us of it leaves the queue none. ROUND 2: THE CONDITIONS MEASURE THE PRESENT'S OWN TIME where the game ran free under vsync
// (fg_cond.h, FgCondFixBaseStep: the least window mean while standing by, the game under 85 of 100 of the refresh) and the machine takes it
// off every hold (FgRenderLine::fixBaseUs). HELD: (1) the measurement over a stand-by of windows whose presents seldom wait reads 500 (and a
// window at the refresh's ceiling, whose presents all wait, is not taken); (2) the fixed shape with the Present at 500 us of its own and that
// measurement handed in: no refresh shown twice, the queue's own wait at least 250 us on the mean (the slack is real). CONTROL (a run): the
// measurement not handed in: the correction holds the Present's own time and the queue's own wait falls under 150 us.
static void CaseFX10()
{
    const char* const name = "FX10 a Present of 500 us of its own is not read as a hold: the conditions measure it standing by and the machine takes it off (2026-10-18, F1 round 2)";
    CaseBegin(name);
    int base = -1;
    static const int means[][2] = { { 640, 900 }, { 520, 900 }, { 500, 880 }, { 3000, 1190 }, { 510, 900 } };   // (the window's mean present us, the game's rate in tenths)
    for (const auto& m : means) FgCondFixBaseStep(base, m[0], m[1], 120000, 1);
    int baseEng = -1; FgCondFixBaseStep(baseEng, 500, 900, 120000, 0);
    printf("  the Present's own time measured over five stand-by windows (640, 520, 500, the refresh's ceiling at 3000, 510 us): %d us; a window engaged is not taken: %d\n", base, baseEng);
    if (base != 500 || baseEng != -1) Err("FX10: the measurement read %d and %d (500, and none from an engaged window, wanted)", base, baseEng);
    // (round 3, the reviewer's second: THE MEASUREMENT WHERE IT REACHES THE TYPICAL GAME, standing by on the fixed panel under vsync with the
    // Present at 500 us of its own: windows of 120 frames, each window's mean of the worker's time inside DXVK's Present over the device thread's
    // present count, as fg_arm_life.h's FgpFixPublish reads them, stepped by FgCondFixBaseStep with the game's own rate. A game of 100 (under
    // 97 of 100 of the refresh: its frames come more than a refresh apart and its presents do not wait) is measured within 500 and 650; a game
    // the vsync caps at the refresh (a frame of 6 ms) is not taken: every present of it waits, and it is FAST's, never the mode's)
    auto standby = [](double costMs, int* base) -> int
    {
        RtPar q = FxPar(120.0, kFx120, 1); q.presCpuUs = 500;
        RtBegin(q); B::S().wt.want = FG_WANT_PASS;
        gRtSeed = 20261018u;
        LONGLONG t0 = dtPresTicksSum; LONG d0 = (LONG)dtPresDone; LONGLONG e0 = 0; int realX10 = 0;
        for (int i = 0; i < 1200; i++)
        {
            if (i % 120 == 0) e0 = B::clockNow;
            RtFrame(RtMs(costMs * RtSpread(0.05)));
            if (i % 120 == 119)
            {
                const LONGLONG t1 = dtPresTicksSum; const LONG d1 = (LONG)dtPresDone;
                realX10 = (int)(1200.0 * (double)B::Qpf() / (double)(B::clockNow - e0) + 0.5);
                const int meanUs = d1 > d0 ? (int)((double)(t1 - t0) * 1e6 / (double)B::Qpf() / (double)(d1 - d0)) : 0;
                if (i > 240) FgCondFixBaseStep(*base, meanUs, realX10, 120000, 1);
                t0 = t1; d0 = d1;
            }
        }
        RtEnd("FX10");
        return realX10;
    };
    int bFree = -1, bCap = -1;
    const int rFree = standby(10.0, &bFree), rCap = standby(6.0, &bCap);
    printf("  STANDING BY on the fixed panel, the Present at 500 us of its own: a game at %d.%d measured %d us; a game the vsync caps at %d.%d: %s\n", rFree / 10, rFree % 10, bFree, rCap / 10, rCap % 10, bCap < 0 ? "not taken" : "TAKEN");
    if (bFree < 500 || bFree > 650 || bCap >= 0) Err("FX10: standing by, a game at %d.%d measured %d us (500 to 650 wanted) and the capped game %d (none wanted)", rFree / 10, rFree % 10, bFree, bCap);
    const RingStats stc = gRingSt;
    RtPar p = FxPar(120.0, kFx120, 1); p.presCpuUs = 500; p.fixBaseUs = bFree;
    const FxRead o = FxRun(p, 11.1, 2401, 1800, 20261017u);
    RtPar pc = p; pc.fixBaseUs = 0;
    const FxRead c = FxRun(pc, 11.1, 2401, 1800, 20261017u);
    ViolRestore(stc);
    printf("  THE PRESENT AT 500 us, ITS TIME TAKEN OFF: %s\n  CONTROL, its time not taken off: %s\n", FxLine(o), FxLine(c));
    const std::string bad = FxJudge(o);
    if (!bad.empty() || o.repAll || o.waitUs < 250.0) Err("FX10: %s %d refresh(es) twice over the run, the queue's own wait %.0f us on the mean (none, and 250 us or more, wanted)", bad.c_str(), o.repAll, o.waitUs);
    if (c.waitUs >= 150.0) Err("FX10 control: with the Present's time not taken off the queue's own wait read %.0f us on the mean (under 150 wanted: the check cannot see the Present read as a hold)", c.waitUs);
    Ctl("the Present's time not taken off: the queue's own wait %.0f us on the mean, %d refresh(es) twice", c.waitUs, c.repAll);
    CaseEnd(name);
}

// ================================================================ ROUND 3 (2026-10-18): THE REVIEWER'S CHECK OF ROUND 2
// ---------------------------------------------------------------- FX12, the way back in relaxes: no latch beyond its event
// THE REVIEWER'S EXAMPLE: an overrun read at L 17.0 ms over a median of 13.5 asked a game of 75.9 standing by before the mode could engage
// again, and round 2 cleared that only at the next engage, which its own SLOW then refused: a calm scene at 72 stood by until a Reset. ROUND 3
// (fg_cond.h, FgCondFixEntryStep): capped at 108 of 100 of the plain floor (66.7 at 120 Hz), and falling to the plain floor in a straight line
// over 60 standing-by windows (doubled where an overrun soon comes back, FX11). HELD, over the conditions themselves on the fixed session: the calm scene at 72 engages (FGC_OK) at once; a
// game at 64 engages within 40 windows; one at 63 within 60; and none stands by for SLOW past window 60. CONTROL: round 2's latch (no cap, no
// relaxing): the scenes at 72 and 64 stand by for SLOW for all of 200 windows.
static void CaseFX12()
{
    const char* const name = "FX12 the way back in behind an overrun is capped and relaxes: the reviewer's calm scene at 72 engages again, none waits past 60 windows (2026-10-18, F1 round 3)";
    CaseBegin(name);
    const FgCondK k = FgCondDefaults();
    const int setX10 = FgCondFixEntryX10(17000, 13500, 16666);
    printf("  the way back in measured at L 17.0 ms over a median of 13.5: %d.%d (round 2 asked that)\n", setX10 / 10, setX10 % 10);
    struct G { int realX10; int round2; int okAt; };
    G g[] = { { 720, 0, -1 }, { 640, 0, -1 }, { 630, 0, -1 }, { 720, 1, -1 }, { 640, 1, -1 } };
    for (G& x : g)
    {
        FgFixEntry E = {}; int latch = 0;
        for (int w = 0; w < 200 && x.okAt < 0; w++)
        {
            FgCondIn in = FxSession(0, 0, 1, 1, 120); in.realFpsX10 = x.realX10;
            const int floorX10 = FgCondFloorX10(in, k);
            if (x.round2) { if (w == 0) latch = setX10; in.fixEntryX10 = latch; }
            else in.fixEntryX10 = FgCondFixEntryStep(E, w == 0 ? setX10 : 0, 1, floorX10);
            if (FgCondFirst(in, k) == FGC_OK) x.okAt = w;
        }
        printf("  %s, a game at %d.%d standing by: engages at window %d\n", x.round2 ? "CONTROL, round 2's latch" : "round 3", x.realX10 / 10, x.realX10 % 10, x.okAt);
    }
    if (g[0].okAt != 0) Err("FX12: the calm scene at 72 engaged at window %d (at once wanted: the cap is 66.7)", g[0].okAt);
    if (g[1].okAt < 0 || g[1].okAt > 40) Err("FX12: a game at 64 engaged at window %d (within 40 wanted)", g[1].okAt);
    if (g[2].okAt < 0 || g[2].okAt > 60) Err("FX12: a game at 63 engaged at window %d (within 60 wanted)", g[2].okAt);
    if (g[3].okAt >= 0 || g[4].okAt >= 0) Err("FX12 control: round 2's latch let the game at 72 engage at window %d and the one at 64 at %d (never wanted: the check cannot see the latch)", g[3].okAt, g[4].okAt);
    Ctl("round 2's latch: the scenes at 72 and 64 never engage in 200 windows");
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX13, a blind window teaches only toward faster, and within a bound
// THE REVIEWER'S LOW ONE: a window in which nothing waited and the pacer held the game still taught -50 ppm, so a screen whose FIFO never holds
// (a compositor) ran the pacer fast without end. ROUND 3: blind windows teach only toward faster and never past -1,200 ppm (fg_machine.h,
// FG_FIX_BLIND_I_PPM). HELD: the fixed shape on a panel whose FIFO never holds, 2,400 frames: the correction never past -1,450 ppm (the bound
// and a blind window's proportional half), and no real frame is lost. (FX2 tells the reviewer's form, no blind teaching, on its mirror panel.)
static void CaseFX13()
{
    const char* const name = "FX13 a blind window teaches only toward faster and never past -1,200 ppm: a FIFO that never holds leaves the correction bounded (2026-10-18, F1 round 3)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    RtPar p = FxPar(120.0, kFx120, 1); p.fifo = 0;
    std::vector<FxWin> w;
    const FxRead o = FxRun(p, 11.1, 2401, 600, 20261018u, nullptr, 0x40000000, &w);
    LONG least = 0; for (const FxWin& x : w) if (x.ppm < least) least = x.ppm;
    printf("  by window, the block's median / the correction:"); for (const FxWin& x : w) printf(" %ld/%+ld", x.med, x.ppm); printf("\n");
    // (G4's round 4: THE BOUND EXACT, from the machine's own constants: the blind integral's FG_FIX_BLIND_I_PPM and a blind window's proportional
    // half, (0 - FG_FIX_TARGET_US) / 2: -1,200 - 250 = -1,450. AND THE PRESENT'S OWN TIME JITTERING 0 to 9 us (RtPar::presNoiseUs): the base runs
    // of the gate (now and then -1,498 or -1,499 at threaded 2, exactly -1,450 at threaded 1) were this, the fake worker's real time inside its
    // Present under load: a median of 1 or 2 us made a window read as not blind, its integral took -49 past the bound, and the blind windows
    // behind it kept that. The tracker, not the bound: fg_machine.h's PaceFixed reads a median under one of its steps, FG_FIX_TRACK_US, as blind)
    const LONG bound = (LONG)FG_FIX_BLIND_I_PPM - (LONG)FG_FIX_TARGET_US / 2;
    RtPar pn = p; pn.presNoiseUs = 9;
    std::vector<FxWin> wn;
    const FxRead on = FxRun(pn, 11.1, 2401, 600, 20261018u, nullptr, 0x40000000, &wn);
    LONG leastN = 0; for (const FxWin& x : wn) if (x.ppm < leastN) leastN = x.ppm;
    printf("  the Present's own time jittering 0 to 9 us, by window:"); for (const FxWin& x : wn) printf(" %ld/%+ld", x.med, x.ppm); printf("\n");
    ViolRestore(stc);
    printf("  a FIFO that never holds: the correction at its least %+ld ppm, at the end %+ld (the bound %+ld); with the Present's time jittering %+ld; %s\n", least, o.ppm, bound, leastN, FxLine(o));
    if (least < bound) Err("FX13: with a FIFO that never holds the correction reached %+ld ppm (never past %+ld wanted)", least, bound);
    if (leastN < bound) Err("FX13: with the Present's own time jittering 0 to 9 us the correction reached %+ld ppm (never past %+ld wanted: a median under one tracker step is blind)", leastN, bound);
    if (on.realTwice || on.never || on.genTwice || on.order || on.dropped) Err("FX13 (the jitter): real frames twice %d, never %d, generated twice %d, out of place %d, dropped %ld", on.realTwice, on.never, on.genTwice, on.order, on.dropped);
    if (o.realTwice || o.never || o.genTwice || o.order || o.dropped) Err("FX13: real frames twice %d, never %d, generated twice %d, out of place %d, dropped %ld (all 0 wanted)", o.realTwice, o.never, o.genTwice, o.order, o.dropped);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX14, THE MORNING'S fixed120 LEG (phase 5 round 2, S2, 2026-10-18)
// WHAT THE LEG SHOWED (docs/reports/fg-5/sitting-2026-10-18, F1C-fixed120: 1455 MHz, G-Sync off, the game about 97 by itself): THE MODE RAN,
// engaged behind window 15, ON 16 to 19 at EXACTLY 60.0 real and 120 of 120 shown; then (a) FGC_SLOW stood it down at R_on 60.0 against the
// STANDING-BY floor 61.8 ("the game runs at about 60 fps BY ITSELF"), and (b) c was filed as 6,421 us "with room" from the pacer's own hold
// (16.67 ms less the game's own 10.25), so FGC_ONSLOW kept the mode out for the rest of the leg; the law's L read 0 in every window. THE
// CAUSE IS LW19's (dt_fg_law.h): in the armed development build the conditions read an engaged session as standing by, so the engaged rule
// (99 of 100 of half the refresh, and the overrun by L) never ran and c had no law period. THE CASE: the fixed 120 Hz shape (NVIDIA's no, vsync
// on, fg_fixed 1), a game of 10.25 ms (5 percent of spread), the card read with idle room standing by and engaged (the leg's at its engage),
// the cut wait 10 s, 60 windows (two minutes), IN THE DEVELOPMENT BUILD'S WINDOW ORDER. HELD, THE PRODUCT'S READING: engaged within 20
// windows and never stood down; c 300 us at the most (the pacer, not the card, holds the game at two refreshes); every real frame once and in
// order. CONTROL, THE READING OF BEFORE: a stand-down by FGC_SLOW at an R_on of 59.4 or more and c filed at 5,000 us or more, as the leg's.
struct Fx14Leg { std::vector<LwRec> w; int engagedAt, downAt, downRow, downX10, cRoomUs, cLimUs, order, n, real1; std::string first; };
static Fx14Leg Fx14Run(int oldRead)
{
    Fx14Leg g = {}; g.engagedAt = g.downAt = -1;
    RtPar par = FxPar(120.0, kFx120, 1);
    RtBegin(par);
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.card = 0; L.cardEngaged = 0; L.devPass = 1; L.oldRead = oldRead;
    L.oldShadow = oldRead;                                    // (review round 2: the product's writers with the product's reading; the control the leg's)
    gRtSeed = 20261018u;
    int overRun = 0; gFxEntryX10 = 0; gFxWasEng = 0; gFxEntry = FgFixEntry(); gFxDipRun = 0;
    for (int win = 0; win < 60; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(10.25 * RtSpread(0.05))); LwLawPoll(L); }
        const LwRec r = FxLawWindow(L, par, first, (int)gRt.pairs.size(), overRun, true);
        g.w.push_back(r);
        if (r.state == FGDR_ON && g.engagedAt < 0) g.engagedAt = win;
        if (r.prev == FGDR_ON && r.state != FGDR_ON && g.downAt < 0) { g.downAt = win; g.downRow = r.row; g.downX10 = r.rX10; }
    }
    g.cRoomUs = L.mem.cRoomUs; g.cLimUs = L.mem.cLimUs;
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("FX14");
    return g;
}
static void Fx14Say(const char* what, const Fx14Leg& g)
{
    LwPrint(what, g.w);
    printf("  engaged at window %d, stood down at window %d (%s at R_on %.1f); c with room %d us, on a card that was the limit %d us; %d of %d real frames shown once, out of place %d\n",
        g.engagedAt, g.downAt, g.downAt >= 0 ? FgCondEnumName(g.downRow) : "none", g.downX10 / 10.0, g.cRoomUs, g.cLimUs, g.real1, g.n, g.order);
}
static void CaseFX14()
{
    const char* const name = "FX14 the morning's fixed120 leg in the development build's window order: the held half refresh stands, c never the pacer's hold (2026-10-18, phase 5 round 2, S2)";
    CaseBegin(name);
    const Fx14Leg a = Fx14Run(0);
    Fx14Say("THE FIXED 120 Hz SHAPE, A GAME OF 10.25 ms, THE PRODUCT'S READING, THE DEVELOPMENT BUILD'S ORDER:", a);
    if (a.engagedAt < 0 || a.engagedAt > 20) Err("FX14: engaged at window %d (within 20 wanted)", a.engagedAt);
    if (a.downAt >= 0) Err("FX14: stood down at window %d by %s at R_on %.1f (never wanted: the pacer holds half the refresh by design)", a.downAt, FgCondEnumName(a.downRow), a.downX10 / 10.0);
    if (a.cRoomUs > 300 || a.cLimUs > 300) Err("FX14: c filed %d us with room, %d us on a card that was the limit (300 at the most wanted: the pacer's hold is not frame generation's cost)", a.cRoomUs, a.cLimUs);
    if (a.order || a.real1 != a.n) Err("FX14: %d picture(s) out of place (%s), %d of %d real frames shown once", a.order, a.first.c_str(), a.real1, a.n);
    const RingStats stc = gRingSt;
    const Fx14Leg c = Fx14Run(1);
    ViolRestore(stc);
    Fx14Say("CONTROL, THE READING OF BEFORE (the want word), THE SAME ORDER:", c);
    const int cFiled = c.cRoomUs > c.cLimUs ? c.cRoomUs : c.cLimUs;
    if (c.downAt < 0 || c.downRow != FGC_SLOW || c.downX10 < 594 || cFiled < 5000) Err("FX14 control: the reading of before stood down at window %d by %s at %.1f and filed c %d us (FGC_SLOW at 59.4 or more and 5,000 us or more wanted, as the leg did): the control shows nothing", c.downAt, c.downAt >= 0 ? FgCondEnumName(c.downRow) : "none", c.downX10 / 10.0, cFiled);
    Ctl("the reading of before: stood down at window %d by %s at R_on %.1f, c filed %d us", c.downAt, c.downAt >= 0 ? FgCondEnumName(c.downRow) : "none", c.downX10 / 10.0, cFiled);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX15, THE PANEL'S OWN EVIDENCE AGAINST NVIDIA'S NO (phase 5 round 2, S2, 2026-10-18)
// WHAT THE LEG SHOWED (S3's reading of the fixed120 and fixed120-off PresentMon files): the screen was not fixed (95.6 and 98.4 of 100 display
// changes off the 120 Hz grid, Independent Flip: the panel followed the game), NVIDIA's per-program answer said fixed (vrr 0), and the mode
// ran; the flip classifier read FREE in 136 of 160 windows standing by. THE RULE (fg_cond.h, FgCondFixedScreen and FgCondFixLatchStep's
// freeMem): the classifier's memory read FREE where the mod's pacer did not hold the game is -1 to the table, and NVIDIA's no does not make the
// mode then: FGC_FIXED stands with its line. HELD: (1) the latch's FREE is taken standing by, kept while held, cleared by a new refresh, a Reset
// and NVIDIA's yes; (2) the table with NVIDIA's no, vsync on, fg_fixed 1: no memory, the mode; FREE, FGC_FIXED with the line naming the grid,
// the mode off; QUANTISED, the mode; (3) fg_fixed 0 reads FREE exactly as no memory (the rows of every other build unchanged). CONTROL, THE
// TABLE OF BEFORE (FREE handed as 0, no evidence): the mode runs on the morning's panel.
static void CaseFX15()
{
    const char* const name = "FX15 the panel's own evidence: the classifier's memory read FREE standing by keeps the fixed mode off against NVIDIA's no (2026-10-18, phase 5 round 2, S2)";
    CaseBegin(name);
    FgFixLatch L = {};
    const unsigned long long k1 = 0x0000007800000001ull;
    struct Step { int mem, held, nvYes; unsigned long long key; int wantFree; const char* what; };
    const Step steps[] =
    {
        { 0, 0, 0, k1, 0, "standing by, no memory yet" },
        { 1, 0, 0, k1, 1, "standing by, FREE" },
        { 2, 1, 0, k1, 1, "engaged, the memory QUANTISED: kept" },
        { 2, 0, 0, k1, 0, "standing by, QUANTISED" },
        { 1, 0, 0, k1, 1, "standing by, FREE again" },
        { 1, 0, 0, k1 ^ 2ull, 1, "a Reset: cleared, and this stand-by window takes FREE" },
        { 1, 1, 0, k1 ^ 6ull, 0, "a new refresh while held: cleared, the held window takes nothing" },
        { 1, 0, 1, k1 ^ 6ull, 0, "NVIDIA says variable: cleared, nothing taken" },
    };
    for (const Step& s : steps)
    {
        FgCondFixLatchStep(L, s.mem, s.held, s.key, s.nvYes);
        printf("  %-62s -> FREE %d\n", s.what, L.freeMem);
        if (L.freeMem != s.wantFree) Err("FX15: %s: FREE %d (%d wanted)", s.what, L.freeMem, s.wantFree);
    }
    const FgCondK k = FgCondDefaults();
    struct Tab { int quant, key, wantMode, wantRow; const char* what; };
    const Tab tabs[] =
    {
        { 0, 1, 1, FGC_OK, "NVIDIA's no, no memory, fg_fixed 1" },
        { -1, 1, 0, FGC_FIXED, "NVIDIA's no, the memory FREE, fg_fixed 1 (the morning's panel)" },
        { 1, 1, 1, FGC_OK, "NVIDIA's no, the memory QUANTISED, fg_fixed 1" },
        { 0, 0, 0, FGC_FIXED, "NVIDIA's no, no memory, fg_fixed 0" },
        { -1, 0, 0, FGC_FIXED, "NVIDIA's no, the memory FREE, fg_fixed 0" },
    };
    for (const Tab& t : tabs)
    {
        // (review round 2: THROUGH THE MEMORY'S OWN PATH, as fg_arm_life.h's FgpCondRead hands it: the classifier's memory into the latch on two
        // standing-by windows, then the latch's answer read by fg_cond.h's FgCondFixQuantIn; never a -1 typed in)
        FgFixLatch M = {};
        const int mem = t.quant > 0 ? 2 : t.quant < 0 ? 1 : 0;
        FgCondFixLatchStep(M, mem, 0, k1, 0); FgCondFixLatchStep(M, mem, 0, k1, 0);
        FgCondIn in = FxSession(0, FgCondFixQuantIn(M), 1, t.key, 120); in.realFpsX10 = 975;
        if (in.flipQuant != t.quant) Err("FX15: %s: the latch handed %d (%d wanted)", t.what, in.flipQuant, t.quant);
        const int c = FgCondFirst(in, k), mode = FgCondFixedMode(in) ? 1 : 0;
        const std::string line = c ? FxCondLine(c, in, k) : std::string("(none)");
        printf("  %-66s: the mode %s, the row %s: %s\n", t.what, mode ? "ON" : "off", FgCondEnumName(c), line.c_str());
        if (mode != t.wantMode || c != t.wantRow) Err("FX15: %s: the mode %d, the row %s (%d and %s wanted)", t.what, mode, FgCondEnumName(c), t.wantMode, FgCondEnumName(t.wantRow));
        if (t.quant < 0 && t.key > 0 && line.find("read off the refresh grid") == std::string::npos) Err("FX15: %s: the line does not name the grid: %s", t.what, line.c_str());
        if (t.quant < 0 && t.key > 0 && (line.find("follow the game") == std::string::npos || line.find("Fast Sync") == std::string::npos)) Err("FX15: %s: the line does not name both causes (the display following the game, NVIDIA's Fast Sync): %s", t.what, line.c_str());   // (review round 2)
    }
    {   // (fg_fixed 0: FREE reads as no memory, row for row and line for line, at both NVIDIA answers that leave the memory a say)
        for (int vrr = -1; vrr <= 0; vrr++)
        {
            FgCondIn a = FxSession(vrr, -1, 1, 0, 120), b = FxSession(vrr, 0, 1, 0, 120); a.realFpsX10 = b.realFpsX10 = 975;
            const int ca = FgCondFirst(a, k), cb = FgCondFirst(b, k);
            if (ca != cb || FgCondMask(a, k) != FgCondMask(b, k) || FxCondLine(ca, a, k) != FxCondLine(cb, b, k)) Err("FX15: fg_fixed 0, NVIDIA %d: FREE reads %s, no memory %s (the same wanted)", vrr, FgCondEnumName(ca), FgCondEnumName(cb));
        }
    }
    {   // CONTROL, THE TABLE OF BEFORE: FREE handed as 0
        FgCondIn in = FxSession(0, 0, 1, 1, 120); in.realFpsX10 = 975;
        const int mode = FgCondFixedMode(in) ? 1 : 0;
        if (!mode) Err("FX15 control: the table of before kept the mode off on the morning's panel (on wanted: the control shows nothing)");
        Ctl("the table of before: the mode %s on a panel that followed the game", mode ? "ON" : "off");
    }
    CaseEnd(name);
}

// ================================================================ ROUND 3 OF THE ROUND (2026-10-18, evening, G4)
// ---------------------------------------------------------------- FX16, THE EVENING'S fixed120 LEG: R_free UNDER THE FIXED MODE'S HOLD
// WHAT THE LEG SHOWED (docs/reports/fg-5/sitting-2026-10-18b, F1C-fixed120: 1455 MHz, G-Sync off, the screen fixed, the game 92 to 104 by
// itself): THE MODE ENGAGED TWICE, behind windows 7 and 84, at 93.4 and 96.7 by itself; it held 60 real and 120 of 120 shown; and FGC_FAST
// stood it down behind its warm-up each time (behind windows 11 and 88) with R_free 293.7 and 141.7, over the fixed mode's top (116 standing
// by, 119 engaged). THE CAUSE (fg_cond.h, FgCondFreeX10): under the hold R_free was the rate of the law's median sample, and a sample is the
// render thread's own time outside the original Present. PaceFixed holds every real frame to two refreshes (16.67 ms) against the game's own
// 10.3, and in the 6 to 13 ms the pacer sleeps, the rest of the game's frame (its update thread, the worker's replay, the card) finishes, so
// the sample is the render thread's part alone: 3.4 to 3.5 ms in the first stint (286.6, 285.7, 289.3, 293.7), 9.8 to 7.1 in the second
// (102.2, 105.6, 119.5, 141.7). (The same reader read 180 to 300 at the variable panel's floor on fill95, against the game's own 95 to 108.)
// THE CASE: (1) THE LEG'S OWN LINES, window by window (the law's window line's R_on and R_free, the fixed mode's line's period, the engage
// window's R_on as the game's own), through the reader and the conditions' FAST row on the fixed session; (2) THE SIMULATION WITH A STAGE THAT
// RUNS IN THE PACER'S SLEEP (dt_fg_law.h, LwStages: the game's own frame 10.3 ms with 5 percent of spread, of which the render thread's own
// part 3.5 ms with 10 percent), FX14's shape and window order, 60 windows. HELD: (1) FAST at none of the eight windows; (2) engaged within 20 windows, never
// stood down, ON at the end; every real frame once and in order. CONTROL, THE READER OF BEFORE: (1) the leg's own R_free back within a tenth
// at all eight, and FAST at the six whose band the leg's line read FAST (8 to 11, 87, 88); (2) a stand-down by FGC_FAST.
struct Fx16Leg { std::vector<LwRec> w; int engagedAt, downAt, downRow, endState, order, n, real1; double sampleMs; std::string first; };
static Fx16Leg Fx16Run(int before)
{
    Fx16Leg g = {}; g.engagedAt = g.downAt = -1; g.downRow = FGC_OK;
    RtPar par = FxPar(120.0, kFx120, 1);
    RtBegin(par);
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), FgCondDefaults()); L.card = 0; L.cardEngaged = 0; L.devPass = 1;
    gRtSeed = 20261018u; gLwFreeBefore = before;
    int overRun = 0; gFxEntryX10 = 0; gFxWasEng = 0; gFxEntry = FgFixEntry(); gFxDipRun = 0;
    LwStages st = {};
    std::vector<double> samples;
    for (int win = 0; win < 60; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            const double c = LwStageCostMs(st, 10.3 * RtSpread(0.05), 3.5 * RtSpread(0.10));
            if (L.mem.state == FGDR_ON) samples.push_back(c);
            RtFrame(RtMs(c)); LwLawPoll(L);
        }
        const LwRec r = FxLawWindow(L, par, first, (int)gRt.pairs.size(), overRun, true);
        g.w.push_back(r);
        if (r.state == FGDR_ON && g.engagedAt < 0) g.engagedAt = win;
        if (r.prev == FGDR_ON && r.state != FGDR_ON && g.downAt < 0) { g.downAt = win; g.downRow = r.row; }
    }
    gLwFreeBefore = 0;
    g.endState = L.mem.state;
    if (!samples.empty()) { std::nth_element(samples.begin(), samples.begin() + samples.size() / 2, samples.end()); g.sampleMs = samples[samples.size() / 2]; }
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("FX16");
    return g;
}
static void CaseFX16()
{
    const char* const name = "FX16 the evening's fixed120 leg: under the fixed mode's hold R_free is the game's own rate, never the render thread's part alone (2026-10-18, round 3, G4)";
    CaseBegin(name);
    const FgCondK k = FgCondDefaults();
    // (1) THE LEG'S OWN LINES: window, R_on, R_free as the leg's line said it, the fixed mode's period, the game's own rate at the engage
    // (and the band the leg's line said: FAST at 1, IN at 0)
    struct W { int win, onX10, freeX10, perUs, aloneX10, fast; };
    static const W leg[] = { { 8, 599, 2866, 16661, 934, 1 }, { 9, 600, 2857, 16678, 934, 1 }, { 10, 600, 2893, 16673, 934, 1 }, { 11, 598, 2937, 16673, 934, 1 },
                             { 85, 602, 1022, 16668, 967, 0 }, { 86, 601, 1056, 16701, 967, 0 }, { 87, 597, 1195, 16681, 967, 1 }, { 88, 601, 1417, 16681, 967, 1 } };
    int fastNow = 0, fastBefore = 0, repro = 0;
    for (const W& x : leg)
    {
        FgCondIn in = FxSession(0, 0, 1, 1, 120); in.engaged = 1; in.realFpsX10 = x.onX10;
        const int medUs = (int)(10000000LL / x.freeX10), floorUs = FgCondFloorUs(in);
        const double medMs = 1000.0 / (x.onX10 / 10.0);
        const int before = LwFreeBeforeX10(medUs, x.perUs, floorUs, medMs);
        const int now = LW_PRODUCT_FREE(medUs, x.perUs, floorUs, medMs, x.aloneX10);
        in.freeFpsX10 = before; const bool fb = FgCondHolds(FGC_FAST, in, k);
        if (before < x.freeX10 - 1 || before > x.freeX10 + 1 || (fb ? 1 : 0) != x.fast) repro++;
        in.freeFpsX10 = now; const bool fn = FgCondHolds(FGC_FAST, in, k);
        fastBefore += fb ? 1 : 0; fastNow += fn ? 1 : 0;
        printf("  the leg's window %3d: R_on %5.1f, the law's median sample %5d us, the period %d us (the floor %d): R_free %5.1f (the reader of before %5.1f, the leg %5.1f); FAST %s (before: %s)\n",
            x.win, x.onX10 / 10.0, medUs, x.perUs, floorUs, now / 10.0, before / 10.0, x.freeX10 / 10.0, fn ? "STANDS" : "no", fb ? "STANDS" : "no");
    }
    if (repro) Err("FX16: the reader of before gave the leg's R_free and band back at %d of 8 windows (8 wanted: the case does not reproduce the leg)", 8 - repro);
    if (fastNow) Err("FX16: on the leg's own lines FAST stands at %d of 8 windows (none wanted: the game ran 93 to 97 by itself, under the top of 116 and 3)", fastNow);
    if (fastBefore != 6) Err("FX16 control: the reader of before stood FAST at %d of the leg's 8 windows (6 wanted, as the leg's band: the control shows nothing)", fastBefore);
    Ctl("the reader of before on the leg's lines: FAST at %d of 8 windows, as the leg's band", fastBefore);
    // (2) THE SIMULATION with the stage that runs in the pacer's sleep
    const Fx16Leg a = Fx16Run(0);
    LwPrint("THE FIXED 120 Hz SHAPE, THE GAME 10.3 ms OF WHICH THE RENDER THREAD'S OWN 3.5, THE PRODUCT'S READER:", a.w);
    printf("  engaged at window %d, stood down at window %d (%s), %s at the end; the law's samples engaged, median %.2f ms; %d of %d real frames shown once, out of place %d\n",
        a.engagedAt, a.downAt, FgCondEnumName(a.downRow), FgDriveStateName(a.endState), a.sampleMs, a.real1, a.n, a.order);
    if (a.engagedAt < 0 || a.engagedAt > 20) Err("FX16: engaged at window %d (within 20 wanted)", a.engagedAt);
    if (a.downAt >= 0 || a.endState != FGDR_ON) Err("FX16: stood down at window %d by %s, %s at the end (never, and ON, wanted)", a.downAt, FgCondEnumName(a.downRow), FgDriveStateName(a.endState));
    if (a.order || a.real1 != a.n) Err("FX16: %d picture(s) out of place (%s), %d of %d real frames shown once", a.order, a.first.c_str(), a.real1, a.n);
    const RingStats stc = gRingSt;
    const Fx16Leg c = Fx16Run(1);
    ViolRestore(stc);
    LwPrint("CONTROL, THE READER OF BEFORE:", c.w);
    printf("  engaged at window %d, stood down at window %d (%s); the law's samples engaged, median %.2f ms\n", c.engagedAt, c.downAt, FgCondEnumName(c.downRow), c.sampleMs);
    if (c.downAt < 0 || c.downRow != FGC_FAST) Err("FX16 control: the reader of before stood down at window %d by %s (by FGC_FAST wanted, as the leg: the control shows nothing)", c.downAt, FgCondEnumName(c.downRow));
    Ctl("the reader of before in the simulation: stood down at window %d by %s", c.downAt, FgCondEnumName(c.downRow));
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX20, THE PLAYER'S fg_min_fps HOLDS ON A VARIABLE DISPLAY
// (2026-10-24, round 6, F; the merge's reviewer R6.) The floor on a variable panel is held under the top less its hysteresis, so with
// fg_min_fps=40 on a VRR 60 Hz panel a game of 32 engaged and the doubling ran it at C, about 29.6 real, under the player's own setting, while
// a fixed screen stood by (FgCondFixedUnderFloor). NOW where the key is set and the doubling's rate C is under it, FGC_SLOW, with its line.
// ROWS (standing by and engaged): VRR 60 with fg_min_fps 40, the game at 32: FGC_SLOW and the line names fg_min_fps; VRR 120 with fg_min_fps
// 40, the game at 50: no row (C 59.1 keeps it). FAILS ON 234c4d85 (no such row: the first engaged at C, built and run there). CONTROL: VRR
// 60 with no key, the game at 32: no row (the floor by display, 30, and C 29.6 is the design).
static void CaseFX20()
{
    const char* const name = "FX20 the player's fg_min_fps holds on a variable display: where the doubling's C is under it, the mode stands by (2026-10-24, round 6, F)";
    CaseBegin(name);
    struct Row { const char* what; int hz, key, real, eng, want; };
    static const Row rows[] = { { "VRR 60, fg_min_fps 40, 32 by itself", 60, 40, 320, 0, FGC_SLOW }, { "VRR 60, fg_min_fps 40, engaged at 32", 60, 40, 296, 1, FGC_SLOW },
                                { "VRR 120, fg_min_fps 40, 50 by itself", 120, 40, 500, 0, FGC_OK } };
    int bad = 0;
    for (const Row& r : rows)
    {
        FgCondK k = FgCondDefaults(); FgCondFloorKey(k, r.key);
        FgCondIn in = FxSession(1, 0, 1, 0, r.hz); in.realFpsX10 = r.real; in.engaged = r.eng; if (r.eng) in.freeFpsX10 = 320;
        const int c = FgCondFirst(in, k);
        const std::string ln = c ? FxCondLine(c, in, k) : std::string();
        printf("  %-40s -> %-9s %s\n", r.what, FgCondEnumName(c), ln.c_str());
        if (c != r.want || (c == FGC_SLOW && ln.find("fg_min_fps") == std::string::npos)) { bad++; Err("FX20: %s: %s (%s wanted, its line naming fg_min_fps)", r.what, FgCondEnumName(c), FgCondEnumName(r.want)); }
    }
    {
        const FgCondK k = FgCondDefaults();
        FgCondIn in = FxSession(1, 0, 1, 0, 60); in.realFpsX10 = 320;
        const int c = FgCondFirst(in, k);
        printf("  CONTROL, VRR 60 with no key, 32 by itself -> %s\n", FgCondEnumName(c));
        if (c != FGC_OK) Err("FX20 control: VRR 60 with no key read %s (FGC_OK wanted: the floor by display, 30)", FgCondEnumName(c));
        else Ctl("VRR 60 with no key: no row, the doubling at C");
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX21, THE OVERRUN BY THE BULK, NEVER THE STUTTER TAIL (round 7, F1)
// (2026-10-24, round 7, F; the round 6 sitting: fill120 and weak60 stood the mode down on GTA IV's ordinary spikes, fg_cond.h's
// FgCondFixShareStep says the rule and what the legs read under it.) THE LAW ITSELF over the fixed shape (FxLawWindow, as FX8), five minutes
// each, the overrun stepped once a window from the machine's own law (FxOverStep):
//   (a) GTA IV'S SPIKES AT 120 Hz: a game of 10 ms (5 of 100 of spread), 2 frames in 100 at 17.5 to 23.5 ms (over two refreshes, under one and a
//       half periods, so the law keeps them), and every 30 s A STREAMING BURST of 1 s at 19 ms;
//   (b) WEAK60's AT 60 Hz: a game of 18 ms, 2 in 100 at 35 to 48 ms, and every 30 s a burst of 1 s at 38 ms;
// HELD: each engages and NEVER STANDS DOWN in the five minutes, every real frame once and in order. FAILS ON c76d58c1: ROUND 6's RULE (gFxR6,
// the control: L over the line two windows running) stands each down. AND WHAT MUST STILL STAND DOWN: (c) A GAME UNIFORMLY OVER THE PERIOD: 12
// ms for 8 windows (engaged), then 17.5 ms every frame at 120 Hz: down within 6 windows of the change (FGC_SLOW, by the held rate and the
// share); FX8's 16.4 ms game (FX8 itself).
struct Fx21Leg { int engagedAt, downs, turns, order, n, real1, maxShare, downAt; std::string first; };
static double Fx21Cost(int kind, int frame, double tMs)
{
    // (a spike: frames whose index hashes into 2 of 100, deterministic; a burst: the first second of every 30)
    const unsigned h = (unsigned)frame * 2654435761u;
    const bool spike = ((h >> 16) % 100u) < 2u;
    const double u = (double)((h >> 8) & 0xFFu) / 255.0;
    const bool burst = kind < 2 && fmod(tMs, 30000.0) < 1000.0 && tMs > 20000.0;
    if (kind == 0) return burst ? 19.0 : spike ? 17.5 + 6.0 * u : 10.0 * RtSpread(0.05);
    if (kind == 1) return burst ? 38.0 : spike ? 35.0 + 13.0 * u : 18.0 * RtSpread(0.05);
    return tMs < 16000.0 ? 12.0 * RtSpread(0.05) : 17.5 * RtSpread(0.02);
}
static Fx21Leg Fx21Run(int kind, int windows, bool r6)
{
    Fx21Leg g = {}; g.engagedAt = g.downAt = -1; g.maxShare = -1;
    const double hz = kind == 1 ? 60.0 : 120.0;
    RtPar par = FxPar(hz, ((ULONGLONG)(ULONG)hz << 32) | 1ull, 1);
    RtBegin(par);
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;
    LwLaw L; LwLawBegin(L, k, FgCondDefaults());
    gRtSeed = 20261024u;
    int overRun = 0; gFxEntryX10 = 0; gFxWasEng = 0; gFxEntry = FgFixEntry(); gFxDipRun = 0; gFxR6 = r6 ? 1 : 0;
    const LONGLONG t0 = B::clockNow;
    int frame = 0;
    for (int win = 0; win < windows; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { const double tMs = (double)(B::clockNow - t0) * 1000.0 / (double)B::Qpf(); RtFrame(RtMs(Fx21Cost(kind, frame++, tMs))); LwLawPoll(L); }
        const LwRec r = FxLawWindow(L, par, first, (int)gRt.pairs.size(), overRun, true);
        if (gFxSharePm > g.maxShare) g.maxShare = gFxSharePm;
        if (r.state == FGDR_ON && g.engagedAt < 0) g.engagedAt = win;
        if (r.prev == FGDR_ON && r.state != FGDR_ON)
        {
            g.downs++; if (g.downAt < 0) g.downAt = win;
            printf("    (%s, %s) window %d: down by %s (R_on %.1f, R_free %.1f, the share %d of 1000, E %ld us, L %ld us, the game's median %.2f ms)\n", kind == 0 ? "a" : kind == 1 ? "b" : "c", r6 ? "round 6's rule" : "round 7's",
                   win, FgCondEnumName(r.row), r.rX10 / 10.0, r.rFree / 10.0, gFxSharePm, (long)B::S().r.lawEUs, (long)B::S().r.lawLUs, r.game);
        }
    }
    gFxR6 = 0;
    g.turns = (int)L.turnsMs.size();
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    B::S().wt.want = FG_WANT_ON;
    RtEnd("FX21");
    return g;
}
static void CaseFX21()
{
    const char* const name = "FX21 the fixed mode's overrun by the bulk: GTA IV's spikes and bursts at 120 and 60 Hz hold the doubling five minutes, a game uniformly over the period stands down (2026-10-24, round 7, F)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const Fx21Leg a = Fx21Run(0, 150, false), b = Fx21Run(1, 75, false), c = Fx21Run(2, 20, false);
    const Fx21Leg ac = Fx21Run(0, 150, true), bc = Fx21Run(1, 75, true);
    ViolRestore(stc);
    printf("  (a) GTA IV's spikes at 120 Hz: engaged at window %d, %d stand-down(s) in 150 windows, the 12 s share at the most %d of 1000; CONTROL, round 6's rule: %d stand-down(s) (the first at window %d)\n", a.engagedAt, a.downs, a.maxShare, ac.downs, ac.downAt);
    printf("  (b) weak60's at 60 Hz: engaged at window %d, %d stand-down(s) in 75 windows, the share at the most %d of 1000; CONTROL, round 6's rule: %d stand-down(s) (the first at window %d)\n", b.engagedAt, b.downs, b.maxShare, bc.downs, bc.downAt);
    printf("  (c) uniformly over the period (12 ms, then 17.5 ms from window 8): engaged at window %d, stood down at window %d, the share at the most %d of 1000\n", c.engagedAt, c.downAt, c.maxShare);
    if (a.engagedAt < 0 || a.downs) Err("FX21 (a): engaged at window %d, %d stand-down(s) (engaged and none wanted)", a.engagedAt, a.downs);
    if (b.engagedAt < 0 || b.downs) Err("FX21 (b): engaged at window %d, %d stand-down(s) (engaged and none wanted)", b.engagedAt, b.downs);
    if (c.engagedAt < 0 || c.engagedAt >= 8 || c.downAt < 8 || c.downAt > 8 + 6) Err("FX21 (c): engaged at window %d, stood down at %d (engaged before 8, down within 6 windows of 8 wanted)", c.engagedAt, c.downAt);
    for (const Fx21Leg* l : { &a, &b, &c }) if (l->order || l->real1 != l->n) Err("FX21: %d picture(s) out of place (%s), %d of %d real frames shown once", l->order, l->first.c_str(), l->real1, l->n);
    if (ac.downs < 1 || bc.downs < 1) Err("FX21 control: round 6's rule stood (a) down %d time(s) and (b) %d (at least once each wanted: the case cannot see the spikes)", ac.downs, bc.downs);
    else Ctl("round 6's rule: (a) %d stand-down(s), (b) %d", ac.downs, bc.downs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX22, 165 Hz AND UP ON THE DOUBLING (round 7, F4)
// (2026-10-24, round 7, F; FG-POLICY-AND-PLAN-1.1.md section 1: "165 to 240 Hz screens: THE DOUBLING".) THE LAW ITSELF over the machine's fake
// (LwLawWindow on a variable panel, FxLawWindow on the fixed one), 40 windows each, the fill not available (it is not on VRR, and fixed 240 is
// past its shapes): VRR 165 at 70 (C 81.3, the top 92.2), VRR 180 at 80 (C 88.7, the top 100.5), VRR 240 at 100 (C 118.2, the top 134.0), and
// FIXED 240 at 125 (held to 120, the way in 123.6). HELD: each engages once and stays to the end (the switch bounded: one engage, no
// stand-down), every real frame once and in order, every pair's generated frame shown (none lost to a freeze or a hold), the pairs flowing to
// the end. WHAT THE MACHINE CANNOT HOLD AND HOW THE LAW KEEPS IT OUT, judged: VRR 240 at 140 (over its top: a pair's half under 4 ms, the
// generated frame's dispatch and present no longer fit) never engages, FGC_FAST, native; FIXED 240 at 110 (under the way in: frames over two
// refreshes of 8.33 ms) never engages, FGC_SLOW. CONTROL: the same 240 Hz rows with the table's top raised to 200 (fg_law_fast, the development
// key): VRR 240 at 140 engages, so the row is what keeps it out. THE HARNESS PROVES LOGIC AND SAFETY; the pacing on such panels is a leg's.
struct Fx22Leg { int engagedAt, turns, downs, lastRow; FxRead o; int pairs; };
static Fx22Leg Fx22Run(double hz, int vrr, double costMs, int windows, int topFps = 0)
{
    Fx22Leg g = {}; g.engagedAt = -1;
    const ULONGLONG packed = ((ULONGLONG)(ULONG)hz << 32) | 1ull;
    RtPar par = FxPar(hz, packed, vrr ? 0 : 1);
    if (vrr) { par.panelVrr = 1; par.fifo = 0; }
    RtBegin(par);
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;
    FgCondK ck = FgCondDefaults(); if (topFps > 0) FgCondFastKey(ck, topFps);
    LwLaw L; LwLawBegin(L, k, ck);
    gRtSeed = 20261024u;
    int overRun = 0; gFxEntryX10 = 0; gFxWasEng = 0; gFxEntry = FgFixEntry(); gFxDipRun = 0;
    int engagedFrom = -1;
    for (int win = 0; win < windows; win++)
    {
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++) { RtFrame(RtMs(costMs * RtSpread(0.05))); LwLawPoll(L); }
        const LwRec r = vrr ? LwLawWindow(L, par, first, (int)gRt.pairs.size()) : FxLawWindow(L, par, first, (int)gRt.pairs.size(), overRun, true);
        if (r.state == FGDR_ON && g.engagedAt < 0) { g.engagedAt = win; engagedFrom = (int)gRt.pairs.size(); }
        if (r.prev == FGDR_ON && r.state != FGDR_ON) g.downs++;
        g.lastRow = r.row;
    }
    g.turns = (int)L.turnsMs.size();
    g.pairs = (int)gRt.pairs.size();
    g.o = FxReadRun(engagedFrom >= 0 ? engagedFrom + 120 : 0);
    B::S().wt.want = FG_WANT_ON;
    RtEnd("FX22");
    return g;
}
static void CaseFX22()
{
    const char* const name = "FX22 165 to 240 Hz on the doubling: VRR 165, 180 and 240 and fixed 240 hold, what the machine cannot hold the law keeps out (2026-10-24, round 7, F)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    struct Row { const char* what; double hz; int vrr; double costMs; };
    static const Row rows[] = { { "VRR 165 at 70", 165.0, 1, 1000.0 / 70.0 }, { "VRR 180 at 80", 180.0, 1, 1000.0 / 80.0 }, { "VRR 240 at 100", 240.0, 1, 10.0 }, { "fixed 240 at 125", 240.0, 0, 8.0 } };
    for (const Row& x : rows)
    {
        const Fx22Leg g = Fx22Run(x.hz, x.vrr, x.costMs, 40);
        printf("  %-18s engaged at window %d, %d engage(s) and stand-down(s), %d stand-down(s); from a window behind the engage: %s\n", x.what, g.engagedAt, g.turns, g.downs, FxLine(g.o));
        if (g.engagedAt < 0 || g.engagedAt > 10 || g.downs || g.turns != 1) Err("FX22: %s: engaged at window %d, %d turn(s), %d stand-down(s) (engaged by window 10, once, never down wanted)", x.what, g.engagedAt, g.turns, g.downs);
        if (g.o.realTwice || g.o.never || g.o.genTwice || g.o.order || g.o.dropped || g.o.gen != g.o.n || g.o.n < 3000)
            Err("FX22: %s: %d pairs, real frames twice %d, never %d, generated shown %d (twice %d), out of place %d (%s), dropped %ld (3,000 pairs or more, every one once and in order wanted)", x.what, g.o.n, g.o.realTwice, g.o.never, g.o.gen, g.o.genTwice, g.o.order, g.o.first.c_str(), g.o.dropped);
    }
    const Fx22Leg fast = Fx22Run(240.0, 1, 1000.0 / 140.0, 20), slow = Fx22Run(240.0, 0, 1000.0 / 110.0, 20);
    printf("  VRR 240 at 140: engaged at window %d, the row %s; fixed 240 at 110: engaged at window %d, the row %s\n", fast.engagedAt, FgCondEnumName(fast.lastRow), slow.engagedAt, FgCondEnumName(slow.lastRow));
    if (fast.engagedAt >= 0 || fast.lastRow != FGC_FAST) Err("FX22: VRR 240 at 140 engaged at %d, the row %s (never, FGC_FAST wanted)", fast.engagedAt, FgCondEnumName(fast.lastRow));
    if (slow.engagedAt >= 0 || slow.lastRow != FGC_SLOW) Err("FX22: fixed 240 at 110 engaged at %d, the row %s (never, FGC_SLOW wanted)", slow.engagedAt, FgCondEnumName(slow.lastRow));
    const Fx22Leg ctl = Fx22Run(240.0, 1, 1000.0 / 140.0, 20, 200);
    ViolRestore(stc);
    printf("  CONTROL, the top raised to 200: VRR 240 at 140 engaged at window %d\n", ctl.engagedAt);
    if (ctl.engagedAt < 0) Err("FX22 control: with the top at 200 VRR 240 at 140 never engaged (an engage wanted: the row is what keeps it out)");
    else Ctl("the top at 200: VRR 240 at 140 engaged at window %d", ctl.engagedAt);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX23, A LASTING DIP OF THE HELD RATE STANDS THE MODE DOWN (round 7 review, F1)
// (2026-10-24, round 7, F; the merge's reviewer, finding 1.) FgCondFixHeldDipStands read R_free in every window, so a held rate under 99 of 100
// of half the refresh with the game's own rate over 103 of 100 never stood the mode down: a card-limited game whose wait lands inside DXVK's
// Present, or whose overruns are hitches past one and a half periods (which the share leaves out), would repeat refreshes for the session. NOW
// (FgCondFixDipStep, FgCondFixHeldDipStands): the dip's first window never, its second where R_free is under the way in, FROM ITS THIRD WHATEVER
// R_free SAYS. THE CASE, the windows through FgCondFixDipStep and FgCondHolds(FGC_SLOW), engaged in the fixed mode at 120 Hz, the overrun not
// over: (a) R_on 59.2 and R_free 70.0 for six windows: FGC_SLOW first at the third; (b) R_on 59.2 and R_free 61.0 (under 61.8): at the second.
// CONTROL: round 6's dips, one window long (fill120's windows 103 to 106: R_on 59.9, 60.1, 59.2, 60.0, R_free 69.8; weak60's 34 to 36: 29.9,
// 30.0, 30.1 at 60 Hz, R_free 54.6): no FGC_SLOW.
struct Fx23W { int ron, rfree; };
static int Fx23Run(int hz, const Fx23W* w, int n)
{
    const FgCondK k = FgCondDefaults();
    int dipRun = 0;
    for (int i = 0; i < n; i++)
    {
        FgCondIn in = FxSession(0, 0, 1, 1, hz);
        in.engaged = 1; in.realFpsX10 = w[i].ron; in.freeFpsX10 = w[i].rfree; in.fixSharePm = 0; in.fixOver = 0;
        in.fixDipFirst = FgCondFixDipStep(dipRun, in);
        if (FgCondHolds(FGC_SLOW, in, k)) return i + 1;
    }
    return -1;
}
static void CaseFX23()
{
    const char* const name = "FX23 a lasting dip of the held rate stands the fixed mode down at its third window whatever R_free; round 6's one-window dips hold (2026-10-24, round 7 review, F)";
    CaseBegin(name);
    static const Fx23W a[] = { { 592, 700 }, { 592, 700 }, { 592, 700 }, { 592, 700 }, { 592, 700 }, { 592, 700 } };
    static const Fx23W b[] = { { 592, 610 }, { 592, 610 }, { 592, 610 } };
    static const Fx23W c120[] = { { 599, 698 }, { 601, 698 }, { 592, 698 }, { 600, 698 } };
    static const Fx23W c60[] = { { 299, 546 }, { 300, 546 }, { 301, 546 } };
    const int ra = Fx23Run(120, a, 6), rb = Fx23Run(120, b, 3), rc = Fx23Run(120, c120, 4), rd = Fx23Run(60, c60, 3);
    printf("  (a) R_on 59.2, R_free 70.0 lasting: FGC_SLOW at window %d; (b) R_free 61.0: at window %d; CONTROL, round 6's one-window dips: fill120 %d, weak60 %d (-1: none)\n", ra, rb, rc, rd);
    if (ra != FG_COND_FIX_DIP_LONG_WIN) Err("FX23 (a): a lasting dip with R_free 70.0 stood the mode down at window %d (%d wanted)", ra, (int)FG_COND_FIX_DIP_LONG_WIN);
    if (rb != 2) Err("FX23 (b): a dip with R_free 61.0 stood the mode down at window %d (2 wanted)", rb);
    if (rc >= 0 || rd >= 0) Err("FX23 control: round 6's one-window dips stood the mode down (fill120 at %d, weak60 at %d; never wanted)", rc, rd);
    else Ctl("round 6's one-window dips: no stand-down at fill120's 103 to 106 or weak60's 34 to 36");
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX24, THE CARD'S WAIT ON THE FIRES' PRESENTS (2026-10-25, the last optimisation's A)
// (docs/reports/fg-5/opt-2026-10-25/PLAN.md, section A.) Round 7's fixed120 and weak60 legs ran the correction at its cap of +3,000 ppm (a
// refresh shown twice every 666 frames) because the block it read carried DXVK's frame-latency wait for the card's own work on the fires'
// presents, which the panel model did not have: its FIFO hold alone. THE ARM (RtPar::cardWaitRealUs): a fire's present returns no sooner
// than the legs' wait behind its call, by its kind, jittered 1.5 ms either way. TWO SHAPES, the legs': (A) fixed120 at 1680 MHz, the panel
// 120.000 read 120, a game of 90, the real frame's present held 4.4 ms and the generated frame's not at all; (B) weak60 at 300 MHz, the
// panel 60.000 read 60, a game of 55, the real frame's 14 ms and the generated frame's 8.5 ms (both carry it). EACH AT FOUR SETTINGS OF THE
// GUARD: THE BASE (bits 10 and 11 off, PaceFixed as it was: every present read, no rule), THE SPLIT ALONE (bit 11 off: the generated
// frames' presents read), THE RULE ALONE (bit 10 off: every present read, the card's wait read at the cap) and BOTH (the build).
// JUDGED: the base must show the fault on both shapes (the correction at 2,000 ppm or more at the end on (A), where the median of the two
// kinds' blocks still climbs it, at 2,900 or more on (B), and a refresh shown twice behind the run-in: else the arm sees nothing); THE BUILD
// on both: FQ4's and the panel's assertions over the stretch behind the run-in (no refresh
// shown twice there), the correction within 200 ppm of 0 at the end; on (A) the block a present's median at its target (250 to 750 us) and
// the rule never reading the card (the split holds it), on (B) the rule reading the card at the end. TOLD: the split alone and the rule
// alone, and the whole run's repeats (the rule's two windows at the cap come first on (B)).
struct Fx24Shape { const char* what; double hz; ULONGLONG read; double costMs; LONG realUs, genUs; int frames, from; };
static FxRead Fx24Run(const Fx24Shape& sh, LONG guardOff)
{
    RtPar p = FxPar(sh.hz, sh.read, 1);
    p.cardWaitRealUs = sh.realUs; p.cardWaitGenUs = sh.genUs; p.cardWaitJitUs = 1500; p.guardOff = guardOff;
    return FxRun(p, sh.costMs, sh.frames, sh.from, 20261025u);
}
static void CaseFX24()
{
    const char* const name = "FX24 the card's wait on the fires' presents: the base runs to the cap and repeats; the split and the card's rule hold within 200 ppm, no repeats (2026-10-25, opt A)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    static const Fx24Shape shapes[2] = {
        { "(A) fixed120 at 1680 MHz: the real frame's present held 4.4 ms, the generated frame's not", 120.0, kFx120, 11.1, 4400, 0, 2401, 1800 },
        { "(B) weak60 at 300 MHz: the real frame's present held 14 ms, the generated frame's 8.5 ms", 60.0, (60ull << 32) | 1ull, 18.0, 14000, 8500, 1801, 1200 } };
    static const LONG kOff[4] = { (1 << 10) | (1 << 11), 1 << 11, 1 << 10, 0 };
    static const char* const kWhat[4] = { "THE BASE (bits 10 and 11 off)", "the split alone (bit 11 off)", "the rule alone (bit 10 off)", "THE BUILD (both)" };
    FxRead o[2][4];
    for (int a = 0; a < 2; a++) for (int v = 0; v < 4; v++) o[a][v] = Fx24Run(shapes[a], kOff[v]);
    ViolRestore(stc);
    for (int a = 0; a < 2; a++)
    {
        printf("  %s\n", shapes[a].what);
        for (int v = 0; v < 4; v++)
            printf("    %s: the correction %+ld ppm after %ld windows, the block a present's median %ld us, the card's wait read %d; refreshes shown twice %d behind the run-in, %d over the whole run; the card's waits %ld (%.0f us on the mean)\n      %s\n",
                   kWhat[v], o[a][v].ppm, o[a][v].wins, o[a][v].medUs, o[a][v].card, o[a][v].repeats, o[a][v].repAll, o[a][v].cardWaits, o[a][v].cardWaitUs, FxLine(o[a][v]));
        const FxRead& b = o[a][0]; const FxRead& g = o[a][3];
        if (b.cardWaits < 100) Err("FX24 %c: the arm made %ld card's wait(s) on the base (100 or more wanted: the arm did not run)", 'A' + a, b.cardWaits);
        if (b.ppm < (a == 0 ? 2000 : 2900) || b.repeats < 1) Err("FX24 %c, the base: the correction %+ld ppm and %d refresh(es) shown twice behind the run-in (%d or more and one or more wanted: the arm does not show the legs' fault)", 'A' + a, b.ppm, b.repeats, a == 0 ? 2000 : 2900);
        else Ctl("%c, the base: the correction at %+ld ppm, %d refresh(es) shown twice behind the run-in", 'A' + a, b.ppm, b.repeats);
        const std::string bad = FxJudge(g);
        if (!bad.empty()) Err("FX24 %c, the build: %s", 'A' + a, bad.c_str());
        if (labs(g.ppm) > 200) Err("FX24 %c, the build: the correction %+ld ppm at the end (within 200 of 0 wanted)", 'A' + a, g.ppm);
        if (a == 0 && (g.medUs < 250 || g.medUs > 750 || g.card)) Err("FX24 A, the build: the block a present's median %ld us, the card's wait read %d (250 to 750 and 0 wanted: the generated frames' presents hold the FIFO's target)", g.medUs, g.card);
        if (a == 1 && !g.card) Err("FX24 B, the build: the card's wait not read at the end (read wanted: both kinds carry it)");
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX25, MANY RESTARTS ON A CARD THAT IS THE LIMIT: THE INTEGRAL DOES NOT RATCHET
// (2026-10-25, the review of e517d2dc, its first.) The integral is kept through a restart (bit 15), and each restart's run-in is followed by
// windows climbing to the cap from a block that is the card's, each teaching the integral about 50 ppm, before the card's wait is read:
// about +200 ppm a cycle, toward the cap over a session of engages and stalls. NOW (bit 16): at the first window read as the card's the
// integral is put back to what it was at the run-in's start or the last window within the learning band. THE CASE, FX24's shape (B)
// (weak60: both kinds carry the card's wait) with a stall of 100 ms planted every 2,400 presents (40 s; each a restart, the stalls'
// own gaps not counted as repeats), read 30 s behind the last: one stall, and four. JUDGED: four stalls leave the integral within 50 ppm
// of what one leaves, the card's wait read at the end, three or more restarts made. FAIL FIRST (bit 16 off): four leave it 300 ppm or
// more above one.
static FxRead Fx25Run(int stalls, LONG guardOff)
{
    RtPar p = FxPar(60.0, (60ull << 32) | 1ull, 1);
    p.cardWaitRealUs = 14000; p.cardWaitGenUs = 8500; p.cardWaitJitUs = 1500; p.guardOff = guardOff;
    p.panelStallAt = 2400; p.panelStallMs = 100; p.panelStallEvery = 2400;
    return FxRun(p, 18.0, 1200 * stalls + 900, 1200 * stalls, 20261025u);
}
static void CaseFX25()
{
    const char* const name = "FX25 many restarts on a card that is the limit: the integral is put back at the card's wait, so four stalls teach it no more than one (2026-10-25, the review of opt A)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const FxRead one = Fx25Run(1, 0), four = Fx25Run(4, 0), oneB = Fx25Run(1, 1 << 16), fourB = Fx25Run(4, 1 << 16);
    ViolRestore(stc);
    printf("  THE BUILD: one stall: the integral %+ld ppm, the correction %+ld, stalls restarted at %ld, the card's wait read %d; four: the integral %+ld, the correction %+ld, stalls %ld, read %d\n",
           one.iPpm, one.ppm, one.stalls, one.card, four.iPpm, four.ppm, four.stalls, four.card);
    printf("  FAIL FIRST, the integral kept (bit 16 off): one stall %+ld ppm (the correction %+ld), four %+ld (the correction %+ld), stalls %ld\n", oneB.iPpm, oneB.ppm, fourB.iPpm, fourB.ppm, fourB.stalls);
    if (four.stalls < 3) Err("FX25: four planted stalls restarted the run-in %ld time(s) (3 or more wanted: the case does not restart)", four.stalls);
    if (four.iPpm > one.iPpm + 50 || !four.card || !one.card) Err("FX25: four stalls left the integral at %+ld ppm against one's %+ld, the card's wait read %d and %d (within 50 and read wanted)", four.iPpm, one.iPpm, four.card, one.card);
    if (fourB.iPpm < oneB.iPpm + 300) Err("FX25, fail first: with bit 16 off four stalls left the integral at %+ld against one's %+ld (300 or more above wanted: the case does not show the ratchet)", fourB.iPpm, oneB.iPpm);
    else Ctl("bit 16 off: the integral %+ld after one stall, %+ld after four", oneB.iPpm, fourB.iPpm);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX26, A FAST INTEGRAL WHEN THE CARD'S WAIT ENDS: NO QUEUE HELD DEEP
// (2026-10-25, the review of e517d2dc, its second.) The card's wait is let go only at a median within the learning band; with the integral
// fast, the correction held at it fills the FIFO behind the card's wait, and when the wait ends the queue's own block stands over the band,
// so the reading holds and the queue stays a refresh deep. NOW (bit 18): while the block is read as the card's the correction is no faster
// than the refresh as read. THE CASE, FX24's shape (B) on a panel at its read, the integral PLANTED at -1,200 ppm (the blind teaching's
// floor) behind the engage, the card's wait ending at the fake's present 4,000 (about 65 s in), 160 s in all. JUDGED, behind the end
// of the wait: the card's wait no longer read, the block a present's median 250 to 750 us, no refresh shown twice over the last 1,000
// pairs. FAIL FIRST (bit 18 off): the reading still held at the end and the median over 1,500 us (the queue a refresh deep).
static FxRead Fx26Run(LONG guardOff)
{
    RtPar p = FxPar(60.0, (60ull << 32) | 1ull, 1);
    p.cardWaitRealUs = 14000; p.cardWaitGenUs = 8500; p.cardWaitJitUs = 1500; p.cardWaitUntil = 4000; p.guardOff = guardOff;
    gFxPlantAt = 300; gFxPlantIPpm = FG_FIX_BLIND_I_PPM;
    std::vector<LONG> tr; std::vector<FxWin> ws;
    const FxRead o = FxRun(p, 18.0, 4801, 3800, 20261025u, &tr, 0x40000000, &ws);
    gFxPlantAt = -1;
    printf("    (bits off %ld) the block's median and the correction every 20 s:", guardOff); for (size_t i = 0; i + 1 < tr.size(); i += 2) printf(" %ld/%+ld", tr[i], tr[i + 1]); printf("\n");
    return o;
}
static void CaseFX26()
{
    const char* const name = "FX26 a fast integral when the card's wait ends: the correction held no faster than the read, the reading let go, the queue at its target (2026-10-25, the review of opt A)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const FxRead o = Fx26Run(0), b = Fx26Run(1 << 18);
    ViolRestore(stc);
    printf("  THE BUILD: %s; the card's wait read at the end %d, the integral %+ld\n", FxLine(o), o.card, o.iPpm);
    printf("  FAIL FIRST, no floor while read (bit 18 off): %s; the card's wait read at the end %d, the integral %+ld\n", FxLine(b), b.card, b.iPpm);
    const std::string bad = FxJudge(o);
    if (!bad.empty()) Err("FX26: %s", bad.c_str());
    if (o.card || o.medUs < 250 || o.medUs > 750) Err("FX26: the card's wait read %d, the block a present's median %ld us at the end (not read, 250 to 750 wanted)", o.card, o.medUs);
    if (!b.card || b.medUs <= FG_FIX_TARGET_US + FG_FIX_LEARN_US) Err("FX26, fail first: with bit 18 off the reading %d and the median %ld us at the end (read and over 1,500 wanted: the case does not show the queue held deep)", b.card, b.medUs);
    else Ctl("bit 18 off: the reading held, the median %ld us, the correction %+ld ppm", b.medUs, b.ppm);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX28, THE FIXED SCREEN'S STANDBY READS THE RATE THE SCREEN GETS
// (2026-10-26, round 9, L1; FX27, round 8's B, is retired with the count it fed: its intervals were the SCREEN'S, quantised by a vsync model,
// a stand-in the shipped build never saw, and it passed while the build did nothing: docs/reports/fg-5/sitting-2026-10-26/READING.md.)
// THE SITTING: rel-weak60 stood by 10 windows on FGC_FAST ("the game runs at about 59 fps by itself", the window's median) while PresentMon
// counted 67 repeats in the 22 s before the engage (57.0 a second) and the waiter's own mean at window 1 read 57.2. NOW (fg_cond.h,
// FgCondFixedMeanUs and FgCondFixedFreeX10, into FAST's R_free): standing by in the fixed mode FAST reads the lower of the median's rate and
// THE SCREEN'S RATE, the window's mean interval with each interval capped at three refreshes.
// THE CASE IS FED THE WAITER'S OWN SAMPLE PATH: tIn, the QPC instant (10 MHz here) the latency waiter saw an injection retire, one a frame,
// and wP[i] = (float)LatMs(tIn - tInPrev), DLSS-IV.cpp's LatWaiterLoop's arithmetic; the window of LAT_WIN (120) through the product's own
// FgCondFixedMeanUs (FgpArmWindowStep's call) and FgCondFixedFreeX10 (FgpCondRead's), the median as LatMedian takes it, then the table and the
// law's step (the cut wait 0, the CPU placement answered). THE CARD'S PACE, the tIn THE LOGS PROVE: a card-limited game's injections retire
// at the card's own pace (the frame's work starts when the card is free and the image it renders into is back from the screen: three
// images, so the screen has shown frame i - 2), so a game under the refresh shows its own frame times there and never the screen's 33 ms
// gaps (READING.md), and a game at or over the refresh waits for the screen and steps on its grid. THE WEAK GAME: 85 of 100 frames 17.0 ms
// (spread 0.3), 15 of 100 at 20 to 24 ms: the median about 17.05 ms (58.6, FAST, as the sitting's warnings line read), the mean about
// 17.7 ms (56.5), and no interval over one and a half refreshes, so round 8's count reads 0.
// HELD: the weak game on a fixed 60 leaves STANDBY within 3 windows; a game of 59.5 (16.81 ms every frame) and one at the refresh (16.0 ms
// of work) stand by FAST all 20 windows; a game at the refresh WITH ONE 200 ms STALL in every window stands by FAST (the cap: one stall does
// not read slow); a card-limited game just under the refresh (16.75 ms) whose frames 25 and 75 of each window take 48 ms (the weak60 legs'
// 46 to 50 ms presents; the screen's 57.9) leaves STANDBY where THE BRIEF'S CAP OF TWO REFRESHES reads 58.7, FAST (the cap's reason, held);
// (a game WITH HEADROOM that misses a refresh catches up behind it in the swapchain's queue: its mean, and the screen's, stay near 60);
// 103 on a fixed 120 engages within 3; 60 on a fixed 120 stands by for FGC_SLOW (the floor 61.8); on a variable 120 Hz display the weak game's
// rows are those with no reading, window by window. CONTROLS: the reading off (n 0, the median alone): the weak game stands by all 20
// windows; the stall uncapped reads under the top and engages. FAIL FIRST ON THE BASE (af718127, built with this case and the base's
// fg_cond.h: the #else below is round 8's count through its own functions): the weak game stands by all 20 windows.
// WHAT IT CANNOT SEE: that the game's tIn has this shape (the logs prove the median, the begins' mean and PresentMon's rate, not each
// retire); fg_arm_life.h's two call sites, which dt_fg_test does not compile (tools/test-fg-cond's text check holds them); the cut wait and
// the CPU placement's wait, which the case sets aside (L-law-rules: the first engage waits for the placement's answer, about window 5).
enum { FX28_N = 120 };   // (the latency waiter's window, LAT_WIN: ivt_latency.h)
struct Fx28Card { double end, shown1, shown2; LONGLONG qPrev; unsigned seed; };
static double Fx28U(Fx28Card& c) { c.seed = c.seed * 1664525u + 1013904223u; return (double)((c.seed >> 8) & 0xFFFFu) / 65535.0; }
// one frame of g ms of work: its retire's instant, the waiter's tIn, in 10 MHz ticks
static LONGLONG Fx28Frame(Fx28Card& c, double g, double T)
{
    const double start = c.end > c.shown2 ? c.end : c.shown2;   // (the card free, and the image back from the screen)
    c.end = start + g;
    double s = ceil((c.end - 1e-6) / T) * T;
    if (s < c.shown1 + T) s = c.shown1 + T;
    c.shown2 = c.shown1; c.shown1 = s;
    return (LONGLONG)(c.end * 10000.0 + 0.5);
}
enum { FX28_WEAK = 0, FX28_EVEN = 1, FX28_STALL = 2, FX28_MISS = 3 };
static double Fx28Cost(Fx28Card& c, int shape, double g, int i)
{
    if (shape == FX28_WEAK) return Fx28U(c) < 0.15 ? 20.0 + 4.0 * Fx28U(c) : 17.0 + 0.6 * (Fx28U(c) - 0.5);
    if (shape == FX28_STALL) return i == 60 ? 200.0 : g;
    if (shape == FX28_MISS) return (i % 50) == 25 ? 48.0 : g;
    return g;
}
// THE WINDOW'S MEAN AT ANOTHER CAP (the case's own copy, told: the brief's two refreshes, and none)
static int Fx28MeanAt(const float* ms, int n, int hzX1000, double capRefreshes)
{
    double s = 0.0; int k = 0; const double cap = capRefreshes > 0.0 ? capRefreshes * 1.0e6 / (double)hzX1000 : 1e30;
    for (int i = 0; i < n; i++) { double v = ms[i]; if (!(v > 0.0)) continue; if (v > cap) v = cap; s += v; k++; }
    return k > 0 ? (int)(s * 1000.0 / k + 0.5) : 0;
}
// read: 1 the product's reading, 0 none (the median alone), 2 the mean at two refreshes, 3 uncapped
struct Fx28Run { int engagedAt, fastWins, rows[20], medX10, meanX10; };
static Fx28Run Fx28Go(int vrr, int hz, int shape, double g, int read)
{
    Fx28Run o = {}; o.engagedAt = -1;
    const FgCondK ck = FgCondDefaults();
    FgDriveK k = FgDriveDefaults(); k.cutWaitMs = 0;
    FgDriveMem m; FgDriveInit(m, k);
    const double T = 1000.0 / (double)hz;
    Fx28Card c = {}; c.seed = 20261026u;
    unsigned now = 0;
    float wP[FX28_N], srt[FX28_N];
    (void)Fx28Frame(c, g, T); c.qPrev = (LONGLONG)(c.end * 10000.0 + 0.5);
    for (int w = 0; w < 20; w++)
    {
        for (int i = 0; i < FX28_N; i++)
        {
            const LONGLONG tIn = Fx28Frame(c, Fx28Cost(c, shape, g, i), T);
            wP[i] = (float)((double)(tIn - c.qPrev) * 1000.0 / 1.0e7);   // (LatMs at a QPC of 10 MHz)
            c.qPrev = tIn;
        }
        for (int i = 0; i < FX28_N; i++) srt[i] = wP[i];
        std::nth_element(srt, srt + FX28_N / 2, srt + FX28_N);   // (LatMedian's)
        FgCondIn in = FxSession(vrr, 0, 1, 1, hz);
        in.realFpsX10 = FgCondRateX10((double)srt[FX28_N / 2], 0);
        in.engaged = m.state == FGDR_ON ? 1 : 0;
        const int hzX = FgCondHzX1000(in);
        in.freeFpsX10 = 0;                                       // (FgpCondRead's order: R_free 0 standing by, then the screen's rate)
        const int wn = read ? FX28_N : 0;
#if defined(FG_COND_FIXMEAN)
        const int mean = read == 1 ? FgCondFixedMeanUs(wP, FX28_N, hzX) : read == 2 ? Fx28MeanAt(wP, FX28_N, hzX, 2.0) : read == 3 ? Fx28MeanAt(wP, FX28_N, hzX, 0.0) : 0;
        in.freeFpsX10 = FgCondFixedFreeX10(in, wn, mean);        // (fgpFixWin[0], fgpFixWin[1] as FgpArmWindowStep writes them)
#else
        // (THE BASE, af718127: round 8's count over one and a half refreshes through its own functions, for the fail-first run alone)
        const int mean = read ? Fx28MeanAt(wP, FX28_N, hzX, read == 3 ? 0.0 : read == 2 ? 2.0 : 3.0) : 0;
        in.freeFpsX10 = FgCondFixedFreeX10(in, wn, read ? FgCondFixedOverOf(wP, FX28_N, hzX) : 0);
#endif
        in.lift = FgDriveWarmLift(m);
        FgDriveIn di = {};
        di.row = FgCondFirst(in, ck); di.band = FgDriveBandOf(in, ck); di.cpuReady = 1; di.realX10 = in.realFpsX10; di.card = 1;
        o.rows[w] = di.row; if (di.row == FGC_FAST) o.fastWins++;
        if (w == 5) { o.medX10 = in.realFpsX10; o.meanX10 = mean > 0 ? (int)(10000000LL / mean) : 0; }
        FgDriveStep(m, di, k, now, 0, 0); now += 2000;
        if (o.engagedAt < 0 && m.state == FGDR_ON) o.engagedAt = w;
    }
    return o;
}
static void CaseFX28()
{
    const char* const name = "FX28 the fixed screen's standby reads the rate the screen gets from the waiter's own intervals: a card-limited 57 engages within 3 windows, 59.5 and 60 stay native (2026-10-26, round 9, L1)";
    CaseBegin(name);
    struct L { const char* what; int vrr, hz, shape; double g; } legs[] = {
        { "a fixed 60, the weak game (17.0 ms, 15 of 100 at 20 to 24)", 0, 60, FX28_WEAK, 0.0 },
        { "a fixed 60, the game at 59.5 (16.81 ms)", 0, 60, FX28_EVEN, 1000.0 / 59.5 },
        { "a fixed 60, the game at the refresh (16.0 ms of work)", 0, 60, FX28_EVEN, 16.0 },
        { "a fixed 60, at the refresh with a 200 ms stall a window", 0, 60, FX28_STALL, 16.0 },
        { "a fixed 60, card-limited at 16.75 ms, 2 of 120 frames 48 ms", 0, 60, FX28_MISS, 16.75 },
        { "a fixed 120, the game at 103", 0, 120, FX28_EVEN, 1000.0 / 103.0 },
        { "a fixed 120, the game at 60", 0, 120, FX28_EVEN, 1000.0 / 60.0 },
        { "VRR at 120 Hz, the weak game", 1, 120, FX28_WEAK, 0.0 } };
    enum { NL = sizeof(legs) / sizeof(legs[0]) };
    Fx28Run now[NL], off[NL], two[NL], raw[NL];
    for (int i = 0; i < NL; i++)
    {
        now[i] = Fx28Go(legs[i].vrr, legs[i].hz, legs[i].shape, legs[i].g, 1); off[i] = Fx28Go(legs[i].vrr, legs[i].hz, legs[i].shape, legs[i].g, 0);
        two[i] = Fx28Go(legs[i].vrr, legs[i].hz, legs[i].shape, legs[i].g, 2); raw[i] = Fx28Go(legs[i].vrr, legs[i].hz, legs[i].shape, legs[i].g, 3);
        printf("  %s: the median's rate %d.%d, the waiter's capped mean %d.%d; leaves STANDBY at window %d (FAST in %d of 20); THE MEDIAN ALONE: %d (FAST in %d); CAPPED AT TWO: %d (the mean %d.%d); UNCAPPED: %d (the mean %d.%d)\n",
               legs[i].what, now[i].medX10 / 10, now[i].medX10 % 10, now[i].meanX10 / 10, now[i].meanX10 % 10, now[i].engagedAt, now[i].fastWins, off[i].engagedAt, off[i].fastWins,
               two[i].engagedAt, two[i].meanX10 / 10, two[i].meanX10 % 10, raw[i].engagedAt, raw[i].meanX10 / 10, raw[i].meanX10 % 10);
    }
    if (now[0].engagedAt < 0 || now[0].engagedAt > 2) Err("FX28: the weak game on a fixed 60 left STANDBY at window %d (within 3, by window 2, wanted)", now[0].engagedAt);
    if (now[0].medX10 <= 582) Err("FX28: the weak game's median read %d.%d (over the top of 58.2 wanted, as the sitting's: else the case shows nothing)", now[0].medX10 / 10, now[0].medX10 % 10);
    if (now[1].engagedAt >= 0 || now[2].engagedAt >= 0) Err("FX28: at 59.5 and at the refresh on a fixed 60 the law engaged at windows %d and %d (never in 20 wanted: the top is unchanged)", now[1].engagedAt, now[2].engagedAt);
    if (now[3].engagedAt >= 0) Err("FX28: at the refresh with a 200 ms stall a window the law engaged at window %d (never wanted: one stall does not read slow)", now[3].engagedAt);
    if (now[4].engagedAt < 0 || now[4].engagedAt > 2) Err("FX28: card-limited at 16.75 ms with two 48 ms frames a window the law left STANDBY at window %d (within 3 wanted: its mean %d.%d is under the top)", now[4].engagedAt, now[4].meanX10 / 10, now[4].meanX10 % 10);
    if (two[4].engagedAt >= 0) Err("FX28: capped at two refreshes the 48 ms frames' game left STANDBY at window %d (never wanted: the case does not show why the cap is three)", two[4].engagedAt);
    if (now[5].engagedAt < 0 || now[5].engagedAt > 2) Err("FX28: at 103 on a fixed 120 the law engaged at window %d (within 3 wanted)", now[5].engagedAt);
    if (now[6].engagedAt >= 0 || now[6].rows[19] != FGC_SLOW) Err("FX28: at 60 on a fixed 120 the law engaged at window %d with the row %s (never, and FGC_SLOW, wanted)", now[6].engagedAt, FgCondEnumName(now[6].rows[19]));
    int vrrDiff = 0; for (int w = 0; w < 20; w++) if (now[7].rows[w] != off[7].rows[w]) vrrDiff++;
    if (vrrDiff || now[7].engagedAt != off[7].engagedAt) Err("FX28: on the variable display %d of 20 rows differ from the median alone, the engage at %d against %d (none, and the same, wanted)", vrrDiff, now[7].engagedAt, off[7].engagedAt);
    if (off[0].engagedAt >= 0) Err("FX28 control: the median alone engaged the weak game at window %d (never wanted: the control shows nothing)", off[0].engagedAt);
    else Ctl("the median alone: the weak game stands by all 20 windows, FAST in %d", off[0].fastWins);
    if (raw[3].engagedAt < 0) Err("FX28 control: uncapped, the stall at the refresh never engaged (an engage wanted: the cap is what holds it)");
    else Ctl("uncapped, the game at the refresh with a 200 ms stall engages at window %d (the mean %d.%d)", raw[3].engagedAt, raw[3].meanX10 / 10, raw[3].meanX10 % 10);
    printf("  THE CAP'S REASON: card-limited at 16.75 ms with two 48 ms frames a window, capped at two refreshes the law leaves STANDBY at window %d (the mean %d.%d), at three %d (%d.%d)\n",
           two[4].engagedAt, two[4].meanX10 / 10, two[4].meanX10 % 10, now[4].engagedAt, now[4].meanX10 / 10, now[4].meanX10 % 10);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX29, THE FIRST SWITCH-ON ON A FIXED SCREEN COMES AT ONCE BEHIND THE DWELL
// (2026-10-26, round 9, L3b; the owner's yes to frame generation switching on "within a few seconds" on a fixed 60 Hz screen.) THE SITTING:
// rel-weak60 left STANDBY behind window 11 and engaged behind 15 (22 s into the record) through three waits: the CPU placement's answer (about
// window 5: cpu_place.h moves the game's threads 600 injected frames into the world), the dwell (3 windows) and the wait for a cut (10 s).
// NOW (fg_cond.h, FgDriveStep, FgDriveIn::fixed, as fg_arm_life.h's FgpLawWindowStep hands it): in the fixed mode the engage comes AT ONCE
// behind the dwell and the first does not wait for the placement (what that wait guards, the first stint's c and the alone rate under it,
// decides nothing in the mode: its hold files c 0 and its prediction takes no K); the dwell stays. THE CASE: FX28's waiter samples (the weak
// game, the card's pace through FgCondFixedMeanUs), the law with ITS OWN NUMBERS (the cut wait 10 s), the CPU placement answering behind
// window 5 as in every leg, the clock 2.2 s a window, the cut poll once a window (the product polls it every frame: the wait's end lands a
// window late here at the most, said). HELD: on a fixed 60 the weak game is ON behind window 3 (index 2) by FGDRW_AT_ONCE, with no ENGAGING
// window; a game at the refresh stays native all 20 windows; on a VRR 120 Hz display the weak game waits as before (the placement, the dwell,
// then the wait for a cut: ON by FGDRW_WAITED, no sooner than window 8). FAIL FIRST ON THE BASE (af718127's fg_cond.h, no FgDriveIn::fixed and
// round 8's count: the #else is the base's reading): the weak game on a fixed 60 never engages in 20 windows. WHAT IT CANNOT SEE: the game's
// first windows on the real card (a hitch burst or a window at the refresh restarts the dwell, as in the logs of 2026-10-25).
struct Fx29Run { int onAt, onWhy, engagingWins, fastWins; };
static Fx29Run Fx29Go(int vrr, int hz, int shape, double g)
{
    Fx29Run o = {}; o.onAt = -1;
    const FgCondK ck = FgCondDefaults();
    const FgDriveK k = FgDriveDefaults();                        // (the law's own numbers: the cut wait 10 s, the placement 30 windows at most)
    FgDriveMem m; FgDriveInit(m, k);
    const double T = 1000.0 / (double)hz;
    Fx28Card c = {}; c.seed = 20261026u;
    unsigned now = 1000;
    float wP[FX28_N], srt[FX28_N];
    (void)Fx28Frame(c, g, T); c.qPrev = (LONGLONG)(c.end * 10000.0 + 0.5);
    for (int w = 0; w < 20; w++)
    {
        for (int i = 0; i < FX28_N; i++)
        {
            const LONGLONG tIn = Fx28Frame(c, Fx28Cost(c, shape, g, i), T);
            wP[i] = (float)((double)(tIn - c.qPrev) * 1000.0 / 1.0e7); c.qPrev = tIn;
        }
        for (int i = 0; i < FX28_N; i++) srt[i] = wP[i];
        std::nth_element(srt, srt + FX28_N / 2, srt + FX28_N);
        FgCondIn in = FxSession(vrr, 0, 1, 1, hz);
        in.realFpsX10 = FgCondRateX10((double)srt[FX28_N / 2], 0);
        in.engaged = m.state == FGDR_ON ? 1 : 0;
        const int hzX = FgCondHzX1000(in);
        in.freeFpsX10 = 0;
#if defined(FG_COND_FIXMEAN)
        in.freeFpsX10 = FgCondFixedFreeX10(in, FX28_N, FgCondFixedMeanUs(wP, FX28_N, hzX));
#else
        in.freeFpsX10 = FgCondFixedFreeX10(in, FX28_N, FgCondFixedOverOf(wP, FX28_N, hzX));
#endif
        in.lift = FgDriveWarmLift(m);
        FgDriveIn di = {};
        di.row = FgCondFirst(in, ck); di.band = FgDriveBandOf(in, ck); di.realX10 = in.realFpsX10; di.card = 1;
        di.cpuReady = w >= 5 ? 1 : 0;                            // (the placement answers behind window 5, as cpu_place.h's 600 injected frames)
#if defined(FG_DRIVE_FIXED_AT_ONCE)
        di.fixed = FgCondFixedMode(in) ? 1 : 0;
#endif
        if (di.row == FGC_FAST) o.fastWins++;
        now += 2200;
        FgDriveStep(m, di, k, now, 0, 0);
        (void)FgDriveCut(m, k, now, 0, 0);                       // (the wait for a cut, once a window here)
        if (m.state == FGDR_ENGAGING) o.engagingWins++;
        if (o.onAt < 0 && m.state == FGDR_ON) { o.onAt = w; o.onWhy = m.why; }
    }
    return o;
}
static void CaseFX29()
{
    const char* const name = "FX29 on a fixed screen the first switch-on comes at once behind the dwell, no wait for the placement or a cut: the weak game ON behind window 3; VRR waits as before (2026-10-26, round 9, L3b)";
    CaseBegin(name);
    const Fx29Run a = Fx29Go(0, 60, FX28_WEAK, 0.0), b = Fx29Go(0, 60, FX28_EVEN, 16.0), v = Fx29Go(1, 120, FX28_WEAK, 0.0);
    printf("  a fixed 60, the weak game: ON at window %d by %s, %d window(s) in the wait for a cut\n", a.onAt, FgDriveWhyName(a.onWhy), a.engagingWins);
    printf("  a fixed 60, the game at the refresh: ON at window %d, FAST in %d of 20\n", b.onAt, b.fastWins);
    printf("  VRR at 120 Hz, the weak game: ON at window %d by %s, %d window(s) in the wait for a cut\n", v.onAt, FgDriveWhyName(v.onWhy), v.engagingWins);
    if (a.onAt != 2 || a.onWhy != FGDRW_AT_ONCE || a.engagingWins) Err("FX29: the weak game on a fixed 60 ON at window %d by %s with %d window(s) waiting for a cut (window 2, FGDRW_AT_ONCE, none wanted)", a.onAt, FgDriveWhyName(a.onWhy), a.engagingWins);
    if (b.onAt >= 0) Err("FX29: a game at the refresh on a fixed 60 engaged at window %d (never wanted: over the top it stays native)", b.onAt);
    if (v.onAt < 7 || v.onWhy != FGDRW_WAITED || v.engagingWins < 3) Err("FX29 control: on VRR the weak game ON at window %d by %s after %d window(s) in the wait (window 7 or later, FGDRW_WAITED, 3 or more wanted: the placement, the dwell and the cut wait as before)", v.onAt, FgDriveWhyName(v.onWhy), v.engagingWins);
    else Ctl("VRR keeps its waits: ON at window %d by %s", v.onAt, FgDriveWhyName(v.onWhy));
    CaseEnd(name);
}

// ---------------------------------------------------------------- FX30, A GAME AT THE REFRESH WITH A FEW LONG FRAMES ON A FIXED 120 DOES NOT CYCLE
// (2026-10-26, round 9, the reviewer's R1.) THE GAME: card-limited at 8.36 ms (its median 119.6 on a fixed 120 Hz screen) with frames 30, 60
// and 90 of each window at 25 ms: L1's capped mean reads about 114, under the top of 116.4, so it is IN standing by and engages at once
// behind the dwell (L3b). Engaged the mode holds it at two refreshes, and R_free is the held samples' bound capped at the law's alone rate
// (FgCondFreeX10). BEFORE THE FIX that alone rate was the median, 119.6, over the engaged top of 119.4: FGC_FAST stood it down behind the
// warm-up, the quick come-back let it back, and the guard spaced the cycle to a stand-down every 30 to 60 s. NOW (fg_cond.h, FgCondAloneX10,
// as FgpLawWindowStep hands it): in the fixed mode the alone rate is the reading FAST used standing by. THE CASE: five minutes of the
// law's own steps (its own numbers, the cut wait, the placement answered), standing-by windows fed FX28's waiter samples (FgCondFixedMeanUs,
// FgCondFixedFreeX10), engaged windows the mode's hold (R_on 60.0, the game's own median cost 8.36 ms, FgCondFreeX10 with the law's
// alone rate). HELD: one engage, no stand-down, ON at the end. CONTROL: a game at the refresh with no long frame (8.33 ms) stands by FAST
// all five minutes. FAIL FIRST ON THE BASE (720b8a7b's fg_cond.h, no FgCondAloneX10: the median): it cycles (engages and stand-downs by
// FGC_FAST). WHAT IT CANNOT SEE: whether the card's held samples on the real card read the game's own 8.36 ms (FX16 holds that reader).
struct Fx30Run { int engages, downs, fastDowns, endState; double mins; };
static Fx30Run Fx30Go(double g, bool longFrames)
{
    Fx30Run o = {};
    const FgCondK ck = FgCondDefaults();
    const FgDriveK k = FgDriveDefaults();
    FgDriveMem m; FgDriveInit(m, k);
    const double T = 1000.0 / 120.0;
    Fx28Card c = {}; c.seed = 20261026u;
    unsigned now = 1000; double tMs = 0.0;
    float wP[FX28_N], srt[FX28_N];
    (void)Fx28Frame(c, g, T); c.qPrev = (LONGLONG)(c.end * 10000.0 + 0.5);
    for (int w = 0; w < 1000 && tMs < 300000.0; w++)
    {
        FgCondIn in = FxSession(0, 0, 1, 1, 120);
        in.engaged = m.state == FGDR_ON ? 1 : 0;
        double winMs;
        if (!in.engaged)
        {
            double sum = 0.0;
            for (int i = 0; i < FX28_N; i++)
            {
                const LONGLONG tIn = Fx28Frame(c, (longFrames && (i == 30 || i == 60 || i == 90)) ? 25.0 : g, T);
                wP[i] = (float)((double)(tIn - c.qPrev) * 1000.0 / 1.0e7); c.qPrev = tIn; sum += wP[i];
            }
            for (int i = 0; i < FX28_N; i++) srt[i] = wP[i];
            std::nth_element(srt, srt + FX28_N / 2, srt + FX28_N);
            in.realFpsX10 = FgCondRateX10((double)srt[FX28_N / 2], 0);
            in.freeFpsX10 = 0;
#if defined(FG_COND_FIXMEAN)
            in.freeFpsX10 = FgCondFixedFreeX10(in, FX28_N, FgCondFixedMeanUs(wP, FX28_N, FgCondHzX1000(in)));
#endif
            winMs = sum;
        }
        else
        {   // the mode's hold: the real frame two refreshes, the game's own median cost the law's samples read
            const double held = 2.0 * T;
            in.realFpsX10 = FgCondRateX10(held, 0);
            in.freeFpsX10 = FgCondFreeX10((int)(g * 1000.0 + 0.5), (int)(held * 1000.0 + 0.5), FgCondFloorUs(in), held, m.aloneX10);
            winMs = FX28_N * held;
            c.end += winMs; c.shown1 = c.end; c.shown2 = c.end; c.qPrev = (LONGLONG)(c.end * 10000.0 + 0.5);
        }
        in.lift = FgDriveWarmLift(m);
        FgDriveIn di = {};
        di.row = FgCondFirst(in, ck); di.band = FgDriveBandOf(in, ck); di.cpuReady = 1; di.card = 1;
#if defined(FG_COND_ALONE_FAST)
        di.realX10 = FgCondAloneX10(in);
#else
        di.realX10 = in.realFpsX10;
#endif
#if defined(FG_DRIVE_FIXED_AT_ONCE)
        di.fixed = FgCondFixedMode(in) ? 1 : 0;
#endif
        if (in.engaged) { di.shownGen = 60; di.shownN = 60; di.perUs = (int)(2.0 * T * 1000.0 + 0.5); di.floorUs = FgCondFloorUs(in); di.lawPerUs = di.perUs; }
        tMs += winMs; now = 1000 + (unsigned)tMs;
        const int was = m.state;
        FgDriveStep(m, di, k, now, 0, 0);
        (void)FgDriveCut(m, k, now, 0, 0);
        if (was != FGDR_ON && m.state == FGDR_ON) o.engages++;
        if (was == FGDR_ON && m.state != FGDR_ON) { o.downs++; if (m.whyRow == FGC_FAST) o.fastDowns++; }
    }
    o.endState = m.state; o.mins = tMs / 60000.0;
    return o;
}
static void CaseFX30()
{
    const char* const name = "FX30 a game at the refresh with a few long frames on a fixed 120 does not cycle: the alone rate is FAST's reading (2026-10-26, round 9, the reviewer's R1)";
    CaseBegin(name);
    const Fx30Run a = Fx30Go(8.36, true), b = Fx30Go(8.33, false);
    printf("  a fixed 120, the game at 8.36 ms with three 25 ms frames a window: %.1f minutes, engages %d, stand-downs %d (by FGC_FAST %d), %s at the end\n", a.mins, a.engages, a.downs, a.fastDowns, FgDriveStateName(a.endState));
    printf("  CONTROL, the game at 8.33 ms with no long frame: %.1f minutes, engages %d, %s at the end\n", b.mins, b.engages, FgDriveStateName(b.endState));
    if (a.mins < 5.0 || a.engages != 1 || a.downs != 0 || a.endState != FGDR_ON) Err("FX30: in %.1f minutes %d engage(s) and %d stand-down(s) (%d by FGC_FAST), %s at the end (1, 0, ON wanted: no cycle)", a.mins, a.engages, a.downs, a.fastDowns, FgDriveStateName(a.endState));
    if (b.engages != 0) Err("FX30 control: the game at the refresh with no long frame engaged %d time(s) (never wanted: over the top it stays native)", b.engages);
    else Ctl("the game at the refresh with no long frame stays native: %d engages in %.1f minutes", b.engages, b.mins);
    CaseEnd(name);
}
