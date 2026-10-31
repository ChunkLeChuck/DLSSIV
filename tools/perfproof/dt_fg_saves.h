// dt_fg_saves.h: THE SAVE'S QUESTION, ANSWERED BY THE DEVICE THREAD (2026-10-12, phase 3 of the revival; the brief
// docs/reports/fg-3/sitting-2026-10-12/BRIEF-saves.md). Included by dt_fg_test.cpp after dt_fg_hud.h.
//
// WHAT IS PROVED HERE. A fire of the merged path saves the back buffer before its write and puts it back after its Present
// only where the binding says the game has written it since the pair was armed (fg_machine.h, B::BackMark and
// B::BackWritten; M11 in dt_fg_merged.h proves the machine's side on the fake's own knowledge). THE PRODUCT'S ANSWER IS
// THE DEVICE THREAD'S (source/devthread/dt_bbs.h), and this harness compiles that file AS A BUILD WHOSE PRODUCT ARMS
// COMPILES IT (dt_fg_test.cpp defines DT_BBS_ON): the same sites in the same wrappers, on the real ring, against the fake
// device. ShArmB::backMode 3 hands the machine that answer, through the two calls the product's binding makes.
//
// THE RULE EVERY CASE HOLDS: A WRITE OF ANY KIND BETWEEN A HAND AND A FIRE FORCES THE SAVE, and where nothing wrote NO SAVE IS
// MADE. The judge of the first is THE PICTURE: the fake counts a fire's Present made over a back buffer the game has
// written with no copy of it into SAVE (RingStats::backLost), and the back buffer after each fire must be the game's
// half-drawn picture, exactly. EVERY KIND HAS ITS CONTROL: the site that sees it switched off (DT_BBS_BLIND, the harness's
// alone), where the same write must LOSE the game's picture, counted. A control that loses nothing fails its case.
// WHERE THE FAKE DOES NOT MODEL A WRITE (an UpdateSurface, a lock given back, a patch, the mod's raw copy) THE HARNESS TELLS IT,
// at the write's own place: in a note on the worker right behind a replayed call, or on the calling thread right behind
// a drained one. A Present and a rectangle are not writes: their sites force the save because the path is not followed,
// their controls show the save not made, and no picture can show more.
// WHERE AN ORDER IS WHAT IS PROVED THE WORKER IS HELD with the commands queued behind it (dt_fg_test.cpp's head).
//
// THE LYING BUILD: `python run_proof.py dt_fg_test -DDT_BBS_LIE=1 2 SV1` compiles the tracker answering "not written",
// always. Every SV case must then FAIL, at the picture.
#pragma once
#include <functional>

// ------------------------------------------------------------------ the legs
struct SvLeg
{
    DWORD tag = 0, half = 0, shownI = 0, shownN = 0, bbI = 0, bbN = 0;
    LONG  savesI = 0, noI = 0, savesN = 0, noN = 0, lostI = 0, lost = 0;
};
// The harness tells the fake of a write the fake does not model: the game's picture `tag` is in the back buffer NOW.
// (the stream is flushed first: what was recorded before this write lands before it)
static void SvLanded(DWORD tag) { B::Flush(); RingSetTag(RingCurrent(), tag); _InterlockedExchange(&gRing.back.dirty, 1); }
static void SvLandedNote(DWORD, DWORD, DWORD tag) { SvLanded(tag); }
// ONE PAIR UNDER THE TRACKER: a frame; `plant` makes the game's write (or none) and returns the picture it left in the back
// buffer (0: nothing a picture shows); the generated frame's fire; `between` (if any); the real frame's fire. blind: the
// site switched off from the plant to the end of the pair (-1 none).
static SvLeg SvPair(const std::function<DWORD()>& plant, int blind = -1, const std::function<DWORD()>& between = nullptr)
{
    SvLeg g; FgArmedState& a = ShArmB::a;
    const RingStats st0 = gRingSt;
    g.tag = ShFrame<MA>();
    _InterlockedExchange(&gBbsBlind, blind);
    g.half = plant ? plant() : 0;
    LONG s0 = a.w.saves, n0 = a.w.noSaves;
    MgDueShow();
    g.savesI = a.w.saves - s0; g.noI = a.w.noSaves - n0; g.shownI = ShLastShown(); g.bbI = MgBb();
    g.lostI = gRingSt.backLost - st0.backLost;
    if (between) { const DWORD h2 = between(); if (h2) g.half = h2; }
    s0 = a.w.saves; n0 = a.w.noSaves;
    MgDueN();
    g.savesN = a.w.saves - s0; g.noN = a.w.noSaves - n0; g.shownN = ShLastShown(); g.bbN = MgBb();
    g.lost = gRingSt.backLost - st0.backLost;
    _InterlockedExchange(&gBbsBlind, -1);
    return g;
}
// What a clean leg must read: both fires saved, the screen showed the pair's generated frame and then its real frame,
// nothing of the game's was lost, and (where the write has a picture) the back buffer is that picture after each fire.
static bool SvForced(const SvLeg& g, bool picture = true)
{
    return g.savesI == 1 && g.savesN == 1 && g.noI == 0 && g.noN == 0 && g.lost == 0 && g.shownI == ShGenTag(g.tag, false) && g.shownN == g.tag
        && (!picture || !g.half || (g.bbI == g.half && g.bbN == g.half));
}
static const char* SvSays(const SvLeg& g)
{
    static char b[4][256]; static int at = 0; char* s = b[at++ & 3];
    snprintf(s, 256, "the generated frame's fire saved %ld and made no save %ld, the real frame's %ld and %ld; the screen %lX then %lX (the frame %lX); the back buffer %lX then %lX (the game's write %lX); the game's picture lost %ld time(s)",
        g.savesI, g.noI, g.savesN, g.noN, g.shownI, g.shownN, g.tag, g.bbI, g.bbN, g.half, g.lost);
    return s;
}
struct SvKind { const char* name; std::function<DWORD()> plant; int blind; bool picture; };
// A kind's clean leg and its control. Returns 1 where both read as they must.
static int SvKindRun(const char* tag, const SvKind& k, int& lostCtl)
{
    const SvLeg g = SvPair(k.plant);
    const bool ok = SvForced(g, k.picture);
    if (!ok) Err("%s: %s between a hand and a fire: %s (both fires must save, nothing lost, the game's picture exact)", tag, k.name, SvSays(g));
    const RingStats stc = gRingSt;
    const SvLeg c = SvPair(k.plant, k.blind);
    ViolRestore(stc);
    bool bit;
    if (k.picture) { bit = c.lostI >= 1 && c.bbI != c.half; lostCtl += c.lost; }
    else bit = c.savesI == 0 && c.noI == 1;
    if (!bit) Err("%s control: %s with its site switched off: %s (%s wanted): the control shows nothing", tag, k.name, SvSays(c),
                  k.picture ? "the game's picture lost at the generated frame's fire" : "no save made");
    ShFrame<MA>(); MgDueN();                                  // (a clean pair behind the control)
    return ok && bit ? 1 : 0;
}
static void SvFresh(int images = 3)
{
    MgFresh<MA>(images);
    ShArmB::backMode = 3;
    D->SetRenderTarget(0, gBb); Drain();                      // the game's back buffer bound, as the game leaves it
    ShWarm<MA>(); MgDueN();
}
static const float kSvVerts[16] = {};
static const WORD kSvIdx[3] = { 0, 1, 2 };

// ---------------------------------------------------------------- SV1, the replayed writes and the worker's own
// WHAT IT SHOWS. Nothing written: neither fire saves (two submits a pair fewer than "always"), and the screen is right.
// Then each kind of write the worker makes itself, planted between the hand and the generated frame's fire and DRAINED
// (the held forms are SV3's): the four draws with the back buffer bound, a Clear of the target, a StretchRect, a
// ColorFill and an UpdateSurface into it, a fill made by a note's body on the worker (an injection body's shape), and
// a ColorFill into THE SECOND back buffer surface (the answer only: the machine's save is of the first). What is not a
// write stays "not written": a Clear of the depth alone, a StretchRect OUT of the back buffer, a fill of another
// surface, a read-only LockRect. And a write between the two fires: the first saves nothing, the second saves.
static void SvWorkerFill(DWORD, DWORD, DWORD tag) { D->ColorFill(gBb, nullptr, tag); }
static void CaseSV1()
{
    const char* name = "SV1 the save's question by the device thread: the replayed writes and the worker's own (2026-10-12)";
    CaseBegin(name);
    SvFresh();
    const LONG marks0 = dtBbs.marks, miss0 = dtBbs.rtMissed, stale0 = dtBbs.rtStale;
    // nothing written, three pairs
    LONG clean = 0;
    for (int i = 0; i < 3; i++)
    {
        const SvLeg g = SvPair(nullptr);
        if (g.savesI == 0 && g.savesN == 0 && g.noI == 1 && g.noN == 1 && g.lost == 0 && g.shownI == ShGenTag(g.tag, false) && g.shownN == g.tag) clean++;
        else Err("SV1: a pair with nothing written: %s (no save at either fire wanted)", SvSays(g));
    }
    // what is not a write
    struct { const char* what; std::function<DWORD()> plant; } nots[] = {
        { "a Clear of the depth alone", [] { D->Clear(0, nullptr, D3DCLEAR_ZBUFFER, 0x55u, 1.0f, 0); Drain(); return (DWORD)0; } },
        { "a StretchRect out of the back buffer", [] { D->StretchRect(gBb, nullptr, gOtherRt, nullptr, D3DTEXF_NONE); Drain(); return (DWORD)0; } },
        { "a ColorFill of another surface", [] { D->ColorFill(gOtherRt, nullptr, NewTag()); Drain(); return (DWORD)0; } },
        { "a read-only LockRect of the back buffer", [] { D3DLOCKED_RECT lr; gBb->LockRect(&lr, nullptr, D3DLOCK_READONLY); return (DWORD)0; } },
    };
    LONG notN = 0;
    for (auto& n : nots)
    {
        const SvLeg g = SvPair(n.plant);
        if (g.savesI == 0 && g.savesN == 0 && g.noI == 1 && g.noN == 1 && g.lost == 0) notN++;
        else Err("SV1: %s is not a write of the back buffer: %s (no save wanted)", n.what, SvSays(g));
    }
    // each kind
    const SvKind kinds[] = {
        { "a replayed DrawPrimitive with the back buffer bound", [] { const DWORD h = NewTag(); gRing.drawTag = h; D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); Drain(); gRing.drawTag = 0; return h; }, DT_BBS_DRAW, true },
        { "a replayed DrawIndexedPrimitive", [] { const DWORD h = NewTag(); gRing.drawTag = h; D->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1); Drain(); gRing.drawTag = 0; return h; }, DT_BBS_DRAW, true },
        { "a replayed DrawPrimitiveUP", [] { const DWORD h = NewTag(); gRing.drawTag = h; D->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, kSvVerts, 16); Drain(); gRing.drawTag = 0; return h; }, DT_BBS_DRAW, true },
        { "a replayed DrawIndexedPrimitiveUP", [] { const DWORD h = NewTag(); gRing.drawTag = h; D->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 3, 1, kSvIdx, D3DFMT_INDEX16, kSvVerts, 16); Drain(); gRing.drawTag = 0; return h; }, DT_BBS_DRAW, true },
        { "a replayed Clear of the target", [] { const DWORD h = NewTag(); D->Clear(0, nullptr, D3DCLEAR_TARGET, h, 1.0f, 0); Drain(); return h; }, DT_BBS_CLEAR, true },
        { "a replayed StretchRect into the back buffer", [] { const DWORD h = NewTag(); D->ColorFill(gOtherRt, nullptr, h); D->StretchRect(gOtherRt, nullptr, gBb, nullptr, D3DTEXF_NONE); Drain(); return h; }, DT_BBS_STRETCH, true },
        { "a replayed ColorFill", [] { const DWORD h = NewTag(); Fill(h); Drain(); return h; }, DT_BBS_FILL, true },
        { "a replayed UpdateSurface into the back buffer", [] { const DWORD h = NewTag(); D->UpdateSurface(gOtherRt, nullptr, gBb, nullptr); Run(SvLandedNote, h); Drain(); return h; }, DT_BBS_UPDATE, true },
        { "a fill made by a note's body on the worker", [] { const DWORD h = NewTag(); Run(SvWorkerFill, h); Drain(); return h; }, DT_BBS_FILL, true },
    };
    int ok = 0, lostCtl = 0;
    for (const SvKind& k : kinds) ok += SvKindRun("SV1", k, lostCtl);
    // (2026-10-14, F: THE TRACKER'S BYTE TEST LEFT DtExec; the replayed writes reach it through the device thread's observers while a pair is
    // armed. THE CONTROL OF THAT PATH: the observers' look switched off (DT_BBS_B_OBS), a replayed draw and a replayed UpdateSurface
    // between the hand and the fire each lose the game's picture: nothing else carries those writes to the tracker any more)
    int obsLost = 0;
    {
        const SvKind viaObs[] = { kinds[0], kinds[7] };
        for (const SvKind& k : viaObs)
        {
            const RingStats st = gRingSt;
            const SvLeg c = SvPair(k.plant, DT_BBS_B_OBS);
            ViolRestore(st);
            if (c.lostI >= 1 && c.bbI != c.half) obsLost++;
            else Err("SV1 control: %s with the observers' look switched off: %s (the game's picture lost wanted): the observers are not what carries it", k.name, SvSays(c));
            ShFrame<MA>(); MgDueN();
        }
    }
    // the second back buffer surface: the answer
    const SvLeg s1 = SvPair([] { D->ColorFill(gRing.surf[1], nullptr, NewTag()); Drain(); return (DWORD)0; });
    if (s1.savesI != 1 || s1.savesN != 1) Err("SV1: a ColorFill into the swap chain's SECOND back buffer surface: %s (both fires must save: any surface of the swap chain counts)", SvSays(s1));
    // written between the two fires
    const SvLeg bt = SvPair(nullptr, -1, [] { const DWORD h = NewTag(); Fill(h); Drain(); return h; });
    if (bt.savesI != 0 || bt.noI != 1 || bt.savesN != 1 || bt.lost != 0 || bt.bbN != bt.half || bt.shownN != bt.tag)
        Err("SV1: written between the two fires: %s (no save at the first, a save at the second, the game's picture exact)", SvSays(bt));
    const DtBbs& b = dtBbs;
    printf("  nothing written: %ld of 3 pairs made no save at either fire; what is not a write (a Clear of the depth alone, a StretchRect out of the back buffer, a fill of another surface, a read-only lock): %ld of 4 made none\n", clean, notN);
    printf("  %d of %d kinds of write forced the save at both fires with the game's picture exact after each, and with its site switched off each lost it (%d picture(s) lost over the controls); the second back buffer surface: saved %ld and %ld; written between the fires: %ld then %ld\n",
        ok, (int)(sizeof(kinds) / sizeof(kinds[0])), lostCtl, s1.savesI, s1.savesN, bt.savesI, bt.savesN);
    printf("  the tracker in this case: %ld marks; bindings DXVK held that it missed %ld, that it held too long %ld\n", b.marks - marks0, b.rtMissed - miss0, b.rtStale - stale0);
    if (b.rtMissed != miss0) Err("SV1: at %ld mark(s) DXVK held a back buffer bound that the tracker's bookkeeping did not (0 wanted: nothing here binds beside it)", b.rtMissed - miss0);
    Ctl("each of %d kinds with its site switched off: the game's half-drawn picture lost at the generated frame's fire, %d in all; with the observers' look switched off, a replayed draw and a replayed UpdateSurface lost it too (%d of 2)", (int)(sizeof(kinds) / sizeof(kinds[0])), lostCtl, obsLost);
    MgClean("SV1");
    MgEnd();
    CaseEnd(name);
}

// ---------------------------------------------------------------- SV2, the calls beside the worker
// WHAT IT SHOWS. The same, for every write that reaches DXVK beside the replay: the render thread's own in a drained scope
// (a ColorFill, a StretchRect, an UpdateSurface, a Clear and a draw with the back buffer bound), ANOTHER THREAD's behind
// the ring, a LockRect that is not read-only, AN UNLOCK ALONE, a GetDC and A RELEASE OF THE DC ALONE, the two patch
// draws, the mod's own raw write, and A BACK BUFFER BOUND BESIDE THE WORKER with a replayed draw behind it (the tracker's
// bookkeeping does not follow that bind: the count beside the worker is what forces the save, and the next mark reads
// the binding from DXVK).
static volatile LONG gSvFT = 0;
static DWORD WINAPI SvForeignFill(void*) { D->ColorFill(gBb, nullptr, (D3DCOLOR)gSvFT); return 0; }
static void CaseSV2()
{
    const char* name = "SV2 the save's question: the calls beside the worker, each counted before and after (2026-10-12)";
    CaseBegin(name);
    SvFresh();
    gFkDcOk = true;
    const SvKind kinds[] = {
        { "the render thread's ColorFill in a drained scope", [] { const DWORD h = NewTag(); { DT_DIRECT(); D->ColorFill(gBb, nullptr, h); } return h; }, DT_BBS_FILL, true },
        { "its StretchRect in a drained scope", [] { const DWORD h = NewTag(); D->ColorFill(gOtherRt, nullptr, h); { DT_DIRECT(); D->StretchRect(gOtherRt, nullptr, gBb, nullptr, D3DTEXF_NONE); } return h; }, DT_BBS_STRETCH, true },
        { "its UpdateSurface in a drained scope", [] { const DWORD h = NewTag(); { DT_DIRECT(); D->UpdateSurface(gOtherRt, nullptr, gBb, nullptr); } SvLanded(h); return h; }, DT_BBS_UPDATE, true },
        { "its Clear of the target in a drained scope", [] { const DWORD h = NewTag(); { DT_DIRECT(); D->Clear(0, nullptr, D3DCLEAR_TARGET, h, 1.0f, 0); } return h; }, DT_BBS_CLEAR, true },
        { "its draw in a drained scope", [] { const DWORD h = NewTag(); gRing.drawTag = h; { DT_DIRECT(); D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); } gRing.drawTag = 0; return h; }, DT_BBS_DRAW, true },
        { "another thread's ColorFill", [] { const DWORD h = NewTag(); _InterlockedExchange(&gSvFT, (LONG)h); Join(Spawn(SvForeignFill), "SV2's other thread"); return h; }, DT_BBS_FILL, true },
        { "a LockRect that is not read-only", [] { const DWORD h = NewTag(); D3DLOCKED_RECT lr; gBb->LockRect(&lr, nullptr, 0); SvLanded(h); return h; }, DT_BBS_LOCK, true },
        { "an UnlockRect alone", [] { const DWORD h = NewTag(); gBb->UnlockRect(); SvLanded(h); return h; }, DT_BBS_LOCK, true },
        { "a GetDC", [] { const DWORD h = NewTag(); HDC dc = nullptr; gBb->GetDC(&dc); SvLanded(h); return h; }, DT_BBS_LOCK, true },
        { "a ReleaseDC alone", [] { const DWORD h = NewTag(); gBb->ReleaseDC((HDC)(UINT_PTR)1); SvLanded(h); return h; }, DT_BBS_LOCK, true },
        { "DrawRectPatch", [] { const DWORD h = NewTag(); D->DrawRectPatch(1, kSvVerts, nullptr); SvLanded(h); return h; }, DT_BBS_PATCH, true },
        { "DrawTriPatch", [] { const DWORD h = NewTag(); D->DrawTriPatch(1, kSvVerts, nullptr); SvLanded(h); return h; }, DT_BBS_PATCH, true },
        { "the mod's own raw write (the home copy's site)", [] { const DWORD h = NewTag(); { DT_DIRECT(); DtBbsRawWrite(); SvLanded(h); } return h; }, DT_BBS_RAW, true },
    };
    int ok = 0, lostCtl = 0;
    for (const SvKind& k : kinds) ok += SvKindRun("SV2", k, lostCtl);
    gFkDcOk = false;
    // A BACK BUFFER BOUND BESIDE THE WORKER, a replayed draw behind it. The pair is armed with ANOTHER target bound (the mark
    // reads that from DXVK); the render thread binds the back buffer in a drained scope; the draw is replayed.
    const LONG miss0 = dtBbs.rtMissed;
    auto bound = [](int blind) {
        D->SetRenderTarget(0, gOtherRt); Drain();
        const SvLeg g = SvPair([] { const DWORD h = NewTag(); { DT_DIRECT(); D->SetRenderTarget(0, gBb); } gRing.drawTag = h; D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); Drain(); gRing.drawTag = 0; return h; }, blind);
        return g;
    };
    const SvLeg bg = bound(-1);
    const bool bindOk = SvForced(bg);
    if (!bindOk) Err("SV2: the back buffer bound beside the worker and a replayed draw behind it: %s (both fires must save)", SvSays(bg));
    ShFrame<MA>(); MgDueN();
    const LONG missClean = dtBbs.rtMissed - miss0;
    const RingStats stc = gRingSt;
    const SvLeg bc = bound(DT_BBS_BIND);
    ViolRestore(stc);
    ShFrame<MA>(); MgDueN();
    const LONG missCtl = dtBbs.rtMissed - miss0 - missClean;
    if (bc.lostI < 1) Err("SV2 control: that bind not counted: %s (the game's picture lost wanted): the control shows nothing", SvSays(bc));
    if (missClean != 0 || missCtl != 1) Err("SV2: the next mark found the back buffer bound in DXVK where the bookkeeping had none: counted unexplained %ld time(s) on the clean leg and %ld on the control's (0 and 1 wanted: a bind counted beside the worker explains it, one that was not counted does not)", missClean, missCtl);
    printf("  %d of %d kinds of write beside the worker forced the save at both fires with the game's picture exact, and with its site switched off each lost it (%d picture(s) lost over the controls)\n",
        ok, (int)(sizeof(kinds) / sizeof(kinds[0])), lostCtl);
    printf("  the back buffer bound beside the worker, a replayed draw behind it: %s; the same with the bind not counted: the game's picture lost %ld time(s), and the next mark counted a binding it had missed %ld time(s) (0 on the clean leg)\n",
        bindOk ? "both fires saved, the game's picture exact" : "NOT AS WANTED", bc.lost, missCtl);
    Ctl("each of %d kinds beside the worker with its site switched off: the game's picture lost, %d in all; a bind of the back buffer not counted: lost %ld, and the next mark's read of DXVK counted the miss", (int)(sizeof(kinds) / sizeof(kinds[0])), lostCtl, bc.lost);
    MgClean("SV2");
    MgEnd();
    CaseEnd(name);
}

// ---------------------------------------------------------------- SV3, the worker held: order
// WHAT IT SHOWS, each with the worker HELD and the commands queued behind it:
//   a. THE GAME'S WRITE QUEUED AHEAD OF THE FIRE: the write is replayed first; the fire saves; the game's picture is exact.
//   b. THE FIRE QUEUED AHEAD OF THE WRITE: the fire finds nothing written and saves nothing; the write lands behind it and is
//      whole; the real frame's fire then saves.
//   c. ANOTHER THREAD'S WRITE THAT LANDS BEFORE THE FIRE. The worker is held; the other thread begins its ColorFill and waits
//      behind the ring; a second hold and the fire's note are queued; the first hold is released, the fill lands while
//      the worker stands in the second, and then the fire runs: it saves. CONTROL: the fire's test of the count
//      beside the worker switched off: the game's picture is lost.
//   d. A CALL IN FLIGHT ACROSS THE MARK. The worker is held with A WHOLE FRAME queued behind it (its Present arms the pair);
//      while it is held another thread begins a ColorFill of the back buffer, which waits for the worker to pass that
//      Present: so the call was counted BEFORE the mark and lands BEHIND it. The mark must see a call in flight, and
//      the pair answers "written" at its fire. CONTROL: that test switched off: the count had moved before the mark, so
//      the fire sees nothing since, saves nothing, and the game's picture is lost.
static HANDLE gSvEv[2] = {};
static volatile LONG gSvIn[2] = {};
static void SvHoldNote(DWORD, DWORD, DWORD i) { _InterlockedExchange(&gSvIn[i], 1); WaitForSingleObject(gSvEv[i], INFINITE); _InterlockedExchange(&gSvIn[i], 0); }
static volatile LONG gSvIn0 = 0;                 // dtBbs.in before the other thread began
static HANDLE gSvT = nullptr;
// (d)'s look and release, on a helper thread while the render thread stands in its Present behind the hold
static void SvFlightLook()
{
    gSvT = Spawn(SvForeignFill);
    if (!WaitFor([] { return dtBbs.in != gSvIn0; }, 3000)) Err("SV3: the other thread never began its call");
    Sleep(30);                                                // it has read how far the render thread committed: past the Present
    SetEvent(gSvEv[0]);
}
static SvLeg SvFlight(int blind)
{
    SvLeg g; FgArmedState& a = ShArmB::a;
    const RingStats st0 = gRingSt;
    g.half = NewTag(); _InterlockedExchange(&gSvFT, (LONG)g.half); _InterlockedExchange(&gSvIn0, dtBbs.in);
    _InterlockedExchange(&gBbsBlind, blind);
    Later l = { [] { return gSvIn[0] != 0; }, &SvFlightLook, 3000, 30, 0, false };
    if (!Run(SvHoldNote, 0)) Err("SV3: the hold's note was refused");
    const HANDLE h = Spawn(LaterMain, &l);
    g.tag = ShFrame<MA>();                                    // the whole frame, queued behind the hold: its Present arms the pair
    Join(h, "SV3's look and release");
    if (!l.met) Err("SV3: the worker never reached the hold");
    if (gSvT) { Join(gSvT, "SV3's other thread"); gSvT = nullptr; }
    B::Flush();
    LONG s0 = a.w.saves, n0 = a.w.noSaves;
    MgDueShow();
    g.savesI = a.w.saves - s0; g.noI = a.w.noSaves - n0; g.shownI = ShLastShown(); g.bbI = MgBb();
    g.lostI = gRingSt.backLost - st0.backLost;
    s0 = a.w.saves; n0 = a.w.noSaves;
    MgDueN();
    g.savesN = a.w.saves - s0; g.noN = a.w.noSaves - n0; g.shownN = ShLastShown(); g.bbN = MgBb();
    g.lost = gRingSt.backLost - st0.backLost;
    _InterlockedExchange(&gBbsBlind, -1);
    return g;
}
// (c): false where the other thread read the ring too late to land ahead of the fire (the leg is then run again)
static bool SvBetween(int blind, SvLeg& g)
{
    FgArmedState& a = ShArmB::a;
    const RingStats st0 = gRingSt;
    g = SvLeg();
    g.tag = ShFrame<MA>();
    g.half = NewTag(); _InterlockedExchange(&gSvFT, (LONG)g.half); _InterlockedExchange(&gSvIn0, dtBbs.in);
    _InterlockedExchange(&gBbsBlind, blind);
    if (!Run(SvHoldNote, 0)) Err("SV3: the first hold's note was refused");
    if (!WaitFor([] { return gSvIn[0] != 0; }, 3000)) Err("SV3: the worker never reached the first hold");
    const HANDLE t = Spawn(SvForeignFill);
    if (!WaitFor([] { return dtBbs.in != gSvIn0; }, 3000)) Err("SV3: the other thread never began its call");
    Sleep(30);                                                // it has read how far the render thread committed: the first hold
    LONG s0 = a.w.saves, n0 = a.w.noSaves;
    if (!Run(SvHoldNote, 1)) Err("SV3: the second hold's note was refused");
    DueI(MgP(), FG_DUE_SHOW); FireNote();
    SetEvent(gSvEv[0]);
    if (!WaitFor([] { return gSvIn[1] != 0; }, 3000)) Err("SV3: the worker never reached the second hold");
    const bool landed = WaitForSingleObject(t, 2000) == WAIT_OBJECT_0;   // the fill has landed while the worker stands in the second hold
    SetEvent(gSvEv[1]);
    Join(t, "SV3's other thread");
    Drain();
    g.savesI = a.w.saves - s0; g.noI = a.w.noSaves - n0; g.shownI = ShLastShown(); g.bbI = MgBb();
    g.lostI = gRingSt.backLost - st0.backLost;
    s0 = a.w.saves; n0 = a.w.noSaves;
    MgDueN();
    g.savesN = a.w.saves - s0; g.noN = a.w.noSaves - n0; g.shownN = ShLastShown(); g.bbN = MgBb();
    g.lost = gRingSt.backLost - st0.backLost;
    _InterlockedExchange(&gBbsBlind, -1);
    return landed;
}
static void CaseSV3()
{
    const char* name = "SV3 the save's question with the worker held: a write ahead of the fire, behind it, another thread's, and one in flight across the mark (2026-10-12)";
    CaseBegin(name);
    if (!gSvEv[0]) { gSvEv[0] = CreateEventW(nullptr, FALSE, FALSE, nullptr); gSvEv[1] = CreateEventW(nullptr, FALSE, FALSE, nullptr); }
    SvFresh();
    FgArmedState& a = ShArmB::a;
    // a. the write ahead of the fire
    auto ahead = [&](int blind, SvLeg& g) {
        const RingStats st0 = gRingSt;
        g = SvLeg(); g.tag = ShFrame<MA>(); g.half = NewTag();
        _InterlockedExchange(&gBbsBlind, blind);
        const LONG s0 = a.w.saves, n0 = a.w.noSaves; const DWORD h = g.half;
        MgHeld([&] { Fill(h); DueI(MgP(), FG_DUE_SHOW); FireNote(); });
        g.savesI = a.w.saves - s0; g.noI = a.w.noSaves - n0; g.shownI = ShLastShown(); g.bbI = MgBb(); g.lostI = gRingSt.backLost - st0.backLost;
        const LONG s1 = a.w.saves, n1 = a.w.noSaves;
        MgDueN();
        g.savesN = a.w.saves - s1; g.noN = a.w.noSaves - n1; g.shownN = ShLastShown(); g.bbN = MgBb(); g.lost = gRingSt.backLost - st0.backLost;
        _InterlockedExchange(&gBbsBlind, -1);
    };
    SvLeg ga; ahead(-1, ga);
    if (!SvForced(ga)) Err("SV3 a: the game's write queued ahead of the fire behind a held worker: %s (both fires must save)", SvSays(ga));
    RingStats stc = gRingSt;
    SvLeg ca; ahead(DT_BBS_FILL, ca);
    ViolRestore(stc);
    if (ca.lostI < 1) Err("SV3 a control: the replayed fill not seen: %s (the game's picture lost wanted): the control shows nothing", SvSays(ca));
    ShFrame<MA>(); MgDueN();
    // b. the fire ahead of the write
    SvLeg gb; gb.tag = ShFrame<MA>(); gb.half = NewTag();
    {
        const RingStats st0 = gRingSt; const DWORD h = gb.half;
        const LONG s0 = a.w.saves, n0 = a.w.noSaves;
        MgHeld([&] { DueI(MgP(), FG_DUE_SHOW); FireNote(); Fill(h); });
        gb.savesI = a.w.saves - s0; gb.noI = a.w.noSaves - n0; gb.shownI = ShLastShown(); gb.bbI = MgBb(); gb.lostI = gRingSt.backLost - st0.backLost;
        const LONG s1 = a.w.saves, n1 = a.w.noSaves;
        MgDueN();
        gb.savesN = a.w.saves - s1; gb.noN = a.w.noSaves - n1; gb.shownN = ShLastShown(); gb.bbN = MgBb(); gb.lost = gRingSt.backLost - st0.backLost;
    }
    if (gb.savesI != 0 || gb.noI != 1 || gb.savesN != 1 || gb.lost != 0 || gb.bbI != gb.half || gb.bbN != gb.half || gb.shownI != ShGenTag(gb.tag, false) || gb.shownN != gb.tag)
        Err("SV3 b: the fire queued ahead of the game's write behind a held worker: %s (no save at the first fire, the write whole behind it, a save at the second)", SvSays(gb));
    // c. another thread's write that lands before the fire
    SvLeg gc; bool landed = false;
    for (int tries = 0; tries < 3 && !landed; tries++) landed = SvBetween(-1, gc);
    if (!landed) Err("SV3 c: in three tries the other thread's fill never landed ahead of the fire: the leg proves nothing");
    else if (!SvForced(gc)) Err("SV3 c: another thread's fill landed while the worker stood between two holds, the fire behind: %s (both fires must save)", SvSays(gc));
    stc = gRingSt;
    SvLeg cc; bool landedC = false;
    for (int tries = 0; tries < 3 && !landedC; tries++) { ViolRestore(stc); landedC = SvBetween(DT_BBS_B_BESIDE, cc); }
    ViolRestore(stc);
    if (!landedC || cc.lostI < 1) Err("SV3 c control: the fire's test of the count beside the worker switched off (landed ahead of the fire: %s): %s (the game's picture lost wanted): the control shows nothing", landedC ? "yes" : "NO", SvSays(cc));
    ShFrame<MA>(); MgDueN();
    // d. a call in flight across the mark
    const LONG fl0 = dtBbs.found[0][DT_BBS_W_FLIGHT];
    const SvLeg gd = SvFlight(-1);
    const LONG flight = dtBbs.found[0][DT_BBS_W_FLIGHT] - fl0;
    if (!SvForced(gd) || flight != 1) Err("SV3 d: another thread's fill begun before the mark and landed behind it: %s; the generated frame's fire answered \"a call in flight at the mark\" %ld time(s) (both fires must save, and 1)", SvSays(gd), flight);
    ShFrame<MA>(); MgDueN();
    stc = gRingSt;
    const SvLeg cd = SvFlight(DT_BBS_B_FLIGHT);
    ViolRestore(stc);
    if (cd.lostI < 1) Err("SV3 d control: the mark's test for a call in flight switched off: %s (the game's picture lost wanted): the control shows nothing", SvSays(cd));
    ShFrame<MA>(); MgDueN();
    printf("  a. the game's write queued ahead of the fire behind a held worker: %s\n", SvSays(ga));
    printf("  b. the fire queued ahead of the write: %s\n", SvSays(gb));
    printf("  c. another thread's fill, landed while the worker stood between two holds, the fire behind: %s\n", SvSays(gc));
    printf("  d. another thread's fill begun before the mark and landed behind it (the worker held with the whole frame queued): %s; answered \"a call in flight at the mark\" %ld time(s)\n", SvSays(gd), flight);
    Ctl("a: the replayed fill not seen: lost %ld; c: the fire's test of the count beside the worker off: lost %ld; d: the mark's test for a call in flight off: lost %ld", ca.lost, cc.lost, cd.lost);
    MgClean("SV3");
    MgEnd();
    CaseEnd(name);
}

// ---------------------------------------------------------------- SV4, what cannot be followed answers "written"
// WHAT IT SHOWS. NOT WRITES, AND STILL "WRITTEN", because the path is not followed (each control shows the save NOT made with
// the site off; no picture can show more):
//   a Present through the device's own slot 17; a Present of the swap chain with a dirty region; a stray replayed
//   Present while the pair is armed; A PAIR WHOSE PRESENT CARRIES A RECTANGLE (DXVK may present it through GDI, with a DC on
//   the back buffer inside its Present).
// THE TABLE: a back buffer surface the fire names that is not in the table; a swap chain with more back buffers than the
// table holds. NO MARK: a question with no mark standing, and with a mark of another arm of the layer.
// AND THE READ OF THE BINDING FROM DXVK AT THE MARK. The tracker's bookkeeping is made to forget that the back buffer is bound
// (a note on the worker clears its bit, as a bind the layer never saw would leave it); the next pair's mark reads DXVK,
// finds the back buffer bound, COUNTS THE MISS, and a replayed draw behind it forces the save. CONTROL: the mark reads
// nothing from DXVK: the draw goes unseen and the game's picture is lost.
static volatile LONG gSvUnit[3] = {};
static void SvUnitNote(DWORD, DWORD, DWORD)
{
    DtBbs& b = dtBbs;
    const LONG m = b.marked; b.marked = 0; gSvUnit[0] = DtBbsWritten(gBb, 0, false) ? 1 : 0; b.marked = m;
    const LONG ar = b.markArm; b.markArm = ar - 1; gSvUnit[1] = DtBbsWritten(gBb, 0, false) ? 1 : 0; b.markArm = ar;
    gSvUnit[2] = DtBbsWritten(gBb, 0, false) ? 1 : 0;        // the pair as it stands: nothing written
}
static void SvForgetNote(DWORD, DWORD, DWORD) { dtBbs.rt = 0; DtBbsHot(); }
static void CaseSV4()
{
    const char* name = "SV4 the save's question: what cannot be followed answers \"written\" (presents, a rectangle, the table, no mark), and the binding read from DXVK (2026-10-12)";
    CaseBegin(name);
    SvFresh();
    static RGNDATA rgn = {};
    const SvKind kinds[] = {
        { "a Present through the device's own slot 17", [] { D->Present(nullptr, nullptr, nullptr, nullptr); return (DWORD)0; }, DT_BBS_PRESENT, false },
        { "a Present of the swap chain with a dirty region", [] { SC->Present(nullptr, nullptr, nullptr, &rgn, 0); return (DWORD)0; }, DT_BBS_PRESENT, false },
        { "a stray replayed Present while the pair is armed", [] { RecordPresent(); Drain(); return (DWORD)0; }, DT_BBS_PRESENT, false },
    };
    int ok = 0, lostCtl = 0;
    for (const SvKind& k : kinds) ok += SvKindRun("SV4", k, lostCtl);
    // a pair whose Present carries a rectangle
    FgArmedState& a = ShArmB::a;
    auto rectPair = [&](bool handIn, LONG& saves, LONG& none) {
        ShArmB::backRectOff = handIn ? 0 : 1;
        ClockAdd(16.7); ShInject();
        Facts(); const DWORD t = NewTag(); Fill(t);
        const RECT rc = { 0, 0, 64, 64 };
        Present<MA>(nullptr, &rc); Drain();
        if (!ShSettle()) Err("SV4: the fake helper did not read the frame's messages");
        const bool armed = B::S().w.openPhase == FG_PAIR_ARMED;
        const LONG s0 = a.w.saves, n0 = a.w.noSaves;
        MgDueShow(); MgDueN();
        saves = a.w.saves - s0; none = a.w.noSaves - n0;
        ShArmB::backRectOff = 0;
        return armed;
    };
    LONG rs = 0, rn = 0, cs = 0, cn = 0;
    const LONG rect0 = dtBbs.found[0][DT_BBS_W_RECT] + dtBbs.found[1][DT_BBS_W_RECT];
    const bool rArmed = rectPair(true, rs, rn);
    const LONG rectN = dtBbs.found[0][DT_BBS_W_RECT] + dtBbs.found[1][DT_BBS_W_RECT] - rect0;
    const bool cArmed = rectPair(false, cs, cn);
    if (!rArmed || rs != 2 || rn != 0 || rectN != 2) Err("SV4: a pair whose Present carries a rectangle (armed %d): %ld fire(s) saved, %ld made no save, answered \"a rectangle\" %ld time(s) (2, 0 and 2 wanted)", (int)rArmed, rs, rn, rectN);
    if (!cArmed || cs != 0 || cn != 2) Err("SV4 control: the rectangle not handed in (armed %d): %ld fire(s) saved, %ld made no save (0 and 2 wanted): the control shows nothing", (int)cArmed, cs, cn);
    // the table
    ShArmB::backSurf = gOtherRt;
    const SvLeg t1 = SvPair(nullptr);
    ShArmB::backSurf = nullptr;
    if (t1.savesI != 1 || t1.savesN != 1) Err("SV4: a back buffer surface the fire names that is not in the table: %s (both fires must save)", SvSays(t1));
    dtBbs.more = 1;                                           // (between frames, the worker idle: as an arm that found more back buffers than the table holds)
    const SvLeg t2 = SvPair(nullptr);
    dtBbs.more = 0;
    if (t2.savesI != 1 || t2.savesN != 1) Err("SV4: a swap chain with more back buffers than the table holds: %s (both fires must save)", SvSays(t2));
    const SvLeg t3 = SvPair(nullptr);
    if (t3.savesI != 0 || t3.savesN != 0) Err("SV4: the table whole again and nothing written: %s (no save wanted)", SvSays(t3));
    // no mark
    ShFrame<MA>();
    Run(SvUnitNote); Drain();
    MgDueN();
    if (gSvUnit[0] != 1 || gSvUnit[1] != 1 || gSvUnit[2] != 0) Err("SV4: asked with no mark standing the answer was %ld, with a mark of another arm %ld, and with the pair's own mark and nothing written %ld (1, 1 and 0 wanted)", gSvUnit[0], gSvUnit[1], gSvUnit[2]);
    // the binding read from DXVK at the mark
    auto forgot = [](int blind) {
        Run(SvForgetNote); Drain();                           // the bookkeeping forgets the back buffer is bound (the device still holds it)
        _InterlockedExchange(&gBbsBlind, blind == DT_BBS_B_REREAD ? blind : -1);
        const SvLeg g = SvPair([] { const DWORD h = NewTag(); gRing.drawTag = h; D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); Drain(); gRing.drawTag = 0; return h; }, blind);
        return g;
    };
    const LONG miss0 = dtBbs.rtMissed;
    const SvLeg gr = forgot(-1);
    const LONG miss = dtBbs.rtMissed - miss0;
    if (!SvForced(gr) || miss != 1) Err("SV4: the bookkeeping made to forget the bound back buffer, a replayed draw behind the next mark: %s; the mark counted the miss %ld time(s) (both fires must save, and 1)", SvSays(gr), miss);
    ShFrame<MA>(); MgDueN();
    const RingStats stc = gRingSt;
    const SvLeg cr = forgot(DT_BBS_B_REREAD);
    ViolRestore(stc);
    if (cr.lostI < 1) Err("SV4 control: the mark reads no binding from DXVK: %s (the game's picture lost wanted): the control shows nothing", SvSays(cr));
    ShFrame<MA>(); MgDueN();                                  // (the mark reads DXVK again: the bookkeeping is whole)
    const SvLeg after = SvPair(nullptr);
    if (after.savesI != 0 || after.savesN != 0) Err("SV4: behind the controls a pair with nothing written: %s (no save wanted)", SvSays(after));
    printf("  %d of %d presents that were not the pair's own forced the save at both fires, and with the site switched off none was made; a pair whose Present carries a rectangle: %ld fires saved, %ld with the rectangle not handed in\n",
        ok, (int)(sizeof(kinds) / sizeof(kinds[0])), rs, cs);
    printf("  the table: a back buffer not in it saved %ld and %ld; more back buffers than it holds %ld and %ld; whole again %ld and %ld. No mark standing: answered written %ld; a mark of another arm: %ld; the pair's own mark, nothing written: %ld\n",
        t1.savesI, t1.savesN, t2.savesI, t2.savesN, t3.savesI, t3.savesN, gSvUnit[0], gSvUnit[1], gSvUnit[2]);
    printf("  the bookkeeping made to forget the bound back buffer: the next mark read DXVK and counted the miss %ld time(s); a replayed draw behind it: %s\n", miss, SvSays(gr));
    Ctl("a present's site off: no save made; the rectangle not handed in: no save made; the mark reading no binding from DXVK: the game's picture lost %ld time(s)", cr.lost);
    MgClean("SV4");
    MgEnd();
    CaseEnd(name);
}

// ---------------------------------------------------------------- SV5, the census under the tracker
// WHAT IT SHOWS. MC's random frames (dt_fg_merged.h: the game drawing into the back buffer before a fire, between the fires
// or not at all, standby frames, builds, refused submits, the placer overruled, fires left to the next frame's
// injection, drain or hand), four segments of five hundred at one to four images, with the save answered by the device
// thread's tracker. EVERY ANSWER IS HELD AGAINST WHAT THE FAKE KNOWS AT THAT VERY QUESTION (ShArmB::BackWritten): never "not
// written" where the fake knows a write; and here, where the game's only writes are replayed fills, never "written"
// where it knows none either. The witness must find the screen whole and the fake must count no picture lost.
// CONTROL: a segment with the tracker answering "not written", always: every such answer over a write is counted, and
// the fake counts the game's picture lost.
static void CaseSV5()
{
    const char* name = "SV5 the census under the tracker: MC's random frames, every answer held against the fake's own knowledge (2026-10-12)";
    CaseBegin(name);
    McTot t; LONG under = 0, over = 0, asked = 0;
    for (int images = 1; images <= 4; images++)
    {
        const LONG a0 = dtBbs.found[0][0] + dtBbs.found[1][0];
        LONG all0 = 0; for (int f = 0; f < 2; f++) for (int k = 0; k < DT_BBS_W_N; k++) all0 += dtBbs.found[f][k];
        gSeed = 0x5A7E0000u ^ (gShAtHand ? 0x00F0F00Fu : 0u) ^ (unsigned)(images * 977);
        McSegment<MA>(0, 500, images, images % 3, 3, false, -1, -1, t);
        under += ShArmB::backUnder; over += ShArmB::backOver;
        LONG all1 = 0; for (int f = 0; f < 2; f++) for (int k = 0; k < DT_BBS_W_N; k++) all1 += dtBbs.found[f][k];
        asked += all1 - all0; (void)a0;
    }
    printf("  %ld frames at one to four images under the tracker: %ld pairs armed, %ld fires saved and %ld made no save; the tracker was asked %ld time(s): \"not written\" where the fake knows a write %ld, \"written\" where it knows none %ld; the witness: %d fault(s)%s%s\n",
        t.frames, t.armed, t.saves, t.noSaves, asked, under, over, t.bad, t.bad ? ": " : "", t.first.c_str());
    if (under) Err("SV5: the tracker answered \"not written\" %ld time(s) where the fake knows the game wrote the back buffer", under);
    if (over) Err("SV5: the tracker answered \"written\" %ld time(s) where the fake knows of no write: in this census the game's only writes are replayed fills, and nothing here is in doubt", over);
    if (t.bad) Err("SV5: the witness counts %d fault(s) under the tracker; the first: %s", t.bad, t.first.c_str());
    if (!t.saves || !t.noSaves || asked < t.saves + t.noSaves) Err("SV5: the run did not take both roads, or the tracker was not what answered (fires that saved %ld, that made no save %ld, questions %ld)", t.saves, t.noSaves, asked);
    const RingStats stc = gRingSt;
    McTot c;
    _InterlockedExchange(&gBbsLie, 1);
    gSeed = 0x5A7E0C71u; McSegment<MA>(0, 300, 3, RING_CS_LAZY, 3, false, -1, -1, c);
    _InterlockedExchange(&gBbsLie, 0);
    const LONG lost = gRingSt.backLost - stc.backLost, underC = ShArmB::backUnder;
    ViolRestore(stc);
    if (lost < 1 || underC < 1 || c.saves != 0) Err("SV5 control: the tracker answering \"not written\", always, three hundred frames: the fake counted the game's picture lost %ld time(s), %ld such answer(s) over a write, and %ld fire(s) saved (at least 1, at least 1, and 0 wanted): the control shows nothing", lost, underC, c.saves);
    Ctl("the tracker answering \"not written\", always, three hundred frames: %ld such answer(s) over a write, and the game's half-drawn picture lost at a fire %ld time(s)", underC, lost);
    CaseEnd(name);
}
