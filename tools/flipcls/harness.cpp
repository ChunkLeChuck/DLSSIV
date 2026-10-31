// THE FLIP CLASSIFIER'S REPLAY HARNESS (2026-09-24, frame generation phase 1B, instrument I-B2; the classifier spec's
// checklist items 6 and 11). The classifier's C code (DLSS-IV.cpp, from "THE FLIP CLASSIFIER" to the waiter's loop) is cut
// from the source by run.py into flip_extract.h, never copied by hand, and compiled here with the mod's own compiler flags
// against stubs of the mod's globals. It then replays, frame by frame, every window the reference model (synth-sim.py --dump)
// classified: LatFlipKeep for every frame, LatFlipTake for every accepted one, LatFlipStep when 120 are in, exactly as
// LatWaiterLoop calls them, and compares the verdict and the display memory with the model's, window by window.
//   harness.exe <dump>          the replay; exit code 0 when every verdict and every memory matches
//   harness.exe --log <dump>    log=1 on two feeds: the model's first cell (a change line by window 2) and an alternating
//                               LIMITED / UNCERTAIN feed that never settles (the liveness line at window 30, none before)
//   harness.exe --key           the inputs the model's dump never moves (2026-09-24, the classifier's verification: the
//                               code's S2 and the arithmetic's third finding): the key's clears on a Reset, a vsync change and
//                               a refresh change, between windows and inside one; the latch and its clearing; the pacer's
//                               share; FusionFix's cap, its Custom -n (rounded and truncated) and DXVK's cap; a rational
//                               refresh (120000/1001); a resync inside a window. Hand-made feeds, each with the answer the
//                               spec's rule gives it, and each channel with a feed that must NOT match, so a broken channel
//                               fails. Exit code 0 when every expectation holds.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#ifndef DLSSIV_DEVLOG
#define DLSSIV_DEVLOG 0
#endif
#if DLSSIV_DEVLOG
#define DEVSTAT(...) __VA_ARGS__
#else
#define DEVSTAT(...)
#endif
#define DLSSIV_FGDATA
enum { LAT_RING = 64, LAT_WIN = 120 };
// ---- the mod's globals the classifier reads, stubbed with the mod's own types
static LONGLONG ivtQpcFreq = 10000000;                     // the dump's clock: 10 MHz
alignas(8) static volatile LONGLONG hdrRefreshPacked;
static volatile LONG latPaceSleptN, latPaceFrames;
static LONG statDeviceResets;
static int latFfCap, latFfRel, latDxvkCap;
static struct { bool vsync, endOn; } latFlags;
#define latVsync (latFlags.vsync)
#define latEndOn (latFlags.endOn)
static int logLevel = 0;
static int gLines = 0, gFirstLineWin = 0, gCurWin = 0, gLinesBefore30 = 0, gLastWin = 0;
static bool gEcho = false;
static char gLast[4096];                                   // the last line printed and its window, for --key's checks of the change line
static void LogEmit(const char* fmt, va_list ap)
{
    _vsnprintf_s(gLast, sizeof(gLast), _TRUNCATE, fmt, ap); gLastWin = gCurWin;
    if (gEcho) printf("    [window %d] %s\n", gCurWin, gLast);
}
static void LogRecord(const char* fmt, ...)
{
    if (logLevel < 1) return;
    gLines++; if (!gFirstLineWin) gFirstLineWin = gCurWin; if (gCurWin < 30) gLinesBefore30++;
    va_list ap; va_start(ap, fmt); LogEmit(fmt, ap); va_end(ap);
}
#if DLSSIV_DEVLOG
static void LogHealth(const char* fmt, ...)
{
    if (logLevel < 2) return;
    va_list ap; va_start(ap, fmt); LogEmit(fmt, ap); va_end(ap);
}
#else
#define LogHealth(...) ((void)0)
#endif
DEVSTAT(static volatile LONG dtPresDoneStub; alignas(8) static volatile LONGLONG latFlipRelQpc[LAT_RING]; static volatile float latFlipRelBlk[LAT_RING]; static volatile LONG latFlipRelN[LAT_RING];)
#define dtPresDone dtPresDoneStub
#include "flip_extract.h"

static void ResetStatics()
{
    latFlipSettled = latFlipDisplay = 0;
    latFlipSince = latFlipLast = latFlipDisplaySince = latFlipLatchWin = latFlipPinRun = latFlipSaidWin = latFlipHeld = latFlipPending = 0;
    memset(latFlipInfo, 0, sizeof(latFlipInfo)); memset(latFlipWins, 0, sizeof(latFlipWins));
    latFlipKeyRefresh = 0; latFlipKeyResets = 0; latFlipKeyVsync = 0; memset(latFlipKeyF, 0, sizeof(latFlipKeyF));
}
static const char* VName(int v) { return v == 1 ? "FREE" : v == 2 ? "QUANTISED" : v == 3 ? "LIMITED" : v == 4 ? "UNCERTAIN" : "none"; }

// The replay. Returns the number of mismatched windows; maxCells > 0 stops after that many cells (the --log run).
static int Replay(const char* path, int maxCells, int* windowsOut)
{
    FILE* fp = nullptr; if (fopen_s(&fp, path, "r") || !fp) { printf("cannot open %s\n", path); return -1; }
    static LatFlipWin_t fw;
    char line[256], tag[64]; int cells = 0, windows = 0, badV = 0, badM = 0, lag = 3, win = 0, cellNo = 0;
    bool marker = true;
    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == 'C')
        {
            int num, vs, mk, F, cont;
            if (sscanf_s(line, "C %63s %d %d %d %d %d %d", tag, (unsigned)sizeof(tag), &num, &vs, &mk, &lag, &F, &cont) != 7) { printf("bad line: %s", line); return -1; }
            if (maxCells > 0 && cells >= maxCells && !cont) break;
            if (!cont) { ResetStatics(); win = 0; }
            memset(&fw, 0, sizeof(fw));                          // the loop's resync
            hdrRefreshPacked = num ? (((LONGLONG)num << 32) | 1) : 0;
            latVsync = vs != 0; latEndOn = mk != 0; marker = mk != 0; latFfCap = F; latFfRel = 0; latDxvkCap = 0;
            cells++; cellNo++;
        }
        else if (line[0] == 'F')
        {
            LONG x; long long tIn, tEnd, b; int acc;
            if (sscanf_s(line, "F %ld %lld %lld %lld %d", &x, &tIn, &tEnd, &b, &acc) != 5) { printf("bad line: %s", line); return -1; }
            LatFlipKeep(fw, x, tIn, tEnd);
            if (acc) LatFlipTake(fw, x, b, lag, marker);
            if (fw.nB == LAT_WIN)
            {
                if (!fgets(line, sizeof(line), fp) || line[0] != 'W') { printf("a window with no W line after frame %ld\n", x); return -1; }
                double mB; int mv, mm;
                if (sscanf_s(line, "W %lf %d %d", &mB, &mv, &mm) != 3) { printf("bad line: %s", line); return -1; }
                gCurWin = ++win;
                LatFlipStep(fw, (float)mB, win);
                windows++;
                const bool okV = latFlipLast == mv, okM = latFlipDisplay == mm;
                if (!okV) { if (badV < 12) printf("  VERDICT DIFFERS: cell %d (%s) window %d: C %s, model %s\n", cellNo, tag, win, VName(latFlipLast), VName(mv)); badV++; }
                if (!okM) { if (badM < 12) printf("  MEMORY DIFFERS: cell %d (%s) window %d: C %s, model %s\n", cellNo, tag, win, VName(latFlipDisplay), VName(mm)); badM++; }
            }
        }
    }
    fclose(fp);
    if (windowsOut) *windowsOut = windows;
    if (maxCells <= 0)
        printf("  replayed %d cells, %d windows: %d verdicts differ from the model's, %d memories differ\n", cells, windows, badV, badM);
    return badV + badM;
}

// The alternating feed: 120 Hz, vsync on, FusionFix's cap 60 known; odd windows 120 frames at exactly 1000/60 ms apart
// (LIMITED), even windows every other frame dropped (no consecutive interval: UNCERTAIN, too few). Nothing settles and no
// window may feed the memory, so the only line due is the liveness line at window 30.
static void AlternatingFeed(int nWin)
{
    ResetStatics();
    static LatFlipWin_t fw; memset(&fw, 0, sizeof(fw));
    hdrRefreshPacked = ((LONGLONG)120 << 32) | 1; latVsync = true; latEndOn = true; latFfCap = 60; latFfRel = 0; latDxvkCap = 0;
    const LONGLONG P = 166667;                                 // 16.6667 ms in 10 MHz ticks
    LONG f = 1000; LONGLONG t = 50000000; int win = 0;
    while (win < nWin)
    {
        const bool drop = (win % 2) == 1;
        for (int k = 0; fw.nB < LAT_WIN; k++)
        {
            f++; t += P + ((f * 7919) % 11) - 5;               // a few ticks of jitter, deterministic
            LatFlipKeep(fw, f, t - 20000, t - 10000);
            if (!drop || (f & 1)) LatFlipTake(fw, f, t, 3, true);
        }
        gCurWin = ++win;
        LatFlipStep(fw, 0.05f, win);
    }
}

// ---- THE KEY AND CHANNEL FEEDS (--key; 2026-09-24, the classifier's verification). The model's dump holds every window's
// key constant (one refresh a whole number of Hz, den 1; no Reset; vsync fixed per cell; latFfRel and latDxvkCap 0; the
// pacer's counters still; no resync inside a window), so these paths were proven only by reading. Each feed below is built
// so the spec's rule gives one answer by a wide margin, at 10 MHz ticks like the dump:
//   FREE:  vsync on, 100 fps with 0.1 ms of jitter, each begin 0.3 ms after the end of the frame three back, block 6 ms
//          (off the grid, anchored to the GPU end: rule 6);
//   PIN:   vsync off, every interval one refresh within 2 us, block 0.2 T (rule 2's "a sync the mod did not ask for");
//   CAP:   vsync on, a fixed rate within 0.5 us, no frame ends (no anchor): LIMITED when a known cap matches, else the
//          rule-7 UNCERTAIN of a period (so every cap channel has a feed that must not match);
//   GRID:  vsync on, begins on the refresh grid (within 2 us) with 30 percent of intervals two refreshes (rule 1).
enum { K_FREE, K_PIN, K_CAP, K_GRID };
struct KOpt { int resetAt = -1, vsyncAt = -1, vsyncTo = 0, refreshAt = -1, resyncAt = -1, paceSlept = 0; LONGLONG refreshTo = 0; double fps = 60.0; };
static LatFlipWin_t kfw;
static LONG kF = 1000; static LONGLONG kT = 50000000; static double kGrid = 0.0; static int kWin = 0, kFails = 0;
static LONGLONG Packed(ULONG num, ULONG den) { return (LONGLONG)(((ULONGLONG)num << 32) | den); }
static void KSession(LONGLONG packed, bool vs)
{
    ResetStatics(); memset(&kfw, 0, sizeof(kfw)); kWin = 0;
    hdrRefreshPacked = packed; latVsync = vs; latEndOn = true; latFfCap = latFfRel = latDxvkCap = 0;
}
static void KWindow(int kind, const KOpt& o = KOpt())
{
    int resetAt = o.resetAt, vsyncAt = o.vsyncAt, refreshAt = o.refreshAt, resyncAt = o.resyncAt;
    for (int k = 0; kfw.nB < LAT_WIN; k++)
    {
        if (kfw.nB == resetAt)   { statDeviceResets++; resetAt = -1; }
        if (kfw.nB == vsyncAt)   { latVsync = o.vsyncTo != 0; vsyncAt = -1; }
        if (kfw.nB == refreshAt) { hdrRefreshPacked = o.refreshTo; refreshAt = -1; }
        if (kfw.nB == resyncAt)  { memset(&kfw, 0, sizeof(kfw)); resyncAt = -1; }   // the waiter's resync: the window starts again
        const LONGLONG pk = hdrRefreshPacked; const ULONG num = (ULONG)((ULONGLONG)pk >> 32), den = (ULONG)pk;
        const double tq = (num && den) ? 1e7 * (double)den / (double)num : 83333.3;
        kF++;
        const LONG j = (kF * 7919) % 41 - 20;                  // +-2 us, deterministic
        LONGLONG step = 0, endF = 0, tIn = 0;
        switch (kind)
        {
        case K_FREE: step = 100000 + (kF * 7919) % 2001 - 1000; break;
        case K_PIN:  step = (LONGLONG)(tq + 0.5) + j; break;
        case K_CAP:  step = (LONGLONG)(1e7 / o.fps + 0.5) + (kF * 7919) % 11 - 5; break;
        default:     { kGrid += tq * (((kF * 7) % 10) < 3 ? 2.0 : 1.0); step = (LONGLONG)(kGrid + 0.5) + j - kT; if (step < 1000) step = 1000; } break;
        }
        kT += step;
        if (kind == K_FREE) { endF = kT + 3 * 100000 - 3000; tIn = endF - 1000; }
        else if (kind != K_CAP) { endF = kT + 3 * (LONGLONG)tq - 30000; tIn = endF - 1000; }
        LatFlipKeep(kfw, kF, tIn, endF);
        LatFlipTake(kfw, kF, kT, 3, true);
        if (o.paceSlept) { latPaceFrames++; if ((kF % 2) == 0) latPaceSleptN++; }   // the pacer slept on half the presents
        else latPaceFrames++;
    }
    gCurWin = ++kWin;
    const float mB = kind == K_FREE ? 6.0f : kind == K_PIN ? (float)(0.2 * 1000.0 / ((double)(ULONG)((ULONGLONG)hdrRefreshPacked >> 32) / (double)(ULONG)hdrRefreshPacked)) : 0.05f;
    LatFlipStep(kfw, mB, kWin);
}
static void KExpect(const char* what, bool ok)
{
    printf("    %s window %2d: %s (verdict %s, display %s, latch %ld)\n", ok ? "ok  " : "FAIL", kWin, what, VName(latFlipLast), VName(latFlipDisplay), latFlipLatchWin);
    if (!ok) kFails++;
}
static bool KLine(const char* text) { return gLastWin == kWin && strstr(gLast, text) != nullptr; }
static int KeyFeeds()
{
    logLevel = 1; gEcho = false;
    const LONGLONG hz120 = Packed(120, 1), hz144 = Packed(144, 1), hz11988 = Packed(120000, 1001);
    printf("  K1. a Reset between windows clears the memory; the key window, wholly after it, fills the first slot\n");
    KSession(hz120, true);
    KWindow(K_FREE); KExpect("FREE, the memory not yet set", latFlipLast == 1 && latFlipDisplay == 0);
    KWindow(K_FREE); KExpect("the memory FREE", latFlipDisplay == 1);
    KExpect("the change line names what a FREE memory cannot tell apart", KLine("Fast Sync over vsync"));
    statDeviceResets++;
    KWindow(K_FREE); KExpect("the key window: the memory cleared", latFlipDisplay == 0);
    KWindow(K_FREE); KExpect("the memory FREE again", latFlipDisplay == 1);
    printf("  K2. a Reset inside a window: that window feeds nothing, so the memory needs two whole windows after it\n");
    KWindow(K_FREE, [] { KOpt o; o.resetAt = 60; return o; }()); KExpect("the straddling window: cleared, and it says it fed nothing", latFlipDisplay == 0 && KLine("straddled a Reset"));
    KWindow(K_FREE); KExpect("the first whole window after: the memory still none", latFlipDisplay == 0);
    KWindow(K_FREE); KExpect("the second: FREE", latFlipDisplay == 1);
    printf("  K3. vsync turned on inside a window (vsync off below the refresh reads UNCERTAIN, and feeds no memory)\n");
    KSession(hz120, false);
    KWindow(K_FREE); KWindow(K_FREE); KExpect("vsync off below the refresh: UNCERTAIN, no memory", latFlipLast == 4 && latFlipDisplay == 0);
    KWindow(K_FREE, [] { KOpt o; o.vsyncAt = 60; o.vsyncTo = 1; return o; }()); KExpect("the straddling window: no memory", latFlipDisplay == 0);
    KWindow(K_FREE); KExpect("the first whole window after: still none", latFlipDisplay == 0);
    KWindow(K_FREE); KExpect("the second: FREE", latFlipDisplay == 1);
    printf("  K4. vsync turned off between windows clears the memory, and vsync-off windows never set it\n");
    latVsync = false;
    KWindow(K_FREE); KExpect("cleared", latFlipDisplay == 0);
    KWindow(K_FREE); KWindow(K_FREE); KExpect("still none", latFlipDisplay == 0);
    printf("  K5. the refresh: read at a window's first frame, so a change inside a window clears at the next\n");
    KSession(hz120, true);
    KWindow(K_FREE); KWindow(K_FREE); KExpect("FREE at 120 Hz", latFlipDisplay == 1);
    KWindow(K_FREE, [&] { KOpt o; o.refreshAt = 60; o.refreshTo = hz144; return o; }()); KExpect("the change inside: judged on the old key, kept", latFlipDisplay == 1);
    KWindow(K_FREE); KExpect("the next window: cleared", latFlipDisplay == 0 && KLine("144.000 Hz"));
    KWindow(K_FREE); KExpect("FREE at 144 Hz", latFlipDisplay == 1);
    hdrRefreshPacked = hz11988;
    KWindow(K_FREE); KExpect("a change between windows, to 120000/1001: cleared", latFlipDisplay == 0 && KLine("119.880 Hz"));
    KWindow(K_FREE); KExpect("FREE at 119.880 Hz", latFlipDisplay == 1);
    printf("  K6. the latch: two pin windows set it, a Reset clears it, and a Reset inside a window feeds its run nothing\n");
    KSession(hz120, false);
    KWindow(K_PIN); KExpect("one pin window: no latch", latFlipLatchWin == 0 && latFlipLast == 4);
    KWindow(K_PIN); KExpect("two: the latch", latFlipLatchWin == 2);
    statDeviceResets++;
    KWindow(K_PIN); KExpect("a Reset between: cleared, and this window starts the run", latFlipLatchWin == 0);
    KWindow(K_PIN); KExpect("latched again", latFlipLatchWin == 4);
    KWindow(K_PIN, [] { KOpt o; o.resetAt = 60; return o; }()); KExpect("a Reset inside: cleared, the run not started", latFlipLatchWin == 0);
    KWindow(K_PIN); KExpect("one whole window after: no latch", latFlipLatchWin == 0);
    KWindow(K_PIN); KExpect("two: latched", latFlipLatchWin == 7);
    printf("  K7. the pacer's share: a pinned run the pacer slept on is its own clock, and never latches\n");
    KSession(hz120, false);
    KWindow(K_PIN, [] { KOpt o; o.paceSlept = 1; return o; }());
    KWindow(K_PIN, [] { KOpt o; o.paceSlept = 1; return o; }()); KExpect("two pacer windows: UNCERTAIN, and the settled line says why", latFlipLast == 4 && KLine("the pacer slept"));
    KWindow(K_PIN, [] { KOpt o; o.paceSlept = 1; return o; }()); KExpect("three: no latch", latFlipLast == 4 && latFlipLatchWin == 0);
    KWindow(K_PIN); KWindow(K_PIN); KExpect("the pacer idle: latched after two", latFlipLatchWin == 5);
    printf("  K8. the cap channels at 120 Hz: none, FusionFix's cap, Custom -60, DXVK's cap\n");
    KSession(hz120, true);
    KWindow(K_CAP); KExpect("60 fps, no known cap: UNCERTAIN (periodic)", latFlipLast == 4);
    latFfCap = 60; KWindow(K_CAP); KExpect("FusionFix 60: LIMITED", latFlipLast == 3);
    latFfCap = 0; latFfRel = 60; KWindow(K_CAP); KExpect("Custom -60 on 120 Hz: LIMITED", latFlipLast == 3);
    latFfRel = 0; latDxvkCap = 60; KWindow(K_CAP); KExpect("DXVK 60: LIMITED", latFlipLast == 3);
    printf("  K9. Custom -n on 120000/1001: both the rounded refresh (120 - 60) and the truncated one (119 - 60) are tried\n");
    KSession(hz11988, true);
    latFfRel = 60;
    KWindow(K_CAP, [] { KOpt o; o.fps = 60.0; return o; }()); KExpect("60 fps: LIMITED (rounded)", latFlipLast == 3);
    KWindow(K_CAP, [] { KOpt o; o.fps = 59.0; return o; }()); KExpect("59 fps: LIMITED (truncated)", latFlipLast == 3 && KLine("limiter Custom -60 = 59 or 60 fps (FusionFix)"));
    latFfRel = 0;
    KWindow(K_CAP, [] { KOpt o; o.fps = 59.0; return o; }()); KExpect("59 fps with no cap: UNCERTAIN", latFlipLast == 4);
    printf("  K10. a rational refresh's grid: QUANTISED at 120000/1001, into the memory\n");
    KSession(hz11988, true); kGrid = (double)kT;
    KWindow(K_GRID); KExpect("QUANTISED", latFlipLast == 2);
    KWindow(K_GRID); KExpect("the memory QUANTISED, on 119.880 Hz", latFlipDisplay == 2 && KLine("119.880 Hz"));
    printf("  K11. a resync inside a window: the window starts again, the memory and the verdict carry on\n");
    KSession(hz120, true);
    KWindow(K_FREE); KWindow(K_FREE); KExpect("FREE", latFlipDisplay == 1);
    KWindow(K_FREE, [] { KOpt o; o.resyncAt = 50; return o; }()); KExpect("after the resync: FREE, the memory kept", latFlipLast == 1 && latFlipDisplay == 1);
    printf(kFails ? "  KEY FEEDS FAILED (%d)\n" : "  key feeds passed\n", kFails);
    return kFails;
}

int main(int argc, char** argv)
{
    if (argc >= 2 && !strcmp(argv[1], "--key")) return KeyFeeds() ? 1 : 0;
    if (argc >= 3 && !strcmp(argv[1], "--log"))
    {
        int fails = 0, w = 0;
        logLevel = 1; gEcho = true;
        printf("  log=1, the model's first cell (F1B-1, G-Sync with vsync on, FREE expected):\n");
        gLines = 0; gFirstLineWin = 0; gLinesBefore30 = 0;
        Replay(argv[2], 1, &w);
        printf("  -> %d change lines in %d windows, the first at window %d (due by window 2)\n", gLines, w, gFirstLineWin);
        if (!gFirstLineWin || gFirstLineWin > 2) fails++;
        printf("  log=1, an alternating LIMITED / UNCERTAIN feed that never settles, 40 windows:\n");
        gLines = 0; gFirstLineWin = 0; gLinesBefore30 = 0;
        AlternatingFeed(40);
        printf("  -> %d change lines, the first at window %d, %d before window 30 (due: the first at 30, none before)\n", gLines, gFirstLineWin, gLinesBefore30);
        if (gFirstLineWin != 30 || gLinesBefore30) fails++;
        printf(fails ? "  LOG TEST FAILED\n" : "  log test passed\n");
        return fails ? 1 : 0;
    }
    if (argc < 2) { printf("usage: harness <dump> | --log <dump>\n"); return 2; }
    const int bad = Replay(argv[1], 0, nullptr);
    return bad == 0 ? 0 : 1;
}
