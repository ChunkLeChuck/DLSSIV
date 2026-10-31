// dt_fg_shadow.h: THE PRODUCT'S WIRE IN THE HARNESS, ITS FAKE HELPER AND ITS MERGED BINDING (2026-10-06, phase 2 of the
// revival, stage 9; docs/DECISIONS.md "PHASE 2, STAGE 0" section B and "PHASE 2, STAGE 9").
// (ROUND 9'S STRIP, 2026-10-26: the product's shadow shape, the binding that could not arm, went from the machine, and with it
// this file's cases over it, L1 to L19, G4, G8 and FK1 to FK7, and their reruns under fg_q_at_hand: docs\INSTRUMENTS-FG.md,
// wave 5. ShB stays as the base of ShArmB, the merged binding every case of dt_fg_merged.h and after runs; no machine is
// compiled over ShB itself, which the machine refuses: fg_machine.h's static_assert.)
//
// Included by dt_fg_test.cpp after its helpers. ShB is the product's wire's seams (it says false to kRig and to kEngage,
// true to kWire) on the same fake of DXVK the rig's cases use, against A FAKE HELPER: a thread
// that reads the fake pipe as the real helper reads its pipe, answers on the fake fgPrep and fgOut timelines, waits on
// fgIn for a DISPATCH and for that kind alone, and counts what the real one counts (DUPLICATE, DRIFT, resets by reason).
// It can be HELD inside a prepare and inside a dispatch, told to give no frame, and told to die.
//
// THE QUEUE'S WAIT. The fake's submits act at once, so the one GPU wait the product queues (a wait-only batch behind
// fgPrep, G4) is modelled where it bites: while one is pending and fgPrep is short of it, every later submit of this
// binding's is TAKEN BUT HELD (its copies and signals do not happen), and runs, in order, when fgPrep arrives.
// SINCE PHASE 3'S STAGE 6 THE COMMAND STREAM'S WORK STANDS BEHIND THAT WAIT TOO, A PRESENT'S BLIT LIKE ANYTHING ELSE
// (ShB::CsGate): what reaches the queue after a wait was queued is held with the submits, in one order.
//
// WHAT THE FAKES LEARNED AT PHASE 3'S STAGE 6 (2026-10-10; the build map's machine-15 and pgk-13, check-machine's X8 and
// R19), for the merged path to be proved against. Each is a rule with a count, and each has a case at the end of this
// file that planted its violation (FK1 to FK8; FK1 to FK7 ran over the shadow shape and went with it in round 9's strip,
// FK8 runs over the rig's shape and stays):
//   - THE GENERATED IMAGE'S TAG NAMES ITS PAIR: the picture it was made from, under a mark of its own (ShGenTag).
//   - THE FAKE HELPER KNOWS WHICH DISPATCH RESET, by the real helper's own lines (resetNext, and the reasons of wire 2).
//   - AN ANSWER CAN BE DELAYED ON THIS HARNESS'S CLOCK (gH.delayUs).
//   - A HELPER THAT IS GONE LEAVES ITS TWO FENCES AT THE TOP (ShHelperGone), as the real one's exit and the kernel do.
//   - THE SET'S IMAGES ARE THE HELPER'S: acquired and released round every copy, absent where the set has none, DEAD AT
//     A BUILD (ShB::WriteB), and each carrying the generation it was last written in (dt_fake_ring.h, "THE HELPER'S SET").
//   - A REFUSED SUBMIT CAN BE PICKED BY ITS PLACE in a fire (gRing.submitFailSkip), and a Present made after one is counted.
//
// THE CLOCK IS THE HARNESS'S (Opt::fakeClock), 16.7 ms a frame: the render thread's facts are made of intervals (a hitch,
// "fewer than four samples"), and a case that needs a DISPATCH pair cannot have them depend on how long a Drain took. A
// frame the harness records by hand (a hand with no FgSwapPresent) takes no time on it, so the next real frame is no hitch.
//
// L9 (2026-10-06) IS NOT THE LIFECYCLE MAP'S: the gate before DXVK's Present, added after phase 2's first sitting froze the
// game. It proves WHERE the machine calls the binding's gate (before every present it forwards, on the presenting thread);
// what the gate waits for is the client's own line, proved on real fences by tools/test-fg-pipe.
#pragma once
#include "../../source/fg/fg_hud_rule.h"   // (2026-10-24: FgHudSrcTake, the mod's own pick of the image the hand's copy names: ShArmB::HudSrc)

// THE GENERATED COUNTS THE FAKE'S SET MAKES (2026-10-18, round 3, G3), the ack's gen_counts: bit k, count k. 2 (one frame a pair) in every case
// before dt_fg_fill.h's; the machine asks it through ShArmB::GenOk, the set's slots name GEN2 by it, the fake helper reads it at each 'G'
static volatile LONG gShGenCounts = 2;
struct ShB : FgHarnessBinding
{
    static constexpr bool kRig = false, kEngage = false, kWire = true;
    static inline FgShadowState x;
    static FgShadowState& X() { return x; }
    static inline volatile LONG mayEngage = 1;
    static bool MayEngage() { return mayEngage != 0; }
    // WHAT A PRESENT COSTS, COUNTED (phase 3, stage 7d; dt_fg_merged.h, T1 and T2): while `countTid` names a thread, every
    // read of the clock, every read of the sleeper count and every wait of the pacer's that the machine makes on that
    // thread is counted. Off (0), which is every other case: the three seams are the harness's own, as they were.
    static inline DWORD countTid = 0;
    static inline LONG cntNow = 0, cntSlept = 0, cntPaceWait = 0;
    static LONGLONG Now() { if (countTid && GetCurrentThreadId() == countTid) cntNow++; return FgHarnessBinding::Now(); }
    static LONG PaceSleptN() { if (countTid && GetCurrentThreadId() == countTid) cntSlept++; return FgHarnessBinding::PaceSleptN(); }
    static int PaceWait(LONGLONG dueQpc) { if (countTid && GetCurrentThreadId() == countTid) cntPaceWait++; return FgHarnessBinding::PaceWait(dueQpc); }
    // the injection's note, shown ahead of its body (the product reads two values of its frame message there): counted
    static inline volatile LONG injSeen = 0;
    static void InjectSeen(const DWORD* payload, DWORD bytes) { (void)payload; (void)bytes; _InterlockedIncrement(&injSeen); }
    // DOES THE SET HAVE A HUD-LESS IMAGE (phase 3, lever 1)? THE DEFAULT IS THE GAME'S SHAPE: none, and the hand's one span
    // copies PRESENT alone. L12 runs both shapes.
    static inline volatile LONG hasHudless = 0;
    static bool HasHudless() { return hasHudless != 0; }
    // WHETHER THE FRAME RUNS PIPELINED: off by default. (Lever 4's development key, EveryPair, honoured by the shadow shape
    // alone and refused by every binding that arms, went with that shape in round 9's strip.)
    static inline volatile LONG pipelined = 0;
    static bool Pipelined() { return pipelined != 0; }
    // LEVER 2'S SEAM (2026-10-10): the development key that puts 'Q' back at the hand. ShFresh sets it from gShAtHand: every
    // case of this file runs in BOTH orders (dt_fg_test.cpp, RunCases), the product's and the key's.
    static inline volatile LONG qAtHand = 0;
    static bool QAtHand() { return qAtHand != 0; }
    // 'Q'(n) WRITTEN BEFORE THE FRAME MESSAGE OF n (the body moves the sent count where it writes that message): the helper
    // would prepare guides no evaluate was queued for. Counted at the write; ShClean reads it.
    static inline volatile LONG qBeforeF = 0;
    // the worker's own time round its three notes: asked for once a note, counted (the harness measures nothing with it)
    static inline volatile LONG workAsked = 0, workAdded = 0;
    static LONGLONG WorkT() { _InterlockedIncrement(&workAsked); return 0; }
    static void WorkAdd(LONGLONG t0) { (void)t0; _InterlockedIncrement(&workAdded); }
    // THE PIPE, with the wire's own arguments, and THE WRITER RULE tested at the write (the wire's B11): the worker, or
    // the render thread with the ring drained, and never a third thread.
    static inline volatile LONG badWriter = 0;
    static inline HANDLE evHelper = nullptr;
    static void CheckWriter()
    {
        const DWORD t = GetCurrentThreadId();
        const bool ok = t == dtWorkerTid || (t == renderTid && (DWORD)dtDone == dtW && (DWORD)dtPub == dtW);
        if (!ok) _InterlockedIncrement(&badWriter);
    }
    // THE ORDER OF A FRAME as this binding saw it: 'Q' and 'G' at their writes, 'S' at each submit asked of it (taken, held or
    // refused). Appended by the pipe's writers only (one at a time, by the rule above), read when they are idle.
    static inline std::vector<char> seq;
    static void WriteQ(LONG n, DWORD flags) { CheckWriter(); if (n > FgHarnessBinding::sentN) _InterlockedIncrement(&qBeforeF); seq.push_back('Q'); PipeWrite('Q', n, flags); if (evHelper) SetEvent(evHelper); }
    static void WriteG(LONG n, DWORD kind)  { CheckWriter(); seq.push_back('G'); PipeWrite('G', n, kind);  if (evHelper) SetEvent(evHelper); }
    // the transport's build message, for the order. SINCE STAGE 6 OF PHASE 3 THE GAME'S SIDE OF THE BUILD IS MODELLED
    // WITH IT: the imports of the old set go, a new set is held (RingSetBuild), BEFORE the message, as the product
    // drops its imports before it writes it. The fake helper takes the new set when it reads the message.
    static void WriteB()                    { RingSetBuild(kSentinel); PipeWrite('B', 0, 0); if (evHelper) SetEvent(evHelper); }
    // THE SET'S IMAGES ARE THE HELPER'S MEMORY (stage 6; dt_fake_ring.h, "THE HELPER'S SET"): this binding acquires one
    // round every copy and releases it in the same buffer, as the product's lines do. The fake counts a copy made
    // without it. What the fake asks of this binding: the pair the worker has open, and which slots the set has.
    static void RecCopyToImg(ULONGLONG bb, int img)   { RecAcquire(img); FgHarnessBinding::RecCopyToImg(bb, img);   RecRelease(img); }
    static void RecCopyFromImg(int img, ULONGLONG bb) { RecAcquire(img); FgHarnessBinding::RecCopyFromImg(img, bb); RecRelease(img); }
    static LONG PairNow() { return x.w.openN; }
    static bool HasSlot(int slot)
    {
        return slot == FG_IMG_GEN2 ? (gShGenCounts & 4) != 0 : (slot != FG_IMG_HUDLESS || hasHudless != 0);   // (round 3, G3: GEN2 where the set makes two)
    }
    // THE QUEUE'S WAIT (above). guardOff is a control's: PrepGuard then queues nothing. All of it under the queue's lock.
    static inline volatile LONG guardOff = 0, guards = 0, heldN = 0, waitFor = 0;
    // EACH HELD BUFFER WITH THE VALUE OF fgPrep ITS OWN WAIT STANDS FOR (phase 3, lever 4): with every pair dispatched several
    // frames can be queued behind an unanswered prepare, each behind the wait-only batch of ITS injection, and a queue
    // starts a buffer when the wait ahead of IT is over, not when the newest one is. (Until then one value stood for
    // all of them, which was the same thing while at most one wait was ever outstanding.)
    // (stage 6: an entry is a submit's buffer OR a list of the command stream's work, in the one order they reached the queue)
    struct Held { LONG wait; bool cs; std::vector<RingRawOp> raw; std::vector<RingCsItem> list; };
    static inline std::vector<Held> heldCbs;
    // FK7's PLANTED LEAK: the stream's work runs though a wait queued before it is pending, and the fake counts it
    static inline volatile LONG csLeak = 0;
    static bool PrepGuard(LONG n)
    {
        if (guardOff) return false;
        RingQEnter(true);
        waitFor = n; _InterlockedIncrement(&guards);
        RingQLeave();
        return true;
    }
    // THE GATE BEFORE DXVK'S PRESENT (fg_machine.h's list; the product's is FgP::PresentGate over the release's core). The
    // harness has no fence of a helper's for it to hold on, so this one NOTES its call with the fake: its thread, and how
    // many times the fake Present had been entered by then. L9 reads those notes against the Present's own entries.
    static void PresentGate() { RingPreNoteCall(); }
    // Called with the submission queue held (a span's own lock, or QueueRun's) and no buffer of the caller's open. In queue
    // order: the buffers whose own wait is over run, and the first whose wait is not stops the rest behind it.
    static void RunHeldLocked()
    {
        const ULONGLONG prep = FgHarnessBinding::TlValue(FG_TL_PREP);
        size_t done = 0;
        while (done < heldCbs.size() && prep >= (ULONGLONG)heldCbs[done].wait)
        {
            if (heldCbs[done].cs) { EnterCriticalSection(&gRing.data); RingApplyCsLocked(heldCbs[done].list); LeaveCriticalSection(&gRing.data); }
            else { gRing.cb = heldCbs[done].raw; gRing.cbOpen = true; FgHarnessBinding::RecSubmit(); }
            done++;
        }
        heldCbs.erase(heldCbs.begin(), heldCbs.begin() + (ptrdiff_t)done);
        if (heldCbs.empty() && prep >= (ULONGLONG)waitFor) waitFor = 0;
    }
    // THE COMMAND STREAM'S GATE (stage 6; dt_fake_ring.h, RingModel::csGate): the stream's list is about to reach the queue,
    // which the caller holds. With no wait queued it runs. With one that is over, what the wait held goes first. WITH
    // ONE PENDING THE LIST IS TAKEN AND HELD BEHIND IT, in order with the submits: so a Present made after the wait was
    // queued is not on the screen until fgPrep arrives, and neither is one made before it whose blit DXVK's command
    // thread had not yet handed over (the three modes of gRing.presentCs say how far that thread got).
    static bool CsGate()
    {
        if (!waitFor) return false;
        if (FgHarnessBinding::TlValue(FG_TL_PREP) >= (ULONGLONG)waitFor)
        {
            std::vector<RingRawOp> mine; mine.swap(gRing.cb); const bool open = gRing.cbOpen;
            RunHeldLocked();
            gRing.cb.swap(mine); gRing.cbOpen = open;
            return false;
        }
        EnterCriticalSection(&gRing.data);
        if (gRing.cs.empty()) { LeaveCriticalSection(&gRing.data); return true; }
        if (csLeak) { LeaveCriticalSection(&gRing.data); _InterlockedIncrement(&gRingSt.csPastWait); return false; }
        heldCbs.push_back(Held{ (LONG)waitFor, true, {}, gRing.cs }); gRing.cs.clear();
        LeaveCriticalSection(&gRing.data);
        _InterlockedIncrement(&gRingSt.csHeld);
        return true;
    }
    static bool RecSubmit()
    {
        seq.push_back('S');
        if (waitFor)
        {
            if (FgHarnessBinding::TlValue(FG_TL_PREP) >= (ULONGLONG)waitFor)
            {   // the wait is over: what it held goes first, then this one
                std::vector<RingRawOp> mine; mine.swap(gRing.cb); const bool open = gRing.cbOpen;
                RunHeldLocked();
                gRing.cb.swap(mine); gRing.cbOpen = open;
            }
            else if (gRing.cbOpen && gRing.submitFailNext <= 0)
            {
                heldCbs.push_back(Held{ (LONG)waitFor, false, gRing.cb, {} }); gRing.cb.clear(); gRing.cbOpen = false;
                _InterlockedIncrement(&heldN);
                return true;                                  // taken by the queue, behind its wait
            }
        }
        return FgHarnessBinding::RecSubmit();
    }
    // fgPrep moved (the fake helper), or the wait was released from the CPU: what the queue held runs now.
    static void QueueRun()
    {
        RingQEnter(true);
        if (waitFor) RunHeldLocked();
        RingQLeave();
    }
    // THE RELEASE OF THE QUEUE'S WAIT (the transport's, stage 10; here its model): fgPrep signalled from the CPU up to THE
    // TOP THE MACHINE RECORDED before it queued the wait (FgShadowShared::prepTop), and the queue runs. A top that was not
    // recorded releases nothing.
    static void ReleaseWait()
    {
        const LONG v = x.sh.prepTop;
        if (v && FgHarnessBinding::TlValue(FG_TL_PREP) < (ULONGLONG)v) RingTlSignal(FG_TL_PREP, (ULONGLONG)v, false);
        QueueRun();
    }
};

// ------------------------------------------------------------------ the injection's body (lever 2, 2026-10-10)
// What the transport's body does as far as the machine and the fake helper can see, ON THE WORKER, INSIDE THE SAVED
// OBSERVER: the frame message goes out (the sent count moves, the frame's ordinal is recorded) and the frame's submit
// signals the frame's in-fence. By default the signal is made at once. With gShFullBody the body makes A REAL SUBMIT
// through this binding's queue, which carries the signal: then the queue's wait (G4) holds it as it holds any batch,
// which is what a guard that named its own frame's 'Q' would turn into a queue that never moves.
// gShBodySawQ: this very injection's 'Q' was on the pipe when its body ended. It never may be.
static const DWORD kShInj = 0x4E494853u;
static LONG gShAtHand = 0;                       // the arm: 1 the development key fg_q_at_hand (set by RunCases, read by ShFresh)
static volatile LONG gShFullBody = 0, gShBodySawQ = 0;
static void ShInjBody(const DWORD* payload, DWORD bytes)
{
    DWORD pay[4] = {}; memcpy(pay, payload, bytes < sizeof(pay) ? bytes : sizeof(pay));
    if (pay[0] != kShInj || !(pay[1] & 1u)) return;           // not sent: no frame message, and the count stands
    const LONG n = ++B::sentN; B::latOrd[n & 63] = (LONG)pay[2];
    if (gShFullBody)
    {
        if (ShB::CbTake(1)) { ShB::QLock(); ShB::RecBegin(); ShB::RecPatch(FG_IMG_SAVE); ShB::RecSignal(FG_TL_LOCAL, n); ShB::RecSubmit(); ShB::QUnlock(); }
    }
    else RingTlSignal(FG_TL_LOCAL, (ULONGLONG)n, false);
    for (const RingPipe& r : RingPipeLog()) if (r.type == 'Q' && r.p == n) _InterlockedIncrement(&gShBodySawQ);
}

// ------------------------------------------------------------------ the fake helper
struct ShHelper
{
    HANDLE thr, go;
    volatile LONG quit, next, busy;
    volatile LONG holdPrep, holdDispatch, inPrep, inDispatch, noFrame, dead;
    DWORD inWaitMs;
    LONG qN, gN, qOpenN; bool qOpen;
    volatile LONG cQ, cG, cDispatch, cNone, cDup, cDrift, cB, openAtBuild, stuck, waitedIn;
    volatile LONG cReset[4], byKind[4];
    // WIRE 2 (2026-10-06): the open 'Q''s reasons, kept as the real helper keeps them; the reasons a 'G' carried that its
    // 'Q' did not (counted once a pair, as the real helper counts: none while the game writes the same four on both);
    // 'G' read with any reason; and 'Q' read with PIPELINED
    LONG qOpenWhy;
    volatile LONG cResetG[4], whyOnG, cPipelined;
    // LEVER 2 (2026-10-10): the prepare of pair n stands behind THE FRAME'S IN-FENCE at n on the real helper's queue (the wait
    // the frame message queued). Here the body signals it (FG_TL_LOCAL, a timeline the product's shape leaves free), the
    // fake prepare waits for it, bounded, and a value nothing signals is counted
    volatile LONG prepStuck;
    // ---- PHASE 3, STAGE 6 (2026-10-10): what the merged path will be proved against
    // WHICH DISPATCH RESET, by the real helper's own lines (host/fg_host.h, FgOnG): a pair that gave no frame sets
    // resetNext; a dispatch resets when the game gave a reason on either message (told) or resetNext stands, and clears
    // it; a reset with no reason given is the helper's OWN. A build sets it. noOwnReset is FK2's control: a helper that
    // forgets "the pair before gave no frame".
    volatile LONG resetNext, noOwnReset, cResetOwn, cDispReset;
    // THE GENERATED IMAGE'S TAG NAMES ITS PAIR (ShGenTag). genPlain is FK1's control: the tag as it was until this stage,
    // PRESENT's own, which no reader can tell from a real frame's.
    volatile LONG genPlain;
    // AN ANSWER DELAYED ON THE HARNESS'S CLOCK: a dispatch answers when B::Now() has reached the moment its wait on fgIn
    // was over plus this many microseconds. The harness steps that clock; the real one moving changes nothing.
    // delayBreak is FK3's planted fault (the helper does not wait), and earlyAns counts every answer signalled before
    // its time.
    volatile LONG delayUs, delayBreak, earlyAns;
    // (stage 7b) THE PAIR WHOSE ANSWER IT IS DELAYING NOW (0: none): set once the moment its wait on fgIn was over has been
    // read from the harness's clock, so a case can step that clock knowing the delay counts from where it thinks
    volatile LONG inDelay;
    // A HELPER THAT IS GONE (ShHelperGone): it reads nothing more, and its two fences go to the top once, on its thread
    volatile LONG gone, topped;
    // ITS OWN SET: the three images as it took them at the last build message it read, and that set's generation
    // (2026-10-18, round 3, G3: and the fourth, GEN2, the second store's, where the set has it)
    LONG setGen; int img[RING_SET_SLOTS];
    // (2026-10-18, round 3, G3) WIRE 3'S COUNTS, as the real helper reads them off a 'G' of a set of wire 3 (feed_ipc.h, FEED_FG_G_GEN_*): flow-only
    // dispatches made (the flow and the store, no picture, answered 2n + 1, the history kept), two-frame dispatches made, and counts asked that
    // the set does not make (answered 2n + 1, the next dispatch reset, as the real helper's)
    volatile LONG cFlow, cTwo, cCountRefused;
    // (2026-10-17, dt_fg_rates.h's RtPar::prepAtRet) THE PREPARE OF PAIR n STANDS BEHIND fgIn AT n TOO: the game's post chain displaces it on the
    // card until about its handoff's retire. 0 everywhere but the replays that ask for it
    volatile LONG prepAtIn;
    // THE SECOND INPUT SET'S BITS, as the real helper reads them (host/fg_host.h, FgOnQ and FgOnG) in a set without it, which every set of the
    // mod's is since round 8's deep strip: a 'Q' naming the second set (not prepared: its 'G' answered with no frame); a 'G' naming it (never
    // dispatched, the next dispatch reset). Both must stay 0 (ShClean)
    bool qBad;
    volatile LONG cSetBad, cSetMix;
};
static ShHelper gH;
static void (*volatile gShAfterOne)() = nullptr;   // (2026-10-14) behind every message the fake helper has served, on its thread (dt_fg_late.h sets it; off everywhere else)
// THE GENERATED IMAGE'S TAG (stage 6). Until now the fake helper put PRESENT's own tag into GEN, so a generated image on
// the screen read as the real frame. Now it is THE PICTURE IT WAS MADE FROM UNDER A MARK OF ITS OWN in the tag's top byte
// (a frame's tag has none: NewTag counts up from 0x00A00000; a sentinel's is 0xDE, the rig's patch and black 0xFF):
// 'n' for a dispatch that did not reset, 'r' for one that did. So the screen's log reads N(n-1), GEN(n), N(n), and a
// stale image reads as GEN of the pair it was made for.
static const DWORD kShGenMark = 0x6E000000u, kShGenResetMark = 0x72000000u;
static DWORD ShGenTag(DWORD picture, bool reset) { return (reset ? kShGenResetMark : kShGenMark) | (picture & 0x00FFFFFFu); }
static bool  ShIsGen(DWORD tag) { const DWORD top = tag & 0xFF000000u; return top == kShGenMark || top == kShGenResetMark; }
static bool  ShIsGenReset(DWORD tag) { return (tag & 0xFF000000u) == kShGenResetMark; }
static DWORD ShGenPicture(DWORD tag) { return ShIsGen(tag) ? (tag & 0x00FFFFFFu) : 0u; }   // the frame's tag a generated image was made from (0: not a generated image)
// (2026-10-18, round 3, G3) THE SECOND GENERATED IMAGE'S TAG, GEN2's (a two-frame dispatch, the second at two thirds): a mark of its own, 't' (and 'u'
// for a reset dispatch's), so the screen's log reads N(n-1), GEN(n), GEN2(n), N(n) and a reader tells the two apart. ShIsGen stays GEN's alone:
// every reader before dt_fg_fill.h's sees a GEN2 picture as no frame of a pair (it never appears there: no case before asks for two)
static const DWORD kShGen2Mark = 0x74000000u, kShGen2ResetMark = 0x75000000u;
static DWORD ShGen2Tag(DWORD picture, bool reset) { return (reset ? kShGen2ResetMark : kShGen2Mark) | (picture & 0x00FFFFFFu); }
static bool  ShIsGen2(DWORD tag) { const DWORD top = tag & 0xFF000000u; return top == kShGen2Mark || top == kShGen2ResetMark; }
static DWORD ShGen2Picture(DWORD tag) { return ShIsGen2(tag) ? (tag & 0x00FFFFFFu) : 0u; }
// (the generated counts the fake's set makes, gShGenCounts, stand above ShB, whose slots name GEN2 by them)
// EVERY DISPATCH THAT GAVE A FRAME, in order (under gRing.data): its pair, whether it reset and on whose account, what
// PRESENT held, and the tag it put into GEN. hud (2026-10-11): what HUDLESS held at that moment, the picture the real
// helper's library reads as the HUD-less colour (a set with no such image: its sentinel).
struct ShDisp { LONG n; bool reset, own; DWORD from, tag, hud; };
static std::vector<ShDisp> gShDisp;
static std::vector<ShDisp> ShDispLog() { EnterCriticalSection(&gRing.data); auto v = gShDisp; LeaveCriticalSection(&gRing.data); return v; }
static void ShHelperBusy(LONG on) { _InterlockedExchange(&gRing.set.genBusyNow, on); _InterlockedExchange(&gRing.set.inBusyNow, on ? 1 : 0); }
static void ShHelperNoFrame() { if (!gH.noOwnReset) _InterlockedExchange(&gH.resetNext, 1); }   // (the real helper: `if (!generated) { cNone++; resetNext = true; }`)
static void ShHelperTakeSet()
{
    EnterCriticalSection(&gRing.data);
    for (int i = 0; i < RING_SET_SLOTS; i++) gH.img[i] = gRing.rigImg[RingSlotImg(i)];
    gH.setGen = gRing.set.bgen;
    LeaveCriticalSection(&gRing.data);
}
static void ShHelperHold(volatile LONG* in)
{
    _InterlockedExchange(in, 1);
    WaitForSingleObject(gH.go, INFINITE);
    _InterlockedExchange(in, 0);
}
static void ShHelperOne(const RingPipe& r)
{
    const ULONGLONG n = (ULONGLONG)r.p;
    if (r.type == 'B')
    {
        if (gH.qOpen) _InterlockedIncrement(&gH.openAtBuild); gH.qOpen = false; _InterlockedIncrement(&gH.cB);
        ShHelperTakeSet(); _InterlockedExchange(&gH.resetNext, 1);                           // (stage 6: the new set is this helper's now, and its first dispatch resets)
        return;
    }
    if (r.type == 'Q')
    {
        if (r.p <= gH.qN) { _InterlockedIncrement(&gH.cDup); return; }                       // DUPLICATE: counted, never answered
        if (gH.qOpen) _InterlockedIncrement(&gH.cDrift);                                     // DRIFT: a 'Q' while a 'Q' is open
        gH.qN = r.p; gH.qOpen = true; gH.qOpenN = r.p; _InterlockedIncrement(&gH.cQ);
        gH.qOpenWhy = (LONG)(r.flags & 15u);
        for (int b = 0; b < 4; b++) if (r.flags & (1u << b)) _InterlockedIncrement(&gH.cReset[b]);
        if (r.flags & 32u) _InterlockedIncrement(&gH.cPipelined);
        // (the real helper's FgOnQ: a 'Q' naming the second input set in a set without it, refused)
        gH.qBad = (r.flags & FG_Q_SET1) != 0;
        if (gH.qBad) _InterlockedIncrement(&gH.cSetBad);
        if (!ShB::TlWait(FG_TL_LOCAL, n, gH.inWaitMs)) _InterlockedIncrement(&gH.prepStuck);   // (the frame's input never retired: the real queue would stand)
        // (2026-10-17: displaced to its handoff's retire)
        if (gH.prepAtIn && !ShB::TlWait(FG_TL_IN, n, gH.inWaitMs)) _InterlockedIncrement(&gH.prepStuck);
        if (_InterlockedExchange(&gH.holdPrep, 0)) ShHelperHold(&gH.inPrep);
        if (gH.dead || gH.quit) return;
        RingTlSignal(FG_TL_PREP, n, false);
        ShB::QueueRun();
        return;
    }
    if (r.type != 'G') return;
    const DWORD kind = r.flags & 3u;
    if (r.p <= gH.gN) { _InterlockedIncrement(&gH.cDup); return; }
    gH.gN = r.p; _InterlockedIncrement(&gH.cG);
    _InterlockedIncrement(&gH.byKind[kind]);
    bool told = false;                                        // (stage 6) the game gave a reset's reason for this pair, on either message
    {   // wire 2: the reasons on 'G' (bits 4 to 7), counted where its 'Q' did not carry them
        const DWORD why = (r.flags >> FG_G_RESET_SHIFT) & 15u, qWhy = (gH.qOpen && gH.qOpenN == r.p) ? (DWORD)gH.qOpenWhy : 0u;
        if (why) _InterlockedIncrement(&gH.whyOnG);
        for (int b = 0; b < 4; b++) if ((why & ~qWhy) & (1u << b)) _InterlockedIncrement(&gH.cResetG[b]);
        told = (why | qWhy) != 0;
    }
    if (!gH.qOpen || gH.qOpenN != r.p) { _InterlockedIncrement(&gH.cDrift); _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }   // DRIFT: answered, with no wait
    gH.qOpen = false;
    if (kind != FG_G_DISPATCH) { _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }                                        // no wait on fgIn for any other kind
    // (the real helper's FgOnG: a DISPATCH naming the second input set in a set without it is NEVER DISPATCHED: no wait, answered with no frame,
    // the next dispatch resets; and a 'Q' refused for its set was not prepared)
    {
        const bool gs = (r.flags & FG_G_SET1) != 0;
        if (gs) _InterlockedIncrement(&gH.cSetMix);
        if (gs || gH.qBad) { _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }
    }
    // DISPATCH: the one kind that waits on fgIn. A value nothing signals leaves the real helper's queue stuck; here the
    // wait is bounded and the case reads `stuck`.
    _InterlockedIncrement(&gH.waitedIn);
    if (!ShB::TlWait(FG_TL_IN, n, gH.inWaitMs)) { _InterlockedIncrement(&gH.stuck); _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }
    // INSIDE THE DISPATCH (stage 6): from here to just before its answer the helper reads PRESENT and HUDLESS and writes
    // GEN, and the fake's set says so (a copy out of GEN meanwhile is counted). t0: the moment the wait on fgIn was over,
    // on the harness's clock: a delayed answer is counted from it.
    // (round 5: A FLOW-ONLY DISPATCH WRITES NO PICTURE, as the real helper's (host/fg_own.h, DLSSIV_FG_STORE_ONLY): it reads the inputs, so they
    // are busy, but GEN is not: a held pair's kept frame read out of GEN meanwhile is GEN's own picture, never a write in flight)
    { const DWORD c0 = (r.flags & FG_G_COUNTED) ? ((r.flags & FG_G_COUNT_MASK) >> FG_G_COUNT_SHIFT) : 1u; if (c0 == 0) _InterlockedExchange(&gRing.set.inBusyNow, 1); else ShHelperBusy(1); }
    const LONGLONG t0 = B::Now(), due = t0 + (LONGLONG)gH.delayUs * B::Qpf() / 1000000;
    if (_InterlockedExchange(&gH.holdDispatch, 0)) ShHelperHold(&gH.inDispatch);
    if (gH.dead || gH.quit) { ShHelperBusy(0); return; }
    if (gH.delayUs > 0) _InterlockedExchange(&gH.inDelay, r.p);
    if (gH.delayUs > 0 && !gH.delayBreak) while (!gH.dead && !gH.quit && B::Now() < due) Sleep(1);
    _InterlockedExchange(&gH.inDelay, 0);
    if (gH.dead || gH.quit) { ShHelperBusy(0); return; }
    if (gH.noFrame) { ShHelperBusy(0); _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }
    if (gH.delayUs > 0 && B::Now() < due) _InterlockedIncrement(&gH.earlyAns);              // (counted: an answer before its time. Only the planted fault makes one)
    // (2026-10-18, round 3, G3: WIRE 3'S COUNT, as the real helper reads it: 0 THE FLOW AND THE STORE ALONE, no picture, answered 2n + 1, and the
    // history kept, so the next dispatch does not reset; 2 TWO PICTURES, GEN at a third and GEN2 at two thirds, one answer; a count the set does
    // not make answered 2n + 1 with the next dispatch reset. A word that names no count asks for one, as every 'G' before wire 3)
    const DWORD cnt = (r.flags & FG_G_COUNTED) ? ((r.flags & FG_G_COUNT_MASK) >> FG_G_COUNT_SHIFT) : 1u;
    if (cnt > 2 || !((gShGenCounts >> cnt) & 1) || (cnt == 2 && gH.img[FG_IMG_GEN2] <= 0))
    { ShHelperBusy(0); _InterlockedIncrement(&gH.cCountRefused); _InterlockedIncrement(&gH.cNone); ShHelperNoFrame(); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return; }
    if (cnt == 0)
    {
        _InterlockedExchange(&gH.resetNext, 0); _InterlockedIncrement(&gH.cFlow); _InterlockedIncrement(&gH.cDispatch);
        ShHelperBusy(0); RingTlSignal(FG_TL_OUT, 2 * n + 1, false); return;
    }
    // THE GENERATED FRAME: the picture PRESENT held, under the mark that says it is a generated one, and whether this
    // dispatch reset (the real helper's line: `FgDispatchSubmit(told || g_fg->resetNext, ...)`, then its own count)
    const bool reset = told || gH.resetNext != 0, own = gH.resetNext != 0 && !told;
    const int inP = FG_IMG_PRESENT, inH = FG_IMG_HUDLESS;
    const DWORD from = RingTag(gH.img[inP]), tag = gH.genPlain ? from : ShGenTag(from, reset);
    RingSetTag(gH.img[FG_IMG_GEN], tag);
    if (cnt == 2) { RingSetTag(gH.img[FG_IMG_GEN2], ShGen2Tag(from, reset)); _InterlockedIncrement(&gH.cTwo); }   // (round 3, G3: the second, in the same dispatch)
    EnterCriticalSection(&gRing.data);
    if (gH.setGen == gRing.set.bgen) { gRing.set.genN = r.p; gRing.set.genResetNow = reset ? 1 : 0; gRing.set.wrote[FG_IMG_GEN] = gRing.set.bgen; if (cnt == 2) gRing.set.wrote[FG_IMG_GEN2] = gRing.set.bgen; }
    gShDisp.push_back({ r.p, reset, own, from, tag, gRing.img[gH.img[inH]].tag });
    LeaveCriticalSection(&gRing.data);
    if (reset) _InterlockedIncrement(&gH.cDispReset);
    if (own) _InterlockedIncrement(&gH.cResetOwn);
    _InterlockedExchange(&gH.resetNext, 0);
    _InterlockedIncrement(&gH.cDispatch);
    ShHelperBusy(0);                                          // (before the answer: a reader that saw the answer finds GEN whole)
    RingTlSignal(FG_TL_OUT, 2 * n, false);
}
static DWORD WINAPI ShHelperMain(void*)
{
    for (;;)
    {
        WaitForSingleObject(ShB::evHelper, 2);
        // A HELPER THAT IS GONE (stage 6): the two fences it answers on go to the top, once, as the real helper's exit
        // signals them and as the kernel leaves a dead one's; what the queue held behind fgPrep runs. It reads nothing more.
        if (gH.gone && !_InterlockedExchange(&gH.topped, 1))
        {
            ShHelperBusy(0);
            RingTlSignal(FG_TL_PREP, ~0ull, false); RingTlSignal(FG_TL_OUT, ~0ull, false);
            ShB::QueueRun();
            if (void (*f)() = gShAfterOne) f();
        }
        if (gH.quit) break;
        if (gH.gone) continue;
        const std::vector<RingPipe> log = RingPipeLog();
        while (!gH.quit && !gH.gone && (size_t)gH.next < log.size())
        {
            _InterlockedExchange(&gH.busy, 1);
            ShHelperOne(log[(size_t)gH.next]);
            if (void (*f)() = gShAfterOne) f();               // (2026-10-14: a case's stage of the queue runs what an answer released, before this thread reads as idle)
            _InterlockedIncrement(&gH.next);
            _InterlockedExchange(&gH.busy, 0);
        }
    }
    return 0;
}
static void ShHelperStop()
{
    if (!gH.thr) return;
    _InterlockedExchange(&gH.quit, 1); SetEvent(gH.go); SetEvent(ShB::evHelper);
    Join(gH.thr, "the fake helper");
    gH.thr = nullptr;
}
static void ShHelperStart()
{
    if (!ShB::evHelper) ShB::evHelper = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    const HANDLE go = gH.go ? gH.go : CreateEventW(nullptr, FALSE, FALSE, nullptr);
    memset((void*)&gH, 0, sizeof(gH));
    gH.go = go; gH.inWaitMs = 400; ResetEvent(go);
    // (stage 6) the set a fresh product holds is this helper's, and its first dispatch resets: a set was just built
    ShHelperTakeSet(); gH.resetNext = 1;
    EnterCriticalSection(&gRing.data); gShDisp.clear(); LeaveCriticalSection(&gRing.data);
    gH.thr = Spawn(ShHelperMain);
}
// THE HELPER IS GONE (stage 6): killed or exited. Whatever it was held in is left; its fences go to the top on its own
// thread (ShHelperMain). A case that makes it go reads no settle afterwards: it reads nothing more.
static void ShHelperGone() { _InterlockedExchange(&gH.gone, 1); _InterlockedExchange(&gH.dead, 1); SetEvent(gH.go); SetEvent(ShB::evHelper); }
// The helper has read everything written so far and is not inside a message (a held one is "inside": a case that holds
// the helper waits for inPrep or inDispatch instead).
static bool ShSettle(DWORD ms = 3000)
{
    return WaitFor([] { return !gH.busy && (size_t)gH.next >= RingPipeLog().size() && !gH.busy; }, ms, 6);
}
static void ShRelease() { SetEvent(gH.go); }

// ------------------------------------------------------------------ a fresh product, and its frames
// THE STAMPERS, UP BEFORE THE FIRST FRAME (dt_fg_merged.h, MgStartThreads, over the merged machine). The harness's
// StamperMain starts from the timeline's value when its thread first runs: a thread that started late (a loaded machine)
// skipped for good every value signalled before it, and a case that waits for one of those stamps failed (G8, once, in a
// whole run of the harness on 2026-10-06). These start from 0, where Fresh leaves every timeline. The OUT stamper wakes the
// placer too, as the product's must: the answer's stamp is what completes a pair's K.
static volatile LONG gShUp = 0;
template <class MM> static DWORD WINAPI ShStamperMain(void* a)
{
    const int tl = (int)(INT_PTR)a;
    LONG last = 0;
    _InterlockedIncrement(&gShUp);
    while (!gThrQuit)
    {
        // A FENCE AT THE TOP IS A HELPER THAT IS GONE (stage 6): a wait on it returns at once, and the step would run flat
        // out until the stop. The product's stamper has this line (fg_threads.h, FgThreadStamper); this one is the
        // harness's own stand-in for that loop, so it has it too.
        if (B::TlValue(tl) >= 0x7FFFFFFFull) { WaitForSingleObject(gThrQuitEv, 20); continue; }
        if (MM::StampStep(tl, &last, 250) && tl != FG_TL_PREP) SetEvent(B::evPlacer);
    }
    return 0;
}
static void ShArmReset();                        // (the merged binding's own state and knobs, below with the binding: stage 7)
// MM: a machine over ShArmB or a binding derived from it (the state and the helper are ShB's). NO THREAD IS STARTED HERE (the
// shadow shape's stampers and placer were, until round 9's strip, and dt_fg_merged.h's MgFresh stopped them at once).
template <class MM> static void ShFresh(int images = 3)
{
    ShHelperStop();
    Opt o; o.fakeClock = true; o.images = images;
    Fresh<MM>(o);
    memset((void*)&ShB::x, 0, sizeof(ShB::x));
    ShArmReset();
    ShB::mayEngage = 1; ShB::badWriter = 0; ShB::guardOff = 0; ShB::guards = 0; ShB::heldN = 0; ShB::waitFor = 0; ShB::heldCbs.clear();
    ShB::seq.clear(); ShB::injSeen = 0; ShB::workAsked = 0; ShB::workAdded = 0;
    ShB::hasHudless = 0;                                      // the game's shape: a set with no HUD-less image
    ShB::pipelined = 0;                                       // same frame
    ShB::qAtHand = gShAtHand; ShB::qBeforeF = 0;              // lever 2: the order of 'Q' this arm runs in
    gInjBody = ShInjBody; gShFullBody = 0; gShBodySawQ = 0;   // the injection's body, inside the saved observer (the worker's)
    // (stage 6) THE HELPER'S SET: the rig's three images as the fresh rig left them are the helper's from here, with the
    // set's rules; and the command stream's work stands behind this binding's queued wait
    ShB::csLeak = 0;
    RingSetFirst(&ShB::PairNow, &ShB::HasSlot);
    gRing.csGate = &ShB::CsGate;
    MM::BeforeRelease();                                      // the session's first build: generation 1
    ShHelperStart();
}
static void ShEnd()
{
    ShHelperStop(); StopThreads(); B::fakeClock = false; gInjBody = nullptr;
    // (stage 6) the stream's gate and the set's rules are this file's cases' alone: the rig's cases run as they did
    gRing.csGate = nullptr; RingSetOff(); ShB::heldCbs.clear(); ShB::waitFor = 0; ShB::csLeak = 0;
    gRing.back.on = 0;                                        // (stage 7: the back buffer's save rule is the merged path's cases' alone)
}
// THE INJECTION OF THE FRAME ABOUT TO BE MADE (fEnd: the frame it belongs to, `frames` before Facts moves it). sent: its
// body sends the frame message (the count moves, on the worker, inside the saved observer).
static void ShInject(bool sent = true, LONG fEnd = -1)
{
    const DWORD pay[4] = { kShInj, sent ? 1u : 0u, (DWORD)(fEnd < 0 ? frames : fEnd), 0 };
    _InterlockedIncrement(&gInjPending);
    if (!DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) { _InterlockedDecrement(&gInjPending); Err("the injection note was refused"); }
}
// One frame as the game makes it: the injection mid-frame, then the frame's end and its Present; executed, and the helper
// has read what it wrote. 16.7 ms on the harness's clock, so the facts see an even run.
template <class MM> static DWORD ShFrame(bool sent = true, bool settle = true)
{
    ClockAdd(16.7);
    ShInject(sent);
    const DWORD tag = Frame<MM>(sent);
    Drain();
    if (settle && !ShSettle()) Err("the fake helper did not read the frame's messages within 3 s");
    return tag;
}
// The first four pairs of a session are withheld by rule (fewer than four samples of the frame time): run them out.
template <class MM> static void ShWarm() { for (int i = 0; i < 6; i++) ShFrame<MM>(); }
// A FRAME RECORDED BY HAND, as far as its hand: the injection, the facts, the fill, and FG_HAND as FgSwapPresent records
// it for this binding (the build generation travels in the payload's hold). kOff moves the ordinal the hand names away
// from the next Present's, which is how "a Present that never matched" is made.
// THE INJECTION'S FACT IS THE WORKER'S HERE, AS IN THE MOD: there the injection is sent inside its own note, on the worker,
// and that is where the sent count moves. SINCE LEVER 2 (2026-10-10) IT MOVES INSIDE THE SAVED OBSERVER ITSELF (ShInjBody,
// installed as the harness's injection body), for every frame of this file: until then it moved on the render thread at
// record time (Facts) or in a note of its own behind the injection's, and a machine that writes 'Q' behind the saved
// observer would have seen an injection that was "not sent yet".
// (This is also why the product needs the injection on the worker: with it on the render thread the count is ahead of
// a worker that runs a frame behind.)
static bool gShNoEntry = false;                  // L14's control: a hand recorded by the harness carries no entry time
static DWORD gShHandWithhold = 0;                // L18: a render fact the next hand recorded by the harness carries (a camera cut)
// (mid: called between the injection's note and the hand's: a case queues a note of its own there)
static bool ShHandFrame(LONG bgen = -1, LONG kOff = 0, bool inject = true, void (*mid)() = nullptr)
{
    frames++; B::frames = frames;
    if (inject) ShInject(true, frames - 1);
    if (mid) mid();
    Fill(NewTag());
    FgState& s = B::S();
    FgHandPayload h = {};
    h.k = dtPresRecN + 1 + kOff; h.gen = dtArmCount; h.p = s.r.nextP; h.fEnd = B::frames - 1; h.tEntry = gShNoEntry ? 0 : B::Now();
    h.hold = bgen < 0 ? ShB::x.sh.bgen : bgen;
    h.withhold = gShHandWithhold;
    if (!DtNote(DT_NOTE_FG_HAND, &h, sizeof(h))) { Err("a hand recorded by the harness was refused"); return false; }
    s.r.lastP = h.p; s.r.lastGen = h.gen; s.r.lastEngaged = true; s.r.nextP++;
    return true;
}
static const RingPipe* ShFind(const std::vector<RingPipe>& log, char type, LONG n)
{
    for (const RingPipe& r : log) if (r.type == type && r.p == n) return &r;
    return nullptr;
}
static int ShCount(const std::vector<RingPipe>& log, char type) { int c = 0; for (const RingPipe& r : log) c += r.type == type; return c; }
static int ShKindOf(const std::vector<RingPipe>& log, LONG n) { const RingPipe* g = ShFind(log, 'G', n); return g ? (int)(g->flags & 3u) : -1; }
static DWORD ShQFlags(const std::vector<RingPipe>& log, LONG n) { const RingPipe* q = ShFind(log, 'Q', n); return q ? q->flags : 0xFFFFu; }
// The reasons pair n's 'G' carries (wire 2: bits 4 to 7), 0xFFFF when it has none.
static DWORD ShGWhy(const std::vector<RingPipe>& log, LONG n) { const RingPipe* g = ShFind(log, 'G', n); return g ? ((g->flags >> FG_G_RESET_SHIFT) & 15u) : 0xFFFFu; }
// WHERE A CAMERA CUT IS SAID (lever 2, 2026-10-10). With 'Q' at the hand (the development key, and the order until this
// date) the hand knows the cut when it writes 'Q': the cut is ON 'Q', and repeated on 'G'. With 'Q' behind the
// injection's body the cut is not known yet: 'Q' carries NO cut, and it ARRIVES ON 'G', which the hand's facts fill.
static bool ShCutSaid(const std::vector<RingPipe>& log, LONG n)
{
    const DWORD q = ShQFlags(log, n), g = ShGWhy(log, n);
    if (q == 0xFFFFu) return false;
    return gShAtHand ? (q & FG_Q_CUT) != 0 : (!(q & FG_Q_CUT) && g != 0xFFFFu && (g & FG_Q_CUT));
}
// Every 'Q' has exactly one 'G' after it and before the next 'Q'; no 'G' without its 'Q'. Returns the pairs.
static int ShPairs(const std::vector<RingPipe>& log, std::string* bad)
{
    LONG open = 0; int pairs = 0;
    for (const RingPipe& r : log)
    {
        if (r.type == 'Q') { if (open && bad->empty()) *bad = Fmt("'Q'(%ld) while 'Q'(%ld) has no 'G'", r.p, open); open = r.p; pairs++; }
        else if (r.type == 'G') { if (open != r.p && bad->empty()) *bad = Fmt("'G'(%ld) and the open 'Q' is %ld", r.p, open); open = 0; }
    }
    if (open && bad->empty()) *bad = Fmt("'Q'(%ld) never got its 'G'", open);
    return pairs;
}
static const char* ShKindName(int k) { return k == (int)FG_G_DISPATCH ? "DISPATCH" : k == (int)FG_G_CLOSED ? "CLOSED" : k == (int)FG_G_NOSOURCE ? "NOSOURCE" : k == (int)FG_G_WITHHELD ? "WITHHELD" : "MISSING"; }
// What every clean leg must leave: nothing the helper counts as a fault, and no write off the pipe's writer threads.
static void ShClean(const char* tag)
{
    if (gH.cDup || gH.cDrift || ShB::badWriter) Err("%s: DUPLICATE %ld, DRIFT %ld, writes off the writer threads %ld", tag, gH.cDup, gH.cDrift, ShB::badWriter);
    // lever 2: no 'Q' ahead of its frame message, none on the pipe when its own injection's body ended, no prepare left
    // behind an input nothing retired; and with the key, no 'Q' written at an injection at all
    if (ShB::qBeforeF || gShBodySawQ || gH.prepStuck) Err("%s: 'Q' written ahead of its frame message %ld time(s), on the pipe at its own body's end %ld time(s); the fake prepare stuck behind the frame's input %ld time(s)", tag, ShB::qBeforeF, gShBodySawQ, gH.prepStuck);
    if (gShAtHand && ShB::x.e.written) Err("%s: under the key %ld 'Q' were written at an injection", tag, ShB::x.e.written);
    // (THE MACHINE NEVER NAMES THE SECOND INPUT SET: no 'Q' and no 'G' carries its bit)
    if (gH.cSetMix || gH.cSetBad) Err("%s: THE SECOND INPUT SET NAMED: %ld 'G' and %ld 'Q' carried its bit, which no set of the mod's has", tag, gH.cSetMix, gH.cSetBad);
}

// ---------------------------------------------------------------- the gate's reader (M9's)
// (the shadow shape's cases L1 to L19, G4 and G8 were here, over a binding that could not arm: they went in round 9's strip with
// that binding. Their L9 read where the machine calls the gate before DXVK's Present; M9 reads it over the merged machine with
// this reader, and L20, at this file's end, runs the gate's core across the machine's install.)
struct ShGateRead { int presents = 0, bare = 0, twice = 0, misplaced = 0, onWorker = 0, onRender = 0, loose = 0; };
// The presents entered since e0 against the gate's calls noted since n0. For each present, the calls noted between the
// entry before it and its own: one is wanted, on the present's own thread, made when exactly that many presents had been
// entered. bare: none; twice: more than one; misplaced: one, of another thread's or another moment's; loose: calls with
// no present after them.
static ShGateRead ShGateSince(size_t e0, size_t n0)
{
    const std::vector<RingEntered> en = RingEnteredLog(); const std::vector<RingPreNote> pn = RingPreNotedLog();
    ShGateRead r; size_t j = n0;
    for (size_t i = e0; i < en.size(); i++)
    {
        const size_t upTo = (size_t)en[i].noted;
        r.presents++;
        if (en[i].tid == dtWorkerTid) r.onWorker++; else if (en[i].tid == renderTid) r.onRender++;
        if (upTo == j) r.bare++;
        else if (upTo - j > 1) r.twice++;
        else if (pn[j].tid != en[i].tid || pn[j].entered != (LONG)i) r.misplaced++;
        j = upTo;
    }
    r.loose = (int)(pn.size() - j);
    return r;
}
static int ShGateFaults(const ShGateRead& r) { return r.bare + r.twice + r.misplaced + r.loose; }
// A NOTE THAT REFUSES THE NEXT SUBMIT (the fake's own switch; dt_fg_hud.h plants it behind a hand)
static void ShFailNextSubmit(DWORD, DWORD, DWORD) { _InterlockedExchange(&gRing.submitFailNext, 1); }

// ================================================================================================================
// PHASE 3, STAGE 6 (2026-10-10): THE FAKES LEARN. EIGHT CASES, FK1 TO FK8, ONE A RULE. (FK1 TO FK7 ran the shadow shape's
// machine and went with it in round 9's strip; FK8, over the rig's shape, stays, and the rules the others planted stand in
// the fakes, where every merged case's clean check reads their counts.)
//
// The merged path of stage 7 (a binding that arms on the real wire) shows the helper's generated image, and what it
// can get wrong the fakes could not count until now. Each case below takes ONE thing a fake learned (the head of this
// file lists them), shows it on a clean leg, and PLANTS ITS VIOLATION: the count must move by what was planted, and a
// case whose count does not move fails, because then the rule holds nothing. The violations the fake counts are held
// to 0 at the end of every case of the harness (dt_fg_test.cpp, CaseEnd), so each planted leg puts them back.
//
// SINCE STAGE 7 A MACHINE SHOWS A GENERATED IMAGE, so where a case needs "the game copies a set image into the back buffer
// and presents it" THE MERGED MACHINE'S OWN FIRE MAKES IT (ShFire: FgMachine<ShArmB>::ArmedFirePresent, on the worker, in
// a note of the harness's). FK1, FK4 and FK5 plant a violation that fire REFUSES TO MAKE (it reads the answer by
// equality first): each shows the refusal, and then plants through a control's binding whose read says yes. (Until
// stage 7 a stand-in of the harness's own landed the image, in the rig's order, because no machine could.)
// ================================================================================================================
typedef HRESULT(__stdcall* ShDxvk_t)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);

// ================================================================================================================
// THE MERGED BINDING (2026-10-10, phase 3, stage 7): IT ARMS, ON THE PRODUCT'S WIRE. The one binding that says kEngage
// and kWire together (fg_machine.h, "THE MERGED PATH"): derived from ShB, so the fake helper, the writer rule, the held
// queue and the set's rules come with it. TWICE, as the build map's machine-15 asks: ShArmB says false to kRig (the
// shape the third build will compile one day, so that shape is a shape that RUNS) and ShArmRigB says true (every
// instrument's line compiled over the merged path). Its cases are dt_fg_merged.h's; the FK cases below land through
// this binding's machine's own fire.
// ================================================================================================================
struct ShArmB : ShB
{
    static constexpr bool kRig = false, kEngage = true, kWire = true;
    static inline FgArmedState a;
    static FgArmedState& A() { return a; }
    // THE IMAGES OF THE GAME'S OWN, AND WHETHER A COPY CAN NAME EACH (B::ImgOk). The two knobs take one away, as a back buffer
    // they were not made for would: a span that needs it then begins nothing.
    static inline volatile LONG noSave = 0, noHold = 0;
    // (2026-10-17, stage 3) THE WRITES' GPU STAMPS: none in the hand-stepped cases (no position, the clock due); dt_fg_rates.h's RtB has them
    static inline volatile LONG placeOn = 0;                  // (dt_fg_place.h's PW2 says it has them)
    static bool PlaceStamps() { return placeOn != 0; }
    static bool ImgOk(int img)
    {
        if (img == FG_IMG_SAVE) return noSave == 0;
        if (img == FG_IMG_HOLD) return noHold == 0;
        if (img == FG_IMG_GEN2) return (gShGenCounts & 4) != 0 && noGen2 == 0;   // (2026-10-18, round 3, G3: the second store's image, where the set makes two)
        return img >= 0 && img < 3 && HasSlot(img);
    }
    // (2026-10-18, round 3, G3: THE GENERATED COUNTS THE FAKE'S SET MAKES, gShGenCounts: 2, one frame a pair, in every case before dt_fg_fill.h's,
    // which hand in 7 (flow-only, one, two) or 3. noGen2, a case's knob: the GEN2 image cannot be named at the fire, as one not made for this build)
    static inline volatile LONG noGen2 = 0;
    static bool GenOk(int c) { return c >= 0 && c < 3 && ((gShGenCounts >> c) & 1) != 0; }
    // THE BACK BUFFER, WRITTEN OR NOT (B::BackMark, B::BackWritten; dt_fake_ring.h, "THE BACK BUFFER, WRITTEN OR NOT").
    // backMode 0: THE DEFAULT ANSWER OF A BINDING, "written", always: every fire saves and restores. 1: what the fake
    // knows (a D3D9 call wrote the back buffer since the mark). 2: A CONTROL'S LIE, "not written", always.
    // 3 (2026-10-12; dt_fg_saves.h): THE PRODUCT'S ANSWER, the device thread's own (source/devthread/dt_bbs.h), through the two
    // calls the product's binding makes (source/fg/fg_arm.h): the mark at every arm, in every mode, as the product marks;
    // the question with the back buffer surface the fire is about to name, which fire asks, and whether the pair's
    // Present carries a rectangle. backSurf, a case's knob: another surface named in the back buffer's place.
    // backRectOff, a control's: the rectangle is not handed in.
    // EVERY ANSWER IS HELD AGAINST WHAT THE FAKE KNOWS, AT THE QUESTION ITSELF (the fake's flag is set at the call, on the thread
    // that replays it, which is the thread that asks): backUnder, the tracker said "not written" where the fake knows a
    // write (THE FAULT: a clean case holds it at 0); backOver, it said "written" where the fake knows none (its doubts,
    // and what is not a write: allowed, counted).
    static inline volatile LONG backMode = 0, backRectOff = 0, backUnder = 0, backOver = 0;
    static inline IDirect3DSurface9* backSurf = nullptr;
    static void BackMark() { RingBackMark(); DtBbsMark(); }
    static bool BackWritten()
    {
        if (backMode != 3) return backMode == 0 ? true : backMode == 1 ? RingBackDirty() : false;
        const FgWorkerLine& w = S().w;
        const bool fake = RingBackDirty();
        const bool said = DtBbsWritten(backSurf ? backSurf : gBb, w.fireKind, !backRectOff && (w.hasSrc || w.hasDst));
        if (fake && !said) _InterlockedIncrement(&backUnder); else if (said && !fake) _InterlockedIncrement(&backOver);
        return said;
    }
    // THE HUD-LESS PICTURE (phase 3, stage 15; fg_machine.h, "THE HUD-LESS PICTURE"; dt_fg_hud.h has the cases). hud: what the
    // render thread answers for a frame (FG_HUD_*), asked only of a set that has the image (ShB::hasHudless). SAME, the
    // default: every case before these runs as it did. THE PICTURE IS A FILL OF THE BACK BUFFER WITH A TAG OF ITS OWN
    // (hudTag), recorded on the render thread behind the hand as the mod records its pass: the fake's back buffer is one
    // tag, so a fill is "the frame with its interface gone", and a screen that shows that tag has shown the HUD-less
    // picture. hudReal: the frame's own tag (the last one a frame made), which the undo fills back.
    // hudDisarm, a case's knob (HU4): the layer is disarmed BY THE RENDER THREAD right behind the picture's draw, between the
    // hand and the HUD-less note, as a Reset asked from another thread is answered at this thread's next call: the note
    // is then refused, and the machine must ask for the undo.
    static inline volatile LONG hud = 0, hudAsked = 0, hudDraws = 0, hudUndos = 0, hudDisarm = 0;
    static inline DWORD hudTag = 0, hudReal = 0;
    // THE SECOND-TARGET FORM (2026-10-11; fg_machine.h, FG_HUD_TEX; dt_fg_hud.h has its cases, HU7 to HU10). THE BINDING'S OWN
    // IMAGE is the fake's D3D9 surface FG_SURF_SAVET (the rig's shape D target, which nothing of the merged path names).
    // For a frame whose answer is TEX, HudWant RECORDS A FILL OF THAT SURFACE WITH A TAG OF THE PICTURE'S OWN (hudTag), on
    // the render thread ahead of the hand, as the mod's encode records its draw into its second target ahead of the
    // Present: so the picture is in DXVK's stream, and it is the hand's own flush that brings it to the queue. The back
    // buffer is not touched. hudSeconds: such fills. hudSrcGone, a case's knob: the image cannot be named at the hand's
    // span. hudTexCopies: copies out of it into a set's image. hudWrong, a control's knob: 1 the copy reads the BACK
    // BUFFER (the picture never reaches the helper); 2 the fill goes INTO THE BACK BUFFER (the form's own hazard: a
    // picture with the interface gone where the Present reads).
    static inline volatile LONG hudSeconds = 0, hudSrcGone = 0, hudSrcAsked = 0, hudTexCopies = 0, hudWrong = 0;
    static IDirect3DSurface9* HudSrcSurf() { return RingSurf(FG_SURF_SAVET); }
    static int  HudSrcImg() { return RingImgOfSurf(HudSrcSurf()); }
    // (2026-10-24, the SDR menu crash; fg_hud_rule.h, FgHudSrcTake: THE MOD'S OWN LINES pick the image the span's copy names, over the fake
    // ring's answer of NOW (the image under the surface, its handle), as the mod's do over GetVulkanImageInfo. madeImg: the handle the
    // make would have read, at the case's start (HuTexOn): the control FGHUD_SRC_CACHED names it. srcIdx: the image this span's copy reads)
    struct RingQ
    {
        static bool Query(IDirect3DSurface9* s, unsigned long long* i, int* l, unsigned* w, unsigned* h, int* f)
        {
            EnterCriticalSection(&gRing.data);
            const int k = RingImgOfSurfLocked(s, nullptr);
            *i = k >= 0 ? gRing.img[k].handle : 0;
            LeaveCriticalSection(&gRing.data);
            *l = 0; *w = 0; *h = 0; *f = 0;
            return *i != 0;
        }
    };
    static inline ULONGLONG madeImg = 0;
    static inline int srcIdx = -1;
    static inline volatile LONG moveAfterFill = 0;            // (a case's knob: the defragmenter moves the binding's image right behind the frame's picture)
    static bool HudSrc()
    {
        _InterlockedIncrement(&hudSrcAsked);
        srcIdx = -1;
        if (hudSrcGone) return false;
        FgHudSrcNow n = {};
        if (!FgHudSrcTake<RingQ>(true, HudSrcSurf(), 0, 0, 0, madeImg, 0, &n)) return false;
        EnterCriticalSection(&gRing.data); srcIdx = RingImgOfHandleLocked(n.img); LeaveCriticalSection(&gRing.data);
        return srcIdx >= 0;
    }
    static void RecCopyHudToImg(int img)
    {
        _InterlockedIncrement(&hudTexCopies);
        if (img == FG_IMG_HUDLESS) _InterlockedIncrement(&hudCopies);
        RecAcquire(img);
        RingRec({ RRAW_COPY, RingRig(img), hudWrong == 1 ? RingCurrent() : srcIdx, 0, 0, 0, 0, 0 });   // (the image HudSrc picked in this span)
        RecRelease(img);
    }
    static void HudSecond()
    {
        hudReal = gTag; hudTag = NewTag(); _InterlockedIncrement(&hudSeconds);
        D->ColorFill(hudWrong == 2 ? gBb : HudSrcSurf(), nullptr, hudTag);
        if (moveAfterFill) D->ColorFill(HudSrcSurf(), nullptr, kRingMove);   // (2026-10-24: the defragmenter's move, in the stream behind the picture)
    }
    static int  HudWant() { _InterlockedIncrement(&hudAsked); if (hud == FG_HUD_TEX) HudSecond(); return hud; }
    static void HudDraw()
    {
        hudReal = gTag; hudTag = NewTag(); Fill(hudTag); _InterlockedIncrement(&hudDraws);
        if (_InterlockedExchange(&hudDisarm, 0)) DtDisarm("the harness: a disarm between the hand and the HUD-less note");
    }
    static void HudUndo() { Fill(hudReal); _InterlockedIncrement(&hudUndos); }
    // COPIES INTO THE HELPER'S HUDLESS, COUNTED: a hand with a HUD-less picture to follow makes none (the note's span makes the
    // one), and a second copy is a cost no picture shows: the note's copy lands over it.
    static inline volatile LONG hudCopies = 0;
    static void RecCopyToImg(ULONGLONG bb, int img) { if (img == FG_IMG_HUDLESS) _InterlockedIncrement(&hudCopies); ShB::RecCopyToImg(bb, img); }
    // A SIGNAL ON A TIMELINE THAT IS NOT THE GAME'S TO SIGNAL (READY-FOR-7.md, section 1, item 4): the rig's fire signals
    // the I marker's, which on the wire is the helper's fgPrep. The merged path signals fgIn alone (and the harness's
    // injection body the frame's own, FG_TL_LOCAL). Counted.
    static inline volatile LONG badSignal = 0;
    // (noSignal, a case's knob: the hand's submit does not signal fgIn, as a handoff that never retires would not)
    static inline volatile LONG noSignal = 0;
    static void RecSignal(int tl, LONG v) { if (tl != FG_TL_IN && tl != FG_TL_LOCAL) _InterlockedIncrement(&badSignal); if (noSignal && tl == FG_TL_IN) return; ShB::RecSignal(tl, v); }
    // THE GATE BEFORE DXVK'S PRESENT, WITH A MODEL OF WHAT IT WAITS FOR (fg_xport.h, FgxPresentGate; the product's is
    // FgP::PresentGate). It notes its call as ShB's does. With gateModel on it also HOLDS, on the CPU, as the product's
    // holds: until fgPrep has reached THE TOP A WAIT WAS QUEUED FOR AS IT STOOD AT THE PREVIOUS CALL (the frame's own
    // out-fence is not modelled here: one fence is enough to see who is held). Then what stands queued now becomes the
    // next call's need. Bounded at 8 s, as the product's (counted): a case that holds the helper releases it.
    static inline volatile LONG gateModel = 0, gatePrev = 0, gateIn = 0, gateHeld = 0, gateGaveUp = 0;
    // (stage 7c) A PRESENT HELD IN THE GATE, WHATEVER FOR: the next call holds on the CPU until GateRelease (one shot,
    // bounded as the model is). The product's gate also waits on the frame's own out-fence, which this harness does not
    // model: with this knob a case holds the present it names, a generated frame's too.
    static inline volatile LONG gateHoldNext = 0;
    static inline HANDLE gateEv = nullptr;
    static void GateRelease() { if (gateEv) SetEvent(gateEv); }
    static void PresentGate()
    {
        RingPreNoteCall();
        if (_InterlockedExchange(&gateHoldNext, 0))
        {
            if (!gateEv) gateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            _InterlockedIncrement(&gateHeld); _InterlockedExchange(&gateIn, 1);
            if (WaitForSingleObject(gateEv, 8000) != WAIT_OBJECT_0) _InterlockedIncrement(&gateGaveUp);
            _InterlockedExchange(&gateIn, 0);
        }
        if (!gateModel) return;
        const LONG need = gatePrev;
        if (need && TlValue(FG_TL_PREP) < (ULONGLONG)need)
        {
            _InterlockedIncrement(&gateHeld); _InterlockedExchange(&gateIn, 1);
            const ULONGLONG end = GetTickCount64() + 8000;
            while (TlValue(FG_TL_PREP) < (ULONGLONG)need)
            {
                if (GetTickCount64() >= end) { _InterlockedIncrement(&gateGaveUp); break; }
                TlWait(FG_TL_PREP, (ULONGLONG)need, 5);
            }
            _InterlockedExchange(&gateIn, 0);
        }
        _InterlockedExchange(&gatePrev, x.sh.prepTop);
    }
    // (stage 7c) THE PIPE OF A HELPER THAT IS GONE. In the mod the worker's next write to a dead helper's pipe FAILS, and the
    // failure stops the transport on that very thread, inside the note that wrote (fg_product.h, FgP::WriteQ and WriteG).
    // With pipeDies on, a write made once the fake helper is gone is not made: it is counted and the case's hook runs
    // in its place, on the writer's thread (the machine's Stop). Off, the default: the write goes to a pipe nobody
    // reads, which is the window before the mod's write has failed.
    static inline volatile LONG pipeDies = 0, pipeFailed = 0;
    static inline void (*onPipeFail)() = nullptr;
    static bool PipeDead() { if (!pipeDies || !gH.gone) return false; _InterlockedIncrement(&pipeFailed); if (onPipeFail) onPipeFail(); return true; }
    static void WriteQ(LONG n, DWORD flags) { if (PipeDead()) return; ShB::WriteQ(n, flags); }
    static void WriteG(LONG n, DWORD kind)  { if (PipeDead()) return; ShB::WriteG(n, kind); }
    // (stage 7c) THE RETIRE WAIT, AS THIS BINDING'S QUEUE HAS IT. The fake's submits act at once, so FgHarnessBinding's wait
    // returns at once; but a submit this binding's queue TOOK AND HOLDS behind a wait on fgPrep has not retired, and the
    // product's build waits for exactly those before the set's images go (fg_product.h, FgpBeforeRelease). Here: until
    // nothing is held, or the time is up.
    static bool RetireWait(DWORD ms)
    {
        const ULONGLONG end = GetTickCount64() + ms;
        for (;;)
        {
            RingQEnter(true); const bool none = heldCbs.empty() && laQ.empty(); RingQLeave();
            if (none) return FgHarnessBinding::RetireWait(ms);
            if (GetTickCount64() >= end) return false;
            Sleep(1);
        }
    }
    // ============================================================================================================
    // THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT (2026-10-14; fg_machine.h, above ArmedOverMay; dt_fg_late.h has the cases).
    // THE FOUR SEAMS, AND A STAGE OF THE FAKE'S QUEUE IN FRONT OF ShB's, for the two waits those cases need:
    //   HOME   the body's home copy, a submit that waits for the helper's answer to its own frame. The fake helper has no
    //          evaluate; what the home really stands behind on the helper's one queue is THE DISPATCH BEFORE IT, so the wait
    //          here is on fgOut reaching that dispatch's answer (LaHome, called by the body's stand-in of a case that
    //          asks: every case before these has no home, and HomeWaited says no, as a body that made none says).
    //   ORD    the order's own fence, this side's: OrdWait queues the wait, OrdSignal moves the fence and runs the queue.
    // ONE LIST IN THE ORDER THINGS REACHED THE QUEUE: a wait is an item, a submit or a list of the stream's work behind it is an
    // item; from the front, a wait passes when it is over and what follows runs THROUGH ShB's own stage (so a wait on
    // fgPrep still holds what it held), and the first wait that is not over stops everything behind it. Off (laOn 0),
    // which is every case before these: nothing is ever in the list and RecSubmit is ShB's.
    // ============================================================================================================
    enum { LA_PAY = 0, LA_ORD = 1, LA_HOME = 2 };
    struct LaItem { int kind; LONG wait; bool cs; std::vector<RingRawOp> raw; std::vector<RingCsItem> list; };
    static inline std::vector<LaItem> laQ;
    static inline volatile LONG laOn = 0, laHeldN = 0, ordBlitsMax = 0;
    static inline volatile LONG ordFence = 0, ordWaits = 0, ordSignals = 0, ordNoLock = 0, ordRefuseNext = 0, ordOff = 0;
    static inline volatile LONG homeN = 0, homeWaits = 0, homeOff = 0, homeLie = 0;
    static inline volatile LONG cardUps = 0, cardDowns = 0, cardNow = 0;
    static inline LONGLONG ordSigAt[64] = {}; static inline DWORD ordSigTid[64] = {};
    static bool LaOver(int kind, LONG v) { return kind == LA_ORD ? ordFence >= v : FgHarnessBinding::TlValue(FG_TL_OUT) >= (ULONGLONG)v; }
    // (with the fake's queue lock held)
    static void LaRunLocked()
    {
        size_t done = 0;
        while (done < laQ.size())
        {
            LaItem& it = laQ[done];
            if (it.kind != LA_PAY) { if (!LaOver(it.kind, it.wait)) break; }
            else if (it.cs)
            {   // a list of the stream's work: through ShB's gate, as if it reached the queue now; what the stream holds meanwhile is put back
                EnterCriticalSection(&gRing.data); std::vector<RingCsItem> mine; mine.swap(gRing.cs); gRing.cs = it.list; LeaveCriticalSection(&gRing.data);
                if (!ShB::CsGate()) { EnterCriticalSection(&gRing.data); RingApplyCsLocked(gRing.cs); gRing.cs.clear(); LeaveCriticalSection(&gRing.data); }
                EnterCriticalSection(&gRing.data); gRing.cs.insert(gRing.cs.begin(), mine.begin(), mine.end()); LeaveCriticalSection(&gRing.data);
            }
            else
            {
                std::vector<RingRawOp> mine; mine.swap(gRing.cb); const bool open = gRing.cbOpen;
                gRing.cb = it.raw; gRing.cbOpen = true; ShB::RecSubmit();
                gRing.cb.swap(mine); gRing.cbOpen = open;
            }
            done++;
        }
        laQ.erase(laQ.begin(), laQ.begin() + (ptrdiff_t)done);
    }
    static void LaQueueRun()
    {
        if (gRing.qOwner == (LONG)GetCurrentThreadId()) { LaRunLocked(); return; }
        RingQEnter(true); LaRunLocked(); RingQLeave();
    }
    static bool LaCsGate()
    {
        if (!laQ.empty()) LaRunLocked();
        if (laQ.empty()) return ShB::CsGate();
        EnterCriticalSection(&gRing.data);
        if (gRing.cs.empty()) { LeaveCriticalSection(&gRing.data); return true; }
        laQ.push_back(LaItem{ LA_PAY, 0, true, {}, gRing.cs }); gRing.cs.clear();
        LeaveCriticalSection(&gRing.data);
        _InterlockedIncrement(&laHeldN);
        // (HOW MANY PRESENTS STAND BEHIND AN ORDER WAIT NOW: the blits in the lists held behind the first order wait that stands. The
        // most ever seen is kept: the machine's own rule is one)
        {
            LONG blits = 0; bool ord = false;
            for (const LaItem& it : laQ)
            {
                if (it.kind == LA_ORD) ord = true;
                else if (ord && it.kind == LA_PAY && it.cs) for (const RingCsItem& c : it.list) blits += c.op == RCS_BLIT;
            }
            if (blits > ordBlitsMax) ordBlitsMax = blits;
        }
        return true;
    }
    static bool RecSubmit()
    {
        if (!laQ.empty()) LaRunLocked();
        if (!laQ.empty() && gRing.cbOpen && gRing.submitFailNext <= 0)
        {
            laQ.push_back(LaItem{ LA_PAY, 0, false, gRing.cb, {} }); gRing.cb.clear(); gRing.cbOpen = false;
            _InterlockedIncrement(&laHeldN);
            return true;                                      // taken by the queue, behind a wait of this stage
        }
        return ShB::RecSubmit();
    }
    // THE BODY'S HOME (a case's body stand-in calls it behind its own submit, for an injection that was sent): the frame's
    // home is submitted behind its wait, and the binding says so for that frame. homeOff, a case's knob: this body makes
    // no home (a frame whose home has no image), and nothing is said. homeLie, A CONTROL'S: the binding says the home was
    // waited for and NO wait is queued: the fact the machine trusts, false.
    static void LaHome()
    {
        if (!laOn || homeOff) return;
        if (!homeLie)
        {
            RingQEnter(true);
            const LONG last = x.w.lastDispatchN;
            if (last && FgHarnessBinding::TlValue(FG_TL_OUT) < 2ull * (ULONGLONG)last) { laQ.push_back(LaItem{ LA_HOME, 2 * last, false, {}, {} }); _InterlockedIncrement(&homeWaits); }
            RingQLeave();
        }
        _InterlockedExchange(&homeN, FgHarnessBinding::sentN);
    }
    static bool HomeWaited(LONG n) { return laOn != 0 && homeN == n; }
    static void CardClass(int up) { if (up) { _InterlockedIncrement(&cardUps); _InterlockedExchange(&cardNow, 1); } else { _InterlockedIncrement(&cardDowns); _InterlockedExchange(&cardNow, 0); } }
    // ordOff, a control's: no wait is queued and the machine is told one was (what a fence that holds nothing would do).
    static bool OrdWait(LONG n)
    {
        if (gRing.qOwner != (LONG)GetCurrentThreadId()) _InterlockedIncrement(&ordNoLock);   // the contract: under the submission lock
        if (_InterlockedExchange(&ordRefuseNext, 0)) return false;
        _InterlockedIncrement(&ordWaits);
        if (!ordOff && ordFence < n) laQ.push_back(LaItem{ LA_ORD, n, false, {}, {} });
        return true;
    }
    static void OrdSignal(LONG n)
    {
        LONG cur = ordFence;
        while (cur < n)
        {
            const LONG was = InterlockedCompareExchange(&ordFence, n, cur);
            if (was == cur) { ordSigAt[n & 63] = FgHarnessBinding::clockNow; ordSigTid[n & 63] = GetCurrentThreadId(); _InterlockedIncrement(&ordSignals); break; }
            cur = was;
        }
        LaQueueRun();
    }
};
struct ShArmRigB : ShArmB { static constexpr bool kRig = true; };
typedef FgMachine<ShArmB> MA;
typedef FgMachine<ShArmRigB> MAR;
// A CONTROL'S BINDING: THE ANSWER'S READ BY EQUALITY, SWITCHED OFF. On the worker fgOut reads "exactly this pair's frame"
// whatever it holds, which is what a fire that asked "at least", or did not ask, would act on. (The placer's read is
// not touched: it runs on another thread.)
struct ShArmLieB : ShArmB
{
    static inline volatile LONG lie = 0;
    static ULONGLONG TlValue(int tl)
    {
        if (lie && tl == FG_TL_OUT && GetCurrentThreadId() == dtWorkerTid) return 2ull * (ULONGLONG)(ULONG)x.w.openN;
        return ShArmB::TlValue(tl);
    }
};
typedef FgMachine<ShArmLieB> MAL;
static void ShArmReset()
{
    memset((void*)&ShArmB::a, 0, sizeof(ShArmB::a));
    ShArmB::a.k.kPlanUs = 2000; ShArmB::a.k.lateUs = 0;
    ShArmB::noGen2 = 0; gShGenCounts = 2;                     // (the set one frame a pair, as the product's install)
    ShArmB::noSave = 0; ShArmB::noHold = 0; ShArmB::backMode = 0; ShArmB::badSignal = 0; ShArmB::noSignal = 0;
    ShArmB::backRectOff = 0; ShArmB::backSurf = nullptr; ShArmB::backUnder = 0; ShArmB::backOver = 0; _InterlockedExchange(&gBbsBlind, -1);
    ShArmB::hud = 0; ShArmB::hudAsked = 0; ShArmB::hudDraws = 0; ShArmB::hudUndos = 0; ShArmB::hudDisarm = 0; ShArmB::hudTag = 0; ShArmB::hudReal = 0; ShArmB::hudCopies = 0;
    ShArmB::hudSeconds = 0; ShArmB::hudSrcGone = 0; ShArmB::hudSrcAsked = 0; ShArmB::hudTexCopies = 0; ShArmB::hudWrong = 0;
    ShArmB::gateModel = 0; ShArmB::gatePrev = 0; ShArmB::gateIn = 0; ShArmB::gateHeld = 0; ShArmB::gateGaveUp = 0;
    ShArmB::gateHoldNext = 0; if (ShArmB::gateEv) ResetEvent(ShArmB::gateEv);
    ShArmB::pipeDies = 0; ShArmB::pipeFailed = 0; ShArmB::onPipeFail = nullptr;
    // (2026-10-14: the four seams of "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT" and their stage of the queue: off, empty)
    ShArmB::laQ.clear(); ShArmB::laOn = 0; ShArmB::laHeldN = 0; ShArmB::ordBlitsMax = 0; gShAfterOne = nullptr;
    ShArmB::ordFence = 0; ShArmB::ordWaits = 0; ShArmB::ordSignals = 0; ShArmB::ordNoLock = 0; ShArmB::ordRefuseNext = 0; ShArmB::ordOff = 0;
    ShArmB::homeN = 0; ShArmB::homeWaits = 0; ShArmB::homeOff = 0; ShArmB::homeLie = 0;
    ShArmB::cardUps = 0; ShArmB::cardDowns = 0; ShArmB::cardNow = 0;
    ShB::countTid = 0;
    gArmedGuardOff = 0;
    ShArmLieB::lie = 0;
    RingBackMark(); gRing.back.on = 1;
}
// A LANDING THROUGH THE MERGED MACHINE'S OWN FIRE (FgMachine::ArmedFirePresent), on the worker, in a note of the harness's:
// until stage 7 these cases landed a set image with a stand-in of the harness's own (ShLand), because no machine could.
// arg: the image in its low byte; bit 8: fired as a REAL frame's fire (the store is one argument of the fire: no read
// of the answer); bit 9: through the control's binding, whose read of the answer says yes. What the fire came to
// (FG_AF_*) is left in gShFired.
static volatile LONG gShFired = -1;
static void ShFireNote(DWORD own, DWORD, DWORD arg)
{
    const int img = (int)(arg & 0xFFu); const bool asN = (arg & 0x100u) != 0, lie = (arg & 0x200u) != 0;
    ShArmLieB::lie = lie ? 1 : 0;
    const int r = lie ? MAL::ArmedFirePresent(!asN, img, FG_PT_B, own, B::Now()) : MA::ArmedFirePresent(!asN, img, FG_PT_B, own, B::Now());
    ShArmLieB::lie = 0;
    _InterlockedExchange(&gShFired, r);
}
static int ShFire(int img, bool asN = false, bool lie = false)
{
    _InterlockedExchange(&gShFired, -1);
    Run(ShFireNote, (DWORD)img | (asN ? 0x100u : 0u) | (lie ? 0x200u : 0u)); Drain();
    return gShFired;
}
static DWORD ShLastShown() { const auto v = Shown(); return v.empty() ? 0u : v.back().tag; }
static bool ShImgGone(int img) { EnterCriticalSection(&gRing.data); const bool g = img >= 0 && img < (int)gRing.img.size() && gRing.img[img].gone; LeaveCriticalSection(&gRing.data); return g; }

// ---------------------------------------------------------------- FK8, a refused submit inside a fire
// THE RULE (check-machine.md, X1; the build map's open question 6). A fire is a copy into the back buffer, a Present, and
// a restore. A submit can be refused. Until this stage the fake could refuse only "the next submit", so only a fire's
// first: now gRing.submitFailSkip lets that many through first, and the fake counts two things: a submit refused inside
// a span (refusedInSpan, a fact) and A PRESENT MADE AFTER ONE BY THE SAME SPAN (presentAfterRefused): that present
// shows the back buffer as it stood, which after the frame's own Present is the NEXT frame half drawn.
// OVER THE RIG'S SHAPE, the one machine with a fire today, which drops every submit's result by design (the product's
// fire must not: stage 7): the worker HELD, the refusal armed and the fire's note queued behind it.
//   clean, N's fire:                 the screen shows the frame; the back buffer afterwards is the next frame's drawing
//   PLANTED, N's copy-in refused:    the screen shows THE NEXT FRAME'S DRAWING where N belongs: counted, once
//   PLANTED, I's restore refused:    the screen is right, nothing presented after the refusal, and THE BACK BUFFER IS NOT
//                                    WHAT IT WAS: the game's drawing is gone from it (counted as refused inside a span)
static void ShFk8Arm(DWORD, DWORD, DWORD skip) { _InterlockedExchange(&gRing.submitFailSkip, (LONG)skip); _InterlockedExchange(&gRing.submitFailNext, 1); }
struct ShFk8Leg { DWORD frame, half, shown, bbAfter; LONG refused, after; };
static ShFk8Leg ShFk8Run(bool isI, int refuseAt)
{
    ShFk8Leg g = {};
    Fresh(); Warm();
    FgState& s = B::S();
    g.frame = Frame(); Drain();
    const LONG p = s.r.lastP;
    AnswerAs(p);
    if (s.w.openPhase != FG_PAIR_ARMED) Err("FK8: the pair was not armed before the fire");
    Facts(); g.half = NewTag(); Fill(g.half); Drain();        // the game is drawing the next frame
    const RingStats st0 = gRingSt;
    if (isI) DueI(p, FG_DUE_SHOW); else DueN(p);
    if (!Gate() || !WaitFor([] { return gGateIn != 0; }, 3000)) Err("FK8: the worker never reached the gate");
    if (refuseAt >= 0 && !Run(ShFk8Arm, (DWORD)refuseAt)) Err("FK8: the refusal's note was refused");
    if (!FireNote()) Err("FK8: the fire's note was refused");
    SetEvent(gGateEv); Drain();
    g.shown = ShLastShown();
    g.bbAfter = RingTag(RingCurrent());
    g.refused = gRingSt.refusedInSpan - st0.refusedInSpan; g.after = gRingSt.presentAfterRefused - st0.presentAfterRefused;
    _InterlockedExchange(&gRing.submitFailNext, 0); _InterlockedExchange(&gRing.submitFailSkip, 0);
    ViolRestore(st0);
    return g;
}
static void CaseFK8()
{
    CaseBegin("FK8 a refused submit inside a fire (stage 6)");
    const ShFk8Leg clean = ShFk8Run(false, -1), copyIn = ShFk8Run(false, 0), restore = ShFk8Run(true, 2);
    printf("  clean, N's fire: the screen showed %lX (the frame %lX), the back buffer afterwards reads %lX (the next frame's drawing %lX); refused inside the span %ld, presents after a refusal %ld\n",
        clean.shown, clean.frame, clean.bbAfter, clean.half, clean.refused, clean.after);
    printf("  PLANTED, N's copy-in refused: the screen showed %lX (the frame %lX, the next frame's drawing %lX); refused inside the span %ld, PRESENTS AFTER A REFUSAL %ld\n",
        copyIn.shown, copyIn.frame, copyIn.half, copyIn.refused, copyIn.after);
    printf("  PLANTED, I's restore refused (its third submit): the screen showed %lX (the frame %lX); the back buffer afterwards reads %lX (the next frame's drawing %lX); refused inside the span %ld, presents after a refusal %ld\n",
        restore.shown, restore.frame, restore.bbAfter, restore.half, restore.refused, restore.after);
    if (clean.shown != clean.frame || clean.bbAfter != clean.half || clean.refused || clean.after) Err("FK8: a clean fire showed %lX (the frame %lX wanted), left the back buffer at %lX (the next frame's drawing %lX wanted), and the fake counted %ld and %ld", clean.shown, clean.frame, clean.bbAfter, clean.half, clean.refused, clean.after);
    if (copyIn.refused != 1 || copyIn.after != 1) Err("FK8: with the copy-in refused the fake counted %ld submit(s) refused inside the span and %ld present(s) after a refusal (1 and 1 wanted)", copyIn.refused, copyIn.after);
    if (copyIn.shown != copyIn.half) Err("FK8: with the copy-in refused the screen showed %lX; the back buffer as it stood, the next frame's drawing %lX, is what a fire that does not look presents (the frame is %lX)", copyIn.shown, copyIn.half, copyIn.frame);
    if (restore.refused != 1 || restore.after != 0 || restore.shown != restore.frame) Err("FK8: with the restore refused the fake counted %ld refused and %ld present(s) after a refusal (1 and 0 wanted), and the screen showed %lX (the frame %lX wanted): the skip did not pick the third submit", restore.refused, restore.after, restore.shown, restore.frame);
    if (restore.bbAfter == restore.half) Err("FK8: with the restore refused the back buffer still reads the next frame's drawing (%lX): the refused submit was made", restore.bbAfter);
    Ctl("the copy-in refused: the screen showed the next frame half drawn (%lX for %lX), counted %ld; the restore refused: the back buffer lost the game's drawing (%lX for %lX)", copyIn.shown, copyIn.frame, copyIn.after, restore.bbAfter, restore.half);
    CaseEnd("FK8 a refused submit inside a fire (stage 6)");
}

// ---------------------------------------------------------------- L20, one gate on one state across the machine's install
// WHAT IT SHOWS (the fold of the release line's gate, 2026-10-10; source/transport/present_gate.h; fg_product.h,
// FgP::DxvkBare and FgP::PresentGate). The release puts its gate before DXVK's Present as a thunk in dtScPresentO. The
// machine is installed in the MIDDLE of a session, takes DXVK's bare pointer and gates its presents itself. If the two
// kept a state each, the machine's first present would be judged with no need at all and go out behind whatever wait
// stood at that moment: the second present behind a standing queue, which is the freeze. With ONE state it is held.
// HOW. On a clean harness, before any machine: THE MOD'S OWN WRAP puts THE MOD'S OWN THUNK in dtScPresentO (dt_fg_test.cpp
// compiles the header whole, over a made-up Vulkan whose out-fence this case moves). Six presents go through it on the
// worker under vsync: five with the helper answering (frame n sent, the fence at n), then frame 6 sent and THE HELPER
// SILENT, the fence left at 5. That sixth present passes, as it must (its need is frame 5's): it is the ONE present
// behind the standing wait, and the thunk stores 6 as the next need. THEN THE MACHINE IS INSTALLED, by its own Install,
// through a binding that takes the pointer as the product's does and gates by the product's rules. Its first present,
// passed through on the worker, must HOLD until the fence reaches 6 (a second thread moves it 200 ms later) and only
// then reach the fake's Present; the present after it passes with no hold.
// WHAT IS THE MOD'S HERE AND WHAT IS A COPY: the thunk, the wrap, the release's wrapper, the core, the term, the state
// and the machine's Install and WorkerPresent are the mod's own lines. The binding's two functions are this file's,
// written after FgP's (which cannot be compiled without the transport); tools/check-fg-fence.py holds FgP's own lines.
// CONTROL: the same run with the binding's gate on A STATE OF ITS OWN (the shape before the fold): the machine's first
// present reaches the fake's Present at once, with the fence still short of what the thunk had stored.
struct ShFoldB : ShArmB                                       // (round 9's strip: the merged binding; until then ShB's shadow shape)
{
    static void* DxvkBare(void* p) { return (p == (void*)&IvtGatedPresent && ivtGateDxvkPresent) ? ivtGateDxvkPresent : p; }
    static inline IvtGate own;                         // the control's: a second state
    static inline IvtGate* st = &ivtGate;              // the state this binding's gate runs on
    static inline volatile bool down = false;          // "the mod is going down" (the product hands `dead`)
    static inline volatile LONG prepTop = 0;           // the highest value a batch waits behind fgPrep for (none in this case)
    static inline volatile LONG calls = 0;
    static inline volatile LONGLONG outAtPass = -1;    // the out-fence as it read when the gate last let a present go
    static inline DWORD lastMs = 0;
    static void PresentGate()
    {
        RingPreNoteCall();
        _InterlockedIncrement(&calls);
        if (!FeedVkHasTimelineQueries(&ivtVk) || ivtSemOut == VK_NULL_HANDLE) return;
        if (!latVsync) { if (st->prevOut) st->prevOut = 0; if (st->prevPrep) st->prevPrep = 0; return; }
        const bool stopped = ivtFailed;
        const uint64_t sent = stopped ? 0 : (uint64_t)(ULONG)ivtWatchAwait;
        const uint64_t sentOut = sent ? sent - (latOrdAsync[sent & (LAT_RING - 1)] ? 1u : 0u) : 0;
        lastMs = IvtGateCore(st, &ivtVk, ivtSemOut, sentOut, kFoldSemPrep, stopped ? 0 : (uint64_t)(ULONG)prepTop, &down);
        InterlockedExchange64(&outAtPass, InterlockedCompareExchange64(&gFoldOut.v, 0, 0));
    }
};
typedef FgMachine<ShFoldB> MSF;
struct ShFoldLeg
{
    bool wrapped, bare, chained; LONG needAtInstall, heldBefore, readsBefore; LONGLONG fenceAtInstall;
    size_t enteredBefore; LONG gateCalls; LONG held, gaveUp, unread; LONGLONG outAtPass; double firstMs, releaseMs, nextMs; DWORD coreMs;
    size_t enteredFirst; LONG heldAfterNext, needAfterNext;
};
static volatile LONG gFoldRelAfterMs = 0; static LONGLONG gFoldRelQpc = 0;
static DWORD WINAPI ShFoldRelease(void*) { Sleep((DWORD)gFoldRelAfterMs); InterlockedExchange64(&gFoldOut.v, 6); LARGE_INTEGER q; QueryPerformanceCounter(&q); gFoldRelQpc = q.QuadPart; return 0; }
static ShFoldLeg* gFoldLeg = nullptr;
// BEFORE THE INSTALL (Fresh calls it on the clean state): the mod's wrap, then six presents through the mod's thunk
static void ShFoldBefore()
{
    ShFoldLeg& g = *gFoldLeg;
    memset(&ivtGateLine, 0, sizeof(ivtGateLine));
    memset(&ivtVk, 0, sizeof(ivtVk)); ivtVk.ok = true; ivtVk.GetSemaphoreCounterValue = FoldCounter; ivtVk.WaitSemaphores = FoldWait;
    memset(&gFoldOut, 0, sizeof(gFoldOut)); memset(&gFoldPrep, 0, sizeof(gFoldPrep));
    memset((void*)latOrdAsync, 0, sizeof(latOrdAsync));     // every frame in the same-frame order: frame n waits behind n
    ivtSemOut = kFoldSemOut; latVsync = true; ivtFailed = false; ivtWatchAwait = 0;
    void* const dxvk = dtScPresentO;
    dtScPresentO = IvtGateWrap(dtScPresentO);
    g.wrapped = dtScPresentO == (void*)&IvtGatedPresent && ivtGateDxvkPresent == dxvk && dxvk == gDxvkPresent0;
    const size_t e0 = RingEnteredLog().size();
    for (LONG n = 1; n <= 6; n++)
    {
        _InterlockedExchange(&ivtWatchAwait, n);                             // frame n is sent: a batch of the game's waits behind the out-fence at n
        if (n <= 5) InterlockedExchange64(&gFoldOut.v, n);                   // the helper answers it; AT FRAME 6 IT IS SILENT
        Facts(false); RecordPresent(); Drain();
    }
    g.enteredBefore = RingEnteredLog().size() - e0;
    g.needAtInstall = ivtGate.prevOut; g.heldBefore = ivtGate.held; g.readsBefore = gFoldOut.reads;
    g.fenceAtInstall = InterlockedCompareExchange64(&gFoldOut.v, 0, 0);
}
static void ShFoldRun(bool ownState, ShFoldLeg* g)
{
    memset(g, 0, sizeof(*g));
    memset(&ShFoldB::own, 0, sizeof(ShFoldB::own));
    ShFoldB::st = ownState ? &ShFoldB::own : &ivtGate; ShFoldB::down = false; ShFoldB::prepTop = 0; ShFoldB::calls = 0; ShFoldB::outAtPass = -1; ShFoldB::lastMs = 0;
    gFoldLeg = g; gFreshBeforeInstall = ShFoldBefore;
    ShFresh<MSF>();                                                          // ... and THE MACHINE'S OWN INSTALL, behind the six presents
    gFreshBeforeInstall = nullptr; gFoldLeg = nullptr;
    FgState& s = B::S();
    g->bare = s.ptr.dxvkPresent == gDxvkPresent0;                            // DXVK's own pointer, not the thunk
    g->chained = dtScPresentO == (void*)&MSF::WorkerPresent;
    s.wt.want = FG_WANT_PASS;
    // THE MACHINE'S FIRST PRESENT, on the worker, passed through. The fence moves to 6 on another thread 200 ms from now
    LARGE_INTEGER qf, q0, q1; QueryPerformanceFrequency(&qf);
    size_t e0 = RingEnteredLog().size();
    gFoldRelAfterMs = 200; gFoldRelQpc = 0;
    const HANDLE h = Spawn(ShFoldRelease);
    QueryPerformanceCounter(&q0);
    Facts(false); RecordPresent(); Drain();
    QueryPerformanceCounter(&q1);
    g->enteredFirst = RingEnteredLog().size() - e0;
    g->firstMs = (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart;
    g->gateCalls = ShFoldB::calls; g->coreMs = ShFoldB::lastMs; g->outAtPass = ShFoldB::outAtPass;
    g->held = ShFoldB::st->held; g->gaveUp = ShFoldB::st->gaveUp; g->unread = ShFoldB::st->unread;
    Join(h, "L20's release of the out-fence");
    g->releaseMs = gFoldRelQpc ? (double)(gFoldRelQpc - q0.QuadPart) * 1000.0 / (double)qf.QuadPart : -1.0;
    // THE PRESENT AFTER IT: its need is what stood queued at the one before (frame 6), which the fence has reached
    QueryPerformanceCounter(&q0);
    Facts(false); RecordPresent(); Drain();
    QueryPerformanceCounter(&q1);
    g->nextMs = (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart;
    g->heldAfterNext = ShFoldB::st->held; g->needAfterNext = ShFoldB::st->prevOut;
    ShEnd();
    // the harness as every other case finds it: no fence for a gate to look at, no thunk kept
    ivtSemOut = VK_NULL_HANDLE; latVsync = false; _InterlockedExchange(&ivtWatchAwait, 0);
    memset(&ivtGateLine, 0, sizeof(ivtGateLine));
}
static void CaseL20()
{
    CaseBegin("L20 one gate, one state, across the machine's install (the fold)");
    ShFoldLeg g, c;
    ShFoldRun(false, &g);
    ShFoldRun(true, &c);
    printf("  before the install: the mod's wrap put the mod's thunk before the fake's Present (%s); 6 presents through it reached the fake (%zu), none held (%ld), the out-fence read %ld time(s);"
           " at the install the stored need is %ld and the fence stands at %lld\n",
        g.wrapped ? "yes" : "NO", g.enteredBefore, g.heldBefore, g.readsBefore, g.needAtInstall, g.fenceAtInstall);
    printf("  the install: the machine took DXVK's bare pointer (%s) and stands in dtScPresentO (%s). ITS FIRST PRESENT: %ld call(s) of the binding's gate, HELD %ld (the core says %lu ms; given up %ld, unread %ld),"
           " the call took %.1f ms and the fence moved at %.1f ms; the fence read %lld when the gate let it go; %zu present(s) reached the fake. The next present: %.1f ms, held in all %ld, the need it leaves %ld\n",
        g.bare ? "yes" : "NO", g.chained ? "yes" : "NO", g.gateCalls, g.held, (unsigned long)g.coreMs, g.gaveUp, g.unread, g.firstMs, g.releaseMs, g.outAtPass, g.enteredFirst, g.nextMs, g.heldAfterNext, g.needAfterNext);
    if (!g.wrapped) Err("L20: the mod's wrap did not put its thunk in dtScPresentO with the fake's own Present kept behind it");
    if (g.enteredBefore != 6 || g.heldBefore != 0 || g.readsBefore < 5) Err("L20: before the install %zu of 6 presents reached the fake through the thunk, %ld were held (none wanted: every need was reached), and the out-fence was read %ld time(s) (5 or more wanted: the thunk's gate looked)", g.enteredBefore, g.heldBefore, g.readsBefore);
    if (g.needAtInstall != 6 || g.fenceAtInstall != 5) Err("L20: at the install the stored need is %ld over a fence at %lld (6 over 5 wanted: a need that is SHORT): the case is not what it says", g.needAtInstall, g.fenceAtInstall);
    if (!g.bare || !g.chained) Err("L20: the machine's install did not take DXVK's bare pointer (%s) or does not stand in dtScPresentO (%s): the thunk would still be in the chain", g.bare ? "it did" : "IT TOOK THE THUNK", g.chained ? "it does" : "IT DOES NOT");
    if (g.gateCalls != 1 || g.enteredFirst != 1) Err("L20: the machine's first present made %ld call(s) of the binding's gate and %zu entr(ies) of the fake's Present (1 and 1 wanted)", g.gateCalls, g.enteredFirst);
    if (g.held != 1 || g.gaveUp != 0 || g.unread != 0 || g.coreMs < 100 || g.outAtPass < 6)
        Err("L20: THE MACHINE'S FIRST PRESENT WAS NOT HELD ON THE NEED THE THUNK STORED: held %ld (1 wanted), given up %ld, unread %ld, %lu ms by the core (100 or more wanted), and the fence read %lld when it was let go (6 wanted)",
            g.held, g.gaveUp, g.unread, (unsigned long)g.coreMs, g.outAtPass);
    if (g.releaseMs < 0 || g.firstMs < g.releaseMs - 20.0) Err("L20: the first present returned after %.1f ms and the fence moved at %.1f ms: it did not wait for the fence", g.firstMs, g.releaseMs);
    if (g.heldAfterNext != 1 || g.needAfterNext != 6) Err("L20: the present after the held one: %ld held in all (1 wanted: the fence had reached its need) and the need it leaves is %ld (6 wanted)", g.heldAfterNext, g.needAfterNext);
    // THE CONTROL
    printf("  control, the binding's gate on a state of its own: at the install the thunk's stored need %ld over a fence at %lld; the machine's first present: held %ld, %.1f ms, the fence read %lld when it went (the fence moved at %.1f ms)\n",
        c.needAtInstall, c.fenceAtInstall, c.held, c.firstMs, c.outAtPass, c.releaseMs);
    if (!c.wrapped || !c.bare || c.needAtInstall != 6 || c.fenceAtInstall != 5 || c.gateCalls != 1 || c.enteredFirst != 1 || c.held != 0 || c.outAtPass != 5 || c.firstMs > 100.0)
        Err("L20 control: with a second state the machine's first present did not go out unchecked (wrapped %d, bare %d, need %ld over %lld, %ld call(s), %zu entr(ies), held %ld, the fence read %lld as it went, %.1f ms): the control shows nothing",
            c.wrapped ? 1 : 0, c.bare ? 1 : 0, c.needAtInstall, c.fenceAtInstall, c.gateCalls, c.enteredFirst, c.held, c.outAtPass, c.firstMs);
    Ctl("the binding's gate on a state of its own: the machine's first present reached DXVK in %.1f ms with the out-fence at %lld of the %ld the thunk had stored, not held (with the one state: held %lu ms, let go at %lld)",
        c.firstMs, c.outAtPass, c.needAtInstall, (unsigned long)g.coreMs, g.outAtPass);
    CaseEnd("L20 one gate, one state, across the machine's install (the fold)");
}
