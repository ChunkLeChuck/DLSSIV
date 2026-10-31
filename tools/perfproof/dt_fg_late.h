// dt_fg_late.h: THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT, IN THE HARNESS (2026-10-14; docs/reports/fg-4/brief-2026-10-13b/
// BRIEF-late-answers.md; fg_machine.h, "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT", above ArmedOverMay).
//
// Included by dt_fg_test.cpp behind dt_fg_rates.h, with ONE line; its cases are LA1 to LA8 (one RUN line in RunCases, ONCE: under the key fg_q_at_hand the CPU test stands:
// no 'Q' is written behind an injection there, so the hand knows no reason early, and each case's control shows that machine).
//
// WHAT THE LEGS OF 2026-10-13 READ (sitting-2026-10-13c): at the lock at 300 MHz 29 of 100 generated frames reached the screen.
// The helper's dispatch runs behind the game's own work on the card, its answer is late, AND ONE LATE ANSWER COST THREE
// FRAMES: its own, the next pair's (the hand found the dispatch unanswered: NO SOURCE) and the one after (fed with a reset).
// THE FAKE GAINS, for these cases alone (every case before them runs what it ran, to the number):
//   THE HELPER'S LISTS ON THE CARD (dt_fg_rates.h, RtDisp): a dispatch is work on the fake card's queue; the next frame's scene
//     stands behind a hand's retire; the game's queue outranks the helper's or not, as the case says;
//   THE BODY'S HOME (dt_fg_shadow.h, ShArmB::LaHome): a submit behind a wait for the dispatch before it, in a stage of the
//     fake's queue where a wait is an item and what follows it stands behind it. SO A COPY MADE OVER A DISPATCH IN FLIGHT
//     LANDS BEHIND THAT DISPATCH, as on the card, and the fake helper, which reads PRESENT at its dispatch's END, makes its
//     image from the right picture; with the wait left out and the fact said all the same (the control) it does not;
//   THE ORDER'S OWN FENCE (ShArmB::OrdWait, OrdSignal), in the same stage.
// THE THREE KEYS ARE HANDED IN BY THE CASE (RtPar::over, card, ord; FgArmedKeys' own), and each case runs its control with the
// key at 0, which is the machine of 2026-10-13: the numbers a leg read.
#pragma once

// The body's stand-in of a hand-stepped case: ShInjBody, and behind it the home, for an injection that was sent.
static void LaBody(const DWORD* payload, DWORD bytes)
{
    const LONG sent0 = B::sentN;
    ShInjBody(payload, bytes);
    if (B::sentN != sent0) ShArmB::LaHome();
}
// The stage of the queue switched on for a machine that was just made fresh (a hand-stepped case; the simulation's own
// cases switch it on in RtBegin).
static void LaOn()
{
    ShArmB::laOn = 1; gRing.csGate = &ShArmB::LaCsGate; gShAfterOne = &ShArmB::LaQueueRun; gInjBody = LaBody;
}
// THE TWO LOCKS' SHAPES WITH THE HELPER'S LISTS ON THE CARD: dt_fg_rates.h's (FqLock90, FqLock70) and a dispatch's own time as
// the legs' helper lines gave it (steady 2,150 to 2,200 us and 2,550; a reset's 700 and 820). scene: the next frame's
// scene work on the game's queue behind a hand's retire. THE SIMULATION HAS NO THROTTLE (a card over its pacer's period
// would run away from the fake game, where DXVK's own frame limit holds the real one), so the shapes stay under the
// period the pacer keeps: 3,500 us of scene, where the legs read about 7,000. What that leaves a leg's alone: the rate
// of a card that is over its period.
static RtPar LaLock(int lock, LONG sceneUs, int rank)
{
    RtPar p = lock == 90 ? FqLock90() : FqLock70();
    p.kWorkUs = lock == 90 ? 2200 : 2550; p.kResetUs = lock == 90 ? 700 : 820;
    p.sceneUs = sceneUs; p.rank = rank;
    return p;
}
struct LaLeg
{
    RtStat set; std::string bad, first, ordBad; int order, n, gen, real1;
    LONG late, nosrc, reset, forced, overN, overNot, waits, relAns, relDead, relPres, relHand, relStop, armedP, src, cardUps, cardDowns;
    LONG dispFirst, dispBehind, dispOrdered, ordChecked, ordLate, blitsMax, kFullN, kFullMed, kFull95;
};
static LONG LaPct(const volatile LONG* h, LONG n, int pct)
{
    if (n <= 0) return 0;
    LONG run = 0; const LONG at = (LONG)(((LONGLONG)n * pct + 99) / 100);
    for (int i = 0; i < FG_K_BINS; i++) { run += h[i]; if (run >= at) return (LONG)i * FG_K_BIN_US; }
    return (LONG)(FG_K_BINS - 1) * FG_K_BIN_US;
}
// frames of the fake game at this shape; `plant(i)` runs ahead of frame i (a case plants an answer's time there, or moves
// the want); what is counted is counted over the pairs from `from` on.
template <class F> static LaLeg LaRun(const RtPar& par, double costMs, int frames, int from, F plant, double tolE = 0.10, double perMostMs = 16.93)
{
    LaLeg g = {};
    RtBegin(par);
    const FgArmedState& a = ShArmB::a;
    LONG w0[FG_AW_N] = {}, over0 = 0;
    for (int i = 0; i < frames; i++)
    {
        if (i == from) { for (int k = 0; k < FG_AW_N; k++) w0[k] = a.w.iWhy[k]; over0 = a.w.overN; }
        plant(i);
        RtFrame(RtMs(costMs));
    }
    g.set = RtRead(from, frames);
    g.bad = RtJudgeQ(g.set, tolE, perMostMs);
    g.order = gRt.order; g.first = gRt.firstBad; g.ordBad = gRtOrdBad;
    const int n = (int)gRt.pairs.size();
    for (int i = from; i < n - 1; i++) { const RtPair& q = gRt.pairs[i]; g.n++; if (q.gen == 1) g.gen++; if (q.real == 1) g.real1++; }
    g.late = a.w.iWhy[FG_AW_LATE] - w0[FG_AW_LATE]; g.nosrc = a.w.iWhy[FG_AW_NOSOURCE] - w0[FG_AW_NOSOURCE]; g.reset = a.w.iWhy[FG_AW_RESET] - w0[FG_AW_RESET];
    g.forced = a.w.iWhy[FG_AW_FORCED] - w0[FG_AW_FORCED];
    g.overN = a.w.overN - over0; g.overNot = a.pl.overPrevNot;
    g.waits = a.w.ordWaits; g.relAns = a.pl.ordRelAnswer; g.relDead = a.pl.ordRelDeadline; g.relPres = a.w.ordRelPresent; g.relHand = a.w.ordRelHand; g.relStop = a.w.ordRelStop;
    g.armedP = a.w.ordFirstP; g.src = a.w.rideSrcN;   // (2026-10-17: the engage's first pair that asked for a wait; the pairs a wait is for)
    g.dispFirst = gRt.dispFirst; g.dispBehind = gRt.dispBehind; g.dispOrdered = gRt.dispOrdered; g.ordChecked = gRt.ordChecked; g.ordLate = gRt.ordLate;
    g.kFullN = a.pl.kFullN; g.kFullMed = LaPct(a.pl.kFull, a.pl.kFullN, 50); g.kFull95 = LaPct(a.pl.kFull, a.pl.kFullN, 95);
    RtEnd("LA");
    g.cardUps = ShArmB::cardUps; g.cardDowns = ShArmB::cardDowns; g.blitsMax = ShArmB::ordBlitsMax;
    return g;
}
static const char* LaLine(const LaLeg& g)
{
    static char bufs[8][900]; static int at = 0;
    char* const b = bufs[at++ & 7];
    snprintf(b, sizeof(bufs[0]), "of %d pairs %d generated frames shown (%.1f of 100), %d real frames shown once, pictures out of place %d; not shown: late %ld, no source %ld, fed with a reset %ld, the real frame forced first %ld;"
        " copied over a dispatch in flight %ld (withheld for the earlier answer's stamp %ld); %.1f real frames a second at a period in force of %.3f ms, the picture's age at worst %.3f ms, the real frame off the middle at worst %.3f ms;"
        " the answer over full dispatches: median %ld us, 95th %ld us (%ld pair(s)); dispatches that ran ahead of the scene %ld, behind it %ld, behind an order wait %ld;"
        " order waits %ld (released by the answer %ld, by the deadline %ld, by the worker ahead of a present %ld, at the next hand %ld, at a stop %ld; the engage's first at pair %ld; the most presents ever behind one %ld);"
        " the helper's place asked for %ld time(s), given back %ld",
        g.n, g.gen, g.n ? 100.0 * g.gen / g.n : 0.0, g.real1, g.order, g.late, g.nosrc, g.reset, g.forced, g.overN, g.overNot, g.set.fps, g.set.perMs, g.set.ageMs, g.set.maxE,
        g.kFullMed, g.kFull95, g.kFullN, g.dispFirst, g.dispBehind, g.dispOrdered, g.waits, g.relAns, g.relDead, g.relPres, g.relHand, g.relStop, g.armedP, g.blitsMax, g.cardUps, g.cardDowns);
    return b;
}
// What every leg of the machine NOW must hold, whatever it shows: each real frame once and in its place, no order wait
// released off its rule, never two presents behind one.
static void LaSound(const char* tag, const LaLeg& g)
{
    if (g.order || g.real1 != g.n) Err("%s: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", tag, g.order, g.first.c_str(), g.real1, g.n);
    if (g.ordLate || !g.ordBad.empty()) Err("%s: %ld order wait(s) were released off the rule: %s", tag, g.ordLate, g.ordBad.c_str());
    if (g.blitsMax > 1) Err("%s: %ld presents stood behind one order wait (one at the most wanted)", tag, g.blitsMax);
}

// ---------------------------------------------------------------- LA1, one late answer costs one generated frame
// WHAT IT SHOWS. The lock at 90's shape, the card free behind a hand (the helper first: K is the dispatch's 2,200 us), and
// ONE ANSWER IN TWENTY PLANTED LATE over a hundred pairs: five of them. (PLANTED AT 11 ms, the largest the legs read: this
// simulation's worker stands closer behind its card than the game's, and the legs' 9.5 ms is in before its next hand.
// What makes an answer cost three is that it is still out at the next hand, whatever its own time.)
// THE MACHINE OF 2026-10-13 (the control: `over` 0): each costs three (itself; the next pair has NO SOURCE; the one after is
// fed with a reset): 85 of 100 shown. NOW: the next pair copies over the dispatch in flight, behind the body's home on
// the queue; it dispatches, nothing is reset, and it is shown: 95 of 100, the five late ones alone.
static void CaseLA1()
{
    const char* const name = "LA1 one late answer in twenty: it costs its own generated frame and no other, 95 of 100 shown where 85 were (2026-10-14)";
    CaseBegin(name);
    RtPar par = LaLock(90, 0, 0); par.over = 1;
    auto plant = [](int i) { if (i >= 520 && (i % 20) == 0) gRt.kNext = 11000; };
    const LaLeg g = LaRun(par, 4.0, 621, 520, plant, 99.0, 99.0);
    printf("  NOW: %s\n", LaLine(g));
    LaSound("LA1", g);
    if (g.late != 5 || g.nosrc != 0 || g.reset != 0 || g.overN != 5 || g.overNot != 0) Err("LA1: late %ld, no source %ld, fed with a reset %ld, copied over a dispatch in flight %ld, withheld for the earlier stamp %ld (5, 0, 0, 5, 0 wanted)", g.late, g.nosrc, g.reset, g.overN, g.overNot);
    if (g.n != 100 || g.gen != 95) Err("LA1: %d of %d generated frames shown (95 of 100 wanted: the five late ones alone)", g.gen, g.n);
    const RingStats stc = gRingSt;
    RtPar pc = par; pc.over = 0;
    const LaLeg c = LaRun(pc, 4.0, 621, 520, plant, 99.0, 99.0);
    ViolRestore(stc);
    printf("  CONTROL, the machine of 2026-10-13 (the CPU test alone): %s\n", LaLine(c));
    printf("  OF 100 GENERATED FRAMES, one answer in twenty late: %d shown before, %d now\n", c.gen, g.gen);
    if (c.late != 5 || c.nosrc != 5 || c.reset != 5 || c.gen != 85) Err("LA1 control: with the CPU test alone late %ld, no source %ld, fed with a reset %ld and %d of %d shown (5, 5, 5 and 85 of 100 wanted): the control shows nothing", c.late, c.nosrc, c.reset, c.gen, c.n);
    Ctl("the CPU test alone: five late answers cost %d generated frames of %d (late %ld, no source %ld, fed with a reset %ld)", c.n - c.gen, c.n, c.late, c.nosrc, c.reset);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA2, a run of late answers
// WHAT IT SHOWS. The same shape with EVERY ANSWER OF THIRTY PAIRS IN A ROW AT 11 ms: a helper slow on its own account, where each
// dispatch really holds the next frame's upscale on the helper's queue. THE COPY OVER A DISPATCH IN FLIGHT IS FOR THE ANSWER THAT IS
// LATE ALONE (fg_machine.h, FG_OVER_CLEAN: eight hands in a row before it found nothing unanswered): the run's first late
// answer is copied over, once; from there the CPU test stands as it stood, so THE REAL FRAMES COME AS THEY CAME (each once, in
// order, at the rate and the age the CPU test alone gave) and no generated frame more is lost.
// CONTROL, the CPU test alone: what the run cost before. (WHAT THE RULE IS FOR, measured while it was being written, on two builds
// that are not kept: with a copy at every hand that found one dispatch unanswered, a run of five cost 10 generated
// frames where the CPU test alone lost 7; with a copy at every second such hand, a run of thirty cost the same 30 of 140
// and the real frames 7 of 100 of their rate, 47.8 a second against 51.4, the picture's age 27.7 ms against 25.5.)
static void CaseLA2()
{
    const char* const name = "LA2 a run of thirty late answers: the first is copied over and no other, the real frames come as they came and no generated frame more is lost (2026-10-14)";
    CaseBegin(name);
    RtPar par = LaLock(90, 0, 0); par.over = 1;
    auto plant = [](int i) { if (i >= 540 && i < 570) gRt.kNext = 11000; };
    const LaLeg g = LaRun(par, 4.0, 661, 520, plant, 99.0, 99.0);
    printf("  NOW: %s\n", LaLine(g));
    LaSound("LA2", g);
    const RingStats stc = gRingSt;
    RtPar pc = par; pc.over = 0;
    const LaLeg c = LaRun(pc, 4.0, 661, 520, plant, 99.0, 99.0);
    ViolRestore(stc);
    printf("  CONTROL, the CPU test alone: %s\n", LaLine(c));
    printf("  THROUGH A RUN OF THIRTY LATE ANSWERS: %d of %d generated frames shown before, %d now; %.1f real frames a second before, %.1f now; the picture's age at worst %.2f ms before, %.2f now\n", c.gen, c.n, g.gen, c.set.fps, g.set.fps, c.set.ageMs, g.set.ageMs);
    if (g.overN != 1) Err("LA2: %ld pair(s) of the run copied over a dispatch in flight (1 wanted: the first late answer, and no late answer among late answers)", g.overN);
    if (g.gen < c.gen) Err("LA2: %d of %d generated frames shown where the CPU test alone showed %d (no fewer wanted)", g.gen, g.n, c.gen);
    if (g.set.fps < c.set.fps - 0.3 || g.set.ageMs > c.set.ageMs + 2.3) Err("LA2: %.1f real frames a second and the picture's age at worst %.2f ms, where the CPU test alone gave %.1f and %.2f (the rate within 0.3 and the age no more than one copied pair's 2.2 ms over it wanted)", g.set.fps, g.set.ageMs, c.set.fps, c.set.ageMs);
    if (c.nosrc < 5 || c.reset < 5) Err("LA2 control: with the CPU test alone no source %ld, fed with a reset %ld through a run of thirty late answers (five or more of each wanted): the control shows nothing", c.nosrc, c.reset);
    Ctl("the CPU test alone through a run of thirty late answers: %d of %d generated frames shown (late %ld, no source %ld, fed with a reset %ld), the picture's age at worst %.2f ms", c.gen, c.n, c.late, c.nosrc, c.reset, c.set.ageMs);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA3, the game outranks the helper: the legs' locked state
// WHAT IT SHOWS. Both locks' shapes with the scene on the game's queue behind every hand and THE GAME FIRST: every dispatch
// runs behind the scene, K is 5.7 and 6.05 ms against a deadline of 4.5.
// THE MACHINE OF 2026-10-13 (the control, the three keys 0): the cycle of three the leg's windows 54 to 59 read (late, no source,
// a reset): NO generated frame shown.
// THE ORDER WAIT ALONE (over 1, ord 1, the helper's place not to be had: rank 1 whatever is asked): two late answers arm it (since
// 2026-10-17 nothing does: it stands from the engage's first sourced ride pair),
// from there every ride pair's dispatch has the card to itself: 97 of 100 or more, every wait released by its answer,
// each real frame once, on the screen's middle, at the pacer's period.
// THE HELPER'S PLACE (card 1, and a card that honours it: rank 2): asked once, at the first hand that rides; the helper is
// first from there, and THE ORDER WAIT IS NEVER ARMED (since 2026-10-17 it stands there too, on every sourced ride pair, each
// released by its answer, at no cost the pace shows).
// THE PACE IS HELD TO A REFERENCE, not to a number: the same shape on A CARD WHERE THE HELPER GOES FIRST BY ITSELF and nothing of
// 2026-10-14 is asked (rank 0, the three keys 0), which is the machine of 2026-10-13 with every answer in time. Its real
// frame stands off the middle of its two generated neighbours by what this shape's scene on the queue makes it (a
// present is on the screen when what was queued ahead of it has retired: the ride's own ground, and a leg's). THE HELPER'S
// PLACE GIVEN must show that pace to a tenth of a millisecond, the same rate and the same age: it is that card.
// THE ORDER WAIT PAYS FOR ITS RELEASE: 300 us from the signal to the game's queue running again, what an armed pair cost a
// frame at a desk where the game's side outranks the helper (RtPar::ordLatUs; 0.31 ms measured, 2026-10-13). The game's
// scene starts that much later, the ride's floor learns it twice over, and where that floor is the period (the lock at
// 70's shape) the rate is 2 frames a second under the reference's and a few generated frames go at the ride. WHAT IT IS
// HELD TO: 97 of 100 shown, no more than 2.5 frames a second and 3.5 ms of age over the reference, half a millisecond more
// off the middle.
struct La3Out { LaLeg ctl, ref, ord, card; };
static La3Out La3Run(int lock, double perMostMs)
{
    La3Out o;
    auto none = [](int) {};
    RtPar pc = LaLock(lock, 3500, 1);
    o.ctl = LaRun(pc, 4.0, 760, 520, none, 99.0, 99.0);
    o.ref = LaRun(LaLock(lock, 3500, 0), 4.0, 760, 520, none, 99.0, 99.0);
    RtPar po = pc; po.over = 1; po.ord = 1; po.card = 1;      // (the place is asked for and this card does not give it)
    po.ordLatUs = 300;                                        // (what an armed pair cost the frame at a desk where the game's side outranks the helper: 0.31 ms)
    o.ord = LaRun(po, 4.0, 760, 520, none, 99.0, 99.0);
    RtPar pa = LaLock(lock, 3500, 2); pa.over = 1; pa.ord = 1; pa.card = 1;
    o.card = LaRun(pa, 4.0, 760, 520, none, o.ref.set.maxE + 0.10, perMostMs);
    return o;
}
static void La3Judge(const char* tag, int lock, const La3Out& o)
{
    printf("  THE LOCK AT %d's SHAPE, the game first on the card.\n  CONTROL, the machine of 2026-10-13: %s\n  THE ORDER WAIT ALONE (the helper's place asked for and not given): %s\n  THE HELPER'S PLACE GIVEN: %s\n",
        lock, LaLine(o.ctl), LaLine(o.ord), LaLine(o.card));
    printf("  OF 100 GENERATED FRAMES at the lock at %d's shape with the game first: %.1f shown before; %.1f with the order wait alone (%.1f real frames a second, the picture's age at worst %.2f ms; before: %.1f and %.2f); %.1f with the helper's place given (%.1f and %.2f)\n",
        lock, 100.0 * o.ctl.gen / o.ctl.n, 100.0 * o.ord.gen / o.ord.n, o.ord.set.fps, o.ord.set.ageMs, o.ctl.set.fps, o.ctl.set.ageMs, 100.0 * o.card.gen / o.card.n, o.card.set.fps, o.card.set.ageMs);
    printf("  THE REFERENCE, a card where the helper goes first by itself, the three keys 0: %s\n", LaLine(o.ref));
    LaSound(tag, o.ord); LaSound(tag, o.card);
    if (o.ref.gen != o.ref.n) Err("%s reference: on a card where the helper goes first %d of %d generated frames were shown at the lock at %d's shape (all wanted): the reference is none", tag, o.ref.gen, o.ref.n, lock);
    if (o.ord.set.fps < o.ref.set.fps - 2.5 || o.card.set.fps < o.ref.set.fps - 0.1) Err("%s: %.1f real frames a second with the order wait and %.1f with the helper's place given, where the reference ran at %.1f (no more than 2.5 and 0.1 under it wanted)", tag, o.ord.set.fps, o.card.set.fps, o.ref.set.fps);
    if (o.ord.set.ageMs > o.ref.set.ageMs + 3.5 || o.card.set.ageMs > o.ref.set.ageMs + 0.05) Err("%s: the picture's age at worst %.2f ms with the order wait and %.2f with the helper's place given, where the reference read %.2f (no more than 3.5 and 0.05 over it wanted)", tag, o.ord.set.ageMs, o.card.set.ageMs, o.ref.set.ageMs);
    if (o.ord.set.maxE > o.ref.set.maxE + 0.50) Err("%s: with the order wait a real frame stood %.3f ms off the middle of its neighbours where the reference's worst is %.3f (no more than half a millisecond over it wanted)", tag, o.ord.set.maxE, o.ref.set.maxE);
    if (o.ctl.gen * 20 > o.ctl.n) Err("%s control: with the three keys at 0 %d of %d generated frames were shown at the lock at %d's shape (under 5 of 100 wanted: the legs' locked state): the control shows nothing", tag, o.ctl.gen, o.ctl.n, lock);
    if (o.ord.gen * 100 < o.ord.n * 97) Err("%s: with the order wait %d of %d generated frames were shown at the lock at %d's shape (97 of 100 wanted)", tag, o.ord.gen, o.ord.n, lock);
    if (!o.ord.armedP || o.ord.relDead || o.ord.relPres || o.ord.relStop > 1 || o.ord.waits != o.ord.src) Err("%s: the order wait's first at pair %ld; released by the deadline %ld, by the worker ahead of a present %ld, at a stop %ld; waits %ld of %ld sourced ride pairs (a pair, 0, 0, one at the most, one each wanted)", tag, o.ord.armedP, o.ord.relDead, o.ord.relPres, o.ord.relStop, o.ord.waits, o.ord.src);
    if (o.ord.cardUps < 1 || o.card.cardUps < 1) Err("%s: the helper's place was asked for %ld and %ld time(s) (once an engage wanted)", tag, o.ord.cardUps, o.card.cardUps);
    if (o.card.gen * 100 < o.card.n * 97) Err("%s: with the helper's place given %d of %d generated frames were shown at the lock at %d's shape (97 of 100 wanted)", tag, o.card.gen, o.card.n, lock);
    if (!o.card.bad.empty()) Err("%s: the helper's place given, at the lock at %d's shape: %s", tag, lock, o.card.bad.c_str());
    // (2026-10-17: the order wait stands on every sourced ride pair there too, armed by nothing; the answers are in time, so each is released by
    // its answer, and the pace above, held to the reference's to a tenth of a millisecond, says what it costs there: nothing seen)
    if (o.card.waits != o.card.src || o.card.relDead || o.card.relPres) Err("%s: with the helper's place given %ld order wait(s) of %ld sourced ride pairs, released by the deadline %ld, by the worker ahead of a present %ld (one each, 0 and 0 wanted)", tag, o.card.waits, o.card.src, o.card.relDead, o.card.relPres);
}
static void CaseLA3()
{
    const char* const name = "LA3 the game outranks the helper at both locks' shapes: no generated frame before; 97 of 100 with the order wait alone, and with the helper's place given (2026-10-14)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const La3Out a = La3Run(90, 16.93);
    const La3Out b = La3Run(70, 17.80);
    ViolRestore(stc);
    La3Judge("LA3", 90, a);
    La3Judge("LA3", 70, b);
    if (a.ord.set.fps < 58.9 || a.card.set.fps < 58.9) Err("LA3: at the lock at 90's shape the game ran at %.1f and %.1f real frames a second (the panel's 59.1 wanted in both)", a.ord.set.fps, a.card.set.fps);
    Ctl("the three keys at 0: %d of %d generated frames shown at the lock at 90's shape and %d of %d at the lock at 70's (late %ld and %ld, no source %ld and %ld, fed with a reset %ld and %ld)",
        a.ctl.gen, a.ctl.n, b.ctl.gen, b.ctl.n, a.ctl.late, b.ctl.late, a.ctl.nosrc, b.ctl.nosrc, a.ctl.reset, b.ctl.reset);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA4, the copy over a dispatch in flight stands behind the home
// WHAT IT SHOWS, by hand, THE HELPER HELD INSIDE PAIR A's DISPATCH and all of frame B made behind it: B's injection (its body's
// home is queued behind the wait for A's answer), B's hand, B's Present. WHILE THE HELPER STANDS THERE: the hand has found
// A unanswered and copied all the same ('G' says DISPATCH, nothing is reset), AND THE HELPER'S PRESENT STILL HOLDS A's PICTURE:
// the hand's span stands on the queue behind the home. RELEASED: A's image is made from A's picture, then the span lands,
// and B's image is made from B's, with no reset.
// THE PATHS THAT KEEP THE CPU TEST, one run each, B found with NO SOURCE: a pipelined frame; a body that made no home; a hand
// that does not ride; a camera cut; the key at 0.
// CONTROL (a run): THE FACT SAID AND THE WAIT NOT QUEUED (ShArmB::homeLie). The machine copies, as it was told it may; the
// copy lands while the helper reads: A's image is made from B's picture, and the fake counts a write of the helper's set
// under its dispatch.
struct La4Leg { bool held, armedA; DWORD tagA, tagB, presHeld, fromA, fromB; LONG nA, nB, over, kindB, whyB, resetB, heldItems, busyWrites, prevB; int shownAWhile; };
static La4Leg La4Run(int path)   // 0 the order holds; 1 pipelined; 2 no home; 3 no ride; 4 a cut; 5 the key at 0; 6 THE CONTROL, the fact said and no wait
{
    La4Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = path == 3 ? 0 : 100000;   // (a room floor over any period: every hand of the want ON rides)
    a.k.over = path == 5 ? 0 : 1;
    LaOn();
    ShWarm<MA>(); MgDueN();
    for (int i = 0; i < 4; i++) { ShFrame<MA>(); MgDueN(); }  // (ten clean hands behind the engage: FG_OVER_CLEAN asks for eight)
    const LONG busy0 = gRingSt.inBusy;
    _InterlockedExchange(&gH.holdDispatch, 1);
    g.tagA = ShFrame<MA>(true, false); g.nA = x.w.lastQN;
    g.held = WaitFor([] { return gH.inDispatch != 0; }, 3000);
    g.armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    if (path == 1) ShB::pipelined = 1;
    if (path == 2) ShArmB::homeOff = 1;
    if (path == 6) ShArmB::homeLie = 1;
    if (path == 4) B::camJumps++;
    const LONG over0 = a.w.overN;
    g.tagB = ShFrame<MA>(true, false); g.nB = x.w.lastQN;
    // THE HELPER STILL HELD IN A's DISPATCH: what has the hand of B done, and what does the helper's PRESENT hold?
    g.over = a.w.overN - over0;
    { const std::vector<RingPipe> log = RingPipeLog(); g.kindB = ShKindOf(log, g.nB); g.whyB = (LONG)ShGWhy(log, g.nB); }
    g.presHeld = RingTag(RingRig(FG_IMG_PRESENT));
    g.heldItems = (LONG)ShArmB::laQ.size();
    g.shownAWhile = MgTagCount(shown0, g.tagA);
    { FgArmedPair rec = {}; if (MgPair(MgP(), &rec)) g.prevB = rec.prev; }
    ShRelease();
    if (!ShSettle()) Err("LA4: the fake helper did not come back behind its release");
    Drain();
    ShB::pipelined = 0; ShArmB::homeOff = 0; ShArmB::homeLie = 0;
    for (const ShDisp& d : ShDispLog()) { if (d.n == g.nA) g.fromA = d.from; if (d.n == g.nB) { g.fromB = d.from; g.resetB = d.reset ? 1 : 0; } }
    g.busyWrites = gRingSt.inBusy - busy0;
    MgDueN();
    if (path == 6) { RingStats st = gRingSt; st.inBusy = busy0; ViolRestore(st); }
    MgClean("LA4");
    MgEnd();
    return g;
}
static void CaseLA4()
{
    const char* const name = "LA4 the copy over a dispatch in flight stands behind the home, the helper held in that dispatch; every path with no home behind the helper keeps NO SOURCE (2026-10-14)";
    CaseBegin(name);
    const La4Leg g = La4Run(0);
    printf("  THE ORDER HOLDS: the helper held in pair %ld's dispatch %s (the pair armed %s); pair %ld's hand copied over it %ld time(s), its 'G' says %s with reasons %ld, its record names pair %ld;"
           " WHILE HELD the helper's PRESENT held picture 0x%lX (A's is 0x%lX, B's 0x%lX), %ld item(s) stood on the queue, A's real frame was on the screen %d time(s); RELEASED: A's image was made from 0x%lX, B's from 0x%lX (a reset: %ld); writes of the set under a dispatch %ld\n",
        g.nA, g.held ? "yes" : "NO", g.armedA ? "yes" : "NO", g.nB, g.over, ShKindName((int)g.kindB), g.whyB, g.prevB, g.presHeld, g.tagA, g.tagB, g.heldItems, g.shownAWhile, g.fromA, g.fromB, g.resetB, g.busyWrites);
    if (!g.held || !g.armedA) Err("LA4: the leg did not reach its hold (the helper in A's dispatch %d, A armed %d)", (int)g.held, (int)g.armedA);
    if (g.over != 1 || g.kindB != (LONG)FG_G_DISPATCH || g.whyB != 0 || g.prevB != g.nA) Err("LA4: B's hand copied over the dispatch %ld time(s), its 'G' says %s with reasons %ld, its record names pair %ld (1, DISPATCH, 0 and pair %ld wanted)", g.over, ShKindName((int)g.kindB), g.whyB, g.prevB, g.nA);
    if (g.presHeld != g.tagA || g.heldItems < 2) Err("LA4: with the helper held in A's dispatch its PRESENT held 0x%lX and %ld item(s) stood on the queue (A's picture 0x%lX, and the home's wait with the span behind it, wanted)", g.presHeld, g.heldItems, g.tagA);
    if (g.fromA != g.tagA || g.fromB != g.tagB || g.resetB || g.busyWrites) Err("LA4: A's image was made from 0x%lX and B's from 0x%lX (a reset: %ld), with %ld write(s) of the set under a dispatch (0x%lX, 0x%lX, no reset and 0 wanted)", g.fromA, g.fromB, g.resetB, g.busyWrites, g.tagA, g.tagB);
    static const char* const kPath[7] = { "", "a pipelined frame", "a body that made no home", "a hand that does not ride", "a camera cut", "the key at 0", "" };
    for (int p = 1; p <= 5; p++)
    {
        const La4Leg q = La4Run(p);
        printf("  %s: B's hand copied over the dispatch %ld time(s), its 'G' says %s; A's image made from 0x%lX (A's picture: %s)\n", kPath[p], q.over, ShKindName((int)q.kindB), q.fromA, q.fromA == q.tagA ? "yes" : "NO");
        if (!q.held || q.over != 0 || q.kindB != (LONG)FG_G_NOSOURCE || q.fromA != q.tagA || q.busyWrites) Err("LA4, %s: the helper held %d; B's hand copied over the dispatch %ld time(s), its 'G' says %s, A's image was made from 0x%lX, writes under a dispatch %ld (held, 0, NOSOURCE, A's own picture and 0 wanted)", kPath[p], (int)q.held, q.over, ShKindName((int)q.kindB), q.fromA, q.busyWrites);
    }
    const La4Leg c = La4Run(6);
    printf("  CONTROL, the fact said and the wait not queued: B's hand copied over the dispatch %ld time(s); while the helper was held its PRESENT held 0x%lX (B's picture: %s); A's image was made from 0x%lX; writes of the set under a dispatch %ld\n",
        c.over, c.presHeld, c.presHeld == c.tagB ? "yes" : "NO", c.fromA, c.busyWrites);
    if (c.over != 1 || c.presHeld != c.tagB || c.fromA != c.tagB || c.busyWrites < 1) Err("LA4 control: with no wait behind the fact B's hand copied %ld time(s), PRESENT held 0x%lX under the held dispatch, A's image was made from 0x%lX and the fake counted %ld write(s) under a dispatch (1, B's picture twice and one or more wanted): the control shows nothing", c.over, c.presHeld, c.fromA, c.busyWrites);
    Ctl("the fact said with no wait queued: the copy landed under the held dispatch, A's image was made from B's picture, and the fake counted %ld write(s) of the set under a dispatch", c.busyWrites);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA5, the order wait's two releasers, each with the other held
// WHAT IT SHOWS, by hand, the order wait asked for on every ride pair (ord 2) and THE HELPER HELD IN THE PAIR'S DISPATCH, so no
// answer comes: a wait stands behind pair A's hand, and A SUBMIT IS MADE BEHIND IT (a note of the harness's), which the queue holds.
//   THE WORKER HELD, THE PLACER FREE, a command queued behind the held worker. The placer, stepped from another thread: one
//     tick before ordMaxUs behind the handoff's retire NOTHING is released; at it THE WAIT IS RELEASED, by the deadline, and
//     the submit behind it runs, with the worker still held. And with the answer in (the helper let go) before the
//     deadline: released by the answer.
//   THE PLACER AND THE ANSWER'S STAMPER HELD (never stepped), THE WORKER FREE (2026-10-17, THE LATCH'S LEG, the evening of 2026-10-14's
//     event by hand): A's generated frame shown, its real frame fired behind B's body, the SECOND present behind A's wait; then the
//     hands of B and C. NOW the worker releases A's wait at the generated frame's own fire, where it read the answer, the second
//     present finds nothing standing, and B's and C's waits are queued: three in all. ON THE MACHINE OF BEFORE THAT DAY the same leg
//     read one release ahead of the second present and one wait in all: the latch, from there to the engage's end.
// CONTROLS: the worker's releases switched off (a binding whose OrdSignal does nothing on the worker): two presents stand behind one
// wait, which the fake counts; AND THE LATCH PUT BACK (a binding whose queue takes no wait once the worker released one): one wait in
// all, the rest not taken, which the check of the waits behind sees.
static volatile LONG gLa5Sub = 0;
static void La5Submit(DWORD, DWORD, DWORD)
{
    if (ShArmB::CbTake(1)) { ShArmB::QLock(); ShArmB::RecBegin(); ShArmB::RecPatch(FG_IMG_SAVE); ShArmB::RecSubmit(); ShArmB::QUnlock(); _InterlockedIncrement(&gLa5Sub); }
}
struct La5Look { LONGLONG tS; LONG ordMaxUs; int mode; LONG before, at, heldBefore, heldAt, relDead, relAns; bool workerHeld; };
static La5Look gLa5;
static void La5LookFn()
{
    La5Look& l = gLa5; const FgArmedState& a = ShArmB::a;
    l.workerHeld = gGateIn != 0;
    if (l.mode == 0)
    {   // the deadline: one tick before it, then at it
        B::clockNow = l.tS + MA::UsToTicks(l.ordMaxUs) - 1; MgPlace<MA>();
        l.before = a.w.ordSig; l.heldBefore = (LONG)ShArmB::laQ.size();
        B::clockNow = l.tS + MA::UsToTicks(l.ordMaxUs); MgPlace<MA>();
    }
    else
    {   // the answer, well before the deadline
        B::clockNow = l.tS + MA::UsToTicks(1000); MgPlace<MA>();
        l.before = a.w.ordSig; l.heldBefore = (LONG)ShArmB::laQ.size();
        ShRelease();
        WaitFor([] { return B::TlValue(FG_TL_OUT) >= 2ull * (ULONGLONG)ShB::x.w.lastQN; }, 3000);
        MgPlace<MA>();
    }
    l.at = a.w.ordSig; l.heldAt = (LONG)ShArmB::laQ.size();
    l.relDead = a.pl.ordRelDeadline; l.relAns = a.pl.ordRelAnswer;
}
struct La5Leg { bool held; LONG nA, waits, sub; La5Look look; LONG relHand, relPres, relFire, waits2, refused, blitsMax; int realA, realB, genB, realC; };
static void La5Fresh(LONG ord)
{
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.over = 1; a.k.ord = ord; a.k.ordMaxUs = 4500;
    LaOn();
    ShWarm<MA>(); MgDueN();
}
static La5Leg La5WorkerHeld(int mode)
{
    La5Leg g = {};
    La5Fresh(2);
    FgShadowState& x = ShB::x; const FgArmedState& a = ShArmB::a;
    const LONG w0 = a.w.ordWaits;
    _InterlockedExchange(&gH.holdDispatch, 1);
    ShFrame<MA>(true, false); g.nA = x.w.lastQN;
    g.held = WaitFor([] { return gH.inDispatch != 0; }, 3000);
    g.waits = a.w.ordWaits - w0;
    MgPlace<MA>();                                            // (the pair is timed: its handoff's retire has its stamp)
    const FgArmedLive* L = MgLive(MgP());
    gLa5 = La5Look(); gLa5.mode = mode; gLa5.ordMaxUs = a.k.ordMaxUs; gLa5.tS = L ? L->tS : B::clockNow;
    gLa5Sub = 0;
    if (!Run(La5Submit)) Err("LA5: the submit's note was refused");
    Drain();
    g.sub = gLa5Sub;
    // THE WORKER HELD AT THE GATE, A COMMAND QUEUED BEHIND IT, and the placer stepped from the look's thread
    if (!MgHeld([] { D->ColorFill(gOtherRt, nullptr, 0xFF112233u); }, &La5LookFn)) Err("LA5: the held leg did not run");
    g.look = gLa5;
    if (mode == 0) ShRelease();
    if (!ShSettle()) Err("LA5: the fake helper did not come back");
    MgDueN();
    g.blitsMax = ShArmB::ordBlitsMax;
    MgClean("LA5");
    MgEnd();
    return g;
}
struct La5NoRelB : ShArmB { static void OrdSignal(LONG n) { if (GetCurrentThreadId() == dtWorkerTid) return; ShArmB::OrdSignal(n); } };
typedef FgMachine<La5NoRelB> MLaNoRel;
// THE LATCH PUT BACK BY THE BINDING (2026-10-17, the latch's control): from the worker's first release of an order wait, of any kind, the
// queue takes none again: what the machine's ordOff did until 2026-10-17 ("no more of them in this engage"), whoever released.
struct La5LatchB : ShArmB
{
    static inline LONG latched = 0;
    static void OrdSignal(LONG n) { if (GetCurrentThreadId() == dtWorkerTid) latched = 1; ShArmB::OrdSignal(n); }
    static bool OrdWait(LONG n) { if (latched) return false; return ShArmB::OrdWait(n); }
};
typedef FgMachine<La5LatchB> MLaLatch;
// (2026-10-17) THE PLACER AND THE ANSWER'S STAMPER NEVER STEP, THE WORKER FREE: the evening of 2026-10-14's event by hand. Pair A's hand
// queues its wait; the helper answers at once; the generated frame is shown at the harness's due (the worker reads 2n itself at its
// fire); pair B's injection fires A's real frame behind its body, the SECOND present behind A's wait; then the hands of B and C.
template <class MM> static La5Leg La5PlacerHeld()
{
    La5Leg g = {};
    MgFresh<MM>();
    FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.over = 1; a.k.ord = 1; a.k.ordMaxUs = 4500; a.k.ordAtStamp = 0;
    LaOn();
    ShWarm<MM>(); MgDueN();
    La5LatchB::latched = 0;                                   // (the control's latch counts from the leg's first pair: nothing of the run-in's)
    const size_t shown0 = MgShownN();
    const LONG w0 = a.w.ordWaits, f0 = a.w.ordRelFire, p0 = a.w.ordRelPresent, h0 = a.w.ordRelHand, r0 = a.w.ordRefused;
    const DWORD tagA = ShFrame<MM>(); g.nA = x.w.lastQN;
    g.waits = a.w.ordWaits - w0;
    MgDueShow();
    const DWORD tagB = ShFrame<MM>();
    g.relPres = a.w.ordRelPresent - p0; g.relFire = a.w.ordRelFire - f0;
    MgDueShow(); MgDueN();
    const DWORD tagC = ShFrame<MM>(); MgDueShow(); MgDueN();
    g.waits2 = a.w.ordWaits - w0; g.relHand = a.w.ordRelHand - h0; g.refused = a.w.ordRefused - r0;
    const auto sh = Shown();
    for (size_t i = shown0; i < sh.size(); i++) { g.realA += sh[i].tag == tagA; g.realB += sh[i].tag == tagB; g.realC += sh[i].tag == tagC; g.genB += sh[i].tag == ShGenTag(tagA, false); }
    g.blitsMax = ShArmB::ordBlitsMax;
    ShArmB::OrdSignal(0x7FFFFF);                              // (whatever a control left standing runs before the case's end)
    Drain();
    MgClean("LA5");
    MgEnd();
    return g;
}
static void CaseLA5()
{
    const char* const name = "LA5 the order wait's releasers: the placer with the worker held (the deadline, the answer); the worker with the placer and the stamper never stepped (at the generated frame's fire), and the waits behind it stand (2026-10-14; the latch's leg 2026-10-17)";
    CaseBegin(name);
    const La5Leg d = La5WorkerHeld(0);
    printf("  THE WORKER HELD, THE PLACER FREE, the helper held in the dispatch: order waits queued %ld, a submit made behind it %ld; the worker held at the look %s; one tick before the deadline the fence stood at %ld of pair %ld with %ld item(s) on the queue;"
           " at the deadline at %ld with %ld (released by the deadline %ld, by the answer %ld)\n", d.waits, d.sub, d.look.workerHeld ? "yes" : "NO", d.look.before, d.nA, d.look.heldBefore, d.look.at, d.look.heldAt, d.look.relDead, d.look.relAns);
    if (!d.held || d.waits != 1 || d.sub != 1 || !d.look.workerHeld) Err("LA5: the leg did not reach its hold (the helper held %d, waits queued %ld, the submit behind it %ld, the worker held at the look %d)", (int)d.held, d.waits, d.sub, (int)d.look.workerHeld);
    if (d.look.before >= d.nA || d.look.heldBefore < 2 || d.look.at != d.nA || d.look.heldAt != 0 || d.look.relDead != 1 || d.look.relAns != 0)
        Err("LA5: one tick before the deadline the fence stood at %ld with %ld item(s) held; at it at %ld with %ld; released by the deadline %ld, by the answer %ld (under pair %ld with two or more; pair %ld with none; 1 and 0 wanted)", d.look.before, d.look.heldBefore, d.look.at, d.look.heldAt, d.look.relDead, d.look.relAns, d.nA, d.nA);
    const La5Leg w = La5WorkerHeld(1);
    printf("  THE SAME WITH THE ANSWER LET IN 1 ms behind the retire: before it the fence at %ld with %ld item(s) held; behind it at %ld with %ld (released by the answer %ld, by the deadline %ld), the worker held %s\n",
        w.look.before, w.look.heldBefore, w.look.at, w.look.heldAt, w.look.relAns, w.look.relDead, w.look.workerHeld ? "yes" : "NO");
    if (!w.held || !w.look.workerHeld || w.look.before >= w.nA || w.look.at != w.nA || w.look.heldAt != 0 || w.look.relAns != 1 || w.look.relDead != 0)
        Err("LA5: with the answer in before the deadline the fence went from %ld to %ld, %ld item(s) left, released by the answer %ld, by the deadline %ld (under pair %ld, pair %ld, 0, 1 and 0 wanted)", w.look.before, w.look.at, w.look.heldAt, w.look.relAns, w.look.relDead, w.nA, w.nA);
    // (2026-10-17) THE LATCH'S LEG: the worker releases A's wait itself, and the waits of B and C are queued all the same. On the machine of
    // before 2026-10-17 this leg read: released ahead of the second present 1, at the generated frame's fire 0, waits in all 1 (the latch)
    const La5Leg p = La5PlacerHeld<MA>();
    printf("  THE PLACER AND THE ANSWER'S STAMPER NEVER STEPPED, THE WORKER FREE: pair A's wait queued (%ld); its generated frame shown, then its real frame fired behind the next body: released by the worker at the generated frame's own fire %ld time(s), ahead of the second present %ld;"
           " THE WAITS OF THE PAIRS BEHIND: %ld queued in all (A's, B's and C's), not taken %ld, released at the next hand %ld; A's real frame on the screen %d time(s), its generated frame %d, B's real frame %d, C's %d; the most presents ever behind one wait %ld\n",
           p.waits, p.relFire, p.relPres, p.waits2, p.refused, p.relHand, p.realA, p.genB, p.realB, p.realC, p.blitsMax);
    if (p.waits != 1 || p.relFire != 1 || p.relPres != 0 || p.waits2 != 3 || p.refused != 0 || p.realA != 1 || p.realB != 1 || p.realC != 1 || p.genB != 1 || p.blitsMax > 1)
        Err("LA5: with the placer and the stamper never stepped: waits %ld, released at the generated frame's fire %ld, ahead of the second present %ld, waits in all %ld, not taken %ld, A's real frame %d, B's %d, C's %d, A's generated frame %d, presents behind one wait at the most %ld (1, 1, 0, 3, 0, 1, 1, 1, 1 and one at the most wanted)",
            p.waits, p.relFire, p.relPres, p.waits2, p.refused, p.realA, p.realB, p.realC, p.genB, p.blitsMax);
    const RingStats stc = gRingSt;
    const La5Leg c = La5PlacerHeld<MLaNoRel>();
    const La5Leg l = La5PlacerHeld<MLaLatch>();
    ViolRestore(stc);
    printf("  CONTROL, the worker's releases switched off: the most presents behind one wait %ld; A's real frame on the screen %d time(s) before the case's own release\n", c.blitsMax, c.realA);
    printf("  CONTROL, THE LATCH PUT BACK (the binding takes no wait again once the worker released one): waits queued in all %ld, not taken by the queue %ld\n", l.waits2, l.refused);
    if (c.blitsMax < 2) Err("LA5 control: with the worker's releases off %ld present(s) stood behind one wait at the most (two or more wanted): the control shows nothing", c.blitsMax);
    if (l.waits2 != 1 || l.refused < 2) Err("LA5 control: with the latch put back %ld wait(s) were queued in all and %ld not taken (one, and two or more, wanted): the control shows nothing", l.waits2, l.refused);
    Ctl("the worker's releases off: %ld presents stood behind one order wait; the latch put back: %ld wait(s) in all, %ld not taken", c.blitsMax, l.waits2, l.refused);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA6, nothing stands where the machine stops
// WHAT IT SHOWS, by hand, a wait standing behind pair A's hand with the helper held in A's dispatch and a submit on the queue
// behind the wait, and then, one run each: THE MACHINE IS STOPPED (from the harness's thread, as the transport's stop comes
// from any); A DISARM; A DEVICE RESET'S BARRIER (BeforeReset: its own wait for the binding's submits to retire must not
// find one behind the wait: it returns at once and clean); THE HELPER IS GONE (its fences at the top: the placer releases
// by the answer's rule). Each time: the fence at A, nothing held, and the helper's place given back where the engage ends.
struct La6Leg { bool held; LONG nA, sig, heldQ, relStop, relAns, cardNow, cardDowns, rc; DWORD ms; };
static La6Leg La6Run(int how)   // 0 Stop; 1 a disarm; 2 BeforeReset; 3 the helper gone
{
    La6Leg g = {};
    La5Fresh(2);
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    a.k.card = 1;
    _InterlockedExchange(&gH.holdDispatch, 1);
    ShFrame<MA>(true, false); g.nA = x.w.lastQN;
    g.held = WaitFor([] { return gH.inDispatch != 0; }, 3000);
    MgPlace<MA>();
    gLa5Sub = 0;
    if (!Run(La5Submit)) Err("LA6: the submit's note was refused");
    Drain();
    const DWORD t0 = GetTickCount();
    if (how == 0) MA::Stop("the harness: a stop with an order wait standing");
    else if (how == 1) { DtDisarm("the harness: a disarm with an order wait standing"); }
    else if (how == 2) g.rc = MA::BeforeReset(false, "the harness");
    else { ShHelperGone(); WaitFor([] { return gH.topped != 0; }, 3000); MgPlace<MA>(); }
    g.ms = GetTickCount() - t0;
    g.sig = a.w.ordSig; g.heldQ = (LONG)ShArmB::laQ.size(); g.relStop = a.w.ordRelStop; g.relAns = a.pl.ordRelAnswer;
    g.cardNow = ShArmB::cardNow; g.cardDowns = ShArmB::cardDowns;
    if (how != 3) { ShRelease(); ShSettle(); }
    Drain();
    MgEnd();
    return g;
}
static void CaseLA6()
{
    const char* const name = "LA6 no order wait stands where the machine stops, is disarmed, meets a Reset or loses its helper (2026-10-14)";
    CaseBegin(name);
    static const char* const kHow[4] = { "a stop", "a disarm", "a Reset's barrier", "the helper gone" };
    const RingStats stc = gRingSt;
    for (int h = 0; h < 4; h++)
    {
        const La6Leg g = La6Run(h);
        printf("  %s, with a wait standing behind pair %ld and a submit behind the wait: the fence at %ld, %ld item(s) left on the queue, released at a stop %ld, by the answer's rule %ld, in %lu ms%s; the helper's place stands asked for: %s\n",
            kHow[h], g.nA, g.sig, g.heldQ, g.relStop, g.relAns, (unsigned long)g.ms, h == 2 ? Fmt(" (the barrier said %ld)", g.rc) : "", g.cardNow ? "YES" : "no");
        if (!g.held || g.sig != g.nA || g.heldQ != 0) Err("LA6, %s: the helper held %d; the fence at %ld of pair %ld, %ld item(s) left on the queue (held, the pair's own number and 0 wanted)", kHow[h], (int)g.held, g.sig, g.nA, g.heldQ);
        if (h < 3 && g.relStop != 1) Err("LA6, %s: released at a stop %ld time(s) (1 wanted)", kHow[h], g.relStop);
        if (h == 3 && g.relAns != 1) Err("LA6, the helper gone: released by the answer's rule %ld time(s) (1 wanted)", g.relAns);
        if (h == 2 && (g.rc != 0 || g.ms > 1500)) Err("LA6, a Reset's barrier: it said %ld after %lu ms (0, at once, wanted): its wait found a submit behind the order wait", g.rc, (unsigned long)g.ms);
        if (h < 3 && g.cardNow) Err("LA6, %s: the helper's place still stands asked for", kHow[h]);
    }
    ViolRestore(stc);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA7, nothing arms the order wait and nothing switches it off
// (2026-10-17: until that day LA7 held the arming by two late answers inside 120 pairs and the latch of three releases at the deadline in a
// row. On the machine of before that day its leg below, every answer late for 40 pairs, gave up and queued 39 waits in all of 419 pairs.)
// WHAT IT SHOWS. The lock at 90's shape, the card free behind a hand (the answers in time), ord 1:
//   NO LATE ANSWER AT ALL: a wait on every ride pair that dispatched with a source, from the engage's first, each released by its
//     answer; none at the deadline;
//   EVERY ANSWER LATE FOR 40 PAIRS (planted at 9.5 ms, over the 4.5 ms deadline): those waits end at the deadline, as many as there
//     were, and THE WAITS OF THE PAIRS BEHIND THE RUN STAND AS BEFORE IT: one on every sourced ride pair, released by its answer.
// CONTROL: the same run with no order wait (ord 0): the check of the waits behind the run sees none.
static LONG gLa7W = 0, gLa7S = 0;
static void CaseLA7()
{
    const char* const name = "LA7 nothing arms the order wait and nothing switches it off: a wait on every sourced ride pair, through a run of answers late for the deadline and behind it (2026-10-17)";
    CaseBegin(name);
    RtPar par = LaLock(90, 0, 0); par.over = 1; par.ord = 1;
    const RingStats stc = gRingSt;
    auto at440 = [](int i) { if (i == 440) { gLa7W = ShArmB::a.w.ordWaits; gLa7S = ShArmB::a.w.rideSrcN; } };
    gLa7W = gLa7S = 0;
    const LaLeg none = LaRun(par, 4.0, 700, 280, at440, 99.0, 99.0);
    const LONG nW = none.waits - gLa7W, nS = none.src - gLa7S;
    gLa7W = gLa7S = 0;
    const LaLeg run = LaRun(par, 4.0, 700, 280, [](int i) { if (i == 440) { gLa7W = ShArmB::a.w.ordWaits; gLa7S = ShArmB::a.w.rideSrcN; } if (i >= 400 && i < 440) gRt.kNext = 9500; }, 99.0, 99.0);
    const LONG rW = run.waits - gLa7W, rS = run.src - gLa7S;
    RtPar pc = par; pc.ord = 0;
    gLa7W = gLa7S = 0;
    const LaLeg ctl = LaRun(pc, 4.0, 700, 280, [](int i) { if (i == 440) { gLa7W = ShArmB::a.w.ordWaits; gLa7S = ShArmB::a.w.rideSrcN; } if (i >= 400 && i < 440) gRt.kNext = 9500; }, 99.0, 99.0);
    const LONG cW = ctl.waits - gLa7W, cS = ctl.src - gLa7S;
    ViolRestore(stc);
    printf("  NO LATE ANSWER: %s; sourced ride pairs %ld\n  EVERY ANSWER LATE FOR 40 PAIRS: %s; sourced ride pairs %ld; BEHIND THE RUN: waits %ld of %ld sourced ride pairs\n  CONTROL, no order wait: behind the run waits %ld of %ld sourced ride pairs\n",
        LaLine(none), none.src, LaLine(run), run.src, rW, rS, cW, cS);
    LaSound("LA7", none); LaSound("LA7", run);
    if (none.src < 300 || none.waits != none.src || none.relDead || none.relAns + none.relStop < none.waits - 2 || !none.armedP)
        Err("LA7: with no late answer %ld wait(s) of %ld sourced ride pairs, released by the answer %ld, by the deadline %ld, the engage's first at pair %ld (300 or more, one each, all but two by the answer, 0, and a pair wanted)", none.waits, none.src, none.relAns, none.relDead, none.armedP);
    if (nS < 200 || nW != nS) Err("LA7: with no late answer, behind pair 440: %ld wait(s) of %ld sourced ride pairs (one each, of 200 or more, wanted)", nW, nS);
    if (run.relDead < 3 || run.relDead > 45 || rS < 200 || rW != rS)
        Err("LA7: with every answer late for 40 pairs: released by the deadline %ld; behind the run %ld wait(s) of %ld sourced ride pairs (three to forty-five; one each, of 200 or more, wanted: nothing switched it off)", run.relDead, rW, rS);
    if (cW != 0 || cS < 200) Err("LA7 control: with no order wait %ld wait(s) behind the run of %ld sourced ride pairs (none, of 200 or more, wanted): the control shows nothing", cW, cS);
    Ctl("no order wait: %ld wait(s) behind the run of %ld sourced ride pairs", cW, cS);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LA8, the helper's place on the card, asked and given back
// WHAT IT SHOWS. The lock at 90's shape, card 1. Three windows engaged, three frames of stand-by (the want PASS), three windows engaged: ASKED
// ONCE AN ENGAGE, at its first hand that rides, twice in all; GIVEN BACK where each engage ends, twice in all. And A PAIR THAT
// FITS (the free shape of FB1, no pair rides): never asked.
static void CaseLA8()
{
    const char* const name = "LA8 the helper's place on the card: asked once an engage at its first hand that rides, given back where the engage ends, never asked where no pair rides (2026-10-14)";
    CaseBegin(name);
    RtPar par = LaLock(90, 3500, 2); par.over = 1; par.card = 1; par.ord = 1;
    const RingStats stc = gRingSt;
    static LONG upsAt[3]; static LONG downsAt[3]; static LONG nowAt[3];
    auto plant = [](int i)
    {
        FgState& s = B::S();
        if (i == 360) { upsAt[0] = ShArmB::cardUps; downsAt[0] = ShArmB::cardDowns; nowAt[0] = ShArmB::cardNow; s.wt.want = FG_WANT_PASS; }
        if (i == 363) { upsAt[1] = ShArmB::cardUps; downsAt[1] = ShArmB::cardDowns; nowAt[1] = ShArmB::cardNow; s.wt.want = FG_WANT_ON; }
        if (i == 719) { upsAt[2] = ShArmB::cardUps; downsAt[2] = ShArmB::cardDowns; nowAt[2] = ShArmB::cardNow; }
    };
    const LaLeg g = LaRun(par, 4.0, 720, 600, plant, 99.0, 99.0);
    RtPar pf; pf.kWorkUs = 1100; pf.kResetUs = 400; pf.tailUs = 500; pf.upUs = 1200; pf.postUs = 1500; pf.planUs = 1500; pf.over = 1; pf.card = 1; pf.ord = 1; pf.rank = 2;
    const LaLeg f = LaRun(pf, 4.0, 400, 200, [](int) {}, 99.0, 99.0);
    ViolRestore(stc);
    printf("  THREE WINDOWS ENGAGED: asked %ld, given back %ld, standing asked %s; BEHIND THREE FRAMES OF STAND-BY: asked %ld, given back %ld, standing %s; BEHIND THREE MORE WINDOWS ENGAGED: asked %ld, given back %ld, standing %s; at the end: asked %ld, given back %ld\n",
        upsAt[0], downsAt[0], nowAt[0] ? "yes" : "no", upsAt[1], downsAt[1], nowAt[1] ? "yes" : "no", upsAt[2], downsAt[2], nowAt[2] ? "yes" : "no", g.cardUps, g.cardDowns);
    printf("  A FREE CARD (no pair rides): %s; hands that rode %ld\n", LaLine(f), f.set.rideHands);
    if (upsAt[0] != 1 || downsAt[0] != 0 || !nowAt[0] || upsAt[1] != 1 || downsAt[1] != 1 || nowAt[1] || upsAt[2] != 2 || downsAt[2] != 1 || !nowAt[2] || g.cardUps != 2 || g.cardDowns != 2)
        Err("LA8: asked and given back, engaged: %ld and %ld; behind the stand-by: %ld and %ld; behind the second engage: %ld and %ld; at the end: %ld and %ld (1 and 0; 1 and 1; 2 and 1; 2 and 2 wanted)", upsAt[0], downsAt[0], upsAt[1], downsAt[1], upsAt[2], downsAt[2], g.cardUps, g.cardDowns);
    if (f.set.rideHands != 0 || f.cardUps != 0 || f.waits != 0 || f.overN != 0) Err("LA8: on a free card %ld hand(s) rode, the helper's place was asked for %ld time(s), %ld order wait(s) were queued and %ld pair(s) copied over a dispatch (all 0 wanted: a pair that fits runs what it ran)", f.set.rideHands, f.cardUps, f.waits, f.overN);
    if (f.gen < f.n) Err("LA8: on a free card %d of %d generated frames were shown (all wanted)", f.gen, f.n);
    CaseEnd(name);
}
