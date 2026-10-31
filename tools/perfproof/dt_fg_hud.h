// dt_fg_hud.h: THE HUD-LESS PICTURE, IN THE HARNESS (2026-10-10, phase 3 of the revival, the build map's stage 15;
// docs/reports/fg-3/map-2026-10-06: critic.md X13 and U13, machine.md machine-4, presentation.md presentation-12;
// docs/reports/fg-3/merge-2026-10-10/READY-FOR-7C.md, section 5, question 3: "the day a HUD-less picture is drawn there,
// an inline pair would show it, and BackMark would be one draw early ... that pass's restore must read HOLD").
//
// Included by dt_fg_test.cpp behind dt_fg_firstlight.h, with ONE line; its cases are HU1 to HU6 (one RUN line in
// RunCases, twice: the two orders of 'Q'). HU6 is the merged path's census run WITH the picture (dt_fg_merged.h's
// McSegment behind gMcHud, which adds no draw to the census's own random stream while it is 0).
//
// WHAT IS NEW IN THE MACHINE (fg_machine.h, "THE HUD-LESS PICTURE"). Three seams of the binding on the render thread
// (HudWant before the hand, HudDraw behind it, HudUndo where the HUD-less note was refused); three flags of the hand;
// the hand's span with `pass` (the real frame out, no HUDLESS, no fgIn); the HUD-less note's own span (HUDLESS and fgIn,
// then HOLD back into the back buffer), run FIRST OF ALL at the note and whatever became of the pair; a store for a hand
// that opens no pair; a present that is not made where the picture could not be taken back; and one reason more a
// generated frame is not shown (no HUD-less picture).
//
// HOW THE FAKE SEES IT. Its back buffer is one tag. A frame is a fill with the frame's tag; THE HUD-LESS PICTURE IS A
// SECOND FILL WITH A TAG OF ITS OWN (ShArmB::HudDraw, recorded on the render thread behind the hand, as the mod records
// its pass), the build map's own proof for machine-4. So "the helper's HUDLESS holds the HUD-less picture" and "the
// screen never showed the HUD-less picture" are each one compare of tags.
#pragma once

static void HuOn() { ShB::hasHudless = 1; ShArmB::hud = FG_HUD_PASS; }
static volatile LONG gHuSeen = -1;
static void HuLook() { gHuSeen = (LONG)RingPresentedLog().size(); }

// ---------------------------------------------------------------- HU1, a pair with a HUD-less picture
// WHAT IT SHOWS. THE PLAN'S ORDER ON THE HOLD. A plain pair whose frame has an interface, the worker HELD with the whole
// frame queued behind it (the injection, the fill, the hand, the picture's fill, the HUD-less note, the Present): nothing
// reaches the screen while it is held. Released: on the worker the frame reads 'Q', the hand's one submit, THE NOTE'S
// TWO (the picture into HUDLESS with fgIn; HOLD back into the back buffer), 'G', then the fires' four. The helper's
// PRESENT and the mod's HOLD hold the frame, the helper's HUDLESS holds THE HUD-LESS PICTURE, and the back buffer holds
// THE FRAME again before the pair is armed. The screen shows the pair's generated image and then the frame, and never
// the HUD-less picture.
// A PAIR WITH NO SOURCE (the helper held inside the dispatch before it): the helper's HUDLESS is LEFT AS IT IS READING IT,
// the frame is put back all the same, and the pair keeps its place.
// A FRAME WITH NO INTERFACE (the binding says SAME): no picture is drawn; the hand's one span copies the frame into
// HUDLESS too, and the frame reads as a plain pair always read.
// THE MARK (the fake's knowledge of the back buffer's writes): the arm marks the back buffer AFTER the picture was taken
// back, so neither of the pair's fires finds it written and neither saves it.
// CONTROLS: the restore made to read the helper's PRESENT (the shared store, the rig's form) on the pair with no source:
// the back buffer gets the frame BEFORE. And the mark made at the hand (a note of the harness's): the picture counts
// as a write of the game's and both fires save.
struct HuFromPresentB : ShArmB
{
    static inline volatile LONG redirect = 0;
    static void RecCopyFromImg(int img, ULONGLONG bb) { ShArmB::RecCopyFromImg((redirect && img == FG_IMG_HOLD) ? (int)FG_IMG_PRESENT : img, bb); }
};
typedef FgMachine<HuFromPresentB> MAHP;
struct HuMarkEarlyB : ShArmB { static void BackMark() {} };
typedef FgMachine<HuMarkEarlyB> MAHM;
static void HuMarkNote(DWORD, DWORD, DWORD) { RingBackMark(); }
struct Hu1NoSrc { DWORD tagB, tagC, hudB, hudC, hudImg, holdImg, bb; LONG n; bool armed, held; int kind; LONG inBusy, back; std::string tail; bool order; };
template <class MM> static Hu1NoSrc Hu1NoSource(bool redirect)
{
    Hu1NoSrc g = {};
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a; FgState& s = B::S();
    _InterlockedExchange(&gH.holdDispatch, 1);
    g.tagB = ShFrame<MM>(true, false); g.hudB = ShArmB::hudTag;
    if (!WaitFor([] { return gH.inDispatch != 0; }, 3000)) Err("HU1: the fake helper never reached the held dispatch");
    const LONG busy0 = gRingSt.inBusy, back0 = a.w.hudBack; const size_t sh0 = MgShownN();
    HuFromPresentB::redirect = redirect ? 1 : 0;
    g.tagC = ShFrame<MM>(true, false); g.hudC = ShArmB::hudTag; g.n = x.w.lastQN;
    HuFromPresentB::redirect = 0;
    g.armed = s.w.openPhase == FG_PAIR_ARMED; g.held = gH.inDispatch != 0;
    g.hudImg = RingTag(RingRig(FG_IMG_HUDLESS)); g.holdImg = RingTag(RingRig(FG_IMG_HOLD)); g.bb = MgBb();
    g.kind = ShKindOf(RingPipeLog(), g.n);
    MgDueN();
    const auto shown = Shown();
    g.tail = TagList(shown, sh0);
    g.order = shown.size() == sh0 + 2 && shown[sh0].tag == g.tagB && shown[sh0 + 1].tag == g.tagC;
    g.inBusy = gRingSt.inBusy - busy0; g.back = a.w.hudBack - back0;
    ShRelease();
    if (!ShSettle()) Err("HU1: the fake helper did not come back after its hold");
    return g;
}
static void CaseHU1()
{
    const char* name = "HU1 the HUD-less picture: behind the hand into the back buffer, into the helper's HUDLESS, and the real frame back from the hold (stage 15)";
    CaseBegin(name);
    MgFresh<MA>();
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a; FgState& s = B::S();
    HuOn();
    ShWarm<MA>(); MgDueN();
    // A PLAIN PAIR, THE WORKER HELD
    const LONG pres0 = gRingSt.presents, pass0 = a.w.hudPass, back0 = a.w.hudBack, fed0 = a.w.hudFedN, draws0 = ShArmB::hudDraws, cop0 = ShArmB::hudCopies; const size_t sq0 = ShB::seq.size();
    const size_t shownAt = MgShownN();
    DWORD tagA = 0; gHuSeen = -1;
    MgHeld([&] { ClockAdd(16.7); ShInject(true); tagA = Frame<MA>(true); }, &HuLook);
    if (!ShSettle()) Err("HU1: the fake helper did not read the frame's messages");
    const LONG nA = x.w.lastQN; const DWORD hudA = ShArmB::hudTag;
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    const DWORD bb0 = MgBb();
    const DWORD presentHeld = RingTag(RingRig(FG_IMG_PRESENT)), holdHeld = RingTag(RingRig(FG_IMG_HOLD)), hudHeld = RingTag(RingRig(FG_IMG_HUDLESS));
    MgDueShow(); const DWORD bb1 = MgBb();
    MgDueN();    const DWORD bb2 = MgBb();
    const std::string seqA(ShB::seq.begin() + sq0, ShB::seq.end());
    const LONG presA = gRingSt.presents - pres0;
    auto shown = Shown();
    const DWORD sI = shown.size() > shown0 ? shown[shown0].tag : 0, sN = shown.size() > shown0 + 1 ? shown[shown0 + 1].tag : 0;
    const size_t shownA = shown.size() - shown0;
    const int hudOnScreen = MgTagCount(shownAt, hudA);
    const int kindA = ShKindOf(RingPipeLog(), nA);
    const LONG passA = a.w.hudPass - pass0, backA = a.w.hudBack - back0, fedA = a.w.hudFedN - fed0, drawsA = ShArmB::hudDraws - draws0;
    const LONG copA = ShArmB::hudCopies - cop0;
    if (copA != 1) Err("HU1: %ld copies into the helper's HUDLESS for the pair (1 wanted, the HUD-less note's: a hand with a picture to follow makes none, the note's copy would land over it)", copA);
    printf("  a plain pair (%ld) with a HUD-less picture, recorded behind a held worker: %ld picture(s) on the screen while it was held; armed %s; on the worker it reads %s; PRESENT %lX and HOLD %lX (the frame is %lX), HUDLESS %lX (the HUD-less picture is %lX);"
           " the back buffer %lX at the arm, %lX after the generated frame's fire, %lX after the real frame's; %ld DXVK present(s), showing %lX then %lX; the HUD-less picture on the screen %d time(s); 'G' %s;"
           " pictures drawn %ld, hands that said one follows %ld, handed to the helper %ld, the frame put back %ld\n",
        nA, (long)(gHuSeen < 0 ? -1 : gHuSeen - (LONG)shownAt), armedA ? "yes" : "NO", seqA.c_str(), presentHeld, holdHeld, tagA, hudHeld, hudA, bb0, bb1, bb2, presA, sI, sN, hudOnScreen, ShKindName(kindA), drawsA, passA, fedA, backA);
    if (gHuSeen != (LONG)shownAt) Err("HU1: %ld picture(s) reached the screen while the worker was held (0 wanted)", (long)(gHuSeen - (LONG)shownAt));
    if (!armedA || seqA != "QSSSGSSSS") Err("HU1: the pair was %s and reads %s on the worker (QSSSGSSSS wanted: 'Q', the hand's one submit, the HUD-less note's two, 'G', the write and the restore of each fire)", armedA ? "armed" : "NOT armed", seqA.c_str());
    if (hudA == tagA || presentHeld != tagA || holdHeld != tagA || hudHeld != hudA) Err("HU1: PRESENT holds %lX and HOLD %lX (the frame %lX wanted in both), HUDLESS %lX (the HUD-less picture %lX wanted)", presentHeld, holdHeld, tagA, hudHeld, hudA);
    if (bb0 != tagA || bb1 != tagA || bb2 != tagA) Err("HU1: the back buffer read %lX at the arm, %lX after the generated frame's fire and %lX after the real frame's (the frame %lX wanted each time: the HUD-less picture is taken back before the Present)", bb0, bb1, bb2, tagA);
    if (presA != 2 || shownA != 2 || sI != ShGenTag(tagA, false) || sN != tagA || hudOnScreen != 0) Err("HU1: %ld DXVK present(s), %zu picture(s), %lX then %lX, the HUD-less picture %d time(s) (2, 2, the pair's generated image %lX, the frame %lX and never wanted)", presA, shownA, sI, sN, hudOnScreen, ShGenTag(tagA, false), tagA);
    if (kindA != (int)FG_G_DISPATCH || drawsA != 1 || passA != 1 || fedA != 1 || backA != 1 || ShArmB::badSignal) Err("HU1: 'G' %s; pictures drawn %ld, hands with one to follow %ld, handed to the helper %ld, put back %ld; signals on a timeline not the game's %ld (DISPATCH, 1 each and 0 wanted)", ShKindName(kindA), drawsA, passA, fedA, backA, ShArmB::badSignal);
    // A PAIR WITH NO SOURCE
    const Hu1NoSrc ns = Hu1NoSource<MA>(false);
    printf("  a pair with no source (%ld, the helper %s inside the dispatch before it): 'G' %s, armed %s; HUDLESS left at %lX (the picture of the frame before, %lX; this frame's is %lX), HOLD %lX and the back buffer %lX (the frame, %lX);"
           " the screen then showed %s; put back %ld; copies into an input the helper was reading %ld\n",
        ns.n, ns.held ? "still held" : "ALREADY RELEASED", ShKindName(ns.kind), ns.armed ? "yes" : "NO", ns.hudImg, ns.hudB, ns.hudC, ns.holdImg, ns.bb, ns.tagC, ns.tail.c_str(), ns.back, ns.inBusy);
    if (!ns.held || ns.kind != (int)FG_G_NOSOURCE || !ns.armed) Err("HU1: the pair behind a held dispatch: the helper %s, 'G' %s, armed %d (held, NOSOURCE and armed wanted)", ns.held ? "held" : "released", ShKindName(ns.kind), (int)ns.armed);
    if (ns.hudImg != ns.hudB || ns.holdImg != ns.tagC || ns.bb != ns.tagC || ns.back != 1 || ns.inBusy != 0 || !ns.order)
        Err("HU1: with no source HUDLESS reads %lX (left at %lX wanted: the helper is reading it), HOLD %lX and the back buffer %lX (the frame %lX wanted), put back %ld (1), copies into a busy input %ld (0), the screen %s (%lX then %lX wanted)",
            ns.hudImg, ns.hudB, ns.holdImg, ns.bb, ns.tagC, ns.back, ns.inBusy, ns.tail.c_str(), ns.tagB, ns.tagC);
    ShFrame<MA>(); MgDueN();
    // A FRAME WITH NO INTERFACE
    ShArmB::hud = FG_HUD_SAME;
    const size_t sq1 = ShB::seq.size(); const LONG draws1 = ShArmB::hudDraws, pass1 = a.w.hudPass, cop1 = ShArmB::hudCopies;
    const DWORD tagS = ShFrame<MA>();
    const DWORD hudS = RingTag(RingRig(FG_IMG_HUDLESS)), presentS = RingTag(RingRig(FG_IMG_PRESENT));
    MgDueShow(); MgDueN();
    const std::string seqS(ShB::seq.begin() + sq1, ShB::seq.end());
    printf("  a frame with no interface (%lX): on the worker it reads %s; PRESENT %lX, HUDLESS %lX; pictures drawn %ld, hands with one to follow %ld\n", tagS, seqS.c_str(), presentS, hudS, ShArmB::hudDraws - draws1, a.w.hudPass - pass1);
    if (ShArmB::hudCopies - cop1 != 1) Err("HU1: a frame with no interface made %ld copies into the helper's HUDLESS (1 wanted, the hand's)", (long)(ShArmB::hudCopies - cop1));
    if (seqS != "QSGSSSS" || hudS != tagS || presentS != tagS || ShArmB::hudDraws != draws1 || a.w.hudPass != pass1)
        Err("HU1: a frame with no interface reads %s (QSGSSSS wanted), PRESENT %lX and HUDLESS %lX (the frame %lX in both wanted), with %ld picture(s) drawn and %ld hand(s) that said one follows (0 and 0 wanted)", seqS.c_str(), presentS, hudS, tagS, ShArmB::hudDraws - draws1, a.w.hudPass - pass1);
    // THE MARK
    ShArmB::hud = FG_HUD_PASS; ShArmB::backMode = 1;
    const LONG saves0 = a.w.saves, noSaves0 = a.w.noSaves;
    const DWORD tagM = ShFrame<MA>();
    const bool dirtyAtArm = RingBackDirty();
    const size_t shM = MgShownN();
    MgDueShow(); MgDueN();
    const int realM = MgTagCount(shM, tagM), genM = MgGenCount(shM);
    const LONG savesM = a.w.saves - saves0, noSavesM = a.w.noSaves - noSaves0;
    ShArmB::backMode = 0;
    printf("  the mark, by the fake's own knowledge of the back buffer's writes: at the arm of a pair with a HUD-less picture the back buffer reads %s; its two fires saved it %ld time(s) and needed no save %ld time(s); the screen showed %d generated frame(s) and the frame %lX %d time(s)\n",
        dirtyAtArm ? "WRITTEN SINCE THE MARK" : "not written since the mark", savesM, noSavesM, genM, tagM, realM);
    if (dirtyAtArm || savesM != 0 || noSavesM != 2 || genM != 1 || realM != 1) Err("HU1: at the arm the back buffer reads %s, the fires saved %ld time(s) and needed none %ld time(s), and the screen showed %d generated frame(s) and the frame %d time(s) (not written, 0, 2, 1 and 1 wanted: the mark comes after the picture was taken back)", dirtyAtArm ? "written" : "not written", savesM, noSavesM, genM, realM);
    ShFrame<MA>(); MgDueN();
    MgClean("HU1");
    MgEnd();
    // CONTROL 1: the restore reads the helper's PRESENT
    const RingStats stc = gRingSt;
    MgFresh<MAHP>(); HuOn();
    ShWarm<MAHP>(); MgDueN();
    const Hu1NoSrc c1 = Hu1NoSource<MAHP>(true);
    ViolRestore(stc);
    MgEnd();
    if (c1.bb == c1.tagC || c1.bb != c1.tagB) Err("HU1 control 1: with the restore reading the helper's PRESENT the back buffer of a pair with no source held %lX (the frame before, %lX, wanted; the frame is %lX): the control shows nothing", c1.bb, c1.tagB, c1.tagC);
    Ctl("the restore reading the helper's PRESENT on a pair with no source: the back buffer got %lX, the frame before, where the frame is %lX", c1.bb, c1.tagC);
    // CONTROL 2: the mark at the hand
    MgFresh<MAHM>(); HuOn(); ShArmB::backMode = 1;
    ShWarm<MAHM>(); MgDueN();
    const LONG saves1 = a.w.saves;
    const DWORD tagK = MgRecBegin();
    MgRecHand(FG_HF_HUDPASS); Run(HuMarkNote); Fill(NewTag()); RecordHudless(); RecordPresent(); Drain();
    const bool dirtyK = RingBackDirty();
    if (!ShSettle()) Err("HU1 control 2: the fake helper did not read the frame's messages");
    MgDueShow(); MgDueN();
    const LONG savesK = a.w.saves - saves1;
    ViolRestore(stc);
    MgEnd();
    if (!dirtyK || savesK != 2) Err("HU1 control 2: with the mark made at the hand the back buffer read %s at the arm and the fires saved %ld time(s) (written and 2 wanted): the control shows nothing", dirtyK ? "written" : "not written", savesK);
    Ctl("the mark made at the hand: the picture counted as a write of the game's, and the pair's fires saved the back buffer %ld time(s)", savesK);
    (void)tagK;
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU2, an inline pair never shows the HUD-less picture
// WHAT IT SHOWS (machine-4's hazard, closed). The picture is drawn behind a hand whatever becomes of that hand on the
// worker, which the render thread cannot know. THE NOTE TAKES IT BACK ALL THE SAME, from HOLD, and for that a hand that
// opens no pair copies its frame into HOLD. Each leg with the worker HELD and the frame queued behind it:
//   (a) AN UNSENT FRAME: nothing on the pipe, nothing into the helper's images; its frame into HOLD; presented inline,
//       THE FRAME, once;
//   (b) A STOP BETWEEN THE HAND AND THE NOTE (the pair open, the machine closing at its note);
//   (c) THE MACHINE KILLED BETWEEN THEM (a pointer check that failed, a ring that timed out);
//   (d) A HAND OF ANOTHER BUILD (closed at once: the set it was recorded for is gone);
// In every leg the screen shows the frame once and the HUD-less picture never.
// CONTROL: a frame recorded with the picture and WITHOUT its note (nobody takes it back, nobody undoes it): the screen
// shows the HUD-less picture. The case can see it.
static void Hu2Kill(DWORD, DWORD, DWORD) { MA::Kill("the harness's kill, between the hand and the HUD-less note"); }
struct Hu2Leg { DWORD tagA, tag, hud, bb, holdImg, hudImg, presentImg; int shownTag, shownHud; size_t wrote; LONG store, back, lost, drop, seen; std::string tail; };
// what: 0 an unsent frame through the machine's own Present; 1 a stop between; 2 a kill between; 3 a hand of another build; 4 the note left out (the control)
static Hu2Leg Hu2Run(int what)
{
    Hu2Leg g = {};
    MgFresh<MA>(); HuOn();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    g.tagA = ShFrame<MA>(); MgDueN();
    const size_t sh0 = MgShownN(), pipe0 = RingPipeLog().size();
    const LONG store0 = a.w.hudStore, back0 = a.w.hudBack, lost0 = a.w.hudBackLost, drop0 = a.w.dropped[FG_AD_HUDLESS];
    gHuSeen = -1;
    if (what == 0) MgHeld([&] { ClockAdd(16.7); ShInject(false); g.tag = Frame<MA>(false); g.hud = ShArmB::hudTag; }, &HuLook);
    else MgHeld([&] {
        g.tag = MgRecBegin();
        if (what == 3)
        {   // a hand recorded for a set that is gone: the payload's hold is another build generation
            FgHandPayload h = {};
            h.k = dtPresRecN + 1; h.gen = dtArmCount; h.p = s.r.nextP; h.fEnd = B::frames - 1; h.tEntry = B::Now(); h.hold = ShB::x.sh.bgen + 7; h.flags = FG_HF_HUDPASS;
            if (!DtNote(DT_NOTE_FG_HAND, &h, sizeof(h))) Err("HU2: a hand recorded by the harness was refused");
            s.r.lastP = h.p; s.r.lastGen = h.gen; s.r.lastEngaged = true; s.r.nextP++;
        }
        else MgRecHand(FG_HF_HUDPASS);
        g.hud = NewTag(); Fill(g.hud);
        if (what == 1) Run(M13Stop);
        if (what == 2) Run(Hu2Kill);
        if (what != 4) RecordHudless();
        RecordPresent();
    }, &HuLook);
    g.seen = gHuSeen < 0 ? -1 : gHuSeen - (LONG)sh0;
    g.bb = MgBb();
    g.holdImg = RingTag(RingRig(FG_IMG_HOLD)); g.hudImg = RingTag(RingRig(FG_IMG_HUDLESS)); g.presentImg = RingTag(RingRig(FG_IMG_PRESENT));
    g.wrote = RingPipeLog().size() - pipe0;
    g.shownTag = MgTagCount(sh0, g.tag); g.shownHud = MgTagCount(sh0, g.hud);
    g.tail = TagList(Shown(), sh0);
    g.store = a.w.hudStore - store0; g.back = a.w.hudBack - back0; g.lost = a.w.hudBackLost - lost0; g.drop = a.w.dropped[FG_AD_HUDLESS] - drop0;
    return g;
}
static void CaseHU2()
{
    const char* name = "HU2 a pair presented inline never shows the HUD-less picture: the note takes it back whatever became of the hand (stage 15)";
    CaseBegin(name);
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    static const char* const what[4] = { "(a) an unsent frame", "(b) a stop between the hand and the note", "(c) the machine killed between them", "(d) a hand of another build" };
    for (int i = 0; i < 4; i++)
    {
        const RingStats st0 = gRingSt;
        const Hu2Leg g = Hu2Run(i);
        // (the back buffer is not read here: behind the Present DXVK's rotation has put another image under the surface.
        // What was presented is the screen's own log, and that is what is held)
        printf("  %s, the worker held: %ld picture(s) on the screen while it was held; released: the screen showed %s (the frame %lX %d time(s), the HUD-less picture %lX %d time(s)); HOLD %lX;"
               " frames stored for a hand that opened no pair %ld, put back %ld, not put back %ld, presents dropped %ld; messages written %zu\n",
            what[i], g.seen, g.tail.c_str(), g.tag, g.shownTag, g.hud, g.shownHud, g.holdImg, g.store, g.back, g.lost, g.drop, g.wrote);
        if (g.seen != 0) Err("HU2 %s: %ld picture(s) reached the screen while the worker was held (0 wanted)", what[i], g.seen);
        if (g.shownTag != 1 || g.shownHud != 0 || g.holdImg != g.tag || g.back != 1 || g.lost != 0 || g.drop != 0)
            Err("HU2 %s: the screen showed the frame %d time(s) and the HUD-less picture %d time(s); HOLD %lX (the frame %lX wanted); put back %ld, not put back %ld, dropped %ld (1, 0, the frame, 1, 0 and 0 wanted)",
                what[i], g.shownTag, g.shownHud, g.holdImg, g.tag, g.back, g.lost, g.drop);
        // (an unsent frame writes nothing at all; a hand of another build may have had a 'Q' written at its injection, which that
        // hand closes: two messages in the product's order, none under the key)
        if ((i == 0 || i == 3) && (g.store != 1 || g.presentImg != g.tagA || (i == 0 && g.wrote != 0))) Err("HU2 %s: stored for the restore alone %ld time(s), %zu message(s) written, PRESENT %lX (1, and the frame before %lX wanted: nothing of this frame is the helper's; an unsent frame writes nothing)", what[i], g.store, g.wrote, g.presentImg, g.tagA);
        if (i == 0 && a.w.inl[FG_AI_UNSENT] < 1) Err("HU2 (a): the unsent frame was not presented inline as unsent");
        if (i == 1 && s.sh.state != FG_CLOSING) Err("HU2 (b): the machine is not closing behind the stop");
        if (i == 2 && s.sh.state != FG_DEAD) Err("HU2 (c): the machine is not dead behind the kill");
        if (i != 2) MgClean(what[i]);
        ViolRestore(st0);
        MgEnd();
    }
    // ((e), a SHADOW pair, went with the want in round 9's S8: no build of the product asks it)
    // CONTROL
    const RingStats stc = gRingSt;
    const Hu2Leg c = Hu2Run(4);
    ViolRestore(stc);
    MgEnd();
    if (c.shownHud != 1 || c.shownTag != 0) Err("HU2 control: with the HUD-less note left out the screen showed the HUD-less picture %d time(s) and the frame %d time(s) (1 and 0 wanted): the control shows nothing", c.shownHud, c.shownTag);
    Ctl("a frame recorded with the picture and without its note: the screen showed the HUD-less picture %lX, and the frame %lX %d time(s)", c.hud, c.tag, c.shownTag);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU3, a picture that cannot be taken back: the present is not made
// WHAT IT SHOWS. Where the note cannot put the frame back, THE PRESENT OF THAT ORDINAL IS NOT MADE: a frame is dropped,
// counted, and the picture with the interface gone never reaches the screen. The worker HELD, three ways:
//   (a) NO STORE: the hand's span was not taken (the HOLD image is not there), so nothing holds the frame;
//   (b) THE RESTORE'S SUBMIT REFUSED by the queue (the note's second submit; its first, the picture into HUDLESS, taken);
//   (c) NO COMMAND BUFFER for the note's span.
// The frame behind each is shown as ever. And the fake counts no Present behind a refused submit.
// CONTROL: the guard off (FG_ARMED_GUARD_ON 3): leg (a)'s Present is made (a pair with no store is presented inline, from
// the back buffer), and the screen shows the HUD-less picture.
struct Hu3Leg { DWORD tag, hud, next; int shownTag, shownHud, shownNext; LONG drop, lost, back, seen, after, presents; int kind; };
static Hu3Leg Hu3Run(int what, LONG guardOff = 0)
{
    Hu3Leg g = {};
    MgFresh<MA>(); HuOn();
    FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueN();
    const size_t sh0 = MgShownN();
    const RingStats st0 = gRingSt;
    gArmedGuardOff = guardOff;
    if (what == 0) ShArmB::noHold = 1;
    gHuSeen = -1;
    MgHeld([&] {
        g.tag = MgRecBegin();
        MgRecHand(FG_HF_HUDPASS);
        g.hud = NewTag(); Fill(g.hud);
        if (what == 1) Run(ShFk8Arm, 1u);                     // the next submit taken, the one behind it refused
        if (what == 2) Run(M8CbFail);
        RecordHudless(); RecordPresent();
    }, &HuLook);
    ShArmB::noHold = 0; gArmedGuardOff = 0;
    _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0); _InterlockedExchange(&gRing.cbFailNext, 0);
    g.seen = gHuSeen < 0 ? -1 : gHuSeen - (LONG)sh0;
    g.presents = gRingSt.presents - st0.presents; g.after = gRingSt.presentAfterRefused - st0.presentAfterRefused;
    g.kind = ShKindOf(RingPipeLog(), ShB::x.w.lastQN);
    g.drop = a.w.dropped[FG_AD_HUDLESS]; g.lost = a.w.hudBackLost; g.back = a.w.hudBack;
    g.shownTag = MgTagCount(sh0, g.tag); g.shownHud = MgTagCount(sh0, g.hud);
    if (!guardOff)
    {
        if (!ShSettle()) Err("HU3: the fake helper did not settle behind the dropped frame");
        const size_t sh1 = MgShownN();
        g.next = ShFrame<MA>(); MgDueN();
        g.shownNext = MgTagCount(sh1, g.next);
    }
    return g;
}
static void CaseHU3()
{
    const char* name = "HU3 a HUD-less picture that cannot be taken back: the frame's Present is not made, counted, and the picture never reaches the screen (stage 15)";
    CaseBegin(name);
    static const char* const what[3] = { "(a) no store (the hand's span was not taken)", "(b) the restore's submit refused", "(c) no command buffer for the note's span" };
    for (int i = 0; i < 3; i++)
    {
        const RingStats st0 = gRingSt;
        const Hu3Leg g = Hu3Run(i);
        printf("  %s, the worker held: %ld picture(s) on the screen while held; %ld DXVK present(s) for the frame; the frame %lX on the screen %d time(s), the HUD-less picture %lX %d time(s); not put back %ld, presents dropped and counted %ld; its 'G' %s;"
               " presents behind a refused submit %ld; the frame behind it shown %d time(s)\n",
            what[i], g.seen, g.presents, g.tag, g.shownTag, g.hud, g.shownHud, g.lost, g.drop, ShKindName(g.kind), g.after, g.shownNext);
        if (g.seen != 0 || g.presents != 0 || g.shownTag != 0 || g.shownHud != 0 || g.lost != 1 || g.drop != 1 || g.after != 0 || g.shownNext != 1)
            Err("HU3 %s: %ld picture(s) while held, %ld present(s), the frame %d time(s), the HUD-less picture %d time(s), not put back %ld, dropped %ld, presents behind a refused submit %ld, the next frame %d time(s) (0, 0, 0, 0, 1, 1, 0 and 1 wanted)",
                what[i], g.seen, g.presents, g.shownTag, g.shownHud, g.lost, g.drop, g.after, g.shownNext);
        if (i == 0 && g.kind != (int)FG_G_NOSOURCE) Err("HU3 (a): the pair whose span was not taken is not NOSOURCE on the wire ('G' %s)", ShKindName(g.kind));
        if (i == 1 && g.kind != (int)FG_G_DISPATCH) Err("HU3 (b): the pair whose picture reached the helper is not a DISPATCH ('G' %s): its dispatch feeds the history, and nothing of it is shown", ShKindName(g.kind));
        if (i == 2 && g.kind != (int)FG_G_NOSOURCE) Err("HU3 (c): the pair whose picture did not reach the helper is not NOSOURCE ('G' %s): fgIn was never signalled for it", ShKindName(g.kind));
        ViolRestore(st0);
        MgEnd();
    }
    // CONTROL
    const RingStats stc = gRingSt;
    const Hu3Leg c = Hu3Run(0, 1 << 3);
    ViolRestore(stc);
    MgEnd();
    if (c.shownHud != 1 || c.drop != 0) Err("HU3 control: with the guard off the screen showed the HUD-less picture %d time(s) and %ld present(s) were dropped (1 and 0 wanted): the control shows nothing", c.shownHud, c.drop);
    Ctl("the guard off: the Present was made and the screen showed the HUD-less picture %lX where the frame is %lX", c.hud, c.tag);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU4, the HUD-less note refused behind the picture
// WHAT IT SHOWS. The layer is disarmed BETWEEN THE HAND AND THE NOTE, on the render thread (a Reset another thread asked
// for is answered at this thread's next call, and the picture's draws are such calls). The ring is drained there, so the
// worker has the frame in HOLD and the picture in the back buffer; the note is then REFUSED, nothing of the worker's
// follows, and this thread's Present goes straight to DXVK. THE MACHINE ASKS THE BINDING TO PUT THE REAL PICTURE BACK
// (HudUndo), before that Present: the screen shows the frame, once, and the HUD-less picture never.
// CONTROL: a binding whose undo does nothing: the screen shows the HUD-less picture.
struct HuNoUndoB : ShArmB { static void HudUndo() { _InterlockedIncrement(&hudUndos); } };
typedef FgMachine<HuNoUndoB> MANU;
struct Hu4Leg { DWORD tagA, tag, hud; int shownA, shownTag, shownHud; LONG undos, refused; bool disarmed, rearmed; DWORD after; int shownAfter; };
template <class MM> static Hu4Leg Hu4Run()
{
    Hu4Leg g = {};
    MgFresh<MM>(); HuOn();
    FgState& s = B::S();
    ShWarm<MM>(); MgDueN();
    g.tagA = ShFrame<MM>();
    const size_t sh0 = MgShownN(); const LONG ref0 = s.c.noteRefused, undo0 = ShArmB::hudUndos;
    ShArmB::hudDisarm = 1;
    ClockAdd(16.7); ShInject(true);
    g.tag = Frame<MM>(true); g.hud = ShArmB::hudTag;
    g.disarmed = !dtArmed;
    g.refused = s.c.noteRefused - ref0; g.undos = ShArmB::hudUndos - undo0;
    g.shownA = MgTagCount(sh0, g.tagA); g.shownTag = MgTagCount(sh0, g.tag); g.shownHud = MgTagCount(sh0, g.hud);
    g.rearmed = MgEnsureArmed<MM>();
    return g;
}
static void CaseHU4()
{
    const char* name = "HU4 the HUD-less note refused behind the picture (a disarm between the hand and the note): the render thread puts the real picture back before its Present (stage 15)";
    CaseBegin(name);
    const RingStats st0 = gRingSt;
    Hu4Leg g = Hu4Run<MA>();
    if (g.rearmed)
    {
        ShWarm<MA>(); MgDueN();
        const size_t sh1 = MgShownN();
        g.after = ShFrame<MA>(); MgDueShow(); MgDueN();
        g.shownAfter = MgTagCount(sh1, g.after);
    }
    printf("  a disarm on the render thread between the hand and the note: the layer disarmed %s, the note refused %ld time(s), the undo asked %ld time(s); the screen showed the pair before %d time(s), the frame %lX %d time(s), the HUD-less picture %lX %d time(s);"
           " armed again %s, and a pair behind the re-arm showed its frame %d time(s)\n",
        g.disarmed ? "yes" : "NO", g.refused, g.undos, g.shownA, g.tag, g.shownTag, g.hud, g.shownHud, g.rearmed ? "yes" : "NO", g.shownAfter);
    if (!g.disarmed || g.refused != 1 || g.undos != 1 || g.shownTag != 1 || g.shownHud != 0 || !g.rearmed || g.shownAfter != 1)
        Err("HU4: disarmed %d, the note refused %ld time(s), the undo asked %ld time(s), the frame shown %d time(s), the HUD-less picture %d time(s), armed again %d, a later pair's frame %d time(s) (yes, 1, 1, 1, 0, yes and 1 wanted)",
            (int)g.disarmed, g.refused, g.undos, g.shownTag, g.shownHud, (int)g.rearmed, g.shownAfter);
    (void)st0;
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    const Hu4Leg c = Hu4Run<MANU>();
    ViolRestore(stc);
    MgEnd();
    if (c.shownHud != 1 || c.shownTag != 0) Err("HU4 control: with an undo that does nothing the screen showed the HUD-less picture %d time(s) and the frame %d time(s) (1 and 0 wanted): the control shows nothing", c.shownHud, c.shownTag);
    Ctl("an undo that does nothing: the screen showed the HUD-less picture %lX where the frame is %lX", c.hud, c.tag);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU5, a frame with no HUD-less picture is fed and never shown
// WHAT IT SHOWS (presentation-12; the brief's rule). The interface drew and the binding has no HUD-less picture of the
// frame (the first frame it is back: no copy of the frame before it; or its check judged the copy point wrong): the
// pair is a DISPATCH, FED WITH THE PRESENTED PICTURE IN BOTH PLACES (the hand's one span), it arms, and ITS GENERATED
// FRAME IS NOT SHOWN though its due says SHOW and its answer is in, counted with its own reason. Its real frame keeps
// its place. THE PAIR BEHIND IT IS SHOWN: the dispatch did not reset, so the rule costs one generated frame, not two.
// CONTROL: the same frame said to have no interface (SAME): its generated frame is shown.
struct Hu5Leg { DWORD tag, next; bool armed; int kind; DWORD hudImg, presentImg; int gens, reals, nextGens; LONG noHud, reset; std::string seq; };
static Hu5Leg Hu5Run(LONG hud)
{
    Hu5Leg g = {};
    MgFresh<MA>(); HuOn();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    const LONG noHud0 = a.w.iWhy[FG_AW_NOHUD], reset0 = a.w.fedReset; const size_t sq0 = ShB::seq.size();
    ShArmB::hud = hud;
    g.tag = ShFrame<MA>();
    ShArmB::hud = FG_HUD_PASS;
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    g.kind = ShKindOf(RingPipeLog(), ShB::x.w.lastQN);
    g.hudImg = RingTag(RingRig(FG_IMG_HUDLESS)); g.presentImg = RingTag(RingRig(FG_IMG_PRESENT));
    const size_t sh0 = MgShownN();
    MgDueShow(); MgDueN();
    g.seq.assign(ShB::seq.begin() + sq0, ShB::seq.end());
    g.gens = MgGenCount(sh0); g.reals = MgTagCount(sh0, g.tag);
    const size_t sh1 = MgShownN();
    g.next = ShFrame<MA>(); MgDueShow(); MgDueN();
    g.nextGens = MgGenCount(sh1);
    g.noHud = a.w.iWhy[FG_AW_NOHUD] - noHud0; g.reset = a.w.fedReset - reset0;
    MgClean("HU5");
    MgEnd();
    return g;
}
static void CaseHU5()
{
    const char* name = "HU5 a frame with an interface and no HUD-less picture: fed with the presented picture in both places, armed, its generated frame never shown, and the pair behind it shown (stage 15)";
    CaseBegin(name);
    static const char* const why[2] = { "no HUD-less picture could be made", "the check judged the copy point wrong" };
    for (int i = 0; i < 2; i++)
    {
        const Hu5Leg g = Hu5Run(i == 0 ? FG_HUD_NONE : FG_HUD_WIDE);
        printf("  %s: 'G' %s, armed %s; PRESENT %lX and HUDLESS %lX (the frame is %lX); on the worker %s; its due said SHOW: generated frames on the screen %d, its real frame %d time(s); counted not shown for want of a HUD-less picture %ld;"
               " dispatches that reset %ld; the pair behind it: generated frames on the screen %d\n",
            why[i], ShKindName(g.kind), g.armed ? "yes" : "NO", g.presentImg, g.hudImg, g.tag, g.seq.c_str(), g.gens, g.reals, g.noHud, g.reset, g.nextGens);
        if (g.kind != (int)FG_G_DISPATCH || !g.armed || g.presentImg != g.tag || g.hudImg != g.tag || g.seq != "QSGSS")
            Err("HU5 (%s): 'G' %s, armed %d, PRESENT %lX and HUDLESS %lX (the frame %lX in both wanted), on the worker %s (DISPATCH, armed, QSGSS: the hand's one submit, and the real frame's fire alone)", why[i], ShKindName(g.kind), (int)g.armed, g.presentImg, g.hudImg, g.tag, g.seq.c_str());
        if (g.gens != 0 || g.reals != 1 || g.noHud != 1 || g.reset != 0 || g.nextGens != 1)
            Err("HU5 (%s): generated frames shown %d, the real frame %d time(s), counted %ld, dispatches that reset %ld, the pair behind it showed %d generated frame(s) (0, 1, 1, 0 and 1 wanted)", why[i], g.gens, g.reals, g.noHud, g.reset, g.nextGens);
    }
    // CONTROL
    const Hu5Leg c = Hu5Run(FG_HUD_SAME);
    if (c.gens != 1 || c.noHud != 0) Err("HU5 control: the same frame said to have no interface showed %d generated frame(s) and was counted %ld time(s) (1 and 0 wanted): the control shows nothing", c.gens, c.noHud);
    Ctl("the same frame said to have no interface: its generated frame was shown (%d), and nothing was counted", c.gens);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU6, the census with the HUD-less picture
// WHAT IT SHOWS. The merged path's census (dt_fg_merged.h, McSegment: random frames, the screen read as a whole by its
// witness) run WITH THE HUD-LESS PICTURE: six thousand frames in six segments at one to four images and the three stream
// modes, the set with its HUD-less image, and for every frame, by chance: a picture drawn behind its hand (eleven in
// sixteen), no interface, no HUD-less picture, or the copy point judged wrong; and by chance a frame whose spans meet no
// store, no command buffer or a refused submit, beside everything the census plants already (unsent frames, standby,
// late and missing answers, a held helper, builds, cuts, the HDR view, dues against the placer, a helper that goes).
// THE WITNESS: every picture on the screen is a frame of the run, once and in order, or its own pair's generated image:
// THE HUD-LESS PICTURE'S TAG IS NO FRAME OF THE RUN, so one on the screen is a fault it counts. THE ACCOUNT: every frame
// that never reached the screen was dropped and counted. AND THE HUD-LESS PICTURE'S OWN BOOKS: every picture drawn was
// behind a hand the worker ran; every such hand's picture was taken back or its Present was not made.
// CONTROL: a segment with the guard off (FG_ARMED_GUARD_ON 3): where a picture could not be taken back the Present is
// made all the same, and the witness counts a picture that is no frame of the run.
static void CaseHU6()
{
    const char* name = "HU6 the census with the HUD-less picture: 6,000 random frames, the screen read as a whole, and the picture's own books (stage 15)";
    CaseBegin(name);
    gSeed = 0x3D7A11C5u ^ (gShAtHand ? 0x00F0F00Fu : 0u);
    McTot t; LONG pass = 0, back = 0, lost = 0, store = 0, fed = 0, fedLost = 0, draws = 0, undos = 0;
    gMcHud = 1; gMcHudGuardOff = 0;
    for (int i = 0; i < 6; i++)
    {
        McSegment<MA>(i, 1000, 1 + i % 4, i % 3, (i / 2) % 2, false, i == 5 ? 940 : -1, -1, t);
        const FgArmedWorker& w = ShArmB::a.w;
        pass += w.hudPass; back += w.hudBack; lost += w.hudBackLost; store += w.hudStore; fed += w.hudFedN; fedLost += w.hudFedLost;
        draws += ShArmB::hudDraws; undos += ShArmB::hudUndos;
    }
    printf("  %ld frames in 6 segments (1 to 4 images, three stream modes, the save answered both ways), every frame with what it has for a HUD-less picture drawn by chance: %ld pairs armed; on the screen %ld real frames and %ld generated ones; never shown %ld, dropped and counted %ld; builds %ld\n",
        t.frames, t.armed, t.real, t.gen, t.never, t.dropped, t.builds);
    printf("  the HUD-less picture: drawn behind a hand %ld time(s); hands that said one follows %ld; handed to the helper %ld, its submit not taken %ld; the real frame put back from the hold %ld, NOT put back %ld, presents not made for that %ld;"
           " frames stored for a hand that opened no pair %ld; generated frames not shown for want of a HUD-less picture %ld; undone by the render thread %ld\n",
        draws, pass, fed, fedLost, back, lost, t.drop[FG_AD_HUDLESS], store, t.why[FG_AW_NOHUD], undos);
    printf("  generated frames: shown %ld; not shown: a render fact %ld, fed and never shown for a reset %ld, no source %ld, late %ld, no frame %ld, forced %ld; inline: unsent %ld, no hold %ld, no HUD-less note %ld; THE WITNESS: %d picture(s) or frame(s) wrong%s%s; THE MIRROR: %ld of %ld dispatches\n",
        t.why[FG_AW_SHOWN], t.why[FG_AW_FACT], t.why[FG_AW_RESET], t.why[FG_AW_NOSOURCE], t.why[FG_AW_LATE], t.why[FG_AW_NOFRAME], t.why[FG_AW_FORCED], t.inl[FG_AI_UNSENT], t.inl[FG_AI_NOHOLD], t.inl[FG_AI_NOHUDLESS],
        t.bad, t.bad ? ": " : "", t.first.c_str(), t.mirror, t.mirrorN);
    if (t.bad) Err("HU6: the witness counts %d fault(s); the first: %s", t.bad, t.first.c_str());
    if (t.mirror) Err("HU6: the worker's mirror differs from the helper's record on %ld of %ld dispatches", t.mirror, t.mirrorN);
    if (t.frames < 6000 || t.real + t.never != t.frames) Err("HU6: %ld frames were made, %ld shown and %ld never shown: the account does not close", t.frames, t.real, t.never);
    if (draws != pass || pass != back + lost || lost != t.drop[FG_AD_HUDLESS] || undos != 0)
        Err("HU6: the picture's books do not close: drawn %ld, hands that said one follows %ld, put back %ld, not put back %ld, presents not made %ld, undone %ld (drawn = hands = put back + not put back, not put back = presents not made, and none undone, wanted)", draws, pass, back, lost, t.drop[FG_AD_HUDLESS], undos);
    if (pass < 2500 || fed < 1500 || !store || !lost || !fedLost || !t.why[FG_AW_NOHUD] || t.why[FG_AW_SHOWN] < 1000 || !t.why[FG_AW_NOSOURCE] || !t.inl[FG_AI_UNSENT] || !t.builds)
        Err("HU6: the run did not take every road (hands with a picture %ld, handed to the helper %ld, stored for a hand with no pair %ld, not put back %ld, a picture's submit not taken %ld, no HUD-less picture %ld, shown %ld, no source %ld, unsent %ld, builds %ld)",
            pass, fed, store, lost, fedLost, t.why[FG_AW_NOHUD], t.why[FG_AW_SHOWN], t.why[FG_AW_NOSOURCE], t.inl[FG_AI_UNSENT], t.builds);
    // CONTROL
    const RingStats stc = gRingSt;
    McTot c;
    gSeed = 0x3D7A11C5u ^ (gShAtHand ? 0x00F0F00Fu : 0u);
    gMcHudGuardOff = 1 << 3;
    McSegment<MA>(0, 1000, 3, RING_CS_LAZY, 0, false, -1, -1, c);
    const LONG lostC = ShArmB::a.w.hudBackLost;
    gMcHud = 0; gMcHudGuardOff = 0;
    ViolRestore(stc);
    if (c.bad < 1 || lostC < 1) Err("HU6 control: with the guard off, over a thousand frames with %ld picture(s) not put back, the witness counted %d fault(s): the control shows nothing", lostC, c.bad);
    Ctl("the guard off, a thousand frames: %ld picture(s) could not be taken back, and the witness counted %d fault(s) (the first: %s)", lostC, c.bad, c.first.c_str());
    CaseEnd(name);
}

// ================================================================================================================
// THE SECOND-TARGET FORM (2026-10-11; docs/reports/fg-3/after-sitting-2026-10-11/BRIEF-hud-cost.md; fg_machine.h, FG_HUD_TEX;
// source/fg/fg_hud.h, "WHERE IT IS DRAWN"). CASES HU7 TO HU10.
//
// The frame's own encode writes the HUD-less picture into an image of the binding's own, AHEAD OF THE HAND; the hand's ONE
// span copies it into the helper's HUDLESS beside PRESENT and HOLD. Nothing is drawn into the back buffer, so the HUD-less
// note makes no span, nothing is put back and no Present can be lost for it. WHAT THE MACHINE GAINED: one flag of the
// hand, one argument of the hand's span, two seams of the binding on the worker (HudSrc, RecCopyHudToImg), three counts.
//
// HOW THE FAKE SEES IT (dt_fg_shadow.h, ShArmB::HudSecond). The binding's own image is a D3D9 surface of the fake's; for a
// frame that answers TEX the render thread records A FILL OF IT WITH A TAG OF THE PICTURE'S OWN ahead of the hand, in
// DXVK's stream, as the mod's encode records its draw. So "the helper was handed this frame's HUD-less picture" is one
// compare of tags, read in the helper's image and, since these cases, in the fake helper's own record of every dispatch
// (ShDisp::hud: what HUDLESS held when the dispatch read it).
// ================================================================================================================
static void HuTexOn()
{
    ShB::hasHudless = 1; ShArmB::hud = FG_HUD_TEX;
    // (2026-10-24) the handle a make would read of the binding's image now: the control FGHUD_SRC_CACHED's (fg_hud_rule.h, FgHudSrcTake)
    EnterCriticalSection(&gRing.data); const int k = RingImgOfSurfLocked(ShArmB::HudSrcSurf(), nullptr); ShArmB::madeImg = k >= 0 ? gRing.img[k].handle : 0; LeaveCriticalSection(&gRing.data);
}
static bool HuDispOf(LONG n, ShDisp* out) { for (const ShDisp& d : ShDispLog()) if (d.n == n) { *out = d; return true; } return false; }

// ---------------------------------------------------------------- HU7, a pair in the second-target form
// WHAT IT SHOWS. A plain pair whose frame has an interface, THE WORKER HELD with the whole frame queued behind it (the
// injection, the frame's fill, the picture's fill into the binding's own image, the hand, the HUD-less note, the Present)
// AND, BEHIND THAT, THE FILL OF THE NEXT FRAME'S PICTURE into the same image: nothing reaches the screen while it is held.
// Released: on the worker the frame reads 'Q', THE HAND'S ONE SUBMIT, 'G', then the fires' four: NO SECOND SPAN. The
// helper's PRESENT and the mod's HOLD hold the frame; the helper's HUDLESS holds THIS FRAME'S PICTURE, not the next one's,
// which the same image holds by the time the case looks (the order is the stream's: the hand's flush ran what was ahead
// of the hand and nothing behind it); the fake helper's dispatch read the frame and that picture. The back buffer holds
// the frame at the arm and after each fire. The screen shows the pair's generated image and then the frame, never a
// picture's tag. And the form before's books read nothing: no picture drawn into the back buffer, no hand that said one
// follows, nothing put back, nothing stored, no undo.
// A PAIR WITH NO SOURCE (the helper held inside the dispatch before it): the helper's HUDLESS is LEFT AS IT IS READING IT,
// the frame is held and keeps its place.
// A FRAME WITH NO INTERFACE between them reads as it always read.
// THE MARK, by the fake's own knowledge of the back buffer's writes: at the arm the back buffer was not written since the
// frame, and neither fire saves it: IN THIS FORM THE MOD WRITES NOTHING INTO THE BACK BUFFER BETWEEN A HAND AND ITS PRESENT.
// CONTROLS: (1) a binding whose copy reads the BACK BUFFER: the helper's HUDLESS holds the frame, and the case sees it;
// (2) the picture's fill recorded BEHIND the hand (an encode that drew late): HUDLESS holds the picture of the frame
// BEFORE; (3) the hand's span with no flush ahead of its copies: the same, and PRESENT is the frame before's too.
struct HuNoFlushB : ShArmB
{
    static inline volatile LONG skip = 0;
    static void Flush() { if (_InterlockedExchange(&skip, 0)) return; ShArmB::Flush(); }
};
typedef FgMachine<HuNoFlushB> MAHF;
static void Hu7SkipFlush(DWORD, DWORD, DWORD) { _InterlockedExchange(&HuNoFlushB::skip, 1); }
static void CaseHU7()
{
    const char* name = "HU7 the second-target form: the HUD-less picture from an image of the binding's own, in the hand's one span; nothing is drawn into the back buffer (2026-10-11)";
    CaseBegin(name);
    MgFresh<MA>();
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a; FgState& s = B::S();
    HuTexOn();
    ShWarm<MA>(); MgDueN();
    // A PLAIN PAIR, THE WORKER HELD, THE NEXT FRAME'S PICTURE QUEUED BEHIND IT
    const LONG pres0 = gRingSt.presents, tex0 = a.w.hudTex, fed0 = a.w.hudTexFed, lost0 = a.w.hudTexLost, sec0 = ShArmB::hudSeconds, cop0 = ShArmB::hudCopies, tcop0 = ShArmB::hudTexCopies, ask0 = ShArmB::hudSrcAsked;
    const size_t sq0 = ShB::seq.size(), shownAt = MgShownN();
    DWORD tagA = 0, hudA = 0, hudLate = 0; gHuSeen = -1;
    MgHeld([&] { ClockAdd(16.7); ShInject(true); tagA = Frame<MA>(true); hudA = ShArmB::hudTag; hudLate = NewTag(); D->ColorFill(ShArmB::HudSrcSurf(), nullptr, hudLate); }, &HuLook);
    if (!ShSettle()) Err("HU7: the fake helper did not read the frame's messages");
    const LONG nA = x.w.lastQN;
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    const DWORD bb0 = MgBb();
    const DWORD presentHeld = RingTag(RingRig(FG_IMG_PRESENT)), holdHeld = RingTag(RingRig(FG_IMG_HOLD)), hudHeld = RingTag(RingRig(FG_IMG_HUDLESS)), ownNow = RingTag(ShArmB::HudSrcImg());
    ShDisp dA = {}; const bool gotA = HuDispOf(nA, &dA);
    MgDueShow(); const DWORD bb1 = MgBb();
    MgDueN();    const DWORD bb2 = MgBb();
    const std::string seqA(ShB::seq.begin() + sq0, ShB::seq.end());
    const LONG presA = gRingSt.presents - pres0;
    auto shown = Shown();
    const DWORD sI = shown.size() > shown0 ? shown[shown0].tag : 0, sN = shown.size() > shown0 + 1 ? shown[shown0 + 1].tag : 0;
    const size_t shownA = shown.size() - shown0;
    const int hudOnScreen = MgTagCount(shownAt, hudA) + MgTagCount(shownAt, hudLate);
    const int kindA = ShKindOf(RingPipeLog(), nA);
    printf("  a plain pair (%ld) whose HUD-less picture stands in the binding's own image, recorded behind a held worker with the next frame's picture queued behind it: %ld picture(s) on the screen while it was held; armed %s; on the worker it reads %s;"
           " PRESENT %lX and HOLD %lX (the frame is %lX), HUDLESS %lX (this frame's picture is %lX; the next one's, %lX, is what the binding's image holds now: %lX); the helper's dispatch read %lX and %lX;"
           " the back buffer %lX at the arm, %lX after the generated frame's fire, %lX after the real frame's; %ld DXVK present(s), showing %lX then %lX; a picture's tag on the screen %d time(s); 'G' %s\n",
        nA, (long)(gHuSeen < 0 ? -1 : gHuSeen - (LONG)shownAt), armedA ? "yes" : "NO", seqA.c_str(), presentHeld, holdHeld, tagA, hudHeld, hudA, hudLate, ownNow, dA.from, dA.hud, bb0, bb1, bb2, presA, sI, sN, hudOnScreen, ShKindName(kindA));
    printf("  its books: the encode wrote the binding's image %ld time(s); hands that said the picture stands there %ld, copied from it into the helper's image %ld (copies into HUDLESS of any kind %ld), not nameable %ld, the binding asked %ld time(s);"
           " AND OF THE FORM BEFORE: pictures drawn into the back buffer %ld, hands that said one follows %ld, put back from the hold %ld, stored for a hand with no pair %ld, undone %ld, presents not made %ld\n",
        ShArmB::hudSeconds - sec0, a.w.hudTex - tex0, a.w.hudTexFed - fed0, ShArmB::hudCopies - cop0, a.w.hudTexLost - lost0, ShArmB::hudSrcAsked - ask0,
        ShArmB::hudDraws, a.w.hudPass, a.w.hudBack, a.w.hudStore, ShArmB::hudUndos, a.w.dropped[FG_AD_HUDLESS]);
    if (gHuSeen != (LONG)shownAt) Err("HU7: %ld picture(s) reached the screen while the worker was held (0 wanted)", (long)(gHuSeen - (LONG)shownAt));
    if (!armedA || seqA != "QSGSSSS") Err("HU7: the pair was %s and reads %s on the worker (QSGSSSS wanted: 'Q', the hand's ONE submit, 'G', the write and the restore of each fire: no span of the HUD-less note's)", armedA ? "armed" : "NOT armed", seqA.c_str());
    if (hudA == tagA || hudLate == hudA || presentHeld != tagA || holdHeld != tagA || hudHeld != hudA || ownNow != hudLate)
        Err("HU7: PRESENT holds %lX and HOLD %lX (the frame %lX wanted in both), HUDLESS %lX (this frame's picture %lX wanted, not the next one's %lX), the binding's image %lX (the next one's wanted: its fill was queued behind the hand)", presentHeld, holdHeld, tagA, hudHeld, hudA, hudLate, ownNow);
    if (!gotA || dA.from != tagA || dA.hud != hudA) Err("HU7: the fake helper's dispatch of pair %ld %s: it read %lX as the presented picture and %lX as the HUD-less one (the frame %lX and its picture %lX wanted)", nA, gotA ? "is on record" : "is NOT on record", dA.from, dA.hud, tagA, hudA);
    if (bb0 != tagA || bb1 != tagA || bb2 != tagA) Err("HU7: the back buffer read %lX at the arm, %lX after the generated frame's fire and %lX after the real frame's (the frame %lX wanted each time)", bb0, bb1, bb2, tagA);
    if (presA != 2 || shownA != 2 || sI != ShGenTag(tagA, false) || sN != tagA || hudOnScreen != 0) Err("HU7: %ld DXVK present(s), %zu picture(s), %lX then %lX, a picture's tag %d time(s) (2, 2, the pair's generated image %lX, the frame %lX and never wanted)", presA, shownA, sI, sN, hudOnScreen, ShGenTag(tagA, false), tagA);
    if (kindA != (int)FG_G_DISPATCH || ShArmB::hudSeconds - sec0 != 1 || a.w.hudTex - tex0 != 1 || a.w.hudTexFed - fed0 != 1 || a.w.hudTexLost != lost0 || ShArmB::hudCopies - cop0 != 1 || ShArmB::hudTexCopies - tcop0 != 1 || ShArmB::hudSrcAsked - ask0 != 1 || ShArmB::badSignal)
        Err("HU7: 'G' %s; the encode wrote the image %ld time(s), hands that said so %ld, copied from it %ld, not nameable %ld, copies into HUDLESS %ld, of them from the image %ld, the binding asked %ld time(s), signals on a timeline not the game's %ld (DISPATCH, 1, 1, 1, 0, 1, 1, 1 and 0 wanted)",
            ShKindName(kindA), ShArmB::hudSeconds - sec0, a.w.hudTex - tex0, a.w.hudTexFed - fed0, a.w.hudTexLost - lost0, ShArmB::hudCopies - cop0, ShArmB::hudTexCopies - tcop0, ShArmB::hudSrcAsked - ask0, ShArmB::badSignal);
    if (ShArmB::hudDraws || a.w.hudPass || a.w.hudBack || a.w.hudBackLost || a.w.hudStore || ShArmB::hudUndos || a.w.dropped[FG_AD_HUDLESS] || a.w.hudFedN)
        Err("HU7: the form before's books are not empty: drawn into the back buffer %ld, hands with one to follow %ld, put back %ld, not put back %ld, stored %ld, undone %ld, presents not made %ld, handed by the note's span %ld (0 each wanted)",
            ShArmB::hudDraws, a.w.hudPass, a.w.hudBack, a.w.hudBackLost, a.w.hudStore, ShArmB::hudUndos, a.w.dropped[FG_AD_HUDLESS], a.w.hudFedN);
    // A PAIR WITH NO SOURCE
    const LONG cop1 = ShArmB::hudCopies, fed1 = a.w.hudTexFed, lost1 = a.w.hudTexLost;
    const Hu1NoSrc ns = Hu1NoSource<MA>(false);
    printf("  a pair with no source (%ld, the helper %s inside the dispatch before it): 'G' %s, armed %s; HUDLESS left at %lX (the picture of the frame before, %lX; this frame's is %lX), HOLD %lX and the back buffer %lX (the frame, %lX);"
           " the screen then showed %s; copies into HUDLESS for it %ld, into an input the helper was reading %ld\n",
        ns.n, ns.held ? "still held" : "ALREADY RELEASED", ShKindName(ns.kind), ns.armed ? "yes" : "NO", ns.hudImg, ns.hudB, ns.hudC, ns.holdImg, ns.bb, ns.tagC, ns.tail.c_str(), ShArmB::hudCopies - cop1 - 1, ns.inBusy);
    if (!ns.held || ns.kind != (int)FG_G_NOSOURCE || !ns.armed) Err("HU7: the pair behind a held dispatch: the helper %s, 'G' %s, armed %d (held, NOSOURCE and armed wanted)", ns.held ? "held" : "released", ShKindName(ns.kind), (int)ns.armed);
    if (ns.hudImg != ns.hudB || ns.holdImg != ns.tagC || ns.bb != ns.tagC || ns.inBusy != 0 || !ns.order || ShArmB::hudCopies - cop1 != 1 || a.w.hudTexFed - fed1 != 1 || a.w.hudTexLost != lost1)
        Err("HU7: with no source HUDLESS reads %lX (left at %lX wanted: the helper is reading it), HOLD %lX and the back buffer %lX (the frame %lX wanted), copies into a busy input %ld (0), the screen %s (%lX then %lX wanted); over the two pairs %ld copies into HUDLESS and %ld handed on (1 and 1 wanted: the pair before's alone), not nameable %ld (0)",
            ns.hudImg, ns.hudB, ns.holdImg, ns.bb, ns.tagC, ns.inBusy, ns.tail.c_str(), ns.tagB, ns.tagC, ShArmB::hudCopies - cop1, a.w.hudTexFed - fed1, a.w.hudTexLost - lost1);
    ShFrame<MA>(); MgDueN();
    // A FRAME WITH NO INTERFACE
    ShArmB::hud = FG_HUD_SAME;
    const size_t sq1 = ShB::seq.size(); const LONG sec1 = ShArmB::hudSeconds, tex1 = a.w.hudTex, tcop1 = ShArmB::hudTexCopies;
    const DWORD tagS = ShFrame<MA>();
    const DWORD hudS = RingTag(RingRig(FG_IMG_HUDLESS)), presentS = RingTag(RingRig(FG_IMG_PRESENT));
    MgDueShow(); MgDueN();
    const std::string seqS(ShB::seq.begin() + sq1, ShB::seq.end());
    printf("  a frame with no interface (%lX): on the worker it reads %s; PRESENT %lX, HUDLESS %lX; the encode wrote the binding's image %ld time(s), hands that said so %ld, copies out of it %ld\n",
        tagS, seqS.c_str(), presentS, hudS, ShArmB::hudSeconds - sec1, a.w.hudTex - tex1, ShArmB::hudTexCopies - tcop1);
    if (seqS != "QSGSSSS" || hudS != tagS || presentS != tagS || ShArmB::hudSeconds != sec1 || a.w.hudTex != tex1 || ShArmB::hudTexCopies != tcop1)
        Err("HU7: a frame with no interface reads %s (QSGSSSS wanted), PRESENT %lX and HUDLESS %lX (the frame %lX in both wanted), the binding's image written %ld time(s), hands that said so %ld, copies out of it %ld (0 each wanted)",
            seqS.c_str(), presentS, hudS, tagS, ShArmB::hudSeconds - sec1, a.w.hudTex - tex1, ShArmB::hudTexCopies - tcop1);
    // THE MARK
    ShArmB::hud = FG_HUD_TEX; ShArmB::backMode = 1;
    const LONG saves0 = a.w.saves, noSaves0 = a.w.noSaves; const size_t sq2 = ShB::seq.size();
    const DWORD tagM = ShFrame<MA>();
    const bool dirtyAtArm = RingBackDirty();
    const size_t shM = MgShownN();
    MgDueShow(); MgDueN();
    const int realM = MgTagCount(shM, tagM), genM = MgGenCount(shM);
    const LONG savesM = a.w.saves - saves0, noSavesM = a.w.noSaves - noSaves0;
    const std::string seqM(ShB::seq.begin() + sq2, ShB::seq.end());
    ShArmB::backMode = 0;
    printf("  the mark, by the fake's own knowledge of the back buffer's writes: at the arm of a pair in this form the back buffer reads %s; its two fires saved it %ld time(s) and needed no save %ld time(s); the whole pair reads %s on the worker; the screen showed %d generated frame(s) and the frame %lX %d time(s)\n",
        dirtyAtArm ? "WRITTEN SINCE THE MARK" : "not written since the mark", savesM, noSavesM, seqM.c_str(), genM, tagM, realM);
    if (dirtyAtArm || savesM != 0 || noSavesM != 2 || genM != 1 || realM != 1 || seqM != "QSGSS")
        Err("HU7: at the arm the back buffer reads %s, the fires saved %ld time(s) and needed none %ld time(s), the pair reads %s, and the screen showed %d generated frame(s) and the frame %d time(s) (not written, 0, 2, QSGSS: three submits a shown pair, 1 and 1 wanted)",
            dirtyAtArm ? "written" : "not written", savesM, noSavesM, seqM.c_str(), genM, realM);
    ShFrame<MA>(); MgDueN();
    MgClean("HU7");
    MgEnd();
    // CONTROL 1: the copy reads the back buffer
    {
        const RingStats stc = gRingSt;
        MgFresh<MA>(); HuTexOn(); ShWarm<MA>(); MgDueN();
        ShArmB::hudWrong = 1;
        const DWORD tagK = ShFrame<MA>(); const DWORD hudK = ShArmB::hudTag;
        const DWORD got = RingTag(RingRig(FG_IMG_HUDLESS));
        ShArmB::hudWrong = 0;
        MgDueShow(); MgDueN();
        ViolRestore(stc);
        MgEnd();
        if (got != tagK || got == hudK) Err("HU7 control 1: with the copy reading the back buffer HUDLESS held %lX (the frame %lX wanted, where the picture is %lX): the control shows nothing", got, tagK, hudK);
        Ctl("a copy that reads the back buffer: the helper's HUDLESS held the frame %lX where the picture is %lX", got, hudK);
    }
    // CONTROL 2: the picture's fill recorded behind the hand
    {
        const RingStats stc = gRingSt;
        MgFresh<MA>(); HuTexOn(); ShWarm<MA>(); MgDueN();
        ShFrame<MA>(); const DWORD hudP = ShArmB::hudTag; MgDueShow(); MgDueN();
        const DWORD tagK = MgRecBegin();
        MgRecHand(FG_HF_HUDTEX); const DWORD hudK = NewTag(); D->ColorFill(ShArmB::HudSrcSurf(), nullptr, hudK); RecordHudless(); RecordPresent(); Drain();
        if (!ShSettle()) Err("HU7 control 2: the fake helper did not read the frame's messages");
        const DWORD got = RingTag(RingRig(FG_IMG_HUDLESS));
        MgDueShow(); MgDueN();
        ViolRestore(stc);
        MgEnd();
        if (got != hudP || got == hudK) Err("HU7 control 2: with the picture's fill recorded behind the hand HUDLESS held %lX (the picture of the frame before, %lX, wanted; this frame's is %lX): the control shows nothing", got, hudP, hudK);
        Ctl("the picture's fill recorded behind the hand: HUDLESS held %lX, the picture of the frame before, where this frame's is %lX", got, hudK);
        (void)tagK;
    }
    // CONTROL 3: the hand's span with no flush ahead of its copies
    {
        const RingStats stc = gRingSt;
        MgFresh<MAHF>(); HuTexOn(); ShWarm<MAHF>(); MgDueN();
        const DWORD tagP = ShFrame<MAHF>(); const DWORD hudP = ShArmB::hudTag; MgDueShow(); MgDueN();
        DWORD tagK = 0, hudK = 0;
        ClockAdd(16.7); ShInject(true); Run(Hu7SkipFlush); tagK = Frame<MAHF>(true); hudK = ShArmB::hudTag; Drain();
        if (!ShSettle()) Err("HU7 control 3: the fake helper did not read the frame's messages");
        ShDisp dK = {}; const bool gotK = HuDispOf(ShB::x.w.lastQN, &dK);
        MgDueShow(); MgDueN();
        ViolRestore(stc);
        MgEnd();
        if (!gotK || dK.hud != hudP || dK.hud == hudK) Err("HU7 control 3: with no flush ahead of the hand's copies the helper's dispatch read %lX as the HUD-less picture (the frame before's, %lX, wanted; this frame's is %lX): the control shows nothing", dK.hud, hudP, hudK);
        Ctl("the hand's span with no flush ahead of its copies: the helper's dispatch read the picture %lX and the frame %lX of the frame before (%lX, %lX), where this frame's are %lX and %lX", dK.hud, dK.from, hudP, tagP, hudK, tagK);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU8, the binding's image cannot be named at the hand
// WHAT IT SHOWS. The hand said the frame's picture stands in the binding's own image, and at the hand's span the binding
// cannot name it (the mod: the encode's second target went between the encode and the hand, or was made for another back
// buffer). THE PAIR IS THEN A FRAME WITH NO HUD-LESS PICTURE: a DISPATCH, fed with the presented picture in both places
// by the same one span, it arms, and ITS GENERATED FRAME IS NOT SHOWN though its due says SHOW and its answer is in,
// counted (hudTexLost, FG_AW_NOHUD). Its real frame keeps its place, and THE PAIR BEHIND IT IS SHOWN.
// CONTROL: the same frame with the image there: its generated frame is shown, and the helper holds its picture.
struct Hu8Leg { DWORD tag, hud; bool armed; int kind; DWORD hudImg, presentImg; int gens, reals, nextGens; LONG noHud, reset, texFed, texLost; std::string seq; };
static Hu8Leg Hu8Run(bool gone)
{
    Hu8Leg g = {};
    MgFresh<MA>(); HuTexOn();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    const LONG noHud0 = a.w.iWhy[FG_AW_NOHUD], reset0 = a.w.fedReset, fed0 = a.w.hudTexFed, lost0 = a.w.hudTexLost; const size_t sq0 = ShB::seq.size();
    ShArmB::hudSrcGone = gone ? 1 : 0;
    g.tag = ShFrame<MA>(); g.hud = ShArmB::hudTag;
    ShArmB::hudSrcGone = 0;
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    g.kind = ShKindOf(RingPipeLog(), ShB::x.w.lastQN);
    g.hudImg = RingTag(RingRig(FG_IMG_HUDLESS)); g.presentImg = RingTag(RingRig(FG_IMG_PRESENT));
    const size_t sh0 = MgShownN();
    MgDueShow(); MgDueN();
    g.seq.assign(ShB::seq.begin() + sq0, ShB::seq.end());
    g.gens = MgGenCount(sh0); g.reals = MgTagCount(sh0, g.tag);
    const size_t sh1 = MgShownN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    g.nextGens = MgGenCount(sh1);
    g.noHud = a.w.iWhy[FG_AW_NOHUD] - noHud0; g.reset = a.w.fedReset - reset0; g.texFed = a.w.hudTexFed - fed0; g.texLost = a.w.hudTexLost - lost0;
    MgClean("HU8");
    MgEnd();
    return g;
}
static void CaseHU8()
{
    const char* name = "HU8 the second-target form: the binding's image cannot be named at the hand: fed with the presented picture in both places, armed, never shown, and the pair behind it shown (2026-10-11)";
    CaseBegin(name);
    const Hu8Leg g = Hu8Run(true);
    printf("  the image not nameable at the hand's span: 'G' %s, armed %s; PRESENT %lX and HUDLESS %lX (the frame is %lX, its picture %lX); on the worker %s; its due said SHOW: generated frames on the screen %d, its real frame %d time(s);"
           " counted not nameable %ld, not shown for want of a HUD-less picture %ld, handed on from the image %ld (the pair behind it alone); dispatches that reset %ld; the pair behind it: generated frames on the screen %d\n",
        ShKindName(g.kind), g.armed ? "yes" : "NO", g.presentImg, g.hudImg, g.tag, g.hud, g.seq.c_str(), g.gens, g.reals, g.texLost, g.noHud, g.texFed, g.reset, g.nextGens);
    if (g.kind != (int)FG_G_DISPATCH || !g.armed || g.presentImg != g.tag || g.hudImg != g.tag || g.seq != "QSGSS")
        Err("HU8: 'G' %s, armed %d, PRESENT %lX and HUDLESS %lX (the frame %lX in both wanted: the presented picture, not the image the binding could not name), on the worker %s (DISPATCH, armed, QSGSS: the hand's one submit and the real frame's fire alone)", ShKindName(g.kind), (int)g.armed, g.presentImg, g.hudImg, g.tag, g.seq.c_str());
    if (g.gens != 0 || g.reals != 1 || g.texLost != 1 || g.noHud != 1 || g.texFed != 1 || g.reset != 0 || g.nextGens != 1)
        Err("HU8: generated frames shown %d, the real frame %d time(s), not nameable %ld, counted for want of a picture %ld, handed on %ld, dispatches that reset %ld, the pair behind it showed %d generated frame(s) (0, 1, 1, 1, 1 (the pair behind), 0 and 1 wanted)", g.gens, g.reals, g.texLost, g.noHud, g.texFed, g.reset, g.nextGens);
    // CONTROL
    const Hu8Leg c = Hu8Run(false);
    if (c.gens != 1 || c.noHud != 0 || c.texLost != 0 || c.hudImg != c.hud) Err("HU8 control: with the image there the frame showed %d generated frame(s), was counted %ld and %ld time(s), and HUDLESS held %lX (1, 0, 0 and its picture %lX wanted): the control shows nothing", c.gens, c.noHud, c.texLost, c.hudImg, c.hud);
    Ctl("the same frame with the image there: its generated frame was shown (%d), nothing was counted, and the helper's HUDLESS held its picture %lX", c.gens, c.hudImg);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU9, every way such a hand can end
// WHAT IT SHOWS. The picture is written by the frame's encode whatever becomes of that frame's hand on the worker, which
// the render thread cannot know. IN THIS FORM THAT COSTS NOTHING AND RISKS NOTHING: the picture is in an image of its
// own, so the back buffer holds the frame on every road. Each leg with the worker HELD and the frame queued behind it:
//   (a) AN UNSENT FRAME through the machine's own Present: nothing on the pipe, nothing copied; presented inline;
//   (b) A STOP BETWEEN THE HAND AND THE NOTE; (c) THE MACHINE KILLED BETWEEN THEM; (d) A HAND OF ANOTHER BUILD;
//   (e) NO COMMAND BUFFER for the hand's span; (f) THE HAND'S SUBMIT REFUSED by the queue;
// In every leg the screen shows the frame once and a picture's tag never; no Present is dropped for a picture; the form
// before's guards are never asked (nothing stored for a hand with no pair, nothing put back, no undo).
// CONTROL: a binding whose encode writes the picture INTO THE BACK BUFFER, with nothing to take it back (what the form
// before guards against with a span, a store and a dropped present): leg (a)'s screen shows the picture.
static void Hu9Kill(DWORD, DWORD, DWORD) { MA::Kill("the harness's kill, between the hand and the HUD-less note"); }
struct Hu9Leg { DWORD tagA, tag, hud; int shownTag, shownHud; size_t wrote; LONG seen, copies, store, back, lost, drop, undos, presents; DWORD presentImg, hudImg; };
// what: 0 unsent; 1 a stop between; 2 a kill between; 3 a hand of another build; 4 no command buffer; 5 the hand's submit refused
static Hu9Leg Hu9Run(int what, LONG wrong = 0)
{
    Hu9Leg g = {};
    MgFresh<MA>(); HuTexOn();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    g.tagA = ShFrame<MA>(); MgDueShow(); MgDueN();
    const size_t sh0 = MgShownN(), pipe0 = RingPipeLog().size();
    const RingStats st0 = gRingSt;
    const LONG cop0 = ShArmB::hudCopies;
    gHuSeen = -1;
    ShArmB::hudWrong = wrong;
    if (what == 0) MgHeld([&] { ClockAdd(16.7); ShInject(false); g.tag = Frame<MA>(false); g.hud = ShArmB::hudTag; }, &HuLook);
    else MgHeld([&] {
        g.tag = MgRecBegin();
        ShArmB::HudSecond(); g.hud = ShArmB::hudTag;          // the frame's encode: its picture into the binding's image, ahead of the hand
        if (what == 4) Run(M8CbFail);
        if (what == 5) Run(ShFailNextSubmit);
        if (what == 3)
        {   // a hand recorded for a set that is gone: the payload's hold is another build generation
            FgHandPayload h = {};
            h.k = dtPresRecN + 1; h.gen = dtArmCount; h.p = s.r.nextP; h.fEnd = B::frames - 1; h.tEntry = B::Now(); h.hold = ShB::x.sh.bgen + 7; h.flags = FG_HF_HUDTEX;
            if (!DtNote(DT_NOTE_FG_HAND, &h, sizeof(h))) Err("HU9: a hand recorded by the harness was refused");
            s.r.lastP = h.p; s.r.lastGen = h.gen; s.r.lastEngaged = true; s.r.nextP++;
        }
        else MgRecHand(FG_HF_HUDTEX);
        if (what == 1) Run(M13Stop);
        if (what == 2) Run(Hu9Kill);
        RecordHudless(); RecordPresent();
    }, &HuLook);
    ShArmB::hudWrong = 0;
    _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0); _InterlockedExchange(&gRing.cbFailNext, 0);
    g.seen = gHuSeen < 0 ? -1 : gHuSeen - (LONG)sh0;
    g.presents = gRingSt.presents - st0.presents;
    g.presentImg = RingTag(RingRig(FG_IMG_PRESENT)); g.hudImg = RingTag(RingRig(FG_IMG_HUDLESS));
    g.wrote = RingPipeLog().size() - pipe0;
    g.shownTag = MgTagCount(sh0, g.tag); g.shownHud = MgTagCount(sh0, g.hud);
    g.copies = ShArmB::hudCopies - cop0;
    g.store = a.w.hudStore; g.back = a.w.hudBack; g.lost = a.w.hudBackLost; g.drop = a.w.dropped[FG_AD_HUDLESS]; g.undos = ShArmB::hudUndos;
    return g;
}
static void CaseHU9()
{
    const char* name = "HU9 the second-target form: whatever becomes of the hand, the screen shows the frame and never the picture, and nothing guards a back buffer that was never written (2026-10-11)";
    CaseBegin(name);
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    static const char* const what[6] = { "(a) an unsent frame", "(b) a stop between the hand and the note", "(c) the machine killed between them", "(d) a hand of another build", "(e) no command buffer for the hand's span", "(f) the hand's submit refused" };
    for (int i = 0; i < 6; i++)
    {
        const RingStats st0 = gRingSt;
        const Hu9Leg g = Hu9Run(i);
        printf("  %s, the worker held: %ld picture(s) on the screen while it was held; released: %ld DXVK present(s), the frame %lX on the screen %d time(s), the picture %lX %d time(s); copies into HUDLESS %ld (HUDLESS %lX, PRESENT %lX);"
               " stored for a hand with no pair %ld, put back %ld, not put back %ld, presents not made %ld, undone %ld; messages written %zu\n",
            what[i], g.seen, g.presents, g.tag, g.shownTag, g.hud, g.shownHud, g.copies, g.hudImg, g.presentImg, g.store, g.back, g.lost, g.drop, g.undos, g.wrote);
        if (g.seen != 0) Err("HU9 %s: %ld picture(s) reached the screen while the worker was held (0 wanted)", what[i], g.seen);
        if (g.shownTag != 1 || g.shownHud != 0 || g.store || g.back || g.lost || g.drop || g.undos)
            Err("HU9 %s: the screen showed the frame %d time(s) and the picture %d time(s); stored %ld, put back %ld, not put back %ld, presents not made %ld, undone %ld (1, 0, and 0 each wanted)", what[i], g.shownTag, g.shownHud, g.store, g.back, g.lost, g.drop, g.undos);
        // (a pair that was not fed copies nothing into the helper's HUDLESS: an unsent frame, a hand of another build, a span that was not taken)
        if ((i == 0 || i == 3 || i == 4) && (g.copies != 0 || g.hudImg == g.hud)) Err("HU9 %s: %ld copies into HUDLESS, which holds %lX (0, and not this frame's picture %lX, wanted: nothing of this frame is the helper's)", what[i], g.copies, g.hudImg, g.hud);
        if (i == 0 && (g.wrote != 0 || a.w.inl[FG_AI_UNSENT] < 1)) Err("HU9 (a): the unsent frame wrote %zu message(s) and was presented inline as unsent %ld time(s) (0 and 1 wanted)", g.wrote, a.w.inl[FG_AI_UNSENT]);
        if (i == 1 && s.sh.state != FG_CLOSING) Err("HU9 (b): the machine is not closing behind the stop");
        if (i == 2 && s.sh.state != FG_DEAD) Err("HU9 (c): the machine is not dead behind the kill");
        if ((i == 4 || i == 5) && a.w.inl[FG_AI_NOHOLD] < 1) Err("HU9 %s: the pair whose span was not taken was not presented inline for want of a hold (%ld)", what[i], a.w.inl[FG_AI_NOHOLD]);
        if (i != 2) MgClean(what[i]);
        ViolRestore(st0);
        MgEnd();
    }
    // ((g), a SHADOW pair, went with the want in round 9's S8: no build of the product asks it)
    // CONTROL
    const RingStats stc = gRingSt;
    const Hu9Leg c = Hu9Run(0, 2);
    ViolRestore(stc);
    MgEnd();
    if (c.shownHud != 1 || c.shownTag != 0) Err("HU9 control: with the encode writing the picture into the back buffer the screen showed the picture %d time(s) and the frame %d time(s) (1 and 0 wanted): the control shows nothing", c.shownHud, c.shownTag);
    Ctl("an encode that writes the picture into the back buffer, with nothing to take it back: the screen showed the picture %lX, and the frame %lX %d time(s)", c.hud, c.tag, c.shownTag);
    CaseEnd(name);
}

// ---------------------------------------------------------------- HU10, the census in the second-target form
// WHAT IT SHOWS. The merged path's census (dt_fg_merged.h, McSegment) run in this form: six thousand frames in six segments
// at one to four images and the three stream modes, the set with its HUD-less image, and for every frame, by chance: its
// picture in the binding's own image (eleven in sixteen), no interface, no HUD-less picture, or the copy point judged
// wrong; and by chance a frame whose hand's span meets no store, no command buffer, a refused submit, OR AN IMAGE IT
// CANNOT NAME, beside everything the census plants already.
// THE WITNESS, as ever: every picture on the screen is a frame of the run, once and in order, or its own pair's generated
// image. A picture's tag is no frame of the run. THE ACCOUNT: every frame that never reached the screen was dropped and
// counted. AND EVERY DISPATCH'S HUD-LESS INPUT, by the fake helper's own record: the picture of THAT VERY FRAME where the
// hand's span handed it on, the frame itself everywhere else, never another frame's. THE BOOKS: of the form before,
// nothing (no picture drawn into the back buffer, nothing put back, no Present not made for one, no undo).
// CONTROLS: a segment over a binding whose copy reads the back buffer: the dispatches' record counts every pair that was
// owed its picture and got the frame. And a segment whose encode writes the picture into the back buffer: the witness
// counts pictures that are no frame of the run.
static void CaseHU10()
{
    const char* name = "HU10 the census in the second-target form: 6,000 random frames, the screen read as a whole, and every dispatch's HUD-less input (2026-10-11)";
    CaseBegin(name);
    gSeed = 0x7B21C9D3u ^ (gShAtHand ? 0x00F0F00Fu : 0u);
    McTot t; LONG said = 0, fed = 0, lost = 0, seconds = 0, draws = 0, pass = 0, back = 0, backLost = 0, store = 0, undos = 0, noteFed = 0;
    gMcHud = 2; gMcHudGuardOff = 0;
    for (int i = 0; i < 6; i++)
    {
        McSegment<MA>(i, 1000, 1 + i % 4, i % 3, (i / 2) % 2, false, i == 5 ? 940 : -1, -1, t);
        const FgArmedWorker& w = ShArmB::a.w;
        said += w.hudTex; fed += w.hudTexFed; lost += w.hudTexLost; seconds += ShArmB::hudSeconds;
        draws += ShArmB::hudDraws; pass += w.hudPass; back += w.hudBack; backLost += w.hudBackLost; store += w.hudStore; undos += ShArmB::hudUndos; noteFed += w.hudFedN;
    }
    printf("  %ld frames in 6 segments (1 to 4 images, three stream modes, the save answered both ways), every frame with what it has for a HUD-less picture drawn by chance: %ld pairs armed; on the screen %ld real frames and %ld generated ones; never shown %ld, dropped and counted %ld; builds %ld\n",
        t.frames, t.armed, t.real, t.gen, t.never, t.dropped, t.builds);
    printf("  the second-target form: the encode wrote the binding's image %ld time(s); hands that said the picture stands there %ld; copied from it into the helper's image %ld; not nameable at the hand %ld; generated frames not shown for want of a HUD-less picture %ld;"
           " EVERY DISPATCH'S HUD-LESS INPUT: %ld dispatches read, %ld of them owed their own frame's picture, %ld wrong%s%s\n",
        seconds, said, fed, lost, t.why[FG_AW_NOHUD], t.hudIn, t.hudInOwn, t.hudInBad, t.hudInBad ? ": " : "", t.hudFirst.c_str());
    printf("  the form before's books: pictures drawn into the back buffer %ld, hands that said one follows %ld, handed by the note's span %ld, put back %ld, not put back %ld, presents not made %ld, stored %ld, undone %ld;"
           " generated frames: shown %ld; not shown: a render fact %ld, a reset %ld, no source %ld, late %ld, no frame %ld, forced %ld; inline: unsent %ld, no hold %ld; THE WITNESS: %d picture(s) or frame(s) wrong%s%s; THE MIRROR: %ld of %ld dispatches\n",
        draws, pass, noteFed, back, backLost, t.drop[FG_AD_HUDLESS], store, undos,
        t.why[FG_AW_SHOWN], t.why[FG_AW_FACT], t.why[FG_AW_RESET], t.why[FG_AW_NOSOURCE], t.why[FG_AW_LATE], t.why[FG_AW_NOFRAME], t.why[FG_AW_FORCED], t.inl[FG_AI_UNSENT], t.inl[FG_AI_NOHOLD],
        t.bad, t.bad ? ": " : "", t.first.c_str(), t.mirror, t.mirrorN);
    if (t.bad) Err("HU10: the witness counts %d fault(s); the first: %s", t.bad, t.first.c_str());
    if (t.mirror) Err("HU10: the worker's mirror differs from the helper's record on %ld of %ld dispatches", t.mirror, t.mirrorN);
    if (t.frames < 6000 || t.real + t.never != t.frames) Err("HU10: %ld frames were made, %ld shown and %ld never shown: the account does not close", t.frames, t.real, t.never);
    if (t.hudInBad) Err("HU10: %ld of %ld dispatches read a HUD-less picture that is not theirs; the first: %s", t.hudInBad, t.hudIn, t.hudFirst.c_str());
    if (draws || pass || noteFed || back || backLost || t.drop[FG_AD_HUDLESS] || store || undos)
        Err("HU10: the form before's books are not empty: drawn into the back buffer %ld, hands with one to follow %ld, handed by the note's span %ld, put back %ld, not put back %ld, presents not made %ld, stored %ld, undone %ld (0 each wanted)", draws, pass, noteFed, back, backLost, t.drop[FG_AD_HUDLESS], store, undos);
    if (said < fed + lost || lost != t.hudLost) Err("HU10: the form's books do not close: hands that said so %ld, handed on %ld, not nameable %ld (by the frames' own record %ld)", said, fed, lost, t.hudLost);
    if (seconds < 3000 || fed < 2000 || t.hudInOwn < 1500 || !lost || !t.why[FG_AW_NOHUD] || t.why[FG_AW_SHOWN] < 1000 || !t.why[FG_AW_NOSOURCE] || !t.inl[FG_AI_UNSENT] || !t.inl[FG_AI_NOHOLD] || !t.builds)
        Err("HU10: the run did not take every road (the image written %ld, handed on %ld, dispatches owed their picture %ld, not nameable %ld, no HUD-less picture %ld, shown %ld, no source %ld, unsent %ld, no hold %ld, builds %ld)",
            seconds, fed, t.hudInOwn, lost, t.why[FG_AW_NOHUD], t.why[FG_AW_SHOWN], t.why[FG_AW_NOSOURCE], t.inl[FG_AI_UNSENT], t.inl[FG_AI_NOHOLD], t.builds);
    // CONTROL 1: the copy reads the back buffer
    {
        const RingStats stc = gRingSt;
        McTot c;
        gSeed = 0x7B21C9D3u ^ (gShAtHand ? 0x00F0F00Fu : 0u);
        ShArmB::hudWrong = 0;
        gMcHudWrong = 1;
        McSegment<MA>(0, 1000, 3, RING_CS_LAZY, 0, false, -1, -1, c);
        gMcHudWrong = 0;
        ViolRestore(stc);
        if (c.hudInBad < 100 || c.hudInBad != c.hudInOwn) Err("HU10 control 1: with the copy reading the back buffer, of %ld dispatches owed their picture %ld read another (every one of them, and at least 100, wanted): the control shows nothing", c.hudInOwn, c.hudInBad);
        Ctl("a copy that reads the back buffer, a thousand frames: %ld of the %ld dispatches owed their frame's picture read the frame instead (the first: %s)", c.hudInBad, c.hudInOwn, c.hudFirst.c_str());
    }
    // CONTROL 2: the encode writes the picture into the back buffer
    {
        const RingStats stc = gRingSt;
        McTot c;
        gSeed = 0x7B21C9D3u ^ (gShAtHand ? 0x00F0F00Fu : 0u);
        gMcHudWrong = 2;
        McSegment<MA>(0, 1000, 3, RING_CS_LAZY, 0, false, -1, -1, c);
        gMcHudWrong = 0;
        ViolRestore(stc);
        if (c.bad < 100) Err("HU10 control 2: with the encode writing the picture into the back buffer, over a thousand frames the witness counted %d fault(s) (at least 100 wanted): the control shows nothing", c.bad);
        Ctl("an encode that writes the picture into the back buffer, a thousand frames: the witness counted %d fault(s) (the first: %s)", c.bad, c.first.c_str());
    }
    gMcHud = 0; gMcHudGuardOff = 0;
    CaseEnd(name);
}

// ================================================================================================================
#include "../../source/fg/fg_hud_rule.h"     // (the game side's pure rules, as the mod compiles them: HU11)
// THE PHONE (2026-10-12; docs/reports/fg-3/sitting-2026-10-12c/BRIEF-phone.md; source/fg/fg_hud.h, "THE PHONE"). CASE HU11.
// TWO COPY POINTS SINCE 2026-10-12, 21:20 (docs/reports/fg-4/brief-2026-10-12/BRIEF-phone-cover.md; fg_hud.h, "TWO COPY POINTS").
//
// The game draws its phone as a 3D model with a projection of its own straight into the back buffer, behind the composite
// and the coronas and ahead of the 2D interface. The copy of the frame "before the interface" is taken at the first 2D
// draw, BEHIND the phone, so a HUD-less picture made from it holds the phone as scene. The armed product asks one thing
// more at the interface's boundary (FgHudOverlay3d), and its rules are pure functions this harness compiles with the mod
// (fg_machine.h, FgHudOverlay3dRule; source/fg/fg_hud_rule.h, FgHudOverlayCopyRule, FgHudSecondRule, FgHudDrewRule,
// FgHudPreOkRule): ahead of the frame's first such draw FRAME GENERATION TAKES A COPY OF ITS OWN, and the frame's HUD-less
// picture is made from that one. THE ENCODE'S OWN COPY AND ITS COUNT ARE A RELEASE'S: the first form of this (eca6ac9b)
// took the encode's copy ahead of the phone and counted the phone as an interface draw, and the real encode then took
// the phone for interface (the owner: "it is only treated as UI in frame gen ... its a bug").
//
// HOW THE FAKE SEES IT. Its back buffer is one tag, so A FRAME IS BUILT DRAW BY DRAW, EVERY DRAW THAT WRITES COLOUR A FILL WITH A TAG
// OF ITS OWN, and the case keeps for each tag WHAT IS ON THE PICTURE IT NAMES (the scene, the coronas, the phone, the HUD).
// Ahead of each draw the case asks IvtOnDraw's two questions in its order: the boundary's own rule as the release has
// it (TRANSCRIBED from DlssIvTransport.h, "THE INTERFACE BOUNDARY": this harness does not compile the transport), which
// takes THE ENCODE'S COPY and counts; and then the armed product's (THE MOD'S OWN LINES), which takes THE OVERLAY'S COPY and
// counts nothing. EACH COPY IS RECORDED WHERE ITS RULE SAYS, in DXVK's stream ahead of the draw: a StretchRect of the back
// buffer into a surface that stands for that copy's texture (the encode's: a surface of this case's own; the
// overlay's: the fake's FG_SURF_HOLD, which nothing of the merged path names). The frame's encode then writes its
// second output by the mod's own rule (FgHudSecondRule): in tags, the copy that rule names, into the binding's own
// image; the hand, the HUD-less note and the Present follow. THE WORKER IS HELD THROUGH ALL OF IT.
// WHAT IS READ: where each copy was recorded and what was on the picture then; the encode's count; and the fake
// helper's own record of the pair's dispatch (what PRESENT and HUDLESS held when it read them). AMD's interpolation
// takes from the real frame what the two pictures differ by and interpolates what they share (tools\test-fg-hudless,
// proofs p to r, hold that by pixels with the product's own encodes and AMD's own line).
// SO: (a) THE ENCODE'S COPY AND ITS COUNT ON A FRAME WITH THE PHONE ARE WHAT THEY ARE WITH THE ARMED PRODUCT'S QUESTION NOT ASKED (a release):
// taken ahead of the first sprite, THE PHONE ON IT; (b) THE PHONE IS ON THE PRESENTED PICTURE AND NOT ON THE HUD-LESS ONE, THE
// CORONAS ON BOTH, at Quality's shape and at native's, three handsets; (c) the phone with no 2D interface behind it: no
// copy of the encode's at all, as in a release, and the HUD-less picture from the overlay's copy alone; (d) no phone:
// what it always was. CONTROLS: the first form's else-if FAILS (a); the armed product's question not asked FAILS (b).
// ================================================================================================================
enum : unsigned { HU11_SCENE = 1, HU11_CORONA = 2, HU11_PHONE = 4, HU11_HUD = 8 };
// (how the frame's draws are judged) THE PRODUCT: the mod's rules. WAS: the first form's else-if (eca6ac9b: the encode's
// one copy taken ahead of the overlay, the overlay counted as an interface draw). RELEASE: the armed product's question
// is not asked.
enum { HU11_PRODUCT = 0, HU11_WAS = 1, HU11_RELEASE = 2 };
struct Hu11Draw { const char* what; bool ortho, sceneCam, colour, blend; int prims; DWORD x, y, w, h; unsigned layer; };
// The armed product's question for a draw into the back buffer the boundary's own rule did not take (IvtOnDraw's fenced
// else-if with its two counts, and the callee's facts): is it an overlay's draw? passOwed: the mod's own sharpening pass
// is still owed at the first 2D draw.
static bool Hu11Overlay(const Hu11Draw& d, unsigned bbBefore, unsigned sceneBefore, DWORD outW, DWORD outH, bool passOwed)
{
    if (!(bbBefore > 0 && sceneBefore > 0)) return false;                                 // (the call site's two counts)
    FgHudDrawFacts f = {};
    f.backBuffer = true; f.ortho = d.ortho; f.sceneCam = d.sceneCam; f.colour = d.colour; f.resolved = true; f.passOwed = passOwed;
    f.bbBefore = bbBefore; f.sceneBefore = sceneBefore; f.vpX = d.x; f.vpY = d.y; f.vpW = d.w; f.vpH = d.h; f.outW = outW; f.outH = outH;
    return FgHudOverlay3dRule(f);
}
// The frame's tail behind the composite, at a back buffer of outW by outH: the quad that restores the depth (no colour),
// two coronas (the scene's camera, the whole target), THE PHONE where one is up (seven blended draws of one model at the
// rect given, none 2D), then where the HUD is on four sprites, a text batch and the frame's final blended quad (2D).
static std::vector<Hu11Draw> Hu11Tail(DWORD outW, DWORD outH, bool phone, DWORD px, DWORD py, DWORD pw, DWORD ph, bool hudOn)
{
    std::vector<Hu11Draw> t;
    t.push_back({ "the quad that restores the depth", false, false, false, false, 2, 0, 0, outW, outH, 0 });
    for (int i = 0; i < 2; i++) t.push_back({ "a corona", false, true, true, true, 128, 0, 0, outW, outH, HU11_CORONA });
    if (phone) for (int i = 0; i < 7; i++) t.push_back({ "the phone", false, false, true, true, 742, px, py, pw, ph, HU11_PHONE });
    if (hudOn)
    {
        for (int i = 0; i < 4; i++) t.push_back({ "a sprite", true, false, true, true, 2, 0, 0, outW, outH, HU11_HUD });
        t.push_back({ "a text batch", true, false, true, true, 90, 0, 0, outW, outH, HU11_HUD });
        t.push_back({ "the final quad", true, false, true, true, 2, 0, 0, outW, outH, 0 });
    }
    return t;
}
// encAt, encWhat, onEnc, preDraws: THE ENCODE'S COPY (the draw it was recorded ahead of, what was on the picture) and its count.
// ovAt, ovWhat, onOv: THE OVERLAY'S. src: what the second output was made from (FG_HUDSRC_*).
struct Hu11Leg { DWORD scene, final; int encAt, ovAt; const char *encWhat, *ovWhat; unsigned preDraws, onFrom, onHud, onEnc, onOv; int src; bool armed, got; ShDisp d; LONG seen; int gens, reals; DWORD flags; };
static Hu11Leg Hu11Run(const std::vector<Hu11Draw>& tail, DWORD outW, DWORD outH, int mode, bool passOwed = false, bool ovRefused = false)
{
    Hu11Leg g = {}; g.encAt = -1; g.ovAt = -1; g.encWhat = "no draw"; g.ovWhat = "no draw";
    MgFresh<MA>(); HuTexOn();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    std::map<DWORD, unsigned> on;
    static FakeSurf* const encSurf = new FakeSurf(1920, 1080, nullptr, D3DUSAGE_RENDERTARGET);     // stands for hdrPreHudTex: the encode's own copy
    IDirect3DSurface9* const ovSurf = RingSurf(FG_SURF_HOLD);                                       // stands for FgHud::ovTex: frame generation's copy ahead of an overlay
    const size_t sh0 = MgShownN();
    gHuSeen = -1;
    MgHeld([&] {
        g.scene = MgRecBegin(); on[g.scene] = HU11_SCENE;      // the frame counter, the injection, and the composite: the frame's first back-buffer draw
        DWORD cur = g.scene; unsigned bb = 1, sc = 3000; bool encOwed = true, ovOpened = false, ovCopy = false;
        auto encCopyHere = [&](size_t i, const Hu11Draw& d) { if (encOwed) { encOwed = false; D->StretchRect(gBb, nullptr, encSurf, nullptr, D3DTEXF_NONE); g.encAt = (int)i; g.encWhat = d.what; g.onEnc = on[cur]; } g.preDraws++; };
        for (size_t i = 0; i < tail.size(); i++)
        {
            const Hu11Draw& d = tail[i];
            const bool opaqueQuad = d.prims <= 2 && !d.blend && d.colour;                 // (IvtIsOpaqueQuad)
            if (d.ortho && sc > 0 && !opaqueQuad) encCopyHere(i, d);                      // (the boundary's own rule: the encode's copy, and its count)
            else if (mode != HU11_RELEASE && Hu11Overlay(d, bb, sc, outW, outH, passOwed))
            {
                if (mode == HU11_WAS) encCopyHere(i, d);                                  // (the first form: the encode's one copy, here, and counted)
                else if (FgHudOverlayCopyRule(ovOpened, g.preDraws))                      // (the mod's own line)
                {
                    ovOpened = true;
                    if (!ovRefused) { D->StretchRect(gBb, nullptr, ovSurf, nullptr, D3DTEXF_NONE); ovCopy = true; g.ovAt = (int)i; g.ovWhat = d.what; g.onOv = on[cur]; }
                }
            }
            if (d.colour && d.layer) { const DWORD t = NewTag(); on[t] = on[cur] | d.layer; Fill(t); cur = t; }
            bb++; if (d.sceneCam) sc++;
        }
        g.final = cur;
        // the frame's encode: its second output by the mod's own rule, into the binding's image; what it says of the frame for the hand
        const bool encCopy = g.preDraws > 0 && !encOwed;
        g.src = FgHudSecondRule(encCopy, ovOpened, ovCopy);
        if (g.src == FG_HUDSRC_TWO || g.src == FG_HUDSRC_OVERLAY) D->StretchRect(ovSurf, nullptr, ShArmB::HudSrcSurf(), nullptr, D3DTEXF_NONE);
        else if (g.src == FG_HUDSRC_ENCODE) D->StretchRect(encSurf, nullptr, ShArmB::HudSrcSurf(), nullptr, D3DTEXF_NONE);
        g.flags = !FgHudDrewRule(g.preDraws, ovOpened) ? 0u : (FgHudPreOkRule(encCopy, ovOpened, ovCopy) && g.src != FG_HUDSRC_NONE) ? (DWORD)FG_HF_HUDTEX : (DWORD)FG_HF_NOHUD;
        MgRecHand(g.flags); RecordHudless(); RecordPresent();
    }, &HuLook);
    if (!ShSettle()) Err("HU11: the fake helper did not read the frame's messages");
    g.seen = gHuSeen < 0 ? -1 : gHuSeen - (LONG)sh0;
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    g.got = HuDispOf(ShB::x.w.lastQN, &g.d);
    const size_t sh1 = MgShownN();
    MgDueShow(); MgDueN();
    g.gens = MgGenCount(sh1); g.reals = MgTagCount(sh1, g.final);
    g.onFrom = on.count(g.d.from) ? on[g.d.from] : 0; g.onHud = on.count(g.d.hud) ? on[g.d.hud] : 0;
    MgClean("HU11");
    MgEnd();
    return g;
}
static const char* Hu11On(unsigned m)
{
    static char b[8][64]; static int k = 0; char* o = b[k++ & 7];
    snprintf(o, 64, "%s%s%s%s", m & HU11_SCENE ? "the scene" : "NOTHING OF THIS FRAME", m & HU11_CORONA ? ", the coronas" : "", m & HU11_PHONE ? ", THE PHONE" : "", m & HU11_HUD ? ", the HUD" : "");
    return o;
}
static const char* Hu11Src(int s) { return s == FG_HUDSRC_TWO ? "both copies" : s == FG_HUDSRC_OVERLAY ? "the overlay's copy alone" : s == FG_HUDSRC_ENCODE ? "the encode's own copy" : "NOTHING"; }
static void CaseHU11()
{
    const char* name = "HU11 the phone: two copy points: the encode's copy and its count are a release's, and the HUD-less picture is made from a copy of frame generation's own, ahead of the overlay (2026-10-12)";
    CaseBegin(name);
    // THE TWO SHAPES AND THE THREE HANDSETS: the rect is each draw's own; none of these numbers is in the rule
    struct Shape { const char* what; DWORD w, h; } const shapes[2] = { { "Quality's shape (the back buffer 3840 by 2160, the scene scaled)", 3840, 2160 }, { "native's shape (2560 by 1440)", 2560, 1440 } };
    struct Set { DWORD x, y, w, h; } const sets[3] = { { 12, 10, 8, 10 }, { 13, 11, 7, 9 }, { 0, 6, 6, 14 } };      // twentieths of the target: x, y, width, height
    const unsigned all = HU11_SCENE | HU11_CORONA | HU11_PHONE | HU11_HUD, sceneOnly = HU11_SCENE | HU11_CORONA;
    for (const Shape& sh : shapes) for (int k = 0; k < 3; k++)
    {
        const DWORD px = sh.w * sets[k].x / 20, py = sh.h * sets[k].y / 20, pw = sh.w * sets[k].w / 20, ph = sh.h * sets[k].h / 20;
        const std::vector<Hu11Draw> tail = Hu11Tail(sh.w, sh.h, true, px, py, pw, ph, true);
        const Hu11Leg g = Hu11Run(tail, sh.w, sh.h, HU11_PRODUCT);
        const RingStats str = gRingSt;
        const Hu11Leg r = Hu11Run(tail, sh.w, sh.h, HU11_RELEASE);          // the same frame with the armed product's question not asked: what a release does
        const Hu11Leg w = Hu11Run(tail, sh.w, sh.h, HU11_WAS);              // and by the first form (eca6ac9b): the ghosting fix as the owner saw it
        ViolRestore(str);
        // THE GHOSTING FIX IS WHAT IT WAS FOR THE GENERATED FRAME (the owner, 2026-10-12, 22:25): the two pictures the helper reads are the first form's
        if (g.onHud != w.onHud || g.onFrom != w.onFrom || w.onHud != sceneOnly || g.gens != w.gens)
            Err("HU11: the helper read %s as the HUD-less picture and %s as the presented one, %d generated frame(s); by the first form: %s, %s, %d (the same wanted: the generated frame's inputs do not change)",
                Hu11On(g.onHud), Hu11On(g.onFrom), g.gens, Hu11On(w.onHud), Hu11On(w.onFrom), w.gens);
        printf("  %s, a handset at %lu,%lu (%lu by %lu), the HUD on, THE WORKER HELD with the whole frame queued behind it: %ld picture(s) on the screen while it was held;"
               " THE ENCODE'S COPY was recorded ahead of draw %d of the tail (%s), with %s on the picture, and counted %u draw(s) (with the question not asked: draw %d, %s, %u);"
               " THE OVERLAY'S COPY ahead of draw %d (%s), with %s on the picture; the second output from %s;"
               " released: the pair armed %s; the helper's dispatch read as the presented picture: %s; as the HUD-less picture: %s; generated frames on the screen %d, the frame %d time(s)\n",
            sh.what, px, py, pw, ph, g.seen, g.encAt, g.encWhat, Hu11On(g.onEnc), g.preDraws, r.encAt, Hu11On(r.onEnc), r.preDraws, g.ovAt, g.ovWhat, Hu11On(g.onOv), Hu11Src(g.src),
            g.armed ? "yes" : "NO", Hu11On(g.onFrom), Hu11On(g.onHud), g.gens, g.reals);
        if (g.seen != 0) Err("HU11: %ld picture(s) reached the screen while the worker was held (0 wanted)", g.seen);
        // (a) the encode's copy and its count are a release's
        if (g.encAt != r.encAt || g.onEnc != r.onEnc || g.preDraws != r.preDraws || g.encAt != 10 || strcmp(g.encWhat, "a sprite") || g.onEnc != (sceneOnly | HU11_PHONE) || g.preDraws != 6)
            Err("HU11 (a): the encode's copy was taken ahead of draw %d (%s) with %s on it and %u draw(s) counted; with the question not asked: draw %d, %s, %u (the same wanted: draw 10, the first sprite, the scene, the coronas and THE PHONE, 6)",
                g.encAt, g.encWhat, Hu11On(g.onEnc), g.preDraws, r.encAt, Hu11On(r.onEnc), r.preDraws);
        // (b) the overlay's copy ahead of the phone, and the HUD-less picture from it
        if (g.ovAt != 3 || strcmp(g.ovWhat, "the phone") || g.onOv != sceneOnly || g.src != FG_HUDSRC_TWO || g.flags != FG_HF_HUDTEX)
            Err("HU11 (b): the overlay's copy was taken ahead of draw %d (%s) with %s on it, the second output from %s (3, the phone's first draw, behind the quad that restores the depth and the two coronas, and both copies wanted)", g.ovAt, g.ovWhat, Hu11On(g.onOv), Hu11Src(g.src));
        if (!g.armed || !g.got || g.onFrom != all || g.onHud != sceneOnly || g.d.from != g.final)
            Err("HU11 (b): armed %d, the dispatch %s; the presented picture holds %s and the HUD-less picture %s (the scene, the coronas, the phone and the HUD on the first; the scene and the coronas ALONE on the second wanted)", (int)g.armed, g.got ? "on record" : "NOT on record", Hu11On(g.onFrom), Hu11On(g.onHud));
        if (g.gens != 1 || g.reals != 1) Err("HU11: the screen showed %d generated frame(s) and the frame %d time(s) (1 and 1 wanted)", g.gens, g.reals);
        if (r.onHud != (sceneOnly | HU11_PHONE) || r.ovAt != -1) Err("HU11: with the question not asked the HUD-less picture holds %s and an overlay's copy was taken at %d (the phone on it, and none, wanted: the reference shows nothing)", Hu11On(r.onHud), r.ovAt);
    }
    // (c) THE HUD OFF IN THE GAME, THE PHONE UP: no copy of the encode's at all, as in a release; the HUD-less picture from the overlay's copy alone
    {
        const std::vector<Hu11Draw> tail = Hu11Tail(3840, 2160, true, 2304, 1080, 1536, 1080, false);
        const Hu11Leg g = Hu11Run(tail, 3840, 2160, HU11_PRODUCT);
        printf("  (c) the HUD off and the phone up: the encode's copy: %s (%u draw(s) counted); the overlay's copy ahead of draw %d (%s); the second output from %s; the presented picture: %s; the HUD-less picture: %s; generated frames on the screen %d\n",
            g.encAt < 0 ? "none" : "TAKEN", g.preDraws, g.ovAt, g.ovWhat, Hu11Src(g.src), Hu11On(g.onFrom), Hu11On(g.onHud), g.gens);
        if (g.encAt != -1 || g.preDraws != 0) Err("HU11 (c): with the HUD off the encode's copy was taken ahead of draw %d and %u draw(s) were counted (none and 0 wanted: a release takes no copy on such a frame)", g.encAt, g.preDraws);
        if (g.ovAt != 3 || g.src != FG_HUDSRC_OVERLAY || g.flags != FG_HF_HUDTEX || g.onFrom != (sceneOnly | HU11_PHONE) || g.onHud != sceneOnly || g.gens != 1)
            Err("HU11 (c): with the HUD off the overlay's copy was taken ahead of draw %d, the second output from %s, the presented picture holds %s and the HUD-less one %s, %d generated frame(s) shown (3, the overlay's copy alone, with the phone, without it, 1 wanted)", g.ovAt, Hu11Src(g.src), Hu11On(g.onFrom), Hu11On(g.onHud), g.gens);
    }
    // AN OVERLAY WHOSE COPY COULD NOT BE TAKEN (the texture refused): no HUD-less picture, the generated frame is not shown; never a picture with the phone as scene
    {
        const RingStats stc = gRingSt;
        const Hu11Leg g = Hu11Run(Hu11Tail(3840, 2160, true, 2304, 1080, 1536, 1080, true), 3840, 2160, HU11_PRODUCT, false, true);
        ViolRestore(stc);
        printf("  the overlay's copy refused: the second output from %s; the hand said %s; generated frames on the screen %d, the frame %d time(s)\n", Hu11Src(g.src), g.flags == FG_HF_NOHUD ? "no HUD-less picture" : "SOMETHING ELSE", g.gens, g.reals);
        if (g.src != FG_HUDSRC_NONE || g.flags != FG_HF_NOHUD || g.gens != 0 || g.reals != 1 || g.encAt != 10 || g.preDraws != 6)
            Err("HU11: with the overlay's copy refused: the second output from %s, flags %lu, %d generated frame(s) and the frame %d time(s) (nothing, no HUD-less picture, 0 and 1 wanted; the encode's copy at draw 10 with 6 counted: %d, %u)", Hu11Src(g.src), g.flags, g.gens, g.reals, g.encAt, g.preDraws);
    }
    // (d) NO PHONE: the encode's copy is the frame's one copy, at the first sprite, and the HUD-less picture is made from it
    {
        const Hu11Leg g = Hu11Run(Hu11Tail(3840, 2160, false, 0, 0, 0, 0, true), 3840, 2160, HU11_PRODUCT);
        printf("  (d) no phone, the HUD on: the encode's copy ahead of draw %d (%s); no overlay's copy (%d); the second output from %s; the presented picture: %s; the HUD-less picture: %s\n", g.encAt, g.encWhat, g.ovAt, Hu11Src(g.src), Hu11On(g.onFrom), Hu11On(g.onHud));
        if (g.encAt != 3 || strcmp(g.encWhat, "a sprite") || g.ovAt != -1 || g.src != FG_HUDSRC_ENCODE || g.onFrom != (sceneOnly | HU11_HUD) || g.onHud != sceneOnly || g.gens != 1)
            Err("HU11 (d): with no phone the encode's copy was taken ahead of draw %d (%s), an overlay's at %d, the second output from %s (3, the first sprite, none, the encode's own copy wanted), the pictures hold %s and %s", g.encAt, g.encWhat, g.ovAt, Hu11Src(g.src), Hu11On(g.onFrom), Hu11On(g.onHud));
    }
    // NO PHONE AND NO INTERFACE: nothing is copied; the frame is its own HUD-less picture, as it always was
    {
        const Hu11Leg g = Hu11Run(Hu11Tail(3840, 2160, false, 0, 0, 0, 0, false), 3840, 2160, HU11_PRODUCT);
        printf("  (d) no phone and no interface: interface draws %u, no copy taken (%d, %d); the hand said %s; the presented picture and the HUD-less one: %s, %s (one tag: %s); generated frames on the screen %d\n",
            g.preDraws, g.encAt, g.ovAt, g.flags == 0 ? "the frame is its own HUD-less picture" : "SOMETHING ELSE", Hu11On(g.onFrom), Hu11On(g.onHud), g.d.from == g.d.hud ? "yes" : "NO", g.gens);
        if (g.preDraws != 0 || g.encAt != -1 || g.ovAt != -1 || g.flags != 0 || g.d.from != g.d.hud || g.d.from != g.final || g.onFrom != sceneOnly || g.gens != 1) Err("HU11 (d): a frame with no phone and no interface: %u interface draw(s), copies at %d and %d, flags %lu, the two pictures %lX and %lX (0, none, none, 0, and the frame %lX in both wanted)", g.preDraws, g.encAt, g.ovAt, g.flags, g.d.from, g.d.hud, g.final);
    }
    // THE MOD'S OWN SHARPENING PASS OWED AT THE FIRST SPRITE: the rule stands back, no overlay's copy is taken (the phone stays in the picture on such a frame)
    {
        const Hu11Leg g = Hu11Run(Hu11Tail(3840, 2160, true, 2304, 1080, 1536, 1080, true), 3840, 2160, HU11_PRODUCT, true);
        printf("  the mod's own sharpening pass still owed at the first 2D draw: the encode's copy ahead of draw %d (%s); no overlay's copy (%d); the HUD-less picture: %s\n", g.encAt, g.encWhat, g.ovAt, Hu11On(g.onHud));
        if (g.encAt != 10 || strcmp(g.encWhat, "a sprite") || g.ovAt != -1 || g.src != FG_HUDSRC_ENCODE || g.onHud != (sceneOnly | HU11_PHONE)) Err("HU11: with a full-screen pass owed the encode's copy was taken ahead of draw %d (%s) and an overlay's at %d (10, the first sprite, and none wanted: a copy ahead of that pass would differ from the frame everywhere)", g.encAt, g.encWhat, g.ovAt);
    }
    // CONTROL OF (a): THE FIRST FORM'S ELSE-IF (the encode's one copy taken ahead of the phone, the phone counted): the encode's copy and its count are NOT a release's
    for (const Shape& sh : shapes)
    {
        const RingStats stc = gRingSt;
        const std::vector<Hu11Draw> tail = Hu11Tail(sh.w, sh.h, true, sh.w * 6 / 10, sh.h * 5 / 10, sh.w * 4 / 10, sh.h * 5 / 10, true);
        const Hu11Leg c = Hu11Run(tail, sh.w, sh.h, HU11_WAS), r = Hu11Run(tail, sh.w, sh.h, HU11_RELEASE);
        ViolRestore(stc);
        if (!(c.encAt == 3 && c.onEnc == sceneOnly && c.preDraws == 13 && r.encAt == 10 && r.onEnc == (sceneOnly | HU11_PHONE) && r.preDraws == 6))
            Err("HU11 control of (a): the first form's else-if took the encode's copy ahead of draw %d with %s on it and counted %u; a release: draw %d, %s, %u (3, no phone on it and 13 against 10, THE PHONE on it and 6 wanted): the control shows nothing", c.encAt, Hu11On(c.onEnc), c.preDraws, r.encAt, Hu11On(r.onEnc), r.preDraws);
        Ctl("the first form's else-if, %s: the ENCODE'S copy ahead of draw %d (%s) with %s on it, %u draw(s) counted; a release's: draw %d with %s, %u", sh.what, c.encAt, c.encWhat, Hu11On(c.onEnc), c.preDraws, r.encAt, Hu11On(r.onEnc), r.preDraws);
    }
    // CONTROL OF (b): THE ARMED PRODUCT'S QUESTION NOT ASKED (the encode's copy alone): the HUD-less picture holds the phone
    for (const Shape& sh : shapes)
    {
        const RingStats stc = gRingSt;
        const Hu11Leg c = Hu11Run(Hu11Tail(sh.w, sh.h, true, sh.w * 6 / 10, sh.h * 5 / 10, sh.w * 4 / 10, sh.h * 5 / 10, true), sh.w, sh.h, HU11_RELEASE);
        ViolRestore(stc);
        if (c.encAt != 10 || strcmp(c.encWhat, "a sprite") || c.ovAt != -1 || c.onHud != (sceneOnly | HU11_PHONE) || c.onFrom != all)
            Err("HU11 control of (b): with the armed product's question not asked the encode's copy was taken ahead of draw %d (%s) and the HUD-less picture holds %s (10, the first sprite, and the scene, the coronas and THE PHONE wanted): the control shows nothing", c.encAt, c.encWhat, Hu11On(c.onHud));
        Ctl("the armed product's question not asked, %s: the one copy ahead of draw %d (%s), and the HUD-less picture the helper read holds %s", sh.what, c.encAt, c.encWhat, Hu11On(c.onHud));
    }
    CaseEnd(name);
}

// ================================================================================================================
// SDR (2026-10-20, round 2; source/fg/fg_hud.h, "SDR"; source/fg/fg_hud_sdr.h, THE SEQUENCE; fg_hud_rule.h, its rules). CASE HU12.
// THE MOD'S OWN SEQUENCE RUNS HERE: FgHudSdr<B> (fg_hud_sdr.h) is the code the mod's hooks call (fg_hud.h's FgHudSdrOnDraw, FgHudSdrCgaArm,
// FgHudSdrCgaDraw and FgHudSdrFrame are its four one-line callers over the binding FgHudSdrPB, which makes the Direct3D 9 calls);
// this case calls the same four entry points over a binding of its own (Hu12B), which RECORDS EACH COPY AND THE CURVE'S SECOND DRAW ON
// THE FAKE RING, in DXVK's stream ahead of the draws behind it, and plants the faults. The frame is built draw by draw as HU11 builds
// it (a fill a draw that writes colour, each tag with what is on its picture: the scene, the coronas, the phone, the HUD, FusionFix's
// curve, another module's full-screen pass, the mod's notice); the hand, the HUD-less note and the Present follow; THE WORKER IS
// HELD THROUGH ALL OF IT. WHAT IS READ: the fake helper's own record of the pair's dispatch (what PRESENT and HUDLESS held when it read
// them), the screen, the curve's copy (the ring's own tag behind the release), and the sequence's counts.
// LEGS. (a) the HUD: the copy at the first sprite, the HUD-less picture the scene and the coronas. (b) A FRAME NOT ASKED (the law's
// rest, a turn of the fill: the draws not asked, FusionFix's copy and curve still followed) and behind it the first frame asked, with
// the HUD, console gamma and the notice: the latch is right, the copy goes into the curve's copy, the curve is drawn a second time,
// the curve on BOTH pictures and the HUD and the notice on the presented one alone. (c) THE PHONE with the HUD and console gamma: no
// HUD-less picture, nothing generated shown, the frame once. (d) A STRAY COPY: another module's point copy of the whole back buffer
// between EndScene and the next frame's resolve: set aside, and that next frame has its picture. (e) ANOTHER MODULE'S FULL-SCREEN PASS
// behind its copy with sampler 0 NOT the copy's texture, the curve latched: not the curve, so no picture (and the latch goes off).
// (f) the curve's second draw refused: no picture. (g) the curve with the latch off (the setting just changed): no picture.
// (h) no interface: the presented picture is its own. (i) no interface and the mod's notice: no picture. (j) A MENU: the rule asks
// nothing of a frame with no scene, and the present path stands by: six frames, nothing generated, each shown once, in order.
// In every leg with a hand: nothing reaches the screen while the worker is held, and the frame is shown exactly once.
// CONTROLS (each brings one rule's absence back, and its leg's reading must fail): the copy point behind the interface (the HUD-less
// picture holds the HUD); FusionFix's copy followed only on frames whose draws are asked (round 1's form: (b)'s first asked frame has no
// picture); every copy counted as inside the frame (round 1's form: (d)'s frame has no picture); sampler 0 not asked (round 1's form:
// (e)'s pass is taken for the curve, and the picture handed over is not the frame without its interface).
// ================================================================================================================
#include "../../source/fg/fg_hud_sdr.h"
enum : unsigned { HU12_SCENE = 1, HU12_CORONA = 2, HU12_PHONE = 4, HU12_HUD = 8, HU12_CURVE = 16, HU12_NOTICE = 32, HU12_OTHER = 64, HU12_IFACE = HU12_PHONE | HU12_HUD | HU12_NOTICE };
enum { HU12_GAME = 0, HU12_FFCOPY = 1, HU12_FFDRAW = 2, HU12_NOTE = 3 };   // (a draw the hooks ask; a module's point copy; that module's draw; the mod's own)
struct Hu12Draw { const char* what; int kind; bool ortho, sceneCam, colour, blend; int prims; DWORD x, y, w, h; unsigned layer; bool inFrame, t0Copy; };
// THE CASE'S WORLD, which the binding reads and writes (one frame at a time)
struct Hu12Env
{
    std::map<DWORD, unsigned> on; DWORD cur, preTag; bool armed, hudDrew, pic, replayOk, copyOk, late, latePending;
    bool ctlAskedOnly, ctlAllInFrame, ctlT0Any;   // (the controls: round 1's three forms)
    DWORD outW, outH; unsigned stamp; IDirect3DSurface9 *pre, *tgt;
};
static Hu12Env gHu12;
static FgHudSdrState gHu12St;
struct Hu12B
{
    static FgHudSdrState& St() { return gHu12St; }
    static unsigned Stamp() { return gHu12.stamp; }
    static bool SceneBefore() { return true; }
    static bool Resolved() { return true; }
    static bool RtIsBackBuffer() { return true; }
    static bool OpaqueQuad(const Hu12Draw& d) { return d.prims <= 2 && !d.blend && d.colour; }
    static bool Overlay3d(const Hu12Draw& d, unsigned bbBefore, bool sceneBefore)
    { return Hu11Overlay({ d.what, d.ortho, d.sceneCam, d.colour, d.blend, d.prims, d.x, d.y, d.w, d.h, 0 }, bbBefore, sceneBefore ? 3000u : 0u, gHu12.outW, gHu12.outH, false); }
    static int Copy(const Hu12Draw&, bool intoPre)
    {
        if (!gHu12.copyOk) return 0;
        if (gHu12.late) { gHu12.latePending = true; return 1; }        // (control: the copy is made behind the last sprite instead)
        D->StretchRect(gBb, nullptr, intoPre ? gHu12.pre : gHu12.tgt, nullptr, D3DTEXF_NONE);
        gHu12.preTag = gHu12.cur;
        return 1;
    }
    static bool InFrame() { return true; }                             // (Hu12Copy below answers it per copy: the copy's own field)
    static bool Arm(const Hu12Draw&) { return true; }
    static void SetArmed(bool a) { gHu12.armed = a; }
    static bool SameModule(const Hu12Draw&) { return true; }
    static FgHudSdrCurveFacts CurveFacts(const Hu12Draw& m) { return { true, true, true, true, true, true, true, true, true, gHu12.ctlT0Any || m.t0Copy }; }
    static bool Replay(const Hu12Draw&)
    {
        if (!gHu12.replayOk) return false;
        const DWORD tc = NewTag(); gHu12.on[tc] = gHu12.on[gHu12.preTag] | HU12_CURVE;    // (the curve over what the curve's copy took: read back below)
        D->ColorFill(gHu12.tgt, nullptr, tc);
        return true;
    }
    static void Publish(bool hudDrew, bool pic) { gHu12.hudDrew = hudDrew; gHu12.pic = pic; }
};
// (the binding's InFrame for one copy: the case's own field, or every copy inside the frame under the control)
struct Hu12BCopy : Hu12B { static inline bool inFrame = true; static bool InFrame() { return gHu12.ctlAllInFrame || inFrame; } };
typedef FgHudSdr<Hu12B> Hu12S;
// The frame's tail behind the composite, at a back buffer of outW by outH: the composite (the frame's first back-buffer draw, the whole
// target), the quad that restores the depth, two coronas, THE PHONE where one is up, the HUD where it is on (four sprites, a text batch,
// the frame's final blended quad), then where asked FusionFix's copy and its draw (the curve, or another pass), and the mod's notice.
static std::vector<Hu12Draw> Hu12Tail(DWORD outW, DWORD outH, bool phone, bool hudOn, int ff, bool notice, bool ffT0 = true)
{
    std::vector<Hu12Draw> t;
    t.push_back({ "the composite", HU12_GAME, false, false, true, false, 2, 0, 0, outW, outH, 0, true, true });
    t.push_back({ "the quad that restores the depth", HU12_GAME, false, false, false, false, 2, 0, 0, outW, outH, 0, true, true });
    for (int i = 0; i < 2; i++) t.push_back({ "a corona", HU12_GAME, false, true, true, true, 128, 0, 0, outW, outH, HU12_CORONA, true, true });
    if (phone) for (int i = 0; i < 7; i++) t.push_back({ "the phone", HU12_GAME, false, false, true, true, 742, outW * 12 / 20, outH * 10 / 20, outW * 8 / 20, outH * 10 / 20, HU12_PHONE, true, true });
    if (hudOn)
    {
        for (int i = 0; i < 4; i++) t.push_back({ "a sprite", HU12_GAME, true, false, true, true, 2, 0, 0, outW, outH, HU12_HUD, true, true });
        t.push_back({ "a text batch", HU12_GAME, true, false, true, true, 90, 0, 0, outW, outH, HU12_HUD, true, true });
        t.push_back({ "the final quad", HU12_GAME, true, false, true, true, 2, 0, 0, outW, outH, HU12_HUD, true, true });
    }
    if (ff)
    {
        t.push_back({ "FusionFix's point copy", HU12_FFCOPY, false, false, false, false, 0, 0, 0, outW, outH, 0, true, true });
        t.push_back({ ff == 1 ? "FusionFix's curve" : "another pass of the module's", HU12_FFDRAW, true, false, true, false, 2, 0, 0, outW, outH, ff == 1 ? HU12_CURVE : HU12_OTHER, true, ffT0 });
    }
    if (notice) t.push_back({ "the mod's notice", HU12_NOTE, true, false, true, true, 2, 0, 0, outW, outH, HU12_NOTICE, true, true });
    return t;
}
// ONE FRAME'S EVENTS through the sequence, in the hooks' order (asked: its draws are asked; watch: FusionFix's copy and draw followed,
// which they always are in the mod while the session is SDR; the control ctlAskedOnly follows them on asked frames alone)
static void Hu12Events(const std::vector<Hu12Draw>& tail, bool asked, bool ring)
{
    const bool watch = !gHu12.ctlAskedOnly || asked;
    for (size_t i = 0; i < tail.size(); i++)
    {
        const Hu12Draw& d = tail[i];
        if (d.kind == HU12_FFCOPY)
        {
            static FakeSurf* const ffSurf = new FakeSurf(1920, 1080, nullptr, D3DUSAGE_RENDERTARGET);   // (the module's own copy of the frame)
            if (ring) D->StretchRect(gBb, nullptr, ffSurf, nullptr, D3DTEXF_POINT);
            if (watch) { Hu12BCopy::inFrame = d.inFrame; FgHudSdr<Hu12BCopy>::OnCopy(d); Hu12BCopy::inFrame = true; }
            continue;
        }
        if (d.kind == HU12_FFDRAW && gHu12.armed) Hu12S::OnModuleDraw(d);
        if (d.kind == HU12_GAME && asked) Hu12S::OnDraw(d, d.ortho, d.sceneCam);
        if (ring && d.colour && d.layer) { const DWORD t = NewTag(); gHu12.on[t] = gHu12.on[gHu12.cur] | d.layer; Fill(t); gHu12.cur = t; }
        if (ring && gHu12.latePending && d.kind == HU12_GAME && !strcmp(d.what, "the final quad"))
        { gHu12.latePending = false; D->StretchRect(gBb, nullptr, gHu12.tgt, nullptr, D3DTEXF_NONE); gHu12.preTag = gHu12.cur; }
    }
}
// A FRAME NOT ASKED (no machine of its own here: the pure sequence over its events, as the mod runs it behind a frame whose draws are not asked)
static void Hu12Unasked(const std::vector<Hu12Draw>& tail) { gHu12.stamp++; Hu12Events(tail, false, false); Hu12S::Frame(false); }
struct Hu12Leg { DWORD scene, final; LONG seen; int gens, reals; DWORD flags; bool armed, got; ShDisp d; unsigned onFrom, onHud; DWORD preTag, preHeld; FgHudSdrState before, after; };
// ONE FRAME ASKED, through the machine with the worker held. strayFirst: a stray copy (between EndScene and this frame's resolve) ahead of it.
static Hu12Leg Hu12Run(const std::vector<Hu12Draw>& tail, DWORD outW, DWORD outH, bool strayFirst = false)
{
    Hu12Leg g = {};
    MgFresh<MA>(); HuTexOn();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    gHu12.on.clear(); gHu12.outW = outW; gHu12.outH = outH; gHu12.pre = RingSurf(FG_SURF_HOLD); gHu12.tgt = ShArmB::HudSrcSurf();
    gHu12.armed = false; gHu12.latePending = false; gHu12.hudDrew = false; gHu12.pic = false;
    bool notice = false; for (const Hu12Draw& d : tail) notice = notice || d.kind == HU12_NOTE;
    g.before = gHu12St;
    const size_t sh0 = MgShownN();
    gHuSeen = -1;
    MgHeld([&] {
        gHu12.stamp++;
        if (strayFirst) { const Hu12Draw st = { "a stray copy", HU12_FFCOPY, false, false, false, false, 0, 0, 0, outW, outH, 0, false, true }; Hu12Events({ st }, true, true); }
        g.scene = MgRecBegin(); gHu12.on[g.scene] = HU12_SCENE; gHu12.cur = g.scene;
        Hu12Events(tail, true, true);
        g.final = gHu12.cur; g.preTag = gHu12.preTag;
        Hu12S::Frame(true);
        g.flags = !gHu12.hudDrew ? (notice ? (DWORD)FG_HF_NOHUD : 0u) : gHu12.pic ? (DWORD)FG_HF_HUDTEX : (DWORD)FG_HF_NOHUD;   // (FgHudWant's answer)
        MgRecHand(g.flags); RecordHudless(); RecordPresent();
    }, &HuLook);
    if (!ShSettle()) Err("HU12: the fake helper did not read the frame's messages");
    g.seen = gHuSeen < 0 ? -1 : gHuSeen - (LONG)sh0;
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    g.got = HuDispOf(ShB::x.w.lastQN, &g.d);
    MgDueShow(); MgDueN();
    g.gens = MgGenCount(sh0); g.reals = MgTagCount(sh0, g.final);
    g.onFrom = gHu12.on.count(g.d.from) ? gHu12.on[g.d.from] : 0; g.onHud = gHu12.on.count(g.d.hud) ? gHu12.on[g.d.hud] : 0;
    g.preHeld = RingTag(RingImgOfSurf(gHu12.pre));
    g.after = gHu12St;
    MgClean("HU12");
    MgEnd();
    return g;
}
static const char* Hu12On(unsigned m)
{
    static char b[8][128]; static int k = 0; char* o = b[k++ & 7];
    snprintf(o, 128, "%s%s%s%s%s%s%s", m & HU12_SCENE ? "the scene" : "NOTHING OF THIS FRAME", m & HU12_CORONA ? ", the coronas" : "", m & HU12_PHONE ? ", THE PHONE" : "",
             m & HU12_HUD ? ", THE HUD" : "", m & HU12_CURVE ? ", the curve" : "", m & HU12_OTHER ? ", ANOTHER PASS" : "", m & HU12_NOTICE ? ", THE NOTICE" : "");
    return o;
}
static void Hu12Reset(bool latch) { gHu12St = FgHudSdrState(); gHu12St.latch = latch; gHu12.copyOk = true; gHu12.replayOk = true; gHu12.late = false; gHu12.ctlAskedOnly = false; gHu12.ctlAllInFrame = false; gHu12.ctlT0Any = false; }
// THE READINGS. A leg with its picture: the pictures the helper read, the generated frame and the frame each shown once. A leg with none: fed
// with the presented picture in both places, nothing generated shown, the frame once.
static bool Hu12PicOk(const Hu12Leg& g, unsigned wantHud, unsigned wantFrom)
{
    return g.seen == 0 && g.armed && g.got && g.flags == FG_HF_HUDTEX && g.onHud == wantHud && g.onFrom == wantFrom && !(g.onHud & HU12_IFACE) && (g.onFrom & ~HU12_IFACE) == g.onHud && g.gens == 1 && g.reals == 1;
}
static bool Hu12NoneOk(const Hu12Leg& g) { return g.seen == 0 && g.flags == FG_HF_NOHUD && (!g.got || g.d.hud == g.d.from) && g.gens == 0 && g.reals == 1; }
static void Hu12Say(const char* leg, const Hu12Leg& g)
{
    printf("  %s: %ld picture(s) on the screen while held; flags %lu; the helper read as the presented picture %s, as the HUD-less picture %s; generated frames shown %d, the frame %d time(s);"
           " the sequence: copies %ld, curves %ld, drawn a second time %ld, odd %ld, set aside %ld, the phone %ld, pictures %ld; the latch now %d\n",
           leg, g.seen, g.flags, Hu12On(g.onFrom), Hu12On(g.onHud), g.gens, g.reals, g.after.copies - g.before.copies, g.after.curves - g.before.curves, g.after.replays - g.before.replays,
           g.after.odd - g.before.odd, g.after.strays - g.before.strays, g.after.phoneFrames - g.before.phoneFrames, g.after.pics - g.before.pics, (int)gHu12St.latch);
}
static void CaseHU12()
{
    const char* name = "HU12 SDR: the mod's own sequence (fg_hud_sdr.h): the copy at the interface's first draw, FusionFix's curve told by its draw inside the frame and drawn a second time, the phone, the latch across a rest (2026-10-20, round 2)";
    CaseBegin(name);
    const DWORD W4 = 3840, H4 = 2160;
    const unsigned sc = HU12_SCENE | HU12_CORONA;
    const std::vector<Hu12Draw> curveTail = Hu12Tail(W4, H4, false, false, 1, false);           // (a frame with console gamma and no HUD: a rest's events)
    {   // (a) the HUD
        Hu12Reset(false);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 0, false), W4, H4); Hu12Say("(a) the HUD", g);
        if (!Hu12PicOk(g, sc, sc | HU12_HUD)) Err("HU12 (a): the HUD: flags %lu, %s and %s, %d and %d (a picture of the scene and the coronas, 1 and 1 wanted)", g.flags, Hu12On(g.onFrom), Hu12On(g.onHud), g.gens, g.reals);
    }
    {   // (b) a frame not asked, then the first frame asked, the HUD, console gamma and the notice
        Hu12Reset(false);
        Hu12Unasked(curveTail);
        const bool latch = gHu12St.latch;
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 1, true), W4, H4); Hu12Say("(b) a frame not asked, then the first asked: the HUD, console gamma, the notice", g);
        if (!latch || !Hu12PicOk(g, sc | HU12_CURVE, sc | HU12_HUD | HU12_CURVE | HU12_NOTICE) || g.preHeld != g.preTag || g.after.replays - g.before.replays != 1)
            Err("HU12 (b): the latch behind the frame not asked %d; flags %lu, %s and %s; the curve's copy %lX against %lX; drawn a second time %ld (1, a picture with the curve, the copy the frame's, 1 wanted)",
                (int)latch, g.flags, Hu12On(g.onFrom), Hu12On(g.onHud), g.preHeld, g.preTag, g.after.replays - g.before.replays);
    }
    {   // (c) the phone
        Hu12Reset(true);
        const Hu12Leg g = Hu12Run(Hu12Tail(2560, 1440, true, true, 1, false), 2560, 1440); Hu12Say("(c) the phone, the HUD and console gamma", g);
        if (!Hu12NoneOk(g) || g.after.phoneFrames - g.before.phoneFrames != 1 || g.after.copies != g.before.copies) Err("HU12 (c): the phone: flags %lu, %d and %d, phone frames %ld, copies %ld (no picture, 0 and 1, 1, 0 wanted)", g.flags, g.gens, g.reals, g.after.phoneFrames - g.before.phoneFrames, g.after.copies - g.before.copies);
    }
    {   // (d) a stray copy between EndScene and this frame's resolve
        Hu12Reset(false);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 0, false), W4, H4, true); Hu12Say("(d) a stray copy ahead of the frame", g);
        if (!Hu12PicOk(g, sc, sc | HU12_HUD) || g.after.strays - g.before.strays != 1 || gHu12St.latch) Err("HU12 (d): a stray copy: flags %lu, %s, set aside %ld, the latch %d (a picture, 1, 0 wanted)", g.flags, Hu12On(g.onHud), g.after.strays - g.before.strays, (int)gHu12St.latch);
    }
    {   // (e) another module's pass behind its copy, sampler 0 not the copy's texture, the curve latched
        Hu12Reset(true);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 2, false, false), W4, H4); Hu12Say("(e) another pass of the module's, sampler 0 not its copy", g);
        if (!Hu12NoneOk(g) || g.after.odd - g.before.odd != 1 || g.after.notCurves - g.before.notCurves != 1 || gHu12St.latch) Err("HU12 (e): another pass: flags %lu, %d generated, odd %ld, not the curve %ld, the latch %d (no picture, 0, 1, 1, 0 wanted)", g.flags, g.gens, g.after.odd - g.before.odd, g.after.notCurves - g.before.notCurves, (int)gHu12St.latch);
    }
    {   // (f) the curve's second draw refused
        Hu12Reset(true); gHu12.replayOk = false;
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 1, false), W4, H4); Hu12Say("(f) the curve's second draw refused", g);
        if (!Hu12NoneOk(g) || g.after.replayFails - g.before.replayFails != 1) Err("HU12 (f): the second draw refused: flags %lu, %d generated, refused %ld (no picture, 0, 1 wanted)", g.flags, g.gens, g.after.replayFails - g.before.replayFails);
        gHu12.replayOk = true;
    }
    {   // (g) the curve with the latch off
        Hu12Reset(false);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, true, 1, false), W4, H4); Hu12Say("(g) console gamma, the latch off (the setting just changed)", g);
        if (!Hu12NoneOk(g) || !gHu12St.latch || g.after.disagree - g.before.disagree != 1) Err("HU12 (g): the latch off: flags %lu, the next latch %d, disagreed %ld (no picture, 1, 1 wanted)", g.flags, (int)gHu12St.latch, g.after.disagree - g.before.disagree);
    }
    {   // (h) no interface
        Hu12Reset(false);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, false, 0, false), W4, H4); Hu12Say("(h) no interface", g);
        if (g.seen != 0 || g.flags != 0 || !g.got || g.d.hud != g.d.from || g.d.from != g.final || g.onFrom != sc || g.gens != 1 || g.reals != 1)
            Err("HU12 (h): no interface: seen %ld, flags %lu, the two pictures %lX and %lX, the frame %lX, generated %d, the frame %d time(s) (0, 0, the frame in both, 1, 1 wanted)", g.seen, g.flags, g.d.from, g.d.hud, g.final, g.gens, g.reals);
    }
    {   // (i) no interface, the mod's notice
        Hu12Reset(false);
        const Hu12Leg g = Hu12Run(Hu12Tail(W4, H4, false, false, 0, true), W4, H4); Hu12Say("(i) no interface, the mod's notice", g);
        if (!Hu12NoneOk(g)) Err("HU12 (i): the notice alone: flags %lu, %d and %d (no picture, 0 and 1 wanted)", g.flags, g.gens, g.reals);
    }
    {   // (j) A MENU: no scene, sprites alone. The rule asks nothing of the frame; the present path stands by.
        const bool asked = FgHudSdrBoundaryRule(true, false, false, false);
        MgFresh<MA>(); HuTexOn();
        _InterlockedExchange(&ShB::mayEngage, 0);
        const size_t sh0 = MgShownN();
        std::vector<DWORD> tags;
        for (int i = 0; i < 6; i++) tags.push_back(ShFrame<MA>());
        Drain(); ShSettle();
        const auto v = Shown();
        std::vector<DWORD> real; int gens = 0;
        for (size_t i = sh0; i < v.size(); i++) { if (ShIsGen(v[i].tag)) gens++; else real.push_back(v[i].tag); }
        _InterlockedExchange(&ShB::mayEngage, 1);
        printf("  (j) a menu: the boundary asked of a frame with no scene: %s; six frames standing by: %d generated frame(s) shown, %zu real frame(s) shown, in order %s\n",
               asked ? "YES" : "no", gens, real.size(), real == tags ? "yes" : "NO");
        if (asked || gens != 0 || real != tags) Err("HU12 (j): a menu: the boundary asked %d, %d generated frame(s), the real frames %s (no, 0, each once in order wanted)", (int)asked, gens, real == tags ? "in order" : "NOT each once in order");
        MgClean("HU12 (j)");
        MgEnd();
    }
    // CONTROL 1: THE COPY POINT BEHIND THE INTERFACE (taken behind the last sprite): the HUD-less picture holds the HUD
    {
        const RingStats stc = gRingSt;
        Hu12Reset(false); gHu12.late = true;
        const Hu12Leg c = Hu12Run(Hu12Tail(W4, H4, false, true, 0, false), W4, H4);
        gHu12.late = false; ViolRestore(stc);
        if (!(c.onHud & HU12_HUD)) Err("HU12 control 1: with the copy behind the last sprite the HUD-less picture holds %s (THE HUD on it wanted): the control shows nothing", Hu12On(c.onHud));
        Ctl("the copy point behind the interface: the HUD-less picture the helper read holds %s", Hu12On(c.onHud));
    }
    // CONTROL 2: FUSIONFIX'S COPY FOLLOWED ONLY WHERE THE DRAWS ARE ASKED (round 1): (b)'s first asked frame has no picture
    {
        const RingStats stc = gRingSt;
        Hu12Reset(false); gHu12.ctlAskedOnly = true;
        Hu12Unasked(curveTail);
        const Hu12Leg c = Hu12Run(Hu12Tail(W4, H4, false, true, 1, true), W4, H4);
        gHu12.ctlAskedOnly = false; ViolRestore(stc);
        if (Hu12PicOk(c, sc | HU12_CURVE, sc | HU12_HUD | HU12_CURVE | HU12_NOTICE) || c.gens != 0) Err("HU12 control 2: with the copy followed on asked frames alone the first asked frame still had its picture (flags %lu, %d generated): the control shows nothing", c.flags, c.gens);
        Ctl("FusionFix's copy followed on asked frames alone: the first asked frame behind a rest has flags %lu and %d generated frame(s) shown", c.flags, c.gens);
    }
    // CONTROL 3: EVERY COPY COUNTED INSIDE THE FRAME (round 1): (d)'s frame has no picture
    {
        const RingStats stc = gRingSt;
        Hu12Reset(false); gHu12.ctlAllInFrame = true;
        const Hu12Leg c = Hu12Run(Hu12Tail(W4, H4, false, true, 0, false), W4, H4, true);
        gHu12.ctlAllInFrame = false; ViolRestore(stc);
        if (Hu12PicOk(c, sc, sc | HU12_HUD) || c.gens != 0) Err("HU12 control 3: with every copy counted inside the frame the frame behind a stray copy still had its picture (flags %lu, %d generated): the control shows nothing", c.flags, c.gens);
        Ctl("every copy counted inside the frame: the frame behind a stray copy has flags %lu and %d generated frame(s) shown", c.flags, c.gens);
    }
    // CONTROL 4: SAMPLER 0 NOT ASKED (round 1): (e)'s pass is taken for the curve, and the picture handed over is not the frame without its interface
    {
        const RingStats stc = gRingSt;
        Hu12Reset(true); gHu12.ctlT0Any = true;
        const Hu12Leg c = Hu12Run(Hu12Tail(W4, H4, false, true, 2, false, false), W4, H4);
        gHu12.ctlT0Any = false; ViolRestore(stc);
        if (!(c.flags == FG_HF_HUDTEX && (c.onFrom & ~HU12_IFACE) != c.onHud)) Err("HU12 control 4: with sampler 0 not asked another pass was not handed as a wrong picture (flags %lu, %s against %s): the control shows nothing", c.flags, Hu12On(c.onHud), Hu12On(c.onFrom));
        Ctl("sampler 0 not asked: another pass taken for the curve, the picture handed over holds %s against the presented %s", Hu12On(c.onHud), Hu12On(c.onFrom));
    }
    Hu12Reset(false);
    CaseEnd(name);
}

// ================================================================================================================
// THE SECOND TARGET MOVED BY DXVK'S DEFRAGMENTER (2026-10-24; the SDR menu crash of the sitting of that day, docs\reports\fg-5\
// sitting-2026-10-24\READING.md; fg_hud_rule.h, FgHudSrcTake, the mod's own lines, which ShArmB::HudSrc runs). CASE HU13.
// THE CRASH: entering the game's menu with frame generation on in SDR faulted in NVIDIA's driver under the hand's span (ArmedSpan, the
// copy out of the HUD-less second target: FgPA::RecCopyHudToImg). The target is a texture of DXVK's, and DXVK's defragmenter moves such
// a texture to a NEW VkImage at a point of its own stream and frees the old one; the mod read the image ONCE, where the target was made,
// and named it in every hand's copy. THE ORDER THIS DRIVES: a plain pair whose picture stands in the binding's own image, the worker
// HELD, and queued behind it, in the stream's order: the frame's picture written into the image, THE MOVE (the fake's kRingMove: the
// surface gets a new image holding the picture, the old image is dead), the hand, and the NEXT frame's write into the image. Released:
// the hand's span flushes, asks the image of NOW (the moved one), and copies out of it.
// WHAT IS HELD: the move was made; no raw copy reached the queue naming a dead image (deadImage, a violation the case's end counts);
// the helper's HUDLESS holds THIS frame's picture, and its dispatch read it; the generated frame is shown and the frame once; nothing
// reached the screen while the worker was held.
// CONTROL (a build of its own: -DFGHUD_SRC_CACHED, which brings back 31e8a2ee's form, the image read at the make): the copy names the
// dead image, the queue counts it (deadImage), and the helper's HUDLESS does not hold the picture: this case FAILS.
// ================================================================================================================
static void CaseHU13()
{
    const char* name = "HU13 the second target moved by DXVK's defragmenter between the frame's picture and its hand: the hand's span names the image of NOW (2026-10-24, the SDR menu crash)";
    CaseBegin(name);
    MgFresh<MA>();
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a; FgState& s = B::S();
    HuTexOn();
    ShWarm<MA>(); MgDueN();
    const LONG moves0 = gRingSt.moves, dead0 = gRingSt.deadImage, fed0 = a.w.hudTexFed, lost0 = a.w.hudTexLost;
    const size_t shownAt = MgShownN();
    DWORD tagA = 0, hudA = 0, hudLate = 0; gHuSeen = -1;
    _InterlockedExchange(&ShArmB::moveAfterFill, 1);
    MgHeld([&] { ClockAdd(16.7); ShInject(true); tagA = Frame<MA>(true); hudA = ShArmB::hudTag; _InterlockedExchange(&ShArmB::moveAfterFill, 0);
                 hudLate = NewTag(); D->ColorFill(ShArmB::HudSrcSurf(), nullptr, hudLate); }, &HuLook);
    if (!ShSettle()) Err("HU13: the fake helper did not read the frame's messages");
    const LONG nA = x.w.lastQN;
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const DWORD hudHeld = RingTag(RingRig(FG_IMG_HUDLESS)), ownNow = RingTag(ShArmB::HudSrcImg());
    ShDisp dA = {}; const bool gotA = HuDispOf(nA, &dA);
    MgDueShow(); MgDueN();
    const int gens = MgGenCount(shownAt), reals = MgTagCount(shownAt, tagA);
    const LONG moves = gRingSt.moves - moves0, dead = gRingSt.deadImage - dead0, fed = a.w.hudTexFed - fed0, lost = a.w.hudTexLost - lost0;
    printf("  the binding's image moved behind the frame's picture, the worker held with the next frame's write queued behind it: %ld picture(s) on the screen while held; armed %s;"
           " moves made %ld; raw copies that reached the queue naming a dead image %ld; HUDLESS %lX (the frame's picture %lX; the next one's, %lX, is what the moved image holds now: %lX);"
           " the helper's dispatch read %lX and %lX; copied from the image %ld, not nameable %ld; generated frames shown %d, the frame %d time(s)\n",
        (long)(gHuSeen < 0 ? -1 : gHuSeen - (LONG)shownAt), armedA ? "yes" : "NO", moves, dead, hudHeld, hudA, hudLate, ownNow, dA.from, dA.hud, fed, lost, gens, reals);
    if (gHuSeen != (LONG)shownAt) Err("HU13: %ld picture(s) reached the screen while the worker was held (0 wanted)", (long)(gHuSeen - (LONG)shownAt));
    if (moves != 1) Err("HU13: %ld move(s) of the binding's image were made (1 wanted: the case did not plant what it reads)", moves);
    if (dead != 0) Err("HU13: %ld raw copy(ies) reached the queue naming an image the move had freed (0 wanted: in the game the driver faults there)", dead);
    // (where the NEXT frame's write lands behind a move is the fake's stream model, printed above and not judged here: what is held is the
    // copy this hand's span made)
    if (!armedA || hudHeld != hudA || !gotA || dA.hud != hudA || dA.from != tagA || fed != 1 || lost != 0)
        Err("HU13: armed %d; HUDLESS %lX, the dispatch read %lX and %lX (the frame's picture %lX and the frame %lX wanted); copied from the image %ld, not nameable %ld (1 and 0 wanted)",
            (int)armedA, hudHeld, dA.from, dA.hud, hudA, tagA, fed, lost);
    if (gens != 1 || reals != 1) Err("HU13: the screen showed %d generated frame(s) and the frame %d time(s) (1 and 1 wanted)", gens, reals);
    _InterlockedExchange(&ShArmB::moveAfterFill, 0);
    MgClean("HU13");
    MgEnd();
    CaseEnd(name);
}

#include "../../source/fg/fg_web_rule.h"     // (the web page's pure half, as the mod compiles it: WP1)
// THE GAME'S WEB PAGE (2026-10-13; source/fg/fg_web.h and fg_web_rule.h; docs/reports/fg-4/web-2026-10-13). CASE WP1.
// The mod finds the id of the script's HTML viewport in the game's code by shape, from two directions, and stands by
// while it is above zero. The search and the rule are pure (fg_web_rule.h) and are run here as the mod compiles them,
// over a block of code LAID OUT AS THE GAME'S THREE BUILDS LAY IT OUT (the instructions' own bytes, as read in GTAIV.exe
// 1.0.4.0, 1.0.8.0 and the Complete Edition; the addresses are this case's own): the frame function's compare beside
// the menu flag in both encodings, the three writers, readers, and noise that names the same addresses in other
// instructions. tools/test-fg-web runs the same function over the real executables.
//   (a) FOUND, in the 1.0.8.0 encoding (cmp [a], ebx; mov al, [flag]) and in the Complete Edition's (cmp [a], 0; mov bl, [flag]);
//   (b) THE RULE: -1 (the game's init) and 0 (the viewport gone) are "no page", 1 and above "a page"; a present stands by for
//       a page, and not where the development switch lifts it.
// CONTROLS, each of which must be REFUSED WITH ITS OWN REASON: no menu flag; the shape nowhere (the compare not right before
// the load; the byte not set to 1); two sites naming two addresses; an address outside the image; the id never set to
// -1; set to 0 twice; stored from eax twice. And the mod's test of the VALUE is the rule's: a control reads it wrongly
// (as "not zero") and takes the game's init for a page.
struct Wp1Code { std::vector<unsigned char> b; };
static void Wp1Put(Wp1Code& c, std::initializer_list<unsigned char> bytes) { for (unsigned char x : bytes) c.b.push_back(x); }
static void Wp1U32(Wp1Code& c, uint32_t v) { for (int i = 0; i < 4; i++) c.b.push_back((unsigned char)(v >> (8 * i))); }
static void Wp1Noise(Wp1Code& c, int n) { for (int i = 0; i < n; i++) c.b.push_back((unsigned char)(0x90 + (i * 7) % 5)); }
enum : unsigned { WP1_CE = 1, WP1_NO_INIT = 2, WP1_TWO_CLEARS = 4, WP1_TWO_STORES = 8, WP1_SECOND_SITE = 16, WP1_NOT_ADJACENT = 32, WP1_BYTE_ZERO = 64, WP1_OUTSIDE = 128 };
static const uint32_t kWp1Lo = 0x00400000u, kWp1Hi = 0x01C00000u, kWp1Flag = 0x010FC63Fu, kWp1Id = 0x018CAF10u, kWp1Byte = 0x018CAFF4u, kWp1Other = 0x018CB000u;
static Wp1Code Wp1Make(unsigned how)
{
    Wp1Code c; const uint32_t id = (how & WP1_OUTSIDE) ? 0x02000000u : kWp1Id;
    Wp1Noise(c, 64);
    if (!(how & WP1_NO_INIT)) { Wp1Put(c, { 0xC7, 0x05 }); Wp1U32(c, id); Wp1U32(c, 0xFFFFFFFFu); }             // the game's init: mov dword ptr [id], -1
    Wp1Noise(c, 40);
    Wp1Put(c, { 0x8B, 0x35 }); Wp1U32(c, id); Wp1Put(c, { 0x33, 0xDB, 0x3B, 0xF3, 0x7E, 0x51 });                  // a reader: mov esi, [id]; xor ebx, ebx; cmp esi, ebx; jle
    Wp1Noise(c, 24);
    // the frame function
    if (how & WP1_CE) { Wp1Put(c, { 0x83, 0x3D }); Wp1U32(c, id); Wp1Put(c, { 0x00 }); }                          // cmp dword ptr [id], 0
    else              { Wp1Put(c, { 0x39, 0x1D }); Wp1U32(c, id); }                                               // cmp dword ptr [id], ebx
    if (how & WP1_NOT_ADJACENT) Wp1Put(c, { 0x90 });
    if (how & WP1_CE) { Wp1Put(c, { 0x8A, 0x1D }); Wp1U32(c, kWp1Flag); } else { Wp1Put(c, { 0xA0 }); Wp1U32(c, kWp1Flag); }   // mov bl / al, [flag]
    Wp1Put(c, { 0xC6, 0x05 }); Wp1U32(c, kWp1Byte); Wp1Put(c, { (unsigned char)((how & WP1_BYTE_ZERO) ? 0x00 : 0x01) });       // mov byte ptr [x], 1
    Wp1Put(c, { 0x7F, 0x06 });                                                                                    // jg
    Wp1Noise(c, 30);
    // the menu flag in the shape the vote of the mod reads, and in a plain load: they name the flag and are not the shape
    Wp1Put(c, { 0x80, 0x3D }); Wp1U32(c, kWp1Flag); Wp1Put(c, { 0x00, 0x74, 0x0D, 0xE8, 1, 2, 3, 4, 0x84, 0xC0 });
    Wp1Put(c, { 0xA0 }); Wp1U32(c, kWp1Flag); Wp1Put(c, { 0x84, 0xC0, 0x74, 0x10 });
    if (how & WP1_SECOND_SITE) { Wp1Noise(c, 9); Wp1Put(c, { 0x39, 0x1D }); Wp1U32(c, kWp1Other); Wp1Put(c, { 0xA0 }); Wp1U32(c, kWp1Flag); Wp1Put(c, { 0xC6, 0x05 }); Wp1U32(c, kWp1Byte); Wp1Put(c, { 0x01 }); }
    Wp1Noise(c, 50);
    Wp1Put(c, { 0x89, 0x06 }); Wp1Put(c, { 0xA3 }); Wp1U32(c, id);                                                // the native: mov [esi], eax; mov [id], eax
    if (how & WP1_TWO_STORES) { Wp1Noise(c, 5); Wp1Put(c, { 0xA3 }); Wp1U32(c, id); }
    Wp1Noise(c, 44);
    Wp1Put(c, { 0xC7, 0x05 }); Wp1U32(c, id); Wp1U32(c, 0u); Wp1Put(c, { 0x8B, 0xCF });                           // the destructor: mov dword ptr [id], 0; mov ecx, edi
    if (how & WP1_TWO_CLEARS) { Wp1Noise(c, 5); Wp1Put(c, { 0xC7, 0x05 }); Wp1U32(c, id); Wp1U32(c, 0u); }
    Wp1Noise(c, 20);
    Wp1Put(c, { 0x83, 0x3D }); Wp1U32(c, id); Wp1Put(c, { 0x00, 0xC6, 0x44, 0x24, 0x13, 0x00, 0x7E, 0x05 });      // a reader: cmp dword ptr [id], 0; mov byte ptr [esp+13h], 0; jle
    Wp1Put(c, { 0x3B, 0x05 }); Wp1U32(c, id);                                                                     // a reader: cmp eax, [id]
    Wp1Put(c, { 0xC7, 0x05 }); Wp1U32(c, kWp1Byte); Wp1U32(c, 0xFFFFFFFFu);                                       // the init of another global to -1: not the id's
    Wp1Noise(c, 64);
    return c;
}
static void CaseWP1()
{
    const char* name = "WP1 the game's web page: its state is found in the game's code by shape from two directions, or not at all, and a present stands by while it is above zero (2026-10-13)";
    CaseBegin(name);
    FgWebFound f;
    // (a) FOUND, in both encodings
    for (unsigned enc : { 0u, (unsigned)WP1_CE })
    {
        const Wp1Code c = Wp1Make(enc);
        const uint32_t a = FgWebFindIn(c.b.data(), c.b.size(), kWp1Flag, kWp1Lo, kWp1Hi, &f);
        if (a != kWp1Id || f.why != FG_WEB_FOUND || f.sites != 1 || f.others != 0 || f.setM1 != 1 || f.set0 != 1 || f.setEax != 1)
            Err("WP1 (a): in %s encoding the search answered %08X (%s; %d site(s), %d other, written -1 %d, 0 %d, from eax %d) where the id is at %08X, named once and written once each way",
                enc ? "the Complete Edition's" : "1.0.8.0's", a, FgWebWhy(f.why), f.sites, f.others, f.setM1, f.set0, f.setEax, kWp1Id);
    }
    // (b) THE RULE
    if (FgWebUpRule(-1) || FgWebUpRule(0) || !FgWebUpRule(1) || !FgWebUpRule(0x7FFF) || FgWebStandByRule(-1, false) || FgWebStandByRule(0, false) || !FgWebStandByRule(1, false) || FgWebStandByRule(1, true) || FgWebStandByRule(0, true))
        Err("WP1 (b): the rule: -1 and 0 are no page, 1 and above a page; a present stands by for a page unless the switch lifts it");
    if ((FG_WEB_LIFT & 0x3FFFFFFFu) != 0 || FG_WEB_LIFT != 0x40000000u) Err("WP1 (b): the development switch is bit 30 of fg_lift, above every row of the conditions");
    // THE CONTROLS: each refused, with its own reason
    struct Ctl1 { unsigned how; uint32_t flag; int why; const char* what; } const ctl[] = {
        { 0, 0, FG_WEB_NO_FLAG, "no menu flag" },
        { WP1_NOT_ADJACENT, kWp1Flag, FG_WEB_NO_SITE, "the compare one byte away from the load of the flag" },
        { WP1_CE | WP1_NOT_ADJACENT, kWp1Flag, FG_WEB_NO_SITE, "the same, the Complete Edition's encoding" },
        { WP1_BYTE_ZERO, kWp1Flag, FG_WEB_NO_SITE, "the byte behind the load set to 0, not to 1" },
        { WP1_OUTSIDE, kWp1Flag, FG_WEB_NO_SITE, "the compared address outside the image" },
        { WP1_SECOND_SITE, kWp1Flag, FG_WEB_TWO, "a second site of the shape that names another address" },
        { WP1_NO_INIT, kWp1Flag, FG_WEB_WRITERS, "the address never set to -1" },
        { WP1_TWO_CLEARS, kWp1Flag, FG_WEB_WRITERS, "the address set to 0 in two places" },
        { WP1_CE | WP1_TWO_STORES, kWp1Flag, FG_WEB_WRITERS, "the address stored from eax in two places" },
    };
    int refused = 0;
    for (const Ctl1& k : ctl)
    {
        const Wp1Code c = Wp1Make(k.how);
        const uint32_t a = FgWebFindIn(c.b.data(), c.b.size(), k.flag, kWp1Lo, kWp1Hi, &f);
        if (a != 0 || f.why != k.why) Err("WP1 control: %s: the search answered %08X (%s) where it must refuse with \"%s\"", k.what, a, FgWebWhy(f.why), FgWebWhy(k.why));
        else refused++;
    }
    // (the flag by another address: the code names it nowhere)
    { const Wp1Code c = Wp1Make(0); if (FgWebFindIn(c.b.data(), c.b.size(), kWp1Flag + 1, kWp1Lo, kWp1Hi, &f) != 0 || f.why != FG_WEB_NO_SITE) Err("WP1 control: with another address for the menu flag the search found %08X", f.at); else refused++; }
    // THE VALUE'S CONTROL: "not zero" in the rule's place takes the game's init (-1) for a page
    { auto wrong = [](long id) { return id != 0; }; if (!wrong(-1) || FgWebUpRule(-1)) Err("WP1 control: the wrong reading of the value does not differ from the rule at -1"); }
    Ctl("%d searches refused each with its own reason (no flag, the shape nowhere in four ways, two addresses, the three writers' counts, the flag by another address); a reading of the value as \"not zero\" takes the game's init, -1, for a page, and the rule does not", refused);
    CaseEnd(name);
}
