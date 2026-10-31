// =====================================================================================================================
// THE COST BENCH: --fake-fg=40 (2026-10-06, phase 3 of the frame generation revival, its first stage;
// docs/reports/fg-3/cost-2026-10-06/reader-bench.md, section 3, and critic.md, "THE ORDER OF WORK", step 3).
// ALL OF THIS MODE IS IN THIS FILE. Its only lines in fgpipe.cpp are the include above FgRun, the first line of FgRun and
// one line of main's argument loop.
//
// WHY. WHAT DOES ONE GENERATED FRAME COST, BY ITS PARTS. The game's two cost legs priced a sum (reset dispatches, wasted
// prepares, the two copies), and no session of this program timed a pair at all: its "copies" were clears. This mode is
// the game as far as the two GPU queues see it, with the parts of a pair sent one kind at a time:
//   - THE PRODUCT'S TWO REAL COPIES, done as source\fg\fg_product.h does them (RecBegin and RecCopyToImg: a memory barrier,
//     the back buffer's barrier, the set's image ACQUIRED, vkCmdCopyImage, RELEASED, the barrier back), each in a buffer
//     and a submit of its own, fgIn in the second one's, out of AN OFF-SCREEN 3840x2160 FP16 "BACK BUFFER" made with the
//     usage DXVK gives the game's (0x97, kept in GENERAL: DECISIONS, "Step 1 PASSED"). NO WINDOW AND NO SWAP CHAIN.
//   - VULKAN TIMESTAMPS round each copy (after the acquire and after the copy, as the rig stamps its copies), round each
//     buffer, and at every frame's end; an empty bracket in every frame's last buffer.
//   - A MOVING PICTURE AND MOVING VECTORS, mode 20's (the same three waves, the same whole-pixel moves), because the cost
//     may depend on the picture. The picture is uploaded ONCE into an image of this program's own, and every frame is a
//     copy out of it at an offset, so a frame costs this side two copies and a clear, not a 63 MiB upload.
//   - THE HELPER'S OWN MARKS (host\fg_cost.h: GPU timestamps on frame generation's lists, the CPU's clock round AMD's
//     calls) price the helper's half; this program starts the helper with whatever arguments run.py gives it
//     (--gpu_probe=1 turns the marks on, --fgcost_csv says where their rows go).
//
// THE ARMS, by the wire's own kinds (what a pair sends; reader-bench.md, item 13):
//     A     'F' only                                         the baseline frame (the set built and idle: standby)
//     P     'Q' and 'G' NOSOURCE                             AMD's prepare alone
//     PC    'Q', the copies with fgIn, 'G' WITHHELD          the prepare and the copies
//     F     the whole pair, 'G' DISPATCH, no reset           A STEADY, INTERPOLATING PAIR, on every pair
//     FR    as F, every 'Q' told a cut                       a reset dispatch on every pair
//     F1    as F, every second 'Q' told a cut                the first dispatch after a reset (the pairs between): NO
//                                                            optical flow search runs in it
//     FK    as F, every 17th 'Q' told a cut                  THE WAY BACK TO STEADY, dispatch by dispatch: AMD's optical
//                                                            flow search returns early for the first five dispatches
//                                                            after a reset and its colour stays out for about nine
//                                                            (AMD's source at v2.3.0), so dispatch 3 against dispatch
//                                                            12 or later is the search's price. 17 and the picture's six
//                                                            moves share no factor: every count meets every move
//     FS    as F, nothing moves, zero vectors                does the cost depend on the picture
//     FN    as F, every vector negated                       the same question (and its picture is NOT the half-way one)
//     FC    as F, the right half's vectors negated           VECTORS THAT COLLIDE at the seam: AMD's game vector field
//                                                            pass scatters, and where two sources land on one target the
//                                                            winner is whichever thread wrote first (its source, at
//                                                            v2.3.0). Is the generated image the same twice?
// A pair with no dispatch leaves the next 'Q' with the gap's reset flag, as the product's machine does.
// A PAIR IS STEADY FROM THE 12TH DISPATCH AFTER A RESET, NEVER EARLIER: the reader takes steady cells by the helper's own
// count of dispatches since the last reset (a column of its rows), not by a pair's place in a cell.
//
// TWO WAYS OF RUNNING THEM.
// IDLE (--fake-cost-idle=R): every part of a pair ALONE on the card. The frame, 'Q', each copy, 'G': each is waited for
// before the next goes, so a stamp brackets one piece of work and nothing of the other queue's. R rounds of every arm,
// the arms in a new order each round, N pairs a cell. What it prices: each part's GPU time. What it cannot: the two
// queues sharing one card.
// LOADED (--fake-cost-load=US[,US]): THE CARD IS THE LIMIT, AS IN THE LEGS FRAME GENERATION IS FOR. Every frame carries
// M full-screen copies of this program's own (half before the injection, "the scene", half behind the home, "the post
// chain"), M found by timestamps so that the baseline frame takes US microseconds. ONE FRAME IS ALWAYS IN FLIGHT (frame
// n is started when frame n-2 has ended on the GPU), in the product's per-frame order: the scene, the wait-only batch
// behind fgPrep when the last prepare has not retired (the client's own line, FgxPrepWait), the injection with the
// frame's in-fence, the frame message, the home (a submit that WAITS for this frame's out-fence), 'Q', the copies with
// fgIn, 'G', the frame's end. NO CPU WAIT ON THE HELPER ANYWHERE. Every pair is dispatched: the next pair's copy stands
// behind its own frame's home on this queue, and that home behind the last dispatch on the helper's one queue, so the
// product's NO SOURCE test is not needed here (critic.md, check 5; --fake-cost-rule=1 runs the product's test instead,
// arm FX). THE MEASURE IS THE PERIOD of the frame-end stamps, arm against baseline in halves A B B A, each half started
// from an empty queue and its first frames dropped by the reader: what the pair costs THE FRAME, which is what a
// player's frame rate pays. More arms there:
//     NULL  the baseline against itself                      the bench's own noise, which every delta is judged against
//     F2    F once more, last                                the same arm twice must agree
//     FX    F with the product's NO SOURCE rule              the share of pairs it dispatches at this load, at a desk
//     LK    the baseline with 8 more of the load's copies    the load is honest: 8 stamped copies must cost 8 copies
//     OFF   the set RELEASED in the B halves                 frame generation off against standby
//
// EVERY COST HAS A CORRECTNESS FIGURE BESIDE IT. After a dispatched pair (one in eight when idle, one a half when loaded,
// inside the frames the reader drops) four 64x64 tiles of GEN are read, and the centre and the quarter tile are compared
// with the pictures this program can compute, as mode 20's observation does: how far the generated image is from the
// HALF-WAY picture, from this frame's, from the previous frame's and from their even mix, as a part of the change
// between the two presented pictures. A steady pair must sit on the half-way picture; a reset pair is a copy of this
// frame's. An arm that is cheaper because it stopped interpolating shows it here. With --fake-cost-gen=PATH the raw
// tiles of chosen pairs go to a file (two of them on the picture's edges, where AMD's inpainting works), so two sessions
// that differ in one lever can be compared bit by bit (HUDLESS own against empty).
// AND THE WHOLE GENERATED IMAGE, HASHED (--fake-cost-hash=N): a part of its own, behind the idle arms and timed by nobody.
// N pairs of each of its arms, and behind EVERY dispatched pair all of GEN is read back (63.3 MiB) and hashed: all four
// channels, the three colours alone, and each block of a 16 by 9 grid, so a difference has a place. The rows go to
// --fake-cost-hashout. Two sessions on the same inputs say whether AMD's library gives the same image twice; only
// against that may two sessions that differ in one lever, or two builds of the library, be compared.
//
// WHAT THIS PROGRAM JUDGES ITSELF is the instrument only: every pair answered, every stamp read, no wait timed out, no
// fence read lower than before. THE NUMBERS ARE run.py's TO READ: one row a frame goes to --fake-cost-out, and one line
// a cell or block says which pairs it was and when ("cost-cell", "cost-block").
//
// ITS OPTIONS (every other argument goes to the helper):
//     --fake-cost-guides=N     0 the guides at the display's size (DLAA, the default); 1 at 2560x1440 (DLSS Quality)
//     --fake-cost-hud=N        1 both copies into a three-image set (the default); 0 A SET ASKED WITHOUT THE HUD-LESS IMAGE
//                              (FEED_FG_BUILD_NO_HUDLESS, wire 2: the product's own shape since lever 1) and ONE copy,
//                              fgIn in its submit; 2 THE TRAP'S CONTROL: a three-image set, ONE copy, and the helper
//                              still binds HUDLESS, which nothing ever wrote: its picture must NOT be the half-way one.
//                              (Until wire 2, 0 was for a helper started with --fghud_empty=1; that lever is still the
//                              helper's, for a set that has the image.)
//     --fake-cost-idle=R       the idle arms, R rounds (0, the default: none)
//     --fake-cost-pairs=N      pairs a cell (256)
//     --fake-cost-arms=LIST    the idle arms, by name (A,P,PC,F,FR,F1)
//     --fake-cost-load=US,..   the loaded parts: the baseline frame's time in microseconds, one or more (none)
//     --fake-cost-larms=LIST   the loaded arms, by name (NULL,P,PC,F,FR,F2)
//     --fake-cost-quads=N      A B B A quads an arm (8)
//     --fake-cost-half=N       frames a half (256)
//     --fake-cost-rule=N       1: every loaded arm that dispatches runs the product's NO SOURCE rule (0)
//     --fake-cost-cutg=N       (wire 2, 2026-10-06) 0: a pair's camera cut rides its 'Q' AND its 'G', as the product writes
//                              it (the default); 1: ON 'G' ALONE, the 'Q' carries no cut: the prepare is then not told
//                              to reset and only the dispatch is. Whether AMD's prepare needs the reason too is what
//                              the hash part of two sessions that differ in this one option answers (the specification's
//                              risk 1: arms FR, F1 and FK must give the same images either way)
//     --fake-cost-qat=N        (lever 2, 2026-10-10) 0: 'Q' is written at the hand, behind the home's submit (the default, and
//                              every number of this bench until that date); 1: RIGHT BEHIND THE FRAME MESSAGE, ahead of the
//                              home's submit, as the product writes it since (a cut then rides the 'G' alone)
//     --fake-cost-order=N      THE ORDER PART: N frames a cell, the two orders against each other, paced and loaded (0: none)
//     --fake-cost-oload=US     its loaded regime: the baseline frame's time in microseconds (0: none)
//     --fake-cost-ogap=US      the CPU's wait between the frame message and the hand in that part (2000)
//     --fake-cost-oreg=BITS    which of its regimes run: 1 the 59 cap, 2 the 118 cap, 4 the loaded one (7: all; a short
//                              session a regime is easier to keep to itself on a shared card)
//     --fake-cost-hash=N       the hash part: N pairs an arm, the whole of GEN hashed behind every dispatch (0: none)
//     --fake-cost-harms=LIST   its arms, by name (F,FR,F1,FN,FC)
//     --fake-cost-out=PATH     one row a frame
//     --fake-cost-gen=PATH     the raw tiles of chosen pairs
//     --fake-cost-hashout=PATH the hash part's rows
//     --fake-cost-fmt=N        (2026-10-06, the own build's stage 1b) 0 the presented format is FP16 (the default); 1 it is
//                              B8G8R8A8, the game's 8-bit back buffer: the picture reaches the off-screen back buffer by a
//                              blit (nearest), the set is asked for in that format and the whole image is hashed as 32-bit
//                              pixels. THE HASH PART ONLY: the timed parts and the correctness figure read FP16 and are
//                              refused in this format
//     --fake-cost-upscale=1    (2026-10-06, the own build's stage 1b) THE UPSCALER'S SET IS A REAL BUILD, not the transport's
//                              copy: the helper makes its upscaler (DLSS on an NVIDIA card; AMD's FSR through AMD's loader
//                              when the helper is started with --backend=1) and every frame message is an evaluate. So an
//                              upscaler's context is ALIVE AND WORKING on the helper's device beside frame generation,
//                              as it is in the game. Frame generation reads the upscaler's guides, never its output: the
//                              generated image must be what it is without it
//     --fake-cost-fq=1         (2026-10-10, the cost marks' head stamp) THE IDLE ARMS WRITE 'Q' RIGHT BEHIND 'F': the frame's
//                              answer is NOT waited for before the pair's 'Q' is written, so the helper's evaluate (or
//                              its transport copy) and its prepare follow each other on its queue with nothing between
//                              them, as they do in the game. The frame's answer and the home are waited for behind the
//                              'Q'; every other part of the pair is waited for as before. With --fake-cost-upscale=1 the
//                              list in front of the prepare is a real upscaler's evaluate: THE CONTROL OF WHETHER THE
//                              MARKS' PREPARE BRACKET HOLDS A STRANGER'S WORK (tools\test-fg-cost\run.py, U0, U1, UW)
//     --fake-cost-raw=LIST     the hash part: the pairs whose WHOLE generated image is also written to a file, as
//                              ARM:index,ARM:index (two sessions that differ in a pair's hash are run again with that pair
//                              named, and the two files say how many pixels differ and by how much)
//     --fake-cost-rawout=PATH  where: PATH-ARM-index.bin, the image's bytes as they were read
// =====================================================================================================================
#pragma once

struct COpt
{
    int guides = 0, hud = 1, idle = 0, pairs = 256, quads = 8, half = 256, rule = 0, hash = 0, cutg = 0, fmt = 0, upscale = 0, fq = 0;
    int qat = 0, order = 0, ogap = 2000, oload = 0, oreg = 7;   // lever 2 (2026-10-10): where 'Q' is written, and the order part (below)
    int ord = 0, hclass = 0, sclass = 0, over = 0, overctl = 0, overall = 0, held = 0;   // 2026-10-14: the helper's answer on a card that is the limit ("THE THREE MENDS AT A DESK", above CFrame)
    int loadUs[4] = {}, nLoad = 0;
    char arms[128] = "A,P,PC,F,FR,F1", larms[128] = "NULL,P,PC,F,FR,F2", harms[128] = "F,FR,F1,FN,FC";
    char out[MAX_PATH] = "", gen[MAX_PATH] = "", hashOut[MAX_PATH] = "", raw[512] = "", rawOut[MAX_PATH] = "";
};
static COpt co;
// One option of this mode, without its "--fake-cost-" (main hands it over; anything it does not know is said and ends the run).
static void CostOption(const char* a)
{
    if      (strncmp(a, "guides=", 7) == 0) co.guides = atoi(a + 7);
    else if (strncmp(a, "hud=", 4) == 0)    co.hud = atoi(a + 4);
    else if (strncmp(a, "idle=", 5) == 0)   co.idle = atoi(a + 5);
    else if (strncmp(a, "pairs=", 6) == 0)  co.pairs = atoi(a + 6);
    else if (strncmp(a, "quads=", 6) == 0)  co.quads = atoi(a + 6);
    else if (strncmp(a, "half=", 5) == 0)   co.half = atoi(a + 5);
    else if (strncmp(a, "rule=", 5) == 0)   co.rule = atoi(a + 5);
    else if (strncmp(a, "cutg=", 5) == 0)   co.cutg = atoi(a + 5);
    else if (strncmp(a, "qat=", 4) == 0)    co.qat = atoi(a + 4);
    else if (strncmp(a, "order=", 6) == 0)  co.order = atoi(a + 6);
    else if (strncmp(a, "ogap=", 5) == 0)   co.ogap = atoi(a + 5);
    else if (strncmp(a, "oload=", 6) == 0)  co.oload = atoi(a + 6);
    else if (strncmp(a, "oreg=", 5) == 0)   co.oreg = atoi(a + 5);
    else if (strncmp(a, "ord=", 4) == 0)    co.ord = atoi(a + 4);
    else if (strncmp(a, "hclass=", 7) == 0) co.hclass = atoi(a + 7);
    else if (strncmp(a, "sclass=", 7) == 0) co.sclass = atoi(a + 7);
    else if (strncmp(a, "overctl=", 8) == 0) co.overctl = atoi(a + 8);
    else if (strncmp(a, "overall=", 8) == 0) co.overall = atoi(a + 8);
    else if (strncmp(a, "over=", 5) == 0)   co.over = atoi(a + 5);
    else if (strncmp(a, "held=", 5) == 0)   co.held = atoi(a + 5);
    else if (strncmp(a, "hashout=", 8) == 0) strcpy_s(co.hashOut, a + 8);
    else if (strncmp(a, "hash=", 5) == 0)   co.hash = atoi(a + 5);
    else if (strncmp(a, "harms=", 6) == 0)  strcpy_s(co.harms, a + 6);
    else if (strncmp(a, "arms=", 5) == 0)   strcpy_s(co.arms, a + 5);
    else if (strncmp(a, "larms=", 6) == 0)  strcpy_s(co.larms, a + 6);
    else if (strncmp(a, "out=", 4) == 0)    strcpy_s(co.out, a + 4);
    else if (strncmp(a, "gen=", 4) == 0)    strcpy_s(co.gen, a + 4);
    else if (strncmp(a, "fmt=", 4) == 0)    co.fmt = atoi(a + 4);
    else if (strncmp(a, "upscale=", 8) == 0) co.upscale = atoi(a + 8);
    else if (strncmp(a, "fq=", 3) == 0)     co.fq = atoi(a + 3);
    else if (strncmp(a, "rawout=", 7) == 0) strcpy_s(co.rawOut, a + 7);
    else if (strncmp(a, "raw=", 4) == 0)    strcpy_s(co.raw, a + 4);
    else if (strncmp(a, "load=", 5) == 0)
    {
        for (const char* p = a + 5; *p && co.nLoad < 4; ) { co.loadUs[co.nLoad++] = atoi(p); while (*p && *p != ',') p++; if (*p == ',') p++; }
    }
    else { Say("  FAIL  --fake-cost-%s is not an option of the cost bench", a); Say("FAIL"); ExitProcess(2); }
}

enum { CA_A, CA_P, CA_PC, CA_F, CA_FR, CA_F1, CA_FS, CA_FN, CA_NULL, CA_F2, CA_FX, CA_LK, CA_OFF, CA_FK, CA_FC, CA_N_ };
static const char* const kCArm[CA_N_] = { "A", "P", "PC", "F", "FR", "F1", "FS", "FN", "NULL", "F2", "FX", "LK", "OFF", "FK", "FC" };
enum { C_FK_EVERY = 17 };                           // arm FK: every 17th 'Q' is told a cut
static int CArms(const char* list, int* out, int cap)
{
    int n = 0;
    for (const char* p = list; *p && n < cap; )
    {
        char name[16]; int k = 0;
        while (*p && *p != ',' && k < 15) name[k++] = *p++;
        name[k] = 0; if (*p == ',') p++;
        int which = -1;
        for (int i = 0; i < CA_N_; i++) if (strcmp(name, kCArm[i]) == 0) which = i;
        NEED(which >= 0, "an arm's name the cost bench does not know");
        out[n++] = which;
    }
    return n;
}

// ---------------------------------------------------------------- this mode's own buffers, stamps and images
static PFN_vkCreateQueryPool cCreateQueryPool; static PFN_vkCmdResetQueryPool cCmdResetQueryPool;
static PFN_vkCmdWriteTimestamp cCmdWriteTimestamp; static PFN_vkGetQueryPoolResults cGetQueryPoolResults; static PFN_vkCmdFillBuffer cCmdFillBuffer;
static VkBuffer cMvBuf;                             // arm FC: the right half's vectors, filled on the GPU and copied into the motion vector image
static VkBuffer cHashBuf; static void* cHashPtr;    // the hash part: all of GEN
static FILE* cHash;
enum { C_RING = 32, C_QPB = 4 };                    // command buffers of the ring, and timestamp queries each owns
struct CBuf { VkCommandBuffer cb = VK_NULL_HANDLE; uint64_t val = 0; uint64_t* dst = nullptr; int nq = 0; };
static CBuf cBuf[C_RING]; static int cNext;
static VkQueryPool cPool; static VkSemaphore cLocal; static uint64_t cVal;   // the ring's own timeline: a buffer's value retires it
static double cNs = 1.0;                            // nanoseconds a timestamp tick
static int cUnread, cLate;                          // stamps that could not be read; buffers that did not retire in 5 s
static Img cBb, cAtlas, cAtlasWide, cLoadImg[2];    // the off-screen back buffer; the picture, uploaded once (and 3/2 as large); the load's two images
static VkBuffer cObsBuf; static void* cObsPtr;      // four 64x64 FP16 tiles of GEN
enum { C_TILES = 4, C_TILE_BYTES = 64 * 64 * 8 };
static UINT cRw = W, cRh = H;                       // the guides' size
static uint64_t cN = 33, cLastQ, cLastDisp, cEnd[8]; static bool cGap = true;
static long cS, cT, cPrevPS, cPrevPT; static int cStep, cGuards;
static bool cOrderOn;                               // the order part (lever 2) is running: a frame waits its gap between the frame message and the hand
// THE THREE MENDS AT A DESK (2026-10-14; source/fg/fg_machine.h, "THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT"; the options at this file's
// head). The machine is not here; what is here is THE PRODUCT'S OWN LINES of fg_xport.h (FgxCardClass, FgxOrdMake, FgxOrdWait,
// FgxOrdSignal) on a real queue beside the real helper, and the order of submits the machine rests on:
//   --fake-cost-hclass=5   the helper's process asked into the scheduler's top class through FgxCardClass, at the cost mode's
//                          start, and given back at its end (what the armed product does while a pair rides)
//   --fake-cost-sclass=5   THIS program's own class raised instead: the stand-in for a game that outranks the helper (the
//                          legs' case: every dispatch then runs behind this side's scene)
//   --fake-cost-ord=N      in the order part the two alternating arms are the order wait off and on ('Q' right behind 'F' in
//                          both): 1 THE PRODUCT'S (a fence of this side's own, released from the CPU by the stamper of
//                          fgOut, which stands in for the placer); 2 the direct form, a wait on the helper's own answer
//                          fence, which the product does not make (the difference of the two is what the release's hop costs)
//   --fake-cost-over=N     THE COPY OVER A DISPATCH IN FLIGHT, N loaded frames with no CPU wait on the helper: the next pair's
//                          copies are submitted at once, behind the frame's home, and every fourth frame's home READS THE
//                          GENERATED IMAGE OF THE PAIR BEFORE BACK IN THE QUEUE'S OWN ORDER (behind its wait for this frame's
//                          answer, which stands behind that pair's dispatch; ahead of this pair's copies), to be held
//                          against the half-way picture of the two frames it was made from
//   --fake-cost-overctl=1  ITS CONTROL: the hand's copies are submitted AHEAD of the frame's home: nothing holds them
//                          behind the dispatch before, and its image is caught made from the wrong picture
//   --fake-cost-overall=1  the over part reads EVERY frame's image back (not every fourth): with the helper's own
//                          --fgslow_d=30 the pair whose dispatch the helper sleeps ahead of is then surely among them
//   --fake-cost-held=N     THE ORDER WAIT WITH THE HELPER HELD (its own --fgslow_d sleeps before every 64th dispatch): the wait
//                          stands and what is queued behind it does not retire; released from the CPU it does, with
//                          the helper still asleep (1), SUSPENDED (2) or KILLED (3)
static FgxOrd cOrd; static FgxCard cCard;
static int cOrdOn = 0;                              // this frame's order wait: 0 none, 1 the product's, 2 the direct form
static int cOverCtl = 0;                            // the over part's control is running
static bool cOverRef = false;                       // the over part's reference stretch: the hand waits ON THE CPU for the dispatch before (today's rule, with a wait in place of NO SOURCE)
static bool cOverObs = false;                       // this frame's home reads GEN's tiles back
static struct CBuf* cHomeBuf = nullptr; static struct CBuf* cEndBuf = nullptr;   // this frame's home and end buffers
static LONGLONG cOrdSubT = 0; static LONG cOrdSubN = 0;   // the order wait's submit on this thread: ticks in all, and how many
static FILE* cOut; static FILE* cGen;
static const char* cWhy = "";                       // why a frame could not go on

static LONGLONG CQpc() { LARGE_INTEGER q; QueryPerformanceCounter(&q); return q.QuadPart; }
static unsigned long long CUnixMs()
{
    FILETIME ft; GetSystemTimeAsFileTime(&ft);
    return ((((unsigned long long)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) - 116444736000000000ull) / 10000ull;
}
// A buffer of the ring before it is used again: its last submit has retired (waited for here, 5 s at most), and what it
// stamped is read into the frame's row. One frame is in flight and the ring holds 32 buffers, so this wait does not wait.
static bool CRetire(CBuf* b)
{
    if (!b->val) return true;
    if (FeedVkTimelineValue(&gVk, cLocal) < b->val && !FeedVkWaitTimeline(&gVk, cLocal, b->val, 5000)) { cLate++; cWhy = "a buffer of this side's did not retire in 5 s"; return false; }
    if (b->dst && b->nq)
    {
        uint64_t t[C_QPB] = {};
        if (cGetQueryPoolResults(gDev, cPool, (uint32_t)(b - cBuf) * C_QPB, (uint32_t)b->nq, sizeof(uint64_t) * (size_t)b->nq, t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
            memcpy(b->dst, t, sizeof(uint64_t) * (size_t)b->nq);
        else cUnread++;
    }
    b->val = 0; b->dst = nullptr; b->nq = 0;
    return true;
}
static CBuf* CTake(uint64_t* dst)
{
    CBuf* b = &cBuf[cNext]; cNext = (cNext + 1) % C_RING;
    if (!CRetire(b)) return nullptr;
    vkResetCommandBuffer_(b->cb, 0);
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer_(b->cb, &bi);
    cCmdResetQueryPool(b->cb, cPool, (uint32_t)(b - cBuf) * C_QPB, C_QPB);
    b->dst = dst; b->nq = 0;
    return b;
}
static void CStamp(CBuf* b, int i)                  // as the rig and the game's probe stamp: when everything before it has left the pipeline
{
    cCmdWriteTimestamp(b->cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, cPool, (uint32_t)(b - cBuf) * C_QPB + (uint32_t)i);
    if (i + 1 > b->nq) b->nq = i + 1;
}
// One buffer, one submit: it may wait for a timeline's value on the queue, it signals the ring's own timeline, and one more when asked.
static bool CSubmit(CBuf* b, VkSemaphore wait, uint64_t waitVal, VkSemaphore sig, uint64_t sigVal)
{
    vkEndCommandBuffer_(b->cb);
    b->val = ++cVal;
    VkSemaphore sems[2] = { cLocal, sig }; uint64_t vals[2] = { b->val, sigVal };
    const uint32_t ns = sig != VK_NULL_HANDLE ? 2u : 1u;
    VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
    tl.signalSemaphoreValueCount = ns; tl.pSignalSemaphoreValues = vals;
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.pNext = &tl; si.commandBufferCount = 1; si.pCommandBuffers = &b->cb; si.signalSemaphoreCount = ns; si.pSignalSemaphores = sems;
    if (wait != VK_NULL_HANDLE) { tl.waitSemaphoreValueCount = 1; tl.pWaitSemaphoreValues = &waitVal; si.waitSemaphoreCount = 1; si.pWaitSemaphores = &wait; si.pWaitDstStageMask = &stage; }
    if (vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS) { cWhy = "the queue refused a submit of this side's"; return false; }
    return true;
}
static bool CWait(const CBuf* b) { if (FeedVkWaitTimeline(&gVk, cLocal, b->val, 5000)) return true; cWhy = "a buffer of this side's did not retire in 5 s"; return false; }
static bool CDrain()                                // nothing of this side's is on the queue, and every stamp is read
{
    bool ok = !cVal || FeedVkWaitTimeline(&gVk, cLocal, cVal, 10000);
    if (!ok) cWhy = "this side's queue did not drain in 10 s";
    for (int i = 0; i < C_RING && ok; i++) ok = CRetire(&cBuf[i]);
    return ok;
}
// An image of this program's own: plain, device-local, dedicated, as DXVK makes a render target (tools\b2-shared, MakePlain).
static Img CMakeImage(UINT w, UINT h, VkImageUsageFlags usage, VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT)
{
    Img i; i.w = w; i.h = h;
    VkImageCreateInfo ici = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ici.imageType = VK_IMAGE_TYPE_2D; ici.format = format; ici.extent = { w, h, 1 }; ici.mipLevels = 1; ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT; ici.tiling = VK_IMAGE_TILING_OPTIMAL; ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; ici.usage = usage;
    NEED(gVk.CreateImage(gDev, &ici, nullptr, &i.img) == VK_SUCCESS, "vkCreateImage for the cost bench");
    VkMemoryRequirements q; gVk.GetImageMemoryRequirements(gDev, i.img, &q);
    VkMemoryDedicatedAllocateInfo ded = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO }; ded.image = i.img;
    VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.pNext = &ded; mi.allocationSize = q.size;
    mi.memoryTypeIndex = MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &i.mem) == VK_SUCCESS, "vkAllocateMemory for the cost bench");
    NEED(gVk.BindImageMemory(gDev, i.img, i.mem, 0) == VK_SUCCESS, "vkBindImageMemory for the cost bench");
    return i;
}
// A w x h piece of the picture after it moved by (sx, sy) in all, out of the image it was uploaded into: TPut's
// arithmetic (what the image shows at (x, y) is the picture at (x - sx, y - sy)), as a copy between two images.
static void CPicture(VkCommandBuffer cb, const Img& atlas, UINT period, long sx, long sy, VkImage dst, UINT w, UINT h)
{
    const long P = (long)period;
    VkImageCopy c = {};
    c.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; c.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    c.srcOffset = { (int32_t)(((-sx) % P + P) % P), (int32_t)(((-sy) % P + P) % P), 0 }; c.extent = { w, h, 1 };
    gVk.CmdCopyImage(cb, atlas.img, VK_IMAGE_LAYOUT_GENERAL, dst, VK_IMAGE_LAYOUT_GENERAL, 1, &c);
}
// THE UPSCALER'S SET AS A REAL BUILD (--fake-cost-upscale=1): TRebuild's lines with transport 0, so the helper makes its
// upscaler; with guides smaller than the display the Output slot is the display's size (DLSS Super Resolution, or FSR).
// What the helper answered is said by the caller.
static bool CRebuildReal(UINT rw, UINT rh, FeedBuildAck* back)
{
    for (int i = 0; i < FEED_SLOTS; i++)
    {
        if (gIm[i].img) gVk.DestroyImage(gDev, gIm[i].img, nullptr);
        if (gIm[i].mem) gVk.FreeMemory(gDev, gIm[i].mem, nullptr);
    }
    const bool up = rw != W || rh != H;
    FeedBuild b = {}; b.width = rw; b.height = rh; b.color_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT; b.output_fmt = (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1; b.transport = 0; b.mv_scale_x = 1.0f; b.mv_scale_y = 1.0f;
    if (up) { b.target_width = W; b.target_height = H; }
    *back = FeedBuildAck(); const uint8_t tagB = 'B';
    if (!(PipeWrite(&tagB, 1) && PipeWrite(&b, sizeof(b)) && PipeRead(back, sizeof(*back), 120000) && back->ok == 1)) return false;
    const VkFormat fm[FEED_SLOTS] = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16G16_SFLOAT };
    bool ok = true;
    for (int i = 0; i < FEED_SLOTS; i++)
    {
        const UINT iw = (i == FEED_OUTPUT && up) ? W : rw, ih = (i == FEED_OUTPUT && up) ? H : rh;
        tIm[i] = Img(); tIm[i].w = iw; tIm[i].h = ih;
        ok = ok && back->tex[i] && FeedVkImportImage(&gVk, (HANDLE)(uintptr_t)back->tex[i], iw, ih, fm[i], i == FEED_OUTPUT, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, true, &tIm[i].img, &tIm[i].mem);
        if (back->tex[i]) CloseHandle((HANDLE)(uintptr_t)back->tex[i]);     // the import duplicated it
    }
    if (!ok) return false;
    Begin();
    { VkImage all[FEED_SLOTS]; for (int i = 0; i < FEED_SLOTS; i++) all[i] = tIm[i].img; FeedVkBarrierN(&gVk, gCb, all, FEED_SLOTS, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); }
    Clear(tIm[FEED_DEPTH], 0.0f, 0.0f, 0.0f, 0.0f);
    if (!Submit(VK_NULL_HANDLE, 0)) return false;
    gIm = tIm;
    return true;
}
// THE SAME PIECE INTO THE OFF-SCREEN BACK BUFFER, which with --fake-cost-fmt=1 is 8-bit: a copy cannot change the format,
// so there it is a blit of the same rectangle, one texel to one texel (nearest). The picture's values are below 1.
static void CPresent(VkCommandBuffer cb, const Img& atlas, UINT period, long sx, long sy, VkImage dst, UINT w, UINT h)
{
    if (!co.fmt) { CPicture(cb, atlas, period, sx, sy, dst, w, h); return; }
    const long P = (long)period;
    const int32_t x0 = (int32_t)(((-sx) % P + P) % P), y0 = (int32_t)(((-sy) % P + P) % P);
    VkImageBlit bl = {};
    bl.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; bl.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    bl.srcOffsets[0] = { x0, y0, 0 }; bl.srcOffsets[1] = { x0 + (int32_t)w, y0 + (int32_t)h, 1 };
    bl.dstOffsets[0] = { 0, 0, 0 };   bl.dstOffsets[1] = { (int32_t)w, (int32_t)h, 1 };
    gVk.CmdBlitImage(cb, atlas.img, VK_IMAGE_LAYOUT_GENERAL, dst, VK_IMAGE_LAYOUT_GENERAL, 1, &bl, VK_FILTER_NEAREST);
}
// THE LOAD: m full-screen copies between this program's own two images, there and back, a barrier between two of them
// (each reads what the one before wrote). 63.3 MiB a copy, the size of one of the game's back buffer copies.
static void CLoadCopies(VkCommandBuffer cb, int m)
{
    VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    for (int k = 0; k < m; k++)
    {
        FeedVkCopyImage(&gVk, cb, cLoadImg[k & 1].img, VK_IMAGE_LAYOUT_GENERAL, cLoadImg[(k + 1) & 1].img, VK_IMAGE_LAYOUT_GENERAL, W, H);
        gVk.CmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    }
}
// THE BACK BUFFER'S BARRIER ROUND A COPY OUT OF IT: source\fg\fg_product.h, FgpBbBarrier, with the masks of
// source\transport\DlssIvTransport.h (kIvtGameWriteStages and its three neighbours) written out, since this program
// includes neither. The back buffer lives in GENERAL under DXVK, so no layout changes: an execution and memory dependency.
static void CBbBarrier(VkCommandBuffer cb, bool into)
{
    const VkAccessFlags gameWrite = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    const VkAccessFlags gameUse = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    const VkPipelineStageFlags gameWriteSt = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    const VkPipelineStageFlags gameUseSt = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.srcAccessMask = into ? gameWrite : VK_ACCESS_TRANSFER_READ_BIT; b.dstAccessMask = into ? VK_ACCESS_TRANSFER_READ_BIT : gameUse;
    b.oldLayout = VK_IMAGE_LAYOUT_GENERAL; b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = cBb.img; b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    gVk.CmdPipelineBarrier(cb, into ? gameWriteSt : VK_PIPELINE_STAGE_TRANSFER_BIT, into ? VK_PIPELINE_STAGE_TRANSFER_BIT : gameUseSt, 0, 0, nullptr, 0, nullptr, 1, &b);
}
// ONE OF THE PRODUCT'S COPIES, AS THE PRODUCT RECORDS IT (fg_product.h: RecBegin's memory barrier, then RecCopyToImg), with
// four stamps: the buffer's head, behind the acquire (before the copy), behind the copy, and the buffer's end.
static void CProductCopy(CBuf* b, VkImage dst)
{
    CStamp(b, 0);
    VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    gVk.CmdPipelineBarrier(b->cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
    CBbBarrier(b->cb, true);
    FeedVkExternalTransferN(&gVk, b->cb, &dst, 1, gFam, false /*acquire*/);
    CStamp(b, 1);
    FeedVkCopyImage(&gVk, b->cb, cBb.img, VK_IMAGE_LAYOUT_GENERAL, dst, VK_IMAGE_LAYOUT_GENERAL, W, H);
    CStamp(b, 2);
    FeedVkExternalTransferN(&gVk, b->cb, &dst, 1, gFam, true /*release*/);
    CBbBarrier(b->cb, false);
    CStamp(b, 3);
}

// ---------------------------------------------------------------- what a frame leaves behind, and the generated image
struct CRow
{
    uint64_t n; int arm, rep, half, idx; uint32_t flags;        // flags: 1 'G' said DISPATCH, 2 GEN was read behind it (its neighbours' timing is spoilt),
                                                                // 4 the 'Q' was told to reset, 8 NO SOURCE by the product's rule, 16 a B half (the arm, not the baseline)
    int move;                                                   // which of the picture's six moves this frame made (-1: it stood still): AMD's cost follows it
    uint64_t sc[2], hm[2], c1[4], c2[4], end[2];                // GPU ticks: the scene, the home and post chain, the two copies, the frame's end (an empty bracket)
    LONGLONG tStart, tQ, tG, tOut, tEnd;                        // the CPU's clock: the frame's start, 'Q' written, 'G' written, fgOut seen (idle only), the frame's last submit
    LONGLONG tF;                                                // the frame message written (the order part reads it; LAST: the rows' file has no column for it)
};
struct CObs { double mid = 0, cur = 0, prev = 0, mix = 0, change = 0; int pairs = 0, dumped = 0; };
// GEN's four tiles behind a dispatched pair: the centre, the upper left quarter, and two on the picture's edges (the right
// one and the bottom one, where AMD's inpainting works). The first two are compared with the four pictures this program
// can compute (TObserve's arithmetic); all four go to the tiles' file when the pair is one of the chosen.
// (pS, pT): where the presented picture stood at the last pair that was DISPATCHED: AMD's "previous" is that one.
static bool CObserve(CObs* o, const TStage& s, long cS_, long cT_, long pS_, long pT_, bool dump, int arm, int rep, int idx, uint64_t n)
{
    const Img gen = SetImg(FEED_FG_GEN);
    const UINT tx[C_TILES] = { W / 2 - 32, W / 4 - 32, W - 64, W / 2 - 32 }, ty[C_TILES] = { H / 2 - 32, H / 4 - 32, H / 2 - 32, H - 64 };
    Begin();
    FeedVkBarrierN(&gVk, gCb, &gen.img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg[C_TILES] = {};
    for (int k = 0; k < C_TILES; k++)
    {
        rg[k].bufferOffset = (VkDeviceSize)k * C_TILE_BYTES; rg[k].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        rg[k].imageOffset = { (int32_t)tx[k], (int32_t)ty[k], 0 }; rg[k].imageExtent = { 64, 64, 1 };
    }
    vkCmdCopyImageToBuffer_(gCb, gen.img, VK_IMAGE_LAYOUT_GENERAL, cObsBuf, C_TILES, rg);
    if (!Submit(VK_NULL_HANDLE, 0)) { cWhy = "the read of GEN was not submitted"; return false; }
    for (int k = 0; k < 2; k++)
    {
        const uint16_t* px = (const uint16_t*)cObsPtr + (size_t)k * 64 * 64 * 4;
        for (UINT y = 0; y < 64; y++) for (UINT x = 0; x < 64; x++)
        {
            const double g = THalfToFloat(px[4 * (y * 64 + x)]);
            const double c = TAt(s, cS_, cT_, tx[k] + x, ty[k] + y), p = TAt(s, pS_, pT_, tx[k] + x, ty[k] + y), m = TAt(s, (cS_ + pS_) / 2, (cT_ + pT_) / 2, tx[k] + x, ty[k] + y);
            o->mid += fabs(g - m); o->cur += fabs(g - c); o->prev += fabs(g - p); o->mix += fabs(g - 0.5 * (c + p)); o->change += fabs(c - p);
        }
    }
    o->pairs++;
    if (dump && cGen)
    {
        struct { char magic[4]; int32_t arm, rep, idx; uint64_t n; int32_t tiles, bytes; } hd = { { 'G', 'E', 'N', 'T' }, arm, rep, idx, n, C_TILES, C_TILE_BYTES };
        fwrite(&hd, sizeof(hd), 1, cGen); fwrite(cObsPtr, 1, (size_t)C_TILES * C_TILE_BYTES, cGen);
        o->dumped++;
    }
    return true;
}

// (2026-10-14, the over part) CObserve's two halves apart, so that the read of GEN can ride IN A BUFFER OF THE FRAME'S OWN, in the
// queue's order, with no wait on the CPU between the pair and the next pair's copies: the copy of the four tiles recorded
// into a buffer the caller submits; and, once that buffer has retired, the first two tiles held against the pictures.
static void CObsRecord(VkCommandBuffer cb)
{
    const Img gen = SetImg(FEED_FG_GEN);
    const UINT tx[C_TILES] = { W / 2 - 32, W / 4 - 32, W - 64, W / 2 - 32 }, ty[C_TILES] = { H / 2 - 32, H / 4 - 32, H / 2 - 32, H - 64 };
    FeedVkBarrierN(&gVk, cb, &gen.img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg[C_TILES] = {};
    for (int k = 0; k < C_TILES; k++)
    {
        rg[k].bufferOffset = (VkDeviceSize)k * C_TILE_BYTES; rg[k].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        rg[k].imageOffset = { (int32_t)tx[k], (int32_t)ty[k], 0 }; rg[k].imageExtent = { 64, 64, 1 };
    }
    vkCmdCopyImageToBuffer_(cb, gen.img, VK_IMAGE_LAYOUT_GENERAL, cObsBuf, C_TILES, rg);
}
static void CObsCompare(CObs* o, const TStage& s, long cS_, long cT_, long pS_, long pT_)
{
    const UINT tx[C_TILES] = { W / 2 - 32, W / 4 - 32, W - 64, W / 2 - 32 }, ty[C_TILES] = { H / 2 - 32, H / 4 - 32, H / 2 - 32, H - 64 };
    for (int k = 0; k < 2; k++)
    {
        const uint16_t* px = (const uint16_t*)cObsPtr + (size_t)k * 64 * 64 * 4;
        for (UINT y = 0; y < 64; y++) for (UINT x = 0; x < 64; x++)
        {
            const double g = THalfToFloat(px[4 * (y * 64 + x)]);
            const double c = TAt(s, cS_, cT_, tx[k] + x, ty[k] + y), p = TAt(s, pS_, pT_, tx[k] + x, ty[k] + y), m = TAt(s, (cS_ + pS_) / 2, (cT_ + pT_) / 2, tx[k] + x, ty[k] + y);
            o->mid += fabs(g - m); o->cur += fabs(g - c); o->prev += fabs(g - p); o->mix += fabs(g - 0.5 * (c + p)); o->change += fabs(c - p);
        }
    }
    o->pairs++;
}
// The images the set in hand has: three, or two when it was asked without the HUD-less one (--fake-cost-hud=0)
static uint32_t CBuildFlags() { return co.hud == 0 ? (uint32_t)FEED_FG_BUILD_NO_HUDLESS : 0u; }
static uint32_t CSetImages() { return gSet.nAll; }   // (wire 3: the images the set holds, over FgxSet::all)
// ---------------------------------------------------------------- ONE FRAME
// How the picture moves, a frame after a frame, in render pixels: mode 20's rule (never twice the same in a row), and
// every move a multiple of four, so that 3/2 of it (the presented move at Quality) is even and half of that is whole
// pixels: the half-way picture is then one this program has, at both guide sizes.
static const int kCMove[6][2] = { { 8, 4 }, { 12, -4 }, { 4, 4 }, { -4, 8 }, { 24, 20 }, { 4, -8 } };
// serial: the idle way (every part waited for before the next). m1, m2: the load's copies before the injection and behind
// the home. rule: the product's NO SOURCE test at the hand. observe: read GEN behind this pair's answer.
static bool CFrame(CRow* r, int arm, int rep, int half, int idx, bool isB, bool serial, int m1, int m2, bool rule, CObs* obs, bool observe, bool dump)
{
    const uint64_t n = cN++;
    r->n = n; r->arm = arm; r->rep = rep; r->half = half; r->idx = idx; r->flags = isB ? 16u : 0u;
    const int kind = !isB ? CA_A : (arm == CA_F2 || arm == CA_FX) ? CA_F : (arm == CA_NULL || arm == CA_LK || arm == CA_OFF) ? CA_A : arm;   // (FK and FC are their own kinds)
    const bool still = kind == CA_FS;
    const int dx = still ? 0 : kCMove[cStep % 6][0], dy = still ? 0 : kCMove[cStep % 6][1];
    r->move = still ? -1 : cStep % 6;
    if (!still) cStep++;
    cS += dx; cT += dy;
    const float vx = kind == CA_FN ? (float)dx : (float)-dx, vy = kind == CA_FN ? (float)dy : (float)-dy;   // the game's vectors: previous minus current
    const long pn = co.guides ? 3 : 1, pd = co.guides ? 2 : 1;        // presented pixels a render pixel
    const long qS = cS * pn / pd, qT = cT * pn / pd;                  // where the presented picture stands
    const TStage& pres = co.guides ? tWide : tPlain; const Img& presAtlas = co.guides ? cAtlasWide : cAtlas;
    r->tStart = CQpc();
    // ONE FRAME IN FLIGHT: this frame starts when the frame before the last has ended on the GPU
    if (!serial && cEnd[(n - 2) & 7] && FeedVkTimelineValue(&gVk, cLocal) < cEnd[(n - 2) & 7] && !FeedVkWaitTimeline(&gVk, cLocal, cEnd[(n - 2) & 7], 5000))
    { cWhy = "the frame before the last did not end in 5 s"; return false; }
    if (m1 > 0)
    {   // THE SCENE: the first half of the load
        CBuf* b = CTake(r->sc); if (!b) return false;
        CStamp(b, 0); CLoadCopies(b->cb, m1); CStamp(b, 1);
        if (!CSubmit(b, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0)) return false;
    }
    // THE GUARD (the product's ShadowBeforeInject, by the client's own line): the last prepare reads the guides the injection
    // is about to write, so the injection goes behind fgPrep on this queue when that prepare has not retired
    if (gSet.have && cLastQ && FeedVkTimelineValue(&gVk, gSet.semPrep) < cLastQ) { if (FgxPrepWait<FakeB>(&gSet, &gWaits, cLastQ)) cGuards++; }
    CBuf* b = CTake(nullptr); if (!b) return false;
    {   // THE INJECTION: this frame's picture into Color, its vectors into the motion vector image, the in-fence in that submit
        CPicture(b->cb, cAtlas, tPlain.period, cS, cT, gIm[FEED_COLOR].img, cRw, cRh);
        VkClearColorValue c; c.float32[0] = vx; c.float32[1] = vy; c.float32[2] = 0.0f; c.float32[3] = 0.0f;
        VkImageSubresourceRange rg = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdClearColorImage_(b->cb, gIm[FEED_MV].img, VK_IMAGE_LAYOUT_GENERAL, &c, 1, &rg);
        if (kind == CA_FC && cMvBuf != VK_NULL_HANDLE)
        {   // THE RIGHT HALF'S VECTORS NEGATED: at the seam the two halves' pixels are sent onto the same targets
            const uint32_t two = (uint32_t)Half(-vx) | ((uint32_t)Half(-vy) << 16);   // one R16G16 texel
            const VkDeviceSize bytes = (VkDeviceSize)(cRw / 2) * cRh * 4u;
            cCmdFillBuffer(b->cb, cMvBuf, 0, bytes, two);
            VkMemoryBarrier mb = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
            mb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; mb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            gVk.CmdPipelineBarrier(b->cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &mb, 0, nullptr, 0, nullptr);
            VkBufferImageCopy half = {}; half.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            half.imageOffset = { (int32_t)(cRw / 2), 0, 0 }; half.imageExtent = { cRw / 2, cRh, 1 };
            tCopyBufferToImage(b->cb, cMvBuf, gIm[FEED_MV].img, VK_IMAGE_LAYOUT_GENERAL, 1, &half);
        }
    }
    if (!CSubmit(b, VK_NULL_HANDLE, 0, gSemIn, n)) return false;
    FeedTaggedFrameIvt f = {}; f.tag = 'F'; f.fm.n = n; f.fm.frame_time_ms = 16.67f; f.fm.camera_near = 0.05f; f.fm.fov_y_rad = 0.7853981f;
    if (!PipeWrite(&f, sizeof(f))) { cWhy = "the pipe refused a frame message"; return false; }
    r->tF = CQpc();
    // LEVER 2 (2026-10-10; --fake-cost-qat=1): 'Q' RIGHT BEHIND THE FRAME MESSAGE, as the product's machine writes it since
    // (fg_machine.h, ShadowAfterInject): ahead of the home's submit, with the reasons known here (the gap). The hand below
    // then writes none, and a cut rides the pair's 'G' alone.
    bool qEarly = false;
    if (co.qat == 1 && kind != CA_A && gSet.have)
    {
        if (!SendQ(n, cGap ? FEED_FG_Q_RESET_GAP : 0u)) { cWhy = "the pipe refused a 'Q'"; return false; }
        cLastQ = n; r->tQ = CQpc(); qEarly = true;
    }
    // (--fake-cost-fq=1: a pair's 'Q' goes out right behind this 'F', and the frame's answer and the home are waited for behind it)
    const bool fq = serial && co.fq && kind != CA_A && gSet.have;
    if (serial && !fq && !FeedVkWaitTimeline(&gVk, gSemOut, n, 5000)) { cWhy = "a frame was not answered in 5 s"; return false; }
    // THE HOME AND THE POST CHAIN: a submit that waits for THIS frame's answer on the queue, as the game's same-frame home
    // does; the second half of the load; and at its end the back buffer holds the picture the game would present
    // (2026-10-14) THE HAND'S COPIES, as a piece that can be submitted here or there: the product's place is the hand, behind the
    // home; THE OVER PART'S CONTROL submits them HERE, AHEAD of the home, where nothing holds them behind the dispatch before.
    // (In the control the back buffer still holds the frame before's picture, which is what a copy that lands under that
    // frame's dispatch would carry on the wire too: the control is about the WRITE of the helper's images, not its content.)
    auto handCopies = [&]() -> bool
    {
        CBuf* c = CTake(r->c1); if (!c) return false;
        CProductCopy(c, gSet.img[FEED_FG_PRESENT]);
        if (!CSubmit(c, VK_NULL_HANDLE, 0, co.hud == 1 ? VK_NULL_HANDLE : gSet.semIn, n)) return false;
        if (serial && !CWait(c)) return false;
        if (co.hud == 1)
        {
            c = CTake(r->c2); if (!c) return false;
            CProductCopy(c, gSet.img[FEED_FG_HUDLESS]);
            if (!CSubmit(c, VK_NULL_HANDLE, 0, gSet.semIn, n)) return false;
            if (serial && !CWait(c)) return false;
        }
        return true;
    };
    const bool copiesEarly = cOverCtl && !cOverRef && !serial && kind != CA_A && kind != CA_P && gSet.have;
    if (copiesEarly)
    {   // (the control: this frame's picture into the back buffer first, so that what the early copies carry is right and only WHEN they land is not)
        CBuf* p = CTake(nullptr); if (!p) return false;
        CPresent(p->cb, presAtlas, pres.period, qS, qT, cBb.img, W, H);
        if (!CSubmit(p, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0)) return false;
        if (!handCopies()) return false;
    }
    b = CTake(r->hm); if (!b) return false;
    if (cOverObs && !serial) CObsRecord(b->cb);               // (2026-10-14: the image of the pair before, read back in the queue's order: this buffer waits for this frame's answer)
    CStamp(b, 0); CLoadCopies(b->cb, m2); CPresent(b->cb, presAtlas, pres.period, qS, qT, cBb.img, W, H); CStamp(b, 1);
    if (!CSubmit(b, gSemOut, n, VK_NULL_HANDLE, 0)) return false;
    if (serial && !fq && !CWait(b)) return false;
    CBuf* const home = b;
    cHomeBuf = b;
    // (the order part: the worker's time on the rest of the frame, between its injection and its hand; the clock is watched)
    if (cOrderOn && co.ogap > 0) { LARGE_INTEGER qf; QueryPerformanceFrequency(&qf); const LONGLONG until = r->tF + (LONGLONG)co.ogap * qf.QuadPart / 1000000; while (CQpc() < until) YieldProcessor(); }
    // THE HAND
    if (kind != CA_A && gSet.have)
    {
        uint32_t qf = cGap ? FEED_FG_Q_RESET_GAP : 0u;                // the first pair after one with no dispatch, as the product's machine says it
        if (kind == CA_FR || (kind == CA_F1 && !(idx & 1)) || (kind == CA_FK && idx % C_FK_EVERY == 0)) qf |= FEED_FG_Q_RESET_CUT;
        if (qf) r->flags |= 4u;
        // WIRE 2: the pair's reasons ride its 'G' too (gWhy); with --fake-cost-cutg=1 the cut rides the 'G' ALONE
        const uint32_t gWhy = qf;
        if (co.cutg || qEarly) qf &= ~(uint32_t)FEED_FG_Q_RESET_CUT;
        // THE PRODUCT'S NO SOURCE TEST (fg_machine.h, G5): one counter read; the last dispatch unanswered means no copy
        const bool source = !(rule && cLastDisp && FeedVkTimelineValue(&gVk, gSet.semOut) < 2 * cLastDisp);
        if (!source) r->flags |= 8u;
        if (!qEarly)
        {
            if (!SendQ(n, qf)) { cWhy = "the pipe refused a 'Q'"; return false; }
            cLastQ = n; r->tQ = CQpc();
        }
        if (fq)
        {   // the frame's answer and the home, which the 'Q' did not wait for
            if (!FeedVkWaitTimeline(&gVk, gSemOut, n, 5000)) { cWhy = "a frame was not answered in 5 s"; return false; }
            if (!CWait(home)) return false;
        }
        if (serial && !(FeedVkWaitTimeline(&gVk, gSet.semPrep, n, 5000) && Seen(gSet.semPrep, &gSeenPrep) >= n)) { cWhy = "a 'Q' was not answered in 5 s"; return false; }
        uint32_t g = FEED_FG_G_NOSOURCE;
        if (kind != CA_P && source)
        {
            // (32, 2026-10-14: these copies are submitted WITH THE DISPATCH BEFORE STILL UNANSWERED, as the hand's CPU test reads it)
            if (cOverRef && cLastDisp && !FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * cLastDisp, 5000)) { cWhy = "the dispatch before was not answered in 5 s (the over part's reference stretch)"; return false; }
            if (co.over && cLastDisp && FeedVkTimelineValue(&gVk, gSet.semOut) < 2 * cLastDisp) r->flags |= 32u;   // (read in the over part alone: no other part's hand pays for it)
            if (!copiesEarly && !handCopies()) return false;
            g = kind == CA_PC ? FEED_FG_G_WITHHELD : FEED_FG_G_DISPATCH;
        }
        if (!SendGx(n, g, gWhy)) { cWhy = "the pipe refused a 'G'"; return false; }
        r->tG = CQpc();
        if (cOrdOn && !serial && g == FEED_FG_G_DISPATCH)
        {   // (2026-10-14) THE ORDER WAIT, right behind the hand's submits on this queue: the product's own line (1), or the direct form (2)
            const LONGLONG o0 = CQpc();
            bool took = false;
            if (cOrdOn == 1) took = FgxOrdWait(&cOrd, n, &FakeB::QueueSubmit);
            else
            {
                uint64_t v = 2 * n;
                VkTimelineSemaphoreSubmitInfo tl = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
                tl.waitSemaphoreValueCount = 1; tl.pWaitSemaphoreValues = &v;
                const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
                si.pNext = &tl; si.waitSemaphoreCount = 1; si.pWaitSemaphores = &gSet.semOut; si.pWaitDstStageMask = &stage;
                took = vkQueueSubmit_(gQ, 1, &si, VK_NULL_HANDLE) == VK_SUCCESS;
            }
            cOrdSubT += CQpc() - o0; cOrdSubN++;
            if (!took) { cWhy = "the queue refused the order wait"; return false; }
        }
        const bool dispatched = g == FEED_FG_G_DISPATCH;
        if (dispatched) { r->flags |= 1u; cLastDisp = n; }
        cGap = !dispatched;
        if (serial || (dispatched && observe))
        {
            if (!FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 5000)) { cWhy = "a 'G' was not answered in 5 s"; return false; }
            const uint64_t out = Seen(gSet.semOut, &gSeenOut);
            r->tOut = CQpc();
            if (out < 2 * n || (serial && out != (dispatched ? 2 * n : 2 * n + 1))) { cWhy = "fgOut does not read what this pair's 'G' asked for"; return false; }
        }
        if (dispatched && observe)
        {
            if (!CObserve(obs, pres, qS, qT, cPrevPS, cPrevPT, dump, arm, rep, idx, n)) return false;
            r->flags |= 2u;
        }
        if (dispatched) { cPrevPS = qS; cPrevPT = qT; }
    }
    else cGap = true;                                                 // no 'Q' at all: the next pair that is sent is the first after a gap
    // THE FRAME'S END: one more submit, as the game's frame-end marker; its two stamps are an empty bracket
    b = CTake(r->end); if (!b) return false;
    CStamp(b, 0); CStamp(b, 1);
    if (!CSubmit(b, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0)) return false;
    cEnd[n & 7] = b->val;
    cEndBuf = b;
    if (serial && !CWait(b)) return false;
    r->tEnd = CQpc();
    return true;
}
// ALL OF GEN, HASHED, behind a dispatched pair that was waited for. Three kinds of hash, each a pair of 32-bit runs over the
// image's 32-bit words (this program is 32-bit: no 64-bit multiply a pixel): every channel; the three colours alone (alpha
// is nobody's picture, and a hash that moved with it would cry wolf); and each block of a 16 by 9 grid, colours alone, so
// that two images that differ say WHERE and over how much of the picture.
enum { C_HGX = 16, C_HGY = 9 };
static bool CHashGen(const char* armName, int idx, uint64_t n)
{
    const Img gen = SetImg(FEED_FG_GEN);
    Begin();
    FeedVkBarrierN(&gVk, gCb, &gen.img, 1, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    VkBufferImageCopy rg = {}; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { W, H, 1 };
    vkCmdCopyImageToBuffer_(gCb, gen.img, VK_IMAGE_LAYOUT_GENERAL, cHashBuf, 1, &rg);
    if (!Submit(VK_NULL_HANDLE, 0)) { cWhy = "the read of the whole of GEN was not submitted"; return false; }
    uint32_t a1 = 0x811C9DC5u, a2 = 0x9E3779B9u, c1 = 0x811C9DC5u, c2 = 0x9E3779B9u;
    static uint32_t blk[C_HGX * C_HGY];
    for (int k = 0; k < C_HGX * C_HGY; k++) blk[k] = 0x811C9DC5u;
    const uint32_t* px = (const uint32_t*)cHashPtr;
    const UINT bw = W / C_HGX, bh = H / C_HGY;
    if (co.fmt) for (UINT y = 0; y < H; y++)
    {   // B8G8R8A8: one 32-bit word a pixel, blue lowest and alpha highest; the three colours alone are its low 24 bits
        const uint32_t* row = px + (size_t)y * W; uint32_t* brow = blk + (size_t)(y / bh < C_HGY ? y / bh : C_HGY - 1) * C_HGX;
        for (UINT bx = 0; bx < C_HGX; bx++)
        {
            uint32_t hb = brow[bx];
            const UINT x1 = bx == C_HGX - 1 ? W : (bx + 1) * bw;
            for (UINT x = bx * bw; x < x1; x++)
            {
                const uint32_t v = row[x], col = v & 0x00FFFFFFu;
                a1 = (a1 ^ v) * 0x01000193u; a2 = _rotl(a2 + v, 5) * 0x85EBCA6Bu;
                c1 = (c1 ^ col) * 0x01000193u; c2 = _rotl(c2 + col, 5) * 0x85EBCA6Bu;
                hb = (hb ^ col) * 0x01000193u;
            }
            brow[bx] = hb;
        }
    }
    else for (UINT y = 0; y < H; y++)
    {
        const uint32_t* row = px + (size_t)y * W * 2u; uint32_t* brow = blk + (size_t)(y / bh < C_HGY ? y / bh : C_HGY - 1) * C_HGX;
        for (UINT bx = 0; bx < C_HGX; bx++)
        {
            uint32_t hb = brow[bx];
            const UINT x1 = bx == C_HGX - 1 ? W : (bx + 1) * bw;
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
    if (cHash)
    {
        fprintf(cHash, "%s,%d,%llu,%08x%08x,%08x%08x,", armName, idx, (unsigned long long)n, a1, a2, c1, c2);
        for (int k = 0; k < C_HGX * C_HGY; k++) fprintf(cHash, "%08x", blk[k]);
        fputc('\n', cHash);
    }
    // THE WHOLE IMAGE TO A FILE, for the pairs --fake-cost-raw names (ARM:index, exactly): its bytes as they were read
    if (co.raw[0] && co.rawOut[0])
    {
        char want[48]; const int wl = sprintf_s(want, "%s:%d", armName, idx);
        bool named = false;
        for (const char* p = co.raw; *p && !named; )
        {
            const char* e = p; while (*e && *e != ',') e++;
            named = (int)(e - p) == wl && strncmp(p, want, (size_t)wl) == 0;
            p = *e ? e + 1 : e;
        }
        if (named)
        {
            char path[MAX_PATH]; sprintf_s(path, "%s-%s-%d.bin", co.rawOut, armName, idx);
            FILE* f = nullptr; fopen_s(&f, path, "wb");
            const size_t bytes = (size_t)W * H * (co.fmt ? 4u : 8u);
            if (!f || fwrite(cHashPtr, 1, bytes, f) != bytes) { if (f) fclose(f); cWhy = "a whole generated image could not be written to its file"; return false; }
            fclose(f);
        }
    }
    return true;
}
static double CSpanUs(const uint64_t* t, int a, int b) { return (t[a] && t[b] >= t[a]) ? (double)(t[b] - t[a]) * cNs / 1000.0 : -1.0; }
static void CRowsWrite(const char* part, const CRow* rows, int count)
{
    if (!cOut) return;
    for (int i = 0; i < count; i++)
    {
        const CRow& r = rows[i];
        fprintf(cOut, "%s,%s,%d,%d,%d,%llu,%u,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%llu,%lld,%lld,%lld,%lld,%lld\n", part, kCArm[r.arm], r.rep, r.half, r.idx, (unsigned long long)r.n, r.flags, r.move,
                CSpanUs(r.sc, 0, 1), CSpanUs(r.hm, 0, 1), CSpanUs(r.c1, 1, 2), CSpanUs(r.c1, 0, 3), CSpanUs(r.c2, 1, 2), CSpanUs(r.c2, 0, 3), CSpanUs(r.end, 0, 1),
                (unsigned long long)r.end[1], (long long)r.tStart, (long long)r.tQ, (long long)r.tG, (long long)r.tOut, (long long)r.tEnd);
    }
    fflush(cOut);
}
static void CObsSay(char* buf, size_t cap, const CObs& o)
{
    if (!o.pairs) { _snprintf_s(buf, cap, _TRUNCATE, "obs=0"); return; }
    if (!(o.change > 0.0)) { _snprintf_s(buf, cap, _TRUNCATE, "obs=%d still=1 mid=%.6f cur=%.6f", o.pairs, o.mid / (o.pairs * 8192.0), o.cur / (o.pairs * 8192.0)); return; }
    _snprintf_s(buf, cap, _TRUNCATE, "obs=%d mid=%.4f cur=%.4f prev=%.4f mix=%.4f dumped=%d", o.pairs, o.mid / o.change, o.cur / o.change, o.prev / o.change, o.mix / o.change, o.dumped);
}
static double CMedian(double* v, int n)
{
    for (int i = 1; i < n; i++) { const double x = v[i]; int j = i; for (; j > 0 && v[j - 1] > x; j--) v[j] = v[j - 1]; v[j] = x; }
    return n ? v[n / 2] : 0.0;
}
// The baseline frame's period at a load of m copies, in microseconds: frames of arm A from an empty queue, the first 32 left out.
// THEIR MEAN, the two largest left out, and not their median: on a card the desktop also draws on the period has several
// peaks (measured: about 0.4 ms apart), a median lands on one of them, and the reader's first figure for a half is its mean.
static double CPeriodAt(int m, int frames, CRow* rows)
{
    memset(rows, 0, sizeof(CRow) * (size_t)frames);
    if (!CDrain()) return -1.0;
    for (int i = 0; i < frames; i++) if (!CFrame(&rows[i], CA_A, 0, 0, i, false, false, m / 2, m - m / 2, false, nullptr, false, false)) return -1.0;
    if (!CDrain()) return -1.0;
    static double p[1024]; int k = 0;
    for (int i = 33; i < frames && k < 1024; i++) if (rows[i].end[1] > rows[i - 1].end[1] && rows[i - 1].end[1]) p[k++] = (double)(rows[i].end[1] - rows[i - 1].end[1]) * cNs / 1000.0;
    if (k < 16) return -1.0;
    CMedian(p, k);                                  // (it leaves them in order)
    double sum = 0.0;
    for (int i = 0; i < k - 2; i++) sum += p[i];
    return sum / (double)(k - 2);
}


// ---------------------------------------------------------------- THE ORDER PART (lever 2, 2026-10-10)
// WHERE 'Q' IS WRITTEN, MEASURED: at the hand (the order until 2026-10-10, and the development key fg_q_at_hand) against
// right behind the frame message (the product's order since). --fake-cost-order=N runs N frames of arm F a cell, every
// pair dispatched, three takes of each order in each regime, the two orders alternating:
//   cap59, cap118   the frame PACED on the CPU (16,949 and 8,475 us), no load: the card is not the limit
//   loadUS          --fake-cost-oload=US: the baseline frame made to take that long by the load, one frame in flight:
//                   THE CARD IS THE LIMIT
// BETWEEN THE FRAME MESSAGE AND THE HAND THE CPU WAITS --fake-cost-ogap MICROSECONDS (2000): in the game the worker replays
// the rest of the frame there. Without it this client's hand follows its injection by microseconds and the two orders
// are one order (which is what the loaded arms of 2026-10-06 ran).
// FOUR STAMPERS, as the product's (fg_product.h, FgpStamperMain): a thread a fence that waits on the CPU for the next
// value and reads the clock: the handoff (fgIn), the prepare (fgPrep), the answer (fgOut) and THE FRAME'S OWN out-fence.
// A CELL SAYS, over its frames from the 32nd on: how far the prepare retired BEHIND the handoff (the product's window
// line: the mean over the pairs where it did, and how many of all); K, the handoff's retire to the answer; 'G' written
// to the answer; THE FRAME'S OWN ANSWER, the frame message written to its out-fence; and the frame's period.
enum { C_OST = 8192 };
struct COStamps { LONGLONG in[C_OST], prep[C_OST], fgout[C_OST], fout[C_OST]; volatile LONG quit, late; HANDLE thr[4]; LONGLONG ordT; LONG ordN; volatile LONG ordHold; };
static COStamps* cOs;
static DWORD WINAPI COStamper(void* which)
{
    const int k = (int)(INT_PTR)which;
    const VkSemaphore sem = k == 0 ? gSet.semIn : k == 1 ? gSet.semPrep : k == 2 ? gSet.semOut : gSemOut;
    LONGLONG* dst = k == 0 ? cOs->in : k == 1 ? cOs->prep : k == 2 ? cOs->fgout : cOs->fout;
    uint64_t last = FeedVkTimelineValue(&gVk, sem);
    while (!cOs->quit)
    {
        if (!FeedVkWaitTimeline(&gVk, sem, last + 1, 50)) continue;
        const uint64_t v = FeedVkTimelineValue(&gVk, sem);
        const LONGLONG t = CQpc();
        if (v >= 0x7FFFFFFFull) break;                                // the helper is gone
        if (v <= last) continue;
        if (k == 2)
        {   // fgOut: 2n behind a dispatch, 2n+1 where there was none (then pair n has no stamp)
            for (uint64_t p = last / 2 + 1; p <= v / 2; p++) dst[p & (C_OST - 1)] = t;
            if (v & 1) dst[(v / 2) & (C_OST - 1)] = 0;
            if (v / 2 > last / 2 + 1) _InterlockedIncrement(&cOs->late);
            // (2026-10-14: THIS THREAD STANDS IN FOR THE PLACER: the answer is in, and the order's own fence is signalled up to its pair,
            // from the CPU, through the product's own line; cOs->ordT: this thread's time in that call)
            if (cOrd.sem && !cOs->ordHold) { const LONGLONG s0 = CQpc(); FgxOrdSignal<FakeB>(&cOrd, v / 2); cOs->ordT += CQpc() - s0; cOs->ordN++; }
        }
        else
        {
            for (uint64_t p = last + 1; p <= v; p++) dst[p & (C_OST - 1)] = t;
            if (v > last + 1) _InterlockedIncrement(&cOs->late);
        }
        last = v;
    }
    return 0;
}
static double COPct(double* v, int n, int pct) { if (!n) return 0.0; CMedian(v, n); int i = n * pct / 100; if (i >= n) i = n - 1; return v[i]; }
// the load of m copies that makes the baseline frame take target microseconds (the loaded part's own search)
static bool COrderLoad(int target, int* mOut, double* pOut)
{
    CRow* cal = (CRow*)malloc(sizeof(CRow) * 224); if (!cal) return false;
    const double pa = CPeriodAt(24, 160, cal), pb = CPeriodAt(72, 160, cal);
    int m = 0; double p = -1.0; const double slope = (pb - pa) / 48.0;
    if (pa > 0.0 && pb > pa)
    {
        m = (int)(24.0 + ((double)target - pa) / slope + 0.5); if (m < 2) m = 2; if (m > 400) m = 400;
        for (int pass = 0; pass < 3; pass++)
        {
            p = CPeriodAt(m, 224, cal);
            if (p <= 0.0) break;
            const int step = (int)(((double)target - p) / slope + (target > p ? 0.5 : -0.5));
            if (!step || pass == 2) break;
            m += step; if (m < 2) m = 2; if (m > 400) m = 400;
        }
    }
    free(cal);
    *mOut = m; *pOut = p;
    return p > 0.0 && fabs(p - (double)target) <= 0.03 * (double)target;
}
static bool COrderCell(const char* regime, int take, int q, int frames, int m, int capUs, CRow* rows, double* v)
{
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    const double us = 1000000.0 / (double)qf.QuadPart;
    co.qat = q;
    // (2026-10-14, --fake-cost-ord: the two arms are THE ORDER WAIT OFF AND ON, 'Q' right behind 'F' in both)
    if (co.ord) { co.qat = 1; cOrdOn = q ? co.ord : 0; }
    cOrdSubT = 0; cOrdSubN = 0; cOs->ordT = 0; cOs->ordN = 0;
    memset(rows, 0, sizeof(CRow) * (size_t)frames);
    if (!CDrain()) return false;
    const int guards0 = cGuards; const LONG late0 = cOs->late;
    const LONGLONG capT = (LONGLONG)capUs * qf.QuadPart / 1000000;
    LONGLONG next = CQpc();
    for (int i = 0; i < frames; i++)
    {
        if (capUs)
        {   // THE CAP: the frame starts on its tick (a sleep while more than 2 ms are left, then the clock is watched)
            for (;;) { const LONGLONG left = next - CQpc(); if (left <= 0) break; if (left > 2 * qf.QuadPart / 1000) Sleep(1); else YieldProcessor(); }
            next += capT;
        }
        if (!CFrame(&rows[i], CA_F, take, q, i, true, false, m / 2, m - m / 2, false, nullptr, false, false)) return false;
    }
    if (!CDrain()) return false;
    Sleep(80);                                                        // the stampers take the last values
    const int skip = 32;
    int pairs = 0, behindN = 0, nb = 0, nk = 0, ng = 0, nf = 0, np = 0; double behindSum = 0.0;
    double* b = v; double* k = v + frames; double* g = v + 2 * frames; double* f = v + 3 * frames; double* per = v + 4 * frames;
    for (int i = skip; i < frames; i++)
    {
        const CRow& r = rows[i]; const size_t at = (size_t)(r.n & (C_OST - 1));
        const LONGLONG tIn = cOs->in[at], tPrep = cOs->prep[at], tOut = cOs->fgout[at], tFo = cOs->fout[at];
        if (tFo && r.tF && tFo >= r.tF) f[nf++] = (double)(tFo - r.tF) * us;
        if (rows[i].end[1] > rows[i - 1].end[1] && rows[i - 1].end[1]) per[np++] = (double)(rows[i].end[1] - rows[i - 1].end[1]) * cNs / 1000.0;
        if (!(r.flags & 1u) || !tIn || !tOut) continue;
        pairs++;
        if (tPrep) { const double d = (double)(tPrep - tIn) * us; b[nb++] = d; if (d > 0.0) { behindN++; behindSum += d; } }
        if (tOut >= tIn) k[nk++] = (double)(tOut - tIn) * us;
        if (r.tG && tOut >= r.tG) g[ng++] = (double)(tOut - r.tG) * us;
    }
    double perSum = 0.0; for (int i = 0; i < np; i++) perSum += per[i];
    const double bMed = COPct(b, nb, 50), kMed = COPct(k, nk, 50), k90 = COPct(k, nk, 90), gMed = COPct(g, ng, 50), fMed = COPct(f, nf, 50), f90 = COPct(f, nf, 90);
    Say("  cost-order regime=%s take=%d q=%d frames=%d pairs=%d behind_n=%d behind_mean_us=%.0f prep_minus_handoff_med_us=%.0f k_med_us=%.0f k_p90_us=%.0f g_to_answer_med_us=%.0f f_answer_med_us=%.0f f_answer_p90_us=%.0f period_us=%.1f guards=%d late_stamps=%ld m=%d",
        regime, take, q, frames, pairs, behindN, behindN ? behindSum / behindN : 0.0, bMed, kMed, k90, gMed, fMed, f90, np ? perSum / np : 0.0, cGuards - guards0, (long)(cOs->late - late0), m);
    if (co.ord)
    {   // (2026-10-14: a line of its own for the order wait's arms; the line above keeps its fields for its reader)
        const double k99 = COPct(k, nk, 99); const double kmx = nk ? k[nk - 1] : 0.0; int kov = 0; for (int i = 0; i < nk; i++) if (k[i] > 4500.0) kov++;
        Say("  cost-ord regime=%s take=%d wait=%d pairs=%d k_med_us=%.0f k_p99_us=%.0f k_max_us=%.0f k_over4500=%d period_us=%.1f f_answer_med_us=%.0f wait_submit_us=%.1f waits=%ld signal_us=%.1f signals=%ld",
            regime, take, cOrdOn, nk, kMed, k99, kmx, kov, np ? perSum / np : 0.0, fMed, cOrdSubN ? (double)cOrdSubT * us / cOrdSubN : 0.0, (long)cOrdSubN, cOs->ordN ? (double)cOs->ordT * us / cOs->ordN : 0.0, (long)cOs->ordN);
        cOrdOn = 0;
    }
    return pairs >= (frames - skip) * 9 / 10;
}
static bool COrderRun()
{
    const int frames = co.order;
    cOs = (COStamps*)calloc(1, sizeof(COStamps));
    CRow* rows = (CRow*)malloc(sizeof(CRow) * (size_t)frames); double* v = (double*)malloc(sizeof(double) * 5 * (size_t)frames);
    NEED(cOs && rows && v, "memory for the order part");
    for (int i = 0; i < 4; i++) { cOs->thr[i] = CreateThread(nullptr, 0, COStamper, (void*)(INT_PTR)i, 0, nullptr); NEED(cOs->thr[i], "a stamper thread"); SetThreadPriority(cOs->thr[i], THREAD_PRIORITY_ABOVE_NORMAL); }
    cOrderOn = true;
    bool ok = true; int cells = 0;
    struct Reg { const char* name; int cap, load; } regs[3] = { { "cap59", 16949, 0 }, { "cap118", 8475, 0 }, { "load", 0, co.oload } };
    for (int ri = 0; ri < 3 && ok; ri++)
    {
        int m = 0; char name[32]; strcpy_s(name, regs[ri].name);
        if (!(co.oreg & (1 << ri))) continue;
        if (ri == 2)
        {
            if (!regs[ri].load) continue;
            double p = 0.0;
            cOrderOn = false;                                         // (the search runs arm A: no hand, no gap)
            const bool found = COrderLoad(regs[ri].load, &m, &p);
            cOrderOn = true;
            if (!Proof(found, "THE ORDER PART, the card as the limit: the load is %d full-screen copies a frame and the baseline frame takes %.0f us (%d wanted)", m, p, regs[ri].load)) { ok = false; break; }
            sprintf_s(name, "load%d", regs[ri].load);
        }
        for (int take = 0; take < 3 && ok; take++)
            for (int j = 0; j < 2 && ok; j++)
            {
                const int q = (take & 1) ? 1 - j : j;                 // the two orders alternate: 0 1, 1 0, 0 1
                ok = COrderCell(name, take, q, frames, m, regs[ri].cap, rows, v);
                cells++;
            }
    }
    cOrderOn = false; co.qat = 0;
    _InterlockedExchange(&cOs->quit, 1);
    WaitForMultipleObjects(4, cOs->thr, TRUE, 2000);
    Proof(ok, "THE ORDER PART: %d cells of %d frames, 'Q' at the hand against 'Q' right behind the frame message, three takes each, every pair dispatched and stamped on four fences%s%s", cells, frames, cWhy[0] ? "; " : "", cWhy);
    free(rows); free(v);
    return ok;
}

// ---------------------------------------------------------------- THE COPY OVER A DISPATCH IN FLIGHT (2026-10-14)
// THE PRODUCT'S CLAIM (source/fg/fg_machine.h, ArmedOverMay): a pair may copy into the helper's PRESENT and HUDLESS while the
// dispatch before it is unanswered, BECAUSE the copies are submitted behind the frame's home on this queue, the home waits
// for this frame's answer, and the helper's one queue runs that answer's evaluate behind the dispatch before. PROVED HERE ON
// THE REAL HELPER AND A REAL QUEUE, BY PICTURES, IMAGE AGAINST IMAGE: --fake-cost-over=N loaded frames of arm F, one frame always in
// flight. EVERY FOURTH FRAME'S HOME READS THE GENERATED IMAGE OF THE PAIR BEFORE BACK, in the queue's order (behind its wait for this
// frame's answer; ahead of this pair's copies), and once that buffer has retired its tiles are measured against the
// half-way picture of the two frames that pair was made from.
//   THE REFERENCE STRETCH (the first 136 frames): the hand WAITS ON THE CPU for the dispatch before, then copies: no copy is ever
//     submitted over a dispatch in flight (today's rule, with a wait where the product says NO SOURCE). What each image
//     measures there is AMD's own picture of that pair of frames: the stage's six moves and its 288 pixel repeat make
//     twelve such pairs, and each measures the same to four places every time it comes round.
//   THE OVER STRETCH (the rest): NO WAIT ON THE HELPER FROM THE CPU. Each hand's copies are submitted at once, most of them with the
//     dispatch before still unanswered (flag 32 of the row, the hand's own CPU test). EVERY IMAGE MUST MEASURE WHAT THE SAME
//     PAIR OF FRAMES MEASURED IN THE REFERENCE STRETCH (within 0.01 of the change between the two frames, on the half-way
//     measure and on the mix measure). A copy that overtook the dispatch would have changed what it read.
// THE CONTROL (--fake-cost-overctl=1): in the over stretch the copies are submitted AHEAD of the home, where nothing holds
// them behind the dispatch before: the images do not measure what their pairs measured. Run with this program's class
// raised (--fake-cost-sclass=5: this side's work first, every dispatch behind this side's scene), which is the legs'
// case and the one where an early copy is sure to land under the dispatch.
// (An absolute bound was this part's first judge and a wrong one: "under 0.10 of the change from the half-way picture"
// refused seven of ninety, all seven the SAME pair of frames, the stage's largest move at one place of its repeat, which
// measures 0.1623 with the wait on the CPU as well. The judge is the reference, not a number of mine.)
enum { C_OVER_REF = 136, C_OVER_KEYS = 64 };
struct COverKey { long k[4]; double mid, mix; int n; };
static bool COverRun()
{
    const int frames = co.over;
    CRow* rows = (CRow*)calloc((size_t)frames, sizeof(CRow)); long* ps = (long*)calloc((size_t)frames * 2, sizeof(long));
    COverKey* keys = (COverKey*)calloc(C_OVER_KEYS, sizeof(COverKey)); int nKeys = 0;
    if (!Proof(rows && ps && keys && frames >= C_OVER_REF + 104, "THE OVER PART: memory, and %d frames or more (%d asked)", C_OVER_REF + 104, frames)) return false;
    int m = 0; double p = 0.0;
    const bool found = COrderLoad(co.oload ? co.oload : 9100, &m, &p);
    if (!Proof(found, "THE OVER PART, the card as the limit: the load is %d full-screen copies a frame and the baseline frame takes %.0f us", m, p)) { free(rows); free(ps); free(keys); return false; }
    if (!CDrain()) { free(rows); free(ps); free(keys); return false; }
    const long pn = co.guides ? 3 : 1, pd = co.guides ? 2 : 1;
    const TStage& pres = co.guides ? tWide : tPlain;
    const long per = (long)pres.period;
    co.qat = 1; cOverCtl = co.overctl; cOrderOn = true;
    int seen = 0, over = 0, rightOver = 0, wrongOver = 0, rightPlain = 0, wrongPlain = 0, noRef = 0, refSeen = 0, refOver = 0, refDiffer = 0, heldOver = 0, heldRight = 0;
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    double worst = 0.0, sum = 0.0, refWorst = 0.0, least = 9.0;
    bool ok = true;
    for (int i = 0; i < frames && ok; i++)
    {
        cOverRef = i < C_OVER_REF;
        cOverObs = i >= 40 && (co.overall || (i & 3) == 3) && (i < C_OVER_REF || i >= C_OVER_REF + 8);
        ok = CFrame(&rows[i], CA_F, 0, 0, i, true, false, m / 2, m - m / 2, false, nullptr, false, false);
        ps[2 * i] = cS * pn / pd; ps[2 * i + 1] = cT * pn / pd;
        if (!ok || !cOverObs) continue;
        cOverObs = false;
        if (!CWait(cHomeBuf)) { ok = false; break; }          // (a buffer of this side's own; this frame's copies stand on the queue behind it already)
        // (this home retired behind the dispatch before: from that pair's 'G' to here is at least what its answer took)
        const double heldMs = (double)(CQpc() - rows[i - 1].tG) * 1000.0 / (double)qf.QuadPart;
        CObs o;
        CObsCompare(&o, pres, ps[2 * (i - 1)], ps[2 * (i - 1) + 1], ps[2 * (i - 2)], ps[2 * (i - 2) + 1]);
        const double mid = o.change > 0.0 ? o.mid / o.change : 9.0, mix = o.change > 0.0 ? o.mix / o.change : 9.0;
        long k[4]; for (int j = 0; j < 4; j++) { const long v = ps[2 * (i - 1 - j / 2) + (j & 1)]; k[j] = ((v % per) + per) % per; }
        COverKey* e = nullptr;
        for (int j = 0; j < nKeys && !e; j++) if (!memcmp(keys[j].k, k, sizeof(k))) e = &keys[j];
        const bool wasOver = (rows[i].flags & 32u) != 0;
        if (cOverRef)
        {   // the reference: this pair of frames with the wait on the CPU; seen again, it must measure the same
            refSeen++; if (wasOver) refOver++;
            if (!e && nKeys < C_OVER_KEYS) { e = &keys[nKeys++]; memcpy(e->k, k, sizeof(k)); e->mid = mid; e->mix = mix; e->n = 0; }
            if (e) { e->n++; if (fabs(e->mid - mid) > 0.002 || fabs(e->mix - mix) > 0.002) refDiffer++; }
            if (mid > refWorst) refWorst = mid;
            Say("  cost-over-obs i=%d n=%llu ref=1 over=%d mid=%.4f mix=%.4f change=%.1f move=%d", i, (unsigned long long)rows[i].n, wasOver ? 1 : 0, mid, mix, o.change, rows[i - 1].move);
            continue;
        }
        const double dm = e ? fabs(mid - e->mid) : 9.0, dx = e ? fabs(mix - e->mix) : 9.0;
        const double d = dm > dx ? dm : dx;
        const bool right = e && d <= 0.01;
        if (!e) noRef++;
        seen++; if (e) { sum += d; if (d > worst) worst = d; if (d < least) least = d; }
        Say("  cost-over-obs i=%d n=%llu ref=0 over=%d right=%d mid=%.4f mix=%.4f ref_mid=%.4f ref_mix=%.4f change=%.1f move=%d", i, (unsigned long long)rows[i].n, wasOver ? 1 : 0, right ? 1 : 0, mid, mix,
            e ? e->mid : -1.0, e ? e->mix : -1.0, o.change, rows[i - 1].move);
        if (wasOver) { over++; if (right) rightOver++; else wrongOver++; } else { if (right) rightPlain++; else wrongPlain++; }
        // (only where every frame's image is read back: there this thread waits for each home, so the time from the pair's 'G' to its
        // home's retire is the answer's own and not the frames in flight)
        if (co.overall && wasOver && heldMs > 25.0) { heldOver++; if (right) heldRight++; }
    }
    cOverObs = false; cOverRef = false; cOverCtl = 0; cOrderOn = false; co.qat = 0;
    const bool drained = ok && CDrain();
    Say("  cost-over frames=%d control=%d ref_observed=%d ref_pairs=%d ref_over=%d ref_differ=%d ref_mid_worst=%.4f observed=%d over=%d right_over=%d wrong_over=%d right_plain=%d wrong_plain=%d no_ref=%d off_mean=%.4f off_worst=%.4f off_least=%.4f held_over=%d held_right=%d m=%d",
        frames, co.overctl, refSeen, nKeys, refOver, refDiffer, refWorst, seen, over, rightOver, wrongOver, rightPlain, wrongPlain, noRef, seen > noRef ? sum / (seen - noRef) : 0.0, worst, least, heldOver, heldRight, m);
    free(rows); free(ps); free(keys);
    if (!Proof(ok && drained && seen >= 8, "THE OVER PART: %d frames, the queue drained; %d images read back in the queue's order in the reference stretch and %d behind it%s%s", frames, refSeen, seen, cWhy[0] ? "; " : "", cWhy)) return false;
    if (!Proof(refSeen >= 20 && refOver == 0 && refDiffer == 0 && noRef == 0, "THE OVER PART'S REFERENCE, the hand waiting on the CPU for the dispatch before: %d images of %d different pairs of frames, none of them copied over a dispatch in flight (%d), and a pair that came round again measured the same to 0.002 (%d did not); every image behind the reference is of a pair the reference holds (%d are not). The half-way measure there is at worst %.4f of the change between the two frames", refSeen, nKeys, refOver, refDiffer, noRef, refWorst)) return false;
    if (co.overctl)
        return Proof(wrongOver + wrongPlain >= 1, "THE OVER PART'S CONTROL, the hand's copies submitted AHEAD of the frame's home: %d of %d images do NOT measure what the same pair of frames measured in the reference stretch (off by %.3f of the change between the two frames at the mean, %.3f at the least, %.3f at the most; 0.01 is the bound): the order is what holds the picture", wrongOver + wrongPlain, seen, seen ? sum / seen : 0.0, least, worst);
    char heldSay[200] = "";
    if (co.overall) _snprintf_s(heldSay, sizeof(heldSay), _TRUNCATE, ". Of them %d were copied over a dispatch that then took more than 25 ms to answer (the helper asleep ahead of it), %d of those right", heldOver, heldRight);
    return Proof(wrongOver == 0 && wrongPlain == 0 && over >= 1, "THE OVER PART: %d of %d images belong to pairs whose NEXT pair's copies were submitted with their dispatch still unanswered, and every image, of those %d and of the other %d, measures what the same pair of frames measured with the wait on the CPU (off by %.4f of the change between the two frames at the mean, %.4f at the most; 0.01 is the bound; wrong %d and %d)%s", over, seen, rightOver, rightPlain, seen ? sum / seen : 0.0, worst, wrongOver, wrongPlain, heldSay);
}

// ---------------------------------------------------------------- THE ORDER WAIT WITH THE HELPER HELD (2026-10-14)
// The product's order wait (FgxOrdWait: a batch with no buffer behind the hand's submits, waiting on a fence of THIS side's
// own) with a helper that does not answer: started with its own --fgslow_d=MS, the helper sleeps that long before every
// 64th dispatch of the connection. Frames go by with the wait released at each answer, until the pair whose answer does
// not come within 200 ms. THERE: the frame's end, queued behind the wait, HAS NOT RETIRED (the wait holds this queue: it is a
// wait); then, with the helper still asleep (--fake-cost-held=1), SUSPENDED first (2) or KILLED first (3), THE FENCE IS
// SIGNALLED FROM THE CPU, as the product's placer signals it at the deadline, and the frame's end retires at once. No helper
// has to live, wake or answer for this queue to run again: the one thing a wait on a helper's own value cannot say.
static bool CHeldRun()
{
    if (!Proof(FgxOrdMake<FakeB>(&cOrd), "THE HELD PART: the order's own fence is made (a timeline of this side's, exported nowhere)")) return false;
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    CRow row; bool ok = true, heldSeen = false; int i = 0; uint64_t n = 0;
    co.qat = 1; cOrdOn = 1;
    for (; i < 200 && ok && !heldSeen; i++)
    {
        memset(&row, 0, sizeof(row));
        ok = CFrame(&row, CA_F, 0, 0, i, true, false, 8, 8, false, nullptr, false, false);
        if (!ok) break;
        n = row.n;
        if (FeedVkWaitTimeline(&gVk, gSet.semOut, 2 * n, 200)) { FgxOrdSignal<FakeB>(&cOrd, n); if (!CWait(cEndBuf)) ok = false; }
        else heldSeen = true;
    }
    cOrdOn = 0; co.qat = 0;
    if (!Proof(ok && heldSeen, "THE HELD PART: %d frames went by with their order waits released at the answer, and the answer of pair %llu did not come within 200 ms (the helper sleeps before that dispatch)%s%s", i - 1, (unsigned long long)n, cWhy[0] ? "; " : "", cWhy)) return false;
    Sleep(60);
    const bool standing = FeedVkTimelineValue(&gVk, cLocal) < cEndBuf->val && FeedVkTimelineValue(&gVk, gSet.semOut) < 2 * n;
    bool did = true; const char* how = "still asleep";
    if (co.held == 2) { did = SuspendHelper(); how = "SUSPENDED"; }
    else if (co.held == 3) { did = TerminateProcess(gHelper, 9) != 0; WaitForSingleObject(gHelper, 2000); gHelperEndedHere = true; how = "KILLED"; }
    const uint64_t outBefore = FeedVkTimelineValue(&gVk, gSet.semOut);
    const LONGLONG t0 = CQpc();
    FgxOrdSignal<FakeB>(&cOrd, n);
    const bool retired = FeedVkWaitTimeline(&gVk, cLocal, cEndBuf->val, 2000);
    const double ms = (double)(CQpc() - t0) * 1000.0 / (double)qf.QuadPart;
    Say("  cost-held mode=%d pair=%llu standing=%d helper_%s=%d retired=%d retire_ms=%.3f fgout_before=%llu wanted=%llu", co.held, (unsigned long long)n, standing ? 1 : 0, co.held == 2 ? "suspended" : co.held == 3 ? "killed" : "asleep", did ? 1 : 0, retired ? 1 : 0, ms, (unsigned long long)outBefore, (unsigned long long)(2 * n));
    Proof(standing, "THE HELD PART: 60 ms on, with no answer, the frame's end queued behind the order wait HAS NOT RETIRED: the wait holds this queue");
    Proof(did && retired && ms < 100.0, "THE HELD PART: the helper %s (%s), the order's own fence signalled from the CPU: the frame's end retired %.2f ms later, with the answer still not in where the helper lives (fgOut %llu of %llu)", how, did ? "done" : "COULD NOT BE DONE", ms, (unsigned long long)outBefore, (unsigned long long)(2 * n));
    if (co.held == 2 && gHelper) { TerminateProcess(gHelper, 9); WaitForSingleObject(gHelper, 2000); gHelperEndedHere = true; }
    return false;                                             // (the session ends here: the helper is asleep for seconds, suspended or gone)
}

// ---------------------------------------------------------------- THE RUN
static void CostRun()
{
    // THE PRESENTED FORMAT (the name is from when it was FP16 alone): FP16, or with --fake-cost-fmt=1 the game's 8-bit back buffer
    const uint32_t FP16 = co.fmt ? (uint32_t)DXGI_FORMAT_B8G8R8A8_UNORM : (uint32_t)DXGI_FORMAT_R16G16B16A16_FLOAT;
    const char* const fmtName = co.fmt ? "B8G8R8A8" : "FP16";
    NEED(!co.fmt || (co.idle == 0 && co.nLoad == 0), "--fake-cost-fmt=1 runs the hash part only: the timed parts and the correctness figure read FP16");
    const DWORD DL = FEED_FG_BUILD_DEADLINE_MS;
#define CVK(p, n) p = (PFN_##n)vkGetDeviceProcAddr_(gDev, #n); NEED(p, #n)
    CVK(cCreateQueryPool, vkCreateQueryPool); CVK(cCmdResetQueryPool, vkCmdResetQueryPool); CVK(cCmdWriteTimestamp, vkCmdWriteTimestamp); CVK(cGetQueryPoolResults, vkGetQueryPoolResults);
    CVK(tCopyBufferToImage, vkCmdCopyBufferToImage); CVK(cCmdFillBuffer, vkCmdFillBuffer);
#undef CVK
    {   // the timestamps' unit, and that this queue has them
        VkPhysicalDeviceProperties2 p2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 }; vkGetPhysicalDeviceProperties2_(gPhys, &p2);
        uint32_t nf = 16; VkQueueFamilyProperties fams[16]; vkGetPhysicalDeviceQueueFamilyProperties_(gPhys, &nf, fams);
        cNs = p2.properties.limits.timestampPeriod;
        if (!Proof(gFam < nf && fams[gFam].timestampValidBits >= 48 && cNs > 0.0, "this queue has timestamps: %u valid bits, %.3f ns a tick", gFam < nf ? fams[gFam].timestampValidBits : 0u, cNs)) return;
    }
    VkQueryPoolCreateInfo qpi = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO }; qpi.queryType = VK_QUERY_TYPE_TIMESTAMP; qpi.queryCount = C_RING * C_QPB;
    NEED(cCreateQueryPool(gDev, &qpi, nullptr, &cPool) == VK_SUCCESS, "the cost bench's query pool");
    VkCommandBuffer cbs[C_RING];
    VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = gPool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = C_RING;
    NEED(vkAllocateCommandBuffers_(gDev, &cai, cbs) == VK_SUCCESS, "the cost bench's command buffers");
    for (int i = 0; i < C_RING; i++) cBuf[i].cb = cbs[i];
    { HANDLE none = nullptr; NEED(FeedVkCreateTimeline(&gVk, false, &cLocal, &none) >= 0 && cLocal != VK_NULL_HANDLE, "the cost bench's own timeline"); if (none) CloseHandle(none); }
    {   // the read-back buffer for GEN's four tiles
        VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = (VkDeviceSize)C_TILES * C_TILE_BYTES; bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        NEED(vkCreateBuffer_(gDev, &bi, nullptr, &cObsBuf) == VK_SUCCESS, "the tiles' buffer");
        VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, cObsBuf, &q);
        VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size;
        mi.memoryTypeIndex = MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDeviceMemory m; NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &m) == VK_SUCCESS, "the tiles' memory");
        NEED(vkBindBufferMemory_(gDev, cObsBuf, m, 0) == VK_SUCCESS && vkMapMemory_(gDev, m, 0, bi.size, 0, &cObsPtr) == VK_SUCCESS, "the tiles' buffer, bound and mapped");
    }
    // ---- the guides' size, the picture (made once, uploaded once), the off-screen back buffer and the load's two images
    if (co.guides) { cRw = 2560; cRh = 1440; }
    if (co.upscale)
    {   // A REAL BUILD: the helper's own upscaler, alive and evaluating every frame beside frame generation
        FeedBuildAck back;
        const bool built = CRebuildReal(cRw, cRh, &back);
        if (!Proof(built, "the upscaler's set is built again as a REAL build at %ux%u under a %ux%u display: the helper made its upscaler (ok %d, result 0x%X, flags 0x%X) and its four images import; every frame is an evaluate",
                   cRw, cRh, W, H, back.ok, back.ngx_result, back.flags)) return;
    }
    else if (co.guides)
    {
        if (!Proof(TRebuild(cRw, cRh), "the upscaler's set is built again at %ux%u (the guides of DLSS Quality under a %ux%u display) and its four images import", cRw, cRh, W, H)) return;
    }
    else { Begin(); Clear(gIm[FEED_DEPTH], 0.0f, 0.0f, 0.0f, 0.0f); NEED(Submit(VK_NULL_HANDLE, 0), "the depth's clear"); }
    const VkImageUsageFlags plain = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    const VkImageUsageFlags dxvkBb = plain | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;   // 0x97, what DXVK gives the game's back buffer
    TStageMake(&tPlain, W, H, 1, 1);
    cAtlas = CMakeImage(tPlain.w, tPlain.h, plain);
    if (co.guides) { TStageMake(&tWide, W, H, 3, 2); cAtlasWide = CMakeImage(tWide.w, tWide.h, plain); }
    cBb = CMakeImage(W, H, dxvkBb, co.fmt ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R16G16B16A16_SFLOAT); cLoadImg[0] = CMakeImage(W, H, dxvkBb); cLoadImg[1] = CMakeImage(W, H, dxvkBb);
    {
        VkImage all[5] = { cAtlas.img, cBb.img, cLoadImg[0].img, cLoadImg[1].img, cAtlasWide.img };
        Begin();
        FeedVkBarrierN(&gVk, gCb, all, co.guides ? 5 : 4, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        VkBufferImageCopy rg = {}; rg.bufferRowLength = tPlain.w; rg.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }; rg.imageExtent = { tPlain.w, tPlain.h, 1 };
        tCopyBufferToImage(gCb, tPlain.buf, cAtlas.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg);
        if (co.guides) { rg.bufferRowLength = tWide.w; rg.imageExtent = { tWide.w, tWide.h, 1 }; tCopyBufferToImage(gCb, tWide.buf, cAtlasWide.img, VK_IMAGE_LAYOUT_GENERAL, 1, &rg); }
        NEED(Submit(VK_NULL_HANDLE, 0), "the picture's upload");
        Begin();
        CPresent(gCb, cAtlas, tPlain.period, 0, 0, cBb.img, W, H); CPicture(gCb, cAtlas, tPlain.period, 0, 0, cLoadImg[0].img, W, H); CPicture(gCb, cAtlas, tPlain.period, 8, 8, cLoadImg[1].img, W, H);
        NEED(Submit(VK_NULL_HANDLE, 0), "the first pictures");
    }
    if (co.out[0]) { fopen_s(&cOut, co.out, "w"); NEED(cOut, "the rows' file"); fprintf(cOut, "part,arm,rep,half,idx,n,flags,move,sc_us,hm_us,c1_us,c1buf_us,c2_us,c2buf_us,empty_us,end_ticks,t_start,t_q,t_g,t_out,t_end\n"); }
    if (co.gen[0]) { fopen_s(&cGen, co.gen, "wb"); NEED(cGen, "the tiles' file"); }
    {   // arm FC's buffer: half a motion vector image of R16G16 texels, on the card
        VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = (VkDeviceSize)(cRw / 2) * cRh * 4u; bi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        NEED(vkCreateBuffer_(gDev, &bi, nullptr, &cMvBuf) == VK_SUCCESS, "the colliding vectors' buffer");
        VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, cMvBuf, &q);
        VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size; mi.memoryTypeIndex = MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceMemory m; NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &m) == VK_SUCCESS && vkBindBufferMemory_(gDev, cMvBuf, m, 0) == VK_SUCCESS, "the colliding vectors' buffer, bound");
    }
    if (co.hash > 0)
    {   // the hash part's buffer: all of GEN, 63.3 MiB, mapped
        VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO }; bi.size = (VkDeviceSize)W * H * 8u; bi.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        NEED(vkCreateBuffer_(gDev, &bi, nullptr, &cHashBuf) == VK_SUCCESS, "the whole image's buffer");
        VkMemoryRequirements q; vkGetBufferMemoryRequirements_(gDev, cHashBuf, &q);
        VkMemoryAllocateInfo mi = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; mi.allocationSize = q.size;
        // A CACHED TYPE WHERE THE CARD HAS ONE (2026-10-06): this buffer is READ, every word of it, and from the first
        // host-visible type (write-combined on this card) an image took 1.2 s to hash; the rest of a pair takes milliseconds
        const VkMemoryPropertyFlags cached = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        uint32_t mt = UINT32_MAX;
        for (uint32_t i = 0; i < gMem.memoryTypeCount && mt == UINT32_MAX; i++) if ((q.memoryTypeBits & (1u << i)) && (gMem.memoryTypes[i].propertyFlags & cached) == cached) mt = i;
        mi.memoryTypeIndex = mt != UINT32_MAX ? mt : MemType(q.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDeviceMemory m; NEED(gVk.AllocateMemory(gDev, &mi, nullptr, &m) == VK_SUCCESS, "the whole image's memory");
        NEED(vkBindBufferMemory_(gDev, cHashBuf, m, 0) == VK_SUCCESS && vkMapMemory_(gDev, m, 0, bi.size, 0, &cHashPtr) == VK_SUCCESS, "the whole image's buffer, bound and mapped");
        if (co.hashOut[0]) { fopen_s(&cHash, co.hashOut, "w"); NEED(cHash, "the hashes' file"); fprintf(cHash, "arm,idx,n,all,colour,blocks\n"); }
    }
    // ---- the set, by the client's own lines
    // (hud 0: the set is asked WITHOUT the HUD-less image, as the product asks since lever 1; its imports are then two)
    int x = FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)FP16, cRw, cRh, 0, DL, CBuildFlags() | gWireFlags);
    if (x == FGX_CREATED) { Begin(); FeedVkBarrierN(&gVk, gCb, gSet.all, CSetImages(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); if (!Submit(VK_NULL_HANDLE, 0)) x = -1; }
    if (x == FGX_CREATED && (gSet.img[FEED_FG_HUDLESS] != VK_NULL_HANDLE) != (co.hud != 0)) { Say("  FAIL  the set has %s HUD-less image and --fake-cost-hud=%d asked for the other", gSet.img[FEED_FG_HUDLESS] != VK_NULL_HANDLE ? "a" : "no", co.hud); x = -1; }
    if (!Proof(x == FGX_CREATED && gSet.have, "a set is CREATED through the client's own lines (status %d): display %ux%u %s over guides of %ux%u; the create and the warm-up took %u.%u ms", gSet.status, W, H, fmtName, cRw, cRh,
               gSet.warmUs / 1000, (gSet.warmUs % 1000) / 100)) return;
    gSeenPrep = FeedVkTimelineValue(&gVk, gSet.semPrep); gSeenOut = FeedVkTimelineValue(&gVk, gSet.semOut);
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    Say("  cost-session guides=%ux%u display=%ux%u hud=%d cutg=%d ns_per_tick=%.6f qpc_hz=%lld first=%llu rule=%d moves=%d:%d/%d:%d/%d:%d/%d:%d/%d:%d/%d:%d fmt=%s fq=%d", cRw, cRh, W, H, co.hud, co.cutg, cNs, (long long)qf.QuadPart,
        (unsigned long long)cN, co.rule, kCMove[0][0], kCMove[0][1], kCMove[1][0], kCMove[1][1], kCMove[2][0], kCMove[2][1], kCMove[3][0], kCMove[3][1], kCMove[4][0], kCMove[4][1], kCMove[5][0], kCMove[5][1], fmtName, co.fq);
    int total = 0, cells = 0; char said[256];
    {   // THE CARD IS WARMED FIRST: a session starts on a resting card, and for its first second the clock is the resting one
        // (measured: the first cell of a session read 7 percent long). 400 loaded frames, nobody's, before anything is measured
        CRow* warm = (CRow*)malloc(sizeof(CRow) * 400); NEED(warm, "memory for the warm-up's rows");
        const unsigned long long t0 = CUnixMs();
        const double p = CPeriodAt(96, 400, warm);
        free(warm);
        if (!Proof(p > 0.0, "the card is warmed before anything is measured: 400 frames of 96 full-screen copies, %.0f us a frame, %llu ms%s%s", p, CUnixMs() - t0, cWhy[0] ? "; " : "", cWhy)) return;
        Say("  cost-warm frames=400 period_us=%.1f", p);
    }

    // ---- IDLE: every part alone on the card
    if (co.idle > 0)
    {
        int arms[16]; const int na = CArms(co.arms, arms, 16);
        CRow* rows = (CRow*)malloc(sizeof(CRow) * (size_t)co.pairs); NEED(rows, "memory for a cell's rows");
        for (int rep = 0; rep < co.idle; rep++) for (int k = 0; k < na; k++)
        {
            const int arm = arms[(k + rep) % na];                     // the arms in a new order each round
            memset(rows, 0, sizeof(CRow) * (size_t)co.pairs);
            CObs obs; int done = 0; const uint64_t first = cN; const unsigned long long t0 = CUnixMs();
            // (the tiles of rounds 0 and 1 go to the file: pairs 63, 127, 191 and 255 of the arms that dispatch)
            for (int i = 0; i < co.pairs; i++, done++)
                if (!CFrame(&rows[i], arm, rep, 0, i, true, true, 0, 0, false, &obs, (i & 7) == 7, rep < 2 && (i & 63) == 63)) break;
            const bool drained = CDrain();
            CRowsWrite("idle", rows, done);
            CObsSay(said, sizeof(said), obs);
            Say("  cost-cell part=idle arm=%s rep=%d pairs=%d first=%llu last=%llu t0=%llu t1=%llu %s", kCArm[arm], rep, done, (unsigned long long)first, (unsigned long long)(cN - 1), t0, CUnixMs(), said);
            total += done; cells++;
            if (done != co.pairs || !drained) { Proof(false, "IDLE, arm %s, round %d: pair %d of %d could not go on: %s", kCArm[arm], rep, done + 1, co.pairs, cWhy); free(rows); return; }
        }
        free(rows);
        Proof(true, "IDLE: %d cells of %d pairs, every part of every pair waited for before the next: each 'F' answered, each 'Q' with fgPrep at n, each 'G' with fgOut at 2n behind a dispatch and 2n+1 without", cells, co.pairs);
    }

    // ---- THE WHOLE GENERATED IMAGE, HASHED: timed by nobody. Every part of a pair is waited for, as in the idle arms
    if (co.hash > 0)
    {
        int arms[16]; const int na = CArms(co.harms, arms, 16);
        CRow* rows = (CRow*)malloc(sizeof(CRow) * (size_t)co.hash); NEED(rows, "memory for the hash part's rows");
        int hashed = 0; const unsigned long long t0 = CUnixMs();
        for (int k = 0; k < na; k++)
        {
            memset(rows, 0, sizeof(CRow) * (size_t)co.hash);
            int done = 0;
            for (int i = 0; i < co.hash; i++, done++)
            {
                if (!CFrame(&rows[i], arms[k], 0, 0, i, true, true, 0, 0, false, nullptr, false, false)) break;
                if ((rows[i].flags & 1u) && !CHashGen(kCArm[arms[k]], i, rows[i].n)) break;
                if (rows[i].flags & 1u) hashed++;
            }
            const bool drained = CDrain();
            CRowsWrite("hash", rows, done);                           // (2026-10-06) the hash part's frames have their rows too: which move each pair made
            Say("  cost-hash arm=%s pairs=%d first=%llu last=%llu", kCArm[arms[k]], done, (unsigned long long)(cN - (uint64_t)done), (unsigned long long)(cN - 1));
            total += done;
            if (done != co.hash || !drained) { Proof(false, "THE HASH PART, arm %s: pair %d of %d could not go on: %s", kCArm[arms[k]], done + 1, co.hash, cWhy); free(rows); return; }
        }
        free(rows);
        Proof(hashed > 0, "THE HASH PART: %d arms of %d pairs; behind each of %d dispatched pairs the whole of GEN (%ux%u %s, %u MiB) was read back and hashed, in %llu ms", na, co.hash, hashed, W, H, fmtName,
              (unsigned)(((unsigned long long)W * H * (co.fmt ? 4u : 8u)) >> 20), CUnixMs() - t0);
    }

    // ---- LOADED: the card is the limit, one frame always in flight
    for (int li = 0; li < co.nLoad; li++)
    {
        const int target = co.loadUs[li];
        CRow* cal = (CRow*)malloc(sizeof(CRow) * 256); NEED(cal, "memory for the calibration's rows");
        // THE LOAD, FOUND BY TIMESTAMPS: the period at two loads gives a copy's price in the frame; then the count that makes the
        // baseline frame take the target, corrected twice
        const double pa = CPeriodAt(24, 160, cal), pb = CPeriodAt(72, 160, cal);
        int m = 0; double p = -1.0, slope = (pb - pa) / 48.0;
        if (pa > 0.0 && pb > pa)
        {
            m = (int)(24.0 + ((double)target - pa) / slope + 0.5); if (m < 2) m = 2; if (m > 400) m = 400;
            for (int pass = 0; pass < 3; pass++)
            {
                p = CPeriodAt(m, 224, cal);
                if (p <= 0.0) break;
                const int step = (int)(((double)target - p) / slope + (target > p ? 0.5 : -0.5));
                if (!step || pass == 2) break;
                m += step; if (m < 2) m = 2; if (m > 400) m = 400;
            }
        }
        free(cal);
        if (!Proof(p > 0.0 && fabs(p - (double)target) <= 0.03 * (double)target, "LOADED, %d us: the load is %d full-screen copies a frame (a copy costs the frame %.1f us: %.0f us at 24 copies, %.0f at 72): the baseline frame takes %.0f us%s%s",
                   target, m, slope, pa, pb, p, cWhy[0] ? "; " : "", cWhy)) return;
        Say("  cost-load target=%d m=%d copy_us=%.2f period_us=%.1f", target, m, slope, p);
        int arms[16]; const int na = CArms(co.larms, arms, 16);
        const int frames = co.quads * 4 * co.half;
        CRow* rows = (CRow*)malloc(sizeof(CRow) * (size_t)frames); NEED(rows, "memory for a block's rows");
        char part[32]; sprintf_s(part, "load%d", target);
        for (int k = 0; k < na; k++)
        {
            const int arm = arms[k];
            memset(rows, 0, sizeof(CRow) * (size_t)frames);
            CObs obs; int done = 0, disp = 0, nosrc = 0, sent = 0; const uint64_t first = cN; const unsigned long long t0 = CUnixMs(); const int guards0 = cGuards; bool ok = true;
            for (int q = 0; q < co.quads && ok; q++) for (int hf = 0; hf < 4 && ok; hf++)
            {
                const bool isB = hf == 1 || hf == 2;                  // A B B A
                ok = CDrain();                                        // every half starts from an empty queue
                if (ok && arm == CA_OFF)
                {   // FRAME GENERATION OFF AGAINST STANDBY: the set is released for the B halves and made again for the A halves
                    if (isB && gSet.have) ok = FgxRelease<FakeB>(&gSet, DL) == FGX_RELEASED;
                    else if (!isB && !gSet.have)
                    {
                        ok = FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)FP16, cRw, cRh, 0, DL, CBuildFlags() | gWireFlags) == FGX_CREATED;
                        if (ok) { Begin(); FeedVkBarrierN(&gVk, gCb, gSet.all, CSetImages(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); ok = Submit(VK_NULL_HANDLE, 0); }
                    }
                    if (!ok) cWhy = "the set was not released or made again between two halves";
                }
                const int extra = (isB && arm == CA_LK) ? 8 : 0;      // the load's own honesty: 8 more of its copies, behind the home
                for (int i = 0; i < co.half && ok; i++, done++)
                {
                    CRow* r = &rows[done];
                    // (GEN is read once a half, at frame 24: behind the reset's ten frames and inside the frames the reader drops)
                    ok = CFrame(r, arm, q, hf, i, isB, false, m / 2, m - m / 2 + extra, isB && (co.rule || arm == CA_FX), &obs, i == 24, false);
                    if (r->tQ) sent++;
                    if (r->flags & 1u) disp++;
                    if (r->flags & 8u) nosrc++;
                }
            }
            if (ok && arm == CA_OFF && !gSet.have)
            {
                ok = CDrain() && FgxAcquire<FakeB>(&gSet, W, H, (DXGI_FORMAT)FP16, cRw, cRh, 0, DL, CBuildFlags() | gWireFlags) == FGX_CREATED;
                if (ok) { Begin(); FeedVkBarrierN(&gVk, gCb, gSet.all, CSetImages(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL); ok = Submit(VK_NULL_HANDLE, 0); }
            }
            const bool drained = CDrain();
            CRowsWrite(part, rows, done);
            CObsSay(said, sizeof(said), obs);
            Say("  cost-block part=%s arm=%s quads=%d half=%d frames=%d first=%llu last=%llu t0=%llu t1=%llu m=%d pairs=%d dispatched=%d nosource=%d guards=%d %s", part, kCArm[arm], co.quads, co.half, done,
                (unsigned long long)first, (unsigned long long)(cN - 1), t0, CUnixMs(), m, sent, disp, nosrc, cGuards - guards0, said);
            total += done;
            if (!ok || !drained || done != frames) { Proof(false, "LOADED, %d us, arm %s: frame %d of %d could not go on: %s", target, kCArm[arm], done + 1, frames, cWhy); free(rows); return; }
        }
        free(rows);
        Proof(true, "LOADED, %d us: %d arms of %d quads (A B B A, %d frames a half), one frame always in flight and no wait on the helper from the CPU: every frame was submitted and every half drained", target, na, co.quads, co.half);
    }

    // ---- (2026-10-14) THE HELPER'S PLACE ON THE CARD, through the product's own line, for the three parts below; and this program's own
    // class raised, the stand-in for a game that outranks the helper
    if (co.hclass == 5)
    {
        FgxCardClass(gHelper, true, &cCard);
        Proof(cCard.took == 1, "the helper's process is asked into the scheduler's top class through the product's own line (FgxCardClass): status 0x%08lX, the class reads %ld behind it (5 is the top), %ld before the ask; taken %ld, refused %ld, no such call %ld",
              (unsigned long)cCard.status, (long)cCard.read, (long)(cCard.prior ? cCard.prior - 1 : -1), (long)cCard.took, (long)cCard.refused, (long)cCard.absent);
    }
    if (co.sclass == 5)
    {
        FgxCard self = {};
        FgxCardClass(GetCurrentProcess(), true, &self);
        Proof(self.took == 1, "THIS program's own class is raised to the top (the stand-in for a game that outranks the helper): status 0x%08lX, reads %ld", (unsigned long)self.status, (long)self.read);
    }
    // ---- (2026-10-14) THE COPY OVER A DISPATCH IN FLIGHT, by pictures; and the order wait with the helper held
    if (co.over > 0 && gSet.have) { if (!COverRun()) return; }
    if (co.held > 0 && gSet.have) { CHeldRun(); return; }

    // ---- THE ORDER PART (lever 2): 'Q' at the hand against 'Q' right behind the frame message
    if (co.ord == 1 && co.order > 0 && gSet.have) { if (!Proof(FgxOrdMake<FakeB>(&cOrd), "the order's own fence is made (a timeline of this side's, exported nowhere)")) return; }
    if (co.order > 0 && gSet.have) { if (!COrderRun()) return; }
    if (co.hclass == 5)
    {
        FgxCardClass(gHelper, false, &cCard);
        Proof(cCard.backs == 1 && cCard.backStatus == 0, "the helper's class is given back through the product's own line: status 0x%08lX, %ld time(s)", (unsigned long)cCard.backStatus, (long)cCard.backs);
    }

    // ---- the end: the set given back by the game's word, so the helper's marks say their closing line for it
    const bool drained = CDrain();
    x = gSet.have ? FgxRelease<FakeB>(&gSet, DL) : FGX_RELEASED;
    Proof(drained && x == FGX_RELEASED, "the set is RELEASED through the client's own lines (status %d): the helper's cost marks close their numbers for it", gSet.status);
    Proof(cUnread == 0 && cLate == 0, "every stamp of this side's was read (%d could not be) and no buffer was late to retire (%d)", cUnread, cLate);
    Proof(!gFenceBack, "no fence the helper signals was ever read lower than before (%d reads)", gFenceReads);
    Say("  cost-total frames=%d guards=%d last=%llu", total, cGuards, (unsigned long long)(cN - 1));
    if (cOut) fclose(cOut);
    if (cGen) fclose(cGen);
    if (cHash) fclose(cHash);
}
// ============================================================================== END OF THE COST BENCH (--fake-fg=40)
