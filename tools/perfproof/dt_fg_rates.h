// dt_fg_rates.h: A PAIR HELD AT OTHER REAL RATES THAN THE PANEL'S HALF, IN THE HARNESS (2026-10-12, phase 4 of the revival;
// docs/reports/fg-4/brief-2026-10-12/BRIEF-rates.md). THE FREE BAND: a game that cannot reach the pacer's floor (two
// refreshes times LOCK), whose period is its own.
//
// Included by dt_fg_test.cpp behind dt_fg_saves.h, with ONE line; its cases are FB1 to FB4 (one RUN line in RunCases,
// twice: the two orders of 'Q').
//
// WHAT RUNS. The merged machine (a binding that arms on the product's wire, not an instrument's: the shape the armed
// build compiles), CLOSED: the pacer paces the game it is given, the placer places the pairs the pacer's period
// makes, the worker shows them, and the screen's log is read with the instant of every present. Every other merged case
// steps one pair by hand at a period the harness chose (16.7 ms); here THE PERIOD IS THE MACHINE'S OWN ANSWER to a game's
// cost, over hundreds of frames, with the pacing law the armed development build runs (fg_law.h).
// SINCE 2026-10-17 (phase 5, the pacer's package) that law is THE FREE BAND'S FORM at a head of 1.0 (fg_law.h, FgLawStepForm;
// fg_machine.h, LawForm), the guard's bit 12 off being the law until then at 1.06: FB1 holds the game's own cost within
// 1 percent, FB2 tells a tail of frames past the period, FB4 runs EVERY PANEL (144, 180, 240 Hz beside FB1's 120). No case was added.
// (FB5, leg 7's forced period alternating with the floor, went with the forced period's key in round 9's S7.)
//
// THE FAKE GAME, on the harness's clock (every number below is handed in, RtPar):
//   a frame COSTS what the case says (the render thread's own work from the pacer's release to the entry at Present);
//   its INJECTION reaches the worker a share of that cost into the frame (0.9: the upscale is the last thing before
//     the interface. AN ASSUMPTION: no line of the game's says where it stands; the legs at 59 say only that it comes
//     behind the real frame's due, which puts it past 0.62);
//   its HANDOFF RETIRES a fixed time behind the entry (3,350 us: the sitting of 2026-10-12, DLSS Quality), signalled
//     THEN, by the simulation, where every other case's fake signals it inside the hand's own submit (RtB::RecSignal);
//   the helper answers at the retire (the answer's own lateness is P1's, P2's and T7's ground);
//   WHILE THE GAME WORKS a due the placer raises is served at once by a replayed call (a checkpoint, point A); WHILE THE
//     PACER SLEEPS it is served by the machine's own wake (RtPaceHook answers "the placer woke you", and SleepUntil
//     records its fire note), but for the sleep's last millisecond, where the real pacer spins on the clock and hears
//     no wake: there the due waits for the next frame's first call, as it would in the game.
// THE CLOCK moves only where the simulation puts it, to the instant of each event in order (a retire, the placer's
// next raise, the game's own milestones), so no verdict here depends on which thread ran first. The machine's reads
// of the clock on the render thread move it by one tick each (RtB::Now), so the pacer's last spin ends.
//
// WHAT IS READ, a pair: the entry at its Present, the period in force its hand carried, the instants of its generated
// frame's present and its real frame's. From them e, THE REAL FRAME'S PLACE AGAINST THE MIDDLE between its own generated
// frame and the next pair's (0: an even screen; under 0: the real frame is early), and THE PICTURE'S AGE at its present
// against the hand, the plan and half the period in force.
//
// WHAT IT CANNOT SHOW (a leg's): a card that is the limit (the worker held inside DXVK's Present, the gate before it),
// where the injection really stands in the game's frame, the panel's own behaviour between the floor and the free
// range, and the timers' lateness. The harness shows the machine's arithmetic and its order under a game it was told.
#pragma once
#include <deque>

struct RtPair { DWORD tag; LONG p, perUs, forced; LONGLONG cost, tEntry, tGen, tReal; int gen, real, genReset; LONG n; LONGLONG tRet;   // (n, tRet: 2026-10-17, the pairs' truth: the hand's pair on the wire and its handoff's retire as the simulation put it)
               LONGLONG dGen, dReal;      // (2026-10-17, night, F1: THE PANEL MODEL's instants the panel SHOWED the two: tGen and tReal stay the queue's, the card's)
               LONG kUs; int disp;        // (2026-10-18, S1: the pair's answer behind its handoff's retire as the simulation put it, and how its dispatch met the game's
                                          // work: 0 none, 1 an order wait stood, 2 first, 3 starved, 4 behind the scene; RtDisp)
               int gen2; LONGLONG tGen2, dGen2;      // (2026-10-18, round 3, G3: the pair's SECOND generated frame, GEN2's, as the card and the panel had it)
               LONG workUs; };                       // (2026-10-24, round 5, H: the dispatch's own card time RtDisp gave it: a picture's kWorkUs, a flow-only one's kFlowUs)
struct RtPar
{
    ULONGLONG refresh = (120ull << 32) | 1ull;   // the panel, packed as the mod reads it
    LONG lsUs = 3350, planUs = 1500;             // the entry to the handoff's retire; the planned generation time
    double inj = 0.9;                            // where in a frame's cost its injection reaches the worker
    // (law and freeMinUs went in round 9's LW, 2026-10-27: the law is every build's, the free range from the floor; the gap's
    // controls with them)
    LONG guardOff = 0;                           // FG_ARMED_GUARD_ON's bits switched off (16: the game's own period; 32: the free range by the panel)
    LONG roomGuardUs = 1250;                     // the room's margin, handed in: since round 2 of phase 5 THE PRODUCT'S VALUE (fg_arm_life.h, FgpArmKeys),
                                                 // which the machine applies to every pair since round 6 (fg_machine.h, ArmedRoomMargin)
    // ---- 2026-10-13 (the ride; "THE CARD'S ONE QUEUE" below)
    int ride = 1;                                // FgArmedKeys::ride, as the armed build hands it in: the ride asked for
    LONG rideGuardUs = 500;                      // ... and its margin
    // THE CARD'S ONE QUEUE (all 0: off, a present is on the screen at its call and the handoff retires lsUs behind the
    // entry, as every case before these ran). On: what a frame queues, in card time: the scene's tail still to run at
    // its injection; the helper's upscale, which the body's home copy WAITS for; the post chain and the hand's copies,
    // whose end is the handoff's retire; and a present's own blit.
    LONG tailUs = 0, upUs = 0, postUs = 0, blitUs = 0;
    bool Queue() const { return tailUs > 0 || upUs > 0 || postUs > 0; }
    // ---- 2026-10-13, evening (the ride holds: "THE ANSWER'S OWN TIME" above FQ6)
    LONG kUs = 0;                                // the helper's answer comes this long behind the handoff's retire (0: at the retire, as every case before ran)
    LONG rideKMaxUs = 4500;                      // FgArmedKeys::rideKMaxUs, as the armed build hands it in (0: the ride's floors of the morning of 2026-10-13)
    int planStep = 0;                            // 1: the product's step of the plan runs once a window of the case's (RtWindow), with the product's numbers
    // ---- 2026-10-14 (the helper's answer on a card that is the limit: "THE HELPER'S LISTS ON THE CARD" above RtDisp; dt_fg_late.h has the cases)
    // All 0: off, the answer comes kUs behind the retire and takes no card time, as every case before these ran.
    LONG kWorkUs = 0;                            // a full dispatch's own time on the card (over 0: the model is on)
    LONG kResetUs = 0;                           // ... and a reset dispatch's (the game gave a reason on 'Q' or 'G')
    LONG sceneUs = 0;                            // the next frame's scene work that stands on the game's queue right behind a hand's retire
    int rank = 0;                                // who has the card when both have work: 0 the helper; 1 THE GAME (the leg's reading);
                                                 // 2 the game, until the machine asked for the helper's place (B::CardClass), then the helper
    LONG ordLatUs = 0;                           // an order wait's release reaches the queue this long behind its signal
    int over = 0, card = 0, ord = 0;             // FgArmedKeys' three, as the case hands them in (the armed build: 1, 0, 2; any ord over 0 is
                                                 // the order wait on every sourced ride pair since 2026-10-17, when its span of late answers went)
    // ---- 2026-10-14, afternoon (every generated frame shown on a busy card: dt_fg_rideframes.h has the cases). FgArmedKeys' four, as the
    // case hands them in (the armed build: 1, 1, 1, 1); 0 in every case before that file's, which run what they ran
    int rideEarly = 0, rideNoWait = 0, armTeach = 0, ordAtStamp = 0;   // (ordRide went on 2026-10-17 with the arming it chose)
    int pubWake = 0;                             // 1: the placer is stepped at every Present, as the product's publication wakes it (RtAfter)
    LONG planMaxUs = 8000;                       // the plan's upper bound (the armed build's since 2026-10-14's afternoon: 4,500)
    // ---- 2026-10-14, evening (the seven legs replayed: dt_fg_replay.h). All 0: off, every case before runs what it ran.
    int rideWaitAns = 0;                         // FgArmedKeys::rideWaitAns, as the armed build of that evening hands it in
    LONG latencyPresents = 0;                    // (2026-10-18, S1 round 2, the reviewer's finding 4) THE PRESENTER'S HOLD BY PRESENTS, DXVK's own count (its
                                                 // SyncFrameLatency waits for frame id less the latency to be presented, d3d9_swapchain.cpp as
                                                 // docs/LATENCY-RESEARCH-2026-09-23.md reads it; d3d9.maxFrameLatency 1 on the owner's legs): a frame does not begin
                                                 // until the present this many before the last one issued has been shown. A GENERATED FRAME'S PRESENT COUNTS
                                                 // AS A REAL ONE'S. 0: off (the time-based hold below, as every case before)
    LONG latencyUs = 0;                          // THE PRESENTER'S HOLD: a frame does not begin while the card's queue stands more than this behind the clock
                                                 // (DXVK holds the render thread in Present; the game then runs at the card's own cycle)
    int leadScene = 0;                           // 1: THE SCENE BEHIND THE RETIRE IS WHAT THE CPU HAD QUEUED BY THEN: sceneUs times the CPU's lead on the retire
                                                 // over the frame's cost to its injection (0: all of sceneUs, as every case before)
    int serve = 0;                               // 1: A WORKER THAT WAITS FOR THE HELPER'S ANSWER IS SERVED: its wait runs the simulation's events up to
                                                 // that answer's instant on its own thread (RtB::TlWait), where the real wait found a clock that stood
    int holdPlaceAt = -1;                        // over -1: THE PLACER DOES NOT STEP during this frame of the run (a plant: the stamper and the placer
                                                 // both late behind one pair's answer, the evening of 2026-10-14's event)
    // ---- 2026-10-17 (stage 2: dt_fg_late.h's LA5 and LA7, dt_fg_replay.h). 0: off, every case before runs what it ran.
    int roomSeed = 0;                            // FgArmedKeys::roomSeed, as the armed build of that day hands it in (1)
    // ---- 2026-10-17, stage 3's Part A (dt_fg_replay.h, RL1 to RL9). 0: off, every case before runs what it ran.
    int prepAtRet = 0;                           // 1: THE HELPER'S PREPARE OF PAIR n IS DONE NO EARLIER THAN ITS HANDOFF RETIRES. On the card the
                                                 // prepare's lists are displaced by the game's own post chain (the helper's line: 0.15 ms of work read
                                                 // 4 to 5 ms from its first stamp to its last, its first 5.7 ms behind its execute): the fake helper
                                                 // waits on fgIn at n before it signals fgPrep at n, so an injection that reaches the worker ahead of
                                                 // the handoff before it finds that prepare not done (a wait behind fgPrep is to be queued)
    // ---- 2026-10-17, stage 3 (dt_fg_replay.h's RL cases run it beside the leg's own machine). 0: off, every case before runs what it ran.
    int prepWait = 0;                            // FgArmedKeys::prepWait, as the armed build hands it in (1)
    LONG prepWaitMs = 0;                         // FgArmedKeys::prepWaitMs (0: the machine's 8 ms, as the armed build hands it)
    int place = 0;                               // FgArmedKeys::place, and THE WRITES' STAMPS: the simulation hands the machine each positioned pair's place on
                                                 // the card from the instants it SHOWED the three frames (RtLook), eight pairs late as the product's ring is
    int fitOnly = 0;                             // FgArmedKeys::rideFitOnly
    int calls = 0;                               // over 0: THE NEXT FRAME'S SCENE IN THIS MANY CHECKPOINT CALLS (RtFrame), each its share of the scene and the
                                                 // tail on the card's queue as the CPU makes it, so a present fired behind call k stands behind that much of
                                                 // the frame's work; the scene behind a retire and the tail at the injection are not added then
    int starve = 0;                              // 1: A DISPATCH NO ORDER WAIT PUTS FIRST, behind a game's queue that holds the next frame up to its
                                                 // injection, runs where the game next waits for the helper (the next home): RtDisp
    // ---- 2026-10-17, night (phase 5, F1: THE PANEL MODEL, "THE PANEL" above RtPanelShow; dt_fg_fixed.h has the cases). All 0: off, every case
    // before runs what it ran, to the byte.
    double panelHz = 0.0;                        // THE PANEL'S TRUE REFRESH (0: off). The machine is told `refresh` above, which may differ (119.88 against 120)
    int panelVrr = 0;                            // 1: VARIABLE REFRESH: a present is shown when the card has it, never sooner than one refresh behind the one
                                                 // before; 0: THE GRID: on the first refresh at or after that, never two on one refresh
    int fifo = 0;                                // DXVK'S FIFO HOLD: a present returns when the present `fifo` ahead of it is shown (0: no hold, as before)
    double panelPhase = 0.0;                     // where the grid's first refresh stands behind the run's start, in refreshes (0 to 1)
    LONG panelStallAt = -1, panelStallMs = 0;    // A PLANTED STALL OF THE PANEL: the run's present number panelStallAt is shown panelStallMs late (and
                                                 // the grid with it): the FIFO holds the presents behind it that long (FX5)
    LONG panelStallEvery = 0;                    // (round 3) and again every this many presents behind it (0: once): a streaming hitch's rhythm
    int fixOn = 0;                               // the machine's fixed-mode word (FgRenderLine::fixOn), as the conditions publish it (fg_arm_life.h)
    LONG presCpuUs = 0;                          // (round 2) THE PRESENT'S OWN TIME on the worker, which is no wait: a FIFO present returns no sooner than this behind its call
    LONG fixBaseUs = 0;                          // (round 2) the machine's FgRenderLine::fixBaseUs, as the conditions publish it (the Present's own time they measured)
    // ---- 2026-10-18 (phase 5 round 2, S1: THE SWITCH-ON'S RAMP; dt_fg_ramp.h has the cases). All 0: off, every case before runs what it ran.
    int postIdle = 0;                            // 1: A FRAME WITH NO HAND QUEUES ITS POST CHAIN TOO (postUs, ahead of its own Present: RtLook), as the game's frame
                                                 // does with frame generation or without: standing by, the card does the frame's whole work. Off, the fake's queue
                                                 // standing by held no post chain (LW1's note: about 108 a second standing by where its leg's game ran 74.5)
    int ramp = 0;                                // FgArmedKeys::ramp, as the armed build hands it in (1)
    int helperFifo = 0;                          // 1: THE HELPER'S QUEUE RUNS ITS LISTS IN ORDER, a dispatch no earlier than the answer to the one before (RtDisp)
    int rampFirst = 0;                           // FgArmedKeys::rampFirst, as the armed build hands it in (1)
    LONG rampAskUs = 0;                          // FgArmedKeys::rampAskUs, as the law's window step would publish it (fg_law.h, FgRampTargetUs): the case works it out
    LONG rampDrainOk = 0;                        // FgArmedKeys::rampDrainOk, the law's word "the card is the limit" (S1 round 2): 1 lets the switch's drain run
    // ---- 2026-10-18 (round 3, G3: THE GRID FILL; dt_fg_fill.h has the cases). 0: off, every case before runs what it ran.
    int holdInOrig = 0;                          // 1: THE PRESENTER'S HOLD (latencyUs, latencyPresents) INSIDE THE ORIGINAL PRESENT, where the game has it
                                                 // (RtOrigPresent): the pacer's sample, the game's CPU part, leaves it out as the mod's does
    // ---- 2026-10-18 (round 3, SW: the switch's queue read directly). 0: off, every case before runs what it ran.
    int holdWorker = 0;                          // 1: THE PRESENTER'S TIME HOLD (latencyUs) IS THE WORKER'S, as DXVK's is: it is added to the worker's sum of its
                                                 // time inside DXVK's Present (dtPresTicksSum), which the machine reads (DXVK's count, latencyPresents, always is)
    // ---- 2026-10-19 (G4's round 4: FX13's planted noise). 0: off, every case before runs what it ran.
    LONG presNoiseUs = 0;                        // over 0, on a panel with no FIFO: THE PRESENT'S OWN TIME JITTERS, 0 to this many us a present added to the worker's
                                                 // sum (dtPresTicksSum), as a real Present's own time does about the base the conditions measured
    // ---- 2026-10-20 (round 5, F: THE CARD THAT IS THE LIMIT, dt_fg_loop.h's LP2). -1: off, every case before runs what it ran.
    LONG kFlowUs = -1;                           // over -1: A FLOW-ONLY PAIR'S DISPATCH (its 'G' counted 0: the optical flow and the store, no picture)
                                                 // takes this much card time in place of kWorkUs
    // ---- 2026-10-25 (the last optimisation's A: THE CARD'S WAIT ON THE FIRES' PRESENTS, dt_fg_fixed.h's FX24). 0: off, every case before runs
    // what it ran. On the panel model's FIFO: a fire's Present returns no sooner than this long behind its call (the real frame's and the
    // generated frame's), each jittered by up to cardWaitJitUs either way: DXVK's frame-latency wait for the card's own work queued ahead,
    // which the round 7 legs read on a card that is the limit (fixed120 at 1680 MHz: the real frame's 3 to 4.5 ms, the generated frame's
    // 5 us; weak60 at 300 MHz: 14 ms and 8.5 ms). It holds the worker (its timeline) and moves no present on the panel.
    LONG cardWaitRealUs = 0, cardWaitGenUs = 0, cardWaitJitUs = 0;
    LONG cardWaitUntil = 0;                      // (the review's FX26) over 0: the card's wait ends behind this many of the run's presents (from RtSim::ord0)
    bool Lists() const { return kWorkUs > 0; }
};
struct RtSim
{
    RtPar par;
    std::vector<RtPair> pairs;
    std::map<DWORD, int> byTag;
    struct Ev { LONG n; LONGLONG t; LONG k; };
    std::deque<Ev> sigs;                         // handoffs not yet retired: the value of fgIn and when (k: its answer's time behind that)
    std::deque<Ev> ends;                         // (the queue) frame ends not yet retired: the frame and when its real frame is shown
    std::deque<Ev> answers;                      // (kUs) answers the helper holds on the harness's clock: the pair and when it comes
    LONG kNext = -1;                             // (kUs) a case's own answer time for the next hand's pair (-1: par.kUs)
    LONG lsNext = -1;                            // (2026-10-28, LW54) a case's own handoff for the next hand's pair, entry to retire, us (-1: par.lsUs;
                                                 // without the card's queue alone): a late handoff of the card's (a GPU hitch, DXVK compiling)
    // (planStep) the product's step of the plan (fg_arm_life.h, FgpPlanStep), on K's histogram as the machine fills it
    LONG planWin[FG_PLAN_WINDOWS][FG_K_BINS] = {}, planSeen[FG_K_BINS] = {}; LONG planAt = 0; FgPlanMem planMem = {}; FgPlanK planK = {};
    LONGLONG qEnd = 0;                           // (the queue) when everything queued so far has retired
    size_t bodyAt = 0; bool bodyPend = false;    // (the queue) the screen's log as the last injection's body found it; its wait not yet applied
    LONGLONG placerNext = 0, nextEv = 0;
    LONG pendSeen = 0;
    size_t shownAt = 0;
    int order = 0, other = 0, lastReal = -1;
    bool owed = false, busy = false, on = false;
    std::string firstBad;
    // (2026-10-14, the helper's lists on the card) when the last dispatched pair's answer comes; the dispatches by how they
    // met the game's work; and each order wait's release as this simulation worked it out, held against the machine's own
    LONGLONG ansAt = 0;
    LONG dispFirst = 0, dispBehind = 0, dispOrdered = 0, ordLate = 0, ordChecked = 0, dispStarved = 0;   // (dispStarved: 2026-10-17, RtPar::starve)
    std::vector<LONG> depth;                     // (2026-10-17) THE QUEUE'S DEPTH: how far the card's queue stood ahead of each frame's begin, in microseconds
    std::vector<LONGLONG> shownAll;              // (2026-10-18, RtPar::latencyPresents) every present's shown instant, in the order they reached the card
    LONG holdsP = 0; LONGLONG holdPTicks = 0;    // ... and that hold's count and its time
    struct PlaceS { LONG idx, p, place, span; };
    std::deque<PlaceS> placeQ;                   // (2026-10-17, RtPar::place) the writes' places not yet handed to the machine
    struct Ord { LONG n; LONGLONG rel; };
    std::deque<Ord> ords;
    // (2026-10-14, evening: the replays) the frame of the run (RtFrame counts them); the presenter's holds and their sum; the worker's
    // waits served and the events they ran; the placer's steps the plant skipped
    int frameN = 0; LONG holds = 0; LONGLONG holdTicks = 0; LONG served = 0, servedEv = 0, placeSkipped = 0;
    // (2026-10-17, night, F1: THE PANEL MODEL) the grid's first refresh and its period in ticks; the last instant shown; every present the panel
    // showed in this run, in order (its instant, whether generated, its pair or -1); the instants by the Present's ordinal, for the FIFO hold;
    // the presents the variable panel held for its refresh; the FIFO's holds, their time and the longest, the holds that met their bound
    LONGLONG vb0 = 0, lastDisp = 0; double vbT = 0.0; LONG ord0 = 0, shownRun = 0;
    struct Disp { LONGLONG t; int gen; int pair; int stall; };   // (stall, round 3: this present was the stalled one: the gap before it is the stall's)
    std::vector<Disp> disps;
    std::deque<std::pair<LONG, LONGLONG>> dispByOrd;
    LONG vrrHeld = 0, fifoHolds = 0, fifoBound = 0, fifoUnknown = 0; LONGLONG fifoTicks = 0, fifoMax = 0, vrrHeldTicks = 0, vrrHeldMax = 0;
    LONGLONG wFree = 0, opDone = 0; std::deque<std::pair<LONG, LONGLONG>> callByOrd;   // THE WORKER'S TIMELINE: free from; the replay of the last Present done at
    LONG renderHeld = 0; LONGLONG renderHeldTicks = 0;
    LONG cardWaits = 0; LONGLONG cardWaitTicks = 0; unsigned cardSeed = 20261025u;   // (RtPar::cardWaitRealUs: the presents the card's wait held, by how much)
    LONG fifoN = 0, fifoN0 = 0; LONGLONG fifoWaitTicks = 0, fifoWait0 = 0;   // (fifoN0, fifoWait0: a case's mark, where its stretch begins)                                         // (round 2) the FIFO's presents, and their time waiting for the one ahead alone                                 // the render thread held at its Present for the worker's replay of the one before
};
static RtSim gRt;
static double gRtPanelHz = 0.0; static int gRtPanelVrr = 0, gRtPanelFifo = 0;   // (2026-10-17, night, F1: the panel model forced on for a run whose RtPar a case cannot reach: FX0)
static LONG gRtInBusy0 = 0;                      // (round 5, H) the fake's count of copies into the set its dispatch read, at the run's start
static LONG gRtTimed0[3] = {};                   // (round 5, H, 2026-10-24) the placer's pairs timed, by their own reading of fgIn, from the arm, at the run's start
static void RtTick();
static int  RtPaceHook(LONGLONG timerDue);
static bool RtRun(LONGLONG until, int how);

// THE BINDING: the merged binding of every other case, with three seams of the simulation's own. NOTHING ELSE DIFFERS.
struct RtB : ShArmB
{
    static inline volatile LONG defer = 0;
    static inline LONG pendIn = 0;
    // (the queue: a real frame's end is signalled by the simulation, when what stood ahead of its present has retired)
    static inline std::vector<LONG> pendEnd;
    static void LatEnd(LONG f) { if (defer && gRt.par.Queue()) { pendEnd.push_back(f); return; } ShArmB::LatEnd(f); }
    static void PrevLatEnd(LONG f) { LatEnd(f); }             // (a present the machine passed through: PrevPresent's hook)
    static LONGLONG Now() { if (gRt.on && GetCurrentThreadId() == renderTid) RtTick(); return ShArmB::Now(); }
    static void RecSignal(int tl, LONG v) { if (defer && tl == FG_TL_IN) { pendIn = v; return; } ShArmB::RecSignal(tl, v); }
    static int PaceWait(LONGLONG dueQpc) { return gRt.on ? RtPaceHook(dueQpc) : ShArmB::PaceWait(dueQpc); }
    // (2026-10-14, evening: THE WORKER'S WAIT FOR THE ANSWER IS SERVED, RtPar::serve. The machine's worker waits for fgOut on this
    // clock, which only the simulation moves, and the render thread stands in its drain behind the worker: the real wait found a
    // clock that stood and ended at its own bound with no answer. Served, the wait runs the events up to the answer's instant on
    // the worker's own thread, RtServe, and returns as the real one would have: with the answer, or at its bound)
    static bool TlWait(int tl, ULONGLONG v, DWORD ms);
    // (2026-10-17, stage 3: THE WRITES' STAMPS, RtPar::place. The product reads a write's two GPU stamps where its ring slot comes round again,
    // inside a span's RecBegin on the worker; here the simulation's shown instants stand for them and are handed over at the same call)
    static bool PlaceStamps();
    static void RecBegin();
    // (2026-10-17, night, F1: THE PANEL MODEL's FIFO HOLD. The machine calls DXVK's Present through the pointer its install takes here: the
    // simulation hands back its own stand-in, which calls the fake's Present and then holds as DXVK's FIFO does where RtPar::fifo asks for it;
    // with the model off it is the fake's Present and nothing else)
    static void* DxvkBare(void* p);
    // (round 2: THE RENDER THREAD'S ONE-FRAME-AHEAD WAIT, INSIDE THE ORIGINAL PRESENT. The machine's saved Present is the device thread's
    // dtScPresent; the simulation stands in front of it, so the wait for the worker's replay of the Present before is inside the time the
    // machine measures there (tInOrig), as DT_R_PACE's is in the mod. Off with the FIFO, as every case before)
    static void*& OrigSwapPresentSlot();
};
typedef FgMachine<RtB> MR;

// The screen's log, read on from where it was read last: each new present with the clock as it stands (the fire that
// made it ran inside the drain just before this call, and the clock has not been moved since).
// THE CARD'S ONE QUEUE (2026-10-13; fg_machine.h, "THE REAL FRAME RIDES BEHIND THE NEXT FRAME'S UPSCALE"). DXVK has one queue
// and the mod's submits take it behind all DXVK has handed over, so A PRESENT IS ON THE SCREEN WHEN WHAT WAS QUEUED AHEAD
// OF IT HAS RETIRED, wherever on the worker's clock it was called. The simulation keeps one instant, `qEnd`: when
// everything queued so far has retired. In the order a frame queues:
//   its injection      the scene's tail: qEnd = max(qEnd, the injection) + tailUs
//   the body           the home copy WAITS for the helper's upscale: qEnd += upUs (RtBody). The body's stand-in notes how
//                      long the screen's log was when it ran (RtInjBody, on the worker), so a present made in that same
//                      drain is known to stand AHEAD of the body or BEHIND it
//   the hand           the post chain and the hand's copies: the handoff retires at qEnd + postUs, never before the entry,
//                      and that instant is fgIn's signal (in place of entry + lsUs)
//   a present          shown at max(its call, qEnd) + blitUs, and qEnd is that instant
// A REAL FRAME'S END is signalled when it is shown (RtB::LatEnd hands the frame over; the placer reads that stamp for a ride).
// WHAT IT DOES NOT MODEL: the helper's own lists (the answer comes at the retire, as before), the scene's work ahead of
// the injection (the card keeps up with it), a present made inline behind its own hand (shown as if ahead of it: the
// run-in's and the closing frames' alone), and two processes sharing the card.
static LONGLONG RtShownAt()
{
    RtSim& g = gRt;
    if (!g.par.Queue()) return B::clockNow;
    const LONGLONG t = (g.qEnd > B::clockNow ? g.qEnd : B::clockNow) + MR::UsToTicks(g.par.blitUs);
    g.qEnd = t;
    return t;
}
// THE PANEL (2026-10-17, night; phase 5, F1, the fixed-refresh mode; docs/reports/fg-5/brief-fill/PLAN-fill.md C, F1's package). Until now a
// present was on the screen the instant the card had it (RtShownAt), the right model for a variable panel inside its range and the one every
// case before this was calibrated on. THE MODEL, off by default (RtPar::panelHz 0), puts a panel behind the card, with its TRUE refresh
// (which the machine is not told: it reads RtPar::refresh, as the mod reads Windows):
//   VARIABLE (panelVrr 1): a present is shown when the card has it, but never sooner than one refresh behind the present before it (the
//     panel's fastest). Nothing the machine reads changes (the card's instants tGen, tReal, the frame ends and the places stay the queue's), so
//     the replays read what they read: FX0 holds RL9 to that;
//   THE GRID (panelVrr 0): a fixed panel under vsync: a present is shown on the first refresh at or after the card has it, and never on the
//     refresh of the one before (a present that comes while the one before waits for its refresh waits a refresh more);
//   DXVK'S FIFO HOLD (fifo 1 or more): DXVK's Present returns when the present `fifo` ahead of it is SHOWN (the reviewer's reading of DXVK's
//     FIFO: "a present wait returns when the frame BEFORE is shown"; the leg's PresentMon settles how deep the real one is). THE WORKER'S OWN
//     TIMELINE: the simulation's clock is the game's, and in the game the render thread goes on while the worker waits inside DXVK, so the hold
//     does not move the clock. The worker is "free" from the instant its last held present returned (RtSim::wFree): a present it makes is
//     CALLED no earlier than that, its blit is on the card from its call, and it returns when the one ahead of it is shown. The time between
//     its call and its return is added to the worker's own sum of its time inside DXVK's Present (dtPresTicksSum), as the machine's timed call
//     adds a real one: the machine reads the hold where the product reads it (PaceFixed). THE RENDER THREAD IS HELD ONLY WHERE THE PRODUCT'S
//     IS: its Present waits for the worker's replay of the Present before it (the device thread's one-frame-ahead wait, DT_R_PACE), which on
//     the worker's timeline comes behind everything the worker did before it (RtFrame: RtSim::opDone). TO KNOW WHEN THE ONE AHEAD IS SHOWN the
//     hold flushes DXVK's stream and reads the screen's log at the present's own call, so a present's blit is on the card at its call, not at
//     the next flush: the kinder of the fake's two readings (RING_CS_ALL's), told.
// WHAT IT DOES NOT MODEL: a panel that drifts, the scan's own time (a present is shown at its refresh's start), the compositor; and of the
// worker's lag only its presents: a worker behind the clock by a hold makes its other calls (the game's replayed ones, the fires' copies) at
// the clock, not behind the hold (it moves the screen nothing: the presents carry the lag).
static LONGLONG RtPanelShow(LONGLONG ready, LONG ord, int gen, int pair)
{
    RtSim& g = gRt; const RtPar& p = g.par;
    if (p.panelHz <= 0.0) return ready;
    for (auto it = g.callByOrd.rbegin(); it != g.callByOrd.rend(); ++it) if (it->first == ord) { if (it->second > ready) ready = it->second; break; }   // (made on the worker's timeline)
    const double T = g.vbT;
    LONGLONG d;
    if (p.panelVrr)
    {
        d = ready;
        if (g.lastDisp && (double)(d - g.lastDisp) < T) { d = g.lastDisp + (LONGLONG)(T + 0.5); g.vrrHeld++; g.vrrHeldTicks += d - ready; if (d - ready > g.vrrHeldMax) g.vrrHeldMax = d - ready; }
    }
    else
    {
        double x = (double)ready;
        if (g.lastDisp && x < (double)g.lastDisp + T * 0.5) x = (double)g.lastDisp + T * 0.5;
        long long k = (long long)ceil((x - (double)g.vb0) / T - 1e-9);
        if (k < 0) k = 0;
        d = g.vb0 + (LONGLONG)((double)k * T + 0.5);
    }
    const bool stall = p.panelStallAt >= 0 && p.panelStallMs > 0 && (g.shownRun == p.panelStallAt
                       || (p.panelStallEvery > 0 && g.shownRun > p.panelStallAt && (g.shownRun - p.panelStallAt) % p.panelStallEvery == 0));
    if (stall)
    {   // (the planted stall: this present and the grid behind it, late by the stall)
        d += MR::UsToTicks(p.panelStallMs * 1000);
        if (!p.panelVrr) g.vb0 = d;
    }
    g.shownRun++;
    g.lastDisp = d;
    g.disps.push_back({ d, gen, pair, stall ? 1 : 0 });
    g.dispByOrd.push_back({ ord, d });
    while (g.dispByOrd.size() > 64) g.dispByOrd.pop_front();
    return d;
}
static bool RtDispOf(LONG ord, LONGLONG* t)
{
    for (auto it = gRt.dispByOrd.rbegin(); it != gRt.dispByOrd.rend(); ++it) if (it->first == ord) { *t = it->second; return true; }
    return false;
}
static void RtLook();
static void RtNextEv();
// THE FIFO HOLD (above): on the worker's thread, inside the machine's timed call of DXVK's Present, the fake's Present just made at the clock
// as it stands: its call on the worker's timeline, the one ahead's showing, the hold added to the worker's sum.
static void RtFifoHold()
{
    RtSim& g = gRt;
    EnterCriticalSection(&gRing.data); const LONG o = gRing.ord; LeaveCriticalSection(&gRing.data);
    const LONGLONG call = B::clockNow > g.wFree ? B::clockNow : g.wFree;
    g.callByOrd.push_back({ o, call });
    while (g.callByOrd.size() > 64) g.callByOrd.pop_front();
    B::Flush(); RtLook();                                        // (this present's blit on the card from its call; the one ahead's showing known)
    LONGLONG ret = call + MR::UsToTicks(g.par.presCpuUs);       // (round 2: the Present's own time, which no queue makes)
    const LONG want = o - g.par.fifo;
    if (want > g.ord0)                                           // (the one ahead of the run's first presents is the run-in's, on no panel)
    {
        LONGLONG d = 0;
        if (RtDispOf(want, &d)) { if (d > ret) ret = d; }
        else g.fifoUnknown++;                                    // (its blit stands behind a queued wait, not on the card: nothing to wait for, counted)
    }
    if ((g.par.cardWaitRealUs > 0 || g.par.cardWaitGenUs > 0) && (g.par.cardWaitUntil <= 0 || o - g.ord0 <= g.par.cardWaitUntil) && ReadAcquire(&B::S().sh.fireBusy))
    {   // (2026-10-25, RtPar::cardWaitRealUs: THE CARD'S WAIT, on the fires' presents by their kind, the fire's own word for it (FgWorkerLine::fireKind))
        LONG us = B::S().w.fireKind == 0 ? g.par.cardWaitGenUs : g.par.cardWaitRealUs;
        if (us > 0 && g.par.cardWaitJitUs > 0) { g.cardSeed = g.cardSeed * 1664525u + 1013904223u; us += (LONG)((g.cardSeed >> 8) % (unsigned)(2 * g.par.cardWaitJitUs + 1)) - g.par.cardWaitJitUs; }
        const LONGLONG c = call + MR::UsToTicks(g.par.presCpuUs) + MR::UsToTicks(us > 0 ? us : 0);
        if (us > 0 && c > ret) { g.cardWaits++; g.cardWaitTicks += c - ret; ret = c; }
    }
    g.wFree = ret;
    const LONGLONG held = ret - call;
    if (held > 0) { InterlockedExchangeAdd64(&dtPresTicksSum, held); g.fifoHolds++; g.fifoTicks += held; if (held > g.fifoMax) g.fifoMax = held; }
    { const LONGLONG w = ret - call - MR::UsToTicks(g.par.presCpuUs); if (w > 0) g.fifoWaitTicks += w; g.fifoN++; }   // (round 2: the queue's own wait, the Present's time aside)
}
typedef HRESULT(__stdcall* RtDxvk_t)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);
static void* gRtDxvk = nullptr;
static HRESULT __stdcall RtDxvkPresent(IDirect3DSwapChain9* sc, const RECT* s, const RECT* t, HWND w, const RGNDATA* r, DWORD f)
{
    const HRESULT hr = ((RtDxvk_t)gRtDxvk)(sc, s, t, w, r, f);
    if (gRt.on && gRt.par.panelHz > 0.0 && gRt.par.fifo > 0 && SUCCEEDED(hr) && GetCurrentThreadId() == dtWorkerTid) RtFifoHold();
    else if (gRt.on && gRt.par.presNoiseUs > 0 && SUCCEEDED(hr) && GetCurrentThreadId() == dtWorkerTid)
    {   // (G4's round 4: the Present's own time jittering, FX13's plant; its own seed, so the case's frames draw as they did)
        static unsigned s = 20261019u; s = s * 1664525u + 1013904223u;
        InterlockedExchangeAdd64(&dtPresTicksSum, MR::UsToTicks((LONG)((s >> 8) % (unsigned)(gRt.par.presNoiseUs + 1))));
    }
    return hr;
}
void* RtB::DxvkBare(void* p) { gRtDxvk = ShArmB::DxvkBare(p); return gRtDxvk ? (void*)&RtDxvkPresent : nullptr; }
static void RtPresenterHold();
static HRESULT __stdcall RtOrigPresent(IDirect3DSwapChain9* sc, const RECT* s, const RECT* t, HWND w, const RGNDATA* r, DWORD f)
{
    RtSim& g = gRt;
    // (2026-10-18, round 3, G3, RtPar::holdInOrig: THE PRESENTER'S HOLD WHERE THE GAME HAS IT, inside the original Present: the render thread waits
    // there for the worker's replay of the Present before, which waits in DXVK for the queue. The pacer's sample leaves that time out, as the mod's)
    if (g.on && g.par.holdInOrig) RtPresenterHold();
    if (g.on && g.par.panelHz > 0.0 && g.par.fifo > 0 && g.opDone > B::clockNow)
    {
        const LONGLONG until = g.opDone; g.renderHeld++; g.renderHeldTicks += until - B::clockNow;
        RtRun(until, 2);
        if (until > B::clockNow) B::clockNow = until;
    }
    return dtScPresent(sc, s, t, w, r, f);
}
void*& RtB::OrigSwapPresentSlot() { void*& slot = ShArmB::OrigSwapPresentSlot(); if (slot == (void*)&dtScPresent) slot = (void*)&RtOrigPresent; return slot; }
static void RtBody()
{
    RtSim& g = gRt;
    if (g.qEnd < B::clockNow) g.qEnd = B::clockNow;
    if (g.par.Lists() && g.ansAt > g.qEnd) g.qEnd = g.ansAt;  // (2026-10-14: the helper's one queue: this frame's upscale stands behind the dispatch before it)
    g.qEnd += MR::UsToTicks(g.par.upUs);
    g.bodyPend = false;
}
static void RtInjBody(const DWORD* payload, DWORD bytes)
{
    RtSim& g = gRt;
    if (g.on && g.par.Queue()) { B::Flush(); EnterCriticalSection(&gRing.data); g.bodyAt = gRing.presented.size(); LeaveCriticalSection(&gRing.data); g.bodyPend = true; }
    const LONG sent0 = B::sentN;
    ShInjBody(payload, bytes);
    if (g.on && g.par.Lists() && B::sentN != sent0) ShArmB::LaHome();   // (2026-10-14: the body's home, behind its wait: dt_fg_shadow.h, ShArmB::LaHome)
}
// THE HELPER'S LISTS ON THE CARD (2026-10-14; fg_machine.h, "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT"; RtPar::kWorkUs over 0).
// Until now the answer came a planted time behind the retire and took no card time. With the model on, at each hand:
//   THE DISPATCH IS WORK ON THE CARD, kWorkUs (kResetUs where the game gave a reason: a reset interpolates nothing), for a
//     pair whose 'G' said DISPATCH, read off the pipe;
//   THE NEXT FRAME'S SCENE (sceneUs) STANDS ON THE GAME'S QUEUE RIGHT BEHIND THE RETIRE, as it does on a card that is the limit;
//   WHO GOES FIRST is RtPar::rank: the helper (0, and 2 once the machine asked for its place); or THE GAME (1, the legs'
//     reading: the game's queue outranks the helper's), and then the dispatch runs behind the scene and K is both;
//   AN ORDER WAIT the machine queued in this hand holds the game's queue: the dispatch goes first whoever outranks whom,
//     and the scene starts when the wait is released. THE RELEASE IS WORKED OUT HERE, from the rule the machine is to
//     keep (the answer, or ordMaxUs behind the retire, whichever is first) and kept for RtOrdCheck, which holds the
//     machine's own signal against it;
//   A PLANTED ANSWER TIME (kNext) is a hitch of the helper's own: the answer comes then, whatever the card did.
// The card's time is the same in every order; only the answer's instant moves. Returns K for the pair's retire event.
static LONG RtDisp(LONG n, LONGLONG tS)
{
    RtSim& g = gRt; const RtPar& p = g.par;
    DWORD gf = ~0u, qf = 0;
    {
        const std::vector<RingPipe> log = RingPipeLog();
        for (size_t i = log.size(); i-- > 0; )
        {
            if (log[i].p != n) continue;
            if (log[i].type == 'G' && gf == ~0u) gf = log[i].flags;
            else if (log[i].type == 'Q') { qf = log[i].flags; break; }
        }
    }
    LONGLONG scene = MR::UsToTicks(p.sceneUs);
    // (2026-10-14, evening, RtPar::leadScene: WHAT STANDS BEHIND THE RETIRE IS WHAT THE CPU HAD QUEUED BY THEN. The next frame begins at the
    // pacer's due (or now, where it does not sleep); the scene reaches the queue over the frame's cost to its injection. A retire
    // ahead of that frame's begin finds the queue dry and the dispatch runs first, as the legs' slack windows had it)
    if (p.leadScene && p.sceneUs > 0)
    {
        const FgRenderLine& r = B::S().r;
        LONGLONG begin = r.due > B::clockNow ? r.due : B::clockNow;
        const LONGLONG toInj = g.pairs.empty() ? MR::UsToTicks(4000) : (LONGLONG)((double)g.pairs.back().cost * p.inj);
        const LONGLONG lead = tS - begin;
        if (lead <= 0) scene = 0;
        else if (toInj > 0 && lead < toInj) scene = scene * lead / toInj;
    }
    const LONGLONG sq = p.calls ? 0 : scene;                 // (2026-10-17, RtPar::calls: the scene reaches the queue in the next frame's calls instead)
    if (gf == ~0u || (gf & 3u) != (DWORD)FG_G_DISPATCH) { g.qEnd = tS + sq; return 0; }
    const bool reset = ((gf >> FG_G_RESET_SHIFT) & 15u) != 0 || (qf & 15u) != 0;
    LONG work = (reset && p.kResetUs > 0) ? p.kResetUs : p.kWorkUs;
    // (2026-10-20, round 5, F, RtPar::kFlowUs: a flow-only pair's dispatch at its own card time)
    if (p.kFlowUs >= 0 && !reset && (gf & FG_G_COUNTED) && ((gf & FG_G_COUNT_MASK) >> FG_G_COUNT_SHIFT) == 0u) work = p.kFlowUs;
    const FgArmedState& a = ShArmB::a;
    const bool ordered = a.w.ordTop == n && ShArmB::ordFence < n && !ShArmB::ordOff;   // (the machine queued one for this very pair, and it stands)
    const bool first = p.rank == 0 || (p.rank == 2 && ShArmB::cardNow != 0) || (p.leadScene && scene == 0);   // (a dry queue: the dispatch runs first)
    const LONG sceneUs = (LONG)(scene * 1000000 / B::Qpf());
    LONG k = g.kNext > 0 ? g.kNext : 0;
    // (2026-10-18, RtPar::helperFifo: THE HELPER'S OWN QUEUE RUNS ITS LISTS IN ORDER: this dispatch starts no earlier than the answer to the one
    // before it. 0: each dispatch as if the helper's queue were empty, as every case before ran)
    const LONG lag = (p.helperFifo && g.ansAt > tS) ? (LONG)((g.ansAt - tS) * 1000000 / B::Qpf()) : 0;
    int disp = 0;
    if (ordered)
    {
        if (k <= 0) k = lag + work;
        const LONG rel = k < a.k.ordMaxUs ? k : a.k.ordMaxUs;
        g.ords.push_back({ n, tS + MR::UsToTicks(rel) });
        g.qEnd = tS + MR::UsToTicks(rel + p.ordLatUs) + sq + (k > rel ? MR::UsToTicks(work) : 0);
        g.dispOrdered++; disp = 1;
    }
    else if (first) { if (k <= 0) k = lag + work; g.qEnd = tS + MR::UsToTicks(lag + work) + sq; g.dispFirst++; disp = 2; }
    else if (p.starve && scene > 0)
    {   // (2026-10-17, RtPar::starve: THE GAME'S QUEUE IS NOT DRY AT THE RETIRE, and the CPU hands it the next frame faster than the card runs it, so
        // it stays not dry up to that frame's injection: the helper's dispatch runs where the game next waits for the helper, the next frame's
        // home, behind its whole scene and its tail (the leg of 2026-10-13's evening: the answer's 95th 9.5 ms; fg_machine.h, "THE HELPER'S
        // ANSWER ON A CARD THAT IS THE LIMIT": "otherwise when the game next waits for the helper, one scene later, at frame n+1's home").
        // The card's work is the same; the scene stands here as before, the tail at the injection, and the home behind the answer (RtBody))
        if (k <= 0) { k = p.sceneUs + p.tailUs; if (lag > k) k = lag; k += work; }
        g.qEnd = tS + sq; g.dispStarved++; disp = 3;
    }
    else { if (k <= 0) k = (sceneUs > lag ? sceneUs : lag) + work; g.qEnd = tS + sq + MR::UsToTicks(work); g.dispBehind++; disp = 4; }
    g.ansAt = tS + MR::UsToTicks(k);
    if (!g.pairs.empty()) { g.pairs.back().kUs = k; g.pairs.back().disp = disp; g.pairs.back().workUs = work; }
    return k;
}
// EVERY ORDER WAIT THAT IS OVER, AGAINST WHERE THE SIMULATION PUT ITS RELEASE: the machine's own signal (the binding stamps the
// first one of each pair with the clock) within 60 us of it. One that came earlier or later is counted, and the first is said.
static std::string gRtOrdBad;
static void RtOrdCheck()
{
    RtSim& g = gRt;
    while (!g.ords.empty() && ShArmB::ordFence >= g.ords.front().n)
    {
        const RtSim::Ord o = g.ords.front(); g.ords.pop_front(); g.ordChecked++;
        const LONGLONG at = ShArmB::ordSigAt[o.n & 63], off = at - o.rel, tol = MR::UsToTicks(60);
        if (off > tol || off < -tol)
        {
            g.ordLate++;
            if (gRtOrdBad.empty()) gRtOrdBad = Fmt("the order wait of pair %ld was released %lld us from where the simulation put it (thread %lu; the worker is %lu)", o.n, (long long)(off * 1000000 / B::Qpf()), (unsigned long)ShArmB::ordSigTid[o.n & 63], (unsigned long)dtWorkerTid);
        }
    }
}
// (2026-10-17: the screen's log from `from` on, flushed as Shown() is, without a copy of all of it: RtLook runs at every replayed call of
// RtPar::calls, 64 a frame, and the whole log's copy each time made a replay of 1,080 frames take minutes)
static size_t RtShownFrom(size_t from, std::vector<RingPresented>& out)
{
    B::Flush();
    EnterCriticalSection(&gRing.data);
    const size_t n = gRing.presented.size();
    out.clear();
    if (from < n) out.assign(gRing.presented.begin() + (ptrdiff_t)from, gRing.presented.end());
    LeaveCriticalSection(&gRing.data);
    return n;
}
static void RtLook()
{
    RtSim& g = gRt;
    static std::vector<RingPresented> tail;
    const size_t base = g.shownAt;
    const size_t total = RtShownFrom(base, tail);
    for (; g.shownAt < total; g.shownAt++)
    {
        if (g.bodyPend && g.shownAt >= g.bodyAt) RtBody();    // (the queue: this present was made behind the body)
        const DWORD tag = tail[g.shownAt - base].tag;
        const bool isGen = ShIsGen(tag);
        const bool isGen2 = ShIsGen2(tag);                    // (2026-10-18, round 3, G3: a pair's second generated frame; none in any case before dt_fg_fill.h's)
        // (2026-10-18, RtPar::postIdle: THE FRAME'S OWN PRESENT WITH NO HAND, made in the frame's own drain (no hand pending, none recorded for its
        // pair): its post chain stands on the card ahead of it, as a hand's does ahead of its handoff's retire)
        if (g.par.postIdle && !isGen && !isGen2 && g.par.Queue())
        {
            const auto jt = g.byTag.find(tag);
            // (and the inline present of a hand not yet taken, a shadow pair's: its post chain stands ahead of it too, and RtAfter adds it once)
            if (jt != g.byTag.end() && jt->second == (int)g.pairs.size() - 1 && !g.pairs[jt->second].n && !g.pairs[jt->second].real)
            { if (g.qEnd < B::clockNow) g.qEnd = B::clockNow; g.qEnd += MR::UsToTicks(g.par.postUs); }
        }
        const LONGLONG shown = RtShownAt();
        if (g.par.latencyPresents > 0) g.shownAll.push_back(shown);
        if (!isGen && !isGen2 && !RtB::pendEnd.empty()) { g.ends.push_back({ RtB::pendEnd.front(), shown }); RtB::pendEnd.erase(RtB::pendEnd.begin()); }
        const auto it = g.byTag.find(isGen ? ShGenPicture(tag) : isGen2 ? ShGen2Picture(tag) : tag);
        const LONGLONG disp = RtPanelShow(shown, tail[g.shownAt - base].ord, isGen ? 1 : isGen2 ? 2 : 0, it == g.byTag.end() ? -1 : it->second);   // (F1: the panel model's; `shown` with it off)
        if (it == g.byTag.end()) { g.other++; continue; }     // (a frame of the run-in, or the closing ones)
        RtPair& q = g.pairs[it->second];
        if (isGen2)
        {   // (round 3, G3: behind its own pair's first generated frame where that was shown, ahead of its real frame, once)
            q.gen2++; q.tGen2 = shown; q.dGen2 = disp;
            if (it->second != g.lastReal + 1 || q.gen2 != 1 || q.real) { g.order++; if (g.firstBad.empty()) g.firstBad = Fmt("the second generated frame of pair %d came behind the real frame of pair %d (its %d. showing)", it->second, g.lastReal, q.gen2); }
        }
        else if (isGen)
        {
            if (ShIsGenReset(tag)) q.genReset++;
            q.gen++; q.tGen = shown; q.dGen = disp;
            if (it->second != g.lastReal + 1 || q.gen != 1 || q.gen2) { g.order++; if (g.firstBad.empty()) g.firstBad = Fmt("the generated frame of pair %d came behind the real frame of pair %d (its %d. showing)", it->second, g.lastReal, q.gen); }
        }
        else
        {
            q.real++; q.tReal = shown; q.dReal = disp;
            if (g.par.place && it->second >= 1 && q.real == 1)
            {   // (2026-10-17, RtPar::place: the pair's place on the card, from the instants this simulation showed its three frames)
                const RtPair& y = g.pairs[it->second - 1];
                if (q.gen == 1 && y.real == 1 && q.tReal > y.tReal)
                {
                    const LONG place = (LONG)(1000.0 * (double)(q.tGen - y.tReal) / (double)(q.tReal - y.tReal) + 0.5);
                    const LONG span = (LONG)((q.tReal - y.tReal) * 1000000 / B::Qpf());
                    g.placeQ.push_back({ (LONG)it->second, q.p, place, span });
                }
            }
            if (it->second != g.lastReal + 1) { g.order++; if (g.firstBad.empty()) g.firstBad = Fmt("the real frame of pair %d came behind the real frame of pair %d", it->second, g.lastReal); }
            if (it->second > g.lastReal) g.lastReal = it->second;
        }
    }
    if (g.bodyPend) RtBody();
}
static void RtNextEv()
{
    RtSim& g = gRt;
    LONGLONG e = g.sigs.empty() ? 0 : g.sigs.front().t;
    if (!g.ends.empty() && (!e || g.ends.front().t < e)) e = g.ends.front().t;
    if (!g.answers.empty() && (!e || g.answers.front().t < e)) e = g.answers.front().t;
    if (g.placerNext && (!e || g.placerNext < e)) e = g.placerNext;
    g.nextEv = e;
}
// Behind a drain: the hand the worker has run left the value its submit would have signalled; its retire is an event.
static void RtAfter()
{
    RtSim& g = gRt;
    Drain();
    const bool q = g.par.Queue();
    if (q) RtLook();                                          // (the queue: what this drain presented stands ahead of the hand's own submit)
    if (RtB::pendIn != g.pendSeen)
    {
        g.pendSeen = RtB::pendIn;
        LONGLONG tS = B::S().r.lastEntry + MR::UsToTicks(g.lsNext >= 0 ? g.lsNext : g.par.lsUs);
        g.lsNext = -1;
        // (2026-10-18, RtPar::postIdle: a hand whose real frame its own drain already showed inline had its post chain queued ahead of that present)
        const bool postDone = g.par.postIdle && !g.pairs.empty() && g.pairs.back().real > 0;
        if (q) { tS = g.qEnd + MR::UsToTicks(postDone ? 0 : g.par.postUs); if (tS < B::S().r.lastEntry) tS = B::S().r.lastEntry; g.qEnd = tS; }
        LONG k = g.kNext >= 0 ? g.kNext : g.par.kUs;
        if (q && g.par.Lists()) k = RtDisp(g.pendSeen, tS);   // (2026-10-14: the helper's lists on the card)
        if (!g.pairs.empty() && !g.pairs.back().n) { g.pairs.back().n = g.pendSeen; g.pairs.back().tRet = tS; }   // (2026-10-17: the pairs' truth)
        g.sigs.push_back({ g.pendSeen, tS, k });
        g.kNext = -1;
        RtNextEv();
    }
    RtLook();
    // (2026-10-14, afternoon, RtPar::pubWake: THE PRODUCT'S PLACER IS WOKEN BY EVERY PUBLICATION (fg_machine.h, PubRecord), so a pair is
    // placed, and its bound asked for, at its own Present. Without this the simulation stepped the placer at the next event only,
    // and a pair whose handoff retired behind its half period's bound was timed by its stamp all the same: the legs' engage at a
    // card that is the limit, every pair timed from the arm, could not be made. Off in every case before dt_fg_rideframes.h's)
    if (g.par.pubWake && !g.busy)
    {
        if (g.par.holdPlaceAt >= 0 && g.frameN == g.par.holdPlaceAt) { g.placeSkipped++; g.placerNext = 0; }   // (2026-10-17: and its timer with it, which stood in the past and was the next event for ever: the plant never ended)
        else g.placerNext = MgPlace<MR>();
        RtNextEv();
    }
}
// THE WORKER'S WAIT SERVED (RtPar::serve; RtB::TlWait). On the worker's thread, inside the machine's own wait for fgOut: the events
// up to the answer's instant are run here, one at a time, until the fence reads what is wanted or nothing is left to come. The
// render thread stands in its drain meanwhile and runs nothing of the simulation (RtRun's `busy` is not held: it is in the drain).
static bool RtServe(ULONGLONG v, DWORD ms)
{
    RtSim& g = gRt;
    const ULONGLONG end = GetTickCount64() + ms;
    // (2026-10-17: AND NO FURTHER THAN THE WAIT'S OWN BOUND ON THE HARNESS'S CLOCK, so a caller's bound in the machine's time is met as in the mod:
    // the clock stops at `ms` behind where the wait began, and the caller looks again)
    const LONGLONG simEnd = B::clockNow + MR::UsToTicks((LONG)ms * 1000);
    g.served++;
    for (;;)
    {
        if (B::TlValue(FG_TL_OUT) >= v) return true;
        RtNextEv();
        if (!g.nextEv || g.busy) break;
        if (g.nextEv > simEnd) { if (simEnd > B::clockNow) B::clockNow = simEnd; return B::TlValue(FG_TL_OUT) >= v; }
        g.servedEv++;
        RtRun(g.nextEv, 2);
    }
    // (nothing of the simulation's brings it: the real wait, bounded, so that a helper that is held answers in its own time)
    return GetTickCount64() < end ? ShArmB::TlWait(FG_TL_OUT, v, (DWORD)(end - GetTickCount64())) : B::TlValue(FG_TL_OUT) >= v;
}
bool RtB::PlaceStamps() { return gRt.on && gRt.par.place != 0; }
void RtB::RecBegin()
{
    ShArmB::RecBegin();
    RtSim& g = gRt;
    if (!g.on || !g.par.place) return;
    const LONG last = (LONG)g.pairs.size() - 1;
    while (!g.placeQ.empty() && g.placeQ.front().idx + 8 <= last)
    {
        const RtSim::PlaceS v = g.placeQ.front(); g.placeQ.pop_front();
        MR::ArmedPlaceTakeP(v.p, v.place, v.span);
    }
}
bool RtB::TlWait(int tl, ULONGLONG v, DWORD ms)
{
    if (gRt.on && gRt.par.serve && tl == FG_TL_OUT && GetCurrentThreadId() == dtWorkerTid) return RtServe(v, ms);
    return ShArmB::TlWait(tl, v, ms);
}
// A replayed call of the game's, the worker's fire point while the game works (point A).
static void RtPoint() { D->ColorFill(gOtherRt, nullptr, 0xFF203040u); Drain(); RtLook(); }
// THE EVENTS UP TO `until`, each at its own instant. how: 0 the game works (a due raised is served by a replayed call); 1
// the pacer sleeps (a due raised ends this call: true, and the caller answers "the placer woke you"); 3 the pacer's last
// wake (the harness records the pacer's fire note itself: RtPaceHook says why); 2 nobody can serve (the pacer's spin,
// or a read of the clock inside the machine): the due is owed to the next frame's first call.
static bool RtRun(LONGLONG until, int how)
{
    RtSim& g = gRt;
    if (g.busy) return false;
    g.busy = true;
    bool woke = false;
    for (;;)
    {
        // (the queue: a frame end's retire is an event too, behind a handoff's of the same instant and ahead of the placer's;
        // and with kUs an answer's coming, behind both and ahead of the placer's)
        const bool end = !g.ends.empty() && (g.sigs.empty() || g.ends.front().t < g.sigs.front().t) && (!g.placerNext || g.ends.front().t <= g.placerNext);
        const bool sig = !end && !g.sigs.empty() && (!g.placerNext || g.sigs.front().t <= g.placerNext);
        const bool ans = !g.answers.empty() && (!g.placerNext || g.answers.front().t <= g.placerNext)
                      && (!(end || sig) || g.answers.front().t < (end ? g.ends.front().t : g.sigs.front().t));
        const LONGLONG e = ans ? g.answers.front().t : end ? g.ends.front().t : sig ? g.sigs.front().t : g.placerNext;
        if (!e || e > until) break;
        if (e > B::clockNow) B::clockNow = e;
        if (ans)
        {
            // (the clock has reached the answer's instant: the helper, which waits on this clock, answers now; and a 'Q' that
            // stood behind this dispatch is prepared before the clock moves again)
            const LONG n = g.answers.front().n; g.answers.pop_front();
            if (!WaitFor([n] { return gH.gone || B::TlValue(FG_TL_OUT) >= 2ull * (ULONGLONG)n; }, 3000, 1)) Err("the fake helper did not answer pair %ld within 3 s of its answer's instant", n);
            const LONG lq = ShB::x.w.lastQN;
            if (!gShAtHand && lq > n && (!g.par.prepAtRet || B::TlValue(FG_TL_IN) >= (ULONGLONG)lq) && !WaitFor([lq] { return gH.gone || B::TlValue(FG_TL_PREP) >= (ULONGLONG)lq; }, 3000, 2)) Err("the fake helper did not prepare pair %ld within 3 s of its answer to pair %ld", lq, n);
            // (2026-10-14: what stood behind that answer on the queue has run by the time the helper reads as quiet: a present among it is read now, at this instant)
            if (g.par.Lists()) { MgQuiet(); RtLook(); }
        }
        else if (end) { const LONG f = g.ends.front().n; g.ends.pop_front(); RingTlSignal(FG_TL_END, (ULONGLONG)f, true); }
        else if (sig)
        {
            const LONG n = g.sigs.front().n, k = g.sigs.front().k; g.sigs.pop_front();
            if (k > 0)
            {   // (kUs: the helper holds its answer on this clock for k: it is in that hold, or was no dispatch, before the clock moves)
                _InterlockedExchange(&gH.delayUs, k);
                RingTlSignal(FG_TL_IN, (ULONGLONG)n, false);
                if (!WaitFor([n] { return gH.gone || gH.inDelay == n || B::TlValue(FG_TL_OUT) >= 2ull * (ULONGLONG)n; }, 3000, 3)) Err("the fake helper did not take up pair %ld within 3 s of its handoff's retire", n);
                if (B::TlValue(FG_TL_OUT) < 2ull * (ULONGLONG)n) g.answers.push_back({ n, B::clockNow + MR::UsToTicks(k), 0 });
            }
            else
            {
            RingTlSignal(FG_TL_IN, (ULONGLONG)n, false);
            // (the helper answers at the retire; the clock stands until it has, so its answer's instant is this one)
            if (!WaitFor([n] { return gH.gone || B::TlValue(FG_TL_OUT) >= 2ull * (ULONGLONG)n; }, 3000, 4)) Err("the fake helper did not answer pair %ld within 3 s of its handoff's retire", n);
            }
        }
        if (g.par.holdPlaceAt >= 0 && g.frameN == g.par.holdPlaceAt) { g.placeSkipped++; g.placerNext = 0; }   // (the plant: no step this frame, nor its timer)
        else g.placerNext = MgPlace<MR>();
        if (g.par.Lists()) { RtOrdCheck(); RtLook(); }        // (2026-10-14: the placer may have released an order wait: what stood behind it has run)
        if (WaitForSingleObject(B::evPace, 0) == WAIT_OBJECT_0)
        {
            if (how == 0) RtPoint();
            else if (how == 1) { woke = true; break; }
            else if (how == 3) { FireNote(); Drain(); RtLook(); }
            else g.owed = true;
        }
    }
    RtNextEv();
    g.busy = false;
    return woke;
}
// The machine's read of the clock on the render thread: one tick, and what has come due in that tick.
static void RtTick()
{
    RtSim& g = gRt;
    if (g.busy) return;
    B::clockNow += 1;
    if (g.nextEv && B::clockNow >= g.nextEv) RtRun(B::clockNow, 2);
}
// FgPace's wait (SleepUntil hands the instant its timer is set for, 600 us short of the sleep's end).
static int RtPaceHook(LONGLONG timerDue)
{
    const LONGLONG until = timerDue + MR::UsToTicks(600);
    RtAfter();                                                // the present's hand has run; a wake's fire note has run
    // WHAT THE REAL PACER HEARS: a wake while it waits on its timer, which is until `timerDue`. One that leaves 300 us or
    // less of that wait is its last (SleepUntil then spins, and hears nothing): the machine is handed every wake but
    // those, so that it always comes back here and nothing is left to a race of two threads; for those the harness
    // records the pacer's own fire note in its place (the same note, the same instant). In the 600 us behind
    // `timerDue` nobody hears.
    if (RtRun(timerDue - MR::UsToTicks(310), 1)) return 1;
    RtRun(timerDue, 3);
    RtRun(until, 2);
    if (until > B::clockNow) B::clockNow = until;
    return 0;
}
// One frame of the fake game: its work, its injection, its work, its Present (and the pacer's sleep inside it).
// THE PRESENTER'S HOLD (the two of RtFrame's head until 2026-10-18; RtPar::holdInOrig moves them inside the original Present)
static void RtPresenterHold()
{
    RtSim& g = gRt;
    // (2026-10-14, evening, RtPar::latencyUs: THE PRESENTER'S HOLD. A card more than this behind the clock holds the render thread in the
    // Present before this frame, where nobody serves a due; the frame begins when the queue is that close again)
    if (g.par.latencyUs > 0 && g.par.Queue())
    {
        const LONGLONG lim = g.qEnd - MR::UsToTicks(g.par.latencyUs);
        if (lim > B::clockNow) { g.holds++; g.holdTicks += lim - B::clockNow; if (g.par.holdWorker) InterlockedExchangeAdd64(&dtPresTicksSum, lim - B::clockNow); RtRun(lim, 2); if (lim > B::clockNow) B::clockNow = lim; }
    }
    if (g.par.latencyPresents > 0 && g.par.Queue())
    {   // (DXVK's count: the present latencyPresents before the last one issued must be shown before this frame begins)
        const size_t M = g.shownAll.size(); const size_t L = (size_t)g.par.latencyPresents;
        if (M > L)
        {
            const LONGLONG lim = g.shownAll[M - 1 - L];
            // (2026-10-18, round 3, SW: AND THE HOLD IS THE WORKER'S, as DXVK's is: its SyncFrameLatency waits inside the Present the worker replays,
            // and the render thread stands behind it in its one-frame-ahead wait. The fake holds the render thread here, where the frame begins;
            // the worker's sum of its time inside DXVK's Present carries the same hold, so the machine's reading of it (the fixed mode's, the
            // switch's RampQueueTick) reads what the real one would)
            if (lim > B::clockNow) { g.holdsP++; g.holdPTicks += lim - B::clockNow; InterlockedExchangeAdd64(&dtPresTicksSum, lim - B::clockNow); RtRun(lim, 2); if (lim > B::clockNow) B::clockNow = lim; }
        }
    }
}
static void RtFrame(LONGLONG cost, bool sent = true)
{
    RtSim& g = gRt; FgState& s = B::S();
    // (2026-10-18, round 3, G3: with RtPar::holdInOrig the presenter's two holds are made inside the original Present instead, RtOrigPresent)
    if (!g.par.holdInOrig) RtPresenterHold();
    g.frameN++;
    if (g.par.Queue()) g.depth.push_back(g.qEnd > B::clockNow ? (LONG)((g.qEnd - B::clockNow) * 1000000 / B::Qpf()) : 0);   // (2026-10-17: the card's queue ahead of the frame's begin)
    const LONGLONG r0 = B::clockNow, tInj = r0 + (LONGLONG)((double)cost * g.par.inj);
    const LONG forced0 = s.c.nEarly + s.c.nLate;
    if (g.owed) { g.owed = false; RtPoint(); }
    if (g.par.calls > 0)
    {   // (2026-10-17, RtPar::calls: THE SCENE IN CHECKPOINT CALLS. Each call reaches the card's queue at its own instant with its share of the scene and
        // the tail, behind whatever the queue holds (an order wait standing among it: RtDisp put qEnd behind its release); then the call itself
        // is replayed, so the worker's checkpoint observer runs there and a present it fires stands behind this much of the frame)
        const LONGLONG share = MR::UsToTicks((g.par.sceneUs + g.par.tailUs) / g.par.calls);
        for (int j = 1; j <= g.par.calls; j++)
        {
            const LONGLONG t = r0 + (tInj - r0) * j / (g.par.calls + 1);
            RtRun(t, 0);
            if (t > B::clockNow) B::clockNow = t;
            if (g.par.Queue()) { if (g.qEnd < t) g.qEnd = t; g.qEnd += share; }
            RtPoint();
        }
    }
    RtRun(tInj, 0);
    if (tInj > B::clockNow) B::clockNow = tInj;
    if (g.par.Queue() && !g.par.calls) { if (g.qEnd < tInj) g.qEnd = tInj; g.qEnd += MR::UsToTicks(g.par.tailUs); }   // (the queue: the scene's tail)
    ShInject(sent); Drain(); RtLook();
    // (the product's order writes 'Q' here: the helper prepares at once unless it stands in a dispatch whose handoff
    // has not retired yet, which the simulation will signal: then nothing is waited for)
    // (2026-10-13: and only where this injection wrote a 'Q': a frame of a standby, and the first of an engage, write none here)
    // (2026-10-17: and not where the prepare stands behind its own handoff, RtPar::prepAtRet: the simulation's retire event brings it)
    if (sent && !gShAtHand && !g.par.prepAtRet && g.sigs.empty() && g.answers.empty() && ShB::x.e.qN == B::sentN) { const LONG nq = B::sentN; if (!WaitFor([nq] { return gH.gone || B::TlValue(FG_TL_PREP) >= (ULONGLONG)nq; }, 3000, 5)) Err("the fake helper did not prepare pair %ld within 3 s of its injection", nq); }
    RtRun(r0 + cost, 0);
    if (r0 + cost > B::clockNow) B::clockNow = r0 + cost;
    const DWORD tag = NewTag();
    RtPair q = {}; q.tag = tag; q.cost = cost;
    g.byTag[tag] = (int)g.pairs.size(); g.pairs.push_back(q);
    Facts(sent); Fill(tag);
    // (2026-10-17, night, F1: THE PANEL MODEL's FIFO, THE RENDER THREAD'S ONE-FRAME-AHEAD WAIT: its Present waits for the worker's replay of
    // the Present before it, which on the worker's timeline stands behind every hold before it; off with the FIFO, as every case before)
    const bool fifo = g.par.panelHz > 0.0 && g.par.fifo > 0;      // (round 2: the wait itself is inside the original Present, RtOrigPresent)
    const LONGLONG cPres = B::clockNow;
    Present<MR>();
    if (fifo) g.opDone = cPres > g.wFree ? cPres : g.wFree;
    RtAfter();
    RtPair& w = g.pairs.back();
    w.p = s.r.lastP; w.perUs = s.r.periodUs; w.tEntry = s.r.lastEntry;
    // (a real frame brought out by this frame's injection or by this present's drain is the pair BEFORE's)
    if (g.pairs.size() >= 2) g.pairs[g.pairs.size() - 2].forced = s.c.nEarly + s.c.nLate - forced0;
}
// A FRESH MERGED MACHINE AT A PANEL, RUN IN (six unpaced frames at 16.7 ms: the facts have their samples), THEN THE
// SIMULATION'S OWN: the armed product's numbers as fg_arm_life.h's FgpArmKeys hands them in (the room asked for, the plan
// the answer's deadline, two late answers a hundred pairs apart raise it one bin, never over 8 ms), the plan itself
// the case's.
static void RtBegin(const RtPar& par)
{
    MgFresh<MR>();
    gRtTimed0[0] = ShArmB::a.pl.nTimed; gRtTimed0[1] = ShArmB::a.pl.tsOwn; gRtTimed0[2] = ShArmB::a.pl.tsNever;
    RtSim& g = gRt;
    g = RtSim(); g.par = par;
    if (gRtPanelHz > 0.0) { g.par.panelHz = gRtPanelHz; g.par.panelVrr = gRtPanelVrr; g.par.fifo = gRtPanelFifo; }   // (F1: FX0, a replay with the panel model forced on)
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    if (par.lsUs < 1000) Err("the simulation asks for a handoff that retires 1 ms or more behind its entry (%ld us)", par.lsUs);
    a.k.kPlanUs = par.planUs; a.k.lateUs = 0; a.k.minLeadUs = 0; a.k.room = 1; a.k.roomGuardUs = par.roomGuardUs;
    a.k.planAns = 1; a.k.planUpPairs = FgPlanLateSpan(990); a.k.planUpUs = FG_K_BIN_US; a.k.planMaxUs = par.planMaxUs;
    a.k.ride = par.ride; a.k.rideGuardUs = par.rideGuardUs; a.k.rideKMaxUs = par.ride ? par.rideKMaxUs : 0;
    // (2026-10-14: the three of "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT", as the case hands them in: 0 in every case before
    // dt_fg_late.h's; and with the lists' model on, the binding's stage of the queue and the body's home)
    a.k.over = par.over; a.k.card = par.card; a.k.ord = par.ord; a.k.ordMaxUs = par.rideKMaxUs;
    a.k.rideEarly = par.rideEarly; a.k.rideNoWait = par.rideNoWait; a.k.armTeach = par.armTeach; a.k.ordAtStamp = par.ordAtStamp;   // (2026-10-14, afternoon)
    a.k.rideWaitAns = par.rideWaitAns;                        // (2026-10-14, evening)
    a.k.roomSeed = par.roomSeed;                              // (2026-10-17)
    a.k.prepWait = par.prepWait; a.k.prepWaitMs = par.prepWaitMs; a.k.place = par.place; a.k.rideFitOnly = par.fitOnly;   // (2026-10-17, stage 3)
    a.k.ramp = par.ramp; a.k.rampAskUs = par.rampAskUs; a.k.rampFirst = par.rampFirst;   // (2026-10-18, S1: the switch-on)
    a.k.rampDrainOk = par.rampDrainOk;                        // (S1 round 2: the card's word for the switch's drain)
    gRtOrdBad.clear();
    if (par.Lists()) { ShArmB::laOn = 1; gRing.csGate = &ShArmB::LaCsGate; gShAfterOne = &ShArmB::LaQueueRun; }
    _InterlockedExchange(&gH.delayUs, 0);
    g.planK = FgPlanDefaults(); g.planK.startUs = par.planUs; g.planK.minUs = 1000; g.planK.quantilePm = 990; g.planK.downWindows = FG_PLAN_WINDOWS; g.planK.maxUs = par.planMaxUs;   // (fg_arm_life.h, FgpArmKeys)
    g.planMem.last = par.planUs; g.planMem.hold = 0;
    for (int i = 0; i < FG_K_BINS; i++) g.planSeen[i] = ShB::x.pl.kHist[i];
    ShWarm<MR>(); MgDueN();
    RtB::pendEnd.clear();
    if (par.Queue()) { gInjBody = RtInjBody; gPrevLatEnd = &RtB::PrevLatEnd; }   // (ShFresh installed ShInjBody; ShEnd takes whichever away)
    gArmedGuardOff = par.guardOff;
    _InterlockedExchange(&gH.prepAtIn, par.prepAtRet ? 1 : 0);   // (2026-10-17)
    gH.inWaitMs = 5000;
    ResetEvent(B::evPace);
    g.shownAt = Shown().size();
    B::refreshPacked = par.refresh;
    RtB::pendIn = 0; g.pendSeen = 0; RtB::defer = 1;
    // (2026-10-17, night, F1: THE PANEL MODEL, off unless asked: the grid's first refresh at the run's start and the phase asked for; the presents
    // of the run-in are on no panel; the machine's fixed-mode word as the case hands it in)
    if (g.par.panelHz > 0.0)
    {
        g.vbT = (double)B::Qpf() / g.par.panelHz;
        g.vb0 = B::clockNow + (LONGLONG)(g.par.panelPhase * g.vbT + 0.5);
        EnterCriticalSection(&gRing.data); g.ord0 = gRing.ord; LeaveCriticalSection(&gRing.data);
    }
    if (par.fixOn) WriteRelease(&s.r.fixOn, par.fixOn);
    if (par.fixBaseUs) WriteRelease(&s.r.fixBaseUs, par.fixBaseUs);
    g.on = true;
}
// The end: what is still owed is served (60 ms of events with the game "working"), then two unpaced frames with the
// want at PASS close the wire, and the fake's rules are read.
static void RtEnd(const char* tag)
{
    RtSim& g = gRt; FgState& s = B::S();
    RtRun(B::clockNow + MR::UsToTicks(60000), 0);
    if (g.owed) { g.owed = false; RtPoint(); }
    if (g.par.Lists()) { RtOrdCheck(); if (!g.ords.empty()) Err("%s: %zu order wait(s) were still standing 60 ms behind the run's last frame", tag, g.ords.size()); }
    g.on = false; RtB::defer = 0; gArmedGuardOff = 0; gPrevLatEnd = nullptr; _InterlockedExchange(&gH.delayUs, 0); _InterlockedExchange(&gH.prepAtIn, 0);
    if (s.r.fixOn) WriteRelease(&s.r.fixOn, 0);                  // (F1: the fixed-mode word goes with the run)
    if (s.r.fixBaseUs) WriteRelease(&s.r.fixBaseUs, 0);
    B::refreshPacked = 0; s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 2; i++) { ClockAdd(16.7); ShInject(true); Frame<MR>(); Drain(); }
    RtLook();
    if (!ShSettle()) Err("%s: the fake helper did not settle behind the run", tag);
    MgClean(tag);
    MgEnd();
}

// ------------------------------------------------------------------ what a stretch of a run reads
struct RtStat
{
    int n, gen, real1, evenN, forced;
    double rmsE, maxE, meanE, d1Pm, ageOver, perMs, perMin, perMax, costMs, fps, freeFps;
    LONG roomUs, planUs, planUps, ownN;
    // (2026-10-13) the two halves of a pair as the screen had them, their means in ms; the picture's age at its real
    // frame's showing, from its entry, at worst; and the ride's own numbers as the run left them
    double d1Ms, d2Ms, ageMs;
    LONG rideUs, rideDUs, rideEvenUs, rideWaitUs, rideFitUs, rode, rideGenLost, rideAhead, rideHands, rideNoEnd;
};
// Pairs [a, b) of the run. e and the age are in ms; d1Pm the generated frame's present to the real frame's as a share
// of the period in force, in thousandths; ageOver the most a real frame's present stood behind (its entry + the hand
// + the plan + half the period in force).
static RtStat RtRead(int a, int b)
{
    const RtSim& g = gRt; RtStat r = {};
    const int n = (int)g.pairs.size();
    if (b > n - 1) b = n - 1;
    if (a < 0) a = 0;
    double sumE = 0, sumE2 = 0, sumD1 = 0, sumPer = 0, sumCost = 0;
    r.perMin = 1e9; r.ageOver = -1e9;
    const double q = (double)B::Qpf() / 1000.0;
    for (int i = a; i < b; i++)
    {
        const RtPair& x = g.pairs[i]; const RtPair& y = g.pairs[i + 1];
        r.n++; r.gen += x.gen == 1 ? 1 : 0; r.real1 += x.real == 1 ? 1 : 0; r.forced += x.forced;
        const double per = (double)x.perUs / 1000.0;
        sumPer += per; sumCost += (double)x.cost / q; if (per < r.perMin) r.perMin = per; if (per > r.perMax) r.perMax = per;
        if (x.real == 1)
        {
            const double over = (double)(x.tReal - x.tEntry) / q - ((double)(g.par.lsUs + g.par.planUs) / 1000.0 + per * 0.5);
            if (over > r.ageOver) r.ageOver = over;
        }
        if (x.real == 1) { const double age = (double)(x.tReal - x.tEntry) / q; if (age > r.ageMs) r.ageMs = age; }
        if (x.gen == 1 && x.real == 1 && y.gen == 1)
        {
            const double d1 = (double)(x.tReal - x.tGen) / q, whole = (double)(y.tGen - x.tGen) / q, e = d1 - whole * 0.5;
            r.evenN++; sumE += e; sumE2 += e * e; if (fabs(e) > r.maxE) r.maxE = fabs(e);
            if (per > 0.0) sumD1 += d1 / per * 1000.0;
            r.d1Ms += d1; r.d2Ms += whole - d1;
        }
    }
    if (r.evenN) { r.d1Ms /= r.evenN; r.d2Ms /= r.evenN; }
    if (r.evenN) { r.meanE = sumE / r.evenN; r.rmsE = sqrt(sumE2 / r.evenN); r.d1Pm = sumD1 / r.evenN; }
    if (r.n) { r.perMs = sumPer / r.n; r.costMs = sumCost / r.n; r.freeFps = r.costMs > 0 ? 1000.0 / r.costMs : 0; }
    if (b > a) { const double span = (double)(g.pairs[b].tEntry - g.pairs[a].tEntry) / q; r.fps = span > 0 ? (double)(b - a) * 1000.0 / span : 0; }
    const FgArmedState& s = ShArmB::a;
    r.roomUs = s.pl.roomUs; r.planUs = s.k.kPlanUs; r.planUps = s.pl.planUps; r.ownN = s.r.ownN;
    r.rideUs = s.pl.rideUs; r.rideDUs = s.pl.rideDUs; r.rideEvenUs = s.pl.rideEvenUs; r.rideWaitUs = s.pl.rideWaitUs; r.rideFitUs = s.pl.rideFitUs;
    r.rode = s.w.rode; r.rideGenLost = s.w.rideGenLost; r.rideAhead = s.w.rideAhead; r.rideHands = s.r.rideN; r.rideNoEnd = s.pl.rideNoEnd;
    return r;
}
static const char* RtLine(const RtStat& r)
{
    static char bufs[8][640]; static int at = 0;              // (eight in turn: one printf names several)
    char* const b = bufs[at++ & 7];
    snprintf(b, sizeof(bufs[0]), "%d pair(s): generated frames shown %d, real frames shown once %d; the real frame against the middle of its two neighbours: rms %.3f ms, at worst %.3f ms, mean %+.3f ms (%d pair(s)); the generated frame to the real one %.0f thousandths of the period in force;"
        " the period in force %.3f ms (%.3f to %.3f), the game's cost %.3f ms: %.1f real frames a second where the game alone gives %.1f; the picture's age over the hand, the plan and half the period at worst %+.3f ms; real frames forced with no due %d",
        r.n, r.gen, r.real1, r.rmsE, r.maxE, r.meanE, r.evenN, r.d1Pm, r.perMs, r.perMin, r.perMax, r.costMs, r.fps, r.freeFps, r.ageOver, r.forced);
    return b;
}
// THE JUDGE, one stretch: every pair shown, each real frame once, the real frame within tolE of the middle, the picture's
// age no more than the hand, the plan and half the period in force (50 us for the clock's ticks), and THE PERIOD IN
// FORCE THE GAME'S OWN: never over the law's headroom on the stretch's dearest frame (1.06 times it, and the law's
// tolerance of 3 percent), or the first period the PANEL'S OWN free range has (2.4 of its refreshes, worked out here
// from the panel handed in), whichever is the longer. Returns what is wrong, or an empty string.
static std::string RtJudge(const RtStat& r, int a, int b, double tolE, bool own = true)
{
    const RtSim& g = gRt; std::string bad;
    if (r.gen != r.n) bad += Fmt("%d of %d generated frames shown; ", r.gen, r.n);
    if (r.real1 != r.n) bad += Fmt("%d of %d real frames shown exactly once; ", r.real1, r.n);
    if (r.evenN < r.n - 1) bad += Fmt("only %d of %d pairs could be read against their neighbours; ", r.evenN, r.n);
    if (r.maxE > tolE) bad += Fmt("a real frame %.3f ms off the middle of its neighbours (%.3f allowed); ", r.maxE, tolE);
    if (r.ageOver > 0.05) bad += Fmt("a picture %.3f ms older than the hand, the plan and half its period; ", r.ageOver);
    if (own)
    {
        double dear = 0; const double q = (double)B::Qpf() / 1000.0;
        for (int i = a; i < b && i < (int)g.pairs.size(); i++) { const double c = (double)g.pairs[i].cost / q; if (c > dear) dear = c; }
        const ULONG num = (ULONG)(g.par.refresh >> 32), den = (ULONG)g.par.refresh;
        const double refresh = 1000.0 * (double)den / (double)num, panel = refresh * 2.4, law = dear * 1.06 * 1.03 + 0.01;
        const double most = law > panel ? law : panel;
        if (r.perMax > most + 0.001) bad += Fmt("the period in force reached %.3f ms where the game's own is at most %.3f (its dearest frame %.3f with the law's headroom) and this panel's free range starts at %.3f; ", r.perMax, law, dear, panel);
    }
    return bad;
}
static LONGLONG RtMs(double ms) { return (LONGLONG)(ms * (double)B::Qpf() / 1000.0); }
// A deterministic spread: the cost times a factor between 1 - s and 1 + s, even over the range.
static unsigned gRtSeed = 1;
static double RtSpread(double s) { gRtSeed = gRtSeed * 1664525u + 1013904223u; return 1.0 + s * ((double)((gRtSeed >> 8) & 0xFFFFu) / 32767.5 - 1.0); }

// ---------------------------------------------------------------- FB1, steady, below the floor
// WHAT IT SHOWS (the brief's Part 1, case 1). The 120 Hz panel (the floor is 16.92 ms, 59.1 real frames a second), a game
// that costs 18.2, 23.8 and 33.3 ms a frame (55, 42 and 30 a second), 420 frames each from the pacer's start:
//   BEHIND THE ENGAGE (pairs 12 to 140: the pacer starts at the floor and the law does not move for 150 frames): every
//     pair is shown, and the real frame stands in the middle of its two generated neighbours, because THE PERIOD IN
//     FORCE IS THE GAME'S OWN where the game is slower than the pacer (fg_machine.h, PublishApplied);
//   SETTLED (pairs 220 on): the law has given the game its own period (at 55, 42 and 30 the cost
//     itself, WITHIN 1 PERCENT: the free band's form of 2026-10-17, fg_law.h, and since the owner's eye on leg 7 that evening
//     the free range from the floor, every build's since round 9's LW; 55 the same on a 144 and a 240 Hz panel), every pair is
//     shown, the real frame in the middle to 50 us, each real frame once and in order, the picture no older than the
//     hand, the plan and half the period; the room floor under the period, the plan where it was.
//   (A5's LAW at 42 went with its law, and the gap's control, the free range from 20 ms, with the key, in round 9's LW; THE FORCED
//   PERIOD with its key in round 9's S7.)
// CONTROL (two runs): THE MACHINE AS IT WAS UNTIL 2026-10-12, the period in force FgPace's own whatever the game costs (the
// guard's bit off, nothing else): the slot planted at the floor's half for a longer period, for the 150 frames behind
// every engage. The game at 30: behind the engage NOT ONE generated frame is shown (each frame is "a hitch", 33.3 ms
// against 1.5 times 16.9). The game at 42: shown, the real frame 3.4 ms off the middle on every pair (8.5 ms behind
// its generated frame, 15.3 ms ahead of the next). The judge says both; settled, where the law has moved, both read
// as the mended machine does.
// CONTROL (2026-10-17, two runs): THE LAW AS IT WAS UNTIL PHASE 5, its head of 1.06 (the guard's bit 12 off, nothing else):
// settled, the game at 42 is paced at 25.23 ms, 39.6 real frames a second, and the game at 30 at 35.30 ms, 28.3.
struct Fb1Leg { RtStat warm, set; std::string badWarm, badSet; int order; std::string first; };
static Fb1Leg Fb1Run(double costMs, const RtPar& par, int frames = 420, int setFrom = 220, bool own = true)
{
    Fb1Leg g = {};
    RtBegin(par);
    for (int i = 0; i < frames; i++) RtFrame(RtMs(costMs));
    g.warm = RtRead(12, 140); g.set = RtRead(setFrom, frames);
    g.badWarm = RtJudge(g.warm, 12, 140, 0.05, false); g.badSet = RtJudge(g.set, setFrom, frames, 0.05, own);
    g.order = gRt.order; g.first = gRt.firstBad;
    RtEnd("FB1");
    return g;
}
static void CaseFB1()
{
    const char* const name = "FB1 the free band, steady: a game at 55, 42 and 30 paced by its own period, behind an engage and settled (phase 4)";
    CaseBegin(name);
    static const double kCost[3] = { 18.2, 23.8, 33.3 };
    static const char* const kName[3] = { "55", "42", "30" };
    RtPar par;
    for (int i = 0; i < 3; i++)
    {
        const Fb1Leg g = Fb1Run(kCost[i], par);
        printf("  a game at %s (%.1f ms), behind the engage: %s\n", kName[i], kCost[i], RtLine(g.warm));
        printf("  a game at %s, settled: %s; the room floor %ld us, the plan %ld us (raised %ld time(s)); hands with the game's own period %ld; pictures out of place %d\n",
            kName[i], RtLine(g.set), g.set.roomUs, g.set.planUs, g.set.planUps, g.set.ownN, g.order);
        if (!g.badWarm.empty()) Err("FB1: a game at %s, behind the engage: %s", kName[i], g.badWarm.c_str());
        if (!g.badSet.empty()) Err("FB1: a game at %s, settled: %s", kName[i], g.badSet.c_str());
        if (g.order) Err("FB1: a game at %s: %d picture(s) out of place; the first: %s", kName[i], g.order, g.first.c_str());
        if (g.set.planUs != par.planUs || g.set.planUps || (double)g.set.roomUs / 1000.0 > g.set.perMin) Err("FB1: a game at %s: the plan %ld us (raised %ld time(s)), the room floor %ld us against a period of %.3f ms (%ld, 0 and a floor under the period wanted: neither climbs)", kName[i], g.set.planUs, g.set.planUps, g.set.roomUs, g.set.perMin, par.planUs);
        if (g.warm.ownN <= 0) Err("FB1: a game at %s: no hand carried the game's own period behind the engage", kName[i]);
        // (2026-10-17, the free band's form and the free range from the floor: the game's own cost within 1 percent)
        const double want = kCost[i];
        if (g.set.perMax > want * 1.01 || g.set.perMin < want - 0.001 || g.set.fps < 1000.0 / want * 0.99)
            Err("FB1: a game at %s, settled: the period in force %.3f to %.3f ms, %.2f real frames a second (%.3f ms within 1 percent, %.2f a second, wanted)", kName[i], g.set.perMin, g.set.perMax, g.set.fps, want, 1000.0 / want);
    }
    for (unsigned hz : { 144u, 240u })                            // (2026-10-17, evening) the game at 55 on two other panels
    {
        RtPar ph; ph.refresh = ((ULONGLONG)hz << 32) | 1ull;
        const Fb1Leg g = Fb1Run(18.2, ph);
        printf("  %u Hz, a game at 55, settled: %s; pictures out of place %d\n", hz, RtLine(g.set), g.order);
        if (!g.badWarm.empty() || !g.badSet.empty() || g.order || g.set.perMax > 18.2 * 1.01 || g.set.perMin < 18.199 || g.set.fps < 1000.0 / 18.2 * 0.99)
            Err("FB1: %u Hz, a game at 55: behind the engage: %s settled: %s the period in force %.3f to %.3f ms, %.2f real frames a second, pictures out of place %d (18.2 within 1 percent wanted)", hz, g.badWarm.c_str(), g.badSet.c_str(), g.set.perMin, g.set.perMax, g.set.fps, g.order);
    }
    // (the forced period's arm went with its key in round 9's S7; A5's law at 42, and the gap's control (the free range from 20 ms),
    // with the law's keys in round 9's LW)
    // CONTROLS
    const RingStats stc = gRingSt;
    RtPar pc; pc.guardOff = 16;
    const Fb1Leg c30 = Fb1Run(33.3, pc), c42 = Fb1Run(23.8, pc), c55 = Fb1Run(18.2, pc);
    ViolRestore(stc);
    printf("  CONTROL, the period in force FgPace's own (the machine until this date), behind the engage: a game at 30: %s\n  CONTROL, a game at 42: %s\n  CONTROL, a game at 55: %s\n", RtLine(c30.warm), RtLine(c42.warm), RtLine(c55.warm));
    if (c30.badWarm.empty() || c30.warm.gen != 0) Err("FB1 control: with the floor's half planted the game at 30 showed %d generated frames of %d behind the engage and the judge said \"%s\" (none shown, and a fault, wanted): the control shows nothing", c30.warm.gen, c30.warm.n, c30.badWarm.c_str());
    if (c42.badWarm.empty() || c42.warm.maxE < 3.0 || c55.badWarm.empty() || c55.warm.maxE < 0.5) Err("FB1 control: with the floor's half planted the real frame was %.3f ms off the middle at worst at 42 and %.3f ms at 55, and the judge said \"%s\" and \"%s\" (3.4 and 0.6 ms, and a fault each, wanted): the control shows nothing", c42.warm.maxE, c55.warm.maxE, c42.badWarm.c_str(), c55.badWarm.c_str());
    if (!c30.badSet.empty() || !c42.badSet.empty() || !c55.badSet.empty()) Err("FB1 control: settled, where the law has moved, the machine as it was should read clean: \"%s\" \"%s\" \"%s\"", c30.badSet.c_str(), c42.badSet.c_str(), c55.badSet.c_str());
    // (2026-10-17) THE HEAD'S CONTROL: the guard's bit 12 off, the law of 2026-10-05 at 1.06
    const RingStats sth = gRingSt;
    RtPar ph; ph.guardOff = 1 << 12;
    const Fb1Leg h42 = Fb1Run(23.8, ph), h30 = Fb1Run(33.3, ph);
    ViolRestore(sth);
    printf("  CONTROL, the law's head of 1.06 (the law until phase 5), settled: a game at 42: %s\n  CONTROL, a game at 30: %s\n", RtLine(h42.set), RtLine(h30.set));
    if (h42.set.perMs < 25.2 || h42.set.perMs > 25.26 || h42.set.fps > 39.7 || h30.set.perMs < 35.27 || h30.set.perMs > 35.33 || h30.set.fps > 28.4 || !h42.badSet.empty() || !h30.badSet.empty() || h42.order || h30.order)
        Err("FB1 control: with the law's head of 1.06 the game at 42 was paced at %.3f ms (%.2f a second) and the game at 30 at %.3f ms (%.2f), judged \"%s\" and \"%s\" (25.23 and 39.6, 35.30 and 28.3, clean, wanted): the control shows nothing", h42.set.perMs, h42.set.fps, h30.set.perMs, h30.set.fps, h42.badSet.c_str(), h30.badSet.c_str());
    Ctl("the machine as it was (the period in force FgPace's own): for the 150 frames behind an engage the slot is planted at the floor's half: at 30 not one generated frame of %d is shown (each frame reads as a hitch); at 42 the real frame is %.2f ms off the middle of its neighbours on every pair, at 55 %.2f ms;"
        " and the law's head of 1.06: the game at 42 paced at %.2f ms (%.1f real frames a second), at 30 at %.2f ms (%.1f)", c30.warm.n, c42.warm.maxE, c55.warm.maxE, h42.set.perMs, h42.set.fps, h30.set.perMs, h30.set.fps);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FB2, a cost that wanders
// WHAT IT SHOWS (the brief's Part 1, case 2). The game at 42 (23.8 ms) with each frame's cost spread evenly by 0, 5, 10, 15
// and 25 percent, 700 frames each, read from pair 260 on; then a slow drift from 50 a second to 36 and back (20.0 to
// 27.8 ms over 500 frames each way). FOR EACH: no real frame lost, doubled or out of order, every pair shown, the
// picture no older than the hand, the plan and half the period. AND THE TABLE: how far the real frame stands off the
// middle of its two generated neighbours, and what the law's headroom costs in real frames.
//   WHAT THE NUMBERS ARE. FgPace paces the START of a frame (its release), so two presents are a period apart only when
//   two frames cost the same: the real frame is off the middle by HALF THE CHANGE OF COST from one frame to the next,
//   and by nothing else while the pacer sleeps. The law's own share is the period it asks for: the dearest frames of the
//   last four seconds (L, the 98th of them; since 2026-10-17 at a head of 1.0 and a way up with no tolerance under it,
//   fg_law.h), so a spread of 15 percent is paced 13 percent under the game's mean rate (36.7 real frames a second
//   against 42.1; 34.8 under the head of 1.06).
//   (2026-10-17) A SIXTH ROW, A TAIL: a spread of 3 percent and one frame in 50 dearer by 12 percent, a frame L does not
//   cover, so the pacer is late behind it (its due passed: the clamp). Held as the spread is (half the whole width and
//   0.35 ms), every pair shown: a frame past the period moves its real frame by half the change of cost, as any other.
//   HELD HERE: the real frame is never further off the middle than half the spread's whole width (the dearest frame
//   behind the cheapest) and 0.35 ms; on the drift, 0.25 ms. THE 0.35 ms IS THE PACER'S SPIN: FgPace spins on the clock
//   through the last 0.6 ms of its sleep and hears no wake there, so a due the placer raises in that stretch is
//   served by the next frame's first call, up to 0.6 ms late, which moves e by half of that. It was so at the floor
//   too; there the slot does not fall into the spin (the sleep ends 3 ms behind it). Counted in no line of the mod's.
//   (In these five streams the worst pair of every row reads inside the spread's own half width: the allowance is for
//   a stream that puts a slot there, not for something seen.)
// (NO CONTROL OF ITS OWN since round 9's S7: the floor's half planted needed the floor's hold, a development key; FB1's control
// holds the plant.)
static RtStat Fb2Spread(double spread, const RtPar& par, std::string* bad, int* order, double tail = 0.0)
{
    RtBegin(par);
    gRtSeed = 20261012u;
    for (int i = 0; i < 700; i++) RtFrame(RtMs(23.8 * RtSpread(spread) * (tail > 0.0 && i % 50 == 25 ? 1.0 + tail : 1.0)));
    const RtStat r = RtRead(260, 700);
    // (the tail: half the whole width is half of the dearest tail frame less the cheapest frame)
    const double tol = tail > 0.0 ? 23.8 * ((1.0 + tail) * (1.0 + spread) - (1.0 - spread)) * 0.5 + 0.35 : 23.8 * spread + (spread > 0.0 ? 0.35 : 0.05);
    *bad = RtJudge(r, 260, 700, tol);
    *order = gRt.order;
    RtEnd("FB2");
    return r;
}
static void CaseFB2()
{
    const char* const name = "FB2 the free band, a cost that wanders: the real frame's place against the spread, and a drift from 50 to 36 and back (phase 4)";
    CaseBegin(name);
    static const double kSpread[5] = { 0.0, 0.05, 0.10, 0.15, 0.25 };
    RtPar par;
    printf("  a game at 42 (23.8 ms), each frame's cost spread evenly:\n  spread   period in force   real frames a second (alone)   the real frame off the middle: rms, at worst   shown   forced with no due\n");
    for (int i = 0; i < 5; i++)
    {
        std::string bad; int order = 0;
        const RtStat r = Fb2Spread(kSpread[i], par, &bad, &order);
        printf("  %3.0f %%    %7.3f ms         %5.1f (%5.1f)                  %.3f ms, %.3f ms                          %d of %d   %d\n",
            kSpread[i] * 100.0, r.perMs, r.fps, r.freeFps, r.rmsE, r.maxE, r.gen, r.n, r.forced);
        // (at 25 percent a frame's injection can come ahead of the real frame's due, and the early rule then forces it:
        // printed, and held to order and to "every real frame once" alone)
        if (kSpread[i] <= 0.151 ? !bad.empty() : (r.real1 != r.n)) Err("FB2: a spread of %.0f percent: %s", kSpread[i] * 100.0, bad.empty() ? "a real frame not shown exactly once" : bad.c_str());
        if (order) Err("FB2: a spread of %.0f percent: %d picture(s) out of place; the first: %s", kSpread[i] * 100.0, order, gRt.firstBad.c_str());
    }
    {   // (2026-10-17) the tail: a spread of 3 percent, one frame in 50 dearer by 12 percent
        std::string bad; int order = 0;
        const RtStat r = Fb2Spread(0.03, par, &bad, &order, 0.12);
        printf("  3 %% and a tail of one frame in 50 at +12 %%: %7.3f ms, %5.1f (%5.1f) real frames a second, the real frame off the middle: rms %.3f ms, at worst %.3f ms, shown %d of %d, forced with no due %d\n",
            r.perMs, r.fps, r.freeFps, r.rmsE, r.maxE, r.gen, r.n, r.forced);
        if (!bad.empty() || order) Err("FB2: a spread of 3 percent with a tail: %s pictures out of place %d (%s)", bad.c_str(), order, gRt.firstBad.c_str());
        RtPar ph = par; ph.guardOff = 1 << 12;                    // (told beside it: the law's head of 1.06, the law until phase 5)
        std::string badH; int orderH = 0;
        const RtStat h = Fb2Spread(0.03, ph, &badH, &orderH, 0.12);
        printf("  the same under the law's head of 1.06 (told): %7.3f ms, %5.1f real frames a second, the real frame off the middle: rms %.3f ms, at worst %.3f ms, shown %d of %d, forced with no due %d%s%s\n",
            h.perMs, h.fps, h.rmsE, h.maxE, h.gen, h.n, h.forced, badH.empty() ? "" : "; the judge: ", badH.c_str());
    }
    {   // the drift
        RtBegin(par);
        for (int i = 0; i < 200; i++) RtFrame(RtMs(20.0));
        for (int i = 0; i < 500; i++) RtFrame(RtMs(20.0 + 7.8 * (double)i / 500.0));
        for (int i = 0; i < 500; i++) RtFrame(RtMs(27.8 - 7.8 * (double)i / 500.0));
        const RtStat up = RtRead(200, 700), down = RtRead(700, 1200);
        const std::string bu = RtJudge(up, 200, 700, 0.25), bd = RtJudge(down, 700, 1200, 0.25, false);
        const int order = gRt.order; const std::string first = gRt.firstBad;
        RtEnd("FB2 (the drift)");
        printf("  a drift from 50 a second to 36 (20.0 to 27.8 ms over 500 frames): %s\n  and back: %s; pictures out of place %d\n", RtLine(up), RtLine(down), order);
        if (!bu.empty() || !bd.empty() || order) Err("FB2: the drift: on the way down the rate: %s on the way back: %s pictures out of place %d (%s)", bu.c_str(), bd.c_str(), order, first.c_str());
    }
    // (THE CONTROL, the floor's half planted under a spread of 15 percent, held the pacer at its floor by the development key
    // fg_test_floor, the only way a settled law leaves the period in force apart from the game's; it went with the key in round 9's
    // S7. The same plant, guard bit 16 off, stays FB1's control, read behind the engage where the law holds the floor.)
    CaseEnd(name);
}

// ---------------------------------------------------------------- FB3, the floor crossed both ways
// WHAT IT SHOWS (the brief's Part 1, case 3). A game at 65 a second (15.4 ms: the pacer holds it at the floor, 59.1), which
// falls to 50 (20.0 ms) over 30 frames, stays 300 frames, climbs back over 30 and stays 900 (the law crosses a band
// downward only when its twelve seconds' memory agrees). NO PAIR IS LOST AT EITHER CROSSING: every generated frame
// shown, every real frame once and in order. NO REAL FRAME IS OFF THE MIDDLE BY MORE THAN THE LAW'S OWN MOVE: half the step
// between the floor and the free range's first period (1.54 ms on this panel), at the two frames where the law
// moves, and half the cost's own change a frame elsewhere. At rest on either side the real frame is in the middle.
// (NO CONTROL OF ITS OWN since round 9's S7: the floor's half planted needed the floor's hold, a development key; FB1's control
// holds the plant.)
struct Fb3Leg { RtStat floor0, fall, rest, climb, floor1; int order; std::string first, bad; LONG moves; };
static Fb3Leg Fb3Run(const RtPar& par)
{
    Fb3Leg g = {};
    RtBegin(par);
    for (int i = 0; i < 260; i++) RtFrame(RtMs(15.4));
    for (int i = 0; i < 30; i++) RtFrame(RtMs(15.4 + 4.6 * (double)(i + 1) / 30.0));
    for (int i = 0; i < 300; i++) RtFrame(RtMs(20.0));
    for (int i = 0; i < 30; i++) RtFrame(RtMs(20.0 - 4.6 * (double)(i + 1) / 30.0));
    for (int i = 0; i < 900; i++) RtFrame(RtMs(15.4));
    g.floor0 = RtRead(200, 258); g.fall = RtRead(258, 330); g.rest = RtRead(380, 588); g.climb = RtRead(588, 1500); g.floor1 = RtRead(1440, 1520);
    g.moves = B::S().r.moves;
    const double floorMs = MR::CPeriodMs();
    auto add = [&](const char* what, const std::string& b) { if (!b.empty()) g.bad += std::string(what) + ": " + b; };
    add("at the floor", RtJudge(g.floor0, 200, 258, 0.05, false));
    add("the fall", RtJudge(g.fall, 258, 330, (20.0 - floorMs) * 0.5 + 0.10, false));
    add("at rest at 50", RtJudge(g.rest, 380, 588, 0.05));
    add("the climb and the way back", RtJudge(g.climb, 588, 1500, (20.0 - floorMs) * 0.5 + 0.10, false));
    add("at the floor again", RtJudge(g.floor1, 1440, 1520, 0.05, false));
    if (g.floor1.perMax > floorMs + 0.001) g.bad += Fmt("the period did not come back to the floor (%.3f ms against %.3f); ", g.floor1.perMax, floorMs);
    g.order = gRt.order; g.first = gRt.firstBad;
    RtEnd("FB3");
    return g;
}
static void CaseFB3()
{
    const char* const name = "FB3 the floor crossed both ways: a game at 65 that falls to 50 and climbs back, no pair lost, no real frame off by more than the law's move (phase 4)";
    CaseBegin(name);
    RtPar par;
    const Fb3Leg g = Fb3Run(par);
    printf("  at the floor (a game at 65): %s\n  the fall to 50 over 30 frames, and 42 behind it: %s\n  at rest at 50: %s\n  the climb back and the way down to the floor: %s\n  at the floor again: %s\n  the pacer moved %ld time(s); pictures out of place %d\n",
        RtLine(g.floor0), RtLine(g.fall), RtLine(g.rest), RtLine(g.climb), RtLine(g.floor1), g.moves, g.order);
    if (!g.bad.empty()) Err("FB3: %s", g.bad.c_str());
    if (g.order) Err("FB3: %d picture(s) out of place; the first: %s", g.order, g.first.c_str());
    // (THE CONTROL, the floor's half planted at rest at 50, held the pacer at its floor by the development key fg_test_floor; it
    // went with the key in round 9's S7. The same plant, guard bit 16 off, stays FB1's control, read behind the engage.)
    CaseEnd(name);
}

/// ---------------------------------------------------------------- FB4, every panel
// WHAT IT SHOWS (the brief's Part 1, case 4; the tenet's two: "60 real shown as 120 on a 144 Hz screen", and a 240 Hz
// panel at about 100; SINCE 2026-10-17, phase 5, EVERY PANEL: 144, 180 and 240 Hz beside FB1's 120, and the free band's
// form, fg_law.h). 420 frames a game, read behind the engage and settled, FB1's judge; each panel's floor is its own (two
// refreshes times 1.015), never 120 Hz's, and the free range starts at it (round 9's LW: "free from" below is the old gap's
// start, 2.4 refreshes, which no build has since):
//   144 Hz (the floor 14.10 ms, 70.9; free from 16.67): a game at 60 (16.67 ms) is paced at 16.67 ms, 60.0 real frames,
//     120 shown (56.6 under the law's head of 1.06); a game at 50 at 20.0 ms.
//   180 Hz (the floor 11.28 ms, 88.7; free from 13.33): a game at 70 (14.29 ms) at 14.29 ms, 70.0.
//   240 Hz (the floor 8.46 ms, 118.2; free from 10.0): a game at 90 (11.11 ms) at 11.11 ms, 90.0 (84.9 under the head of
//     1.06), and a game at 100 (10.0 ms) at 10.0 ms, 100 (94.3).
//   EACH PANEL'S GAME AGAIN WITH A SPREAD OF 5 PERCENT: every pair shown, each real frame once and in order, the real frame
//     within half the spread's whole width and 0.35 ms of the middle (FB2's allowance), the period the game's own.
//   AT 240 Hz, WITH THE HAND OF THE OWNER'S 4K (3.35 ms), THE PAIR DOES NOT FIT IN THIS FAKE BELOW ABOUT 11.9 ms: the next
//     frame's injection comes 0.41 ms (at 90) and 0.85 ms (at 100) ahead of the real frame's due, and the early rule
//     forces it, every pair (printed, counted; under the old head 0.07 and 0.55 ms: the head's 6 percent hid part of it).
//     The room floor has no term for the injection's lead on the Present. TWO THINGS ANSWER IT, neither the pacer's: the
//     room's margin (FgArmedKeys::roomGuardUs, which the product hands in as 1,250 us since phase 5's round 2, for every pair since round 6:
//     with 1,250 us handed in the room floor is 12.25 ms, 81.6 real frames, and the pair is even), and THE RIDE, which this
//     fake cannot show (it shows a present at its call; FQ3 is the 240 Hz panel with the card's queue, riding at the
//     panel's floor). So the 240 Hz games are held to order, to every pair shown and real frame once, and to the period;
//     the leg with the margin to the whole judge.
//   A GAME IN EACH PANEL'S OLD GAP between its floor and 2.4 refreshes (144 Hz at 65, 180 Hz at 80, 240 Hz at 110): until
//     the owner's eye on leg 7 (2026-10-17, evening) told and paced at 2.4 refreshes; since, JUDGED at its own period within
//     1 percent, the free range starting at the floor on every panel (every build's since round 9's LW).
// (THE GAP'S CONTROL, the free range's start as the key's 20 ms on two panels, went with the key in round 9's LW.)
// CONTROL (2026-10-17, a run): the law's head of 1.06 (the guard's bit 12 off): the 144 Hz panel's game of 60 is paced at
// 17.67 ms, 56.6 real frames a second.
struct Fb4Game { const char* what; unsigned hz; double costMs; };
static RtStat Fb4Spread(const RtPar& par, double costMs, double spread, std::string* bad, int* order)
{
    RtBegin(par);
    gRtSeed = 20261017u;
    for (int i = 0; i < 600; i++) RtFrame(RtMs(costMs * RtSpread(spread)));
    const RtStat r = RtRead(260, 600);
    *bad = RtJudge(r, 260, 600, costMs * spread + 0.35);
    *order = gRt.order;
    RtEnd("FB4 (a spread)");
    return r;
}
static void CaseFB4()
{
    const char* const name = "FB4 every panel: 144, 180 and 240 Hz, each game paced by its own period, steady and with a spread (phase 4; every panel since phase 5)";
    CaseBegin(name);
    static const Fb4Game kGame[4] = { { "144 Hz, a game at 60", 144, 1000.0 / 60.0 }, { "144 Hz, a game at 50", 144, 20.0 },
                                      { "180 Hz, a game at 70", 180, 1000.0 / 70.0 }, { "240 Hz, a game at 90", 240, 1000.0 / 90.0 } };
    // (2026-10-13: THE 240 Hz LEGS RUN WITH THE RIDE NOT ASKED FOR, the machine until that date, which is what their numbers
    // are of: a pair there is close to the room floor, and this fake shows a present at its call, so a ride here would be
    // a place with no meaning. FQ3 is that panel with the card's queue, riding.)
    for (int i = 0; i < 4; i++)
    {
        const Fb4Game& gm = kGame[i];
        RtPar p; p.refresh = ((ULONGLONG)gm.hz << 32) | 1ull; if (gm.hz == 240) p.ride = 0;
        p.roomGuardUs = 0;                                        // (round 2: the pacer's own legs, with no room margin, the machine before it; the margin's leg is below)
        const Fb1Leg g = Fb1Run(gm.costMs, p);
        printf("  %s (%.2f ms), behind the engage: %s\n  %s, settled: %s; the room floor %ld us; pictures out of place %d\n", gm.what, gm.costMs, RtLine(g.warm), gm.what, RtLine(g.set), g.set.roomUs, g.order);
        if (gm.hz == 240 ? (g.set.gen != g.set.n || g.set.real1 != g.set.n || g.warm.gen != g.warm.n || g.warm.real1 != g.warm.n || g.order)
                         : (!g.badWarm.empty() || !g.badSet.empty() || g.order))
            Err("FB4: %s: behind the engage: %s settled: %s shown %d of %d and %d of %d, real frames once %d and %d, pictures out of place %d (%s)", gm.what, g.badWarm.c_str(), g.badSet.c_str(), g.warm.gen, g.warm.n, g.set.gen, g.set.n, g.warm.real1, g.set.real1, g.order, g.first.c_str());
        if (g.set.perMax > gm.costMs * 1.01 || g.set.perMin < gm.costMs - 0.001 || g.set.fps < 1000.0 / gm.costMs * 0.99)
            Err("FB4: %s, settled: the period in force %.3f to %.3f ms, %.2f real frames a second (%.3f ms within 1 percent, %.2f a second, wanted)", gm.what, g.set.perMin, g.set.perMax, g.set.fps, gm.costMs, 1000.0 / gm.costMs);
        if (i == 1) continue;                                     // (the spread on each panel's first game)
        std::string bad; int order = 0;
        const RtStat r = Fb4Spread(p, gm.costMs, 0.05, &bad, &order);
        printf("  %s with a spread of 5 percent: %s\n", gm.what, RtLine(r));
        if (gm.hz == 240 ? (r.gen != r.n || r.real1 != r.n || order) : (!bad.empty() || order))
            Err("FB4: %s with a spread of 5 percent: %s shown %d of %d, real frames once %d, pictures out of place %d (%s)", gm.what, bad.c_str(), r.gen, r.n, r.real1, order, gRt.firstBad.c_str());
    }
    RtPar p240; p240.refresh = (240ull << 32) | 1ull; p240.ride = 0; p240.roomGuardUs = 0;   // (round 2: the margin-0 arm, the control of the product's value)
    RtPar p240g = p240; p240g.roomGuardUs = 1250;
    const Fb1Leg b = Fb1Run(10.0, p240), bg = Fb1Run(10.0, p240g, 420, 220, false);
    printf("  240 Hz, a game at 100, settled: %s; the room floor %ld us; pictures out of place %d\n", RtLine(b.set), b.set.roomUs, b.order);
    printf("  240 Hz, the same with a room margin of 1,250 us handed in, behind the engage: %s\n  240 Hz, that leg settled: %s; the room floor %ld us; pictures out of place %d\n", RtLine(bg.warm), RtLine(bg.set), bg.set.roomUs, bg.order);
    if (b.set.gen != b.set.n || b.set.real1 != b.set.n || b.order || b.set.perMs < 9.999 || b.set.perMs > 10.05 || b.set.fps < 99.0)
        Err("FB4: 240 Hz, a game at 100: %d of %d generated frames shown, %d real frames once, %d picture(s) out of place, the period in force %.3f ms, %.2f real frames a second (all, all, 0, 10.00 and 100 wanted)", b.set.gen, b.set.n, b.set.real1, b.order, b.set.perMs, b.set.fps);
    if (!bg.badSet.empty() || bg.order || bg.set.forced) Err("FB4: 240 Hz with the room's margin: %s pictures out of place %d, real frames forced with no due %d", bg.badSet.c_str(), bg.order, bg.set.forced);
    {   // TOLD: a game in each panel's gap
        static const Fb4Game kGap[3] = { { "144 Hz, a game at 65", 144, 1000.0 / 65.0 }, { "180 Hz, a game at 80", 180, 1000.0 / 80.0 }, { "240 Hz, a game at 110", 240, 1000.0 / 110.0 } };
        for (int i = 0; i < 3; i++)
        {
            RtPar p; p.refresh = ((ULONGLONG)kGap[i].hz << 32) | 1ull; if (kGap[i].hz == 240) p.ride = 0;
            p.roomGuardUs = 0;                                    // (round 2: as the panels' own legs above)
            const Fb1Leg g = Fb1Run(kGap[i].costMs, p);
            printf("  the gap of 2026-10-12: %s (%.2f ms), settled: the period in force %.3f ms, %.1f real frames a second (2.4 refreshes, the free range's old start, %.3f ms); shown %d of %d, pictures out of place %d\n",
                kGap[i].what, kGap[i].costMs, g.set.perMs, g.set.fps, 2400.0 / (double)kGap[i].hz, g.set.gen, g.set.n, g.order);
            if (g.order || g.set.real1 != g.set.n || g.set.gen != g.set.n) Err("FB4: %s (the gap): %d picture(s) out of place, %d of %d real frames once, %d generated shown", kGap[i].what, g.order, g.set.real1, g.set.n, g.set.gen);
            // (2026-10-17, evening: JUDGED SINCE THE OWNER'S EYE ON LEG 7, the free range from the floor: the game's own period within 1
            // percent, or the room floor where the pair needs more, as at 240 Hz with the 4K hand: 9.75 ms for a game of 9.09)
            const double want = kGap[i].costMs > (double)g.set.roomUs / 1000.0 ? kGap[i].costMs : (double)g.set.roomUs / 1000.0;
            if (g.set.perMax > want * 1.01 || g.set.perMin < want - 0.001 || g.set.fps < 1000.0 / want * 0.99)
                Err("FB4: %s (the gap): the period in force %.3f to %.3f ms, %.2f real frames a second (%.3f ms within 1 percent wanted: the game's own, or the room floor %ld us)", kGap[i].what, g.set.perMin, g.set.perMax, g.set.fps, want, g.set.roomUs);
        }
    }
    // (THE GAP'S CONTROLS, the free range's start at the key's 20 ms at 144 and 240 Hz, went with the key in round 9's LW: the free range
    // from the floor is every build's)
    const RingStats stc = gRingSt;
    RtPar h144; h144.refresh = (144ull << 32) | 1ull; h144.guardOff = 1 << 12;
    const Fb1Leg ch = Fb1Run(1000.0 / 60.0, h144);
    ViolRestore(stc);
    printf("  CONTROL, the law's head of 1.06, 144 Hz: %s\n", RtLine(ch.set));
    if (ch.set.perMs < 17.6 || ch.set.perMs > 17.75 || ch.set.fps > 56.7) Err("FB4 control: with the law's head of 1.06 the 144 Hz panel's game of 60 was paced at %.3f ms, %.2f real frames a second (17.67 and 56.6 wanted): the control shows nothing", ch.set.perMs, ch.set.fps);
    Ctl("the law's head of 1.06: the 144 Hz panel's game of 60 at %.2f ms, %.1f real frames a second", ch.set.perMs, ch.set.fps);
    CaseEnd(name);
}

// ---------------------------------------------------------------- PL1, THE PRODUCT'S LAW ON THE PRODUCT'S BINDING (round 9, 2026-10-27)
// docs/DECISIONS.md, "THE NEW PACING LAW SHIPS; A5 GOES". WHAT IT SHOWS: the merged machine closed over the fake game as every FB case
// runs it, with THE PRODUCT'S LAW: nothing of the law set by the case (the keys, FgKeys, went at round 9's merge). A game of 12 ms
// on the 120 Hz panel, paced at the floor (16.92 ms), 420 frames to settle, then:
//   (a) ONE frame of 31 ms (the plant of the owner's 5r and 5s legs of 2026-10-05): the period never leaves the floor and the pacer makes
//       no move in the 300 frames behind it;
//   (b) EIGHT of 31 ms in a row, then 12 ms again: eight open the law's door (the level may have moved: the eighth and every later one is
//       taken), but one taken sample is not the level: no move;
//   (c) A LEVEL THAT HOLDS, thirty of 31 ms in a row: ONE rise, to the level or the law's top (the 4 s median plus 18 ms: 30.0 ms here),
//       once the 4 s memory's E is over the floor (its twelve dearest at 59 a second: the level's eighteenth frame); then 12 ms again, and
//       the period back ON the floor within 16 s (the 12 s memory agrees about 12 s behind the level), with no rise between.
// FAIL FIRST ON THE BASE (1b4a30da, FgKeys::law 0, A5's law in the product): the one frame of (a) is taken as the game's new cost and the
// period climbs off the floor. NO CONTROL OF ITS OWN: (c) is the reading's positive, the same reading seeing the law's one rise.
static void CasePL1()
{
    const char* const name = "PL1 the product's law on the product's binding: one long frame and eight in a row move nothing, a level that holds gives one rise and a return to the floor (round 9)";
    CaseBegin(name);
    RtPar par;
    RtBegin(par);
    FgState& s = B::S();
    int frame = 0;
    auto run = [&](int n, double ms, double* most, LONG* ups, LONG* downs)
    {
        for (int i = 0; i < n; i++)
        {
            const double before = s.r.period;
            RtFrame(RtMs(ms)); frame++;
            const double p = s.r.period;
            if (most && p > *most) *most = p;
            if (ups && p > before + 1e-9) (*ups)++;
            if (downs && p < before - 1e-9) (*downs)++;
        }
    };
    run(420, 12.0, nullptr, nullptr, nullptr);
    const double floorMs = MR::CPeriodMs();
    const double settled = s.r.period;
    // (a)
    const LONG mvA0 = s.r.moves; double mostA = 0.0;
    run(1, 31.0, &mostA, nullptr, nullptr); run(300, 12.0, &mostA, nullptr, nullptr);
    const LONG movesA = s.r.moves - mvA0;
    // (b)
    const LONG mvB0 = s.r.moves; double mostB = 0.0;
    run(8, 31.0, &mostB, nullptr, nullptr); run(300, 12.0, &mostB, nullptr, nullptr);
    const LONG movesB = s.r.moves - mvB0;
    // (c)
    double mostC = 0.0; LONG upsC = 0, downsC = 0, upsBack = 0; int riseAt = -1;
    for (int i = 0; i < 30; i++) { const double b = s.r.period; run(1, 31.0, &mostC, &upsC, nullptr); if (riseAt < 0 && s.r.period > b + 1e-9) riseAt = i + 1; }
    const LONGLONG tEnd = B::clockNow; double backS = -1.0; int backFrames = 0;
    for (int i = 0; i < 1400 && backS < 0.0; i++)
    {
        run(1, 12.0, &mostC, &upsBack, &downsC); backFrames++;
        if (s.r.period <= floorMs + 1e-9) backS = (double)(B::clockNow - tEnd) / (double)B::Qpf();
    }
    const RtStat all = RtRead(12, frame);
    const int order = gRt.order; const std::string first = gRt.firstBad;
    RtEnd("PL1");
    printf("  the floor %.3f ms, settled at %.3f ms; (a) one frame of 31 ms: the period at most %.3f ms behind it, %ld move(s); (b) eight in a row: at most %.3f ms, %ld move(s)\n",
        floorMs, settled, mostA, movesA, mostB, movesB);
    printf("  (c) thirty in a row: %ld rise(s), the first at the level's frame %d, to %.3f ms at most; then 12 ms: back on the floor %s (%.1f s, %d frames, %ld move(s) down, %ld up)\n",
        upsC, riseAt, mostC, backS >= 0.0 ? "yes" : "NO", backS, backFrames, downsC, upsBack);
    printf("  the run: %d pair(s), real frames shown once %d, generated frames shown %d, pictures out of place %d\n", all.n, all.real1, all.gen, order);
    if (fabs(settled - floorMs) > 1e-6) Err("PL1: the game of 12 ms settled at %.3f ms, not on the floor %.3f", settled, floorMs);
    if (mostA > floorMs + 1e-6 || movesA != 0) Err("PL1: ONE frame of 31 ms raised the period to %.3f ms (the floor %.3f) with %ld move(s): the law must call it a hitch and move nothing", mostA, floorMs, movesA);
    if (mostB > floorMs + 1e-6 || movesB != 0) Err("PL1: EIGHT frames of 31 ms in a row raised the period to %.3f ms with %ld move(s): one taken sample is not the level", mostB, movesB);
    if (upsC != 1 || mostC < 30.0 || mostC > 32.0 || riseAt < 9) Err("PL1: a level of 31 ms gave %ld rise(s), the first at its frame %d, to %.3f ms (one rise to 30 to 32 ms, behind the eight that open the door, wanted)", upsC, riseAt, mostC);
    if (backS < 0.0 || backS > 16.0 || upsBack != 0) Err("PL1: behind the level the period %s (%.1f s, %ld rise(s) on the way: the floor within 16 s and no rise wanted)", backS < 0.0 ? "never came back onto the floor" : "came back", backS, upsBack);
    if (order || all.real1 != all.n) Err("PL1: %d picture(s) out of place, %d of %d real frames shown exactly once (%s)", order, all.real1, all.n, first.c_str());
    CaseEnd(name);
}

// ================================================================ THE RIDE, AND THE CARD'S ONE QUEUE (2026-10-13)
// docs/reports/fg-4/brief-2026-10-13/DESIGN-the-ride.md; fg_machine.h, "THE REAL FRAME RIDES BEHIND THE NEXT FRAME'S UPSCALE".
// The cases above show a present at its call. These give the fake the card's queue ("THE CARD'S ONE QUEUE", above RtLook),
// with the shapes the sitting of 2026-10-13 read off its two logs, and judge THE SCREEN: when each picture was shown.
static const char* RtLineQ(const RtStat& r)
{
    static char bufs[8][900]; static int at = 0;
    char* const b = bufs[at++ & 7];
    snprintf(b, sizeof(bufs[0]), "%d pair(s): generated frames shown %d, real frames shown once %d; %.1f real frames a second (%.1f shown) at a period in force of %.3f ms (%.3f to %.3f);"
        " on the screen the generated frame to its real frame %.3f ms and on to the next generated frame %.3f ms, the real frame off the middle at worst %.3f ms (rms %.3f, %d pair(s));"
        " the picture's age at its real frame's showing, from its entry, at worst %.3f ms; real frames forced with no due %d;"
        " the room floor %ld us, the plan %ld us (raised %ld time(s)), the ride's floor %ld us (even %ld, wait %ld, fit %ld), its place %ld us behind the handoff's retire; hands that rode %ld, real frames fired at the ride %ld,"
        " generated frames withheld there %ld, ride pairs fired ahead of the body %ld, rides with no frame end %ld",
        r.n, r.gen, r.real1, r.fps, r.fps * 2.0, r.perMs, r.perMin, r.perMax, r.d1Ms, r.d2Ms, r.maxE, r.rmsE, r.evenN, r.ageMs, r.forced,
        r.roomUs, r.planUs, r.planUps, r.rideUs, r.rideEvenUs, r.rideWaitUs, r.rideFitUs, r.rideDUs, r.rideHands, r.rode, r.rideGenLost, r.rideAhead, r.rideNoEnd);
    return b;
}
// THE JUDGE OF A STRETCH ON THE SCREEN: every pair shown, each real frame once, the real frame within tolE of the middle of
// its two generated neighbours, and the period in force no longer than perMostMs (the panel's floor, or what the ride's
// own room asks at that shape: the case says which and why).
static std::string RtJudgeQ(const RtStat& r, double tolE, double perMostMs)
{
    std::string bad;
    if (r.gen != r.n) bad += Fmt("%d of %d generated frames shown; ", r.gen, r.n);
    if (r.real1 != r.n) bad += Fmt("%d of %d real frames shown exactly once; ", r.real1, r.n);
    if (r.evenN < r.n - 1) bad += Fmt("only %d of %d pairs could be read against their neighbours; ", r.evenN, r.n);
    if (r.maxE > tolE) bad += Fmt("a real frame %.3f ms off the middle of its neighbours (%.3f allowed); ", r.maxE, tolE);
    if (r.perMax > perMostMs + 0.001) bad += Fmt("the period in force reached %.3f ms where %.3f is the most this shape asks; ", r.perMax, perMostMs);
    return bad;
}
struct FqLeg { RtStat set; std::string bad; int order; std::string first; };
static FqLeg FqRun(const RtPar& par, double costMs, double tolE, double perMostMs, int frames = 760, int from = 520)
{
    FqLeg g = {};
    RtBegin(par);
    for (int i = 0; i < frames; i++) RtFrame(RtMs(costMs));
    g.set = RtRead(from, frames);
    g.bad = RtJudgeQ(g.set, tolE, perMostMs);
    g.order = gRt.order; g.first = gRt.firstBad;
    RtEnd("FQ");
    return g;
}
// THE TWO LOCKS' SHAPES, as the sitting's logs gave them (the design's point 1; each number a median of a log line or a
// difference of two, so half a millisecond of doubt stands in each): the injection 1.0 ms ahead of the entry (a game
// that costs 4 ms, its injection at 0.75), and behind the injection
//   the lock at 90:  a scene tail of 2.0 ms, an upscale of 2.45 ms, a post chain and the hand's copies of 4.3 ms (the
//                    handoff retires 7.75 ms behind the entry), a plan of 2.75 ms
//   the lock at 70:  2.4 ms, 3.3 ms, 5.3 ms (10.0 ms behind the entry), a plan of 3.3 ms
static RtPar FqLock90() { RtPar p; p.inj = 0.75; p.tailUs = 2000; p.upUs = 2450; p.postUs = 4300; p.planUs = 2750; p.lsUs = 7750; return p; }
static RtPar FqLock70() { RtPar p; p.inj = 0.75; p.tailUs = 2400; p.upUs = 3300; p.postUs = 5300; p.planUs = 3300; p.lsUs = 10000; return p; }

// ---------------------------------------------------------------- FQ1, the lock at 90
// WHAT IT SHOWS. The 120 Hz panel (the floor 16.92 ms, 59.1 real frames), the card as the lock at 90 had it. THE MACHINE
// UNTIL 2026-10-13 (the control: the ride's guard off, nothing else) paces this game at its room floor, twice (the handoff's
// 7.75 ms and the plan), 21.25 ms: 47 real frames a second with the card idle, and its real frame is forced ahead of the
// next injection on every pair. WITH THE RIDE the period in force is the panel's floor, every pair is shown, each real
// frame once and in order, and ON THE SCREEN the real frame stands in the middle of its two generated neighbours: it is
// shown when the next frame's upscale answers, and its generated frame was planted half a period ahead of that.
static void CaseFQ1()
{
    const char* const name = "FQ1 the ride at the lock at 90's shape: the card's queue, the panel's floor held, the real frame on the screen's middle (2026-10-13)";
    CaseBegin(name);
    const RtPar par = FqLock90();
    const FqLeg g = FqRun(par, 4.0, 0.10, 16.93);
    printf("  WITH THE RIDE: %s; pictures out of place %d\n", RtLineQ(g.set), g.order);
    if (!g.bad.empty()) Err("FQ1: %s", g.bad.c_str());
    if (g.order) Err("FQ1: %d picture(s) out of place; the first: %s", g.order, g.first.c_str());
    if (g.set.fps < 58.9 || g.set.fps > 59.3) Err("FQ1: %.2f real frames a second (59.1, the panel's floor, wanted)", g.set.fps);
    if (g.set.rode <= 0 || g.set.rideUs <= 0 || g.set.rideUs > 16916 || g.set.rideNoEnd) Err("FQ1: real frames fired at the ride %ld, the ride's floor %ld us, rides with no frame end %ld (some, a floor under the panel's 16,916, and 0 wanted)", g.set.rode, g.set.rideUs, g.set.rideNoEnd);
    const RingStats stc = gRingSt;
    RtPar pc = par; pc.guardOff = 64;
    const FqLeg c = FqRun(pc, 4.0, 0.10, 16.93);
    ViolRestore(stc);
    printf("  CONTROL, the machine until this date (no pair rides): %s\n", RtLineQ(c.set));
    printf("  THE RATE AT THE LOCK AT 90's SHAPE: %.1f real frames a second before, %.1f with the ride (the game alone at that lock gave 94.5; the panel's floor is 59.1)\n", c.set.fps, g.set.fps);
    if (c.bad.empty() || c.set.fps > 50.0 || c.set.rode != 0) Err("FQ1 control: with no pair riding the game ran at %.1f real frames a second, %ld real frames were fired at the ride and the judge said \"%s\" (under 50, 0 and a fault wanted): the control shows nothing", c.set.fps, c.set.rode, c.bad.c_str());
    Ctl("no pair rides: the pacer at the room floor, %.2f ms, %.1f real frames a second, the real frame %.2f ms off the screen's middle at worst and forced with no due %d time(s) in %d", c.set.perMs, c.set.fps, c.set.maxE, c.set.forced, c.set.n);
    CaseEnd(name);
}
// ---------------------------------------------------------------- FQ2, the lock at 70
// WHAT IT SHOWS. The same panel, the card as the lock at 70 had it. THE MACHINE UNTIL THIS DATE: the room floor is 26.75 ms,
// which FgPace's one sleep of 20 ms cannot hold with a game of 4 ms, so the frames come every 24 ms while each hand
// carries 26.75 (the sitting's own fault at that lock), 41.7 real frames a second. WITH THE RIDE: the ride's own floor
// binds, twice (the post chain's 5.3 ms and the plan), 17.25 to 17.5 ms: 57 to 58 real frames a second, every pair shown,
// the real frame on the screen's middle.
static void CaseFQ2()
{
    const char* const name = "FQ2 the ride at the lock at 70's shape: the ride's own floor binds, the real frame on the screen's middle (2026-10-13)";
    CaseBegin(name);
    const RtPar par = FqLock70();
    const FqLeg g = FqRun(par, 4.0, 0.10, 17.51);
    printf("  WITH THE RIDE: %s; pictures out of place %d\n", RtLineQ(g.set), g.order);
    if (!g.bad.empty()) Err("FQ2: %s", g.bad.c_str());
    if (g.order) Err("FQ2: %d picture(s) out of place; the first: %s", g.order, g.first.c_str());
    if (g.set.fps < 57.0 || g.set.perMin < 17.2 || g.set.rideUs < 17250 || g.set.rideUs > 17500) Err("FQ2: %.2f real frames a second at a period of %.3f ms, the ride's floor %ld us (57 or more, 17.25 to 17.5 ms: twice the post chain and the plan, wanted)", g.set.fps, g.set.perMin, g.set.rideUs);
    const RingStats stc = gRingSt;
    RtPar pc = par; pc.guardOff = 64;
    const FqLeg c = FqRun(pc, 4.0, 0.10, 17.51);
    ViolRestore(stc);
    printf("  CONTROL, the machine until this date (no pair rides): %s\n", RtLineQ(c.set));
    printf("  THE RATE AT THE LOCK AT 70's SHAPE: %.1f real frames a second before, %.1f with the ride (the game alone at that lock gave 74.3)\n", c.set.fps, g.set.fps);
    if (c.bad.empty() || c.set.fps > 45.0 || c.set.rode != 0) Err("FQ2 control: with no pair riding the game ran at %.1f real frames a second and the judge said \"%s\" (under 45 and a fault wanted): the control shows nothing", c.set.fps, c.bad.c_str());
    Ctl("no pair rides: the hands carry the room floor, %.2f ms, the frames come at %.1f a second (one sleep of 20 ms and the game's 4), real frames forced with no due %d in %d", c.set.perMs, c.set.fps, c.set.forced, c.set.n);
    CaseEnd(name);
}
// ---------------------------------------------------------------- FQ3, a 240 Hz panel at its ceiling
// WHAT IT SHOWS (FB4's question, with the queue). A 240 Hz panel (the floor 8.46 ms, 118.2 real frames), a game of 3 ms on a
// card that is free: a scene tail of 0.5 ms, an upscale of 1.27 ms (the sitting's "helper 1.27 ms"), a post chain of
// 2.33 ms (the handoff retires 3.35 ms behind the entry, FB4's number), a plan of 1.5 ms. THE SHAPE'S THREE PARTS ARE AN
// ASSUMPTION: no leg has run at that ceiling. The room floor is 9.75 ms there (FB4: "the pair does not quite fit");
// with the ride the pair is shown at the panel's floor, even on the screen TO THE PACER'S SPIN: the generated frame's due
// falls 0.2 ms ahead of the end of FgPace's sleep in this shape, inside the last 0.6 ms the pacer spins on the clock and
// hears no wake, so it is served by the next frame's first call, 0.21 ms late on every pair (FB2's own note on that
// spin; counted in no line of the mod's). The judge allows 0.35 ms, as FB2 does.
static void CaseFQ3()
{
    const char* const name = "FQ3 the ride at a 240 Hz panel's ceiling: 118 real frames with the hand of the owner's 4K (2026-10-13)";
    CaseBegin(name);
    RtPar par; par.refresh = (240ull << 32) | 1ull; par.inj = 0.75; par.tailUs = 500; par.upUs = 1270; par.postUs = 2330; par.planUs = 1500;
    const FqLeg g = FqRun(par, 3.0, 0.35, 8.47);
    printf("  WITH THE RIDE: %s; pictures out of place %d\n", RtLineQ(g.set), g.order);
    if (!g.bad.empty()) Err("FQ3: %s", g.bad.c_str());
    if (g.order) Err("FQ3: %d picture(s) out of place; the first: %s", g.order, g.first.c_str());
    if (g.set.fps < 117.5 || g.set.rode <= 0) Err("FQ3: %.2f real frames a second, %ld real frames fired at the ride (118.2 and some wanted)", g.set.fps, g.set.rode);
    const RingStats stc = gRingSt;
    RtPar pc = par; pc.guardOff = 64;
    const FqLeg c = FqRun(pc, 3.0, 0.35, 8.47);
    ViolRestore(stc);
    printf("  CONTROL, no pair rides: %s\n", RtLineQ(c.set));
    if (c.bad.empty() || c.set.fps > 105.0) Err("FQ3 control: with no pair riding the game ran at %.1f real frames a second and the judge said \"%s\" (under 105 and a fault wanted): the control shows nothing", c.set.fps, c.bad.c_str());
    Ctl("no pair rides: the pacer at the room floor, %.2f ms, %.1f real frames a second", c.set.perMs, c.set.fps);
    CaseEnd(name);
}
// ---------------------------------------------------------------- FQ4, what can meet a ride that is pending
// WHAT IT SHOWS. The lock at 90's shape, settled and riding, then, each with 40 frames between: A FRAME THAT COMES 9 ms LATE
// (the real frame is fired by its own due, at its place, and not held for the late frame); A FRAME WHOSE INJECTION IS NOT
// SENT (its body sends nothing; the pending real frame is fired behind it all the same; the frame itself goes out
// inline); TWENTY FRAMES OF STANDBY (the want at PASS) and the engage behind them; A SPREAD OF 15 PERCENT in the game's
// cost for 120 frames. HELD FOR THE WHOLE RUN: every real frame on the screen exactly once and in the game's order, and
// no generated frame behind its own real frame.
static void CaseFQ4()
{
    const char* const name = "FQ4 a ride that is pending meets a late frame, an unsent injection, a standby and a wandering cost: every real frame once and in order (2026-10-13)";
    CaseBegin(name);
    RtBegin(FqLock90());
    FgState& s = B::S(); const FgArmedState& a = ShArmB::a;
    for (int i = 0; i < 520; i++) RtFrame(RtMs(4.0));
    const LONG rode0 = a.w.rode, clockN0 = s.c.nOnTime;
    RtFrame(RtMs(13.0)); for (int i = 0; i < 40; i++) RtFrame(RtMs(4.0));
    const LONG rodeLate = a.w.rode - rode0;
    RtFrame(RtMs(4.0), false); for (int i = 0; i < 40; i++) RtFrame(RtMs(4.0));
    const LONG inl = a.w.inl[FG_AI_UNSENT];
    s.wt.want = FG_WANT_PASS; for (int i = 0; i < 20; i++) RtFrame(RtMs(4.0));
    s.wt.want = FG_WANT_ON;   for (int i = 0; i < 200; i++) RtFrame(RtMs(4.0));
    gRtSeed = 20261013u;
    for (int i = 0; i < 120; i++) RtFrame(RtMs(4.0 * RtSpread(0.15)));
    const int n = (int)gRt.pairs.size();
    int once = 0, twice = 0, none = 0, genTwice = 0;
    for (int i = 0; i < n - 1; i++) { const RtPair& q = gRt.pairs[i]; if (q.real == 1) once++; else if (q.real > 1) twice++; else none++; if (q.gen > 1) genTwice++; }
    const int order = gRt.order; const std::string first = gRt.firstBad;
    const RtStat tail = RtRead(n - 100, n);
    const LONG rode = a.w.rode - rode0, dropped = s.c.nDropped;
    RtEnd("FQ4");
    printf("  %d frames: real frames on the screen once %d, twice or more %d, never %d; generated frames shown twice %d; pictures out of place %d; real frames dropped %ld;"
           " real frames fired at the ride from the first event on %ld (%ld of them in the 41 frames with the late one); pairs inline for an unsent injection %ld;"
           " the last hundred pairs, under the spread: %s\n", n - 1, once, twice, none, genTwice, order, dropped, rode, rodeLate, inl, RtLineQ(tail));
    (void)clockN0;
    if (twice || none || genTwice || order || dropped) Err("FQ4: real frames shown twice %d, never %d, generated frames twice %d, pictures out of place %d (%s), real frames dropped %ld (all 0 wanted)", twice, none, genTwice, order, first.c_str(), dropped);
    if (rode <= 0 || inl < 1) Err("FQ4: real frames fired at the ride %ld, pairs inline for the unsent injection %ld (some and one wanted): the events did not meet a ride", rode, inl);
    CaseEnd(name);
}
// ---------------------------------------------------------------- FQ5, the ride's place in the worker's order, the worker held
// WHAT IT SHOWS, with the worker HELD INSIDE THE NEXT INJECTION'S BODY and that frame's hand and Present queued behind it (no
// simulation: the frames are made by hand, the body's stand-in holds on an event). Pair A is armed and rides (the room
// floor handed in over any period). WHILE THE WORKER STANDS IN THE BODY A's real frame is NOT on the screen: it was not
// forced ahead of the body. RELEASED, it is there ONCE, with A's own picture (the one HOLD: the next hand's span, queued
// behind, had not run), ahead of everything of frame B, and B's real frame follows once at its own fire.
// CONTROL (a run): the ride's guard off: A's real frame is on the screen WHILE the worker stands in the body, forced
// ahead of it, as every pair's was until this date.
// AND THE SAVE'S QUESTION AT THE RIDE, ANSWERED BY THE PRODUCT'S OWN TRACKER (source/devthread/dt_bbs.h, through the two calls the
// product's binding makes: ShArmB::backMode 3, dt_fg_saves.h's setting, the back buffer bound as the game leaves it), two
// more runs of the same leg:
//   NOTHING OF FRAME B IN THE BACK BUFFER YET (the game draws its frame there behind the injection, as this harness's Fill
//     does): the ride's fire is answered "not written" and makes NO save: no copy more than a real frame's fire ever made;
//   FRAME B HAS BEGUN TO DRAW INTO THE BACK BUFFER AHEAD OF ITS INJECTION (a ColorFill planted there): the ride's fire is
//     answered "written", SAVES the back buffer ahead of its own write and puts it back behind its Present. The fake
//     counts a game's picture lost under a fire's write: 0; and the tracker is never found saying "not written" where
//     the fake knows a write: 0.
static HANDLE gFqEv = nullptr;
static volatile LONG gFqIn = 0, gFqHold = 0;
static void FqHoldBody(const DWORD* payload, DWORD bytes)
{
    if (gFqHold) { _InterlockedExchange(&gFqHold, 0); _InterlockedExchange(&gFqIn, 1); WaitForSingleObject(gFqEv, 5000); _InterlockedExchange(&gFqIn, 0); }
    ShInjBody(payload, bytes);
}
struct Fq5Leg { bool armed, rides, held; int inBody, after, bCount, aBeforeB; LONG rode, genLost, forcedC, saves, noSaves, under, lost; };
static Fq5Leg Fq5Run(LONG guardOff, int tracker = 0)
{
    Fq5Leg g = {};
    if (!gFqEv) gFqEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    MgFresh<MA>();
    if (tracker) { ShArmB::backMode = 3; D->SetRenderTarget(0, gBb); Drain(); }   // (SvFresh's lines: the product's tracker, the back buffer bound)
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;   // (a room floor over any period: every hand of the want ON rides)
    gArmedGuardOff = guardOff;
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>(true, false);
    if (!ShSettle()) Err("FQ5: the fake helper did not read pair A's messages");
    g.armed = s.w.openPhase == FG_PAIR_ARMED; g.rides = (s.w.openFlags & FG_HF_RIDE) != 0;
    const size_t shown0 = MgShownN();
    const LONG rode0 = a.w.rode, gl0 = a.w.rideGenLost, early0 = s.c.nEarly;
    const LONG saves0 = a.w.saves, no0 = a.w.noSaves, under0 = ShArmB::backUnder, lost0 = gRingSt.backLost;
    gInjBody = FqHoldBody; _InterlockedExchange(&gFqHold, 1);
    ClockAdd(16.7);
    if (tracker == 2) D->ColorFill(gBb, nullptr, 0xFF445566u);   // FRAME B HAS BEGUN TO DRAW INTO THE BACK BUFFER, ahead of its injection
    ShInject(true); DtPublishFenced();
    g.held = WaitFor([] { return gFqIn != 0; }, 3000);
    g.inBody = MgTagCount(shown0, tagA);                      // THE WORKER HELD IN THE BODY: is A's real frame on the screen?
    Later l = { [] { return true; }, [] { SetEvent(gFqEv); }, 1000, 60 };
    HANDLE h = Spawn(LaterMain, &l);
    const DWORD tagB = Frame<MA>();                           // frame B's hand and Present, QUEUED BEHIND THE HELD WORKER
    Join(h, "FQ5's release");
    Drain();
    g.saves = a.w.saves - saves0; g.noSaves = a.w.noSaves - no0; g.under = ShArmB::backUnder - under0; g.lost = gRingSt.backLost - lost0;
    gInjBody = ShInjBody;
    if (!ShSettle()) Err("FQ5: the fake helper did not read frame B's messages");
    g.after = MgTagCount(shown0, tagA);
    MgDueN();
    const auto sh = Shown();
    int iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++) { if (sh[i].tag == tagA && iA < 0) iA = (int)i; if (sh[i].tag == tagB) { g.bCount++; if (iB < 0) iB = (int)i; } }
    g.aBeforeB = (iA >= 0 && (iB < 0 || iA < iB)) ? 1 : 0;
    g.rode = a.w.rode - rode0; g.genLost = a.w.rideGenLost - gl0; g.forcedC = s.c.nEarly - early0;
    gArmedGuardOff = 0;
    MgClean("FQ5");
    MgEnd();
    ShArmB::backMode = 0;
    return g;
}
static void CaseFQ5()
{
    const char* const name = "FQ5 the ride in the worker's order: held in the next body, the real frame not yet shown; released, once, its own picture, ahead of the next frame (2026-10-13)";
    CaseBegin(name);
    const Fq5Leg g = Fq5Run(0);
    printf("  pair A armed %s and rides %s; the worker held in the next injection's body %s, frame B's hand and Present queued behind it: A's real frame on the screen %d time(s) while held, %d after the release (ahead of B's: %s);"
           " fired at the ride %ld, its generated frame withheld there %ld, forced ahead of the body %ld; B's real frame shown %d time(s)\n",
        g.armed ? "yes" : "NO", g.rides ? "yes" : "NO", g.held ? "yes" : "NO", g.inBody, g.after, g.aBeforeB ? "yes" : "NO", g.rode, g.genLost, g.forcedC, g.bCount);
    if (!g.armed || !g.rides || !g.held) Err("FQ5: the leg did not reach its hold (pair A armed %d, rides %d, the worker held in the body %d)", (int)g.armed, (int)g.rides, (int)g.held);
    if (g.inBody != 0 || g.after != 1 || !g.aBeforeB || g.rode != 1 || g.forcedC != 0) Err("FQ5: A's real frame was on the screen %d time(s) with the worker held in the body and %d time(s) after (0 and 1 wanted), ahead of B's %d, fired at the ride %ld, forced ahead of the body %ld (1, 1 and 0 wanted)", g.inBody, g.after, g.aBeforeB, g.rode, g.forcedC);
    if (g.bCount != 1) Err("FQ5: frame B's real frame was on the screen %d time(s) (1 wanted)", g.bCount);
    const Fq5Leg t = Fq5Run(0, 1), w = Fq5Run(0, 2);
    printf("  THE SAVE'S QUESTION AT THE RIDE, by the product's tracker: nothing of frame B in the back buffer: fires that saved %ld, fires that made no save %ld; the tracker said \"not written\" where the fake knows a write %ld time(s), pictures of the game's lost %ld; A's real frame shown %d time(s), fired at the ride %ld\n"
           "  frame B's first write planted ahead of its injection: fires that saved %ld, made no save %ld; \"not written\" where the fake knows a write %ld, pictures lost %ld; A's real frame shown %d time(s), fired at the ride %ld\n",
        t.saves, t.noSaves, t.under, t.lost, t.after, t.rode, w.saves, w.noSaves, w.under, w.lost, w.after, w.rode);
    if (t.inBody != 0 || t.after != 1 || t.rode != 1 || t.saves != 0 || t.noSaves != 1 || t.under || t.lost) Err("FQ5, the tracker, a clean back buffer: the ride's fire saved %ld time(s) and made no save %ld time(s), the tracker under %ld, pictures lost %ld, A's real frame shown %d time(s), fired at the ride %ld (0, 1, 0, 0, 1 and 1 wanted)", t.saves, t.noSaves, t.under, t.lost, t.after, t.rode);
    if (w.inBody != 0 || w.after != 1 || w.rode != 1 || w.saves != 1 || w.noSaves != 0 || w.under || w.lost) Err("FQ5, the tracker, frame B begun in the back buffer: the ride's fire saved %ld time(s) and made no save %ld time(s), the tracker under %ld, pictures lost %ld, A's real frame shown %d time(s), fired at the ride %ld (1, 0, 0, 0, 1 and 1 wanted)", w.saves, w.noSaves, w.under, w.lost, w.after, w.rode);
    const RingStats stc = gRingSt;
    const Fq5Leg c = Fq5Run(64);
    ViolRestore(stc);
    printf("  CONTROL, the ride's guard off: A's real frame on the screen %d time(s) with the worker held in the body, %d after; fired at the ride %ld, forced ahead of the body %ld\n", c.inBody, c.after, c.rode, c.forcedC);
    if (c.inBody != 1 || c.after != 1 || c.rode != 0 || c.forcedC != 1) Err("FQ5 control: with the guard off A's real frame was on the screen %d time(s) while the worker stood in the body and %d after, fired at the ride %ld, forced ahead %ld (1, 1, 0 and 1 wanted): the control shows nothing", c.inBody, c.after, c.rode, c.forcedC);
    Ctl("the ride's guard off: the real frame is forced ahead of the next injection's body (on the screen %d time(s) while the worker stands in it)", c.inBody);
    CaseEnd(name);
}
// ================================================================ THE RIDE HOLDS (the evening of 2026-10-13)
// docs/reports/fg-4/sitting-2026-10-13b (the owner's leg t90.2 on the ride); fg_machine.h, "THE RIDE'S NEEDS ARE MEASURED, NOT
// PLANNED". THE ANSWER'S OWN TIME: with RtPar::kUs the helper's answer comes that long behind the handoff's retire, on the
// harness's clock (its own delayUs), so K has a value, a case can plant slow answers (RtSim::kNext), and the product's
// step of the plan can run on K's histogram as the machine fills it.
// THE CASE'S WINDOW (RtWindow), once every 120 frames, AS THE PRODUCT'S LATENCY WAITER RUNS ITS OWN (fg_arm_life.h,
// FgpArmWindowStep): THE PLAN'S STEP, FgpPlanStep's lines with FgpArmKeys' numbers over the pure functions of fg_plan.h (the
// 990 quantile of the last two windows to go up, four to come down, 1,000 to 8,000 us); and THE CONDITIONS' ONE RULE THAT
// MOVED ON THAT LEG, fg_cond.h's FGC_SLOW with its own numbers: the window's real rate, by the median interval of its
// frames, under 45 standing by or under 42 engaged: the want is PASS for the next window, else ON.
struct FqWin { double fps; LONG want, rode, planUs, rideUs, roomUs, lateN; };
static FqWin RtWindow(int first, int last)
{
    RtSim& g = gRt; FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    FqWin w = {};
    // the window's real rate: the median interval between its frames' entries
    std::vector<LONGLONG> iv;
    for (int i = first + 1; i < last && i < (int)g.pairs.size(); i++) iv.push_back(g.pairs[i].tEntry - g.pairs[i - 1].tEntry);
    if (!iv.empty()) { std::nth_element(iv.begin(), iv.begin() + iv.size() / 2, iv.end()); const double ms = (double)iv[iv.size() / 2] * 1000.0 / (double)B::Qpf(); w.fps = ms > 0 ? 1000.0 / ms : 0; }
    w.want = s.wt.want; w.rode = a.w.rode; w.lateN = a.pl.iNot[FG_AW_LATE];
    if (g.par.planStep)
    {   // FgpPlanStep's lines
        const FgShadowPlacer& sp = ShB::x.pl;
        LONG n = g.planK.windows; if (n < 1) n = 1; if (n > FG_PLAN_WINDOWS) n = FG_PLAN_WINDOWS;
        LONG nd = g.planK.downWindows; if (nd < n) nd = n; if (nd > FG_PLAN_WINDOWS) nd = FG_PLAN_WINDOWS;
        LONG* win = g.planWin[g.planAt % FG_PLAN_WINDOWS];
        for (int i = 0; i < FG_K_BINS; i++) { const LONG now = sp.kHist[i]; win[i] = now - g.planSeen[i]; g.planSeen[i] = now; }
        g.planAt++;
        LONG up[FG_K_BINS], down[FG_K_BINS];
        for (int i = 0; i < FG_K_BINS; i++)
        {
            LONG su = 0, sd = 0;
            for (LONG back = 0; back < nd; back++) { const LONG c = g.planWin[(g.planAt - 1 - back) & (FG_PLAN_WINDOWS - 1)][i]; sd += c; if (back < n) su += c; }
            up[i] = su; down[i] = sd;
        }
        const LONG cur = a.k.kPlanUs;
        const LONG next = FgPlanStep(g.planMem, up, down, FG_K_BINS, FG_K_BIN_US, g.planK, cur);
        if (next != cur) a.k.kPlanUs = next;
    }
    // FGC_SLOW (fg_cond.h: minRealFpsX10 450, rateHystX10 30)
    const bool engaged = s.wt.want != FG_WANT_PASS;
    const int fpsX10 = (int)(w.fps * 10.0);
    const bool slow = fpsX10 > 0 && fpsX10 < 450 - (engaged ? 30 : 0);
    s.wt.want = slow ? FG_WANT_PASS : FG_WANT_ON;
    w.planUs = a.k.kPlanUs; w.rideUs = a.pl.rideUs; w.roomUs = a.pl.roomUs;
    return w;
}
// ---------------------------------------------------------------- FQ6, the ride through slow answers, the conditions in the loop
// WHAT IT SHOWS. The lock at 90's shape with a game of 5 ms (the leg's own: 20 ms and the game gave its 26), the answers
// taking the plan's 2.75 ms (the plan stands at 3,000 us, the upper edge of their bin), the product's plan step and the
// conditions' rate rule run once a window of 120 frames. Eighteen windows: four to settle; IN THE FIFTH THREE ANSWERS TAKE
// 8.5 ms (the leg's window 18: "late 3"); IN THE ELEVENTH EIGHT TAKE 6 ms (its window 51: "late 7").
// THE MACHINE OF THE MORNING OF 2026-10-13 (the control: rideKMaxUs 0, nothing else): the three slow answers are the plan's
// 99th of 240, the plan goes to 8,000 us, the ride's floor with it from 14.75 to 24.75 ms; the game runs at 40.4, under
// the 42 an engaged session needs; stand-by; engage; and every engaged pair from there is a warm-up pair, so the floor
// stands where the plan of 8,000 put it while the plan itself comes back: ENGAGED AND STANDING BY BY TURNS TO THE END, each
// engaged window at 40.4.
// THE MACHINE NOW: the plan goes to 8,000 us all the same (its law is not touched), AND COUNTS IN THE RIDE'S FLOORS FOR THE CAP,
// 4,500 us: the floor goes to twice (the post chain's 4.3 ms and 4.5), 17.75 ms, 56.3 real frames a second, AND COMES BACK
// WITH THE PLAN, five windows on (the plan's own hold); the same behind the eight. NO WINDOW UNDER 55, NO STAND-BY, every
// real frame once and in order; the eleven slow answers' generated frames are withheld, and a few beside them while a
// ride pair's deadline is the cap and not the plan.
struct Fq6Leg { std::vector<FqWin> w; int standbys, order; double least, last; LONG withheld; int n, gen, real1; std::string first; };
static Fq6Leg Fq6Run(LONG rideKMaxUs)
{
    Fq6Leg g = {};
    RtPar par = FqLock90(); par.inj = 0.8; par.kUs = par.planUs; par.planStep = 1; par.rideKMaxUs = rideKMaxUs;
    RtBegin(par);
    FgState& s = B::S(); const FgArmedState& a = ShArmB::a;
    g.least = 1e9;
    const LONG late0 = a.pl.iNot[FG_AW_LATE];
    for (int win = 0; win < 18; win++)
    {
        const int first = (int)gRt.pairs.size();
        const bool engaged = s.wt.want != FG_WANT_PASS;
        for (int i = 0; i < 120; i++)
        {
            if (win == 4 && (i == 20 || i == 50 || i == 80)) gRt.kNext = 8500;
            if (win == 10 && i >= 30 && i < 110 && (i % 10) == 0) gRt.kNext = 6000;
            RtFrame(RtMs(5.0));
        }
        const FqWin w = RtWindow(first, (int)gRt.pairs.size());
        g.w.push_back(w);
        if (!engaged) g.standbys++;
        else if (win >= 2) { if (w.fps < g.least) g.least = w.fps; g.last = w.fps; }
    }
    g.withheld = a.pl.iNot[FG_AW_LATE] - late0;
    const int n = (int)gRt.pairs.size();
    for (int i = 0; i < n - 1; i++) { const RtPair& q = gRt.pairs[i]; g.n++; if (q.gen == 1) g.gen++; if (q.real == 1) g.real1++; }
    g.order = gRt.order; g.first = gRt.firstBad;
    s.wt.want = FG_WANT_ON;
    RtEnd("FQ6");
    return g;
}
static void Fq6Print(const char* what, const Fq6Leg& g)
{
    printf("  %s\n  window   engaged   real frames a second   the plan   the ride's floor   the room floor   real frames fired at the ride (so far)   late answers (so far)\n", what);
    for (size_t i = 0; i < g.w.size(); i++)
        printf("  %4d     %-8s  %6.1f                 %5ld us   %6ld us          %6ld us        %6ld                                    %ld\n",
            (int)i, g.w[i].want != FG_WANT_PASS ? "yes" : "STOOD BY", g.w[i].fps, g.w[i].planUs, g.w[i].rideUs, g.w[i].roomUs, g.w[i].rode, g.w[i].lateN);
    printf("  in all: %d frames, generated frames shown %d, real frames shown once %d, pictures out of place %d; windows stood by %d; the engaged windows from the third on ran at %.1f real frames a second at the least, the last at %.1f; answers late %ld\n",
        g.n, g.gen, g.real1, g.order, g.standbys, g.least, g.last, g.withheld);
}
static void CaseFQ6()
{
    const char* const name = "FQ6 the ride holds through slow answers, the plan's step and the conditions' rate rule in the loop: 56 real frames while the plan stands high, back with it, no stand-by (2026-10-13, evening)";
    CaseBegin(name);
    const Fq6Leg g = Fq6Run(4500);
    Fq6Print("THE MACHINE NOW (the plan counts in the ride's floors for at most 4,500 us; the needs are stamps' differences; a need is taken behind an engage too):", g);
    if (g.order || g.real1 != g.n) Err("FQ6: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", g.order, g.first.c_str(), g.real1, g.n);
    if (g.standbys != 0 || g.least < 55.0 || g.last < 58.9) Err("FQ6: %d window(s) stood by, the engaged windows ran at %.1f real frames a second at the least and the last at %.1f (0, 55 or more, and the panel's 59.1 wanted)", g.standbys, g.least, g.last);
    if (g.w.size() == 18)
    {
        // (window 4's three slow answers: the plan at 8,000 us from that window's step, the floor at twice (4.3 and the cap) in
        // window 5, and back under the panel's floor once the plan is, by window 10; window 10's eight: the same, back by 17)
        if (g.w[4].planUs != 8000 || g.w[5].rideUs < 17500 || g.w[5].rideUs > 18000 || g.w[5].fps < 55.0) Err("FQ6: behind three slow answers the plan is %ld us, the ride's floor %ld us and the game ran at %.1f (8,000, 17,750: twice the post chain and the cap, and 56.3 wanted)", g.w[4].planUs, g.w[5].rideUs, g.w[5].fps);
        // (window 10's own figure is its rate: the floor is back for most of it, and the eight slow answers planted in it raise
        // the plan again before its end)
        if (g.w[9].planUs > 4500 || g.w[10].fps < 58.9) Err("FQ6: five windows behind the three slow answers the plan is %ld us, and the window after ran at %.1f (a plan under the cap again and 59.1 wanted): the floor did not come back with the plan", g.w[9].planUs, g.w[10].fps);
        if (g.w[17].rideUs > 16916 || g.w[17].fps < 58.9) Err("FQ6: seven windows behind the eight slow answers the ride's floor is %ld us and the game ran at %.1f (a floor under 16,916 us and 59.1 wanted): the floor did not come back with the plan", g.w[17].rideUs, g.w[17].fps);
    }
    if (g.withheld < 11 || g.withheld > 40) Err("FQ6: %ld answers were late (the eleven planted, and no more than a few beside them, wanted)", g.withheld);
    const RingStats stc = gRingSt;
    const Fq6Leg c = Fq6Run(0);
    ViolRestore(stc);
    Fq6Print("CONTROL, the machine of the morning of 2026-10-13 (the whole plan in the ride's floors, no need taken from a warm-up pair, a floor worked out only where one was):", c);
    printf("  THE RATE THROUGH THE PLANTED SLOW ANSWERS: before, %d of 18 windows stood by and the engaged ones ran at %.1f real frames a second at the least (the last at %.1f); now none stood by, %.1f at the least, the last at %.1f\n",
        c.standbys, c.least, c.last, g.least, g.last);
    if (c.standbys < 5 || c.least > 42.0 || c.last > 42.0) Err("FQ6 control: with the morning's floors %d window(s) stood by and the engaged windows ran at %.1f at the least, the last at %.1f (five or more stand-bys and under 42 wanted): the control shows nothing", c.standbys, c.least, c.last);
    Ctl("the morning's floors: three slow answers put the plan at 8,000 us and the ride's floor at %ld us; %d of 18 windows stood by, the engaged ones at %.1f real frames a second, the floor frozen to the end (%ld us in the last window)",
        c.w.size() > 5 ? c.w[5].rideUs : 0, c.standbys, c.least, c.w.empty() ? 0 : c.w.back().rideUs);
    CaseEnd(name);
}
