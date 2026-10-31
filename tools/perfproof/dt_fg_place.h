// dt_fg_place.h: STAGE 3, THE GENERATED FRAME PLACED BY THE CARD, AND ITS FIRST MEND (2026-10-17; docs/reports/fg-4/brief-2026-10-17/
// BRIEF-stage3-placement.md and the main session's order of that day). Included by dt_fg_test.cpp behind dt_fg_replay.h, with ONE line.
//
// ---------------------------------------------------------------- PW1, the prepare's wait behind the answer (fg_machine.h, ArmedPrepWait)
// WHAT IT SHOWS (stage 2's leg, sitting-2026-10-17: 155 ride pairs fired ahead of the body for a wait on the helper's prepare, 67 generated frames
// forced, then no source 70 and a reset 73), by hand: pair A armed and riding, AN ORDER WAIT STANDING behind its handoff (the placer never
// stepped, so only the answer, the worker or a stop ends it), and THE HELPER HELD IN A's PREPARE, so B's injection reaches the worker with that
// prepare not done and a wait on it to be queued, as the leg's card behind the worker had it. Then, with FgArmedKeys::prepWait:
//   THE HELPER LET GO DURING THE WAIT (from another thread, once the wait has begun): the prepare is done once A's answer is in; A's real frame is
//     NOT fired ahead of B's body (no guard, no ride pair ahead), A's generated frame is shown, A's real frame once behind it, B's once behind
//     that, in order; B's hand finds A's dispatch answered: B's 'G' says DISPATCH, nothing reset behind it.
//   THE HELPER NEVER LET GO DURING THE WAIT: the wait ends at its own bound; A's real frame is forced ahead of the prepare's wait (never behind
//     it), once; A's generated frame withheld; no freeze (B's frame back within a second).
//   THE HELPER KILLED DURING THE WAIT (its fences go to the top): the wait ends at once; A's real frame once; no freeze.
//   A RESET'S STOP DURING THE WAIT (ArmedStopped, the first step of BeforeReset: the order wait released): the wait ends; A's real frame once,
//     forced ahead of the prepare's wait; no freeze.
//   NO ORDER WAIT STANDING (none asked): no wait is made.
// CONTROL (a run): the key at 0, stage 2's machine: A's real frame fired ahead of the body at once, its generated frame withheld, and B's hand
// finds the dispatch running: NO SOURCE (the leg's chain).
#pragma once

static volatile LONG gPw1Wait0 = 0;
struct Pw1Leg { bool armed, rides; int genA, realA, realB, inOrder; LONG kindB, waits, got, maxUs, guard, ahead; DWORD ms; };
static Pw1Leg Pw1Run(int key, int how, int ord)
{
    Pw1Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.rideEarly = 1; a.k.rideWaitAns = 1; a.k.prepWait = key;
    if (ord) { a.k.ord = 2; a.k.ordMaxUs = 4500; LaOn(); }
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);                    // (A's prepare is held: the next 'Q' the helper reads)
    const DWORD tagA = ShFrame<MA>(true, false);
    WaitFor([] { return gH.inPrep != 0; }, 3000);
    g.armed = s.w.openPhase == FG_PAIR_ARMED; g.rides = (s.w.openFlags & FG_HF_RIDE) != 0;
    const size_t shown0 = MgShownN();
    gPw1Wait0 = a.w.prepWaitN;
    const LONG got0 = a.w.prepWaitGot, guard0 = a.w.guardForced, ahead0 = a.w.rideAhead;
    _InterlockedExchange(&a.w.prepWaitMaxUs, 0);
    HANDLE h = nullptr;
    Later l = { [] { return ShArmB::a.w.prepWaitN > gPw1Wait0; }, nullptr, 3000, 0 };
    if (how == 0) l.then = [] { ShRelease(); };
    else if (how == 2) l.then = [] { ShHelperGone(); };
    else if (how == 3) l.then = [] { MA::ArmedStopped(); };
    if (l.then) h = Spawn(LaterMain, &l);
    const DWORD t0 = GetTickCount();
    ClockAdd(16.7);
    ShInject(true);
    const DWORD tagB = Frame<MA>();
    const LONG nB = x.w.lastQN;
    Drain();
    g.ms = GetTickCount() - t0;
    if (h) Join(h, "PW1's hand");
    if (how != 2) ShRelease();
    if (how != 2 && !ShSettle()) Err("PW1: the fake helper did not read frame B's messages");
    MgDueN();
    if (ord) { ShArmB::OrdSignal(0x7FFFFF); Drain(); }       // (what stands behind B's own wait runs: B's real frame among it)
    { const std::vector<RingPipe> log = RingPipeLog(); g.kindB = ShKindOf(log, nB); }
    const auto sh = Shown(); const DWORD genA = ShGenTag(tagA, false);
    int iG = -1, iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++) { if (sh[i].tag == genA) { g.genA++; if (iG < 0) iG = (int)i; } if (sh[i].tag == tagA) { g.realA++; if (iA < 0) iA = (int)i; } if (sh[i].tag == tagB) { g.realB++; if (iB < 0) iB = (int)i; } }
    g.inOrder = (iG < 0 || iG < iA) && (iA >= 0 && (iB < 0 || iA < iB)) ? 1 : 0;
    g.waits = a.w.prepWaitN - gPw1Wait0; g.got = a.w.prepWaitGot - got0; g.maxUs = a.w.prepWaitMaxUs;
    g.guard = a.w.guardForced - guard0; g.ahead = a.w.rideAhead - ahead0;
    MgClean("PW1");
    MgEnd();
    return g;
}
static void Pw1Say(const char* what, const Pw1Leg& g)
{
    printf("  %s: pair A armed %s, rides %s; waits at B's injection %ld, the prepare done behind %ld, the longest %ld us; A's real frame fired ahead of the body %ld (forced ahead of a wait %ld); A's generated frame shown %d time(s), its real frame %d, B's %d, in order %s; B's 'G' says %s; frame B took %lu ms\n",
        what, g.armed ? "yes" : "NO", g.rides ? "yes" : "NO", g.waits, g.got, g.maxUs, g.ahead, g.guard, g.genA, g.realA, g.realB, g.inOrder ? "yes" : "NO", ShKindName((int)g.kindB), (unsigned long)g.ms);
}
static void CasePW1()
{
    const char* const name = "PW1 the prepare's wait behind the answer: at the next injection, a wait on the helper's prepare due and the order wait standing, the worker waits for the answer first; the real frame rides, the next pair keeps its source (2026-10-17)";
    CaseBegin(name);
    const Pw1Leg g = Pw1Run(1, 0, 1);
    Pw1Say("THE HELPER LET GO DURING THE WAIT", g);
    if (!g.armed || !g.rides || g.waits != 1 || g.got != 1 || g.ahead != 0 || g.guard != 0 || g.genA != 1 || g.realA != 1 || g.realB != 1 || !g.inOrder || g.kindB != (LONG)FG_G_DISPATCH)
        Err("PW1: armed %d, rides %d, waits %ld, the prepare done behind %ld, ahead of the body %ld, forced ahead of a wait %ld, A's generated frame %d, real frame %d, B's %d, in order %d, B's 'G' %s (yes, yes, 1, 1, 0, 0, 1, 1, 1, yes, DISPATCH wanted)",
            (int)g.armed, (int)g.rides, g.waits, g.got, g.ahead, g.guard, g.genA, g.realA, g.realB, g.inOrder, ShKindName((int)g.kindB));
    const Pw1Leg b = Pw1Run(1, 1, 1);
    Pw1Say("THE HELPER NEVER LET GO DURING THE WAIT", b);
    if (b.waits != 1 || b.got != 0 || b.guard != 1 || b.genA != 0 || b.realA != 1 || !b.inOrder || b.ms > 1000 || b.maxUs > 60000)
        Err("PW1: with the helper held through the wait: waits %ld, the prepare done behind %ld, forced ahead of a wait %ld, A's generated frame %d, real frame %d, in order %d, frame B %lu ms (1, 0, 1, 0, 1, yes, a second at the most wanted)", b.waits, b.got, b.guard, b.genA, b.realA, b.inOrder, (unsigned long)b.ms);
    const Pw1Leg k = Pw1Run(1, 2, 1);
    Pw1Say("THE HELPER KILLED DURING THE WAIT", k);
    if (k.waits != 1 || k.realA != 1 || k.genA != 0 || !k.inOrder || k.ms > 1000) Err("PW1: with the helper killed in the wait: waits %ld, A's real frame %d, generated %d, in order %d, frame B %lu ms (1, 1, 0, yes, a second at the most wanted)", k.waits, k.realA, k.genA, k.inOrder, (unsigned long)k.ms);
    const Pw1Leg r = Pw1Run(1, 3, 1);
    Pw1Say("A RESET'S STOP DURING THE WAIT", r);
    if (r.waits != 1 || r.got != 0 || r.realA != 1 || r.genA != 0 || !r.inOrder || r.ms > 1000) Err("PW1: with a Reset's stop in the wait: waits %ld, the prepare done behind %ld, A's real frame %d, generated %d, in order %d, frame B %lu ms (1, 0, 1, 0, yes, a second at the most wanted)", r.waits, r.got, r.realA, r.genA, r.inOrder, (unsigned long)r.ms);
    const Pw1Leg n = Pw1Run(1, 1, 0);
    Pw1Say("NO ORDER WAIT STANDING", n);
    if (n.waits != 0 || n.realA != 1) Err("PW1: with no order wait standing %ld wait(s) were made and A's real frame shown %d time(s) (0 and 1 wanted)", n.waits, n.realA);
    const RingStats stc = gRingSt;
    const Pw1Leg c = Pw1Run(0, 1, 1);
    ViolRestore(stc);
    Pw1Say("CONTROL, the key at 0", c);
    if (c.waits != 0 || c.guard != 1 || c.genA != 0 || (gCfg == 2 && c.kindB != (LONG)FG_G_NOSOURCE)) Err("PW1 control: with the key at 0 waits %ld, forced ahead of a wait %ld, A's generated frame %d, B's 'G' %s (0, 1, 0 and NOSOURCE wanted): the control shows nothing", c.waits, c.guard, c.genA, ShKindName((int)c.kindB));
    Ctl("the key at 0: A's real frame fired ahead of the prepare's wait at once, its generated frame withheld, B's hand with no source: the leg's chain");
    CaseEnd(name);
}

// ---------------------------------------------------------------- PW2, the position and its wait (fg_machine.h, ArmedPlaceArm, ArmedPlaceAt)
// WHAT IT SHOWS, by hand, the binding saying it has the writes' stamps (ShArmB::placeOn): ride pair P runs eight checkpoint calls of the next frame
// up to that frame's injection, so pair A, armed behind it, is given its position at f (8 of 32 at an engage) of eight: the second call of
// frame B. A's dispatch is HELD (the helper in it), A's order wait stands, and the worker meets the position with A's answer not in:
//   THE HELPER LET GO DURING THE POSITION'S WAIT: A's generated frame is fired AT THE POSITION (counted), ahead of B's injection; then A's real
//     frame once behind it, B's behind that, in order.
//   NEVER LET GO: the wait ends at its bound; no frame of A's is fired there; A's real frame once, in order; no freeze.
//   KILLED DURING THE WAIT, A RESET'S STOP DURING THE WAIT: the wait ends; A's real frame once, in order; no freeze.
// CONTROL (a run): no stamps (placeOn 0): no position is given and none is fired, the clock due as stage 2.
// EXTENDED (phase 5, P3; BRIEF-P3-place.md's proofs; memory: a drained proof hides an ordering bug):
//   THE WORKER HELD AT THE POSITION WITH COMMANDS QUEUED BEHIND IT: frame B's eight calls are made with no drain between them, each its own
//     colour, and B's injection and Present behind them. WHILE THE WORKER STANDS IN THE POSITION'S WAIT the device has seen B's calls up to the
//     position and none behind it, and no present of A's; let go, A's generated frame reaches the device right behind B's call at the position
//     and ahead of the next one, every one of B's calls once and in order.
//   THE SECOND POSITION (FgArmedKeys::place2, a plan of 12 ms so its wait has room before the real frame's earliest due): the helper held through the first wait and let go
//     in the second: A's generated frame fired at the second position (the third call: f plus 4 of 32 of eight), right behind that call and
//     ahead of the next, with the worker held there and B's calls queued behind it as above. NEVER LET GO: two waits, each at its bound, no
//     frame of A's at either, A's real frame once and in order, frame B back within a second: THE FREEZE PROOF WITH THE POSITION'S WAITS.
//   ITS CONTROL: the same with place2 at 0: one wait, no second position, no frame of A's fired at a position.
static volatile LONG gPw2Wait0 = 0, gPw2Need = 1, gPw2Seen = -1, gPw2SeenPres = -1;
static size_t gPw2Mark = 0;
static const DWORD kPw2Fill = 0xFF100000u;                    // (frame B's calls: this colour and their number, 1 to 8)
struct Pw2Leg { int genA, realA, realB, inOrder; LONG posAt, fires, fires2, arms2, waits, got, noAns, maxUs; DWORD ms; LONG seen, seenPres; int genAfter, fillsInOrder; LONG bound2; };
static void Pw2Calls(int n) { for (int i = 0; i < n; i++) { D->ColorFill(gOtherRt, nullptr, 0xFF203040u); Drain(); } }
// B's calls the device has seen behind the mark (their numbers in the order the device took them) and the presents among them, in order:
// `seq` gets one entry a record, a fill's number (1 to 8) or 0 for a present
static void Pw2Seq(std::vector<int>& seq)
{
    seq.clear();
    EnterCriticalSection(&gFkLock);
    for (size_t i = gPw2Mark; i < gFk.size(); i++)
    {
        const FkRec& r = gFk[i];
        if (r.slot == 35 && r.a.size() >= 3 && (r.a[2] & 0xFFFFFF00u) == kPw2Fill) seq.push_back((int)(r.a[2] & 0xFFu));
        else if (r.slot == 1803) seq.push_back(0);
    }
    LeaveCriticalSection(&gFkLock);
}
static void Pw2Snap()
{   // (on the releasing thread, WHILE THE WORKER STANDS IN THE WAIT: what the device has seen of frame B)
    std::vector<int> seq; Pw2Seq(seq);
    LONG fills = 0, pres = 0; for (int v : seq) { if (v) fills++; else pres++; }
    _InterlockedExchange(&gPw2Seen, fills); _InterlockedExchange(&gPw2SeenPres, pres);
}
static Pw2Leg Pw2Run(int stamps, int how, int place2 = 0, int held = 0, int plant = 1, LONG planUs = 0, LONG riseUs = 0)
{
    Pw2Leg g = {};
    g.seen = -1; g.seenPres = -1; g.genAfter = -1;
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a;
    a.k.room = 1; a.k.ride = 1; a.k.rideGuardUs = 500; a.pl.roomUs = 100000;
    a.k.rideEarly = 1; a.k.rideWaitAns = 1; a.k.prepWait = 1; a.k.place = 1;
    a.k.ord = 2; a.k.ordMaxUs = 4500; LaOn();
    a.k.place2 = place2;
    if (planUs > 0) { a.k.kPlanUs = planUs; a.k.rideKMaxUs = 0; }   // (round 2: a plan that puts the second wait's bound inside its 8 ms)
    _InterlockedExchange(&a.w.pos2BoundUs, 0);
    _InterlockedExchange(&ShArmB::placeOn, stamps);
    ShWarm<MA>(); MgDueN();
    ClockAdd(16.7); Pw2Calls(8); ShInject(true); Frame<MA>(); Drain();      // (pair P: its next frame's eight calls below give the count)
    if (!ShSettle()) Err("PW2: the fake helper did not read pair P's messages");
    ClockAdd(16.7); Pw2Calls(8);
    _InterlockedExchange(&gH.holdDispatch, 1);                // (A's dispatch is held: the next 'G' the helper serves)
    ShInject(true);
    const DWORD tagA = Frame<MA>();
    Drain();
    WaitFor([] { return gH.inDispatch != 0; }, 3000);
    if (place2 && plant)
    {   // (A's HANDOFF'S STAMP, PLANTED at the arm's instant: the fake's queue holds the hand behind the held dispatch, and the second wait asks for a
        // pair timed by its stamp, as the leg's pairs were)
        const LONG nA = ShB::x.w.openN; FgStamp& e = (FgStamp&)B::S().st[FG_TL_IN][nA & (FG_STAMPS - 1)];
        WriteRelease(&e.v1, -1); e.t = B::S().w.tArm; WriteRelease(&e.v2, nA); WriteRelease(&e.v1, nA);
    }
    g.posAt = a.w.posAt;
    if (riseUs > 0) a.k.kPlanUs = riseUs;                    // (round 3: THE PLAN RISES behind A's arm, ahead of the second position, as a window step may move it several bins)
    const size_t shown0 = MgShownN();
    gPw2Wait0 = a.w.posWaits;
    gPw2Need = how == 4 ? 2 : 1;                              // (how 4: let go in the SECOND wait)
    _InterlockedExchange(&gPw2Seen, -1); _InterlockedExchange(&gPw2SeenPres, -1);
    const LONG f0 = a.w.posFires, got0 = a.w.posWaitGot, na0 = a.w.posNoAns, f20 = a.w.pos2Fires, a20 = a.w.pos2Arms;
    _InterlockedExchange(&a.w.posWaitMaxUs, 0);
    HANDLE h = nullptr;
    Later l = { [] { return ShArmB::a.w.posWaits >= gPw2Wait0 + gPw2Need; }, nullptr, 3000, 0 };
    if (how == 0 || how == 4) l.then = [] { Pw2Snap(); ShRelease(); };
    else if (how == 2) l.then = [] { ShHelperGone(); };
    else if (how == 3) l.then = [] { MA::ArmedStopped(); };
    if (how == 4 && (!place2 || !plant)) l.waitMs = 200;              // (with no second wait to come, soon: the case reads that none came)
    if (l.then) h = Spawn(LaterMain, &l);
    const DWORD t0 = GetTickCount();
    ClockAdd(16.7);
    { EnterCriticalSection(&gFkLock); gPw2Mark = gFk.size(); LeaveCriticalSection(&gFkLock); }
    if (held) { for (int i = 1; i <= 8; i++) D->ColorFill(gOtherRt, nullptr, kPw2Fill | (DWORD)i); }   // (frame B: QUEUED BEHIND THE WORKER, no drain between)
    else Pw2Calls(8);                                         // (frame B: the position is among these)
    ShInject(true);
    const DWORD tagB = Frame<MA>();
    Drain();
    g.ms = GetTickCount() - t0;
    if (h) Join(h, "PW2's hand");
    if (how != 2) { ShRelease(); if (!ShSettle()) Err("PW2: the fake helper did not read frame B's messages"); }
    MgDueN();
    ShArmB::OrdSignal(0x7FFFFF); Drain();
    const auto sh = Shown(); const DWORD genA = ShGenTag(tagA, false);
    int iG = -1, iA = -1, iB = -1;
    for (size_t i = shown0; i < sh.size(); i++) { if (sh[i].tag == genA) { g.genA++; if (iG < 0) iG = (int)i; } if (sh[i].tag == tagA) { g.realA++; if (iA < 0) iA = (int)i; } if (sh[i].tag == tagB) { g.realB++; if (iB < 0) iB = (int)i; } }
    g.inOrder = (iG < 0 || iG < iA) && (iA >= 0 && (iB < 0 || iA < iB)) ? 1 : 0;
    g.fires = a.w.posFires - f0; g.waits = a.w.posWaits - gPw2Wait0; g.got = a.w.posWaitGot - got0; g.noAns = a.w.posNoAns - na0; g.maxUs = a.w.posWaitMaxUs;
    g.fires2 = a.w.pos2Fires - f20; g.arms2 = a.w.pos2Arms - a20; g.bound2 = a.w.pos2BoundUs;
    if (held) { g.seen = gPw2Seen; g.seenPres = gPw2SeenPres; }
    if (held)
    {   // (THE ORDER THE DEVICE TOOK: B's calls each once and in order, and where A's generated frame stands among them: the presents behind the
        // mark are, in order, the presented images behind shown0)
        std::vector<int> seq; Pw2Seq(seq);
        int fill = 0, pres = 0; g.fillsInOrder = 1;
        for (int v : seq)
        {
            if (v) { if (v != fill + 1) g.fillsInOrder = 0; fill = v; continue; }
            const size_t k = shown0 + (size_t)pres++;
            if (k < sh.size() && sh[k].tag == genA && g.genAfter < 0) g.genAfter = fill;
        }
        if (fill != 8) g.fillsInOrder = 0;
    }
    _InterlockedExchange(&ShArmB::placeOn, 0);
    MgClean("PW2");
    MgEnd();
    return g;
}
static void Pw2Say(const char* what, const Pw2Leg& g)
{
    printf("  %s: A's position %ld; waits there %ld (the answer came %ld), no answer %ld; fired at the position %ld (at the second %ld, second positions armed %ld); A's generated frame shown %d time(s), its real frame %d, B's %d, in order %s; frame B took %lu ms",
        what, g.posAt, g.waits, g.got, g.noAns, g.fires, g.fires2, g.arms2, g.genA, g.realA, g.realB, g.inOrder ? "yes" : "NO", (unsigned long)g.ms);
    if (g.seen >= 0) printf("; WHILE THE WORKER WAS HELD the device had seen %ld of B's calls and %ld present(s) behind the mark", g.seen, g.seenPres);
    if (g.genAfter >= 0 || g.fillsInOrder) printf("; A's generated frame reached the device behind B's call %d; B's calls each once and in order %s", g.genAfter, g.fillsInOrder ? "yes" : "NO");
    printf("\n");
}
static void CasePW2()
{
    const char* const name = "PW2 the position: a ride pair's generated frame fired at its position in the next frame's calls, the worker waiting there for the answer behind the order wait; held, killed, a Reset's stop; no stamps, no position (2026-10-17)";
    CaseBegin(name);
    const Pw2Leg g = Pw2Run(1, 0);
    Pw2Say("THE HELPER LET GO DURING THE POSITION'S WAIT", g);
    if (g.posAt != 2 || g.waits != 1 || g.got != 1 || g.fires != 1 || g.genA != 1 || g.realA != 1 || g.realB != 1 || !g.inOrder)
        Err("PW2: position %ld, waits %ld, answered %ld, fired there %ld, A's generated frame %d, real frame %d, B's %d, in order %d (2, 1, 1, 1, 1, 1, 1, yes wanted)", g.posAt, g.waits, g.got, g.fires, g.genA, g.realA, g.realB, g.inOrder);
    const Pw2Leg b = Pw2Run(1, 1);
    Pw2Say("NEVER LET GO", b);
    if (b.waits != 1 || b.got != 0 || b.fires != 0 || b.realA != 1 || !b.inOrder || b.ms > 1000) Err("PW2: with the helper held through the wait: waits %ld, answered %ld, fired there %ld, A's real frame %d, in order %d, frame B %lu ms (1, 0, 0, 1, yes, a second at the most wanted)", b.waits, b.got, b.fires, b.realA, b.inOrder, (unsigned long)b.ms);
    const Pw2Leg k = Pw2Run(1, 2);
    Pw2Say("KILLED DURING THE WAIT", k);
    if (k.waits != 1 || k.realA != 1 || !k.inOrder || k.ms > 1000) Err("PW2: with the helper killed in the wait: waits %ld, A's real frame %d, in order %d, frame B %lu ms (1, 1, yes, a second at the most wanted)", k.waits, k.realA, k.inOrder, (unsigned long)k.ms);
    const Pw2Leg r = Pw2Run(1, 3);
    Pw2Say("A RESET'S STOP DURING THE WAIT", r);
    if (r.waits != 1 || r.got != 0 || r.realA != 1 || !r.inOrder || r.ms > 1000) Err("PW2: with a Reset's stop in the wait: waits %ld, answered %ld, A's real frame %d, in order %d, frame B %lu ms (1, 0, 1, yes, a second at the most wanted)", r.waits, r.got, r.realA, r.inOrder, (unsigned long)r.ms);
    // (phase 5, P3) THE WORKER HELD AT THE POSITION WITH B's CALLS QUEUED BEHIND IT
    const Pw2Leg q = Pw2Run(1, 0, 0, 1);
    Pw2Say("HELD AT THE POSITION, B's CALLS QUEUED BEHIND IT", q);
    if (q.waits != 1 || q.got != 1 || q.fires != 1 || q.seen != q.posAt || q.seenPres != 0 || q.genAfter != q.posAt || !q.fillsInOrder || q.genA != 1 || q.realA != 1 || q.realB != 1 || !q.inOrder)
        Err("PW2 held: waits %ld, answered %ld, fired there %ld; while held the device had seen %ld of B's calls and %ld present(s) (the position's %ld and none wanted); the generated frame behind B's call %d (%ld wanted), B's calls in order %d; A's generated %d, real %d, B's %d, in order %d",
            q.waits, q.got, q.fires, q.seen, q.seenPres, q.posAt, q.genAfter, q.posAt, q.fillsInOrder, q.genA, q.realA, q.realB, q.inOrder);
    // (phase 5, P3) THE SECOND POSITION: let go in its wait, the worker held there with B's calls queued behind it; never let go; the control
    const Pw2Leg s2 = Pw2Run(1, 4, 1, 1, 1, 12000);         // (round 2: the plan that leaves the second wait room before the real frame's earliest due)
    Pw2Say("THE SECOND POSITION, LET GO IN ITS WAIT, HELD THERE", s2);
    if (s2.waits != 2 || s2.got != 1 || s2.noAns != 1 || s2.arms2 != 1 || s2.fires != 1 || s2.fires2 != 1 || s2.seen != 3 || s2.seenPres != 0 || s2.genAfter != 3 || !s2.fillsInOrder || s2.genA != 1 || s2.realA != 1 || s2.realB != 1 || !s2.inOrder)
        Err("PW2 second: waits %ld, answered %ld, no answer %ld, second armed %ld, fired at a position %ld (at the second %ld); while held %ld of B's calls and %ld present(s) (3 and none wanted); the generated frame behind B's call %d (3 wanted), in order %d; A's generated %d, real %d, B's %d, in order %d (2, 1, 1, 1, 1, 1 wanted)",
            s2.waits, s2.got, s2.noAns, s2.arms2, s2.fires, s2.fires2, s2.seen, s2.seenPres, s2.genAfter, s2.fillsInOrder, s2.genA, s2.realA, s2.realB, s2.inOrder);
    const Pw2Leg s2b = Pw2Run(1, 1, 1, 0, 1, 12000);
    Pw2Say("THE SECOND POSITION, NEVER LET GO (the freeze proof)", s2b);
    if (s2b.waits != 2 || s2b.got != 0 || s2b.noAns != 2 || s2b.arms2 != 1 || s2b.fires != 0 || s2b.realA != 1 || !s2b.inOrder || s2b.ms > 1000)
        Err("PW2 second, never let go: waits %ld, answered %ld, no answer %ld, second armed %ld, fired at a position %ld, A's real frame %d, in order %d, frame B %lu ms (2, 0, 2, 1, 0, 1, yes, a second at the most wanted)", s2b.waits, s2b.got, s2b.noAns, s2b.arms2, s2b.fires, s2b.realA, s2b.inOrder, (unsigned long)s2b.ms);
    const Pw2Leg s2n = Pw2Run(1, 4, 1, 1, 0);
    Pw2Say("THE SECOND POSITION, A's HANDOFF NOT STAMPED, THE DEFAULT PLAN (the real frame's earliest due already behind the second position)", s2n);
    if (s2n.waits != 1 || s2n.arms2 != 1 || s2n.fires != 0 || s2n.bound2 != 0 || s2n.realA != 1 || !s2n.inOrder)
        Err("PW2 second, no room before the real due: waits %ld, second armed %ld, fired at a position %ld, the bound %ld, A's real frame %d, in order %d (1: no second wait, 1, 0, 0, 1, yes wanted)", s2n.waits, s2n.arms2, s2n.fires, s2n.bound2, s2n.realA, s2n.inOrder);
    // (round 2, the reviewer's finding 4) THE BOUND IS THE BINDING LIMIT: A's handoff NOT stamped (the stamper lagging past the placer's bound, so the
    // placer would time A from the arm), the helper never let go, and a plan that puts the arm, the plan and half the period less the service time
    // and the margins 2 to 4 ms behind the second position: the second wait IS allowed and ends at that bound, under its own 8 ms; with the plan
    // a millisecond longer the bound is a millisecond longer (the plan is in it); no frame of A's, A's real frame once and in order
    const Pw2Leg bb = Pw2Run(1, 1, 1, 0, 0, 12000), bb2 = Pw2Run(1, 1, 1, 0, 0, 13000);
    Pw2Say("THE SECOND POSITION'S BOUND BINDS, A's HANDOFF NOT STAMPED, THE PLAN 12 ms", bb);
    Pw2Say("THE SAME, THE PLAN 13 ms", bb2);
    printf("  the second wait's bound: %ld us with the plan at 12 ms, %ld us at 13 ms (its own bound 8,000 us)\n", bb.bound2, bb2.bound2);
    if (bb.waits != 2 || bb.arms2 != 1 || bb.fires != 0 || bb.bound2 <= 0 || bb.bound2 >= 8000 || bb.realA != 1 || !bb.inOrder
        || bb2.waits != 2 || bb2.bound2 - bb.bound2 < 900 || bb2.bound2 - bb.bound2 > 1100)
        Err("PW2 the bound: waits %ld and %ld, second armed %ld, fired at a position %ld, the bound %ld and %ld us, A's real frame %d, in order %d (2 each, 1, 0, a bound inside 8,000 us that grows by the plan's millisecond within 100 us, 1, yes wanted)",
            bb.waits, bb2.waits, bb.arms2, bb.fires, bb.bound2, bb2.bound2, bb.realA, bb.inOrder);
    // (round 3, the reviewer's finding: THE PLAN RISES SINCE THE ARM, 12 to 20 ms, thirty-two bins: the placer timed A with the plan of its arm or
    // a later one, so the bound must be the one of the smaller: the 12 ms leg's above, to 100 us, and never the risen plan's)
    const Pw2Leg br = Pw2Run(1, 1, 1, 0, 0, 12000, 20000);
    Pw2Say("THE SECOND POSITION'S BOUND, THE PLAN RISEN FROM 12 TO 20 ms SINCE A's ARM", br);
    printf("  the second wait's bound with the plan risen since the arm: %ld us (the 12 ms plan's %ld us)\n", br.bound2, bb.bound2);
    if (br.waits != 2 || br.fires != 0 || br.bound2 <= 0 || labs(br.bound2 - bb.bound2) > 100 || br.realA != 1 || !br.inOrder)
        Err("PW2 the risen plan: waits %ld, fired at a position %ld, the bound %ld us where the plan of the arm gives %ld, A's real frame %d, in order %d (2, 0, the arm's bound within 100 us, 1, yes wanted)",
            br.waits, br.fires, br.bound2, bb.bound2, br.realA, br.inOrder);
    const RingStats stc = gRingSt;
    const Pw2Leg c = Pw2Run(0, 0);
    const Pw2Leg c2 = Pw2Run(1, 4, 0, 1);
    ViolRestore(stc);
    Pw2Say("CONTROL, no stamps", c);
    if (c.posAt != 0 || c.waits != 0 || c.fires != 0 || c.realA != 1) Err("PW2 control: with no stamps position %ld, waits %ld, fired there %ld, A's real frame %d (0, 0, 0, 1 wanted): the control shows nothing", c.posAt, c.waits, c.fires, c.realA);
    Ctl("no stamps: no position, no wait there, the clock due as stage 2");
    Pw2Say("CONTROL, the second position not asked for", c2);
    if (c2.arms2 != 0 || c2.fires2 != 0 || c2.waits != 1 || c2.fires != 0) Err("PW2 control: with place2 at 0 second positions armed %ld, fired there %ld, waits %ld, fired at a position %ld (0, 0, 1, 0 wanted): the control shows nothing", c2.arms2, c2.fires2, c2.waits, c2.fires);
    Ctl("place2 at 0: one wait, no second position, no generated frame fired at a position");
    CaseEnd(name);
}
