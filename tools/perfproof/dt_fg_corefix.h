// dt_fg_corefix.h: TWO FINDINGS OF 1.1'S FRAME GENERATION CORE AUDIT, EACH HELD BY A CASE WITH ITS CONTROL (2026-10-28, branch fg19-corefix).
//
// Included by dt_fg_test.cpp behind dt_fg_linux.h, with ONE line; its cases are HU14 and LW54.
#pragma once

// ---------------------------------------------------------------- HU14, a Reset with the hand's HUD-less copy still on the card
// THE FINDING (the core audit, 2026-10-28): hkReset (source/DLSS-IV.cpp) runs the device thread's barrier first, which DISARMS the layer, and
// then HdrEncodeRelease, whose FgHudTargetGive(true) (source/fg/fg_hud.h) gives the encode's second target back to DXVK. Its wait for the ring
// stands inside `DtSyncScope direct(...)` and runs only where that scope is on; with the layer disarmed it is off, so the target went back
// while the hand's raw copy out of it (FgPA::RecCopyHudToImg, a submit DXVK does not track) could still be on the card. The machine's own
// retire wait (FgpOnResetBarrier: BeforeReset's RetireWait(2000)) ran only further down, just before origReset. THE MEND: hkReset calls the
// machine's barrier BEFORE HdrEncodeRelease, so every submit of the ring the queue took has retired before any texture of the mod's goes
// (tools/check-fg-fence.py holds that order in hkReset's own text: a row that fails on a19c680b).
// WHAT THIS CASE SHOWS, ON THE MACHINE'S OWN RESET PATH (FgMachine::BeforeReset over the merged binding, the fake ring's retire wait): a pair
// in the second-target form whose hand's one span copied the binding's image into the helper's HUDLESS, THAT SUBMIT HELD ON THE CARD
// (FgHarnessBinding::cardHold: taken, applied, not retired). Then hkReset's steps as the mend orders them: the device thread's barrier on the
// render thread (the layer disarmed, the ring drained), the machine's barrier, and only then THE RELEASE, observed: no submit that names the
// binding's image is still on the card there. The card finishes 60 ms into the machine's wait (a thread of the case's), so the wait is seen
// to hold the release back by that long, and it returns with the machine live (no time-out). CONTROL, THE ORDER OF a19c680b: the release
// observed right behind the barrier, ahead of the machine's wait: the hand's copy out of that image is ON THE CARD there.
// THE CASE MODELS hkReset'S ORDER, IT DOES NOT READ IT: the order itself is held as text (check-fg-fence.py's row on hkReset), which is the
// stand-in rule (memory: a harness stand-in proves the model; the real writer is read as text).
struct Hu14Leg { int copied, onAtHand, onAtRelease, r, state; ULONGLONG waitedMs; const char* how; };
static DWORD WINAPI Hu14CardDone(void*) { Sleep(60); B::CardRelease(); return 0; }
static Hu14Leg Hu14Run(bool mendOrder)
{
    Hu14Leg g = {};
    MgFresh<MA>(); HuTexOn(); ShWarm<MA>(); MgDueN();
    const LONG tcop0 = ShArmB::hudTexCopies;
    // THE PAIR, ITS HAND'S SUBMIT HELD ON THE CARD
    _InterlockedExchange(&B::cardHold, 1);
    ShFrame<MA>();
    g.copied = (int)(ShArmB::hudTexCopies - tcop0);
    const int src = ShArmB::HudSrcImg();
    g.onAtHand = B::OnCard(src);
    // hkReset: THE DEVICE THREAD'S BARRIER (on the render thread: the producer's own, the layer disarmed)
    DWORD ms = 0;
    g.how = DtResetBarrier(&ms);
    HANDLE card = Spawn(Hu14CardDone);
    const ULONGLONG t0 = GetTickCount64();
    if (mendOrder)
    {
        g.r = MA::BeforeReset(dtArmed != 0, g.how);          // FgpOnResetBarrier(rsHow), where the mend puts it
        g.waitedMs = GetTickCount64() - t0;
        g.onAtRelease = B::OnCard(src);                       // HdrEncodeRelease -> FgHudRelease -> FgHudTargetGive(true)
    }
    else
    {
        g.onAtRelease = B::OnCard(src);                       // a19c680b: the release right behind the barrier
        g.r = MA::BeforeReset(dtArmed != 0, g.how);
        g.waitedMs = GetTickCount64() - t0;
    }
    g.state = B::S().sh.state;
    Join(card, "HU14's card");
    if (B::cardHold) B::CardRelease();
    // THE RESET ITSELF, THEN THE LAYER ARMS AGAIN AND A FRAME RUNS, as the game's next frames would
    D3DPRESENT_PARAMETERS pp = ResetParams(); D->Reset(&pp);
    if (!MgEnsureArmed<MA>()) Err("HU14: the layer did not arm again behind the Reset");
    ShFrame<MA>(); MgDueN();
    MgEnd();
    return g;
}
static void CaseHU14()
{
    const char* const name = "HU14 a Reset with the hand's HUD-less copy still on the card: the machine's barrier before the release of the encode's second target (2026-10-28, core audit finding 1)";
    CaseBegin(name);
    const Hu14Leg a = Hu14Run(true);
    printf("  THE MEND'S ORDER: the hand copied out of the binding's image %d time(s), %d submit copy(ies) naming it on the card behind the hand; the barrier: %s;"
           " the machine's barrier returned %d in %llu ms, the machine %s; at the release %d copy(ies) naming it still on the card\n",
           a.copied, a.onAtHand, a.how, a.r, (unsigned long long)a.waitedMs, a.state == FG_LIVE ? "live" : "NOT LIVE", a.onAtRelease);
    if (a.copied != 1 || a.onAtHand < 1) Err("HU14: the hand copied out of the binding's image %d time(s) and %d copy(ies) naming it stood on the card (1 and 1 or more wanted: the case is not the one the finding names)", a.copied, a.onAtHand);
    if (strcmp(a.how, "the producer's own")) Err("HU14: the barrier read \"%s\" (the producer's own wanted: hkReset on the render thread)", a.how);
    if (a.onAtRelease != 0) Err("HU14: %d copy(ies) out of the encode's second target still on the card at its release (0 wanted: the machine's retire wait stands ahead of it)", a.onAtRelease);
    if (a.r != 0 || a.waitedMs < 50 || a.state != FG_LIVE) Err("HU14: the machine's barrier returned %d in %llu ms and the machine is %s (0, 50 ms or more: it waited for the card, and live wanted)", a.r, (unsigned long long)a.waitedMs, a.state == FG_LIVE ? "live" : "not live");
    const RingStats stc = gRingSt;
    const Hu14Leg c = Hu14Run(false);
    ViolRestore(stc);
    printf("  CONTROL, a19c680b's ORDER (the release behind the barrier, the machine's barrier after it): %d copy(ies) naming the target on the card at its release; the machine's barrier then returned %d in %llu ms\n",
           c.onAtRelease, c.r, (unsigned long long)c.waitedMs);
    if (c.onAtRelease < 1) Err("HU14 control: a19c680b's order released the target with %d copy(ies) out of it on the card (1 or more wanted): the control shows nothing", c.onAtRelease);
    else Ctl("a19c680b's order: the encode's second target released with %d copy(ies) out of it still on the card", c.onAtRelease);
    CaseEnd(name);
}

// ---------------------------------------------------------------- LW54, late handoffs, the floors and the come-back
// THE FINDING (the core audit, 2026-10-28): the room floor and the ride's floors (fg_plan.h, FgRoomTake and FgRoomFloor; fg_machine_armed.h,
// ArmedRoom and ArmedRide*) move only by pairs and are zeroed only at the install, so a floor raised by a burst of late handoffs (a GPU hitch,
// DXVK compiling pipelines) would stand through the stand-down it caused; the next stint would be held, file c as an upper bound, and
// FGC_ONSLOW keep frame generation off. ASKED FOR: a case that FAILS on a19c680b. IT DOES NOT, and this case says why, leg by leg.
// THE SCENE: LW52's busy card (by itself at 61, 16.4 ms; 6.5 ms laid on every engaged frame: R_on about 44), the room seed and the teaching
// from the arm (the armed product's keys; it holds the arm's teaching off where the placement by the card is on, so it takes FEWER late needs
// than this), the law as the product runs it. Everything planted behind the stint's warm-up and cost windows (c filed from its samples), at
// its 12th window. RtSim::lsNext plants a handoff; on the card's one queue each holds the handoffs behind it too.
// (A) THE BURST ASKED FOR: SIX HANDOFFS OF 50 TO 90 ms, three at the end of a window and three at the start of the next (the order that leaves
//     them the newest of the floor's history if a stand-down came). The placer never takes such a need: a pair whose stamp has not come by
//     half a period behind its arm (a whole one for a ride) is timed from the arm, and its late stamp teaches only within two periods.
//     HELD: the floors two windows behind the burst as they stood before it; no stand-down; ON at the end.
// (B) A STRETCH THE PLACER DOES TAKE: 190 handoffs of 16 ms (about four seconds of a slower card): the room floor rises, the stint is held,
//     FGC_SLOW stands it down. HELD: ON again within 10 windows (LW52's quick come-back), ON at the end, no window of the come-back's stint
//     under 40 behind its warm-up. TOLD: the floors at the stand-down (the room floor has come back by then; the ride's floor, taught by
//     ride pairs alone, stands until the next ride).
// (C) THE FLOORS THE FINDING DESCRIBES, PLANTED AT (B)'s STAND-DOWN: six needs of 50 to 90 ms the newest of the room floor's history, the
//     floor at its rise (the pacer clamps at 40 ms). HELD as (B), and the room floor back under 20 ms one window behind the engage: the
//     come-back's own pairs give it back (they teach from the 13th, FG_RIDE_SKIP, inside its warm-up). TOLD: the engage's window's rate.
// THE LOCK-OUT IS c's, NOT THE FLOORS': a c filed only from windows a floor of the pacer's held is an upper bound (fg_cond.h, FgDriveStep),
// and until 2026-10-28 it kept FGC_ONSLOW standing to the next Reset or build. On a19c680b's fg_cond.h (D) below never came back (ON again
// at window -1, STANDBY at the end). THE MEND (fg_cond.h, FgDriveBoundAge): standing by, the bound ages one window each time the game's own
// rate is read, and past FG_DRIVE_BOUND_WIN it is dropped, once a lineage.
// (D) (C) WITH c PLANTED AS AN UPPER BOUND OF 30 ms AT THE STAND-DOWN (marked a bound, as a stint a floor held through its cost windows
//     files it): HELD: ON again within 10 + FG_DRIVE_BOUND_WIN windows (the quick come-back behind the bound's own windows), ON at the end,
//     no window of the come-back under 40 behind its warm-up (its own c is measured, a sample).
// (E) THE BOUND BEHIND A DROP (planted KEPT, as the next stint behind a drop files one): HELD: never ON again (once a lineage: no cycling).
// CONTROL, THE RULE OF a19c680b: (D) with the bound planted again behind every window standing by (it never ages): never ON again.
enum { LW54_BURST = 0, LW54_STRETCH = 1, LW54_STALE = 2, LW54_CBOUND = 3, LW54_TODAY = 4, LW54_KEPT = 5 };
struct Lw54Leg { std::vector<LwRec> w; std::vector<LONG> room, ride; LwFrames f; int burstAt, downAt, downRow, backAt, endState, engages, late, slowBack; LONG roomDown, rideDown, roomPlanted; double backFps; };
static Lw54Leg Lw54Run(int leg)
{
    Lw54Leg g = {}; g.burstAt = g.downAt = g.backAt = -1;
    RtPar par = Lw9Par(16.4, 6.5);
    par.roomSeed = 1; par.armTeach = 1;                       // (the armed product's: fg_arm_life.h, FgpArmKeys)
    RtBegin(par);
    gRtSeed = 20261028u;
    LwLaw L; LwLawBegin(L, FgDriveDefaults(), LwSceneK()); L.card = 1;
    int onIdx = 0;
    static const LONG kLate[6] = { 50000, 58000, 66000, 74000, 82000, 90000 };
    for (int win = 0; win < 60; win++)
    {
        const bool on = L.mem.state == FGDR_ON;
        onIdx = on ? onIdx + 1 : 0;
        const int bw = (g.burstAt < 0 && on && onIdx == 12) ? 1 : (g.burstAt >= 0 && win == g.burstAt + 1 && on) ? 2 : 0;
        if (bw == 1) g.burstAt = win;
        const int first = (int)gRt.pairs.size();
        for (int i = 0; i < 120; i++)
        {
            if (leg == LW54_BURST)
            {
                if (bw == 1 && (i == 100 || i == 107 || i == 114)) gRt.lsNext = kLate[(i - 100) / 7];
                if (bw == 2 && (i == 0 || i == 7 || i == 14)) gRt.lsNext = kLate[3 + i / 7];
            }
            else if ((bw == 1 && i >= 40) || (bw == 2 && i < 110)) gRt.lsNext = 16000;
            RtFrame(RtMs(16.4 * RtSpread(0.01)));
            LwLawPoll(L);
        }
        const LwRec r = LwLawWindow(L, par, first, (int)gRt.pairs.size());
        g.w.push_back(r); g.room.push_back(ReadAcquire(&ShArmB::a.pl.roomUs)); g.ride.push_back(ReadAcquire(&ShArmB::a.pl.rideUs));
        if (g.downAt < 0 && g.burstAt >= 0 && r.prev == FGDR_ON && r.state != FGDR_ON)
        {
            g.downAt = win; g.downRow = r.row; g.roomDown = g.room.back(); g.rideDown = g.ride.back();
            if (leg >= LW54_STALE)
            {   // (the placer is idle standing by: the harness writes its history as the placer's own FgRoomTake would)
                FgArmedPlacer& pl = ShArmB::a.pl; LONG up = 0, down = 0;
                for (int j = 0; j < 6; j++) FgRoomTake(pl.room, kLate[j], &up, &down);
                pl.roomUpUs = up; pl.roomDownUs = down;
                WriteRelease(&pl.roomUs, FgRoomFloor(ReadAcquire(&pl.roomUs), up, down, ReadAcquire(&ShArmB::a.k.kPlanUs) + ShArmB::a.k.roomGuardUs, 250));
                g.roomPlanted = pl.roomUs;
            }
            if (leg >= LW54_CBOUND) { L.mem.cLimUs = 30000; L.mem.cLimErrUs = 0; L.mem.cLimBound = leg == LW54_KEPT ? (int)FG_DRIVE_BOUND_KEPT : 1; }
        }
        else if (leg == LW54_TODAY && g.downAt >= 0 && L.mem.state != FGDR_ON) { L.mem.cLimUs = 30000; L.mem.cLimErrUs = 0; L.mem.cLimBound = 1; }   // (a19c680b: it never ages)
        if (g.downAt >= 0 && g.backAt < 0 && L.mem.state == FGDR_ON) { g.backAt = win; g.backFps = r.med > 0.0 ? 1000.0 / r.med : 0.0; }
        // (the come-back's stint behind its warm-up: a window under the floor's 40 there is a stint held by something)
        if (g.backAt >= 0 && win > g.backAt + 3 && L.mem.state == FGDR_ON && r.med > 0.0 && 1000.0 / r.med < 40.0) g.slowBack++;
    }
    const LONGLONG q = B::Qpf();
    for (const RtPair& p : gRt.pairs) if (p.tRet && p.tEntry && (p.tRet - p.tEntry) * 1000 / q >= 40) g.late++;
    g.endState = L.mem.state; g.engages = L.mem.engages;
    g.f = LwFramesRead();
    B::S().wt.want = FG_WANT_ON;
    Lw9ParEnd();
    RtEnd("LW54");
    return g;
}
static void Lw54Say(const char* what, const Lw54Leg& g)
{
    LwPrint(what, g.w);
    printf("  window  room floor us  ride floor us\n");
    for (size_t i = 0; i < g.w.size(); i++)
        if ((int)i >= g.burstAt - 1 && (int)i <= (g.backAt >= 0 ? g.backAt + 2 : g.burstAt + 3)) printf("  %5d  %13ld  %13ld\n", (int)i, g.room[i], g.ride[i]);
    printf("  the plant from window %d (%d handoffs 40 ms or more behind their entries over the run); the stand-down behind window %d (%s), the floors there %ld and %ld us%s;"
           " ON again at window %d (that window %.1f real a second); engages %d, %s at the end; windows of the come-back's stint under 40 behind its warm-up %d; %d frames, real frames shown once %d, out of place %d\n",
           g.burstAt, g.late, g.downAt, g.downAt >= 0 ? FgCondEnumName(g.downRow) : "none", g.roomDown, g.rideDown, g.roomPlanted ? " (the room floor planted at the finding's)" : "",
           g.backAt, g.backFps, g.engages, FgDriveStateName(g.endState), g.slowBack, g.f.n, g.f.real1, g.f.order);
}
static bool Lw54Back(const char* tag, const Lw54Leg& g)
{
    bool ok = true;
    if (g.downAt < 0 || g.downRow != FGC_SLOW) { Err("LW54 %s: the stand-down behind window %d by %s (one by FGC_SLOW wanted)", tag, g.downAt, g.downAt >= 0 ? FgCondEnumName(g.downRow) : "none"); ok = false; }
    if (g.backAt < 0 || g.backAt > g.downAt + 10) { Err("LW54 %s: stood down behind window %d, ON again at window %d (within 10 windows wanted: the quick come-back)", tag, g.downAt, g.backAt); ok = false; }
    if (g.endState != FGDR_ON || g.slowBack) { Err("LW54 %s: %s at the end, %d window(s) of the come-back's stint under 40 behind its warm-up (ON and none wanted)", tag, FgDriveStateName(g.endState), g.slowBack); ok = false; }
    return ok;
}
static void Lw54Frames(const char* tag, const Lw54Leg& g)
{
    if (g.f.order || g.f.real1 != g.f.n) Err("LW54 %s: %d picture(s) out of place (%s), %d of %d real frames shown exactly once", tag, g.f.order, g.f.first.c_str(), g.f.real1, g.f.n);
}
static void CaseLW54()
{
    const char* const name = "LW54 late handoffs, the floors and the come-back: a burst of 50 to 90 ms moves no floor; a stretch the placer takes, and the finding's floors planted, come back within 10 windows; a c filed as an upper bound ages out, once a lineage (2026-10-28, core audit finding 2)";
    CaseBegin(name);
    const Lw54Leg a = Lw54Run(LW54_BURST);
    Lw54Say("(A) 61 BY ITSELF ON A BUSY CARD (6.5 ms laid on), SIX HANDOFFS OF 50 TO 90 ms ACROSS THE STINT'S 12th AND 13th WINDOWS:", a);
    if (a.burstAt < 0 || a.late < 6) Err("LW54 (A): the burst from window %d, %d late handoff(s) (a burst and 6 or more wanted: the case is not the one asked for)", a.burstAt, a.late);
    else if (a.room[a.burstAt + 2] != a.room[a.burstAt - 1] || a.ride[a.burstAt + 2] != a.ride[a.burstAt - 1] || a.downAt >= 0 || a.endState != FGDR_ON)
        Err("LW54 (A): the floors %ld and %ld us two windows behind the burst (%ld and %ld before it), a stand-down behind window %d, %s at the end (as before, none and ON wanted)",
            a.room[a.burstAt + 2], a.ride[a.burstAt + 2], a.room[a.burstAt - 1], a.ride[a.burstAt - 1], a.downAt, FgDriveStateName(a.endState));
    Lw54Frames("(A)", a);
    const Lw54Leg b = Lw54Run(LW54_STRETCH);
    Lw54Say("(B) THE SAME, 190 HANDOFFS OF 16 ms FROM THE STINT'S 12th WINDOW (a card slower for about four seconds):", b);
    if (b.burstAt < 0 || b.room[b.burstAt] <= b.room[b.burstAt - 1]) Err("LW54 (B): the room floor %ld us in the stretch's first window, %ld before it (a rise wanted: the case is not the one it names)", b.burstAt >= 0 ? b.room[b.burstAt] : 0L, b.burstAt > 0 ? b.room[b.burstAt - 1] : 0L);
    Lw54Back("(B)", b);
    Lw54Frames("(B)", b);
    const Lw54Leg c = Lw54Run(LW54_STALE);
    Lw54Say("(C) (B) WITH THE FINDING'S FLOORS PLANTED AT THE STAND-DOWN (six needs of 50 to 90 ms the newest of the room floor's history):", c);
    if (c.roomPlanted < 40000) Err("LW54 (C): the room floor planted at %ld us (40,000 or more wanted: the floor the finding names)", c.roomPlanted);
    if (Lw54Back("(C)", c) && c.room[c.backAt + 1] >= 20000) Err("LW54 (C): the room floor %ld us one window behind the engage (under 20,000 wanted: the come-back's pairs give it back)", c.room[c.backAt + 1]);
    Lw54Frames("(C)", c);
    const Lw54Leg d = Lw54Run(LW54_CBOUND);
    Lw54Say("(D) (C) WITH c PLANTED AS AN UPPER BOUND OF 30 ms AT THE STAND-DOWN:", d);
    if (d.downAt < 0 || d.backAt < 0 || d.backAt > d.downAt + 10 + (int)FG_DRIVE_BOUND_WIN || d.endState != FGDR_ON || d.slowBack)
        Err("LW54 (D): with c an upper bound, stood down behind window %d, ON again at window %d, %s at the end, %d slow window(s) behind the warm-up (within %d windows, ON and none wanted: the bound ages out)",
            d.downAt, d.backAt, FgDriveStateName(d.endState), d.slowBack, 10 + (int)FG_DRIVE_BOUND_WIN);
    Lw54Frames("(D)", d);
    const Lw54Leg e = Lw54Run(LW54_KEPT);
    printf("  (E) THE BOUND BEHIND A DROP (planted KEPT): stood down behind window %d, ON again at window %d, %s at the end\n", e.downAt, e.backAt, FgDriveStateName(e.endState));
    if (e.downAt < 0 || e.backAt >= 0) Err("LW54 (E): a kept bound: stood down behind window %d, ON again at window %d (never wanted: once a lineage, no cycling)", e.downAt, e.backAt);
    Lw54Frames("(E)", e);
    const RingStats stc = gRingSt;
    const Lw54Leg k = Lw54Run(LW54_TODAY);
    ViolRestore(stc);
    printf("  CONTROL, a19c680b's RULE ((D) with the bound planted again behind every window standing by): stood down behind window %d, ON again at window %d, %s at the end\n", k.downAt, k.backAt, FgDriveStateName(k.endState));
    if (k.downAt < 0 || k.backAt >= 0) Err("LW54 control: a bound that never ages: stood down behind window %d and ON again at window %d (never wanted): the control shows nothing", k.downAt, k.backAt);
    else Ctl("a bound that never ages (a19c680b's rule): stood down behind window %d, never ON again", k.downAt);
    CaseEnd(name);
}
