// THE BENCH (tools/test-refresh-guard/gen.py appends this to the guard cut from source/DLSS-IV.cpp).
// A synthetic render thread: each frame costs some time from its release to its present; the present then returns at once
// (the driver does not wait: NVIDIA's layered path with the engine renamed) or as a FIFO of some depth returns it (vsync
// waits: a present finds a free slot or waits for the next refresh, which shows one queued frame). The guard sees only the
// return time, as it does in the game, and a hold moves the clock to the time it returned.
static const LONGLONG Q = 10000000;   // ticks a second: a typical QueryPerformanceFrequency
static int gFail = 0, gPass = 0;
static void Check(bool ok, const char* name, const char* fmt = "", ...)
{
    char b[400] = ""; va_list ap; va_start(ap, fmt); _vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, ap); va_end(ap);
    printf("%s  %s%s%s\n", ok ? "PASS" : "FAIL", name, b[0] ? ": " : "", b);
    if (ok) gPass++; else gFail++;
}
struct Display
{
    double hz = 0.0;     // 0: the driver does not wait
    int    depth = 3;    // presented and not yet shown, at most
    long long nextVb = 1;
    int    queued = 0;
    LONGLONG Vb(long long k) const { return (LONGLONG)((double)k * (double)Q / hz); }
    LONGLONG Present(LONGLONG t)
    {
        if (hz <= 0.0) return t;
        while (Vb(nextVb) <= t) { if (queued) queued--; nextVb++; }   // every refresh up to now shows one queued frame
        if (queued < depth) { queued++; return t; }
        const LONGLONG at = Vb(nextVb); nextVb++;                    // full: wait for the next refresh, which frees a slot
        return at;                                                    // (one shown, this one queued: the count is unchanged)
    }
};
struct Out
{
    std::vector<LONGLONG> rel, hold;
    bool everEngaged = false, holdInPast = false;
    int engagedAt = -1, releasedAt = -1;
    LONGLONG maxHold = 0;
};
// THE GAME WORLD: how many injections land before each present (1 in steady play; 0 in a menu or on a loading screen;
// 0 and 2 in turn when the injection body runs on its own schedule). countAll stands for a session with no transport.
static int OnePerPresent(int) { return 1; }
static LONG gInj = 0;
template<class F, class L = int (*)(int)> static Out Sim(RgState& s, Display& d, int n, F cost, LONGLONG& t, bool guard = true, L inj = OnePerPresent, bool countAll = false)
{
    Out o; bool was = s.engaged;
    for (int i = 0; i < n; i++)
    {
        t += cost(i);
        t = d.Present(t);
        LONGLONG h = 0;
        if (guard)
        {
            gInj += inj(i);
            const LONGLONG u = RgStep(s, t, gInj, countAll);
            if (u) { if (u <= t) o.holdInPast = true; h = u - t; t = u; }
        }
        o.rel.push_back(t); o.hold.push_back(h);
        if (h > o.maxHold) o.maxHold = h;
        if (s.engaged) o.everEngaged = true;
        if (s.engaged && !was && o.engagedAt < 0) o.engagedAt = i;
        if (!s.engaged && was && o.releasedAt < 0) o.releasedAt = i;
        was = s.engaged;
    }
    return o;
}
static LONGLONG MinInterval(const Out& o, int from)
{
    LONGLONG m = LLONG_MAX;
    for (size_t i = (size_t)from + 1; i < o.rel.size(); i++) { const LONGLONG d = o.rel[i] - o.rel[i - 1]; if (d < m) m = d; }
    return m;
}
static double MeanFps(const Out& o, int from)
{
    if ((int)o.rel.size() < from + 2) return 0.0;
    return (double)(o.rel.size() - 1 - (size_t)from) * (double)Q / (double)(o.rel.back() - o.rel[(size_t)from]);
}
static LONGLONG Ms(double ms) { return (LONGLONG)(ms * (double)Q / 1000.0 + 0.5); }
static RgState Armed(UINT32 num, UINT32 den) { RgState s{}; RgSetRefresh(s, num, den, Q); return s; }

// Presents that vsync holds, with a hitch every 200 frames (the queue drains, then refills at once): the guard must never
// engage and never hold one.
static void VsyncHeld(const char* name, UINT32 num, UINT32 den, double displayHz, int depth)
{
    RgState s = Armed(num, den); Display d; d.hz = displayHz; d.depth = depth; LONGLONG t = Q / 10;
    const Out o = Sim(s, d, 20000, [](int i) { return i % 200 == 199 ? Ms(30.0) : Ms(3.0); }, t);
    // PROOF THE CASE IS THE ONE NAMED: the refills happened (presents let through at once, under half a refresh apart), and
    // the tightest run of RG_WIN presents, in refreshes, against the engage line of RG_WIN - RG_SLACK.
    const double per = (double)Q / displayHz;
    int burst = 0; for (size_t i = 1; i < o.rel.size(); i++) if ((double)(o.rel[i] - o.rel[i - 1]) < per / 2) burst++;
    double tight = 1e9; for (size_t i = RG_WIN; i < o.rel.size(); i++) { const double r = (double)(o.rel[i] - o.rel[i - RG_WIN]) / per; if (r < tight) tight = r; }
    Check(!o.everEngaged && o.maxHold == 0 && (depth == 1 || (burst > 0 && tight < RG_WIN)), name,   // a queue of 1 has nothing to refill
          "%s over 20000 presents at %.2f fps, a 30 ms hitch every 200, queue depth %d; %d presents let through at once, the tightest"
          " %d presents took %.2f refreshes (the line is %d)", o.everEngaged ? "ENGAGED" : "never engaged, never held", MeanFps(o, 0), depth,
          burst, (int)RG_WIN, tight, (int)(RG_WIN - RG_SLACK));
}

int main(int argc, char** argv)
{
    gVerbose = argc > 1 && !strcmp(argv[1], "-v");
    const LONGLONG cap120 = Armed(120, 1).capTicks;

    // ---- the cap: NVIDIA Reflex's formula
    {
        const double f120 = (double)Q / (double)Armed(120, 1).capTicks, f144 = (double)Q / (double)Armed(144, 1).capTicks;
        const double f60 = (double)Q / (double)Armed(60, 1).capTicks, f11988 = (double)Q / (double)Armed(120000, 1001).capTicks;
        Check(fabs(f120 - 116.0) < 0.01 && fabs(f144 - 138.24) < 0.01 && fabs(f60 - 59.0) < 0.01 && fabs(f11988 - 115.888) < 0.01,
              "the cap is hz - hz*hz/3600", "120 Hz %.3f, 144 Hz %.3f, 60 Hz %.3f, 119.88 Hz %.3f fps", f120, f144, f60, f11988);
    }

    // ---- where vsync waits, nothing happens
    VsyncHeld("vsync holds at 120 Hz", 120, 1, 120.0, 3);
    VsyncHeld("vsync holds at 120 Hz, a queue of 5 (DXVK's worst, about)", 120, 1, 120.0, 5);
    VsyncHeld("vsync holds at 120 Hz, a queue of 7 (the margin)", 120, 1, 120.0, 7);
    VsyncHeld("vsync holds at 119.88 Hz, the rational", 120000, 1001, 120000.0 / 1001.0, 3);
    VsyncHeld("vsync holds at 59.94 Hz, the rational", 60000, 1001, 60000.0 / 1001.0, 3);
    VsyncHeld("vsync holds at 144 Hz, the mod's queue of 1", 144, 1, 144.0, 1);
    {   // THE CONTROL for the five above: the same check can see an engagement. A display 4 percent faster than the refresh the
        // guard was armed for is presents outrunning it, which is what engaging means.
        RgState s = Armed(120, 1); Display d; d.hz = 125.0; d.depth = 3; LONGLONG t = Q / 10;
        const Out o = Sim(s, d, 20000, [](int i) { return i % 200 == 199 ? Ms(30.0) : Ms(3.0); }, t);
        Check(o.everEngaged, "CONTROL: presents held at 125 Hz against a guard armed for 120 engage it", "%s", o.everEngaged ? "engaged" : "NOT engaged");
    }

    // ---- presents that outrun the refresh are held to the cap
    RgState s = Armed(120, 1); Display free; LONGLONG t = Q / 10;
    const Out a = Sim(s, free, 3000, [](int) { return Ms(5.0); }, t);
    Check(a.engagedAt == RG_WIN, "200 fps at 120 Hz engages after one window", "engaged at present %d (the window is %d)", a.engagedAt, (int)RG_WIN);
    Check(!a.holdInPast && MinInterval(a, a.engagedAt + 1) >= cap120, "engaged, no two presents closer than the cap",
          "shortest %.4f ms, the cap %.4f ms", MinInterval(a, a.engagedAt + 1) * 1000.0 / Q, cap120 * 1000.0 / Q);
    Check(fabs(MeanFps(a, a.engagedAt + 1) - 116.0) < 0.05, "engaged, held to 116 fps", "%.3f fps", MeanFps(a, a.engagedAt + 1));
    {   // THE CONTROL for the spacing check: the same run with no guard has presents closer than the cap
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(5.0); }, t2, false);
        Check(MinInterval(o, 0) < cap120, "CONTROL: without the guard the same run breaks the spacing check", "shortest %.4f ms", MinInterval(o, 0) * 1000.0 / Q);
    }
    const Out b = Sim(s, free, 3000, [](int) { return Ms(12.0); }, t);
    Check(b.releasedAt >= 0 && b.releasedAt < 2 * RG_WIN, "slowing to 83 fps releases within two windows", "released at present %d", b.releasedAt);
    {
        LONGLONG held = 0; for (size_t i = (size_t)(b.releasedAt >= 0 ? b.releasedAt : 0); i < b.hold.size(); i++) held += b.hold[i];
        Check(b.releasedAt >= 0 && held == 0 && fabs(MeanFps(b, b.releasedAt + 1) - 1000.0 / 12.0) < 0.01, "released, nothing is held",
              "%.3f fps after the release, %.3f ms held", MeanFps(b, b.releasedAt >= 0 ? b.releasedAt + 1 : 0), held * 1000.0 / Q);
    }

    // ---- the band between the refresh and the engage line
    {
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 6000, [](int) { return Q / 118; }, t2);
        Check(!o.everEngaged, "118 fps at 120 Hz stays dormant (under the refresh)", "%s", o.everEngaged ? "ENGAGED" : "dormant");
        RgState w = Armed(120, 1); Display f3; LONGLONG t3 = Q / 10;
        const Out p = Sim(w, f3, 6000, [](int) { return Q / 126; }, t3);
        Check(p.everEngaged && fabs(MeanFps(p, p.engagedAt + 1) - 116.0) < 0.05, "126 fps at 120 Hz engages and is held to 116",
              "%s, %.3f fps after", p.everEngaged ? "engaged" : "NOT engaged", MeanFps(p, p.engagedAt >= 0 ? p.engagedAt + 1 : 0));
    }

    // ---- jitter
    {
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 6000, [](int i) { return (i & 1) ? Ms(15.0) : Ms(5.0); }, t2);
        Check(!o.everEngaged, "5 and 15 ms frames in turn (100 fps) stay dormant", "%s", o.everEngaged ? "ENGAGED" : "dormant");
        RgState w = Armed(120, 1); Display f3; LONGLONG t3 = Q / 10;
        Sim(w, f3, 600, [](int) { return Ms(5.0); }, t3);
        const Out p = Sim(w, f3, 6000, [](int i) { return (i & 1) ? Ms(15.0) : Ms(5.0); }, t3);
        Check(w.engaged && p.releasedAt < 0 && MinInterval(p, 0) >= cap120, "engaged, 5 and 15 ms in turn stay held: no pair closer than the cap",
              "%s, shortest %.4f ms", w.engaged ? "engaged" : "RELEASED", MinInterval(p, 0) * 1000.0 / Q);
    }

    // ---- a pause while engaged: the late present goes at once, the next one a cap after it, no burst
    {
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        Sim(u, f2, 600, [](int) { return Ms(5.0); }, t2);
        const Out o = Sim(u, f2, 60, [](int i) { return i == 10 ? Ms(2000.0) : Ms(5.0); }, t2);
        const LONGLONG after = o.rel[11] - o.rel[10];
        Check(o.hold[10] == 0 && after == cap120 && MinInterval(o, 0) >= cap120, "a 2 s pause: the late present goes at once, the next one cap after it",
              "the late one held %.3f ms, the next %.4f ms after it", o.hold[10] * 1000.0 / Q, after * 1000.0 / Q);
    }

    // ---- a low refresh: every hold within the 20 ms bound
    {
        RgState u = Armed(30, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(5.0); }, t2);
        Check(o.everEngaged && o.maxHold <= Q / 50 && o.maxHold > 0, "30 Hz: engaged, and no hold is longer than 20 ms",
              "%s, the longest hold %.3f ms", o.everEngaged ? "engaged" : "NOT engaged", o.maxHold * 1000.0 / Q);
    }

    // ---- disarmed
    {
        RgState u = Armed(0, 0); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(2.0); }, t2);
        Check(!o.everEngaged && o.maxHold == 0 && Armed(10, 1).capTicks == 0 && Armed(2000, 1).capTicks == 0 && Armed(120, 0).capTicks == 0,
              "disarmed: no refresh, 10 Hz, 2000 Hz, a zero denominator", "%s", o.everEngaged ? "ENGAGED" : "never engaged");
    }

    // ---- the log: the first engage at log=1, every later engage and release at log=2, and a re-arm keeps the count
    {
        gRecords = 0; gHealths = 0;
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        for (int k = 0; k < 5; k++) Sim(u, f2, 600, [k](int) { return (k & 1) ? Ms(12.0) : Ms(5.0); }, t2);
        const int r1 = gRecords, h1 = gHealths;
        RgSetRefresh(u, 120, 1, Q);
        Sim(u, f2, 600, [](int) { return Ms(12.0); }, t2);
        Sim(u, f2, 600, [](int) { return Ms(5.0); }, t2);
        Check(r1 == 1 && h1 == 4 && gRecords == 1 && gHealths == 5 && u.engaged, "one log=1 line a session, the rest at log=2, kept across a re-arm",
              "three engages and two releases wrote %d and %d; after a re-arm and one more engage, %d and %d", r1, h1, gRecords, gHealths);
    }

    // ---- ONLY THE GAME WORLD IS JUDGED (2026-09-27, the two N3 runs): a loading screen the driver does not wait for, then a
    // world vsync holds, must leave the guard dormant; a world that outruns the refresh must still engage it.
    {
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out a1 = Sim(u, f2, 3000, [](int) { return Ms(1.3); }, t2, true, [](int) { return 0; });   // a loading screen at 770 fps, no injections
        Display v; v.hz = 120.0; v.depth = 3; v.nextVb = t2 * 120 / Q + 1;
        const Out a2 = Sim(u, v, 6000, [](int i) { return i % 200 == 199 ? Ms(30.0) : Ms(3.0); }, t2);   // then the world, held by vsync
        Check(!a1.everEngaged && !a2.everEngaged && a2.maxHold == 0, "a free loading screen, then a vsync-held world: never engaged",
              "%s on the loading screen, %s in the world", a1.everEngaged ? "ENGAGED" : "dormant", a2.everEngaged ? "ENGAGED" : "dormant");
    }
    {   // THE CONTROL: the same loading screen counted as the world (no transport) engages it, which is the 2026-09-26 behaviour
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(1.3); }, t2, true, [](int) { return 0; }, true);
        Check(o.everEngaged, "CONTROL: the same loading screen, counted as the world, engages it", "%s", o.everEngaged ? "engaged" : "NOT engaged");
    }
    {
        RgState u = Armed(60, 1); Display f2; LONGLONG t2 = Q / 10;
        Sim(u, f2, 1000, [](int) { return Ms(1.3); }, t2, true, [](int) { return 0; });
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(6.6); }, t2);   // then a world that runs free at 150 fps (E6)
        Check(o.engagedAt >= 0 && o.engagedAt <= 2 * RG_WIN, "a free loading screen, then a free world: engaged within two windows of the world",
              "engaged at world present %d (two windows are %d; the window across the step is not judged)", o.engagedAt, 2 * (int)RG_WIN);
    }
    {   // SITTING 7'S N8: injections land 0 and 2 in turn between presents; the world must engage one window in, not 23 s later
        RgState u = Armed(60, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(6.4); }, t2, true, [](int i) { return (i & 1) ? 2 : 0; });
        Check(o.engagedAt == RG_WIN, "injections 0 and 2 in turn (their own schedule): engaged one window in", "engaged at present %d (the window is %d)",
              o.engagedAt, (int)RG_WIN);
    }
    {   // a world where one frame in ten has no injection (above the line) engages; one in two (below it) does not
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        const Out o = Sim(u, f2, 3000, [](int) { return Ms(5.0); }, t2, true, [](int i) { return (i % 10) ? 1 : 0; });
        RgState w = Armed(120, 1); Display f3; LONGLONG t3 = Q / 10;
        const Out p = Sim(w, f3, 3000, [](int) { return Ms(5.0); }, t3, true, [](int i) { return i & 1; });
        Check(o.engagedAt == RG_WIN && !p.everEngaged, "nine frames in ten injected engages; one in two does not (the line is nine in ten)",
              "nine in ten engaged at %d; one in two %s", o.engagedAt, p.everEngaged ? "ENGAGED" : "never engaged");
    }

    // ---- THE KNOWN LIMIT, pinned so that changing it is a decision: engaged, then the driver starts to wait (a window
    // composited again). The frames arrive at the refresh, faster than the cap, so the guard keeps holding them.
    {
        RgState u = Armed(120, 1); Display f2; LONGLONG t2 = Q / 10;
        Sim(u, f2, 600, [](int) { return Ms(5.0); }, t2);
        Display v; v.hz = 120.0; v.depth = 3; v.nextVb = t2 * 120 / Q + 1;
        const Out o = Sim(u, v, 3000, [](int) { return Ms(3.0); }, t2);
        Check(u.engaged && o.releasedAt < 0, "KNOWN LIMIT: engaged, then vsync starts to wait: it stays engaged",
              "%s, %.2f fps", u.engaged ? "engaged" : "released", MeanFps(o, 0));
    }

    printf("\n%d passed, %d failed\n", gPass, gFail);
    return gFail;
}
