// THE SECOND INPUT SET, THE FAKE CLIENT'S MODE --fake-fg=15 (DLSS-IV, 2026-10-20, frame generation round 5, H; source/transport/feed_ipc.h,
// "THE SECOND INPUT SET").
//
// WHAT IT PROVES, through the client's own lines (source/fg/fg_xport.h) against the BUILT helper, which must offer 4096:
//   W4a  the sizes as this program compiled them: the ack of a 'g' with FEED_FG_BUILD_TWIN 220 bytes (wire 3's 124 and six images), 'Q' 81,
//        'G' 17, 'g' 29
//   W4b  A SET WITH THE SECOND INPUT SET (and HUDLESS): PRESENT, GEN, HUDLESS, PRESENT2 and HUDLESS2 imported in the presented format, GEN
//        alone with storage usage, GEN2's place all zero, the packed list the five in the machine's order; with --fake-fg-stamp the
//        helper's five constants each where the places say
//   W4c  PAIRS BY TURNS: pair 33 in the first set (PRESENT the constant A), pair 34 in the SECOND (PRESENT2 A, while PRESENT is seeded
//        with B): both have their frames, and GEN is A: pair 34's dispatch read PRESENT2
//   W4d  THE CHECK'S CONTROL, FAILS FIRST ON THE SAME PIXELS: pair 35 written exactly as 34 (PRESENT2 A, PRESENT B) but named in the FIRST
//        set: it has its frame, and GEN is NOT A (the dispatch read PRESENT, B): the read of W4c can tell the two sets apart
//   W4e  THE SETS NEVER MIX: pair 36, its 'Q' in the second set and its 'G' in the first: answered with no frame (fgOut 2n + 1), GEN
//        not written (its seed stands); the pair after it (37, the first set) has its frame, a reset of the helper's own
//   W4f  A SET WITHOUT THE SECOND (wire 3's, asked without FEED_FG_BUILD_TWIN): a 'Q' naming the second set there is not prepared,
//        and its pair is answered with no frame (38); the pair after it (39) has its frame
//   W4g  A SET WITH THE SECOND AND NO HUD-LESS IMAGE (NO_HUDLESS): PRESENT, GEN and PRESENT2; HUDLESS and HUDLESS2 all zero, not imported
//   W4h  twenty acquire and release cycles of a set with the second, released in the 220-byte layout, this side's handles flat
//   (A1, 2026-10-24) every set with the second carries the two sets' guides (GDEPTH0, GMV0, GDEPTH1, GMV1) at the render size; the pairs here
//        write their set's guides in the same submit as PRESENT, as the game's hand copies the shared ones, and the helper's prepare,
//        now in the dispatch's list behind fgIn, reads them
// The helper's own counts of this run are run.py's to read (TW_COUNTS and TW_SET there).
#pragma once

// THE GAME'S COPIES FOR A PAIR IN SET s: "the back buffer" into that set's PRESENT and, where the set has it, its HUD-less image;
// fgIn = n in the one submit
// (A1, 2026-10-24: AND THE SET'S OWN GUIDES, which the helper's prepare of that pair reads: here a flat depth and still vectors, as the
// upscaler's shared two of a still picture; the game's hand copies the shared two in the same submit)
static Img GuideImg(int k) { Img i; i.img = gSet.img[k]; i.mem = gSet.mem[k]; i.w = gSet.gw; i.h = gSet.gh; return i; }
static bool CopiesIn(int s, float r, float g, float b, uint64_t n)
{
    Begin(); Clear(SetImg(s ? FEED_FG_PRESENT2 : FEED_FG_PRESENT), r, g, b, 1.0f);
    const int hk = s ? FEED_FG_HUDLESS2 : FEED_FG_HUDLESS;
    if (gSet.img[hk] != VK_NULL_HANDLE) Clear(SetImg(hk), r, g, b, 1.0f);
    if (gSet.img[FEED_FG_GDEPTH(s)] != VK_NULL_HANDLE) { Clear(GuideImg(FEED_FG_GDEPTH(s)), 0.5f, 0.0f, 0.0f, 0.0f); Clear(GuideImg(FEED_FG_GMV(s)), 0.0f, 0.0f, 0.0f, 0.0f); }
    return Submit(gSet.semIn, n);
}
// A WHOLE DISPATCH PAIR with its 'Q' in set qs and its 'G' in set gs (the same for every pair the game writes; a control's plant where
// they differ), its copies into set cs. What fgOut reads once it is at or past 2n, 0 when it never got there in 5 s.
static uint64_t PairSet(uint64_t n, uint32_t qflags, int qs, int gs, int cs, const float* c, bool* prepOk)
{
    bool ok = SendF(n, c[0], c[1], c[2], true) && SendQ(n, qflags | (qs ? FEED_FG_Q_SET1 : 0u));
    *prepOk = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) == n;
    const uint32_t why = n == gLastQN ? gLastQWhy : 0u;
    ok = ok && CopiesIn(cs, c[0], c[1], c[2], n)
            && FgxWriteG<FakeB>(n, FEED_FG_G_DISPATCH | ((why & FEED_FG_Q_RESET_MASK) << FEED_FG_G_RESET_SHIFT) | (gs ? FEED_FG_G_SET1 : 0u), 800.0f);
    if (!ok || !FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000)) return 0;
    return Seen(gSet.semOut, &gSeenOut);
}
static bool W4Seed(int k, float v) { Begin(); Clear(SetImg(k), v, v, v, 1.0f); return Submit(VK_NULL_HANDLE, 0); }
// (A1, 2026-10-24) THE GUIDES' COPY, ITS COST AT A DESK (W4i; told, not judged): the hand's copy of a pair's two guides, a depth R32_FLOAT and
// a vector R16G16_FLOAT image of w x h, image to image in GENERAL as the product records it (fg_arm.h, RecCopyGuides), bracketed by two
// timestamps in one buffer, alone on the card, 64 times; the median and the largest in microseconds. The acquire and release round it are
// the set's other copies' own and not timed here. Images of this program's own (made, timed, freed here).
static void W4GuideCost(UINT w, UINT h, double* med, double* most)
{
    *med = *most = -1.0;
    PFN_vkCreateQueryPool cqp = (PFN_vkCreateQueryPool)vkGetDeviceProcAddr_(gDev, "vkCreateQueryPool");
    PFN_vkCmdResetQueryPool crq = (PFN_vkCmdResetQueryPool)vkGetDeviceProcAddr_(gDev, "vkCmdResetQueryPool");
    PFN_vkCmdWriteTimestamp cwt = (PFN_vkCmdWriteTimestamp)vkGetDeviceProcAddr_(gDev, "vkCmdWriteTimestamp");
    PFN_vkGetQueryPoolResults gqr = (PFN_vkGetQueryPoolResults)vkGetDeviceProcAddr_(gDev, "vkGetQueryPoolResults");
    PFN_vkDestroyQueryPool dqp = (PFN_vkDestroyQueryPool)vkGetDeviceProcAddr_(gDev, "vkDestroyQueryPool");
    if (!cqp || !crq || !cwt || !gqr || !dqp) return;
    VkPhysicalDeviceProperties2 p2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }; vkGetPhysicalDeviceProperties2_(gPhys, &p2);
    const double ns = p2.properties.limits.timestampPeriod;
    const VkImageUsageFlags u = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    Img im[4] = { CMakeImage(w, h, u, VK_FORMAT_R32_SFLOAT), CMakeImage(w, h, u, VK_FORMAT_R32_SFLOAT), CMakeImage(w, h, u, VK_FORMAT_R16G16_SFLOAT), CMakeImage(w, h, u, VK_FORMAT_R16G16_SFLOAT) };
    VkQueryPool pool = VK_NULL_HANDLE;
    VkQueryPoolCreateInfo qpi = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO }; qpi.queryType = VK_QUERY_TYPE_TIMESTAMP; qpi.queryCount = 2;
    if (cqp(gDev, &qpi, nullptr, &pool) == VK_SUCCESS)
    {
        VkImage all[4] = { im[0].img, im[1].img, im[2].img, im[3].img };
        Begin(); FeedVkBarrierN(&gVk, gCb, all, 4, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); Submit(VK_NULL_HANDLE, 0);
        double us[64]; int nu = 0;
        for (int r = 0; r < 64; r++)
        {
            Begin(); crq(gCb, pool, 0, 2);
            cwt(gCb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, 0);
            FeedVkCopyImage(&gVk, gCb, im[0].img, VK_IMAGE_LAYOUT_GENERAL, im[1].img, VK_IMAGE_LAYOUT_GENERAL, w, h);
            FeedVkCopyImage(&gVk, gCb, im[2].img, VK_IMAGE_LAYOUT_GENERAL, im[3].img, VK_IMAGE_LAYOUT_GENERAL, w, h);
            cwt(gCb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, 1);
            if (!Submit(VK_NULL_HANDLE, 0)) break;
            uint64_t t[2] = {};
            if (gqr(gDev, pool, 0, 2, sizeof(t), t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS && t[1] > t[0]) us[nu++] = (double)(t[1] - t[0]) * ns / 1000.0;
        }
        for (int i = 1; i < nu; i++) { const double v = us[i]; int j = i; while (j > 0 && us[j - 1] > v) { us[j] = us[j - 1]; j--; } us[j] = v; }
        if (nu) { *med = us[nu / 2]; *most = us[nu - 1]; }
        dqp(gDev, pool, nullptr);
    }
    for (Img& i : im) { gVk.DestroyImage(gDev, i.img, nullptr); gVk.FreeMemory(gDev, i.mem, nullptr); }
}

static void Wire4Run(bool stamp, unsigned long long caps)
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    const UINT CX = W / 2, CY = H / 2;
    const float A[3] = { 0.25f, 0.5f, 0.75f };
    const float Bv = 0.125f;
    bool prep = false, ok = false;
    Proof(sizeof(FeedFgAck4Ivt) == 220 && sizeof(FeedFgAckIvt) == 124 && sizeof(FeedFgBuildIvt) == 29 && sizeof(FeedFgQIvt) == 81 && sizeof(FeedFgGIvt) == 17,
          "W4a THE SECOND INPUT SET'S SIZES as this program compiled them: its ack %u bytes (220: wire 3's %u and six images, the two sets' guides among them), 'g' %u (29), 'Q' %u (81), 'G' %u (17)",
          (unsigned)sizeof(FeedFgAck4Ivt), (unsigned)sizeof(FeedFgAckIvt), (unsigned)sizeof(FeedFgBuildIvt), (unsigned)sizeof(FeedFgQIvt), (unsigned)sizeof(FeedFgGIvt));
    const bool offered = (caps & FEED_CAP_FG) && (caps & FEED_CAP_FG_WIRE3) && (caps & FEED_CAP_FG_TWIN) && gWireFlags;
    Proof(offered, "W4a the helper OFFERS 4096 beside 1024 and 2048, and this run speaks wire 3 (caps 0x%llX)", caps);
    if (!offered) return;

    // ---- W4b: a set with the second input set and HUDLESS
    int x = Acquire(FP16, DL, FEED_FG_BUILD_TWIN);
    bool have = x == FGX_CREATED && gSet.have && gSet.semIn && gSet.semPrep && gSet.semOut;
    // (A1, 2026-10-24: AND THE TWO SETS' GUIDES, at the render size in the depth's and the vectors' formats: nine images in all)
    const int places[9] = { FEED_FG_PRESENT, FEED_FG_GEN, FEED_FG_HUDLESS, FEED_FG_PRESENT2, FEED_FG_HUDLESS2, FEED_FG_GDEPTH(0), FEED_FG_GMV(0), FEED_FG_GDEPTH(1), FEED_FG_GMV(1) };
    const uint32_t R32 = (uint32_t)DXGI_FORMAT_R32_FLOAT, RG16 = (uint32_t)DXGI_FORMAT_R16G16_FLOAT;
    bool five = have && gSet.twin && FgxTwin(&gSet) && gSet.nAll == 9 && gSet.img[FEED_FG_GEN2] == VK_NULL_HANDLE && gSet.nt[FEED_FG_GEN2] == nullptr && gSet.gw == W && gSet.gh == H;
    for (int i = 0; i < 9; i++)
    {
        const int k = places[i];
        const uint32_t want = FEED_FG_IS_GUIDE(k) ? (((k - FEED_FG_GDEPTH0) & 1) ? RG16 : R32) : FP16;
        five = five && gSet.img[k] != VK_NULL_HANDLE && gSet.nt[k] != nullptr && gSet.fmt[k] == want && gSet.all[i] == gSet.img[k] && !(FEED_FG_IS_GUIDE(k) && gSet.storage[k]);
    }
    Proof(five && gSet.storage[FEED_FG_GEN] && !gSet.storage[FEED_FG_PRESENT] && !gSet.storage[FEED_FG_HUDLESS] && !gSet.storage[FEED_FG_PRESENT2] && !gSet.storage[FEED_FG_HUDLESS2],
          "W4b A SET WITH THE SECOND INPUT SET (FEED_FG_BUILD_TWIN) is CREATED through the client's own lines (status %d): PRESENT, GEN, HUDLESS, PRESENT2 and HUDLESS2 imported in the presented format, and THE TWO SETS' GUIDES (A1) at the render size %ux%u, depth R32_FLOAT and vectors R16G16_FLOAT; GEN alone with storage usage, GEN2's place all zero, the packed list the nine in the machine's order",
          gSet.status, gSet.gw, gSet.gh);
    if (!five) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    if (stamp)
    {   // which handle is which image, by pixels: the helper stamps the five it has
        const uint8_t tagS = 'S'; uint8_t done = 0;
        ok = PipeWrite(&tagS, 1) && PipeRead(&done, 1, 10000) && done == 1;
        const int c0 = ok ? CountAt(SetImg(FEED_FG_PRESENT),  CX, CY, 0.125f, 0.25f, 0.5f, nullptr) : -1;
        const int c1 = ok ? CountAt(SetImg(FEED_FG_GEN),      CX, CY, 0.5f, 0.125f, 0.25f, nullptr) : -1;
        const int c2 = ok ? CountAt(SetImg(FEED_FG_HUDLESS),  CX, CY, 0.25f, 0.5f, 0.125f, nullptr) : -1;
        const int c4 = ok ? CountAt(SetImg(FEED_FG_PRESENT2), CX, CY, 0.25f, 0.125f, 0.5f, nullptr) : -1;
        const int c5 = ok ? CountAt(SetImg(FEED_FG_HUDLESS2), CX, CY, 0.125f, 0.5f, 0.25f, nullptr) : -1;
        const int cx = ok ? CountAt(SetImg(FEED_FG_PRESENT),  CX, CY, 0.25f, 0.125f, 0.5f, nullptr) : -1;
        Proof(c0 == 4096 && c1 == 4096 && c2 == 4096 && c4 == 4096 && c5 == 4096 && cx == 0,
              "W4b THE FIVE CONSTANTS are where the places say: PRESENT %d, GEN %d, HUDLESS %d, PRESENT2 %d, HUDLESS2 %d of 4096, and PRESENT holds none of PRESENT2's (%d): the helper's 'S' stamps the five", c0, c1, c2, c4, c5, cx);
    }

    // ---- W4c: pairs by turns; pair 34 in the second set reads PRESENT2
    const uint64_t o33 = PairSet(33, FEED_FG_Q_RESET_BUILD, 0, 0, 0, A, &prep);
    NEED(W4Seed(FEED_FG_PRESENT, Bv) && W4Seed(FEED_FG_HUDLESS, Bv), "the seeds of PRESENT and HUDLESS before pair 34");
    const uint64_t o34 = PairSet(34, 0, 1, 1, 1, A, &prep);
    const int g34 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o33 == 66 && o34 == 68 && g34 == 4096,
          "W4c PAIRS BY TURNS: pair 33 in the first set and pair 34 in the SECOND have their frames (fgOut %llu, %llu), and GEN is the constant A on %d of 4096 while PRESENT holds B: pair 34's dispatch read PRESENT2",
          (unsigned long long)o33, (unsigned long long)o34, g34);

    // ---- W4d: the control on the same pixels: written as 34, named in the first set
    NEED(W4Seed(FEED_FG_PRESENT, Bv) && W4Seed(FEED_FG_HUDLESS, Bv), "the seeds of PRESENT and HUDLESS before pair 35");
    const uint64_t o35 = PairSet(35, 0, 0, 0, 1, A, &prep);
    const int g35 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o35 == 70 && g35 < 4096,
          "W4d THE CONTROL, ON THE SAME PIXELS: pair 35 copied into the second set as 34 was but NAMED IN THE FIRST has its frame (fgOut %llu), and GEN is NOT the constant A (%d of 4096): the dispatch read PRESENT (B), so W4c's read tells the sets apart",
          (unsigned long long)o35, g35);

    // ---- W4e: the sets never mix
    NEED(W4Seed(FEED_FG_GEN, -1.0f), "the seed of GEN before pair 36");
    const uint64_t o36 = PairSet(36, 0, 1, 0, 1, A, &prep);
    const int s36 = CountAt(SetImg(FEED_FG_GEN), CX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    const uint64_t o37 = PairSet(37, 0, 0, 0, 0, A, &prep);
    const int g37 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o36 == 73 && s36 == 4096 && o37 == 74 && g37 == 4096,
          "W4e THE SETS NEVER MIX: pair 36, its 'Q' in the second set and its 'G' in the first, is answered with NO FRAME (fgOut %llu) and GEN is not written (its seed on %d of 4096); pair 37 after it has its frame (%llu, GEN the constant on %d), the helper's own reset (run.py reads that count)",
          (unsigned long long)o36, s36, (unsigned long long)o37, g37);
    x = FgxRelease<FakeB>(&gSet, DL);
    Proof(x == FGX_RELEASED && !gSet.have && gSet.nAll == 0, "W4e and the set is RELEASED in the 220-byte layout (status %d)", gSet.status);

    // ---- W4f: a set without the second, and a 'Q' that names it
    x = Acquire(FP16, DL);
    have = x == FGX_CREATED && gSet.have && !gSet.twin && !FgxTwin(&gSet) && gSet.img[FEED_FG_PRESENT2] == VK_NULL_HANDLE && gSet.nt[FEED_FG_PRESENT2] == nullptr && gSet.nAll == 3;
    Proof(have, "W4f A SET WITHOUT THE SECOND (wire 3's, no FEED_FG_BUILD_TWIN) is CREATED (status %d) with three images, PRESENT2 neither imported nor named", gSet.status);
    if (!have) return;
    NEED(W4Seed(FEED_FG_GEN, -1.0f), "the seed of GEN before pair 38");
    const uint64_t o38 = PairSet(38, FEED_FG_Q_RESET_BUILD, 1, 1, 0, A, &prep);
    const int s38 = CountAt(SetImg(FEED_FG_GEN), CX, CY, -1.0f, -1.0f, -1.0f, nullptr);
    const uint64_t o39 = PairSet(39, 0, 0, 0, 0, A, &prep);
    const int g39 = CountAt(SetImg(FEED_FG_GEN), CX, CY, A[0], A[1], A[2], nullptr);
    Proof(prep && o38 == 77 && s38 == 4096 && o39 == 78 && g39 == 4096,
          "W4f a pair whose 'Q' names the second set in a set WITHOUT it (38) is answered with NO FRAME (fgOut %llu), GEN not written (%d of 4096); the pair after it has its frame (%llu, GEN the constant on %d)",
          (unsigned long long)o38, s38, (unsigned long long)o39, g39);
    x = FgxRelease<FakeB>(&gSet, DL);
    NEED(x == FGX_RELEASED, "the wire 3 set's release");

    // ---- W4g: the second set with no HUD-less image
    x = Acquire(FP16, DL, FEED_FG_BUILD_TWIN | FEED_FG_BUILD_NO_HUDLESS);
    have = x == FGX_CREATED && gSet.have && FgxTwin(&gSet) && gSet.nAll == 7 && gSet.img[FEED_FG_HUDLESS] == VK_NULL_HANDLE && gSet.img[FEED_FG_HUDLESS2] == VK_NULL_HANDLE   // (A1: and the four guides)
        && gSet.nt[FEED_FG_HUDLESS2] == nullptr && gSet.all[2] == gSet.img[FEED_FG_PRESENT2];
    Proof(have, "W4g THE SECOND SET WITH NO HUD-LESS IMAGE (TWIN and NO_HUDLESS) is CREATED (status %d): PRESENT, GEN and PRESENT2, HUDLESS and HUDLESS2 all zero and not imported", gSet.status);
    x = FgxRelease<FakeB>(&gSet, DL);
    NEED(x == FGX_RELEASED, "the two-and-one set's release");

    // ---- W4h: twenty cycles
    DWORD h2 = 0, h20 = 0; int made = 0, freed = 0, fives = 0;
    for (int k = 1; k <= 20; k++)
    {
        if (Acquire(FP16, DL, FEED_FG_BUILD_TWIN) == FGX_CREATED) { made++; if (gSet.nAll == 9 && FgxTwin(&gSet)) fives++; }
        if (FgxRelease<FakeB>(&gSet, DL) == FGX_RELEASED) freed++;
        if (k == 2)  GetProcessHandleCount(GetCurrentProcess(), &h2);
        if (k == 20) GetProcessHandleCount(GetCurrentProcess(), &h20);
    }
    Proof(made == 20 && freed == 20 && fives == 20 && h20 <= h2 + 2, "W4h TWENTY ACQUIRE AND RELEASE CYCLES of a set with the second: %d created and imported (%d with nine images, the guides among them), %d released; this process's handles %lu after the second, %lu after the twentieth",
          made, fives, freed, (unsigned long)h2, (unsigned long)h20);
    x = Acquire(FP16, DL, FEED_FG_BUILD_TWIN);
    Proof(x == FGX_CREATED && gSet.nAll == 9, "W4h a last set with the second is held as the pipe closes (status %d): the helper must end by itself", gSet.status);
    Proof(!gFenceBack && gFenceReads > 12, "W4 no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
    // ---- W4i: the guides' copy, its cost (told)
    double m4 = 0, x4 = 0, m2 = 0, x2 = 0;
    W4GuideCost(3840, 2160, &m4, &x4);
    W4GuideCost(1920, 1080, &m2, &x2);
    Say("  W4i THE GUIDES' COPY a pair (A1: depth R32_FLOAT and vectors R16G16_FLOAT, image to image, alone on the card, 64 times): at a 3840x2160 render median %.1f us, most %.1f us; at 1920x1080 median %.1f us, most %.1f us",
        m4, x4, m2, x2);
}
