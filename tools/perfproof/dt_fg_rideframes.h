// dt_fg_rideframes.h: EVERY GENERATED FRAME SHOWN ON A BUSY CARD, IN THE HARNESS (2026-10-14, afternoon; docs/reports/fg-4/brief-2026-10-14/
// BRIEF-ride-frames.md; fg_machine.h, "EVERY GENERATED FRAME SHOWN ON A BUSY CARD", above ArmedEarly, and ArmedRoomFromArm).
//
// Included by dt_fg_test.cpp behind dt_fg_late.h, with ONE line; its cases are RF1 to RF5 (one RUN line in RunCases, ONCE, as LA's).
//
// WHAT THE SITTING OF 2026-10-14 READ (sitting-2026-10-14, the legs t70.2 and t70.4 at the lock at 300 MHz): about 10 of every 100
// generated frames never reached the screen, in two places:
//   AFTER THE ENGAGE, about 8 in 120 evenly: the period stood at the ride's WAIT floor in every riding window, half a period about
//     half a millisecond over the sixth largest wait of the ride behind the next injection's body, and every pair whose wait
//     came within that of half a period met the next injection with its generated frame still due, its answer in;
//   THE ENGAGE'S FIRST TWO WINDOWS: the room floor at 0, no pair rode, and every pair was timed from the arm (its handoff 10 to 12
//     ms behind the entry against a half of 8.46), late by rule and teaching nothing.
// THE SHAPE OF THAT LOCK (RfLock300), from the legs' medians: the game first on the card (the legs' reading), a steady dispatch of
// 2,700 us (the helper's line: 2,710), the plan 3,250, the post chain and the hand's copies 4,000 us, the next frame's upscale
// 3,300 us, the scene's tail at the injection 4,500 us, varying frame by frame by 29 percent each way, and the game's own cost by
// 25 percent, which moves its injection. With them the ride's place stands 15 ms behind the handoff's retire, the ride's
// fire about 7 ms, the wait floor binds, and its margin over the sixth largest wait is the ride's guard (the legs: 15.0 ms,
// 6 to 7 ms, the wait floor in 53 of 53 windows, 0.4 to 0.75 ms). THE SCENE BEHIND THE ORDER WAIT is 2,900 us, so the card's own
// cycle with everything of frame generation is 17.7 ms, as the cost agent's inventory of that leg reads it (INFERRED there:
// the game 13.43, the dispatch 2.67, the rest about 1.6). WHAT THE SHAPE IS NOT: the leg's every number. The simulation lumps
// the scene's tail at the injection, so a generated frame fired just ahead of an injection is shown ahead of all of it,
// and one fired at the injection behind it: the unevenness it reads for an early show is the most such a frame can have.
#pragma once

static RtPar RfLock300()
{
    RtPar p = LaLock(70, 2900, 1);
    p.kWorkUs = 2700; p.postUs = 4000; p.upUs = 3300; p.tailUs = 4500; p.planUs = 3250;
    p.over = 1; p.ord = 1; p.ordLatUs = 300; p.pubWake = 1;
    return p;
}
// The armed build's keys of this afternoon on a shape (all four on), or off (the machine of that morning, the legs')
static RtPar RfKeys(RtPar p, int early, int noWait, int engage, int stamp)
{
    p.rideEarly = early; p.rideNoWait = noWait; p.armTeach = engage; p.ordAtStamp = stamp;
    p.roomSeed = engage;                                      // (2026-10-17: the engage's mends since stage 2, as fg_pace's bit 1024 takes both away)
    if (engage) p.planMaxUs = 4500;                           // (the plan's bound of that afternoon, with the engage's mends)
    return p;
}
struct RfWin { int n, gen; LONG fromArm, room, rode, waits, relPres, src; };   // (waits, relPres: 2026-10-14, evening, the replays; src, 2026-10-17: the sourced ride pairs)
struct RfLeg
{
    RtStat set; std::string bad, first; int order, n, gen, real1;
    LONG forced, rideLost, early, noAns, earlyNot, relInj, relStamp, relAns, relDead, relPres, armedP, armTaught, waits, blitsMax, seeded;   // (seeded: 2026-10-17)
    int uneven; double unevenMeanE, unevenMaxE, evenMaxE, d1Least, ageMed;
    RfWin w[2];
};
// THE FAKE GAME AT THAT LOCK: frames of it, each with its tail and its cost drawn by a fixed seed; what is counted is counted over
// the pairs from `from` on, and the first two windows of 120 behind the run's start besides.
static RfLeg RfRun(const RtPar& par, int frames, int from, unsigned seed, LONG roomSeedUs = 0)
{
    RfLeg g = {};
    RtBegin(par);
    // (a room floor handed in: what an earlier engage of the session left, which a stand-by keeps: the machine rides from the first pair)
    if (roomSeedUs > 0) ShArmB::a.pl.roomUs = roomSeedUs;
    const FgArmedState& a = ShArmB::a;
    gRtSeed = seed;
    LONG w0[FG_AW_N] = {}, gl0 = 0, e0 = 0, na0 = 0, en0 = 0;
    for (int i = 0; i < frames; i++)
    {
        if (i == from) { for (int k = 0; k < FG_AW_N; k++) w0[k] = a.w.iWhy[k]; gl0 = a.w.rideGenLost; e0 = a.w.rideEarlyN; na0 = a.w.rideNoAns; en0 = a.w.rideEarlyNot; }
        gRt.par.tailUs = (LONG)(4500.0 * RtSpread(0.29));
        RtFrame(RtMs(4.0 * RtSpread(0.25)));
        if (i == 119 || i == 239) { RfWin& w = g.w[i / 120]; w.fromArm = a.pl.tsNever; w.room = a.pl.roomUs; w.rode = ShArmB::a.r.rideN; w.waits = a.w.ordWaits; w.relPres = a.w.ordRelPresent; w.src = a.w.rideSrcN; }
    }
    g.set = RtRead(from, frames);
    g.bad = RtJudgeQ(g.set, 99.0, 99.0);
    g.order = gRt.order; g.first = gRt.firstBad;
    const int n = (int)gRt.pairs.size();
    const double q = (double)B::Qpf() / 1000.0;
    g.d1Least = 1e9;
    for (int i = from; i < n - 1; i++)
    {
        const RtPair& x = gRt.pairs[i]; const RtPair& y = gRt.pairs[i + 1];
        g.n++; if (x.gen == 1) g.gen++; if (x.real == 1) g.real1++;
        if (x.gen == 1 && x.real == 1 && y.gen == 1)
        {
            const double d1 = (double)(x.tReal - x.tGen) / q, whole = (double)(y.tGen - x.tGen) / q, e = d1 - whole * 0.5;
            if (d1 < g.d1Least) g.d1Least = d1;
            if (fabs(e) > 2.0) { g.uneven++; g.unevenMeanE += e; if (fabs(e) > g.unevenMaxE) g.unevenMaxE = fabs(e); }
            else if (fabs(e) > g.evenMaxE) g.evenMaxE = fabs(e);
        }
    }
    if (g.uneven) g.unevenMeanE /= g.uneven;
    {   // the picture's age at its real frame's showing, from its entry: the median over the pairs read
        std::vector<double> ages;
        for (int i = from; i < n - 1; i++) { const RtPair& x = gRt.pairs[i]; if (x.real == 1) ages.push_back((double)(x.tReal - x.tEntry) / q); }
        if (!ages.empty()) { std::nth_element(ages.begin(), ages.begin() + ages.size() / 2, ages.end()); g.ageMed = ages[ages.size() / 2]; }
    }
    for (int b = 0; b < 2; b++)
    {
        RfWin& w = g.w[b];
        for (int i = b * 120; i < (b + 1) * 120 && i < n - 1; i++) { w.n++; if (gRt.pairs[i].gen == 1) w.gen++; }
    }
    g.forced = a.w.iWhy[FG_AW_FORCED] - w0[FG_AW_FORCED]; g.rideLost = a.w.rideGenLost - gl0;
    g.early = a.w.rideEarlyN - e0; g.noAns = a.w.rideNoAns - na0; g.earlyNot = a.w.rideEarlyNot - en0;
    g.relInj = a.w.ordRelInj; g.relStamp = a.pl.ordRelStamp; g.relAns = a.pl.ordRelAnswer; g.relDead = a.pl.ordRelDeadline; g.relPres = a.w.ordRelPresent;
    g.armedP = a.w.ordFirstP; g.armTaught = a.pl.armTaught; g.waits = a.w.ordWaits; g.seeded = a.pl.roomSeeded;
    RtEnd("RF");
    g.blitsMax = ShArmB::ordBlitsMax;
    return g;
}
static const char* RfLine(const RfLeg& g)
{
    static char bufs[8][1100]; static int at = 0;
    char* const b = bufs[at++ & 7];
    snprintf(b, sizeof(bufs[0]), "of %d pairs %d generated frames shown (%.1f of 100), %d real frames shown once, pictures out of place %d; not shown: forced %ld (of them at the ride %ld, for want of the answer %ld);"
        " shown early at the next injection %ld (not shown there for the earlier answer's stamp %ld); %.1f real frames a second at a period in force of %.3f ms (%.3f to %.3f),"
        " the ride's floor %ld us (even %ld, wait %ld, fit %ld); on the screen the generated frame to its real frame %.3f ms, on to the next %.3f ms (%.0f thousandths), the least %.3f ms;"
        " the real frame off the middle at worst %.3f ms over the pairs within 2 ms of it, %d pair(s) further (mean %+.3f ms, worst %.3f ms); the picture's age median %.3f ms, at worst %.3f ms;"
        " order waits %ld (released by the placer at the answer %ld, by the answer's stamper %ld, by the worker at the injection %ld, by the deadline %ld, by the worker ahead of a present %ld; armed at pair %ld; the most presents behind one %ld)",
        g.n, g.gen, g.n ? 100.0 * g.gen / g.n : 0.0, g.real1, g.order, g.forced, g.rideLost, g.noAns, g.early, g.earlyNot, g.set.fps, g.set.perMs, g.set.perMin, g.set.perMax,
        g.set.rideUs, g.set.rideEvenUs, g.set.rideWaitUs, g.set.rideFitUs, g.set.d1Ms, g.set.d2Ms, g.set.d1Pm, g.d1Least, g.evenMaxE, g.uneven, g.unevenMeanE, g.unevenMaxE, g.ageMed, g.set.ageMs,
        g.waits, g.relAns, g.relStamp, g.relInj, g.relDead, g.relPres, g.armedP, g.blitsMax);
    return b;
}
// What every run of the machine must hold, whatever it shows: each real frame once and in its place, no order wait released off its
// rule, never two presents behind one.
static void RfSound(const char* tag, const RfLeg& g)
{
    if (g.order || g.real1 != g.n) Err("%s: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", tag, g.order, g.first.c_str(), g.real1, g.n);
    if (g.blitsMax > 1) Err("%s: %ld presents stood behind one order wait (one at the most wanted)", tag, g.blitsMax);
}

// ---------------------------------------------------------------- RF1, the lock at 300 MHz, settled: every generated frame shown, and the card's own rate
// WHAT IT SHOWS. RfLock300, 760 frames, read from the 520th.
//   THE MACHINE OF THAT MORNING (the control, the keys at 0, the legs' machine): the period at the ride's wait floor, and pairs whose
//     generated frame is still due at the next injection, withheld there with their answer in: the legs' fault, 3 to 10 of 100.
//   THE EARLY SHOW ALONE (the wait floor kept): every such frame shown at the injection, its answer in; the period and the rate as the
//     control's; 99 of 100 or more.
//   THE MACHINE NOW (the armed build's keys: the early show, the engage's two mends, the answer's stamper releasing the order wait;
//     the wait floor KEPT in the period): 99 of 100 or more shown, the rate, the halves and the picture's age as the control's.
//   AND THE WAIT FLOOR OUT OF THE PERIOD (FgArmedKeys::rideNoWait, a development key's): THE TRADE IT MAKES, said and held as a fact:
//     the period comes down to what FIT (the answer's deadline ahead of the ride) lets the card's queue settle at, the rate
//     rises, AND the card's queue grows behind a dear frame (the picture's age at worst), a few answers come behind the ride,
//     and the pairs whose generated frame is shown early, uneven, multiply: half a period under the ride's wait is a
//     generated frame that cannot be planted even. That is why the floor stays: see fg_machine.h, ArmedRideFloors.
// THE HALVES: the generated frame to its real frame against half the period, over the run (500 of 1000 is even), and the pairs
// whose real frame stands more than 2 ms off the middle, each said.
static void CaseRF1()
{
    const char* const name = "RF1 the lock at 300 MHz settled: the generated frames still due at the next injection are shown there, the rate, the halves and the age kept; what the wait floor's factor buys (2026-10-14, afternoon)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const RfLeg c = RfRun(RfKeys(RfLock300(), 0, 0, 0, 0), 760, 520, 20261014u, 33000);
    const RfLeg e = RfRun(RfKeys(RfLock300(), 1, 0, 0, 0), 760, 520, 20261014u, 33000);
    const RfLeg g = RfRun(RfKeys(RfLock300(), 1, 0, 1, 1), 760, 520, 20261014u, 33000);
    const RfLeg o = RfRun(RfKeys(RfLock300(), 1, 1, 1, 1), 760, 520, 20261014u, 33000);
    ViolRestore(stc);
    printf("  CONTROL, the machine of that morning (the legs'): %s\n  THE EARLY SHOW ALONE: %s\n  THE MACHINE NOW: %s\n  THE WAIT FLOOR OUT OF THE PERIOD (a development key's): %s\n", RfLine(c), RfLine(e), RfLine(g), RfLine(o));
    printf("  OF 100 GENERATED FRAMES at the lock at 300 MHz's shape: %.1f shown before, %.1f now; %.1f real frames a second before, %.1f now; the halves %.0f thousandths before, %.0f now;"
           " pairs more than 2 ms off the middle %d before, %d now (their real frame %+.2f ms off it on average, %.2f at worst); the picture's age median %.2f ms before, %.2f now (at worst %.2f and %.2f)\n"
           "  THE WAIT FLOOR'S FACTOR OF TWO: out of the period, %.1f real frames a second (%+.1f) and the picture's age median %.2f ms (%+.2f), at worst %.2f ms (%+.2f), %.1f of 100 shown\n",
        100.0 * c.gen / c.n, 100.0 * g.gen / g.n, c.set.fps, g.set.fps, c.set.d1Pm, g.set.d1Pm, c.uneven, g.uneven, g.unevenMeanE, g.unevenMaxE, c.ageMed, g.ageMed, c.set.ageMs, g.set.ageMs,
        o.set.fps, o.set.fps - g.set.fps, o.ageMed, o.ageMed - g.ageMed, o.set.ageMs, o.set.ageMs - g.set.ageMs, 100.0 * o.gen / o.n);
    RfSound("RF1", e); RfSound("RF1", g); RfSound("RF1", o);
    if (c.rideLost * 100 < c.n * 3 || c.gen * 100 > c.n * 97) Err("RF1 control: the machine of that morning withheld %ld of %d generated frames at the ride and showed %d (3 of 100 or more withheld, under 97 shown, wanted): the shape does not show the legs' fault", c.rideLost, c.n, c.gen);
    if (e.gen * 100 < e.n * 99 || e.early <= 0 || e.noAns > 1) Err("RF1: with the early show alone %d of %d generated frames shown, %ld shown early, %ld not for want of the answer (99 of 100, some, and one at the most wanted)", e.gen, e.n, e.early, e.noAns);
    if (e.set.fps < c.set.fps - 0.3 || e.set.fps > c.set.fps + 0.3) Err("RF1: the early show alone moved the rate from %.1f to %.1f real frames a second (the same wanted: it changes no period)", c.set.fps, e.set.fps);
    if (g.gen * 100 < g.n * 99) Err("RF1: now %d of %d generated frames shown (99 of 100 or more wanted)", g.gen, g.n);
    if (g.set.fps < c.set.fps - 0.3) Err("RF1: now %.1f real frames a second where the machine of that morning ran at %.1f (no lower wanted)", g.set.fps, c.set.fps);
    if (g.ageMed > c.ageMed + 0.3 || g.set.ageMs > c.set.ageMs + 0.5) Err("RF1: now the picture's age median %.2f ms and at worst %.2f where the control read %.2f and %.2f (no older wanted)", g.ageMed, g.set.ageMs, c.ageMed, c.set.ageMs);
    if (g.set.d1Pm < c.set.d1Pm - 25.0 || g.set.d1Pm > 525.0) Err("RF1: now the generated frame to its real frame stands at %.0f thousandths of the period where the control read %.0f (within 25 of it, and no more than 525, wanted)", g.set.d1Pm, c.set.d1Pm);
    if (g.relStamp <= 0 || g.relDead > 2 || g.relPres) Err("RF1: now the answer's stamper released %ld order wait(s), the deadline %ld, the worker ahead of a present %ld (some, two at the most and 0 wanted)", g.relStamp, g.relDead, g.relPres);
    if (o.set.fps < g.set.fps + 1.0 || o.set.ageMs < g.set.ageMs + 2.0 || o.uneven < g.uneven * 2) Err("RF1: with the wait floor out of the period %.1f real frames a second, the picture's age at worst %.2f ms and %d pairs more than 2 ms off the middle, where with it %.1f, %.2f and %d (a frame a second more, two milliseconds older at worst or more, twice the uneven pairs or more: the trade this case says)", o.set.fps, o.set.ageMs, o.uneven, g.set.fps, g.set.ageMs, g.uneven);
    Ctl("the machine of that morning at the lock at 300 MHz's shape: %ld of %d generated frames withheld at the ride with their answer in, at a period of %.3f ms (the wait floor %ld us), %.1f real frames a second", c.rideLost, c.n, c.set.perMs, c.set.rideWaitUs, c.set.fps);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RF2, the engage at the lock at 300 MHz: the first two windows
// WHAT IT SHOWS. RfLock300 from a fresh machine, the room floor at 0 (a session's first engage), 360 frames; THE FIRST TWO WINDOWS OF
// 120 behind the simulation's start (the run-in's six pairs ahead of them) are what is read.
//   THE MACHINE OF THAT MORNING (the control): no pair rides in the first window, every pair is timed from the arm and none teaches
//     the room floor: under 40 of 100 shown in the first window (the legs: 0 of 118 and 26 of 121).
//   NOW: the pairs timed from the arm teach the floor by their handoff's stamp, the ride starts within the first window, and the
//     order wait stands from the first ride pair: 80 of 100 or more in the first window (the first twelve pairs behind an
//     engage teach nothing, and the six that teach must come first), 97 or more in the second.
//   AND THE SAME WITH THE HELPER'S PLACE GIVEN (the card honours it at the first hand that rides: the leg t70.4's state): before,
//     the first window as poor (the place is asked at the first hand that rides, and none rides); now, as the mend's.
static void CaseRF2()
{
    const char* const name = "RF2 the engage at the lock at 300 MHz: pairs timed from the arm teach the room floor, the order wait stands from the first ride pair; the first two windows shown (2026-10-14, afternoon)";
    CaseBegin(name);
    const RingStats stc = gRingSt;
    const RfLeg c = RfRun(RfKeys(RfLock300(), 0, 0, 0, 0), 360, 240, 7u);
    const RfLeg g = RfRun(RfKeys(RfLock300(), 1, 0, 1, 1), 360, 240, 7u);
    RtPar pa = RfLock300(); pa.rank = 2; pa.card = 1;
    const RfLeg ca = RfRun(RfKeys(pa, 0, 0, 0, 0), 360, 240, 7u);
    const RfLeg ga = RfRun(RfKeys(pa, 1, 0, 1, 1), 360, 240, 7u);
    ViolRestore(stc);
    printf("  CONTROL, the machine of that morning: the first window %d of %d shown, the second %d of %d; then %s\n  NOW: the first window %d of %d shown, the second %d of %d; then %s (pairs that taught the room floor from the arm %ld, the floor seeded %ld time(s); the order wait's first at pair %ld)\n",
        c.w[0].gen, c.w[0].n, c.w[1].gen, c.w[1].n, RfLine(c), g.w[0].gen, g.w[0].n, g.w[1].gen, g.w[1].n, RfLine(g), g.armTaught, g.seeded, g.armedP);
    printf("  AT EACH WINDOW'S END, pairs timed from the arm so far, the room floor, hands that rode so far: before %ld, %ld us, %ld and %ld, %ld us, %ld; now %ld, %ld us, %ld and %ld, %ld us, %ld\n",
        c.w[0].fromArm, c.w[0].room, c.w[0].rode, c.w[1].fromArm, c.w[1].room, c.w[1].rode, g.w[0].fromArm, g.w[0].room, g.w[0].rode, g.w[1].fromArm, g.w[1].room, g.w[1].rode);
    printf("  THE HELPER'S PLACE GIVEN: before, the first window %d of %d, the second %d of %d; now %d of %d and %d of %d\n", ca.w[0].gen, ca.w[0].n, ca.w[1].gen, ca.w[1].n, ga.w[0].gen, ga.w[0].n, ga.w[1].gen, ga.w[1].n);
    RfSound("RF2", g); RfSound("RF2", ga);
    if (c.w[0].gen * 100 > c.w[0].n * 40) Err("RF2 control: the machine of that morning showed %d of %d generated frames in the engage's first window (under 40 of 100 wanted): the shape does not show the legs' engage", c.w[0].gen, c.w[0].n);
    if (g.w[0].gen * 100 < g.w[0].n * 80 || g.w[1].gen * 100 < g.w[1].n * 97) Err("RF2: now %d of %d and %d of %d generated frames shown in the engage's first two windows (80 and 97 of 100 or more wanted)", g.w[0].gen, g.w[0].n, g.w[1].gen, g.w[1].n);
    if (ga.w[0].gen * 100 < ga.w[0].n * 80 || ga.w[1].gen * 100 < ga.w[1].n * 97) Err("RF2: with the helper's place given now %d of %d and %d of %d shown in the first two windows (80 and 97 of 100 or more wanted)", ga.w[0].gen, ga.w[0].n, ga.w[1].gen, ga.w[1].n);
    // (2026-10-17: or the floor seeded by the engage's first stamped pair, which then rides, is timed by its stamp and leaves no pair to teach from the arm)
    if ((g.armTaught < 6 && g.seeded < 1) || g.armedP <= 0) Err("RF2: now %ld pair(s) taught the room floor from the arm, the floor seeded %ld time(s), the order wait's first at pair %ld (six or more, or a seed; and a pair, wanted)", g.armTaught, g.seeded, g.armedP);
    if (g.gen * 100 < g.n * 99) Err("RF2: now behind the two windows %d of %d shown (99 of 100 or more wanted)", g.gen, g.n);
    Ctl("the machine of that morning: %d of %d generated frames shown in the engage's first window, %d of %d in the second (the helper's place given: %d and %d)", c.w[0].gen, c.w[0].n, c.w[1].gen, c.w[1].n, ca.w[0].gen, ca.w[1].gen);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RF3, the early show in the worker's order, the worker held
// WHAT IT SHOWS, by hand, as FQ5 does (the frames made by hand, the body's stand-in held on an event, the next frame's hand and Present
// queued behind it). Pair A armed and riding (the room floor handed in over any period), its generated frame NOT DUE (the placer
// never stepped: its due was never raised), and then frame B's injection:
//   THE ANSWER IN: WHILE THE WORKER STANDS IN B's BODY A's generated frame IS on the screen, once, and A's real frame is NOT; released,
//     A's real frame once, behind its generated frame and ahead of everything of B. Counted shown early, once.
//   THE ANSWER NOT IN (the helper held in A's dispatch): nothing of A's while held; released, A's real frame once, its generated frame
//     never (withheld at the ride, counted "for want of the answer"), and never behind its real frame.
//   THE ORDER WAIT STANDING behind A's handoff, its answer in and the placer never stepped (a wait on every ride pair): the worker
//     releases it at the injection ahead of the early show, counted, and the ride does not find it standing (no release ahead
//     of a present, the wait not switched off for the engage, one present behind it at the most).
// CONTROL (a run): the key at 0: with the answer in, A's generated frame is not on the screen while held, and never: withheld at the
// ride, as the legs lost it.
struct Rf3Leg { bool armed, rides, held, ansIn; int genIn, realIn, genAfter, realAfter, genBeforeReal, bCount; LONG early, noAns, lost, relInj, relPres, blitsMax; };
static Rf3Leg Rf3Run(int early, int noAnswer, int ord)
{
    Rf3Leg g = {};
    if (!gFqEv) gFqEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;   // (a room floor over any period: every hand of the want ON rides)
    a.k.rideEarly = early;
    if (ord) { a.k.over = 1; a.k.ord = 2; a.k.ordMaxUs = 4500; LaOn(); }
    ShWarm<MA>(); MgDueN();
    if (noAnswer) _InterlockedExchange(&gH.holdDispatch, 1);
    const DWORD tagA = ShFrame<MA>(true, false);
    const LONG nA = x.w.lastQN;
    if (noAnswer) WaitFor([] { return gH.inDispatch != 0; }, 3000);
    else if (!ShSettle()) Err("RF3: the fake helper did not read pair A's messages");
    g.ansIn = B::TlValue(FG_TL_OUT) == 2ull * (ULONGLONG)nA;
    g.armed = s.w.openPhase == FG_PAIR_ARMED; g.rides = (s.w.openFlags & FG_HF_RIDE) != 0;
    const size_t shown0 = MgShownN();
    const LONG e0 = a.w.rideEarlyN, na0 = a.w.rideNoAns, gl0 = a.w.rideGenLost, ri0 = a.w.ordRelInj, rp0 = a.w.ordRelPresent;
    gInjBody = FqHoldBody; _InterlockedExchange(&gFqHold, 1);
    ClockAdd(16.7);
    ShInject(true); DtPublishFenced();
    g.held = WaitFor([] { return gFqIn != 0; }, 3000);
    const DWORD genA = ShGenTag(tagA, false);
    g.genIn = MgTagCount(shown0, genA); g.realIn = MgTagCount(shown0, tagA);   // THE WORKER HELD IN B's BODY
    Later l = { [] { return true; }, [] { SetEvent(gFqEv); }, 1000, 60 };
    HANDLE h = Spawn(LaterMain, &l);
    const DWORD tagB = Frame<MA>();                           // frame B's hand and Present, QUEUED BEHIND THE HELD WORKER
    Join(h, "RF3's release");
    Drain();
    gInjBody = ShInjBody;
    if (noAnswer) ShRelease();
    if (!ShSettle()) Err("RF3: the fake helper did not read frame B's messages");
    MgDueN();
    g.genAfter = MgTagCount(shown0, genA); g.realAfter = MgTagCount(shown0, tagA);
    const auto sh = Shown();
    int iG = -1, iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++) { if (sh[i].tag == genA && iG < 0) iG = (int)i; if (sh[i].tag == tagA && iA < 0) iA = (int)i; if (sh[i].tag == tagB) { g.bCount++; if (iB < 0) iB = (int)i; } }
    g.genBeforeReal = (iG >= 0 && iA > iG && (iB < 0 || iA < iB)) ? 1 : 0;
    g.early = a.w.rideEarlyN - e0; g.noAns = a.w.rideNoAns - na0; g.lost = a.w.rideGenLost - gl0;
    g.relInj = a.w.ordRelInj - ri0; g.relPres = a.w.ordRelPresent - rp0; g.blitsMax = ShArmB::ordBlitsMax;
    if (ord) { ShArmB::OrdSignal(0x7FFFFF); Drain(); }       // (whatever a run left standing runs before the case's end)
    MgClean("RF3");
    MgEnd();
    return g;
}
static void CaseRF3()
{
    const char* const name = "RF3 the early show in the worker's order, the worker held in the next body: the generated frame ahead of the body, its real frame behind it; no answer, no show; the order wait released ahead of it (2026-10-14, afternoon)";
    CaseBegin(name);
    const Rf3Leg g = Rf3Run(1, 0, 0);
    printf("  THE ANSWER IN: pair A armed %s, rides %s, answered %s; the worker held in B's body %s: A's generated frame on the screen %d time(s), its real frame %d; released: %d and %d, the generated frame first and both ahead of B's: %s;"
           " shown early %ld, withheld at the ride %ld; B's real frame %d time(s)\n",
        g.armed ? "yes" : "NO", g.rides ? "yes" : "NO", g.ansIn ? "yes" : "NO", g.held ? "yes" : "NO", g.genIn, g.realIn, g.genAfter, g.realAfter, g.genBeforeReal ? "yes" : "NO", g.early, g.lost, g.bCount);
    if (!g.armed || !g.rides || !g.held || !g.ansIn) Err("RF3: the leg did not reach its hold (A armed %d, rides %d, answered %d, the worker held %d)", (int)g.armed, (int)g.rides, (int)g.ansIn, (int)g.held);
    if (g.genIn != 1 || g.realIn != 0 || g.genAfter != 1 || g.realAfter != 1 || !g.genBeforeReal || g.early != 1 || g.lost != 0 || g.bCount != 1)
        Err("RF3: with the answer in: A's generated frame %d and real frame %d while held, %d and %d after, in order %d, shown early %ld, withheld %ld, B's real frame %d (1, 0, 1, 1, yes, 1, 0 and 1 wanted)", g.genIn, g.realIn, g.genAfter, g.realAfter, g.genBeforeReal, g.early, g.lost, g.bCount);
    const Rf3Leg n = Rf3Run(1, 1, 0);
    printf("  THE ANSWER NOT IN (the helper held in A's dispatch): A's generated frame %d and real frame %d while held; released: %d and %d; shown early %ld, withheld at the ride %ld (for want of the answer %ld)\n",
        n.genIn, n.realIn, n.genAfter, n.realAfter, n.early, n.lost, n.noAns);
    if (!n.held || n.genIn != 0 || n.realIn != 0 || n.genAfter != 0 || n.realAfter != 1 || n.early != 0 || n.lost != 1 || n.noAns != 1)
        Err("RF3: with the answer not in: A's generated frame %d and real frame %d while held, %d and %d after, shown early %ld, withheld %ld, for want of the answer %ld (0, 0, 0, 1, 0, 1 and 1 wanted)", n.genIn, n.realIn, n.genAfter, n.realAfter, n.early, n.lost, n.noAns);
    const Rf3Leg o = Rf3Run(1, 0, 1);
    printf("  THE ORDER WAIT STANDING (one on every ride pair), its answer in, the placer never stepped: released by the worker at the injection %ld, ahead of a present from there on %ld; the most presents behind one %ld; A's generated frame %d, real frame %d, in order %s\n",
        o.relInj, o.relPres, o.blitsMax, o.genAfter, o.realAfter, o.genBeforeReal ? "yes" : "NO");
    if (o.relInj != 1 || o.relPres != 0 || o.blitsMax > 1 || o.genAfter != 1 || o.realAfter != 1 || !o.genBeforeReal)
        Err("RF3: with the order wait standing: released at the injection %ld, ahead of a present %ld, presents behind one %ld, A's generated frame %d and real frame %d, in order %d (1, 0, one at the most, 1, 1, yes wanted)", o.relInj, o.relPres, o.blitsMax, o.genAfter, o.realAfter, o.genBeforeReal);
    const RingStats stc = gRingSt;
    const Rf3Leg c = Rf3Run(0, 0, 0);
    ViolRestore(stc);
    printf("  CONTROL, the key at 0, the answer in: A's generated frame %d while held, %d after; withheld at the ride %ld\n", c.genIn, c.genAfter, c.lost);
    if (c.genIn != 0 || c.genAfter != 0 || c.lost != 1 || c.realAfter != 1) Err("RF3 control: with the key at 0 A's generated frame was on the screen %d time(s) while held and %d after, withheld at the ride %ld, its real frame %d (0, 0, 1 and 1 wanted): the control shows nothing", c.genIn, c.genAfter, c.lost, c.realAfter);
    Ctl("the key at 0: a generated frame with its answer in, still due at the next injection, is withheld at the ride (%ld)", c.lost);
    CaseEnd(name);
}

// ---------------------------------------------------------------- RF4, FgPace's last spin hears the placer's wake
// WHAT IT SHOWS, by hand, on the render thread: FgPace's sleep (SleepUntil) to a due 1 ms ahead, so its timer is set for 0.4 ms and
// the last 0.6 ms are spun; THE PLACER'S WAKE COMES INSIDE THE SPIN (the binding's clock sets the pace event 0.2 ms into it).
//   A BINDING THAT SAYS IT CAN ASK THE EVENT WITHOUT A WAIT (PacePoll, the armed product's): the spin hears it, once: a wake counted
//     there, and the event taken.
//   CONTROL, a binding that does not say it (the machine of that morning, the rig's and the equivalence proofs' to this day): the spin
//     hears nothing, and the event is still set when the sleep returns.
// The clock moves a microsecond each time this binding's render thread reads it.
static LONGLONG gRf4SetAt = 0; static volatile LONG gRf4Set = 0;
struct Rf4ClockB : ShArmB
{
    static LONGLONG Now()
    {
        if (fakeClock && GetCurrentThreadId() == RenderTid())
        {
            clockNow += Qpf() / 1000000;
            if (gRf4SetAt && clockNow >= gRf4SetAt && !gRf4Set) { _InterlockedExchange(&gRf4Set, 1); SetEvent(evPace); }
        }
        return ShArmB::Now();
    }
};
struct Rf4PollB : Rf4ClockB { static int PacePoll() { return WaitForSingleObject(evPace, 0) == WAIT_OBJECT_0 ? 1 : 0; } };
typedef FgMachine<Rf4ClockB> MRf4Deaf;
typedef FgMachine<Rf4PollB> MRf4Poll;
struct Rf4Leg { LONG wakes, spinWakes; bool stillSet; bool set; double sleptUs; };
template <class MM> static Rf4Leg Rf4Run()
{
    Rf4Leg g = {};
    MgFresh<MM>();
    ShWarm<MM>(); MgDueN();
    FgState& s = B::S(); const FgArmedState& a = ShArmB::a;
    ResetEvent(B::evPace);
    const LONGLONG t0 = B::clockNow, due = t0 + MM::UsToTicks(1000);
    gRf4SetAt = due - MM::UsToTicks(600) + MM::UsToTicks(200); gRf4Set = 0;
    const LONG w0 = s.r.wakes, sw0 = a.r.spinWakes;
    MM::SleepUntil(due);
    g.wakes = s.r.wakes - w0; g.spinWakes = a.r.spinWakes - sw0; g.set = gRf4Set != 0;
    g.stillSet = WaitForSingleObject(B::evPace, 0) == WAIT_OBJECT_0;
    g.sleptUs = (double)(B::clockNow - t0) * 1e6 / (double)B::Qpf();
    gRf4SetAt = 0;
    Drain();
    MgClean("RF4");
    MgEnd();
    return g;
}
static void CaseRF4()
{
    const char* const name = "RF4 FgPace's last spin hears the placer's wake where the binding can ask the event without a wait (2026-10-14, afternoon)";
    CaseBegin(name);
    const Rf4Leg g = Rf4Run<MRf4Poll>();
    printf("  THE ARMED PRODUCT'S BINDING: the wake set inside the spin %s; wakes heard %ld, of them in the spin %ld; the event still set at the sleep's end %s; the sleep %.0f us of 1,000\n",
        g.set ? "yes" : "NO", g.wakes, g.spinWakes, g.stillSet ? "YES" : "no", g.sleptUs);
    if (!g.set || g.wakes != 1 || g.spinWakes != 1 || g.stillSet || g.sleptUs < 999.0 || g.sleptUs > 1010.0) Err("RF4: the wake set %d, heard %ld, in the spin %ld, the event left set %d, the sleep %.0f us (yes, 1, 1, no, and 1,000 wanted: the spin hears it and runs to the due)", (int)g.set, g.wakes, g.spinWakes, (int)g.stillSet, g.sleptUs);
    const RingStats stc = gRingSt;
    const Rf4Leg c = Rf4Run<MRf4Deaf>();
    ViolRestore(stc);
    printf("  CONTROL, a binding that cannot ask it: wakes heard %ld, in the spin %ld; the event still set at the sleep's end %s\n", c.wakes, c.spinWakes, c.stillSet ? "yes" : "NO");
    if (!c.set || c.wakes != 0 || !c.stillSet) Err("RF4 control: with no PacePoll the spin heard %ld wake(s) and the event was left set %d (0 and yes wanted): the control shows nothing", c.wakes, (int)c.stillSet);
    Ctl("a binding with no PacePoll: the wake set inside FgPace's last spin is not heard there (the event left set)");
    CaseEnd(name);
}

// ---------------------------------------------------------------- RF5, the worker ahead of the card waits for the answer behind a standing order wait
// WHAT IT SHOWS (the sitting of 2026-10-14's afternoon, t70.2: 108 generated frames "not shown there for want of the answer", and behind
// each the next hand finding that dispatch still running: 69 pairs with no source and 71 fed with a reset, 38 copies over it), by
// hand: pair A armed and riding, AN ORDER WAIT STANDING behind its handoff (one on every ride pair; the placer never stepped, so
// only the answer or the worker can end it), and THE HELPER HELD IN A's DISPATCH, so B's injection reaches the worker ahead of A's
// answer, as the game's CPU did there. Then:
//   THE MACHINE NOW: the worker waits at B's injection; the helper let go once that wait has begun (from another thread); A's generated
//     frame is shown there, ahead of B's body, its real frame behind it, once each and in order; B's hand finds A's dispatch
//     answered: B's 'G' says DISPATCH, and nothing is reset behind it.
//   THE HELPER NEVER LET GO DURING THE WAIT: the wait ends at its own bound (FG_EARLY_WAIT_MS), A's generated frame is withheld and
//     its real frame shown once: no freeze, nothing out of order.
//   NO ORDER WAIT STANDING (none asked): no wait is made.
// CONTROL (a run): the key at 0, the leg's machine: A's generated frame withheld for want of the answer, and B's hand finds the
// dispatch running: NO SOURCE (the copy over it is not asked here: eight clean hands are not there).
static volatile LONG gRf5Wait0 = 0;
struct Rf5Leg { bool armed, rides; int genA, realA, realB, inOrder; LONG kindB, waits, got, noAns, maxUs; DWORD ms; };
static Rf5Leg Rf5Run(int key, int release, int ord)
{
    Rf5Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.rideEarly = 1; a.k.rideWaitAns = key;
    if (ord) { a.k.ord = 2; a.k.ordMaxUs = 4500; LaOn(); }
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdDispatch, 1);
    const DWORD tagA = ShFrame<MA>(true, false);
    WaitFor([] { return gH.inDispatch != 0; }, 3000);
    g.armed = s.w.openPhase == FG_PAIR_ARMED; g.rides = (s.w.openFlags & FG_HF_RIDE) != 0;
    const size_t shown0 = MgShownN();
    gRf5Wait0 = a.w.rideWaitN;
    const LONG got0 = a.w.rideWaitGot, na0 = a.w.rideNoAns;
    HANDLE h = nullptr; Later l = { [] { return ShArmB::a.w.rideWaitN > gRf5Wait0; }, [] { ShRelease(); }, 3000, 0 };
    if (release) h = Spawn(LaterMain, &l);
    const DWORD t0 = GetTickCount();
    ClockAdd(16.7);
    ShInject(true);
    const DWORD tagB = Frame<MA>();
    const LONG nB = x.w.lastQN;
    Drain();
    g.ms = GetTickCount() - t0;
    if (h) Join(h, "RF5's release");
    else ShRelease();
    if (!ShSettle()) Err("RF5: the fake helper did not read frame B's messages");
    MgDueN();
    if (ord) { ShArmB::OrdSignal(0x7FFFFF); Drain(); }       // (what stands behind B's own wait runs: B's real frame among it)
    { const std::vector<RingPipe> log = RingPipeLog(); g.kindB = ShKindOf(log, nB); }
    const auto sh = Shown(); const DWORD genA = ShGenTag(tagA, false);
    int iG = -1, iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++) { if (sh[i].tag == genA) { g.genA++; if (iG < 0) iG = (int)i; } if (sh[i].tag == tagA) { g.realA++; if (iA < 0) iA = (int)i; } if (sh[i].tag == tagB) { g.realB++; if (iB < 0) iB = (int)i; } }
    g.inOrder = (iG < 0 || iG < iA) && (iA >= 0 && (iB < 0 || iA < iB)) ? 1 : 0;
    g.waits = a.w.rideWaitN - gRf5Wait0; g.got = a.w.rideWaitGot - got0; g.noAns = a.w.rideNoAns - na0; g.maxUs = a.w.rideWaitMaxUs;
    MgClean("RF5");
    MgEnd();
    return g;
}
static void CaseRF5()
{
    const char* const name = "RF5 the worker ahead of the card: at the next injection it waits for the answer behind a standing order wait; the generated frame shown, the next pair keeps its source (2026-10-14, evening)";
    CaseBegin(name);
    const Rf5Leg g = Rf5Run(1, 1, 1);
    printf("  THE MACHINE NOW: pair A armed %s, rides %s; waits at B's injection %ld, the answer came in %ld; A's generated frame shown %d time(s), its real frame %d, B's %d, in order %s; B's 'G' says %s; frame B took %lu ms\n",
        g.armed ? "yes" : "NO", g.rides ? "yes" : "NO", g.waits, g.got, g.genA, g.realA, g.realB, g.inOrder ? "yes" : "NO", ShKindName((int)g.kindB), (unsigned long)g.ms);
    if (!g.armed || !g.rides || g.waits != 1 || g.got != 1 || g.genA != 1 || g.realA != 1 || g.realB != 1 || !g.inOrder || g.kindB != (LONG)FG_G_DISPATCH)
        Err("RF5: armed %d, rides %d, waits %ld, answered %ld, A's generated frame %d, real frame %d, B's %d, in order %d, B's 'G' %s (yes, yes, 1, 1, 1, 1, 1, yes, DISPATCH wanted)", (int)g.armed, (int)g.rides, g.waits, g.got, g.genA, g.realA, g.realB, g.inOrder, ShKindName((int)g.kindB));
    const Rf5Leg b = Rf5Run(1, 0, 1);
    printf("  THE HELPER NEVER LET GO DURING THE WAIT: waits %ld, the answer came %ld, for want of the answer %ld, the wait's longest %ld us; A's generated frame %d, real frame %d, in order %s; frame B took %lu ms\n",
        b.waits, b.got, b.noAns, b.maxUs, b.genA, b.realA, b.inOrder ? "yes" : "NO", (unsigned long)b.ms);
    if (b.waits != 1 || b.got != 0 || b.noAns != 1 || b.genA != 0 || b.realA != 1 || !b.inOrder)
        Err("RF5: with the helper held through the wait: waits %ld, answered %ld, for want of the answer %ld, A's generated frame %d, real frame %d, in order %d (1, 0, 1, 0, 1, yes wanted)", b.waits, b.got, b.noAns, b.genA, b.realA, b.inOrder);
    const Rf5Leg n = Rf5Run(1, 1, 0);
    printf("  NO ORDER WAIT STANDING: waits %ld; A's generated frame %d, real frame %d\n", n.waits, n.genA, n.realA);
    if (n.waits != 0 || n.realA != 1) Err("RF5: with no order wait standing %ld wait(s) were made and A's real frame shown %d time(s) (0 and 1 wanted)", n.waits, n.realA);
    const RingStats stc = gRingSt;
    const Rf5Leg c = Rf5Run(0, 0, 1);                         // (the helper let go behind frame B: the leg's order of events)
    ViolRestore(stc);
    printf("  CONTROL, the key at 0: waits %ld; A's generated frame %d, for want of the answer %ld; B's 'G' says %s\n", c.waits, c.genA, c.noAns, ShKindName((int)c.kindB));
    // (B's hand is judged where the worker runs with the render thread, threaded=2: at threaded=1 it may run a frame behind, and then behind
    // the helper's release, which this hand-made order cannot hold it ahead of)
    if (c.waits != 0 || c.genA != 0 || c.noAns != 1 || (gCfg == 2 && c.kindB != (LONG)FG_G_NOSOURCE)) Err("RF5 control: with the key at 0 waits %ld, A's generated frame %d, for want of the answer %ld, B's 'G' %s (0, 0, 1 and NOSOURCE wanted): the control shows nothing", c.waits, c.genA, c.noAns, ShKindName((int)c.kindB));
    Ctl("the key at 0: the worker ahead of the answer withholds A's generated frame and B's hand has no source, the leg's event");
    CaseEnd(name);
}
