// THE TRUE FILL'S WIRE, THE FAKE CLIENT'S MODE --fake-fg=14 (DLSS-IV, 2026-10-18, frame generation's true fill, package G2;
// docs/reports/fg-5/brief-truefill/PLAN-truefill.md B.2 to B.4; source/transport/feed_ipc.h, "WIRE 3").
//
// WHAT IT PROVES, through the client's own lines (source/fg/fg_xport.h) against the BUILT helper:
//   a helper that OFFERS 2048 (this run speaks wire 3, as the game does):
//     W3a  wire 3's sizes as this program compiled them ('g' 29, 'G' 17, the ack 124, wire 2's ack 104)
//     W3b  a set asked at wire 3 WITHOUT the fourth image: two images (NO_HUDLESS), the third and fourth places all zero, what
//          counts the helper says the set makes; released in wire 3's layout
//     W3c  A SET OF FOUR IMAGES (FEED_FG_BUILD_GEN2): four imports, GEN and GEN2 with storage usage, the packed list of the set's
//          images; with --fake-fg-stamp the helper's FOUR constants each where the machine's order says
//     W3d  the answer at count 1: GEN is the constant, GEN2 is not written
//     W3e  the answer at count 2: where the set makes two, ONE answer 2n and BOTH images the constant; where it does not (the
//          stock arm, AMD's library, makes one), answered 2n+1 and neither image written: THE STOCK ARM'S FALLBACK
//     W3f  the answer at count 0, flow-only: 2n+1, no picture written; where the set makes it, the next pair is not a reset (the
//          helper's own count of resets, run.py's to read); where it does not, the same answer and the helper counts it
//     W3g  a count of 3, which no set makes: no frame
//     W3h  A 'g' OF WIRE 2 ON THIS HELPER, through the client's own lines (an old client's layout): wire 2's 104-byte ack, one
//          frame a pair, and a 'G' whose bits 2 and 3 say two is one frame in such a set
//     W3i  twenty acquire and release cycles of a four-image set, this side's handles flat (the helper's video memory is
//          run.py's to read)
//   a helper that does NOT offer 2048 (a helper of wire 2, or this run told --fake-wire=2):
//     W3o  this run speaks wire 2: the set is wire 2's, one frame a pair, FgxGenOk says no to 0 and 2, and a 'G' whose
//          bits 2 and 3 say two (this build writes them on every 'G') is one frame
// The helper's own counts of this run are run.py's to read (W3_COUNTS there, by the arm).
#pragma once

// 'G' with a COUNT, through the client's own line: the pair's 'Q' reasons as the product writes them (SendG's rule)
static bool SendGn(uint64_t n, uint32_t kind, uint32_t gen)
{
    const uint32_t why = (kind != FEED_FG_G_CLOSED && n == gLastQN) ? gLastQWhy : 0u;
    return FgxWriteG<FakeB>(n, (kind & FEED_FG_G_KIND_MASK) | ((why & FEED_FG_Q_RESET_MASK) << FEED_FG_G_RESET_SHIFT), 800.0f, gen);
}
// A whole DISPATCH pair with a count: 'F', 'Q', fgPrep, the copies with fgIn, 'G' with the count. What fgOut reads once it is
// at or past 2n (2n a frame, 2n + 1 none), 0 when it never got there in 5 s.
static uint64_t PairN(uint64_t n, uint32_t qflags, uint32_t gen, const float* c, bool* prepOk)
{
    bool ok = SendF(n, c[0], c[1], c[2], true) && SendQ(n, qflags);
    *prepOk = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) == n;
    ok = ok && Copies(c[0], c[1], c[2], true, n) && SendGn(n, FEED_FG_G_DISPATCH, gen);
    if (!ok || !FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000)) return 0;
    return Seen(gSet.semOut, &gSeenOut);
}
static bool W3Seed(int k)   // an image of the set seeded with a value no dispatch makes
{
    Begin(); Clear(SetImg(k), -1.0f, -1.0f, -1.0f, -1.0f);
    return Submit(VK_NULL_HANDLE, 0);
}
static int W3Seeded(int k) { return CountAt(SetImg(k), W / 2, H / 2, -1.0f, -1.0f, -1.0f, nullptr); }

static void Wire3Run(bool stamp)
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    bool prep = false, ok = false;
    Proof(sizeof(FeedFgBuildIvt) == 29 && sizeof(FeedFgGIvt) == 17 && sizeof(FeedFgAckIvt) == 124 && sizeof(FeedFgAck2Ivt) == 104 && sizeof(FeedFgQIvt) == 81,
          "W3a WIRE 3's sizes as this program compiled them: 'g' %u bytes (29), 'G' %u (17), 'Q' %u (81), the ack %u (124: four images and gen_counts), wire 2's ack %u (104)",
          (unsigned)sizeof(FeedFgBuildIvt), (unsigned)sizeof(FeedFgGIvt), (unsigned)sizeof(FeedFgQIvt), (unsigned)sizeof(FeedFgAckIvt), (unsigned)sizeof(FeedFgAck2Ivt));

    if (!gWireFlags)
    {   // ---- W3o: A HELPER OF WIRE 2 (or a run told to speak as a client of wire 2): wire 2, one frame a pair
        int x = Acquire(FP16, DL);
        const bool have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut;
        Proof(have && !gSet.wire3 && gSet.genCounts == FEED_FG_GENS_ONE && gSet.nAll == 3 && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE && gSet.nt[FEED_FG_GEN2] == nullptr,
              "W3o THIS RUN SPEAKS WIRE 2 (the helper offered no 2048, or the run was told so): the set is CREATED through the client's own lines in wire 2's layout (status %d), three images and no fourth, and it makes one frame a pair (counts 0x%X)",
              gSet.status, gSet.genCounts);
        if (!have) return;
        Proof(FgxGenOk(&gSet, FEED_FG_G_GEN_ONE) && !FgxGenOk(&gSet, FEED_FG_G_GEN_FLOW) && !FgxGenOk(&gSet, FEED_FG_G_GEN_TWO),
              "W3o and the client's own question says so: one may be asked of it, flow-only and two may not (FgxGenOk)");
        gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
        const uint64_t o33 = PairN(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_GEN_ONE, A, &prep);
        NEED(W3Seed(FEED_FG_GEN), "the seed of GEN before pair 34");
        const uint64_t o34 = PairN(34, 0, FEED_FG_G_GEN_ONE, A, &prep);
        const int g34 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
        Proof(prep && o33 == 66 && o34 == 68 && g34 == 4096, "W3o pairs 33 and 34 have their frames (fgOut %llu, %llu), GEN the constant (%d of 4096): this build writes the count's bits 2 and 3 on every 'G' (one), and a helper of wire 2 reads past them",
              (unsigned long long)o33, (unsigned long long)o34, g34);
        NEED(W3Seed(FEED_FG_GEN), "the seed of GEN before pair 35");
        const uint64_t o35 = PairN(35, 0, FEED_FG_G_GEN_TWO, A, &prep);
        const int g35 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
        Proof(o35 == 70 && g35 == 4096, "W3o A 'G' WHOSE BITS 2 AND 3 SAY TWO, in a set of wire 2, is ONE frame (fgOut %llu, GEN the constant on %d of 4096): the count is read only in a set of wire 3",
              (unsigned long long)o35, g35);
        Proof(!gFenceBack && gFenceReads >= 6, "W3o no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
        return;
    }

    // ---- W3b: a set of wire 3 with no HUD-less image and no fourth image
    int x = Acquire(FP16, DL, FEED_FG_BUILD_NO_HUDLESS);
    bool have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut;
    Proof(have && gSet.wire3 && gSet.nAll == 2 && gSet.all[0] == gSet.img[FEED_FG_PRESENT] && gSet.all[1] == gSet.img[FEED_FG_GEN]
          && gSet.img[FEED_FG_HUDLESS] == VK_NULL_HANDLE && gSet.nt[FEED_FG_HUDLESS] == nullptr && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE && gSet.nt[FEED_FG_GEN2] == nullptr
          && (gSet.genCounts & FEED_FG_GENS_ONE) && !(gSet.genCounts & FEED_FG_GENS_TWO),
          "W3b A SET OF WIRE 3 WITHOUT THE FOURTH IMAGE (NO_HUDLESS) is CREATED (status %d): PRESENT and GEN, the third and fourth places all zero and not imported, the packed list of two; it makes counts 0x%X (one always, two never without GEN2)",
          gSet.status, gSet.genCounts);
    Say("  wire 3: the helper says a set of this arm makes counts 0x%X (bit k is count k: 1 flow-only, 2 one, 4 two)", gSet.genCounts);
    x = FgxRelease<FakeB>(&gSet, DL);
    Proof(x == FGX_RELEASED && !gSet.have && gSet.nAll == 0 && gSet.genCounts == 0, "W3b and it is RELEASED in wire 3's layout (status %d), the packed list and the counts gone with it", gSet.status);

    // ---- W3c: A SET OF FOUR IMAGES
    x = Acquire(FP16, DL, FEED_FG_BUILD_GEN2);
    have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut;
    bool four = have && gSet.wire3 && gSet.nAll == 4;
    for (int k = 0; k < FEED_FG_IMAGES; k++) four = four && gSet.img[k] != VK_NULL_HANDLE && gSet.nt[k] != nullptr && gSet.fmt[k] == FP16 && gSet.all[k] == gSet.img[k];
    Proof(four && gSet.storage[FEED_FG_GEN] && gSet.storage[FEED_FG_GEN2] && !gSet.storage[FEED_FG_PRESENT] && !gSet.storage[FEED_FG_HUDLESS],
          "W3c A SET OF FOUR IMAGES (FEED_FG_BUILD_GEN2) is CREATED through the client's own lines (status %d): PRESENT, GEN, HUDLESS and GEN2 imported in the presented format, GEN and GEN2 alone with storage usage, the packed list the four in the machine's order; counts 0x%X",
          gSet.status, gSet.genCounts);
    if (!four) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    const bool two = FgxGenOk(&gSet, FEED_FG_G_GEN_TWO), flow = FgxGenOk(&gSet, FEED_FG_G_GEN_FLOW);
    if (stamp)
    {   // which handle is which image, by pixels: the helper stamps the four it has
        const uint8_t tagS = 'S'; uint8_t done = 0;
        ok = PipeWrite(&tagS, 1) && PipeRead(&done, 1, 10000) && done == 1;
        const int c0 = ok ? CountAt(SetImg(FEED_FG_PRESENT), CX, CY, 0.125f, 0.25f, 0.5f, nullptr) : -1;
        const int c1 = ok ? CountAt(SetImg(FEED_FG_GEN),     CX, CY, 0.5f, 0.125f, 0.25f, nullptr) : -1;
        const int c2 = ok ? CountAt(SetImg(FEED_FG_HUDLESS), CX, CY, 0.25f, 0.5f, 0.125f, nullptr) : -1;
        const int c3 = ok ? CountAt(SetImg(FEED_FG_GEN2),    CX, CY, 0.5f, 0.25f, 0.125f, nullptr) : -1;
        const int cx = ok ? CountAt(SetImg(FEED_FG_GEN),     CX, CY, 0.5f, 0.25f, 0.125f, nullptr) : -1;
        Proof(c0 == 4096 && c1 == 4096 && c2 == 4096 && c3 == 4096 && cx == 0,
              "W3c THE FOUR CONSTANTS are where the machine's order says: PRESENT %d, GEN %d, HUDLESS %d, GEN2 %d of 4096, and GEN holds none of GEN2's (%d): the helper's 'S' stamps the four", c0, c1, c2, c3, cx);
    }

    // ---- W3d: count 1 writes GEN alone
    const uint64_t o33 = PairN(33, FEED_FG_Q_RESET_BUILD, FEED_FG_G_GEN_ONE, A, &prep);
    NEED(W3Seed(FEED_FG_GEN) && W3Seed(FEED_FG_GEN2), "the seeds of GEN and GEN2 before pair 34");
    const uint64_t o34 = PairN(34, 0, FEED_FG_G_GEN_ONE, A, &prep);
    const int g34 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr), s34 = W3Seeded(FEED_FG_GEN2);
    Proof(prep && o33 == 66 && o34 == 68 && g34 == 4096 && s34 == 4096, "W3d COUNT 1 (pairs 33 and 34, fgOut %llu, %llu): GEN is the constant (%d of 4096), GEN2 is NOT written (the seed on %d of 4096)",
          (unsigned long long)o33, (unsigned long long)o34, g34, s34);

    // ---- W3e: count 2
    NEED(W3Seed(FEED_FG_GEN) && W3Seed(FEED_FG_GEN2), "the seeds of GEN and GEN2 before pair 35");
    const uint64_t o35 = PairN(35, 0, FEED_FG_G_GEN_TWO, A, &prep);
    const int g35 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr), h35 = CountAt(SetImg(FEED_FG_GEN2), CX, CY, A[0], A[1], A[2], nullptr);
    const int sg35 = W3Seeded(FEED_FG_GEN), sh35 = W3Seeded(FEED_FG_GEN2);
    if (two)
        Proof(prep && o35 == 70 && g35 == 4096 && h35 == 4096, "W3e COUNT 2 (pair 35): ONE ANSWER, 2n (fgOut %llu), and BOTH images written: GEN (at 1/3) the constant on %d, GEN2 (at 2/3) on %d of 4096 (two pairs of one constant give it at any t)",
              (unsigned long long)o35, g35, h35);
    else
        Proof(prep && o35 == 71 && sg35 == 4096 && sh35 == 4096, "W3e COUNT 2 ON A SET THAT DOES NOT MAKE IT (counts 0x%X: THE STOCK ARM'S FALLBACK, AMD's library makes one): answered with no frame (fgOut %llu), neither image written (seeds on %d and %d of 4096); the helper counts it",
              gSet.genCounts, (unsigned long long)o35, sg35, sh35);

    // ---- W3f: count 0, flow-only
    NEED(W3Seed(FEED_FG_GEN), "the seed of GEN before pair 36");
    const uint64_t o36 = PairN(36, 0, FEED_FG_G_GEN_FLOW, A, &prep);
    const int s36 = W3Seeded(FEED_FG_GEN);
    const uint64_t o37 = PairN(37, 0, FEED_FG_G_GEN_ONE, A, &prep);
    const int g37 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o36 == 73 && s36 == 4096 && o37 == 74 && g37 == 4096,
          flow ? "W3f COUNT 0, FLOW-ONLY (pair 36): answered with no frame (fgOut %llu: it wrote no picture), GEN not written (the seed on %d of 4096); the pair after it has its frame (%llu, GEN the constant on %d) and is not a reset of the helper's own (run.py reads that count)"
               : "W3f COUNT 0 ON A SET THAT DOES NOT MAKE IT (pair 36): answered with no frame (fgOut %llu), GEN not written (the seed on %d of 4096); the pair after it has its frame (%llu, GEN the constant on %d), the helper's own reset after a pair with none",
          (unsigned long long)o36, s36, (unsigned long long)o37, g37);

    // ---- W3g: a count no set makes
    NEED(W3Seed(FEED_FG_GEN) && W3Seed(FEED_FG_GEN2), "the seeds of GEN and GEN2 before pair 38");
    const uint64_t o38 = PairN(38, 0, 3u, A, &prep);
    const int sg38 = W3Seeded(FEED_FG_GEN), sh38 = W3Seeded(FEED_FG_GEN2);
    Proof(prep && o38 == 77 && sg38 == 4096 && sh38 == 4096, "W3g A COUNT OF 3, which no set makes (pair 38): answered with no frame (fgOut %llu), nothing written (seeds on %d and %d of 4096)",
          (unsigned long long)o38, sg38, sh38);

    // ---- W3h: a 'g' OF WIRE 2 on this helper, as an old client asks it
    x = FgxRelease<FakeB>(&gSet, DL);
    NEED(x == FGX_RELEASED, "the four-image set's release");
    const uint32_t keep = gWireFlags; gWireFlags = 0;
    x = Acquire(FP16, DL);
    gWireFlags = keep;
    have = x == FGX_CREATED && gSet.have;
    Proof(have && !gSet.wire3 && gSet.genCounts == FEED_FG_GENS_ONE && gSet.nAll == 3 && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE,
          "W3h A 'g' OF WIRE 2 ON A HELPER OF WIRE 3 (an old client's layout, through the client's own lines): CREATED (status %d) with wire 2's 104-byte ack, three images, one frame a pair (counts 0x%X)", gSet.status, gSet.genCounts);
    if (!have) return;
    NEED(W3Seed(FEED_FG_GEN), "the seed of GEN before pair 39");
    const uint64_t o39 = PairN(39, FEED_FG_Q_RESET_BUILD, FEED_FG_G_GEN_TWO, A, &prep);
    const int g39 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o39 == 78 && g39 == 4096, "W3h and its pair 39, whose 'G' says TWO in bits 2 and 3, is ONE frame (fgOut %llu, GEN the constant on %d of 4096): the helper reads the count only in a set of wire 3",
          (unsigned long long)o39, g39);
    x = FgxRelease<FakeB>(&gSet, DL);
    Proof(x == FGX_RELEASED && !gSet.have, "W3h and that set is RELEASED in wire 2's layout (status %d)", gSet.status);

    // ---- W3i: twenty cycles of a four-image set
    DWORD h2 = 0, h20 = 0; int made = 0, freed = 0, fours = 0;
    for (int k = 1; k <= 20; k++)
    {
        if (Acquire(FP16, DL, FEED_FG_BUILD_GEN2) == FGX_CREATED) { made++; if (gSet.nAll == 4 && gSet.img[FEED_FG_GEN2] != VK_NULL_HANDLE) fours++; }
        if (FgxRelease<FakeB>(&gSet, DL) == FGX_RELEASED) freed++;
        if (k == 2)  GetProcessHandleCount(GetCurrentProcess(), &h2);
        if (k == 20) GetProcessHandleCount(GetCurrentProcess(), &h20);
    }
    Proof(made == 20 && freed == 20 && fours == 20 && h20 <= h2 + 2, "W3i TWENTY ACQUIRE AND RELEASE CYCLES of a four-image set: %d created and imported (%d with four images), %d released; this process's handles %lu after the second, %lu after the twentieth",
          made, fours, freed, (unsigned long)h2, (unsigned long)h20);
    x = Acquire(FP16, DL, FEED_FG_BUILD_GEN2);
    Proof(x == FGX_CREATED && gSet.nAll == 4, "W3i a last four-image set is held as the pipe closes (status %d): the helper must end by itself", gSet.status);
    Proof(!gFenceBack && gFenceReads > 12, "W3 no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}
