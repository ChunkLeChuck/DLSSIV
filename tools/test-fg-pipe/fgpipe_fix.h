// THE HELPER'S TWO FIXES OWED BEFORE A GENERATED FRAME IS SHOWN, AT A DESK: --fake-fg=12 and --fake-fg=13 (2026-10-10, frame
// generation phase 3; docs/reports/fg-3/map-2026-10-06/helper-wire.md, helper-wire-18 and helper-wire-19;
// check-transport.md, R6, which says why helper-wire-19's proof could not pass as the map worded it).
//
// helper-wire-18, --fake-fg=12: AN ANSWER SIGNALLED FROM THE CPU MUST NEVER SAY "A FRAME". The helper answers a pair on
// fgOut with a QUEUE signal; where the queue refuses the signal the CPU signals. This program reads fgOut AS THE GAME
// DOES, BY EQUALITY (source/fg/fg_machine.h, ShadowAnswerSeen): exactly 2n is "a frame", exactly 2n+1 "no frame",
// anything else is no answer for that pair, and no frame is shown for it.
//     --fake-fix=1   EVERY KIND OF PAIR, ITS ANSWER MADE BY THE CPU (the helper's levers --fgrefuse_at=2 --fgrefuse_n=12
//                    --fgfail_at=3 --fgraise_at=6): a pair that WAS dispatched, a WITHHELD one, one with NO SOURCE, a
//                    CLOSED one, a 'G' with no 'Q', three planted failures of the dispatch, the pair after them (not
//                    prepared), a pair after the set's release (no set), the pair whose dispatch RAISES in AMD's library
//                    and the pair after the latch. Each must read "no frame"
//     --fake-fix=2   NOTHING THE QUEUE STILL OWES MAY LAND BEHIND A CPU SIGNAL (--fgrefuse_at=3): pair 34's dispatch is held
//                    behind its fgIn, so its 2n is queued; pair 35's answer is then the CPU's to make. It must NOT be
//                    signalled while 34's is queued, and once fgIn is signalled fgOut must read 68 and never lower
//     --fake-fix=3   THE DEVICE REMOVED FOR REAL (--fgremove_at=2: Direct3D's own RemoveDevice behind the submit of pair
//                    34's dispatch, before its answer): fgOut must never read 68, and THE HELPER MUST END THE SESSION
//                    ITSELF at that 'G', with a removed device's exit code. What fgOut does read, and whether the
//                    dispatch wrote the generated image, is printed: a measurement of this driver and runtime
// helper-wire-19, --fake-fg=13: FRAME GENERATION'S RING NEVER BLOCKS THE SERVE THREAD WHILE FRAMES ARE SERVED.
//     --fake-fix=1   THE REAL HOLD, ON A RING OF THREE (the third build's own; a development helper with --fgring=3): pair
//                    34's dispatch is held behind its fgIn, so its list has really not retired, and THREE more pairs are
//                    written behind it (35 a dispatch too, its fgIn withheld as well). The prepares of pairs 36 and 37
//                    need the slot of the held dispatch. NOTHING ON THE QUEUE CAN ANSWER WHILE IT IS HELD (one queue:
//                    the fake client's own held 'G' proves that), so what is measured is THE SERVE THREAD'S OWN TIME: a
//                    'g' the helper must refuse is written behind each of those 'Q' and its ack, which comes over the
//                    pipe and not from the queue, is timed. (THREE FRAMES AND NOT FOUR: the UPSCALER's own ring is
//                    three long too and still waits 2 s for its slot. A fourth frame behind the hold took 2009 ms on
//                    the serve thread in this case's first run. That ring is the shipped release's and is not changed.)
//     --fake-fix=2   EVERY SLOT HELD, THE QUEUE GOING ON (--fgring_hold=18 --fgring_holdn=6: six served lists find every
//                    slot not retired): pair 41's dispatch and the prepares of pairs 42 to 46 get no list. Each pair must
//                    be answered "no frame" at once, fgPrep must come at once, and the out-fence of the frame written
//                    right behind each such pair's 'G' must come at the latency it has behind a pair that dispatched
// THE CONTROLS are run.py's: the same runs against a helper started with --fgcpu_same=1 or --fgring_wait=1, which restore
// what the helper did until 2026-10-10. The proofs named for each must FAIL there, and no other.
// THIS FILE INCLUDES NOTHING OF host\, as the rest of this program.
#pragma once

// A 'g' the helper must refuse at once (a format outside its five), written and its ack read: how long that took, in ms,
// or -1 when no ack came in 10 s. The helper answers it on its serve thread, so the time is that thread's own.
static double SRefusedG()   // (fgpipe_stills.h's until round 8's deep strip took the stills mode out)
{
    LARGE_INTEGER f, a, b; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&a);
    FeedFgBuildIvt g = {}; g.tag = 'g'; g.disp_w = W; g.disp_h = H; g.present_fmt = (uint32_t)DXGI_FORMAT_R32G32B32A32_FLOAT; g.render_w = W; g.render_h = H;
    FeedFgAck2Ivt ack = {};   // (a 'g' without FEED_FG_BUILD_WIRE3 is answered in wire 2's 104 bytes)
    if (!PipeWrite(&g, sizeof(g)) || !PipeRead(&ack, sizeof(ack), 10000) || ack.status != FEED_FG_ST_FORMAT) return -1.0;
    QueryPerformanceCounter(&b);
    return (double)(b.QuadPart - a.QuadPart) * 1000.0 / (double)f.QuadPart;
}

static int   xCase = 1;                             // --fake-fix=N
static DWORD gExitWant = 0;                         // the exit code the helper must end with when it is not 0 (main's tail reads it)

// THE GAME'S OWN READING OF fgOut FOR PAIR n: by equality, never "at least"
enum { X_FRAME = 0, X_NONE = 1, X_NEITHER = 2 };
static int XReads(uint64_t n, uint64_t out) { return out == 2 * n ? X_FRAME : out == 2 * n + 1 ? X_NONE : X_NEITHER; }
static const char* XWord(int r) { return r == X_FRAME ? "A FRAME" : r == X_NONE ? "no frame" : "no answer for this pair, so no frame"; }

static double XNowMs()
{
    LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}
// 'F'(n) as SendF writes it, and WHEN the message was written (the colour's own submit comes first and is not timed)
static bool XSendF(uint64_t n, const float* c, double* at)
{
    Begin(); Clear(gIm[FEED_COLOR], c[0], c[1], c[2], 1.0f);
    if (!Submit(gSemIn, n)) return false;
    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.frame_time_ms = 16.67f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
    *at = XNowMs();
    return PipeWrite(&f, sizeof(f));
}
// A wait for a fence, and how long after `since` it was reached
static bool XWait(VkSemaphore s, uint64_t v, DWORD ms, double since, double* took)
{
    const bool ok = FeedVkWaitTimeline(&gVk, s, v, ms);
    *took = XNowMs() - since;
    return ok;
}
static void XSeedGen()
{
    Begin(); Clear(SetImg(FEED_FG_GEN), -1.0f, -1.0f, -1.0f, -1.0f);
    NEED(Submit(VK_NULL_HANDLE, 0), "the seed of GEN");
}
static bool XSet()                                  // the game's own shape since lever 1: no HUD-less image
{
    const int x = Acquire((uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT, FEED_FG_BUILD_DEADLINE_MS, FEED_FG_BUILD_NO_HUDLESS);
    if (!Proof(x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut, "the game's set (no HUD-less image) is CREATED through the client's own lines (status %d)", gSet.status)) return false;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    return true;
}
static void XMedMax(const double* v, int n, double* med, double* mx)
{
    double s[64]; if (n > 64) n = 64;
    for (int i = 0; i < n; i++) s[i] = v[i];
    for (int i = 1; i < n; i++) { const double k = s[i]; int j = i - 1; while (j >= 0 && s[j] > k) { s[j + 1] = s[j]; j--; } s[j + 1] = k; }
    *med = n ? s[n / 2] : -1.0; *mx = n ? s[n - 1] : -1.0;
}

// =====================================================================================================================
// helper-wire-18, case 1: every kind of pair, answered by the CPU
// =====================================================================================================================
static void FixCpuKinds()
{
    const UINT CY = H / 2, LX = W / 4;
    const float A[3] = { 0.25f, 0.5f, 0.75f }, B[3] = { 0.75f, 0.25f, 0.5f };
    bool prep = false;
    if (!XSet()) return;
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    if (!Proof(prep && o33 == 66, "pair 33, answered BY THE QUEUE behind its dispatch: fgOut reads %llu: %s", (unsigned long long)o33, XWord(XReads(33, o33)))) return;
    // FROM HERE EVERY ANSWER IS THE CPU'S TO MAKE (the helper's lever: the queue "refuses" the signal of 'G' 2 to 13)
    const uint64_t o34 = Pair(34, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    Proof(o34 == 69, "A PAIR THAT WAS DISPATCHED (34: its list was submitted), the queue refusing its answer: the CPU's answer reads %llu: %s (69 wanted: a CPU answer never says \"a frame\")",
          (unsigned long long)o34, XWord(XReads(34, o34)));
    const uint64_t o35 = Pair(35, 0, FEED_FG_G_WITHHELD, A[0], A[1], A[2], true, &prep);
    Proof(o35 == 71, "a pair WITHHELD by the game (35), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o35, XWord(XReads(35, o35)));
    const uint64_t o36 = Pair(36, 0, FEED_FG_G_NOSOURCE, 0, 0, 0, true, &prep);
    Proof(o36 == 73, "a pair with NO SOURCE (36), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o36, XWord(XReads(36, o36)));
    const uint64_t o37 = Pair(37, 0, FEED_FG_G_CLOSED, 0, 0, 0, true, &prep);
    Proof(o37 == 75, "a pair CLOSED before its 'G' was due (37), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o37, XWord(XReads(37, o37)));
    bool ok = SendG(45, FEED_FG_G_DISPATCH) && FeedVkWaitTimeline(&gVk, gSet.semOut, 90, 5000);
    const uint64_t o45 = Seen(gSet.semOut, &gSeenOut);
    Proof(ok && o45 == 91, "a 'G' with no 'Q' (45, said to be a dispatch), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o45, XWord(XReads(45, o45)));
    uint64_t o[3];
    for (int k = 0; k < 3; k++) o[k] = Pair(46 + k, k == 0 ? FEED_FG_Q_RESET_GAP : 0u, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    Proof(o[0] == 93 && o[1] == 95 && o[2] == 97, "three pairs whose dispatch FAILED before AMD's call (46, 47, 48; planted), each answered by the CPU: fgOut reads %llu, %llu, %llu: no frame, three times",
          (unsigned long long)o[0], (unsigned long long)o[1], (unsigned long long)o[2]);
    const uint64_t o49 = Pair(49, 0, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    Proof(prep && o49 == 99, "the pair after three failures (49: NOT PREPARED, frame generation is over for the set), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o49, XWord(XReads(49, o49)));
    int x = FgxRelease<FakeB>(&gSet, FEED_FG_BUILD_DEADLINE_MS);
    ok = x == FGX_RELEASED && SendF(50, A[0], A[1], A[2], true) && SendQ(50, 0) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 50, 5000)
      && SendG(50, FEED_FG_G_DISPATCH) && FeedVkWaitTimeline(&gVk, gSet.semOut, 100, 5000);
    const uint64_t o50 = Seen(gSet.semOut, &gSeenOut);
    Proof(ok && o50 == 101, "a pair after the set's RELEASE (50: the teardown ran, there is NO SET), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o50, XWord(XReads(50, o50)));
    x = Acquire((uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT, FEED_FG_BUILD_DEADLINE_MS, FEED_FG_BUILD_NO_HUDLESS);
    const uint64_t o51 = x == FGX_CREATED ? Pair(51, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep) : 0;
    Proof(x == FGX_CREATED && o51 == 103, "the pair whose dispatch RAISES inside AMD's library (51; the raise lever), answered by the CPU: fgOut reads %llu: %s", (unsigned long long)o51, XWord(XReads(51, o51)));
    const uint64_t o52 = Pair(52, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    Proof(prep && o52 == 105, "the pair after the raise (52: the latch, nothing of AMD's is called), answered by the CPU: fgOut reads %llu: %s; and fgPrep still comes", (unsigned long long)o52, XWord(XReads(52, o52)));
    ok = SendF(53, B[0], B[1], B[2], true) && FeedVkWaitTimeline(&gVk, gSemOut, 53, 5000);
    const int c = ok ? OutputAt(gIm[FEED_OUTPUT], LX, CY, B[0], B[1], B[2], nullptr) : -1;
    Proof(c == 4096, "and the upscaler's frames go on, by pixels: frame 53's constant is in Output (%d of 4096)", c);
    Proof(!gFenceBack && gFenceReads >= 20, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}

// =====================================================================================================================
// helper-wire-18, case 2: an older answer is still queued when the CPU has one to make
// =====================================================================================================================
static void FixCpuOrder()
{
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f }, B[3] = { 0.75f, 0.25f, 0.5f };
    bool prep = false; uint16_t first[4] = {};
    if (!XSet()) return;
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    if (!Proof(prep && o33 == 66, "pair 33: fgOut reads %llu: a frame", (unsigned long long)o33)) return;
    XSeedGen();
    // pair 34: its dispatch is submitted and stands behind fgIn, which this side withholds: its 2n is QUEUED
    bool ok = SendF(34, A[0], A[1], A[2], true) && SendQ(34, 0) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 34, 5000)
           && Copies(A[0], A[1], A[2], false, 34) && SendG(34, FEED_FG_G_DISPATCH);
    // pair 35, as the game writes it while 34 is unanswered: NO SOURCE. The queue "refuses" its answer (the helper's lever)
    ok = ok && SendF(35, A[0], A[1], A[2], true) && SendQ(35, 0) && SendG(35, FEED_FG_G_NOSOURCE);
    Sleep(500);
    const uint64_t held = Seen(gSet.semOut, &gSeenOut);
    Proof(ok && held == 66, "WITH PAIR 34'S ANSWER STILL QUEUED behind its held dispatch, the answer of pair 35, which the queue refused, was NOT signalled from the CPU: half a second later fgOut reads %llu (66 wanted: a CPU signal of 71 here is what an older list's queued 68 would later move BACK)",
          (unsigned long long)held);
    // fgIn is signalled: pair 34's dispatch runs and its own queued answer lands. Frame 35's answer stands behind it.
    ok = ok && SignalNow(gSet.semIn, 34) && FeedVkWaitTimeline(&gVk, gSemOut, 35, 5000) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 35, 5000);
    const uint64_t after = Seen(gSet.semOut, &gSeenOut);
    const int g34 = ok ? CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first) : -1;
    Proof(ok && after == 68 && g34 == 4096 && !gFenceBack,
          "once fgIn is signalled pair 34's own answer lands: fgOut reads %llu (%s for pair 34, and GEN is its constant on %d of 4096; for pair 35: %s), and fgOut was %s",
          (unsigned long long)after, XWord(XReads(34, after)), g34, XWord(XReads(35, after)), gFenceBack ? "READ LOWER THAN BEFORE: the fence moved back" : "never read lower than before");
    const uint64_t o36 = Pair(36, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    XSeedGen();
    const uint64_t o37 = Pair(37, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    const int g37 = CountAt(SetImg(FEED_FG_GEN), CX, CY, B[0], B[1], B[2], first);
    Proof(prep && o36 == 72 && o37 == 74 && g37 == 4096, "and the wire goes on: pairs 36 and 37 have their frames (fgOut %llu, %llu), GEN the new constant on %d of 4096", (unsigned long long)o36, (unsigned long long)o37, g37);
    Proof(!gFenceBack && gFenceReads >= 6, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}

// =====================================================================================================================
// helper-wire-18, case 3: the device removed for real, behind a dispatch's submit and before its answer
// =====================================================================================================================
static void FixCpuRemoved()
{
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    bool prep = false;
    if (!XSet()) return;
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    if (!Proof(prep && o33 == 66, "pair 33, before the removal: fgOut reads %llu: a frame", (unsigned long long)o33)) return;
    XSeedGen();
    gExitWant = 3;                                  // from here the helper ends by itself, as on any removed device
    bool ok = SendF(34, A[0], A[1], A[2], true) && SendQ(34, 0) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 34, 5000)
           && Copies(A[0], A[1], A[2], true, 34) && SendG(34, FEED_FG_G_DISPATCH);
    // every value fgOut reads for a second and a half, in the order they came
    uint64_t seen[8]; int ns = 0; bool frame = false; uint64_t last = 0;
    const double t0 = XNowMs();
    while (XNowMs() - t0 < 1500.0)
    {
        last = FeedVkTimelineValue(&gVk, gSet.semOut);
        if (last == 68) frame = true;
        if ((ns == 0 || seen[ns - 1] != last) && ns < 8) seen[ns++] = last;
        Sleep(1);
    }
    char list[256] = ""; int at = 0;
    for (int i = 0; i < ns; i++) at += sprintf_s(list + at, sizeof(list) - at, "%s0x%llX", i ? ", " : "", (unsigned long long)seen[i]);
    const int seed = CountAt(SetImg(FEED_FG_GEN), CX, CY, -1.0f, -1.0f, -1.0f, nullptr), cst = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Say("  fix-removed values=%s last=0x%llX reads=%s gen_seed=%d gen_constant=%d", list, (unsigned long long)last, last == 68 ? "frame" : last == 69 ? "none" : "neither", seed, cst);
    Proof(ok && !frame, "THE DEVICE REMOVED behind the submit of pair 34's dispatch, before its answer: for 1.5 s fgOut NEVER READ 68, \"a frame\" for pair 34 (it read %s; the last, by the game's reading: %s). GEN holds the seed on %d of 4096 pixels and the pair's constant on %d",
          list, XWord(XReads(34, last)), seed, cst);
    // the helper asked its device behind that submit and ended the session itself: no frame is served from a device that runs nothing
    const bool wrote = SendF(35, A[0], A[1], A[2], true);
    const bool ended = WaitForSingleObject(gHelper, 5000) == WAIT_OBJECT_0;
    Proof(ended, "THE HELPER ENDED THE SESSION ITSELF at that 'G': it is gone within 5 s (frame 35's message %s)", wrote ? "was still taken by the pipe" : "found the pipe closed");
}

// =====================================================================================================================
// helper-wire-19, case 1: the real hold on a ring of three, and the serve thread's own time
// =====================================================================================================================
static void FixRingReal()
{
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f }, B[3] = { 0.75f, 0.25f, 0.5f };
    bool prep = false; uint16_t first[4] = {};
    if (!XSet()) return;
    const uint64_t o33 = Pair(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_DISPATCH, A[0], A[1], A[2], true, &prep);
    if (!Proof(prep && o33 == 66, "pair 33: fgOut reads %llu: a frame", (unsigned long long)o33)) return;
    XSeedGen();
    // pair 34: its dispatch is submitted and HELD behind fgIn. Its list has really not retired, and the whole queue stands behind it
    bool ok = SendF(34, A[0], A[1], A[2], true) && SendQ(34, 0) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 34, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 34, 5000)
           && Copies(A[0], A[1], A[2], false, 34) && SendG(34, FEED_FG_G_DISPATCH);
    // THREE more pairs behind it. Pair 35 is a dispatch too, as under the game's development key (its copies go WITHOUT
    // their signal: a fence only rises, so fgIn = 35 would release pair 34). Pairs 36 and 37 are written as the game
    // writes them while a pair is unanswered (NO SOURCE). Behind each 'Q' a 'g' the helper must refuse, its ack timed:
    // the ack comes over the pipe, so the time is the serve thread's own. On a ring of three the lists so far are the
    // prepare and the held dispatch of 34 and the prepare and the dispatch of 35: the prepares of 36 and 37 need the
    // slot of 34's dispatch, which has not retired
    double a[3] = { -1.0, -1.0, -1.0 };
    const double t0 = XNowMs();
    for (int k = 0; k < 3 && ok; k++)
    {
        const uint64_t n = 35 + (uint64_t)k;
        ok = SendF(n, A[0], A[1], A[2], true) && SendQ(n, 0);
        a[k] = ok ? SRefusedG() : -1.0;
        ok = ok && a[k] >= 0.0;
        if (k == 0) ok = ok && Copies(A[0], A[1], A[2], false, n);
        ok = ok && SendG(n, k == 0 ? FEED_FG_G_DISPATCH : FEED_FG_G_NOSOURCE);
    }
    const double all = XNowMs() - t0;
    Sleep(200);
    const uint64_t hOut = Seen(gSet.semOut, &gSeenOut), hPrep = Seen(gSet.semPrep, &gSeenPrep), hUp = FeedVkTimelineValue(&gVk, gSemOut);
    Say("  fix-ring-real ack_ms=%.1f,%.1f,%.1f all_ms=%.1f held_out=%llu held_prep=%llu held_up=%llu", a[0], a[1], a[2], all, (unsigned long long)hOut, (unsigned long long)hPrep, (unsigned long long)hUp);
    Proof(ok && hOut == 66 && hPrep == 34 && hUp == 34, "THE HOLD IS REAL: with pair 34's fgIn withheld nothing on the helper's one queue answers: fgOut reads %llu, fgPrep %llu and the frame's out-fence %llu, with three more pairs written (66, 34 and 34 wanted)",
          (unsigned long long)hOut, (unsigned long long)hPrep, (unsigned long long)hUp);
    Proof(ok && a[0] >= 0.0 && a[0] < 250.0 && a[1] >= 0.0 && a[1] < 250.0 && a[2] >= 0.0 && a[2] < 250.0 && all < 1000.0,
          "THE SERVE THREAD DID NOT WAIT FOR THE RING: a message the helper answers over the pipe, written behind the 'Q' of each of the three pairs, was answered in %.1f, %.1f and %.1f ms (under 250 wanted each; the last two 'Q' needed the slot of the held dispatch), and the three pairs took %.0f ms in all (under 1000 wanted; the wait this replaces is 2000 a list)",
          a[0], a[1], a[2], all);
    // fgIn is signalled to 35: both held dispatches go, and everything behind them. Pairs 34 and 35 have their frames, 36 and 37 none
    ok = ok && SignalNow(gSet.semIn, 35) && FeedVkWaitTimeline(&gVk, gSet.semOut, 75, 5000) && FeedVkWaitTimeline(&gVk, gSet.semPrep, 37, 5000) && FeedVkWaitTimeline(&gVk, gSemOut, 37, 5000);
    const uint64_t rOut = Seen(gSet.semOut, &gSeenOut), rPrep = Seen(gSet.semPrep, &gSeenPrep);
    const int g35 = ok ? CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first) : -1;
    Proof(ok && rOut == 75 && rPrep == 37 && g35 == 4096, "once fgIn is signalled everything goes: the held dispatches wrote GEN (the constant on %d of 4096), pairs 36 and 37 are answered with no frame (fgOut reads %llu: %s for pair 37), and fgPrep reads %llu: it came for the two pairs the ring refused too",
          g35, (unsigned long long)rOut, XWord(XReads(37, rOut)), (unsigned long long)rPrep);
    const uint64_t o38 = Pair(38, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    XSeedGen();
    const uint64_t o39 = Pair(39, 0, FEED_FG_G_DISPATCH, B[0], B[1], B[2], true, &prep);
    const int g39 = CountAt(SetImg(FEED_FG_GEN), CX, CY, B[0], B[1], B[2], first);
    Proof(prep && o38 == 76 && o39 == 78 && g39 == 4096, "and the ring goes on: pair 38 (a reset of the helper's own) and pair 39 have their frames (fgOut %llu, %llu), GEN the new constant on %d of 4096", (unsigned long long)o38, (unsigned long long)o39, g39);
    Proof(!gFenceBack && gFenceReads >= 8, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}

// =====================================================================================================================
// helper-wire-19, case 2: every slot of the ring held by the helper's lever, the queue going on
// =====================================================================================================================
static void FixRingHeld()
{
    enum { FIRST = 33, LAST = 48, HOLD0 = 41, HOLD1 = 46, N = LAST - FIRST + 1 };   // pairs 41 to 46 get no list (the helper's lever: served lists 18 to 23)
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    uint16_t first[4] = {};
    if (!XSet()) return;
    double tF[N + 1] = {}, Lf[N + 1] = {}, Lq[N] = {}, La[N] = {}; uint64_t out[N] = {};
    bool ok = XSendF(FIRST, A, &tF[0]) && XWait(gSemOut, FIRST, 5000, tF[0], &Lf[0]);
    int done = 0;
    for (uint64_t n = FIRST; n <= LAST && ok; n++)
    {
        const int i = (int)(n - FIRST);
        if (n == LAST) XSeedGen();                  // the last pair's own dispatch must write GEN
        const double tQ = XNowMs();
        ok = SendQ(n, n == FIRST ? FEED_FG_Q_RESET_BUILD : 0u) && XWait(gSet.semPrep, n, 5000, tQ, &Lq[i]);
        ok = ok && Copies(A[0], A[1], A[2], true, n);
        const double tG = XNowMs();
        ok = ok && SendG(n, FEED_FG_G_DISPATCH);
        if (n < LAST) ok = ok && XSendF(n + 1, A, &tF[i + 1]);          // THE NEXT FRAME'S MESSAGE RIGHT BEHIND THIS PAIR'S 'G'
        ok = ok && XWait(gSet.semOut, 2 * n, 5000, tG, &La[i]);
        out[i] = Seen(gSet.semOut, &gSeenOut); Seen(gSet.semPrep, &gSeenPrep);
        if (n < LAST) ok = ok && XWait(gSemOut, n + 1, 5000, tF[i + 1], &Lf[i + 1]);
        if (ok) done++;
    }
    if (!Proof(ok && done == N, "sixteen pairs, 33 to 48, each written as the game writes it with the next frame's message right behind its 'G', were each answered (%d of %d)", done, (int)N)) return;
    bool values = true; char got[256] = ""; int at = 0;
    for (int i = 0; i < N; i++)
    {
        const uint64_t n = FIRST + (uint64_t)i; const bool held = n >= HOLD0 && n <= HOLD1;
        if (XReads(n, out[i]) != (held ? X_NONE : X_FRAME)) values = false;
        if (held) at += sprintf_s(got + at, sizeof(got) - at, "%s%llu", at ? ", " : "", (unsigned long long)out[i]);
    }
    double hA[8], hQ[8], hF[8], bF[16], bA[16]; int nh = 0, nb = 0;
    for (uint64_t n = HOLD0; n <= HOLD1; n++) { const int i = (int)(n - FIRST); hA[nh] = La[i]; hQ[nh] = Lq[i]; hF[nh] = Lf[i + 1]; nh++; }   // (the frame written behind that pair's 'G')
    for (uint64_t n = FIRST; n < HOLD0; n++)  { const int i = (int)(n - FIRST); bF[nb] = Lf[i + 1]; bA[nb] = La[i]; nb++; }
    double hAmed, hAmax, hQmed, hQmax, hFmed, hFmax, bFmed, bFmax, bAmed, bAmax;
    XMedMax(hA, nh, &hAmed, &hAmax); XMedMax(hQ, nh, &hQmed, &hQmax); XMedMax(hF, nh, &hFmed, &hFmax); XMedMax(bF, nb, &bFmed, &bFmax); XMedMax(bA, nb, &bAmed, &bAmax);
    double whole = 0.0; for (int k = 0; k < nh; k++) whole += hA[k] + hQ[k];
    Say("  fix-ring-held answer_ms med=%.2f max=%.2f prep_ms med=%.2f max=%.2f frame_ms med=%.2f max=%.2f usual_frame_ms med=%.2f max=%.2f usual_answer_ms med=%.2f max=%.2f waits_ms=%.1f",
        hAmed, hAmax, hQmed, hQmax, hFmed, hFmax, bFmed, bFmax, bAmed, bAmax, whole);
    Proof(values, "EVERY SLOT OF THE RING HELD for six served lists (the helper's lever; its queue goes on): pair 41's dispatch and the prepares of pairs 42 to 46 got no list, and each of those six pairs reads NO FRAME by equality (fgOut %s), while the eight pairs before them and the two after read A FRAME",
          got);
    Proof(hAmax < 250.0, "each of the six was answered AT ONCE: fgOut read its 2n+1 within %.2f ms of the pair's 'G' at most (median %.2f; under 250 wanted; the wait this replaces is 2000). A pair that dispatched took %.2f ms (median) to its 2n",
          hAmax, hAmed, bAmed);
    Proof(hQmax < 250.0, "and fgPrep came at once for each: within %.2f ms of the pair's 'Q' at most (median %.2f; under 250 wanted)", hQmax, hQmed);
    Proof(hFmax < 250.0 && hFmax <= bFmax + 50.0,
          "THE FRAME'S OWN ANSWER KEPT ITS LATENCY: the out-fence of the frame written right behind each of those pairs' 'G' came %.2f ms after its message at most (median %.2f), against %.2f ms at most (median %.2f) behind a pair that dispatched (not later than that by 50 ms, and under 250, wanted)",
          hFmax, hFmed, bFmax, bFmed);
    Proof(whole < 1000.0, "NOTHING WAITED 2 s: the six pairs' prepares and answers took %.1f ms together (under 1000 wanted)", whole);
    const int g48 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], first);
    Proof(out[LAST - 1 - FIRST] == 2 * (uint64_t)(LAST - 1) && out[LAST - FIRST] == 2 * (uint64_t)LAST && g48 == 4096,
          "and the ring goes on: six pairs in a row with no frame did not end frame generation for the set: pair 47 (a reset of the helper's own) and pair 48 have their frames (fgOut %llu, %llu), GEN the constant on %d of 4096",
          (unsigned long long)out[LAST - 1 - FIRST], (unsigned long long)out[LAST - FIRST], g48);
    Proof(!gFenceBack && gFenceReads >= 2 * N, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}

static void FixRun(int mode)
{
    if (mode == 12) { if (xCase == 1) FixCpuKinds(); else if (xCase == 2) FixCpuOrder(); else if (xCase == 3) FixCpuRemoved(); else Proof(false, "--fake-fix=%d is no case of --fake-fg=12 (1, 2 or 3)", xCase); }
    else            { if (xCase == 1) FixRingReal(); else if (xCase == 2) FixRingHeld(); else Proof(false, "--fake-fix=%d is no case of --fake-fg=13 (1 or 2)", xCase); }
}
