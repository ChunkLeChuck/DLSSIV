// FRAME GENERATION UNDER WINE AND PROTON, THE FAKE CLIENT'S MODE --fake-fg=16 (DLSS-IV, 2026-10-24, round 7, L; source/fg/fg_linux.h,
// source/transport/feed_fg.h "UNDER WINE AND PROTON", docs/reports/fg-5/linux/READING.md).
//
// WHAT IT PROVES, through the client's own lines (fg_xport.h and fg_linux.h, this program's binding saying Wine) against the BUILT
// helper, on Windows: the game's own three fences (OPAQUE_WIN32 timelines this program makes, duplicated into the helper, opened there
// with OpenSharedHandle), the probe both ways, and what fails closed. Windows' D3D12 opens such a handle as Wine's vkd3d-proton does
// (DECISIONS, the Linux sync path: sync=1 ran FAST on Windows), so the lines run here as they would there. NOT LINUX: no Wine runs.
//   --fake-wine=1  THE FENCES CONNECT: a set CREATED, the three the game's own, the ack echoing them, the probe's value on fgPrep and
//                  fgOut; three whole pairs, each generated image read by pixels; the set released and asked again (the echo held
//                  against the first: no fence made twice), one pair more; at the helper's exit fgPrep and fgOut at the top (the tail)
//   --fake-wine=2  THE HELPER SEES NOTHING (a silent in-fence, planted: the helper is handed three fences of this program's that no
//                  signal reaches): refused FEED_FG_ST_FENCES within the helper's bound, the session's verdict and its one warnings
//                  line; the next build refused with NO exchange; no wait queued, a submit behind it runs at once; the upscaler's
//                  frames go on by pixels
//   --fake-wine=3  THE GAME SEES NOTHING (fgPrep and fgOut silent, planted the same way): the set CREATED and given back, the
//                  verdict and its line, nothing held; the frames go on; and THE CONTROL, what the probe exists to prevent: the
//                  silent fence TRUSTED (a set holding it, as the base's lines hold an import that said success) and a submit queued
//                  behind a wait on it: held, as long as it is looked at, until the client's own release lets it run
//   --fake-wine=4  THE BRIDGE: the binding says the transport rides its CPU bridge: refused before any 'g', the reason said once
#pragma once

// (gFakeWine, gWineWarns and gWineWarnLast are declared above the binding, in fgpipe.cpp)
static HANDLE FakeHelperProcess() { return gHelper; }

// A TIMELINE OF THIS PROGRAM'S, exported, its handle duplicated into the helper: what FgxWineBefore makes, made by hand for a plant
static bool WineDup(VkSemaphore* sem, uint64_t* inHelper)
{
    HANDLE h = nullptr, r = nullptr;
    if (FeedVkCreateTimeline(&gVk, true, sem, &h) != 1 || !h) return false;
    const bool ok = DuplicateHandle(GetCurrentProcess(), h, gHelper, &r, 0, FALSE, DUPLICATE_SAME_ACCESS) && r;
    CloseHandle(h);
    *inHelper = (uint64_t)(uintptr_t)r;
    return ok;
}
// THE PLANT: the session's three made as FgxWineBefore makes them (the game's test signal on the in-fence), but the helper handed
// DECOYS, three more of this program's that nothing ever signals or reads: kind 1 all three (the helper's read of the in-fence never
// sees the game's signal), kind 2 the real in-fence and decoys for fgPrep and fgOut (the helper signals the decoys; the game's own
// stay at 0). Exactly the shape of Wine's silent import: every call says success, and the values never connect.
static bool WinePlant(int kind)
{
    VkSemaphore real[3] = {}, decoy[3] = {}; uint64_t hr[3] = {}, hd[3] = {};
    bool ok = true;
    for (int i = 0; i < 3 && ok; i++) ok = WineDup(&real[i], &hr[i]) && WineDup(&decoy[i], &hd[i]);
    ok = ok && SignalNow(real[0], FGX_WINE_PROBE_N);
    if (!ok) return false;
    for (int i = 0; i < 3; i++) { gWineSt.sem[i] = real[i]; gWineSt.inHelper[i] = (kind == 2 && i == 0) ? hr[0] : hd[i]; }
    gWineSt.state = FGX_WINE_MADE;
    return true;
}
// THE QUEUE IS FREE: a submit queued now runs (nothing of frame generation's stands on the queue ahead of it)
static bool WineQueueFree(uint64_t v) { return BodySubmit(v) && FeedVkWaitTimeline(&gVk, gBodySem, v, 1000); }
// THE UPSCALER GOES ON: frames n..n+2, each answered with its pixels in Output
static bool WineFramesGoOn(uint64_t n)
{
    const UINT LX = W / 4, CY = H / 2;
    int good = 0;
    for (uint64_t k = n; k < n + 3; k++)
    {
        const float r = (float)(k % 4) * 0.25f, g = (float)((k + 1) % 4) * 0.25f, b = (float)((k + 2) % 4) * 0.25f;
        if (SendF(k, r, g, b, true) && FeedVkWaitTimeline(&gVk, gSemOut, k, 5000) && OutputAt(gIm[FEED_OUTPUT], LX, CY, r, g, b, nullptr) == 4096) good++;
    }
    return good == 3;
}
// THE CONTROL'S WAIT AND THE SUBMIT BEHIND IT, on a thread of their own (as WaitThread): the set holding the silent fence
static FgxSet gWineCtl; static FgxWaits gWineCtlW; static volatile LONG gWineCtlQueued, gWineCtlBody;
static DWORD WINAPI WineCtlThread(void*)
{
    if (FgxPrepWait<FakeB>(&gWineCtl, &gWineCtlW, 34)) _InterlockedExchange(&gWineCtlQueued, 1);
    if (BodySubmit(3)) _InterlockedExchange(&gWineCtlBody, 1);
    return 0;
}

static void WineRun()
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    const UINT CX = W / 2, CY = H / 2;
    HANDLE none = nullptr;
    const bool body = FeedVkCreateTimeline(&gVk, false, &gBodySem, &none) >= 0 && gBodySem != VK_NULL_HANDLE;
    if (none) CloseHandle(none);
    if (!Proof(body && gFakeWine >= 1 && gFakeWine <= 4, "the Wine case %d is set up (this program's binding says Wine; a timeline of its own for the submits behind)", gFakeWine)) return;
    if (gFakeWine == 1)
    {   // ---- THE FENCES CONNECT
        const int x = Acquire(FP16, DL);
        Proof(x == FGX_CREATED && gWineSt.state == FGX_WINE_LIVE && gSet.semIn == gWineSt.sem[0] && gSet.semPrep == gWineSt.sem[1] && gSet.semOut == gWineSt.sem[2],
              "CREATED with THE GAME'S OWN three fences (the lines' state %ld, LIVE wanted): the set holds the semaphores this side made, none imported", (long)gWineSt.state);
        Proof(gSet.fence[0] == gWineSt.inHelper[0] && gSet.fence[1] == gWineSt.inHelper[1] && gSet.fence[2] == gWineSt.inHelper[2] && gSet.fence[0] && gSet.fence[1] && gSet.fence[2],
              "the ack ECHOES the three handles this side sent into the helper");
        const uint64_t p0 = Seen(gSet.semPrep, &gSeenPrep), o0 = Seen(gSet.semOut, &gSeenOut);
        Proof(p0 == FGX_WINE_PROBE_N && o0 == FGX_WINE_PROBE_N, "THE PROBE, the helper's half seen here: fgPrep %llu and fgOut %llu at the probe's value (%llu), signalled from its CPU before it answered",
              (unsigned long long)p0, (unsigned long long)o0, (unsigned long long)FGX_WINE_PROBE_N);
        const float C[3][3] = { { 0.25f, 0.5f, 0.75f }, { 0.75f, 0.25f, 0.5f }, { 0.5f, 0.75f, 0.25f } };
        int frames = 0, pixels = 0; bool prep = false, preps = true;
        for (int i = 0; i < 3 && x == FGX_CREATED; i++)
        {
            const uint64_t n = 33 + (uint64_t)i;
            const uint64_t o = Pair(n, i == 0 ? FEED_FG_Q_RESET_BUILD : 0u, FEED_FG_G_DISPATCH, C[i][0], C[i][1], C[i][2], true, &prep);
            preps = preps && prep;
            if (o == 2 * n) frames++;
            if (CountAt(SetImg(FEED_FG_GEN), CX, CY, C[i][0], C[i][1], C[i][2], nullptr) == 4096) pixels++;
        }
        Proof(frames == 3 && pixels == 3 && preps, "three whole pairs on the game's own fences (33 to 35): fgPrep at each n, fgOut at 2n (a frame) %d of 3, the generated image the constant the copies put in, by pixels, %d of 3", frames, pixels);
        const int r = x == FGX_CREATED ? FgxRelease<FakeB>(&gSet, DL) : -1;
        const VkSemaphore keep = gSet.semIn;
        const int x2 = r == FGX_RELEASED ? Acquire(FP16, DL) : -1;
        Proof(r == FGX_RELEASED && x2 == FGX_CREATED && gSet.semIn == keep && keep == gWineSt.sem[0],
              "released and asked again: CREATED, the ack's three the same as the first (the lines' check), the same three semaphores, nothing made twice");
        bool prep2 = false;
        const uint64_t o36 = x2 == FGX_CREATED ? Pair(36, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, C[0][0], C[0][1], C[0][2], true, &prep2) : 0;
        Proof(o36 == 72 && prep2, "and pair 36 in the second set has its frame (fgOut %llu, 72 wanted)", (unsigned long long)o36);
        Proof(gWineWarns == 0 && !gFenceBack, "nothing was said in the warnings file (%d lines), and no fence ever read lower than before", gWineWarns);
        return;
    }
    if (gFakeWine == 4)
    {   // ---- THE BRIDGE
        const ULONGLONG t0 = GetTickCount64();
        const int x = Acquire(FP16, DL);
        const ULONGLONG ms = GetTickCount64() - t0;
        Proof(x == FGX_REFUSED && gSet.status == FEED_FG_ST_FENCES && gWineSt.state == FGX_WINE_FAILED && gWineSt.why == FGX_WHY_BRIDGE && !gSet.semIn && ms < 200 && FgxWineLocalWhy(gWineSt),
              "ON THE CPU BRIDGE the set is refused before any 'g' (%llu ms), the session's verdict the bridge, nothing made, the log saying THE GAME refused it", (unsigned long long)ms);
        Proof(gWineWarns == 1 && !strcmp(gWineWarnLast, FgxWineWarnText(FGX_WHY_BRIDGE)), "its one warnings line: \"%s\"", gWineWarnLast);
        Proof(WineFramesGoOn(33), "and the upscaler's frames go on: 33 to 35 answered, each with its pixels");
        return;
    }
    // ---- THE TWO SILENT PLANTS
    const int kind = gFakeWine == 2 ? 1 : 2;
    if (!Proof(WinePlant(kind), "the plant: %s", kind == 1 ? "the helper handed three fences nothing signals (it cannot see the game's test signal)"
                                                             : "the helper handed the real in-fence and two fences the game never reads (the game cannot see the helper's test signal)")) return;
    const ULONGLONG t0 = GetTickCount64();
    const int x = Acquire(FP16, DL);
    const ULONGLONG ms = GetTickCount64() - t0;
    const int whyWant = kind == 1 ? FGX_WHY_HELPER : FGX_WHY_GAME;
    if (kind == 1)
        Proof(x == FGX_REFUSED && gSet.status == FEED_FG_ST_FENCES && ms >= 1300 && ms < 10000 && !FgxWineLocalWhy(gWineSt),
              "THE HELPER SEES NOTHING: refused FEED_FG_ST_FENCES (status %d) after %llu ms, the helper's bound (%u ms) and no more; the log names the HOST's refusal", gSet.status, (unsigned long long)ms, FGX_WINE_PROBE_MS);
    else
        Proof(x == FGX_IMPORT && !gSet.have && ms >= 1300 && ms < 3000,
              "THE GAME SEES NOTHING: the set was CREATED and given back (the lines say %d, IMPORT wanted) after %llu ms, ONE bound of this side's (%u ms): fgOut is not waited for once fgPrep was not seen", x, (unsigned long long)ms, FGX_WINE_PROBE_MS);
    Proof(gWineSt.state == FGX_WINE_FAILED && gWineSt.why == whyWant && !gSet.semIn && !gSet.semPrep && !gSet.semOut,
          "the session's verdict, failed for good (reason %ld, %d wanted), and THE SET HOLDS NO FENCE: nothing can be queued on one", (long)gWineSt.why, whyWant);
    Proof(gWineWarns == 1 && !strcmp(gWineWarnLast, FgxWineWarnText(whyWant)), "its one warnings line: \"%s\"", gWineWarnLast);
    const ULONGLONG t1 = GetTickCount64();
    const int x2 = Acquire(FP16, DL);
    const ULONGLONG ms2 = GetTickCount64() - t1;
    Proof(x2 == FGX_REFUSED && gSet.status == FEED_FG_ST_FENCES && ms2 < 200 && gWineWarns == 1 && FgxWineLocalWhy(gWineSt),
          "the next build is refused with NO exchange (%llu ms) and says nothing again (%d warnings line in all); the log says THE GAME refused it: %s", (unsigned long long)ms2, gWineWarns, FgxWineLocalWhy(gWineSt) ? FgxWineLocalWhy(gWineSt) : "(NO REASON)");
    // FAIL CLOSED, BY THE QUEUE: the wait the machine would queue for a pair cannot be (no fence), and a submit behind it runs at once
    const bool queued = FgxPrepWait<FakeB>(&gSet, &gWaits, 33);
    Proof(!queued && WineQueueFree(1), "FAIL CLOSED: the pair's wait on fgPrep is NOT queued (no fence in the set), and a submit queued behind it runs within 1 s");
    Proof(WineFramesGoOn(33), "and the upscaler's frames go on: 33 to 35 answered, each with its pixels in Output");
    if (kind == 2)
    {   // ---- THE CONTROL: the silent fgPrep TRUSTED, as an import that said success was trusted before the probe. A set holding it,
        // a wait queued on it for pair 34 and a submit behind the wait, on a thread of their own: the submit must NOT run, whatever
        // the helper does, until the client's own release; then it runs.
        gWineCtl = {}; gWineCtl.semPrep = gWineSt.sem[1];
        const HANDLE th = CreateThread(nullptr, 0, WineCtlThread, nullptr, 0, nullptr);
        const bool returned = th && WaitForSingleObject(th, 1500) == WAIT_OBJECT_0;
        const bool ranEarly = FeedVkWaitTimeline(&gVk, gBodySem, 3, 3000);
        Proof(returned && gWineCtlQueued && !ranEarly,
              "THE CONTROL, what the probe prevents: with the silent fence trusted, the wait was queued and the submit behind it did NOT run in 3 s (the shape of a frozen game)");
        bool did = false;
        const VkResult rr = FgxPrepRelease<FakeB>(&gWineCtl, &gWineCtlW, &did);
        const bool ran = FeedVkWaitTimeline(&gVk, gBodySem, 3, 2000);
        Proof(rr == VK_SUCCESS && did && ran, "and the client's own release (fgPrep signalled past the top from the CPU) lets it run: released %s, the submit ran %s", did ? "yes" : "NO", ran ? "yes" : "NO");
        if (th) CloseHandle(th);
    }
}
