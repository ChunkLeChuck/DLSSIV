// dt_fg_replay.h: THE SEVEN LEGS AT THE TWO LOCKS, REPLAYED IN THE HARNESS (2026-10-14, evening; docs/reports/fg-4/brief-2026-10-14b/
// BRIEF-busy-card-whole.md, Part 0's item 2 and Part 1's stage 1).
//
// Included by dt_fg_test.cpp behind dt_fg_rideframes.h, with ONE line; its cases are RL1 to RL7, RUN BY NAME ALONE (dt_fg_test 2 RL3):
// THE GATE IS OPEN (2026-10-14, evening, the first try): RL6 reads its leg (99.7 of 100, 59.2 real, the windows 117 and 120 against
// 99.6, 59.1, 113 and 115); RL3 reads 98.9 against 95.8 and its first window 108 against 77; RL1 reads 0.3 against 28.9: without the
// teaching from the arm the fake's handoff never retires inside half a period, so its room floor never learns and no pair rides,
// where the leg's learned it from a few cheap frames in two windows; RL4's event does not come by itself (0 releases by the worker),
// and the plant (holdPlaceAt) does not end on this clock. What the simulation lacks for the 300 MHz legs: the real distribution of
// the entry to the handoff's retire at an engage (its low tail under 8.46 ms), and the stamper's own wake lag.
//
// WHAT A REPLAY IS. One fake game a leg, from the leg's own lines (the entry to the handoff's retire, the answer's steady time, the post
// chain, the scene, the plan, the panel) and THE KEYS THAT BUILD RAN (fg_pace's bits as the kit staged them), run from a fresh machine
// (the engage is the run's start, as the leg's window 10 was), and read as the leg's W read it: of 100 generated frames shown over the
// run, the engage's first two windows of 120, the real rate. THE SIMULATION GAINS THREE THINGS FOR THESE (dt_fg_rates.h, RtPar; 0 in
// every case before): the presenter's hold (a card over its period held the real game in DXVK's Present; the fake ran away from it),
// the scene behind the retire as what the CPU had queued by then (a retire ahead of the next frame's begin finds the queue dry and
// the dispatch runs first: the slack windows of the leg of 2026-10-13 showed 29 of 100 where "the game first" alone shows none), and
// a worker that waits for the answer served on its own thread (the evening's early wait met a clock that stood).
// WHAT IS HELD: shown of 100 within 3 of the leg, the real rate within 2 a second, the first two windows' shape, and for the leg of
// 2026-10-14's evening THE LATCH: one release by the worker ahead of a present in the first window and no order wait after it. Where
// the run of that evening's keys does not fire it by itself, it is planted (the placer and the answer's stamper both late behind one
// pair: RtPar::holdPlaceAt) and said so.
// SINCE 2026-10-17 (stage 2, the order wait per pair: no arming, no latch, the room floor seeded) EVERY REPLAY RUNS TODAY'S MACHINE on its
// leg's shape and keys (RlPar). The legs whose machine was today's but for the latch and the seed (RL3, RL6, RL7) are held to their
// bounds as before; THE LATCH'S LEG (RL4) is held to what today's machine must do where the latch fired: the order wait standing on
// every sourced ride pair behind the first window, with its event planted (the plant ends now: holdPlaceAt took the placer's timer
// with it), and its control is the same run with no order wait at all. RL1, RL2 and RL5 stay what the gate found them.
// SINCE 2026-10-17, STAGE 3's PART A (BRIEF-stage3-placement.md): THE GATE CLOSED FROM A LEG'S PER-PAIR DATA. RL8 is stage 2's own leg (t70.2
// of 2026-10-17: 96.2 of 100, 48.2 real, its engage's first two windows 86 of 119 and 120 of 120), ITS CARD TIMES DRAWN PAIR BY PAIR from the
// leg's entry to the handoff's retire (dt_fg_rlshape.h, from t70.2-pairs.json by gen_rl_shape.py) in place of a spread; RL9 is the free card
// of the same morning (flw: 99.9, 59.1 real, 114 and 120). EVERY REPLAY NOW RUNS THE FAKE WITH ITS TWO MENDS OF THAT DAY: the helper's
// prepare displaced to its handoff's retire (RtPar::prepAtRet), and THE CARD THAT STARVES THE HELPER WHERE NO ORDER WAIT HOLDS THE GAME
// (RtPar::starve, dt_fg_rates.h's RtDisp: a dispatch behind the game's queue runs where the game next waits for the helper, the next home).
// Each replay prints the fake's own entry to the handoff's retire beside the leg's, so the shape is read and not assumed.
// THE GATE (2026-10-17). The brief's first gate held for RL8, RL6, RL7 and RL9 and NOT for RL1 (85.4 against 28.9): the fake does not starve the
// helper without the order wait (today's floors follow the ride's wait, which in the fake grows with the CPU's lead on the card, so the period
// stays over the card's own cycle; on the card of 2026-10-13 the CPU's frame from its begin to its entry was the whole period). THE MAIN SESSION
// WAIVED RL1 (the product never runs a ride pair without the order wait now): A KNOWN LIMIT, told and not judged; and the fake cannot price what
// the order wait itself costs the card. THE GATE IN ITS PLACE: RL8's STEADY WINDOWS lose what the leg's lost, BY KIND, each within 0.6 of 100
// (the chain of finding 1: ride pairs fired ahead of the body for a wait on the helper's prepare, their generated frame forced, then no source,
// then a reset), with the whole run's shown within 3 and the real rate within 2; the first window a known spread, told. HELD with the calibration
// below: 95.8 of 100 (96.2), 49.1 real (48.2), the steady windows 2.86, 1.31, 1.31, 1.31 of 100 (2.37, 1.14, 0.96, 0.96); on the card 520 (537).
#pragma once
#include "dt_fg_rlshape.h"

struct RlLeg
{
    const char* name; const char* when;
    double shown, fps; int w0, w1;                // the leg's own numbers (W's)
    int lock;                                     // 70 or 90 (and 0: the free card, 2026-10-17)
    int over, ord, early, noWait, engage, stamp, waitAns; LONG planMaxUs;
    int latch;                                    // 1: the leg is the latch's (one worker release in the first window, no wait after)
    // (2026-10-17) the leg's card times pair by pair: the entry to the handoff's retire of the engage's first 240 pairs, then of every 16th
    // behind them, in microseconds (nullptr: the spread of before)
    const LONG* lsA; int lsAN; const LONG* lsB; int lsBN;
    // (2026-10-17, the main session's gate) the leg's steady windows by kind, of 100 pairs: ride pairs fired ahead of the body, generated frames
    // forced, no source, fed with a reset (all 0: not judged); and 1 where the first window is a known spread, told and not judged
    double stA, stF, stN, stR; int firstTold;
    // (phase 5, P3: THE GATE OF STAGE 3's LEGS, sitting-2026-10-17b) the stage 3 keys the leg's build ran (RL_NOW_PREP | RL_NOW_PLACE: 3; 0 for the
    // legs of before), and the leg's own placement as its lines say it: on the card (the writes' place, 500 the middle; 0: not judged), and of 100
    // ride pairs fired at the position, at the clock due, at the next injection
    int keys; LONG s2; double posPct, duePct, injPct;
    // (phase 5, P3) THE CALIBRATION OF STAGE 3's LEGS (below kRlCost300): the fake game's CPU frame and its spread, where its injection reaches the
    // worker in it, the scene behind the retire, the post chain, the order wait's own card time (0: the lock's as before)
    double costMs, spread, inj; LONG sceneUs, postUs, ordUs;
};
static const int kRlN = 11;
static const RlLeg kRl[kRlN] =
{
    { "RL1", "2026-10-13 evening, t70.2 (the ride; no order wait)",                         28.9, 56.3,   0,  34, 70, 0, 0, 0, 0, 0, 0, 0, 8000, 0,
      nullptr, 0, nullptr, 0, 0, 0, 0, 0, 2 },   // (2026-10-17: WAIVED by the main session, a known limit of the fake: CaseRl's note)
    { "RL2", "2026-10-14 morning, t70.2 (the order wait armed by two late answers)",        89.6, 52.2,   0,  26, 70, 1, 1, 0, 0, 0, 0, 0, 8000, 0 },
    { "RL3", "2026-10-14 afternoon, t70.2 (the early show, from the first ride pair)",      95.8, 49.5,  77, 116, 70, 1, 1, 1, 0, 1, 1, 0, 4500, 0 },
    { "RL4", "2026-10-14 evening, t70.2 (+ the early wait: the latch)",                     27.4, 55.0,  22, 116, 70, 1, 1, 1, 0, 1, 1, 1, 4500, 1 },
    { "RL5", "2026-10-13 evening, t90.2",                                                   92.1, 59.1, 105, 117, 90, 0, 0, 0, 0, 0, 0, 0, 8000, 0 },
    { "RL6", "2026-10-14 morning, t90.2",                                                   99.6, 59.1, 113, 115, 90, 1, 1, 0, 0, 0, 0, 0, 8000, 0 },
    { "RL7", "2026-10-14 afternoon, t90.2",                                                 99.8, 59.1, 107, 120, 90, 1, 1, 1, 0, 1, 1, 0, 4500, 0 },
    { "RL8", "2026-10-17 morning, t70.2 (stage 2: the order wait on every sourced ride pair)", 96.2, 48.2,  86, 120, 70, 1, 1, 1, 0, 1, 1, 1, 4500, 0,
      kRl8LsFirst, (int)(sizeof(kRl8LsFirst) / sizeof(kRl8LsFirst[0])), kRl8LsSteady, (int)(sizeof(kRl8LsSteady) / sizeof(kRl8LsSteady[0])),
      2.37, 1.14, 0.96, 0.96, 1 },
    { "RL9", "2026-10-17 morning, flw (the free card)",                                     99.9, 59.1, 114, 120,  0, 1, 1, 1, 0, 1, 1, 1, 4500, 0 },
    // (phase 5, P3) STAGE 3's TWO LEGS, the placement by the card and the prepare's wait on (the pair 1cc0c1d8's build): read from their logs by
    // read_pairs.py and the window lines (docs/reports/fg-4/sitting-2026-10-17b; the windows 10 to 62 and 10 to 71)
    { "RL10", "2026-10-17 afternoon, t70.2 (stage 3: the placement by the card)",           99.2, 50.1, 108, 119, 70, 1, 1, 1, 0, 1, 1, 1, 4500, 0,
      kRl10LsFirst, (int)(sizeof(kRl10LsFirst) / sizeof(kRl10LsFirst[0])), kRl10LsSteady, (int)(sizeof(kRl10LsSteady) / sizeof(kRl10LsSteady[0])),
      0, 0, 0, 0, 1, 3, 505, 75.4, 23.2, 0.6, 10.5, 0.10, 0.95, 1500, 4500, 1000 },
    { "RL11", "2026-10-17 afternoon, t90.2 (stage 3: the placement by the card)",           99.8, 59.1, 113, 119, 90, 1, 1, 1, 0, 1, 1, 1, 4500, 0,
      kRl11LsFirst, (int)(sizeof(kRl11LsFirst) / sizeof(kRl11LsFirst[0])), kRl11LsSteady, (int)(sizeof(kRl11LsSteady) / sizeof(kRl11LsSteady[0])),
      0, 0, 0, 0, 0, 3, 504, 80.4, 19.4, 0.0, 6.0, 0.10, 0.95, 5000, 3300, 0 },
};
// THE FREE CARD (2026-10-17, flw): the entry to the handoff's retire 3.4 ms, the answer 1,000 us, the plan 1,750 to 2,000; every pair fits.
static RtPar RlFree()
{
    RtPar p = LaLock(90, 1000, 1);
    p.tailUs = 1000; p.upUs = 900; p.postUs = 2500; p.kWorkUs = 1000; p.kResetUs = 300; p.planUs = 1750;
    p.over = 1; p.ord = 1; p.ordLatUs = 300; p.pubWake = 1;
    return p;
}
// The lock at 960 MHz's shape, as RfLock300 is the 300's: FqLock90's card (the handoff 7.75 ms behind the entry, the post chain 4.3,
// the upscale 2.45, the tail 2.0), the dispatch 2,200 us steady, the scene 2,900 behind the retire, the game first.
static RtPar RlLock960()
{
    RtPar p = LaLock(90, 2900, 1);
    p.kWorkUs = 2200; p.planUs = 2750;
    p.over = 1; p.ord = 1; p.ordLatUs = 300; p.pubWake = 1;
    return p;
}
// THE CALIBRATION OF 2026-10-17 (stage 3's Part A), at the lock at 300 MHz alone, fitted to RL8 and said as such. THE GATE IT IS FITTED TO (the
// main session's of that day, RL1 waived): RL8's STEADY WINDOWS, behind the engage's first two, LOSE WHAT THE LEG'S WINDOWS 11 TO 56 LOST, BY KIND
// (of 100 pairs: ride pairs fired ahead of the body for a wait on the helper's prepare 2.37, generated frames forced 1.14, no source 0.96, fed
// with a reset 0.96), and the whole run's shown and real rate as the leg's:
//   THE FRAME'S SCENE IN 64 CHECKPOINT CALLS (RtPar::calls, every replay: the planting by the card places a generated frame by them) of 2,000 us of
//     scene and 3,500 of tail at that lock (5.5 ms ahead of the injection: the game alone at 13.43 ms less the upscale 3.3 and the post chain 4.0
//     is 6.1), and THE FAKE GAME'S CPU FRAME 9 ms (4 elsewhere): with it the generated frame reaches the card off the middle as the leg's did
//     (stage 2's machine: 520 of 1000, windows' medians 508 to 573, the leg 537 and 510 to 686), where at 4 ms the CPU never ran ahead of a
//     handoff and every frame stood at the middle (505);
//   THE ORDER WAIT'S OWN CARD TIME, 1,900 us a sourced ride pair (RtPar::ordLatUs: the release reaching the game's queue that long behind its signal,
//     where the desk at full clock measured 310 us). WHY: the legs at that lock without the order wait ran at the card's own cycle, 56.3 and 55.0
//     real (2026-10-13 evening; 2026-10-14 evening behind its latch), with it 48.2 to 52.2; the replay of stage 2's leg reads its rate only
//     with the card paying that much more a pair. INFERRED, NOT SEEN: no stamp of the game's queue says where the time goes (the release's
//     path through the stamper, the scheduler's switch between the two processes' queues, or both);
//   THE CARD'S TIMES PAIR BY PAIR from the leg's entry to the handoff's retire behind the engage's first 240 (dt_fg_rlshape.h, kRl8LsSteady), each
//     sample reached over kRlHold pairs from the one before (the leg's card drifts: its every-16th samples are a slow scene, and held one a pair
//     they are a card that jumps, which never lets the worker run ahead of a handoff), its swing laid on the post chain whole and on the tail
//     and the upscale at a half. The first 240's own values are NOT fed: they carry the engage's backlog, which the fake makes itself (fed,
//     the first window read 3 of 120). (The dry entry to the retire the swing is taken against keeps the 4 ms frame's: lsDry below.)
// Sweeps of these (2026-10-17, scratch, not kept): THE FIRST WINDOW IS A KNIFE EDGE, read as a known spread and not fitted (81 to 117 of 120 over
// calibrations whose steady windows differ little; the leg's 86); the steady windows go with kRlHold (1: no loss at all; 4: the leg's; 8: three
// times the leg's) and with the CPU frame and the order wait's card time together (9 ms and 1,900: 2.86, 1.31, 1.31, 1.31 of 100; 10 ms and 1,250:
// 3.58 ahead); nothing in the ranges tried makes RL1 starve: see CaseRl's note. HELD: RL8 95.8 of 100, 49.1 real, the second window 120, the
// steady windows 2.86, 1.31, 1.31, 1.31.
// THE CALIBRATION OF STAGE 3's LEGS (phase 5, P3, 2026-10-17; BRIEF-P3-place.md point 1), RlLeg's last six fields, fitted to RL10 and RL11 and said
// as such. THE GATE: each leg's shown within 1 of 100, its real rate within 2 a second, on the card within 25 of 1000, and each share of its ride
// pairs' placement (at the position, at the clock due, at the next injection) within 10 of 100. WITH THE CALIBRATION ABOVE (RL8's, kept for RL8)
// RL10 read 98.0, 44.8 real, 506, 27.4 / 67.7 / 3.8 against 99.2, 50.1, 505, 75.4 / 23.2 / 0.6, and RL11 read 99.7, 59.2, 503, 0.4 / 99.5 / 0.0
// against 99.8, 59.1, 504, 80.4 / 19.4 / 0.0: THE FAKE'S NEXT FRAME BEGAN BEHIND THE GENERATED FRAME'S CLOCK DUE, where the legs' worker reached
// the position first in three pairs of four. WHAT MOVED IT, from sweeps (scratch, not kept): THE INJECTION AT 95 OF 100 OF THE CPU FRAME (the
// scene's thousands of calls ahead of it, the post chain's and the interface's few behind it; 75 before) AND THE CPU FRAME LONGER, ITS SPREAD
// 10 PERCENT (25 before): 6.0 ms at 960 MHz, 10.5 at 300 (the leg's own line: DXVK's Present held the worker 0.8 ms at the median and up to 9.4 at
// 300 MHz, nothing at 960, which stretches the frame the worker replays: INFERRED, not seen); and a card whose split follows the legs' needs:
// at 960 the scene behind the retire 5,000 us and the post chain 3,300 (the leg's EVEN need 3.6 ms, the fake's 5.5 before), at 300 the scene
// 1,500, the post chain 4,500 and the order wait's own card time 1,000 (1,900 before; at 1,900 the rate read 48.6). HELD: RL10 99.5, 50.0 real,
// 488, 75.8 / 21.9 / 1.8; RL11 99.7, 59.2, 506, 77.1 / 22.8 / 0.0 (the CPU frame 10.0 or 11.0 at 300 holds too). WHAT IT DOES NOT REPRODUCE,
// TOLD: the entry to the handoff's retire (7.3 ms behind the engage at 300 against the leg's 11.0; 7.3 against 8.2 at 960: the real worker
// lags the render thread, whose Present the entry is, and the fake's does not), so the WAIT need (3.6 ms against 8.9 at 300); f (10 of 32 at
// the end against the leg's 15; 6 against 12); the waits at the position (25 of 100 ride pairs against 17); AND RL8's LEG: run with this
// calibration stage 2's machine read 54.6 real where its leg read 48.2, so RL8 keeps the calibration it was fitted with.
static const double kRlShTail = 0.5, kRlShUp = 0.5, kRlShPost = 1.0;
static const LONG kRlOrdCard300 = 1900, kRlScene300 = 2000, kRlTail300 = 3500;
static const double kRlCost300 = 9.0;
static const int kRlCalls = 64;
static double gRlCostMs = 4.0;                                // (the fake game's CPU frame for the replay being run: RlPar sets it)
static double gRlSpread = 0.25;                               // (and its spread each way, frame by frame: RlPar sets it)
static const int kRlHold = 4;
// THE MACHINE NOW (2026-10-17, stage 3): a replay runs its leg's own keys (the gate), and the legs today's machine is judged on (RL6 to RL9) run a
// second time with stage 3's keys on, as the armed build hands them: bit 1 the prepare's wait behind the answer (FgArmedKeys::prepWait)
enum { RL_NOW_PREP = 1, RL_NOW_PLACE = 2, RL_NOW_FITONLY = 4, RL_NOW_NOWAIT = 8, RL_NOW_ALL = RL_NOW_PREP | RL_NOW_PLACE, RL_NOW_PLACE2 = 16 };
static int gRlNow = 0;
static LONG gRlPlace2 = 0;                                    // (phase 5, P3: RlPar sets it from RL_NOW_PLACE2, RlFrames hands it to the machine behind RtBegin)
static LONG gRlPrepMs = 0;                                    // (the prepare's wait's bound for a told comparison: 0 the machine's 8 ms)
static RtPar RlPar(const RlLeg& l, int plant)
{
    RtPar p = l.lock == 70 ? RfLock300() : l.lock == 90 ? RlLock960() : RlFree();
    p.over = l.over; p.ord = l.ord; p.card = 0;
    p.prepAtRet = 1; p.starve = l.lock != 0 ? 1 : 0;          // (2026-10-17: the fake's two mends, on every replay; round 2 of phase 5: THE STARVED DISPATCH ONLY
                                                              // WHERE THE CARD IS THE LIMIT, the two locks: on the free card no game's queue holds the next frame up to its injection)
    const int now = gRlNow | l.keys;                          // (phase 5: a leg of stage 3 ran its keys itself)
    if (now & RL_NOW_PREP) { p.prepWait = 1; p.prepWaitMs = gRlPrepMs; }
    if (now & RL_NOW_PLACE) p.place = 1;
    if (now & RL_NOW_FITONLY) p.fitOnly = 1;
    gRlPlace2 = (now & RL_NOW_PLACE2) ? 1 : 0;
    p.latencyUs = l.lock == 70 ? 18000 : 17000;
    if (l.lock == 70) { p.ordLatUs = kRlOrdCard300; p.sceneUs = kRlScene300; p.tailUs = kRlTail300; }
    p.calls = kRlCalls;
    gRlCostMs = l.lock == 70 ? kRlCost300 : 4.0;
    gRlSpread = 0.25;
    if (l.costMs > 0.0)
    {   // (phase 5, P3: THE CALIBRATION OF STAGE 3's LEGS, below kRlCost300)
        gRlCostMs = l.costMs; gRlSpread = l.spread; p.inj = l.inj; p.sceneUs = l.sceneUs; p.postUs = l.postUs;
        if (l.ordUs > 0) p.ordLatUs = l.ordUs;
    }
    p.rideEarly = l.early; p.rideNoWait = l.noWait; p.armTeach = l.engage; p.ordAtStamp = l.stamp; p.rideWaitAns = l.waitAns;
    if (gRlNow & RL_NOW_NOWAIT) { p.rideEarly = 1; p.rideNoWait = 1; }   // (the WAIT floor out, EVEN kept: the floors' second question)
    p.planMaxUs = l.planMaxUs;
    p.leadScene = 1; p.serve = 1;
    // (2026-10-17, stage 2: THE MACHINE IS TODAY'S, whatever the leg ran: no arming and no latch exist any more, and the room floor is seeded
    // by an engage's first stamped pair, the armed build's key. So a replay holds what today's machine does on the leg's shape, and a leg
    // whose own machine is gone (RL2's arming, RL4's latch, RL6's arming) is read against that, not against its own numbers alone)
    p.roomSeed = 1;
    if (plant) p.holdPlaceAt = 40;
    return p;
}
struct RlOut { RfLeg g; double shownAll; int n0, g0, n1, g1; LONG waitsAfter, relPres0, served, holds; LONG why[FG_AW_N]; LONG first, behind, ordered, kMed, k95, kN, room, rideF, tsNever; LONG srcAfter, seeded;
               LONG starved; LONG ls05, ls50, ls95, lsMost, lsUnder, lsN, ls2_50; LONG rideD, needE, needW, needF, guardForced, rideAhead;
               double fpsSteady, shownSteady; double winFps[9]; int winGen[9], winN[9];
               LONG stF, stN, stR, stA, stG, stL; double shown1s, shownAfter1s; LONG prepWaits, prepGot, prepMaxUs;
               LONG plMed[9], plLo[9], plHi[9], pl05[9], pl95[9], plMedAll;
               LONG posFires, posWaits, posGot, posMaxUs, posNoAns, posInj, posDue, placeSteps, placeF, placeN, posArms, posRides, pos2Arms, pos2Fires;
               LONG depth50, depth95, depthMax; double age50, age95, ageMax; };   // (2026-10-17: ON THE CARD, the generated frame's place by window)
// ON THE CARD IN THE FAKE (2026-10-17): the generated frame's place between its two real frames by the instants the simulation SHOWED them (the
// queue's own: a present is on the screen when what was queued ahead of it has retired, dt_fg_rates.h), in thousandths (500: the middle),
// over the pairs [a, b) whose generated frame and both real frames were shown once: the median, the least, the most, the 5th and 95th.
static void RlPlace(int a, int b, LONG* med, LONG* lo, LONG* hi, LONG* p05, LONG* p95)
{
    std::vector<LONG> v;
    const int n = (int)gRt.pairs.size();
    for (int i = a < 1 ? 1 : a; i < b && i < n; i++)
    {
        const RtPair& x = gRt.pairs[i]; const RtPair& y = gRt.pairs[i - 1];
        if (x.gen != 1 || x.real != 1 || y.real != 1 || x.tReal <= y.tReal) continue;
        v.push_back((LONG)(1000.0 * (double)(x.tGen - y.tReal) / (double)(x.tReal - y.tReal) + 0.5));
    }
    *med = *lo = *hi = *p05 = *p95 = 0;
    if (v.empty()) return;
    std::sort(v.begin(), v.end());
    *med = v[v.size() / 2]; *lo = v.front(); *hi = v.back(); *p05 = v[v.size() * 5 / 100]; *p95 = v[v.size() * 95 / 100];
}
static void RlCardLine(const char* tag, const RlOut& o)
{
    printf("    %s ON THE CARD (the fake's shown instants): the median of the windows' medians %ld; by window, median (5th to 95th, least to most):", tag, o.plMedAll);
    for (int b = 0; b < 9; b++) printf(" %ld (%ld-%ld, %ld-%ld);", o.plMed[b], o.pl05[b], o.pl95[b], o.plLo[b], o.plHi[b]);
    printf("\n");
}   // (2026-10-17: the steady windows by kind, the gate's own numbers)   // (2026-10-17: starved, and the fake's own entry to the handoff's retire)
// The replay's own frames: RfRun's loop (the tail by 29 percent, the cost by 25) with THE POST CHAIN AND THE UPSCALE SPREAD TOO (20 and 15
// percent): the legs' entry to the handoff's retire has a low tail under half a period (the cheap frames that teach the room floor
// its first value on a busy card), which fixed post and upscale times never give.
// (2026-10-17) WITH A LEG'S PER-PAIR SHAPE (RlLeg::lsB; every replay at the lock at 300 MHz takes RL8's): frame i's tail, upscale and post chain
// follow the leg's entry to the handoff's retire of a pair (lsB in turn) over the shape's own on a queue that is dry (the three less the frame's
// cost behind its injection, at the cost's middle), each by its share of the swing (kRlShTail, kRlShUp, kRlShPost above); the cost keeps its spread.
static LONG gRlWhy0[FG_AW_N], gRlGuard0, gRlWhy1[FG_AW_N], gRlGuard1, gRlAhead1;
static RfLeg RlFrames(const RtPar& par, int frames, unsigned seed, const RlLeg* l = nullptr)
{
    RfLeg g = {};
    RtBegin(par);
    ShArmB::a.k.place2 = gRlPlace2;                           // (phase 5, P3: the second position, FgArmedKeys::place2, where the run asks for it)
    const FgArmedState& a = ShArmB::a;
    gRtSeed = seed;
    const LONG post0 = par.postUs, up0 = par.upUs;
    // (every replay at the lock at 300 MHz takes the card's shape of that lock: the leg of 2026-10-17's, the one with per-pair data)
    RlLeg sh = l ? *l : RlLeg{};
    if (l && l->lock == 70 && !l->lsB) { sh.lsA = kRl8LsFirst; sh.lsAN = (int)(sizeof(kRl8LsFirst) / sizeof(kRl8LsFirst[0])); sh.lsB = kRl8LsSteady; sh.lsBN = (int)(sizeof(kRl8LsSteady) / sizeof(kRl8LsSteady[0])); }
    if (l) l = &sh;
    const bool shape = l && l->lsA && l->lsAN > 0 && l->lsB && l->lsBN > 0;
    const double lsDry = (double)(par.tailUs + par.upUs + par.postUs) - 4000.0 * (1.0 - par.inj);
    for (int i = 0; i < frames; i++)
    {
        if (shape && lsDry > 1000.0)
        {
            const int j = i / kRlHold; const double fr = (double)(i % kRlHold) / (double)kRlHold;
            const double ls = (1.0 - fr) * (double)l->lsB[j % l->lsBN] + fr * (double)l->lsB[(j + 1) % l->lsBN];
            double k = ls / lsDry; if (k < 0.3) k = 0.3; if (k > 3.0) k = 3.0;
            gRt.par.tailUs = (LONG)(par.tailUs * (1.0 + (k - 1.0) * kRlShTail)); gRt.par.postUs = (LONG)(post0 * (1.0 + (k - 1.0) * kRlShPost)); gRt.par.upUs = (LONG)(up0 * (1.0 + (k - 1.0) * kRlShUp));
        }
        else
        {
        gRt.par.tailUs = (LONG)(4500.0 * RtSpread(0.29) * (par.tailUs / 4500.0));
        gRt.par.postUs = (LONG)((double)post0 * RtSpread(0.20));
        gRt.par.upUs = (LONG)((double)up0 * RtSpread(0.15));
        }
        RtFrame(RtMs(gRlCostMs * RtSpread(gRlSpread)));
        if (i == 119) { for (int k = 0; k < FG_AW_N; k++) gRlWhy0[k] = a.w.iWhy[k]; gRlGuard0 = a.w.guardForced; }   // (2026-10-17: the engage's first window by kind)
        if (i == 239) { for (int k = 0; k < FG_AW_N; k++) gRlWhy1[k] = a.w.iWhy[k]; gRlGuard1 = a.w.guardForced; gRlAhead1 = a.w.rideAhead; }   // (and where the steady windows begin)
        if (i == 119 || i == 239) { RfWin& w = g.w[i / 120]; w.fromArm = a.pl.tsNever; w.room = a.pl.roomUs; w.rode = ShArmB::a.r.rideN; w.waits = a.w.ordWaits; w.relPres = a.w.ordRelPresent; w.src = a.w.rideSrcN; }
    }
    g.set = RtRead(0, frames);
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { const RtPair& x = gRt.pairs[i]; g.n++; if (x.gen == 1) g.gen++; if (x.real == 1) g.real1++; }
    {
        const double q = (double)B::Qpf() / 1000.0; std::vector<double> ages;
        for (int i = 0; i < n - 1; i++) { const RtPair& x = gRt.pairs[i]; if (x.real == 1) ages.push_back((double)(x.tReal - x.tEntry) / q); }
        if (!ages.empty()) { std::nth_element(ages.begin(), ages.begin() + ages.size() / 2, ages.end()); g.ageMed = ages[ages.size() / 2]; }
    }
    for (int b = 0; b < 2; b++) { RfWin& w = g.w[b]; for (int i = b * 120; i < (b + 1) * 120 && i < n - 1; i++) { w.n++; if (gRt.pairs[i].gen == 1) w.gen++; } }
    g.forced = a.w.iWhy[FG_AW_FORCED]; g.rideLost = a.w.rideGenLost; g.early = a.w.rideEarlyN; g.noAns = a.w.rideNoAns; g.earlyNot = a.w.rideEarlyNot;
    g.relInj = a.w.ordRelInj; g.relStamp = a.pl.ordRelStamp; g.relAns = a.pl.ordRelAnswer; g.relDead = a.pl.ordRelDeadline; g.relPres = a.w.ordRelPresent;
    g.armedP = a.w.ordFirstP; g.armTaught = a.pl.armTaught; g.waits = a.w.ordWaits; g.seeded = a.pl.roomSeeded;
    RtEnd("RL");
    g.blitsMax = ShArmB::ordBlitsMax;
    return g;
}
static RlOut RlRun(const RlLeg& l, int plant)
{
    RlOut o = {};
    const RtPar p = RlPar(l, plant);
    o.g = RlFrames(p, 1080, 20261014u, &l);
    {   // (what the run left in the machine and the simulation, read before the next fresh one)
        const FgArmedState& a = ShArmB::a;
        for (int k = 0; k < FG_AW_N; k++) o.why[k] = a.w.iWhy[k];
        o.first = gRt.dispFirst; o.behind = gRt.dispBehind; o.ordered = gRt.dispOrdered; o.starved = gRt.dispStarved;
        o.rideD = a.pl.rideDUs; o.needE = a.pl.rideNeedE; o.needW = a.pl.rideNeedW; o.needF = a.pl.rideNeedF; o.guardForced = a.w.guardForced; o.rideAhead = a.w.rideAhead;
        o.stF = o.why[FG_AW_FORCED] - gRlWhy1[FG_AW_FORCED]; o.stN = o.why[FG_AW_NOSOURCE] - gRlWhy1[FG_AW_NOSOURCE]; o.stR = o.why[FG_AW_RESET] - gRlWhy1[FG_AW_RESET];
        o.stA = o.rideAhead - gRlAhead1; o.stG = o.guardForced - gRlGuard1; o.stL = o.why[FG_AW_LATE] - gRlWhy1[FG_AW_LATE];
        o.prepWaits = a.w.prepWaitN; o.prepGot = a.w.prepWaitGot; o.prepMaxUs = a.w.prepWaitMaxUs;
        {   // (2026-10-17, the floors' judge) the queue's depth ahead of a frame's begin and the picture's age, behind the engage's first two windows
            std::vector<LONG> d(gRt.depth.size() > 240 ? gRt.depth.begin() + 240 : gRt.depth.end(), gRt.depth.end()); std::sort(d.begin(), d.end());
            if (!d.empty()) { o.depth50 = d[d.size() / 2]; o.depth95 = d[d.size() * 95 / 100]; o.depthMax = d.back(); }
            const double q = (double)B::Qpf() / 1000.0; std::vector<double> ag;
            for (size_t i = 240; i + 1 < gRt.pairs.size(); i++) { const RtPair& x = gRt.pairs[i]; if (x.real == 1) ag.push_back((double)(x.tReal - x.tEntry) / q); }
            std::sort(ag.begin(), ag.end());
            if (!ag.empty()) { o.age50 = ag[ag.size() / 2]; o.age95 = ag[ag.size() * 95 / 100]; o.ageMax = ag.back(); }
        }
        o.posFires = a.w.posFires; o.posWaits = a.w.posWaits; o.posGot = a.w.posWaitGot; o.posMaxUs = a.w.posWaitMaxUs; o.posNoAns = a.w.posNoAns;
        o.pos2Arms = a.w.pos2Arms; o.pos2Fires = a.w.pos2Fires;
        o.posInj = a.w.posInj; o.posDue = a.w.posDue; o.placeSteps = a.w.placeSteps; o.placeF = a.w.placeF; o.placeN = a.w.placeN; o.posArms = a.w.posArms; o.posRides = a.w.posRides;
        {   // (2026-10-17) THE RUN BY WINDOWS OF 120, and behind the engage's first two (the legs' rates are a whole session's, mostly steady)
            for (int b = 0; b < 9; b++) { const RtStat r = RtRead(b * 120, (b + 1) * 120); o.winFps[b] = r.fps; o.winGen[b] = r.gen; o.winN[b] = r.n; RlPlace(b * 120, (b + 1) * 120, &o.plMed[b], &o.plLo[b], &o.plHi[b], &o.pl05[b], &o.pl95[b]); }
            { LONG m[9]; int k = 0; for (int b = 0; b < 9; b++) if (o.plMed[b] > 0) m[k++] = o.plMed[b]; std::sort(m, m + k); o.plMedAll = k ? m[k / 2] : 0; }
            const RtStat r = RtRead(240, 1080); o.fpsSteady = r.fps; o.shownSteady = r.n ? 100.0 * r.gen / r.n : 0.0;
            // (the engage's first second, about 50 pairs at the lock at 300 MHz, and everything behind it: the brief's two targets)
            const RtStat f = RtRead(0, 50), b2 = RtRead(50, 1080);
            o.shown1s = f.n ? 100.0 * f.gen / f.n : 0.0; o.shownAfter1s = b2.n ? 100.0 * b2.gen / b2.n : 0.0;
        }
        {   // (2026-10-17) THE FAKE'S OWN ENTRY TO THE HANDOFF'S RETIRE, as the simulation put it, over the first 240 pairs (the leg's json says the
            // same of the leg's), the pairs under half the period in force among them, and the median behind them
            const double q = (double)B::Qpf() / 1000000.0; std::vector<LONG> v, w; LONG under = 0;
            for (size_t i = 0; i < gRt.pairs.size(); i++)
            {
                const RtPair& x = gRt.pairs[i];
                if (x.n <= 0 || x.tRet < x.tEntry || x.tEntry <= 0) continue;
                const LONG us = (LONG)((double)(x.tRet - x.tEntry) / q + 0.5);
                if (i < 240) { v.push_back(us); if (x.perUs > 0 && us * 2 < x.perUs) under++; } else w.push_back(us);
            }
            std::sort(v.begin(), v.end()); std::sort(w.begin(), w.end());
            o.lsN = (LONG)v.size(); o.lsUnder = under;
            if (!v.empty()) { o.ls05 = v[v.size() * 5 / 100]; o.ls50 = v[v.size() / 2]; o.ls95 = v[v.size() * 95 / 100]; o.lsMost = v.back(); }
            if (!w.empty()) o.ls2_50 = w[w.size() / 2];
        }
        o.kN = a.pl.kFullN; o.kMed = LaPct(a.pl.kFull, a.pl.kFullN, 50); o.k95 = LaPct(a.pl.kFull, a.pl.kFullN, 95);
        o.room = a.pl.roomUs; o.rideF = a.pl.rideUs; o.tsNever = a.pl.tsNever;
        o.srcAfter = a.w.rideSrcN - o.g.w[0].src; o.seeded = a.pl.roomSeeded;   // (2026-10-17)
    }
    o.shownAll = o.g.n ? 100.0 * o.g.gen / o.g.n : 0.0;
    o.n0 = o.g.w[0].n; o.g0 = o.g.w[0].gen; o.n1 = o.g.w[1].n; o.g1 = o.g.w[1].gen;
    o.waitsAfter = o.g.waits - o.g.w[0].waits; o.relPres0 = o.g.w[0].relPres;
    o.served = gRt.served; o.holds = gRt.holds;
    return o;
}
static void RlJudge(const RlLeg& l, const RlOut& o, const char* tag)
{
    printf("  %s %s: the leg %.1f of 100 shown, %.1f real, the first two windows %d and %d; THE REPLAY %.1f of 100 (%d of %d), %.1f real at %.3f ms, %d of %d and %d of %d;"
           " order waits %ld (after the first window %ld of %ld sourced ride pairs), by the worker ahead of a present in it %ld, in all %ld, by the deadline %ld, the engage's first at pair %ld; the room floor seeded %ld time(s); the worker's waits served %ld, the presenter's holds %ld\n",
        tag, l.when, l.shown, l.fps, l.w0, l.w1, o.shownAll, o.g.gen, o.g.n, o.g.set.fps, o.g.set.perMs, o.g0, o.n0, o.g1, o.n1,
        o.g.waits, o.waitsAfter, o.srcAfter, o.relPres0, o.g.relPres, o.g.relDead, o.g.armedP, o.seeded, o.served, o.holds);
    printf("    not shown: late %ld, no source %ld, fed with a reset %ld, forced %ld, no answer %ld; dispatches first %ld, behind the scene %ld, behind an order wait %ld; K over full dispatches median %ld us, 95th %ld (%ld); the room floor %ld us, the ride's %ld, timed from the arm %ld; %s\n",
        o.why[FG_AW_LATE], o.why[FG_AW_NOSOURCE], o.why[FG_AW_RESET], o.why[FG_AW_FORCED], o.why[FG_AW_NOANSWER], o.first, o.behind, o.ordered, o.kMed, o.k95, o.kN, o.room, o.rideF, o.tsNever, RfLine(o.g));
    printf("    (2026-10-17) the fake's entry to the handoff's retire over the first 240 pairs: p05 %ld us, p50 %ld, p95 %ld, most %ld (%ld pairs, %ld under half the period in force); behind them p50 %ld; dispatches starved behind the next frame %ld; the picture's age median %.1f ms;"
           " at the end the ride's place %ld us behind the retire, the sixth largest needs: even %ld, wait %ld, fit %ld; real frames forced ahead of a wait %ld, ride pairs fired ahead of the body %ld\n",
        o.ls05, o.ls50, o.ls95, o.lsMost, o.lsN, o.lsUnder, o.ls2_50, o.starved, o.g.ageMed, o.rideD, o.needE, o.needW, o.needF, o.guardForced, o.rideAhead);
    printf("    (2026-10-17) by windows of 120, shown and real:");
    for (int b = 0; b < 9; b++) printf(" %d/%d %.1f;", o.winGen[b], o.winN[b], o.winFps[b]);
    printf(" behind the first two windows %.1f of 100 shown, %.1f real; the first window not shown: reset %ld, no source %ld, forced %ld, late %ld; real frames forced ahead of a wait %ld\n", o.shownSteady, o.fpsSteady,
        gRlWhy0[FG_AW_RESET], gRlWhy0[FG_AW_NOSOURCE], gRlWhy0[FG_AW_FORCED], gRlWhy0[FG_AW_LATE], gRlGuard0);
    {   // (2026-10-17) THE STEADY WINDOWS BY KIND (behind the engage's first two), the gate's own numbers: of 100 pairs there
        const double ns = o.g.n > 240 ? (double)(o.g.n - 240) : 1.0;
        printf("    (2026-10-17) THE STEADY WINDOWS (%d pairs): ride pairs fired ahead of the body %ld (%.2f of 100), real frames forced ahead of a wait %ld; not shown: forced %ld (%.2f), no source %ld (%.2f), fed with a reset %ld (%.2f), late %ld"
               " [RL8's leg, windows 11 to 56: ahead of the body 2.37, forced 1.14, no source 0.96, reset 0.96 of 100]\n",
            (int)ns, o.stA, 100.0 * o.stA / ns, o.stG, o.stF, 100.0 * o.stF / ns, o.stN, 100.0 * o.stN / ns, o.stR, 100.0 * o.stR / ns, o.stL);
    }
    RlCardLine(tag, o);
    RfSound(tag, o.g);
    if (l.latch)
    {   // (2026-10-17: THE LATCH'S LEG ON TODAY'S MACHINE, the event planted: no latch is left to fire, so the leg's own numbers, which were the
        // latch's, are not held. What is: the order wait stands on every sourced ride pair behind the first window, and the afternoon's 90)
        if (o.waitsAfter != o.srcAfter || o.srcAfter < 600) Err("%s: behind the first window %ld order wait(s) of %ld sourced ride pairs (one each, of 600 or more, wanted: nothing switched it off)", tag, o.waitsAfter, o.srcAfter);
        if (o.shownAll < 90.0) Err("%s: %.1f of 100 generated frames shown on today's machine (90 or more wanted)", tag, o.shownAll);
        return;
    }
    if (l.firstTold == 2)
    {   // (2026-10-17: THE LEG WITH NO ORDER WAIT IS WAIVED by the main session: the product never runs a ride pair without it now. A KNOWN LIMIT OF
        // THE FAKE: it does not starve the helper there (today's floors follow the ride's wait, which in the fake grows with the CPU's lead, so the
        // period stays over the card's own cycle), and so it cannot price what the order wait itself costs the card either)
        printf("  %s KNOWN LIMIT, told and not judged: %.1f of 100 where the leg read %.1f\n", tag, o.shownAll, l.shown);
        return;
    }
    if (l.s2 > 0)
    {   // (phase 5, P3: THE GATE OF STAGE 3's LEGS, BRIEF-P3-place.md point 1: shown within 1, real within 2, on the card within 25, and each share of the
        // placement within 10 of 100, before any design is judged on them)
        const double rides = o.posRides > 0 ? (double)o.posRides : 1.0;
        const double pos = 100.0 * o.posFires / rides, due = 100.0 * o.posDue / rides, inj = 100.0 * o.posInj / rides;
        LONG out = 0, outW = 0; for (int w = 0; w < 9; w++) if (o.winN[w] > 0) { outW++; if (o.plMed[w] && (o.plLo[w] < 300 || o.plHi[w] > 700)) out++; }
        printf("  %s THE GATE: shown %.1f (the leg %.1f, within 1), real %.1f (%.1f, within 2), on the card %ld (%ld, within 25); of 100 ride pairs at the position %.1f (%.1f), at the clock due %.1f (%.1f),"
               " at the next injection %.1f (%.1f), each within 10; TOLD: positions met with no answer %ld, waits there %ld (the answer came %ld, the longest %ld us); windows with a pair outside 300 to 700 %ld of %ld; f at the end %ld of 32\n",
            tag, o.shownAll, l.shown, o.g.set.fps, l.fps, o.plMedAll, l.s2, pos, l.posPct, due, l.duePct, inj, l.injPct, o.posNoAns, o.posWaits, o.posGot, o.posMaxUs, out, outW, o.placeF);
        if (fabs(o.shownAll - l.shown) > 1.0) Err("%s THE GATE: %.1f of 100 generated frames shown where the leg read %.1f (within 1 wanted)", tag, o.shownAll, l.shown);
        if (fabs(o.g.set.fps - l.fps) > 2.0) Err("%s THE GATE: %.1f real frames a second where the leg read %.1f (within 2 wanted)", tag, o.g.set.fps, l.fps);
        if (o.plMedAll < l.s2 - 25 || o.plMedAll > l.s2 + 25) Err("%s THE GATE: on the card %ld where the leg read %ld (within 25 wanted)", tag, o.plMedAll, l.s2);
        if (fabs(pos - l.posPct) > 10.0 || fabs(due - l.duePct) > 10.0 || fabs(inj - l.injPct) > 10.0)
            Err("%s THE GATE: of 100 ride pairs at the position %.1f, the clock due %.1f, the injection %.1f where the leg's were %.1f, %.1f, %.1f (each within 10 wanted)", tag, pos, due, inj, l.posPct, l.duePct, l.injPct);
    }
    if (fabs(o.shownAll - l.shown) > 3.0) Err("%s: %.1f of 100 generated frames shown where the leg read %.1f (within 3 wanted)", tag, o.shownAll, l.shown);
    if (fabs(o.g.set.fps - l.fps) > 2.0) Err("%s: %.1f real frames a second where the leg read %.1f (within 2 wanted)", tag, o.g.set.fps, l.fps);
    // (the first two windows' shape: each within 15 of 120 of the leg's; where the first is a known spread, the second alone, the first told)
    if (l.firstTold) { if (abs(o.g1 - l.w1) > 15) Err("%s: the engage's second window %d of 120 where the leg read %d (within 15 wanted)", tag, o.g1, l.w1); printf("  %s the engage's first window %d of 120 (the leg %d): a known spread, told\n", tag, o.g0, l.w0); }
    else if (abs(o.g0 - l.w0) > 15 || abs(o.g1 - l.w1) > 15) Err("%s: the engage's first two windows %d and %d of 120 where the leg read %d and %d (within 15 wanted)", tag, o.g0, o.g1, l.w0, l.w1);
    // (2026-10-17, the main session's gate: THE STEADY WINDOWS BY KIND, each within 0.6 of 100 of the leg's)
    if (l.stA > 0.0)
    {
        const double ns = o.g.n > 240 ? (double)(o.g.n - 240) : 1.0;
        const double a = 100.0 * o.stA / ns, f = 100.0 * o.stF / ns, nn = 100.0 * o.stN / ns, r = 100.0 * o.stR / ns;
        if (fabs(a - l.stA) > 0.6 || fabs(f - l.stF) > 0.6 || fabs(nn - l.stN) > 0.6 || fabs(r - l.stR) > 0.6)
            Err("%s: the steady windows lost, of 100: ahead of the body %.2f, forced %.2f, no source %.2f, reset %.2f where the leg's lost %.2f, %.2f, %.2f, %.2f (each within 0.6 wanted)", tag, a, f, nn, r, l.stA, l.stF, l.stN, l.stR);
    }
    if (l.ord && o.g.relPres) Err("%s: %ld release(s) by the worker ahead of a present (none wanted on this leg)", tag, o.g.relPres);
}
// THE MACHINE NOW ON A LEG'S SHAPE (2026-10-17, stage 3): the same run with stage 3's keys (`now`), judged against the targets: at the lock at
// 300 MHz 99 of 100 or more behind the engage's first second and 95 or more in it, no source and resets near 0 in the steady windows (a
// quarter of one in 100 each at the most), the real rate no lower than the leg's machine gave on the same shape; at 960 and on the free card
// the leg's bounds as before or better. Every picture in its place and each real frame once (RfSound).
static void RlNowJudge(const RlLeg& l, int now, const RlOut& b)
{
    const RingStats stc = gRingSt;
    gRlNow = now; const RlOut o = RlRun(l, 0);
    gRlNow = 0;
    ViolRestore(stc);
    char tag[48]; snprintf(tag, sizeof(tag), "%s NOW", l.name);
    const double ns = o.g.n > 240 ? (double)(o.g.n - 240) : 1.0;
    printf("  %s (stage 3's keys %d): %.1f of 100 shown (the leg's machine on the same shape %.1f), in the engage's first second %.1f, behind it %.1f; %.1f real (%.1f); the steady windows lost, of 100: ahead of the body %.2f, forced %.2f, no source %.2f, reset %.2f (%.2f, %.2f, %.2f, %.2f);"
           " the prepare's waits behind the answer %ld, the prepare done behind %ld, the longest %ld us; the picture's age median %.1f ms (%.1f)\n",
        tag, now, o.shownAll, b.shownAll, o.shown1s, o.shownAfter1s, o.g.set.fps, b.g.set.fps,
        100.0 * o.stA / ns, 100.0 * o.stF / ns, 100.0 * o.stN / ns, 100.0 * o.stR / ns, 100.0 * b.stA / ns, 100.0 * b.stF / ns, 100.0 * b.stN / ns, 100.0 * b.stR / ns,
        o.prepWaits, o.prepGot, o.prepMaxUs, o.g.ageMed, b.g.ageMed);
    printf("    %s by windows of 120:", tag);
    for (int w = 0; w < 9; w++) printf(" %d/%d %.1f;", o.winGen[w], o.winN[w], o.winFps[w]);
    printf(" the first window not shown: reset %ld, no source %ld, forced %ld, late %ld, fact %ld; at the end the ride's floor %ld us (needs: even %ld, wait %ld, fit %ld), the ride's place %ld us\n",
        gRlWhy0[FG_AW_RESET], gRlWhy0[FG_AW_NOSOURCE], gRlWhy0[FG_AW_FORCED], gRlWhy0[FG_AW_LATE], gRlWhy0[FG_AW_FACT], o.rideF, o.needE, o.needW, o.needF, o.rideD);
    RlCardLine(tag, o);
    printf("    %s THE PLACEMENT: ride pairs %ld, given a position %ld; fired at the position %ld, waits there %ld (the answer came %ld, the longest %ld us), met with no answer %ld; fallbacks: the injection %ld, the clock due %ld; f at the end %ld of 32, moved %ld time(s), places taken %ld\n",
        tag, o.posRides, o.posArms, o.posFires, o.posWaits, o.posGot, o.posMaxUs, o.posNoAns, o.posInj, o.posDue, o.placeF, o.placeSteps, o.placeN);
    RfSound(tag, o.g);
    printf("    %s THE AGE AND THE QUEUE behind the engage's first two windows: the picture's age median %.1f ms, 95th %.1f, worst %.1f; the card's queue ahead of a frame's begin median %ld us, 95th %ld, most %ld\n",
        tag, o.age50, o.age95, o.ageMax, o.depth50, o.depth95, o.depthMax);
    if (l.lock != 0 && !(now & RL_NOW_FITONLY))
    {   // (TOLD, the floors' question, stage 3's step 3: the same with the ride's floor FIT alone. The main session's rule: out only where the period
        // comes down WITHOUT the age or the queue's depth rising and without a single frame lost)
        gRlNow = now | RL_NOW_FITONLY; const RlOut t = RlRun(l, 0); gRlNow = 0;
        ViolRestore(stc);
        const double tns = t.g.n > 240 ? (double)(t.g.n - 240) : 1.0;
        printf("    %s TOLD, FIT ALONE (WAIT and EVEN out): %.1f of 100 (%.1f), behind the first second %.1f (%.1f); %.1f real (%.1f); the steady windows lost ahead %.2f, forced %.2f, no source %.2f, reset %.2f; age median %.1f ms (%.1f), 95th %.1f (%.1f), worst %.1f (%.1f); the queue median %ld us (%ld), 95th %ld (%ld), most %ld (%ld); on the card %ld (%ld); the ride's floor at the end %ld us (%ld)\n",
            tag, t.shownAll, o.shownAll, t.shownAfter1s, o.shownAfter1s, t.g.set.fps, o.g.set.fps, 100.0 * t.stA / tns, 100.0 * t.stF / tns, 100.0 * t.stN / tns, 100.0 * t.stR / tns,
            t.age50, o.age50, t.age95, o.age95, t.ageMax, o.ageMax, t.depth50, o.depth50, t.depth95, o.depth95, t.depthMax, o.depthMax, t.plMedAll, o.plMedAll, t.rideF, o.rideF);
        RfSound("told FIT alone", t.g);
        gRlNow = now | RL_NOW_NOWAIT; const RlOut u = RlRun(l, 0); gRlNow = 0;
        ViolRestore(stc);
        printf("    %s TOLD, THE WAIT FLOOR OUT, EVEN KEPT: %.1f of 100, behind the first second %.1f; %.1f real; the steady windows lost ahead %.2f, forced %.2f, no source %.2f, reset %.2f; age median %.1f ms, 95th %.1f, worst %.1f; the queue median %ld us, 95th %ld, most %ld; on the card %ld; the ride's floor at the end %ld us\n",
            tag, u.shownAll, u.shownAfter1s, u.g.set.fps, 100.0 * u.stA / tns, 100.0 * u.stF / tns, 100.0 * u.stN / tns, 100.0 * u.stR / tns, u.age50, u.age95, u.ageMax, u.depth50, u.depth95, u.depthMax, u.plMedAll, u.rideF);
        RfSound("told the wait floor out", u.g);
    }
    if ((now & RL_NOW_PLACE) && l.lock != 0)
    {   // (ON THE CARD, the planted legs: the median of the windows' medians 500 plus or minus 25, every window's median within 400 to 600; the 5th and
        // 95th of each window's pairs are told beside it)
        if (o.plMedAll < 475 || o.plMedAll > 525) Err("%s: on the card the median of the windows' medians %ld (500 plus or minus 25 wanted)", tag, o.plMedAll);
        for (int w = 0; w < 9; w++) if (o.plMed[w] && (o.plMed[w] < 400 || o.plMed[w] > 600)) Err("%s: on the card window %d's median %ld (within 400 to 600 wanted)", tag, w + 1, o.plMed[w]);
    }
    if (l.lock == 70)
    {
        if (o.shownAfter1s < 99.0) Err("%s: %.1f of 100 shown behind the engage's first second (99 or more wanted)", tag, o.shownAfter1s);
        if (100.0 * o.stN / ns > 0.25 || 100.0 * o.stR / ns > 0.25) Err("%s: the steady windows lost %.2f of 100 for no source and %.2f for a reset (a quarter at the most each wanted)", tag, 100.0 * o.stN / ns, 100.0 * o.stR / ns);
        // (TOLD, the planting's and the floors' to judge: the engage's first second, 95 or more wanted at the end of stage 3; the rate against the leg's machine)
        printf("    %s TOLD: in the engage's first second %.1f of 100 (95 or more is stage 3's target); %.1f real frames a second where the leg's machine gave %.1f\n", tag, o.shown1s, o.g.set.fps, b.g.set.fps);
        if (!gRlPrepMs && (now & RL_NOW_PREP))
        {   // (TOLD: the same with the prepare's wait bounded at 16 ms, for the main session's decision on the bound)
            gRlPrepMs = 16; gRlNow = now; const RlOut t = RlRun(l, 0); gRlNow = 0; gRlPrepMs = 0;
            ViolRestore(stc);
            printf("    %s TOLD, the prepare's wait bounded at 16 ms: %.1f of 100, behind the first second %.1f, in it %.1f; the steady windows lost ahead %.2f, forced %.2f, no source %.2f, reset %.2f; waits %ld, the prepare done behind %ld, the longest %ld us; %.1f real\n",
                tag, t.shownAll, t.shownAfter1s, t.shown1s, 100.0 * t.stA / ns, 100.0 * t.stF / ns, 100.0 * t.stN / ns, 100.0 * t.stR / ns, t.prepWaits, t.prepGot, t.prepMaxUs, t.g.set.fps);
            RfSound("told 16 ms", t.g);
        }
    }
    else
    {
        if (o.shownAll < l.shown - 3.0) Err("%s: %.1f of 100 shown where the leg read %.1f (as before or better wanted)", tag, o.shownAll, l.shown);
        if (fabs(o.g.set.fps - l.fps) > 2.0) Err("%s: %.1f real frames a second where the leg read %.1f (within 2 wanted)", tag, o.g.set.fps, l.fps);
    }
}
// THE SECOND POSITION ON STAGE 3's LEGS (phase 5, P3; fg_machine.h, "THE SECOND POSITION"), the leg's machine (`o`, the gate's run) against the same
// run with FgArmedKeys::place2: every picture in its place and each real frame once; shown and the real rate not lower (a tenth of 100 and a
// quarter of a frame a second of the simulation's own wobble); no window's median on the card moved more than 15 of 1000; second positions armed
// where positions met no answer, and the windows with a pair outside 300 to 700 not more (BRIEF-P3-place.md aimed at the leg's 14 of 53 toward
// under 5: NOT REACHED, fg_machine.h's "THE SECOND POSITION" says why; RL10 reads 3 of 9 both ways, 4 of 25 late positions caught).
static void RlP2Judge(const RlLeg& l, const RlOut& o)
{
    const RingStats stc = gRingSt;
    gRlNow = RL_NOW_PLACE2; const RlOut t = RlRun(l, 0); gRlNow = 0;
    ViolRestore(stc);
    char tag[48]; snprintf(tag, sizeof(tag), "%s SECOND", l.name);
    LONG outO = 0, outT = 0, moved = 0, movedMax = 0;
    for (int w = 0; w < 9; w++)
    {
        if (o.plMed[w] && (o.plLo[w] < 300 || o.plHi[w] > 700)) outO++;
        if (t.plMed[w] && (t.plLo[w] < 300 || t.plHi[w] > 700)) outT++;
        const LONG d = labs(t.plMed[w] - o.plMed[w]); if (d > movedMax) movedMax = d; if (d > 15) moved++;
    }
    const double rides = t.posRides > 0 ? (double)t.posRides : 1.0;
    printf("  %s (FgArmedKeys::place2): %.1f of 100 shown (the leg's machine %.1f), %.1f real (%.1f); of 100 ride pairs at the position %.1f, of them at the second %.1f, at the clock due %.1f, at the injection %.1f;"
           " positions met with no answer %ld (%ld), second positions armed %ld, fired there %ld; windows with a pair outside 300 to 700 %ld of 9 (%ld); the windows' medians moved at most %ld of 1000\n",
        tag, t.shownAll, o.shownAll, t.g.set.fps, o.g.set.fps, 100.0 * t.posFires / rides, 100.0 * t.pos2Fires / rides, 100.0 * t.posDue / rides, 100.0 * t.posInj / rides,
        t.posNoAns, o.posNoAns, t.pos2Arms, t.pos2Fires, outT, outO, movedMax);
    RlCardLine(tag, t);
    RfSound(tag, t.g);
    if (t.shownAll < o.shownAll - 0.1) Err("%s: %.1f of 100 shown where the leg's machine showed %.1f (not lower wanted)", tag, t.shownAll, o.shownAll);
    if (t.g.set.fps < o.g.set.fps - 0.25) Err("%s: %.1f real frames a second where the leg's machine gave %.1f (not lower wanted)", tag, t.g.set.fps, o.g.set.fps);
    if (moved) Err("%s: %ld window median(s) on the card moved more than 15 of 1000 (at most %ld): none wanted", tag, moved, movedMax);
    if (o.posNoAns > 0 && (t.pos2Arms <= 0 || outT > outO)) Err("%s: second positions armed %ld, windows with a pair outside 300 to 700 %ld where the leg's machine had %ld (some armed and no more such windows wanted)", tag, t.pos2Arms, outT, outO);
}
static void CaseRl(int i)
{
    const RlLeg& l = kRl[i];
    char name[256]; snprintf(name, sizeof(name), "%s the leg of %s replayed: shown of 100, the first two windows, the real rate (2026-10-14, evening)", l.name, l.when);
    CaseBegin(name);
    const RingStats stc = gRingSt;
    // (2026-10-17: the latch's leg runs with its event planted from the start, the placer and the stamper held behind the pair of frame 40:
    // the plant now ends, and on the machine of before that day it did not bring the event either, for the early show released the wait)
    RlOut o = RlRun(l, l.latch ? 1 : 0);
    RlOut c = {};
    if (l.latch) { RlLeg n = l; n.ord = 0; c = RlRun(n, 1); }
    ViolRestore(stc);
    RlJudge(l, o, l.name);
    if (l.latch)
    {   // the control: the same run with no order wait at all, which is what the latch left the leg with: the check of the waits sees it
        printf("  CONTROL, the same with no order wait (what the latch left that evening): %.1f of 100, waits behind the first window %ld of %ld sourced ride pairs\n", c.shownAll, c.waitsAfter, c.srcAfter);
        if (c.waitsAfter != 0 || c.srcAfter < 600) Err("%s control: with no order wait %ld wait(s) behind the first window of %ld sourced ride pairs (none, of 600 or more, wanted): the control shows nothing", l.name, c.waitsAfter, c.srcAfter);
        Ctl("no order wait: %.1f of 100 shown, %ld wait(s) behind the first window", c.shownAll, c.waitsAfter);
    }
    if (i >= 5 && !l.keys) RlNowJudge(l, RL_NOW_ALL, o);      // (a leg of stage 3 ran stage 3's keys itself: its run above is that)
    if (l.keys & RL_NOW_PLACE) RlP2Judge(l, o);               // (phase 5, P3: and the second position on it)
    CaseEnd(name);
}
static void CaseRL1() { CaseRl(0); }
static void CaseRL2() { CaseRl(1); }
static void CaseRL3() { CaseRl(2); }
static void CaseRL4() { CaseRl(3); }
static void CaseRL5() { CaseRl(4); }
static void CaseRL6() { CaseRl(5); }
static void CaseRL7() { CaseRl(6); }
static void CaseRL8() { CaseRl(7); }
static void CaseRL9() { CaseRl(8); }
static void CaseRL10() { CaseRl(9); }
static void CaseRL11() { CaseRl(10); }

// ---------------------------------------------------------------- HR1, THE HUD-LESS PICTURE'S SECOND TARGET ACROSS ENGAGES (phase 5, rounds 4 and 5)
// RUN BY NAME ALONE (dt_fg_test 2 HR1). THE OWNER'S LEG F1C lawon of 2026-10-17 (pair 9415cf8c): the law engaged, stood down to REST for 70
// windows, engaged again, and the second stint showed 0 of 120 generated frames a window with no cause on the window line; its HUD-less line said
// why: every frame "no HUD-less picture (no store or shader)", the encode's second target "does not stand (made 1 time(s), given back 1)". THE
// TARGET IS MADE AND GIVEN BACK BY fg_hud.h's FgHudTargetTick, whose decisions are fg_hud_rule.h's FgHudT2Tick and FgHudT2FailedAfter: THIS CASE
// RUNS THOSE TWO, once a frame as the encode does, and hands the machine each frame's answer from them (the target stands: TEX; it does not, on a
// frame with an interface: NONE, the frame's generated frame withheld for it, FG_AW_NOHUD). The Direct3D create is the one stand-in (it succeeds,
// or fails where the run plants it). THREE RUNS, each on RL10's busy card and RL9's free card:
//   A, THE REST (round 4): 360 frames engaged, 700 standing by past FG_HUDT2_IDLE's 600 (the limit where nothing can engage, the one that gives
//     the target back), 480 engaged again: HELD behind each engage's first 120 frames 90 of 100 shown, none for want of a picture, made twice.
//     CONTROL (a build of its own, -DFGHUD_T2_TRIED_AT_TRY): made once, the second engage 0 shown.
//   B, A FAILED MAKE (round 5): engaged from the start, the first make planted to fail for now: asked again FG_HUDT2_RETRY frames on, made, and
//     behind that the engage shows 90 of 100; a permanent refusal (a multisampled back buffer) is never asked again. CONTROL (a build of its own,
//     -DFGHUD_T2_FAIL_LATCHED, round 4's rule): never made again, 0 shown.
//   C, THE FILL ALTERNATION'S TURNS (round 5): four turns of 950 frames engaged and 950 standing by, the driver able to engage throughout
//     (FG_HUDT2_IDLE_DRIVEN): the target made once and never given back. CONTROL (an arm of the same build): the same turns with the limit at
//     600 remake it at every turn.
// WHY NO CASE CAUGHT ROUND 4's FAULT: every case of the HUD-less picture hands the machine the answer from a stand-in, ShArmB::hud (dt_fg_shadow.h),
// which says TEX for ever once a case sets it; the law's cases (LW3, LW4: engages and stand-downs) and the replays run with no HUD-less picture
// at all (ShB::hasHudless 0, frames with no interface). No case ran the target's own life across a stand-down.
struct Hr1Out { LONG made, given, failNow, failGood, noHud2; double sh1, sh2; int g1, n1, g2, n2; LONG un; };
// run: 0 the REST, 1 a failed make (perm: refused for good), 2 the alternation's turns at idleMax
static Hr1Out Hr1Run(int shape, int run, bool perm, unsigned idleMax)
{
    Hr1Out o = {};
    const RlLeg& l = shape == 0 ? kRl[9] : kRl[8];            // (RL10's busy card at 300 MHz; RL9's free card)
    RtPar p = RlPar(l, 0);
    RtBegin(p);
    ShB::hasHudless = 1; ShArmB::hud = FG_HUD_TEX;
    FgState& s = B::S(); const FgArmedState& a = ShArmB::a;
    gRtSeed = 20261018u;
    bool stands = false, failed = false; unsigned idle = 0, retryAt = 0; int tries = 0;
    LONG un0 = s.c.pubArmed; for (int k = 0; k < FG_AW_N; k++) un0 -= a.w.iWhy[k];
    const int E1 = 360, SB = 700, E2 = 480, TURN = 950, TURNS = 4;
    const int frames = run == 0 ? E1 + SB + E2 : run == 1 ? 1200 : 2 * TURN * TURNS;
    // (the stretches read: run 0, behind each engage's first 120 frames; run 1, the engage from the retry's frame on, behind 120 more; run 2, the last turn)
    int r1a = 0, r1b = 0, r2a = 0, r2b = 0;
    if (run == 0) { r1a = 120; r1b = E1; r2a = E1 + SB + 120; r2b = frames; }
    else if (run == 1) { r1a = 0; r1b = 0; r2a = FG_HUDT2_RETRY + 120; r2b = frames; }
    else { r1a = 120; r1b = TURN; r2a = 2 * TURN * (TURNS - 1) + 120; r2b = 2 * TURN * (TURNS - 1) + TURN; }
    size_t pairAt[4] = {}; LONG noHudAt = 0;
    for (int f = 0; f < frames; f++)
    {
        const bool engaged = run == 0 ? (f < E1 || f >= E1 + SB) : run == 1 ? true : ((f / TURN) % 2) == 0;
        if (f == r1a) pairAt[0] = gRt.pairs.size(); if (f == r1b) pairAt[1] = gRt.pairs.size();
        if (f == r2a) { pairAt[2] = gRt.pairs.size(); noHudAt = a.w.iWhy[FG_AW_NOHUD]; }
        s.wt.want = engaged ? FG_WANT_ON : FG_WANT_PASS;
        // (THE ENCODE'S TICK, fg_hud.h's own two rules; the encode's count is the frame's)
        const int act = FgHudT2Tick(engaged, stands, failed, (unsigned)f, retryAt, &idle, run == 0 ? (unsigned)FG_HUDT2_IDLE : idleMax);
        if (act == FG_HUDT2_MAKE)
        {
            const bool ok = !(run == 1 && (perm || tries == 0));   // (run B: the first try fails, for now or for good)
            tries++;
            if (ok) { stands = true; o.made++; idle = 0; retryAt = 0; failed = FgHudT2FailedAfter(true, false); }
            else { failed = FgHudT2FailedAfter(false, perm); if (failed) o.failGood++; else { o.failNow++; retryAt = (unsigned)f + FG_HUDT2_RETRY; } }
        }
        else if (act == FG_HUDT2_GIVE) { stands = false; o.given++; idle = 0; }
        ShArmB::hud = stands ? FG_HUD_TEX : FG_HUD_NONE;
        RtFrame(RtMs(gRlCostMs * RtSpread(gRlSpread)));
    }
    if (r2b >= frames) pairAt[3] = gRt.pairs.size(); else pairAt[3] = pairAt[2] + (size_t)(r2b - r2a);
    auto read = [&](size_t from, size_t to, int* gen, int* n) { *gen = 0; *n = 0; for (size_t i = from; i < to && i + 1 < gRt.pairs.size(); i++) { (*n)++; if (gRt.pairs[i].gen == 1) (*gen)++; } };
    read(pairAt[0], pairAt[1], &o.g1, &o.n1); read(pairAt[2], pairAt[3], &o.g2, &o.n2);
    o.sh1 = o.n1 ? 100.0 * o.g1 / o.n1 : 0.0; o.sh2 = o.n2 ? 100.0 * o.g2 / o.n2 : 0.0;
    o.noHud2 = a.w.iWhy[FG_AW_NOHUD] - noHudAt;
    LONG un = s.c.pubArmed; for (int k = 0; k < FG_AW_N; k++) un -= a.w.iWhy[k]; o.un = un - un0;
    RfLeg g = {}; g.order = gRt.order; g.first = gRt.firstBad;
    for (size_t i = 0; i + 1 < gRt.pairs.size(); i++) { g.n++; if (gRt.pairs[i].real == 1) g.real1++; }
    RtEnd("HR1");
    g.blitsMax = ShArmB::ordBlitsMax;
    ShB::hasHudless = 0; ShArmB::hud = 0;
    char tag[96]; snprintf(tag, sizeof(tag), "HR1 %s, run %c%s", shape == 0 ? "RL10's busy card" : "RL9's free card", 'A' + run, perm ? " (refused for good)" : "");
    printf("  %s: the target made %ld time(s), given back %ld, failed for now %ld, refused for good %ld; the first stretch %d of %d shown (%.1f of 100); the second %d of %d (%.1f of 100), withheld for want of a HUD-less picture %ld; armed less shown and every cause %ld\n",
        tag, o.made, o.given, o.failNow, o.failGood, o.g1, o.n1, o.sh1, o.g2, o.n2, o.sh2, o.noHud2, o.un);
    RfSound(tag, g);
    if (o.un < 0 || o.un > 2) Err("%s: armed less shown and every cause read %ld (the open pair or two at the most wanted: the window line's sum)", tag, o.un);
    return o;
}
static void CaseHR1()
{
    const char* const name = "HR1 the HUD-less picture's second target across engages: made again after a REST, a failed make asked again, kept through the alternation's turns (phase 5, rounds 4 and 5)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    for (int shape = 0; shape < 2; shape++)
    {
        const char* sn = shape == 0 ? "RL10's busy card" : "RL9's free card";
        const Hr1Out A = Hr1Run(shape, 0, false, FG_HUDT2_IDLE);
        if (A.made != 2 || A.given != 1 || A.sh1 < 90.0 || A.sh2 < 90.0 || A.noHud2 != 0)
            Err("HR1 %s, run A (the REST): made %ld, given back %ld, %.1f and %.1f of 100 shown behind each engage's first 120 frames, %ld for want of a picture in the second (2, 1, 90 or more each, 0 wanted)", sn, A.made, A.given, A.sh1, A.sh2, A.noHud2);
        const Hr1Out B = Hr1Run(shape, 1, false, FG_HUDT2_IDLE_DRIVEN);
        if (B.failNow != 1 || B.made != 1 || B.sh2 < 90.0 || B.noHud2 != 0)
            Err("HR1 %s, run B (a failed make): failed for now %ld, made %ld, %.1f of 100 shown behind the retry, %ld for want of a picture (1, 1, 90 or more, 0 wanted)", sn, B.failNow, B.made, B.sh2, B.noHud2);
        const Hr1Out Bp = Hr1Run(shape, 1, true, FG_HUDT2_IDLE_DRIVEN);
        if (Bp.failGood != 1 || Bp.made != 0) Err("HR1 %s, run B refused for good: refused %ld, made %ld (1 and 0 wanted: never asked again before a Reset)", sn, Bp.failGood, Bp.made);
        const Hr1Out C = Hr1Run(shape, 2, false, FG_HUDT2_IDLE_DRIVEN);
        if (C.made != 1 || C.given != 0 || C.sh1 < 90.0 || C.sh2 < 90.0)
            Err("HR1 %s, run C (the alternation's turns): made %ld, given back %ld, %.1f and %.1f of 100 shown in the first and the last turn (1, 0, 90 or more each wanted)", sn, C.made, C.given, C.sh1, C.sh2);
        const Hr1Out Cc = Hr1Run(shape, 2, false, FG_HUDT2_IDLE);
        if (Cc.made < 4 || Cc.given < 3) Err("HR1 %s control, the alternation's turns at today's 600: made %ld, given back %ld (4 and 3 or more wanted: remade at every turn): the control shows nothing", sn, Cc.made, Cc.given);
        Ctl("%s, the alternation's turns with the idle limit at 600: the target made %ld time(s) and given back %ld", sn, Cc.made, Cc.given);
    }
    ViolRestore(stc);
    CaseEnd(name);
}

// ---------------------------------------------------------------- PS1, EVERY PANEL (phase 5, P3; BRIEF-P3-place.md point 4 and the plan's P3 (4))
// RUN BY NAME ALONE (dt_fg_test 2 PS1). The armed build's machine as it stands (the ride, the order wait, the planting by the card, the prepare's
// wait; RlFree's card with the owner's 4K hand, so the card is free) over a fake game whose own cost sets the rate, on six shapes: 144 Hz at 60
// and 70 real, 180 Hz at 90, 240 Hz at 90, 100 and 118; each with the free card's answers (1,000 us) and with the lock at 300 MHz's (2,700 us, the
// plan 3,250); each with the room's margin as the product hands it (FgArmedKeys::roomGuardUs 1,250 us, which the machine applies to
// every pair since round 6: fg_machine.h, ArmedRoomMargin) and with 0 as the control. THE CARD IS FREE ON EVERY SHAPE, so no dispatch is
// starved behind the game's next scene (RtPar::starve 0; round 2 of phase 5: with it on, K stood at 4,700 us over the cap and the shapes at
// 144 Hz with 300 MHz's answers showed 2.5 to 24 of 100, the fake's own artefact).
// Read behind the engage's first two windows: of 100 generated frames shown, the real rate, the period in force, the real frames forced
// with no due (each such frame came before its own due: early on the screen), the generated frame's place on the card (the fake's shown
// instants, 500 the middle) and every picture in its place, each real frame once (RfSound, HELD on every shape and arm). HELD WITH THE PRODUCT'S
// MARGIN on every shape (round 2): no real frame forced with no due, 99 of 100 or more shown, on the card 500 plus or minus 25 (the brief's
// 240 at 100 among them). TOLD: the control's arm, and the rest.
static void CasePS1()
{
    const char* const name = "PS1 every panel: 144 Hz at 60 and 70, 180 Hz at 90, 240 Hz at 90, 100 and 118, the free card's answers and 300 MHz's, the room's margin 0 and 1,250 us (phase 5)";
    CaseBegin(name);
    struct Sh { const char* name; int hz; double fps; };
    static const Sh kSh[6] = { { "144 Hz at 60", 144, 60.0 }, { "144 Hz at 70", 144, 70.0 }, { "180 Hz at 90", 180, 90.0 }, { "240 Hz at 90", 240, 90.0 }, { "240 Hz at 100", 240, 100.0 }, { "240 Hz at 118", 240, 118.0 } };
    const RingStats stc = gRingSt;
    for (int i = 0; i < 6; i++)
        for (int slow = 0; slow < 2; slow++)
            for (int guard = 0; guard < 2; guard++)
            {
                RtPar p = RlFree();
                p.refresh = ((ULONGLONG)kSh[i].hz << 32) | 1ull; p.roomGuardUs = guard ? 1250 : 0;
                if (slow) { p.kWorkUs = 2700; p.planUs = 3250; }
                p.over = 1; p.ord = 1; p.card = 0; p.prepAtRet = 1; p.starve = 0; p.prepWait = 1; p.place = 1; p.latencyUs = 17000; p.calls = kRlCalls; p.inj = 0.95;
                p.rideEarly = 1; p.rideNoWait = 0; p.armTeach = 1; p.ordAtStamp = 1; p.rideWaitAns = 1; p.planMaxUs = 4500; p.leadScene = 1; p.serve = 1; p.roomSeed = 1;
                RtBegin(p);
                gRtSeed = 20261017u + (unsigned)(i * 4 + slow * 2);   // (round 2: one seed for both margins' arms, so their difference is the margin's alone)
                const double costMs = 1000.0 / kSh[i].fps;
                for (int f = 0; f < 600; f++) RtFrame(RtMs(costMs * RtSpread(0.05)));
                RfLeg g = {};
                g.set = RtRead(240, 600); g.order = gRt.order; g.first = gRt.firstBad;
                const int n = (int)gRt.pairs.size();
                for (int k = 0; k < n - 1; k++) { const RtPair& x = gRt.pairs[k]; g.n++; if (x.gen == 1) g.gen++; if (x.real == 1) g.real1++; }
                LONG med = 0, lo = 0, hi = 0, p05 = 0, p95 = 0; RlPlace(240, 600, &med, &lo, &hi, &p05, &p95);
                const FgArmedState& a = ShArmB::a;
                const LONG rides = a.w.posRides, fires = a.w.posFires, due = a.w.posDue, room = a.pl.roomUs, ride = a.pl.rideUs;
                const LONG wLate = a.w.iWhy[FG_AW_LATE], wSrc = a.w.iWhy[FG_AW_NOSOURCE], wRes = a.w.iWhy[FG_AW_RESET], wFor = a.w.iWhy[FG_AW_FORCED], wAns = a.w.iWhy[FG_AW_NOANSWER], wLf = a.w.iWhy[FG_AW_LATEFIRE];
                const LONG kMed = LaPct(a.pl.kFull, a.pl.kFullN, 50), plan = a.k.kPlanUs;
                const LONG pN = a.w.placeN, pK[5] = { a.w.placeNK[0], a.w.placeNK[1], a.w.placeNK[2], a.w.placeNK[3], a.w.placeNK[4] };
                LONG genShown = 0; for (int k = 0; k < n - 1; k++) if (gRt.pairs[k].gen == 1) genShown++;
                RtEnd("PS1");
                g.blitsMax = ShArmB::ordBlitsMax;
                char tag[96]; snprintf(tag, sizeof(tag), "PS1 %s, %s answers, margin %d us", kSh[i].name, slow ? "300 MHz's" : "the free card's", guard ? 1250 : 0);
                printf("  %s: %.1f of 100 shown, %.1f real at %.3f ms (the panel's floor %.3f ms); real frames forced with no due %d; on the card median %ld (5th to 95th %ld to %ld, least %ld, most %ld);"
                       " ride pairs %ld, fired at the position %ld, at the clock due %ld; the room floor %ld us, the ride's %ld; not shown in the run: late %ld, late fire %ld, no source %ld, reset %ld, forced %ld, no answer %ld; K's median %ld us, the plan %ld\n",
                    tag, g.set.n ? 100.0 * g.set.gen / g.set.n : 0.0, g.set.fps, g.set.perMs, 2000.0 / kSh[i].hz, g.set.forced, med, p05, p95, lo, hi, rides, fires, due, room, ride, wLate, wLf, wSrc, wRes, wFor, wAns, kMed, plan);
                RfSound(tag, g);
                // (round 2, the reviewer's finding 1: EVERY SHOWN GENERATED FRAME's place enters the machine's ring, whatever fired it; the fake hands a
                // place eight pairs late, as the product's ring of stamps, and only where the frame and both real frames were shown once)
                printf("    the machine's ring of places: %ld of %ld shown generated frames, by what fired them: the position %ld, the second %ld, the clock due %ld, the injection %ld, another point %ld\n",
                    pN, genShown, pK[0], pK[1], pK[2], pK[3], pK[4]);
                if (pN < genShown - 12 || pK[0] + pK[1] + pK[2] + pK[3] + pK[4] != pN)
                    Err("%s: the ring of places took %ld of %ld shown generated frames, %ld by kind (all but the last few, and every one by its kind, wanted)", tag, pN, genShown, pK[0] + pK[1] + pK[2] + pK[3] + pK[4]);
                if (guard)
                {   // (round 2: what ships, judged)
                    const double sh = g.set.n ? 100.0 * g.set.gen / g.set.n : 0.0;
                    if (g.set.forced || sh < 99.0 || med < 475 || med > 525)
                        Err("%s: %d real frame(s) forced with no due, %.1f of 100 shown, on the card %ld (none, 99 or more, 500 plus or minus 25 wanted)", tag, g.set.forced, sh, med);
                }
            }
    ViolRestore(stc);
    CaseEnd(name);
}

