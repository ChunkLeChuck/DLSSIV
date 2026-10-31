// THE SHIPPED BUILD'S WIRING (round 6, W, 2026-10-24; source/fg/fg_ship.h): cases SH1 to SH4. Included by dt_fg_test.cpp after
// dt_fg_fixed.h (its FxSession and FxCondLine).
//
// The conditions and the law are fg_cond.h's, run over the inputs a build that ships hands them: fg_ship.h's seed (the fixed mode's
// key; the fill's readings went in round 8's deep strip) and its keys (fg_min_fps read in every build
// that arms). Each case runs the shipped wiring and THE BASE'S (df0646fd: fg_fixed 0 by default, fg_min_fps read by the armed
// development build alone) or the armed development build's with the fill, its control, so a case cannot pass on inputs that make
// no difference.
#include "../../source/fg/fg_ship.h"

// ONE SESSION OF WINDOWS through the conditions and the law, as fg_arm_life.h's window step runs them (FgpCondStep, then
// FgpLawWindowStep; the wait for a cut by a cut every window). game: the game's own rate in tenths. Engaged, the real rate is the
// pacer's (the fixed mode's half refresh, or C on a variable display where the game is faster, else the game's own) and R_free the
// game's. Returns the windows engaged; *row: the row that stood at the last window.
struct ShWarn { int kind, at, onAt; };   // the fixed mode's warnings line (fg_ship.h, FgShipFixWarn): the first said, its window, the first window ON
static int ShLawRun(FgCondIn in, const FgCondK& k, int game, int windows, int* row, ShWarn* wn = nullptr)
{
    if (wn) { wn->kind = 0; wn->at = -1; wn->onAt = -1; }
    FgDriveK dk = FgDriveDefaults(); FgDriveMem mem; FgDriveInit(mem, dk);
    int on = 0, c = FGC_OK;
    for (int w = 0; w < windows; w++)
    {
        const unsigned now = 1000u + 1000u * (unsigned)w;
        in.engaged = FgDriveWantsOn(mem) ? 1 : 0;
        in.engaging = mem.state == FGDR_ENGAGING ? 1 : 0;
        if (in.engaged)
        {
            const int cap = FgCondFixedMode(in) ? FgCondHalfX10(in) : FgCondCFpsX10(in);
            in.realFpsX10 = game < cap ? game : cap; in.freeFpsX10 = game; in.predFpsX10 = 0;
        }
        else { in.realFpsX10 = game; in.freeFpsX10 = 0; in.predFpsX10 = game; }
        c = FgCondFirst(in, k);
        FgDriveIn di = {};
        di.row = c; di.band = FgDriveBandOf(in, k); di.cpuReady = 1; di.realX10 = in.realFpsX10; di.card = 0;
        if (in.engaged) { di.shownGen = 100; di.shownN = 100; di.floorUs = FgCondFloorUs(in); di.perUs = di.floorUs; di.lawPerUs = di.floorUs; }
        (void)FgDriveStep(mem, di, dk, now, w, 0);
        if (mem.state == FGDR_ENGAGING) (void)FgDriveCut(mem, dk, now + 1u, w + 1, 0);
        if (FgDriveWantsOn(mem)) on++;
        if (wn)
        {   // (as fg_arm_life.h's law step asks it: behind the law's step, the row that stood and the law's state)
            if (FgDriveWantsOn(mem) && wn->onAt < 0) wn->onAt = w;
            const int kind = FgShipFixWarn(in, k, c, FgDriveWantsOn(mem) ? 1 : 0);
            if (kind && !wn->kind) { wn->kind = kind; wn->at = w; }
        }
    }
    if (row) *row = c;
    return on;
}
// A RELEASE'S SESSION: FxSession's display, then fg_ship.h's seed with the release's keys
static FgCondIn ShSession(int vrr, int hz, int den, const FgShipKeys& f)
{
    FgCondIn in = FxSession(vrr, 0, 1, 0, hz); in.refreshDen = den;
    FgShipSeed(in, f);
    return in;
}

// ---------------------------------------------------------------- SH1, the release stands by on VRR above half the refresh
// THE DESIGN: on a variable display the game above half the refresh runs native (VRR paces it); only below half the doubling. A build that
// ships has no fill, so FGC_FAST (the top, 67 at 120 Hz scaled with C) stands it by: VRR 120 Hz at 75, 90 and 110 by itself, VRR 144 at 85
// and 120; and a game under half runs the doubling: 120 Hz at 50, 144 at 60. The law over 40 windows: never engaged at the first five,
// engaged at the last two. CONTROL (since round 8's deep strip, which took the fill and its control out): the table's top raised over these
// games, and the same games above half no longer stand by, so the case can see the top that stands them by.
static void CaseSH1()
{
    const char* const name = "SH1 the shipped build stands by on VRR above half the refresh (no fill in it) and doubles below half (2026-10-24, round 6, W)";
    CaseBegin(name);
    struct Row { const char* what; int hz, game, wantOn; };
    static const Row rows[] = { { "VRR 120 at 75", 120, 750, 0 }, { "VRR 120 at 90", 120, 900, 0 }, { "VRR 120 at 110", 120, 1100, 0 },
                                { "VRR 144 at 85", 144, 850, 0 }, { "VRR 144 at 120", 144, 1200, 0 },
                                { "VRR 120 at 50", 120, 500, 1 }, { "VRR 144 at 60", 144, 600, 1 } };
    const FgShipKeys rel = {};
    FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    int cOn = 0;
    for (const Row& r : rows)
    {
        int row = 0; const int on = ShLawRun(ShSession(1, r.hz, 1, rel), k, r.game, 40, &row);
        printf("  %-16s engaged %2d of 40 window(s), the row at the end %s\n", r.what, on, FgCondEnumName(row));
        if (!r.wantOn && (on || row != FGC_FAST)) Err("SH1: %s: engaged %d window(s), the row %s (never, and FGC_FAST, wanted: native, VRR paces it)", r.what, on, FgCondEnumName(row));
        if (r.wantOn && on < 20) Err("SH1: %s: engaged %d of 40 window(s) (20 or more wanted: the doubling below half)", r.what, on);
        if (!r.wantOn)
        {   // (THE CONTROL: the table's top raised over the game)
            FgCondIn dev = ShSession(1, r.hz, 1, rel); dev.realFpsX10 = r.game;
            FgCondK kd = k; kd.maxRealFpsX10 = 2400;
            if (FgCondFirst(dev, kd) == FGC_OK) cOn++;
        }
    }
    if (cOn < 4) Err("SH1 control: with the top raised %d of the 5 games above half read no row (4 or more wanted: the check cannot see the top)", cOn);
    else Ctl("with the top raised %d of the 5 games above half read no row: FGC_FAST, the top, is what stands them by", cOn);
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH2, the fixed mode on by default on a fixed screen
// THE OWNER'S FIXED-SCREEN RULE: a fixed screen with vsync runs the fixed mode (held to half the refresh, doubled) BY DEFAULT, with no key in
// the file. A release's keys (none): a fixed 120 Hz screen at 80 by itself engages the mode (FgCondFixedMode, FGC_OK, the law ON); a fixed
// 60 Hz at 45 and a 59.94 Hz at 45 too (30 real shown at 60); fixed 120 with vsync off stands by for FGC_VSYNCOFF, not FGC_FIXED. The armed
// development build's fg_fixed=0 still refuses (FGC_FIXED, the row of before). CONTROL: THE BASE'S DEFAULT (fg_fixed 0 where the file is
// silent): a RESET row at all three (FGC_FIXED at 120 Hz; FGC_PANEL, met first, at 60 and 59.94).
static void CaseSH2()
{
    const char* const name = "SH2 the fixed mode is on by default on a fixed screen: 120 at 80, 60 and 59.94 at 45 engage with no key in the file (2026-10-24, round 6, W)";
    CaseBegin(name);
    struct Row { const char* what; int num, den, game; };
    static const Row rows[] = { { "fixed 120 at 80", 120, 1, 800 }, { "fixed 60 at 45", 60, 1, 450 }, { "fixed 59.94 at 45", 60000, 1001, 450 } };
    const FgShipKeys rel = {};
    FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    int base = 0;
    for (const Row& r : rows)
    {
        const FgCondIn in = ShSession(0, r.num, r.den, rel);
        int row = 0; const int on = ShLawRun(in, k, r.game, 40, &row);
        printf("  %-18s fixedKey %d, the mode %s, engaged %2d of 40 window(s), the row at the end %s\n", r.what, in.fixedKey, FgCondFixedMode(in) ? "on" : "off", on, FgCondEnumName(row));
        if (!FgCondFixedMode(in) || on < 20 || row != FGC_OK) Err("SH2: %s: the mode %s, engaged %d of 40, the row %s (on, 20 or more, FGC_OK wanted)", r.what, FgCondFixedMode(in) ? "on" : "off", on, FgCondEnumName(row));
        {   // (THE CONTROL: the base's default)
            FgCondIn b = in; b.fixedKey = 0; b.realFpsX10 = r.game; const int cb = FgCondFirst(b, k); if (cb == FGC_FIXED || cb == FGC_PANEL) base++;   // (a 60 Hz screen meets PANEL first)
        }
    }
    {
        FgCondIn off = ShSession(0, 120, 1, rel); off.vsync = 0; off.realFpsX10 = 800;
        const int c = FgCondFirst(off, k);
        if (c != FGC_VSYNCOFF) Err("SH2: fixed 120 with vsync off read %s (FGC_VSYNCOFF wanted: the mode's own word to turn vsync on)", FgCondEnumName(c));
        FgCondIn d = ShSession(0, 120, 1, rel); d.fixedKey = 0; d.realFpsX10 = 800;
        if (FgCondFirst(d, k) != FGC_FIXED) Err("SH2: the mode's key at 0 read %s (FGC_FIXED wanted: the conditions still refuse a fixed screen without the mode)", FgCondEnumName(FgCondFirst(d, k)));
    }
    if (base != 3) Err("SH2 control: the base's default (fg_fixed 0) refused %d of the 3 screens by FGC_FIXED or FGC_PANEL (3 wanted: the check cannot see the default)", base);
    else Ctl("the base's default, fg_fixed 0: a RESET row at all three screens (FGC_FIXED at 120 Hz, FGC_PANEL at 60 and 59.94)");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH3, the floors by display, and fg_min_fps in the shipped build
// THE OWNER'S FLOORS: 30 real on a 60 Hz screen (59.94 read as 60), 40 elsewhere; fg_min_fps, his key, ships and overrides. A release's
// keys: a VRR 144 Hz game at 38 stands by (SLOW or ONSLOW: 38 real under 40), at 45 engages; WITH fg_min_fps=30 IN THE FILE the game at 35
// engages; a fixed 60 at 45 engages (30 real, the 60 Hz floor), and with fg_min_fps=40 it stands by (the half refresh under the key's 40,
// FGC_SLOW). CONTROL: THE BASE'S RELEASE, which did not read fg_min_fps: the VRR 144 game at 35 with fg_min_fps=30 in the file stands by.
static void CaseSH3()
{
    const char* const name = "SH3 the floors by display (30 real on 60 Hz, 38 elsewhere) and fg_min_fps read by the shipped build (2026-10-24, round 6, W; 38 since 2026-10-27)";
    CaseBegin(name);
    struct Row { const char* what; int vrr, num, den, game, minFps, wantOn; };
    static const Row rows[] = { { "VRR 144 at 37", 1, 144, 1, 370, 0, 0 }, { "VRR 144 at 39", 1, 144, 1, 390, 0, 1 }, { "VRR 144 at 45", 1, 144, 1, 450, 0, 1 },
                                { "VRR 144 at 35, fg_min_fps=30", 1, 144, 1, 350, 30, 1 },
                                { "fixed 60 at 45", 0, 60, 1, 450, 0, 1 }, { "fixed 59.94 at 45", 0, 60000, 1001, 450, 0, 1 },
                                { "fixed 60 at 45, fg_min_fps=40", 0, 60, 1, 450, 40, 0 } };
    int ctlOn = -1;
    for (const Row& r : rows)
    {
        FgShipKeys rel = {}; rel.minFps = r.minFps;
        FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
        const FgCondIn in = ShSession(r.vrr, r.num, r.den, rel);
        int row = 0; const int on = ShLawRun(in, k, r.game, 40, &row);
        printf("  %-30s the floor %d.%d, engaged %2d of 40 window(s), the row at the end %s\n", r.what, FgCondFloorX10(in, k) / 10, FgCondFloorX10(in, k) % 10, on, FgCondEnumName(row));
        if (r.wantOn && on < 20) Err("SH3: %s: engaged %d of 40 window(s) (20 or more wanted)", r.what, on);
        if (!r.wantOn && on) Err("SH3: %s: engaged %d window(s), the row %s (never wanted: under the floor)", r.what, on, FgCondEnumName(row));
        if (r.minFps == 30)
        {   // (THE CONTROL: the base's release, fg_min_fps not read)
            FgCondK kb = FgCondDefaults(); ctlOn = ShLawRun(in, kb, r.game, 40, nullptr);
        }
    }
    if (ctlOn != 0) Err("SH3 control: the base's release (fg_min_fps not read) engaged the game at 35 for %d window(s) (never wanted: the check cannot see the key)", ctlOn);
    else Ctl("the base's release, fg_min_fps not read: the VRR 144 game at 35 stands by");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH4 (BY NAME), the VRR panels of 59 Hz and up in a build without the fill
// THE DESIGN: VRR panels of 59 Hz and up. FGC_PANEL's waiver (fg_cond.h, F's) once read whether the fill could exist, and a build that
// shipped had none: a VRR 60, 75 and 90 Hz panel there read FGC_PANEL, a RESET row, and never asked for a set. The waiver must not depend on
// the fill (F's hunk, round 6; the fill itself went in round 8's deep strip). Release inputs, at the first build (the law not started): VRR 59.94, 60, 75 and 90 Hz read no SESSION or
// RESET row; a 50 Hz panel still reads FGC_PANEL. BY NAME until F's hunk is merged (it fails on this tree by design: the fail-first).
// CONTROL: the 50 Hz panel, in the same rows: FGC_PANEL still stands under 59 Hz, so the reader sees the row where it must.
static void CaseSH4()
{
    const char* const name = "SH4 VRR panels of 59 Hz and up ask for their set in a build without the fill (FGC_PANEL's waiver not on the fill; BY NAME until F's hunk) (2026-10-24, round 6, W)";
    CaseBegin(name);
    struct Row { const char* what; int num, den, game, wantPanel; };
    static const Row rows[] = { { "VRR 59.94 at 28", 60000, 1001, 280, 0 }, { "VRR 60 at 31", 60, 1, 310, 0 }, { "VRR 75 at 36", 75, 1, 360, 0 },
                                { "VRR 90 at 44", 90, 1, 440, 0 }, { "VRR 50 at 24", 50, 1, 240, 1 } };
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    int ctl = 0;
    for (const Row& r : rows)
    {
        FgCondIn in = ShSession(1, r.num, r.den, rel); in.realFpsX10 = r.game;
        const int c = FgCondFirst(in, k), cls = c ? FgCondClassOf(c) : -1;
        printf("  %-18s -> %-10s %s\n", r.what, FgCondEnumName(c), c ? FxCondLine(c, in, k).c_str() : "");
        if (!r.wantPanel && c && cls != FGCC_STANDBY) Err("SH4: %s: %s, a %s row (none, or a STANDBY row, wanted: the set is asked)", r.what, FgCondEnumName(c), cls == FGCC_SESSION ? "SESSION" : "RESET");
        if (r.wantPanel && c == FGC_PANEL) ctl++;
    }
    if (ctl != 1) Err("SH4 control: the 50 Hz panel did not read FGC_PANEL (it must: under 59 Hz; the check cannot see the row)");
    else Ctl("the 50 Hz panel reads FGC_PANEL: the row stands under 59 Hz");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH5, the fixed mode's warnings line only where frame generation holds the game
// (round 6, the reviewer's finding: the line was said when the mode's word turned on, before and whatever the table answered.) The line "the
// game is held to half the refresh" only at the mode's FIRST ENGAGE, never before it: fixed 120 at 80. WHERE THE FLOOR REFUSES THE SCREEN (its
// half refresh under the floor) the floor's own line instead: fixed 75 at 50 (37.5 under 40), fixed 60 at 45 with fg_min_fps=40. NEITHER
// where frame generation never runs for another reason: a game near the refresh (native, FGC_FAST), exclusive fullscreen (its own line), a
// game under half the refresh at 120 Hz (a slow stretch, FGC_SLOW, which comes and goes). CONTROL: THE RULE OF BEFORE (said where the mode's
// word is on): it says "held to half the refresh" at the four screens where frame generation never runs.
static void CaseSH5()
{
    const char* const name = "SH5 the fixed mode's warnings line is said at its first engage, the floor's line where the floor refuses the screen, neither where frame generation never runs (2026-10-24, round 6, W)";
    CaseBegin(name);
    struct Row { const char* what; int num, game, minFps, windowed, want; };
    static const Row rows[] = { { "fixed 120 at 80", 120, 800, 0, 1, 1 }, { "fixed 75 at 50", 75, 500, 0, 1, 2 }, { "fixed 60 at 45, fg_min_fps=40", 60, 450, 40, 1, 2 },
                                { "fixed 120 at 118 (native)", 120, 1180, 0, 1, 0 }, { "fixed 120 at 80, exclusive", 120, 800, 0, 0, 0 },
                                { "fixed 120 at 50 (under half)", 120, 500, 0, 1, 0 } };
    int before = 0;
    for (const Row& r : rows)
    {
        FgShipKeys rel = {}; rel.minFps = r.minFps;
        FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
        FgCondIn in = ShSession(0, r.num, 1, rel); in.windowed = r.windowed;
        int row = 0; ShWarn wn; const int on = ShLawRun(in, k, r.game, 40, &row, &wn);
        printf("  %-30s engaged %2d of 40, the row %-14s the line %d at window %d (the first engage at %d)\n", r.what, on, FgCondEnumName(row), wn.kind, wn.at, wn.onAt);
        if (wn.kind != r.want) Err("SH5: %s: line %d (%d wanted: 1 held to half, 2 the floor's, 0 none)", r.what, wn.kind, r.want);
        if (wn.kind == 1 && (wn.onAt < 0 || wn.at < wn.onAt)) Err("SH5: %s: the line at window %d, the first engage at %d (never before it wanted)", r.what, wn.at, wn.onAt);
        if (r.want != 1 && FgCondFixedMode(in)) before++;   // (the rule of before: said where the mode's word is on)
    }
    if (before < 4) Err("SH5 control: the rule of before says the line at %d of the 5 screens where nothing holds the game (4 or more wanted: the check cannot see it)", before);
    else Ctl("the rule of before (said where the mode's word is on) says \"held to half the refresh\" at screens where nothing holds the game");
    CaseEnd(name);
}

// ================================================================================================ ROUND 7, W: THE SCREEN FACTS (SH6 to SH8)
// THE READ AS THE PRODUCT MAKES IT (fg_arm_life.h, FgpVrrRead and then FgpFixQuant, once a window inside FgpCondRead): the display's key, the
// panel's evidence taken where the pacer did not hold the game, the answer; then the fixed mode's own latch (fg_cond.h, F's) with NVIDIA's "yes"
// as the formed answer's. base 1: THE BASE'S WIRING (c76d58c1: FgCondIn::vrr is NVIDIA's answer alone; the latch's yes is NVIDIA's).
struct ShVrrRead { FgShipVrr v; FgFixLatch L; int info0, info1, mem, since; };
// THE CLASSIFIER'S DISPLAY MEMORY AS LatFlipStep KEEPS IT (two windows that agree; a contradiction empties it; a new key empties it): v the
// window's verdict, 0 none (UNCERTAIN, LIMITED, a pace of the mod's own that reads nothing), 1 FREE, 2 QUANTISED
static void ShMemFeed(ShVrrRead& r, int v, int win, bool keyChanged)
{
    if (keyChanged) { r.info0 = r.info1 = 0; if (r.mem) { r.mem = 0; r.since = win; } }
    if (v != 1 && v != 2) return;
    r.info0 = r.info1; r.info1 = v;
    if (!r.info0) return;
    const int m = r.info0 == r.info1 ? v : 0;
    if (m != r.mem) { r.mem = m; r.since = win; }
}
static void ShVrrStep(ShVrrRead& r, FgCondIn& in, int nv, int fast, unsigned long long key, int win, int held, int base)
{
    if (base) in.vrr = nv;
    else { FgShipVrrKey(r.v, key, key, win); FgShipVrrTake(r.v, r.mem, r.since, win, held); in.vrr = FgShipVrrAnswer(r.v, nv, fast); }
    FgCondFixLatchStep(r.L, r.mem, held, key, in.vrr == 1 ? 1 : 0);
    in.flipQuant = FgCondFixQuantIn(r.L);
}
// ONE SESSION through the read, the conditions and the law (ShLawRun's loop with the read in front of the table). panel: what the screen does,
// 1 FREE (it follows the game: an unheld window reads FREE; A HELD WINDOW READS QUANTISED, the worst a pace on whole refreshes could read), 2 a
// fixed grid (an unheld window reads QUANTISED, a held one nothing: the fixed mode's own pace reads LIMITED), 0 nothing ever. resetAt: a Reset at
// that window (the key changes).
struct ShVrrOut { int on, row, vrr, mode, fixedRows, unknownRows, firstFormed, mayFormAt0, mayFormEnd; };
static ShVrrOut ShVrrRun(int nv, int fast, int panel, int vrrHz, int game, int windows, int base, int vsync = 1)
{
    ShVrrOut o = {}; o.firstFormed = -1;
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    FgCondIn in = ShSession(nv, vrrHz, 1, rel); in.vsync = vsync;
    ShVrrRead r = {};
    FgDriveK dk = FgDriveDefaults(); FgDriveMem mem; FgDriveInit(mem, dk);
    const unsigned long long key = 0x0000000100000001ull ^ ((unsigned long long)vrrHz << 32);
    for (int w = 1; w <= windows; w++)
    {
        const unsigned now = 1000u + 1000u * (unsigned)w;
        const int held = FgDriveWantsOn(mem) ? 1 : 0;
        // (the classifier's step for this window, ahead of the conditions' read, as the waiter runs them; no memory without vsync)
        const int verdict = !vsync ? 0 : panel == 1 ? (held ? 2 : 1) : panel == 2 ? (held ? 0 : 2) : 0;
        ShMemFeed(r, verdict, w, false);
        ShVrrStep(r, in, nv, fast, key, w, held, base);
        if (w == 1) o.mayFormAt0 = FgShipVrrMayForm(r.v, nv, vsync, w);
        if (!base && o.firstFormed < 0 && r.v.ev) o.firstFormed = w;
        in.engaged = held; in.engaging = mem.state == FGDR_ENGAGING ? 1 : 0;
        if (in.engaged)
        {
            const int cap = FgCondFixedMode(in) ? FgCondHalfX10(in) : FgCondCFpsX10(in);
            in.realFpsX10 = game < cap ? game : cap; in.freeFpsX10 = game; in.predFpsX10 = 0;
        }
        else { in.realFpsX10 = game; in.freeFpsX10 = 0; in.predFpsX10 = game; }
        const int c = FgCondFirst(in, k);
        if (c == FGC_FIXED) o.fixedRows++;
        if (c == FGC_VRR_UNKNOWN) o.unknownRows++;
        FgDriveIn di = {};
        di.row = c; di.band = FgDriveBandOf(in, k); di.cpuReady = 1; di.realX10 = in.realFpsX10; di.card = 0;
        if (in.engaged) { di.shownGen = 100; di.shownN = 100; di.floorUs = FgCondFloorUs(in); di.perUs = di.floorUs; di.lawPerUs = di.floorUs; }
        (void)FgDriveStep(mem, di, dk, now, w, 0);
        if (mem.state == FGDR_ENGAGING) (void)FgDriveCut(mem, dk, now + 1u, w + 1, 0);
        if (FgDriveWantsOn(mem)) o.on++;
        o.row = c;
    }
    o.vrr = in.vrr; o.mode = FgCondFixedMode(in) ? 1 : 0;
    o.mayFormEnd = FgShipVrrMayForm(r.v, nv, vsync, windows);
    return o;
}

// ---------------------------------------------------------------- SH6, NVIDIA's answer checked against the panel's own evidence
// THE OWNER'S DECISION 2: VRR ACTIVE FOR THE GAME NOW, not merely possible. (1) THE MORNING OF 2026-10-18: NVIDIA said fixed (the per-program
// G-Sync setting) and the panel followed the game: the answer is variable refresh and the doubling runs (a game of 50 on 120 Hz); the base refused
// it (FGC_FIXED). (2) NVIDIA says variable and the presents land on a fixed grid (G-Sync off on the display itself): the answer is fixed and the
// fixed mode runs (a game of 80 on 120 Hz, 60 real, 120 shown); the base read VRR and stood by (FGC_FAST: native, every refresh not a new frame).
// (3) NVIDIA's no with NVIDIA's own Fast Sync: the presents off the grid are no sign of variable refresh, FGC_FIXED as before. (4) NVIDIA's yes
// and the panel following the game: the doubling, and HELD WINDOWS THAT READ QUANTISED (the worst a pace of the mod's own could read) never turn
// the answer. (5) NVIDIA's no and nothing from the panel: the fixed mode as before. THE READ'S OWN RULES, step by step (as FX15 steps the latch):
// a memory from a window the pacer held is not taken; a memory set before the key changed is the old display's; once taken it holds against a
// later contradiction; a new key drops it. CONTROL: THE BASE'S WIRING (NVIDIA's answer alone) over the same sessions: (1) and (2) read otherwise,
// (3) and (5) the same, and (4) the same.
static void CaseSH6()
{
    const char* const name = "SH6 VRR active for the game now: NVIDIA's answer checked against the panel's own evidence, the doubling or the fixed mode by what the screen does (2026-10-24, round 7, W1)";
    CaseBegin(name);
    struct Row { const char* what; int nv, fast, panel, hz, game, wantVrr, wantMode, wantOn, wantFixedRow; };
    static const Row rows[] = {
        { "NVIDIA no, the panel follows (10-18)", 0, 0, 1, 120, 500, 1, 0, 1, 0 },
        { "NVIDIA yes, a fixed grid",              1, 0, 2, 120, 800, 0, 1, 1, 0 },
        { "NVIDIA no + Fast Sync, off the grid",   0, 1, 1, 120, 500, 0, 0, 0, 1 },
        { "NVIDIA yes, the panel follows",         1, 0, 1, 120, 500, 1, 0, 1, 0 },
        { "NVIDIA no, nothing from the panel",     0, 0, 0, 120, 800, 0, 1, 1, 0 } };
    int ctlDiff = 0, ctlSame = 0;
    for (const Row& r : rows)
    {
        const ShVrrOut o = ShVrrRun(r.nv, r.fast, r.panel, r.hz, r.game, 60, 0), b = ShVrrRun(r.nv, r.fast, r.panel, r.hz, r.game, 60, 1);
        printf("  %-40s vrr %2d, the mode %d, engaged %2d of 60, FGC_FIXED in %2d window(s), the row at the end %-14s (formed at window %d) | the base: vrr %2d, the mode %d, engaged %2d, FGC_FIXED %2d, %s\n",
               r.what, o.vrr, o.mode, o.on, o.fixedRows, FgCondEnumName(o.row), o.firstFormed, b.vrr, b.mode, b.on, b.fixedRows, FgCondEnumName(b.row));
        if (o.vrr != r.wantVrr || o.mode != r.wantMode) Err("SH6: %s: vrr %d, the mode %d (%d and %d wanted)", r.what, o.vrr, o.mode, r.wantVrr, r.wantMode);
        if (r.wantOn && o.on < 20) Err("SH6: %s: engaged %d of 60 window(s) (20 or more wanted)", r.what, o.on);
        if (!r.wantOn && o.on) Err("SH6: %s: engaged %d window(s) (never wanted)", r.what, o.on);
        if (r.wantFixedRow ? o.fixedRows == 0 : o.fixedRows != 0) Err("SH6: %s: FGC_FIXED in %d window(s) (%s wanted)", r.what, o.fixedRows, r.wantFixedRow ? "some" : "none");
        if (b.vrr != o.vrr || b.mode != o.mode || (b.on > 0) != (o.on > 0)) ctlDiff++; else ctlSame++;
    }
    // (THE READ'S OWN RULES, step by step: key 1, then a Reset)
    {
        FgShipVrr v = {};
        const unsigned long long k1 = 0x0000007800000001ull;
        struct Step { unsigned long long key; int mem, since, win, held, nv, want; const char* what; };
        const Step steps[] = {
            { k1,        0,  0,  1, 0, 1, 1, "NVIDIA yes, no memory: NVIDIA's" },
            { k1,        2,  4,  4, 1, 1, 1, "QUANTISED set at 4 in a held window: not taken" },
            { k1,        2,  4,  5, 0, 1, 1, "standing by at 5, that memory (from 3 and 4, held): not taken" },
            { k1,        2,  7,  7, 0, 1, 0, "QUANTISED set at 7 from 6 and 7, unheld: fixed" },
            { k1,        1,  9,  9, 0, 1, 0, "a later FREE: not taken (no flapping)" },
            { k1 ^ 2ull, 1,  9, 10, 0, 1, 1, "a Reset at 10: dropped, the old memory (9) not taken" },
            { k1 ^ 2ull, 1, 12, 12, 0, 1, 1, "FREE set at 12 from 11 and 12: variable" },
            { k1 ^ 2ull, 2, 14, 14, 0, 1, 1, "a later QUANTISED: not taken" } };
        for (const Step& s : steps)
        {
            FgShipVrrKey(v, s.key, k1, s.win); FgShipVrrTake(v, s.mem, s.since, s.win, s.held);
            const int a = FgShipVrrAnswer(v, s.nv, 0);
            printf("  %-62s -> %d\n", s.what, a);
            if (a != s.want) Err("SH6: %s: the answer %d (%d wanted)", s.what, a, s.want);
        }
    }
    if (ctlDiff != 2 || ctlSame != 3) Err("SH6 control: the base's wiring read %d of the 5 sessions otherwise and %d the same (2 and 3 wanted: the check cannot see the read)", ctlDiff, ctlSame);
    else Ctl("the base's wiring (NVIDIA's answer alone): the 10-18 panel refused (FGC_FIXED) and the fixed grid under NVIDIA's yes on the VRR path; the other three the same");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH7, the classifier alone on AMD, Intel and Linux
// No answer from NVIDIA (another vendor's card, Wine, a reading that failed): THE PANEL ALONE. (1) A panel that follows the game: variable
// refresh, the doubling at 50 on 120 Hz; the answer may still form at the first window (so the build asks for its set: FgShipVrrMayForm) and
// not once it has. (2) A fixed grid: the fixed mode at 80 on 120 Hz. (3) Nothing from the panel with vsync on: it may still form for
// FG_SHIP_VRR_WAIT_WIN windows, then FGC_VRR_UNKNOWN is said; (4) with vsync off it cannot form (no memory without vsync): said at once. CONTROL:
// THE BASE'S WIRING over the same sessions: (1) stands by on FGC_VRR_UNKNOWN every window (frame generation never runs on an AMD card's VRR
// panel); (2) the fixed mode there too (the base's memory already made it: the same).
static void CaseSH7()
{
    const char* const name = "SH7 the classifier alone on AMD, Intel and Linux: a panel that follows the game runs the doubling, a fixed grid the fixed mode (2026-10-24, round 7, W1)";
    CaseBegin(name);
    const ShVrrOut f = ShVrrRun(-1, 0, 1, 120, 500, 60, 0), fb = ShVrrRun(-1, 0, 1, 120, 500, 60, 1);
    const ShVrrOut q = ShVrrRun(-1, 0, 2, 120, 800, 60, 0), qb = ShVrrRun(-1, 0, 2, 120, 800, 60, 1);
    const ShVrrOut n = ShVrrRun(-1, 0, 0, 120, 500, 40, 0), n0 = ShVrrRun(-1, 0, 0, 120, 500, 20, 0), x = ShVrrRun(-1, 0, 1, 120, 500, 10, 0, 0);
    printf("  the panel follows: vrr %d, engaged %2d of 60, FGC_VRR_UNKNOWN in %d window(s), may form at the first window %d and at the end %d (formed at %d) | the base: vrr %d, engaged %d, FGC_VRR_UNKNOWN %d\n",
           f.vrr, f.on, f.unknownRows, f.mayFormAt0, f.mayFormEnd, f.firstFormed, fb.vrr, fb.on, fb.unknownRows);
    printf("  a fixed grid:      vrr %d, the mode %d, engaged %2d of 60 | the base: vrr %d, the mode %d, engaged %d\n", q.vrr, q.mode, q.on, qb.vrr, qb.mode, qb.on);
    printf("  nothing, vsync on: may form at window 20 %d, at window 40 %d (the row %s); vsync off: may form at the first window %d\n", n0.mayFormEnd, n.mayFormEnd, FgCondEnumName(n.row), x.mayFormAt0);
    if (f.vrr != 1 || f.on < 20 || f.mode) Err("SH7: the panel follows: vrr %d, engaged %d of 60, the mode %d (1, 20 or more, 0 wanted)", f.vrr, f.on, f.mode);
    if (!f.mayFormAt0 || f.mayFormEnd) Err("SH7: the panel follows: may form %d at the first window, %d at the end (1 and 0 wanted)", f.mayFormAt0, f.mayFormEnd);
    if (q.vrr != 0 || !q.mode || q.on < 20) Err("SH7: a fixed grid: vrr %d, the mode %d, engaged %d of 60 (0, 1, 20 or more wanted)", q.vrr, q.mode, q.on);
    if (!n0.mayFormEnd || n.mayFormEnd || n.row != FGC_VRR_UNKNOWN) Err("SH7: nothing from the panel: may form %d at window 20, %d at 40, the row %s (1, 0, FGC_VRR_UNKNOWN wanted)", n0.mayFormEnd, n.mayFormEnd, FgCondEnumName(n.row));
    if (x.mayFormAt0) Err("SH7: vsync off: may form %d at the first window (0 wanted: no memory forms without vsync)", x.mayFormAt0);
    if (fb.on != 0 || fb.unknownRows != 60 || !qb.mode || qb.on < 20) Err("SH7 control: the base: the following panel engaged %d, FGC_VRR_UNKNOWN in %d of 60; the grid the mode %d, engaged %d (0, 60, 1, 20 or more wanted: the check cannot see the read)", fb.on, fb.unknownRows, qb.mode, qb.on);
    else Ctl("the base's wiring: the following panel stood by on FGC_VRR_UNKNOWN all 60 windows (never engaged); the fixed grid ran the fixed mode, as now");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH8, the better refresh said once, never a display switched
// THE OWNER'S EXAMPLE: a fixed 144 Hz screen at a game of about 60 shows 72 at most (the even rate), where 120 Hz would show 120: the warnings file
// names 120 once. Rows (the game steady at the given rate, the refreshes the screen offers at its size): fixed 144 at 62 {60, 100, 120, 144}:
// 120; fixed 165 at 70 {60, 120, 144, 165}: 120 (144's floor is 74.2); fixed 240 at 100 {60, 120, 144, 165, 240}: 165; fixed 120 at 50
// {60, 100, 120}: none (60 Hz shows 60, the even rate at 120 already); fixed 144 at 80: none (the fixed mode runs); VRR 144 at 62: none (any
// rate is even); fixed 144 at 56 and 70 in turns: none (not steady). Said once: nothing is named after the line. CONTROL: WITHOUT THE EVEN-RATE
// TEST (any offered refresh where the table reads no row): fixed 120 at 50 would name 60 Hz, 30 real frames for the even rate it has already.
static void CaseSH8()
{
    const char* const name = "SH8 the better refresh: said once where an offered refresh would double cleanly (144 at 62: 120), never where the even rate is no worse (2026-10-24, round 7, W2)";
    CaseBegin(name);
    static const int o144[] = { 60, 100, 120, 144 }, o165[] = { 60, 120, 144, 165 }, o240[] = { 60, 120, 144, 165, 240 }, o120[] = { 60, 100, 120 };
    struct Row { const char* what; int vrr, hz, game, swing; const int* off; int n, want; };
    const Row rows[] = {
        { "fixed 144 at 62",       0, 144,  620,   0, o144, 4, 120 }, { "fixed 165 at 70", 0, 165, 700, 0, o165, 4, 120 },
        { "fixed 240 at 100",      0, 240, 1000,   0, o240, 5, 165 }, { "fixed 120 at 50", 0, 120, 500, 0, o120, 3, 0 },
        { "fixed 144 at 80 (on)",  0, 144,  800,   0, o144, 4, 0 },   { "VRR 144 at 62",   1, 144, 620, 0, o144, 4, 0 },
        { "fixed 144 at 56 and 70", 0, 144, 560, 140, o144, 4, 0 } };
    printf("  the even rate (the owner's rule): 144 Hz at 60: %d.%d; 120 Hz at 60: %d.%d\n", FgShipEvenX10(144000, 600) / 10, FgShipEvenX10(144000, 600) % 10, FgShipEvenX10(120000, 600) / 10, FgShipEvenX10(120000, 600) % 10);
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    int ctl = -1;
    for (const Row& r : rows)
    {
        FgCondIn in = ShSession(r.vrr, r.hz, 1, rel);
        FgShipBetter b = {};
        FgDriveK dk = FgDriveDefaults(); FgDriveMem mem; FgDriveInit(mem, dk);
        int named = 0, at = -1, again = 0;
        for (int w = 0; w < 40; w++)
        {
            const unsigned now = 1000u + 1000u * (unsigned)w;
            const int game = (r.swing && (w & 1)) ? r.game + r.swing : r.game;
            in.engaged = FgDriveWantsOn(mem) ? 1 : 0; in.engaging = mem.state == FGDR_ENGAGING ? 1 : 0;
            if (in.engaged) { const int cap = FgCondFixedMode(in) ? FgCondHalfX10(in) : FgCondCFpsX10(in); in.realFpsX10 = game < cap ? game : cap; in.freeFpsX10 = game; in.predFpsX10 = 0; }
            else { in.realFpsX10 = game; in.freeFpsX10 = 0; in.predFpsX10 = game; }
            const int c = FgCondFirst(in, k);
            FgDriveIn di = {};
            di.row = c; di.band = FgDriveBandOf(in, k); di.cpuReady = 1; di.realX10 = in.realFpsX10; di.card = 0;
            if (in.engaged) { di.shownGen = 100; di.shownN = 100; di.floorUs = FgCondFloorUs(in); di.perUs = di.floorUs; di.lawPerUs = di.floorUs; }
            (void)FgDriveStep(mem, di, dk, now, w, 0);
            if (mem.state == FGDR_ENGAGING) (void)FgDriveCut(mem, dk, now + 1u, w + 1, 0);
            // (as fg_arm_life.h's FgpFixWarnStep asks it: behind the law's step)
            const int g = FgShipBetterStep(b, in, c, FgDriveWantsOn(mem) ? 1 : 0);
            if (g)
            {
                const int hz = FgShipBetterHz(in, k, g, r.off, r.n);
                if (hz) { b.said = 1; if (!named) { named = hz; at = w; } else again++; }
                if (r.hz == 120 && r.game == 500)
                {   // (THE CONTROL: any offered refresh where the table reads no row with the mode on, the even rate not asked)
                    ctl = 0;
                    for (int i = 0; i < r.n; i++)
                    {
                        FgCondIn t = in; t.refreshNum = r.off[i]; t.refreshDen = 1; t.realFpsX10 = g;
                        if (r.off[i] != r.hz && FgCondFixedMode(t) && FgCondFirst(t, k) == FGC_OK && r.off[i] > ctl) ctl = r.off[i];
                    }
                }
            }
        }
        printf("  %-24s named %3d Hz at window %2d (%d more after it) | the even rate at %d Hz: %d.%d\n", r.what, named, at, again, r.hz, FgShipEvenX10(r.hz * 1000, r.game) / 10, FgShipEvenX10(r.hz * 1000, r.game) % 10);
        if (named != r.want) Err("SH8: %s: named %d Hz (%d wanted)", r.what, named, r.want);
        if (named && at != FG_SHIP_BETTER_WIN - 1) Err("SH8: %s: named at window %d (%d wanted: the run's last)", r.what, at, FG_SHIP_BETTER_WIN - 1);
        if (again) Err("SH8: %s: named %d more time(s) after the line (once wanted)", r.what, again);
    }
    if (ctl != 60) Err("SH8 control: without the even-rate test fixed 120 at 50 named %d Hz (60 wanted: the check cannot see the rule)", ctl);
    else Ctl("without the even-rate test fixed 120 at 50 would name 60 Hz (30 real, the even rate at 120 already 60)");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH9, the wait for the panel's answer is not reopened by a Reset (the round's review)
// THE REVIEW'S FINDING: where the answer never forms (no NVIDIA answer, vsync on, nothing from the panel: an AMD card with the game at the
// cap), the builds of the wait ask for a set, and with the wait counted from the display's key, which a Reset changes, every Reset reopened
// it and the build after it asked for a set again, held all session for nothing. NOW: the wait runs from the first read of a refresh and
// again only at a new refresh. A session: the first build at window 1 asks (the wait); window 31, the wait over: the next build asks for none;
// a Reset at window 40 (the same refresh): still none; a new refresh at window 50: the wait opens again and its build asks. FAIL-FIRST, THE
// FORM OF aa22b2a6 (the wait from the key): the build after the Reset at 40 asks for a set again. CONTROL: the new refresh at 50 opens the wait in
// both forms, and both ask at the first build: the check sees the wait where it must be.
static void CaseSH9()
{
    const char* const name = "SH9 the wait for the panel's answer is not reopened by a Reset: the build after it asks for no set where the answer never formed (2026-10-24, round 7, W1, the review)";
    CaseBegin(name);
    FgShipVrr v = {};
    const unsigned long long h1 = 0x0000007800000001ull, h2 = 0x0000003C00000001ull;
    struct Step { unsigned long long key, hz; int win, wantAsk, baseAsk; const char* what; };
    const Step steps[] = {
        { h1,        h1,  1, 1, 1, "the first build at window 1" },
        { h1,        h1, 20, 1, 1, "window 20, in the wait" },
        { h1,        h1, 31, 0, 0, "window 31, the wait over" },
        { h1 ^ 2ull, h1, 40, 0, 1, "a Reset at window 40, the same refresh" },
        { h1 ^ 2ull, h1, 45, 0, 1, "window 45" },
        { h2 ^ 2ull, h2, 50, 1, 1, "a new refresh at window 50" },
        { h2 ^ 2ull, h2, 81, 0, 0, "window 81, that wait over" } };
    int bad = 0, baseAgain = 0, ctl = 0; int baseKeyWin = 0; unsigned long long baseKey = 0; bool baseKeyed = false;
    for (const Step& s : steps)
    {
        FgShipVrrKey(v, s.key, s.hz, s.win);
        const int ask = FgShipVrrMayForm(v, -1, 1, s.win);
        // (the form of aa22b2a6: the wait counted from the display's key)
        if (!baseKeyed || s.key != baseKey) { baseKey = s.key; baseKeyed = true; baseKeyWin = s.win; }
        const int bAsk = (s.win - baseKeyWin < FG_SHIP_VRR_WAIT_WIN) ? 1 : 0;
        printf("  %-44s the build asks for a set: %d (the form of aa22b2a6: %d)\n", s.what, ask, bAsk);
        if (ask != s.wantAsk) { bad++; Err("SH9: %s: asks %d (%d wanted)", s.what, ask, s.wantAsk); }
        if (bAsk != s.baseAsk) Err("SH9: %s: the form of aa22b2a6 asks %d (%d: the model of the old form is wrong)", s.what, bAsk, s.baseAsk);
        if (bAsk && !s.wantAsk) baseAgain++;
        if (s.win == 1 || s.win == 50) ctl += (ask && bAsk) ? 1 : 0;
    }
    if (baseAgain != 2 || ctl != 2) Err("SH9 control: the old form asked again after the Reset at %d build(s) (2 wanted), both forms ask at the wait's start at %d of 2 (the check cannot see the wait)", baseAgain, ctl);
    else Ctl("the form of aa22b2a6 asks for a set again at the 2 builds after the Reset; both forms ask where a wait opens (window 1, a new refresh at 50)");
    CaseEnd(name);
}

// ================================================================ fg18-fix, THE RELEASE AUDIT OF 2026-10-28 (F1 to F4, F10): SH10 to SH12
// A PLAYER'S LINE, HELD TO THE OWNER'S RULES: plain ASCII, no we, us or our, no log=1, no vendor named where the mod read none
static bool ShPlain(const std::string& s)
{
    for (unsigned char ch : s) if (ch < 0x20 || ch > 0x7E) return false;
    std::string w = " " + s + " ";
    for (char& ch : w) if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a');
    for (const char* bad : { " we ", " us ", " our ", "log=1" }) if (w.find(bad) != std::string::npos) return false;
    for (const char* p : { " we,", " we.", " us,", " us.", " our,", " our." }) if (w.find(p) != std::string::npos) return false;
    return true;
}

// ---------------------------------------------------------------- SH10, a fixed screen of 59 Hz and up with vsync off says to turn vsync on (F2)
// THE AUDIT'S F2: a fixed 60 to 90 Hz screen with the game's vsync off fell through to FGC_PANEL ("the display runs at 60.00 Hz and it needs about
// 97 Hz or more"), a RESET row, so no set until the next build. NOW such a screen the fixed mode may run on stands by on FGC_VSYNCOFF, a STANDBY row
// that warns, whose line says to turn vsync on (at 75 Hz, whose half refresh 37.5 is under the floor of 38, fg_min_fps=30 too); a fixed screen under
// 59 Hz and a VRR panel under 59 Hz still read FGC_PANEL. AND THE REMEDY HOLDS: the same screens with vsync on (and at 75 Hz fg_min_fps=30) run the
// fixed mode, the table reading no row. FAIL-FIRST CONTROL: THE RULE OF BEFORE (the panel row without its new clause: C under 48 on a display read
// fixed) reads FGC_PANEL at all four screens of 59 Hz and up, so the case sees the row it replaced.
static void CaseSH10()
{
    const char* const name = "SH10 a fixed screen of 59 Hz and up with vsync off stands by on FGC_VSYNCOFF (turn vsync on), not FGC_PANEL; under 59 Hz the panel row stays (fg18-fix, F2)";
    CaseBegin(name);
    struct Row { const char* what; int vrr, num, den, game, want, keyLine; };
    static const Row rows[] = { { "fixed 60, vsync off, at 45", 0, 60, 1, 450, FGC_VSYNCOFF, 0 }, { "fixed 59.94, vsync off, at 45", 0, 60000, 1001, 450, FGC_VSYNCOFF, 0 },
                                { "fixed 75, vsync off, at 60", 0, 75, 1, 600, FGC_VSYNCOFF, 1 }, { "fixed 90, vsync off, at 70", 0, 90, 1, 700, FGC_VSYNCOFF, 0 },
                                { "fixed 50, vsync off, at 45", 0, 50, 1, 450, FGC_PANEL, 0 }, { "VRR 50, at 30", 1, 50, 1, 300, FGC_PANEL, 0 } };
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    int before = 0;
    for (const Row& r : rows)
    {
        FgCondIn in = ShSession(r.vrr, r.num, r.den, rel); in.vsync = r.vrr ? 1 : 0; in.realFpsX10 = r.game; in.predFpsX10 = r.game;
        const int c = FgCondFirst(in, k);
        const std::string line = c ? FxCondLine(c, in, k) : std::string();
        printf("  %-30s -> %-12s %s\n", r.what, FgCondEnumName(c), line.c_str());
        if (c != r.want) Err("SH10: %s: %s (%s wanted)", r.what, FgCondEnumName(c), FgCondEnumName(r.want));
        if (r.want == FGC_VSYNCOFF)
        {
            if (FgCondClassOf(c) != FGCC_STANDBY || !FgCondWarns(c)) Err("SH10: %s: class %d, warns %d (STANDBY and the warnings file wanted)", r.what, FgCondClassOf(c), FgCondWarns(c) ? 1 : 0);
            if (line.find("turn vsync on") == std::string::npos) Err("SH10: %s: the line does not say to turn vsync on: %s", r.what, line.c_str());
            if ((line.find("fg_min_fps=30") != std::string::npos) != (r.keyLine != 0)) Err("SH10: %s: the line %s fg_min_fps=30 (%s wanted): %s", r.what, r.keyLine ? "does not name" : "names", r.keyLine ? "named: the half refresh is under the floor" : "not named", line.c_str());
            if (!ShPlain(line)) Err("SH10: %s: the line breaks the player text's rules: %s", r.what, line.c_str());
            // (the remedy: vsync on, and at 75 Hz the key too, runs the fixed mode)
            FgCondIn on = in; on.vsync = 1;
            FgShipKeys kk = {}; kk.minFps = r.keyLine ? 30 : 0; FgCondK k2 = FgCondDefaults(); FgShipCondKeys(k2, kk);
            const int c2 = FgCondFirst(on, k2);
            if (!FgCondFixedMode(on) || c2 != FGC_OK) Err("SH10: %s: the remedy (vsync on%s) reads %s, the mode %s (FGC_OK, the mode on, wanted)", r.what, r.keyLine ? " and fg_min_fps=30" : "", FgCondEnumName(c2), FgCondFixedMode(on) ? "on" : "off");
            // (THE RULE OF BEFORE: the panel row with no clause for a fixed screen with vsync off)
            if (FgCondFixedSeen(in) && FgCondCFpsX10(in) > 0 && FgCondCFpsX10(in) < k.minCFpsX10) before++;
        }
    }
    if (before != 4) Err("SH10 control: the rule of before reads FGC_PANEL at %d of the 4 screens of 59 Hz and up (4 wanted: the case cannot see the row it replaced)", before);
    else Ctl("the rule of before (FGC_PANEL, C under 48 on a display read fixed) holds at all 4 screens of 59 Hz and up with vsync off");
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH11, the rate rows reach the warnings file only after a long stretch with no engage (F1)
// THE AUDIT'S F1: FGC_FAST and FGC_ONSLOW went to the warnings file at the first window they stood, and the owner's release legs' files opened with
// them on fixed screens before the engage. NOW: neither warns by itself (FgCondWarns), and fg_ship.h's FgShipLongStep says ONE line where a session
// that asked for frame generation never switched it on in its first five minutes of play and one family of the rate rows stood in at least 3 of 4
// of those windows. Windows of 2 s on a made-up clock: (A) FAST throughout: said once, at the first window past 300 s of play, the line naming the
// game's 90 and the top 67; (B) SLOW throughout at 30 on VRR 120: said, with fg_min_fps=30; (C) ONSLOW throughout: said, the rate with it named;
// (D) FAST, then one engage at window 100: never; (E) half FAST, half SLOW: never; (F) 200 windows of menu, then FAST: said only once 300 s OUT of
// menus are counted; (G) a pause of a minute between two windows counts as no play; (H) SLOW where the fixed screen's half refresh is under the
// floor (its own line, FgShipFixWarn's 2): never. FAIL-FIRST CONTROL: THE RULE OF BEFORE (FgCondWarns with FAST and ONSLOW in) writes A's row at
// its first window, and C's too.
static int ShLongRun(FgCondIn in, const FgCondK& k, const int* rows, int n, int engageAt, int menuTo, int gapAt, int* sayRow, std::string* line, int* saidN)
{
    FgShipLong L = {}; int at = -1; *saidN = 0; *sayRow = 0;
    unsigned now = 5000;
    for (int w = 0; w < n; w++)
    {
        now += (w == gapAt) ? 60000u : 2000u;
        const int r = rows[w % 2 == 0 ? 0 : 1];
        const int s = FgShipLongStep(L, in, k, r, w == engageAt ? 1 : 0, w < menuTo ? 1 : 0, now);
        if (s)
        {
            (*saidN)++;
            if (at < 0) { at = w; *sayRow = s; char b[400]; if (FgShipLongLine(s, in, k, b, sizeof(b)) > 0) *line = b; }
        }
    }
    return at;
}
static void CaseSH11()
{
    const char* const name = "SH11 a rate row reaches the warnings file only after five minutes of play with no engage, once (fg18-fix, F1)";
    CaseBegin(name);
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    if (FgCondWarns(FGC_FAST) || FgCondWarns(FGC_ONSLOW) || FgCondWarns(FGC_SLOW) || !FgCondWarns(FGC_VSYNCOFF) || !FgCondWarns(FGC_FFCAP))
        Err("SH11: FgCondWarns: FAST %d, ONSLOW %d, SLOW %d, VSYNCOFF %d, FFCAP %d (0, 0, 0, 1, 1 wanted)", FgCondWarns(FGC_FAST), FgCondWarns(FGC_ONSLOW), FgCondWarns(FGC_SLOW), FgCondWarns(FGC_VSYNCOFF), FgCondWarns(FGC_FFCAP));
    FgCondIn fast = ShSession(1, 120, 1, rel); fast.realFpsX10 = 900; fast.predFpsX10 = 800;
    FgCondIn slow = ShSession(1, 120, 1, rel); slow.realFpsX10 = 300; slow.predFpsX10 = 280;
    FgCondIn ons = ShSession(1, 120, 1, rel); ons.realFpsX10 = 400; ons.predFpsX10 = 350;
    FgCondIn half = ShSession(0, 75, 1, rel); half.realFpsX10 = 600;   // (a fixed 75 Hz with vsync: its half refresh 37.5 under 38, FGC_SLOW by the floor)
    const int playWin = FG_SHIP_LONG_MS / 2000;                        // (300 s of play at 2 s a window: window 0 counts none, so window 150 is the first at 300 s)
    struct Case { const char* what; const FgCondIn* in; int r0, r1, n, engageAt, menuTo, gapAt; int wantAt, wantRow; const char* words; };
    const Case cs[] = {
        { "A FAST throughout (VRR 120, 90 by itself)", &fast, FGC_FAST, FGC_FAST, 400, -1, 0, -1, playWin, FGC_FAST, "about 90 fps by itself" },
        { "B SLOW throughout (VRR 120 at 30)", &slow, FGC_SLOW, FGC_SLOW, 400, -1, 0, -1, playWin, FGC_SLOW, "fg_min_fps=30" },
        { "C ONSLOW throughout (40 alone, 35 with it)", &ons, FGC_ONSLOW, FGC_ONSLOW, 400, -1, 0, -1, playWin, FGC_ONSLOW, "about 35 with frame generation" },
        { "D FAST, one engage at window 100", &fast, FGC_FAST, FGC_FAST, 400, 100, 0, -1, -1, 0, nullptr },
        { "E half FAST, half SLOW", &fast, FGC_FAST, FGC_SLOW, 400, -1, 0, -1, -1, 0, nullptr },
        { "F 200 windows of menu, then FAST", &fast, FGC_FAST, FGC_FAST, 400, -1, 200, -1, 200 + playWin - 1, FGC_FAST, "about 90 fps by itself" },
        { "G FAST, a pause of a minute at window 50", &fast, FGC_FAST, FGC_FAST, 400, -1, 0, 50, playWin + 1, FGC_FAST, "about 90 fps by itself" },
        { "H SLOW by the fixed screen's floor (its own line)", &half, FGC_SLOW, FGC_SLOW, 400, -1, 0, -1, -1, 0, nullptr } };
    for (const Case& c : cs)
    {
        const int rr[2] = { c.r0, c.r1 };
        int row = 0, saidN = 0; std::string line;
        const int at = ShLongRun(*c.in, k, rr, c.n, c.engageAt, c.menuTo, c.gapAt, &row, &line, &saidN);
        printf("  %-50s said at window %4d (%d time(s)), %s%s%s\n", c.what, at, saidN, FgCondEnumName(row), line.empty() ? "" : ": ", line.c_str());
        if (at != c.wantAt || row != c.wantRow) Err("SH11: %s: said at window %d, %s (%d, %s wanted)", c.what, at, FgCondEnumName(row), c.wantAt, FgCondEnumName(c.wantRow));
        if (saidN > 1) Err("SH11: %s: said %d times (once wanted)", c.what, saidN);
        if (c.words && line.find(c.words) == std::string::npos) Err("SH11: %s: the line does not say \"%s\": %s", c.what, c.words, line.c_str());
        if (c.wantRow == FGC_FAST && line.find("under about 67 fps") == std::string::npos) Err("SH11: %s: the line does not name the top, 67: %s", c.what, line.c_str());
        if (!line.empty() && (!ShPlain(line) || line.size() >= 400 || line.find("5 minutes") == std::string::npos)) Err("SH11: %s: the line breaks the rules (plain, under 400, names the 5 minutes): %s", c.what, line.c_str());
    }
    {   // (THE RULE OF BEFORE: the row's line in the file at the first window it stood)
        auto oldWarns = [](int c) { return c != FGC_OK && c != FGC_MENU && c != FGC_SLOW && c != FGC_FP16; };
        if (!oldWarns(FGC_FAST) || !oldWarns(FGC_ONSLOW)) Err("SH11 control: the rule of before does not write FAST and ONSLOW (the model of the old rule is wrong)");
        else Ctl("the rule of before writes A's FAST and C's ONSLOW at their first window (window 0), where the new rule waits until window %d (300 s of play)", playWin);
    }
    CaseEnd(name);
}

// ---------------------------------------------------------------- SH12, fg_min_fps over the screen, and the lines a player reads (F3, F4, F10)
// THE AUDIT'S F4 (the key over the screen): on VRR 120 fg_min_fps=60 keeps FGC_SLOW standing for good (C is 59.1): FgShipFloorUnreachable names the
// most this screen allows, 59; at 59 and at 0 nothing; on VRR 240 at 100 nothing (C 118); on a FIXED 120 with vsync off and the key at 60 nothing
// (vsync on runs the fixed mode at 60, which the key allows: the CONTROL of the VRR-only rule). F3: the could-not-tell line names no vendor, and
// with vsync off says to turn it on; it keeps the kit's and the take-back's opening words. F10: the rows a player cannot mend ask for the log, and
// exclusive fullscreen says Borderless, not "parked". CONTROL: the could-not-tell line of before names NVIDIA's driver, which the check sees.
static void CaseSH12()
{
    const char* const name = "SH12 fg_min_fps over what the screen allows is said; the could-not-tell line names no vendor; the internal rows ask for the log (fg18-fix, F3, F4, F10)";
    CaseBegin(name);
    struct Fl { const char* what; int vrr, hz, vsync, key, want; };
    static const Fl fl[] = { { "VRR 120, fg_min_fps=60", 1, 120, 1, 60, 59 }, { "VRR 120, fg_min_fps=59", 1, 120, 1, 59, 0 }, { "VRR 120, no key", 1, 120, 1, 0, 0 },
                             { "VRR 240, fg_min_fps=100", 1, 240, 1, 100, 0 }, { "fixed 120, vsync off, fg_min_fps=60", 0, 120, 0, 60, 0 } };
    for (const Fl& f : fl)
    {
        FgShipKeys kk = {}; kk.minFps = f.key; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, kk);
        FgCondIn in = ShSession(f.vrr, f.hz, 1, kk); in.vsync = f.vsync;
        const int got = FgShipFloorUnreachable(in, k);
        printf("  %-40s the most this screen allows: %d\n", f.what, got);
        if (got != f.want) Err("SH12: %s: %d (%d wanted)", f.what, got, f.want);
    }
    const FgShipKeys rel = {}; FgCondK k = FgCondDefaults(); FgShipCondKeys(k, rel);
    for (int vs = 0; vs < 2; vs++)
    {
        FgCondIn in = ShSession(-1, 120, 1, rel); in.vsync = vs;
        const std::string l = FxCondLine(FGC_VRR_UNKNOWN, in, k);
        printf("  could not tell, vsync %s: %s\n", vs ? "on " : "off", l.c_str());
        if (l.find("NVIDIA") != std::string::npos || l.find("AMD") != std::string::npos || l.find("Intel") != std::string::npos) Err("SH12: the could-not-tell line names a vendor: %s", l.c_str());
        if ((l.find("Turn the game's vsync on") != std::string::npos) != (vs == 0)) Err("SH12: vsync %d: the line %s to turn vsync on: %s", vs, vs ? "says" : "does not say", l.c_str());
        if (l.rfind("frame generation: off: the mod could not tell whether the display runs variable refresh", 0) != 0) Err("SH12: the line lost its opening words (the kit and the take-back read them): %s", l.c_str());
        if (!ShPlain(l)) Err("SH12: the line breaks the rules: %s", l.c_str());
    }
    {
        FgCondIn in = ShSession(1, 120, 1, rel); in.restoreLost = 2; in.swapEffect = 3; in.backBuffers = 5; in.windowed = 0;
        for (int c : { FGC_DEAD, FGC_RESTORE, FGC_PRESENT, FGC_MARKER, FGC_SWAPCHAIN, FGC_EXCLUSIVE })
        {
            const std::string l = FxCondLine(c, in, k);
            printf("  %-14s %s\n", FgCondEnumName(c), l.c_str());
            if (!FgCondWarns(c)) Err("SH12: %s does not reach the warnings file", FgCondEnumName(c));
            if (c == FGC_EXCLUSIVE ? (l.find("Borderless") == std::string::npos || l.find("parked") != std::string::npos) : l.find("please send me DLSS-IV.log") == std::string::npos)
                Err("SH12: %s: %s", FgCondEnumName(c), l.c_str());
            if (!ShPlain(l) || !(l.rfind("frame generation: off: ", 0) == 0 || l.rfind("frame generation: off for this session: ", 0) == 0)) Err("SH12: %s: the rules or the kit's opening words: %s", FgCondEnumName(c), l.c_str());
        }
    }
    {   // (THE CONTROL: the line of before, which named NVIDIA's driver on every card)
        const std::string old = "frame generation: off: the mod could not tell whether the display runs variable refresh (G-Sync or FreeSync) for the game: NVIDIA's driver did not say, and the game's frames did not show it";
        if (old.find("NVIDIA") == std::string::npos) Err("SH12 control: the vendor check cannot see the line of before");
        else Ctl("the vendor check sees NVIDIA in the could-not-tell line of before");
    }
    CaseEnd(name);
}
