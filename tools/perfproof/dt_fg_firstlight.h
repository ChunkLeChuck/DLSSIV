// dt_fg_firstlight.h: FIRST LIGHT'S COPY MODE, IN THE HARNESS (2026-10-10, phase 3 of the revival, the build map's stage 10;
// the owner's decision of that day: the first picture is a leg that shows a copy of the real frame in the generated slot).
//
// Included by dt_fg_test.cpp behind dt_fg_threads.h, with ONE line; its case is FL1 (one RUN line in RunCases, twice: the
// two orders of 'Q').
//
// WHAT IS NEW IN THE MACHINE. One key more in FgArmedKeys, slotCopy, and one place that reads it: ArmedFireI hands the
// fire HOLD as its source image where it handed GEN. Nothing else of the fire knows: the answer is still read by
// equality, the back buffer is still saved, written, presented and put back, the generated frame is still counted
// SHOWN. The product sets the key from the development key fg_slot (source/fg/fg_arm_life.h, FgpArmKeys), in the armed
// development build only; every other binding leaves it 0.
//
// WHAT FL1 SHOWS. M1's plain pair, its two dues written by the harness, with the key at 1: the fire in the generated
// slot presents THE PAIR'S REAL FRAME, the frame's own tag, and then the real frame's fire presents it again; the
// helper's GEN holds the generated image all the same (it was made and nobody showed it); on the worker the frame
// reads what a plain pair reads; the back buffer after each fire is the back buffer before it.
// CONTROL: the same pair with the key at 0. The slot shows the helper's generated image, so the case's own test, "the
// slot showed the frame", fails on it as it must.
#pragma once

struct Fl1Leg { DWORD tag, sI, sN, gen, hold, bb0, bb1, bb2; LONG pres, counted; size_t shown; std::string seq; bool armed; };
static Fl1Leg Fl1Run(LONG slotCopy)
{
    Fl1Leg g = {};
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    a.k.slotCopy = slotCopy;
    ShWarm<MA>(); MgDueN();                                   // (the last pair of the warm-up brought to its real frame: nothing is pending)
    const LONG pres0 = gRingSt.presents, counted0 = a.w.iWhy[FG_AW_SHOWN]; const size_t sq0 = ShB::seq.size();
    g.tag = ShFrame<MA>();
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    g.bb0 = MgBb();
    MgDueShow();
    g.bb1 = MgBb();
    g.gen = RingTag(RingRig(FG_IMG_GEN)); g.hold = RingTag(RingRig(FG_IMG_HOLD));
    MgDueN();
    g.bb2 = MgBb();
    g.seq.assign(ShB::seq.begin() + sq0, ShB::seq.end());
    g.pres = gRingSt.presents - pres0;
    g.counted = a.w.iWhy[FG_AW_SHOWN] - counted0;
    const auto shown = Shown();
    g.sI = shown.size() > shown0 ? shown[shown0].tag : 0; g.sN = shown.size() > shown0 + 1 ? shown[shown0 + 1].tag : 0;
    g.shown = shown.size() - shown0;
    ShFrame<MA>(); MgDueN();
    MgClean("FL1");
    MgEnd();
    return g;
}
static void CaseFL1()
{
    const char* name = "FL1 first light's copy mode: the generated slot shows the pair's real frame from the hold, and the rest of the fire is the same (stage 10)";
    CaseBegin(name);
    const Fl1Leg g = Fl1Run(1);
    printf("  a plain pair with the copy key at 1, armed %s: on the worker it reads %s; %ld DXVK present(s), showing %lX in the generated slot and then %lX (the frame is %lX, its generated image %lX);"
           " HOLD %lX, GEN %lX; the back buffer %lX before, %lX after the slot's fire, %lX after the real frame's; generated frames counted shown %ld\n",
        g.armed ? "yes" : "NO", g.seq.c_str(), g.pres, g.sI, g.sN, g.tag, ShGenTag(g.tag, false), g.hold, g.gen, g.bb0, g.bb1, g.bb2, g.counted);
    if (!g.armed || g.seq != "QSGSSSS") Err("FL1: the pair was %s and reads %s on the worker (QSGSSSS wanted, a plain pair's: 'Q', the hand's one submit, 'G', the write and the restore of each fire)", g.armed ? "armed" : "NOT armed", g.seq.c_str());
    if (g.pres != 2 || g.shown != 2 || g.sI != g.tag || g.sN != g.tag) Err("FL1: the pair made %ld DXVK present(s) and the screen showed %zu picture(s), %lX then %lX (2, 2 and the frame %lX both times wanted)", g.pres, g.shown, g.sI, g.sN, g.tag);
    if (g.hold != g.tag || g.gen != ShGenTag(g.tag, false)) Err("FL1: HOLD holds %lX (the frame %lX wanted) and GEN %lX (the pair's generated image %lX wanted: the helper generates as ever, and the copy mode only does not show it)", g.hold, g.tag, g.gen, ShGenTag(g.tag, false));
    if (g.bb1 != g.bb0 || g.bb2 != g.bb0) Err("FL1: the back buffer read %lX before the fires, %lX after the slot's and %lX after the real frame's: a fire must leave it as it found it", g.bb0, g.bb1, g.bb2);
    if (g.counted != 1) Err("FL1: %ld generated frame(s) counted shown for the pair (1 wanted: the slot's fire is counted as any other)", g.counted);
    // CONTROL
    const Fl1Leg c = Fl1Run(0);
    if (c.sI == c.tag || c.sI != ShGenTag(c.tag, false) || c.sN != c.tag)
        Err("FL1 control: with the copy key at 0 the slot showed %lX and then %lX (the generated image %lX and then the frame %lX wanted): the control shows nothing", c.sI, c.sN, ShGenTag(c.tag, false), c.tag);
    Ctl("the same pair with the copy key at 0: the slot showed %lX, the helper's generated image, where the frame is %lX", c.sI, c.tag);
    CaseEnd(name);
}
