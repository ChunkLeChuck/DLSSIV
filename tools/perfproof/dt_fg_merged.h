// dt_fg_merged.h: THE MERGED PATH IN THE HARNESS (2026-10-10, phase 3 of the revival, stage 7; docs/reports/fg-3/
// map-2026-10-06, critic.md's stage 7 and its section 5, "IN THE HARNESS, ARMED, ON THE REAL WIRE, THE WORKER HELD";
// docs/reports/fg-3/merge-2026-10-10/READY-FOR-7.md, whose section 1 lists the eight things a binding that armed on
// the wire compiled wrong until this stage: each is closed here by a named case).
//
// Included by dt_fg_test.cpp after dt_fg_shadow.h. Everything here runs FgMachine over ShArmB (dt_fg_shadow.h): the one
// binding that says kEngage and kWire together, on the same fake of DXVK, against the same fake helper, with the set's
// rules, the stream's gate and (new) the back buffer's save rule live. THE MACHINE IS THE ONE THAT WILL SHOW FRAMES:
// the helper's generated image is landed in the back buffer and presented, and the real frame is brought back from
// HOLD and presented at its own time.
//
// HOW A CASE DRIVES IT. The harness's clock (16.7 ms a frame). A frame is the game's: the injection, the facts, the
// fill, FgSwapPresent (ShFrame over the merged machine). STAMPS AND THE PLACER ARE STEPPED BY HAND (MgStamp, MgPlace)
// where a case's answer depends on them, so no case is judged by which thread ran first; where a case needs only a due
// it writes one with the machine's own DueWrite, standing in for a placer that decided it, and says so. Where ORDER is
// what is proved the worker is HELD (the harness's gate note) with the commands queued behind it.
//
// EVERY CASE RUNS IN BOTH ORDERS OF 'Q' (the product's, behind the injection's body, and the development key's, at the
// hand: dt_fg_test.cpp's RunCases), and the fake's eleven violation counts are held to 0 at the end of every one
// (CaseEnd): a control that breaks one on purpose puts the counts back.
#pragma once

// ------------------------------------------------------------------ a fresh merged machine, and its steps by hand
static LONG gMgLast[FG_TL_N];
// The four timelines of the wire, stamped now, on this thread, at the harness's clock. A fence at the top (a helper that
// is gone) is left alone, as the product's stamper leaves it.
template <class MM = MA> static void MgStamp()
{
    static const int tls[4] = { FG_TL_IN, FG_TL_PREP, FG_TL_OUT, FG_TL_END };
    for (int tl : tls) { if (B::TlValue(tl) >= 0x7FFFFFFFull) continue; while (MM::StampStep(tl, &gMgLast[tl], 0)) {} }
}
template <class MM = MA> static LONGLONG MgPlace() { MgStamp<MM>(); return MM::PlacerStep(B::Now()); }
// ShFresh over the merged machine (no thread of its own: a merged case steps the stamps and the placer by hand unless it
// starts threads of its own; StopThreads ends any a case before it left)
template <class MM = MA> static void MgFresh(int images = 3)
{
    ShFresh<MM>(images);
    StopThreads();
    for (int t = 0; t < FG_TL_N; t++) gMgLast[t] = 0;
}
static void MgEnd() { ShEnd(); }
static LONG MgP() { return B::S().r.lastP; }
// THE HARNESS STANDING IN FOR THE PLACER (dt_fg_test.cpp's DueI and DueN, then the placer's wake note): the open pair's
// generated frame is due and may be shown; its real frame is due.
static void MgDueShow() { DueI(MgP(), FG_DUE_SHOW); FireNote(); Drain(); }
static void MgDueN()    { DueN(MgP()); FireNote(); Drain(); }
static size_t MgShownN() { return Shown().size(); }
// What the back buffer holds now: DXVK's stream flushed first, so everything the game has drawn so far has landed.
static DWORD MgBb() { B::Flush(); return RingTag(RingCurrent()); }
// The merged path's own counts, as one line a case can print.
static const char* MgCounts()
{
    static char b[512];
    const FgArmedWorker& w = ShArmB::a.w;
    snprintf(b, sizeof(b), "shown %ld, fact %ld, fed-reset %ld, no source %ld, late %ld, no frame %ld, no answer %ld, late fire %ld, forced %ld, no buffer %ld, no copy %ld, dropped %ld;"
        " inline: unsent %ld, no hold %ld, no HUD-less note %ld, closing %ld; real frames dropped %ld",
        w.iWhy[FG_AW_SHOWN], w.iWhy[FG_AW_FACT], w.iWhy[FG_AW_RESET], w.iWhy[FG_AW_NOSOURCE], w.iWhy[FG_AW_LATE], w.iWhy[FG_AW_NOFRAME], w.iWhy[FG_AW_NOANSWER],
        w.iWhy[FG_AW_LATEFIRE], w.iWhy[FG_AW_FORCED], w.iWhy[FG_AW_NOBUFFER], w.iWhy[FG_AW_NOCOPY], w.iWhy[FG_AW_DROPPED],
        w.inl[FG_AI_UNSENT], w.inl[FG_AI_NOHOLD], w.inl[FG_AI_NOHUDLESS], w.inl[FG_AI_CLOSING],
        w.dropped[0] + w.dropped[1] + w.dropped[2] + w.dropped[3] + w.dropped[4]);
    return b;
}
static void MgClean(const char* tag)
{
    ShClean(tag);
    if (ShArmB::badSignal) Err("%s: the game signalled a timeline that is not its own %ld time(s) (the I marker's is the helper's fgPrep on the wire)", tag, ShArmB::badSignal);
    if (ShArmB::a.w.restoreLost) Err("%s: %ld restore(s) of a fire were not taken by the queue", tag, ShArmB::a.w.restoreLost);
}

// ---------------------------------------------------------------- M1, a pair of the merged path
// WHAT IT SHOWS (the build map's machine-2 to -5 and -7; the owner's ruling on X5; READY-FOR-7.md, section 1, item 4).
// A PLAIN PAIR, its two dues written by the harness: on the worker the frame reads 'Q', the hand's ONE submit, 'G', then
// two submits for each fire (the write, the restore); DXVK is presented to TWICE, the generated frame OF THIS PAIR and
// then the real frame, each the right picture; the helper's PRESENT, the game's HOLD and the helper's GEN hold the frame,
// the frame, and the frame under the generated mark; the back buffer after each fire is the back buffer before it; and
// the game signals fgIn alone (the rig's fire signals the I marker's timeline, which on the wire is the helper's fgPrep).
// A PAIR WITH NO SOURCE (the helper HELD inside the dispatch before it, so its hand may not write PRESENT): 'G' says
// NOSOURCE, PRESENT is left as the helper is reading it, AND THE REAL FRAME IS IN HOLD ALL THE SAME: the pair ARMS, shows
// nothing generated, and its real frame goes out from HOLD at its own fire, in its place.
// CONTROL: a binding whose copy into HOLD is not made: the real frame's fire shows what HOLD held before, another frame.
struct MgNoHoldCopyB : ShArmB { static void RecCopyToImg(ULONGLONG bb, int img) { if (img == FG_IMG_HOLD) return; ShArmB::RecCopyToImg(bb, img); } };
typedef FgMachine<MgNoHoldCopyB> MANH;
template <class MM> static void MgM1(const char* name)
{
    CaseBegin(name);
    MgFresh<MM>();
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MM>(); MgDueN();                                   // (the last pair of the warm-up brought to its real frame: nothing is pending)
    // A PLAIN PAIR
    const LONG pres0 = gRingSt.presents; const size_t sq0 = ShB::seq.size();
    const DWORD tagA = ShFrame<MM>(); const LONG nA = x.w.lastQN;
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    const DWORD bb0 = MgBb();
    MgDueShow();
    const DWORD bb1 = MgBb();
    const DWORD genHeld = RingTag(RingRig(FG_IMG_GEN)), presentHeld = RingTag(RingRig(FG_IMG_PRESENT)), holdHeld = RingTag(RingRig(FG_IMG_HOLD));
    MgDueN();
    const DWORD bb2 = MgBb();
    const std::string seqA(ShB::seq.begin() + sq0, ShB::seq.end());
    const LONG presA = gRingSt.presents - pres0;
    auto shown = Shown();
    const DWORD sI = shown.size() > shown0 ? shown[shown0].tag : 0, sN = shown.size() > shown0 + 1 ? shown[shown0 + 1].tag : 0;
    const size_t shownA = shown.size() - shown0;
    const LONG prepByHelper = (LONG)B::TlValue(FG_TL_PREP);
    // A PAIR WITH NO SOURCE: the helper held inside the dispatch of the pair before it
    _InterlockedExchange(&gH.holdDispatch, 1);
    const DWORD tagB = ShFrame<MM>(true, false);
    if (!WaitFor([] { return gH.inDispatch != 0; }, 3000)) Err("M1: the fake helper never reached the held dispatch");
    const LONG inl0 = a.w.inl[0] + a.w.inl[1] + a.w.inl[2] + a.w.inl[3], ns0 = a.w.iWhy[FG_AW_NOSOURCE], busy0 = gRingSt.inBusy;
    const size_t shown1 = MgShownN();
    const DWORD tagC = ShFrame<MM>(true, false); const LONG nC = x.w.lastQN;
    const bool armedC = s.w.openPhase == FG_PAIR_ARMED, stillHeld = gH.inDispatch != 0;
    const DWORD presentAtC = RingTag(RingRig(FG_IMG_PRESENT)), holdAtC = RingTag(RingRig(FG_IMG_HOLD));
    MgDueShow();                                              // (a due that says SHOW, planted: the pair may show nothing, and its fire does not ask)
    MgDueN();
    shown = Shown();
    std::string tail = TagList(shown, shown1);
    const bool orderBC = shown.size() == shown1 + 2 && shown[shown1].tag == tagB && shown[shown1 + 1].tag == tagC;
    const LONG inlC = a.w.inl[0] + a.w.inl[1] + a.w.inl[2] + a.w.inl[3] - inl0, nsC = a.w.iWhy[FG_AW_NOSOURCE] - ns0, inBusy = gRingSt.inBusy - busy0;
    ShRelease();
    if (!ShSettle()) Err("M1: the fake helper did not come back after its hold");
    ShFrame<MM>(); MgDueN();
    const auto log = RingPipeLog();
    printf("  a plain pair (%ld), armed %s: on the worker it reads %s; %ld DXVK present(s), showing %lX then %lX (the frame is %lX: its generated image %lX); PRESENT %lX, HOLD %lX, GEN %lX;"
           " the back buffer %lX before, %lX after the generated frame's fire, %lX after the real frame's; the game signalled a timeline not its own %ld time(s) (fgPrep reads %ld, the helper's)\n",
        nA, armedA ? "yes" : "NO", seqA.c_str(), presA, sI, sN, tagA, ShGenTag(tagA, false), presentHeld, holdHeld, genHeld, bb0, bb1, bb2, ShArmB::badSignal, prepByHelper);
    printf("  a pair with no source (%ld, the helper %s inside the dispatch before it): 'G' %s, armed %s, presented inline %ld; PRESENT left at %lX (the frame before, %lX), HOLD %lX (the frame, %lX);"
           " the screen then showed %s; not shown for want of a source %ld; copies into an input the helper was reading %ld\n",
        nC, stillHeld ? "still held" : "ALREADY RELEASED", ShKindName(ShKindOf(log, nC)), armedC ? "yes" : "NO", inlC, presentAtC, tagB, holdAtC, tagC, tail.c_str(), nsC, inBusy);
    printf("  the merged path's own counts: %s\n", MgCounts());
    if (!armedA || seqA != "QSGSSSS") Err("M1: a plain pair was %s and reads %s on the worker (QSGSSSS wanted: 'Q', the hand's one submit, 'G', the write and the restore of each fire)", armedA ? "armed" : "NOT armed", seqA.c_str());
    if (presA != 2 || shownA != 2 || sI != ShGenTag(tagA, false) || sN != tagA) Err("M1: a plain pair made %ld DXVK present(s) and the screen showed %zu picture(s), %lX then %lX (2, 2, the pair's generated image %lX then the frame %lX wanted)", presA, shownA, sI, sN, ShGenTag(tagA, false), tagA);
    if (presentHeld != tagA || holdHeld != tagA || genHeld != ShGenTag(tagA, false)) Err("M1: after the hand PRESENT holds %lX and HOLD %lX (the frame %lX wanted in both), GEN %lX", presentHeld, holdHeld, tagA, genHeld);
    if (bb1 != bb0 || bb2 != bb0) Err("M1: the back buffer read %lX before the fires, %lX after the generated frame's and %lX after the real frame's: a fire must leave it as it found it", bb0, bb1, bb2);
    if (ShKindOf(log, nA) != (int)FG_G_DISPATCH || ShArmB::badSignal) Err("M1: the plain pair's 'G' is %s, and the game signalled a timeline not its own %ld time(s)", ShKindName(ShKindOf(log, nA)), ShArmB::badSignal);
    if (!stillHeld || ShKindOf(log, nC) != (int)FG_G_NOSOURCE || !armedC || inlC != 0) Err("M1: the pair behind a held dispatch: the helper %s, 'G' %s, armed %d, presented inline %ld (held, NOSOURCE, armed and 0 wanted: its real frame keeps its place)", stillHeld ? "held" : "released", ShKindName(ShKindOf(log, nC)), (int)armedC, inlC);
    if (presentAtC != tagB || holdAtC != tagC) Err("M1: with no source PRESENT reads %lX (it must be left at %lX, which the helper is reading) and HOLD %lX (the frame %lX wanted)", presentAtC, tagB, holdAtC, tagC);
    if (!orderBC || nsC != 1 || inBusy != 0) Err("M1: behind the pair with no source the screen showed %s (%lX then %lX wanted, each once); not shown for want of a source %ld (1 wanted); copies into a busy input %ld", tail.c_str(), tagB, tagC, nsC, inBusy);
    MgClean("M1");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MANH>();
    ShWarm<MANH>();
    const DWORD tagK = ShFrame<MANH>();
    const size_t k0 = MgShownN();
    MgDueN();
    const auto sk = Shown();
    const DWORD shownK = sk.size() > k0 ? sk[k0].tag : 0;
    ViolRestore(stc);
    MgEnd();
    if (shownK == tagK) Err("M1 control: with the copy into HOLD left out the real frame's fire still showed the frame (%lX): the control shows nothing", shownK);
    Ctl("a binding that does not copy into HOLD: the real frame's fire showed %lX where the frame is %lX", shownK, tagK);
    CaseEnd(name);
}
static void CaseM1()  { MgM1<MA>("M1 a pair of the merged path: the wire, the hold, the generated frame of its own pair, the real frame (stage 7)"); }
static void CaseM1r() { MgM1<MAR>("M1r the same over the merged binding that is an instrument's (kRig true)"); }

// ------------------------------------------------------------------ the worker held, and a hand recorded by the harness
// THE WORKER HELD WITH COMMANDS QUEUED BEHIND IT (dt_fg_test.cpp's head: a case that drains before it tests cannot see an
// ordering bug). The gate note; then `rec` records on the render thread, which may block behind the hold (at threaded=2
// every recorded Present drains): so a helper thread looks, 30 ms after the worker reached the gate, and releases.
// False: the worker never reached the gate.
static void (*gMgLook)() = nullptr;
static void MgLookRelease() { if (gMgLook) gMgLook(); SetEvent(gGateEv); }
template <class F> static bool MgHeld(F rec, void (*look)() = nullptr)
{
    gMgLook = look;
    Later l = { [] { return gGateIn != 0; }, &MgLookRelease, 3000, 30, 0, false };
    if (!Gate()) { Err("the gate note was refused"); return false; }
    const HANDLE h = Spawn(LaterMain, &l);
    rec();
    Drain();
    Join(h, "a held case's look and release");
    if (!l.met) Err("the worker never reached the gate");
    return l.met;
}
// FG_HAND as FgSwapPresent records it for a binding on the wire (the build generation in the payload's hold), with the
// flags and the render facts a case wants: for a frame a case records note by note, to put a note of its own between.
static bool MgRecHand(DWORD flags = 0, DWORD withhold = 0)
{
    FgState& s = B::S();
    FgHandPayload h = {};
    h.k = dtPresRecN + 1; h.gen = dtArmCount; h.p = s.r.nextP; h.fEnd = B::frames - 1; h.tEntry = B::Now(); h.hold = ShB::x.sh.bgen;
    h.flags = flags; h.withhold = withhold;
    if (!DtNote(DT_NOTE_FG_HAND, &h, sizeof(h))) { Err("a hand recorded by the harness was refused"); return false; }
    s.r.lastP = h.p; s.r.lastGen = h.gen; s.r.lastEngaged = true; s.r.nextP++;
    return true;
}
// The start of a frame recorded note by note: the frame counter, its injection (sent), and its drawing.
static DWORD MgRecBegin(bool sent = true)
{
    frames++; B::frames = frames;
    ClockAdd(16.7);
    ShInject(sent, frames - 1);
    const DWORD t = NewTag(); Fill(t);
    return t;
}
// The record the worker published for pair p at its arm (false: it did not arm, or the ring moved on).
static bool MgPair(LONG p, FgArmedPair* out) { return MA::Read(ShArmB::a.pair[p & (FG_RING - 1)], p, out); }
static int MgTagCount(size_t from, DWORD tag) { int n = 0; const auto v = Shown(); for (size_t i = from; i < v.size(); i++) n += v[i].tag == tag; return n; }

// ---------------------------------------------------------------- M2, a frame that was not sent never arms
// WHAT IT SHOWS (the owner's ruling; READY-FOR-7.md, section 1, item 2). A frame whose injection was not sent: nothing
// goes to the pipe, nothing is copied (PRESENT and HOLD keep the frame before), and THE PAIR DOES NOT ARM: its Present
// goes to DXVK at once, inline, counted "unsent". The pair before, still armed, is brought to its real frame first, so
// the screen reads the frame before, then the unsent frame, each once and in order. THE WORKER IS HELD with the whole
// frame queued behind it: nothing reaches the screen while it is held.
// CONTROL: what the two half paths, compiled together, did before this stage: the pair made to arm with nothing in the
// store for it (the worker's two facts forced by a note of the harness's between its hand and its Present). Its real
// frame's fire then serves from HOLD the frame BEFORE, a second time, and the unsent frame is never shown.
static volatile LONG gM2Shown = -1;
static void M2Look() { gM2Shown = (LONG)RingPresentedLog().size(); }
static void M2Force(DWORD, DWORD, DWORD) { B::S().w.openSent = 1; ShArmB::a.w.held = 1; }
static void CaseM2()
{
    CaseBegin("M2 a frame that was not sent never arms (stage 7)");
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a; FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>();
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN(), pipe0 = RingPipeLog().size();
    const LONG uns0 = a.w.inl[FG_AI_UNSENT], pubIn0 = s.c.pubInline[FG_IN_NOSOURCE];
    DWORD tagU = 0; gM2Shown = -1;
    MgHeld([&] { ClockAdd(16.7); ShInject(false); tagU = Frame<MA>(false); }, &M2Look);
    const bool armedU = s.w.openPhase == FG_PAIR_ARMED;
    const size_t wrote = RingPipeLog().size() - pipe0;
    const DWORD presentHeld = RingTag(RingRig(FG_IMG_PRESENT)), holdHeld = RingTag(RingRig(FG_IMG_HOLD));
    const auto shown = Shown();
    const std::string tail = TagList(shown, shown0);
    const bool order = shown.size() == shown0 + 2 && shown[shown0].tag == tagA && shown[shown0 + 1].tag == tagU;
    const LONG unsent = a.w.inl[FG_AI_UNSENT] - uns0, pubIn = s.c.pubInline[FG_IN_NOSOURCE] - pubIn0;
    ShFrame<MA>(); MgDueN();                                  // (a sent frame behind it: the wire goes on)
    printf("  the frame before (%lX) armed %s; an unsent frame (%lX) recorded behind a held worker: %ld picture(s) on the screen while it was held; released: the screen showed %s; the unsent pair armed %s, presented inline as unsent %ld (published inline %ld); it wrote %zu message(s); PRESENT %lX and HOLD %lX\n",
        tagA, armedA ? "yes" : "NO", tagU, (long)(gM2Shown < 0 ? -1 : gM2Shown - (LONG)shown0), tail.c_str(), armedU ? "YES" : "no", unsent, pubIn, wrote, presentHeld, holdHeld);
    if (!armedA || gM2Shown != (LONG)shown0) Err("M2: the frame before was %s, and %ld picture(s) reached the screen while the worker was held (0 wanted)", armedA ? "armed" : "NOT armed", (long)(gM2Shown - (LONG)shown0));
    if (!order) Err("M2: the screen showed %s (the frame before %lX, then the unsent frame %lX, each once, wanted)", tail.c_str(), tagA, tagU);
    if (armedU || unsent != 1 || pubIn != 1) Err("M2: the unsent pair armed %d, was presented inline as unsent %ld time(s) and published inline %ld time(s) (no, 1 and 1 wanted)", (int)armedU, unsent, pubIn);
    if (wrote != 0 || presentHeld != tagA || holdHeld != tagA) Err("M2: an unsent frame wrote %zu message(s) and left PRESENT at %lX and HOLD at %lX (nothing, and the frame before %lX in both, wanted)", wrote, presentHeld, holdHeld, tagA);
    MgClean("M2");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagB = ShFrame<MA>(); MgDueN();
    const size_t c0 = MgShownN();
    frames++; B::frames = frames; ClockAdd(16.7);
    const DWORD tagV = NewTag(); Fill(tagV);
    MgRecHand(); RecordHudless(); Run(M2Force); RecordPresent(); Drain();
    const bool armedV = s.w.openPhase == FG_PAIR_ARMED;
    MgDueN();
    const int againB = MgTagCount(c0, tagB), onceV = MgTagCount(c0, tagV);
    ViolRestore(stc);
    MgEnd();
    if (!armedV || againB != 1 || onceV != 0) Err("M2 control: an unsent pair made to arm (armed %d) showed the frame before %d more time(s) and itself %d time(s) (1 and 0 wanted): the control shows nothing", (int)armedV, againB, onceV);
    Ctl("an unsent pair made to arm: its fire served from HOLD the frame before (%lX) a second time, and the unsent frame %lX was shown %d time(s)", tagB, tagV, onceV);
    CaseEnd("M2 a frame that was not sent never arms (stage 7)");
}

// ---------------------------------------------------------------- M3, the hand finds the pair armed
// WHAT IT SHOWS (the build map's machine-3 as check-machine.md's R3 corrected its proof; READY-FOR-7.md, section 1, item
// 1). A pair is still ARMED when the next frame's hand comes: rule C3 (the injection forces nothing), no due raised, and
// the next frame recorded WITHOUT the drain note that normally heads its present. THE HAND BRINGS THAT PAIR TO ITS REAL
// FRAME FIRST, "hand open", and only then copies its own frame into HOLD: the screen reads the pair before's frame, then
// this one's, each under its own name. The worker is HELD with the injection, the drawing, the hand, the HUD-less note
// and the Present queued behind it.
// AND UNDER RULE C1, THE DEFAULT (no early key): with no wait due, the next frame's INJECTION forces the pending real frame,
// counted early, before that frame's work and long before its hand, which then finds nothing armed.
// CONTROL: the copy into HOLD made BEFORE the hand (a note of the harness's, queued ahead of the hand: the order the
// first lines of the hand exist to forbid): the pair before's fire shows THIS frame under its name, and the frame
// before is never shown.
static void M3CopyHold(DWORD, DWORD, DWORD)
{
    if (!ShArmB::CbTake(1)) return;
    MA::Lock(); ShArmB::Flush();
    const ULONGLONG bb = ShArmB::BBImage();
    ShArmB::QLock(); ShArmB::RecBegin(); ShArmB::RecCopyToImg(bb, FG_IMG_HOLD); ShArmB::RecSubmit(); ShArmB::QUnlock();
    MA::Unlock();
}
struct M3Leg { DWORD tagA, tagB; bool armedA, order; LONG handOpen, late, early; std::string tail; int a, b; };
static M3Leg M3Run(bool copyFirst, bool c3 = true)
{
    M3Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S();
    if (c3) gC1Off = 1;                                        // rule C3: the next injection forces nothing
    ShWarm<MA>(); MgDueN();
    g.tagA = ShFrame<MA>();
    g.armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN(); const LONG ho0 = s.c.handOpen, late0 = s.c.nLate, early0 = s.c.nEarly;
    MgHeld([&] { g.tagB = MgRecBegin(); if (copyFirst) Run(M3CopyHold); MgRecHand(); RecordHudless(); RecordPresent(); });   // (the hand's EARLYC3 flag, which the merged path never read, went with SLOPE in round 9's S9)
    if (!ShSettle()) Err("M3: the fake helper did not read the frame's messages");
    MgDueN();
    const auto shown = Shown();
    g.tail = TagList(shown, shown0);
    g.order = shown.size() == shown0 + 2 && shown[shown0].tag == g.tagA && shown[shown0 + 1].tag == g.tagB;
    g.a = MgTagCount(shown0, g.tagA); g.b = MgTagCount(shown0, g.tagB);
    g.handOpen = s.c.handOpen - ho0; g.late = s.c.nLate - late0; g.early = s.c.nEarly - early0;
    return g;
}
static void CaseM3()
{
    CaseBegin("M3 the hand finds the pair armed: its real frame first, then this frame's copy (stage 7)");
    const M3Leg g = M3Run(false);
    printf("  pair %lX still armed (%s) when the next hand came, the worker held with the frame queued behind it: found armed %ld time(s), its real frame fired late %ld; the screen showed %s (the next frame is %lX)\n",
        g.tagA, g.armedA ? "yes" : "NO", g.handOpen, g.late, g.tail.c_str(), g.tagB);
    if (!g.armedA || g.handOpen != 1 || g.late != 1) Err("M3: the pair before was %s, the hand found it armed %ld time(s) and its real frame was counted late %ld time(s) (armed, 1 and 1 wanted)", g.armedA ? "armed" : "NOT armed", g.handOpen, g.late);
    if (!g.order) Err("M3: the screen showed %s (%lX then %lX, each once, wanted)", g.tail.c_str(), g.tagA, g.tagB);
    MgClean("M3");
    MgEnd();
    const M3Leg e = M3Run(false, false);
    printf("  under rule C1, the same frame queued behind a held worker: the injection forced the pending real frame %ld time(s) (counted early), the hand found a pair armed %ld time(s); the screen showed %s\n", e.early, e.handOpen, e.tail.c_str());
    if (!e.armedA || e.early != 1 || e.handOpen != 0 || e.late != 0 || !e.order) Err("M3 (rule C1): the pair before was %s; the injection forced its real frame %ld time(s), the hand found it armed %ld time(s), counted late %ld; the screen showed %s (armed, 1, 0, 0, and %lX then %lX wanted)", e.armedA ? "armed" : "NOT armed", e.early, e.handOpen, e.late, e.tail.c_str(), e.tagA, e.tagB);
    MgClean("M3 (rule C1)");
    MgEnd();
    const RingStats stc = gRingSt;
    const M3Leg c = M3Run(true);
    ViolRestore(stc);
    MgEnd();
    if (c.a != 0 || c.b != 2) Err("M3 control: with the copy into HOLD made before the hand the screen showed %s (the frame before %lX never, this frame %lX twice, wanted): the control shows nothing", c.tail.c_str(), c.tagA, c.tagB);
    Ctl("the copy into HOLD made before the hand: the screen showed %s: this frame under the pair before's name, and the frame before (%lX) never", c.tail.c_str(), c.tagA);
    CaseEnd("M3 the hand finds the pair armed: its real frame first, then this frame's copy (stage 7)");
}

// ---------------------------------------------------------------- M4, fed, never shown
// WHAT IT SHOWS (the owner's ruling on the build map's X4; machine.md's machine-8; the desk's reading of a reset dispatch,
// docs/reports/fg-3/desk-2026-10-06/REPORT.md, item 1). EVERY pair's generated frame is asked for (a due that says SHOW,
// written by the harness). It is shown for a plain pair, AND NEVER FOR A PAIR WHOSE DISPATCH RESETS, whoever's account
// the reset is on:
//   a camera cut            written DISPATCH with its reason (the shadow shape writes WITHHELD): fed, not shown, AND THE
//                           PAIR BEHIND IT IS SHOWN: a cut costs ONE pair
//   the HDR view            WITHHELD (its inputs must not enter the history); the pair behind it resets on the helper's own
//                           account, which the wire does not say: the worker's mirror knows, and it is not shown
//   a hitch                 DISPATCH with no reason: fed, no reset, not shown (a fact of time); the pair behind it is shown
//   a dispatch with no frame   nothing to show; the pair behind it resets on the helper's own account: not shown
//   a pipelined pair        (the injection's note says its guides are a frame ahead of the presented picture): DISPATCH,
//                           fed, with no reset, and NOT SHOWN; the pair behind it is shown
//   (a pair under lever 4's key was a leg here until stage 7d: the key is REFUSED in a binding that arms, and T5 holds that)
// THE WORKER'S MIRROR IS THE HELPER'S OWN RECORD: for every dispatch that gave a frame, "it reset" as the fake helper
// kept it (ShDispLog, by the real helper's lines) equals "fed, never shown for a reset" as the machine decided it; and
// the fake counts no landing of a reset dispatch's picture.
// CONTROL: the mirror overruled for the cut pair (a note of the harness's sets "may be shown" between its HUD-less note
// and its Present): the fire lands the reset dispatch's picture, the fake counts it, and the screen shows a picture
// marked as a reset dispatch's.
static void M4ForceShow(DWORD, DWORD, DWORD) { ShArmB::a.w.show = 1; ShArmB::a.w.why = FG_AW_SHOWN; }
static void CaseM4()
{
    CaseBegin("M4 fed, never shown: a dispatch that resets is not shown, and a cut costs one pair (stage 7)");
    MgFresh<MA>();
    FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    struct Fr { const char* what; DWORD tag; LONG n, p; bool wantShown; LONG why; };
    std::vector<Fr> fr;
    // one frame of the script: the frame (and what the worker decided for its pair at the arm), its generated frame asked
    // for, its real frame
    auto frame = [&](const char* what, bool wantShown)
    {
        const DWORD t = ShFrame<MA>();
        FgArmedPair rec = {}; const bool armed = MgPair(MgP(), &rec);
        fr.push_back({ what, t, x.w.lastQN, MgP(), wantShown, armed ? rec.why : -1 });
        MgDueShow(); MgDueN();
    };
    ShWarm<MA>(); MgDueN();
    const size_t shown0 = MgShownN(); const size_t disp0 = ShDispLog().size();
    frame("a plain pair", true);
    B::camJumps++; frame("a camera cut", false);
    frame("the pair behind the cut", true);
    B::hdrView = true; frame("the HDR view", false); B::hdrView = false;
    frame("the pair behind the view", false);
    frame("a plain pair", true);
    ClockAdd(40.0); frame("a hitch", false);
    frame("the pair behind the hitch", true);
    _InterlockedExchange(&gH.noFrame, 1); frame("a dispatch with no frame", false); _InterlockedExchange(&gH.noFrame, 0);
    frame("the pair behind it", false);
    frame("a plain pair", true);
    _InterlockedExchange(&ShB::pipelined, 1); frame("a pipelined pair", false); _InterlockedExchange(&ShB::pipelined, 0);
    frame("the pair behind the pipelined one", true);
    // the screen, frame by frame: [its generated image,] the frame
    const auto shown = Shown();
    size_t at = shown0; int bad = 0; std::string line;
    for (const Fr& f : fr)
    {
        const bool gen = at < shown.size() && shown[at].tag == ShGenTag(f.tag, false);
        if (gen) at++;
        const bool real = at < shown.size() && shown[at].tag == f.tag;
        if (real) at++;
        if (gen != f.wantShown || !real) { bad++; if (line.empty()) line = Fmt("%s (%lX): its generated frame %s, its real frame %s", f.what, f.tag, gen ? "shown" : "not shown", real ? "shown" : "NOT shown"); }
    }
    if (at != shown.size()) bad++;
    // the mirror against the helper's own record
    const auto log = RingPipeLog(); const auto disp = ShDispLog();
    int mirrorBad = 0, resets = 0, own = 0; std::string recs;
    for (size_t i = disp0; i < disp.size(); i++)
    {
        const ShDisp& d = disp[i];
        recs += d.reset ? (d.own ? 'O' : 'T') : 'N';
        resets += d.reset; own += d.own;
        bool found = false;
        for (const Fr& f : fr) if (f.n == d.n) { found = true; if ((f.why == FG_AW_RESET) != d.reset) mirrorBad++; }
        if (!found) mirrorBad++;
    }
    const LONG nCut = fr[1].n, nView = fr[3].n, nHitch = fr[6].n;
    printf("  thirteen frames, each with its generated frame asked for: the screen showed %s; the helper's dispatches that gave a frame (O reset on its own account, T on the game's reason, N none): %s;"
           " the cut is written %s with reasons 0x%lX, the view %s, the hitch %s with reasons 0x%lX; the worker's mirror differs from the helper's record on %d dispatch(es); fed and never shown for a reset %ld; not shown as pipelined %ld\n",
        bad ? "NOT what each frame is owed" : "every real frame once and in order, and a generated frame for exactly the pairs that may have one", recs.c_str(),
        ShKindName(ShKindOf(log, nCut)), ShGWhy(log, nCut), ShKindName(ShKindOf(log, nView)), ShKindName(ShKindOf(log, nHitch)), ShGWhy(log, nHitch), mirrorBad, a.w.iWhy[FG_AW_RESET], a.w.iWhy[FG_AW_PIPELINED]);
    printf("  the merged path's own counts: %s\n", MgCounts());
    if (bad) Err("M4: the screen is not what the script is owed (%d frame(s) wrong; the first: %s; %zu picture(s) over)", bad, line.c_str(), shown.size() - at);
    if (recs != "NTNONNNONNN") Err("M4: the helper's record of its dispatches reads %s (NTNONNNONNN wanted: the cut told, the pair behind the view and the pair behind the dispatch with no frame on its own account)", recs.c_str());
    if (a.w.iWhy[FG_AW_PIPELINED] != 1) Err("M4: %ld pair(s) were not shown as pipelined, 1 wanted", a.w.iWhy[FG_AW_PIPELINED]);
    if (ShKindOf(log, nCut) != (int)FG_G_DISPATCH || !(ShGWhy(log, nCut) & FG_Q_CUT) || ShKindOf(log, nView) != (int)FG_G_WITHHELD || ShKindOf(log, nHitch) != (int)FG_G_DISPATCH || ShGWhy(log, nHitch) != 0)
        Err("M4: the kinds: a cut %s with reasons 0x%lX (DISPATCH with the cut wanted), the view %s (WITHHELD wanted), a hitch %s with reasons 0x%lX (DISPATCH with none wanted)", ShKindName(ShKindOf(log, nCut)), ShGWhy(log, nCut), ShKindName(ShKindOf(log, nView)), ShKindName(ShKindOf(log, nHitch)), ShGWhy(log, nHitch));
    if (mirrorBad) Err("M4: the worker's mirror of \"this dispatch resets\" differs from the helper's own record on %d dispatch(es)", mirrorBad);
    MgClean("M4");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueN();
    MgRecBegin(); MgRecHand(0, FG_W_CUT); RecordHudless(); Run(M4ForceShow); RecordPresent(); Drain();
    if (!ShSettle()) Err("M4 control: the fake helper did not read the cut frame's messages");
    const LONG r0 = gRingSt.genReset;
    MgDueShow();
    const LONG counted = gRingSt.genReset - r0; const DWORD shownCut = ShLastShown();
    MgDueN();
    ViolRestore(stc);
    MgEnd();
    if (counted != 1 || !ShIsGenReset(shownCut)) Err("M4 control: with the mirror overruled the fake counted %ld landing(s) of a reset dispatch's picture and the screen showed %lX (1, and a picture marked as a reset dispatch's, wanted): the control shows nothing", counted, shownCut);
    Ctl("the mirror overruled for a cut pair: the fire landed the reset dispatch's picture (%lX), counted %ld", shownCut, counted);
    CaseEnd("M4 fed, never shown: a dispatch that resets is not shown, and a cut costs one pair (stage 7)");
}

// ---------------------------------------------------------------- M5, the pending real frame goes out ahead of the guard's wait
// WHAT IT SHOWS (the build map's machine-9 as check-machine.md's R2 corrected it; critic X11; READY-FOR-7.md, section 1,
// item 5; the critic's section 5: "the real frame is never queued behind the helper's wait, in all three stream modes").
// The fake helper is HELD in the prepare of pair n, and pair n is ARMED with its real frame not yet due (rule C3: the
// early rule forces nothing). The worker is HELD and the next frame's injection is queued behind it: its guard is about
// to queue a wait on DXVK's queue behind that prepare, and everything that reaches the queue after it stands behind the
// helper. THE REAL FRAME OF PAIR n IS ON THE SCREEN WHILE THE HELPER IS STILL HELD, in all three modes of the fake's
// command stream (how far DXVK's own thread got inside the Present): forced first, counted, the stream flushed, counted,
// and only then the wait.
// CONTROL: a binding whose flush between that present and the wait is not made (the order the build map proposed before
// its checker read DXVK: "fire it first, then queue the wait"): where DXVK's thread had not yet handed the blit over
// (LAZY, PRE) the wait's batch lands AHEAD of it, and the real frame is not on the screen until the helper answers.
// AND A PRESENT THE MACHINE PASSED THROUGH IS A REAL FRAME TOO (M5Pass). A frame that is not engaged has no hand and no
// fire: its Present goes to DXVK as it is, and nothing of the merged path's own flushes behind it. The frame before
// was engaged and its prepare is held; this frame's injection queued the wait behind that prepare (so this frame's own
// present stands behind it, as every frame's own work does). THEN THE NEXT INJECTION QUEUES ANOTHER WAIT, and the passed
// present's blit must be on the queue AHEAD of that one: the stream is flushed first, counted. Where 'Q' is written at
// the injection the second wait is for a LATER prepare, the helper is let go to it and held there, and the passed
// frame is ON THE SCREEN WHILE THE HELPER IS HELD, in the three stream modes; its control is the binding with no flush
// (LAZY and PRE: not on the screen until the helper answers). Under the development key no 'Q' was written for a frame
// with no hand, the second wait is for the same prepare, and the count alone says the flush was made.
// AND SO IS A PRESENT THE MERGED PATH MADE INLINE: the same legs with that frame ENGAGED and its hand finding no store for
// the real frame (no HOLD image: the span is not taken, and its flush has already run), so its Present goes to DXVK
// inline. It has a 'Q' in both orders, so the second wait is for a later prepare in both, and the frame is on the screen
// while the helper is held there, in the three stream modes, with the same control.
struct MgNoGuardFlushB : ShArmB { static void Flush() { if (GetCurrentThreadId() == dtWorkerTid && FgHarnessBinding::S().w.lockDepth == 0) return; ShArmB::Flush(); } };
typedef FgMachine<MgNoGuardFlushB> MANF;
struct M5Leg { DWORD tagA; bool held, armed; int whileHeld, after; LONG forced, flushes, guards, waitFor, nA; };
template <class MM> static M5Leg M5Run(int mode)
{
    M5Leg g = {};
    MgFresh<MM>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    gC1Off = 1;                                               // rule C3: the early rule forces nothing at the injection: only the guard's own order can
    ShWarm<MM>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    g.tagA = ShFrame<MM>(true, false); g.nA = x.w.lastQN;
    g.held = WaitFor([] { return gH.inPrep != 0; }, 3000);
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN();
    const LONG f0 = a.w.guardForced, fl0 = a.w.guardFlushes, g0 = x.w.prepGuards;
    gRing.presentCs = mode;
    MgHeld([&] { ClockAdd(16.7); ShInject(true); });          // THE WORKER HELD, the next frame's injection queued behind it
    g.whileHeld = MgTagCount(shown0, g.tagA);
    g.held = g.held && gH.inPrep != 0;
    g.forced = a.w.guardForced - f0; g.flushes = a.w.guardFlushes - fl0; g.guards = x.w.prepGuards - g0; g.waitFor = ShB::waitFor;
    gRing.presentCs = RING_CS_LAZY;
    ShRelease();
    if (!WaitFor([] { return ShB::waitFor == 0; }, 3000)) Err("M5: the queue's wait did not end after the helper's release");
    Frame<MM>(); Drain();                                     // (the rest of that frame)
    if (!ShSettle()) Err("M5: the fake helper did not come back after its hold");
    MgDueN();
    g.after = MgTagCount(shown0, g.tagA);
    return g;
}
struct M5Pass { bool held1, held2; LONG flushD, guardsD, flushE, guardsE, inl; int dWhile, dAfter, cAfter; DWORD tagC, tagD; };
template <class MM> static M5Pass M5PassRun(int mode, bool inl)
{
    M5Pass g = {};
    MgFresh<MM>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    ShWarm<MM>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    g.tagC = ShFrame<MM>(true, false);
    static LONG nC; nC = x.w.lastQN;
    g.held1 = WaitFor([] { return gH.inPrep != 0; }, 3000);
    MgDueN();                                                 // pair C's real frame: fired, on the screen
    const size_t shown0 = MgShownN();
    gRing.presentCs = mode;
    LONG fl = a.w.guardFlushes, gd = x.w.prepGuards;
    ClockAdd(16.7); ShInject(true); Drain();                  // the next frame's injection: C's prepare is held: the flush, then the wait
    g.flushD = a.w.guardFlushes - fl; g.guardsD = x.w.prepGuards - gd;
    const LONG nh0 = a.w.inl[FG_AI_NOHOLD];
    if (inl) { ShArmB::noHold = 1; g.tagD = Frame<MM>(); Drain(); ShArmB::noHold = 0; }   // engaged, no store for its real frame: presented inline
    else { s.wt.want = FG_WANT_PASS; g.tagD = Frame<MM>(); Drain(); s.wt.want = FG_WANT_ON; }   // not engaged: its Present is passed through
    g.inl = a.w.inl[FG_AI_NOHOLD] - nh0;
    if (inl || !gShAtHand)
    {   // the helper goes on to the prepare of that frame's own 'Q', and is held there
        _InterlockedExchange(&gH.holdPrep, 1);
        ShRelease();
        g.held2 = WaitFor([] { return B::TlValue(FG_TL_PREP) >= (ULONGLONG)nC && gH.inPrep != 0 && ShB::waitFor == 0; }, 3000);
    }
    else g.held2 = gH.inPrep != 0;
    fl = a.w.guardFlushes; gd = x.w.prepGuards;
    ClockAdd(16.7); ShInject(true); Drain();                  // THE NEXT INJECTION: another wait is about to be queued
    g.flushE = a.w.guardFlushes - fl; g.guardsE = x.w.prepGuards - gd;
    g.dWhile = MgTagCount(shown0, g.tagD);
    g.held2 = g.held2 && gH.inPrep != 0;
    gRing.presentCs = RING_CS_LAZY;
    ShRelease();
    if (!WaitFor([] { return ShB::waitFor == 0; }, 3000)) Err("M5: the queue's wait did not end after the helper's release");
    Frame<MM>(); Drain();                                     // (the rest of that frame)
    if (!ShSettle()) Err("M5: the fake helper did not come back after its hold");
    MgDueN();
    g.dAfter = MgTagCount(shown0, g.tagD); g.cAfter = MgTagCount(shown0, g.tagC);
    return g;
}
static void CaseM5()
{
    CaseBegin("M5 the pending real frame goes out ahead of the guard's wait, in the three stream modes (stage 7)");
    static const int kModes[3] = { RING_CS_LAZY, RING_CS_PRE, RING_CS_ALL };
    static const char* const kName[3] = { "LAZY", "PRE", "ALL" };
    M5Leg g[3], c[3];
    for (int m = 0; m < 3; m++)
    {
        g[m] = M5Run<MA>(kModes[m]);
        printf("  %-4s  the helper %s in the prepare of pair %ld, the pair armed %s: the next injection queued a wait for %ld (%ld queued), forced the pending real frame %ld time(s) and flushed %ld time(s): on the screen while the helper was held %d, after its release %d\n",
            kName[m], g[m].held ? "held" : "NOT HELD", g[m].nA, g[m].armed ? "yes" : "NO", g[m].waitFor, g[m].guards, g[m].forced, g[m].flushes, g[m].whileHeld, g[m].after);
        if (!g[m].held || !g[m].armed || g[m].guards != 1 || g[m].waitFor != g[m].nA) Err("M5 %s: the leg did not reach its hold (helper held %d, the pair armed %d, %ld wait(s) queued for %ld where pair %ld was held)", kName[m], (int)g[m].held, (int)g[m].armed, g[m].guards, g[m].waitFor, g[m].nA);
        if (g[m].forced != 1 || g[m].flushes != 1) Err("M5 %s: the pending real frame was forced %ld time(s) ahead of the wait and the stream flushed %ld time(s) (1 and 1 wanted)", kName[m], g[m].forced, g[m].flushes);
        if (g[m].whileHeld != 1 || g[m].after != 1) Err("M5 %s: the pending real frame was on the screen %d time(s) while the helper was held and %d time(s) after (1 and 1 wanted): it stood behind the helper's prepare", kName[m], g[m].whileHeld, g[m].after);
        MgClean("M5");
        MgEnd();
    }
    // A PRESENT THE MACHINE PASSED THROUGH (0), AND ONE THE MERGED PATH MADE INLINE (1)
    static const char* const kPass[2] = { "a passed present", "an inline present" };
    static const char* const kWhat[2] = { "a frame that is not engaged", "an engaged frame with no store for its real frame" };
    for (int k = 0; k < 2; k++)
    {
        const bool second = k == 1 || !gShAtHand;                 // (the second wait is for a LATER prepare: the frame has a 'Q')
        for (int m = 0; m < 3; m++)
        {
            const M5Pass p = M5PassRun<MA>(kModes[m], k == 1);
            printf("  %-4s  %s, behind a held prepare: its injection flushed %ld time(s) and queued %ld wait(s); presented inline for want of a store %ld; the next injection flushed %ld time(s) and queued %ld wait(s): that frame on the screen while the helper was held %d, after its release %d\n",
                kName[m], kWhat[k], p.flushD, p.guardsD, p.inl, p.flushE, p.guardsE, p.dWhile, p.dAfter);
            if (!p.held1 || !p.held2 || p.flushD != 1 || p.guardsD != 1 || p.inl != k) Err("M5 %s (%s): the leg did not reach its hold (helper held %d and %d; the first injection flushed %ld time(s) and queued %ld wait(s), 1 and 1 wanted; presented inline for want of a store %ld, %d wanted)", kName[m], kPass[k], (int)p.held1, (int)p.held2, p.flushD, p.guardsD, p.inl, k);
            if (p.flushE != 1 || p.guardsE != 1) Err("M5 %s (%s): the next injection flushed the stream %ld time(s) ahead of %ld wait(s) (1 and 1 wanted): that present is not marked for the next wait", kName[m], kPass[k], p.flushE, p.guardsE);
            if (p.dAfter != 1 || p.cAfter != 0) Err("M5 %s (%s): after the helper's release that frame was on the screen %d time(s) and the frame before %d more time(s) (1 and 0 wanted)", kName[m], kPass[k], p.dAfter, p.cAfter);
            if (second && p.dWhile != 1) Err("M5 %s (%s): that frame was on the screen %d time(s) while the helper was held in the next prepare (1 wanted): it stood behind a wait queued after it was presented", kName[m], kPass[k], p.dWhile);
            MgClean(kPass[k]);
            MgEnd();
        }
    }
    const RingStats stc = gRingSt;
    for (int m = 0; m < 3; m++) { c[m] = M5Run<MANF>(kModes[m]); MgEnd(); }
    M5Pass pc[2][3];
    for (int k = 0; k < 2; k++) for (int m = 0; m < 3; m++) { pc[k][m] = M5PassRun<MANF>(kModes[m], k == 1); MgEnd(); }
    ViolRestore(stc);
    for (int k = 0; k < 2; k++)
    {
        if (k == 0 && gShAtHand) continue;                        // (no second prepare to stand behind: the count is that leg's proof)
        const M5Pass* q = pc[k];
        if (q[0].dWhile != 0 || q[1].dWhile != 0 || q[2].dWhile != 1 || q[0].dAfter != 1 || q[1].dAfter != 1 || q[2].dAfter != 1)
            Err("M5 control (%s): without the flush that frame was on the screen while the helper was held %d, %d and %d time(s) in LAZY, PRE and ALL (0, 0 and 1 wanted) and %d, %d, %d after: the control shows nothing", kPass[k], q[0].dWhile, q[1].dWhile, q[2].dWhile, q[0].dAfter, q[1].dAfter, q[2].dAfter);
        Ctl("%s with the flush left out: behind the next prepare in LAZY and PRE (on the screen while held %d and %d), ahead of it in ALL (%d)", kPass[k], q[0].dWhile, q[1].dWhile, q[2].dWhile);
    }
    printf("  control, the flush between the present and the wait not made: on the screen while the helper was held: LAZY %d, PRE %d, ALL %d; after its release %d %d %d\n", c[0].whileHeld, c[1].whileHeld, c[2].whileHeld, c[0].after, c[1].after, c[2].after);
    if (c[0].whileHeld != 0 || c[1].whileHeld != 0 || c[2].whileHeld != 1 || c[0].after != 1 || c[1].after != 1 || c[2].after != 1)
        Err("M5 control: without the flush the real frame was on the screen while the helper was held %d, %d and %d time(s) in LAZY, PRE and ALL (0, 0 and 1 wanted) and %d, %d, %d after: the control shows nothing", c[0].whileHeld, c[1].whileHeld, c[2].whileHeld, c[0].after, c[1].after, c[2].after);
    Ctl("the flush between the forced present and the wait left out: the real frame was behind the helper's prepare in LAZY and PRE (on the screen while held %d and %d), and only ahead of it where DXVK's thread had already handed the blit over (ALL: %d)", c[0].whileHeld, c[1].whileHeld, c[2].whileHeld);
    CaseEnd("M5 the pending real frame goes out ahead of the guard's wait, in the three stream modes (stage 7)");
}

// ---------------------------------------------------------------- M6, what has no hand is closed in its own frame
// WHAT IT SHOWS (READY-FOR-7.md, section 1, item 6, and section 2, point 2: the early 'Q' in a binding that arms). The
// rig's fire note returns unless a pair is armed, and a binding that arms installs no close note of the shadow shape's.
// The merged path has its own, recorded once at the first present that follows an engaged one and has no hand:
//   A 'Q' WITH NO HAND. The want falls between a frame's injection and its Present: the injection wrote 'Q'(n) behind its
//   body, and no hand comes. The close note writes 'G'(n) CLOSED IN THAT SAME FRAME, counted "closed with no hand", and
//   the pair before, still armed at that injection, was brought to its real frame first. (Under the development key no
//   'Q' is written at an injection: there is none to close, and the case says so.)
//   A PAIR STILL OPEN WITH NO NEXT HAND. A pair whose Present never matched (its hand named another ordinal) stays OPEN
//   with its 'Q' and its 'G' written: the next present's notes close it, published CLOSED, and nothing is open after.
// CONTROL: the present that has no hand is recorded PAST the machine (dtScPresent, with no FgSwapPresent: no note of the
// machine's is recorded): the early 'Q' is still open when that frame is over, and the open pair is still open.
struct M6Leg { LONG earlyN, earlyAfter, noHand, pairOpenAfter, closedPub; bool closedInFrame; int shownA; DWORD tagA; std::string bad; };
static M6Leg M6Run(bool past)
{
    M6Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x;
    ShWarm<MA>(); MgDueN();
    g.tagA = ShFrame<MA>();
    const size_t shown0 = MgShownN();
    // THE WANT FALLS BETWEEN THE INJECTION AND ITS PRESENT
    ClockAdd(16.7); ShInject(true); Drain();
    g.earlyN = x.e.qN; const LONG nh0 = x.e.noHand; const size_t pipe1 = RingPipeLog().size();
    s.wt.want = FG_WANT_PASS;
    if (past) { Facts(); Fill(NewTag()); RecordPresent(); Drain(); } else { Frame<MA>(); Drain(); }
    if (!ShSettle()) Err("M6: the fake helper did not read the frame's messages");
    g.earlyAfter = x.e.qN; g.noHand = x.e.noHand - nh0;
    { const auto log = RingPipeLog(); for (size_t i = pipe1; i < log.size(); i++) if (log[i].type == 'G' && log[i].p == g.earlyN && (log[i].flags & 3u) == FG_G_CLOSED) g.closedInFrame = true; }
    g.shownA = MgTagCount(shown0, g.tagA);
    // A PAIR STILL OPEN WITH NO NEXT HAND
    s.wt.want = FG_WANT_ON;
    ShFrame<MA>(); ShFrame<MA>(); MgDueN();
    ShHandFrame(-1, 1000); RecordHudless(); RecordPresent(); Drain();   // its hand names another ordinal: the Present never matches
    if (!ShSettle()) Err("M6: the fake helper did not read the unmatched frame's messages");
    const LONG open0 = s.sh.pairOpen, cl0 = s.c.pubClosed[FG_CL_UNARMED];
    s.wt.want = FG_WANT_PASS;
    ClockAdd(16.7); ShInject(true);
    if (past) { Facts(); Fill(NewTag()); RecordPresent(); Drain(); } else { Frame<MA>(); Drain(); }
    if (!ShSettle()) Err("M6: the fake helper did not read the standby frame's messages");
    g.pairOpenAfter = open0 ? s.sh.pairOpen : -1; g.closedPub = s.c.pubClosed[FG_CL_UNARMED] - cl0;
    // (engaged again: what was left is closed by the next hand and the next injection, and the wire goes on)
    s.wt.want = FG_WANT_ON;
    ShFrame<MA>(); ShFrame<MA>(); MgDueN();
    ShPairs(RingPipeLog(), &g.bad);
    return g;
}
static void CaseM6()
{
    CaseBegin("M6 what has no hand is closed in its own frame: an early 'Q', and a pair still open (stage 7)");
    const M6Leg g = M6Run(false);
    printf("  the want fell between an injection and its Present: 'Q'(%ld) was on the pipe with no hand; after that frame it is %s, its 'G' CLOSED written in that frame: %s, counted closed with no hand %ld; the pair before's real frame on the screen %d time(s)\n",
        g.earlyN, g.earlyAfter ? "STILL OPEN" : "closed", g.closedInFrame ? "yes" : "no", g.noHand, g.shownA);
    printf("  a pair whose Present never matched, then a present with no hand: pairs still open after it %ld, published CLOSED %ld\n", g.pairOpenAfter, g.closedPub);
    if (gShAtHand) { if (g.earlyN != 0 || g.noHand != 0) Err("M6: under the key a 'Q' was written at an injection (%ld) or closed with no hand (%ld)", g.earlyN, g.noHand); }
    else if (g.earlyN == 0 || g.earlyAfter != 0 || !g.closedInFrame || g.noHand != 1) Err("M6: the 'Q' written at the injection (%ld) is %s after its frame, its 'G' CLOSED in that frame %s, counted %ld (closed, yes and 1 wanted)", g.earlyN, g.earlyAfter ? "still open" : "closed", g.closedInFrame ? "yes" : "NO", g.noHand);
    if (g.shownA != 1) Err("M6: the pair before's real frame was on the screen %d time(s), once wanted", g.shownA);
    if (g.pairOpenAfter != 0 || g.closedPub != 1) Err("M6: after a present with no hand %ld pair(s) are still open and %ld were published CLOSED (0 and 1 wanted; -1: the pair was not open to begin with)", g.pairOpenAfter, g.closedPub);
    if (!g.bad.empty()) Err("M6: %s", g.bad.c_str());
    MgClean("M6");
    MgEnd();
    const RingStats stc = gRingSt;
    const M6Leg c = M6Run(true);
    ViolRestore(stc);
    MgEnd();
    if (c.pairOpenAfter != 1 || (!gShAtHand && (c.earlyAfter == 0 || c.closedInFrame))) Err("M6 control: with the present recorded past the machine the early 'Q' is %s after its frame (its 'G' in that frame: %d) and %ld pair(s) are open: the control shows nothing", c.earlyAfter ? "still open" : "closed", (int)c.closedInFrame, c.pairOpenAfter);
    Ctl("the present with no hand recorded past the machine: the open pair stayed open (%ld)%s", c.pairOpenAfter, gShAtHand ? "" : Fmt(", and 'Q'(%ld) was still open when its frame was over", c.earlyN));
    CaseEnd("M6 what has no hand is closed in its own frame: an early 'Q', and a pair still open (stage 7)");
}

// ---------------------------------------------------------------- M7, no pair stays armed across a build
// WHAT IT SHOWS (READY-FOR-7.md, section 1, item 7; the owner's ruling: HOLD and SAVE are the game's own and live with the
// back buffer, the set's images go with the build). Until stage 7 the quiesce before a build knew only an OPEN pair: one
// ARMED across a build would have fired from the NEW set's images, which nothing of it ever filled.
//   (a) THE PAIR IS BROUGHT TO ITS REAL FRAME BEFORE THE SET GOES: the render thread records a drain note ahead of the
//       build (BeforeBuild) and the build's own drain replays it: the real frame is on the screen before the build
//       message, nothing is dropped, HOLD is the same image after the build, and two frames of the new set go by clean.
//   (b) THAT NOTE REFUSED (no BeforeBuild): the quiesce finds the pair armed and ENDS it: its generated frame is not
//       shown and its real frame is DROPPED, counted with its reason; no fire after the build names it.
// CONTROL: the quiesce as it stood until this stage (a copy in the harness: it leaves an armed pair alone): the pair's
// generated frame, asked for after the build, is landed from the NEW set's GEN, which nothing has written: the fake
// counts it and the screen shows a sentinel.
static void M7OldBeforeRelease()
{
    FgState& s = B::S(); FgShadowState& x = ShB::x;
    MA::ShadowWriteOwed(); MA::ShadowCloseEarly(true); x.e.engaged = 0;
    x.w.gapLatch = 1; x.w.failRun = 0; s.w.lastSentN = B::sentN; _InterlockedIncrement(&x.sh.bgen); s.r.lastEngaged = false;
}
static void CaseM7()
{
    CaseBegin("M7 no pair stays armed across a build: its real frame first, or dropped and counted (stage 7)");
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    // (a)
    const DWORD tagA = ShFrame<MA>();
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown0 = MgShownN(); const int hold0 = RingRig(FG_IMG_HOLD);
    MA::BeforeBuild(); Drain();
    const int shownBefore = MgTagCount(shown0, tagA); const bool armedAtBuild = s.w.openPhase == FG_PAIR_ARMED;
    const size_t pipeA = RingPipeLog().size();
    MA::BeforeRelease(); ShB::WriteB();
    if (!ShSettle()) Err("M7: the helper did not read the build");
    const bool holdSame = RingRig(FG_IMG_HOLD) == hold0 && !ShImgGone(hold0);
    const LONG droppedA = a.w.dropped[FG_AD_BUILD];
    const DWORD t1 = ShFrame<MA>(); MgDueShow(); MgDueN();
    const DWORD t2 = ShFrame<MA>(); MgDueShow(); MgDueN();
    const auto shownA = Shown();
    const std::string tailA = TagList(shownA, shown0);
    const bool orderA = shownA.size() == shown0 + 4 && shownA[shown0].tag == tagA && shownA[shown0 + 1].tag == t1 && shownA[shown0 + 2].tag == ShGenTag(t2, false) && shownA[shown0 + 3].tag == t2;
    const bool buildAfterN = pipeA < RingPipeLog().size() && RingPipeLog()[pipeA].type == 'B';
    // (b)
    const DWORD tagB = ShFrame<MA>();
    const bool armedB = s.w.openPhase == FG_PAIR_ARMED;
    const size_t shown1 = MgShownN();
    MA::BeforeRelease();
    const bool armedAfter = s.w.openPhase == FG_PAIR_ARMED; const LONG droppedB = a.w.dropped[FG_AD_BUILD] - droppedA;
    ShB::WriteB();
    if (!ShSettle()) Err("M7: the helper did not read the second build");
    MgDueShow(); MgDueN();                                    // (dues for the pair that is gone: nothing may fire)
    const int shownB = MgTagCount(shown1, tagB); const size_t firedAfter = MgShownN() - shown1;
    ShFrame<MA>(); MgDueN(); ShFrame<MA>(); MgDueShow(); MgDueN();
    printf("  (a) pair %lX armed (%s), the build's drain note recorded ahead of it: its real frame on the screen %d time(s) BEFORE the build message (still armed at the build: %s); HOLD the same image after the build: %s; dropped %ld; then the screen showed %s (the first pair of the new set shows nothing generated, the second does)\n",
        tagA, armedA ? "yes" : "NO", shownBefore, armedAtBuild ? "YES" : "no", holdSame ? "yes" : "NO", droppedA, tailA.c_str());
    printf("  (b) pair %lX armed (%s), no drain note: after the quiesce it is %s, dropped for a build %ld; dues written for it after the build fired %zu picture(s); the frame was on the screen %d time(s)\n",
        tagB, armedB ? "yes" : "NO", armedAfter ? "STILL ARMED" : "ended", droppedB, firedAfter, shownB);
    if (!armedA || shownBefore != 1 || armedAtBuild || !buildAfterN || droppedA != 0) Err("M7 (a): the pair was %s, its real frame on the screen %d time(s) before the build, still armed at the build %d, the build message next on the pipe %d, dropped %ld (armed, 1, no, yes and 0 wanted)", armedA ? "armed" : "NOT armed", shownBefore, (int)armedAtBuild, (int)buildAfterN, droppedA);
    if (!holdSame || !orderA) Err("M7 (a): HOLD is %s after the build, and the screen showed %s (%lX, %lX, %lX, %lX wanted)", holdSame ? "the same image" : "ANOTHER image or gone", tailA.c_str(), tagA, t1, ShGenTag(t2, false), t2);
    if (!armedB || armedAfter || droppedB != 1 || firedAfter != 0 || shownB != 0) Err("M7 (b): the pair was %s before the quiesce and %s after, dropped for a build %ld time(s), and %zu picture(s) fired for it after the build (armed, ended, 1 and 0 wanted)", armedB ? "armed" : "NOT armed", armedAfter ? "still armed" : "ended", droppedB, firedAfter);
    MgClean("M7");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>();
    M7OldBeforeRelease(); ShB::WriteB();
    if (!ShSettle()) Err("M7 control: the helper did not read the build");
    const LONG v0 = gRingSt.setVirgin;
    MgDueShow();
    const LONG virgin = gRingSt.setVirgin - v0; const DWORD shownC = ShLastShown();
    MgDueN();
    ViolRestore(stc);
    MgEnd();
    if (virgin != 1 || shownC != kSentinel + 0x100u * (DWORD)FG_IMG_GEN) Err("M7 control: with the quiesce as it was a pair armed across a build landed %ld image(s) nothing wrote and the screen showed %lX (1 and GEN's sentinel %lX wanted): the control shows nothing", virgin, shownC, kSentinel + 0x100u * (DWORD)FG_IMG_GEN);
    Ctl("the quiesce as it stood until stage 7: the pair stayed armed across the build and its fire landed the new set's GEN, which nothing wrote: counted %ld, the screen showed %lX", virgin, shownC);
    CaseEnd("M7 no pair stays armed across a build: its real frame first, or dropped and counted (stage 7)");
}

// ---------------------------------------------------------------- M8, a write that was not submitted presents nothing
// WHAT IT SHOWS (the owner's ruling; check-machine.md, X1; the critic's section 5: "a fire whose copy did not happen
// presents nothing"; READY-FOR-7.md, section 6, point 5). A pair is armed and THE GAME IS ALREADY DRAWING THE NEXT FRAME
// into the back buffer. The worker is HELD, a fault is armed and the fire's note is queued behind it:
//   the generated frame's write refused by the queue    NOTHING is presented, nothing restored; the back buffer is the
//                                                       game's drawing still; the pair goes on and its real frame is shown
//   the real frame's write refused                      NOTHING is presented: the real frame is DROPPED, counted, never
//                                                       shown from a back buffer that holds the next frame half drawn
//   no command buffer for a fire                        the same, before any lock
//   no SAVE image (the back buffer was written)         nothing is begun, nothing submitted, nothing presented
//   (the RESTORE refused, the fire's second submit, was a leg here until stage 7c: a lost restore now stands the machine
//   down for the session, and M20 holds all of it)
//   no HOLD image at the hand                          the hand's span begins nothing: the pair is NOSOURCE on the wire
//                                                       and is presented inline, at once (it has no store)
// and the fake counts no Present after a refused submit in any leg.
// CONTROL: the rig's fire with the same refusal (FK8's own leg): it presents all the same, the next frame half drawn.
static void M8CbFail(DWORD, DWORD, DWORD) { _InterlockedExchange(&gRing.cbFailNext, 1); }
struct M8Leg { DWORD tag, half, bb; LONG presents, refused, after, submits; bool armedAfter; };
static M8Leg M8Fire(bool isI, int plant)
{
    M8Leg g = {};
    FgState& s = B::S();
    g.tag = ShFrame<MA>();
    if (s.w.openPhase != FG_PAIR_ARMED) Err("M8: the pair was not armed before the fire");
    g.half = NewTag(); Fill(g.half); Drain();                 // the game is drawing the next frame
    const RingStats st0 = gRingSt;
    if (isI) DueI(MgP(), FG_DUE_SHOW); else DueN(MgP());
    if (plant == 3) ShArmB::noSave = 1;
    MgHeld([&] { if (plant == 0 || plant == 1) Run(ShFk8Arm, (DWORD)plant); if (plant == 2) Run(M8CbFail); FireNote(); });
    g.presents = gRingSt.presents - st0.presents; g.refused = gRingSt.refusedInSpan - st0.refusedInSpan; g.after = gRingSt.presentAfterRefused - st0.presentAfterRefused;
    g.submits = gRingSt.submits - st0.submits; g.bb = MgBb(); g.armedAfter = s.w.openPhase == FG_PAIR_ARMED;
    _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0); _InterlockedExchange(&gRing.cbFailNext, 0); ShArmB::noSave = 0;
    return g;
}
static void CaseM8()
{
    CaseBegin("M8 a write that was not submitted presents nothing and restores nothing (stage 7)");
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x; FgState& s = B::S();
    ShWarm<MA>(); MgDueN(); ShFrame<MA>(); MgDueN();
    LONG afterAll = 0;
    // the generated frame's write refused: nothing presented, and the pair goes on with its real frame
    const LONG nc0 = a.w.iWhy[FG_AW_NOCOPY];
    const M8Leg i0 = M8Fire(true, 0);
    const LONG iNoCopy = a.w.iWhy[FG_AW_NOCOPY] - nc0; const size_t sh0 = MgShownN();
    MgDueN();
    const int i0Real = MgTagCount(sh0, i0.tag); afterAll += i0.after;
    // the real frame's write refused: dropped, counted
    const LONG d0 = a.w.dropped[FG_AD_NOCOPY]; const size_t sh1 = MgShownN();
    const M8Leg n0 = M8Fire(false, 0);
    const LONG nDropped = a.w.dropped[FG_AD_NOCOPY] - d0; const int n0Shown = MgTagCount(sh1, n0.tag); afterAll += n0.after;
    // no command buffer
    const LONG nb0 = a.w.iWhy[FG_AW_NOBUFFER];
    const M8Leg cb = M8Fire(true, 2);
    const LONG iNoBuffer = a.w.iWhy[FG_AW_NOBUFFER] - nb0; MgDueN(); afterAll += cb.after;
    // no SAVE image
    const LONG d1 = a.w.dropped[FG_AD_NOCOPY];
    const M8Leg ns = M8Fire(false, 3);
    const LONG nsDropped = a.w.dropped[FG_AD_NOCOPY] - d1; afterAll += ns.after;
    // no HOLD image at the hand
    ShFrame<MA>(); MgDueN();
    const LONG nh0 = a.w.inl[FG_AI_NOHOLD], sub0 = gRingSt.submits; const size_t sh3 = MgShownN();
    ShArmB::noHold = 1; const DWORD tagH = ShFrame<MA>(); ShArmB::noHold = 0;
    const LONG nNoHold = a.w.inl[FG_AI_NOHOLD] - nh0, subH = gRingSt.submits - sub0; const int hShown = MgTagCount(sh3, tagH);
    const int kindH = ShKindOf(RingPipeLog(), x.w.lastQN); const bool armedH = s.w.openPhase == FG_PAIR_ARMED;
    ShFrame<MA>(); MgDueN();
    printf("  the generated frame's write refused: %ld present(s), back buffer %lX (the game's drawing %lX), refused inside the span %ld, not shown for want of its copy %ld; the pair went on: its real frame on the screen %d time(s)\n", i0.presents, i0.bb, i0.half, i0.refused, iNoCopy, i0Real);
    printf("  the real frame's write refused: %ld present(s), back buffer %lX (%lX), dropped and counted %ld, on the screen %d time(s); no command buffer: %ld present(s), counted %ld; no SAVE image: %ld present(s), %ld submit(s), dropped %ld\n",
        n0.presents, n0.bb, n0.half, nDropped, n0Shown, cb.presents, iNoBuffer, ns.presents, ns.submits, nsDropped);
    printf("  no HOLD image at the hand: 'G' %s, armed %s, presented inline %ld, %ld submit(s), on the screen %d time(s); presents after a refused submit, all legs: %ld\n",
        ShKindName(kindH), armedH ? "YES" : "no", nNoHold, subH, hShown, afterAll);
    if (i0.presents != 0 || i0.bb != i0.half || i0.refused != 1 || iNoCopy != 1 || !i0.armedAfter || i0Real != 1) Err("M8: the generated frame's write refused: %ld present(s), back buffer %lX (%lX wanted), refused %ld, counted %ld, still armed %d, its real frame shown %d time(s) (0, the drawing, 1, 1, yes and 1 wanted)", i0.presents, i0.bb, i0.half, i0.refused, iNoCopy, (int)i0.armedAfter, i0Real);
    if (n0.presents != 0 || n0.bb != n0.half || nDropped != 1 || n0Shown != 0 || n0.armedAfter) Err("M8: the real frame's write refused: %ld present(s), back buffer %lX (%lX wanted), dropped %ld, on the screen %d, still armed %d (0, the drawing, 1, 0 and no wanted)", n0.presents, n0.bb, n0.half, nDropped, n0Shown, (int)n0.armedAfter);
    if (cb.presents != 0 || iNoBuffer != 1 || cb.bb != cb.half) Err("M8: no command buffer: %ld present(s), counted %ld, back buffer %lX (0, 1 and the drawing %lX wanted)", cb.presents, iNoBuffer, cb.bb, cb.half);
    if (ns.presents != 0 || ns.submits != 0 || nsDropped != 1 || ns.bb != ns.half) Err("M8: no SAVE image: %ld present(s), %ld submit(s), dropped %ld, back buffer %lX (0, 0, 1 and the drawing %lX wanted)", ns.presents, ns.submits, nsDropped, ns.bb, ns.half);
    if (kindH != (int)FG_G_NOSOURCE || armedH || nNoHold != 1 || subH != 0 || hShown != 1) Err("M8: no HOLD image at the hand: 'G' %s, armed %d, inline %ld, %ld submit(s), on the screen %d time(s) (NOSOURCE, no, 1, 0 and 1 wanted)", ShKindName(kindH), (int)armedH, nNoHold, subH, hShown);
    if (afterAll != 0) Err("M8: %ld Present(s) were made after a refused submit of the same fire", afterAll);
    MgClean("M8");
    MgEnd();
    const RingStats stc = gRingSt;
    const ShFk8Leg rig = ShFk8Run(false, 0);
    ViolRestore(stc);
    if (rig.after != 1 || rig.shown != rig.half) Err("M8 control: the rig's fire with its copy-in refused made %ld present(s) after the refusal and showed %lX (1 and the next frame's drawing %lX wanted): the control shows nothing", rig.after, rig.shown, rig.half);
    Ctl("the rig's fire with the same refusal: it presented all the same (%ld), the next frame half drawn (%lX for %lX)", rig.after, rig.shown, rig.frame);
    CaseEnd("M8 a write that was not submitted presents nothing and restores nothing (stage 7)");
}

// ---------------------------------------------------------------- M9, the gate before every present, of any kind
// WHAT IT SHOWS (the owner's ruling R4; READY-FOR-7.md, section 1, item 8, and section 6, point 1; fg_xport.h,
// FgxPresentGate). The gate before DXVK's Present was built for one present a frame. The merged path makes up to three
// (a generated frame's fire, a real frame's, and the forwarded ones), and THE RULE IS MADE GENERAL: every present that
// goes to DXVK, whatever its kind, is preceded by ONE call of the binding's gate, on the thread that presents, made
// before DXVK's Present is entered and after the present before it was, WITH NO LOCK HELD.
//   BY THE FAKE'S TWO LOGS: over fires of both kinds, an inline present and presents passed through, every present has
//   exactly one call before it, on its own thread. CONTROLS, by the machine's own switch for each site: the generated
//   frame's fire ungated, then the real frame's: exactly those presents reach DXVK bare.
//   BY WHAT THE GATE HOLDS, in the three stream modes, with a model of the product's gate (it holds on the CPU until the
//   helper has reached what was queued when the PREVIOUS call was made) and the fake helper HELD in a prepare: the
//   pending real frame goes out AHEAD of the wait (M5); the wait is queued; THE FIRST PRESENT AFTER IT GOES OUT (it
//   stands on the queue behind the wait); THE SECOND DOES NOT: the worker stands in the gate, on the CPU, holding no
//   lock, and DXVK's Present is not entered until the helper answers. Then everything is shown, once, in order.
//   CONTROL: the real frame's fire ungated: the second present is entered behind the wait while the helper is held.
struct M9Read { ShGateRead r; LONG fI, fN, inl, pass; };
static M9Read M9Logs(LONG off)
{
    M9Read g = {};
    _InterlockedExchange(&gPresentGateOff, off);
    MgFresh<MA>();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    const size_t e0 = RingEnteredLog().size(), n0 = RingPreNotedLog().size();
    const LONG i0 = s.c.presentsI, r0 = s.c.presentsN, l0 = s.c.presentsInline, p0 = s.c.presentsPass;
    for (int i = 0; i < 3; i++) { ShFrame<MA>(); MgDueShow(); MgDueN(); }
    ShFrame<MA>(false);                                       // not sent: inline
    s.wt.want = FG_WANT_PASS; ShFrame<MA>(); ShFrame<MA>(); s.wt.want = FG_WANT_ON;
    g.r = ShGateSince(e0, n0);
    g.fI = s.c.presentsI - i0; g.fN = s.c.presentsN - r0; g.inl = s.c.presentsInline - l0; g.pass = s.c.presentsPass - p0;
    MgEnd();
    _InterlockedExchange(&gPresentGateOff, 0);
    return g;
}
struct M9Hold { bool inPrep, inGate, stillPrep; int entered, a, b, c, aAfter, bAfter, cAfter; LONG lockDepth, held, gaveUp; };
static M9Hold M9Run(int mode, LONG off)
{
    M9Hold g = {};
    _InterlockedExchange(&gPresentGateOff, off);
    MgFresh<MA>();
    FgState& s = B::S();
    ShArmB::gateModel = 1;
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    const DWORD tagA = ShFrame<MA>(true, false);
    g.inPrep = WaitFor([] { return gH.inPrep != 0; }, 3000);
    gRing.presentCs = mode;
    const size_t shown0 = MgShownN();
    ClockAdd(16.7); ShInject(true); Drain();                  // frame B's injection: the pending real frame, the flush, then the wait
    const size_t e1 = RingEnteredLog().size();
    const DWORD tagB = Frame<MA>(); Drain();
    MgDueN();                                                 // B's real frame: the first present after the wait was queued
    ClockAdd(16.7); ShInject(true); const DWORD tagC = Frame<MA>(); Drain();
    DueN(MgP()); FireNote();                                  // C's real frame: the second
    g.inGate = WaitFor([] { return ShArmB::gateIn != 0; }, off ? 400 : 3000);
    if (!g.inGate) Drain();
    g.entered = (int)(RingEnteredLog().size() - e1); g.lockDepth = s.w.lockDepth + (gRingInterop.held ? 100 : 0);
    g.a = MgTagCount(shown0, tagA); g.b = MgTagCount(shown0, tagB); g.c = MgTagCount(shown0, tagC);
    g.stillPrep = gH.inPrep != 0;
    gRing.presentCs = RING_CS_LAZY;
    ShRelease();
    Drain();
    if (!WaitFor([] { return ShB::waitFor == 0; }, 3000)) Err("M9: the queue's wait did not end after the helper's release");
    if (!ShSettle()) Err("M9: the fake helper did not come back after its hold");
    g.aAfter = MgTagCount(shown0, tagA); g.bAfter = MgTagCount(shown0, tagB); g.cAfter = MgTagCount(shown0, tagC);
    g.held = ShArmB::gateHeld; g.gaveUp = ShArmB::gateGaveUp;
    ShFrame<MA>(); MgDueN();
    MgClean("M9");
    MgEnd();
    _InterlockedExchange(&gPresentGateOff, 0);
    return g;
}
static void CaseM9()
{
    CaseBegin("M9 the gate before every present, of any kind: one call, no lock held, and at most one present behind a wait (stage 7)");
    const M9Read g = M9Logs(0);
    printf("  by the two logs: %d present(s) (%ld generated frames' fires, %ld real frames', %ld inline, %ld passed through), %d on the worker: bare %d, with two calls %d, misplaced %d, calls with no present after them %d\n",
        g.r.presents, g.fI, g.fN, g.inl, g.pass, g.r.onWorker, g.r.bare, g.r.twice, g.r.misplaced, g.r.loose);
    if (g.fI != 3 || g.fN < 3 || g.inl != 1 || g.pass != 2 || g.r.presents != (int)(g.fI + g.fN + g.inl + g.pass)) Err("M9: the leg made %ld, %ld, %ld and %ld presents of the four kinds (3, at least 3, 1 and 2 wanted), %d in all", g.fI, g.fN, g.inl, g.pass, g.r.presents);
    if (ShGateFaults(g.r) || g.r.onWorker != g.r.presents) Err("M9: of %d present(s), %d had no call of the gate before them, %d had two, %d had one misplaced; %d call(s) had no present; %d were on the worker", g.r.presents, g.r.bare, g.r.twice, g.r.misplaced, g.r.loose, g.r.onWorker);
    static const int kModes[3] = { RING_CS_LAZY, RING_CS_PRE, RING_CS_ALL };
    static const char* const kName[3] = { "LAZY", "PRE", "ALL" };
    for (int m = 0; m < 3; m++)
    {
        const M9Hold h = M9Run(kModes[m], 0);
        printf("  %-4s  the helper held in a prepare (%s): presents entered after the wait was queued %d; the worker in the gate %s, holding %ld lock(s); on the screen meanwhile: the frame forced ahead %d, the first behind the wait %d, the second %d; after the release %d %d %d; the gate held %ld time(s), gave up %ld\n",
            kName[m], h.inPrep && h.stillPrep ? "yes" : "NO", h.entered, h.inGate ? "yes" : "NO", h.lockDepth, h.a, h.b, h.c, h.aAfter, h.bAfter, h.cAfter, h.held, h.gaveUp);
        if (!h.inPrep || !h.stillPrep) Err("M9 %s: the helper was not held in its prepare through the leg", kName[m]);
        if (h.entered != 1 || !h.inGate || h.lockDepth != 0) Err("M9 %s: after the wait was queued %d present(s) entered DXVK while the helper was held (1 wanted: the first goes out, the second stands in the gate), the worker in the gate %d, holding %ld lock(s) (0 wanted)", kName[m], h.entered, (int)h.inGate, h.lockDepth);
        if (h.a != 1 || h.b != 0 || h.c != 0) Err("M9 %s: while the helper was held the screen had the frame forced ahead of the wait %d time(s), the first behind it %d, the second %d (1, 0 and 0 wanted)", kName[m], h.a, h.b, h.c);
        if (h.aAfter != 1 || h.bAfter != 1 || h.cAfter != 1 || h.gaveUp != 0 || h.held < 1) Err("M9 %s: after the release the three real frames were on the screen %d, %d and %d time(s) (once each wanted); the gate held %ld time(s) and gave up %ld", kName[m], h.aAfter, h.bAfter, h.cAfter, h.held, h.gaveUp);
    }
    // CONTROLS
    const RingStats stc = gRingSt;
    const M9Read cI = M9Logs(4), cN = M9Logs(8);
    const M9Hold cH = M9Run(RING_CS_LAZY, 8);
    ViolRestore(stc);
    if (cI.r.bare != (int)cI.fI || cN.r.bare != (int)cN.fN || cI.fI != 3 || cN.fN < 3) Err("M9 control: with the generated frame's fire ungated %d present(s) were bare (its %ld wanted), with the real frame's %d (its %ld wanted): the control shows nothing", cI.r.bare, cI.fI, cN.r.bare, cN.fN);
    if (cH.entered != 2 || cH.inGate) Err("M9 control: with the real frame's fire ungated %d present(s) entered DXVK behind the wait while the helper was held and the worker stood in the gate %d (2 and no wanted): the control shows nothing", cH.entered, (int)cH.inGate);
    Ctl("the generated frame's fire ungated: %d of %d presents bare; the real frame's ungated: %d bare, and with the helper held a second present entered DXVK behind the wait (%d)", cI.r.bare, cI.r.presents, cN.r.bare, cH.entered);
    CaseEnd("M9 the gate before every present, of any kind: one call, no lock held, and at most one present behind a wait (stage 7)");
}

// ---------------------------------------------------------------- M10, the answer is read by equality, on the worker
// WHAT IT SHOWS (the owner's ruling R6; critic X2; machine.md's machine-7 b with check-machine.md's R4, whose control this
// is; helper-wire.md, helper-wire-11). A DUE THAT SAYS SHOW IS PLANTED (the harness writes it, whatever a placer would
// have decided) for a pair that may show a frame, with fgOut standing at each value that is not "this pair's frame":
//   2n + 1      the helper answered "no frame"
//   below 2n    no answer yet: the helper is HELD inside this pair's dispatch (and is writing GEN)
//   the top     the helper is gone, and its fences stand at the top for ever
// THE FIRE READS THE FENCE ITSELF, ON THE WORKER, AND SHOWS NOTHING, each time; the pair goes on with its real frame alone,
// shown once, in its place; and the fake counts no read of GEN at all. Nothing waits: the real frame's fire does not ask.
// CONTROL: a binding whose read on the worker says "this pair's frame" whatever the fence holds (the read by equality
// switched off): with the helper held inside the dispatch the fire lands GEN, which is the pair BEFORE's image and is
// being written: the screen shows another pair's generated frame, and the fake counts both.
static void CaseM10()
{
    CaseBegin("M10 the answer is read by equality, on the worker, inside the fire (stage 7)");
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    LONG pres[3] = {}; int real[3] = {}; DWORD tag[3] = {}; bool reached[3] = {};
    const LONG na0 = a.w.iWhy[FG_AW_NOANSWER];
    auto leg = [&](int i) { const LONG p0 = gRingSt.presents; const size_t sh = MgShownN(); MgDueShow(); pres[i] = gRingSt.presents - p0; MgDueN(); real[i] = MgTagCount(sh, tag[i]) + 10 * (int)(MgShownN() - sh - 1); };
    // 2n + 1
    _InterlockedExchange(&gH.noFrame, 1); tag[0] = ShFrame<MA>(); _InterlockedExchange(&gH.noFrame, 0);
    reached[0] = B::TlValue(FG_TL_OUT) == 2ull * (ULONGLONG)ShB::x.w.openN + 1;
    leg(0);
    ShFrame<MA>(); MgDueN(); ShFrame<MA>(); MgDueShow(); MgDueN();   // (the pair behind it resets; the next is plain again)
    // below 2n
    _InterlockedExchange(&gH.holdDispatch, 1); tag[1] = ShFrame<MA>(true, false);
    reached[1] = WaitFor([] { return gH.inDispatch != 0; }, 3000) && B::TlValue(FG_TL_OUT) < 2ull * (ULONGLONG)ShB::x.w.openN;
    leg(1);
    ShRelease();
    if (!ShSettle()) Err("M10: the fake helper did not come back after its hold");
    ShFrame<MA>(); MgDueShow(); MgDueN();
    // the top
    _InterlockedExchange(&gH.holdDispatch, 1); tag[2] = ShFrame<MA>(true, false);
    reached[2] = WaitFor([] { return gH.inDispatch != 0; }, 3000);
    ShHelperGone();
    reached[2] = reached[2] && WaitFor([] { return B::TlValue(FG_TL_OUT) == ~0ull; }, 3000);
    leg(2);
    const LONG noAnswer = a.w.iWhy[FG_AW_NOANSWER] - na0;
    printf("  a due that says SHOW, planted, with fgOut at 2n+1, below 2n (the helper inside the dispatch) and at the top (the helper gone): presents made for the generated frame %ld %ld %ld; not shown because the answer read on the worker was not this pair's frame: %ld; each pair's real frame on the screen %d %d %d time(s)\n",
        pres[0], pres[1], pres[2], noAnswer, real[0], real[1], real[2]);
    if (!reached[0] || !reached[1] || !reached[2]) Err("M10: a leg did not reach its fence value (2n+1 %d, below 2n with the helper in the dispatch %d, the top %d)", (int)reached[0], (int)reached[1], (int)reached[2]);
    if (pres[0] || pres[1] || pres[2] || noAnswer != 3) Err("M10: with a due that says SHOW and no frame of this pair's, %ld, %ld and %ld present(s) were made for the generated frame, and %ld were counted not shown for the answer (0, 0, 0 and 3 wanted)", pres[0], pres[1], pres[2], noAnswer);
    if (real[0] != 1 || real[1] != 1 || real[2] != 1) Err("M10: the pairs' real frames were on the screen %d, %d and %d time(s) (once each, and nothing else, wanted)", real[0], real[1], real[2]);
    MgClean("M10");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MAL>();
    ShWarm<MAL>(); MgDueN();
    const DWORD tagP = ShFrame<MAL>(); MgDueShow(); MgDueN();
    _InterlockedExchange(&gH.holdDispatch, 1); const DWORD tagL = ShFrame<MAL>(true, false);
    if (!WaitFor([] { return gH.inDispatch != 0; }, 3000)) Err("M10 control: the fake helper never reached the held dispatch");
    ShArmLieB::lie = 1; MgDueShow(); ShArmLieB::lie = 0;
    const DWORD shownL = ShLastShown();
    const LONG unans = gRingSt.genUnanswered - stc.genUnanswered, busy = gRingSt.genBusy - stc.genBusy;
    MgDueN();
    ViolRestore(stc);
    ShRelease(); ShSettle();
    MgEnd();
    if (shownL != ShGenTag(tagP, false) || unans != 1 || busy != 1) Err("M10 control: with the read by equality switched off the screen showed %lX (the pair before's generated image %lX wanted; this pair's frame is %lX) and the fake counted %ld and %ld (1 and 1 wanted): the control shows nothing", shownL, ShGenTag(tagP, false), tagL, unans, busy);
    Ctl("the read by equality switched off, the helper inside the dispatch: the screen showed %lX, the generated frame of the pair BEFORE; the fake counted GEN read with no answer of its pair's %ld and while the helper wrote it %ld", shownL, unans, busy);
    CaseEnd("M10 the answer is read by equality, on the worker, inside the fire (stage 7)");
}

// ---------------------------------------------------------------- M11, the back buffer's save: needed, or not
// WHAT IT SHOWS (the main session's ruling R8 after the desk priced a shown pair; the critic's section 5: "no game image
// keeps a generated pixel; the back buffer after a fire equals the back buffer before it, at one to four images"). A fire
// writes a picture into the back buffer, where the game may already be drawing the next frame: so it saves the back
// buffer first and puts it back after its Present, two full-screen copies a fire. THE BINDING SAYS WHETHER THAT IS
// NEEDED (B::BackWritten: has the game written the back buffer since the pair was armed). At ONE, TWO, THREE AND FOUR
// images in the ring, a generated frame's fire and a real frame's each time:
//   THE DEFAULT ANSWER ("written", always) and the back buffer written: both fires save; after each the back buffer IS
//       THE GAME'S HALF-DRAWN PICTURE, exactly, and no generated picture is left in it.
//   THE FAKE'S OWN KNOWLEDGE, the back buffer written: the same.
//   THE FAKE'S OWN KNOWLEDGE, the back buffer NOT written: NO COPY IS MADE FOR SAVE (each fire is ONE submit: its write),
//       and the screen still shows the pair's generated frame and then its real frame.
//   Not written at the generated frame's fire, WRITTEN BEFORE THE REAL FRAME'S: the first saves nothing, the second saves,
//       and the game's picture is exact after it.
// THE PLANTED VIOLATION, which the fake must count: the back buffer written and a binding that says "not written": the
// fire presents over the game's picture with no save of it, and the back buffer afterwards is not what the game drew.
struct M11Res { int bad = 0; std::string first; LONG savesD = 0, noSavesU = 0, lost = 0; };
static void M11Images(int images, M11Res& r)
{
    MgFresh<MA>(images);
    FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    auto fail = [&](const char* what, DWORD got, DWORD want) { r.bad++; if (r.first.empty()) r.first = Fmt("%d image(s), %s: %lX where %lX is wanted", images, what, got, want); };
    // legs 0 the default answer, written; 1 the fake's knowledge, written
    for (int mode = 0; mode < 2; mode++)
    {
        ShArmB::backMode = mode;
        const DWORD t = ShFrame<MA>(); const DWORD half = NewTag(); Fill(half); Drain();
        const LONG s0 = a.w.saves; size_t sh = MgShownN();
        MgDueShow(); if (ShLastShown() != ShGenTag(t, false) || MgShownN() != sh + 1) fail("written, the generated frame on the screen", ShLastShown(), ShGenTag(t, false));
        if (MgBb() != half) fail("written, the back buffer after the generated frame's fire", MgBb(), half);
        sh = MgShownN();
        MgDueN(); if (ShLastShown() != t || MgShownN() != sh + 1) fail("written, the real frame on the screen", ShLastShown(), t);
        if (MgBb() != half) fail("written, the back buffer after the real frame's fire", MgBb(), half);
        if (a.w.saves - s0 != 2) fail("written, fires that saved", (DWORD)(a.w.saves - s0), 2);
        if (!mode) r.savesD += a.w.saves - s0;
    }
    // leg 2 the fake's knowledge, not written: no copy for SAVE
    {
        ShArmB::backMode = 1;
        const DWORD t = ShFrame<MA>();
        const LONG n0 = a.w.noSaves, sub0 = gRingSt.submits; size_t sh = MgShownN();
        MgDueShow(); if (ShLastShown() != ShGenTag(t, false) || MgShownN() != sh + 1) fail("not written, the generated frame on the screen", ShLastShown(), ShGenTag(t, false));
        sh = MgShownN();
        MgDueN(); if (ShLastShown() != t || MgShownN() != sh + 1) fail("not written, the real frame on the screen", ShLastShown(), t);
        if (a.w.noSaves - n0 != 2 || gRingSt.submits - sub0 != 2) fail("not written, submits of the two fires (2 wanted: one write each)", (DWORD)(gRingSt.submits - sub0), 2);
        r.noSavesU += a.w.noSaves - n0;
    }
    // leg 3 not written at the generated frame's fire, written before the real frame's
    {
        const DWORD t = ShFrame<MA>();
        const LONG n0 = a.w.noSaves, s0 = a.w.saves;
        MgDueShow();
        const DWORD half = NewTag(); Fill(half); Drain();
        const size_t sh = MgShownN();
        MgDueN(); if (ShLastShown() != t || MgShownN() != sh + 1) fail("written between the fires, the real frame on the screen", ShLastShown(), t);
        if (MgBb() != half) fail("written between the fires, the back buffer after the real frame's fire", MgBb(), half);
        if (a.w.noSaves - n0 != 1 || a.w.saves - s0 != 1) fail("written between the fires, fires that saved (1 wanted)", (DWORD)(a.w.saves - s0), 1);
    }
    if (gRingSt.backLost != gSt0.backLost) fail("the game's picture lost at a fire, on the clean legs", (DWORD)(gRingSt.backLost - gSt0.backLost), 0);
    // THE PLANTED VIOLATION
    {
        const RingStats st0 = gRingSt;
        ShArmB::backMode = 2;
        ShFrame<MA>(); const DWORD half = NewTag(); Fill(half); Drain();
        MgDueShow();
        const LONG lost = gRingSt.backLost - st0.backLost; const DWORD bb = MgBb();
        MgDueN();
        ShArmB::backMode = 0;
        if (lost != 1 || bb == half) fail("PLANTED, the lie counted (1 wanted), and the back buffer must NOT be the game's picture", (DWORD)lost, 1);
        r.lost += lost;
        ViolRestore(st0);
    }
    ShFrame<MA>(); MgDueN();
    MgClean("M11");
    MgEnd();
}
static void CaseM11()
{
    CaseBegin("M11 the back buffer's save, needed or not, at one to four images (stage 7)");
    M11Res r;
    for (int images = 1; images <= 4; images++) M11Images(images, r);
    printf("  at 1, 2, 3 and 4 images, a generated frame's fire and a real frame's each: with the default answer %ld fires saved and restored; with the back buffer not written and the binding saying so %ld fires made no copy for SAVE; %d check(s) failed%s%s\n",
        r.savesD, r.noSavesU, r.bad, r.bad ? ": " : "", r.first.c_str());
    printf("  PLANTED, the back buffer written and a binding that says \"not written\": the game's picture lost at a fire, counted %ld (once at each ring size)\n", r.lost);
    if (r.bad) Err("M11: %d check(s) failed; the first: %s", r.bad, r.first.c_str());
    if (r.savesD != 8 || r.noSavesU != 8) Err("M11: %ld fires saved under the default answer and %ld made no copy for SAVE where none was needed (8 and 8 wanted)", r.savesD, r.noSavesU);
    if (r.lost != 4) Err("M11 control: the planted lie was counted %ld time(s) over the four ring sizes, 4 wanted: the control shows nothing", r.lost);
    Ctl("the back buffer written and a binding that says \"not written\": the game's half-drawn picture was lost at the fire, counted %ld time(s), once at each ring size", r.lost);
    CaseEnd("M11 the back buffer's save, needed or not, at one to four images (stage 7)");
}

// ---------------------------------------------------------------- M12, one image: the flush before the restore
// WHAT IT SHOWS (the main session's ruling R9: on real DXVK a Present returns before its blit is submitted, and with one
// back buffer image the restore overtook the blit in 2 of 4 desk runs). ONE image in the ring, the game drawing the next
// frame, and a fire of each kind, in the three modes of the fake's command stream: the screen shows THE PICTURE THE FIRE
// WROTE (the pair's generated frame, then its real frame), never the game's half-drawn picture the restore puts back; and
// the back buffer afterwards is that half-drawn picture.
// CONTROL: a binding whose flush between a fire's Present and its restore is not made: where DXVK's thread had not yet
// run the blit (LAZY, PRE) the blit reads the image AFTER the restore, and the screen shows the game's half-drawn frame.
struct MgNoPostFlushB : ShArmB
{
    static inline size_t ent0 = 0;
    static void DevLock() { ShArmB::DevLock(); ent0 = RingEnteredLog().size(); }
    static void Flush() { if (GetCurrentThreadId() == dtWorkerTid && FgHarnessBinding::S().w.lockDepth > 0 && RingEnteredLog().size() != ent0) return; ShArmB::Flush(); }
};
typedef FgMachine<MgNoPostFlushB> MANP;
struct M12Leg { DWORD tag, half, sI, sN, bbI, bbN; };
template <class MM> static M12Leg M12Run(int mode)
{
    M12Leg g = {};
    MgFresh<MM>(1);
    ShWarm<MM>(); MgDueN();
    gRing.presentCs = mode;
    g.tag = ShFrame<MM>(); g.half = NewTag(); Fill(g.half); Drain();
    MgDueShow(); g.sI = ShLastShown(); g.bbI = MgBb();
    MgDueN(); g.sN = ShLastShown(); g.bbN = MgBb();
    gRing.presentCs = RING_CS_LAZY;
    ShFrame<MM>(); MgDueN();
    return g;
}
static void CaseM12()
{
    CaseBegin("M12 one image: the flush before the restore, in the three stream modes (stage 7)");
    static const int kModes[3] = { RING_CS_LAZY, RING_CS_PRE, RING_CS_ALL };
    static const char* const kName[3] = { "LAZY", "PRE", "ALL" };
    int wrongCtl[3] = {};
    for (int m = 0; m < 3; m++)
    {
        const M12Leg g = M12Run<MA>(kModes[m]);
        printf("  %-4s  one image, the game drawing %lX: the generated frame's fire showed %lX (%lX wanted) and the real frame's %lX (%lX wanted); the back buffer after each: %lX, %lX\n", kName[m], g.half, g.sI, ShGenTag(g.tag, false), g.sN, g.tag, g.bbI, g.bbN);
        if (g.sI != ShGenTag(g.tag, false) || g.sN != g.tag || g.bbI != g.half || g.bbN != g.half) Err("M12 %s: with one image the fires showed %lX and %lX (%lX and %lX wanted) and left the back buffer at %lX and %lX (the game's drawing %lX wanted)", kName[m], g.sI, g.sN, ShGenTag(g.tag, false), g.tag, g.bbI, g.bbN, g.half);
        MgClean("M12");
        MgEnd();
    }
    const RingStats stc = gRingSt;
    for (int m = 0; m < 3; m++) { const M12Leg c = M12Run<MANP>(kModes[m]); wrongCtl[m] = (c.sI == c.half ? 1 : 0) + (c.sN == c.half ? 1 : 0); MgEnd(); }
    ViolRestore(stc);
    if (wrongCtl[0] != 2 || wrongCtl[1] != 2 || wrongCtl[2] != 0) Err("M12 control: without the flush before the restore the screen showed the game's half-drawn frame %d, %d and %d time(s) of two in LAZY, PRE and ALL (2, 2 and 0 wanted): the control shows nothing", wrongCtl[0], wrongCtl[1], wrongCtl[2]);
    Ctl("the flush before the restore left out, one image: the screen showed the game's half-drawn frame in place of the fire's picture %d of 2 times in LAZY, %d in PRE, %d in ALL", wrongCtl[0], wrongCtl[1], wrongCtl[2]);
    CaseEnd("M12 one image: the flush before the restore, in the three stream modes (stage 7)");
}

// ================================================================================================================
// STAGE 7b: THE PLACER ON REAL ANSWERS (fg_machine.h, ArmedPlacerStep; critic.md's X1 and X2; pacing.md's PAC-1 to
// PAC-3; check-machine.md's R1 and X2; READY-FOR-7.md, section 1, item 3, and section 6, point 9).
//
// THE PLACER AND THE STAMPERS ARE STEPPED BY HAND HERE, on the harness's clock, to the tick: a frame begins on an even
// grid (16.7 ms), its handoff is stamped where it is made, and the clock is then set to the instants a case names
// (ticks after the handoff). The fake helper answers when that clock reaches its delay; a case that steps to an answer
// waits for the helper's thread to have signalled it before it steps the placer. So no verdict here depends on which
// thread ran first.
// ================================================================================================================
static const LONG kMgPeriodUs = 16700;
static LONGLONG gMgNext = 0;                     // when the next frame of an even run begins, on the harness's clock
static void MgEvenStart() { gMgNext = B::clockNow + MA::UsToTicks(kMgPeriodUs); }
// One frame of an even run: it begins on the grid whatever the case did to the clock inside the frame before.
template <class MM = MA> static DWORD MgEven(bool sent = true, bool settle = true)
{
    if (gMgNext < B::clockNow) gMgNext = B::clockNow;         // (a case that ran past the grid: the clock never goes back)
    B::clockNow = gMgNext; gMgNext += MA::UsToTicks(kMgPeriodUs);
    ShInject(sent);
    const DWORD tag = Frame<MM>(sent);
    Drain();
    if (settle && !ShSettle()) Err("the fake helper did not read the frame's messages within 3 s");
    return tag;
}
template <class MM = MA> static void MgStampOnly(int tl) { if (B::TlValue(tl) < 0x7FFFFFFFull) while (MM::StampStep(tl, &gMgLast[tl], 0)) {} }
static bool MgDue(bool real, LONG p, FgDue* out) { FgState& s = B::S(); return MA::Read(*(real ? s.dueN : s.dueI), p, out); }
static const FgArmedLive* MgLive(LONG p) { FgArmedPlacer& pl = ShArmB::a.pl; for (int i = 0; i < pl.liveN; i++) if (pl.live[i].p == p) return &pl.live[i]; return nullptr; }
static bool MgAnswered(LONG n) { return WaitFor([n] { return B::TlValue(FG_TL_OUT) >= 2ull * (ULONGLONG)n; }, 3000); }
// The helper has done all it can do now: it read everything and is idle, or it stands in a delay, or it is held.
static bool MgQuiet() { return WaitFor([] { return gH.gone || gH.inDelay || gH.inDispatch || gH.inPrep || (!gH.busy && (size_t)gH.next >= RingPipeLog().size() && !gH.busy); }, 3000, 7); }

// ONE PAIR UNDER THE PLACER, STEP BY STEP. answerUs: when the helper answers, after the handoff (0: at once; below 0: it
// answers "no frame"). The placer is stepped at the handoff (the pair is timed), one tick before the slot (nothing may
// be raised), at each of `looks` (ticks after the handoff, in order; the answer is waited for where a look is at or past
// it), then the placer's wake note; one tick before the real frame's due (it may not be raised), at the due, and the
// note again. What was raised, when, and what reached the screen. noIFire: the wake note behind the looks is left out, so
// no fire point reaches the worker between the slot and the real frame's due.
struct MgPl
{
    DWORD tag; LONG p, n; LONGLONG tS, tI, tN; bool timed, own, early, nEarly, hasI, hasN; FgDue di, dn; int hasIAt[4]; LONG verdictAt[4];
    LONG pad; int gen, real; size_t shown;
};
static MgPl MgPair1(LONG answerUs, const LONG* looksUs, int nLooks, bool held = false, bool noIFire = false)
{
    MgPl g = {};
    FgState& s = B::S(); FgShadowState& x = ShB::x;
    const LONG pad0 = s.sh.padQms;
    _InterlockedExchange(&gH.delayUs, answerUs > 0 ? answerUs : 0); _InterlockedExchange(&gH.noFrame, answerUs < 0 ? 1 : 0);
    const size_t sh = MgShownN();
    g.tag = MgEven<MA>(true, false);
    if (!MgQuiet()) Err("the fake helper did not settle behind the frame");
    g.p = MgP(); g.n = x.w.lastQN;
    MgPlace<MA>();
    if (const FgArmedLive* L = MgLive(g.p)) { g.timed = L->timed; g.own = L->tsOwn; g.tS = L->tS; g.tI = L->tI; g.tN = L->tN; }
    if (!g.timed) { Err("a pair of an even run was not armed and timed at its handoff (pair %ld)", g.p); return g; }
    FgDue d = {};
    const LONGLONG ansT = MA::UsToTicks(answerUs > 0 ? answerUs : 0);
    bool answered = answerUs <= 0;
    auto to = [&](LONGLONG t)
    {
        if (!answered && t >= ansT) { if (g.tS + ansT > B::clockNow) B::clockNow = g.tS + ansT; if (!MgAnswered(g.n)) Err("the delayed answer of pair %ld never came", g.n); answered = true; }
        if (g.tS + t > B::clockNow) B::clockNow = g.tS + t;
    };
    to(g.tI - g.tS - 1); MgPlace<MA>(); g.early = MgDue(false, g.p, &d) || MgDue(true, g.p, &d);
    for (int i = 0; i < nLooks && i < 4; i++)
    {
        to(MA::UsToTicks(looksUs[i])); MgPlace<MA>();
        g.hasIAt[i] = MgDue(false, g.p, &d) ? 1 : 0; g.verdictAt[i] = g.hasIAt[i] ? d.verdict : 0;
    }
    g.hasI = MgDue(false, g.p, &g.di);
    if (!noIFire) { FireNote(); Drain(); }
    to(g.tN - g.tS - 1); MgPlace<MA>(); g.nEarly = MgDue(true, g.p, &d);
    to(g.tN - g.tS); MgPlace<MA>(); g.hasN = MgDue(true, g.p, &g.dn);
    FireNote(); Drain();
    if (!answered) to(ansT);                                  // (an answer later than the pair's whole life: let it come before the next frame)
    const auto shown = Shown();
    g.shown = shown.size() - sh;
    for (size_t i = sh; i < shown.size(); i++) { g.gen += shown[i].tag == ShGenTag(g.tag, false); g.real += shown[i].tag == g.tag; }
    g.pad = s.sh.padQms - pad0;
    _InterlockedExchange(&gH.delayUs, 0); _InterlockedExchange(&gH.noFrame, 0);
    if (!held && !ShSettle()) Err("the fake helper did not come back behind the pair");   // (held: the case holds the helper through this pair and lets it go itself)
    return g;
}
static LONG MgWhyOf(const FgDue& d) { return (LONG)(d.why >> 16); }

// ---------------------------------------------------------------- P1, the placer on real answers
// WHAT IT SHOWS (the build map's machine-6 as critic X1 and X2 settled it; pacing.md, PAC-1 and PAC-3). The planned
// generation time is 2,000 us. One pair each:
//   the answer at 1,500 us          NOTHING is raised before the slot though the answer is in; AT THE SLOT, to the tick, the
//                                   generated frame is due and SHOWN; then the real frame, at the slot plus half the period
//   the answer at 1,900 us          the same (0.1 ms before the slot)
//   the answer at 2,100 us          0.1 ms after it: at the slot the fence reads below 2n: LATE, withheld, the pad told once;
//                                   the generated frame is never read; THE REAL FRAME'S DUE IS WHERE IT WAS
//   "no frame" (2n + 1)             withheld, no frame; the pad is not told; the real frame's due is where it was
//   the pair behind it              its dispatch resets: it may show nothing, so NO due is raised for its generated frame at
//                                   all, and its real frame's due is where every other pair's is
//   a late allowance of 500 us:     the answer at 2,300 us: undecided at the slot, SHOWN when the answer comes; at 2,700 us:
//                                   undecided at the slot, LATE at the allowance's end
//   a late allowance of 20,000 us   longer than half the period, the answer at 12,000 us: THE VERDICT IS GIVEN AT THE REAL
//                                   FRAME'S OWN RAISE, late, and never after it: the real frame's due is where it was
//   no fire point at the slot       the answer is in and the placer says SHOW at the slot, but no fire point reaches the
//                                   worker before the real frame's due is raised: THE GENERATED FRAME IS NOT SHOWN (it would
//                                   stand for an instant in front of the frame it leads to; machine-7 e), counted; the real
//                                   frame once, and the pair behind it shows its own
// and across all of them the real frame's due is the handoff plus the planned time plus half the period, the same number
// of ticks for every pair, and from one pair's real frame to the next is the frame's period, to the tick.
// CONTROL (a run): the same pair with the planned time handed in as 2,600 us where the answer comes at 2,100: SHOWN. The
// verdict follows the number that was handed in and the fence, nothing else.
static void CaseP1()
{
    CaseBegin("P1 the placer on real answers: one planned slot a pair, the answer read from the fence by equality (stage 7)");
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000; a.k.lateUs = 0;
    ShWarm<MA>(); MgDueN();
    s.r.sinceEngage = 1000;                                   // (past the warm-up: the pad hears a late pair)
    MgEvenStart();
    for (int i = 0; i < 4; i++) { const LONG at[1] = { 2000 }; MgPair1(0, at, 1); }   // (an even run behind the warm-up: the period's median is the grid's)
    const LONG slot[1] = { 2000 };
    const LONGLONG kT = MA::UsToTicks(2000);
    const MgPl e1 = MgPair1(1500, slot, 1), e2 = MgPair1(1900, slot, 1), late = MgPair1(2100, slot, 1), plain = MgPair1(0, slot, 1), none = MgPair1(-1, slot, 1), behind = MgPair1(0, slot, 1);
    const MgPl again = MgPair1(0, slot, 1);
    a.k.lateUs = 500;
    const LONG two[2] = { 2000, 2300 }, three[2] = { 2000, 2500 };
    const MgPl al1 = MgPair1(2300, two, 2), al2 = MgPair1(2700, three, 2);
    a.k.lateUs = 0;
    // (N's shift and the one-sided grid, the rig's two placer keys the merged placer carried, went with their keys in round 9's S8)
    MgPair1(0, slot, 1);
    // A LATE ALLOWANCE LONGER THAN HALF THE PERIOD, and A FIRE POINT THAT COMES AFTER THE REAL FRAME'S DUE
    a.k.lateUs = 20000;
    const LONG far2[2] = { 2000, 6000 };
    const MgPl al3 = MgPair1(12000, far2, 2);
    FgDue d3 = {}; const bool has3 = MgDue(false, al3.p, &d3);
    a.k.lateUs = 0;
    MgPair1(0, slot, 1);
    const LONG lf0 = a.w.iWhy[FG_AW_LATEFIRE];
    const MgPl lf = MgPair1(0, slot, 1, false, true);
    const LONG lfN = a.w.iWhy[FG_AW_LATEFIRE] - lf0;
    const MgPl lfNext = MgPair1(0, slot, 1);
    const MgPl* all[9] = { &e1, &e2, &late, &plain, &none, &behind, &again, &al1, &al2 };
    const LONGLONG hT = e1.tN - e1.tI;
    int dueBad = 0, stepBad = 0, realBad = 0, earlyBad = 0;
    for (int i = 0; i < 9; i++)
    {
        const MgPl& g = *all[i];
        if (!g.timed || g.own || !g.hasN || g.dn.tDue - g.tS != kT + hT || g.tI - g.tS != kT) dueBad++;
        if (i && all[i]->dn.tDue - all[i - 1]->dn.tDue != MA::UsToTicks(kMgPeriodUs)) stepBad++;
        if (g.real != 1) realBad++;
        if (g.early || g.nEarly) earlyBad++;
    }
    printf("  planned 2,000 us, half the period %.3f ms. The answer at 1,500 us: raised before the slot %s, at the slot verdict %ld due %lld ticks after the handoff (%lld the slot), shown %d; at 1,900: verdict %ld, shown %d; at 2,100: verdict %ld why %ld, shown %d, the pad %+ld\n",
        MA::TicksToMs(hT), e1.early ? "YES" : "no", e1.di.verdict, (long long)(e1.di.tDue - e1.tS), (long long)kT, e1.gen, e2.di.verdict, e2.gen, late.di.verdict, MgWhyOf(late.di), late.gen, late.pad);
    printf("  \"no frame\": verdict %ld why %ld, shown %d, the pad %+ld; the pair behind it (its dispatch resets): a due for its generated frame %s, shown %d; with a late allowance of 500 us: the answer at 2,300: a due at the slot %d, verdict %ld at 2,300, due %lld ticks after the handoff, shown %d; at 2,700: a due at the slot %d, verdict %ld why %ld at 2,500, shown %d\n",
        none.di.verdict, MgWhyOf(none.di), none.gen, none.pad, behind.hasI ? "RAISED" : "none", behind.gen, al1.hasIAt[0], al1.verdictAt[1], (long long)(al1.di.tDue - al1.tS), al1.gen, al2.hasIAt[0], al2.verdictAt[1], MgWhyOf(al2.di), al2.gen);
    printf("  over the nine pairs: the real frame's due is not the handoff plus the planned time plus half the period on %d; one real frame's due to the next is not the period on %d; a due raised before its instant on %d; a real frame not shown exactly once on %d\n", dueBad, stepBad, earlyBad, realBad);
    if (e1.early || e1.di.verdict != FG_DUE_SHOW || e1.di.tDue - e1.tS != kT || e1.gen != 1 || e2.di.verdict != FG_DUE_SHOW || e2.gen != 1 || e1.pad || e2.pad)
        Err("P1: an answer before the slot: raised early %d, verdict %ld (SHOW %d wanted), due %lld ticks after the handoff (%lld wanted), shown %d and %d, the pad %+ld %+ld", (int)e1.early, e1.di.verdict, (int)FG_DUE_SHOW, (long long)(e1.di.tDue - e1.tS), (long long)kT, e1.gen, e2.gen, e1.pad, e2.pad);
    if (late.di.verdict != FG_DUE_WITHHOLD || MgWhyOf(late.di) != FG_AW_LATE || late.gen != 0 || late.pad != 1) Err("P1: an answer 0.1 ms after the slot: verdict %ld why %ld, shown %d, the pad %+ld (WITHHOLD, LATE, 0 and +1 wanted)", late.di.verdict, MgWhyOf(late.di), late.gen, late.pad);
    if (none.di.verdict != FG_DUE_WITHHOLD || MgWhyOf(none.di) != FG_AW_NOFRAME || none.gen != 0 || none.pad != 0) Err("P1: \"no frame\": verdict %ld why %ld, shown %d, the pad %+ld (WITHHOLD, no frame, 0 and 0 wanted)", none.di.verdict, MgWhyOf(none.di), none.gen, none.pad);
    if (behind.hasI || behind.gen != 0 || again.gen != 1) Err("P1: the pair behind a dispatch with no frame had a due raised for its generated frame (%d) and showed %d; the pair after it showed %d (none, 0 and 1 wanted)", (int)behind.hasI, behind.gen, again.gen);
    if (al1.hasIAt[0] || al1.verdictAt[1] != FG_DUE_SHOW || al1.di.tDue - al1.tS != MA::UsToTicks(2300) || al1.gen != 1) Err("P1: inside the late allowance: a due at the slot %d, verdict %ld at the answer, due %lld ticks after the handoff, shown %d (none, SHOW, %lld and 1 wanted)", al1.hasIAt[0], al1.verdictAt[1], (long long)(al1.di.tDue - al1.tS), al1.gen, (long long)MA::UsToTicks(2300));
    if (al2.hasIAt[0] || al2.verdictAt[1] != FG_DUE_WITHHOLD || MgWhyOf(al2.di) != FG_AW_LATE || al2.gen != 0) Err("P1: past the late allowance: a due at the slot %d, verdict %ld why %ld at its end, shown %d (none, WITHHOLD, LATE and 0 wanted)", al2.hasIAt[0], al2.verdictAt[1], MgWhyOf(al2.di), al2.gen);
    if (dueBad || stepBad || earlyBad || realBad) Err("P1: over the nine pairs %d real frames' dues are not where the handoff puts them, %d steps are not the period, %d dues were raised early, %d real frames were not shown once", dueBad, stepBad, earlyBad, realBad);
    printf("  a late allowance of 20,000 us, the answer at 12,000: a due at the slot %d, at 6,000 us %d; at the real frame's raise: verdict %ld why %ld, the real frame's due %lld ticks after the handoff (%lld wanted), raised early %s, shown %d and %d\n",
        al3.hasIAt[0], al3.hasIAt[1], has3 ? d3.verdict : -1, has3 ? MgWhyOf(d3) : -1, (long long)(al3.dn.tDue - al3.tS), (long long)(kT + hT), al3.nEarly ? "YES" : "no", al3.gen, al3.real);
    if (al3.hasIAt[0] || al3.hasIAt[1] || !has3 || d3.verdict != FG_DUE_WITHHOLD || MgWhyOf(d3) != FG_AW_LATE || !al3.hasN || al3.nEarly || al3.dn.tDue - al3.tS != kT + hT || al3.gen != 0 || al3.real != 1)
        Err("P1: with a late allowance longer than half the period: a due at the slot %d and at 6,000 us %d, at the real frame's raise a verdict %d (%ld why %ld), the real frame's due raised %d, early %d, %lld ticks after the handoff, shown %d and %d (none, none, WITHHOLD and LATE, yes, no, %lld, 0 and 1 wanted): the real frame waited for the answer",
            al3.hasIAt[0], al3.hasIAt[1], (int)has3, d3.verdict, MgWhyOf(d3), (int)al3.hasN, (int)al3.nEarly, (long long)(al3.dn.tDue - al3.tS), al3.gen, al3.real, (long long)(kT + hT));
    printf("  no fire point between the slot and the real frame's due: the placer's verdict %ld, the generated frame shown %d time(s), counted not shown for a late fire point %ld, the real frame %d, its due %lld ticks after the handoff; the pair behind it: %d and %d\n",
        lf.di.verdict, lf.gen, lfN, lf.real, (long long)(lf.dn.tDue - lf.tS), lfNext.gen, lfNext.real);
    if (!lf.hasI || lf.di.verdict != FG_DUE_SHOW || lf.gen != 0 || lfN != 1 || lf.real != 1 || lf.dn.tDue - lf.tS != kT + hT || lfNext.gen != 1 || lfNext.real != 1)
        Err("P1: a fire point that came after the real frame's due: the placer's verdict %ld (SHOW wanted), the generated frame shown %d time(s), counted not shown for a late fire point %ld, the real frame %d, its due %lld ticks after the handoff; the pair behind it %d and %d (0, 1, 1, %lld, 1 and 1 wanted)",
            lf.di.verdict, lf.gen, lfN, lf.real, (long long)(lf.dn.tDue - lf.tS), lfNext.gen, lfNext.real, (long long)(kT + hT));
    // CONTROL
    a.k.kPlanUs = 2600;
    const LONG slot2[1] = { 2600 };
    const MgPl c = MgPair1(2100, slot2, 1);
    a.k.kPlanUs = 2000;
    MgPair1(0, slot, 1);
    MgClean("P1");
    MgEnd();
    if (c.di.verdict != FG_DUE_SHOW || c.gen != 1 || c.tI - c.tS != MA::UsToTicks(2600)) Err("P1 control: with 2,600 us handed in the answer at 2,100 gave verdict %ld and showed %d (SHOW and 1 wanted): the control shows nothing", c.di.verdict, c.gen);
    Ctl("the same answer, at 2,100 us, with a planned time of 2,600 us handed in: verdict SHOW (%ld), shown %d: the verdict is the number handed in and the fence, nothing else", c.di.verdict, c.gen);
    CaseEnd("P1 the placer on real answers: one planned slot a pair, the answer read from the fence by equality (stage 7)");
}

// ---------------------------------------------------------------- P2, the real frame's due does not move with the answer
// WHAT IT SHOWS (critic X1; check-machine.md, R1; the critic's section 5: "the real frame's due does not move with the
// answer: late, missing or withheld"; the build map's machine-2 a). Seven pairs in an even run, the planned time 2,000 us:
// the answer at 500 us, at 6,000 us, NEVER (the helper held in the dispatch), "no frame", a camera cut (fed, never
// shown), an unsent frame's neighbour, and a pair behind TEN BUILDS (the hand's hold field carries the build generation,
// 11 by then: until stage 7 the rig's placer would have added it to the due, microseconds that grow with every build).
// FOR EVERY ONE THE REAL FRAME'S DUE IS THE HANDOFF PLUS 2,000 us PLUS HALF THE PERIOD, the same ticks.
// THE ORACLE, while it lives: the rig's own placer, over the rig's binding, with its modelled answer at 2,000 us (no service
// time on the fake clock, so its generated frame is due there, as a FIXED K of 2,000 us put it until the rig's fixed K went
// in round 9's S9) and the same even frames: its generated frame's due and its real frame's due sit the same ticks behind the
// handoff as the merged placer's.
// CONTROL (a run): the rig's placer at READINESS, the mode every screen leg of phase 1 ran, with a modelled answer of
// 500 us and then of 6,000 us (the harness's FG_STUB_GEN_US, the key fg_test_gen_us until round 9's S9): its real frame's due
// moves by the difference, 5.5 ms. That arithmetic on the real wire would hold the real frame behind the helper.
struct P2Rig { LONGLONG dI, dN; bool ok; };
static P2Rig P2RigRun(int gen)
{
    P2Rig r = {};
    Opt o; o.fakeClock = true;
    Fresh(o); gStubGenUs = gen;
    FgState& s = B::S();
    const LONGLONG t0 = B::clockNow, per = M::UsToTicks(kMgPeriodUs);
    for (int i = 1; i <= 10; i++)
    {
        B::clockNow = t0 + per * i;
        Frame(); Drain(); StampNow(); PlaceNow();
        ClockAdd((double)gen / 1000.0 + 0.1); PlaceNow();
    }
    FgPlan pl = {};
    r.ok = M::Read(s.plan[s.r.lastP & (FG_RING - 1)], s.r.lastP, &pl);
    r.dI = pl.tI - pl.tS; r.dN = pl.tN - pl.tS;
    return r;
}
static void CaseP2()
{
    CaseBegin("P2 the real frame's due does not move with the answer: late, missing or withheld (stage 7)");
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000; a.k.lateUs = 0;
    ShWarm<MA>(); MgDueN();
    MgEvenStart();
    const LONG slot[1] = { 2000 };
    for (int i = 0; i < 4; i++) MgPair1(0, slot, 1);
    const LONGLONG kT = MA::UsToTicks(2000);
    const MgPl early = MgPair1(500, slot, 1), vlate = MgPair1(6000, slot, 1);
    MgPair1(0, slot, 1);                                      // (behind the very late one: its hand found the answer in by then, a plain pair)
    // NEVER: the helper held inside this pair's dispatch through the pair's whole life
    _InterlockedExchange(&gH.holdDispatch, 1);
    const MgPl never = MgPair1(0, slot, 1, true);
    const bool heldThrough = gH.inDispatch != 0;
    ShRelease();
    if (!ShSettle()) Err("P2: the fake helper did not come back after its hold");
    MgPair1(0, slot, 1);
    const MgPl noframe = MgPair1(-1, slot, 1);
    MgPair1(0, slot, 1);
    B::camJumps++; const MgPl cut = MgPair1(0, slot, 1);
    const MgPl plain = MgPair1(0, slot, 1);
    // TEN BUILDS, then a pair
    for (int i = 0; i < 10; i++) { MA::BeforeBuild(); Drain(); MA::BeforeRelease(); ShB::WriteB(); }
    if (!ShSettle()) Err("P2: the helper did not read the builds");
    MgPair1(0, slot, 1);
    const MgPl built = MgPair1(0, slot, 1);
    const LONG bgen = x.sh.bgen;
    const MgPl* all[7] = { &early, &vlate, &never, &noframe, &cut, &plain, &built };
    static const char* const kName[7] = { "the answer at 500 us", "at 6,000 us", "never", "\"no frame\"", "a camera cut", "a plain pair", "behind ten builds" };
    const LONGLONG hT = plain.tN - plain.tI;
    int bad = 0; std::string first, line;
    for (int i = 0; i < 7; i++)
    {
        const MgPl& g = *all[i];
        const LONGLONG d = g.hasN ? g.dn.tDue - g.tS : -1;
        line += Fmt("%s%s %lld", i ? "; " : "", kName[i], (long long)d);
        if (!g.timed || d != kT + hT || g.real != 1) { bad++; if (first.empty()) first = Fmt("%s: the due %lld ticks after the handoff (%lld wanted), the real frame shown %d time(s)", kName[i], (long long)d, (long long)(kT + hT), g.real); }
    }
    printf("  the real frame's due, in ticks after its handoff (the planned 2,000 us is %lld, half the period %lld): %s; generated frames shown: %d %d %d %d %d %d %d; the helper held through the pair that never had an answer: %s; the build generation by the last pair: %ld\n",
        (long long)kT, (long long)hT, line.c_str(), early.gen, vlate.gen, never.gen, noframe.gen, cut.gen, plain.gen, built.gen, heldThrough ? "yes" : "NO", bgen);
    if (bad) Err("P2: %d of seven pairs' real frames are not due where their handoff puts them; the first: %s", bad, first.c_str());
    if (early.gen != 1 || vlate.gen != 0 || never.gen != 0 || noframe.gen != 0 || cut.gen != 0 || plain.gen != 1 || !heldThrough || bgen != 11)
        Err("P2: the generated frames shown were %d %d %d %d %d %d %d (1 0 0 0 0 1 and any wanted), the helper held %d, the build generation %ld (11 wanted)", early.gen, vlate.gen, never.gen, noframe.gen, cut.gen, plain.gen, built.gen, (int)heldThrough, bgen);
    MgClean("P2");
    MgEnd();
    // THE ORACLE AND THE CONTROL: the rig's own placer
    const P2Rig fix = P2RigRun(2000), r1 = P2RigRun(500), r2 = P2RigRun(6000);
    printf("  the rig's placer, its answer modelled at 2,000 us on the same even frames: its generated frame due %lld and its real frame %lld ticks after the handoff (the merged placer: %lld and %lld)\n", (long long)fix.dI, (long long)fix.dN, (long long)kT, (long long)(kT + hT));
    if (!fix.ok || fix.dI != kT || fix.dN != kT + hT) Err("P2: the oracle disagrees: the rig's placer with its answer modelled at 2,000 us puts the dues %lld and %lld ticks after the handoff, the merged placer %lld and %lld", (long long)fix.dI, (long long)fix.dN, (long long)kT, (long long)(kT + hT));
    const LONGLONG moved = r2.dN - r1.dN;
    if (!r1.ok || !r2.ok || moved != M::UsToTicks(5500)) Err("P2 control: the rig's placer at readiness moved the real frame's due by %lld ticks between a modelled answer of 500 us and one of 6,000 us (%lld wanted): the control shows nothing", (long long)moved, (long long)M::UsToTicks(5500));
    Ctl("the rig's placer at readiness, a modelled answer of 500 us against 6,000 us: the real frame's due moved by %.3f ms with the answer", M::TicksToMs(moved));
    CaseEnd("P2 the real frame's due does not move with the answer: late, missing or withheld (stage 7)");
}

// ---------------------------------------------------------------- P3, no pair is judged by which stamper ran first
// WHAT IT SHOWS (critic X2; check-machine.md, X2; READY-FOR-7.md, section 1, item 3, and section 6, point 9: the fake
// helper answers in microseconds, so the answer's stamper can beat the handoff's, and the shadow shape's placer then
// took no generation time for the pair). The stampers are stepped by hand, so each leg says exactly which stamps exist:
//   NO STAMP OF THE ANSWER     the answer is in and its stamper never ran: the pair is SHOWN at its slot all the same (the
//                              verdict is the fence's value), and K's histogram, which is fed by the stamps, took nothing
//   THE ANSWER STAMPED, THE HANDOFF NOT   the stampers in the "wrong" order: the fence says the handoff retired, so the pair
//                              is timed from the placer's own clock, counted, SHOWN at its slot, and kept out of K
//   THE HANDOFF NEVER RETIRES  (its submit signals nothing): half a period after the arm the pair is timed FROM THE ARM,
//                              counted; its generated frame is withheld and ITS REAL FRAME'S DUE IS RAISED: not stranded
//   n IS NOT p                 behind a frame that was not sent the pair's ordinal is one past its number on the wire: it is
//                              timed by the stamp of n; the stamp ring holds nothing at p
// CONTROLS (what the two rules this replaces would have done, read from the same run): a verdict by the stamp would
// have had none to read in the first leg (the ring holds no stamp of 2n); a placer that keys the handoff by p would
// never have timed the last pair (no stamp at p, and none can come before the next pair's).
static void CaseP3()
{
    CaseBegin("P3 no pair is judged, timed or dropped by which stamper ran first (stage 7)");
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000;
    ShWarm<MA>(); MgDueN();
    MgEvenStart();
    const LONG slot[1] = { 2000 };
    for (int i = 0; i < 4; i++) MgPair1(0, slot, 1);
    const LONGLONG kT = MA::UsToTicks(2000);
    FgDue d = {}; LONGLONG ts = 0;
    // (the steps of one pair by hand: the placer alone, with the stamps this leg allows)
    auto pair = [&](int stampFirst, bool noSignal, MgPl& g, bool* stampOut, bool* stampInAtP)
    {
        g = MgPl();
        const size_t sh = MgShownN();
        ShArmB::noSignal = noSignal ? 1 : 0;
        g.tag = MgEven<MA>(true, !noSignal); ShArmB::noSignal = 0;
        g.p = MgP(); g.n = x.w.lastQN;
        const LONGLONG t0 = B::clockNow;
        if (noSignal) { const LONG st0 = gH.stuck; if (!WaitFor([st0] { return gH.stuck > st0; }, 3000)) Err("P3: the helper did not give up on a handoff that never retired"); }
        if (stampFirst >= 0) MgStampOnly<MA>(stampFirst);
        MA::PlacerStep(B::Now());
        const FgArmedLive* L = MgLive(g.p);
        g.timed = L && L->timed; g.own = L && L->tsOwn;
        if (!g.timed) { B::clockNow = t0 + (LONGLONG)(MA::HalfOf((double)kMgPeriodUs / 1000.0) * (double)B::Qpf() / 1000.0); MA::PlacerStep(B::Now()); L = MgLive(g.p); g.timed = L && L->timed; g.own = L && L->tsOwn; }
        if (L) { g.tS = L->tS; g.tI = L->tI; g.tN = L->tN; }
        if (g.tI > B::clockNow) B::clockNow = g.tI;
        MA::PlacerStep(B::Now());
        g.hasI = MgDue(false, g.p, &g.di);
        if (stampOut) *stampOut = MA::ReadStamp(s.st[FG_TL_OUT], 2 * g.n, &ts);
        if (stampInAtP) *stampInAtP = MA::ReadStamp(s.st[FG_TL_IN], g.p, &ts);
        FireNote(); Drain();
        if (g.tN > B::clockNow) B::clockNow = g.tN;
        MA::PlacerStep(B::Now());
        g.hasN = MgDue(true, g.p, &g.dn);
        FireNote(); Drain();
        const auto shown = Shown();
        for (size_t i = sh; i < shown.size(); i++) { g.gen += shown[i].tag == ShGenTag(g.tag, false); g.real += shown[i].tag == g.tag; }
        // (the handoff's, the prepare's and the frame end's stamps are caught up before the next leg. THE ANSWER'S IS NOT: a
        // stamper stamps the value it FINDS, so a value it did not look at in time has no stamp for ever, which is the
        // very thing these legs are about)
        MgStampOnly<MA>(FG_TL_IN); MgStampOnly<MA>(FG_TL_PREP); MgStampOnly<MA>(FG_TL_END);
    };
    MgPl noOut, outFirst, never, behindUnsent; bool outStamped = true, inAtP = true;
    const LONG kN0 = x.pl.kN, own0 = a.pl.tsOwn, nev0 = a.pl.tsNever;
    pair(FG_TL_IN, false, noOut, &outStamped, nullptr);
    const LONG kNoOut = x.pl.kN - kN0;
    pair(FG_TL_OUT, false, outFirst, nullptr, nullptr);
    const LONG ownOutFirst = a.pl.tsOwn - own0, kOutFirst = x.pl.kN - kN0 - kNoOut;
    pair(-1, true, never, nullptr, nullptr);
    const LONG nev = a.pl.tsNever - nev0;
    MgPair1(0, slot, 1); MgPair1(0, slot, 1);                 // (the pair behind it resets; then a plain one)
    MgEven<MA>(false);                                        // a frame that was not sent: p moves, n does not
    MgPair1(0, slot, 1);                                      // (the first pair behind it says GAP: fed, not shown)
    pair(FG_TL_IN, false, behindUnsent, nullptr, &inAtP);
    printf("  no stamp of the answer (the ring holds one of 2n: %s): timed %s, verdict %ld, shown %d, K taken %ld; the answer stamped and the handoff not: timed by the placer's own clock %ld, verdict %ld, shown %d, K taken %ld\n",
        outStamped ? "YES" : "no", noOut.timed ? "yes" : "NO", noOut.di.verdict, noOut.gen, kNoOut, ownOutFirst, outFirst.di.verdict, outFirst.gen, kOutFirst);
    printf("  the handoff never retires: timed from the arm %ld, verdict %ld, shown %d, the real frame's due raised %s and shown %d; behind an unsent frame: pair %ld is number %ld on the wire, timed %s (its own clock: %s), a stamp at p in the ring: %s, shown %d and %d\n",
        nev, never.di.verdict, never.gen, never.hasN ? "yes" : "NO", never.real, behindUnsent.p, behindUnsent.n, behindUnsent.timed ? "yes" : "NO", behindUnsent.own ? "YES" : "no", inAtP ? "YES" : "no", behindUnsent.gen, behindUnsent.real);
    if (outStamped || !noOut.timed || noOut.own || noOut.di.verdict != FG_DUE_SHOW || noOut.gen != 1 || noOut.real != 1 || kNoOut != 0 || noOut.tI - noOut.tS != kT)
        Err("P3: with no stamp of the answer the pair was timed %d, verdict %ld, shown %d (timed by its stamp, SHOW and 1 wanted), K taken %ld (0 wanted: K is the stamps')", (int)noOut.timed, noOut.di.verdict, noOut.gen, kNoOut);
    if (ownOutFirst != 1 || !outFirst.own || outFirst.di.verdict != FG_DUE_SHOW || outFirst.gen != 1 || outFirst.real != 1 || kOutFirst != 0)
        Err("P3: with the answer stamped and the handoff not, the pair was timed by the placer's own clock %ld time(s), verdict %ld, shown %d, K taken %ld (1, SHOW, 1 and 0 wanted)", ownOutFirst, outFirst.di.verdict, outFirst.gen, kOutFirst);
    if (nev != 1 || !never.timed || never.di.verdict != FG_DUE_WITHHOLD || never.gen != 0 || !never.hasN || never.real != 1)
        Err("P3: a handoff that never retires: timed from the arm %ld time(s), verdict %ld, shown %d, the real frame's due raised %d and shown %d (1, WITHHOLD, 0, yes and 1 wanted)", nev, never.di.verdict, never.gen, (int)never.hasN, never.real);
    if (behindUnsent.p == behindUnsent.n || !behindUnsent.timed || behindUnsent.own || inAtP || behindUnsent.gen != 1 || behindUnsent.real != 1)
        Err("P3: behind an unsent frame pair %ld is number %ld (they must differ), timed %d by its own stamp %d, a stamp at p %d, shown %d and %d", behindUnsent.p, behindUnsent.n, (int)behindUnsent.timed, (int)!behindUnsent.own, (int)inAtP, behindUnsent.gen, behindUnsent.real);
    MgClean("P3");
    MgEnd();
    Ctl("read from the same run: a verdict by the stamp had no stamp of 2n to read (in the ring: %s) and K took %ld; a placer that keys the handoff by p had no stamp at p (%s) for pair %ld, number %ld", outStamped ? "YES" : "no", kNoOut, inAtP ? "YES" : "no", behindUnsent.p, behindUnsent.n);
    CaseEnd("P3 no pair is judged, timed or dropped by which stamper ran first (stage 7)");
}

// ---------------------------------------------------------------- P4, the merged machine under its own threads
// WHAT IT SHOWS (READY-FOR-7.md, section 4, "ITS THREADS"). Every other case of this file steps the stampers and the
// placer by hand. Here they are THREADS over the merged machine itself: four stampers (fgIn, fgPrep, fgOut and the frame
// end's, which the merged path owns) and the placer's loop with its timer, as the harness's stand-ins for the product's
// own loops run them (check-machine.md, R12: those loops are compiled by no desk tool until they move behind seams).
// Thirty pairs of an even run: the placer's THREAD raises each pair's two dues when the harness's clock reaches them,
// every generated frame is shown and then its real frame, and every frame's end is stamped.
// CONTROL: the same run with the placer's thread not started: no due is ever raised, no generated frame is shown, and
// every real frame goes out forced, at the next injection.
static void MgStartThreads(bool placer)
{
    gThrQuit = 0; ResetEvent(gThrQuitEv); gThrN = 0; gShUp = 0;
    // (three of the wire's stampers are the harness's own loop over the machine's StampStep, which is the same over every
    // binding on the wire but for one count; the frame end's and the placer are the merged machine's own)
    gThr[gThrN++] = Spawn(&ShStamperMain<MA>, (void*)(INT_PTR)FG_TL_IN);
    gThr[gThrN++] = Spawn(&ShStamperMain<MA>, (void*)(INT_PTR)FG_TL_PREP);
    gThr[gThrN++] = Spawn(&ShStamperMain<MA>, (void*)(INT_PTR)FG_TL_OUT);
    gThr[gThrN++] = Spawn(&StamperMain<MA>, (void*)(INT_PTR)FG_TL_END);
    if (placer) gThr[gThrN++] = Spawn(&PlacerMain<MA>);
    if (!WaitFor([] { return gShUp == 3; }, 5000)) Err("the merged machine's stampers did not start within 5 s");
}
struct P4Leg { int gen, real, order; LONG iShow, nRaised, forced, ends; };
static P4Leg P4Run(bool placer)
{
    P4Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000;
    ShWarm<MA>(); MgDueN();
    MgStartThreads(placer);
    MgEvenStart();
    const size_t sh = MgShownN();
    const LONG i0 = a.pl.iShow, n0 = a.pl.nRaised, f0 = a.w.iWhy[FG_AW_FORCED];
    std::vector<DWORD> tags; std::vector<LONG> ends;
    for (int i = 0; i < 30; i++)
    {
        const LONG timed0 = a.pl.nTimed;
        const DWORD t = MgEven<MA>(); tags.push_back(t); ends.push_back(B::frames - 1);
        const LONG p = MgP(); const LONGLONG t0 = B::clockNow;
        FgDue d = {};
        // (the pair is timed by the placer's thread while the clock still stands at the handoff: whichever of the handoff's
        // stamper and the placer runs first, the handoff's time is this instant)
        if (placer && !WaitFor([&] { SetEvent(B::evPlacer); return a.pl.nTimed > timed0; }, 3000)) { Err("P4: the placer's thread did not time pair %ld", p); break; }
        B::clockNow = t0 + MA::UsToTicks(2100); SetEvent(B::evPlacer);
        if (placer) { if (!WaitFor([&] { return MgDue(false, p, &d); }, 3000)) { Err("P4: the placer's thread raised no due for pair %ld's generated frame", p); break; } }
        else Sleep(2);
        FireNote(); Drain();
        B::clockNow = t0 + MA::UsToTicks(2100 + kMgPeriodUs / 2); SetEvent(B::evPlacer);
        if (placer) { if (!WaitFor([&] { return MgDue(true, p, &d); }, 3000)) { Err("P4: the placer's thread raised no due for pair %ld's real frame", p); break; } }
        else Sleep(2);
        FireNote(); Drain();
    }
    ShFrame<MA>(); MgDueN();
    const auto shown = Shown();
    size_t at = sh;
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (at < shown.size() && shown[at].tag == ShGenTag(tags[i], false)) { g.gen++; at++; }
        if (at < shown.size() && shown[at].tag == tags[i]) { g.real++; at++; }
    }
    g.order = (int)(shown.size() - at);
    g.iShow = a.pl.iShow - i0; g.nRaised = a.pl.nRaised - n0; g.forced = a.w.iWhy[FG_AW_FORCED] - f0;
    LONGLONG te = 0;
    WaitFor([&] { return MA::EndFor(ends.back(), &te); }, 1000);
    for (LONG f : ends) g.ends += MA::EndFor(f, &te) ? 1 : 0;
    StopThreads();
    MgClean("P4");
    MgEnd();
    return g;
}
static void CaseP4()
{
    CaseBegin("P4 the merged machine under its own threads: four stampers and the placer (stage 7)");
    const P4Leg g = P4Run(true);
    printf("  thirty pairs, the stampers and the placer as threads: the placer raised %ld generated frames' dues and %ld real frames'; on the screen %d generated frames and %d real ones, %d picture(s) out of place; frame ends stamped %ld of 30; real frames forced first %ld\n",
        g.iShow, g.nRaised, g.gen, g.real, g.order - 1, g.ends, g.forced);
    if (g.iShow != 30 || g.nRaised != 30 || g.gen != 30 || g.real != 30 || g.order != 1 || g.forced != 0) Err("P4: the placer's thread raised %ld and %ld dues, and the screen showed %d generated and %d real frames with %d other picture(s) (30, 30, 30, 30 and the one closing frame wanted); forced %ld", g.iShow, g.nRaised, g.gen, g.real, g.order, g.forced);
    // (a stamper stamps the value it finds: a frame end it did not look at in time has no stamp, so not every one need be there)
    if (g.ends < 20) Err("P4: %ld of 30 frame ends were stamped by the frame end's stamper", g.ends);
    const RingStats stc = gRingSt;
    const P4Leg c = P4Run(false);
    ViolRestore(stc);
    if (c.iShow != 0 || c.gen != 0 || c.real != 30 || c.forced < 29) Err("P4 control: with no placer thread %ld dues were raised, %d generated frames shown, %d real frames, %ld forced first (0, 0, 30 and at least 29 wanted): the control shows nothing", c.iShow, c.gen, c.real, c.forced);
    Ctl("the placer's thread not started: %ld dues raised, %d generated frames shown, every real frame forced out at the next injection (%ld), %d shown", c.iShow, c.gen, c.forced, c.real);
    CaseEnd("P4 the merged machine under its own threads: four stampers and the placer (stage 7)");
}

// ---------------------------------------------------------------- MC, the census: 10,000 random frames, the screen read as a whole
// WHAT IT SHOWS (the critic's section 5, first line: "over 10,000 random frames every presented picture is a real frame,
// once and in order, or the generated frame of its own pair. Never black, never the sentinel, never an older one";
// machine.md's machine-15: "one that judges the screen as a whole"). Ten segments of a thousand frames, each on a fresh
// machine, at ONE, TWO, THREE AND FOUR images in the ring, in the three modes of the fake's command stream, with the
// back buffer's save answered "always" and by what the fake knows, with and without a late allowance. EVERY FRAME BY
// CHANCE: sent or not; engaged or standby; rule C1 or C3; a camera cut; the HDR view; the helper's answer at once, early,
// late by up to 3 ms, "no frame", or HELD inside its dispatch for up to three frames; a build; the game drawing into
// the back buffer before a fire, between the fires, or not at all; a command buffer that does not retire; a submit the
// queue refuses (a fire's write, a hand's span); the placer stepped at the slot, or OVERRULED by a due that
// says SHOW whatever the answer; each fire's note recorded or left to the next frame's injection, drain or hand. In the
// last segment the helper is GONE for the last sixty frames. AND IN THE SEGMENT BEFORE IT THE QUEUE REFUSES A FIRE'S
// RESTORE, two hundred frames before its end (stage 7c): the machine stands down for the session there, every frame
// behind it is passed through, and the account must still close.
// THE WITNESS reads the whole screen: every picture is a frame of the run, in the order the frames were made and at most
// once, or a generated image whose mark names a frame not yet shown, never one of a reset dispatch, and whose own real
// frame is the next real frame on the screen (or was dropped and counted); never black, never a sentinel, never the
// game's half-drawn frame, never a picture of no frame. EVERY FRAME IS ACCOUNTED FOR: shown exactly once, or dropped
// and counted by the machine with its reason, and the two counts are equal. THE MIRROR: for every dispatch the fake
// helper made, "it reset" by the helper's own record equals "fed, never shown for a reset" as the worker decided it.
// And the fake's eleven violation counts are 0.
// CONTROL: six hundred frames of the same over a binding whose read of the answer on the worker says yes (the read by
// equality switched off): where a due was planted for a pair with no frame of its own, another pair's generated
// image goes to the screen: the witness counts it, and so does the fake.
struct McTot
{
    LONG frames = 0, armed = 0, real = 0, gen = 0, never = 0, dropped = 0, planted = 0, builds = 0, mirror = 0, mirrorN = 0;
    LONG why[FG_AW_N] = {}, inl[FG_AI_N] = {}, drop[FG_AD_N] = {}, saves = 0, noSaves = 0, restoreLost = 0, guardForced = 0, guardFlushes = 0, late = 0, noFrame = 0, held = 0;
    int bad = 0; std::string first;
    LONG hudIn = 0, hudInOwn = 0, hudInBad = 0, hudLost = 0; std::string hudFirst;   // (HU10: every dispatch's HUD-less input, read by the fake helper)
};
// (lostAt: from this frame of the segment on, the first fire that saves has its RESTORE refused, once; -1 never)
// (gMcHud, stage 15, dt_fg_hud.h's HU6: 1 runs the segment WITH THE HUD-LESS PICTURE: a set that has the image, what each
// frame has for a picture drawn by chance, and by chance a frame whose spans meet no store, no command buffer or a
// refused submit. 0, the census as it was: not one draw of its random stream is added or moved.)
// (gMcHudGuardOff: that census's control switches one of the machine's guards off for a segment, FG_ARMED_GUARD_ON's bits)
// (gMcHud 2, 2026-10-11, HU10: the same in THE SECOND-TARGET FORM: the picture in the binding's own image where 1 draws it
// into the back buffer, and by chance an image the hand's span cannot name. gMcHudWrong: that census's controls, the
// binding's hudWrong for a segment)
static LONG gMcHud = 0, gMcHudGuardOff = 0, gMcHudWrong = 0;
template <class MM> static void McSegment(int seg, int nFrames, int images, int cs, int backMode, bool lie, int goneAt, int lostAt, McTot& t)
{
    MgFresh<MM>(images);
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    if (gMcHud) { ShB::hasHudless = 1; gArmedGuardOff = gMcHudGuardOff; ShArmB::hudWrong = gMcHudWrong; }
    a.k.kPlanUs = 2000; a.k.lateUs = (seg & 1) ? 300 : 0;
    ShArmB::backMode = backMode; gRing.presentCs = cs;
    std::vector<DWORD> frameTag; std::map<DWORD, int> index; std::set<DWORD> half; std::map<LONG, LONG> whyOfN, prevOfN;
    std::map<DWORD, DWORD> hudPic; std::set<DWORD> hudFed, hudLost;   // (the second-target form, HU10: by the frame's tag)
    const size_t shown0 = MgShownN();
    const LONGLONG kT = MA::UsToTicks(2000);
    int standby = 0; LONG heldFor = 0; bool gone = false, broke = false;
    auto fail = [&](const char* what, DWORD tag, size_t i) { t.bad++; if (t.first.empty()) t.first = Fmt("segment %d (%d image(s), stream mode %d): picture %zu: %s (%lX)", seg, images, cs, i, what, tag); };
    MgEvenStart();
    for (int f = 0; f < nFrames && !broke; f++)
    {
        if (!standby && Rnd() % 211 == 0) standby = 1 + (int)(Rnd() % 4);
        s.wt.want = standby ? FG_WANT_PASS : FG_WANT_ON; if (standby) standby--;
        gC1Off = Rnd() % 5 == 0 ? 1 : 0;
        if (Rnd() % 53 == 0) B::camJumps++;
        B::hdrView = Rnd() % 97 == 0;
        LONG answerUs = 0; bool noFrame = false;
        if (goneAt >= 0 && f >= goneAt && !gone && !heldFor)
        {
            ShHelperGone(); gone = true;
            if (!WaitFor([] { return B::TlValue(FG_TL_OUT) == ~0ull; }, 3000)) Err("MC: a helper that is gone did not leave its fences at the top");
        }
        if (!heldFor && !gone)
        {
            const unsigned h = Rnd() % 100;
            if (h < 8) { answerUs = 2100 + (LONG)(Rnd() % 3000); t.late++; }
            else if (h < 11) { noFrame = true; t.noFrame++; }
            else if (h < 14) { _InterlockedExchange(&gH.holdDispatch, 1); heldFor = 1 + (LONG)(Rnd() % 3); t.held++; }
            else if (h < 22) answerUs = 200 + (LONG)(Rnd() % 1700);
            if (Rnd() % 307 == 0) { MM::BeforeBuild(); Drain(); MM::BeforeRelease(); ShB::WriteB(); t.builds++; }
        }
        _InterlockedExchange(&gH.delayUs, answerUs); _InterlockedExchange(&gH.noFrame, noFrame ? 1 : 0);
        const bool sent = Rnd() % 41 != 0;
        if (gMcHud)
        {   // what this frame has for a HUD-less picture, and by chance a fault in the spans of its hand and of its HUD-less note
            const unsigned h = Rnd() % 16;
            ShArmB::hud = h < 11 ? (gMcHud == 2 ? FG_HUD_TEX : FG_HUD_PASS) : h < 13 ? FG_HUD_SAME : h < 15 ? FG_HUD_NONE : FG_HUD_WIDE;
            // (only where no pair is still armed: the fault is then this frame's own spans', not a fire's of the pair before, whose
            // lost restore would stand the machine down for the rest of the segment)
            const unsigned fault = Rnd() % 61;
            const unsigned skip = Rnd() % 3;
            if (s.w.openPhase != FG_PAIR_ARMED)
            {
                if (fault == 0) ShArmB::noHold = 1;
                else if (fault == 1) _InterlockedExchange(&gRing.cbFailNext, 1);
                else if (fault == 2 || fault == 3) { _InterlockedExchange(&gRing.submitFailSkip, (LONG)skip); _InterlockedExchange(&gRing.submitFailNext, 1); }
                // (the second-target form, HU10: by chance the binding's own image cannot be named at the hand's span. No draw of
                // the random stream is added for it: two more values of the one above)
                else if (gMcHud == 2 && (fault == 4 || fault == 5)) ShArmB::hudSrcGone = 1;
            }
        }
        const LONG sec0 = ShArmB::hudSeconds, texFed0 = a.w.hudTexFed, texLost0 = a.w.hudTexLost;
        const DWORD tag = MgEven<MM>(sent, false);
        if (gMcHud) { ShArmB::noHold = 0; ShArmB::hudSrcGone = 0; _InterlockedExchange(&gRing.cbFailNext, 0); _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0); }
        // (HU10: the picture this frame's "encode" wrote into the binding's own image, and whether the hand's span handed it on)
        if (ShArmB::hudSeconds != sec0) { hudPic[tag] = ShArmB::hudTag; if (a.w.hudTexFed != texFed0) hudFed.insert(tag); if (a.w.hudTexLost != texLost0) hudLost.insert(tag); }
        index[tag] = (int)frameTag.size(); frameTag.push_back(tag); t.frames++;
        if (!MgQuiet()) { Err("MC: the fake helper did not settle behind frame %d of segment %d", f, seg); broke = true; break; }
        if (s.w.openPhase == FG_PAIR_ARMED)
        {
            t.armed++;
            const LONG p = MgP(), n = x.w.openN;
            FgArmedPair rec = {}; if (MgPair(p, &rec) && rec.kind == (LONG)FG_G_DISPATCH) { whyOfN[rec.n] = rec.why; prevOfN[rec.n] = rec.prev; }
            MgPlace<MM>();
            const FgArmedLive* L = MgLive(p);
            const LONGLONG tS = L && L->timed ? L->tS : 0, tI = L ? L->tI : 0, tN = L ? L->tN : 0;
            auto draw = [&] { const DWORD hf = NewTag(); half.insert(hf); Fill(hf); Drain(); };
            if (Rnd() % 2) draw();                            // the game is already drawing the next frame
            if (Rnd() % 199 == 0) _InterlockedExchange(&gRing.cbFailNext, 1);
            // (a refused submit, by chance: a fire's WRITE, which presents nothing and leaves the machine live. Until stage 7c
            // every other one of these let the first submit through and refused a fire's RESTORE; that now stands the
            // machine down, so it is planted once, by `lostAt`, and the draw that chose between them is still made)
            if (Rnd() % 199 == 0) { (void)Rnd(); _InterlockedExchange(&gRing.submitFailSkip, 0); _InterlockedExchange(&gRing.submitFailNext, 1); }
            if (lostAt >= 0 && f >= lostAt && !a.w.restoreLost) { _InterlockedExchange(&gRing.submitFailSkip, 1); _InterlockedExchange(&gRing.submitFailNext, 1); }
            if (tS)
            {
                const LONGLONG ansT = MA::UsToTicks(answerUs);
                if (answerUs && ansT <= kT) { B::clockNow = tS + ansT; MgAnswered(n); }
                if (tI > B::clockNow) B::clockNow = tI;
                if (Rnd() % 7 == 0) { DueI(p, FG_DUE_SHOW); t.planted++; }   // THE PLACER OVERRULED: SHOW, whatever the answer
                else MgPlace<MM>();
                if (lie) ShArmLieB::lie = 1;
                if (Rnd() % 8) { FireNote(); Drain(); }
                ShArmLieB::lie = 0;
                if (answerUs && ansT > kT) { B::clockNow = tS + ansT; MgAnswered(n); }
                if (Rnd() % 3 == 0) draw();
                if (tN > B::clockNow) B::clockNow = tN;
                MgPlace<MM>();
                if (Rnd() % 6) { FireNote(); Drain(); }
            }
        }
        _InterlockedExchange(&gRing.cbFailNext, 0); _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0);
        if (heldFor && --heldFor == 0)
        {
            _InterlockedExchange(&gH.holdDispatch, 0); ShRelease();
            if (!ShSettle()) { Err("MC: the fake helper did not come back after its hold (segment %d, frame %d)", seg, f); broke = true; }
        }
    }
    // the end of the run: what is held is let go, and one standby frame brings the last pair to its real frame
    _InterlockedExchange(&gH.holdDispatch, 0); _InterlockedExchange(&gH.delayUs, 0); _InterlockedExchange(&gH.noFrame, 0); B::hdrView = false;
    if (heldFor) ShRelease();
    s.wt.want = FG_WANT_PASS;
    const DWORD endTag = MgEven<MM>(true, false);
    index[endTag] = (int)frameTag.size(); frameTag.push_back(endTag); t.frames++;
    if (!gone && !ShSettle()) Err("MC: the fake helper did not settle at the end of segment %d", seg);
    gRing.presentCs = RING_CS_LAZY;
    // THE WITNESS
    const auto shown = Shown();
    std::vector<char> seen(frameTag.size(), 0); std::set<DWORD> genSeen; std::vector<int> orphan;
    int last = -1, pendingGen = -1;
    for (size_t i = shown0; i < shown.size(); i++)
    {
        const DWORD tag = shown[i].tag;
        if (ShIsGen(tag))
        {
            if (ShIsGenReset(tag)) { fail("a reset dispatch's picture", tag, i - shown0); continue; }
            const auto it = index.find(ShGenPicture(tag));
            if (it == index.end()) { fail("a generated image of no frame of this run", tag, i - shown0); continue; }
            if (it->second <= last) { fail("a generated image of a pair whose real frame was already shown, or of an older one", tag, i - shown0); continue; }
            if (!genSeen.insert(tag).second) { fail("a generated image shown twice", tag, i - shown0); continue; }
            if (pendingGen >= 0) orphan.push_back(pendingGen);
            pendingGen = it->second; t.gen++;
        }
        else
        {
            const auto it = index.find(tag);
            if (it == index.end())
            {
                fail(half.count(tag) ? "the game's half-drawn frame" : tag == kBlack ? "black" : (tag & 0xFFFF0000u) == (kSentinel & 0xFFFF0000u) ? "a sentinel" : "a picture that is no frame of this run", tag, i - shown0);
                continue;
            }
            if (it->second <= last) { fail(seen[it->second] ? "a real frame shown twice" : "an older real frame", tag, i - shown0); continue; }
            if (pendingGen >= 0 && it->second < pendingGen) fail("a real frame older than the generated frame shown before it", tag, i - shown0);
            if (pendingGen >= 0 && it->second > pendingGen) orphan.push_back(pendingGen);
            pendingGen = -1;
            last = it->second; seen[last] = 1; t.real++;
        }
    }
    if (pendingGen >= 0) orphan.push_back(pendingGen);
    for (int g : orphan) if (seen[g]) fail("a generated image that was not followed by its own pair's real frame", frameTag[g], 0);
    LONG never = 0; for (char c : seen) never += c ? 0 : 1;
    LONG dropped = 0; for (int i = 0; i < FG_AD_N; i++) { dropped += a.w.dropped[i]; t.drop[i] += a.w.dropped[i]; }
    if (never != dropped) { t.bad++; if (t.first.empty()) t.first = Fmt("segment %d: %ld frame(s) never reached the screen and the machine counts %ld dropped", seg, never, dropped); }
    t.never += never; t.dropped += dropped;
    // THE MIRROR
    for (const ShDisp& d : ShDispLog())
    {
        const auto it = whyOfN.find(d.n);
        if (it == whyOfN.end()) continue;
        t.mirrorN++;
        const bool mach = it->second == FG_AW_RESET;
        if (mach != d.reset) t.mirror++;
    }
    // THE HUD-LESS INPUT OF EVERY DISPATCH (the second-target form, HU10; a set that has the image): what the fake helper found
    // in HUDLESS is THE PICTURE OF THAT VERY FRAME where the hand's span handed it on from the binding's own image, and
    // the frame itself everywhere else (no interface, no picture, the image not nameable). Never another frame's.
    if (gMcHud == 2) for (const ShDisp& d : ShDispLog())
    {
        if (index.find(d.from) == index.end()) continue;      // (a dispatch before this segment's first frame)
        t.hudIn++;
        const bool fed = hudFed.count(d.from) != 0;
        if (fed) t.hudInOwn++;
        if (d.hud != (fed ? hudPic[d.from] : d.from)) { t.hudInBad++; if (t.hudFirst.empty()) t.hudFirst = Fmt("segment %d: the dispatch of frame %lX read %lX as its HUD-less picture (%lX wanted: %s)", seg, d.from, d.hud, fed ? hudPic[d.from] : d.from, fed ? "its own picture, handed on from the binding's image" : "the frame itself"); }
    }
    t.hudLost += (LONG)hudLost.size();
    for (int i = 0; i < FG_AW_N; i++) t.why[i] += a.w.iWhy[i];
    for (int i = 0; i < FG_AI_N; i++) t.inl[i] += a.w.inl[i];
    t.saves += a.w.saves; t.noSaves += a.w.noSaves; t.restoreLost += a.w.restoreLost; t.guardForced += a.w.guardForced; t.guardFlushes += a.w.guardFlushes;
    ShClean("MC");
    if (ShArmB::badSignal) Err("MC: the game signalled a timeline that is not its own %ld time(s)", ShArmB::badSignal);
    MgEnd();
}
template <class MM> static void MgCensus(const char* name, int segments, int perSegment)
{
    CaseBegin(name);
    gSeed = 0x51C7A3E9u ^ (gShAtHand ? 0x00F0F00Fu : 0u) ^ (unsigned)(segments * 131 + perSegment);
    McTot t;
    for (int i = 0; i < segments; i++) McSegment<MM>(i, perSegment, 1 + i % 4, i % 3, (i / 2) % 2, false, i == segments - 1 ? perSegment - 60 : -1, i == segments - 2 ? perSegment - 200 : -1, t);
    printf("  %ld frames in %d segments (1 to 4 images, three stream modes, the save answered both ways): %ld pairs armed; on the screen %ld real frames and %ld generated ones; never shown %ld, dropped and counted %ld; builds %ld; dues planted against the placer %ld; answers planted late %ld, with no frame %ld, the helper held %ld time(s)\n",
        t.frames, segments, t.armed, t.real, t.gen, t.never, t.dropped, t.builds, t.planted, t.late, t.noFrame, t.held);
    printf("  generated frames: shown %ld; not shown: a render fact %ld, fed and never shown for a reset %ld, no source %ld, late %ld, no frame %ld, the answer not this pair's at the fire %ld, a late fire %ld, the real frame forced first %ld, no buffer %ld, the write not taken %ld, dropped %ld\n",
        t.why[FG_AW_SHOWN], t.why[FG_AW_FACT], t.why[FG_AW_RESET], t.why[FG_AW_NOSOURCE], t.why[FG_AW_LATE], t.why[FG_AW_NOFRAME], t.why[FG_AW_NOANSWER], t.why[FG_AW_LATEFIRE], t.why[FG_AW_FORCED], t.why[FG_AW_NOBUFFER], t.why[FG_AW_NOCOPY], t.why[FG_AW_DROPPED]);
    printf("  inline: unsent %ld, no hold %ld, no HUD-less note %ld, closing %ld; real frames dropped: a dead machine %ld, a disarm %ld, a build %ld, no buffer %ld, the write not taken %ld; fires that saved %ld and that needed no save %ld; restores lost %ld; real frames forced ahead of a wait %ld, flushes there %ld\n",
        t.inl[FG_AI_UNSENT], t.inl[FG_AI_NOHOLD], t.inl[FG_AI_NOHUDLESS], t.inl[FG_AI_CLOSING], t.drop[FG_AD_DEAD], t.drop[FG_AD_DISARM], t.drop[FG_AD_BUILD], t.drop[FG_AD_NOBUFFER], t.drop[FG_AD_NOCOPY],
        t.saves, t.noSaves, t.restoreLost, t.guardForced, t.guardFlushes);
    printf("  THE WITNESS: %d picture(s) or frame(s) wrong%s%s; THE MIRROR: %ld of %ld dispatches where the worker's \"this one resets\" is not the helper's own record\n", t.bad, t.bad ? ": " : "", t.first.c_str(), t.mirror, t.mirrorN);
    if (t.bad) Err("%s: the witness counts %d fault(s); the first: %s", name, t.bad, t.first.c_str());
    if (t.mirror) Err("%s: the worker's mirror differs from the helper's record on %ld of %ld dispatches", name, t.mirror, t.mirrorN);
    if (t.frames < (LONG)segments * perSegment || t.real + t.never != t.frames) Err("%s: %ld frames were made, %ld shown and %ld never shown: the account does not close", name, t.frames, t.real, t.never);
    // (the census must have BEEN somewhere: each of these is a road a frame can take, and a run that took none of one proves nothing of it)
    const bool whole = segments * perSegment >= 10000;        // (the short census over the instrument's binding is held to the roads every run takes)
    if (t.why[FG_AW_SHOWN] < t.frames / 5 || !t.why[FG_AW_RESET] || !t.why[FG_AW_NOSOURCE] || !t.why[FG_AW_LATE] || !t.why[FG_AW_NOFRAME] || !t.why[FG_AW_FORCED] || !t.inl[FG_AI_UNSENT] || !t.saves || !t.noSaves || !t.mirrorN
        || (whole && (!t.why[FG_AW_NOANSWER] || !t.dropped || !t.builds || t.restoreLost != 1)))
        Err("%s: the run did not take every road (shown %ld, reset %ld, no source %ld, late %ld, no frame %ld, no answer at the fire %ld, forced %ld, unsent %ld, saves %ld, no saves %ld, dropped %ld, builds %ld, restores lost %ld where one is planted)", name,
            t.why[FG_AW_SHOWN], t.why[FG_AW_RESET], t.why[FG_AW_NOSOURCE], t.why[FG_AW_LATE], t.why[FG_AW_NOFRAME], t.why[FG_AW_NOANSWER], t.why[FG_AW_FORCED], t.inl[FG_AI_UNSENT], t.saves, t.noSaves, t.dropped, t.builds, t.restoreLost);
    // CONTROL
    const RingStats stc = gRingSt;
    McTot c;
    McSegment<MAL>(0, 600, 3, RING_CS_LAZY, 0, true, -1, -1, c);
    const LONG unans = gRingSt.genUnanswered - stc.genUnanswered;
    ViolRestore(stc);
    if (c.bad < 1 || unans < 1) Err("%s control: with the read by equality switched off the witness counted %d fault(s) and the fake %ld read(s) of GEN with no answer of its pair's: the control shows nothing", name, c.bad, unans);
    Ctl("the read by equality switched off, six hundred frames: the witness counted %d fault(s) (the first: %s), the fake %ld", c.bad, c.first.c_str(), unans);
    CaseEnd(name);
}
static void CaseMC()  { MgCensus<MA>("MC the census: 10,000 random frames over the merged path, the screen read as a whole (stage 7)", 10, 1000); }
static void CaseMCr() { MgCensus<MAR>("MCr the census over the merged binding that is an instrument's (kRig true), 2,000 frames", 4, 500); }

// ================================================================================================================
// STAGE 7c: THE LIFE (2026-10-10; critic.md's stage 7, part c, and its section 5: "a build, a Reset from another thread, a
// stop, a helper dead at six points, a disengage: one real frame a pair, shown or counted as dropped with its reason";
// machine.md's machine-8 and machine-10; lifecycle-safety.md's LS-1, -2, -10, -12, -13, -16 and -20; check-machine.md's
// R3, R4 and R19; docs/reports/fg-3/merge-2026-10-10/READY-FOR-7C.md, section 3, items 1 to 7).
//
// THE RULE EVERY CASE HERE HOLDS: one real frame a pair, shown once and in order, or counted as dropped with its reason;
// never a generated pixel left in a game image; never a present behind an unfinished wait beyond the gate's bound.
// The worker is HELD wherever an order is what is proved, with the commands queued behind it.
// ================================================================================================================
// The layer arms again after 300 presents that worked, through the merged machine's own FgSwapPresent.
template <class MM = MA> static bool MgEnsureArmed() { for (int i = 0; i < 400 && !dtArmed; i++) { Facts(false); Present<MM>(); } return dtArmed; }
// The publication the worker made for pair p (false: none, or the ring moved on).
static bool MgPub(LONG p, FgPub* out) { return MA::Read(B::S().pub[p & (FG_RING - 1)], p, out); }
// Pictures on the screen, from `from` on, that thread `tid` presented.
static int MgShownBy(size_t from, DWORD tid) { int n = 0; const auto v = Shown(); for (size_t i = from; i < v.size(); i++) n += v[i].tid == tid; return n; }
// The screen from `from` on is exactly these pictures, each once, in this order.
static bool MgExactly(size_t from, const std::vector<DWORD>& want, std::string* tail)
{
    const auto v = Shown();
    *tail = TagList(v, from);
    if (v.size() != from + want.size()) return false;
    for (size_t i = 0; i < want.size(); i++) if (v[from + i].tag != want[i]) return false;
    return true;
}
static int MgGenCount(size_t from) { int n = 0; const auto v = Shown(); for (size_t i = from; i < v.size(); i++) n += ShIsGen(v[i].tag) ? 1 : 0; return n; }
// The helper is gone, and its two fences read the top.
static bool MgGone() { ShHelperGone(); return WaitFor([] { return B::TlValue(FG_TL_OUT) == ~0ull && B::TlValue(FG_TL_PREP) == ~0ull; }, 3000); }

// ---------------------------------------------------------------- M13, a stop with a pair armed
// WHAT IT SHOWS (machine.md, machine-10; lifecycle-safety.md, LS-16; H13's shape over the merged binding). The transport
// stops (any thread, the worker included: the pipe's own failure stops it there) WITH A PAIR ARMED:
//   (a) its generated frame is due, and its answer is in. The machine is CLOSING and the present observer is unchained
//       at once; the next fire point of any kind, here the game's next draw, fires THE REAL FRAME ONCE, counted "at the
//       stop"; nothing generated is shown after a stop; the checkpoint observers go; a second point finds nothing; and
//       three frames more are passed through, each once, with nothing written to the pipe.
//   (b) THE WORKER HELD, the stop (a note on the worker) and the whole next frame queued behind it: nothing reaches the
//       screen while it is held; released, the screen reads the armed pair's real frame and then the next frame, each
//       once, and no message was written after the stop: no 'Q' behind the injection, none at the hand.
//   (c) THE STOP BETWEEN A PAIR'S NOTES AND ITS PRESENT (the worker held, the frame before still armed and no drain note):
//       the hand brings the frame before to its real frame, this pair's Present goes out inline, "closing", at once.
// CONTROL: the stop made DEAD (the machine's Kill): the armed pair's real frame is never shown. It is dropped, counted.
static void M13Stop(DWORD, DWORD, DWORD) { MA::Stop("the harness's stop, on the worker"); }
static volatile LONG gM13Seen = -1;
static void M13Look() { gM13Seen = (LONG)RingPresentedLog().size(); }
static void CaseM13()
{
    const char* const name = "M13 a stop with a pair armed: its real frame once, nothing generated, nothing more on the pipe (stage 7c)";
    CaseBegin(name);
    FgState& s = B::S(); FgArmedState& a = ShArmB::a; FgShadowState& x = ShB::x;
    // (a)
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>();
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    DueI(MgP(), FG_DUE_SHOW);                                 // (its generated frame is due and its answer is in: a fire point would show it)
    const size_t sh0 = MgShownN(); const LONG stop0 = s.c.nAtStop, f0 = a.w.iWhy[FG_AW_FORCED];
    MA::Stop("the harness's stop");
    const bool closing = s.sh.state == FG_CLOSING, unchained = dtPresentObs == PrevPresent;
    const size_t pipe0 = RingPipeLog().size();
    Fill(NewTag()); Drain();                                  // the next fire point: a checkpoint
    const bool ckGone = dtObs[35] == nullptr && dtObs[82] == PrevObs82;
    Fill(NewTag()); FireNote(FG_PT_D); Drain();               // a second point: nothing left
    const int realA = MgTagCount(sh0, tagA), genA = MgGenCount(sh0); const size_t shownA = MgShownN() - sh0;
    const LONG atStop = s.c.nAtStop - stop0, forcedA = a.w.iWhy[FG_AW_FORCED] - f0;
    const size_t sh1 = MgShownN(); std::vector<DWORD> three;
    for (int i = 0; i < 3; i++) { ClockAdd(16.7); ShInject(); three.push_back(Frame<MA>()); Drain(); }
    std::string tail3; const bool after3 = MgExactly(sh1, three, &tail3);
    const size_t wroteA = RingPipeLog().size() - pipe0;
    printf("  (a) a stop with pair %lX armed (%s) and its generated frame due: state %ld, the present observer unchained %d; at the next fire point: its real frame %d time(s), generated %d, %zu picture(s) in all, counted at the stop %ld, the generated frame not shown for the forced real frame %ld; checkpoint observers removed %d; three frames more: %s, %zu message(s) written since the stop\n",
        tagA, armedA ? "yes" : "NO", s.sh.state, (int)unchained, realA, genA, shownA, atStop, forcedA, (int)ckGone, tail3.c_str(), wroteA);
    if (!armedA || !closing || !unchained) Err("M13 (a): the pair was %s, and the stop left state %ld with the present observer %s (armed, CLOSING and unchained wanted)", armedA ? "armed" : "NOT armed", s.sh.state, unchained ? "unchained" : "still the machine's");
    if (realA != 1 || genA != 0 || shownA != 1 || atStop != 1 || forcedA != 1 || !ckGone) Err("M13 (a): after the stop the real frame was shown %d time(s), %d generated image(s), %zu picture(s) in all; counted at the stop %ld, forced %ld, checkpoint observers removed %d (1, 0, 1, 1, 1 and yes wanted)", realA, genA, shownA, atStop, forcedA, (int)ckGone);
    if (!after3 || wroteA != 0 || x.sh.stopped != 1) Err("M13 (a): three frames after the stop showed %s (each once, in order, wanted) and %zu message(s) were written (0 wanted)", tail3.c_str(), wroteA);
    MgClean("M13 (a)");
    MgEnd();
    // (b)
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagB0 = ShFrame<MA>();
    const size_t sh2 = MgShownN(), pipe2 = RingPipeLog().size(); const LONG stop2 = s.c.nAtStop;
    DWORD tagB1 = 0; gM13Seen = -1;
    MgHeld([&] { Run(M13Stop); tagB1 = MgRecBegin(); MgRecHand(); RecordHudless(); RecordPresent(); }, &M13Look);
    std::string tailB; const bool orderB = MgExactly(sh2, { tagB0, tagB1 }, &tailB);
    const size_t wroteB = RingPipeLog().size() - pipe2; const LONG atStopB = s.c.nAtStop - stop2;
    const bool openB = s.w.openPhase != FG_PAIR_NONE;
    printf("  (b) the worker held, a stop and the whole next frame queued behind it: %ld picture(s) on the screen while held; released: %s (%lX then %lX wanted); counted at the stop %ld; %zu message(s) written; a pair left open: %s\n",
        (long)(gM13Seen < 0 ? -1 : gM13Seen - (LONG)sh2), tailB.c_str(), tagB0, tagB1, atStopB, wroteB, openB ? "YES" : "no");
    if (gM13Seen != (LONG)sh2) Err("M13 (b): %ld picture(s) reached the screen while the worker was held (0 wanted)", (long)(gM13Seen - (LONG)sh2));
    if (!orderB || atStopB != 1 || wroteB != 0 || openB) Err("M13 (b): the screen showed %s, the real frame counted at the stop %ld time(s), %zu message(s) written after it, a pair left open %d (%lX then %lX, 1, 0 and no wanted)", tailB.c_str(), atStopB, wroteB, (int)openB, tagB0, tagB1);
    MgClean("M13 (b)");
    MgEnd();
    // (c)
    MgFresh<MA>();
    gC1Off = 1;                                               // (rule C3: the injection forces nothing, so the hand is what finds the frame before armed)
    ShWarm<MA>(); MgDueN();
    const DWORD tagC0 = ShFrame<MA>();
    const size_t sh3 = MgShownN(); const LONG cl0 = a.w.inl[FG_AI_CLOSING];
    DWORD tagC1 = 0;
    MgHeld([&] { tagC1 = MgRecBegin(); MgRecHand(); RecordHudless(); Run(M13Stop); RecordPresent(); });
    std::string tailC; const bool orderC = MgExactly(sh3, { tagC0, tagC1 }, &tailC);
    const LONG inlC = a.w.inl[FG_AI_CLOSING] - cl0;
    printf("  (c) the worker held, a frame's notes, a stop, then its Present: the screen showed %s (%lX then %lX wanted); presented inline as closing %ld\n", tailC.c_str(), tagC0, tagC1, inlC);
    if (!orderC || inlC != 1 || s.w.openPhase != FG_PAIR_NONE) Err("M13 (c): the screen showed %s and the pair whose Present came after the stop was presented inline as closing %ld time(s) (%lX then %lX, and 1, wanted)", tailC.c_str(), inlC, tagC0, tagC1);
    if (!ShSettle()) Err("M13 (c): the fake helper did not read what was written before the stop");
    MgClean("M13 (c)");
    MgEnd();
    // CONTROL
    const RingStats stc = gRingSt;
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagK = ShFrame<MA>();
    const size_t sh4 = MgShownN();
    MA::Kill("the harness's control: a stop straight to DEAD");
    Fill(NewTag()); Drain();
    ClockAdd(16.7); ShInject(); Frame<MA>(); Drain();
    const int shownK = MgTagCount(sh4, tagK); const LONG droppedK = a.w.dropped[FG_AD_DEAD];
    ViolRestore(stc);
    MgEnd();
    if (shownK != 0 || droppedK != 1) Err("M13 control: with the stop made DEAD the armed pair's real frame was shown %d time(s) and counted dropped %ld time(s) (0 and 1 wanted): the control shows nothing", shownK, droppedK);
    Ctl("a stop straight to DEAD: the armed pair's real frame was never shown (%d), dropped and counted (%ld)", shownK, droppedK);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M14, a disarm and a Reset from another thread with a pair armed
// WHAT IT SHOWS (machine.md, machine-10, with check-machine.md's R5; lifecycle-safety.md, LS-10; the review's F34).
//   A RESET FROM ANOTHER THREAD, the render thread idle, BETWEEN A PAIR'S TWO FIRES (its generated frame is on the screen,
//   its real frame pending; rule C3, and the next frame's injection has run, so with 'Q' behind the body a 'Q' is open
//   with no hand yet): the disarm's tail, on the Reset's thread, DROPS the real frame, counted with its reason; presents
//   nothing from that thread; writes nothing there (the open 'Q''s CLOSED 'G' is owed, and the worker writes it after the
//   re-arm); and leaves no pair armed and no checkpoint observer. The dropped frame never reaches the new images.
//   AFTER THE RE-ARM THE FIRST PAIR IS AN ENGAGE'S FIRST (F34, fixed for the merged path): its hand carries FIRST and the
//   warm-up; its 'Q' says the device changed; its dispatch resets, so it is fed and not shown; AND THE PAIR BEHIND IT IS
//   SHOWN: a Reset costs the dropped frame and one pair.
//   A RESET WHILE THE RENDER THREAD RECORDS, a pair armed: the render thread answers the ask and runs the disarm's tail
//   itself, mid-frame: the same drop, counted.
//   A BUILD WITH ENGAGED PRESENTS ON BOTH SIDES (no present between them that was not engaged): the new set's first pair
//   is an engage's first too.
// CONTROLS: the arm generation's compare defeated (the harness makes the last hand's generation the new one before the
// first frame after the re-arm): that pair is NOT first. And "was engaged" put back by hand behind the build's quiesce:
// the new set's first pair is NOT first. (Either way the 'Q' says why the helper must reset, so the screen is the same:
// what F34 costs is the warm-up and the placer's restarts, never a wrong frame. check-machine.md, R5.)
struct M14Leg { bool armed, closed, fromReset, rearmed, first, warm; LONG dropped, owed, early, wrote, whyC, owedWritten; DWORD qC; int genA, realA, genC, realC, genD, realD; std::string bad; };
static M14Leg M14Foreign(bool defeat)
{
    M14Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    gC1Off = 1;                                               // rule C3: the next injection forces nothing, so the pair is still armed at the Reset
    ShWarm<MA>(); MgDueN();
    s.r.sinceEngage = 1000;                                   // (past an engage's warm-up: the warm-up the first pair behind the re-arm carries is then the re-arm's own)
    const DWORD tagA = ShFrame<MA>();
    const size_t sh0 = MgShownN();
    MgDueShow();                                              // its generated frame is on the screen: the Reset falls between the pair's two fires
    ClockAdd(16.7); ShInject(true); Drain();                  // the next frame's injection
    if (!ShSettle()) Err("M14: the fake helper did not read the injection's 'Q'");
    g.early = x.e.qN;
    g.armed = s.w.openPhase == FG_PAIR_ARMED;
    const size_t pipe0 = RingPipeLog().size(); const LONG disarms0 = dtDisarms;
    HANDLE h = Spawn(ResetFromTid); Join(h, "M14's foreign Reset");
    if (dtDisarms != disarms0 + 1 || dtArmed) Err("M14: the foreign Reset did not disarm the layer");
    g.wrote = (LONG)(RingPipeLog().size() - pipe0);
    g.dropped = a.w.dropped[FG_AD_DISARM]; g.owed = x.sh.owedG;
    g.fromReset = MgShownBy(sh0, (DWORD)gForeignResetTid) != 0;
    g.closed = s.w.openPhase == FG_PAIR_NONE && dtObs[35] == nullptr;
    g.rearmed = MgEnsureArmed<MA>();
    if (defeat) s.r.lastGen = dtArmCount;                      // THE CONTROL: the last hand's generation made the new one
    const DWORD tagC = ShFrame<MA>();
    const LONG pC = MgP();
    FgPub v = {}; const bool hasPub = MgPub(pC, &v);
    g.first = hasPub && (v.flags & FG_HF_FIRST) != 0; g.warm = hasPub && (v.flags & FG_HF_WARM) != 0;
    FgArmedPair rec = {}; g.whyC = MgPair(pC, &rec) ? rec.why : -1;
    g.qC = ShQFlags(RingPipeLog(), x.w.lastQN);
    const size_t sh1 = MgShownN();
    MgDueShow(); MgDueN();
    const DWORD tagD = ShFrame<MA>(); MgDueShow(); MgDueN();
    g.genC = MgTagCount(sh1, ShGenTag(tagC, false)) + MgTagCount(sh1, ShGenTag(tagC, true)); g.realC = MgTagCount(sh1, tagC);
    g.genD = MgTagCount(sh1, ShGenTag(tagD, false)); g.realD = MgTagCount(sh1, tagD);
    g.genA = MgTagCount(sh0, ShGenTag(tagA, false)); g.realA = MgTagCount(sh0, tagA);
    g.owedWritten = x.w.gOwedWritten;
    ShPairs(RingPipeLog(), &g.bad);
    return g;
}
static bool M14BuildFirst(bool undo)
{
    MgFresh<MA>();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueN();
    MA::BeforeBuild(); Drain(); MA::BeforeRelease();
    if (undo) s.r.wasEngaged = true;                          // THE CONTROL: "was engaged" as it stood across a build until this stage
    ShB::WriteB();
    if (!ShSettle()) Err("M14: the helper did not read the build");
    ShFrame<MA>();
    FgPub v = {}; const bool first = MgPub(MgP(), &v) && (v.flags & FG_HF_FIRST) != 0;
    MgDueN(); ShFrame<MA>(); MgDueN();
    return first;
}
static void CaseM14()
{
    const char* const name = "M14 a disarm and a Reset from another thread with a pair armed, and the first pair behind them (stage 7c)";
    CaseBegin(name);
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    const M14Leg g = M14Foreign(false);
    printf("  a Reset from another thread between a pair's two fires (armed %s): its generated frame on the screen %d time(s), its real frame %d (dropped for the disarm, counted %ld); presented from the Reset's thread: %s; %ld message(s) written there, the 'G' owed for %ld (the 'Q' open at the Reset: %ld); no pair armed and no checkpoint observer afterwards: %s\n",
        g.armed ? "yes" : "NO", g.genA, g.realA, g.dropped, g.fromReset ? "YES" : "no", g.wrote, g.owed, g.early, g.closed ? "yes" : "NO");
    printf("  after the re-arm (%s): the first hand carries FIRST %d and the warm-up %d; its 'Q' carries 0x%lX; its generated frame decided %ld (fed, never shown for a reset is %d), shown %d, its real frame %d; the pair behind it: %d and %d; owed 'G' written later %ld\n",
        g.rearmed ? "armed" : "NOT ARMED", (int)g.first, (int)g.warm, g.qC, g.whyC, (int)FG_AW_RESET, g.genC, g.realC, g.genD, g.realD, g.owedWritten);
    if (!g.armed || g.genA != 1 || g.realA != 0 || g.dropped != 1 || g.fromReset || g.wrote != 0 || !g.closed) Err("M14: the Reset between the two fires: armed %d, the generated frame shown %d, the real frame %d, dropped for the disarm %ld, presented from the Reset's thread %d, %ld message(s) written there, closed %d (yes, 1, 0, 1, no, 0 and yes wanted)", (int)g.armed, g.genA, g.realA, g.dropped, (int)g.fromReset, g.wrote, (int)g.closed);
    if (g.owed != g.early || (gShAtHand ? g.early != 0 : g.early == 0) || g.owedWritten != (g.early ? 1 : 0)) Err("M14: the 'Q' open at the Reset was %ld, the 'G' owed for %ld, and %ld owed 'G' written after the re-arm (under the key none is open; behind the body one is, owed and written once)", g.early, g.owed, g.owedWritten);
    if (!g.rearmed || !g.first || !g.warm || !(g.qC & FG_Q_DEVICE) || g.whyC != FG_AW_RESET) Err("M14: after the re-arm the first hand carries FIRST %d and the warm-up %d, its 'Q' 0x%lX, its generated frame decided %ld (FIRST, the warm-up, the device's reason and \"fed, never shown\" wanted)", (int)g.first, (int)g.warm, g.qC, g.whyC);
    if (g.genC != 0 || g.realC != 1 || g.genD != 1 || g.realD != 1) Err("M14: after the re-arm the first pair showed its generated frame %d time(s) and its real frame %d, the pair behind it %d and %d (0, 1, 1 and 1 wanted: a Reset costs one pair)", g.genC, g.realC, g.genD, g.realD);
    if (!g.bad.empty()) Err("M14: %s", g.bad.c_str());
    MgClean("M14");
    MgEnd();
    // A RESET WHILE THE RENDER THREAD RECORDS, a pair armed
    {
        MgFresh<MA>();
        ShWarm<MA>(); MgDueN();
        const DWORD tagE = ShFrame<MA>();
        const bool armedE = s.w.openPhase == FG_PAIR_ARMED;
        const size_t sh0 = MgShownN(), pipe0 = RingPipeLog().size();
        gBusyGo = 0;
        HANDLE h = Spawn(ResetBusy); gBusyGo = 1;
        { const ULONGLONG t0 = GetTickCount64(); unsigned n = 0; while (dtArmed && GetTickCount64() - t0 < 3000) { D->SetRenderState((D3DRENDERSTATETYPE)(7 + n % 200), n); n++; } }
        Join(h, "M14's recording Reset");
        if (dtArmed) Err("M14: a Reset did not unarm a recording render thread");
        const LONG droppedE = a.w.dropped[FG_AD_DISARM]; const size_t wroteE = RingPipeLog().size() - pipe0;
        const bool closedE = s.w.openPhase == FG_PAIR_NONE && dtObs[35] == nullptr;
        if (!MgEnsureArmed<MA>()) Err("M14: no re-arm after the recording Reset");
        const size_t sh1 = MgShownN();
        const DWORD t1 = ShFrame<MA>(); MgDueShow(); MgDueN();
        const DWORD t2 = ShFrame<MA>(); MgDueShow(); MgDueN();
        std::string tail; const bool order = MgExactly(sh1, { t1, ShGenTag(t2, false), t2 }, &tail);
        const int shownE = MgTagCount(sh0, tagE);
        printf("  a Reset while the render thread records, pair %lX armed (%s): dropped for the disarm %ld, %zu message(s) written in the tail, closed %s, the frame on the screen %d time(s); after the re-arm the screen showed %s\n",
            tagE, armedE ? "yes" : "NO", droppedE, wroteE, closedE ? "yes" : "NO", shownE, tail.c_str());
        if (!armedE || droppedE != 1 || wroteE != 0 || !closedE || shownE != 0 || !order) Err("M14: the recording Reset: armed %d, dropped %ld, %zu message(s), closed %d, the frame shown %d time(s), and afterwards %s (yes, 1, 0, yes, 0, and the first pair's real frame then the second's two pictures wanted)", (int)armedE, droppedE, wroteE, (int)closedE, shownE, tail.c_str());
        MgClean("M14 (recording)");
        MgEnd();
    }
    const bool firstBuild = M14BuildFirst(false);
    MgClean("M14 (a build)");
    MgEnd();
    printf("  a build with engaged presents on both sides: the new set's first hand carries FIRST: %s\n", firstBuild ? "yes" : "NO");
    if (!firstBuild) Err("M14: behind a build with engaged presents on both sides the new set's first pair is not an engage's first");
    // CONTROLS
    const RingStats stc = gRingSt;
    const M14Leg c = M14Foreign(true);
    MgEnd();
    const bool firstUndo = M14BuildFirst(true);
    MgEnd();
    ViolRestore(stc);
    if (c.first || c.warm || firstUndo) Err("M14 control: with the arm generation's compare defeated the first hand after the re-arm carries FIRST %d and the warm-up %d, and with \"was engaged\" put back behind the quiesce the new set's first hand carries FIRST %d (none wanted): the control shows nothing", (int)c.first, (int)c.warm, (int)firstUndo);
    Ctl("the arm generation's compare defeated: the first hand after the re-arm is NOT first (FIRST %d, the warm-up %d; its generated frame shown %d, its real frame %d: the same screen); \"was engaged\" put back behind a build's quiesce: NOT first (%d)", (int)c.first, (int)c.warm, c.genC, c.realC, (int)firstUndo);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M15, a machine that fires nothing, with a pair armed
// WHAT IT SHOWS (lifecycle-safety.md, LS-1 and LS-11; READY-FOR-7C.md, section 3, item 2, and the one line of its section 7
// that no case held: row 49). A machine that is DEAD fires nothing, whoever asks: an armed pair's real frame is then
// DROPPED, never presented, and counted with its reason, at the first of these that comes:
//   (a) THE NEXT HAND (the worker held; the kill, a note on the worker, and the whole next frame queued behind it; a look
//       queued ahead of the hand and one right behind it). Ahead of the hand the pair is still armed on the dead machine,
//       its checkpoint observers installed. BEHIND THE HAND NOTHING IS ARMED: dropped, counted, the observers gone; the
//       hand itself opens nothing, and its frame is passed through, once.
//   (b) NO HAND COMES (a dead machine records none): the close note of the next present ends the pair, the same count.
//   (c) A FIRE'S PRESENT WAS HELD IN THE GATE WHEN THE MACHINE DIED. The real frame's fire stands in the gate (a helper
//       that is behind holds it there, for seconds); a Reset on another thread, whose barrier stood aside because the
//       worker did not come back, kills the machine (FgMachine::BeforeReset) and goes ahead. The fire is let out of the
//       gate: IT READS THE MACHINE'S STATE AGAIN, presents nothing, submits nothing, and the pair ends dropped, counted.
//   (d) AND A FIRE THAT IS PAST THAT READ (it stands inside DXVK's Present) IS WAITED FOR: the same Reset's BeforeReset
//       kills the machine and then waits out the fire, which ends with its picture presented and its back buffer given
//       back before the Reset goes on (H9's rule, over the merged binding).
// CONTROLS: (a) is its own: the look AHEAD of the hand is the state the hand's line removes. (c): the read behind the gate
// switched off: the dead machine's fire writes the back buffer and PRESENTS, after the kill.
static void M15Kill(DWORD, DWORD, DWORD) { MA::Kill("the harness's kill, with a pair armed"); }
static volatile LONG gM15[2][4], gM15Ret = -1;
static void M15Look(DWORD, DWORD, DWORD at)
{
    FgState& s = B::S();
    gM15[at][0] = s.w.openPhase; gM15[at][1] = ShArmB::a.w.dropped[FG_AD_DEAD]; gM15[at][2] = dtObs[35] != nullptr ? 1 : 0; gM15[at][3] = s.sh.pairOpen;
}
static DWORD WINAPI M15Reset(void*) { _InterlockedExchange(&gM15Ret, MA::BeforeReset(true, "the harness: the barrier stood aside")); return 0; }
struct M15Gate { bool inGate; LONG presents, submits, dropped, phase, busy, ret, state; int shown; };
static M15Gate M15GateRun(LONG guardOff)
{
    M15Gate g = {};
    MgFresh<MA>();
    gArmedGuardOff = guardOff;
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>();
    const size_t sh0 = MgShownN(); const LONG p0 = gRingSt.presents, sub0 = gRingSt.submits;
    ShArmB::gateHoldNext = 1;
    DueN(MgP()); FireNote();                                  // the real frame's fire: it stands in the gate
    g.inGate = WaitFor([] { return ShArmB::gateIn != 0; }, 3000);
    gM15Ret = -1;
    HANDLE h = Spawn(M15Reset); Join(h, "M15's Reset that stood aside");
    g.ret = gM15Ret; g.state = s.sh.state;
    ShArmB::GateRelease();
    Drain();
    g.presents = gRingSt.presents - p0; g.submits = gRingSt.submits - sub0;
    g.shown = MgTagCount(sh0, tagA); g.dropped = a.w.dropped[FG_AD_DEAD]; g.phase = s.w.openPhase; g.busy = s.sh.fireBusy;
    gArmedGuardOff = 0;
    return g;
}
static void CaseM15()
{
    const char* const name = "M15 a machine that fires nothing, with a pair armed: dropped and counted, at the hand, at the close note, behind the gate (stage 7c)";
    CaseBegin(name);
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    // (a)
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>();
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t sh0 = MgShownN(); const LONG closed0 = s.c.pubClosed[FG_CL_DEAD];
    DWORD tagB = 0; memset((void*)gM15, 0xFF, sizeof(gM15));
    MgHeld([&] { Run(M15Kill); tagB = MgRecBegin(); Run(M15Look, 0); MgRecHand(); Run(M15Look, 1); RecordHudless(); RecordPresent(); });
    std::string tailA; const bool onlyB = MgExactly(sh0, { tagB }, &tailA);
    const LONG closedA = s.c.pubClosed[FG_CL_DEAD] - closed0;
    printf("  (a) pair %lX armed (%s), the worker held, a kill and the next frame queued behind it. AHEAD of the hand: the pair's phase %ld (armed is %d), dropped for a dead machine %ld, checkpoint observers installed %ld. BEHIND the hand: phase %ld, dropped %ld, observers %ld, pairs open %ld; the hand published closed for a dead machine %ld; the screen showed %s (the next frame %lX alone wanted)\n",
        tagA, armedA ? "yes" : "NO", gM15[0][0], (int)FG_PAIR_ARMED, gM15[0][1], gM15[0][2], gM15[1][0], gM15[1][1], gM15[1][2], gM15[1][3], closedA, tailA.c_str(), tagB);
    if (!armedA || gM15[0][0] != FG_PAIR_ARMED || gM15[0][1] != 0 || gM15[0][2] != 1) Err("M15 (a): ahead of the hand the pair's phase is %ld, dropped %ld, observers installed %ld (armed, 0 and 1 wanted: the leg did not reach a pair armed on a dead machine)", gM15[0][0], gM15[0][1], gM15[0][2]);
    if (gM15[1][0] != FG_PAIR_NONE || gM15[1][1] != 1 || gM15[1][2] != 0 || gM15[1][3] != 0) Err("M15 (a): right behind the hand the pair's phase is %ld, dropped for a dead machine %ld, checkpoint observers installed %ld, pairs open %ld (none, 1, 0 and 0 wanted): a hand left a pair armed on a machine that fires nothing", gM15[1][0], gM15[1][1], gM15[1][2], gM15[1][3]);
    if (!onlyB || closedA != 1 || a.w.iWhy[FG_AW_DROPPED] != 1) Err("M15 (a): the screen showed %s (the next frame %lX alone wanted), the hand was published closed for a dead machine %ld time(s), and the pair's generated frame counted dropped %ld time(s)", tailA.c_str(), tagB, closedA, a.w.iWhy[FG_AW_DROPPED]);
    MgClean("M15 (a)");
    MgEnd();
    // (b)
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    const DWORD tagC = ShFrame<MA>();
    const size_t sh1 = MgShownN();
    MA::Kill("the harness's kill, with a pair armed and no hand to come");
    const LONG before = a.w.dropped[FG_AD_DEAD];
    ClockAdd(16.7); ShInject(); const DWORD tagD = Frame<MA>(); Drain();
    std::string tailB; const bool onlyD = MgExactly(sh1, { tagD }, &tailB);
    const LONG droppedB = a.w.dropped[FG_AD_DEAD];
    printf("  (b) pair %lX armed, the machine killed, no hand to come: dropped %ld before the next present and %ld after it (its close note); the pair's phase %ld; the screen showed %s\n", tagC, before, droppedB, s.w.openPhase, tailB.c_str());
    if (before != 0 || droppedB != 1 || s.w.openPhase != FG_PAIR_NONE || !onlyD) Err("M15 (b): dropped %ld before the next present and %ld after it, the pair's phase %ld, the screen %s (0, 1, none and the next frame %lX alone wanted)", before, droppedB, s.w.openPhase, tailB.c_str(), tagD);
    MgClean("M15 (b)");
    MgEnd();
    // (c)
    const M15Gate g = M15GateRun(0);
    MgClean("M15 (c)");
    MgEnd();
    printf("  (c) the real frame's fire held in the gate (%s), a Reset whose barrier stood aside on another thread (it returned %ld, the machine's state %ld, DEAD is %d): released, the fire made %ld present(s) and %ld submit(s); the frame on the screen %d time(s); dropped for a dead machine %ld; the pair's phase %ld; a span still marked busy %ld\n",
        g.inGate ? "yes" : "NO", g.ret, g.state, (int)FG_DEAD, g.presents, g.submits, g.shown, g.dropped, g.phase, g.busy);
    if (!g.inGate || !(g.ret & 1) || g.state != FG_DEAD) Err("M15 (c): the leg did not reach its state (in the gate %d, BeforeReset returned %ld, state %ld)", (int)g.inGate, g.ret, g.state);
    if (g.presents != 0 || g.submits != 0 || g.shown != 0 || g.dropped != 1 || g.phase != FG_PAIR_NONE || g.busy != 0) Err("M15 (c): a fire let out of the gate on a dead machine made %ld present(s) and %ld submit(s), showed the frame %d time(s), dropped %ld, left phase %ld and busy %ld (0, 0, 0, 1, none and 0 wanted)", g.presents, g.submits, g.shown, g.dropped, g.phase, g.busy);
    // (d)
    {
        MgFresh<MA>();
        ShWarm<MA>(); MgDueN();
        const DWORD tagE = ShFrame<MA>(); const DWORD half = NewTag(); Fill(half); Drain();
        const size_t sh2 = MgShownN();
        RingGateHoldNext();
        DueN(MgP()); FireNote();                              // the real frame's fire: past its reads, inside DXVK's Present
        const bool inPresent = WaitFor([] { return RingGateHeld(); }, 3000);
        const LONG busyIn = s.sh.fireBusy;
        gM15Ret = -1;
        HANDLE h = Spawn(M15Reset);
        Sleep(250);
        const bool waiting = WaitForSingleObject(h, 0) == WAIT_TIMEOUT;   // BeforeReset has killed the machine and stands waiting for the fire
        const LONG stateIn = s.sh.state;
        RingGateRelease();
        Join(h, "M15's Reset that waited for the fire");
        const LONG busyOut = s.sh.fireBusy;
        Drain();
        const int shownE = MgTagCount(sh2, tagE); const DWORD bb = MgBb();
        printf("  (d) the real frame's fire inside DXVK's Present (%s, a span busy %ld) when the same Reset came: the machine DEAD at once (%ld), the Reset still waiting a quarter second on: %s; released: the frame on the screen %d time(s), the back buffer %lX (the game's drawing %lX); busy when the Reset went on: %ld\n",
            inPresent ? "yes" : "NO", busyIn, stateIn, waiting ? "yes" : "NO", shownE, bb, half, busyOut);
        if (!inPresent || busyIn != 1 || stateIn != FG_DEAD || !waiting) Err("M15 (d): the fire was inside DXVK's Present %d with busy %ld, the machine's state %ld, and the Reset waited for it %d (yes, 1, DEAD and yes wanted)", (int)inPresent, busyIn, stateIn, (int)waiting);
        if (shownE != 1 || bb != half || busyOut != 0) Err("M15 (d): the fire the Reset waited for showed its frame %d time(s) and left the back buffer at %lX (1 and the game's drawing %lX wanted); busy when the Reset went on %ld (0 wanted)", shownE, bb, half, busyOut);
        MgClean("M15 (d)");
        MgEnd();
    }
    // CONTROL
    const RingStats stc = gRingSt;
    const M15Gate c = M15GateRun(1);
    ViolRestore(stc);
    MgEnd();
    if (!c.inGate || c.state != FG_DEAD || c.presents != 1 || c.shown != 1) Err("M15 control: with the read behind the gate switched off the fire of a dead machine made %ld present(s) and showed the frame %d time(s) (1 and 1 wanted; in the gate %d, state %ld): the control shows nothing", c.presents, c.shown, (int)c.inGate, c.state);
    Ctl("ahead of the hand the pair stood armed on the dead machine with its observers in (phase %ld, observers %ld); the read behind the gate switched off: the dead machine's fire PRESENTED after the kill (%ld present(s), the frame on the screen %d time(s))", gM15[0][0], gM15[0][2], c.presents, c.shown);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M16, the helper dead at five more points
// WHAT IT SHOWS (lifecycle-safety.md, LS-13 to LS-15; the critic's section 5: "a helper dead at six points"; READY-FOR-7C.md,
// section 3, item 3: gone while held inside a dispatch with a pair armed is M10's, the other five are here). A helper
// that is GONE leaves its two fences at the top, as its exit and the kernel do; and the worker's next write to its pipe
// FAILS, which stops the transport on that thread, inside the note that wrote. Each leg is run BOTH WAYS where the
// pipe matters: with the write failing (the mod's chain: gone, the write fails, the stop), and with the write going to
// a pipe nobody reads (the window before that write). In every leg the screen is each real frame once, in order, and
// NOT ONE GENERATED IMAGE after the death.
//   1. IN A PREPARE WITH A WAIT QUEUED. The helper is held in a prepare; the next injection forced the pending real frame
//      out and queued its wait; a whole frame stands on the queue behind it. The helper dies there, with the worker
//      held and that frame's real frame's fire and the next frame queued behind it: the fences at the top let the
//      queue run, and the three frames are on the screen, once, in order.
//   2. BETWEEN 'Q' AND 'G'. The worker held between the two writes (behind the injection that wrote 'Q'; under the key,
//      behind the hand that did), the helper dies, released: with the pipe failing the 'G' is the write that fails, the
//      machine closes inside that note and the pair's Present goes out inline, at once; with it not failing the pair
//      arms, its generated frame, ASKED FOR, is refused by the read on the worker, and its real frame is shown.
//   3. BEFORE THE FIRST ANSWER. The session's first pair, the helper inside its first dispatch, and it dies: fgOut goes
//      from 0 to the top with no answer ever stamped. The pair was timed from its own handoff, so the placer raises its
//      real frame's due all the same; and the next pair is NOT left with no source for ever (the top is "answered").
//   4. DURING A BUILD. The helper is inside a dispatch when the build comes and dies before it reads the build message:
//      the game holds a new set nothing ever wrote. Two frames of it, each with its generated frame asked for AND THE
//      MIRROR OVERRULED (a note says "may be shown"): the read by equality alone stands between the screen and an image
//      nothing wrote, and nothing is landed.
//   5. WITH A PRESENT HELD IN THE GATE. (i) A real frame's present stands in the gate behind a prepare the helper is held
//      in. KILLED: the fence at the top ends the hold by itself, inside its bound. HUNG (it answers nothing, its fences
//      stay): the transport's watch stops the transport and releases the wait, and the gate ends there; either way the
//      three frames are on the screen, once, in order, and at most one present was entered behind the wait.
//      (ii) A GENERATED frame's present stands in the gate (its answer was read, rightly, before it) and the helper dies
//      meanwhile: let out, the fire READS THE ANSWER AGAIN, finds the top, and shows nothing; the real frame follows.
// CONTROLS: 4 over a binding whose read on the worker says yes: the new set's GEN, which nothing wrote, is landed: the
// fake counts it and the screen shows its sentinel. 5 (ii) with the read behind the gate switched off: the generated
// image of a helper that is gone is landed after its death: the fake counts a read of GEN with no answer.
static void M16PipeStop() { MA::Stop("the harness, standing in for the transport: a write to the helper's pipe failed"); }
struct M16Leg { bool reached, exact; std::string tail; int gen; LONG state, failed, extra; size_t whileHeld; };
static volatile LONG gM16Seen = -1;
static void M16Look() { gM16Seen = (LONG)RingPresentedLog().size(); }
// 1: in a prepare with a wait queued
static M16Leg M16Prep(bool pipe)
{
    M16Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S();
    ShArmB::pipeDies = pipe ? 1 : 0; ShArmB::onPipeFail = &M16PipeStop;
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    const DWORD tagA = ShFrame<MA>(true, false);
    g.reached = WaitFor([] { return gH.inPrep != 0; }, 3000);
    const size_t sh0 = MgShownN();
    ClockAdd(16.7); ShInject(true); Drain();                  // B's injection: A's real frame forced ahead of the wait, the wait queued
    const DWORD tagB = Frame<MA>(); Drain();                  // B: its span is on the queue, behind the wait
    g.reached = g.reached && ShB::waitFor != 0 && gH.inPrep != 0 && s.w.openPhase == FG_PAIR_ARMED;
    DWORD tagC = 0; gM16Seen = -1; bool gone = false;
    DueN(MgP());
    MgHeld([&] { gone = MgGone(); FireNote(); ClockAdd(16.7); ShInject(true); tagC = Frame<MA>(); }, &M16Look);
    if (s.w.openPhase == FG_PAIR_ARMED) { MgDueShow(); MgDueN(); }
    g.reached = g.reached && gone;
    g.whileHeld = gM16Seen < 0 ? 99 : (size_t)gM16Seen - sh0;
    g.exact = MgExactly(sh0, { tagA, tagB, tagC }, &g.tail);
    g.gen = MgGenCount(sh0); g.state = s.sh.state; g.failed = ShArmB::pipeFailed;
    return g;
}
// 5 (i): a real frame's present held in the gate, the helper killed, or hung and then the transport's watch
static M16Leg M16Gate(bool kill)
{
    M16Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S();
    ShArmB::gateModel = 1;
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    const DWORD tagA = ShFrame<MA>(true, false);
    g.reached = WaitFor([] { return gH.inPrep != 0; }, 3000);
    if (!kill) _InterlockedExchange(&gH.dead, 1);             // IT HANGS THERE: it will answer nothing, and its fences stay where they are
    const size_t sh0 = MgShownN();
    ClockAdd(16.7); ShInject(true); Drain();                  // frame B's injection: the pending real frame, the flush, then the wait
    const size_t e1 = RingEnteredLog().size();
    const DWORD tagB = Frame<MA>(); Drain();
    MgDueN();                                                 // B's real frame: the first present after the wait was queued
    ClockAdd(16.7); ShInject(true); const DWORD tagC = Frame<MA>(); Drain();
    DueN(MgP()); FireNote();                                  // C's real frame: the second: it stands in the gate
    g.reached = g.reached && WaitFor([] { return ShArmB::gateIn != 0; }, 3000);
    g.whileHeld = RingEnteredLog().size() - e1;               // presents that entered DXVK behind the wait while the helper stood
    g.extra = s.w.lockDepth + (gRingInterop.held ? 100 : 0);
    if (kill) g.reached = MgGone() && g.reached;
    else { MA::Stop("the harness, standing in for the transport's watch: the helper has not answered"); ShB::ReleaseWait(); }
    Drain();
    if (!WaitFor([] { return ShB::waitFor == 0 || ShB::heldCbs.empty(); }, 3000)) Err("M16: the queue did not run after the helper's end");
    g.exact = MgExactly(sh0, { tagA, tagB, tagC }, &g.tail);
    g.gen = MgGenCount(sh0); g.state = s.sh.state; g.failed = ShArmB::gateGaveUp;
    return g;                                                 // (the hung helper's thread stays in its hold until the case's end stops it)
}
// 2: between 'Q' and 'G'
static M16Leg M16QG(bool pipe)
{
    M16Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    ShArmB::pipeDies = pipe ? 1 : 0; ShArmB::onPipeFail = &M16PipeStop;
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueN();
    MgStamp<MA>();                                            // (the answer of the pair before is STAMPED while the helper lives, as the product's stamper stamps it: the hand behind the death can then tell it was a frame, and this pair may show one)
    const size_t sh0 = MgShownN();
    const DWORD tagB = MgRecBegin();                          // the frame's injection: with 'Q' behind the body, 'Q' is on the pipe when it has run
    if (gShAtHand) MgRecHand();                               // (under the key the hand writes it)
    Drain();
    if (!ShSettle()) Err("M16: the fake helper did not read the pair's 'Q'");
    const LONG nB = x.w.lastQN; const auto log0 = RingPipeLog();
    g.reached = ShFind(log0, 'Q', nB) != nullptr && ShFind(log0, 'G', nB) == nullptr;
    if (!Gate() || !WaitFor([] { return gGateIn != 0; }, 3000)) Err("M16: the worker never reached the gate");
    g.reached = MgGone() && g.reached;                        // THE HELPER DIES BETWEEN THE PAIR'S 'Q' AND ITS 'G'
    if (!gShAtHand) MgRecHand();
    RecordHudless();
    SetEvent(gGateEv); RecordPresent(); Drain();
    const LONG na0 = a.w.iWhy[FG_AW_NOANSWER];
    if (s.w.openPhase == FG_PAIR_ARMED) { MgDueShow(); MgDueN(); }
    g.extra = a.w.iWhy[FG_AW_NOANSWER] - na0;
    g.exact = MgExactly(sh0, { tagB }, &g.tail);
    g.gen = MgGenCount(sh0); g.state = s.sh.state; g.failed = ShArmB::pipeFailed;
    return g;
}
// 3: before the first answer
static M16Leg M16First(bool pipe)
{
    M16Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    ShArmB::pipeDies = pipe ? 1 : 0; ShArmB::onPipeFail = &M16PipeStop;
    a.k.kPlanUs = 2000;
    MgEvenStart();
    const size_t sh0 = MgShownN();
    _InterlockedExchange(&gH.holdDispatch, 1);
    std::vector<DWORD> tags;
    tags.push_back(MgEven<MA>(true, false));                  // the session's first pair: its dispatch is the helper's first
    g.reached = WaitFor([] { return gH.inDispatch != 0; }, 3000) && B::TlValue(FG_TL_OUT) == 0 && s.w.openPhase == FG_PAIR_ARMED;
    const LONG p1 = MgP();
    g.reached = MgGone() && g.reached;
    // each pair: timed at its handoff, the clock stepped to its real frame's due, the placer's raise, the fire point
    auto finish = [&]
    {
        if (s.w.openPhase != FG_PAIR_ARMED) return;
        const LONG p = MgP();
        MgPlace<MA>();
        if (const FgArmedLive* L = MgLive(p)) { if (L->tN > B::clockNow) B::clockNow = L->tN; }
        MgPlace<MA>();
        FireNote(); Drain();
    };
    finish();
    FgDue d = {}; const bool due1 = MgDue(true, p1, &d);
    tags.push_back(MgEven<MA>(true, false)); const LONG n2 = x.w.lastQN; finish();
    tags.push_back(MgEven<MA>(true, false)); finish();
    g.extra = (due1 ? 1 : 0) + (ShKindOf(RingPipeLog(), n2) == (int)FG_G_DISPATCH ? 2 : 0) + (x.w.ansUnknown > 0 ? 4 : 0);
    g.exact = MgExactly(sh0, tags, &g.tail);
    g.gen = MgGenCount(sh0); g.state = s.sh.state; g.failed = ShArmB::pipeFailed;
    return g;
}
// 4: during a build (MM: the merged machine, or the control's, whose read on the worker says yes)
template <class MM> static M16Leg M16Build(bool lie)
{
    M16Leg g = {};
    MgFresh<MM>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MM>(); MgDueN();
    ShFrame<MM>(); MgDueShow(); MgDueN();
    _InterlockedExchange(&gH.holdDispatch, 1);
    const DWORD tagA = ShFrame<MM>(true, false);
    g.reached = WaitFor([] { return gH.inDispatch != 0; }, 3000) && s.w.openPhase == FG_PAIR_ARMED;
    const size_t sh0 = MgShownN(); const LONG v0 = gRingSt.setVirgin, na0 = a.w.iWhy[FG_AW_NOANSWER];
    MM::BeforeBuild(); Drain();                               // THE BUILD: its drain note brings the pair to its real frame,
    MM::BeforeRelease();                                      // the quiesce,
    g.reached = MgGone() && g.reached;                        // AND THE HELPER DIES, before it reads the build message
    if (!ShArmB::RetireWait(2000)) Err("M16: the copies on the queue did not retire before the set went");
    ShB::WriteB();
    std::vector<DWORD> tags; tags.push_back(tagA);
    for (int i = 0; i < 2; i++)
    {
        tags.push_back(MgRecBegin()); MgRecHand(); RecordHudless(); Run(M4ForceShow); RecordPresent(); Drain();
        ShArmLieB::lie = lie ? 1 : 0; MgDueShow(); ShArmLieB::lie = 0;
        MgDueN();
    }
    g.extra = gRingSt.setVirgin - v0; g.failed = a.w.iWhy[FG_AW_NOANSWER] - na0;
    g.exact = MgExactly(sh0, tags, &g.tail);
    g.gen = MgGenCount(sh0); g.state = s.sh.state;
    return g;
}
// 5 (ii): a generated frame's present held in the gate, the helper dies meanwhile
struct M16Gen { bool inGate, gone; int gen, real; LONG gateRead, unanswered; };
static M16Gen M16GenGate(LONG guardOff)
{
    M16Gen g = {};
    MgFresh<MA>();
    gArmedGuardOff = guardOff;
    FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    const DWORD tagA = ShFrame<MA>();
    const size_t sh0 = MgShownN(); const LONG u0 = gRingSt.genUnanswered;
    ShArmB::gateHoldNext = 1;
    DueI(MgP(), FG_DUE_SHOW); FireNote();                     // its generated frame's fire: the answer is read, this pair's frame; then the gate
    g.inGate = WaitFor([] { return ShArmB::gateIn != 0; }, 3000);
    g.gone = MgGone();                                        // THE HELPER DIES WHILE THAT PRESENT IS HELD
    ShArmB::GateRelease(); Drain();
    MgDueN();
    g.gen = MgTagCount(sh0, ShGenTag(tagA, false)); g.real = MgTagCount(sh0, tagA);
    g.gateRead = a.w.gateRead; g.unanswered = gRingSt.genUnanswered - u0;
    gArmedGuardOff = 0;
    return g;
}
static void CaseM16()
{
    const char* const name = "M16 the helper dead at five more points: every real frame once, nothing generated after the death (stage 7c)";
    CaseBegin(name);
    for (int pipe = 1; pipe >= 0; pipe--)
    {
        const char* const how = pipe ? "the pipe's write fails and stops the transport" : "the write goes to a pipe nobody reads";
        const M16Leg p = M16Prep(pipe != 0);
        printf("  1 in a prepare with a wait queued (%s): %zu picture(s) on the screen while the worker was held; then the screen showed %s; generated images %d; the machine's state %ld; writes that failed %ld\n", how, p.whileHeld, p.tail.c_str(), p.gen, p.state, p.failed);
        if (!p.reached || p.whileHeld != 1 || !p.exact || p.gen != 0) Err("M16 (1, %s): reached %d; %zu picture(s) while held (the frame forced ahead of the wait, 1, wanted); the screen %s (three frames, each once, in order, wanted); generated %d", how, (int)p.reached, p.whileHeld, p.tail.c_str(), p.gen);
        if (pipe ? (p.state != FG_CLOSING || p.failed < 1) : (p.state != FG_LIVE || p.failed != 0)) Err("M16 (1, %s): the machine's state is %ld with %ld failed write(s)", how, p.state, p.failed);
        MgClean("M16 (1)"); MgEnd();
        const M16Leg q = M16QG(pipe != 0);
        printf("  2 between 'Q' and 'G' (%s): the screen showed %s; generated images %d; refused by the read on the worker %ld; the machine's state %ld; writes that failed %ld\n", how, q.tail.c_str(), q.gen, q.extra, q.state, q.failed);
        if (!q.reached || !q.exact || q.gen != 0) Err("M16 (2, %s): reached %d (the pair's 'Q' on the pipe and no 'G' when the helper died); the screen %s (the pair's real frame alone wanted); generated %d", how, (int)q.reached, q.tail.c_str(), q.gen);
        if (pipe ? (q.state != FG_CLOSING || q.failed != 1 || ShArmB::a.w.inl[FG_AI_CLOSING] != 1) : (q.state != FG_LIVE || q.extra != 1)) Err("M16 (2, %s): the machine's state is %ld, %ld write(s) failed, presented inline as closing %ld, refused by the read on the worker %ld (closing, 1 and 1 with the pipe failing; live and 1 refusal without)", how, q.state, q.failed, ShArmB::a.w.inl[FG_AI_CLOSING], q.extra);
        MgClean("M16 (2)"); MgEnd();
        const M16Leg f = M16First(pipe != 0);
        printf("  3 before the first answer (%s): the screen showed %s; generated images %d; the first pair's real frame's due raised by the placer: %s; the pair behind it written DISPATCH: %s; answers the worker could not tell: %s\n",
            how, f.tail.c_str(), f.gen, (f.extra & 1) ? "yes" : "NO", (f.extra & 2) ? "yes" : "no", (f.extra & 4) ? "yes" : "no");
        if (!f.reached || !f.exact || f.gen != 0 || !(f.extra & 1)) Err("M16 (3, %s): reached %d; the screen %s (three frames, each once, in order, wanted); generated %d; the first pair's real frame's due raised %d", how, (int)f.reached, f.tail.c_str(), f.gen, (int)(f.extra & 1));
        if (!pipe && (f.extra & 6) != 6) Err("M16 (3): with the helper gone before its first answer the pair behind was not written DISPATCH from an answer read as \"gone\" (%ld): it would have no source for ever", f.extra);
        MgClean("M16 (3)"); MgEnd();
    }
    const M16Leg b = M16Build<MA>(false);
    printf("  4 during a build: the screen showed %s; generated images %d; landings of a set image nothing wrote %ld; refused by the read on the worker %ld\n", b.tail.c_str(), b.gen, b.extra, b.failed);
    if (!b.reached || !b.exact || b.gen != 0 || b.extra != 0 || b.failed != 2) Err("M16 (4): reached %d; the screen %s (the pair before the build and two frames of the new set, each once, wanted); generated %d; a set image nothing wrote landed %ld time(s); refused by the read on the worker %ld (2 wanted)", (int)b.reached, b.tail.c_str(), b.gen, b.extra, b.failed);
    MgClean("M16 (4)"); MgEnd();
    for (int kill = 1; kill >= 0; kill--)
    {
        const M16Leg h = M16Gate(kill != 0);
        printf("  5 (i) a real frame's present held in the gate, the helper %s: presents entered behind the wait while it stood %zu, the worker holding %ld lock(s) there; then the screen showed %s; the gate gave up %ld time(s); the machine's state %ld\n",
            kill ? "KILLED" : "HUNG, then the transport's watch", h.whileHeld, h.extra, h.tail.c_str(), h.failed, h.state);
        if (!h.reached || h.whileHeld != 1 || h.extra != 0 || !h.exact || h.gen != 0 || h.failed != 0) Err("M16 (5 i, %s): reached %d; %zu present(s) entered behind the wait (1 wanted), holding %ld lock(s); the screen %s; generated %d; the gate gave up %ld time(s)", kill ? "killed" : "hung", (int)h.reached, h.whileHeld, h.extra, h.tail.c_str(), h.gen, h.failed);
        MgClean("M16 (5 i)"); MgEnd();
    }
    const M16Gen gg = M16GenGate(0);
    printf("  5 (ii) a generated frame's present held in the gate (%s), the helper dies meanwhile: let out, the generated frame on the screen %d time(s), refused by the read behind the gate %ld, its real frame %d; reads of GEN with no answer of its pair's %ld\n", gg.inGate ? "yes" : "NO", gg.gen, gg.gateRead, gg.real, gg.unanswered);
    if (!gg.inGate || !gg.gone || gg.gen != 0 || gg.gateRead != 1 || gg.real != 1 || gg.unanswered != 0) Err("M16 (5 ii): in the gate %d, the helper gone %d; the generated frame shown %d time(s), refused behind the gate %ld, the real frame %d, GEN read unanswered %ld (0, 1, 1 and 0 wanted)", (int)gg.inGate, (int)gg.gone, gg.gen, gg.gateRead, gg.real, gg.unanswered);
    MgClean("M16 (5 ii)"); MgEnd();
    // CONTROLS
    const RingStats stc = gRingSt;
    const M16Leg cb = M16Build<MAL>(true);
    MgEnd();
    const M16Gen cg = M16GenGate(2);
    MgEnd();
    ViolRestore(stc);
    if (cb.extra < 1 || cb.exact) Err("M16 control (4): with the read on the worker saying yes, %ld image(s) nothing wrote were landed and the screen showed %s (at least 1, and a picture that is no frame, wanted): the control shows nothing", cb.extra, cb.tail.c_str());
    if (cg.gen != 1 || cg.unanswered != 1 || cg.gateRead != 0) Err("M16 control (5 ii): with the read behind the gate switched off the generated frame was shown %d time(s) and the fake counted %ld read(s) of GEN with no answer (1 and 1 wanted): the control shows nothing", cg.gen, cg.unanswered);
    Ctl("a build with the helper gone and the read on the worker saying yes: the new set's GEN, which nothing wrote, landed %ld time(s) (the screen: %s); the read behind the gate switched off: the generated image of a helper that was gone went to the screen (%d), counted %ld", cb.extra, cb.tail.c_str(), cg.gen, cg.unanswered);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M17, a build with a pair armed, the worker held
// WHAT IT SHOWS (lifecycle-safety.md, LS-12, with check-machine.md's R19; machine.md, machine-10; the main session's ruling
// on READY-FOR-7C.md's question 1: the product calls BeforeBuild at the build's head, the drop stays as the fallback, and
// nothing is ever presented from the drained render thread). M7 runs both roads drained; here the worker is HELD:
//   (a) BeforeBuild AHEAD OF THE DRAIN. The pair is armed and the game is already drawing the next frame; the worker is
//       held with that drawing (a fire point) and the build's drain note queued behind it, and the build's own drain
//       replays them: nothing was on the screen while it was held; then the real frame is, BEFORE the quiesce, which
//       finds nothing armed; the back buffer is the game's drawing still; HOLD and SAVE are the same images after the
//       build; the copies on the queue have retired before the set goes; and the new set's first pair is an engage's
//       first, the second shows.
//   (b) WITHOUT IT. The same hold with no drain note: the build's drain is not a fire point, the quiesce finds the pair
//       ARMED and ends it: dropped, counted with its reason, never presented by the render thread; dues written for it
//       after the build fire nothing.
//   (c) THE RETIRE WAIT, WHICH THE HARNESS'S BUILD DID NOT HAVE (READY-FOR-7.md, section 6, point 6). A pair's hand span
//       can stand on the queue BEHIND THE QUEUE'S WAIT with a copy into the helper's PRESENT in it: the pair before
//       dispatched nothing (the HDR view), so nothing is pending and this pair has a source, while the helper is still
//       in that pair's prepare. The build comes: its drain note brings the pair to its real frame (taken by the queue
//       too), the quiesce, and THE BUILD WAITS, as the product's does (fg_product.h, FgpBeforeRelease), until what the
//       queue took has retired; the helper answers a quarter second in; only then do the set's images go, and no copy
//       names an image of a set that is gone.
// CONTROL: (c) with no retire wait (the harness's build as it stood): the set goes with that span on the queue, and when
// the queue runs its copy names an image of the released set: the fake counts it.
static volatile LONG gM17Seen = -1;
static void M17Look() { gM17Seen = (LONG)RingPresentedLog().size(); }
struct M17Q { bool reached, retired, exact; LONG held, setDead; ULONGLONG waitedMs; std::string tail; };
static M17Q M17Queue(bool retire)
{
    M17Q g = {};
    MgFresh<MA>();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueN();
    B::hdrView = true; _InterlockedExchange(&gH.holdPrep, 1);
    const DWORD tagX = ShFrame<MA>(true, false);              // X: the HDR view withholds it from the history, and the helper is held in its prepare
    B::hdrView = false;
    g.reached = WaitFor([] { return gH.inPrep != 0; }, 3000);
    const size_t sh0 = MgShownN();
    ClockAdd(16.7); ShInject(true); Drain();                  // Y's injection: X's real frame forced ahead, the wait queued behind X's prepare
    const DWORD tagY = Frame<MA>(); Drain();                  // Y: it HAS a source (X dispatched nothing), and its span is on the queue behind the wait
    g.held = (LONG)ShB::heldCbs.size();
    g.reached = g.reached && s.w.openPhase == FG_PAIR_ARMED && gH.inPrep != 0 && ShKindOf(RingPipeLog(), ShB::x.w.lastQN) == (int)FG_G_DISPATCH;
    const LONG dead0 = gRingSt.setDead;
    Later l = { [] { return true; }, [] { ShRelease(); }, 10, 250 };   // the helper answers a quarter second into the build
    const HANDLE hl = retire ? Spawn(LaterMain, &l) : nullptr;
    MA::BeforeBuild(); Drain();                               // THE BUILD: its drain note (Y's real frame, taken by the queue behind the wait too),
    MA::BeforeRelease();                                      // the quiesce,
    const ULONGLONG t0 = GetTickCount64();
    g.retired = retire ? ShArmB::RetireWait(3000) : true;     // THE RETIRE WAIT,
    g.waitedMs = GetTickCount64() - t0;
    ShB::WriteB();                                            // and the set's images go
    if (!retire) ShRelease();                                 // (the control: only now does the helper answer, and the queue runs)
    if (hl) Join(hl, "M17's release of the helper");
    if (!WaitFor([] { return ShB::heldCbs.empty(); }, 3000)) Err("M17: the queue did not run after the helper's release");
    if (!ShSettle()) Err("M17: the fake helper did not come back");
    g.setDead = gRingSt.setDead - dead0;
    const DWORD t1 = ShFrame<MA>(); MgDueShow(); MgDueN();
    const DWORD t2 = ShFrame<MA>(); MgDueShow(); MgDueN();
    g.exact = MgExactly(sh0, { tagX, tagY, t1, ShGenTag(t2, false), t2 }, &g.tail);
    return g;
}
static void CaseM17()
{
    const char* const name = "M17 a build with a pair armed, the worker held: the drain note ahead of the drain, the drop without it, the retire wait (stage 7c)";
    CaseBegin(name);
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    // (a)
    const DWORD tagA = ShFrame<MA>();
    const bool armedA = s.w.openPhase == FG_PAIR_ARMED;
    const size_t sh0 = MgShownN(); const int hold0 = RingRig(FG_IMG_HOLD), save0 = RingRig(FG_IMG_SAVE);
    const DWORD half = NewTag(); gM17Seen = -1;
    MgHeld([&] { Fill(half); MA::BeforeBuild(); }, &M17Look); // (the drain behind the hold is the build's own)
    const int before = MgTagCount(sh0, tagA); const bool armedAtQuiesce = s.w.openPhase == FG_PAIR_ARMED; const DWORD bbA = MgBb();
    MA::BeforeRelease();
    const bool retiredA = ShArmB::RetireWait(2000);
    ShB::WriteB();
    if (!ShSettle()) Err("M17: the helper did not read the build");
    const bool imagesSame = RingRig(FG_IMG_HOLD) == hold0 && RingRig(FG_IMG_SAVE) == save0 && !ShImgGone(hold0) && !ShImgGone(save0);
    const LONG droppedA = a.w.dropped[FG_AD_BUILD]; const int byRenderA = MgShownBy(sh0, renderTid);
    const DWORD t1 = ShFrame<MA>(); FgPub v1 = {}; const bool first1 = MgPub(MgP(), &v1) && (v1.flags & FG_HF_FIRST) != 0; MgDueShow(); MgDueN();
    const DWORD t2 = ShFrame<MA>(); MgDueShow(); MgDueN();
    std::string tailA; const bool orderA = MgExactly(sh0, { tagA, t1, ShGenTag(t2, false), t2 }, &tailA);
    printf("  (a) pair %lX armed (%s), the game drawing %lX, the worker held with that drawing and the build's drain note behind it: %ld picture(s) on the screen while held; after the drain the real frame %d time(s), still armed at the quiesce: %s, the back buffer %lX; the copies retired %s; HOLD and SAVE the same images: %s; dropped %ld; presented by the render thread %d; the new set's first hand FIRST %d; the screen: %s\n",
        tagA, armedA ? "yes" : "NO", half, (long)(gM17Seen < 0 ? -1 : gM17Seen - (LONG)sh0), before, armedAtQuiesce ? "YES" : "no", bbA, retiredA ? "yes" : "NO", imagesSame ? "yes" : "NO", droppedA, byRenderA, (int)first1, tailA.c_str());
    if (!armedA || gM17Seen != (LONG)sh0 || before != 1 || armedAtQuiesce || bbA != half) Err("M17 (a): armed %d; %ld picture(s) while held (0 wanted); after the build's drain the real frame was on the screen %d time(s) (1 wanted), the pair still armed %d, the back buffer %lX (the game's drawing %lX wanted)", (int)armedA, (long)(gM17Seen - (LONG)sh0), before, (int)armedAtQuiesce, bbA, half);
    if (!retiredA || !imagesSame || droppedA != 0 || byRenderA != 0 || !first1 || !orderA) Err("M17 (a): the copies retired %d, HOLD and SAVE the same %d, dropped %ld, presented by the render thread %d, the new set's first hand FIRST %d, the screen %s (yes, yes, 0, 0, yes and %lX %lX %lX %lX wanted)", (int)retiredA, (int)imagesSame, droppedA, byRenderA, (int)first1, tailA.c_str(), tagA, t1, ShGenTag(t2, false), t2);
    // (b)
    const DWORD tagB = ShFrame<MA>();
    const bool armedB = s.w.openPhase == FG_PAIR_ARMED;
    const size_t sh1 = MgShownN(); const DWORD half2 = NewTag();
    MgHeld([&] { Fill(half2); });
    const bool armedAtQuiesceB = s.w.openPhase == FG_PAIR_ARMED;
    MA::BeforeRelease();
    const bool armedAfterB = s.w.openPhase == FG_PAIR_ARMED; const LONG droppedB = a.w.dropped[FG_AD_BUILD] - droppedA;
    const bool retiredB = ShArmB::RetireWait(2000);
    ShB::WriteB();
    if (!ShSettle()) Err("M17: the helper did not read the second build");
    MgDueShow(); MgDueN();                                    // (dues for the pair that is gone: nothing may fire)
    const int shownB = MgTagCount(sh1, tagB); const size_t firedB = MgShownN() - sh1; const int byRenderB = MgShownBy(sh1, renderTid);
    ShFrame<MA>(); MgDueN(); ShFrame<MA>(); MgDueShow(); MgDueN();
    printf("  (b) pair %lX armed (%s), no drain note, the worker held with the game's drawing behind it: at the quiesce it is %s; after it %s, dropped for a build %ld; the copies retired %s; dues written for it afterwards fired %zu picture(s); the frame on the screen %d time(s); presented by the render thread %d\n",
        tagB, armedB ? "yes" : "NO", armedAtQuiesceB ? "still armed" : "NOT ARMED", armedAfterB ? "STILL ARMED" : "ended", droppedB, retiredB ? "yes" : "NO", firedB, shownB, byRenderB);
    if (!armedB || !armedAtQuiesceB || armedAfterB || droppedB != 1 || !retiredB || firedB != 0 || shownB != 0 || byRenderB != 0) Err("M17 (b): armed %d, at the quiesce %d, after it %d, dropped for a build %ld, retired %d, fired afterwards %zu, shown %d, by the render thread %d (yes, yes, no, 1, yes, 0, 0 and 0 wanted)", (int)armedB, (int)armedAtQuiesceB, (int)armedAfterB, droppedB, (int)retiredB, firedB, shownB, byRenderB);
    MgClean("M17");
    MgEnd();
    // (c)
    const M17Q q = M17Queue(true);
    MgClean("M17 (c)");
    MgEnd();
    printf("  (c) a pair's span with a copy into the helper's PRESENT on the queue behind the wait (%ld submit(s) held) when the build came: the build waited %llu ms for them to retire (%s), then the set went: copies naming an image of the released set %ld; the screen: %s\n",
        q.held, (unsigned long long)q.waitedMs, q.retired ? "retired" : "NOT RETIRED", q.setDead, q.tail.c_str());
    if (!q.reached || q.held < 1 || !q.retired || q.waitedMs < 100 || q.setDead != 0 || !q.exact) Err("M17 (c): reached %d, %ld submit(s) held, retired %d after %llu ms (the helper answered a quarter second in), %ld cop(ies) naming a released set's image (0 wanted), the screen %s", (int)q.reached, q.held, (int)q.retired, (unsigned long long)q.waitedMs, q.setDead, q.tail.c_str());
    // CONTROL
    const RingStats stc = gRingSt;
    const M17Q c = M17Queue(false);
    ViolRestore(stc);
    MgEnd();
    if (!c.reached || c.setDead != 1) Err("M17 control: with no retire wait the fake counted %ld cop(ies) naming an image of the released set (1 wanted; reached %d): the control shows nothing", c.setDead, (int)c.reached);
    Ctl("the build with no retire wait: the set went with a span on the queue, and its copy named an image of the released set: counted %ld", c.setDead);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M18, the set that ends itself, with a pair armed
// WHAT IT SHOWS (lifecycle-safety.md, LS-18; the wire's B13; READY-FOR-7C.md, section 3, item 5). The helper answers every
// dispatch with no frame. Each of those pairs ARMS and shows nothing generated, though its generated frame is asked for
// (the answer read on the worker is not "a frame"), and its real frame goes out in its place. THE THIRD ANSWER IS READ
// BY A HAND WITH THE WORKER HELD and the whole frame queued behind it, the third pair STILL ARMED: nothing is on the
// screen while it is held; released, the armed pair's real frame comes first (forced by that frame's injection under rule
// C1, counted early; by the drain note that heads that frame's present under C3, counted late), the hand then finds
// nothing armed, reads the third answer, and frame generation ends itself for the set, once, with one
// line, the frame whose hand read it is passed through, and a 'Q' its injection wrote early is closed CLOSED. Three
// frames more: passed through, no hand recorded, nothing written. Every real frame once, in order, and nothing generated.
// CONTROL: two dispatches with no frame and then one with: nothing ends, and the frames behind go on as pairs.
static volatile LONG gM18Seen = -1;
static void M18Look() { gM18Seen = (LONG)RingPresentedLog().size(); }
struct M18Leg { bool exact, realOnce; std::string tail, bad; int gen; LONG down, none, wroteAfter, handsAfter, noAnswer, handOpen, early, late; int lines; size_t whileHeld; };
static M18Leg M18Run(bool c3, int nNone)
{
    M18Leg g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    if (c3) gC1Off = 1;
    ShWarm<MA>(); MgDueN();
    const size_t sh0 = MgShownN(); std::vector<DWORD> want;
    const int lines0 = B::lines; const LONG none0 = x.w.ansNone;
    _InterlockedExchange(&gH.noFrame, 1);
    for (int i = 0; i < 3; i++)
    {
        if (i == nNone) _InterlockedExchange(&gH.noFrame, 0);
        want.push_back(ShFrame<MA>());
        if (i < 2) { MgDueShow(); MgDueN(); }                 // (the third pair stays ARMED, its generated frame's due planted)
    }
    DueI(MgP(), FG_DUE_SHOW);
    const LONG ho0 = s.c.handOpen, e0 = s.c.nEarly, l0 = s.c.nLate;
    gM18Seen = -1; DWORD t1 = 0;
    MgHeld([&] { ClockAdd(16.7); ShInject(true); t1 = Frame<MA>(); }, &M18Look);
    want.push_back(t1);
    g.whileHeld = gM18Seen < 0 ? 99 : (size_t)gM18Seen - sh0 - 2;   // (two real frames were on the screen before the hold)
    g.handOpen = s.c.handOpen - ho0; g.early = s.c.nEarly - e0; g.late = s.c.nLate - l0;
    if (!ShSettle()) Err("M18: the fake helper did not read the held frame's messages");
    const size_t pipe1 = RingPipeLog().size(); const LONG hands1 = s.c.hands;
    for (int i = 0; i < 3; i++)
    {
        ClockAdd(16.7); ShInject(true); want.push_back(Frame<MA>()); Drain();
        if (!ShSettle()) Err("M18: the fake helper did not settle");
        if (s.w.openPhase == FG_PAIR_ARMED) { MgDueShow(); MgDueN(); }
    }
    _InterlockedExchange(&gH.noFrame, 0);
    g.wroteAfter = (LONG)(RingPipeLog().size() - pipe1); g.handsAfter = s.c.hands - hands1;
    g.down = x.w.standDowns; g.none = x.w.ansNone - none0; g.lines = B::lines - lines0; g.noAnswer = a.w.iWhy[FG_AW_NOANSWER];
    g.exact = MgExactly(sh0, want, &g.tail); g.gen = MgGenCount(sh0);
    g.realOnce = true; for (DWORD t : want) if (MgTagCount(sh0, t) != 1) g.realOnce = false;
    ShPairs(RingPipeLog(), &g.bad);
    return g;
}
static void CaseM18()
{
    const char* const name = "M18 the set that ends itself, with a pair armed: its real frame first, then the frame that read the third answer (stage 7c)";
    CaseBegin(name);
    for (int c3 = 0; c3 < 2; c3++)
    {
        const M18Leg g = M18Run(c3 != 0, 3);
        printf("  rule %s: three dispatches answered with no frame, the third pair armed, the worker held with the next frame behind it: %zu picture(s) while held; the armed pair's real frame forced early (the injection) %ld, late (the drain note) %ld, the hand found a pair armed %ld time(s); frame generation ended itself %ld time(s) after %ld such answers, in %d line(s); then over three frames %ld message(s) and %ld hand(s); the screen: %s; generated images %d\n",
            c3 ? "C3" : "C1", g.whileHeld, g.early, g.late, g.handOpen, g.down, g.none, g.lines, g.wroteAfter, g.handsAfter, g.tail.c_str(), g.gen);
        if (g.whileHeld != 0 || !g.exact || g.gen != 0) Err("M18 (%s): %zu picture(s) reached the screen while the worker was held (0 wanted); the screen showed %s (seven real frames, each once, in order, wanted); generated images %d", c3 ? "C3" : "C1", g.whileHeld, g.tail.c_str(), g.gen);
        if (g.down != 1 || g.none != 3 || g.lines != 1 || g.wroteAfter != 0 || g.handsAfter != 0) Err("M18 (%s): frame generation ended itself %ld time(s) after %ld answers with no frame, in %d line(s); afterwards %ld message(s) and %ld hand(s) (1, 3, 1, 0 and 0 wanted)", c3 ? "C3" : "C1", g.down, g.none, g.lines, g.wroteAfter, g.handsAfter);
        if (g.early != (c3 ? 0 : 1) || g.late != (c3 ? 1 : 0) || g.handOpen != 0 || g.noAnswer != 1) Err("M18 (%s): the armed pair's real frame was forced early %ld time(s) and late %ld (%d and %d wanted), the hand found a pair armed %ld time(s) (0 wanted), and %ld generated frame(s) asked for were refused by the read on the worker (1 wanted: the first pair's; the two behind it may show nothing)", c3 ? "C3" : "C1", g.early, g.late, c3 ? 0 : 1, c3 ? 1 : 0, g.handOpen, g.noAnswer);
        if (!g.bad.empty()) Err("M18 (%s): %s", c3 ? "C3" : "C1", g.bad.c_str());
        MgClean("M18");
        MgEnd();
    }
    const M18Leg c = M18Run(false, 2);
    MgClean("M18 control");
    MgEnd();
    if (c.down != 0 || c.none != 2 || c.handsAfter != 3 || !c.realOnce) Err("M18 control: two dispatches with no frame and then one with: ended %ld time(s), %ld answers with none read, %ld hand(s) over the three frames behind, the screen %s (0, 2, 3 and every real frame once wanted, generated images apart): the control shows nothing", c.down, c.none, c.handsAfter, c.tail.c_str());
    Ctl("two dispatches with no frame and then one with a frame: nothing ended (%ld answers with none read), and the three frames behind went on as pairs (%ld hands)", c.none, c.handsAfter);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M19, the mirror's own control
// WHAT IT SHOWS (machine.md, machine-8, in its own words: "the fake helper records 'this dispatch reset' by the real
// helper's two lines; the machine records 'prime'; the two are equal for every pair. Control: the mirror without 'the
// answer was not a frame', which must differ on a no-frame leg"). Sixteen frames, each recorded note by note with its
// generated frame asked for; the helper answers "no frame" for the third, the eighth and the ninth. For every dispatch
// that gave a frame, "it reset" by the helper's own record equals "fed, never shown for a reset" as the worker decided
// it; three hands read an answer that was not a frame; and the fake counts no landing of a reset dispatch's picture.
// CONTROL, AS THE MAP WORDS IT: the same frames with that one clause of the mirror undone (a note right behind each hand
// that read such an answer clears what the clause set): the helper's record is the same, the worker's differs from it
// on exactly the two dispatches behind a "no frame" that gave a frame, and each of those pictures, a reset dispatch's,
// goes to the screen: the fake counts both. (M4's control overrules the mirror whole; the census compares the two over
// thousands of dispatches; this is the clause alone.)
static volatile LONG gM19None = 0, gM19Hit = 0;
static void M19Pre(DWORD, DWORD, DWORD) { gM19None = ShB::x.w.ansNone + ShB::x.w.ansUnknown; }
static void M19NoClause(DWORD, DWORD, DWORD undo)
{
    if (ShB::x.w.ansNone + ShB::x.w.ansUnknown == gM19None) return;
    _InterlockedIncrement(&gM19Hit);
    if (undo) ShArmB::a.w.primeNext = 0;
}
struct M19Leg { std::string recs; int mirrorBad, resetShown; LONG genReset, hits; };
static M19Leg M19Run(bool without)
{
    M19Leg g = {};
    MgFresh<MA>();
    FgShadowState& x = ShB::x;
    ShWarm<MA>(); MgDueN();
    const size_t disp0 = ShDispLog().size(), sh0 = MgShownN(); const LONG r0 = gRingSt.genReset;
    std::map<LONG, LONG> whyOfN;
    gM19Hit = 0;
    for (int i = 0; i < 16; i++)
    {
        _InterlockedExchange(&gH.noFrame, (i == 2 || i == 7 || i == 8) ? 1 : 0);
        MgRecBegin(); Run(M19Pre); MgRecHand(); Run(M19NoClause, without ? 1u : 0u); RecordHudless(); RecordPresent(); Drain();
        if (!ShSettle()) Err("M19: the fake helper did not read frame %d's messages", i);
        FgArmedPair rec = {}; if (MgPair(MgP(), &rec)) whyOfN[x.w.lastQN] = rec.why;
        MgDueShow(); MgDueN();
    }
    _InterlockedExchange(&gH.noFrame, 0);
    const auto disp = ShDispLog();
    for (size_t i = disp0; i < disp.size(); i++)
    {
        const ShDisp& d = disp[i];
        g.recs += d.reset ? (d.own ? 'O' : 'T') : 'N';
        const auto it = whyOfN.find(d.n);
        if (it == whyOfN.end() || (it->second == FG_AW_RESET) != d.reset) g.mirrorBad++;
    }
    const auto shown = Shown();
    for (size_t i = sh0; i < shown.size(); i++) g.resetShown += ShIsGenReset(shown[i].tag) ? 1 : 0;
    g.genReset = gRingSt.genReset - r0; g.hits = gM19Hit;
    return g;
}
static void CaseM19()
{
    const char* const name = "M19 the mirror's own control: without \"the answer was not a frame\" it differs from the helper's record (stage 7c)";
    CaseBegin(name);
    const M19Leg g = M19Run(false);
    MgClean("M19");
    MgEnd();
    printf("  sixteen frames, the helper's answer \"no frame\" for three of them: its dispatches that gave a frame (O reset on its own account, N none): %s; the worker's mirror differs on %d; hands that read an answer that was not a frame %ld; a reset dispatch's picture landed %ld time(s), on the screen %d\n",
        g.recs.c_str(), g.mirrorBad, g.hits, g.genReset, g.resetShown);
    if (g.recs != "NNONNNONNNNNN" || g.mirrorBad != 0 || g.hits != 3 || g.genReset != 0 || g.resetShown != 0) Err("M19: the helper's record reads %s (NNONNNONNNNNN wanted), the mirror differs on %d dispatch(es), %ld hand(s) read an answer that was not a frame, %ld landing(s) of a reset dispatch's picture, %d on the screen (0, 3, 0 and 0 wanted)", g.recs.c_str(), g.mirrorBad, g.hits, g.genReset, g.resetShown);
    const RingStats stc = gRingSt;
    const M19Leg c = M19Run(true);
    ViolRestore(stc);
    MgEnd();
    if (c.recs != g.recs || c.mirrorBad != 2 || c.genReset != 2 || c.resetShown != 2) Err("M19 control: without the clause the helper's record reads %s, the mirror differs on %d dispatch(es), and %ld reset dispatch's picture(s) were landed, %d on the screen (the same record, 2, 2 and 2 wanted): the control shows nothing", c.recs.c_str(), c.mirrorBad, c.genReset, c.resetShown);
    Ctl("the mirror without \"the answer was not a frame\": it differs from the helper's record on %d dispatch(es), the two behind a \"no frame\" that gave a frame, and each one's picture, a reset dispatch's, went to the screen (%d, counted %ld)", c.mirrorBad, c.resetShown, c.genReset);
    CaseEnd(name);
}

// ---------------------------------------------------------------- M20, a lost restore stands the machine down
// WHAT IT SHOWS (the main session's ruling on READY-FOR-7C.md's question 9; until stage 7c this was a leg of M8 that
// counted the lost restore and went on). A pair is armed and the game is drawing the next frame. Its generated frame's
// fire writes and presents, rightly, AND ITS RESTORE IS REFUSED BY THE QUEUE (the fire's second submit; the worker held,
// the fault and the fire's note queued behind it): the picture was shown, the restore is counted lost, and THE MACHINE
// IS DEAD FOR THE SESSION, said in one line: a queue that refuses a copy is not one to go on writing the back buffer
// through. The pair ends as a dead machine's pair ends: its real frame is dropped, counted, at the next present's close
// note. Three frames more are passed through, each once; no hand is recorded.
// CONTROL: the stand-down switched off: the machine stays live behind a lost restore, and the next frame ARMS a pair on it.
struct M20Leg { M8Leg f; LONG lost, state, dropped, hands; int lines, gen, real; bool exact, armedNext; std::string tail; };
static M20Leg M20Run(LONG guardOff)
{
    M20Leg g = {};
    MgFresh<MA>();
    gArmedGuardOff = guardOff;
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN(); ShFrame<MA>(); MgDueN();
    const int lines0 = B::lines; const size_t sh0 = MgShownN();
    g.f = M8Fire(true, 1);                                    // the generated frame's fire, its restore refused
    g.lost = a.w.restoreLost; g.state = s.sh.state; g.lines = B::lines - lines0;
    g.gen = MgTagCount(sh0, ShGenTag(g.f.tag, false));
    MgDueN();                                                 // (the pair's real frame, asked for)
    g.real = MgTagCount(sh0, g.f.tag);
    const size_t sh1 = MgShownN(); const LONG hands0 = s.c.hands;
    std::vector<DWORD> three;
    for (int i = 0; i < 3; i++) { ClockAdd(16.7); ShInject(); three.push_back(Frame<MA>()); Drain(); if (!i) g.armedNext = s.w.openPhase == FG_PAIR_ARMED; if (s.w.openPhase == FG_PAIR_ARMED) MgDueN(); }
    g.exact = MgExactly(sh1, three, &g.tail);
    g.dropped = a.w.dropped[FG_AD_DEAD]; g.hands = s.c.hands - hands0;
    a.w.restoreLost = 0;                                      // (asked for: MgClean holds every other case to none)
    gArmedGuardOff = 0;
    return g;
}
static void CaseM20()
{
    const char* const name = "M20 a lost restore stands the machine down for the session (stage 7c)";
    CaseBegin(name);
    const M20Leg g = M20Run(0);
    MgClean("M20");
    MgEnd();
    printf("  the generated frame's fire with its restore refused: %ld present(s), the generated frame on the screen %d time(s), restores lost %ld; the machine's state %ld (DEAD is %d), said in %d line(s); the pair's real frame on the screen %d time(s), dropped for a dead machine %ld; three frames more: %s, %ld hand(s) recorded\n",
        g.f.presents, g.gen, g.lost, g.state, (int)FG_DEAD, g.lines, g.real, g.dropped, g.tail.c_str(), g.hands);
    if (g.f.presents != 1 || g.gen != 1 || g.lost != 1 || g.f.refused != 1) Err("M20: the fire with its restore refused made %ld present(s), showed the generated frame %d time(s), lost %ld restore(s), %ld refused inside the span (1, 1, 1 and 1 wanted)", g.f.presents, g.gen, g.lost, g.f.refused);
    if (g.state != FG_DEAD || g.lines != 1) Err("M20: behind a lost restore the machine's state is %ld, said in %d line(s) (DEAD, one line, wanted)", g.state, g.lines);
    if (g.real != 0 || g.dropped != 1 || !g.exact || g.hands != 0 || g.armedNext) Err("M20: the pair's real frame was on the screen %d time(s) and dropped for a dead machine %ld time(s); three frames more showed %s with %ld hand(s), the first of them armed %d (0, 1, each once in order, 0 and no wanted)", g.real, g.dropped, g.tail.c_str(), g.hands, (int)g.armedNext);
    const RingStats stc = gRingSt;
    const M20Leg c = M20Run(4);
    ViolRestore(stc);
    MgEnd();
    if (c.lost != 1 || c.state != FG_LIVE || !c.armedNext) Err("M20 control: with the stand-down switched off a lost restore (%ld) left the machine's state %ld and the next frame armed %d (1, live and yes wanted): the control shows nothing", c.lost, c.state, (int)c.armedNext);
    Ctl("the stand-down switched off: behind a lost restore the machine stayed live (state %ld) and the next frame ARMED a pair on it (%d)", c.state, (int)c.armedNext);
    CaseEnd(name);
}

// ================================================================================================================
// STAGE 7d: THE FRAME END, THE TAIL, THE WANTS (2026-10-10; critic.md's stage 7, part d, and its section 5: "a standby
// present costs what shadow's did"; machine.md's machine-11 and machine-12; pacing.md's PAC-4, PAC-6 and PAC-7;
// check-machine.md's X4, X7 and R10; READY-FOR-7C.md, section 3, items 8 to 10, 12 and 13; and the main session's rulings
// on its questions 5 (lever 4's key is refused in a binding that arms) and 6 (the least lead of a generated frame is a
// number handed in).
// ================================================================================================================

// ---------------------------------------------------------------- T1, a standby present costs one clock read
// WHAT IT SHOWS (the critic's section 5, the harness block's last line, NOT PROVED until this stage; machine.md, machine-12,
// with check-machine.md's X7). Two shapes in standby (the want is PASS), forty presents each, FgSwapPresent called with
// the counters on: THE LEAN MERGED SHAPE (it arms and is not an instrument's: the product) and the merged binding that IS
// an instrument's, which keeps the rig's whole tail. COUNTED, A PRESENT: reads of the clock, reads of the sleeper count,
// waits of the pacer's, records written into the ownership ring and into the present's record ring, the pace wait's
// census, and the dwords the render thread recorded into the device thread's ring. THE LEAN SHAPE'S: one clock read (the
// entry), and nothing else of these (until round 9's strip held to the shadow shape's, read in the same run, which were
// exactly these).
// AND TIMED, the original present taken out (a stub in its place, so what is timed is FgSwapPresent's own work for a
// standby present; the real clock; the best of seven rounds of 200,000): the lean shape is cheaper than the instrument's tail.
// It sorts the ten period samples at every present (PublishApplied), phase 6's.
// CONTROL: the instrument's shape: six clock reads, two reads of the sleeper count and two ring records a present.
// ONCE, NOT IN BOTH ORDERS OF 'Q': a standby present writes none.
struct T1Cost { LONG nowReads, slept, paceWaits, ownWrites, presWrites, census; DWORD dwords; double ns; int n; };
static HRESULT __stdcall T1Orig(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD) { return S_OK; }
template <class MM> static T1Cost T1Run()
{
    T1Cost c = {}; c.dwords = 0xFFFFFFFFu; c.n = 40;
    MgFresh<MM>();
    FgState& s = B::S();
    ShWarm<MM>();
    s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 3; i++) ShFrame<MM>();                // (the first of them carries the close note: standby is what follows)
    const LONG census0 = s.c.paceWaitN;
    for (int i = 0; i < c.n; i++)
    {
        ClockAdd(16.7); ShInject(true);
        Facts(true); Fill(NewTag());
        Drain();
        const LONG f = B::frames - 1;
        const DWORD w0 = dtW;
        ShB::cntNow = ShB::cntSlept = ShB::cntPaceWait = 0; ShB::countTid = GetCurrentThreadId();
        MM::SwapPresent(SC, nullptr, nullptr, nullptr, nullptr, 0);
        ShB::countTid = 0;
        const DWORD w1 = dtW;
        if (w1 >= w0 && w1 - w0 < c.dwords) c.dwords = w1 - w0;   // (a present whose record wrapped the ring is left out: the least of forty)
        c.nowReads += ShB::cntNow; c.slept += ShB::cntSlept; c.paceWaits += ShB::cntPaceWait;
        c.ownWrites += ReadAcquire(&s.own[f & (FG_STAMPS - 1)].f1) == f ? 1 : 0;
        c.presWrites += ReadAcquire(&s.pres[f & (FG_STAMPS - 1)].f1) == f ? 1 : 0;
        Drain();
    }
    c.census = s.c.paceWaitN - census0;
    // THE TIME, the original present taken out, on the real clock
    B::fakeClock = false;
    void* const keep = s.ptr.origSwapPresent; s.ptr.origSwapPresent = (void*)&T1Orig;
    c.ns = 1e18;
    for (int round = 0; round < 7; round++)
    {
        LARGE_INTEGER q0, q1; QueryPerformanceCounter(&q0);
        for (int i = 0; i < 200000; i++) MM::SwapPresent(SC, nullptr, nullptr, nullptr, nullptr, 0);
        QueryPerformanceCounter(&q1);
        const double ns = (double)(q1.QuadPart - q0.QuadPart) * 1e9 / (double)B::Qpf() / 200000.0;
        if (ns < c.ns) c.ns = ns;
    }
    s.ptr.origSwapPresent = keep; B::fakeClock = true;
    return c;
}
static void CaseT1()
{
    const char* const name = "T1 a standby present of the merged shape costs one clock read and nothing of the pacer's (stage 7d)";
    CaseBegin(name);
    const T1Cost me = T1Run<MA>(); MgClean("T1"); MgEnd();
    const T1Cost in = T1Run<MAR>(); MgEnd();
    static const char* const kName[2] = { "the lean merged shape", "the merged instrument's shape" };
    const T1Cost* const all[2] = { &me, &in };
    for (int i = 0; i < 2; i++)
        printf("  %-34s over %d standby presents: clock reads %ld, reads of the sleeper count %ld, waits of the pacer's %ld, ownership records %ld, present records %ld, the pace wait's census %ld; dwords recorded a present %lu; FgSwapPresent's own time, the original present taken out: %.1f ns\n",
            kName[i], all[i]->n, all[i]->nowReads, all[i]->slept, all[i]->paceWaits, all[i]->ownWrites, all[i]->presWrites, all[i]->census, (unsigned long)all[i]->dwords, all[i]->ns);
    // (until round 9's strip the yardstick was the shadow shape's standby present, read in the same run: one clock read and
    // nothing else, the same dwords; that shape went, and its numbers are the lean shape's own now, held absolutely)
    if (me.nowReads != me.n || me.slept != 0 || me.paceWaits != 0 || me.ownWrites != 0 || me.presWrites != 0 || me.census != 0 || me.dwords > in.dwords)
        Err("T1: a standby present of the lean merged shape: clock reads %ld (one a present wanted), sleeper count reads %ld, pacer's waits %ld, ownership records %ld, present records %ld, census %ld (0 each wanted), dwords recorded %lu (no more than the instrument's %lu wanted)",
            me.nowReads, me.slept, me.paceWaits, me.ownWrites, me.presWrites, me.census, (unsigned long)me.dwords, (unsigned long)in.dwords);
    if (!(me.ns < in.ns)) Err("T1: FgSwapPresent's own time for a standby present: the lean merged shape %.1f ns, the instrument's tail %.1f: the lean shape is not the cheaper", me.ns, in.ns);
    if (in.nowReads < me.nowReads + 3 * in.n || in.slept != 2 * in.n || in.ownWrites != in.n || in.presWrites != in.n || in.census != in.n)
        Err("T1 control: the instrument's tail made %ld clock reads, %ld reads of the sleeper count, %ld ownership and %ld present records, census %ld over %d presents (at least four reads more a present, two, one, one and one a present wanted): the control shows nothing", in.nowReads, in.slept, in.ownWrites, in.presWrites, in.census, in.n);
    Ctl("the rig's whole tail (the merged instrument's shape): %ld clock reads, %ld reads of the sleeper count and %ld ring records over %d standby presents, %.1f ns a present where the lean shape takes %.1f", in.nowReads, in.slept, in.ownWrites + in.presWrites, in.n, in.ns, me.ns);
    CaseEnd(name);
}

// ---------------------------------------------------------------- T2, the wants
// WHAT IT SHOWS (the build map's X8; machine.md, machine-5 and machine-12; pacing.md, PAC-6 and PAC-7; check-machine.md, X4;
// READY-FOR-7.md, section 2, point 4). Beside PASS and ON the merged path knows a want that does one thing and not the other
// (SHADOW, the wire alone, went in round 9's S8: no build of the product asks it):
//   CAP, THE PACER ALONE. Four frames behind engaged ones: no hand is recorded, NOTHING is written to the pipe (the first
//     of them has an injection that ran while the last hand was an engaged one: with 'Q' behind the body the worker asks
//     the want there, and CAP writes none), nothing is copied, every present is passed through, once.
//   WHO SLEEPS, at a refresh the pacer owns (500 Hz: C is 4.06 ms; a frame 1 ms behind the last release): under ON and under CAP FgPace sleeps
//     the present, ONE sleeper, at one period; under PASS it does not run at all.
// CONTROLS: the same six frames under ON: every pair arms, HOLD is written, the machine owns every frame's end and the
// saved observer signals none. And both sleepers forced under ON (the latency pacer's, which the session parks): the
// count sees two.
struct T2Six { std::string seq, tail; bool exact; LONG presents, inl, armed, latObs, kN, hands; int owned, present, dispatch; DWORD holdBefore, holdAfter; };
static T2Six T2SixRun(LONG want, std::vector<DWORD>* tags)
{
    T2Six g = {};
    FgState& s = B::S(); FgShadowState& x = ShB::x; FgArmedState& a = ShArmB::a;
    s.wt.want = want;
    ShFrame<MA>(); MgPlace<MA>();                             // (the first frame under this want: the pair before it is done by its injection)
    if (want == FG_WANT_ON) MgDueN();
    const size_t sh0 = MgShownN(); g.holdBefore = RingTag(RingRig(FG_IMG_HOLD));
    const LONG p0 = gRingSt.presents, inl0 = s.c.presentsInline, armed0 = s.c.pubArmed, lat0 = gLatObsCalls, k0 = x.pl.kN, hands0 = s.c.hands;
    for (int i = 0; i < 6; i++)
    {
        const size_t sq0 = ShB::seq.size();
        const DWORD t = ShFrame<MA>(); tags->push_back(t);
        if (!i) g.seq.assign(ShB::seq.begin() + sq0, ShB::seq.end());
        g.owned += MA::OwnsEnd(B::frames - 1) ? 1 : 0;
        g.present += RingTag(RingRig(FG_IMG_PRESENT)) == t ? 1 : 0;
        g.dispatch += ShKindOf(RingPipeLog(), x.w.lastQN) == (int)FG_G_DISPATCH ? 1 : 0;
        MgPlace<MA>();
        if (want == FG_WANT_ON) MgDueN();
    }
    g.holdAfter = RingTag(RingRig(FG_IMG_HOLD));
    g.presents = gRingSt.presents - p0; g.inl = s.c.presentsInline - inl0; g.armed = s.c.pubArmed - armed0; g.latObs = gLatObsCalls - lat0;
    g.kN = x.pl.kN - k0; g.hands = s.c.hands - hands0;
    g.exact = MgExactly(sh0, *tags, &g.tail);
    return g;
}
struct T2Pace { LONG s1, s2, waits; double period; };
static void CaseT2()
{
    const char* const name = "T2 the wants: CAP paces and speaks none (stage 7d)";
    CaseBegin(name);
    MgFresh<MA>();
    FgState& s = B::S(); FgShadowState& x = ShB::x;
    ShWarm<MA>(); MgDueN();
    ShFrame<MA>(); MgDueShow(); MgDueN();
    // CAP
    s.wt.want = FG_WANT_CAP;
    const size_t pipe0 = RingPipeLog().size(), sh2 = MgShownN();
    const LONG hands0 = s.c.hands, sub0 = gRingSt.submits, pass0 = s.c.presentsPass, wr0 = x.e.written, nh0 = x.e.noHand;
    std::vector<DWORD> tagsC;
    for (int i = 0; i < 4; i++) { ClockAdd(16.7); ShInject(true); tagsC.push_back(Frame<MA>()); Drain(); }
    if (!ShSettle()) Err("T2: the fake helper did not settle under CAP");
    std::string tailC; const bool exactC = MgExactly(sh2, tagsC, &tailC);
    const size_t wroteC = RingPipeLog().size() - pipe0;
    const LONG handsC = s.c.hands - hands0, subC = gRingSt.submits - sub0, passC = s.c.presentsPass - pass0, earlyC = x.e.written - wr0, noHandC = x.e.noHand - nh0;
    printf("  CAP, four frames behind an engaged one: hands recorded %ld, messages written %zu ('Q' written at an injection %ld, closed with no hand %ld), submits %ld, presents passed through %ld; the screen: %s\n",
        handsC, wroteC, earlyC, noHandC, subC, passC, exactC ? "the four frames, each once" : tailC.c_str());
    if (handsC != 0 || wroteC != 0 || earlyC != 0 || noHandC != 0 || subC != 0 || passC != 4 || !exactC) Err("T2: four CAP frames recorded %ld hand(s), wrote %zu message(s) (%ld 'Q' at an injection, %ld closed with no hand), made %ld submit(s), passed %ld present(s) through, and the screen showed %s (0, 0, 0, 0, 0, 4 and the four frames each once wanted)", handsC, wroteC, earlyC, noHandC, subC, passC, tailC.c_str());
    // WHO SLEEPS, at a refresh the pacer owns: a frame comes 1 ms behind the last release, on the harness's clock
    B::refreshPacked = (500ull << 32) | 1ull;
    auto pace = [&](LONG want, bool both)
    {
        T2Pace g = {};
        s.wt.want = want;
        for (int i = 0; i < 4; i++) { ClockAdd(1.0); ShInject(true); Frame<MA>(); Drain(); }
        const LONG a1 = s.r.sleeps1, a2 = s.r.sleeps2;
        ShB::cntPaceWait = 0; ShB::countTid = GetCurrentThreadId();
        if (both) gLatOnPresentHook = LatPaceSleeps;
        for (int i = 0; i < 20; i++) { ClockAdd(1.0); ShInject(true); Frame<MA>(); Drain(); }
        gLatOnPresentHook = nullptr; ShB::countTid = 0;
        g.s1 = s.r.sleeps1 - a1; g.s2 = s.r.sleeps2 - a2; g.waits = ShB::cntPaceWait; g.period = s.r.period;
        return g;
    };
    const T2Pace pOn = pace(FG_WANT_ON, false), pCap = pace(FG_WANT_CAP, false), pPass = pace(FG_WANT_PASS, false), pBoth = pace(FG_WANT_ON, true);
    const double cMs = MA::CPeriodMs();
    B::refreshPacked = 0; s.wt.want = FG_WANT_PASS;
    for (int i = 0; i < 2; i++) { ClockAdd(16.7); ShInject(true); Frame<MA>(); Drain(); }
    if (!ShSettle()) Err("T2: the fake helper did not settle behind the paced frames");
    printf("  who sleeps, twenty presents each at C = %.2f ms: ON one sleeper %ld, two %ld, the pacer's waits %ld, period %.2f ms; CAP %ld, %ld, %ld, %.2f ms; PASS %ld, %ld, %ld\n",
        cMs, pOn.s1, pOn.s2, pOn.waits, pOn.period, pCap.s1, pCap.s2, pCap.waits, pCap.period, pPass.s1, pPass.s2, pPass.waits);
    if (pOn.s1 < 15 || pOn.s2 != 0 || pOn.waits < 15 || pCap.s1 < 15 || pCap.s2 != 0 || pCap.waits < 15) Err("T2: under ON FgPace slept %ld of 20 presents alone (%ld with two sleepers, %ld waits) and under CAP %ld (%ld, %ld): at least 15, 0 and 15 wanted for each", pOn.s1, pOn.s2, pOn.waits, pCap.s1, pCap.s2, pCap.waits);
    if (pOn.period != pCap.period || pOn.period < cMs - 1e-9) Err("T2: FgPace's period under ON is %.3f ms and under CAP %.3f (one period, never under C = %.3f, wanted)", pOn.period, pCap.period, cMs);
    if (pPass.s1 || pPass.s2 || pPass.waits) Err("T2: under PASS the sleeper count moved %ld and %ld with %ld wait(s) (nobody sleeps: 0 each wanted)", pPass.s1, pPass.s2, pPass.waits);
    MgClean("T2");
    MgEnd();
    // CONTROLS
    const RingStats stc = gRingSt;
    MgFresh<MA>();
    ShWarm<MA>(); MgDueN();
    std::vector<DWORD> tagsO;
    const T2Six on = T2SixRun(FG_WANT_ON, &tagsO);
    ViolRestore(stc);
    MgEnd();
    if (on.armed != 6 || on.owned != 6 || on.latObs != 0 || on.inl != 0 || on.holdAfter == on.holdBefore) Err("T2 control: the same six frames under ON: armed %ld, the frame's end owned %d time(s), the saved observer signalled %ld, presented inline %ld, HOLD %lX before and %lX after (6, 6, 0, 0 and written wanted): the control shows nothing", on.armed, on.owned, on.latObs, on.inl, on.holdBefore, on.holdAfter);
    if (pBoth.s2 < 15) Err("T2 control: both sleepers forced under ON and the count saw two on %ld of 20 presents (at least 15 wanted): the control shows nothing", pBoth.s2);
    Ctl("the same six frames under ON: armed %ld, the frame's end owned %d time(s), HOLD written; both sleepers forced under ON: the count saw two on %ld of 20 presents", on.armed, on.owned, pBoth.s2);
    CaseEnd(name);
}

// ---------------------------------------------------------------- T3, the frame end the merged path owns
// WHAT IT SHOWS (machine.md, machine-11; pacing.md, PAC-6; RT1, RT2 and F64 over the merged binding). The machine moves a
// frame's end from the present's own record to the real frame's fire, for the pairs it arms:
//   AN ARMED PAIR: owned; NOT signalled at the arm (the saved observer is not called for its ordinal), not by its
//     generated frame's fire; SIGNALLED BY THE REAL FRAME'S FIRE, behind its Present.
//   A PAIR PRESENTED INLINE (its injection was not sent): owned, and signalled by the machine at its own present.
//   A PRESENT PASSED THROUGH: not owned (though the lean shape writes "not owned" nowhere: T1), signalled by the saved observer.
//   A REAL FRAME THAT IS DROPPED (a build's quiesce finds the pair armed): owned, and NEVER signalled: the frame never
//     reached the screen, and the waiter's route counts its sample "not back".
//   THE ROUTE over the merged machine: RT1's replay of 4,000 waiter iterations against the old routing, exact; and RT2's
//     held leg with the merged path's own fire: the worker held with the real frame's fire queued behind it, the frame
//     end's stamper a thread: the sample is deferred at the first look and dropped "not back" at the sixth call, never
//     taken; released, the end is signalled and stamped.
// CONTROLS: the route's own (the old routing's mutant: the replay differs). And F64's shape: the present observer's
// suppression skipped for one pair OVER THE LEAN SHAPE, THE ONE THAT SHIPS (dt_fg_test.cpp's PresentObs_SkipAt20, a present
// observer standing in front of the machine's; until round 9's S9 the fault key's 64 over the merged binding that is an
// instrument's, which the lean shape did not read): that frame's end is signalled at the arm and the real frame's own is
// dropped by the guard.
template <class MM> static void T3Fault(LONG* skipped, LONG* dropped, int* endAtArm)
{
    MgFresh<MM>();
    FgState& s = B::S();
    ShWarm<MM>(); MgDueN();
    dtPresentObs = &PresentObs_SkipAt20<MM>;                  // (behind the engage's pointer check)
    const LONG sk0 = gSkipSuppressed, dr0 = gRingSt.latEndDropped; *endAtArm = 0;
    for (int i = 0; i < 24; i++)
    {
        ShFrame<MM>();
        if (s.w.openPhase == FG_PAIR_ARMED && B::TlValue(FG_TL_END) == (ULONGLONG)(B::frames - 1)) (*endAtArm)++;
        MgDueN();
    }
    dtPresentObs = &MM::PresentObs;
    *skipped = gSkipSuppressed - sk0; *dropped = gRingSt.latEndDropped - dr0;
}
static void CaseT3()
{
    const char* const name = "T3 the frame end the merged path owns: at the real frame's fire, the route, and the instrument's fault (stage 7d)";
    CaseBegin(name);
    MgFresh<MA>();
    FgState& s = B::S();
    ShWarm<MA>(); MgDueN();
    const LONG lat0 = gLatObsCalls;
    ShFrame<MA>(); const LONG fA = B::frames - 1;
    const bool ownedA = MA::OwnsEnd(fA); const LONG endArm = (LONG)B::TlValue(FG_TL_END), latArm = gLatObsCalls - lat0;
    MgDueShow(); const LONG endI = (LONG)B::TlValue(FG_TL_END);
    MgDueN(); const LONG endN = (LONG)B::TlValue(FG_TL_END);
    ShFrame<MA>(false); const LONG fU = B::frames - 1;
    const bool ownedU = MA::OwnsEnd(fU); const LONG endU = (LONG)B::TlValue(FG_TL_END), latU = gLatObsCalls - lat0 - latArm;
    s.wt.want = FG_WANT_PASS; ShFrame<MA>(); ShFrame<MA>(); const LONG fP = B::frames - 1;
    const bool ownedP = MA::OwnsEnd(fP); const LONG endP = (LONG)B::TlValue(FG_TL_END), latP = gLatObsCalls - lat0 - latArm - latU;
    s.wt.want = FG_WANT_ON;
    ShFrame<MA>(); MgDueN();
    ShFrame<MA>(); const LONG fD = B::frames - 1;
    MA::BeforeRelease();                                      // (the quiesce of a build finds the pair armed: its real frame is dropped)
    const bool ownedD = MA::OwnsEnd(fD); const LONG endD = (LONG)B::TlValue(FG_TL_END), droppedD = ShArmB::a.w.dropped[FG_AD_BUILD];
    ShB::WriteB();
    if (!ShSettle()) Err("T3: the helper did not read the build");
    ShFrame<MA>(); MgDueN();
    printf("  an armed pair (frame %ld): owned %d; the end's fence at the arm %ld, behind the generated frame's fire %ld, behind the real frame's %ld; the saved observer called at the arm %ld time(s). An inline pair (frame %ld): owned %d, the fence %ld, the saved observer %ld. A passed present (frame %ld): owned %d, the fence %ld, the saved observer %ld for two presents. A dropped real frame (frame %ld, dropped %ld): owned %d, the fence %ld\n",
        fA, (int)ownedA, endArm, endI, endN, latArm, fU, (int)ownedU, endU, latU, fP, (int)ownedP, endP, latP, fD, droppedD, (int)ownedD, endD);
    if (!ownedA || endArm >= fA || endI >= fA || endN != fA || latArm != 0) Err("T3: an armed pair's frame end: owned %d, the fence %ld at the arm and %ld behind the generated frame's fire (under %ld wanted), %ld behind the real frame's (%ld wanted), the saved observer called %ld time(s) at the arm (0 wanted)", (int)ownedA, endArm, endI, fA, endN, fA, latArm);
    if (!ownedU || endU != fU || latU != 0) Err("T3: an inline pair's frame end: owned %d, the fence %ld (%ld wanted), the saved observer %ld (0 wanted: the machine signals it)", (int)ownedU, endU, fU, latU);
    if (ownedP || endP != fP || latP != 2) Err("T3: a passed present's frame end: owned %d, the fence %ld (%ld wanted), the saved observer %ld for two presents (not owned, and 2, wanted)", (int)ownedP, endP, fP, latP);
    if (!ownedD || endD >= fD || droppedD != 1) Err("T3: a dropped real frame's end: owned %d, the fence %ld (under %ld wanted: never signalled), dropped %ld", (int)ownedD, endD, fD, droppedD);
    MgClean("T3");
    MgEnd();
    // THE ROUTE: RT1's replay over the merged machine
    MgFresh<MA>();
    const ReplayRes r = Replay<MA>(false, false);
    MgEnd();
    MgFresh<MA>();
    const ReplayRes rc = Replay<MA>(true, false);
    MgEnd();
    printf("  the route over the merged machine, 4,000 iterations against the old routing: taken %zu and %zu (first difference %d), returns differing %d, not back %ld and %ld, end before begin %ld (the sequence's %ld)\n", r.oldN, r.newN, r.firstDiff, r.retDiff, r.oldNotBack, r.newNotBack, r.beforeBegin, r.wantBefore);
    if (r.firstDiff >= 0 || r.retDiff || r.oldNotBack != r.newNotBack || r.oldN < 1000 || r.beforeBegin != r.wantBefore || r.fpBad) Err("T3: the route over the merged machine did not replay the old routing exactly");
    // RT2's held leg, with the merged path's own fire
    LONG notBackAt[2] = { -1, -1 }; size_t took[2] = { 0, 0 }; bool back = false, held = false;
    for (int hold = 1; hold >= 0; hold--)
    {
        MgFresh<MA>();
        ShWarm<MA>(); MgDueN();
        ShFrame<MA>(); const LONG p = MgP(); static LONG fe; fe = B::frames - 1;
        MgStamp<MA>();
        gThrQuit = 0; ResetEvent(gThrQuitEv); gThrN = 0;
        gThr[gThrN++] = Spawn(&StamperMain<MA>, (void*)(INT_PTR)FG_TL_END);   // the frame end's stamper, a thread: only the held worker keeps the end from it
        Sleep(50);                                            // (the stamper starts from the fence's value when its thread first runs: it must be running before the end is signalled, or it would never stamp it)
        if (hold) { Gate(); DueN(p); FireNote(); held = WaitFor([] { return gGateIn != 0; }, 3000); }
        else { DueN(p); FireNote(); Drain(); }
        FgRoute rt = {}; NewSink sink; gNewTook.clear();
        const LONGLONG b = B::Now() - MsTick(5.0);
        for (int look = 1; look <= 8; look++)
        {
            Sleep(5);
            const LONG cand = look == 1 ? fe : fe + 1000 + look;
            MA::Route(rt, cand, cand, true, b, 1.0f, 1.0f, 1.0f, 1.0f, sink);
            if (notBackAt[hold] < 0 && rt.notBack > 0) notBackAt[hold] = look;
        }
        took[hold] = gNewTook.size();
        if (hold) { SetEvent(gGateEv); Drain(); back = held && WaitFor([] { LONGLONG t = 0; return MA::EndFor(fe, &t); }, 2000); }
        StopThreads();
        MgEnd();
    }
    printf("  the worker held with the real frame's fire behind it: the sample deferred, dropped not back at call %ld, taken %zu time(s); released, the end signalled and stamped: %s; the same looks with the worker not held: taken %zu time(s), not back at call %ld\n",
        notBackAt[1], took[1], back ? "yes" : "NO", took[0], notBackAt[0]);
    if (!held || notBackAt[1] != 6 || took[1] != 0 || !back) Err("T3: with the real frame's fire held, the frame's sample was dropped not back at call %ld and taken %zu time(s); the end stamped once released %d (6, 0 and yes wanted; held %d)", notBackAt[1], took[1], (int)back, (int)held);
    // CONTROLS
    const RingStats stc = gRingSt;
    LONG skI, drI; int armI;
    T3Fault<MA>(&skI, &drI, &armI); MgEnd();
    ViolRestore(stc);
    gRingSt.latEndDropped = stc.latEndDropped;
    printf("  the suppression skipped at pair 20 over the lean shape, 24 pairs: suppression skipped %ld, ends dropped by the guard %ld, a frame's end signalled at its arm %d time(s)\n", skI, drI, armI);
    if (rc.firstDiff < 0) Err("T3 control: the old routing's mutant replayed the same over the merged machine: the control shows nothing");
    if (took[0] != 1 || notBackAt[0] >= 0) Err("T3 control: with the worker not held the frame's sample was taken %zu time(s) and dropped not back at call %ld (taken once, never not back, wanted): the held leg shows nothing", took[0], notBackAt[0]);
    if (skI != 1 || drI != 1 || armI != 1) Err("T3 control: the suppression skipped over the lean shape %ld time(s), the guard dropped %ld end(s), and %d frame's end was signalled at its arm (1, 1 and 1 wanted): the control shows nothing", skI, drI, armI);
    Ctl("the old routing's mutant: the replay differs at take %d; the worker not held: the sample taken at once (%zu); the suppression skipped over the lean shape: the end at the arm (%d), the real frame's own dropped by the guard (%ld)", rc.firstDiff, took[0], armI, drI);
    CaseEnd(name);
}

// ---------------------------------------------------------------- T4, the pair's room
// WHAT IT SHOWS (pacing.md, PAC-4, as check-machine.md's R10 corrected where it acts; READY-FOR-7C.md, section 3, item 12).
// THE PLACER'S HALF, on the harness's clock (THE LAW OF 2026-10-11: fg_plan.h, "THE ROOM FLOOR'S LAW"; until then the
// floor rose with the first pair that needed more, and the sitting of that morning showed what one hitch at an engage
// then costs). Each pair's handoff is stamped 5.4 ms behind its entry, the planned generation time is 2 ms, the margin
// 1 ms: a pair needs 8.4 ms of half a period, and THE ROOM FLOOR is 17.00 ms (twice that, in steps of a quarter
// millisecond, rounded up).
//   FIVE PAIRS PUBLISH NOTHING; THE SIXTH DOES: the floor is what six pairs needed.
//   ONE PAIR whose handoff comes 44.9 ms behind its entry (a hitch at an engage), and four more: THE FLOOR DOES NOT MOVE.
//     The sixth such pair moves it AT ONCE, to what that sixth needs: six pairs of the last 120 is a room that is short.
//   A ROOM THAT IS TRULY SHORT (a fresh machine, every handoff 11.9 ms behind its entry, a 240 Hz panel's pair): nothing
//     for five pairs, 30.00 ms with the sixth, ONE rise.
//   IT COMES BACK when fewer than three of the last 120 pairs still need the step: 117 pairs at 5.4 ms leave three of
//     the six long ones in the 120, and the floor stands; the 118th leaves two, and it is 17.00 ms: ONE return.
//   THE PLAN IS ADDED AS IT STANDS NOW: at 3 ms for one pair the floor is 19.00 ms with that pair, and 17.00 again with
//     the next at 2 ms (no sample of the older plan holds it).
//   A NEED THAT HOVERS AT A STEP: one pair in thirty a fifth of a millisecond slower (four of 120) makes NO rise and no
//     return; one in seven (seventeen of 120) puts the floor on the upper step, ONE rise, and leaves it there.
//   A WARM-UP PAIR'S NEED IS NOT TAKEN: seven pairs 44.9 ms behind their entries inside the 150 pairs behind an engage
//     are not measured, and the floor stands where it stood.
// THE PACER'S HALF, at a 500 Hz refresh (the panel's floor, C, is 4.06 ms; a frame 1 ms behind the last release, on the
// harness's clock), the room asked for and the floor written by the harness, standing in for the placer: under THE LAW the period is the panel's floor, then 6 ms
// with the first step behind a room floor of 6 ms, the regime still the law's; the pad's ratchet, set to a whole
// millisecond, IS NOT ADDED; and with the floor gone the period is the panel's again, at once. (A5's arm, the room floor a
// floor of its target reached by its own moves, went with its law in round 9's LW.)
// CONTROL: the room not asked for (the pad's arm, as stage 7b left the merged path): the same room floor moves nothing,
// and the pad's millisecond is added: the period is C plus 1 ms.
struct T4Pace { double c, at0, atRoom, withPad, atBack; LONG regime; };
static T4Pace T4PaceRun(bool room)
{
    T4Pace g = {};
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    ShWarm<MA>(); MgDueN();
    B::refreshPacked = (500ull << 32) | 1ull;
    a.k.room = room ? 1 : 0;
    auto frames = [&](int n) { for (int i = 0; i < n; i++) { ClockAdd(1.0); ShInject(true); Frame<MA>(); Drain(); } };
    frames(6); g.c = MA::CPeriodMs(); g.at0 = s.r.period;
    WriteRelease(&a.pl.roomUs, 6000);                         // the harness standing in for the placer: a room floor of 6 ms
    if (!room) _InterlockedExchange(&s.sh.padQms, 4);
    frames(4); g.atRoom = s.r.period; g.regime = s.r.regime;
    _InterlockedExchange(&s.sh.padQms, 4);
    frames(4); g.withPad = s.r.period;
    _InterlockedExchange(&s.sh.padQms, 0);
    WriteRelease(&a.pl.roomUs, 0);
    frames(4); g.atBack = s.r.period;
    B::refreshPacked = 0; s.wt.want = FG_WANT_PASS;
    frames(2);
    if (!ShSettle()) Err("T4: the fake helper did not settle behind the paced frames");
    return g;
}
static void CaseT4()
{
    const char* const name = "T4 the pair's room: the placer's room floor, one rise and one return, and the pacer's clamp to it (stage 7d)";
    CaseBegin(name);
    MgFresh<MA>();
    FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000; a.k.roomGuardUs = 1000;                // (round 6 of phase 5: handed in and added for every pair again, fg_machine.h's ArmedRoomMargin)
    ShWarm<MA>(); MgDueN();
    B::S().r.sinceEngage = 1000;                              // (past the warm-up: a warm-up pair's need is not taken)
    MgEvenStart();
    auto pair = [&](LONG lsUs)
    {
        MgEven<MA>(true, false);
        if (!MgQuiet()) Err("T4: the fake helper did not settle behind a frame");
        B::clockNow += MA::UsToTicks(lsUs);                   // the handoff retires this long behind the frame's entry
        MgPlace<MA>();
    };
    struct At { LONG room, up, down; };
    auto at = [&]() { const At r = { a.pl.roomUs, a.pl.roomUp, a.pl.roomDown }; return r; };
    for (int i = 0; i < 5; i++) pair(5400);
    const At five = at();
    pair(5400);
    const At six = at();
    pair(44900);
    const At one = at();
    for (int i = 0; i < 4; i++) pair(44900);
    const At fiveOut = at();
    pair(44900);
    const At sixOut = at();
    printf("  the placer: a handoff 5.4 ms behind the entry, 2 ms planned, 1 ms of margin: after five pairs the room floor %ld us (rises %ld); with the sixth %ld us (rises %ld); ONE pair 44.9 ms behind its entry: %ld us (rises %ld); five of them: %ld us (rises %ld); the sixth: %ld us (rises %ld)\n",
        five.room, five.up, six.room, six.up, one.room, one.up, fiveOut.room, fiveOut.up, sixOut.room, sixOut.up);
    if (five.room != 0 || five.up != 0 || six.room != 17000 || six.up != 1) Err("T4: the room floor read %ld us with %ld rise(s) after five pairs and %ld us with %ld after the sixth (0, 0, 17000 and 1 wanted: it is what six pairs needed)", five.room, five.up, six.room, six.up);
    if (one.room != 17000 || one.up != 1 || fiveOut.room != 17000 || fiveOut.up != 1) Err("T4: behind ONE pair whose handoff came 44.9 ms late the room floor read %ld us with %ld rise(s), and behind five of them %ld us with %ld (17000 and 1 both times wanted: a single outlier, or five, moves nothing)", one.room, one.up, fiveOut.room, fiveOut.up);
    if (sixOut.room != 96000 || sixOut.up != 2 || sixOut.down != 0) Err("T4: behind the sixth pair 44.9 ms late the room floor read %ld us with %ld rise(s) and %ld return(s) (96000, 2 and 0 wanted: six of the last 120 is a short room, and it rises at once)", sixOut.room, sixOut.up, sixOut.down);
    MgClean("T4");
    MgEnd();
    // A ROOM THAT IS TRULY SHORT, on a fresh machine; its return; the plan added as it stands; a need that hovers
    MgFresh<MA>();
    a.k.kPlanUs = 2000; a.k.roomGuardUs = 1000;                // (round 6 of phase 5: handed in and added for every pair again, fg_machine.h's ArmedRoomMargin)
    ShWarm<MA>(); MgDueN();
    B::S().r.sinceEngage = 1000;
    MgEvenStart();
    for (int i = 0; i < 5; i++) pair(11900);
    const At s5 = at();
    pair(11900);
    const At s6 = at();
    for (int i = 0; i < 117; i++) pair(5400);
    const At b117 = at();
    pair(5400);
    const At b118 = at();
    a.k.kPlanUs = 3000; pair(5400);
    const At p3 = at();
    a.k.kPlanUs = 2000; pair(5400);
    const At p2 = at();
    for (int i = 0; i < 240; i++) pair(i % 30 == 3 ? 5600 : 5400);
    const At sparse = at();
    for (int i = 0; i < 130; i++) pair(i % 7 == 3 ? 5600 : 5400);
    const At dense = at();
    const LONG measured = a.pl.roomN;
    B::S().r.sinceEngage = 1;                                 // (just behind an engage: these pairs are its warm-up)
    for (int i = 0; i < 7; i++) pair(44900);
    const At warm = at(); const LONG warmN = a.pl.roomN - measured;
    B::S().r.sinceEngage = 1000;
    MgDueN();
    printf("  a room that is truly short (every handoff 11.9 ms behind its entry): after five pairs %ld us (rises %ld), with the sixth %ld us (rises %ld); back at 5.4 ms: after 117 pairs %ld us (returns %ld), with the 118th %ld us (returns %ld)\n",
        s5.room, s5.up, s6.room, s6.up, b117.room, b117.down, b118.room, b118.down);
    printf("  the plan at 3 ms for one pair: %ld us (rises %ld), at 2 ms again with the next: %ld us (returns %ld); a need that hovers at a step: one pair in thirty for 240 pairs: %ld us (rises %ld, returns %ld), one in seven for 130: %ld us (rises %ld, returns %ld); pairs measured %ld\n",
        p3.room, p3.up, p2.room, p2.down, sparse.room, sparse.up, sparse.down, dense.room, dense.up, dense.down, measured);
    if (s5.room != 0 || s5.up != 0 || s6.room != 30000 || s6.up != 1) Err("T4: a room that is truly short read %ld us with %ld rise(s) after five pairs and %ld us with %ld after the sixth (0, 0, 30000 and 1 wanted: it must rise, and six pairs in)", s5.room, s5.up, s6.room, s6.up);
    if (b117.room != 30000 || b117.down != 0 || b118.room != 17000 || b118.down != 1 || b118.up != 1) Err("T4: back at the smaller need the room floor read %ld us with %ld return(s) while three of the long pairs were among the last 120, and %ld us with %ld return(s) and %ld rise(s) once two were (30000, 0, 17000, 1 and 1 wanted: one rise, one return)", b117.room, b117.down, b118.room, b118.down, b118.up);
    if (p3.room != 19000 || p3.up != 2 || p2.room != 17000 || p2.down != 2) Err("T4: with the plan at 3 ms for one pair the room floor read %ld us (rises %ld) and %ld us (returns %ld) with the next pair at 2 ms (19000, 2, 17000 and 2 wanted: the plan is added as it stands now)", p3.room, p3.up, p2.room, p2.down);
    if (sparse.room != 17000 || sparse.up != 2 || sparse.down != 2) Err("T4: with one pair in thirty a fifth of a millisecond slower the room floor read %ld us with %ld rise(s) and %ld return(s) (17000, 2 and 2 wanted: four of 120 move nothing)", sparse.room, sparse.up, sparse.down);
    if (dense.room != 17250 || dense.up != 3 || dense.down != 2 || measured < 480) Err("T4: with one pair in seven a fifth of a millisecond slower the room floor read %ld us with %ld rise(s) and %ld return(s) over %ld pairs measured (17250, 3 and 2 wanted: the upper step, one rise, no return)", dense.room, dense.up, dense.down, measured);
    printf("  seven pairs 44.9 ms behind their entries inside an engage's warm-up: measured %ld, the room floor %ld us (rises %ld)\n", warmN, warm.room, warm.up);
    if (warmN != 0 || warm.room != 17250 || warm.up != 3) Err("T4: of seven warm-up pairs %ld were measured, and the room floor read %ld us with %ld rise(s) (0, 17250 and 3 wanted: a warm-up pair's need is not taken)", warmN, warm.room, warm.up);
    MgClean("T4 (a short room)");
    MgEnd();
    const T4Pace n1 = T4PaceRun(true); MgClean("T4 (the law)"); MgEnd();
    printf("  the pacer, C = %.2f ms, a room floor of 6 ms: %.2f ms before it, %.2f behind it (the regime %ld), %.2f with the pad at a millisecond, %.2f with the floor gone\n",
        n1.c, n1.at0, n1.atRoom, n1.regime, n1.withPad, n1.atBack);
    auto is = [](double x, double want) { return x > want - 1e-6 && x < want + 1e-6; };
    if (!is(n1.at0, n1.c) || !is(n1.atRoom, 6.0) || n1.regime != 0 || !is(n1.withPad, 6.0) || !is(n1.atBack, n1.c)) Err("T4: the period was %.3f ms before the room floor, %.3f behind it (regime %ld), %.3f with the pad set, %.3f with the floor gone (C = %.3f, 6, the law's regime 0, 6 and C wanted)", n1.at0, n1.atRoom, n1.regime, n1.withPad, n1.atBack, n1.c);
    // CONTROL (A5's arm, the room floor reached by A5's own moves, went with its law in round 9's LW)
    const RingStats stc = gRingSt;
    const T4Pace c1 = T4PaceRun(false); MgEnd();
    ViolRestore(stc);
    if (!is(c1.atRoom, c1.c + 1.0)) Err("T4 control: with the room not asked for, a room floor of 6 ms and the pad at a millisecond gave a period of %.3f ms (C plus 1 ms = %.3f wanted): the control shows nothing", c1.atRoom, c1.c + 1.0);
    Ctl("the room not asked for (the pad's arm): a room floor of 6 ms moves nothing and the pad's millisecond is added: %.2f ms, where C is %.2f", c1.atRoom, c1.c);
    CaseEnd(name);
}

// (T5, lever 4's development key refused in a binding that arms, with its control over the shadow shape, which honoured it,
// went with that shape and the key in round 9's strip: no binding has the key now.)

// ---------------------------------------------------------------- T6, the least lead of a generated frame
// WHAT IT SHOWS (machine.md, machine-7 e; the main session's ruling on READY-FOR-7C.md's question 6: the bound is a NUMBER
// HANDED IN beside the planned generation time and the late allowance, its value a later sitting's). The placer says
// SHOW at the slot for every pair here (the answer is in), and with its due sends how far the slot is from the instant
// the real frame's own due will be raised. THE FIRE POINT is then placed 3 ms, 1 ms, 0.6 ms and 0.3 ms before that
// raise, at each of three bounds:
//   0 (the default, and what stage 7b left)    all four are shown: any fire point before the raise shows it
//   500 us                                     the first three are shown; the one 0.3 ms before the raise is NOT
//   2,000 us                                   only the one 3 ms before is shown
// A generated frame that is not shown is counted "late fire", and EVERY REAL FRAME IS SHOWN ONCE, at a due that does not
// move with the bound.
// CONTROL: the row at 0: the same four fire points all show. The bound is the number handed in and nothing else.
struct T6Pair { int gen, real; bool hasI, hasN; LONG leadUs; LONGLONG dN; };
static T6Pair T6One(LONG boundUs, LONG beforeUs)
{
    T6Pair g = {};
    FgArmedState& a = ShArmB::a;
    a.k.minLeadUs = boundUs;
    const size_t sh = MgShownN();
    const DWORD tag = MgEven<MA>(true, false);
    if (!MgQuiet() || !MgAnswered(ShB::x.w.lastQN)) Err("T6: the fake helper did not answer the pair");
    const LONG p = MgP();
    MgPlace<MA>();
    const FgArmedLive* L = MgLive(p);
    if (!L || !L->timed) { Err("T6: a pair of an even run was not armed and timed at its handoff"); return g; }
    const LONGLONG tS = L->tS, tI = L->tI, tN = L->tN;
    B::clockNow = tI; MgPlace<MA>();                          // the slot: the verdict, with its lead
    FgDue di = {}; g.hasI = MgDue(false, p, &di) && di.verdict == FG_DUE_SHOW; g.leadUs = di.eUs;
    B::clockNow = tN - MA::UsToTicks(beforeUs);               // THE FIRE POINT, this long before the real frame's raise
    FireNote(); Drain();
    B::clockNow = tN; MgPlace<MA>();
    FgDue dn = {}; g.hasN = MgDue(true, p, &dn); g.dN = dn.tDue - tS;
    FireNote(); Drain();
    const auto shown = Shown();
    for (size_t i = sh; i < shown.size(); i++) { g.gen += shown[i].tag == ShGenTag(tag, false); g.real += shown[i].tag == tag; }
    return g;
}
static void CaseT6()
{
    const char* const name = "T6 the least lead of a generated frame: a number handed in, proved at three values (stage 7d)";
    CaseBegin(name);
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000; a.k.lateUs = 0;
    ShWarm<MA>(); MgDueN();
    s.r.sinceEngage = 1000;
    MgEvenStart();
    for (int i = 0; i < 4; i++) T6One(0, 3000);               // (an even run behind the warm-up: the period's median is the grid's)
    static const LONG kBound[3] = { 0, 500, 2000 }, kBefore[4] = { 3000, 1000, 600, 300 };
    const LONG lf0 = a.w.iWhy[FG_AW_LATEFIRE];
    int bad = 0, realBad = 0, dueBad = 0; std::string rows, first; LONGLONG dN0 = 0; LONG lead0 = 0;
    for (int b = 0; b < 3; b++)
    {
        rows += Fmt("%s%ld us:", b ? "; " : "", kBound[b]);
        for (int f = 0; f < 4; f++)
        {
            const T6Pair g = T6One(kBound[b], kBefore[f]);
            const bool want = kBound[b] == 0 || kBefore[f] > kBound[b];
            rows += Fmt(" %d", g.gen);
            if (!b && !f) { dN0 = g.dN; lead0 = g.leadUs; }
            if (!g.hasI || (g.gen == 1) != want || g.gen > 1) { bad++; if (first.empty()) first = Fmt("at a bound of %ld us a fire point %ld us before the raise showed the generated frame %d time(s) (the placer said SHOW: %d)", kBound[b], kBefore[f], g.gen, (int)g.hasI); }
            if (g.real != 1) realBad++;
            if (!g.hasN || g.dN != dN0) dueBad++;
        }
    }
    a.k.minLeadUs = 0;
    const LONG lateFire = a.w.iWhy[FG_AW_LATEFIRE] - lf0;
    T6One(0, 3000);
    printf("  fire points 3,000, 1,000, 600 and 300 us before the real frame's raise, the generated frame shown at each bound: %s; the lead the due carried %ld us (the slot to the raise); not shown for a late fire point %ld; real frames not shown exactly once %d; real frames' dues that moved %d\n",
        rows.c_str(), lead0, lateFire, realBad, dueBad);
    if (bad) Err("T6: %d of twelve pairs wrong; the first: %s", bad, first.c_str());
    if (lateFire != 4 || realBad || dueBad || lead0 <= 3000) Err("T6: %ld generated frame(s) were counted not shown for a late fire point (4 wanted), %d real frame(s) were not shown once, %d real frames' dues moved, and the due carried a lead of %ld us (half a period, over 3,000, wanted)", lateFire, realBad, dueBad, lead0);
    MgClean("T6");
    MgEnd();
    Ctl("read from the same run: at a bound of 0 the four fire points all show (the rule stage 7b left); at 500 us the one 300 us before the raise does not, at 2,000 us only the one 3,000 us before does");
    CaseEnd(name);
}

// ---------------------------------------------------------------- T7, the plan is the answer's deadline; two late answers
// WHAT IT SHOWS (2026-10-11, the sitting's fault B; source/fg/fg_plan.h; FgArmedKeys::planAns, planUpPairs, planUpUs,
// planMaxUs). The planned generation time is 2,000 us and THE GENERATED FRAME'S FIRE SERVICE TIME, which the worker
// publishes and the placer raises its verdict ahead by, is held at 300 us.
//   THE PLAN IS THE ANSWER'S DEADLINE (planAns = 1): the slot is 2,300 us behind the handoff, the plan and the service
//     time; nothing is raised before the plan; an answer at 1,900 us and one at 1,990 us are SHOWN, the generated
//     frame due at the slot; an answer at 2,100 us, behind the plan and before the slot, is LATE; the real frame's
//     due is the slot plus half the period for every one of them. With no service time the slot is the plan.
//   TWO LATE ANSWERS (the span handed in as 5 pairs, a bin of 250 us, never over 2,500 us): one late answer moves
//     nothing; a second one SIX pairs behind it moves nothing; a third TWO pairs behind that raises the plan to
//     2,250 us AT ONCE, and the very next pair's answer at 2,100 us is shown. A pair whose slot was fixed with the plan
//     since replaced says nothing (the placer's rule, called on such a pair). Two more late answers: 2,500 us, and
//     no further: the bound.
// CONTROLS (runs): planAns = 0, the law until this date: the slot is the plan, the verdict falls 300 us before it, and
// the answer at 1,900 us, 100 us INSIDE the plan, is LATE: the sitting's fault, made at a desk. And with the span at 0
// two late answers in a row raise nothing.
struct T7Pair { LONG p; LONGLONG tS, tI, tN; bool early, hasI, hasN; FgDue di, dn; int gen, real; };
static T7Pair T7One(LONG answerUs, LONG svcUs)
{
    T7Pair g = {};
    FgState& s = B::S(); FgShadowState& x = ShB::x;
    s.w.wI = (double)svcUs / 1000.0; WriteRelease(&s.w.wIUs, svcUs);   // (the worker's own figure moves a tenth of the way at every fire: held for this pair)
    _InterlockedExchange(&gH.delayUs, answerUs);
    const size_t sh = MgShownN();
    const DWORD tag = MgEven<MA>(true, false);
    if (!MgQuiet()) Err("T7: the fake helper did not settle behind the frame");
    g.p = MgP(); const LONG n = x.w.lastQN;
    MgPlace<MA>();
    const FgArmedLive* L = MgLive(g.p);
    if (!L || !L->timed) { Err("T7: a pair of an even run was not armed and timed at its handoff (pair %ld)", g.p); return g; }
    g.tS = L->tS; g.tI = L->tI; g.tN = L->tN;
    const LONGLONG ansT = MA::UsToTicks(answerUs), at = g.tI - g.tS - MA::UsToTicks(svcUs);
    bool answered = answerUs <= 0;
    auto to = [&](LONGLONG t)
    {
        if (!answered && t >= ansT) { if (g.tS + ansT > B::clockNow) B::clockNow = g.tS + ansT; if (!MgAnswered(n)) Err("T7: the delayed answer of pair %ld never came", n); answered = true; }
        if (g.tS + t > B::clockNow) B::clockNow = g.tS + t;
    };
    FgDue d = {};
    to(at - 1); MgPlace<MA>(); g.early = MgDue(false, g.p, &d) || MgDue(true, g.p, &d);
    to(at); MgPlace<MA>(); g.hasI = MgDue(false, g.p, &g.di);
    FireNote(); Drain();
    to(g.tN - g.tS); MgPlace<MA>(); g.hasN = MgDue(true, g.p, &g.dn);
    FireNote(); Drain();
    if (!answered) to(ansT);
    const auto shown = Shown();
    for (size_t i = sh; i < shown.size(); i++) { g.gen += shown[i].tag == ShGenTag(tag, false); g.real += shown[i].tag == tag; }
    _InterlockedExchange(&gH.delayUs, 0);
    if (!ShSettle()) Err("T7: the fake helper did not come back behind the pair");
    return g;
}
static void CaseT7()
{
    const char* const name = "T7 the plan is the answer's deadline, and two late answers too close raise it at once (2026-10-11)";
    CaseBegin(name);
    MgFresh<MA>();
    FgState& s = B::S(); FgArmedState& a = ShArmB::a;
    a.k.kPlanUs = 2000; a.k.lateUs = 0; a.k.planAns = 1;
    ShWarm<MA>(); MgDueN();
    s.r.sinceEngage = 1000;
    MgEvenStart();
    for (int i = 0; i < 4; i++) T7One(0, 0);                  // (an even run behind the warm-up)
    const LONGLONG kT = MA::UsToTicks(2000), sT = MA::UsToTicks(300);
    const T7Pair in1 = T7One(1900, 300), in2 = T7One(1990, 300), late1 = T7One(2100, 300), plain = T7One(0, 300), zero = T7One(1900, 0);
    const LONGLONG hT = in1.tN - in1.tI;
    printf("  planned 2,000 us, the generated frame's service time 300 us, the plan the answer's deadline: the slot %lld ticks behind the handoff (%lld the plan, %lld the service time); the answer at 1,900 us: raised before the plan %s, verdict %ld, due %lld ticks behind the handoff, shown %d; at 1,990: verdict %ld, shown %d; at 2,100: verdict %ld why %ld, shown %d; the real frame's due %lld, %lld and %lld ticks behind the handoff (%lld wanted); with no service time the slot %lld\n",
        (long long)(in1.tI - in1.tS), (long long)kT, (long long)sT, in1.early ? "YES" : "no", in1.di.verdict, (long long)(in1.di.tDue - in1.tS), in1.gen, in2.di.verdict, in2.gen, late1.di.verdict, MgWhyOf(late1.di), late1.gen,
        (long long)(in1.dn.tDue - in1.tS), (long long)(in2.dn.tDue - in2.tS), (long long)(late1.dn.tDue - late1.tS), (long long)(kT + sT + hT), (long long)(zero.tI - zero.tS));
    if (in1.tI - in1.tS != kT + sT || in1.early || !in1.hasI || in1.di.verdict != FG_DUE_SHOW || in1.di.tDue - in1.tS != kT + sT || in1.gen != 1 || in1.real != 1)
        Err("T7: with the plan as the answer's deadline the slot is %lld ticks behind the handoff (%lld wanted), raised before the plan %d, the answer at 1,900 us: verdict %ld (SHOW %d wanted), due %lld, shown %d, the real frame %d", (long long)(in1.tI - in1.tS), (long long)(kT + sT), (int)in1.early, in1.di.verdict, (int)FG_DUE_SHOW, (long long)(in1.di.tDue - in1.tS), in1.gen, in1.real);
    if (in2.early || in2.di.verdict != FG_DUE_SHOW || in2.gen != 1) Err("T7: the answer at 1,990 us, 10 us before the plan: raised early %d, verdict %ld, shown %d (no, SHOW and 1 wanted: every answer under the plan is in time)", (int)in2.early, in2.di.verdict, in2.gen);
    if (late1.early || late1.di.verdict != FG_DUE_WITHHOLD || MgWhyOf(late1.di) != FG_AW_LATE || late1.gen != 0 || late1.real != 1) Err("T7: the answer at 2,100 us, behind the plan and before the slot: verdict %ld why %ld, shown %d, the real frame %d (WITHHOLD, LATE, 0 and 1 wanted)", late1.di.verdict, MgWhyOf(late1.di), late1.gen, late1.real);
    if (!in1.hasN || !in2.hasN || !late1.hasN || in1.dn.tDue - in1.tS != kT + sT + hT || in2.dn.tDue - in2.tS != kT + sT + hT || late1.dn.tDue - late1.tS != kT + sT + hT) Err("T7: the real frame's due is not the slot plus half the period for the three pairs (%lld, %lld and %lld ticks behind the handoff, %lld wanted)", (long long)(in1.dn.tDue - in1.tS), (long long)(in2.dn.tDue - in2.tS), (long long)(late1.dn.tDue - late1.tS), (long long)(kT + sT + hT));
    if (zero.tI - zero.tS != kT || zero.di.verdict != FG_DUE_SHOW || plain.real != 1) Err("T7: with no service time the slot is %lld ticks behind the handoff (%lld, the plan, wanted), verdict %ld", (long long)(zero.tI - zero.tS), (long long)kT, zero.di.verdict);
    // TWO LATE ANSWERS
    a.k.planUpPairs = 5; a.k.planUpUs = 250; a.k.planMaxUs = 2500;
    T7One(0, 0);
    const T7Pair l1 = T7One(2100, 0); const LONG plan1 = a.k.kPlanUs;
    for (int i = 0; i < 5; i++) T7One(0, 0);
    const T7Pair l2 = T7One(2100, 0); const LONG plan2 = a.k.kPlanUs, ups2 = a.pl.planUps;
    T7One(0, 0);
    const T7Pair l3 = T7One(2100, 0); const LONG plan3 = a.k.kPlanUs, ups3 = a.pl.planUps;
    const T7Pair next = T7One(2100, 0);
    FgArmedLive stale = {}; stale.p = MgP() + 1; stale.planUs = 2000;
    MA::ArmedPlanLate(stale);
    const LONG planS = a.k.kPlanUs, upsS = a.pl.planUps;
    const T7Pair l4 = T7One(2350, 0); const LONG plan4 = a.k.kPlanUs;
    const T7Pair l5 = T7One(2600, 0); const LONG plan5 = a.k.kPlanUs, ups5 = a.pl.planUps;
    printf("  two late answers, the span 5 pairs: one late answer: the plan %ld us; a second six pairs behind it: %ld us (raised %ld time(s)); a third two pairs behind that: %ld us (raised %ld); the next pair, its answer at 2,100 us: the slot %lld ticks behind the handoff, verdict %ld, shown %d; a late pair of the plan since replaced: %ld us (raised %ld); two more late answers: %ld us, then %ld us (raised %ld; the bound is 2,500)\n",
        plan1, plan2, ups2, plan3, ups3, (long long)(next.tI - next.tS), next.di.verdict, next.gen, planS, upsS, plan4, plan5, ups5);
    if (l1.di.verdict != FG_DUE_WITHHOLD || l2.di.verdict != FG_DUE_WITHHOLD || l3.di.verdict != FG_DUE_WITHHOLD || l4.di.verdict != FG_DUE_WITHHOLD || l5.di.verdict != FG_DUE_WITHHOLD) Err("T7: one of the five late answers was not judged late (verdicts %ld %ld %ld %ld %ld)", l1.di.verdict, l2.di.verdict, l3.di.verdict, l4.di.verdict, l5.di.verdict);
    if (plan1 != 2000 || plan2 != 2000 || ups2 != 0) Err("T7: one late answer left the plan at %ld us, and a second six pairs behind it at %ld us, raised %ld time(s) (2000, 2000 and 0 wanted: late answers as far apart as the quantile allows move nothing)", plan1, plan2, ups2);
    if (plan3 != 2250 || ups3 != 1 || next.tI - next.tS != MA::UsToTicks(2250) || next.di.verdict != FG_DUE_SHOW || next.gen != 1) Err("T7: a late answer two pairs behind the last left the plan at %ld us, raised %ld time(s); the next pair's slot %lld ticks behind the handoff, verdict %ld, shown %d (2250, 1, %lld, SHOW and 1 wanted: one bin, at once)", plan3, ups3, (long long)(next.tI - next.tS), next.di.verdict, next.gen, (long long)MA::UsToTicks(2250));
    if (planS != 2250 || upsS != 1) Err("T7: a late pair whose slot was fixed with the plan since replaced moved the plan to %ld us, raised %ld time(s) (2250 and 1 wanted: it says nothing of the plan in force)", planS, upsS);
    if (plan4 != 2500 || plan5 != 2500 || ups5 != 2) Err("T7: two more late answers left the plan at %ld us and then %ld us, raised %ld time(s) (2500, 2500 and 2 wanted: never over the bound)", plan4, plan5, ups5);
    // CONTROLS
    a.k.planUpPairs = 0; a.k.planMaxUs = 8000;
    T7One(0, 0);
    T7One(2600, 0); const T7Pair c2 = T7One(2600, 0);
    const LONG planC = a.k.kPlanUs, upsC = a.pl.planUps;
    a.k.kPlanUs = 2000; a.k.planAns = 0;
    T7One(0, 0);
    const T7Pair c1 = T7One(1900, 300);
    a.k.planAns = 1;
    T7One(0, 0);
    s.w.wI = 0.0; WriteRelease(&s.w.wIUs, 0);
    MgClean("T7");
    MgEnd();
    if (c2.di.verdict != FG_DUE_WITHHOLD || planC != 2500 || upsC != 2) Err("T7 control: with the span at 0 two late answers in a row left the plan at %ld us, raised %ld time(s) in all (2500 and 2 wanted): the control shows nothing", planC, upsC);
    if (c1.tI - c1.tS != kT || c1.di.verdict != FG_DUE_WITHHOLD || MgWhyOf(c1.di) != FG_AW_LATE || c1.gen != 0) Err("T7 control: with the slot at the plan (planAns = 0) and a service time of 300 us the answer at 1,900 us gave the slot %lld ticks behind the handoff, verdict %ld why %ld, shown %d (%lld, WITHHOLD, LATE and 0 wanted): the control shows nothing", (long long)(c1.tI - c1.tS), c1.di.verdict, MgWhyOf(c1.di), c1.gen, (long long)kT);
    Ctl("the law until 2026-10-11 (the slot at the plan): the same answer at 1,900 us, 100 us inside the plan of 2,000, is LATE (verdict %ld, shown %d): the verdict falls the service time before the slot; and with the span at 0 two late answers in a row raise nothing (the plan %ld us)", c1.di.verdict, c1.gen, planC);
    CaseEnd(name);
}

// ---------------------------------------------------------------- FK9 AND FK10, THE FAKE'S SET AND QUEUE RULES PLANTED OVER THE ARMED BINDING
// (2026-10-26, round 9, the reviewer's R2.) Round 9's strip took the product's shadow shape out (bf2fcce2) and with it FK5's and FK7's planted
// legs, which ran over the shadow binding: every case still holds setAbsent, setNotAcquired, setAcquireOrder and csPastWait at 0 (CaseEnd),
// but no case planted them any more, so a count that stopped counting would go unseen. These two plant them again, over THE ARMED BINDING
// THE SHIPPED MACHINE USES (ShArmB, the merged path), each leg one seam changed, each counted by its own rule, beside a clean leg that counts
// nothing. Plant-only: they prove the fake's rules can fire, not a behaviour of the product's.
// FK9, THE HELPER'S SET (dt_fake_ring.h, "THE HELPER'S SET"; FK5's plants): a copy into a set image with no acquire (setNotAcquired); a
// second acquire of an acquired image (setAcquireOrder); an acquire never released (setAcquireOrder, at the buffer's end); a binding that
// says the set has a HUD-less image it has not (setAbsent: the copy is not made). The clean pair: acquires and releases equal, none counted.
struct Fk9NoAcqB  : ShArmB { static void RecCopyToImg(ULONGLONG bb, int img) { FgHarnessBinding::RecCopyToImg(bb, img); } };
struct Fk9Acq2B   : ShArmB { static void RecCopyToImg(ULONGLONG bb, int img) { RecAcquire(img); RecAcquire(img); FgHarnessBinding::RecCopyToImg(bb, img); RecRelease(img); } };
struct Fk9KeepB   : ShArmB { static void RecCopyToImg(ULONGLONG bb, int img) { RecAcquire(img); FgHarnessBinding::RecCopyToImg(bb, img); } };
struct Fk9LieHudB : ShArmB { static bool HasHudless() { return true; } };
typedef FgMachine<Fk9NoAcqB> MAFNA; typedef FgMachine<Fk9Acq2B> MAFA2; typedef FgMachine<Fk9KeepB> MAFKP; typedef FgMachine<Fk9LieHudB> MAFLH;
struct Fk9Leg { LONG notAcq, order, absent, acq, rel; };
template <class MM> static Fk9Leg Fk9Run(bool planted)
{
    const RingStats st0 = gRingSt;
    MgFresh<MM>();
    ShWarm<MM>(); MgDueN();
    const RingStats s1 = gRingSt;
    ShFrame<MM>(); MgDueShow(); MgDueN();                      // one pair of the merged path: its hand, its two fires
    Fk9Leg g = { gRingSt.setNotAcquired - s1.setNotAcquired, gRingSt.setAcquireOrder - s1.setAcquireOrder, gRingSt.setAbsent - s1.setAbsent,
                 gRingSt.setAcquires - s1.setAcquires, gRingSt.setReleases - s1.setReleases };
    if (planted) ViolRestore(st0);
    else MgClean("FK9");
    MgEnd();
    return g;
}
static void CaseFK9()
{
    const char* const name = "FK9 the helper's set's rules count each planted violation over the armed binding: no acquire, a second acquire, no release, a slot it lacks (round 9, R2; FK5's plants)";
    CaseBegin(name);
    const Fk9Leg clean = Fk9Run<MA>(false), noAcq = Fk9Run<MAFNA>(true), twice = Fk9Run<MAFA2>(true), keep = Fk9Run<MAFKP>(true), lie = Fk9Run<MAFLH>(true);
    printf("  a clean pair: %ld acquire(s), %ld release(s); not acquired %ld, out of order %ld, absent %ld\n", clean.acq, clean.rel, clean.notAcq, clean.order, clean.absent);
    printf("  PLANTED, one pair each: no acquire: not acquired %ld; a second acquire: out of order %ld; no release: out of order %ld; a HUD-less slot the set lacks: absent %ld\n",
           noAcq.notAcq, twice.order, keep.order, lie.absent);
    if (clean.acq < 1 || clean.acq != clean.rel || clean.notAcq || clean.order || clean.absent) Err("FK9: a clean pair made %ld acquire(s) and %ld release(s) (equal, 1 or more wanted) and the rules counted %ld, %ld, %ld (none wanted)", clean.acq, clean.rel, clean.notAcq, clean.order, clean.absent);
    if (noAcq.notAcq < 1) Err("FK9: a copy into a set image with no acquire was counted %ld time(s) (1 or more wanted)", noAcq.notAcq);
    if (twice.order < 1) Err("FK9: a second acquire of an acquired image was counted %ld time(s) (1 or more wanted)", twice.order);
    if (keep.order < 1) Err("FK9: an acquire never released was counted %ld time(s) at its buffer's end (1 or more wanted)", keep.order);
    if (lie.absent < 1) Err("FK9: a copy into a HUD-less image the set does not have was counted %ld time(s) (1 or more wanted)", lie.absent);
    Ctl("each planted violation counted by its own rule: no acquire %ld, a second acquire %ld, no release %ld, a slot the set lacks %ld; the clean pair none", noAcq.notAcq, twice.order, keep.order, lie.absent);
    CaseEnd(name);
}
// FK10, THE PRESENT OBEYS THE QUEUE'S WAIT (FK7's plant): the helper HELD in a prepare, the next injection queues the merged path's one wait
// on fgPrep (FgMachine's ShadowBeforeInject, B::PrepGuard), then the game's stream work and a Present of DXVK's on the worker. Clean: the
// stream is held behind the wait (csHeld), none ran past it. PLANTED (ShB::csLeak, the gate's leak): the stream's work runs past the queued
// wait and the fake counts it (csPastWait).
static void Fk10Present(DWORD, DWORD, DWORD) { ((ShDxvk_t)gDxvkPresent0)(SC, nullptr, nullptr, nullptr, nullptr, 0); }
struct Fk10Leg { bool held; LONG guards, csHeld, past; };
static Fk10Leg Fk10Run(LONG leak)
{
    Fk10Leg g = {};
    const RingStats st0 = gRingSt;
    MgFresh<MA>();
    FgShadowState& x = ShB::x;
    ShWarm<MA>(); MgDueN();
    _InterlockedExchange(&gH.holdPrep, 1);
    ShFrame<MA>(true, false);
    g.held = WaitFor([] { return gH.inPrep != 0; }, 3000);
    MgDueN();                                                 // the pair's real frame out first: nothing of it behind the wait
    const LONG g0 = x.w.prepGuards;
    ClockAdd(16.7); ShInject(true); Drain();                  // the next injection: the prepare is held, so the wait is queued
    g.guards = x.w.prepGuards - g0;
    const RingStats s1 = gRingSt;
    gRing.presentCs = RING_CS_ALL; _InterlockedExchange(&ShB::csLeak, leak);   // (ALL: DXVK's command thread hands the stream over at the Present)
    Fill(NewTag()); Run(Fk10Present); Drain();                 // the stream's work and a Present, behind the queued wait
    g.csHeld = gRingSt.csHeld - s1.csHeld; g.past = gRingSt.csPastWait - s1.csPastWait;
    g.held = g.held && gH.inPrep != 0;
    gRing.presentCs = RING_CS_LAZY; _InterlockedExchange(&ShB::csLeak, 0);
    ShRelease();
    if (!WaitFor([] { return ShB::waitFor == 0; }, 3000)) Err("FK10: the queue's wait did not end after the helper's release");
    Frame<MA>(); Drain();
    if (!ShSettle()) Err("FK10: the fake helper did not come back after its hold");
    MgDueN();
    if (leak) ViolRestore(st0);
    MgEnd();
    return g;
}
static void CaseFK10()
{
    const char* const name = "FK10 the stream obeys the merged path's queued wait over the armed binding; the gate's planted leak is counted (round 9, R2; FK7's plant)";
    CaseBegin(name);
    const Fk10Leg c = Fk10Run(0), p = Fk10Run(1);
    printf("  clean: the helper held %s, waits queued %ld, the stream held behind the wait %ld time(s), ran past it %ld\n", c.held ? "yes" : "NO", c.guards, c.csHeld, c.past);
    printf("  PLANTED, the gate's leak on: the helper held %s, waits queued %ld, ran past the wait %ld time(s)\n", p.held ? "yes" : "NO", p.guards, p.past);
    if (!c.held || c.guards < 1 || c.csHeld < 1 || c.past != 0) Err("FK10: clean, the helper held %d, %ld wait(s) queued, the stream held %ld time(s) and ran past the wait %ld (held, 1 or more, 1 or more and 0 wanted)", (int)c.held, c.guards, c.csHeld, c.past);
    if (!p.held || p.past < 1) Err("FK10 control: with the gate's leak on the stream ran past the queued wait %ld time(s) (1 or more wanted): the rule cannot see it", p.past);
    else Ctl("the gate's leak on: the stream ran past the queued wait, counted %ld time(s)", p.past);
    CaseEnd(name);
}
