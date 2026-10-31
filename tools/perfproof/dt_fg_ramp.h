// dt_fg_ramp.h: THE SWITCH-ON AND THE STAND-DOWN, IN THE HARNESS (2026-10-18, phase 5 round 2, S1; docs/reports/fg-5/brief-R2/BRIEF-S1-switch.md).
//
// Included by dt_fg_test.cpp behind dt_fg_fixed.h, with ONE line; its cases are RP1 to RP4 (RUN lines of their own).
//
// THE OWNER'S LEGS lawon AND lawonff OF 2026-10-18 ("a little hitch when FG turned on"): at each switch-on one real frame of 39.0 and 43.3 ms
// (the frametime lines), the real rate stepping at once from the game's own (about 60) to the paced one (about 40), the engage's first pairs'
// handoffs retiring 25 to 37 ms behind their entries where the steady ones retire 14 to 15 (the pairs' records). fg_law.h, "THE SWITCH-ON'S
// RAMP", has the reading and the form.
//
// WHAT A RUN IS. dt_fg_rates.h's simulation with THE CARD'S WHOLE FRAME STANDING BY (RtPar::postIdle: until this file a frame with no hand
// queued no post chain, so the fake standing by ran far over its card, LW1's note), THE HELPER'S LISTS IN ORDER (RtPar::helperFifo) and the
// presenter's hold (DXVK holding the render thread while the card's queue stands more than latencyUs behind): a game that is the card's, as
// lawon's was. It stands by (the want at PASS), the want goes ON as the law's would, the machine runs, the want goes PASS (a stand-down), and
// the same again: two stints, as the leg had. Where the switch is asked for (RtPar::ramp, the armed build's key) the case publishes the
// target as the law's window step would (fg_law.h's FgRampTargetUs and FgRampCapUs, over the rate the fake ran standing by, the prediction from
// c, the card's reading and the floor's out-rate 37): THE FIRST STINT WITH NOTHING MEASURED (the prior), THE SECOND WITH c AS THE FIRST
// STINT'S REAL RATE GAVE IT, the law's own arithmetic (the stint's real frame time less the game's own).
// WHAT IS READ, at each switch, from the simulation's own instants (the frames' entries at their Present, the render thread's frame to frame;
// and every present as the card showed it): the game's own interval before it (the median of the 60 ahead), and in the 120 behind it the
// longest interval, the largest rise and fall from one to the next, the longest gap between two shown presents, the most period in force;
// the generated frames shown in the first 50 pairs; every real frame shown once and in order; the picture's age and the card's queue at their
// 95th, and the generated frame's place between its real frames on the card, in the stint's first 120 pairs and in its steady stretch. AND
// THE INSTRUMENT ITSELF (FgSwStep below, once the armed development build's switch line), fed the same intervals, must read the same numbers.
// THE JUDGE (RpJudge): NO LONG FRAME, on the render thread or on the screen: no interval behind the switch over FG_RP_STALL, and no gap between
// two shown presents over FG_RP_LONG, of the game's own median (the larger of the 60 ahead's and the 120 behind's; never the period the machine
// chose: S1 round 2, the reviewer's finding 4). The step of the real rate itself
// is printed and not judged: it is the period changing, which the screen does not show while the card's queue is full (fg_law.h says why a
// glide before the first pair was worse: RP1's numbers of 2026-10-18, a 40.5 ms gap at the first pair).
#pragma once
// THE ARMED BUILD'S SWITCH KEYS AS THE CASES RUN THEM (FgArmedKeys::ramp: the switch and the queue's drain ahead of its first pair, as
// fg_arm_life.h's FgpArmKeys sets them); -DFG_RP_KEYS=1 builds the switch without the drain, by hand
#ifndef FG_RP_KEYS
#define FG_RP_KEYS (FG_RAMP_KEY_ON | FG_RAMP_KEY_DRAIN | FG_RAMP_KEY_QREAD)
#endif

struct RpSw { double own, before, longest, rise, fall, per, perMax, gap, after; int gen50, n50; };
static double RpQ() { return (double)B::Qpf() / 1000.0; }
// THE SWITCH AT FRAME e (the first frame the want's change met): the entries' intervals of the 60 frames ahead and the 120 behind, and the
// shown presents of that stretch.
static RpSw RpRead(int e)
{
    RpSw w = {};
    const std::vector<RtPair>& P = gRt.pairs; const int n = (int)P.size(); const double q = RpQ();
    std::vector<double> before;
    for (int i = e - 60; i < e; i++) if (i >= 1) before.push_back((double)(P[i].tEntry - P[i - 1].tEntry) / q);
    if (!before.empty())
    {
        w.before = *std::max_element(before.begin(), before.end());
        std::nth_element(before.begin(), before.begin() + before.size() / 2, before.end()); w.own = before[before.size() / 2];
    }
    double prev = e >= 1 && e < n ? (double)(P[e].tEntry - P[e - 1].tEntry) / q : 0.0;
    std::vector<double> behind;
    for (int i = e + 1; i <= e + 120 && i < n; i++)
    {
        const double iv = (double)(P[i].tEntry - P[i - 1].tEntry) / q;
        behind.push_back(iv);
        if (iv > w.longest) w.longest = iv;
        if (prev > 0.0) { const double d = iv - prev; if (d > w.rise) w.rise = d; if (-d > w.fall) w.fall = -d; }
        prev = iv; w.per = (double)P[i].perUs / 1000.0; if (w.per > w.perMax) w.perMax = w.per;
    }
    if (!behind.empty()) { std::nth_element(behind.begin(), behind.begin() + behind.size() / 2, behind.end()); w.after = behind[behind.size() / 2]; }
    std::vector<LONGLONG> sh;
    for (int i = e - 60 < 0 ? 0 : e - 60; i <= e + 120 && i < n; i++) { if (P[i].gen == 1) sh.push_back(P[i].tGen); if (P[i].real == 1) sh.push_back(P[i].tReal); }
    std::sort(sh.begin(), sh.end());
    for (size_t i = 1; i < sh.size(); i++) { const double g = (double)(sh[i] - sh[i - 1]) / q; if (g > w.gap) w.gap = g; }
    for (int i = e; i < e + 50 && i < n - 1; i++) { w.n50++; if (P[i].gen == 1) w.gen50++; }
    return w;
}
// (MOVED HERE FROM fg_law.h in round 8's deep strip, S4, 2026-10-26: no build calls it since the armed development build's switch line
// went, and this file is its one reader. The arithmetic is as it was.)
// THE SWITCH'S READING (the armed development build's instrument, 2026-10-18, S1; the machine fed it at every present, fg_machine.h's
// SwitchLook, and the law's window step prints each record, fg_arm_life.h's FgpRampStep). THE LEG'S QUESTION: is the switch on or off a hitch
// on the render thread. The frametime line gives the longest of 120 frames and no step between two; this reads, around EACH switch (an engage
// from its first pair; a stand-down from the first present engaged no more), the game's own present-to-present
// interval before it (the median and the longest of the FG_SW_BEFORE intervals ahead of it) and, over the FG_SW_AFTER intervals behind it, the
// longest, the largest rise from one interval to the next and the largest fall. A switch that comes before the last one's FG_SW_AFTER are read
// closes the last one with what it has (rN says how many). Pure: the caller hands each interval (us, 0 none) and whether the present was
// engaged, and publishes a record where it returns 1. AN INTERVAL OF A TENTH OF A SECOND OR MORE (the caller hands -1: a stall, not a frame)
// is no frame the longest or the steps read, and IS COUNTED (rStall; 2026-10-18, S1 round 2, the coordinator's: a stall at a switch must not be
// invisible): the switching present's own interval with those behind it.
enum { FG_SW_RING = 64, FG_SW_BEFORE = 60, FG_SW_AFTER = 120 };
struct FgSwLook
{
    int iv[FG_SW_RING]; int n, at;                     // the last intervals, us (a ring)
    int on, kind, left;                                // the last present's state; the switch being read (1 an engage, 2 a stand-down, 0 none); intervals left
    int own, before, longest, rise, fall, prev, per, got, stall;
    int rKind, rOwn, rBefore, rLong, rRise, rFall, rPer, rN, rStall;   // the last record
    // (2026-10-18, round 3, SW: WHERE THE LONGEST STOOD, the interval's place behind the switch, 1 the first; the evening's lawmarg read its longest
    // as the switch's when its one long pair was the 83rd: the place lets a reader tell the switch's own frames from a stall seconds behind it)
    int longAt, rLongAt;
};
static inline int FgSwMedianBefore(const FgSwLook& L, int* most)
{
    int v[FG_SW_RING]; int k = L.n < FG_SW_BEFORE ? L.n : FG_SW_BEFORE;
    *most = 0;
    for (int i = 0; i < k; i++)
    {
        const int x = L.iv[(L.at - 1 - i + FG_SW_RING) % FG_SW_RING];
        if (x > *most) *most = x;
        int j = i; while (j > 0 && v[j - 1] > x) { v[j] = v[j - 1]; j--; } v[j] = x;
    }
    return k > 0 ? v[k / 2] : 0;
}
static inline int FgSwClose(FgSwLook& L)
{
    if (!L.kind) return 0;
    L.rKind = L.kind; L.rOwn = L.own; L.rBefore = L.before; L.rLong = L.longest; L.rRise = L.rise; L.rFall = L.fall; L.rPer = L.per; L.rN = L.got; L.rStall = L.stall; L.rLongAt = L.longAt;
    L.kind = 0;
    return 1;
}
// ONE PRESENT: ivUs its interval from the present before (0: none; -1: a stall, a tenth of a second or more), on 1 engaged, perUs the period
// in force. Returns 1 where a record was closed (the caller publishes rKind to rStall).
static inline int FgSwStep(FgSwLook& L, int ivUs, int on, int perUs)
{
    int closed = 0;
    if (on != L.on)
    {
        closed = FgSwClose(L);
        // (the switching present's own interval is the last one before the switch: the first one behind it is read against it)
        L.kind = on ? 1 : 2; L.left = FG_SW_AFTER; L.got = 0; L.longest = 0; L.longAt = 0; L.rise = 0; L.fall = 0; L.prev = ivUs > 0 ? ivUs : 0; L.per = perUs;
        L.stall = ivUs < 0 ? 1 : 0;
        L.own = FgSwMedianBefore(L, &L.before);
        L.on = on;
    }
    else if (L.kind && ivUs > 0)
    {
        if (ivUs > L.longest) { L.longest = ivUs; L.longAt = L.got + 1; }
        if (L.prev > 0) { const int d = ivUs - L.prev; if (d > L.rise) L.rise = d; if (-d > L.fall) L.fall = -d; }
        L.prev = ivUs; L.per = perUs; L.got++;
        if (--L.left <= 0) closed = FgSwClose(L);
    }
    else if (L.kind && ivUs < 0) L.stall++;
    if (ivUs > 0) { L.iv[L.at] = ivUs; L.at = (L.at + 1) % FG_SW_RING; if (L.n < FG_SW_RING) L.n++; }
    return closed;
}
// THE INSTRUMENT ON THE SAME STREAM: FgSwStep above fed every frame's interval and whether it was engaged (a hand recorded), as SwitchLook
// fed it in the armed development build. The records it closed, in order.
struct RpRec { int kind, own, before, longest, rise, fall, n, stall; };
static std::vector<RpRec> gRpRecs;
static FgSwLook gRpLook;
static void RpLookFrame(bool on)
{
    const std::vector<RtPair>& P = gRt.pairs; const int n = (int)P.size();
    int iv = n >= 2 ? (int)((P[n - 1].tEntry - P[n - 2].tEntry) * 1000000 / B::Qpf()) : 0;
    if (iv >= 100000) iv = -1;                                  // (as SwitchLook hands it: a tenth of a second or more is a stall, counted)
    if (FgSwStep(gRpLook, iv, on ? 1 : 0, (int)P[n - 1].perUs))
        gRpRecs.push_back({ gRpLook.rKind, gRpLook.rOwn, gRpLook.rBefore, gRpLook.rLong, gRpLook.rRise, gRpLook.rFall, gRpLook.rN, gRpLook.rStall });
}
struct RpOut
{
    RpSw on1, off1, on2, off2; int e1, d1, e2, d2;
    int n, real1, gen, order; std::string first;
    double alone, fpsOn1, fpsOn2;
    double age95, ageMed, depth95, depthMax; LONG place50;     // the first stint's steady stretch (its 400th frame to its end)
    double age95e, depth95e; LONG place50e;                    // the first stint's first 120 pairs
    LONG ramps, rampHeld, floorAt1, floorAt2, floorEnd; LONG ask1, ask2, rampTo1, rampTo2;
    LONG q1, q2, dr1, dr2;                                     // (round 3, SW) the queue the switch read standing by at each engage (us), and its drain's presents
    LONG pw1, pw2, pp1, pp2;                                   // ... and the worker's waits for an answer in each stint's first 120 frames: at the position, behind the prepare
    std::vector<RtPair> pairs;                                 // (the run's pairs, kept for a case that compares two runs)
};
static void RpFrame(double costMs, double spread)
{
    const LONG h0 = B::S().c.hands;
    RtFrame(RtMs(costMs * RtSpread(spread)));
    RpLookFrame(B::S().c.hands != h0 || ShArmB::a.r.rampSt == FG_RAMP_DRAIN);   // (as SwitchLook is fed: the drain's presents are the switch's)
}
static double RpFps(int a, int b)
{
    const std::vector<RtPair>& P = gRt.pairs; if (b > (int)P.size() - 1) b = (int)P.size() - 1;
    return b > a ? (double)(b - a) * 1000.0 / ((double)(P[b].tEntry - P[a].tEntry) / RpQ()) : 0.0;
}
// THE TARGET AS THE LAW'S WINDOW STEP PUBLISHES IT (fg_arm_life.h, FgpRampStep): over the rate the game ran standing by (the window's median, as
// the conditions read it), the law's prediction (c none: the game's own rate; c measured: 1 / (1/R + c)), the card's reading, the floor's out-rate.
// cUs: the c the law measured, us; under 0 nothing measured (a stint's first engage)
static LONG RpTarget(int aloneX10, const FgDriveMem& m, int card);
static LONG RpAsk(double aloneFps, double cUs, int card)
{
    const int aloneX10 = (int)(aloneFps * 10.0 + 0.5);
    // (as the law files it: c measured goes to the card's reading of the stint's engage; the prediction the law's own, FgDrivePredHiX10)
    FgDriveMem m; FgDriveInit(m, FgDriveDefaults());
    if (cUs >= 0.0) { if (card == 0) m.cRoomUs = (int)cUs; else m.cLimUs = (int)cUs; }
    return RpTarget(aloneX10, m, card);
}
static void RpStretch(int a, int b, double* age95, double* depth95, double* depthMax, LONG* place50)
{
    const std::vector<RtPair>& P = gRt.pairs; const double q = RpQ(); std::vector<double> ag; std::vector<LONG> dp;
    for (int i = a; i < b && i < (int)P.size() - 1; i++) if (P[i].real == 1) ag.push_back((double)(P[i].tReal - P[i].tEntry) / q);
    for (int i = a; i < b && i < (int)gRt.depth.size(); i++) dp.push_back(gRt.depth[i]);
    std::sort(ag.begin(), ag.end()); std::sort(dp.begin(), dp.end());
    *age95 = ag.empty() ? 0.0 : ag[ag.size() * 95 / 100];
    *depth95 = dp.empty() ? 0.0 : (double)dp[dp.size() * 95 / 100] / 1000.0;
    if (depthMax) *depthMax = dp.empty() ? 0.0 : (double)dp.back() / 1000.0;
    LONG lo = 0, hi = 0, p05 = 0, p95 = 0; RlPlace(a, b, place50, &lo, &hi, &p05, &p95);
}
// TWO STINTS: `stand` frames standing by, `on` engaged, `rest` standing by, `on` engaged, `rest` standing by. card: the card's reading the law
// would hand the target (1 the limit, 0 idle room). A frame of the fake game costs `costMs` with `spread` each way.
static RpOut RpGo(RtPar par, double costMs, double spread, int stand, int on, int rest, int card, const char* tag)
{
    RpOut o = {};
    gRpRecs.clear(); gRpLook = FgSwLook{};
    RtBegin(par);
    gRtSeed = 20261018u;
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < stand; i++) RpFrame(costMs, spread);
    {   // (the rate standing by, the window's median as the conditions read it: the last 120 frames)
        const std::vector<RtPair>& P = gRt.pairs; const int n = (int)P.size(); std::vector<double> iv;
        for (int i = n - 120 < 1 ? 1 : n - 120; i < n; i++) iv.push_back((double)(P[i].tEntry - P[i - 1].tEntry) / RpQ());
        std::nth_element(iv.begin(), iv.begin() + iv.size() / 2, iv.end()); o.alone = 1000.0 / iv[iv.size() / 2];
    }
    o.ask1 = par.ramp ? RpAsk(o.alone, -1.0, card) : 0; WriteRelease(&a.k.rampAskUs, o.ask1); WriteRelease(&a.k.rampCapUs, (LONG)FgRampCapUs(370));
    o.e1 = (int)gRt.pairs.size(); s.wt.want = FG_WANT_ON;
    { const LONG pw0 = a.w.posWaits, pp0 = a.w.prepWaitN;
    for (int i = 0; i < on; i++) { RpFrame(costMs, spread); if (i == 0) { o.floorAt1 = a.r.rampFloorUs; o.q1 = a.rq.blkAt; o.dr1 = a.r.rampSt == FG_RAMP_DRAIN ? a.r.rampDrain : 0; } if (i == 119) { o.pw1 = a.w.posWaits - pw0; o.pp1 = a.w.prepWaitN - pp0; } } }
    o.rampTo1 = a.r.rampToUs; o.floorEnd = a.r.rampFloorUs;
    o.d1 = (int)gRt.pairs.size(); s.wt.want = FG_WANT_PASS;
    o.fpsOn1 = RpFps(o.d1 - 240, o.d1 - 1);
    for (int i = 0; i < rest; i++) RpFrame(costMs, spread);
    const double cUs = (1000000.0 / o.fpsOn1) - (1000000.0 / o.alone);
    o.ask2 = par.ramp ? RpAsk(o.alone, cUs > 0.0 ? cUs : 0.0, card) : 0; WriteRelease(&a.k.rampAskUs, o.ask2);
    o.e2 = (int)gRt.pairs.size(); s.wt.want = FG_WANT_ON;
    { const LONG pw0 = a.w.posWaits, pp0 = a.w.prepWaitN;
    for (int i = 0; i < on; i++) { RpFrame(costMs, spread); if (i == 0) { o.floorAt2 = a.r.rampFloorUs; o.q2 = a.rq.blkAt; o.dr2 = a.r.rampSt == FG_RAMP_DRAIN ? a.r.rampDrain : 0; } if (i == 119) { o.pw2 = a.w.posWaits - pw0; o.pp2 = a.w.prepWaitN - pp0; } } }
    o.rampTo2 = a.r.rampToUs;
    o.d2 = (int)gRt.pairs.size(); s.wt.want = FG_WANT_PASS;
    o.fpsOn2 = RpFps(o.d2 - 240, o.d2 - 1);
    for (int i = 0; i < rest; i++) RpFrame(costMs, spread);
    o.on1 = RpRead(o.e1); o.off1 = RpRead(o.d1); o.on2 = RpRead(o.e2); o.off2 = RpRead(o.d2);
    RpStretch(o.e1 + 400, o.d1, &o.age95, &o.depth95, &o.depthMax, &o.place50);
    RpStretch(o.e1, o.e1 + 120, &o.age95e, &o.depth95e, nullptr, &o.place50e);
    {   const std::vector<RtPair>& P = gRt.pairs; std::vector<double> ag; const double q = RpQ();
        for (int i = o.e1 + 400; i < o.d1 - 1; i++) if (P[i].real == 1) ag.push_back((double)(P[i].tReal - P[i].tEntry) / q);
        std::sort(ag.begin(), ag.end()); o.ageMed = ag.empty() ? 0.0 : ag[ag.size() / 2]; }
    if (getenv("RP_DUMP") && strstr(tag, getenv("RP_DUMP")))
    {   // (by hand: RP_DUMP=<the run's tag>: each switch's frames, one a line)
        const std::vector<RtPair>& P = gRt.pairs; const double q = RpQ();
        for (int e : { o.e1, o.d1, o.e2, o.d2 })
        {
            printf("    %s: the frames from %d\n", tag, e);
            for (int i = e - 3; i < e + 24 && i < (int)P.size(); i++)
            {
                const RtPair& x = P[i];
                printf("      %4d interval %6.2f period %6.2f pair %4ld gen %d real %d | from the entry: gen %+7.2f real %+7.2f retire %+7.2f; K %5ld (%d); queue %5ld\n",
                       i, i ? (double)(x.tEntry - P[i - 1].tEntry) / q : 0.0, x.perUs / 1000.0, x.n, x.gen, x.real, x.gen == 1 ? (double)(x.tGen - x.tEntry) / q : 0.0,
                       x.real == 1 ? (double)(x.tReal - x.tEntry) / q : 0.0, x.tRet ? (double)(x.tRet - x.tEntry) / q : 0.0, x.kUs, x.disp, i < (int)gRt.depth.size() ? gRt.depth[i] : -1);
            }
        }
    }
    o.ramps = a.r.ramps; o.rampHeld = a.r.rampHeld;
    for (int i = 0; i < (int)gRt.pairs.size() - 1; i++) { o.n++; if (gRt.pairs[i].real == 1) o.real1++; if (gRt.pairs[i].gen == 1) o.gen++; }
    o.order = gRt.order; o.first = gRt.firstBad;
    o.pairs = gRt.pairs;
    s.wt.want = FG_WANT_ON;
    RtEnd(tag);
    return o;
}
static void RpSwLine(const char* what, const RpSw& w)
{
    printf("    %-13s the game's own %.2f ms (the longest of the 60 ahead %.2f); behind it: the game's own %.2f ms at the median, the longest %.2f ms, the most period in force %.2f,"
           " the longest gap between two shown presents %.2f ms; the largest rise %.2f ms (told), fall %.2f; the period at the end %.2f ms; generated frames shown in the first 50 pairs %d of %d\n",
           what, w.own, w.before, w.after, w.longest, w.perMax, w.gap, w.rise, w.fall, w.per, w.gen50, w.n50);
}
static void RpPrint(const char* what, const RpOut& o)
{
    printf("  %s: standing by %.1f a second; engaged %.1f and %.1f (each stint's last 240 frames); the target published %ld and %ld us, the switch's target %ld and %ld us,"
           " its floor at the first pair %ld and %ld us (at the first stint's end %ld); switches to a target %ld, the floor raised %ld present(s); %d frames, real frames shown once %d,"
           " generated shown %d, out of place %d%s%s; THE FIRST 120 PAIRS: the picture's age at its 95th %.2f ms, the card's queue at its 95th %.2f ms, on the card %ld;"
           " STEADY: the age %.2f ms at the median, %.2f at the 95th; the queue at its 95th %.2f ms (most %.2f); on the card %ld\n",
           what, o.alone, o.fpsOn1, o.fpsOn2, o.ask1, o.ask2, o.rampTo1, o.rampTo2, o.floorAt1, o.floorAt2, o.floorEnd, o.ramps, o.rampHeld, o.n, o.real1, o.gen, o.order,
           o.order ? ": " : "", o.order ? o.first.c_str() : "", o.age95e, o.depth95e, o.place50e, o.ageMed, o.age95, o.depth95, o.depthMax, o.place50);
    printf("    the queue the switch read standing by: %ld and %ld us at the two engages; the drain's presents %ld and %ld; the worker's waits for an answer in each stint's first 120: at the position %ld and %ld, behind the prepare %ld and %ld\n",
           o.q1, o.q2, o.dr1, o.dr2, o.pw1, o.pw2, o.pp1, o.pp2);
    RpSwLine("ENGAGE 1", o.on1); RpSwLine("STAND-DOWN 1", o.off1); RpSwLine("ENGAGE 2", o.on2); RpSwLine("STAND-DOWN 2", o.off2);
}
// THE JUDGE OF ONE SWITCH: no gap between two shown presents longer than FG_RP_LONG of the GAME'S OWN MEDIAN (the larger of the median of the
// 60 intervals ahead of the switch and of the 120 behind it), a long frame on the screen, and no interval of the render thread over FG_RP_STALL
// of it (a stall: the leg's were 1.6 of the paced period; a frame the pipeline's hold absorbs, under it, is printed and not judged). NEVER
// THE PERIOD THE MACHINE CHOSE (S1 round 2, the reviewer's finding 4: the floor's period is the switch's own, and a judge read against it
// excuses what it raised).
static const double FG_RP_LONG = 1.10, FG_RP_STALL = 1.30;
static std::string RpJudge(const char* what, const RpSw& w)
{
    std::string bad;
    const double ref = w.own > w.after ? w.own : w.after;
    if (w.longest > ref * FG_RP_STALL) bad += Fmt("%s: an interval of %.2f ms behind it (%.2f of the larger of the game's own medians %.2f ahead and %.2f behind; %.2f allowed: a stall); ", what, w.longest, w.longest / ref, w.own, w.after, FG_RP_STALL);
    if (w.gap > ref * FG_RP_LONG) bad += Fmt("%s: %.2f ms between two shown presents (%.2f of %.2f; %.2f allowed); ", what, w.gap, w.gap / ref, ref, FG_RP_LONG);
    return bad;
}
static std::string RpJudgeAll(const RpOut& o)
{
    std::string bad = RpJudge("engage 1", o.on1) + RpJudge("stand-down 1", o.off1) + RpJudge("engage 2", o.on2) + RpJudge("stand-down 2", o.off2);
    if (o.order || o.real1 != o.n) bad += Fmt("%d picture(s) out of place (%s), %d of %d real frames shown exactly once; ", o.order, o.first.c_str(), o.real1, o.n);
    return bad;
}
// THE INSTRUMENT AGAINST THE CASE'S OWN READING: four records (on, off, on, off), each the case's numbers within a microsecond's rounding
static std::string RpLookCheck(const RpOut& o)
{
    if (gRpRecs.size() != 4) return Fmt("the instrument closed %zu record(s) (4 wanted); ", gRpRecs.size());
    const RpSw* sw[4] = { &o.on1, &o.off1, &o.on2, &o.off2 };
    std::string bad;
    for (int i = 0; i < 4; i++)
    {
        const RpRec& r = gRpRecs[i]; const RpSw& w = *sw[i];
        if (r.kind != (i % 2 == 0 ? 1 : 2) || r.n != 120 || r.stall != 0 || abs(r.own - (int)(w.own * 1000.0)) > 2 || abs(r.longest - (int)(w.longest * 1000.0)) > 2
            || abs(r.rise - (int)(w.rise * 1000.0)) > 2 || abs(r.fall - (int)(w.fall * 1000.0)) > 2)
            bad += Fmt("record %d: kind %d, own %d, longest %d, rise %d, fall %d over %d, %d stall(s) against the case's %.0f, %.0f, %.0f, %.0f over 120 and none; ", i + 1, r.kind, r.own, r.longest, r.rise, r.fall, r.n, r.stall, w.own * 1000, w.longest * 1000, w.rise * 1000, w.fall * 1000);
    }
    return bad;
}

// ---------------------------------------------------------------- RP1, lawon's switch-on, reproduced and mended
// THE SHAPE: RL10's busy card (the lock at 300 MHz, stage 3's calibration, the armed build's keys of today) scaled so the game standing by runs
// at the card's own 60 or so, as lawon's did (FG_RP_CARD), with the post chain queued standing by too and the helper's lists in order.
// lawon: standing by about 58 to 61, the engaged stints at 39 to 43, one long frame at each switch-on and the step.
// FIRST ON TODAY'S MACHINE (the switch's key 0), THE CONTROL, which must show the leg's hitch: at the first engage the pacer starts at the
// panel's floor and frame generation's work goes onto the full queue at the game's own rate. HELD (2026-10-18): the first pair starved behind
// the queue (its answer 7.6 ms behind its retire), the real frames shown 46 to 50 ms behind their entries for eight pairs, one gap of 35.0 ms
// between two shown presents, the period then jumping to 27 ms. WHAT THE FAKE DOES NOT REPRODUCE, TOLD: the leg's render thread held 39 and
// 43 ms (the fake's longest 26.7: its presenter's hold is a time, DXVK's frame latency counts presents, and frame generation's are twice as
// many: RP9, by name, runs DXVK's count, and today's code holds the render thread 36.65 ms at the first engage there), and the second
// engage's stall (the fake's second engage, with the floors standing, shows the queue full for three to seven pairs and the generated frames
// bunched 16.8 / 4.3 ms, no long frame).
// THEN WITH THE SWITCH: no long frame on the render thread or the screen at either engage or stand-down, every real frame once and in order,
// the generated frames shown from the first pairs that can be (46 of 50 at the first engage where the control shows 32), the picture's age
// and the queue at their 95th in the first 120 pairs not over the control's, the place on the card 400 to 600; and the instrument reads the
// case's numbers. A SECOND CONTROL, the drain's margin compiled at 0 (-DFG_RAMP_DRAIN_PM=0: run by hand, its numbers in S1's report).
static const double FG_RP_CARD = 1.30;
static RtPar RpLawon(int ramp)
{
    RtPar p = RlPar(kRl[9], 0);
    const double sc = getenv("RP_CARD") ? atof(getenv("RP_CARD")) : FG_RP_CARD;   // (by hand: a heavier card, the scale on RL10's)
    p.tailUs = (LONG)(p.tailUs * sc); p.upUs = (LONG)(p.upUs * sc); p.postUs = (LONG)(p.postUs * sc); p.sceneUs = (LONG)(p.sceneUs * sc);
    p.kWorkUs = (LONG)(p.kWorkUs * sc); p.kResetUs = (LONG)(p.kResetUs * sc);
    p.postIdle = 1; p.helperFifo = 1; p.ramp = ramp ? FG_RP_KEYS : 0; p.rampFirst = ramp; p.rampDrainOk = 1;   // (the card the limit, as the law reads lawon's)
    p.holdWorker = 1;                                         // (round 3: the presenter's hold is the worker's, so the switch reads the queue standing by)
    return p;
}
static void CaseRP1()
{
    const char* const name = "RP1 lawon's switch-on (2026-10-18): today's machine shows the gap at the first engage, the switch to the target shows no long frame, every real frame once and in order";
    CaseBegin(name);
    const RtPar p0 = RpLawon(0), p1 = RpLawon(1);
    const double cost = gRlCostMs, spread = gRlSpread;   // (RlPar set them: RL10's CPU frame)
    const RpOut c = RpGo(p0, cost, spread, 600, 1080, 600, 1, "RP1 control");
    RpPrint("TODAY'S MACHINE (the switch's key 0)", c);
    const RpOut g = RpGo(p1, cost, spread, 600, 1080, 600, 1, "RP1");
    RpPrint("THE SWITCH TO THE TARGET", g);
    const std::string lk = RpLookCheck(g);
    printf("  the instrument (FgSwStep) on the switch's run: %zu record(s):", gRpRecs.size());
    for (const RpRec& r : gRpRecs) printf(" [%s own %d us, longest %d, rise %d, fall %d, of %d, stalls %d]", r.kind == 1 ? "on" : "off", r.own, r.longest, r.rise, r.fall, r.n, r.stall);
    printf("; against the case's own reading: %s\n", lk.empty() ? "the same" : lk.c_str());
    const std::string cb = RpJudge("engage 1", c.on1);
    if (cb.empty()) Err("RP1 control: today's machine showed no long frame and no gap at the first engage (the leg's hitch wanted): the case does not reproduce the leg");
    else Ctl("today's machine: %s", cb.c_str());
    const std::string gb = RpJudgeAll(g);
    if (!gb.empty()) Err("RP1: %s", gb.c_str());
    if (!lk.empty()) Err("RP1: the instrument: %s", lk.c_str());
    if (g.ramps != 2 || g.rampHeld <= 0 || g.floorEnd != 0) Err("RP1: %ld switch(es) to a target, the floor raised %ld present(s), the floor at the first stint's end %ld us (2, some, and 0: let go, wanted)", g.ramps, g.rampHeld, g.floorEnd);
    if (g.on1.gen50 < c.on1.gen50 || g.age95e > c.age95e + 0.5 || g.depth95e > c.depth95e + 0.5 || g.age95 > c.age95 + 0.5 || g.depth95 > c.depth95 + 0.5)
        Err("RP1: the first engage's generated frames %d of 50 (today %d), the age's 95th %.2f and %.2f ms, the queue's %.2f and %.2f ms in the first 120 pairs and the steady stretch (today %.2f, %.2f, %.2f, %.2f): none worse wanted",
            g.on1.gen50, c.on1.gen50, g.age95e, g.age95, g.depth95e, g.depth95, c.age95e, c.age95, c.depth95e, c.depth95);
    if (g.place50e < 400 || g.place50e > 600 || g.place50 < 400 || g.place50 > 600) Err("RP1: on the card %ld in the first 120 pairs and %ld steady (400 to 600 wanted)", g.place50e, g.place50);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP2, every panel
// 144 and 240 Hz VARIABLE panels with a busy card (lawon's card at a game of 60 on 144 Hz; scaled to a game of about 100 on 240 Hz) and a FREE
// CARD (RL9's, the owner's flw) at 120, 144 and 240 Hz: the same two stints, judged as RP1 judges. ON THE FREE CARD THE LAW'S READING IS IDLE
// ROOM and nothing is measured, so no target is asked (FgRampTargetUs gives the game's own) and the switch is today's: every number the same
// as the run with the key at 0, the pairs' instants to the tick.
static RtPar RpFree(ULONGLONG hz, int ramp)
{
    RtPar p = RlPar(kRl[8], 0);
    p.refresh = hz; p.postIdle = 1; p.helperFifo = 1; p.ramp = ramp ? FG_RP_KEYS : 0; p.rampFirst = ramp;
    p.holdWorker = 1;                                         // (round 3, as RpLawon)
    return p;
}
// TWO RUNS THE SAME AT EVERY SWITCH, within what a run repeated reads (the fixed mode's FIFO runs on the worker's own timing): the longest
// interval and the longest shown gap within half a millisecond, the engaged rates within 0.05 a second
static bool RpNear(const RpOut& a, const RpOut& b, std::string* why)
{
    const RpSw* x[4] = { &a.on1, &a.off1, &a.on2, &a.off2 }; const RpSw* y[4] = { &b.on1, &b.off1, &b.on2, &b.off2 };
    for (int i = 0; i < 4; i++)
        if (fabs(x[i]->longest - y[i]->longest) > 0.5 || fabs(x[i]->gap - y[i]->gap) > 0.5)
        { *why = Fmt("switch %d: the longest %.2f against %.2f ms, the gap %.2f against %.2f ms", i + 1, y[i]->longest, x[i]->longest, y[i]->gap, x[i]->gap); return false; }
    if (fabs(a.fpsOn1 - b.fpsOn1) > 0.05 || fabs(a.fpsOn2 - b.fpsOn2) > 0.05) { *why = Fmt("engaged %.2f and %.2f against %.2f and %.2f", b.fpsOn1, b.fpsOn2, a.fpsOn1, a.fpsOn2); return false; }
    return true;
}
static bool RpSame(const RpOut& a, const RpOut& b, std::string* why, size_t upto = (size_t)-1)
{
    if (a.pairs.size() != b.pairs.size()) { *why = Fmt("%zu pairs against %zu", a.pairs.size(), b.pairs.size()); return false; }
    for (size_t i = 0; i < a.pairs.size() && i < upto; i++)
    {
        const RtPair& x = a.pairs[i]; const RtPair& y = b.pairs[i];
        // (the harness's clock runs on across runs: every instant is read from the run's first entry)
        const LONGLONG ax = a.pairs[0].tEntry, by = b.pairs[0].tEntry;
        if (x.tEntry - ax != y.tEntry - by || (x.gen ? x.tGen - ax : 0) != (y.gen ? y.tGen - by : 0) || (x.real ? x.tReal - ax : 0) != (y.real ? y.tReal - by : 0)
            || x.gen != y.gen || x.real != y.real || x.perUs != y.perUs)
        { *why = Fmt("pair %zu differs (entry %+lld ticks, period %ld against %ld us)", i, (long long)((x.tEntry - ax) - (y.tEntry - by)), x.perUs, y.perUs); return false; }
    }
    return true;
}
static void CaseRP2()
{
    const char* const name = "RP2 every panel: 144 and 240 Hz with a busy card, the switch with no long frame; the free card at 120, 144 and 240 Hz today's until c is measured, then no long frame (2026-10-18)";
    CaseBegin(name);
    struct Busy { const char* what; ULONGLONG hz; double scale; };
    const Busy busy[] = { { "144 Hz, a busy card, a game of 60", (144ull << 32) | 1ull, 1.0 }, { "240 Hz, a busy card, a game of about 100", (240ull << 32) | 1ull, 0.78 } };
    for (const Busy& b : busy)
    {
        RtPar p = RpLawon(1); p.refresh = b.hz;
        p.tailUs = (LONG)(p.tailUs * b.scale); p.upUs = (LONG)(p.upUs * b.scale); p.postUs = (LONG)(p.postUs * b.scale); p.sceneUs = (LONG)(p.sceneUs * b.scale);
        p.kWorkUs = (LONG)(p.kWorkUs * b.scale); p.kResetUs = (LONG)(p.kResetUs * b.scale);
        const double cost = gRlCostMs * b.scale, spread = gRlSpread;
        RtPar pc = p; pc.ramp = 0; pc.rampFirst = 0;
        const RpOut c = RpGo(pc, cost, spread, 600, 1080, 600, 1, "RP2 control");
        const RpOut g = RpGo(p, cost, spread, 600, 1080, 600, 1, "RP2");
        printf("  %s:\n", b.what); RpPrint("today's machine", c); RpPrint("the switch", g);
        const std::string gb = RpJudgeAll(g);
        if (!gb.empty()) Err("RP2, %s: %s", b.what, gb.c_str());
        if (g.ramps != 2) Err("RP2, %s: %ld switch(es) to a target (2 wanted)", b.what, g.ramps);
        if (g.on1.gen50 < c.on1.gen50 - 2 || g.age95e > c.age95e + 0.5 || g.depth95e > c.depth95e + 0.5) Err("RP2, %s: generated frames %d of 50 (today %d), the first 120 pairs' age and queue at their 95th %.2f and %.2f ms (today %.2f and %.2f): none worse wanted", b.what, g.on1.gen50, c.on1.gen50, g.age95e, g.depth95e, c.age95e, c.depth95e);
        Ctl("%s, today: %s", b.what, RpJudge("engage 1", c.on1).empty() ? "no long frame" : RpJudge("engage 1", c.on1).c_str());
    }
    // (THE FREE CARD: a game the CPU holds, about 60 a second at 120 and 144 Hz and about 100 at 240; idle room on the card. THE FIRST STINT HAS
    // NOTHING MEASURED, so no target is asked and it is today's, pair for pair. THE SECOND STINT'S TARGET IS THE PREDICTION THE LAW ENGAGES ON
    // (S1 round 2, the reviewer's finding 2), from the first stint's real frame less the game's own as the case files it: on this card that is
    // the law's upper bound (the stint ran at a floor of the pacer's, fg_cond.h's "bound"), the worst a law can file. Its engage is judged as
    // RP1 judges on the render thread, and its shown gap not over today's second engage's (the latency's own step there is today's, told).
    // The rate the switch holds the stint's first 120 frames at, against today's, is TOLD)
    struct Fr { ULONGLONG hz; double cost; };
    const Fr frs[] = { { (120ull << 32) | 1ull, 16.0 }, { (144ull << 32) | 1ull, 16.0 }, { (240ull << 32) | 1ull, 10.0 } };
    for (const Fr& f : frs)
    {
        const unsigned hz = (unsigned)(f.hz >> 32);
        const RpOut c = RpGo(RpFree(f.hz, 0), f.cost, 0.10, 600, 720, 360, 0, "RP2 free control");
        const RpOut g = RpGo(RpFree(f.hz, 1), f.cost, 0.10, 600, 720, 360, 0, "RP2 free");
        printf("  the free card at %u Hz, a game of %.0f:\n", hz, 1000.0 / f.cost); RpPrint("today's machine", c); RpPrint("the switch", g);
        std::string why;
        const bool same = RpSame(c, g, &why, (size_t)g.e2);
        if (g.ramps > 1 || !same) Err("RP2, the free card at %u Hz: %ld switch(es) to a target, %s (at most the second stint's, and every pair before it the same as today's, wanted: nothing measured)", hz, g.ramps, same ? "the same" : why.c_str());
        const double ref2 = g.on2.own > g.on2.after ? g.on2.own : g.on2.after;
        if (g.on2.longest > ref2 * FG_RP_STALL || g.on2.gap > c.on2.gap + 0.5)
            Err("RP2, the free card at %u Hz, the second engage: the render thread's longest %.2f ms (%.2f allowed), the longest shown gap %.2f ms (today's %.2f)", hz, g.on2.longest, ref2 * FG_RP_STALL, g.on2.gap, c.on2.gap);
        if (g.order || g.real1 != g.n) Err("RP2, the free card at %u Hz: %d picture(s) out of place, %d of %d real frames shown once", hz, g.order, g.real1, g.n);
        Ctl("the free card at %u Hz: the engages' longest shown gaps %.2f and %.2f ms (today's %.2f and %.2f) against the periods %.2f and %.2f; the second stint's first 120 at a median of %.2f ms with the switch's target %ld us, today's %.2f (told)",
            hz, g.on1.gap, g.on2.gap, c.on1.gap, c.on2.gap, g.on1.perMax, g.on2.perMax, g.on2.after, g.rampTo2, c.on2.after);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP3, the fixed-refresh mode
// FIXED 60 AND 120 Hz PANELS (the panel model a grid behind DXVK's FIFO, FX1's shape), the fixed mode on: THE SWITCH MAKES NONE (the period is two
// refreshes from the first present by design, PaceFixed), so every pair is the same as the run with the key at 0 even with a target asked
// (the card's reading the limit), and the two-refresh hold stands: no refresh shown twice behind the first engage's correction run-in.
static void CaseRP3()
{
    const char* const name = "RP3 the fixed-refresh mode at 60 and 120 Hz: the switch makes none, every pair the same as today's, the two-refresh hold standing (2026-10-18)";
    CaseBegin(name);
    struct Fx { const char* what; double hz; ULONGLONG read; double cost; };
    const Fx fx[] = { { "a fixed 120 Hz panel, a game of 90", 120.0, (120ull << 32) | 1ull, 11.1 }, { "a fixed 60 Hz panel, a game of 45", 60.0, (60ull << 32) | 1ull, 22.2 } };
    for (const Fx& f : fx)
    {
        RtPar p = FxPar(f.hz, f.read, 1); p.postIdle = 0;
        RtPar pc = p; pc.ramp = 0; pc.rampFirst = 0; p.ramp = FG_RP_KEYS; p.rampFirst = 1;
        const RpOut c = RpGo(pc, f.cost, 0.05, 600, 1080, 600, 1, "RP3 control");
        const RpOut g = RpGo(p, f.cost, 0.05, 600, 1080, 600, 1, "RP3");
        std::string why;
        const bool same = RpNear(c, g, &why);
        printf("  %s: standing by %.1f, engaged %.2f and %.2f, the target asked %ld us, switches to a target %ld, the floor raised %ld; %s\n", f.what, g.alone, g.fpsOn1, g.fpsOn2, g.ask1, g.ramps, g.rampHeld, same ? "every switch as today's" : why.c_str());
        RpSwLine("ENGAGE 1", g.on1); RpSwLine("STAND-DOWN 1", g.off1);
        if (!same || g.ramps != 0 || g.rampHeld != 0) Err("RP3, %s: %s, %ld switch(es) to a target, the floor raised %ld (the same as today, 0 and 0 wanted)", f.what, same ? "the same" : why.c_str(), g.ramps, g.rampHeld);
        if (g.ask1 <= 0) Err("RP3, %s: no target was asked (one wanted, so the case shows the fixed mode refusing it)", f.what);
        if (fabs(g.fpsOn1 - f.hz / 2.0) > 0.1) Err("RP3, %s: engaged at %.3f real frames a second (%.1f wanted: two refreshes a pair)", f.what, g.fpsOn1, f.hz / 2.0);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP4, the floor is never a latch, and the reader of a window
// lawon's shape, one stint of 1,080 frames: (a) THE FLOOR IS HELD WHILE THE LAW WARMS UP AND LET GO BEHIND IT: at the first pair the target and the
// drain's margin; eased to the target; gone (0) within 150 presents behind the law's 150 steps of warm-up; (b) A STAND-DOWN TAKES IT AT ONCE:
// a stint stood down at its 20th frame, the floor 0 at the next present, and the next engage starts from a new target; (c) THE CAP: a target
// asked over the floor's out-rate (a game of 30 with the prior) holds the period no longer than that out-rate's; (d) THE READER: the windows of
// 120 frames whose presents the floor raised are the switch's first ones only (FgpRampWindow's count, the floor's count a window, read here as the
// law's window step reads it), and none behind the floor's end.
static void CaseRP4()
{
    const char* const name = "RP4 the switch's floor: held while the law warms up, let go behind it, taken by a stand-down at once, never over the floor's out-rate; the reader marks its windows alone (2026-10-18)";
    CaseBegin(name);
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    // (a) and (d)
    {
        RtBegin(RpLawon(1)); gRtSeed = 20261018u; s.wt.want = FG_WANT_PASS;
        for (int i = 0; i < 600; i++) RpFrame(gRlCostMs, gRlSpread);
        WriteRelease(&a.k.rampAskUs, RpAsk(60.0, -1.0, 1)); WriteRelease(&a.k.rampCapUs, (LONG)FgRampCapUs(370));
        s.wt.want = FG_WANT_ON;
        LONG first = 0, gone = -1, seen = a.r.rampHeld; std::vector<int> wins;
        for (int i = 0; i < 1080; i++)
        {
            RpFrame(gRlCostMs, gRlSpread);
            if (i == 0) first = a.r.rampFloorUs;
            if (gone < 0 && i > 0 && a.r.rampFloorUs == 0) gone = i;
            if ((i % 120) == 119) { wins.push_back(a.r.rampHeld != seen ? 1 : 0); seen = a.r.rampHeld; }
        }
        const LONG to = a.r.rampToUs;
        s.wt.want = FG_WANT_ON; RtEnd("RP4 a");
        std::string ws; int marked = 0, lastMarked = -1; for (size_t w = 0; w < wins.size(); w++) { ws += wins[w] ? "1" : "0"; if (wins[w]) { marked++; lastMarked = (int)w; } }
        printf("  (a) the target %ld us, the floor at the first pair %ld us, gone at the stint's present %d; (d) the windows the floor raised a present in: %s\n", to, first, gone, ws.c_str());
        const double want = FgRampFloorMs((double)to / 1000.0, (double)FgRampCapUs(370) / 1000.0) * 1000.0;
        if (labs(first - (LONG)want) > 300) Err("RP4 (a): the floor at the first pair %ld us (the target %ld and the drain's margin: %.0f wanted)", first, to, want);
        if (gone < 150 || gone > 300) Err("RP4 (a): the floor gone at the stint's present %d (behind the law's 150 steps of warm-up and within 150 more wanted)", gone);
        if (marked < 1 || lastMarked > 2) Err("RP4 (d): the reader marked %d window(s), the last the %d. (one to three of the first, none behind, wanted)", marked, lastMarked + 1);
    }
    // (b)
    {
        RtBegin(RpLawon(1)); gRtSeed = 20261018u; s.wt.want = FG_WANT_PASS;
        for (int i = 0; i < 600; i++) RpFrame(gRlCostMs, gRlSpread);
        WriteRelease(&a.k.rampAskUs, RpAsk(60.0, -1.0, 1)); WriteRelease(&a.k.rampCapUs, (LONG)FgRampCapUs(370));
        s.wt.want = FG_WANT_ON; for (int i = 0; i < 20; i++) RpFrame(gRlCostMs, gRlSpread);
        const LONG held = a.r.rampFloorUs;
        s.wt.want = FG_WANT_PASS; RpFrame(gRlCostMs, gRlSpread);
        const LONG after = a.r.rampFloorUs, st = a.r.rampSt;
        for (int i = 0; i < 300; i++) RpFrame(gRlCostMs, gRlSpread);
        s.wt.want = FG_WANT_ON; RpFrame(gRlCostMs, gRlSpread);
        const LONG again = a.r.rampFloorUs, ramps = a.r.ramps;
        s.wt.want = FG_WANT_ON; RtEnd("RP4 b");
        printf("  (b) the floor 20 presents in %ld us; the present behind the stand-down: the floor %ld us, the state %ld; the next engage: the floor %ld us, switches %ld\n", held, after, st, again, ramps);
        if (held <= 0 || after != 0 || st != FG_RAMP_NONE || again <= 0 || ramps != 2) Err("RP4 (b): the floor %ld, then %ld (state %ld) behind the stand-down, %ld at the next engage, %ld switch(es) (some, 0, 0, some and 2 wanted)", held, after, st, again, ramps);
    }
    // (c)
    {
        const LONG ask = RpAsk(30.0, -1.0, 1), cap = (LONG)FgRampCapUs(370);
        const double f = FgRampFloorMs((double)ask / 1000.0, (double)cap / 1000.0) * 1000.0;
        printf("  (c) a game of 30 with the prior: the target %ld us, the cap %ld us, the floor at the switch %.0f us\n", ask, cap, f);
        if (ask > cap + 1 && ask > 33334) Err("RP4 (c): the target %ld us over the cap %ld us", ask, cap);
        if (f > (double)(ask > cap ? ask : cap) + 1.0) Err("RP4 (c): the floor %.0f us over the larger of the target %ld and the cap %ld", f, ask, cap);
        const LONG a2 = RpAsk(45.0, -1.0, 1);
        const double f2 = FgRampFloorMs((double)a2 / 1000.0, (double)cap / 1000.0) * 1000.0;
        printf("      a game of 45: the target %ld us (the prior's 33,333 held at the cap), the floor %.0f us\n", a2, f2);
        if (a2 > cap + 1 || f2 > (double)cap + 1.0) Err("RP4 (c): a game of 45: the target %ld and the floor %.0f us over the cap %ld", a2, f2, cap);
    }
    // (e) THE READER COUNTS A STALL (S1 round 2, the coordinator's): SwitchLook hands an interval of a tenth of a second or more as -1, which
    // FgSwStep leaves out of the longest and the steps and counts. A stream: 60 frames of 16,667 us standing by, the engage's own interval a
    // stall, 40 frames of 25,000, a stall, 79 of 25,000 (120 read), a stall behind the record's close (not this record's)
    {
        FgSwLook L = {}; int closed = 0;
        for (int i = 0; i < 60; i++) closed += FgSwStep(L, 16667, 0, 16667);
        closed += FgSwStep(L, -1, 1, 25000);
        for (int i = 0; i < 40; i++) closed += FgSwStep(L, 25000, 1, 25000);
        closed += FgSwStep(L, -1, 1, 25000);
        for (int i = 0; i < 80; i++) closed += FgSwStep(L, 25000, 1, 25000);
        const int st = L.rStall, lg = L.rLong, rn = L.rN, rs = L.rRise;
        closed += FgSwStep(L, -1, 1, 25000);
        printf("  (e) a stall at the engage and one behind it: %d record(s) closed, of %d frames, the longest %d us, the largest rise %d us, stalls counted %d (then %d after the close)\n", closed, rn, lg, rs, st, L.rStall);
        if (closed != 1 || rn != 120 || lg != 25000 || rs != 0 || st != 2 || L.rStall != 2)
            Err("RP4 (e): %d record(s), %d frames, the longest %d, the rise %d, %d stall(s), %d after the close (1, 120, 25000, 0, 2 and 2 wanted)", closed, rn, lg, rs, st, L.rStall);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP5, the switch's cost a present (by name alone: a timing, told)
// The cost rule: what the switch adds to every present, on the render thread, read with the real clock over a million calls each (the
// machine's own functions on the harness's binding): RampWant engaged (the state ON: one load of the key, two compares), RampPer with no floor
// (one load, a compare), RampPer with the floor standing in the law's warm-up (its multiply and store), and the armed development build's
// instrument (FgSwStep) a present. Nothing here reads the card: the figures are the CPU's, the same at every resolution.
static void CaseRP5()
{
    const char* const name = "RP5 the switch's cost a present on the render thread (2026-10-18; told, a timing)";
    CaseBegin(name);
    FgArmedState& a = ShArmB::a;
    RtBegin(RpLawon(1)); B::S().wt.want = FG_WANT_PASS;
    for (int i = 0; i < 20; i++) RpFrame(gRlCostMs, gRlSpread);
    const int N = 1000000; LARGE_INTEGER f, t0, t1; QueryPerformanceFrequency(&f);
    volatile LONG sink = 0; volatile double dsink = 0.0;
    a.r.rampSt = FG_RAMP_ON; a.r.rampFloorUs = 0;
    QueryPerformanceCounter(&t0); for (int i = 0; i < N; i++) sink += MR::RampWant(FG_WANT_ON); QueryPerformanceCounter(&t1);
    const double wantNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / N;
    QueryPerformanceCounter(&t0); for (int i = 0; i < N; i++) dsink += MR::RampPer(20.0 + (i & 7)); QueryPerformanceCounter(&t1);
    const double perNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / N;
    double heldNs = 0.0;
    {   // (the floor standing: reset to its value before each call, so every call holds and eases)
        FgLaw* lw = B::S().law; const int warm0 = lw ? lw->warm : 0; if (lw) lw->warm = 100;
        a.r.rampToUs = 24000;
        QueryPerformanceCounter(&t0); for (int i = 0; i < N; i++) { a.r.rampFloorUs = 26000; dsink += MR::RampPer(20.0); } QueryPerformanceCounter(&t1);
        heldNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / N;
        if (lw) lw->warm = warm0;
        a.r.rampFloorUs = 0;
    }
    FgSwLook lk = {};
    QueryPerformanceCounter(&t0); for (int i = 0; i < N; i++) sink += FgSwStep(lk, 16000 + (i & 255), (i / 300) & 1, 24000); QueryPerformanceCounter(&t1);
    const double swNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / N;
    // (round 3, SW: THE QUEUE READ STANDING BY, a present standing by: RampWant at PASS with the key, its read of the worker's sum and count, the
    // divide and the store; the ring's last count put one behind the worker's before each call, so the divide runs every time (the worker's own
    // counters are not touched); and the median of eight at an engage)
    a.r.rampSt = FG_RAMP_NONE;
    QueryPerformanceCounter(&t0); for (int i = 0; i < N; i++) { a.rq.cnt = (LONG)dtPresDone - 1; a.rq.sumT = 0; sink += MR::RampWant(FG_WANT_PASS); } QueryPerformanceCounter(&t1);
    const double qNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / N;
    QueryPerformanceCounter(&t0); for (int i = 0; i < N / 10; i++) { a.rq.n = FG_RAMPQ_N; a.rq.blk[i & 7] = i & 1023; sink += MR::RampQueueUs(); } QueryPerformanceCounter(&t1);
    const double medNs = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)f.QuadPart / (N / 10);
    B::S().wt.want = FG_WANT_ON; RtEnd("RP5");
    printf("  a present's cost on this CPU: RampWant engaged %.1f ns; RampPer with no floor %.1f ns, with the floor standing %.1f ns; the instrument (FgSwStep, the armed development build alone) %.1f ns, a switch every 300 presents included;"
           " (round 3) RampWant standing by with the queue's read %.1f ns (its divide taken every call), the median of eight at an engage %.1f ns (%ld %.0f)\n",
        wantNs, perNs, heldNs, swNs, qNs, medNs, (long)sink, (double)dsink);
    if (wantNs > 50.0 || perNs > 50.0 || heldNs > 100.0 || swNs > 1000.0 || qNs > 100.0 || medNs > 500.0) Err("RP5: %.1f, %.1f, %.1f, %.1f, %.1f and %.1f ns (under 50, 50, 100, 1,000, 100 and 500 wanted: the cost rule's 50 us is orders over)", wantNs, perNs, heldNs, swNs, qNs, medNs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP6, the ride's WAIT floor at lawon's shape (by name alone: told)
// THE MAIN SESSION'S ITEM OF 2026-10-18 (S4's report: at lawon c = 8.1 ms is about 4.3 ms of frame generation's work and about 3.8 of pacing; the
// period equals the ride's WAIT floor in every ON window, twice the ride's wait W, which carries the next frame's upscale). One stint of 1,800
// frames behind 600 standing by at RP1's shape (the switch on, as the armed build hands it), read over its steady stretch (from its 400th frame),
// in three arms: TODAY'S FLOORS; THE WAIT FLOOR OUT, EVEN AND FIT KEPT (FgArmedKeys::rideNoWait with the early show: fg_pace's bit 4096); FIT
// ALONE (rideFitOnly, the planting on: bit 65536). Each: the real rate, generated frames shown, the generated frame's place between its real
// frames on the card (the median, the 5th and 95th, pairs more than 150 thousandths off the middle), the picture's age and the card's queue at
// their 95th, the ride's floors at the end, the position fire's counts.
struct Rp6Out { double fps, shown, age95, ageMax, depth95; LONG pl50, pl05, pl95; int off, n; LONG even, wait, fit, ride, period, posArms, posFires, posNoAns, posDue, posInj; };
static Rp6Out Rp6Run(RtPar p, const char* tag)
{
    Rp6Out o = {};
    RtBegin(p); gRtSeed = 20261018u;
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 600; i++) RpFrame(gRlCostMs, gRlSpread);
    WriteRelease(&a.k.rampAskUs, RpAsk(60.0, -1.0, 1)); WriteRelease(&a.k.rampCapUs, (LONG)FgRampCapUs(370));
    const int e = (int)gRt.pairs.size(); s.wt.want = FG_WANT_ON;
    for (int i = 0; i < 1800; i++) RpFrame(gRlCostMs, gRlSpread);
    const int b = (int)gRt.pairs.size() - 1, a0 = e + 400;
    o.fps = RpFps(a0, b);
    const std::vector<RtPair>& P = gRt.pairs; const double q = RpQ(); std::vector<double> ag; std::vector<LONG> dp, pl;
    for (int i = a0; i < b; i++)
    {
        o.n++; if (P[i].gen == 1) o.shown += 1.0;
        if (P[i].real == 1) ag.push_back((double)(P[i].tReal - P[i].tEntry) / q);
        if (i < (int)gRt.depth.size()) dp.push_back(gRt.depth[i]);
        if (i >= 1 && P[i].gen == 1 && P[i].real == 1 && P[i - 1].real == 1 && P[i].tReal > P[i - 1].tReal)
        { const LONG v = (LONG)(1000.0 * (double)(P[i].tGen - P[i - 1].tReal) / (double)(P[i].tReal - P[i - 1].tReal) + 0.5); pl.push_back(v); if (v < 350 || v > 650) o.off++; }
    }
    o.shown = o.n ? 100.0 * o.shown / o.n : 0.0;
    std::sort(ag.begin(), ag.end()); std::sort(dp.begin(), dp.end()); std::sort(pl.begin(), pl.end());
    if (!ag.empty()) { o.age95 = ag[ag.size() * 95 / 100]; o.ageMax = ag.back(); }
    if (!dp.empty()) o.depth95 = (double)dp[dp.size() * 95 / 100] / 1000.0;
    if (!pl.empty()) { o.pl50 = pl[pl.size() / 2]; o.pl05 = pl[pl.size() * 5 / 100]; o.pl95 = pl[pl.size() * 95 / 100]; }
    o.even = a.pl.rideEvenUs; o.wait = a.pl.rideWaitUs; o.fit = a.pl.rideFitUs; o.ride = a.pl.rideUs; o.period = s.r.periodUs;
    o.posArms = a.w.posArms; o.posFires = a.w.posFires; o.posNoAns = a.w.posNoAns; o.posDue = a.w.posDue; o.posInj = a.w.posInj;
    s.wt.want = FG_WANT_ON; RtEnd(tag);
    return o;
}
static void Rp6Line(const char* what, const Rp6Out& o)
{
    printf("  %-34s %.2f real a second; %.1f of 100 shown; on the card %ld (5th %ld, 95th %ld), %d of %d pairs more than 150 off the middle; the age's 95th %.2f ms (most %.2f), the queue's 95th %.2f ms;"
           " the floors at the end: even %ld, wait %ld, fit %ld, the ride's %ld us, the period %ld us; positions armed %ld, fired there %ld, met with no answer %ld, fallbacks: the clock due %ld, the injection %ld\n",
           what, o.fps, o.shown, o.pl50, o.pl05, o.pl95, o.off, o.n, o.age95, o.ageMax, o.depth95, o.even, o.wait, o.fit, o.ride, o.period, o.posArms, o.posFires, o.posNoAns, o.posDue, o.posInj);
}
static void CaseRP6()
{
    const char* const name = "RP6 the ride's WAIT floor at lawon's shape: today's floors, the WAIT floor out, FIT alone (2026-10-18; told)";
    CaseBegin(name);
    RtPar p = RpLawon(1);
    if (getenv("RP6_POST")) { const LONG post = atol(getenv("RP6_POST")), moved = p.postUs - post; p.postUs = post; p.sceneUs += moved; }   // (by hand: the card's split)
    const Rp6Out t = Rp6Run(p, "RP6 today");
    RtPar pw = p; pw.rideEarly = 1; pw.rideNoWait = 1; const Rp6Out w = Rp6Run(pw, "RP6 wait out");
    RtPar pf = p; pf.fitOnly = 1; const Rp6Out f = Rp6Run(pf, "RP6 fit alone");
    Rp6Line("TODAY'S FLOORS:", t); Rp6Line("THE WAIT FLOOR OUT, EVEN AND FIT KEPT:", w); Rp6Line("FIT ALONE:", f);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP7, the target follows the prediction the law engages on (round 2 of S1)
// THE REVIEWER'S FINDINGS 1 AND 2 (2026-10-18): the prior must stand only where NOTHING has been measured, never where c was measured near 0;
// and where the law predicts with a c (S2's merged prediction gives an idle card with no c of its own the limit's), the target is that
// prediction, whatever the card's word. The target is worked out as fg_arm_life.h's FgpRampStep works it out (RpTarget: the same calls, the law's
// memory as fg_cond.h keeps it), at a game of 60 by itself, the floor's out-rate 37:
//   (a) a card read as the limit, c MEASURED 0: no switch (the game's own period);
//   (b) an idle card, no c of its own, the limit's c 8,120 us measured: the prediction's period, as the law engages on it;
//   (c) a card read as the limit, nothing measured: the prior (1.5 times the game's own);
//   (d) an idle card, nothing measured: no switch;
//   (e) a card read as the limit, c 8,120 us with a spread of 743 (lawon's first stint): the prediction with S2's margin.
static LONG RpTarget(int aloneX10, const FgDriveMem& m, int card)
{
    int which = 0; (void)FgDriveCUs(m, card, &which);
    return (LONG)FgRampTargetUs(aloneX10, FgDrivePredHiX10(m, aloneX10, card), which != 0 ? 1 : 0, card, 370);
}
static void CaseRP7()
{
    const char* const name = "RP7 the switch's target follows the law's prediction: the prior only where nothing is measured (S1 round 2, 2026-10-18)";
    CaseBegin(name);
    const int alone = 600; const LONG own = 16667;
    FgDriveMem none; FgDriveInit(none, FgDriveDefaults());
    FgDriveMem zero = none; zero.cLimUs = 0;
    FgDriveMem lim = none; lim.cLimUs = 8120;
    FgDriveMem sd = none; sd.cLimUs = 8120; sd.cLimErrUs = 743;
    const LONG a = RpTarget(alone, zero, 1), b = RpTarget(alone, lim, 0), c = RpTarget(alone, none, 1), d = RpTarget(alone, none, 0), e = RpTarget(alone, sd, 1);
    const LONG bWant = (LONG)(10000000.0 / FgDrivePredHiX10(lim, alone, 0) + 0.5), eWant = (LONG)(10000000.0 / FgDrivePredHiX10(sd, alone, 1) + 0.5);
    printf("  a game of 60 (%ld us): (a) the limit, c measured 0: %ld us; (b) idle, the limit's c 8,120: %ld us (the prediction's %ld); (c) the limit, nothing measured: %ld us; (d) idle, nothing: %ld us; (e) the limit, c 8,120 and its spread 743: %ld us (the prediction's %ld)\n",
        own, a, b, bWant, c, d, e, eWant);
    if (labs(a - own) > 20) Err("RP7 (a): a card read as the limit with c measured 0 got the target %ld us (the game's own %ld wanted: no switch; the prior is for nothing measured)", a, own);
    if (labs(b - bWant) > 20) Err("RP7 (b): an idle card predicted with the limit's c got %ld us (the prediction's %ld wanted)", b, bWant);
    if (labs(c - 25000) > 20) Err("RP7 (c): nothing measured on a card that is the limit: %ld us (the prior's 25,000 wanted)", c);
    if (labs(d - own) > 20) Err("RP7 (d): nothing measured on an idle card: %ld us (the game's own %ld wanted)", d, own);
    if (labs(e - eWant) > 20) Err("RP7 (e): the prediction with its margin %ld us, the target %ld", eWant, e);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP8, the switch at the ENGAGING band's edge (S1 round 2, the reviewer's finding 3)
// S2's merged prediction carries c's margin, and its ENGAGING band lets the wait for a cut last down to a predicted 37 (the floor's out-rate). A
// game of 60 by itself whose prediction WITH THE MARGIN is about 37.2 (c 9,500 us and its spread 710, filed on a card that is the limit): the
// target is that prediction's period, and its drain's margin is held under the cap, THE FLOOR'S OUT-RATE'S PERIOD, so the switch holds the game
// AT 37.0 at the worst. WHAT PROTECTS THE STINT is the stint's warm-up, whose rows do not stand it down for its first warmWin windows of LAT_WIN
// presents (fg_cond.h, FgDriveWarmLift; the windows 120 frames, ivt_latency.h). HELD: the floor ends (0) before the first window behind the
// warm-up begins (warmWin x 120 presents behind the engage's first), and that window has no present the floor raised; never a period over
// the cap while it stood.
static void CaseRP8()
{
    const char* const name = "RP8 the switch at the ENGAGING band's edge: the floor at most the out-rate's period, and gone before the stint's warm-up ends (S1 round 2, 2026-10-18)";
    CaseBegin(name);
    FgDriveMem m; FgDriveInit(m, FgDriveDefaults()); m.cLimUs = 9500; m.cLimErrUs = 710;
    const int alone = 600, pred = FgDrivePredHiX10(m, alone, 1);
    const LONG target = RpTarget(alone, m, 1), cap = (LONG)FgRampCapUs(370);
    const int warmWin = FgDriveDefaults().warmWin, liftEnd = warmWin * 120;
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    RtBegin(RpLawon(1)); gRtSeed = 20261018u; s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 600; i++) RpFrame(gRlCostMs, gRlSpread);
    WriteRelease(&a.k.rampAskUs, target); WriteRelease(&a.k.rampCapUs, cap);
    s.wt.want = FG_WANT_ON;
    LONG first = 0, mostPer = 0; int gone = -1; LONG heldAtLift = -1;
    for (int i = 0; i < liftEnd + 240; i++)
    {
        RpFrame(gRlCostMs, gRlSpread);
        if (i == 0) first = a.r.rampFloorUs;
        if (a.r.rampFloorUs > 0 && s.r.periodUs > mostPer) mostPer = s.r.periodUs;
        if (gone < 0 && i > 0 && a.r.rampFloorUs == 0) gone = i;
        if (i == liftEnd) heldAtLift = a.r.rampHeld;
    }
    const LONG heldAfter = a.r.rampHeld - heldAtLift;
    s.wt.want = FG_WANT_ON; RtEnd("RP8");
    printf("  the prediction with the margin %.1f a second; the target %ld us (%.1f), the cap %ld us (%.1f); the floor at the first pair %ld us, the most period while it stood %ld us;"
           " the floor gone at the stint's present %d; the warm-up's lift %d windows, %d presents; presents the floor raised behind the lift %ld\n",
           pred / 10.0, target, 1000000.0 / target, cap, 1000000.0 / cap, first, mostPer, gone, warmWin, liftEnd, heldAfter);
    if (pred < 370 || pred > 380) Err("RP8: the case's prediction %d tenths (37.0 to 38.0 wanted: the band's edge)", pred);
    if (first > cap + 50 || mostPer > cap + 50) Err("RP8: the floor %ld us, the period while it stood %ld us, over the cap %ld (the out-rate's period)", first, mostPer, cap);
    if (gone < 0 || gone >= liftEnd || heldAfter != 0) Err("RP8: the floor gone at present %d with the lift ending at %d, %ld present(s) raised behind it (gone first, none behind, wanted)", gone, liftEnd, heldAfter);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP9, DXVK's frame latency counted by presents (S1 round 2, the reviewer's finding 4; by name alone)
// RP1 IS A FIT (the reviewer): its presenter's hold is a time (RtPar::latencyUs), and DXVK's is a count of presents (its SyncFrameLatency waits
// for the present the latency before the one being made to be shown, docs/LATENCY-RESEARCH-2026-09-23.md; a generated frame's present counts
// as a real one's). RP9 runs RP1's shape with that count (RtPar::latencyPresents 1 and no time: d3d9.maxFrameLatency 1, as on the owner's legs)
// and asks two things. (1) THE HYPOTHESIS, on today's machine (the switch's key 0): the first engage holds the render thread a frame over
// FG_RP_STALL of the game's own median, with the first pairs' handoffs retiring 20 ms or more behind their entries (lawon: the frametime
// line's 39.0 ms, the first pairs' 25 to 37). (2) THE SWITCH, judged as RP1 judges.
// HELD 2026-10-18: (1) HOLDS at the first engage: the render thread's 36.65 ms against the game's 16.70 (lawon's frametime line: 39.0), the
// first 8 pairs' handoffs 20.3 to 29.9 ms behind their entries (to 41.0 by the twelfth; lawon's 25 to 37). At the second engage today's code
// holds the render thread 25.25 ms only (lawon's 43.3 is not reproduced) and shows 39.78 ms between two presents.
// (2) THE SWITCH WITHOUT ITS DRAIN (-DFG_RP_KEYS=1): the render thread's 31.09 and 29.45 ms, within FG_RP_STALL of the stints' medians, and
// 29.42 and 35.76 ms between two shown presents: the queue a frame deep at the engage, the second engage's second pair's real frame waiting
// behind its own generation in the helper's order (shown 25.6 ms behind its retire); a wider drain's margin does not reach it (at 200
// thousandths the floor is held at the out-rate: the same 35.76). WITH THE DRAIN (fg_machine.h, RampDrainN: the pacer alone at the switch's
// floor until the queue is empty, where the card is the limit and a stint's floors stand): the second engage's render thread 25.60 ms and
// 24.41 ms between two shown presents, the handoffs at 9.6 ms from the drain's end. STILL OPEN, THE CASE FAILS ON IT (by name, not in the
// suite): THE FIRST ENGAGE's 29.42 ms between two shown presents, 1.17 of the stint's median 25.12: the helper's first pairs (its prepare)
// show no generated frame, so three real frames go out at the floor's period, the last 2.6 ms late behind the queue. A drain there moved the
// picture's latency down ahead of the engage's own step up and read worse (35.37 ms).
static void CaseRP9()
{
    const char* const name = "RP9 lawon's switch-on with DXVK's frame latency counted by presents (S1 round 2, 2026-10-18): today's machine shows the long frame; the switch judged as RP1 judges";
    CaseBegin(name);
    RtPar p0 = RpLawon(0), p1 = RpLawon(1);
    p0.latencyPresents = p1.latencyPresents = 1; p0.latencyUs = p1.latencyUs = 0;
    const double cost = gRlCostMs, spread = gRlSpread;
    const RpOut c = RpGo(p0, cost, spread, 600, 1080, 600, 1, "RP9 control");
    RpPrint("TODAY'S MACHINE, DXVK'S COUNT (the switch's key 0)", c);
    const RpOut g = RpGo(p1, cost, spread, 600, 1080, 600, 1, "RP9");
    RpPrint("THE SWITCH TO THE TARGET, DXVK'S COUNT", g);
    const double q = RpQ();
    auto ret = [&](const RpOut& o, double* lo, double* hi)
    {   *lo = 1e9; *hi = 0.0;
        for (int i = o.e1; i < o.e1 + 8 && i < (int)o.pairs.size(); i++)
            if (o.pairs[i].tRet) { const double r = (double)(o.pairs[i].tRet - o.pairs[i].tEntry) / q; if (r < *lo) *lo = r; if (r > *hi) *hi = r; } };
    double clo, chi, glo, ghi; ret(c, &clo, &chi); ret(g, &glo, &ghi);
    printf("  the first engage's first 8 pairs: their handoffs retire %.2f to %.2f ms behind their entries today, %.2f to %.2f with the switch\n", clo, chi, glo, ghi);
    const std::string cb = RpJudge("engage 1", c.on1);
    const double cref = c.on1.own > c.on1.after ? c.on1.own : c.on1.after;
    if (c.on1.longest <= cref * FG_RP_STALL || chi < 20.0)
        Err("RP9 control: today's machine under DXVK's count held the render thread %.2f ms at the first engage (%.2f allowed) with the handoffs at most %.2f ms behind (20 wanted): the hypothesis does not hold", c.on1.longest, cref * FG_RP_STALL, chi);
    else Ctl("today's machine under DXVK's count: %s", cb.c_str());
    const std::string gb = RpJudgeAll(g);
    if (!gb.empty()) Err("RP9: the switch under DXVK's count: %s", gb.c_str());
    CaseEnd(name);
}

// ---------------------------------------------------------------- RP10, fill95's switch-ons: the card's queue read directly (SW, round 3)
// THE LEG (sitting-2026-10-18b, fill95, 1455 MHz, the fill alternation): at each of the five switch-ons behind the session's first the render
// thread's longest interval was 23.7 to 25.7 ms (38.3 once) against the panel's floor of 16.9, at the panel's floor with no target asked; the
// card standing by was the limit (DXVK held the worker 4.9 to 6.9 ms of each 10 ms present: the latency lines' present block; the picture 21 to
// 25 ms old) though the law read it as idle room ("in flight 0.0"); the first pairs' handoffs retired 12 to 15 ms behind their entries against 7.5
// steady, and in each first ON window a real frame's Present was held 4.5 to 15 ms and the worker waited at the position 1 to 17 times.
// THE SHAPE, FITTED TO THAT AND SAID AS SUCH: the free card's (RlFree) with the card's work a frame deep standing by (the scene's tail 4,500 us,
// the upscale 2,500, the post chain 3,000: about 10 ms a frame), the dispatch 1,600 (the leg's 1,450 to 1,724), the fake game's CPU frame 4.5 ms
// (10 percent each way), DXVK's frame latency counted by presents at 1 (RP9's model, the worker's hold), and the product's keys of stage 3 (the
// prepare's wait behind the answer, the placement by the card: the leg's build ran both). Standing by about 91 a second, engaged 59.1.
// AT 120 Hz TODAY'S MACHINE (S1's switch: its drain behind a target alone) MUST SHOW THE LEG: the render thread's longest over FG_RP_LONG of the
// game's own median at the second engage (held 2026-10-18: 23.94 ms, 1.41 of 16.88; the leg's 1.40 to 1.52). THEN THE SWITCH THAT READS THE QUEUE
// (FG_RAMP_KEY_QREAD), at 120 and 144 Hz and at 60 Hz with a game of about 41 (the card and the CPU frame scaled by 2.2): at every switch but
// the session's first the render thread's longest within FG_RP_LONG of the game's own median and the longest gap between two shown presents not
// over today's (within FG_RP_LONG too at 120 Hz); the session's first engage, the helper's first, where nothing is drained, not over today's;
// every real frame once and in order, the generated frames of the first 50 pairs not two fewer than today's, the first 120 pairs' age and queue
// at their 95th not over today's by half a millisecond, and the queue read at both engages (the drain's presents told).
static RtPar RpFill95(ULONGLONG hz, double scale, int keys)
{
    gRlNow = RL_NOW_ALL; RtPar p = RpFree(hz, 1); gRlNow = 0;
    p.ramp = keys;
    p.tailUs = (LONG)(4500 * scale); p.upUs = (LONG)(2500 * scale); p.postUs = (LONG)(3000 * scale); p.kWorkUs = (LONG)(1600 * scale);
    p.latencyPresents = 1; p.latencyUs = 0;
    return p;
}
static void CaseRP10()
{
    const char* const name = "RP10 fill95's switch-ons (2026-10-18 evening): the card's queue a frame deep standing by; today's machine shows the long frame, the switch that reads the queue shows none, on 120, 144 and 60 Hz (round 3, SW)";
    CaseBegin(name);
    struct Pn { const char* what; ULONGLONG hz; double scale, cost; };
    const Pn pn[] = { { "120 Hz, a game of about 91", (120ull << 32) | 1ull, 1.0, 4.5 }, { "144 Hz, a game of about 91", (144ull << 32) | 1ull, 1.0, 4.5 },
                      { "60 Hz, a game of about 41", (60ull << 32) | 1ull, 2.2, 9.9 } };
    for (const Pn& x : pn)
    {
        const RpOut c = RpGo(RpFill95(x.hz, x.scale, FG_RAMP_KEY_ON | FG_RAMP_KEY_DRAIN), x.cost, 0.10, 600, 720, 360, 0, "RP10 control");
        const RpOut g = RpGo(RpFill95(x.hz, x.scale, FG_RP_KEYS), x.cost, 0.10, 600, 720, 360, 0, "RP10");
        printf("  %s:\n", x.what); RpPrint("today's machine", c); RpPrint("the switch that reads the queue", g);
        const RpSw* cs[2] = { &c.on1, &c.on2 }; double cWorst = 0.0;
        for (const RpSw* w : cs) { const double ref = w->own > w->after ? w->own : w->after; if (w->longest / ref > cWorst) cWorst = w->longest / ref; }
        if (x.hz == ((120ull << 32) | 1ull))
        {
            if (cWorst <= FG_RP_LONG) Err("RP10 control, %s: today's machine's longest interval at an engage %.2f of the game's own median (over %.2f wanted: the leg's 1.40 to 1.52): the case does not reproduce the leg", x.what, cWorst, FG_RP_LONG);
            else Ctl("%s, today's machine: the longest interval at an engage %.2f of the game's own median", x.what, cWorst);
        }
        else Ctl("%s, today's machine: the longest interval at an engage %.2f of the game's own median (told)", x.what, cWorst);
        // (THE JUDGE: every switch but the session's first, the render thread's longest within FG_RP_LONG of the game's own median, and its longest
        // shown gap within FG_RP_LONG or not over today's by half a millisecond (within FG_RP_LONG at 120 Hz, the leg's panel); THE SESSION'S FIRST ENGAGE, which is
        // the helper's first and drains nothing (fg_machine.h, RampWant), not over today's by half a millisecond, told)
        const RpSw* gs[4] = { &g.on1, &g.off1, &g.on2, &g.off2 }; const RpSw* cs4[4] = { &c.on1, &c.off1, &c.on2, &c.off2 };
        const char* nm[4] = { "engage 1", "stand-down 1", "engage 2", "stand-down 2" };
        const bool is120 = x.hz == ((120ull << 32) | 1ull);
        for (int i = 0; i < 4; i++)
        {
            const RpSw& w = *gs[i]; const RpSw& t = *cs4[i]; const double ref = w.own > w.after ? w.own : w.after;
            const bool bad = i == 0 ? (w.longest > t.longest + 0.5 || w.gap > t.gap + 0.5)
                                    : (w.longest > ref * FG_RP_LONG || (w.gap > t.gap + 0.5 && w.gap > ref * FG_RP_LONG) || (is120 && w.gap > ref * FG_RP_LONG));
            if (bad) Err("RP10, %s, %s: the render thread's longest %.2f ms and the longest shown gap %.2f ms against the game's own median %.2f (today's %.2f and %.2f)", x.what, nm[i], w.longest, w.gap, ref, t.longest, t.gap);
            else Ctl("%s, %s: the render thread's longest %.2f, the longest shown gap %.2f, of the game's own median %.2f (today %.2f and %.2f)", x.what, nm[i], w.longest / ref, w.gap / ref, ref, t.longest / ref, t.gap / ref);
        }
        if (g.order || g.real1 != g.n) Err("RP10, %s: %d picture(s) out of place (%s), %d of %d real frames shown once", x.what, g.order, g.first.c_str(), g.real1, g.n);
        if (g.on1.gen50 < c.on1.gen50 - 2 || g.on2.gen50 < c.on2.gen50 - 2 || g.age95e > c.age95e + 0.5 || g.depth95e > c.depth95e + 0.5)
            Err("RP10, %s: generated frames %d and %d of 50 (today %d and %d), the first 120 pairs' age and queue at their 95th %.2f and %.2f ms (today %.2f and %.2f): none worse wanted",
                x.what, g.on1.gen50, g.on2.gen50, c.on1.gen50, c.on2.gen50, g.age95e, g.depth95e, c.age95e, c.depth95e);
        if (g.q1 <= 0 || g.q2 <= 0) Err("RP10, %s: the queue read standing by %ld and %ld us (a block wanted: the card is the limit standing by)", x.what, g.q1, g.q2);
    }
    CaseEnd(name);
}
