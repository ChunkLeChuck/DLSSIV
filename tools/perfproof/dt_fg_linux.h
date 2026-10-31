// FRAME GENERATION UNDER WINE AND PROTON, THE PURE RULES WITH WINE'S ANSWERS FAKED AT THE READER (round 7, L, 2026-10-24;
// source/fg/fg_linux_rule.h, docs/reports/fg-5/linux/READING.md): cases WN1 to WN5. Included by dt_fg_test.cpp after dt_fg_ship.h.
//
// No Linux machine is at this desk. What the game decides under Wine is fg_linux_rule.h's pure functions, and each case runs THEM over
// the answers Wine can give (the sync mode, the bridge, the two fences' readings, the helper's status), beside THE BASE'S rule
// (c76d58c1: frame generation refused under Wine, and an import that said success trusted with no probe) as the fail-first, and a
// spoiled copy of the rule as the control, so a case cannot pass on inputs that make no difference. The wire and the real fences
// are tools\test-fg-pipe's (sessions L1 to L4, the BUILT helper); pacing is the field's, never this file's.
#include "../../source/fg/fg_linux_rule.h"

// THE BASE'S TWO RULES, as c76d58c1 has them: FgxPlatformOk's Wine test, and the import trusted (no probe: every reading passes)
static bool WnBasePlatform(bool wine, int sync, bool bridge) { return !wine && sync == 0 && !bridge; }
static int WnBaseProbe(bool, uint64_t, uint64_t) { return FGX_WHY_OK; }

// ---------------------------------------------------------------- WN1, the platform under Wine
// THE RULE: under Wine frame generation may be asked on the game's own semaphores (sync 1) and never on the bridge; sync 0 (the
// helper's D3D12 fences, Valve's silent break) and sync 2 (the bridge forced) are refused. FAIL FIRST: the base refuses all six.
static void CaseWN1()
{
    const char* const name = "WN1 the platform under Wine: sync 1 off the bridge may ask, nothing else (fail-first: the base refuses all) (2026-10-24, round 7, L)";
    CaseBegin(name);
    int now = 0, base = 0;
    for (int sync = 0; sync < 3; sync++) for (int bridge = 0; bridge < 2; bridge++)
    {
        const bool ok = FgxWinePlatformOk(sync, bridge != 0), want = sync == 1 && !bridge;
        printf("  Wine, sync %d, bridge %d: %s (the base: %s)\n", sync, bridge, ok ? "may ask" : "refused", WnBasePlatform(true, sync, bridge != 0) ? "may ask" : "refused");
        if (ok != want) Err("WN1: Wine, sync %d, bridge %d reads %s (%s wanted)", sync, bridge, ok ? "may ask" : "refused", want ? "may ask" : "refused");
        now += ok ? 1 : 0; base += WnBasePlatform(true, sync, bridge != 0) ? 1 : 0;
    }
    if (now != 1) Err("WN1: Wine may ask in %d of the six cases (one wanted)", now);
    if (base != 0) Err("WN1 fail-first: the base's rule lets Wine ask in %d case(s) (none: the base refused Wine)", base);
    else Ctl("THE BASE refuses Wine in all six cases (it fails first on Wine, sync 1, off the bridge); the rule now asks in exactly that one");
    CaseEnd(name);
}

// ---------------------------------------------------------------- WN2, the gate before a 'g'
// THE RULE: a session whose verdict is FAILED never asks again and says nothing again (-1); the bridge refuses with its reason; every
// other state asks. CONTROL: a gate that does not read the state would let a failed session ask again at the next build.
static int WnSpoiledGate(int, bool bridge) { return bridge ? FGX_WHY_BRIDGE : FGX_WHY_OK; }
static void CaseWN2()
{
    const char* const name = "WN2 the gate before a 'g': a failed session never asks again, the bridge refuses with its reason (2026-10-24, round 7, L)";
    CaseBegin(name);
    struct Row { int state; int bridge; int want; };
    static const Row rows[] = { { FGX_WINE_NONE, 0, FGX_WHY_OK }, { FGX_WINE_MADE, 0, FGX_WHY_OK }, { FGX_WINE_LIVE, 0, FGX_WHY_OK },
                                { FGX_WINE_NONE, 1, FGX_WHY_BRIDGE }, { FGX_WINE_MADE, 1, FGX_WHY_BRIDGE },
                                { FGX_WINE_FAILED, 0, -1 }, { FGX_WINE_FAILED, 1, -1 } };
    int spoiledWrong = 0;
    for (const Row& r : rows)
    {
        const int g = FgxWineGate(r.state, r.bridge != 0);
        printf("  state %d, bridge %d: %d (%d wanted)\n", r.state, r.bridge, g, r.want);
        if (g != r.want) Err("WN2: state %d, bridge %d: the gate reads %d (%d wanted)", r.state, r.bridge, g, r.want);
        if (WnSpoiledGate(r.state, r.bridge != 0) != r.want) spoiledWrong++;
    }
    if (spoiledWrong < 2) Err("WN2 control: a gate blind to the state is wrong in %d row(s) (both FAILED rows wanted): the rows cannot see it", spoiledWrong);
    else Ctl("a gate blind to the state is wrong in %d rows (a failed session would ask again): the rows see it", spoiledWrong);
    CaseEnd(name);
}

// ---------------------------------------------------------------- WN3, the game's half of the probe, Wine's readings faked
// THE RULE: the set holds the game's fences only where the ack echoes the three handles and fgPrep and fgOut BOTH read the probe's
// value or above. A silent fence (Wine's "success" whose values never connect) reads 0. FAIL FIRST: the base trusted the import (every
// reading passes), so it holds a silent fence. CONTROL: a verdict that reads fgPrep alone misses a silent fgOut.
static int WnSpoiledProbe(bool echo, uint64_t prep, uint64_t) { return !echo ? FGX_WHY_ECHO : prep < FGX_WINE_PROBE_N ? FGX_WHY_GAME : FGX_WHY_OK; }
static void CaseWN3()
{
    const char* const name = "WN3 the game's half of the probe over Wine's readings: silent, one way, echo wrong, connected (fail-first: the base holds a silent fence) (2026-10-24, round 7, L)";
    CaseBegin(name);
    struct Row { const char* what; int echo; uint64_t prep, out; int want; };
    static const Row rows[] = { { "silent both (Wine's silent import)", 1, 0, 0, FGX_WHY_GAME }, { "fgPrep silent", 1, 0, 1, FGX_WHY_GAME },
                                { "fgOut silent", 1, 1, 0, FGX_WHY_GAME }, { "echo wrong", 0, 1, 1, FGX_WHY_ECHO }, { "a missing fence (echo 0)", 0, 0, 0, FGX_WHY_ECHO },
                                { "connected at the value", 1, 1, 1, FGX_WHY_OK }, { "connected above it", 1, 5, 7, FGX_WHY_OK } };
    int baseHeldSilent = 0, spoiledWrong = 0;
    for (const Row& r : rows)
    {
        const int v = FgxWineProbeWhy(r.echo != 0, r.prep, r.out);
        printf("  %-36s %d (%d wanted; the base %d)\n", r.what, v, r.want, WnBaseProbe(r.echo != 0, r.prep, r.out));
        if (v != r.want) Err("WN3: %s: the verdict %d (%d wanted)", r.what, v, r.want);
        if (r.want != FGX_WHY_OK && WnBaseProbe(r.echo != 0, r.prep, r.out) == FGX_WHY_OK) baseHeldSilent++;
        if (WnSpoiledProbe(r.echo != 0, r.prep, r.out) != r.want) spoiledWrong++;
    }
    if (baseHeldSilent < 3) Err("WN3 fail-first: the base held %d of the faulty readings (the three silent ones at least wanted)", baseHeldSilent);
    else Ctl("THE BASE (no probe) holds %d of the 5 faulty readings, the three silent fences among them; the verdict now refuses every one", baseHeldSilent);
    if (spoiledWrong < 1) Err("WN3 control: a verdict that reads fgPrep alone is wrong in no row: the rows cannot see a silent fgOut");
    else Ctl("a verdict that reads fgPrep alone is wrong in %d row(s): the rows see a silent fgOut", spoiledWrong);
    CaseEnd(name);
}

// ---------------------------------------------------------------- WN4, the session's verdict over a session of builds
// THE RULE: the state moves NONE -> MADE (the fences made) -> LIVE (the probe passed); any reason fails the session for good; nothing
// leaves FAILED, so after a failure EVERY later build is refused at the gate with no exchange and no line. Run over three sessions
// built from Wine's answers: connected, the helper silent (its status), the game silent (its readings). CONTROL: a step that lets a
// later good probe lift FAILED would ask the helper again after a silent fence.
static int WnSpoiledNext(int state, int why, bool probed) { if (why != FGX_WHY_OK) return FGX_WINE_FAILED; return probed ? FGX_WINE_LIVE : state; }
struct WnSession { int asks, refusedQuiet, said, state; };
static WnSession WnRun(int (*next)(int, int, bool), int helperStatusFences, uint64_t prep, uint64_t out, int builds)
{
    WnSession s = { 0, 0, 0, FGX_WINE_NONE };
    for (int b = 0; b < builds; b++)
    {
        const int g = FgxWineGate(s.state, false);
        if (g < 0) { s.refusedQuiet++; continue; }
        if (s.state == FGX_WINE_NONE) s.state = FGX_WINE_MADE;   // (FgxWineBefore: made, handed over, the game's test signal given)
        s.asks++;
        int why = FgxWineAckWhy(helperStatusFences != 0 && b == 0);
        // (the game's half only on a CREATED set it does not hold yet; a later build of a LIVE session holds them already)
        if (!why && s.state == FGX_WINE_MADE) why = FgxWineProbeWhy(true, b == 0 ? prep : 1, b == 0 ? out : 1);
        const int was = s.state;
        s.state = next(s.state, why, why == FGX_WHY_OK && was == FGX_WINE_MADE);
        if (s.state == FGX_WINE_FAILED && was != FGX_WINE_FAILED) s.said++;
    }
    return s;
}
static void CaseWN4()
{
    const char* const name = "WN4 the session's verdict over ten builds: connected stays LIVE, a silent fence fails it for good and the helper is never asked again (2026-10-24, round 7, L)";
    CaseBegin(name);
    const WnSession ok = WnRun(FgxWineNext, 0, 1, 1, 10), hs = WnRun(FgxWineNext, 1, 0, 0, 10), gs = WnRun(FgxWineNext, 0, 0, 0, 10);
    printf("  connected: asks %d, refused quietly %d, said %d, the end %d\n", ok.asks, ok.refusedQuiet, ok.said, ok.state);
    printf("  the helper silent: asks %d, refused quietly %d, said %d, the end %d\n", hs.asks, hs.refusedQuiet, hs.said, hs.state);
    printf("  the game silent: asks %d, refused quietly %d, said %d, the end %d\n", gs.asks, gs.refusedQuiet, gs.said, gs.state);
    if (ok.asks != 10 || ok.said || ok.state != FGX_WINE_LIVE) Err("WN4: connected: asks %d, said %d, the end %d (10, 0 and LIVE wanted)", ok.asks, ok.said, ok.state);
    if (hs.asks != 1 || hs.refusedQuiet != 9 || hs.said != 1 || hs.state != FGX_WINE_FAILED) Err("WN4: the helper silent: asks %d, quiet %d, said %d, the end %d (1, 9, 1 and FAILED wanted)", hs.asks, hs.refusedQuiet, hs.said, hs.state);
    if (gs.asks != 1 || gs.refusedQuiet != 9 || gs.said != 1 || gs.state != FGX_WINE_FAILED) Err("WN4: the game silent: asks %d, quiet %d, said %d, the end %d (1, 9, 1 and FAILED wanted)", gs.asks, gs.refusedQuiet, gs.said, gs.state);
    // THE CONTROL: the spoiled step over a session whose first probe fails and whose later readings would pass
    const int a = WnSpoiledNext(FGX_WINE_FAILED, FGX_WHY_OK, true), r = FgxWineNext(FGX_WINE_FAILED, FGX_WHY_OK, true);
    if (a == FGX_WINE_FAILED || r != FGX_WINE_FAILED) Err("WN4 control: a step that lifts FAILED reads %d and the rule %d (LIVE and FAILED wanted: the case cannot see a lifted verdict)", a, r);
    else Ctl("a step that lets a good reading lift FAILED would go back to LIVE (%d): the rule keeps FAILED (%d)", a, r);
    CaseEnd(name);
}

// ---------------------------------------------------------------- WN5, the one warnings line, by its reason
// THE RULE: every reason has one line, plain ASCII, no em dash, no "we", "us" or "our" (tools\check-we.py's rule), saying frame
// generation is off for this session and that the upscaler keeps running; no reason, no line. CONTROL: the check finds a planted "we".
static bool WnHasWe(const char* s)
{
    static const char* const words[] = { "we", "us", "our", "ours", "ourselves" };
    for (const char* p = s; *p; p++)
    {
        if (p != s && isalpha((unsigned char)p[-1])) continue;
        for (const char* w : words)
        {
            const size_t n = strlen(w);
            if (_strnicmp(p, w, n) == 0 && !isalpha((unsigned char)p[n])) return true;
        }
    }
    return false;
}
static void CaseWN5()
{
    const char* const name = "WN5 the one warnings line of each reason: plain, no we/us/our, off for the session, the upscaler running (2026-10-24, round 7, L)";
    CaseBegin(name);
    const char* seen[FGX_WHY_N] = {};
    for (int w = FGX_WHY_BRIDGE; w < FGX_WHY_N; w++)
    {
        const char* t = FgxWineWarnText(w);
        printf("  %d: %s\n", w, t ? t : "(none)");
        if (!t) { Err("WN5: reason %d has no line", w); continue; }
        bool ascii = true; for (const char* p = t; *p; p++) if ((unsigned char)*p > 127) ascii = false;
        if (!ascii) Err("WN5: reason %d's line is not plain ASCII (an em dash or another mark)", w);
        if (WnHasWe(t)) Err("WN5: reason %d's line says we, us or our", w);
        if (strncmp(t, "frame generation: off for this session: ", 40) != 0) Err("WN5: reason %d's line does not begin \"frame generation: off for this session: \"", w);
        const size_t n = strlen(t);
        if (n < 26 || strcmp(t + n - 26, "The upscaler keeps running") != 0) Err("WN5: reason %d's line does not end \"The upscaler keeps running\"", w);
        if (strchr(t, '\n')) Err("WN5: reason %d's line is more than one line", w);
        for (int k = FGX_WHY_BRIDGE; k < w; k++) if (seen[k] && !strcmp(seen[k], t)) Err("WN5: reasons %d and %d say the same line", k, w);
        seen[w] = t;
    }
    if (FgxWineWarnText(FGX_WHY_OK)) Err("WN5: no reason has a line");
    if (!WnHasWe("frame generation: off because we could not") || WnHasWe("the upscaler keeps running, user") || WnHasWe("ours?") == false)
        Err("WN5 control: the we/us/our check does not find a planted word, or finds one in \"user\"");
    else Ctl("the check finds a planted \"we\" and \"ours\" and passes \"user\": it can see the fault it is for");
    CaseEnd(name);
}
