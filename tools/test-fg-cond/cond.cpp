// cond.cpp: THE BENCH OF FRAME GENERATION'S CONDITIONS TABLE AND OF ITS PLANNED GENERATION TIME (2026-10-10, phase 3 of the
// revival, the build map's stage 8; source/fg/fg_cond.h and source/fg/fg_plan.h, which this includes UNCHANGED).
//
//   cond            every combination, every line, the lifts, the plan: exit 0 when all of it holds
//   cond control    THE CONTROL, WHICH MUST FAIL: the table asked with one constant moved (the rate's bound, 67 to 70) and
//                   the plan with its quantile moved; the same checks must then count mismatches, and it exits 1
//
// WHAT IS CHECKED.
//   1. EVERY COMBINATION of the session's facts over domains that put each row on both of its sides (about eighty
//      million): the table's first row equals an ORACLE's, written here a second time from the map's words in plain
//      floating point, row by row and in the map's order; the mask's bit of every row equals the oracle's row; the
//      class of the answer is its own.
//   2. EVERY ROW is the session's answer for some combination, and so is "no row".
//   3. EVERY LINE is rendered, for every row, with the facts of the first combination that row answered: it begins
//      "frame generation: ", holds no format left over, no dash that is not a hyphen, and fits; every 4096th
//      combination's line is rendered and held to the same.
//   4. THE LIFTS: a row a key lifts is never the answer and never in the mask; the five rows that cannot be lifted
//      still are.
//   5. THE PLAN (fg_plan.h): the quantile's upper edge on histograms with a step, a spike and a tail; too few pairs
//      leave it; the bounds hold; the last bin is the upper bound.
//   6. THE LAW (phase 5, P1; fg_cond.h's FgDrive*): its states against the brief's numbers, written here a second time (LawBench);
//      the control plants a dwell of 2 and a back-off capped at 600 beside the two faults above, and all three must be caught.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <chrono>
typedef long LONG;
typedef long long LONGLONG;   // (2026-10-18, the merge of fg5r-switch: fg_plan.h's FgRoomFloorPm reads it; the bench has no Windows header)
#include "../../source/fg/fg_cond.h"
#include "../../source/fg/fg_plan.h"

static long long gBad = 0;
static void Bad(const char* what, const FgCondIn& in, int got, int want)
{
    if (++gBad <= 12)
        printf("  MISMATCH %s: got %d (%s), the oracle %d (%s) at dead %d lost %d present %d marker %d agility %d dxvk %d async %d/%d refresh %d/%d swap %d/%d windowed %d fp16 %d hdr %d vrr %d ff %d/%d dxvkcap %d menu %d real %d engaged %d lift 0x%X nvcap %d\n",
               what, got, FgCondName(got), want, FgCondName(want), in.machineDead, in.restoreLost, in.presentOurs, in.markerOk, in.agility, in.dxvk31, in.asyncNamed, in.asyncHome,
               in.refreshNum, in.refreshDen, in.swapEffect, in.backBuffers, in.windowed, in.bbFp16, in.hdrOn, in.vrr, in.ffCap, in.ffRel, in.dxvkCap, in.menu, in.realFpsX10, in.engaged, in.lift, in.nvCap);
}

// ---------------------------------------------------------------- the oracle: the map's words, in floating point
struct OracleK { double minC, ffNear, maxReal, minReal, hyst; int minBb, maxBb; bool unknownRefused; };
// (round 5) THE FLOOR BY DISPLAY, written a second time: 30 on a screen of 59 to 61 Hz, the table's floor on every other (no key in the bench's table)
// (round 5 round 2, the reviewer's fourth: "a 59.94 Hz screen is a 60 Hz screen": the 60 Hz floor is 30, or its half refresh to a tenth where
// lower; the half refresh is read to a tenth, rounded: OracleHalf)
static double OracleHalf(double hz) { return floor(hz / 2.0 * 10.0 + 0.5) / 10.0; }
static double OracleMin(double hz, const OracleK& k) { return (hz >= 59.0 && hz <= 61.0) ? (OracleHalf(hz) < 30.0 ? OracleHalf(hz) : 30.0) : k.minReal; }
static bool OracleRow(int c, const FgCondIn& in, const OracleK& k)
{
    const double hz = (in.refreshNum > 0 && in.refreshDen > 0) ? (double)in.refreshNum / (double)in.refreshDen : 0.0;
    const double C = hz > 0.0 ? hz / (2.0 * (double)in.lockPm / 1000.0) : 0.0;            // the ceiling: half the refresh, less the lock
    const double ff = in.ffCap > 0 ? (double)in.ffCap : in.ffRel > 0 ? floor(hz + 0.5) - (double)in.ffRel : 0.0;
    const double real = (double)in.realFpsX10 / 10.0;                                      // the game's own rate (phase 5: engaged, R_free; nothing clamps it)
    const double h = in.engaged ? k.hyst : 0.0;
    const double topC = C > 0.0 ? k.maxReal * C / (120.0 / (2.0 * 1.015)) : k.maxReal;      // the top on this panel (design point 1)
    const double floorP = OracleMin(hz, k) < topC - k.hyst ? OracleMin(hz, k) : topC - k.hyst;              // "the floor follows the panel" (round 3)
    switch (c)
    {
    case FGC_DEAD:        return in.machineDead == 1;                                      // "stays off for the session if its machine dies"
    case FGC_RESTORE:     return in.restoreLost >= 1;                                      // "or a restore is lost"
    case FGC_PRESENT:     return in.presentOurs != 1;
    case FGC_MARKER:      return in.markerOk != 1;
    case FGC_AGILITY:     return in.agility == 1;                                          // "an Agility runtime in DLSSIV\D3D12"
    case FGC_DXVK:        return in.dxvk31 == 0;                                           // known to be another version; unknown refuses nothing
    case FGC_ASYNC:       return in.asyncNamed == 1 && in.asyncHome == 1;                  // "a named pipelined home"
    case FGC_REFRESH:     return hz <= 0.0;
    case FGC_SWAPCHAIN:   return !(in.swapEffect == 1 || in.swapEffect == 2) || in.backBuffers < k.minBb || in.backBuffers > k.maxBb;
    case FGC_EXCLUSIVE:   return in.windowed == 0;                                         // "parks in exclusive fullscreen"
    case FGC_PANEL:       { const bool screen = in.fixedKey > 0 && ((in.vrr == 0 && in.flipQuant != -1) || (in.vrr < 0 && in.flipQuant == 1));
                            if (in.vrr != 0 && in.flipQuant != 1 && hz >= 59.0) return false;   // (round 6: "a variable panel of 59 Hz and up runs", every build; a display read fixed as before)
                            if (screen && !in.vsync && hz >= 59.0) return false;   // (fg18-fix, F2, the release audit: "a fixed 60 to 90 Hz screen with the game's vsync off says to turn vsync on, not a reset row"; under 59 Hz still the panel)
                            return C > 0.0 && C < k.minC; }                                // "a panel under about 97 Hz"
    case FGC_FP16:        return in.bbFp16 == 1 && in.hdrOn == 0;                          // "FP16 without HDR"
    case FGC_SDR:         return in.bbFp16 == 0 && in.hdrOn == 0 && in.sdrMsaa != 0;       // "SDR with no HUD-less picture" (2026-10-20: none of a multisampled back buffer)
    case FGC_FIXED:       return in.vrr == 0;                                              // "no VRR (fixed refresh)"
    case FGC_VRR_UNKNOWN: return in.vrr < 0 && k.unknownRefused;
    case FGC_FFCAP:       return ff > 0.0 && C > 0.0 && ff <= k.ffNear * C;                // "a FusionFix limiter at or under 1.03 C"
    case FGC_DXVKCAP:     return in.dxvkCap > 0;                                           // "a DXVK cap"
    case FGC_MENU:        return in.menu == 1;                                             // "stands by in menus"
    case FGC_FAST:        { const double f = in.freeFpsX10 > 0 ? (double)in.freeFpsX10 / 10.0 : real;                       // "the game's own rate above about 67 at 120 Hz" (engaged R_free, 70: LW2)
                            const double top = C > 0.0 ? k.maxReal * C / (120.0 / (2.0 * 1.015)) : k.maxReal;                // "scaled with C on other panels" (design point 1)
                            return f > 0.0 && f > top + h; }
    case FGC_ONSLOW:      return !in.engaged && in.predFpsX10 > 0 && (double)in.predFpsX10 / 10.0 < floorP - (in.engaging ? k.hyst : 0.0);   // "predicted R_on under the floor: not engaged" (design point 2; S2 round 2: in the wait for a cut, past its hysteresis)
    case FGC_SLOW:        return real > 0.0 && real < floorP - h;                       // "under 38" (amendment 1; engaged 35; 40 and 37 until the owner's word of 2026-10-27)
    case FGC_NVCAP:       return in.nvCap > 0 && C > 0.0 && (double)in.nvCap <= k.ffNear * 2.0 * C;   // "a driver cap at or under the presents a pair makes"
    }
    return false;
}
// THE MAP'S ORDER, written out a second time: the session's rows, then what a Reset can change, then what comes and goes.
static const int kOrder[] = { FGC_DEAD, FGC_RESTORE, FGC_PRESENT, FGC_MARKER, FGC_AGILITY, FGC_DXVK, FGC_ASYNC,
                              FGC_REFRESH, FGC_SWAPCHAIN, FGC_EXCLUSIVE, FGC_PANEL, FGC_FP16, FGC_SDR, FGC_FIXED, FGC_VRR_UNKNOWN,
                              FGC_FFCAP, FGC_DXVKCAP, FGC_MENU, FGC_FAST, FGC_SLOW, FGC_NVCAP, FGC_ONSLOW, FGC_VSYNCOFF };
static const int kOracleClass[FGC_N] = { -1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 };
static bool OracleLiftable(int c) { return !(c == FGC_DEAD || c == FGC_RESTORE || c == FGC_PRESENT || c == FGC_MARKER || c == FGC_REFRESH); }
static int OracleFirst(const FgCondIn& in, const OracleK& k)
{
    for (int c : kOrder)
        if (OracleRow(c, in, k) && !(OracleLiftable(c) && ((in.lift >> c) & 1u))) return c;
    return FGC_OK;
}
// THE FIXED-REFRESH MODE (phase 5, F1, the night of 2026-10-17), written from docs/reports/fg-5/brief-fill/PLAN-fill.md (section 0, B's rules 5
// and 6, "THE LAW", "KNOWING THE SCREEN IS FIXED") and F5's binding reading, not from fg_cond.h: "FGC_FIXED becomes a MODE when vsync is effective
// and the screen is fixed"; the screen is fixed by NVIDIA's "no" or by the classifier's memory where NVIDIA has no answer, and "VRR still needs
// NVAPI's yes", so a memory of QUANTISED under NVIDIA's yes is refused as before; with the key at 0 nothing of it (the table of before); "the
// floor is Hz/2 exactly; SLOW reads R_on against it": in at Hz/2 x 1.03, engaged under 99 of 100 of Hz/2; a fixed display whose half is under
// the table's floor stands by (a fixed 60 Hz at 40); "TOP on fixed is the cap itself": a game at 97 of 100 of the refresh shows it by itself;
// C is half the refresh; the panel row is the variable panel's; "FIXED REFRESH, VSYNC OFF: stand by with one line saying to turn vsync on",
/// the rate rows saying nothing there. (Round 2, 2026-10-18: engaged, the overrun of the law's L stands it down; standing by behind one, the
// game must show the way back in.)
static bool OracleRowF(int c, const FgCondIn& in, const OracleK& k)
{
    // (phase 5 round 2, S2: flipQuant -1, the classifier's memory read FREE standing by, contradicts NVIDIA's no; it is not a quantised memory)
    const bool quant = in.flipQuant == 1, freeSeen = in.flipQuant == -1;
    const bool seen = in.vrr == 0 || quant;
    const bool screen = in.fixedKey > 0 && ((in.vrr == 0 && !freeSeen) || (in.vrr < 0 && quant));
    const bool mode = screen && in.vsync != 0;
    if (c == FGC_FIXED) return seen && !screen;
    if (c == FGC_VRR_UNKNOWN) return in.vrr < 0 && !quant && k.unknownRefused;
    if (c == FGC_VSYNCOFF) return screen && !in.vsync;
    if (screen && !in.vsync && (c == FGC_FAST || c == FGC_SLOW || c == FGC_ONSLOW)) return false;
    if (!mode) return OracleRow(c, in, k);
    const double hz = (in.refreshNum > 0 && in.refreshDen > 0) ? (double)in.refreshNum / (double)in.refreshDen : 0.0;
    const double half = hz / 2.0, real = (double)in.realFpsX10 / 10.0, h = in.engaged ? k.hyst : 0.0;
    const double ff = in.ffCap > 0 ? (double)in.ffCap : in.ffRel > 0 ? floor(hz + 0.5) - (double)in.ffRel : 0.0;
    switch (c)
    {
    case FGC_PANEL:  return false;
    case FGC_FFCAP:  return ff > 0.0 && half > 0.0 && ff <= k.ffNear * half;
    case FGC_NVCAP:  return in.nvCap > 0 && half > 0.0 && (double)in.nvCap <= k.ffNear * 2.0 * half;
    case FGC_FAST:   { const double f = in.freeFpsX10 > 0 ? (double)in.freeFpsX10 / 10.0 : real; return f > 0.0 && f > 0.97 * hz + h; }
    case FGC_SLOW:   if (real <= 0.0) return false;
                     if (OracleHalf(hz) < OracleMin(hz, k)) return true;
                     // (round 7, F1: the held rate's dip stands the mode down only where the game's own rate does not clear 1.03 of half the refresh,
                     // or is unread, and not in the first window of a dip; the overrun is the share of the law's 12 s, fixOver as handed)
                     if (in.engaged) return in.fixOver != 0 || (real < 0.99 * half && !in.fixDipFirst && (in.freeFpsX10 <= 0 || in.freeFpsX10 / 10.0 < 1.03 * half));
                     return real < (in.fixEntryX10 / 10.0 > 1.03 * half ? in.fixEntryX10 / 10.0 : 1.03 * half);   // (round 2: behind an overrun, the way back in)
    case FGC_ONSLOW: return !in.engaged && in.predFpsX10 > 0 && (double)in.predFpsX10 / 10.0 < 1.03 * half - (in.engaging ? k.hyst : 0.0);
    default:         return OracleRow(c, in, k);
    }
}
static int OracleFirstF(const FgCondIn& in, const OracleK& k)
{
    for (int c : kOrder)
        if (OracleRowF(c, in, k) && !(OracleLiftable(c) && ((in.lift >> c) & 1u))) return c;
    return FGC_OK;
}
// ---------------------------------------------------------------- a line, held to its rules
static int gLinesBad = 0;
static bool LineOk(int c, const FgCondIn& in, const FgCondK& k, char* b, size_t cap)
{
    const int n = FgCondLine(c, in, k, b, cap);
    bool ok = n > 20 && n < 230 && strncmp(b, "frame generation: ", 18) == 0 && !strchr(b, '%') && !strchr(b, '\n');
    for (const unsigned char* p = (const unsigned char*)b; *p; p++) if (*p < 0x20 || *p > 0x7E) ok = false;   // plain ASCII: no dash but the hyphen
    if (!ok && ++gLinesBad <= 6) printf("  A LINE BREAKS ITS RULES (row %d, %s, %d characters): %s\n", c, FgCondName(c), n, b);
    return ok;
}

// ---------------------------------------------------------------- THE LAW (phase 5, P1): fg_cond.h's FgDrive*, against the brief's words
// The numbers the law is held to are written here a second time from BRIEF-P1-law.md and PLAN.md 1.B, not taken from FgDriveDefaults:
// a dwell of 3 windows; the cut wait 10 s, at once at 0, a cut or a menu ends it; the first engage waits for the CPU placement 30 windows
// at most; (round 9, L2: behind a stand-down by FGC_SLOW or FGC_FAST rest 1 window and no back-off, the dwell held 30 s behind the second
// short stint running and 60 s from the third; row S) rest 10 windows after any other exit; the back-off 60 windows after a short stint, doubling, capped at 300, back to 60 after a long
// stint, cleared by a Reset or a build and by a band move read while nothing is generated; the health exit under 90 of 100 shown or the
// card's median outside 300 to 700, three windows running, a window too thin to read breaking no run; the memory bounded by those numbers.
// (Round 5, the main session's) A STINT'S WARM-UP OF 3 WINDOWS: no health exit and no rate row (SLOW, FAST, ONSLOW) stands it down there, a
// settings row still does at once; a Reset or a build ends it; c measured over its windows 4 to 8, filed whole or where SLOW stands it down.
static long long gLawBad = 0;
static void LawBad(const char* fmt, ...)
{
    if (++gLawBad > 12) return;
    char b[400]; va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
    printf("  THE LAW: %s\n", b);
}
struct LawRig
{
    FgDriveMem m; FgDriveK k; unsigned now; int cam, menu;
    void Init(const FgDriveK& kk) { k = kk; FgDriveInit(m, k); now = 1000; cam = 0; menu = 0; }
    int Step(const FgDriveIn& in) { now += 2000; return FgDriveStep(m, in, k, now, cam, menu); }
    int Poll(unsigned dt) { now += dt; return FgDriveCut(m, k, now, cam, menu); }
};
static FgDriveIn LawWin(int row, int band = FGDB_IN)
{
    FgDriveIn i = {}; i.row = row; i.band = band; i.cpuReady = 1; return i;
}
static FgDriveIn LawOn(int gen, int n, int cardMed = 0, int cardN = 0)
{
    FgDriveIn i = LawWin(FGC_OK); i.shownGen = gen; i.shownN = n; i.cardMed = cardMed; i.cardN = cardN; return i;
}
// From STANDBY with nothing standing: the windows to the engage (the dwell), then a cut. Returns the windows the dwell took.
static int LawEngage(LawRig& r)
{
    int w = 0;
    while (r.m.state == FGDR_STANDBY && w < 1000) { r.Step(LawWin(FGC_OK)); w++; }
    if (r.m.state == FGDR_ENGAGING) { r.cam++; r.Poll(16); }
    return w;
}
// (round 5) The stint's warm-up of 3 windows, healthy, so what follows is judged as before it.
static void LawWarm(LawRig& r) { for (int i = 0; i < 3; i++) r.Step(LawOn(100, 100)); }
static long long LawBench(const FgDriveK& dk)
{
    gLawBad = 0;
    LawRig r;
    // A. THE DWELL AND THE CUT
    r.Init(dk);
    const int dw = LawEngage(r);
    if (dw != 3) LawBad("the engage came after %d window(s) in band (3 wanted)", dw);
    if (r.m.state != FGDR_ON || r.m.why != FGDRW_CUT || r.m.prev != FGDR_ENGAGING) LawBad("a cut in the wait: state %s, reason %s, from %s (ON, FGDRW_CUT, ENGAGING wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why), FgDriveStateName(r.m.prev));
    // B. THE WAIT ENDS AT 10 S WITH NO CUT; C. A MENU ENDS IT
    r.Init(dk);
    for (int i = 0; i < 3; i++) r.Step(LawWin(FGC_OK));
    if (r.m.state != FGDR_ENGAGING) LawBad("three windows in band: state %s (ENGAGING wanted)", FgDriveStateName(r.m.state));
    { const unsigned t0 = r.m.engT0; r.now = t0 + 9999; r.Poll(0); if (r.m.state != FGDR_ENGAGING) LawBad("9,999 ms into the wait with no cut: state %s (ENGAGING wanted)", FgDriveStateName(r.m.state));
      r.now = t0 + 10000; r.Poll(0); if (r.m.state != FGDR_ON || r.m.why != FGDRW_WAITED) LawBad("10,000 ms into the wait with no cut: state %s, %s (ON, FGDRW_WAITED wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why)); }
    r.Init(dk);
    for (int i = 0; i < 3; i++) r.Step(LawWin(FGC_OK));
    r.menu = 1; r.Poll(16); r.menu = 0;
    if (r.m.state != FGDR_ON || r.m.why != FGDRW_MENU) LawBad("a menu in the wait: state %s, %s (ON, FGDRW_MENU wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why));
    // D. THE WAIT AT 0: AT ONCE
    { FgDriveK k0 = dk; k0.cutWaitMs = 0; r.Init(k0); int w = 0; while (r.m.state == FGDR_STANDBY && w < 10) { r.Step(LawWin(FGC_OK)); w++; }
      if (r.m.state != FGDR_ON || r.m.why != FGDRW_AT_ONCE || w != 3) LawBad("the wait at 0: state %s, %s after %d window(s) (ON, FGDRW_AT_ONCE, 3 wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why), w); }
    // E. THE CPU PLACEMENT FIRST
    r.Init(dk);
    { int w = 0; while (r.m.state == FGDR_STANDBY && w < 100) { FgDriveIn i = LawWin(FGC_OK); i.cpuReady = 0; r.Step(i); w++; }
      if (w != 33) LawBad("the CPU placement never answering: the engage after %d window(s) (30 waited and 3 in band, 33 wanted)", w); }
    // F. A ROW IN THE WAIT
    r.Init(dk);
    for (int i = 0; i < 3; i++) r.Step(LawWin(FGC_OK));
    r.Step(LawWin(FGC_MENU));
    if (r.m.state != FGDR_STANDBY || r.m.why != FGDRW_ROW || r.m.whyRow != FGC_MENU) LawBad("a row in the wait: state %s, %s, %s (STANDBY, FGDRW_ROW, FGC_MENU wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why), FgCondEnumName(r.m.whyRow));
    // G. A LONG STINT, THEN A ROW: (round 9, L2, THE QUICK COME-BACK, renamed from "rest 10": a rate row) rest 1, no back-off
    r.Init(dk); LawEngage(r);
    for (int i = 0; i < 70; i++) r.Step(LawOn(118, 120));
    if (r.m.state != FGDR_ON) LawBad("70 healthy windows engaged: state %s (ON wanted)", FgDriveStateName(r.m.state));
    r.Step(LawWin(FGC_FAST, FGDB_FAST));
    if (r.m.state != FGDR_REST || r.m.why != FGDRW_ROW || r.m.whyRow != FGC_FAST || r.m.backoffLeft != 0) LawBad("a row after a long stint: state %s, %s, %s, back-off %d (REST, FGDRW_ROW, FGC_FAST, 0 wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why), FgCondEnumName(r.m.whyRow), r.m.backoffLeft);
    { int w = 0; while (r.m.state == FGDR_REST && w < 1000) { r.Step(LawWin(FGC_FAST, FGDB_IN)); w++; }   // (the band read IN as before the engage: no move)
      if (w != 1 || r.m.why != FGDRW_RESTED) LawBad("the rest after a long stint stood down by FGC_FAST: %d window(s), %s (1, FGDRW_RESTED wanted: the quick come-back)", w, FgDriveWhyName(r.m.why)); }
    // H. THE HEALTH EXIT ON THE SHOWN SHARE: two sick, one well, three sick; a thin window breaks no run
    r.Init(dk); LawEngage(r); LawWarm(r);
    r.Step(LawOn(80, 100)); r.Step(LawOn(80, 100)); r.Step(LawOn(95, 100));
    if (r.m.state != FGDR_ON) LawBad("two windows at 80 of 100 and one at 95: state %s (ON wanted)", FgDriveStateName(r.m.state));
    r.Step(LawOn(80, 100)); r.Step(LawOn(80, 100)); r.Step(LawOn(2, 10));
    if (r.m.state != FGDR_ON) LawBad("two windows at 80 of 100 and a thin one: state %s (ON wanted)", FgDriveStateName(r.m.state));
    r.Step(LawOn(89, 100));
    if (r.m.state != FGDR_REST || r.m.why != FGDRW_SHOWN) LawBad("three windows running under 90 of 100 (a thin one between): state %s, %s (REST, FGDRW_SHOWN wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why));
    r.Init(dk); LawEngage(r); LawWarm(r);
    for (int i = 0; i < 6; i++) r.Step(LawOn(90, 100));
    if (r.m.state != FGDR_ON) LawBad("six windows at exactly 90 of 100: state %s (ON wanted)", FgDriveStateName(r.m.state));
    // I. THE HEALTH EXIT ON THE CARD
    r.Init(dk); LawEngage(r); LawWarm(r);
    r.Step(LawOn(100, 100, 700, 50)); r.Step(LawOn(100, 100, 300, 50)); r.Step(LawOn(100, 100, 750, 4));
    if (r.m.state != FGDR_ON) LawBad("the card at 700, 300 and a thin 750: state %s (ON wanted)", FgDriveStateName(r.m.state));
    r.Step(LawOn(100, 100, 701, 50)); r.Step(LawOn(100, 100, 299, 50)); r.Step(LawOn(100, 100, 760, 50));
    if (r.m.state != FGDR_REST || r.m.why != FGDRW_CARD) LawBad("the card at 701, 299 and 760: state %s, %s (REST, FGDRW_CARD wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why));
    // J. THE BACK-OFF DOUBLES AND CAPS: five short stints, each a health exit; the rest's windows each time
    r.Init(dk);
    {
        static const int want[6] = { 70, 130, 250, 310, 310, 310 };
        for (int c = 0; c < 6; c++)
        {
            LawEngage(r); LawWarm(r);
            for (int i = 0; i < 3; i++) r.Step(LawOn(50, 100));
            int w = 0; while (r.m.state == FGDR_REST && w < 2000) { r.Step(LawWin(FGC_OK)); w++; }
            if (w != want[c]) LawBad("short stint %d: %d window(s) before it stood by again (%d wanted: rest 10 and the back-off 60, 120, 240, 300, 300)", c + 1, w, want[c]);
        }
    }
    // K. A LONG STINT PUTS THE BACK-OFF BACK TO ITS START
    {
        LawEngage(r);
        for (int i = 0; i < 61; i++) r.Step(LawOn(100, 100));
        for (int i = 0; i < 3; i++) r.Step(LawOn(50, 100));
        int w = 0; while (r.m.state == FGDR_REST && w < 2000) { r.Step(LawWin(FGC_OK)); w++; }
        if (w != 10 || r.m.backoffNext != 60) LawBad("a health exit after a long stint: %d window(s) of rest, the next back-off %d (10 and 60 wanted)", w, r.m.backoffNext);
    }
    // L. A RESET OR A BUILD CLEARS THE REST AND THE BACK-OFF
    r.Init(dk); LawEngage(r); LawWarm(r);
    for (int i = 0; i < 3; i++) r.Step(LawOn(50, 100));
    for (int i = 0; i < 4; i++) r.Step(LawWin(FGC_OK));
    { FgDriveIn i = LawWin(FGC_OK); i.cleared = 1; r.Step(i); }
    if (r.m.state != FGDR_STANDBY || r.m.why != FGDRW_CLEARED || r.m.backoffNext != 60 || r.m.restLeft || r.m.backoffLeft) LawBad("a Reset mid-rest: state %s, %s, rest %d, back-off %d, next %d (STANDBY, FGDRW_CLEARED, 0, 0, 60 wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why), r.m.restLeft, r.m.backoffLeft, r.m.backoffNext);
    // M. A BAND MOVE WHILE NOTHING IS GENERATED CLEARS THEM; AN ENGAGED READING DOES NOT
    r.Init(dk); LawEngage(r); LawWarm(r);                                         // (the standby band: IN)
    r.Step(LawWin(FGC_FAST, FGDB_FAST));                                          // (engaged: R_free over 70)
    if (r.m.state != FGDR_REST) LawBad("FAST engaged: state %s (REST wanted)", FgDriveStateName(r.m.state));
    r.Step(LawWin(FGC_FAST, FGDB_FAST));
    if (r.m.state != FGDR_STANDBY || r.m.why != FGDRW_BAND) LawBad("standing by the game reads FAST, against IN before the engage: state %s, %s (STANDBY, FGDRW_BAND wanted)", FgDriveStateName(r.m.state), FgDriveWhyName(r.m.why));
    r.Init(dk); LawEngage(r); LawWarm(r);
    r.Step(LawWin(FGC_SLOW, FGDB_SLOW));                                          // (THE STRUCTURAL FLAP: the game at 42 by itself runs at 36 with generation)
    { int w = 0; while (r.m.state == FGDR_REST && w < 2000) { r.Step(LawWin(FGC_OK, FGDB_IN)); w++; }   // (round 9, L2: renamed from "the back-off holds", 70)
      if (w != 1 || r.m.why != FGDRW_RESTED) LawBad("a stand-down for SLOW engaged, the game IN standing by as before: %d window(s), %s (1, FGDRW_RESTED wanted: the quick come-back; the prediction keeps it out where the frames do not allow it)", w, FgDriveWhyName(r.m.why)); }
    // S. (round 9, L2) THE QUICK COME-BACK AND ITS GUARD, by the law's own numbers written a second time here: behind a stand-down by FGC_SLOW or
    // FGC_FAST the rest is 1 window and no back-off; a stint under 60 windows is short; from the second short stint running the dwell does not
    // count for 30 s from the stand-down, from the third 60 s (the cap); a long stint, a Reset or a build clear the run and the hold; a band move
    // does not clear the hold; quickRestWin 0 is the law of before
    {
        auto shortSlow = [&](LawRig& q) { LawEngage(q); LawWarm(q); q.Step(LawWin(FGC_SLOW, FGDB_IN)); };   // (an engage, the warm-up, SLOW: a short stint)
        auto backAfter = [&](LawRig& q) { int w = 0; while (q.m.state != FGDR_ENGAGING && q.m.state != FGDR_ON && w < 400) { q.Step(LawWin(FGC_OK, FGDB_IN)); w++; } return w; };
        LawRig q; q.Init(dk);
        shortSlow(q);
        if (q.m.state != FGDR_REST || q.m.restLeft != 1 || q.m.backoffLeft != 0 || q.m.flapRun != 1 || q.m.holdMs != 0) LawBad("the first short stint stood down by SLOW: %s, rest %d, back-off %d, short stints running %d, the hold %d ms (REST, 1, 0, 1, 0 wanted)", FgDriveStateName(q.m.state), q.m.restLeft, q.m.backoffLeft, q.m.flapRun, q.m.holdMs);
        { const int w = backAfter(q); if (w != 4) LawBad("behind the first short stint: the wait for a cut after %d window(s) (4 wanted: the rest of 1 and the dwell of 3)", w); }
        q.cam++; q.Poll(16);
        LawWarm(q); q.Step(LawWin(FGC_SLOW, FGDB_IN));
        if (q.m.flapRun != 2 || q.m.holdMs != 30000) LawBad("the second short stint running: %d running, the hold %d ms (2, 30000 wanted)", q.m.flapRun, q.m.holdMs);
        { const unsigned t0 = q.m.holdT0; const int w = backAfter(q); const unsigned dt = q.now - t0;
          if (dt < 30000u || dt > 30000u + 4 * 2000u) LawBad("behind the second: the wait for a cut %u ms after the stand-down (%d windows of 2 s; 30 s and the dwell's 6 s, about 36,000 ms, wanted)", dt, w); }
        q.cam++; q.Poll(16);
        LawWarm(q); q.Step(LawWin(FGC_SLOW, FGDB_IN));
        if (q.m.flapRun != 3 || q.m.holdMs != 60000) LawBad("the third short stint running: %d running, the hold %d ms (3, 60000 wanted)", q.m.flapRun, q.m.holdMs);
        q.Step(LawWin(FGC_FAST, FGDB_FAST));                                       // (a band move in the rest's window: STANDBY by FGDRW_BAND)
        if (q.m.state != FGDR_STANDBY || q.m.why != FGDRW_BAND || q.m.holdMs != 60000) LawBad("a band move behind the third: %s, %s, the hold %d ms (STANDBY, FGDRW_BAND, the hold kept at 60000 wanted)", FgDriveStateName(q.m.state), FgDriveWhyName(q.m.why), q.m.holdMs);
        { const unsigned t0 = q.m.holdT0; backAfter(q); const unsigned dt = q.now - t0;
          if (dt < 60000u) LawBad("behind the third and a band move: the wait for a cut %u ms after the stand-down (60 s or more wanted)", dt); }
        q.cam++; q.Poll(16);
        LawWarm(q); q.Step(LawWin(FGC_SLOW, FGDB_IN));
        if (q.m.flapRun != 4 || q.m.holdMs != 60000) LawBad("the fourth: %d running, the hold %d ms (4, 60000 wanted: the cap)", q.m.flapRun, q.m.holdMs);
        { FgDriveIn i = LawWin(FGC_OK); i.cleared = 1; q.Step(i); }
        if (q.m.flapRun != 0 || q.m.holdMs != 0 || q.m.state != FGDR_STANDBY) LawBad("a Reset in the hold: %d running, the hold %d ms, %s (0, 0, STANDBY wanted)", q.m.flapRun, q.m.holdMs, FgDriveStateName(q.m.state));
        LawRig l; l.Init(dk); shortSlow(l); backAfter(l); l.cam++; l.Poll(16);      // (one short stint, then a long one, then SLOW)
        for (int i = 0; i < 70; i++) l.Step(LawOn(118, 120));
        l.Step(LawWin(FGC_SLOW, FGDB_IN));
        if (l.m.flapRun != 0 || l.m.holdMs != 0 || l.m.restLeft != 1) LawBad("a long stint stood down by SLOW behind a short one: %d running, the hold %d ms, rest %d (0, 0, 1 wanted)", l.m.flapRun, l.m.holdMs, l.m.restLeft);
        {   // (round 9, L3b) THE FIXED MODE: at once behind the dwell, the cut wait and the CPU placement not waited; the same law elsewhere waits
            LawRig f; f.Init(dk); int w = 0;
            while (f.m.state == FGDR_STANDBY && w < 100) { FgDriveIn i = LawWin(FGC_OK); i.cpuReady = 0; i.fixed = 1; f.Step(i); w++; }
            if (f.m.state != FGDR_ON || f.m.why != FGDRW_AT_ONCE || w != 3) LawBad("the fixed mode, the placement not answered, the cut wait %d ms: %s, %s after %d window(s) (ON, FGDRW_AT_ONCE, 3 wanted)", dk.cutWaitMs, FgDriveStateName(f.m.state), FgDriveWhyName(f.m.why), w);
            LawRig v; v.Init(dk); int u = 0;
            while (v.m.state == FGDR_STANDBY && u < 100) { FgDriveIn i = LawWin(FGC_OK); i.cpuReady = 0; v.Step(i); u++; }
            if (v.m.state != FGDR_ENGAGING || u != 33) LawBad("not the fixed mode, the placement not answered: %s after %d window(s) (ENGAGING, 33 wanted: as before)", FgDriveStateName(v.m.state), u);
        }
        FgDriveK old = dk; old.quickRestWin = 0;
        LawRig o; o.Init(old); shortSlow(o);
        { int w = 0; while (o.m.state == FGDR_REST && w < 2000) { o.Step(LawWin(FGC_OK, FGDB_IN)); w++; }
          if (w != 70) LawBad("quickRestWin 0, a short stint stood down by SLOW: %d window(s) of rest (70 wanted: the law of before)", w); }
    }
    // N. THE MEMORY IS BOUNDED BY THE NUMBERS ABOVE, whatever the windows bring (200,000 windows of random readings)
    {
        r.Init(dk); unsigned s = 12345u; int restRun = 0, restMost = 0; long long changes = 0;
        for (int i = 0; i < 200000; i++)
        {
            s = s * 1664525u + 1013904223u; const unsigned x = s >> 8;
            FgDriveIn in = LawWin((x & 7) == 0 ? FGC_SLOW : (x & 15) == 1 ? FGC_FAST : FGC_OK, 1 + (int)((x >> 4) % 3));
            in.shownN = 100; in.shownGen = 70 + (int)((x >> 6) % 31); in.cardN = 20; in.cardMed = 250 + (int)((x >> 11) % 500); in.cleared = (x % 997) == 0; in.cpuReady = 1;
            if ((x >> 20) & 1) r.cam++;
            changes += r.Step(in);
            if (r.m.state == FGDR_ENGAGING && ((x >> 21) & 3) == 0) { r.cam++; r.Poll(16); }
            restRun = r.m.state == FGDR_REST ? restRun + 1 : 0; if (restRun > restMost) restMost = restRun;
            if (r.m.restLeft < 0 || r.m.restLeft > 10 || r.m.backoffLeft < 0 || r.m.backoffLeft > 300 || r.m.backoffNext < 60 || r.m.backoffNext > 300 || r.m.onWin > 60
                || r.m.sickShown > 3 || r.m.sickCard > 3 || r.m.dwell > 3 || r.m.state < 0 || r.m.state >= FGDR_N
                || r.m.warmLeft < 0 || r.m.warmLeft > 3 || (r.m.warmLeft > 0 && r.m.state != FGDR_ON) || r.m.postWin < 0 || r.m.postWin > 6
                || r.m.holdMs < 0 || r.m.holdMs > 60000 || r.m.flapRun < 0 || r.m.flapRun > 1000)   // (round 9, L2: the come-back's guard)
            { LawBad("window %d: a count past its bound (rest %d, back-off %d, next %d, stint %d, sick %d/%d, dwell %d, state %d, warm-up left %d, behind it %d)", i, r.m.restLeft, r.m.backoffLeft, r.m.backoffNext, r.m.onWin, r.m.sickShown, r.m.sickCard, r.m.dwell, r.m.state, r.m.warmLeft, r.m.postWin); break; }
        }
        if (restMost > 310) LawBad("a rest of %d windows running (310 at the most wanted)", restMost);
        printf("    200,000 windows of random readings: %lld change(s) of state, %d engage(s), the longest rest %d window(s); every count within its bound\n", changes, r.m.engages, restMost);
    }
    // (O. WHO DRIVES: retired in round 9's sweep with FgDriveWhoOf; the law is the one driver since round 8's deep strip took the forced
    // driver and the shadow schedule out)
    // P. THE FLOOR'S KEY AND THE READER
    {
        struct { int key, want; } f[] = { { 30, 300 }, { 59, 590 }, { 29, 380 }, { 60, 600 }, { 0, 380 }, { 40, 400 }, { 240, 2400 }, { 241, 380 } };   // (round 3: 30 to 240)
        { // (round 3: THE FLOOR FOLLOWS THE PANEL: the key at 80 is 64 at 120 Hz, 80 at 240; the default 38 untouched at either)
            // (round 5: THE FLOOR BY DISPLAY: no key, 30 on a 60 Hz screen and 38 elsewhere (40 until 2026-10-27); the key at 40 holds on 60 Hz too, under the top)
            struct { int hz, key, want; } fp[] = { { 120, 80, 640 }, { 240, 80, 800 }, { 120, 40, 400 }, { 240, 40, 400 }, { 144, 79, 773 }, { 60, 0, 300 }, { 120, 0, 380 }, { 240, 0, 380 }, { 60, 40, 304 } };
            for (auto& x : fp) { FgCondK k = FgCondDefaults(); FgCondFloorKey(k, x.key); FgCondIn ti = {}; ti.refreshNum = x.hz; ti.refreshDen = 1; ti.lockPm = 1015;
                                 if (FgCondFloorX10(ti, k) != x.want) LawBad("the floor at %d Hz with the key at %d: %d (%d wanted)", x.hz, x.key, FgCondFloorX10(ti, k), x.want); }
        }
        for (auto& x : f) { FgCondK k = FgCondDefaults(); FgCondFloorKey(k, x.key); if (k.minRealFpsX10 != x.want) LawBad("the floor's key %d: %d (%d wanted)", x.key, k.minRealFpsX10, x.want); }
        if (FgCondRateX10(11.0, 0) != 909 || FgCondRateX10(0.0, 20000) != 500 || FgCondRateX10(0.0, 0) != 0)
            LawBad("the measured rate: at 11 ms %d, nothing but a period of 20 ms %d, nothing %d (909, 500, 0 wanted)", FgCondRateX10(11.0, 0), FgCondRateX10(0.0, 20000), FgCondRateX10(0.0, 0));
        FgCondIn in = {}; in.refreshNum = 120000; in.refreshDen = 1001; in.lockPm = 1015; in.engaged = 1; in.realFpsX10 = 591; in.freeFpsX10 = 704;
        if (!FgCondHolds(FGC_FAST, in, FgCondDefaults())) LawBad("engaged at 119.88 Hz paced at 59.1 with R_free %d: FAST does not hold (it must, over 66.8 and 3)", in.freeFpsX10);
        in.freeFpsX10 = 0;
        if (FgCondHolds(FGC_FAST, in, FgCondDefaults())) LawBad("engaged with no R_free, the pacer's 59.1 read: FAST holds (it must not)");
        // THE TOP SCALED WITH C (design point 1): 67 at a 120 Hz panel, 80 at 144, 134 at 240, 55 at 100
        { struct { int num, den, want; } t[] = { { 120, 1, 670 }, { 144, 1, 803 }, { 240, 1, 1340 }, { 100, 1, 557 } };
          for (auto& x : t) { FgCondIn ti = {}; ti.refreshNum = x.num; ti.refreshDen = x.den; ti.lockPm = 1015; const int got = FgCondTopX10(ti, FgCondDefaults()); if (got != x.want) LawBad("the top at %d Hz: %d (%d wanted)", x.num, got, x.want); } }
        // R_FREE WHILE ENGAGED: the reading standing by (G4, below) where the pacer holds the game at the panel's floor (the period at the floor within 1 percent, the median
        // within 3 percent of it), the measured rate everywhere else (a busy card: E 11,000 us, a CPU frame, against the game running at
        // 20.2 ms; the room floor raising the period to the card's cycle); NONE where E is 0 (a forced period: no law step ran) or no period
        // (ROUND 3 OF THE ROUND, G4, 2026-10-18: HELD, the samples are the render thread's part alone, an upper bound of the rate: R_free is the
        // game's own rate as the law last read it standing by, never over that bound; none with no such reading; the evening's fixed120 leg,
        // the law's median sample 3,405 us under the fixed mode's 16,673 against a game of 93.4 by itself: 93.4, where the reader of before said 293.7)
        struct { int e, per, floor; double med; int alone, want; const char* what; } fr[] = {
            { 14200, 16920, 16920, 16.92, 620, 620, "held at the floor: the game's own rate standing by, the samples' 70.4 a bound over it" },
            { 14200, 16920, 16920, 17.40, 620, 620, "2.8 percent over the floor: still held" },
            { 14200, 16920, 16920, 16.92, 800, 704, "the game heavier while held than standing by: the samples' bound" },
            { 14200, 16920, 16920, 16.92, 0, 0, "held, nothing read standing by: none" },
            { 3405, 16673, 16666, 16.72, 934, 934, "the fixed120 leg's window 11: 93.4, not 293.7" },
            { 11000, 16920, 16920, 20.2, 620, 495, "the card holds the game under the floor: the measured rate, not E's 90.9" }, { 11000, 16920, 16920, 17.50, 620, 571, "3.4 percent over it: the measured rate" },
            { 11000, 22400, 16920, 22.4, 620, 446, "the room floor raised the period to the card's cycle: the measured rate" },
            { 0, 16920, 16920, 16.92, 620, 620, "held at the floor with no sample: the reading standing by, never a rate read from the 0" }, { 0, 20000, 16920, 20.0, 620, 500, "a period forced over the floor: the measured rate" },
            { 11000, 0, 16920, 20.2, 620, 0, "no period: none" }, { 11000, 16920, 0, 20.2, 620, 0, "no floor read: none" } };
        for (auto& x : fr) { const int got = FgCondFreeX10(x.e, x.per, x.floor, x.med, x.alone); if (got != x.want) LawBad("R_free, %s: %d (%d wanted)", x.what, got, x.want); }
        { FgCondIn fi = {}; fi.refreshNum = 120000; fi.refreshDen = 1001; fi.lockPm = 1015; if (FgCondFloorUs(fi) < 16930 || FgCondFloorUs(fi) > 16936) LawBad("the floor at 119.88 Hz: %d us (16,933 wanted)", FgCondFloorUs(fi)); }
        // THE ADDED TIME ON THE CARD (round 2): R_on = 1 / (1/R + c). None measured: c = 0, optimistic, whatever the card. A stint at 60 by
        // itself on a card that is the limit, 45 engaged in its windows 2 to 6: c 5,555 us, filed as the limit's, the prediction 45.0 for a
        // card that is the limit and for one that cannot be read, still 60 for a card with room (none measured with room); a stint with room
        // files c under room; a row in the stint's first window that stands it down still leaves its c; a Reset or a build forgets both.
        // The card's reading: in flight half a frame or more, or the latency pacer sleeping on half the presents, is the limit.
        {
            LawRig q; q.Init(dk);
            if (FgDrivePredX10(q.m, 600, 1) != 600 || FgDrivePredX10(q.m, 450, -1) != 450 || FgDrivePredX10(q.m, 520, 0) != 520) LawBad("the prediction with nothing measured: %d, %d, %d (600, 450, 520 wanted: optimistic)", FgDrivePredX10(q.m, 600, 1), FgDrivePredX10(q.m, 450, -1), FgDrivePredX10(q.m, 520, 0));
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 600; w.card = 1; q.Step(w); }
            q.cam++; q.Poll(16);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 300; q.Step(w); }   // (round 5: the warm-up's three windows: not samples)
            if (q.m.cLimUs != -1) LawBad("c behind the warm-up alone: %d us (none wanted)", q.m.cLimUs);
            for (int i = 0; i < 4; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 450; q.Step(w); }
            if (q.m.cLimUs != -1) LawBad("c behind four windows of five: %d us (none wanted: filed whole)", q.m.cLimUs);
            for (int i = 4; i < 8; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = i < 5 ? 450 : 200; q.Step(w); }   // (windows 4 to 8 at 45; 9 on are not samples)
            if (q.m.cLimUs != 5556 || q.m.cRoomUs != -1) LawBad("c of a stint at 60 by itself and 45 engaged on a card that was the limit: limit %d, room %d (5,556 and none wanted: windows 4 to 8 alone)", q.m.cLimUs, q.m.cRoomUs);
            // (S2 review round 2: idle room with nothing measured with room takes the limit's c: a measured cost beats an unmeasured zero)
            if (FgDrivePredX10(q.m, 600, 1) != 450 || FgDrivePredX10(q.m, 600, -1) != 450 || FgDrivePredX10(q.m, 600, 0) != 450) LawBad("the prediction behind that stint, by itself at 60: the card the limit %d, cannot tell %d, room %d (450, 450, 450 wanted)", FgDrivePredX10(q.m, 600, 1), FgDrivePredX10(q.m, 600, -1), FgDrivePredX10(q.m, 600, 0));
            FgDriveIn cl = LawOn(120, 120); cl.cleared = 1; q.Step(cl);
            if (q.m.cLimUs != -1 || q.m.cRoomUs != -1) LawBad("a Reset or a build: c %d and %d (both forgotten wanted)", q.m.cLimUs, q.m.cRoomUs);
            LawRig p; p.Init(dk);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 500; w.card = 0; p.Step(w); }
            p.cam++; p.Poll(16);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_SLOW); w.realX10 = 300; w.shownN = 100; w.shownGen = i * 40; p.Step(w); }   // (round 5: SLOW in the warm-up does not stand it down)
            if (p.m.state != FGDR_ON || p.m.cRoomUs != -1) LawBad("SLOW in a stint's three warm-up windows: %s, c with room %d (ON, none wanted)", FgDriveStateName(p.m.state), p.m.cRoomUs);
            { FgDriveIn w = LawWin(FGC_SLOW); w.realX10 = 360; w.shownN = 100; w.shownGen = 95; p.Step(w); }   // (SLOW in the stint's first window behind its warm-up: its c is kept)
            // (ROUND 3: c only beyond the law's own period: a stint at 55 by itself paced at the free range's start of 20 ms leaves 0; and
            // only from a window that generated: a window of a menu at 30 with nothing shown is not a sample)
            LawRig f; f.Init(dk);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 550; w.card = 0; f.Step(w); }
            f.cam++; f.Poll(16);
            for (int i = 0; i < 8; i++) { FgDriveIn w = LawOn(118, 120); w.realX10 = 500; w.lawPerUs = 20000; f.Step(w); }   // (round 5: the warm-up's 3 and the 5 behind it)
            if (f.m.cRoomUs != 0) LawBad("a stint at 55 by itself paced at the free range's start (20 ms): c %d us (0 wanted: the pacer's choice is never cost)", f.m.cRoomUs);
            LawRig g; g.Init(dk);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 600; w.card = 1; g.Step(w); }
            g.cam++; g.Poll(16);
            { FgDriveIn w = LawOn(120, 120); g.Step(w); }
            for (int i = 0; i < 5; i++) { FgDriveIn w = LawOn(0, 120); w.realX10 = 300; g.Step(w); }   // (a menu: nothing generated, 30 a second)
            if (g.m.cLimUs != -1) LawBad("five windows of a menu with nothing generated: c %d us (none wanted)", g.m.cLimUs);
            { FgDriveIn w = LawOn(10, 20); w.realX10 = 300; g.Step(w); }                              // (too few real frames to read: not a sample)
            if (g.m.cLimUs != -1) LawBad("a window of 20 real frames: c %d us (none wanted)", g.m.cLimUs);
            if (p.m.cRoomUs != 7777 || p.m.cLimUs != -1 || p.m.state != FGDR_REST) LawBad("SLOW in the first window behind the warm-up of a stint with room: room %d, limit %d, %s (7,777, none, REST wanted)", p.m.cRoomUs, p.m.cLimUs, FgDriveStateName(p.m.state));
            // (G4's round 3: and the present's block against the game's interval: fill95's window 65, 5,700 us of 10,410, the limit; flw's 10 of
            // 16,900, room; a block under 1,500 us, room whatever its share)
            struct { int q, s, n, b, iv, want; } cr[] = { { 10, 0, 120, -1, 0, 1 }, { 5, 0, 120, -1, 0, 1 }, { 4, 60, 120, -1, 0, 1 }, { 4, 59, 120, -1, 0, 0 }, { 0, 0, 120, -1, 0, 0 }, { -1, 0, 0, -1, 0, -1 }, { -1, 80, 120, -1, 0, 1 },
                                                          { 0, 0, 120, 5700, 10410, 1 }, { 0, 0, 120, 10, 16900, 0 }, { 0, 0, 120, 1400, 4000, 0 }, { 0, 0, 120, 3100, 10410, 0 }, { 0, 0, 120, 3130, 10410, 1 } };
            for (auto& x : cr) if (FgDriveCardOf(x.q, x.s, x.n, x.b, x.iv) != x.want) LawBad("the card at in flight %d tenths, the pacer on %d of %d, the block %d us of %d: %d (%d wanted)", x.q, x.s, x.n, x.b, x.iv, FgDriveCardOf(x.q, x.s, x.n, x.b, x.iv), x.want);
            // (a row of the player's settings stands down at once, with no rest and no back-off; a rate row rests)
            LawRig r; r.Init(dk); LawEngage(r); r.Step(LawWin(FGC_FFCAP));             // (round 5: in the warm-up's first window: a settings row acts at once)
            if (r.m.state != FGDR_STANDBY || r.m.restLeft || r.m.backoffLeft || r.m.warmLeft) LawBad("FusionFix's limiter while engaged, in the warm-up: %s, rest %d, back-off %d, warm-up left %d (STANDBY, 0, 0, 0 wanted)", FgDriveStateName(r.m.state), r.m.restLeft, r.m.backoffLeft, r.m.warmLeft);
            int w2 = 0; while (r.m.state == FGDR_STANDBY && w2 < 20) { r.Step(LawWin(FGC_OK)); w2++; }
            if (w2 != 3) LawBad("the limiter lifted: engaged again after %d window(s) (the dwell, 3, wanted)", w2);
        }
    }

    // R. A STINT'S WARM-UP (round 5): three windows; not a latch
    {
        LawRig a; a.Init(dk); LawEngage(a);
        const unsigned rate = (1u << FGC_SLOW) | (1u << FGC_FAST) | (1u << FGC_ONSLOW);
        if (a.m.warmLeft != 3 || FgDriveWarmLift(a.m) != rate) LawBad("at the engage: warm-up left %d, the lift 0x%X (3 and SLOW, FAST, ONSLOW 0x%X wanted)", a.m.warmLeft, FgDriveWarmLift(a.m), rate);
        a.Step(LawWin(FGC_SLOW, FGDB_SLOW)); a.Step(LawWin(FGC_FAST, FGDB_FAST)); a.Step(LawOn(0, 100));
        if (a.m.state != FGDR_ON || a.m.sickShown || FgDriveWarmLift(a.m) != 0) LawBad("SLOW, FAST and 0 of 100 shown in the warm-up's three windows: %s, sick %d, the lift 0x%X (ON, 0, none wanted)", FgDriveStateName(a.m.state), a.m.sickShown, FgDriveWarmLift(a.m));
        a.Step(LawWin(FGC_SLOW, FGDB_SLOW));
        if (a.m.state != FGDR_REST || a.m.why != FGDRW_ROW || a.m.backoffLeft != 0 || a.m.restLeft != 1 || a.m.flapRun != 1) LawBad("SLOW in the window behind the warm-up: %s, %s, back-off %d, rest %d, short stints running %d (REST, FGDRW_ROW, 0, 1, 1 wanted: a short stint behind its warm-up, the quick come-back of round 9's L2; it was back-off 60)", FgDriveStateName(a.m.state), FgDriveWhyName(a.m.why), a.m.backoffLeft, a.m.restLeft, a.m.flapRun);
        LawRig b; b.Init(dk); LawEngage(b);
        for (int i = 0; i < 5; i++) b.Step(LawOn(10, 100));
        if (b.m.state != FGDR_ON) LawBad("five windows at 10 of 100 from the engage: %s (ON wanted: three of warm-up, two judged)", FgDriveStateName(b.m.state));
        b.Step(LawOn(10, 100));
        if (b.m.state != FGDR_REST || b.m.why != FGDRW_SHOWN || b.m.cLimUs != -1 || b.m.cRoomUs != -1) LawBad("the sixth: %s, %s, c %d and %d (REST, FGDRW_SHOWN, none and none wanted: a health exit learns nothing)", FgDriveStateName(b.m.state), FgDriveWhyName(b.m.why), b.m.cLimUs, b.m.cRoomUs);
        LawRig c; c.Init(dk); LawEngage(c);
        { FgDriveIn i = LawOn(100, 100); i.cleared = 1; c.Step(i); }
        if (c.m.warmLeft != 0 || c.m.state != FGDR_ON) LawBad("a Reset in the warm-up's first window: warm-up left %d, %s (0, ON wanted)", c.m.warmLeft, FgDriveStateName(c.m.state));
        c.Step(LawWin(FGC_SLOW, FGDB_SLOW));
        if (c.m.state != FGDR_REST) LawBad("SLOW behind a Reset that ended the warm-up: %s (REST wanted)", FgDriveStateName(c.m.state));
        LawRig d; d.Init(dk); LawEngage(d); d.Step(LawWin(FGC_NVCAP));          // (a settings row: the menu is the present path's, never a row the law is handed)
        if (d.m.state != FGDR_STANDBY || d.m.warmLeft != 0 || FgDriveWarmLift(d.m) != 0) LawBad("NVIDIA's cap in the warm-up: %s, warm-up left %d (STANDBY, 0 wanted)", FgDriveStateName(d.m.state), d.m.warmLeft);
        LawEngage(d);
        if (d.m.state != FGDR_ON || d.m.warmLeft != 3) LawBad("the engage behind it: %s, warm-up left %d (ON, 3 wanted: a warm-up of its own)", FgDriveStateName(d.m.state), d.m.warmLeft);
        FgDriveK k0 = dk; k0.warmWin = 0; LawRig e; e.Init(k0); LawEngage(e); e.Step(LawWin(FGC_SLOW, FGDB_SLOW));
        if (e.m.state != FGDR_REST) LawBad("with the warm-up at 0, SLOW in the stint's first window: %s (REST wanted, round 4's law)", FgDriveStateName(e.m.state));
    }
    // S. PHASE 5 ROUND 2 (S2, 2026-10-18): "ENGAGED" BY THE LAW'S OWN STATE; THE CARD'S READING TAKEN WHERE THE LAW HELD NOTHING, STANDING
    // OVER THREE; C'S MARGIN; THE PACER'S FLOOR NEVER C; A RAMP TEACHES NOTHING
    {
        // ("engaged": where the law drives its own state, whatever the want word; any other driver its want; a row that stands, never)
        struct { int law, on, want, ref, eng; } en[] = { { 1, 1, 0, 0, 1 }, { 1, 0, 1, 0, 0 }, { 1, 1, 1, 0, 1 }, { 1, 1, 1, FGC_SLOW, 0 }, { 0, 0, 1, 0, 1 }, { 0, 1, 0, 0, 0 }, { 0, 0, 1, FGC_MENU, 0 } };
        for (auto& x : en) if (FgCondEngagedOf(x.law, x.on, x.want, x.ref) != x.eng) LawBad("engaged with the law driving %d, its state ON %d, the want on %d, the row %d: %d (%d wanted)", x.law, x.on, x.want, x.ref, FgCondEngagedOf(x.law, x.on, x.want, x.ref), x.eng);
        // (round 2, the reviewer's second: FGC_ONSLOW's band. Standing by under 38 it holds; in the wait for a cut it holds only under 35)
        { FgCondIn o = {}; o.presentOurs = 1; o.markerOk = 1; o.dxvk31 = -1; o.refreshNum = 120; o.refreshDen = 1; o.lockPm = 1015; o.swapEffect = 2; o.backBuffers = 2; o.windowed = 1; o.bbFp16 = 1; o.hdrOn = 1; o.vrr = 1; o.realFpsX10 = 610;
          struct { int pred, engaging; bool holds; } ob[] = { { 378, 0, true }, { 380, 0, false }, { 378, 1, false }, { 351, 1, false }, { 349, 1, true } };
          for (auto& x : ob) { o.predFpsX10 = x.pred; o.engaging = x.engaging; if (FgCondHolds(FGC_ONSLOW, o, FgCondDefaults()) != x.holds) LawBad("FGC_ONSLOW at a predicted %d, in the wait %d: %d (%d wanted)", x.pred, x.engaging, (int)FgCondHolds(FGC_ONSLOW, o, FgCondDefaults()), (int)x.holds); } }
        // (the card: idle room only on three readings of it running; one of the limit among the last three is the limit; held, kept)
        struct { int reading, held, want; } cs[] = { { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 0 },
                                                     { -1, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 } };
        FgDriveCardH ch = {};
        if (FgDriveCardTake(ch, 1, 1) != -1) LawBad("the card held before any reading: %d (-1 wanted: cannot tell)", FgDriveCardTake(ch, 1, 1));
        int ci = 0; for (auto& x : cs) { const int got = FgDriveCardTake(ch, x.reading, x.held); if (got != x.want) LawBad("the card's reading %d at step %d (held %d): took %d (%d wanted)", x.reading, ci, x.held, got, x.want); ci++; }
        // (c's margin: the leg's first stint, by itself at 61.1 on a card that is the limit, its windows 4 to 8 at 41.3, 42.2, 41.1, 38.9 and 40.5:
        // c 8,161 us, its margin the samples' spread, 670; FGC_ONSLOW's rate at 61.1 under 40 (39.7), at 65 over it (41.3); the mean's at 61.1 over 40)
        LawRig q; q.Init(dk);
        for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 611; w.card = 1; q.Step(w); }
        q.cam++; q.Poll(16);
        static const int ron[8] = { 418, 402, 410, 413, 422, 411, 389, 405 };
        for (int i = 0; i < 8; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = ron[i]; w.lawPerUs = 16920; w.floorUs = 16920; q.Step(w); }
        const int cm = q.m.cLimUs, ce = q.m.cLimErrUs;
        if (cm < 8100 || cm > 8220 || ce < 640 || ce > 700) LawBad("c of the leg's first stint: %d us, its margin %d us (8,161 and 670 wanted)", cm, ce);
        if (FgDrivePredX10(q.m, 611, 1) < 400 || FgDrivePredHiX10(q.m, 611, 1) >= 400 || FgDrivePredHiX10(q.m, 650, 1) < 400)
            LawBad("the rate with frame generation: at 61.1 on the mean %d, with the spread %d; at 65 with the spread %d (40 or more, under 40, 40 or more wanted)", FgDrivePredX10(q.m, 611, 1), FgDrivePredHiX10(q.m, 611, 1), FgDrivePredHiX10(q.m, 650, 1));
        { const int sp[] = { 7847, 7330, 7964, 9340 }; long long s = 0, s2 = 0; for (int v : sp) { s += v; s2 += (long long)v * v; }   // (the integer square root against the leg's 743)
          if (FgDriveSpreadUs(s, s2, 4) < 742 || FgDriveSpreadUs(s, s2, 4) > 744 || FgDriveSpreadUs(s, s2, 1) != 0 || FgDriveSpreadUs(8000, 64000000, 1) != 0) LawBad("the spread of the leg's four samples: %d (743 wanted; one sample, 0)", FgDriveSpreadUs(s, s2, 4)); }
        if (FgDriveCErrUs(q.m, 0) != ce || FgDrivePredHiX10(q.m, 611, 0) != FgDrivePredHiX10(q.m, 611, 1)) LawBad("a card with idle room and nothing measured with room: margin %d, rate %d (the limit's, %d and %d, wanted: review round 2)", FgDriveCErrUs(q.m, 0), FgDrivePredHiX10(q.m, 611, 0), ce, FgDrivePredHiX10(q.m, 611, 1));
        {   // (review round 3: BOUNDED: the limit's c stands for a card with idle room only while the idle run is short; past it, the cheap prior)
            LawRig y = q; const int lim = FgDrivePredHiX10(y.m, 611, 1); int at = -1;
            { FgDriveIn s = LawWin(FGC_SLOW, FGDB_SLOW); s.realX10 = 360; s.shownN = 120; s.shownGen = 120; y.Step(s); }   // (the stint stands down: REST)
            for (int i = 0; i < 40 && at < 0; i++) { FgDriveIn w = LawWin(FGC_ONSLOW); w.realX10 = 611; w.card = 0; y.Step(w); if (FgDrivePredHiX10(y.m, 611, 0) == 611) at = i + 1; }
            if (at != FG_DRIVE_IDLE_PRIOR_WIN) LawBad("a card with idle room behind a stint on the limit: the cheap prior came back after %d window(s) of idle room (%d wanted)", at, (int)FG_DRIVE_IDLE_PRIOR_WIN);
            FgDriveIn lw = LawWin(FGC_ONSLOW); lw.realX10 = 611; lw.card = 1; y.Step(lw);
            if (y.m.idleRun != 0 || FgDrivePredHiX10(y.m, 611, 0) != lim) LawBad("one reading of the limit behind the idle run: run %d, the rate with idle room %d (0 and the limit's %d wanted)", y.m.idleRun, FgDrivePredHiX10(y.m, 611, 0), lim);
        }
        { LawRig z; z.Init(dk); if (FgDrivePredHiX10(z.m, 611, 0) != 611 || FgDrivePredHiX10(z.m, 611, 1) != 611) LawBad("nothing measured: %d and %d (61.1 both wanted: the session's first engage is optimistic)", FgDrivePredHiX10(z.m, 611, 0), FgDrivePredHiX10(z.m, 611, 1)); }
        { FgDriveIn w = LawWin(FGC_SLOW, FGDB_SLOW); w.realX10 = 360; w.shownN = 120; w.shownGen = 120; q.Step(w); }   // (SLOW behind the cost windows: REST)
        // (round 9, L2, CHANGED BY NAME: the rest behind SLOW is one window, so the band read IN in it would end the rest by FGDRW_RESTED: the band
        // move is read in the rest's own window now, the first band read there, and it must keep c's margin as before)
        { FgDriveIn w = LawWin(FGC_FAST, FGDB_FAST); w.realX10 = 720; q.Step(w); }                                     // (the band moves: the game is elsewhere)
        if (q.m.cLimErrUs != ce || q.m.cLimUs != cm || q.m.state != FGDR_STANDBY || q.m.why != FGDRW_BAND) LawBad("a band move in REST: margin %d, c %d, %s, %s (%d, %d, STANDBY, FGDRW_BAND wanted: round 2, the margin lives as c does)", q.m.cLimErrUs, q.m.cLimUs, FgDriveStateName(q.m.state), FgDriveWhyName(q.m.why), ce, cm);
        { FgDriveIn w = LawWin(FGC_OK); w.cleared = 1; q.Step(w); }
        if (q.m.cLimErrUs != 0 || q.m.cLimUs != -1) LawBad("a Reset: margin %d, c %d (0 and none wanted)", q.m.cLimErrUs, q.m.cLimUs);
        // (the pacer's floor is never c: in the fixed mode the game by itself at 97.5, engaged at exactly 60.0 with no law period read: c 0 with
        // the floor of two refreshes in, 6,410 us without it, the morning's fixed120 fault)
        for (int withFloor = 1; withFloor >= 0; withFloor--)
        {
            LawRig f; f.Init(dk);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 975; w.card = 0; f.Step(w); }
            f.cam++; f.Poll(16);
            for (int i = 0; i < 8; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 600; w.lawPerUs = 0; w.floorUs = withFloor ? 16667 : 0; f.Step(w); }
            if (withFloor ? f.m.cRoomUs != 0 : (f.m.cRoomUs < 6300 || f.m.cRoomUs > 6500)) LawBad("the fixed mode's hold %s the floor: c %d us (%s wanted)", withFloor ? "with" : "without", f.m.cRoomUs, withFloor ? "0" : "about 6,410, the fault");
        }
        // (S4's lawon: the period in force a floor of the pacer's, 25.0 ms over the law's own 16.9, the real frame at it: an upper bound, filed only
        // where no window of the stint was a sample and marked so; one window of the card holding the game (25.0 ms against a period of 20 ms)
        // makes the stint's c a sample of its own)
        for (int mixed = 0; mixed < 2; mixed++)
        {
            LawRig b; b.Init(dk);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 611; w.card = 1; b.Step(w); }
            b.cam++; b.Poll(16);
            for (int i = 0; i < 8; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 400; w.lawPerUs = 16920; w.floorUs = 16920; w.perUs = (mixed && i == 4) ? 20000 : 25000; b.Step(w); }
            const int want = 25000 - 16366;
            if (mixed ? (b.m.cLimBound != 0 || b.m.cLimUs != want) : (b.m.cLimBound != 1 || b.m.cLimUs != want || b.m.cLimErrUs != 0))
                LawBad("%s: c %d us, a bound %d, margin %d (%d, %s wanted)", mixed ? "one window of the card holding among the pacer's floor" : "every window at a floor of the pacer's", b.m.cLimUs, b.m.cLimBound, b.m.cLimErrUs, want, mixed ? "a sample" : "a bound, margin 0");
            if (strcmp(FgDriveCWhy(1, 1, FgDriveCBound(b.m, 1)), mixed ? "measured where the card was the limit standing by" : "an upper bound: a floor of the pacer's held every window of its stint, the card the limit standing by"))
                LawBad("the reason said: %s", FgDriveCWhy(1, 1, FgDriveCBound(b.m, 1)));
            if (mixed) continue;
            // (2026-10-28, the core audit: A BOUND AGES WHILE NOTHING IS GENERATED, fg_cond.h, FgDriveBoundAge: engaged it stands; standing by,
            // dropped after FG_DRIVE_BOUND_WIN windows that read the game's own rate, the prediction back to the optimistic first engage's;
            // ONCE A LINEAGE: the bound the next stint files behind the drop is kept, whatever the windows standing by)
            LawRig g = b;
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 400; w.lawPerUs = 16920; w.floorUs = 16920; w.perUs = 25000; g.Step(w); }
            if (g.m.state != FGDR_ON || g.m.cLimBound != 1 || g.m.cLimUs != want) LawBad("a bound engaged three windows more: %s, the mark %d, c %d (ON, 1 and %d wanted: engaged nothing ages)", FgDriveStateName(g.m.state), g.m.cLimBound, g.m.cLimUs, want);
            { FgDriveIn s = LawWin(FGC_SLOW, FGDB_SLOW); s.realX10 = 400; s.shownN = 120; s.shownGen = 120; g.Step(s); }   // (the stint stands down)
            int at = -1;
            for (int i = 0; i < 30 && at < 0; i++) { FgDriveIn w = LawWin(FGC_ONSLOW); w.realX10 = 611; w.card = 1; g.Step(w); if (g.m.cLimUs < 0) at = i + 1; }
            if (at != FG_DRIVE_BOUND_WIN || g.m.cLimBound != -1 || g.m.cLimErrUs != 0 || FgDrivePredHiX10(g.m, 611, 1) != 611)
                LawBad("a bound standing by: dropped after %d window(s), the mark %d, margin %d, the rate with frame generation %d (%d, -1, 0 and 61.1 wanted)", at, g.m.cLimBound, g.m.cLimErrUs, FgDrivePredHiX10(g.m, 611, 1), (int)FG_DRIVE_BOUND_WIN);
            for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 611; w.card = 1; g.Step(w); }
            g.cam++; g.Poll(16);
            for (int i = 0; i < 8; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 400; w.lawPerUs = 16920; w.floorUs = 16920; w.perUs = 25000; g.Step(w); }
            { FgDriveIn s = LawWin(FGC_SLOW, FGDB_SLOW); s.realX10 = 400; s.shownN = 120; s.shownGen = 120; g.Step(s); }
            for (int i = 0; i < 30; i++) { FgDriveIn w = LawWin(FGC_ONSLOW); w.realX10 = 611; w.card = 1; g.Step(w); }
            if (g.m.engages != 2 || g.m.cLimBound != FG_DRIVE_BOUND_KEPT || g.m.cLimUs != want || !FgDriveCBound(g.m, 1))
                LawBad("the bound behind a drop: %d engage(s), the mark %d, c %d us, a bound %d (2, KEPT, %d and a bound wanted: once a lineage, no cycling)", g.m.engages, g.m.cLimBound, g.m.cLimUs, FgDriveCBound(g.m, 1), want);
            { FgDriveIn w = LawWin(FGC_OK); w.cleared = 1; g.Step(w); }
            if (g.m.cLimBound != 0 || g.m.cLimUs != -1) LawBad("a Reset behind a kept bound: the mark %d, c %d (0 and none wanted)", g.m.cLimBound, g.m.cLimUs);
        }
        // (a ramp: standing by its rate is not the game's own; ON it is not a sample and the cost windows start behind it)
        LawRig p; p.Init(dk);
        for (int i = 0; i < 3; i++) { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 600; w.card = 1; p.Step(w); }
        { FgDriveIn w = LawWin(FGC_OK); w.realX10 = 450; w.card = 1; w.ramp = 1; p.Step(w); }   // (ENGAGING: a ramp window is not the game's own)
        if (p.m.aloneX10 != 600) LawBad("a ramp window standing by: the game's own rate %d (600 wanted)", p.m.aloneX10);
        p.cam++; p.Poll(16);
        for (int i = 0; i < 3; i++) p.Step(LawOn(120, 120));
        for (int i = 0; i < 3; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 300; w.ramp = 1; p.Step(w); }   // (three ramp windows behind the warm-up)
        for (int i = 0; i < 5; i++) { FgDriveIn w = LawOn(120, 120); w.realX10 = 450; w.lawPerUs = 16920; w.floorUs = 16920; p.Step(w); }
        if (p.m.cLimUs != 5556) LawBad("c behind three ramp windows: %d us (5,556 wanted: the five windows behind the ramp alone)", p.m.cLimUs);
    }
    // Q. THE PLACE ON THE CARD
    {
        volatile long ring[128] = {}; int med = 0;
        for (long i = 0; i < 200; i++) ring[i & 127] = 300 + (i % 9) * 50;       // (the last 128 of 200 values 300 to 700 by 50)
        const int n = FgDrivePlaceMed<long>(ring, 200L, 0L, &med);
        const int n2 = FgDrivePlaceMed<long>(ring, 200L, 195L, &med);
        if (n != 128 || n2 != 5) LawBad("the place ring: %d and %d read (128 and 5 wanted)", n, n2);
        int m3 = 0; FgDrivePlaceMed<long>(ring, 200L, 197L, &m3);                // (197, 198, 199: 300 + (i % 9) * 50 = 700, 300, 350)
        if (m3 != 350) LawBad("the median of the last three places: %d (350 wanted)", m3);
    }
    return gLawBad;
}

int main(int argc, char** argv)
{
    const bool control = argc > 1 && !strcmp(argv[1], "control");
    FgCondK k = FgCondDefaults();
    const OracleK ok = { 48.0, 1.03, 67.0, 38.0, 3.0, 1, 3, true };
    FgPlanK pk = FgPlanDefaults();
    if (control) { k.maxRealFpsX10 = 700; pk.quantilePm = 500; }   // THE PLANTED FAULTS: the table's rate bound, the plan's quantile (the true fill's top, the fourth, went with the fill in round 8's deep strip)
    printf("test-fg-cond%s: the conditions table (%d rows) and the planned generation time, against their oracles\n", control ? " (THE CONTROL: one constant of each moved, the checks must fail)" : "", FGC_N - 1);

    // ---- the domains: each puts every row it feeds on both of its sides
    static const int dAsync[][2] = { { 0, 1 }, { 1, 0 }, { 1, 1 } };
    static const int dRefresh[][2] = { { 0, 0 }, { 60000, 1001 }, { 100, 1 }, { 120000, 1001 }, { 144, 1 }, { 240, 1 } };
    static const int dSwap[][2] = { { 1, 2 }, { 2, 1 }, { 1, 1 }, { 1, 3 }, { 3, 1 }, { 1, 4 }, { 0, 0 } };
    static const int dHdr[][3] = { { 1, 1, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 0, 0 } };   // (fp16, hdr on, the back buffer multisampled: 2026-10-20)
    static const int dVrr[] = { 1, 0, -1 };
    static const int dFf[][2] = { { 0, 0 }, { 58, 0 }, { 61, 0 }, { 100, 0 }, { 0, 1 } };      // (cap, Custom -N)
    static const int dReal[][2] = { { 0, 0 }, { 300, 0 }, { 380, 0 }, { 420, 0 }, { 590, 0 }, { 680, 0 }, { 710, 0 }, { 1000, 0 }, { 590, 710 }, { 420, 1000 } };   // (phase 5: the measured rate and R_free; the floor 40 in, 37 out)
    static const int dDxvk[] = { 1, 0, -1 };
    static const int dNv[][2] = { { 0, 0 }, { 116, 0 }, { 0, 350 } };          // (phase 5: NVIDIA's cap, 116 under 2 C x 1.03 at 120 Hz and up, over it at 100 Hz; and a predicted R_on of 35)
    long long combos = 0, answered[FGC_N] = {}, lines = 0;
    FgCondIn firstOf[FGC_N]; bool haveFirst[FGC_N] = {};
    char b[512];
    FgCondIn in = {};
    in.lockPm = 1015;
#define EACH(var, n) for (int var = 0; var < (int)(n); var++)
    EACH(iDead, 2) EACH(iLost, 2) EACH(iPres, 2) EACH(iMark, 2) EACH(iAgi, 2) EACH(iDx, 3) EACH(iAs, 3) EACH(iRef, 6) EACH(iSw, 7) EACH(iWin, 2)
    EACH(iHdr, 5) EACH(iVrr, 3) EACH(iFf, 5) EACH(iCap, 2) EACH(iMenu, 2) EACH(iReal, 10) EACH(iEng, 2) EACH(iNv, 3)
    {
        in.nvCap = dNv[iNv][0]; in.predFpsX10 = dNv[iNv][1];
        in.machineDead = iDead; in.restoreLost = iLost * 2; in.presentOurs = 1 - iPres; in.markerOk = 1 - iMark; in.agility = iAgi; in.dxvk31 = dDxvk[iDx];
        in.asyncNamed = dAsync[iAs][0]; in.asyncHome = dAsync[iAs][1];
        in.refreshNum = dRefresh[iRef][0]; in.refreshDen = dRefresh[iRef][1];
        in.swapEffect = dSwap[iSw][0]; in.backBuffers = dSwap[iSw][1]; in.windowed = 1 - iWin;
        in.bbFp16 = dHdr[iHdr][0]; in.hdrOn = dHdr[iHdr][1]; in.sdrMsaa = dHdr[iHdr][2]; in.vrr = dVrr[iVrr];
        in.ffCap = dFf[iFf][0]; in.ffRel = dFf[iFf][1]; in.dxvkCap = iCap * 60; in.menu = iMenu;
        in.realFpsX10 = dReal[iReal][0]; in.freeFpsX10 = dReal[iReal][1]; in.engaged = iEng; in.lift = 0;
        in.engaging = (!iEng && iMenu) ? 1 : 0;   // (S2 round 2: the wait for a cut, with the menu's index: no loop of its own)
        combos++;
        const unsigned mask = FgCondMask(in, k);
        const int got = FgCondFirst(in, k), want = OracleFirst(in, ok);
        if (got != want) Bad("the first row", in, got, want);
        unsigned omask = 0;
        for (int c = 1; c < FGC_N; c++) if (OracleRow(c, in, ok)) omask |= 1u << c;
        if (mask != omask) Bad("the mask", in, (int)mask, (int)omask);
        if (got != FGC_OK && FgCondClassOf(got) != kOracleClass[got]) Bad("the class", in, FgCondClassOf(got), kOracleClass[got]);
        answered[got]++;
        if (!haveFirst[got]) { haveFirst[got] = true; firstOf[got] = in; }
        if (got != FGC_OK && (combos & 4095) == 0) { lines++; LineOk(got, in, k, b, sizeof(b)); }
    }
    printf("  %lld combinations: the table's first row, its mask and its class against the oracle's: %lld mismatch(es)\n", combos, gBad);
    // ---- THE FIXED-REFRESH MODE (phase 5, F1): the key, vsync, the classifier's memory and NVIDIA's answer over the refreshes, the rates, the
    // limiters and the cap, on a session that refuses nothing else; against OracleRowF
    {
        static const int fRefresh[][2] = { { 120, 1 }, { 120000, 1001 }, { 60, 1 }, { 144, 1 } };
        static const int fReal[][3] = { { 0, 0, 0 }, { 300, 0, 0 }, { 590, 0, 0 }, { 610, 0, 0 }, { 640, 0, 350 }, { 1000, 0, 0 }, { 1180, 0, 0 }, { 600, 1200, 0 } };   // (the rate, R_free, a predicted R_on)
        static const int fFf[] = { 0, 58, 100 };
        const long long bad0 = gBad, combos0 = combos;
        FgCondIn f = {};
        f.presentOurs = 1; f.markerOk = 1; f.dxvk31 = -1; f.lockPm = 1015; f.swapEffect = 1; f.backBuffers = 2; f.windowed = 1; f.bbFp16 = 1; f.hdrOn = 1;
        EACH(iKey, 3) EACH(iVs, 2) EACH(iQ, 3) EACH(iVrr, 3) EACH(iRef, 4) EACH(iReal, 8) EACH(iEng, 2) EACH(iNv, 2) EACH(iFf, 3) EACH(iOver, 2) EACH(iEntry, 2)
        {
            f.fixOver = iOver; f.fixLUs = iOver ? 17200 : 16500; f.fixEntryX10 = iEntry ? 655 : 0;   // (round 2: the overrun, and the way back in at 65.5)
            f.fixedKey = iKey; f.vsync = iVs; f.flipQuant = iQ == 2 ? -1 : iQ; f.vrr = dVrr[iVrr];   // (S2: -1, the memory read FREE)
            f.refreshNum = fRefresh[iRef][0]; f.refreshDen = fRefresh[iRef][1];
            f.realFpsX10 = fReal[iReal][0]; f.freeFpsX10 = fReal[iReal][1]; f.predFpsX10 = fReal[iReal][2]; f.engaged = iEng; f.engaging = (!iEng && iOver) ? 1 : 0;
            f.nvCap = iNv ? 116 : 0; f.ffCap = fFf[iFf]; f.lift = 0;
            combos++;
            const unsigned mask = FgCondMask(f, k);
            const int got = FgCondFirst(f, k), want = OracleFirstF(f, ok);
            if (got != want) Bad("the first row (the fixed mode's facts)", f, got, want);
            unsigned omask = 0;
            for (int c = 1; c < FGC_N; c++) if (OracleRowF(c, f, ok)) omask |= 1u << c;
            if (mask != omask) Bad("the mask (the fixed mode's facts)", f, (int)mask, (int)omask);
            if (got != FGC_OK && FgCondClassOf(got) != kOracleClass[got]) Bad("the class", f, FgCondClassOf(got), kOracleClass[got]);
            answered[got]++;
            if (!haveFirst[got]) { haveFirst[got] = true; firstOf[got] = f; }
            if (got != FGC_OK) { lines++; LineOk(got, f, k, b, sizeof(b)); }
        }
        printf("  %lld combinations of the fixed-refresh mode's facts (fg_fixed, vsync, the classifier's memory, NVIDIA's answer): %lld mismatch(es)\n", combos - combos0, gBad - bad0);
    }
    int never = 0;
    for (int c = 0; c < FGC_N; c++) if (!answered[c]) { never++; printf("  ROW %d (%s) WAS NEVER THE ANSWER: the domains do not reach it\n", c, FgCondName(c)); }
    printf("  every row was the answer for some combination, and \"no row\" for %lld: %s\n", answered[FGC_OK], never ? "NO" : "yes");
    // ---- every line, once, with the facts of the first combination its row answered
    printf("  the lines, one a row (the class in brackets: S the session, R until a build or a Reset, B standby; * also in the warnings file):\n");
    for (int c = 1; c < FGC_N; c++)
    {
        if (!haveFirst[c]) continue;
        lines++;
        const bool lok = LineOk(c, firstOf[c], k, b, sizeof(b));
        const int cl = FgCondClassOf(c);
        printf("    %2d [%c%c] %s%s\n", c, cl == FGCC_SESSION ? 'S' : cl == FGCC_RESET ? 'R' : 'B', FgCondWarns(c) ? '*' : ' ', b, lok ? "" : "   <-- BREAKS ITS RULES");
    }
    printf("  %lld line(s) rendered and held to their rules: %d broke them\n", lines, gLinesBad);
    // ---- the lifts, over a session in which EVERY row holds
    long long liftBad = 0;
    {
        FgCondIn all = {};
        all.machineDead = 1; all.restoreLost = 1; all.presentOurs = 0; all.markerOk = 0; all.agility = 1; all.dxvk31 = 0; all.asyncNamed = 1; all.asyncHome = 1;
        all.lockPm = 1015; all.swapEffect = 3; all.backBuffers = 1; all.windowed = 0; all.vrr = 0; all.dxvkCap = 60; all.menu = 1;
        // (the rows that need a refresh, and the two pairs that exclude each other, are asked in sessions of their own below)
        FgCondIn s2 = all; s2.refreshNum = 60; s2.refreshDen = 1; s2.ffCap = 20; s2.realFpsX10 = 200; s2.bbFp16 = 1; s2.vrr = -1; s2.nvCap = 30; s2.predFpsX10 = 200;
        const FgCondIn* ss[2] = { &all, &s2 };
        for (int si = 0; si < 2; si++)
            for (int c = 1; c < FGC_N; c++)
            {
                FgCondIn x = *ss[si];
                if (!OracleRow(c, x, ok)) continue;
                x.lift = 1u << c;
                const bool inMask = (FgCondMask(x, k) >> c) & 1u;
                if (OracleLiftable(c) ? inMask : !inMask) { liftBad++; printf("  THE LIFT of row %d (%s) is wrong: in the mask %d, liftable %d\n", c, FgCondName(c), inMask ? 1 : 0, OracleLiftable(c) ? 1 : 0); }
                x.lift = 0xFFFFFFFFu;
                const int first = FgCondFirst(x, k);
                if (first != FGC_OK && OracleLiftable(first)) { liftBad++; printf("  WITH EVERY ROW LIFTED the answer is row %d (%s), which a key lifts\n", first, FgCondName(first)); }
                if (first != OracleFirst(x, ok)) { liftBad++; printf("  WITH EVERY ROW LIFTED the answer is row %d and the oracle's %d\n", first, OracleFirst(x, ok)); }
            }
    }
    printf("  the lifts: a lifted row is never in the mask, the five that cannot be lifted still are: %lld fault(s)\n", liftBad);

    // ---- the plan
    long long planBad = 0;
    {
        enum { BINS = 64, BIN = 250 };
        LONG h[BINS];
        auto plan = [&](const char* what, LONG prev, LONG want)
        {
            const LONG got = FgPlanFromHist(h, BINS, BIN, pk, prev);
            printf("    %-66s planned %ld us (wanted %ld)%s\n", what, got, want, got == want ? "" : "   <-- NOT IT");
            if (got != want) planBad++;
        };
        printf("  the plan (the quantile %ld per mille, its upper edge, bins of %d us):\n", pk.quantilePm, (int)BIN);
        memset(h, 0, sizeof(h)); h[6] = 100;                              // every K in 1500 to 1750 us
        plan("a steady K, every pair in the bin 1500 to 1750 us:", 2000, 1750);
        memset(h, 0, sizeof(h)); h[6] = 99; h[40] = 1;                    // one spike at 10 ms in a hundred
        plan("the same with one spike of 10 ms in a hundred:", 2000, 1750);
        memset(h, 0, sizeof(h)); h[6] = 90; h[9] = 10;                    // a tail: ten in a hundred at 2250 to 2500
        plan("a tail, ten in a hundred in the bin 2250 to 2500 us:", 2000, 2500);
        memset(h, 0, sizeof(h)); h[4] = 50; h[12] = 50;                   // a step: half at 1000, half at 3000
        plan("a step, half the pairs at 1000 us and half at 3000 us:", 2000, 3250);
        memset(h, 0, sizeof(h)); h[6] = 10;                               // too few
        plan("ten pairs only (fewer than it needs): the plan it had, unchanged:", 2250, 2250);
        memset(h, 0, sizeof(h));
        plan("no pair at all and no plan yet: the start value:", 0, 2000);
        memset(h, 0, sizeof(h)); h[63] = 100;                             // the last bin: at or over 15.75 ms
        plan("every K in the last bin (at or over 15750 us): the upper bound:", 2000, 8000);
        memset(h, 0, sizeof(h)); h[0] = 100;
        plan("every K under 250 us: the upper edge of the first bin:", 2000, 250);
    }
    printf("  the plan: %lld fault(s)\n", planBad);

    // ---- THE LAW (phase 5, P1)
    FgDriveK dk = FgDriveDefaults();
    if (control) { dk.dwellWin = 2; dk.backoffMaxWin = 600; }   // THE PLANTED FAULTS OF THE LAW: a dwell of 2 and a back-off capped at 600
    printf("  the law (fg_cond.h, FgDrive*): the states, the dwell, the wait for a cut, the rest and the back-off, the health exit, who drives, the floor's key, the reader:\n");
    const long long lawBad = LawBench(dk);
    {   // WHAT THE PER-FRAME POLL COSTS (told, not judged): the waiter's thread asks FgDriveCut every frame, nearly always in a state that
        // returns at its first test
        FgDriveMem m; FgDriveInit(m, dk); volatile int sink = 0; const int N = 50000000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < N; i++) sink += FgDriveCut(m, dk, (unsigned)i, i & 1, 0);
        const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / N;
        printf("    the cut poll outside the wait: %.2f ns a call over %d calls (the waiter's thread, once a frame; nothing on the render thread)\n", ns, N);
    }
    {   // (round 9, L1, told, not judged) WHAT THE SCREEN'S RATE COSTS: FgCondFixedMeanUs over the waiter's 120 intervals, once a window (about
        // 2 s) on the waiter's thread, FgpArmWindowStep's; nothing a frame and nothing on the render thread
        float w[120]; for (int i = 0; i < 120; i++) w[i] = 16.0f + (float)(i % 7) * 0.5f + (i == 60 ? 200.0f : 0.0f);
        volatile int sink = 0; const int N = 2000000;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < N; i++) { w[i % 120] += 0.0f; sink += FgCondFixedMeanUs(w, 120, 60000); }
        const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / N;
        printf("    the screen's rate (FgCondFixedMeanUs, 120 intervals): %.0f ns a window over %d windows (the waiter's thread, once a window)\n", ns, N);
    }
    printf("  the law: %lld fault(s)\n", lawBad);

    const long long faults = gBad + never + gLinesBad + liftBad + planBad + lawBad;
    if (control)
    {
        // THE CONTROL PASSES ONLY IF EVERY PLANTED FAULT WAS SEEN: the table's (mismatches against the oracle), the plan's and the law's
        const bool seen = gBad > 0 && planBad > 0 && lawBad > 0;
        printf("THE CONTROL: the table with its rate bound moved counted %lld mismatch(es), the plan with its quantile moved %lld fault(s), the law with its dwell and its cap moved %lld fault(s): %s\n",
               gBad, planBad, lawBad, seen ? "all three were caught, as they must be" : "A PLANTED FAULT WAS NOT CAUGHT");
        printf("%s\n", seen ? "FAILED (as a control must)" : "PASS (WHICH A CONTROL MUST NOT)");
        return seen ? 1 : 0;
    }
    printf("%s\n", faults ? "FAILED" : "PASS");
    return faults ? 1 : 0;
}
