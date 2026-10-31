// =====================================================================================================================
// THE DESK MEASUREMENTS: --fake-fg=50 (2026-10-06, phase 3 of the frame generation revival, the build map's STAGE 1;
// docs/reports/fg-3/map-2026-10-06/critic.md, section 4, and helper-wire.md, H0).
// ALL OF THIS MODE IS IN THIS FILE. Its only lines in fgpipe.cpp are the include above FgRun, one line of FgRun and one
// line of main's argument loop. It stands on mode 20's moving picture and on the cost bench's images (fgpipe_cost.h).
//
// WHY. Frame generation runs in shadow: nothing AMD's library writes has reached a screen. Before a machine that SHOWS
// generated frames is built, what the library writes is read here, at a desk, against the real helper: the whole
// generated image (3840x2160, every pixel, read back through this side's imports), never a tile of it.
// One session is one ITEM of the stage's list (tools\test-fg-desk\run.py runs them and holds every judgement):
//     1   THE RESET PAIR AND THE PAIRS AFTER IT. The moving picture; every 17th pair an event; behind every dispatch the
//         whole of GEN against the four pictures this program can compute, by the dispatch's count since the reset.
//         Arms: CUT (the 'Q' is told a cut and the picture JUMPS: the map's rule 3, a cut written DISPATCH), HIT (a
//         reset in the middle of steady motion, no jump: a hitch), WH (today's kinds: the cut pair is WITHHELD with its
//         inputs made, and the helper resets the next dispatch itself), ZV (as CUT with every vector ZERO: what is left
//         is optical flow's own work, and it comes back dispatch by dispatch), ZL (as ZV with an event every 49th pair:
//         where that curve ends)
//     2   THE GUIDES SMALLER THAN THE DISPLAY. Steady pairs, the whole image by its quadrants, its four borders and a
//         16 by 9 grid; arm F (right vectors), F80 and F0 (the same pairs with the peak on 'G' at 80 and at 0: is the
//         image the same bit for bit) and arm FN (negated: the control). Run at both guide sizes, and once with the
//         picture at 14 times its values (--fake-desk-scale=14: highlights to 12.3 in motion)
//     3   HDR. Still pairs of five constants at four peaks on 'G', the whole of GEN compared BY BITS; alpha (what GEN
//         carries when PRESENT and HUDLESS carry other values); a still PICTURE with highlights up to 12.3 and a
//         negative blue; then the same in an 8-bit set. GEN is SEEDED before every pair that is read
//     4   THE HUD RECTANGLE. PRESENT carries a rectangle of a colour that changes every pair and six small plants
//         (the picture plus one code step, plus 0.004, 0.02, 0.05, 0.10, 0.25); HUDLESS is the picture without them
//         (arm OWN), the same as PRESENT (arm SAME), or the pair before's picture (arm STALE)
//     5   THE HELD ANSWER. The generated image of pair n hashed at its answer, 1.5 s later, behind the next frame,
//         behind the next 'Q', behind the next copies, behind the next 'G' with fgIn withheld, and behind fgIn. Then
//         the rule on the queue itself: a read submitted BEFORE the submit that signals fgIn, and one behind it
//     6   TWO DEPTH PLANES. A near object moves over a still far plane; the planes right, swapped, flat, the near plane
//         on 'Q' at three values, the object's vectors left out (the control), and the first arm again
//     7   THE HELPER KILLED with a pair pending (its dispatch held behind fgIn, or in flight): the five fences, a wait,
//         and the three images through this side's imports, at once and 3 s later; the product's order on PRESENT
//     8   K AT A DESK. 'G' written to the answer seen, pair after pair, each part waited for (order 0) or in the game's
//         order with nothing waited between a pair's parts (order 1)
// WHAT THIS PROGRAM JUDGES ITSELF is the instrument only: every pair answered as its kind says, every read made, the
// rows written. THE NUMBERS AND THE CONTROLS ARE run.py's: one row a reading goes to --fake-desk-out.
//
// WIRE 2 (2026-10-07, after the wire's one revision on the main line). This mode speaks it as the product does, through
// the client's own lines: 'Q' is 81 bytes (no jitter: these pictures are not jittered; no camera), and a pair's 'G' carries
// the reasons its 'Q' carried. EVERY ITEM RUNS ON A SET WITH THE HUD-LESS IMAGE, ASKED EXPLICITLY (the 'g' flag clear):
// items 3 to 8 read or write that image, and items 1 and 2 were measured with it. Items 1 and 2 can also run on THE
// GAME'S SHAPE since wire 2, a set asked WITHOUT it (FEED_FG_BUILD_NO_HUDLESS: two images, PRESENT alone is copied).
//
// ITS OPTIONS (every other argument goes to the helper):
//     --fake-desk-item=N       which item (1 to 8)
//     --fake-desk-hudless=N    1 a set WITH the HUD-less image (the default); 0 a set asked WITHOUT it, the game's shape
//                              (items 1 and 2 only)
//     --fake-desk-guides=N     0 the guides at the display's size (DLAA, the default); 1 at 2560x1440 (DLSS Quality)
//     --fake-desk-scale=N      the moving picture at N times its values (1; its largest value is 0.88)
//     --fake-desk-cycles=N     item 1: events an arm (6)
//     --fake-desk-pairs=N      item 8: timed pairs (2200); item 5: trials of the queue's order (36)
//     --fake-desk-order=N      item 8: 0 every part waited for, 1 the game's order
//     --fake-desk-poll=N       item 8: 1 the answer is seen by reading the fence in a loop (the default), 0 by a wait
//     --fake-desk-killus=N     item 7: 0 the dispatch is held behind fgIn when the helper is killed; N it is in flight,
//                              and the helper is killed N microseconds after 'G' was written
//     --fake-desk-out=PATH     the rows
// =====================================================================================================================
#pragma once

struct DOpt { int item = 0, guides = 0, cycles = 6, pairs = 0, order = 0, poll = 1, killUs = 0, scale = 1, hudless = 1; char out[MAX_PATH] = ""; };
static DOpt dop;
// One option of this mode, without its "--fake-desk-" (main hands it over; anything it does not know is said and ends the run).
static void DeskOption(const char* a)
{
    if      (strncmp(a, "item=", 5) == 0)   dop.item = atoi(a + 5);
    else if (strncmp(a, "guides=", 7) == 0) dop.guides = atoi(a + 7);
    else if (strncmp(a, "cycles=", 7) == 0) dop.cycles = atoi(a + 7);
    else if (strncmp(a, "scale=", 6) == 0)  dop.scale = atoi(a + 6);
    else if (strncmp(a, "hudless=", 8) == 0) dop.hudless = atoi(a + 8);
    else if (strncmp(a, "pairs=", 6) == 0)  dop.pairs = atoi(a + 6);
    else if (strncmp(a, "order=", 6) == 0)  dop.order = atoi(a + 6);
    else if (strncmp(a, "poll=", 5) == 0)   dop.poll = atoi(a + 5);
    else if (strncmp(a, "killus=", 7) == 0) dop.killUs = atoi(a + 7);
    else if (strncmp(a, "out=", 4) == 0)    strcpy_s(dop.out, a + 4);
    else { Say("  FAIL  --fake-desk-%s is not an option of the desk measurements", a); Say("FAIL"); ExitProcess(2); }
}

// ---------------------------------------------------------------- this mode's own state
static FILE* dOut;
static VkBuffer dRb; static void* dRbPtr;           // a whole image, read back (63.3 MiB at 3840x2160 FP16), and the staging for an upload
static VkBuffer dPatch; static void* dPatchPtr;     // item 4: the rectangles this side writes into PRESENT, made on the CPU
static VkBuffer dFillMv, dFillZ;                    // item 6: the object's vectors and its depth, filled on the card
static VkCommandBuffer dCb[8];                      // buffers for the submits nobody waits for (items 5 and 8)
static float dH2F[65536];                           // every half float as a float (infinities and NaNs as 0: they are counted, never summed)
static uint64_t dN = 33;                            // the next frame's number (the run of stage 4 used 1 to 32)
static long dS, dT, dQS, dQT, dPrevQS, dPrevQT;     // where the picture stands: render pixels; presented pixels; and at the last DISPATCHED pair
static int dStep, dRows;
static LONGLONG dTRead, dTStat; static int dCached;       // how long the reads and the measuring took (QPC ticks); the read-back's memory is cached
static const char* dWhy = "";
static const float D_FOV = 0.7853981f;
// THE PICTURE IN ORDINARY MEMORY, one channel of it (its three colours are one value, its alpha 1.0). The stage's own pixels
// (mode 20's TStage) live in the buffer they are uploaded from, which is write-combined: measured here, a whole image
// compared against them took 2.1 s, against a copy in ordinary memory a small part of that.
struct DPic { uint16_t* r = nullptr; UINT w = 0, h = 0, period = 0; };
static DPic dPicPlain, dPicWide;
static void DPicMake(DPic* d, const TStage& s)
{
    d->w = s.w; d->h = s.h; d->period = s.period;
    d->r = (uint16_t*)malloc((size_t)s.w * s.h * sizeof(uint16_t)); NEED(d->r, "memory for the picture's copy");
    for (size_t i = 0, n = (size_t)s.w * s.h; i < n; i++) d->r[i] = s.px[4 * i];
}
// THE PICTURE AT N TIMES ITS VALUES (highlights: 14 times takes its 0.88 to 12.3, a 985 nit white in scRGB). The copy first,
// then the stage's own pixels, which are only written here (never read: they are write-combined), before their upload.
// A frame is still the one before it moved by whole pixels, bit for bit.
static void DPicScale(DPic* d, const TStage& s, int scale)
{
    uint16_t* out = const_cast<uint16_t*>(s.px);
    for (size_t i = 0, n = (size_t)s.w * s.h; i < n; i++)
    {
        const uint16_t v = Half(dH2F[d->r[i]] * (float)scale);
        d->r[i] = v; out[4 * i] = v; out[4 * i + 1] = v; out[4 * i + 2] = v;
    }
}

static void DRow(const char* fmt, ...)
{
    if (!dOut) return;
    va_list a; va_start(a, fmt); vfprintf(dOut, fmt, a); va_end(a);
    fputc('\n', dOut); fflush(dOut); dRows++;
}
static void DTables()
{
    for (uint32_t v = 0; v < 65536u; v++)
    {
        const uint32_t e = (v >> 10) & 0x1Fu, m = v & 0x3FFu; float f;
        if (e == 31) f = 0.0f;
        else if (e == 0) f = (float)m * (1.0f / 16777216.0f);                  // a subnormal: m times two to the minus 24
        else { const uint32_t bits = ((e + 112u) << 23) | (m << 13); memcpy(&f, &bits, 4); }
        dH2F[v] = (v & 0x8000u) ? -f : f;
    }
}
// A buffer of this mode's: on the card, or mapped (cached memory first: a whole image is read out of it pixel by pixel).
static bool DBuffer(VkDeviceSize bytes, VkBufferUsageFlags usage, bool host, VkBuffer* buf, void** ptr)
{
    VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = bytes; bi.usage = usage;
    if (vkCreateBuffer_(gDev, &bi, nullptr, buf) != VK_SUCCESS) return false;
    VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, *buf, &q);
    uint32_t type = UINT32_MAX;
    for (int pass = 0; pass < 2 && type == UINT32_MAX; pass++)
    {
        VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (host) want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | (pass == 0 ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0);
        for (uint32_t i = 0; i < gMem.memoryTypeCount && type == UINT32_MAX; i++)
            if ((q.memoryTypeBits & (1u << i)) && (gMem.memoryTypes[i].propertyFlags & want) == want) type = i;
    }
    if (type == UINT32_MAX) return false;
    if (host && buf == &dRb) dCached = (gMem.memoryTypes[type].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? 1 : 0;
    VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size; mi.memoryTypeIndex = type;
    VkDeviceMemory m;
    if (gVk.AllocateMemory(gDev, &mi, nullptr, &m) != VK_SUCCESS || vkBindBufferMemory_(gDev, *buf, m, 0) != VK_SUCCESS) return false;
    return !host || vkMapMemory_(gDev, m, 0, bytes, 0, ptr) == VK_SUCCESS;
}
static void DTransferBarrier(VkCommandBuffer cb)    // between two transfers of one buffer where the second meets what the first wrote
{
    VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    gVk.CmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
}
static void DReadInto(VkCommandBuffer cb, VkImage img)
{
    FeedVkBarrierN(&gVk, cb, &img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg = {}; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { W, H, 1 };
    vkCmdCopyImageToBuffer_(cb, img, VK_IMAGE_LAYOUT_GENERAL, dRb, 1, &rg);
}
// A WHOLE IMAGE, READ BACK through this side's import, and waited for.
static bool DRead(VkImage img)
{
    const LONGLONG t0 = CQpc();
    Begin(); DReadInto(gCb, img);
    const bool ok = Submit(VK_NULL_HANDLE, 0);
    dTRead += CQpc() - t0;
    if (ok) return true;
    dWhy = "the read of a whole image was not submitted"; return false;
}
// A submit nobody waits for, in a buffer of its own: it may wait for a timeline's value on the queue and may signal one.
static void DBegin(int i)
{
    vkResetCommandBuffer_(dCb[i], 0);
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer_(dCb[i], &bi);
}
static bool DSubmit(int i, VkSemaphore wait, uint64_t waitVal, VkSemaphore sig, uint64_t sigVal)
{
    vkEndCommandBuffer_(dCb[i]);
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.pNext = &tl; si.commandBufferCount = 1; si.pCommandBuffers = &dCb[i];
    if (wait != VK_NULL_HANDLE) { tl.waitSemaphoreValueCount = 1; tl.pWaitSemaphoreValues = &waitVal; si.waitSemaphoreCount = 1; si.pWaitSemaphores = &wait; si.pWaitDstStageMask = &stage; }
    if (sig != VK_NULL_HANDLE) { tl.signalSemaphoreValueCount = 1; tl.pSignalSemaphoreValues = &sigVal; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &sig; }
    if (vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) == VK_SUCCESS) return true;
    dWhy = "the queue refused a submit of this side's"; return false;
}

// ---------------------------------------------------------------- a whole image's hash
// As the cost bench hashes (CHashGen): a pair of 32-bit runs over every channel, the same over the three colours alone,
// and one run for each block of a 16 by 9 grid (colours alone), so that two images that differ say where.
struct DHash { uint32_t all[2], col[2], blk[C_HGX * C_HGY]; };
static void DHashImage(DHash* hs, const void* p)
{
    uint32_t a1 = 0x811C9DC5u, a2 = 0x9E3779B9u, c1 = 0x811C9DC5u, c2 = 0x9E3779B9u;
    for (int k = 0; k < C_HGX * C_HGY; k++) hs->blk[k] = 0x811C9DC5u;
    const uint32_t* px = (const uint32_t*)p; const UINT bw = W / C_HGX, bh = H / C_HGY;
    for (UINT y = 0; y < H; y++)
    {
        const uint32_t* row = px + (size_t)y * W * 2u; uint32_t* brow = hs->blk + (size_t)(y / bh < C_HGY ? y / bh : C_HGY - 1) * C_HGX;
        for (UINT bx = 0; bx < C_HGX; bx++)
        {
            uint32_t hb = brow[bx]; const UINT x1 = bx == C_HGX - 1 ? W : (bx + 1) * bw;
            for (UINT x = bx * bw; x < x1; x++)
            {
                const uint32_t lo = row[2 * x], hi = row[2 * x + 1], col = hi & 0xFFFFu;   // red and green; blue and alpha; blue alone
                a1 = (a1 ^ lo) * 0x01000193u; a1 = (a1 ^ hi) * 0x01000193u;
                a2 = _rotl(a2 + lo, 5) * 0x85EBCA6Bu; a2 = _rotl(a2 + hi, 5) * 0x85EBCA6Bu;
                c1 = (c1 ^ lo) * 0x01000193u; c1 = (c1 ^ col) * 0x01000193u;
                c2 = _rotl(c2 + lo, 5) * 0x85EBCA6Bu; c2 = _rotl(c2 + col, 5) * 0x85EBCA6Bu;
                hb = (hb ^ lo) * 0x01000193u; hb = (hb ^ col) * 0x01000193u;
            }
            brow[bx] = hb;
        }
    }
    hs->all[0] = a1; hs->all[1] = a2; hs->col[0] = c1; hs->col[1] = c2;
}
static int DHashSame(const DHash& a, const DHash& b)      // how many of the 144 blocks are the same
{
    int n = 0;
    for (int k = 0; k < C_HGX * C_HGY; k++) if (a.blk[k] == b.blk[k]) n++;
    return n;
}
static bool DHashEq(const DHash& a, const DHash& b) { return a.all[0] == b.all[0] && a.all[1] == b.all[1] && a.col[0] == b.col[0] && a.col[1] == b.col[1]; }
static bool DHashColEq(const DHash& a, const DHash& b) { return a.col[0] == b.col[0] && a.col[1] == b.col[1]; }

// ---------------------------------------------------------------- wire 2: 'Q', 'G', and what the set in hand has
// 'Q' AND 'G' THROUGH THE CLIENT'S OWN LINES, AS THE PRODUCT WRITES THEM ON WIRE 2: the 'Q' with no jitter and no camera,
// and the pair's 'G' with the reasons its 'Q' carried in bits 4 to 7 (fgpipe.cpp's SendQx and SendGx do the same for the
// other modes; this mode's 'Q' has its own near plane and its 'G' its own peak).
static uint64_t dLastQN; static uint32_t dLastQWhy;
static bool DSendQ(uint64_t n, uint32_t qf, float nearPlane)
{
    dLastQN = n; dLastQWhy = qf & FEED_FG_Q_RESET_MASK;
    return FgxWriteQ<FakeB>(&gWaits, n, qf, 16.67f, nearPlane, D_FOV, 0.0f, 0.0f, nullptr);
}
static bool DSendG(uint64_t n, uint32_t kind, float peak)
{
    const uint32_t why = n == dLastQN ? dLastQWhy : 0u;
    return FgxWriteG<FakeB>(n, (kind & FEED_FG_G_KIND_MASK) | ((why & FEED_FG_Q_RESET_MASK) << FEED_FG_G_RESET_SHIFT), peak);
}
// The set in hand has the HUD-less image, or was asked without it (two images: PRESENT and GEN)
static bool DHasHudless() { return gSet.img[FEED_FG_HUDLESS] != VK_NULL_HANDLE; }
static uint32_t DSetImages() { return gSet.nAll; }   // (wire 3: the images the set holds, over FgxSet::all)

// ---------------------------------------------------------------- the parts of a pair, each waited for
static bool DFrameMsg(uint64_t n)
{
    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.frame_time_ms = 16.67f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = D_FOV;
    if (PipeWrite(&f, sizeof(f))) return true;
    dWhy = "the pipe refused a frame message"; return false;
}
// THE FRAME: this frame's picture into Color at the guides' size, one vector at every pixel, the in-fence in that submit.
static bool DFrame(uint64_t n, float vx, float vy)
{
    Begin();
    CPicture(gCb, cAtlas, tPlain.period, dS, dT, gIm[FEED_COLOR].img, cRw, cRh);
    Clear(gIm[FEED_MV], vx, vy, 0.0f, 0.0f);
    if (!Submit(gSemIn, n)) { dWhy = "the frame's submit was not taken"; return false; }
    if (!DFrameMsg(n)) return false;
    if (FeedVkWaitTimeline(&gVk, gSemOut, n, 5000)) return true;
    dWhy = "a frame was not answered in 5 s"; return false;
}
static bool DQ(uint64_t n, uint32_t qf, float nearPlane)
{
    if (!DSendQ(n, qf, nearPlane)) { dWhy = "the pipe refused a 'Q'"; return false; }
    if (FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) == n) return true;
    dWhy = "a 'Q' was not answered with fgPrep at its n in 5 s"; return false;
}
static bool DG(uint64_t n, uint32_t kind, float peak, uint64_t* out)
{
    if (!DSendG(n, kind, peak)) { dWhy = "the pipe refused a 'G'"; return false; }
    if (!FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000)) { dWhy = "a 'G' was not answered in 5 s"; return false; }
    *out = Seen(gSet.semOut, &gSeenOut);
    if (*out == (kind == FEED_FG_G_DISPATCH ? 2 * n : 2 * n + 1)) return true;
    dWhy = "fgOut does not read what this pair's 'G' asked for"; return false;
}
enum { DV_RIGHT, DV_ZERO, DV_NEG };
static const DPic& DPres() { return dop.guides ? dPicWide : dPicPlain; }
static void DPutPicture(VkCommandBuffer cb, VkImage dst, long qs, long qt) { CPicture(cb, dop.guides ? cAtlasWide : cAtlas, DPres().period, qs, qt, dst, W, H); }
// The picture's next move (the cost bench's six, whole pixels at both guide sizes), and a jump on top of it for a cut.
static void DStepPicture(bool still, long jumpS, long jumpT, int* move, float* vx, float* vy, int vec)
{
    const int dx = still ? 0 : kCMove[dStep % 6][0], dy = still ? 0 : kCMove[dStep % 6][1];
    *move = still ? -1 : dStep % 6;
    if (!still) dStep++;
    dS += dx + jumpS; dT += dy + jumpT;
    *vx = vec == DV_RIGHT ? (float)-dx : vec == DV_NEG ? (float)dx : 0.0f;       // the game's vectors: previous minus current
    *vy = vec == DV_RIGHT ? (float)-dy : vec == DV_NEG ? (float)dy : 0.0f;
    dQS = dS * (dop.guides ? 3 : 2) / 2; dQT = dT * (dop.guides ? 3 : 2) / 2;
}
// A WHOLE PAIR OF THE MOVING PICTURE, every part waited for: the frame, 'Q' and fgPrep, the picture into PRESENT and,
// where the set has the image, into HUDLESS, with fgIn (for the kinds whose inputs the game makes), 'G' and fgOut.
static bool DMovePair(uint64_t n, uint32_t qf, uint32_t kind, int vec, bool still, long jumpS, long jumpT, int* move, uint64_t* out, float peak = 1000.0f)
{
    float vx, vy;
    DStepPicture(still, jumpS, jumpT, move, &vx, &vy, vec);
    if (!DFrame(n, vx, vy) || !DQ(n, qf, 0.05f)) return false;
    if (kind == FEED_FG_G_DISPATCH || kind == FEED_FG_G_WITHHELD)
    {
        Begin(); DPutPicture(gCb, gSet.img[FEED_FG_PRESENT], dQS, dQT);
        if (DHasHudless()) DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dQS, dQT);
        if (!Submit(gSet.semIn, n)) { dWhy = "the copies' submit was not taken"; return false; }
    }
    return DG(n, kind, peak, out);
}

// ---------------------------------------------------------------- the whole generated image against the moving picture
// TObserve's four distances (mode 20), over EVERY pixel and by place: the image, its inside (64 pixels in from every
// border), its four borders (64 pixels deep), its four quadrants and a 16 by 9 grid. And by bits: how many pixels are
// exactly this frame's, the previous frame's or the half-way picture's; what alpha holds.
struct DZone { double mid = 0, cur = 0, prev = 0, mix = 0, change = 0; };
struct DStat
{
    DZone all, inner, band[4], quad[4];                 // band: top, bottom, left, right; quad: upper left, upper right, lower left, lower right
    double blkMid[C_HGX * C_HGY], blkChange[C_HGX * C_HGY];
    double maxMid; UINT maxX, maxY;
    uint32_t eqCur, eqPrev, eqMid, notGrey, nonFinite, over[4];
    uint32_t aOne, aZero, aOther, aIsRed, aIsCur, offEdge, offInside; uint16_t aSeen; double aMin, aMax, aSum;   // alpha: 1.0, 0, anything else; the generated red's bits; this frame's red's bits
                                                        // offEdge, offInside: pixels over 0.05 within 8 pixels of a border, and 64 and more from every border
    UINT x0, y0, x1, y1;                                // the box of the pixels that are NOT this frame's bit for bit
};
static void DZoneAdd(DZone* z, const double* v) { z->mid += v[0]; z->cur += v[1]; z->prev += v[2]; z->mix += v[3]; z->change += v[4]; }
static UINT DOff(long v, long P) { return (UINT)(((-v) % P + P) % P); }
static void DMoveStat(DStat* st, const uint16_t* px, const DPic& s, long cS_, long cT_, long pS_, long pT_)
{
    const LONGLONG t0 = CQpc();
    memset(st, 0, sizeof(*st)); st->x0 = W; st->y0 = H; st->aMin = 1e30; st->aMax = -1e30;
    const long P = (long)s.period; const double sc = (double)dop.scale;      // (what counts as far off is counted at the picture's own scale)
    const UINT cox = DOff(cS_, P), coy = DOff(cT_, P), pox = DOff(pS_, P), poy = DOff(pT_, P), mox = DOff((cS_ + pS_) / 2, P), moy = DOff((cT_ + pT_) / 2, P);
    // the row's segments: the sixteen columns of the grid, the first and the last cut at the 64-pixel borders
    UINT segA[18], segB[18]; int segBlk[18], segSide[18], ns = 0;
    const UINT bw = W / C_HGX, bh = H / C_HGY;
    for (UINT bx = 0; bx < C_HGX; bx++)
    {
        const UINT a = bx * bw, b = a + bw;
        if (bx == 0)               { segA[ns] = 0; segB[ns] = 64; segBlk[ns] = 0; segSide[ns++] = 2; segA[ns] = 64; segB[ns] = b; segBlk[ns] = 0; segSide[ns++] = 0; }
        else if (bx == C_HGX - 1)  { segA[ns] = a; segB[ns] = W - 64; segBlk[ns] = (int)bx; segSide[ns++] = 0; segA[ns] = W - 64; segB[ns] = W; segBlk[ns] = (int)bx; segSide[ns++] = 3; }
        else                       { segA[ns] = a; segB[ns] = b; segBlk[ns] = (int)bx; segSide[ns++] = 0; }
    }
    for (UINT y = 0; y < H; y++)
    {
        const uint16_t* g = px + (size_t)y * W * 4u;
        const uint16_t* c = s.r + ((size_t)(y + coy) * s.w + cox);
        const uint16_t* p = s.r + ((size_t)(y + poy) * s.w + pox);
        const uint16_t* m = s.r + ((size_t)(y + moy) * s.w + mox);
        const bool top = y < 64, bot = y >= H - 64; const UINT by = y / bh;
        for (int k = 0; k < ns; k++)
        {
            double v[5] = {};
            for (UINT x = segA[k]; x < segB[k]; x++)
            {
                const uint16_t gr = g[4 * x], cb = c[x], pb = p[x], mb = m[x], ga = g[4 * x + 3];
                const double gf = dH2F[gr], cf = dH2F[cb], pf = dH2F[pb], mf = dH2F[mb], dm = fabs(gf - mf);
                v[0] += dm; v[1] += fabs(gf - cf); v[2] += fabs(gf - pf); v[3] += fabs(gf - 0.5 * (cf + pf)); v[4] += fabs(cf - pf);
                if (dm > st->maxMid) { st->maxMid = dm; st->maxX = x; st->maxY = y; }
                if (dm > 0.002 * sc) { st->over[0]++; if (dm > 0.01 * sc) { st->over[1]++; if (dm > 0.05 * sc) { st->over[2]++; if (dm > 0.25 * sc) st->over[3]++; } } }
                if (dm > 0.05 * sc) { if (x < 8 || x >= W - 8 || y < 8 || y >= H - 8) st->offEdge++; else if (!top && !bot && !segSide[k]) st->offInside++; }
                const bool grey = g[4 * x + 1] == gr && g[4 * x + 2] == gr;
                if (!grey) st->notGrey++;
                if ((gr & 0x7C00u) == 0x7C00u) st->nonFinite++;
                if (grey && gr == cb) st->eqCur++;
                else { if (x < st->x0) st->x0 = x; if (x > st->x1) st->x1 = x; if (y < st->y0) st->y0 = y; if (y > st->y1) st->y1 = y; }
                if (grey && gr == pb) st->eqPrev++;
                if (grey && gr == mb) st->eqMid++;
                if (ga == 0x3C00u) st->aOne++; else if (ga == 0) st->aZero++; else { st->aOther++; st->aSeen = ga; }
                if (ga == gr) st->aIsRed++;
                if (ga == cb) st->aIsCur++;
                { const double af = dH2F[ga]; st->aSum += af; if (af < st->aMin) st->aMin = af; if (af > st->aMax) st->aMax = af; }
            }
            DZoneAdd(&st->all, v);
            st->blkMid[by * C_HGX + (UINT)segBlk[k]] += v[0]; st->blkChange[by * C_HGX + (UINT)segBlk[k]] += v[4];
            DZoneAdd(&st->quad[(y >= H / 2 ? 2 : 0) + (segA[k] >= W / 2 ? 1 : 0)], v);
            if (top) DZoneAdd(&st->band[0], v);
            if (bot) DZoneAdd(&st->band[1], v);
            if (segSide[k]) DZoneAdd(&st->band[segSide[k]], v);
            if (!top && !bot && !segSide[k]) DZoneAdd(&st->inner, v);
        }
    }
    dTStat += CQpc() - t0;
}
static double DRatio(double a, double b) { return b > 0.0 ? a / b : -1.0; }
// One row for a reading: `head` says which pair it was. Distances are parts of the change between the two presented
// pictures (0 is that picture exactly, 1 is as far as the two frames are from each other); "a..." are absolute means.
static void DStatRow(const char* head, const DStat& st)
{
    static char blk[C_HGX * C_HGY * 10 + 16]; int at = 0; double worst = -1.0; int wi = -1;
    for (int k = 0; k < C_HGX * C_HGY; k++)
    {
        const double r = DRatio(st.blkMid[k], st.blkChange[k]);
        if (r > worst) { worst = r; wi = k; }
        at += sprintf_s(blk + at, sizeof(blk) - (size_t)at, "%s%.4f", k ? ";" : "", r);
    }
    const double np = (double)W * (double)H;
    DRow("desk-move %s mid=%.5f cur=%.5f prev=%.5f mix=%.5f imid=%.5f icur=%.5f bt=%.5f bb=%.5f bl=%.5f br=%.5f q0=%.5f q1=%.5f q2=%.5f q3=%.5f amid=%.7f achange=%.7f maxmid=%.5f maxat=%u:%u"
         " over=%u/%u/%u/%u ring=%u far=%u eqcur=%u eqprev=%u eqmid=%u notgrey=%u nonfinite=%u a1=%u a0=%u ax=%u aseen=0x%04X ared=%u acur=%u amin=%.5f amax=%.5f amean=%.5f box=%u:%u:%u:%u wblk=%.5f wblkat=%d blk=%s",
         head, DRatio(st.all.mid, st.all.change), DRatio(st.all.cur, st.all.change), DRatio(st.all.prev, st.all.change), DRatio(st.all.mix, st.all.change),
         DRatio(st.inner.mid, st.inner.change), DRatio(st.inner.cur, st.inner.change),
         DRatio(st.band[0].mid, st.band[0].change), DRatio(st.band[1].mid, st.band[1].change), DRatio(st.band[2].mid, st.band[2].change), DRatio(st.band[3].mid, st.band[3].change),
         DRatio(st.quad[0].mid, st.quad[0].change), DRatio(st.quad[1].mid, st.quad[1].change), DRatio(st.quad[2].mid, st.quad[2].change), DRatio(st.quad[3].mid, st.quad[3].change),
         st.all.mid / np, st.all.change / np, st.maxMid, st.maxX, st.maxY, st.over[0], st.over[1], st.over[2], st.over[3], st.offEdge, st.offInside, st.eqCur, st.eqPrev, st.eqMid, st.notGrey, st.nonFinite,
         st.aOne, st.aZero, st.aOther, st.aSeen, st.aIsRed, st.aIsCur, st.aMin, st.aMax, st.aSum / np, st.x0, st.y0, st.x1, st.y1, worst, wi, blk);
}

static uint32_t DSteps(uint16_t a, uint16_t b);     // (item 3's: how many code steps lie between two values of one channel)

// ---------------------------------------------------------------- ITEMS 1 AND 2: the moving picture, the whole image
static void DeskMove()
{
    struct Arm { const char* name; int vec; bool withheld, jump, event; int warm, read; float peak; int per; };
    // (an arm's cycle shares no factor with the picture's six moves, so every count since a reset meets every move: 17, and 49 for the long one)
    static const Arm kReset[] = { { "CUT", DV_RIGHT, false, true, true, 12, 0, 1000.0f, 17 }, { "HIT", DV_RIGHT, false, false, true, 12, 0, 1000.0f, 17 },
                                  { "WH", DV_RIGHT, true, true, true, 12, 0, 1000.0f, 17 },   { "ZV", DV_ZERO, false, true, true, 12, 0, 1000.0f, 17 },
                                  { "ZL", DV_ZERO, false, true, true, 12, 0, 1000.0f, 49 } };
    // ITEM 2: THE SAME PAIRS AT THREE PEAKS (every arm of it starts the picture where the first did, on a reset of its own, so
    // pair i of one arm has pair i's inputs of another, and two arms' images can be compared by their hashes), then the control
    static const Arm kSteady[] = { { "F", DV_RIGHT, false, false, false, 14, 36, 1000.0f, 0 }, { "F80", DV_RIGHT, false, false, false, 14, 36, 80.0f, 0 },
                                   { "F0", DV_RIGHT, false, false, false, 14, 36, 0.0f, 0 },   { "FN", DV_NEG, false, false, false, 14, 12, 1000.0f, 0 } };
    const Arm* arms = dop.item == 1 ? kReset : kSteady; const int na = dop.item == 1 ? 5 : 4;
    const uint16_t* px = (const uint16_t*)dRbPtr; static DStat st; static DHash hs;
    // ITEM 2: arm F's image of every second pair is KEPT (its red: the picture's three colours are one value), so that the
    // arms at other peaks can be compared with it PIXEL BY PIXEL: how many differ, by how much, and where
    enum { KEEP_EVERY = 2, KEEP_N = 18 };
    uint16_t* kept = nullptr;
    if (dop.item == 2) { kept = (uint16_t*)malloc((size_t)KEEP_N * W * H * sizeof(uint16_t)); NEED(kept, "memory for arm F's kept images"); }
    int pairs = 0, reads = 0; bool first = true; char head[256];
    for (int ai = 0; ai < na; ai++)
    {
        const Arm& a = arms[ai]; const int per = a.per ? a.per : 17;
        const int total = a.warm + (a.event ? dop.cycles * per : a.read);
        int since = 0; bool gap = false;
        if (dop.item == 2) { dS = 0; dT = 0; dStep = 0; }
        for (int i = 0; i < total; i++)
        {
            const uint64_t n = dN++; const int ci = i - a.warm;
            const bool event = a.event && ci >= 0 && ci % per == 0;
            uint32_t qf = first ? FEED_FG_Q_RESET_BUILD : (i == 0 ? FEED_FG_Q_RESET_CUT : 0u);     // every arm starts on a reset of its own
            uint32_t kind = FEED_FG_G_DISPATCH;
            if (event && !a.withheld) qf |= FEED_FG_Q_RESET_CUT;
            if (event && a.withheld) kind = FEED_FG_G_WITHHELD;
            int move; uint64_t out;
            if (!DMovePair(n, qf, kind, a.vec, false, (event && a.jump) ? 1000 : 0, (event && a.jump) ? 700 : 0, &move, &out, a.peak))
            { Proof(false, "ITEM %d, arm %s: pair %d (%llu) could not go on: %s", dop.item, a.name, i + 1, (unsigned long long)n, dWhy); return; }
            first = false; pairs++;
            if (kind != FEED_FG_G_DISPATCH) { gap = true; continue; }           // no dispatch: the helper resets the next one on its own account
            since = (qf || gap) ? 0 : since + 1; gap = false;
            if (ci >= 0)
            {
                if (!DRead(gSet.img[FEED_FG_GEN])) { Proof(false, "ITEM %d, arm %s: %s", dop.item, a.name, dWhy); return; }
                DMoveStat(&st, px, DPres(), dQS, dQT, dPrevQS, dPrevQT);
                sprintf_s(head, "item=%d arm=%s cyc=%d k=%d n=%llu move=%d told=%u out=%llu", dop.item, a.name, a.event ? ci / per : 0, since, (unsigned long long)n, move, qf, (unsigned long long)out);
                DStatRow(head, st); reads++;
                if (dop.item == 2)
                {
                    DHashImage(&hs, dRbPtr);
                    DRow("desk-hash item=2 arm=%s i=%d n=%llu peak=%.0f all=%08x%08x col=%08x%08x", a.name, ci, (unsigned long long)n, a.peak, hs.all[0], hs.all[1], hs.col[0], hs.col[1]);
                    if (ci % KEEP_EVERY == 0 && ci / KEEP_EVERY < KEEP_N && a.vec == DV_RIGHT)
                    {
                        uint16_t* k = kept + (size_t)(ci / KEEP_EVERY) * W * H;
                        if (ai == 0) { for (size_t t = 0, np = (size_t)W * H; t < np; t++) k[t] = px[4 * t]; }
                        else
                        {   // against arm F's image of the same pair
                            uint32_t differ = 0, edge = 0, inside = 0, steps = 0, other = 0; double worst = 0.0, sum = 0.0, rel = 0.0; UINT x0 = W, y0 = H, x1 = 0, y1 = 0;
                            for (UINT y = 0; y < H; y++) for (UINT x = 0; x < W; x++)
                            {
                                const uint16_t* g = px + ((size_t)y * W + x) * 4u; const uint16_t f = k[(size_t)y * W + x];
                                if (g[1] != g[0] || g[2] != g[0]) other++;
                                if (g[0] == f) continue;
                                const double d = fabs(dH2F[g[0]] - dH2F[f]); const uint32_t s = DSteps(g[0], f);
                                differ++; sum += d;
                                if (d > worst) worst = d;
                                if (s > steps) steps = s;
                                if (fabs(dH2F[f]) > 0.0 && d / fabs(dH2F[f]) > rel) rel = d / fabs(dH2F[f]);
                                if (x < 8 || x >= W - 8 || y < 8 || y >= H - 8) edge++; else if (x >= 64 && x < W - 64 && y >= 64 && y < H - 64) inside++;
                                if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y;
                            }
                            DRow("desk-peak-diff item=2 arm=%s against=F i=%d peak=%.0f differ=%u edge=%u inside=%u max=%.6g mean=%.6g maxsteps=%u maxrel=%.6g notgrey=%u box=%u:%u:%u:%u", a.name, ci, a.peak, differ, edge, inside,
                                 worst, differ ? sum / differ : 0.0, steps, rel, other, x0, y0, x1, y1);
                        }
                    }
                }
            }
            dPrevQS = dQS; dPrevQT = dQT;
        }
        Say("  desk-arm item=%d arm=%s pairs=%d last=%llu", dop.item, a.name, total, (unsigned long long)(dN - 1));
    }
    free(kept);
    Proof(reads > 0, "ITEM %d: %d arms, %d pairs, each answered as its kind says; behind %d dispatches the whole of GEN (%ux%u) was read back and measured against the four pictures", dop.item, na, pairs, reads, W, H);
}

// ---------------------------------------------------------------- ITEM 3: HDR, by bits
// A whole image against what it must be, BY BITS: one value at every pixel (keep null) or a kept image. The three colours
// are compared; alpha is counted by value, the first four values it holds.
struct DCmp
{
    uint32_t diff[3], pixels, blocks, aVal[4], aCnt[4], aKinds, aMore, fGot[4], fWant[4];
    uint32_t maxSteps, beyond;                          // the largest difference of one channel in CODE STEPS, and the pixels with a channel more than one step off
    double maxAbs[3], sumAbs[3], maxRel, maxRelPix;     // maxRelPix: the largest difference of a pixel as a part of that pixel's own largest channel
    UINT x0, y0, x1, y1, fx, fy; bool any; uint8_t hit[C_HGX * C_HGY];
};
static double DValOf(uint16_t v) { return dH2F[v]; }
static double DValOf(uint8_t v) { return (double)v / 255.0; }
// How many code steps lie between two values of one channel (half floats of one sign count by their bits; across zero the two distances from zero add up)
static uint32_t DSteps(uint16_t a, uint16_t b) { return ((a ^ b) & 0x8000u) ? (uint32_t)(a & 0x7FFFu) + (uint32_t)(b & 0x7FFFu) : (uint32_t)(a > b ? a - b : b - a); }
static uint32_t DSteps(uint8_t a, uint8_t b) { return (uint32_t)(a > b ? a - b : b - a); }
template <class T> static void DCmpImage(DCmp* c, const T* px, const T* want4, const T* keep)
{
    memset(c, 0, sizeof(*c)); c->x0 = W; c->y0 = H;
    uint8_t* blk = c->hit;
    const UINT bw = W / C_HGX, bh = H / C_HGY;
    for (UINT y = 0; y < H; y++) for (UINT x = 0; x < W; x++)
    {
        const T* g = px + ((size_t)y * W + x) * 4u; const T* w = keep ? keep + ((size_t)y * W + x) * 4u : want4;
        bool d = false, far1 = false; double worst = 0.0;
        for (int ch = 0; ch < 3; ch++) if (g[ch] != w[ch])
        {
            const double wv = DValOf(w[ch]), a = fabs(DValOf(g[ch]) - wv); const uint32_t st = DSteps(g[ch], w[ch]);
            c->diff[ch]++; c->sumAbs[ch] += a; d = true;
            if (a > c->maxAbs[ch]) c->maxAbs[ch] = a;
            if (a > worst) worst = a;
            if (st > c->maxSteps) c->maxSteps = st;
            if (st > 1) far1 = true;
            if (fabs(wv) > 0.0 && a / fabs(wv) > c->maxRel) c->maxRel = a / fabs(wv);
        }
        if (d)
        {
            const double a0 = fabs(DValOf(w[0])), a1 = fabs(DValOf(w[1])), a2 = fabs(DValOf(w[2])), top = a0 > a1 ? (a0 > a2 ? a0 : a2) : (a1 > a2 ? a1 : a2);
            if (top > 0.0 && worst / top > c->maxRelPix) c->maxRelPix = worst / top;
            if (far1) c->beyond++;
            c->pixels++; blk[(y / bh) * C_HGX + x / bw] = 1;
            if (x < c->x0) c->x0 = x; if (x > c->x1) c->x1 = x; if (y < c->y0) c->y0 = y; if (y > c->y1) c->y1 = y;
            if (!c->any) { c->any = true; c->fx = x; c->fy = y; for (int ch = 0; ch < 4; ch++) { c->fGot[ch] = g[ch]; c->fWant[ch] = w[ch]; } }
        }
        uint32_t k = 0;
        while (k < c->aKinds && c->aVal[k] != g[3]) k++;
        if (k < c->aKinds) c->aCnt[k]++;
        else if (c->aKinds < 4) { c->aVal[c->aKinds] = g[3]; c->aCnt[c->aKinds++] = 1; }
        else c->aMore++;
    }
    for (int k = 0; k < C_HGX * C_HGY; k++) c->blocks += blk[k];
    if (!c->any) for (int ch = 0; ch < 4; ch++) { c->fGot[ch] = px[ch]; c->fWant[ch] = keep ? keep[ch] : want4[ch]; }
}
static void DCmpRow(const char* head, const DCmp& c)
{
    const double np = (double)W * (double)H;
    char map[C_HGX * C_HGY + 1];
    for (int k = 0; k < C_HGX * C_HGY; k++) map[k] = c.hit[k] ? '1' : '0';
    map[C_HGX * C_HGY] = 0;
    DRow("desk-const %s px=%u d0=%u d1=%u d2=%u max0=%.6g max1=%.6g max2=%.6g mean0=%.6g mean1=%.6g mean2=%.6g maxrel=%.6g maxsteps=%u beyond=%u relpix=%.6g box=%u:%u:%u:%u blocks=%u at=%u:%u got=%04X:%04X:%04X:%04X want=%04X:%04X:%04X:%04X"
         " alpha=%04X:%u/%04X:%u/%04X:%u/%04X:%u akinds=%u amore=%u blkmap=%s",
         head, c.pixels, c.diff[0], c.diff[1], c.diff[2], c.maxAbs[0], c.maxAbs[1], c.maxAbs[2], c.sumAbs[0] / np, c.sumAbs[1] / np, c.sumAbs[2] / np, c.maxRel, c.maxSteps, c.beyond, c.maxRelPix,
         c.x0, c.y0, c.x1, c.y1, c.blocks, c.fx, c.fy, c.fGot[0], c.fGot[1], c.fGot[2], c.fGot[3], c.fWant[0], c.fWant[1], c.fWant[2], c.fWant[3],
         c.aVal[0], c.aCnt[0], c.aVal[1], c.aCnt[1], c.aVal[2], c.aCnt[2], c.aVal[3], c.aCnt[3], c.aKinds, c.aMore, map);
}
// FOURTEEN STILL PAIRS OF ONE PICTURE, the first told a cut: a constant (c, with alpha aP in PRESENT and aH in HUDLESS) or
// the kept picture out of `src`. GEN IS SEEDED before every pair that is read, in the submit that signals fgIn, with a
// value nothing sends: a dispatch that wrote nothing reads as the seed, never as exact (the review of stages 5 to 7).
// Read behind dispatch 0 (the reset), 1, 2, 6, 10 and 13. TWO CONTROLS on the last: the seed read back before 'G', and the
// answer compared with a value ONE CODE STEP off in the first channel.
template <class T> static bool DStillCase(const char* fmt, const char* name, const float* c, float aP, float aH, float peak, const Img* src, const T* keep, const T* want4, const float* seed, const T* seed4, bool first)
{
    static DCmp cm; char head[256];
    for (int k = 0; k < 14; k++)
    {
        const uint64_t n = dN++; uint64_t out = 0;
        const bool read = k == 0 || k == 1 || k == 2 || k == 6 || k == 10 || k == 13;
        Begin(); Clear(gIm[FEED_COLOR], 0.5f, 0.5f, 0.5f, 1.0f); Clear(gIm[FEED_MV], 0.0f, 0.0f, 0.0f, 0.0f);
        if (!Submit(gSemIn, n) || !DFrameMsg(n) || !FeedVkWaitTimeline(&gVk, gSemOut, n, 5000)) { dWhy = dWhy[0] ? dWhy : "a frame was not answered in 5 s"; return false; }
        if (!DQ(n, k == 0 ? (first ? FEED_FG_Q_RESET_BUILD : FEED_FG_Q_RESET_CUT) : 0u, 0.05f)) return false;
        Begin();
        if (src)
        {
            FeedVkCopyImage(&gVk, gCb, src->img, VK_IMAGE_LAYOUT_GENERAL, gSet.img[FEED_FG_PRESENT], VK_IMAGE_LAYOUT_GENERAL, W, H);
            FeedVkCopyImage(&gVk, gCb, src->img, VK_IMAGE_LAYOUT_GENERAL, gSet.img[FEED_FG_HUDLESS], VK_IMAGE_LAYOUT_GENERAL, W, H);
        }
        else { Clear(SetImg(FEED_FG_PRESENT), c[0], c[1], c[2], aP); Clear(SetImg(FEED_FG_HUDLESS), c[0], c[1], c[2], aH); }
        if (read) Clear(SetImg(FEED_FG_GEN), seed[0], seed[1], seed[2], seed[3]);
        if (!Submit(gSet.semIn, n)) { dWhy = "the copies' submit was not taken"; return false; }
        if (k == 13)
        {   // THE FIRST CONTROL: what an unwritten GEN reads as
            if (!DRead(gSet.img[FEED_FG_GEN])) return false;
            DCmpImage<T>(&cm, (const T*)dRbPtr, keep ? nullptr : want4, keep);
            sprintf_s(head, "fmt=%s case=%s peak=%.0f k=%d n=%llu ctl=seed", fmt, name, peak, k, (unsigned long long)n);
            DCmpRow(head, cm);
            DCmpImage<T>(&cm, (const T*)dRbPtr, seed4, (const T*)nullptr);
            sprintf_s(head, "fmt=%s case=%s peak=%.0f k=%d n=%llu ctl=seedis", fmt, name, peak, k, (unsigned long long)n);
            DCmpRow(head, cm);
        }
        if (!DG(n, FEED_FG_G_DISPATCH, peak, &out)) return false;
        if (!read) continue;
        if (!DRead(gSet.img[FEED_FG_GEN])) return false;
        DCmpImage<T>(&cm, (const T*)dRbPtr, keep ? nullptr : want4, keep);
        sprintf_s(head, "fmt=%s case=%s peak=%.0f k=%d n=%llu ctl=none", fmt, name, peak, k, (unsigned long long)n);
        DCmpRow(head, cm);
        if (k == 13 && !keep)
        {   // THE SECOND CONTROL: the same reading against a value one code step off in the first channel
            T off[4] = { (T)(want4[0] + 1), want4[1], want4[2], want4[3] };
            DCmpImage<T>(&cm, (const T*)dRbPtr, off, (const T*)nullptr);
            sprintf_s(head, "fmt=%s case=%s peak=%.0f k=%d n=%llu ctl=step", fmt, name, peak, k, (unsigned long long)n);
            DCmpRow(head, cm);
        }
    }
    return true;
}
static void DeskHdr()
{
    const uint32_t BGRA8 = (uint32_t)DXGI_FORMAT_B8G8R8A8_UNORM;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    static const float kC[5][3] = { { 1.0f, 1.0f, 1.0f }, { 4.0f, 4.0f, 4.0f }, { 12.5f, 12.5f, 12.5f }, { 12.5f, 0.1f, 0.1f }, { -0.05f, 0.6f, 0.2f } };
    static const char* const kCName[5] = { "1", "4", "12.5", "12.5red", "negative" };
    static const float kPeak[4] = { 1000.0f, 400.0f, 0.0f, 80.0f };
    const float seed[4] = { -1.0f, -1.0f, -1.0f, -1.0f }; const uint16_t seed4[4] = { Half(-1.0f), Half(-1.0f), Half(-1.0f), Half(-1.0f) };
    int cases = 0; bool first = true;
    // ---- the five constants at four peaks, alpha 1 in both inputs
    for (int ci = 0; ci < 5; ci++) for (int pi = 0; pi < 4; pi++)
    {
        const uint16_t want[4] = { Half(kC[ci][0]), Half(kC[ci][1]), Half(kC[ci][2]), Half(1.0f) };
        if (!DStillCase<uint16_t>("fp16", kCName[ci], kC[ci], 1.0f, 1.0f, kPeak[pi], nullptr, nullptr, want, seed, seed4, first)) { Proof(false, "ITEM 3, the constant %s at a peak of %.0f: %s", kCName[ci], kPeak[pi], dWhy); return; }
        first = false; cases++;
    }
    // ---- alpha: PRESENT and HUDLESS carry other values than 1
    {
        const float grey[3] = { 0.5f, 0.5f, 0.5f }; const uint16_t want[4] = { Half(0.5f), Half(0.5f), Half(0.5f), Half(1.0f) };
        if (!DStillCase<uint16_t>("fp16", "alpha.25/.75", grey, 0.25f, 0.75f, 1000.0f, nullptr, nullptr, want, seed, seed4, false)
         || !DStillCase<uint16_t>("fp16", "alpha0/0", grey, 0.0f, 0.0f, 1000.0f, nullptr, nullptr, want, seed, seed4, false)) { Proof(false, "ITEM 3, alpha: %s", dWhy); return; }
        cases += 2;
    }
    // ---- a still PICTURE with highlights: red 14 times mode 20's three waves (0.3 to 12.3), green 4 times, blue the waves
    // less 0.45 (negative over half of the picture). Made on the CPU, uploaded once, kept for the comparison
    {
        const size_t bytes = (size_t)W * H * 8u;
        uint16_t* keep = (uint16_t*)malloc(bytes); NEED(keep, "memory for the still picture");
        uint16_t* stage = (uint16_t*)dRbPtr; const double tau = 6.283185307179586;
        float* su = (float*)malloc(sizeof(float) * (W + H) * 3u); NEED(su, "memory for the waves");
        float* sv = su + (W + H); float* sd = sv + (W + H);
        for (UINT i = 0; i < W + H; i++) { su[i] = (float)(0.20 * sin(tau * i / 96.0)); sv[i] = (float)(0.15 * sin(tau * i / 64.0)); sd[i] = (float)(0.08 * sin(tau * i / 48.0)); }
        const uint16_t one = Half(1.0f);
        for (UINT y = 0; y < H; y++) for (UINT x = 0; x < W; x++)
        {
            const float w = 0.45f + su[x] + sv[y] + sd[x + y]; uint16_t* o = stage + ((size_t)y * W + x) * 4u;
            o[0] = Half(14.0f * w); o[1] = Half(4.0f * w); o[2] = Half(w - 0.45f); o[3] = one;
        }
        free(su);
        memcpy(keep, stage, bytes);
        const Img still = CMakeImage(W, H, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        Begin();
        FeedVkBarrierN(&gVk, gCb, &still.img, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        VkBufferImageCopy rg = {}; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { W, H, 1 };
        tCopyBufferToImage(gCb, dRb, still.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg);
        NEED(Submit(VK_NULL_HANDLE, 0), "the still picture's upload");
        static DCmp cm;
        memset(dRbPtr, 0, bytes);
        if (!DRead(still.img)) { Proof(false, "ITEM 3: %s", dWhy); return; }
        DCmpImage<uint16_t>(&cm, (const uint16_t*)dRbPtr, nullptr, keep);
        if (!Proof(cm.pixels == 0, "ITEM 3, the instrument: the still picture read back from the image it was uploaded into is the kept one, bit for bit (%u pixels differ)", cm.pixels)) return;
        for (int pi = 0; pi < 3; pi++)
        {
            if (!DStillCase<uint16_t>("fp16", "picture", nullptr, 1.0f, 1.0f, kPeak[pi], &still, keep, (const uint16_t*)nullptr, seed, seed4, false)) { Proof(false, "ITEM 3, the still picture at a peak of %.0f: %s", kPeak[pi], dWhy); return; }
            cases++;
        }
        free(keep);
    }
    // ---- the same in an 8-bit set (sRGB): five constants and alpha. The image is B8G8R8A8: memory holds blue first
    int x = FgxRelease<FakeB>(&gSet, DL);
    if (x == FGX_RELEASED) x = FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)BGRA8, cRw, cRh, 0, DL, gWireFlags);      // (wire 2: the flag clear, a set WITH the HUD-less image)
    if (x == FGX_CREATED) { Begin(); FeedVkBarrierN(&gVk, gCb, gSet.all, DSetImages(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); if (!Submit(VK_NULL_HANDLE, 0)) x = -1; }
    if (!Proof(x == FGX_CREATED && DHasHudless() && gSet.fmt[FEED_FG_GEN] == BGRA8, "ITEM 3: the FP16 set is released and a set in B8G8R8A8 is CREATED through the client's own lines (status %d)", gSet.status)) return;
    {
        static const uint8_t kB[5][3] = { { 255, 255, 255 }, { 64, 128, 191 }, { 16, 16, 16 }, { 255, 26, 26 }, { 13, 153, 51 } };   // red, green, blue
        static const char* const kBName[5] = { "white", "64/128/191", "16", "255/26/26", "13/153/51" };
        const float seed8[4] = { 77.0f / 255.0f, 33.0f / 255.0f, 201.0f / 255.0f, 99.0f / 255.0f }; const uint8_t seed8b[4] = { 201, 33, 77, 99 };
        for (int ci = 0; ci < 7; ci++)
        {
            const int b = ci < 5 ? ci : 1; const uint8_t aP = ci == 5 ? 64 : ci == 6 ? 0 : 255, aH = ci == 5 ? 191 : ci == 6 ? 0 : 255;
            const float c[3] = { kB[b][0] / 255.0f, kB[b][1] / 255.0f, kB[b][2] / 255.0f };
            const uint8_t want[4] = { kB[b][2], kB[b][1], kB[b][0], 255 };
            const char* name = ci < 5 ? kBName[ci] : ci == 5 ? "alpha64/191" : "alpha0/0";
            if (!DStillCase<uint8_t>("bgra8", name, c, aP / 255.0f, aH / 255.0f, 0.0f, nullptr, nullptr, want, seed8, seed8b, ci == 0)) { Proof(false, "ITEM 3, the 8-bit set, %s: %s", name, dWhy); return; }
            cases++;
        }
    }
    Proof(cases == 32, "ITEM 3: %d cases of fourteen still pairs, each pair answered with a frame; GEN was seeded before every pair that was read and the whole of it compared by bits", cases);
}

// ---------------------------------------------------------------- ITEM 4: the HUD's rectangle
enum { D_HX = 480, D_HY = 1500, D_HW = 512, D_HH = 256, D_PX = 1536, D_PY = 400, D_PW = 128, D_PSTEP = 192, D_PLANTS = 6 };
static const float kDPlant[D_PLANTS] = { 0.0f, 0.004f, 0.02f, 0.05f, 0.10f, 0.25f };        // (the first is ONE CODE STEP, not an amount)
static const float kDHud[3][3] = { { 0.95f, 0.10f, 0.10f }, { 0.10f, 0.95f, 0.10f }, { 0.10f, 0.10f, 0.95f } };
static uint16_t DPlanted(int p, uint16_t b) { return p >= D_PLANTS ? b : p == 0 ? (uint16_t)(b + 1) : Half(dH2F[b] + kDPlant[p]); }
struct DHud
{
    uint32_t aN, aThis, aPrev, pN, pEqP[7], pEqC[7]; double aErr, eNum[4][64], eDen[4][64], w[7], pMid[7], pCur[7], pVal[7];
    DZone farZ;
};
static void DHudStat(DHud* o, const uint16_t* px, const DPic& s, long cS_, long cT_, long pS_, long pT_, const uint16_t* hudNow, const uint16_t* hudPrev)
{
    memset(o, 0, sizeof(*o));
    const long P = (long)s.period;
    const UINT cox = DOff(cS_, P), coy = DOff(cT_, P), pox = DOff(pS_, P), poy = DOff(pT_, P), mox = DOff((cS_ + pS_) / 2, P), moy = DOff((cT_ + pT_) / 2, P);
    auto G = [&](UINT x, UINT y) { return px + ((size_t)y * W + x) * 4u; };
    auto C = [&](UINT x, UINT y) { return s.r[(size_t)(y + coy) * s.w + x + cox]; };
    auto Pv = [&](UINT x, UINT y) { return s.r[(size_t)(y + poy) * s.w + x + pox]; };
    auto M = [&](UINT x, UINT y) { return s.r[(size_t)(y + moy) * s.w + x + mox]; };
    // the rectangle's inside, 32 pixels in from its edge
    for (UINT y = D_HY + 32; y < D_HY + D_HH - 32; y++) for (UINT x = D_HX + 32; x < D_HX + D_HW - 32; x++)
    {
        const uint16_t* g = G(x, y); o->aN++;
        if (g[0] == hudNow[0] && g[1] == hudNow[1] && g[2] == hudNow[2]) o->aThis++;
        if (g[0] == hudPrev[0] && g[1] == hudPrev[1] && g[2] == hudPrev[2]) o->aPrev++;
        o->aErr += (fabs(dH2F[g[0]] - dH2F[hudNow[0]]) + fabs(dH2F[g[1]] - dH2F[hudNow[1]]) + fabs(dH2F[g[2]] - dH2F[hudNow[2]])) / 3.0;
    }
    // its four edges, by the distance INTO the rectangle (d from -32, outside, to 31): the difference from what should be there
    // (the rectangle's colour inside, the half-way picture outside) as a part of the contrast between the two
    auto edge = [&](int side, int d, UINT x, UINT y)
    {
        const uint16_t* g = G(x, y); const double mf = dH2F[M(x, y)];
        for (int ch = 0; ch < 3; ch++)
        {
            const double hf = dH2F[hudNow[ch]];
            o->eNum[side][d + 32] += fabs(dH2F[g[ch]] - (d >= 0 ? hf : mf)); o->eDen[side][d + 32] += fabs(hf - mf);
        }
    };
    for (int d = -32; d < 32; d++)
    {
        for (UINT y = D_HY + 32; y < D_HY + D_HH - 32; y++) { edge(0, d, (UINT)((int)D_HX + d), y); edge(1, d, (UINT)((int)(D_HX + D_HW - 1) - d), y); }
        for (UINT x = D_HX + 32; x < D_HX + D_HW - 32; x++) { edge(2, d, x, (UINT)((int)D_HY + d)); edge(3, d, x, (UINT)((int)(D_HY + D_HH - 1) - d)); }
    }
    // the six plants and the place beside them where nothing was planted: how far GEN goes from the half-way picture
    // towards what PRESENT held there (w: 0 the half-way picture, 1 the presented pixel)
    for (int p = 0; p < 7; p++)
    {
        double sgm = 0, scm = 0, mid = 0, cur = 0, chg = 0, val = 0; o->pN = 0;
        for (UINT y = D_PY + 16; y < D_PY + D_PW - 16; y++) for (UINT x = D_PX + D_PSTEP * (UINT)p + 16; x < D_PX + D_PSTEP * (UINT)p + D_PW - 16; x++)
        {
            const uint16_t* g = G(x, y); const uint16_t c0 = C(x, y), cp = DPlanted(p, c0);
            const double gf = dH2F[g[0]], mf = dH2F[M(x, y)], cf = dH2F[cp];
            sgm += (gf - mf) * (cf - mf); scm += (cf - mf) * (cf - mf); mid += fabs(gf - mf); cur += fabs(gf - cf); chg += fabs(dH2F[c0] - dH2F[Pv(x, y)]); val += dH2F[c0];
            if (g[0] == cp && g[1] == cp && g[2] == cp) o->pEqP[p]++;
            if (g[0] == c0 && g[1] == c0 && g[2] == c0) o->pEqC[p]++;
            o->pN++;
        }
        o->w[p] = scm > 0.0 ? sgm / scm : 0.0; o->pMid[p] = DRatio(mid, chg); o->pCur[p] = DRatio(cur, chg); o->pVal[p] = o->pN ? val / o->pN : 0.0;
    }
    // the rows far from both (80 pixels and more): the picture itself
    for (UINT y = 0; y < H; y++)
    {
        if ((y + 80 >= D_PY && y < D_PY + D_PW + 80) || (y + 80 >= D_HY && y < D_HY + D_HH + 80)) continue;
        double v[5] = {};
        for (UINT x = 0; x < W; x++)
        {
            const double gf = dH2F[G(x, y)[0]], cf = dH2F[C(x, y)], pf = dH2F[Pv(x, y)], mf = dH2F[M(x, y)];
            v[0] += fabs(gf - mf); v[1] += fabs(gf - cf); v[2] += fabs(gf - pf); v[3] += fabs(gf - 0.5 * (cf + pf)); v[4] += fabs(cf - pf);
        }
        DZoneAdd(&o->farZ, v);
    }
}
static void DeskHud()
{
    static const char* const kArm[3] = { "OWN", "SAME", "STALE" };
    const DPic& s = dPicPlain; uint16_t* patch = (uint16_t*)dPatchPtr; const uint16_t* px = (const uint16_t*)dRbPtr;
    static DHud hs; static char line[4 * 64 * 9 + 2048];
    int reads = 0, pairs = 0; bool first = true; uint16_t hudPrev[4] = {};
    for (int arm = 0; arm < 3; arm++) for (int i = 0; i < 26; i++)
    {
        const uint64_t n = dN++; int move; float vx, vy; uint64_t out;
        DStepPicture(false, 0, 0, &move, &vx, &vy, DV_RIGHT);
        if (!DFrame(n, vx, vy) || !DQ(n, first ? FEED_FG_Q_RESET_BUILD : (i == 0 ? FEED_FG_Q_RESET_CUT : 0u), 0.05f)) { Proof(false, "ITEM 4, arm %s: pair %d: %s", kArm[arm], i + 1, dWhy); return; }
        first = false;
        // this pair's rectangles, made on the CPU: the HUD's in this pair's colour, then the six plants out of the picture
        const int col = (int)(n % 3);
        const uint16_t hud[4] = { Half(kDHud[col][0]), Half(kDHud[col][1]), Half(kDHud[col][2]), Half(1.0f) };
        VkBufferImageCopy rg[1 + D_PLANTS] = {}; size_t at = 0;
        rg[0].bufferOffset = 0; rg[0].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg[0].imageOffset = { D_HX, D_HY, 0 }; rg[0].imageExtent = { D_HW, D_HH, 1 };
        for (size_t k = 0; k < (size_t)D_HW * D_HH; k++) memcpy(patch + 4 * k, hud, 8);
        at = (size_t)D_HW * D_HH;
        const UINT cox = DOff(dQS, (long)s.period), coy = DOff(dQT, (long)s.period); const uint16_t one = Half(1.0f);
        for (int p = 0; p < D_PLANTS; p++)
        {
            const UINT x0 = D_PX + D_PSTEP * (UINT)p;
            rg[1 + p].bufferOffset = at * 8u; rg[1 + p].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg[1 + p].imageOffset = { (int32_t)x0, D_PY, 0 }; rg[1 + p].imageExtent = { D_PW, D_PW, 1 };
            for (UINT y = 0; y < D_PW; y++) for (UINT x = 0; x < D_PW; x++, at++)
            {
                const uint16_t v = DPlanted(p, s.r[(size_t)(D_PY + y + coy) * s.w + x0 + x + cox]);
                patch[4 * at] = v; patch[4 * at + 1] = v; patch[4 * at + 2] = v; patch[4 * at + 3] = one;
            }
        }
        Begin();
        DPutPicture(gCb, gSet.img[FEED_FG_PRESENT], dQS, dQT);
        if (arm == 2) DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dPrevQS, dPrevQT); else DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dQS, dQT);
        DTransferBarrier(gCb);
        tCopyBufferToImage(gCb, dPatch, gSet.img[FEED_FG_PRESENT], VK_IMAGE_LAYOUT_GENERAL, 1 + D_PLANTS, rg);
        if (arm == 1) tCopyBufferToImage(gCb, dPatch, gSet.img[FEED_FG_HUDLESS], VK_IMAGE_LAYOUT_GENERAL, 1 + D_PLANTS, rg);
        if (!Submit(gSet.semIn, n) || !DG(n, FEED_FG_G_DISPATCH, 1000.0f, &out)) { Proof(false, "ITEM 4, arm %s: pair %d: %s", kArm[arm], i + 1, dWhy[0] ? dWhy : "the copies' submit was not taken"); return; }
        pairs++;
        if (i >= 14 || i == 0)
        {   // (pair 0 is the arm's reset dispatch: which of the two inputs does it copy)
            if (!DRead(gSet.img[FEED_FG_GEN])) { Proof(false, "ITEM 4: %s", dWhy); return; }
            DHudStat(&hs, px, s, dQS, dQT, dPrevQS, dPrevQT, hud, hudPrev);
            int la = sprintf_s(line, "desk-hud arm=%s i=%d n=%llu move=%d col=%d an=%u athis=%u aprev=%u aerr=%.6f far_mid=%.5f far_cur=%.5f", kArm[arm], i, (unsigned long long)n, move, col,
                               hs.aN, hs.aThis, hs.aPrev, hs.aN ? hs.aErr / hs.aN : 0.0, DRatio(hs.farZ.mid, hs.farZ.change), DRatio(hs.farZ.cur, hs.farZ.change));
            for (int p = 0; p < 7; p++) la += sprintf_s(line + la, sizeof(line) - (size_t)la, " w%d=%.5f pm%d=%.5f pc%d=%.5f eqp%d=%u eqc%d=%u pv%d=%.4f", p, hs.w[p], p, hs.pMid[p], p, hs.pCur[p], p, hs.pEqP[p], p, hs.pEqC[p], p, hs.pVal[p]);
            la += sprintf_s(line + la, sizeof(line) - (size_t)la, " pn=%u", hs.pN);
            for (int sd = 0; sd < 4; sd++)
            {
                la += sprintf_s(line + la, sizeof(line) - (size_t)la, " edge%d=", sd);
                for (int d = 0; d < 64; d++) la += sprintf_s(line + la, sizeof(line) - (size_t)la, "%s%.4f", d ? ";" : "", DRatio(hs.eNum[sd][d], hs.eDen[sd][d]));
            }
            DRow("%s", line); reads++;
        }
        dPrevQS = dQS; dPrevQT = dQT; memcpy(hudPrev, hud, sizeof(hud));
    }
    Proof(reads == 39, "ITEM 4: three arms of 26 pairs (%d answered with a frame); behind the first (the reset dispatch) and the last twelve of each the whole of GEN was read and measured in and round the rectangles (%d readings)", pairs, reads);
}

// ---------------------------------------------------------------- ITEM 5: the held answer
static bool DHeldRow(const char* step, uint64_t n, const DHash& ref, DHash* got, uint64_t fgOut)
{
    if (!DRead(gSet.img[FEED_FG_GEN])) return false;
    DHashImage(got, dRbPtr);
    DRow("desk-held step=%s n=%llu fgout=%llu hash=%08x%08x col=%08x%08x same=%d colsame=%d blocks=%d", step, (unsigned long long)n, (unsigned long long)fgOut, got->all[0], got->all[1], got->col[0], got->col[1],
         DHashEq(*got, ref) ? 1 : 0, DHashColEq(*got, ref) ? 1 : 0, DHashSame(*got, ref));
    return true;
}
static void DeskHeld()
{
    const uint16_t* px = (const uint16_t*)dRbPtr; static DStat st; static DHash h0, hx, hA, hB; int move; uint64_t out = 0, n = 0; float vx, vy; char head[128];
    for (int i = 0; i < 16; i++)
    {
        if (i) { dPrevQS = dQS; dPrevQT = dQT; }
        n = dN++;
        if (!DMovePair(n, i == 0 ? FEED_FG_Q_RESET_BUILD : 0u, FEED_FG_G_DISPATCH, DV_RIGHT, false, 0, 0, &move, &out)) { Proof(false, "ITEM 5: pair %d: %s", i + 1, dWhy); return; }
    }
    // ---- pair n is answered: its image, then the same image behind every step of the next pair up to the game's own signal
    bool ok = DRead(gSet.img[FEED_FG_GEN]);
    if (ok)
    {
        DHashImage(&h0, dRbPtr); DMoveStat(&st, px, DPres(), dQS, dQT, dPrevQS, dPrevQT);
        sprintf_s(head, "item=5 arm=answer cyc=0 k=15 n=%llu move=%d told=0 out=%llu", (unsigned long long)n, move, (unsigned long long)out); DStatRow(head, st);
        DRow("desk-held step=answer n=%llu fgout=%llu hash=%08x%08x col=%08x%08x same=1 colsame=1 blocks=%d", (unsigned long long)n, (unsigned long long)out, h0.all[0], h0.all[1], h0.col[0], h0.col[1], C_HGX * C_HGY);
    }
    Sleep(1500);
    ok = ok && DHeldRow("later", n, h0, &hx, FeedVkTimelineValue(&gVk, gSet.semOut));
    const long keepS = dQS, keepT = dQT; const uint64_t m = dN++;
    DStepPicture(false, 0, 0, &move, &vx, &vy, DV_RIGHT);
    ok = ok && DFrame(m, vx, vy) && DHeldRow("nextframe", n, h0, &hx, FeedVkTimelineValue(&gVk, gSet.semOut));
    ok = ok && DQ(m, 0, 0.05f) && DHeldRow("nextQ", n, h0, &hx, FeedVkTimelineValue(&gVk, gSet.semOut));
    if (ok) { Begin(); DPutPicture(gCb, gSet.img[FEED_FG_PRESENT], dQS, dQT); DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dQS, dQT); ok = Submit(VK_NULL_HANDLE, 0); }   // fgIn WITHHELD
    ok = ok && DHeldRow("nextcopies", n, h0, &hx, FeedVkTimelineValue(&gVk, gSet.semOut));
    ok = ok && DSendG(m, FEED_FG_G_DISPATCH, 1000.0f);
    Sleep(1500);
    const uint64_t held = Seen(gSet.semOut, &gSeenOut);
    ok = ok && DHeldRow("nextG", n, h0, &hx, held);
    ok = ok && SignalNow(gSet.semIn, m) && FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * m, 5000);
    const uint64_t after = Seen(gSet.semOut, &gSeenOut);
    ok = ok && DHeldRow("nextIn", m, h0, &hx, after);
    if (ok)
    {
        DMoveStat(&st, px, DPres(), dQS, dQT, keepS, keepT);
        sprintf_s(head, "item=5 arm=next cyc=0 k=16 n=%llu move=%d told=0 out=%llu", (unsigned long long)m, move, (unsigned long long)after); DStatRow(head, st);
    }
    if (!Proof(ok && held == 2 * n && after == 2 * m, "ITEM 5: pair %llu's image was read at its answer and behind five steps of pair %llu (fgOut read %llu while fgIn was withheld for 1.5 s, %llu behind it)%s%s",
               (unsigned long long)n, (unsigned long long)m, (unsigned long long)held, (unsigned long long)after, ok ? "" : ": ", ok ? "" : dWhy)) return;
    dPrevQS = dQS; dPrevQT = dQT;
    // ---- THE RULE ON THE QUEUE ITSELF, nothing waited for between the two submits. Pair q is prepared, its inputs are in
    // place and its 'G' is written with fgIn withheld, so the helper's dispatch stands on its queue behind the wait. Then:
    //   way 0  the read of GEN is submitted BEFORE the submit that signals fgIn (the map's rule): it must hold pair p's image
    //   way 1  the read is submitted right BEHIND that submit, nothing waited: what a late read gets (counted, not judged)
    //   way 2  the read is submitted after pair q's answer: THE CONTROL, it must NOT hold pair p's image
    const int trials = dop.pairs > 0 ? dop.pairs : 36; int done = 0;
    for (int t = 0; t < trials; t++, done++)
    {
        ok = DRead(gSet.img[FEED_FG_GEN]);
        if (!ok) break;
        DHashImage(&hA, dRbPtr);
        const uint64_t q = dN++; const int way = t % 3;
        DStepPicture(false, 0, 0, &move, &vx, &vy, DV_RIGHT);
        ok = DFrame(q, vx, vy) && DQ(q, 0, 0.05f);
        if (ok) { Begin(); DPutPicture(gCb, gSet.img[FEED_FG_PRESENT], dQS, dQT); DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dQS, dQT); ok = Submit(VK_NULL_HANDLE, 0); }
        ok = ok && DSendG(q, FEED_FG_G_DISPATCH, 1000.0f);
        if (!ok) break;
        Sleep(4);                                                               // the helper has read 'G' and queued its dispatch behind the wait
        if (way == 0)      { DBegin(0); DReadInto(dCb[0], gSet.img[FEED_FG_GEN]); ok = DSubmit(0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0); DBegin(1); ok = ok && DSubmit(1, VK_NULL_HANDLE, 0, gSet.semIn, q); }
        else if (way == 1) { DBegin(1); ok = DSubmit(1, VK_NULL_HANDLE, 0, gSet.semIn, q); DBegin(0); DReadInto(dCb[0], gSet.img[FEED_FG_GEN]); ok = ok && DSubmit(0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0); }
        else               { DBegin(1); ok = DSubmit(1, VK_NULL_HANDLE, 0, gSet.semIn, q) && FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * q, 5000); DBegin(0); DReadInto(dCb[0], gSet.img[FEED_FG_GEN]); ok = ok && DSubmit(0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0); }
        ok = ok && FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * q, 5000) && Seen(gSet.semOut, &gSeenOut) == 2 * q && vkQueueWaitIdle_(gQ) == VK_SUCCESS;
        if (!ok) { dWhy = dWhy[0] ? dWhy : "a trial of the queue's order was not answered"; break; }
        DHashImage(&hx, dRbPtr);
        ok = DRead(gSet.img[FEED_FG_GEN]);
        if (!ok) break;
        DHashImage(&hB, dRbPtr);
        DRow("desk-order t=%d way=%d p=%llu q=%llu is_p=%d blocks_p=%d is_q=%d blocks_q=%d", t, way, (unsigned long long)(q - 1), (unsigned long long)q, DHashEq(hx, hA) ? 1 : 0, DHashSame(hx, hA), DHashEq(hx, hB) ? 1 : 0, DHashSame(hx, hB));
        dPrevQS = dQS; dPrevQT = dQT;
    }
    Proof(done == trials, "ITEM 5, the rule on the queue: %d trials of %d, each a reference read of pair p, pair q's dispatch queued behind fgIn, a read of GEN and the submit that signals fgIn back to back, then a read of pair q%s%s",
          done, trials, done == trials ? "" : ": ", done == trials ? "" : dWhy);
}

// ---------------------------------------------------------------- ITEM 6: two depth planes
// A STILL PLANE (the picture standing at 0, 0) AND AN OBJECT IN FRONT OF IT that moves by the picture's six moves and
// carries its own piece of the picture with it. The guides: the object's vectors over its rectangle of THIS frame and
// zero elsewhere; the depth as the game writes it (near over Z: 1 at the near plane, towards 0 far away), one value in
// the object's rectangle and one elsewhere. What should come out is the object at its half-way place over the plane.
enum { D_OW = 960, D_OH = 640, D_OU = 64, D_OV = 64, D_OX0 = 1280, D_OY0 = 704 };
struct DDepthArm { const char* name; float zObj, zBg, nearPlane; bool objVec; };
static const DDepthArm kDArm[] = {
    { "R",  0.5f,  0.02f, 0.05f,  true  },          // THE RIGHT PLANES: the object near, the plane behind it far
    { "S",  0.02f, 0.5f,  0.05f,  true  },          // swapped
    { "F0", 0.0f,  0.0f,  0.05f,  true  },          // one plane at zero: what every desk run before this one sent
    { "F1", 0.1f,  0.1f,  0.05f,  true  },          // one plane, not zero
    { "N1", 0.5f,  0.02f, 1.0f,   true  },          // the right planes, the near plane on 'Q' at 1.0 (the game's other camera)
    { "N2", 0.5f,  0.02f, 100.0f, true  },          // the right planes, a near plane that is wrong
    { "V",  0.5f,  0.02f, 0.05f,  false },          // THE CONTROL: the right planes, the object's vectors left at zero
    { "R2", 0.5f,  0.02f, 0.05f,  true  },          // the right planes once more: the same inputs must give the same image
};
struct DDepth { double err[11], con[11], farMax; uint32_t n[11], bgEq, bgN; };   // zones: 0 the object's inside, 1 to 4 the band inside (left, right, top, bottom), 5 to 8 outside, 9 the plane near, 10 the plane far
static int DDepthZone(int x, int y, int hx, int hy)
{
    const int l = x - hx, r = hx + D_OW - 1 - x, t = y - hy, b = hy + D_OH - 1 - y;
    if (l >= 0 && r >= 0 && t >= 0 && b >= 0)
    {
        const int di = l < r ? (l < t ? (l < b ? l : b) : (t < b ? t : b)) : (r < t ? (r < b ? r : b) : (t < b ? t : b));
        if (di >= 48) return 0;
        return di == l ? 1 : di == r ? 2 : di == t ? 3 : 4;
    }
    const int ol = -l, orr = -r, ot = -t, ob = -b;
    const int dox = ol > orr ? ol : orr, doy = ot > ob ? ot : ob, dd = dox > doy ? dox : doy;
    if (dd > 400) return 10;
    if (dd > 48) return 9;
    return dd == ol ? 5 : dd == orr ? 6 : dd == ot ? 7 : 8;
}
static void DDepthStat(DDepth* o, const uint16_t* px, const DPic& s, int ox, int oy, int pox, int poy)
{
    memset(o, 0, sizeof(*o));
    const int hx = (ox + pox) / 2, hy = (oy + poy) / 2;
    for (int y = 0; y < (int)H; y++) for (int x = 0; x < (int)W; x++)
    {
        const uint16_t* g = px + ((size_t)y * W + (size_t)x) * 4u;
        const uint16_t bg = s.r[(size_t)y * s.w + (size_t)x];
        const int z = DDepthZone(x, y, hx, hy);
        if (z >= 9)
        {   // the plane, away from the object's edge: an absolute difference. Far from the object nothing moves: is the still plane returned bit for bit
            if (z == 10) { o->bgN++; if (g[0] == bg && g[1] == bg && g[2] == bg) o->bgEq++; }
            const double e = fabs(dH2F[g[0]] - dH2F[bg]);
            o->err[z] += e; o->n[z]++;
            if (z == 10 && e > o->farMax) o->farMax = e;
            continue;
        }
        const uint16_t ob = s.r[(size_t)(y - hy + D_OV) * s.w + (size_t)(x - hx + D_OU)];    // the object's piece of the picture, carried on past its edge
        const double gf = dH2F[g[0]], bf = dH2F[bg], of = dH2F[ob];
        o->err[z] += fabs(gf - (z <= 4 ? of : bf)); o->con[z] += fabs(of - bf); o->n[z]++;
    }
}
static void DeskDepth()
{
    const DPic& s = dPicPlain; const uint16_t* px = (const uint16_t*)dRbPtr; static DDepth ds; static DHash hh;
    uint16_t* keep = (uint16_t*)malloc((size_t)W * H * 8u); NEED(keep, "memory for the kept image");
    const int na = (int)(sizeof(kDArm) / sizeof(kDArm[0])); int reads = 0, pairs = 0; bool first = true;
    Begin(); CPicture(gCb, cAtlas, tPlain.period, 0, 0, gIm[FEED_COLOR].img, cRw, cRh);
    NEED(Submit(VK_NULL_HANDLE, 0), "the still plane into Color");
    for (int ai = 0; ai < na; ai++)
    {
        const DDepthArm& a = kDArm[ai]; int ox = D_OX0, oy = D_OY0;
        for (int i = 0; i < 26; i++)
        {
            const uint64_t n = dN++; const int dx = kCMove[i % 6][0], dy = kCMove[i % 6][1]; const int pox = ox, poy = oy; uint64_t out;
            ox += dx; oy += dy;
            VkBufferImageCopy rc = {}; rc.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rc.imageOffset = { ox, oy, 0 }; rc.imageExtent = { D_OW, D_OH, 1 };
            // the frame: the vectors (zero, and the object's over its rectangle) and the two depth planes, the in-fence in that submit
            Begin();
            Clear(gIm[FEED_MV], 0.0f, 0.0f, 0.0f, 0.0f); Clear(gIm[FEED_DEPTH], a.zBg, 0.0f, 0.0f, 0.0f);
            uint32_t zbits; memcpy(&zbits, &a.zObj, 4);
            cCmdFillBuffer(gCb, dFillMv, 0, (VkDeviceSize)D_OW * D_OH * 4u, (uint32_t)Half((float)-dx) | ((uint32_t)Half((float)-dy) << 16));
            cCmdFillBuffer(gCb, dFillZ, 0, (VkDeviceSize)D_OW * D_OH * 4u, zbits);
            DTransferBarrier(gCb);
            if (a.objVec) tCopyBufferToImage(gCb, dFillMv, gIm[FEED_MV].img, VK_IMAGE_LAYOUT_GENERAL, 1, &rc);
            tCopyBufferToImage(gCb, dFillZ, gIm[FEED_DEPTH].img, VK_IMAGE_LAYOUT_GENERAL, 1, &rc);
            bool ok = Submit(gSemIn, n) && DFrameMsg(n) && FeedVkWaitTimeline(&gVk, gSemOut, n, 5000);
            ok = ok && DQ(n, first ? FEED_FG_Q_RESET_BUILD : (i == 0 ? FEED_FG_Q_RESET_CUT : 0u), a.nearPlane);
            first = false;
            if (ok)
            {   // the presented picture: the plane, then the object over it
                Begin();
                VkImageCopy oc = {}; oc.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; oc.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                oc.srcOffset = { D_OU, D_OV, 0 }; oc.dstOffset = { ox, oy, 0 }; oc.extent = { D_OW, D_OH, 1 };
                for (int k = 0; k < 2; k++) CPicture(gCb, cAtlas, tPlain.period, 0, 0, gSet.img[k ? FEED_FG_HUDLESS : FEED_FG_PRESENT], W, H);
                DTransferBarrier(gCb);
                for (int k = 0; k < 2; k++) gVk.CmdCopyImage(gCb, cAtlas.img, VK_IMAGE_LAYOUT_GENERAL, gSet.img[k ? FEED_FG_HUDLESS : FEED_FG_PRESENT], VK_IMAGE_LAYOUT_GENERAL, 1, &oc);
                ok = Submit(gSet.semIn, n) && DG(n, FEED_FG_G_DISPATCH, 1000.0f, &out);
            }
            if (!ok) { Proof(false, "ITEM 6, arm %s: pair %d: %s", a.name, i + 1, dWhy[0] ? dWhy : "a submit was not taken or a frame was not answered"); free(keep); return; }
            pairs++;
            if (i < 14) continue;
            if (!DRead(gSet.img[FEED_FG_GEN])) { Proof(false, "ITEM 6: %s", dWhy); free(keep); return; }
            DDepthStat(&ds, px, s, ox, oy, pox, poy); DHashImage(&hh, dRbPtr);
            DRow("desk-depth arm=%s i=%d n=%llu dx=%d dy=%d zobj=%.3f zbg=%.3f near=%.2f col=%08x%08x obj=%.5f in_l=%.5f in_r=%.5f in_t=%.5f in_b=%.5f out_l=%.5f out_r=%.5f out_t=%.5f out_b=%.5f near_abs=%.7f far_abs=%.7f far_max=%.7f far_eq=%u far_n=%u",
                 a.name, i, (unsigned long long)n, dx, dy, a.zObj, a.zBg, a.nearPlane, hh.col[0], hh.col[1], DRatio(ds.err[0], ds.con[0]),
                 DRatio(ds.err[1], ds.con[1]), DRatio(ds.err[2], ds.con[2]), DRatio(ds.err[3], ds.con[3]), DRatio(ds.err[4], ds.con[4]),
                 DRatio(ds.err[5], ds.con[5]), DRatio(ds.err[6], ds.con[6]), DRatio(ds.err[7], ds.con[7]), DRatio(ds.err[8], ds.con[8]), ds.n[9] ? ds.err[9] / ds.n[9] : 0.0,
                 ds.n[10] ? ds.err[10] / ds.n[10] : 0.0, ds.farMax, ds.bgEq, ds.bgN);
            reads++;
            if (i != 20) continue;
            if (ai == 0) { memcpy(keep, px, (size_t)W * H * 8u); continue; }
            // against the first arm's image of the same pair, pixel by pixel: how many differ, where, and by how much
            uint32_t differ = 0, zone[3] = {}; double worst = 0.0; UINT x0 = W, y0 = H, x1 = 0, y1 = 0; const int hx = (ox + pox) / 2, hy = (oy + poy) / 2;
            for (UINT y = 0; y < H; y++) for (UINT x = 0; x < W; x++)
            {
                const uint16_t* g = px + ((size_t)y * W + x) * 4u; const uint16_t* k = keep + ((size_t)y * W + x) * 4u;
                if (g[0] == k[0] && g[1] == k[1] && g[2] == k[2]) continue;
                differ++; const int z = DDepthZone((int)x, (int)y, hx, hy); zone[z == 0 ? 0 : z <= 8 ? 1 : 2]++;
                const double d = fabs(dH2F[g[0]] - dH2F[k[0]]); if (d > worst) worst = d;
                if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y;
            }
            DRow("desk-depth-diff arm=%s against=R i=%d differ=%u in_object=%u in_band=%u in_plane=%u max=%.6f box=%u:%u:%u:%u", a.name, i, differ, zone[0], zone[1], zone[2], worst, x0, y0, x1, y1);
        }
    }
    free(keep);
    Proof(reads == na * 12, "ITEM 6: %d arms of 26 pairs (%d answered with a frame); behind the last twelve of each the whole of GEN was read and measured by zone (%d readings), and pair 21 of each compared with the first arm's pixel by pixel", na, pairs, reads);
}

// ---------------------------------------------------------------- ITEM 7: the helper killed with a pair pending
static void DProductCopyOut(VkCommandBuffer cb, VkImage src)        // the product's order for a read OUT of a set image: acquired, copied into the back buffer, released
{
    VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    gVk.CmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    FeedVkExternalTransferN(&gVk, cb, &src, 1, gFam, false /*acquire*/);
    FeedVkCopyImage(&gVk, cb, src, VK_IMAGE_LAYOUT_GENERAL, cBb.img, VK_IMAGE_LAYOUT_GENERAL, W, H);
    FeedVkExternalTransferN(&gVk, cb, &src, 1, gFam, true /*release*/);
}
static bool DDeadRow(const char* step, const char* which, VkImage img, const DHash& ref, DHash* got)
{
    Say("  desk-dead reading %s at step %s", which, step);
    if (!DRead(img)) return false;
    DHashImage(got, dRbPtr);
    DRow("desk-dead step=%s image=%s hash=%08x%08x same=%d blocks=%d", step, which, got->all[0], got->all[1], DHashEq(*got, ref) ? 1 : 0, DHashSame(*got, ref));
    return true;
}
static void DeskDead()
{
    const uint16_t* px = (const uint16_t*)dRbPtr; static DStat st; static DHash hp, hh, hg, hx, hy; int move; uint64_t out = 0, n = 0; float vx, vy; char head[128];
    for (int i = 0; i < 16; i++)
    {
        if (i) { dPrevQS = dQS; dPrevQT = dQT; }
        n = dN++;
        if (!DMovePair(n, i == 0 ? FEED_FG_Q_RESET_BUILD : 0u, FEED_FG_G_DISPATCH, DV_RIGHT, false, 0, 0, &move, &out)) { Proof(false, "ITEM 7: pair %d: %s", i + 1, dWhy); return; }
    }
    const long nS = dQS, nT = dQT;
    // ---- pair m: prepared, its inputs in place, its 'G' written. Its dispatch is held behind fgIn, or in flight
    const uint64_t m = dN++;
    DStepPicture(false, 0, 0, &move, &vx, &vy, DV_RIGHT);
    bool ok = DRead(gSet.img[FEED_FG_GEN]);
    if (ok) DHashImage(&hg, dRbPtr);
    ok = ok && DFrame(m, vx, vy) && DQ(m, 0, 0.05f);
    if (ok) { Begin(); DPutPicture(gCb, gSet.img[FEED_FG_PRESENT], dQS, dQT); DPutPicture(gCb, gSet.img[FEED_FG_HUDLESS], dQS, dQT); ok = Submit(dop.killUs ? gSet.semIn : VK_NULL_HANDLE, m); }
    ok = ok && DRead(gSet.img[FEED_FG_PRESENT]);
    if (ok)
    {
        DHashImage(&hp, dRbPtr); DMoveStat(&st, px, DPres(), dQS, dQT, nS, nT);
        sprintf_s(head, "item=7 arm=present_alive cyc=0 k=0 n=%llu move=%d told=0 out=0", (unsigned long long)m, move); DStatRow(head, st);
    }
    ok = ok && DRead(gSet.img[FEED_FG_HUDLESS]);
    if (ok) DHashImage(&hh, dRbPtr);
    DRow("desk-dead step=alive image=PRESENT hash=%08x%08x same=1 blocks=144", hp.all[0], hp.all[1]);
    DRow("desk-dead step=alive image=HUDLESS hash=%08x%08x same=1 blocks=144", hh.all[0], hh.all[1]);
    DRow("desk-dead step=alive image=GEN hash=%08x%08x same=1 blocks=144", hg.all[0], hg.all[1]);
    if (!Proof(ok, "ITEM 7: sixteen pairs answered, pair %llu prepared with its inputs in place, and the three images read through this side's imports with the helper alive%s%s", (unsigned long long)m, ok ? "" : ": ", ok ? "" : dWhy)) return;
    ok = DSendG(m, FEED_FG_G_DISPATCH, 1000.0f);
    const LONGLONG tG = CQpc(); LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    if (dop.killUs) { while ((CQpc() - tG) * 1000000 / qf.QuadPart < dop.killUs) { } } else Sleep(300);
    const uint64_t outBefore = FeedVkTimelineValue(&gVk, gSet.semOut);
    const LONGLONG tK = CQpc();
    gHelperEndedHere = TerminateProcess(gHelper, 9) != 0;
    const DWORD gone = WaitForSingleObject(gHelper, 5000);
    Say("  desk-dead the helper is killed (%s), %lld us after 'G' was written", gone == WAIT_OBJECT_0 ? "its process has ended" : "ITS PROCESS DID NOT END IN 5 s", (long long)((tK - tG) * 1000000 / qf.QuadPart));
    // ---- the fences, read at once; then a wait on the answer fence for the pending pair's own value
    const uint64_t fPrep = FeedVkTimelineValue(&gVk, gSet.semPrep), fOut = FeedVkTimelineValue(&gVk, gSet.semOut), fIn = FeedVkTimelineValue(&gVk, gSet.semIn),
                   uIn = FeedVkTimelineValue(&gVk, gSemIn), uOut = FeedVkTimelineValue(&gVk, gSemOut);
    const LONGLONG tw0 = CQpc();
    const bool waited = FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * m, 200);
    const LONGLONG tw1 = CQpc();
    DRow("desk-dead-fence pending=%llu want=%llu killus=%lld out_before=%llu prep=0x%llX out=0x%llX fgin=0x%llX frame_in=0x%llX frame_out=0x%llX wait_returned=%d wait_us=%lld at_least=%d equal=%d ended=%d",
         (unsigned long long)m, (unsigned long long)(2 * m), (long long)((tK - tG) * 1000000 / qf.QuadPart), (unsigned long long)outBefore, (unsigned long long)fPrep, (unsigned long long)fOut, (unsigned long long)fIn,
         (unsigned long long)uIn, (unsigned long long)uOut, waited ? 1 : 0, (long long)((tw1 - tw0) * 1000000 / qf.QuadPart), fOut >= 2 * m ? 1 : 0, fOut == 2 * m ? 1 : 0, gone == WAIT_OBJECT_0 ? 1 : 0);
    // ---- the three images through the imports, at once and 3 s later
    ok = gHelperEndedHere && DDeadRow("dead", "PRESENT", gSet.img[FEED_FG_PRESENT], hp, &hx) && DDeadRow("dead", "HUDLESS", gSet.img[FEED_FG_HUDLESS], hh, &hx) && DDeadRow("dead", "GEN", gSet.img[FEED_FG_GEN], hg, &hx);
    if (ok)
    {   // which pair's image GEN holds: against pair m's two pictures (0 from the half-way picture: m's own image, whole)
        DMoveStat(&st, px, DPres(), dQS, dQT, nS, nT);
        sprintf_s(head, "item=7 arm=gen_dead cyc=0 k=0 n=%llu move=%d told=0 out=0", (unsigned long long)m, move); DStatRow(head, st);
    }
    Sleep(3000);
    ok = ok && DDeadRow("dead3s", "PRESENT", gSet.img[FEED_FG_PRESENT], hp, &hy) && DDeadRow("dead3s", "HUDLESS", gSet.img[FEED_FG_HUDLESS], hh, &hy) && DDeadRow("dead3s", "GEN", gSet.img[FEED_FG_GEN], hx, &hy);
    // ---- the product's order on PRESENT (acquire, copy into the back buffer, release), then the back buffer read
    if (ok)
    {
        Say("  desk-dead the product's order on PRESENT");
        Begin(); DProductCopyOut(gCb, gSet.img[FEED_FG_PRESENT]); ok = Submit(VK_NULL_HANDLE, 0);
        ok = ok && DDeadRow("product", "BACKBUFFER", cBb.img, hp, &hy);
    }
    // ---- THE CONTROL: a constant written into PRESENT through the import must change what is read
    if (ok)
    {
        Say("  desk-dead the control: a constant written into PRESENT");
        Begin(); Clear(SetImg(FEED_FG_PRESENT), 0.125f, 0.25f, 0.5f, 1.0f); ok = Submit(VK_NULL_HANDLE, 0);
        ok = ok && DDeadRow("control", "PRESENT", gSet.img[FEED_FG_PRESENT], hp, &hy);
        const uint16_t want[4] = { Half(0.125f), Half(0.25f), Half(0.5f), Half(1.0f) }; static DCmp cm;
        DCmpImage<uint16_t>(&cm, px, want, (const uint16_t*)nullptr);
        DRow("desk-dead-control written_differs=%u of=%u", cm.pixels, W * H);
    }
    DRow("desk-dead-end fences_again prep=0x%llX out=0x%llX", (unsigned long long)FeedVkTimelineValue(&gVk, gSet.semPrep), (unsigned long long)FeedVkTimelineValue(&gVk, gSet.semOut));
    Proof(ok, "ITEM 7: the helper was killed with pair %llu pending; the five fences were read, a wait on the answer fence was tried, and the three images were read through this side's imports at once and 3 s later, then PRESENT by the product's order, then a constant written into it%s%s",
          (unsigned long long)m, ok ? "" : ": ", ok ? "" : (dWhy[0] ? dWhy : "a step was not submitted"));
}

// ---------------------------------------------------------------- ITEM 8: K at a desk
static void DProductCopy(VkCommandBuffer cb, VkImage dst)           // one of the product's two copies, as fg_product.h records it (CProductCopy without its stamps)
{
    VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    gVk.CmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    CBbBarrier(cb, true);
    FeedVkExternalTransferN(&gVk, cb, &dst, 1, gFam, false /*acquire*/);
    FeedVkCopyImage(&gVk, cb, cBb.img, VK_IMAGE_LAYOUT_GENERAL, dst, VK_IMAGE_LAYOUT_GENERAL, W, H);
    FeedVkExternalTransferN(&gVk, cb, &dst, 1, gFam, true /*release*/);
    CBbBarrier(cb, false);
}
// THE TIME FROM 'G' WRITTEN TO THE ANSWER SEEN, pair after pair, the card this pair's alone. The pair is the product's:
// the picture into an off-screen back buffer, the two real copies out of it (acquire, copy, release), fgIn in the second.
//   order 0  every part is waited for before the next (the frame, 'Q', the back buffer, each copy), so K is the helper's
//            reading of 'G', AMD's recording, the dispatch on the card and the answer's way back, and nothing else
//   order 1  the game's order with nothing waited between a pair's parts: the injection, the frame message, the home
//            (a submit that waits for the frame's answer on the queue), 'Q', the two copies, 'G'. K then holds whatever
//            of the frame, the prepare and the copies is still on the two queues when 'G' is written
// The answer is seen by reading the fence in a loop (the value itself, as the product's placer reads it) or by a wait.
// The first 600 pairs are nobody's (the card's clock settles); the rows are the pairs after them. A pair is steady from
// the 12th dispatch after a reset, and only the first pair resets.
static void DeskK()
{
    const int warm = 600, pairs = dop.pairs > 0 ? dop.pairs : 2200;
    struct KRow { uint64_t n; LONGLONG tQ, tIn, tG, tOut, tEnd; int move; uint32_t polls; };
    KRow* rows = (KRow*)malloc(sizeof(KRow) * (size_t)pairs); NEED(rows, "memory for the timed pairs");
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    int done = 0; const unsigned long long t0 = CUnixMs(); unsigned long long tWarm = 0;
    for (int i = 0; i < warm + pairs; i++)
    {
        const uint64_t n = dN++; int move; float vx, vy; KRow r = {}; r.n = n;
        if (i == warm) tWarm = CUnixMs();
        DStepPicture(false, 0, 0, &move, &vx, &vy, DV_RIGHT); r.move = move;
        const uint32_t qflags = i == 0 ? FEED_FG_Q_RESET_BUILD : 0u;
        bool ok;
        if (dop.order == 0)
        {
            ok = DFrame(n, vx, vy);
            ok = ok && DSendQ(n, qflags, 0.05f); r.tQ = CQpc();
            ok = ok && FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) == n;
            if (ok) { Begin(); DPutPicture(gCb, cBb.img, dQS, dQT); ok = Submit(VK_NULL_HANDLE, 0); }
            if (ok) { Begin(); DProductCopy(gCb, gSet.img[FEED_FG_PRESENT]); ok = Submit(VK_NULL_HANDLE, 0); }
            if (ok) { Begin(); DProductCopy(gCb, gSet.img[FEED_FG_HUDLESS]); ok = Submit(gSet.semIn, n); }
            r.tIn = CQpc();
        }
        else
        {
            DBegin(0); CPicture(dCb[0], cAtlas, tPlain.period, dS, dT, gIm[FEED_COLOR].img, cRw, cRh);
            { VkClearColorValue c; c.float32[0] = vx; c.float32[1] = vy; c.float32[2] = 0.0f; c.float32[3] = 0.0f; VkImageSubresourceRange rg = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
              vkCmdClearColorImage_(dCb[0], gIm[FEED_MV].img, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &rg); }
            ok = DSubmit(0, VK_NULL_HANDLE, 0, gSemIn, n) && DFrameMsg(n);
            DBegin(1); DPutPicture(dCb[1], cBb.img, dQS, dQT); ok = ok && DSubmit(1, gSemOut, n, VK_NULL_HANDLE, 0);
            ok = ok && DSendQ(n, qflags, 0.05f); r.tQ = CQpc();
            DBegin(2); DProductCopy(dCb[2], gSet.img[FEED_FG_PRESENT]); ok = ok && DSubmit(2, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0);
            DBegin(3); DProductCopy(dCb[3], gSet.img[FEED_FG_HUDLESS]); ok = ok && DSubmit(3, VK_NULL_HANDLE, 0, gSet.semIn, n);
            r.tIn = CQpc();
        }
        ok = ok && DSendG(n, FEED_FG_G_DISPATCH, 1000.0f); r.tG = CQpc();
        uint64_t v = 0;
        if (ok && dop.poll)
        {
            const LONGLONG limit = r.tG + 5 * qf.QuadPart;
            for (;;) { v = FeedVkTimelineValue(&gVk, gSet.semOut); r.tOut = CQpc(); if (v >= 2 * n || r.tOut > limit) break; r.polls++; }
        }
        else if (ok) { ok = FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000); r.tOut = CQpc(); v = FeedVkTimelineValue(&gVk, gSet.semOut); }
        if (ok && v != 2 * n) { ok = false; dWhy = "fgOut did not read the pair's own value"; }
        if (ok) { gFenceReads++; if (v < gSeenOut) gFenceBack = true; else gSeenOut = v; }
        ok = ok && vkQueueWaitIdle_(gQ) == VK_SUCCESS && FeedVkTimelineValue(&gVk, gSet.semPrep) >= n;
        r.tEnd = CQpc();
        if (!ok) { Proof(false, "ITEM 8: pair %d (%llu) could not go on: %s", i + 1, (unsigned long long)n, dWhy[0] ? dWhy : "a part of it was not submitted or not answered"); free(rows); return; }
        dPrevQS = dQS; dPrevQT = dQT;
        if (i >= warm) rows[done++] = r;
    }
    const unsigned long long t1 = CUnixMs();
    for (int i = 0; i < done; i++)
    {
        const KRow& r = rows[i];
        DRow("desk-k i=%d n=%llu move=%d g_us=%.1f in_us=%.1f q_us=%.1f pair_us=%.1f polls=%u", i, (unsigned long long)r.n, r.move, (double)(r.tOut - r.tG) * 1e6 / (double)qf.QuadPart, (double)(r.tOut - r.tIn) * 1e6 / (double)qf.QuadPart,
             (double)(r.tOut - r.tQ) * 1e6 / (double)qf.QuadPart, (double)(r.tEnd - r.tQ) * 1e6 / (double)qf.QuadPart, r.polls);
    }
    free(rows);
    // the correctness figure beside the time: the last pair's image, whole (read here, behind every timed pair)
    {
        // (the last pair's previous picture is the one before its move)
        const int last = (dStep + 5) % 6; const long pS = dS - kCMove[last][0], pT = dT - kCMove[last][1];
        static DStat st; char head[128];
        if (DRead(gSet.img[FEED_FG_GEN]))
        {
            DMoveStat(&st, (const uint16_t*)dRbPtr, DPres(), dQS, dQT, pS * (dop.guides ? 3 : 2) / 2, pT * (dop.guides ? 3 : 2) / 2);
            sprintf_s(head, "item=8 arm=last cyc=0 k=%d n=%llu move=%d told=0 out=%llu", warm + pairs - 1, (unsigned long long)(dN - 1), last, (unsigned long long)(2 * (dN - 1))); DStatRow(head, st);
        }
    }
    Say("  desk-k-take order=%d poll=%d warm=%d pairs=%d first=%llu last=%llu t0=%llu twarm=%llu t1=%llu qpc_hz=%lld", dop.order, dop.poll, warm, done, (unsigned long long)(dN - (uint64_t)done), (unsigned long long)(dN - 1), t0, tWarm, t1, (long long)qf.QuadPart);
    Proof(done == pairs, "ITEM 8: %d pairs nobody's, then %d timed pairs, each answered with a frame (fgOut read 2n exactly) and each ended on an empty queue; %s; the answer seen by %s", warm, done,
          dop.order ? "the game's order, nothing waited between a pair's parts" : "every part waited for before the next", dop.poll ? "reading the fence in a loop" : "a wait");
}

// ---------------------------------------------------------------- THE RUN
static void DeskRun()
{
    const uint32_t FP16 = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
    if (!Proof(dop.item >= 1 && dop.item <= 8 && dop.out[0], "the desk measurements are asked for one item of the eight (%d) and a file for its rows", dop.item)) return;
    if (!dop.hudless && dop.item != 1 && dop.item != 2)
    { Proof(false, "item %d reads or writes the HUD-less image: it runs on a set that has one (--fake-desk-hudless=1), and only items 1 and 2 run on the game's two-image shape", dop.item); return; }
    tCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkGetDeviceProcAddr_(gDev, "vkCmdCopyBufferToImage"); NEED(tCopyBufferToImage, "vkCmdCopyBufferToImage");
    cCmdFillBuffer = (PFN_vkCmdFillBuffer)vkGetDeviceProcAddr_(gDev, "vkCmdFillBuffer"); NEED(cCmdFillBuffer, "vkCmdFillBuffer");
    DTables();
    fopen_s(&dOut, dop.out, "w"); NEED(dOut, "the rows' file");
    // ---- the guides' size, the picture (made once, uploaded once), the off-screen back buffer, this mode's buffers
    if (dop.guides)
    {
        cRw = 2560; cRh = 1440;
        if (!Proof(TRebuild(cRw, cRh), "the upscaler's set is built again at %ux%u (the guides of DLSS Quality under a %ux%u display) and its four images import", cRw, cRh, W, H)) return;
    }
    else { Begin(); Clear(gIm[FEED_DEPTH], 0.0f, 0.0f, 0.0f, 0.0f); NEED(Submit(VK_NULL_HANDLE, 0), "the depth's clear"); }
    const VkImageUsageFlags plain = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    NEED(DBuffer((VkDeviceSize)W * H * 8u, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, &dRb, &dRbPtr), "the whole image's buffer");
    if (dop.item != 3)
    {
        TStageMake(&tPlain, W, H, 1, 1); cAtlas = CMakeImage(tPlain.w, tPlain.h, plain); DPicMake(&dPicPlain, tPlain);
        if (dop.guides) { TStageMake(&tWide, W, H, 3, 2); cAtlasWide = CMakeImage(tWide.w, tWide.h, plain); DPicMake(&dPicWide, tWide); }
        if (dop.scale > 1) { DPicScale(&dPicPlain, tPlain, dop.scale); if (dop.guides) DPicScale(&dPicWide, tWide, dop.scale); }
        cBb = CMakeImage(W, H, plain | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);      // 0x97, what DXVK gives the game's back buffer
        VkImage all[3] = { cAtlas.img, cBb.img, cAtlasWide.img };
        Begin();
        FeedVkBarrierN(&gVk, gCb, all, dop.guides ? 3 : 2, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        VkBufferImageCopy rg = {}; rg.bufferRowLength = tPlain.w; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { tPlain.w, tPlain.h, 1 };
        tCopyBufferToImage(gCb, tPlain.buf, cAtlas.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg);
        if (dop.guides) { rg.bufferRowLength = tWide.w; rg.imageExtent = { tWide.w, tWide.h, 1 }; tCopyBufferToImage(gCb, tWide.buf, cAtlasWide.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg); }
        NEED(Submit(VK_NULL_HANDLE, 0), "the picture's upload");
        Begin(); CPicture(gCb, cAtlas, tPlain.period, 0, 0, cBb.img, W, H);
        NEED(Submit(VK_NULL_HANDLE, 0), "the back buffer's first picture");
    }
    if (dop.item == 4) NEED(DBuffer(2u << 20, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, &dPatch, &dPatchPtr), "the rectangles' buffer");
    if (dop.item == 6)
    {
        NEED(DBuffer((VkDeviceSize)D_OW * D_OH * 4u, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, &dFillMv, nullptr)
          && DBuffer((VkDeviceSize)D_OW * D_OH * 4u, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false, &dFillZ, nullptr), "the object's two buffers");
    }
    {
        VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = gPool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 8;
        NEED(vkAllocateCommandBuffers_(gDev, &cai, dCb) == VK_SUCCESS, "the desk measurements' command buffers");
    }
    // ---- the set, by the client's own lines
    // (wire 2: WITH the HUD-less image the flag is clear, said here and not left to a default; WITHOUT it the set is the game's)
    int x = FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)FP16, cRw, cRh, 0, DL, (dop.hudless ? 0u : (uint32_t)FEED_FG_BUILD_NO_HUDLESS) | gWireFlags);
    if (x == FGX_CREATED) { Begin(); FeedVkBarrierN(&gVk, gCb, gSet.all, DSetImages(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); if (!Submit(VK_NULL_HANDLE, 0)) x = -1; }
    if (!Proof(x == FGX_CREATED && gSet.have && DHasHudless() == (dop.hudless != 0), "a set is CREATED through the client's own lines (status %d), %s the HUD-less image as it was asked (%u images): display %ux%u FP16 over guides of %ux%u; the create and the warm-up took %u.%u ms",
               gSet.status, DHasHudless() ? "WITH" : "WITHOUT", DSetImages(), W, H, cRw, cRh, gSet.warmUs / 1000, (gSet.warmUs % 1000) / 100)) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    Say("  desk-session item=%d scale=%d images=%u guides=%ux%u display=%ux%u first=%llu moves=%d:%d/%d:%d/%d:%d/%d:%d/%d:%d/%d:%d", dop.item, dop.scale, DSetImages(), cRw, cRh, W, H, (unsigned long long)dN,
        kCMove[0][0], kCMove[0][1], kCMove[1][0], kCMove[1][1], kCMove[2][0], kCMove[2][1], kCMove[3][0], kCMove[3][1], kCMove[4][0], kCMove[4][1], kCMove[5][0], kCMove[5][1]);
    switch (dop.item)
    {
        case 1: case 2: DeskMove(); break;
        case 3: DeskHdr(); break;
        case 4: DeskHud(); break;
        case 5: DeskHeld(); break;
        case 6: DeskDepth(); break;
        case 7: DeskDead(); break;
        case 8: DeskK(); break;
    }
    { LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
      Say("  desk-total item=%d rows=%d last=%llu read_ms=%lld stat_ms=%lld cached=%d", dop.item, dRows, (unsigned long long)(dN - 1), (long long)(dTRead * 1000 / qf.QuadPart), (long long)(dTStat * 1000 / qf.QuadPart), dCached); }
    if (dOut) { fclose(dOut); dOut = nullptr; }
    if (gHelperEndedHere) return;                   // item 7: there is nobody to give the set back to
    if (gFails) return;
    x = gSet.have ? FgxRelease<FakeB>(&gSet, DL) : FGX_RELEASED;
    Proof(x == FGX_RELEASED, "the set is RELEASED through the client's own lines (status %d)", gSet.status);
    Proof(!gFenceBack, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
}
// ============================================================================== END OF THE DESK MEASUREMENTS (--fake-fg=50)
