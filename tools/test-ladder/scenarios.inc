// THE SCENARIOS. A Machine says what each rung costs and wins; Window() feeds the controller one window of what the
// machine does on the rung the controller chose, so the loop is closed exactly as in the game: the controller flips,
// the machine answers, the controller reads the answer. Periods in ms, ages in frames, in flight as a mean (a window's
// own figure is a median over 120 frames and reads 0 or 1, so a mean is emulated by alternating windows).
struct Machine
{
    float p0;                          // rung 0 (the queue alone, pipelined) period
    float fSf;                         // rung 1 period as a factor of rung 0
    float fPaceYoung, fPaceSteady;     // rung 2 period as a factor of rung 1, right after the pacer's release and 25 windows later
    float q0, q1, q2;                  // mean frames in flight on each rung
    float a0, a1, a2;                  // picture age in frames on each rung
    float noise;                       // +- fraction on the period, a fixed pattern
};
static float Noise(int w, float amp) { static const float pat[7] = { 0.0f, 0.6f, -0.4f, 1.0f, -0.9f, 0.3f, -0.6f }; return 1.0f + amp * pat[w % 7]; }
static int gW = 0, gLastRung = 0;
static void ResetLadder()
{
    adState = AD_STAY0; adCount = 55; adRung = 0; adFlips = 0; adMeasured = 0; adPairs = AD_PAIRS_MIN; adOut = 0; adAudits = 0;
    adInit = true; adFirst = true; adLastN = 0; adBase = 0; adCostSf = adCostPace = adWinSf = adWinPace = adQ1 = 0; adKeptSf = adKeptPace = false;
    adHeld = false; adHeldUp = 0;
    latAsyncWant = 1; latPaceHold = 1; latLimiter = latVsync = false; gLog.clear(); gPaceReleases = 0; gSincePaceRelease = 0; gLastRung = 0;
}
static void Window(const Machine& m)
{
    // THE TRANSITION (audit F6): the controller runs at the end of a window, so the window after a flip ran on the OLD rung.
    const int now = adRung, rung = (now != gLastRung) ? gLastRung : now; gLastRung = now;
    float period = m.p0, q = m.q0, age = m.a0;
    if (rung >= 1) { period = m.p0 * m.fSf; q = m.q1; age = m.a1; }
    if (rung == 2)
    {
        const float ramp = gSincePaceRelease >= 25 ? 1.0f : gSincePaceRelease / 25.0f;
        period = m.p0 * m.fSf * (m.fPaceYoung + (m.fPaceSteady - m.fPaceYoung) * ramp); q = m.q2; age = m.a2; gSincePaceRelease++;
    }
    const float n = Noise(gW, m.noise);
    const float qWin = q >= 0.999f ? 1.0f : (q <= 0.001f ? 0.0f : (((gW % 4) < (int)(q * 4.0f + 0.5f)) ? 1.0f : 0.0f));
    LatAdaptiveStep(period * n, qWin, age * period * n);
    gW++;
}
static void Run(const Machine& m, int windows) { for (int i = 0; i < windows; i++) Window(m); }
static bool Has(const char* s) { return gLog.find(s) != std::string::npos; }
static int  Count(const char* s) { int c = 0; size_t p = 0; while ((p = gLog.find(s, p)) != std::string::npos) { c++; p += strlen(s); } return c; }
static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "ok  " : "FAIL", what); if (!ok) fails++; }
static const char* StateName(int s) { return AdName(s); }
static void Story() { if (gVerbose) return; size_t p = 0; while ((p = gLog.find("latency: ladder", p)) != std::string::npos) { size_t e = gLog.find('\n', p); std::string l = gLog.substr(p, e - p); if (l.find("-> rung") == std::string::npos) printf("      | %s\n", l.c_str()); p = e; } }

int main(int argc, char** argv)
{
    gVerbose = argc > 1 && strcmp(argv[1], "-v") == 0;

    printf("S1  a machine whose period does not answer to the rung at all, nothing queued (a held frame rate, or a CPU-bound machine standing still)\n");
    { ResetLadder(); gW = 0; Machine m = { 7.6f, 1.00f, 1.09f, 1.09f, 0, 0, 0, 2.45f, 1.2f, 1.1f, 0 };
      Run(m, 400); Story();
      Check(Has("the first measurement, five live windows in"), "the first measurement five live windows in");
      Check(Has("kept; queue empty"), "the same frame kept, the queue found empty");
      Check(!Has("measures the pacer"), "the pacer stage never opened");
      Check(adState == AD_STAY1 && adRung == 1 && latAsyncWant == 0 && latPaceHold == 1, "resting on rung 1: same frame on, pacer held");
      Check(Has("the frame rate is HELD"), "every pair flat with nothing in flight: the rate is known as held, and said");
      Check(Count("measures the same frame") == 1, "NOT re-measured at the 300-window check: the clock stands while the rate holds (it was once in 400 windows until 2026-10-11)");
      Check(gPaceReleases == 0, "the pacer was never released"); }

    printf("S2  a GPU-bound machine (a 3070 at 4K): the same frame costs 1 percent, the pacer 1 and wins most of a frame\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.01f, 1.005f, 1.01f, 1, 1, 0, 3.0f, 2.0f, 1.2f, 0 };
      Run(m, 760); Story();
      Check(Has("the pacer next"), "the same frame kept and the queue seen in use");
      Check(Has("kept for 300 windows, audited every 90"), "the pacer measured and kept");
      Check(adAudits >= 4, "audited at least four times in 760 windows (90-window cadence, counts restored after each)");
      Check(Count("in flight without it 1.0: kept") == adAudits, "every audit kept it (period, win and queue all in order)");
      Check(adState == AD_STAY2 && adRung == 2 && latAsyncWant == 0 && latPaceHold == 0, "resting on rung 2: same frame and pacer running");
      Check(Count("measures the same frame") == 3, "re-measured at each 300-window check, twice in 760 windows (each cycle: 36 measuring, 300 resting, 12 auditing)"); }

    printf("S3  a GPU-bound machine whose pacer ratchets: 1 percent young, 7 percent after 25 windows\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.01f, 1.01f, 1.07f, 1, 1, 0, 3.0f, 2.0f, 1.2f, 0 };
      while (adAudits < 1 && gW < 400) Window(m);
      Window(m); Window(m); Window(m); Window(m);   // the audit's four held windows
      Story();
      Check(Has("kept for 300 windows"), "the young pacer passed the stage (as it would in the game)");
      Check(Has("over the line now"), "the audit read the steady pacer over the line");
      Check(adState == AD_STAY1 && latPaceHold == 1, "dropped to rung 1 by the audit, the pacer held");
      { const int rel = gPaceReleases; Run(m, 60); Check(gPaceReleases == rel && latPaceHold == 1 && adState == AD_STAY1, "no release after the drop: the pacer stays held on rung 1 through the rest"); } }

    printf("S4  the borderline rule and the hysteresis: 3.5 percent is over on entry, 3.5 is kept once kept, 5 is dropped\n");
    { ResetLadder(); gW = 0; Machine m = { 8.6f, 1.035f, 1.05f, 1.05f, 0, 0, 0, 3.0f, 2.0f, 1.9f, 0 };
      while (!Has("over the 3 percent line") && gW < 200) Window(m);
      Check(Has("percent line: two more pairs"), "3.5 percent after three pairs took two more");
      Check(Has("(pairs +3.5 +3.5 +3.5 +3.5 +3.5), over the 3 percent line"), "five pairs at 3.5: over the 3 percent line on entry");
      Check(adState == AD_STAY0 && adRung == 0, "held on the queue alone");
      m.fSf = 1.02f;
      while (!Has("kept; queue empty") && gW < 400) Window(m);
      Check(Has("costs 2.0 percent (pairs") && adState == AD_STAY1, "2 percent a minute later: kept");
      m.fSf = 1.035f; int before = Count("costs ");
      while (Count("costs ") == before && gW < 900) Window(m);
      Check(Has(", line 4), wins") && adState == AD_STAY1, "3.5 percent on a kept rung: judged on the 4 percent line, kept");
      m.fSf = 1.05f; before = Count("costs ");
      while (Count("costs ") == before && gW < 1400) Window(m);
      Check(Has("over the 4 percent line") && adState == AD_STAY0, "5 percent on a kept rung: over the 4 percent line, dropped");
      Story(); }

    printf("S5  a frame limiter, then its removal\n");
    { ResetLadder(); gW = 0; Machine m = { 10.0f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 2.0f, 1.2f, 1.2f, 0 };
      latLimiter = true; Run(m, 30);
      Check(adState == AD_FORCED && latAsyncWant == 0 && latPaceHold == 1, "forced: same frame on, the pacer held (it stands down on its own rule)");
      latLimiter = false; Window(m);
      Check(Has("the limiter is gone") && adState == AD_MEASURE_SF, "the limiter gone: a measurement starts");
      Story(); }

    printf("S6  a scene change: one light window is ignored, three consecutive re-measure\n");
    { ResetLadder(); gW = 0; Machine m = { 7.6f, 1.00f, 1.09f, 1.09f, 0, 0, 0, 2.45f, 1.2f, 1.1f, 0 };
      while (adState != AD_STAY1 && gW < 200) Window(m);
      Run(m, 10);
      LatAdaptiveStep(7.6f * 0.5f, 0, 1.2f * 3.8f);
      Check(adState == AD_STAY1, "one light window: still resting");
      Run(m, 3);
      LatAdaptiveStep(7.6f * 0.5f, 0, 1.2f * 3.8f); LatAdaptiveStep(7.6f * 0.5f, 0, 1.2f * 3.8f);
      Check(adState == AD_STAY1, "two light windows: still resting");
      LatAdaptiveStep(7.6f * 0.5f, 0, 1.2f * 3.8f);
      Check(adState == AD_MEASURE_SF && Has("the scene got lighter for three windows"), "three consecutive: re-measuring");
      Story(); }

    printf("S7  a GPU-bound machine with 8 percent window-to-window noise (information: the design's tolerance, not a rule)\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.01f, 1.005f, 1.01f, 1, 1, 0, 3.0f, 2.0f, 1.2f, 0.08f };
      Run(m, 400); Story();
      printf("      ends on: %s, %d flips, %d audits\n", StateName(adState), adFlips, adAudits);
      Check(Has("), wins") && Count("over the 3 percent line") <= 1 && (adState == AD_STAY1 || adState == AD_STAY2 || adState == AD_AUDIT), "with an 8 percent rhythm the same frame is misread at most once and ends kept (the alternating order; the residual is documented; an audit in progress is a kept pacer)"); }

    printf("S9  a GPU-bound machine whose queue alternates (a mean of half a frame in flight): the audit must keep what the gate let in\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.01f, 1.005f, 1.01f, 0.5f, 0.5f, 0, 3.0f, 2.0f, 1.2f, 0 };
      Run(m, 500); Story();
      Check(Has("kept for 300 windows"), "the pacer stage opened on a mean of a half and kept the pacer");
      Check(adAudits >= 2 && Count("nothing left to convert") == 0, "every audit kept it: the audit's queue threshold matches the gate's"); }

    printf("S8  a GPU-bound machine whose pacer wins too little (a tenth of a frame): the benefit test\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.01f, 1.00f, 1.00f, 1, 1, 0, 3.0f, 2.0f, 1.9f, 0 };
      Run(m, 120); Story();
      Check(Has("under a quarter of a frame; no pacer"), "the pacer dropped for winning too little at no cost");
      Check(adState == AD_STAY1 && latPaceHold == 1, "resting on rung 1"); }

    printf("S10 the owner's 4090 with its clock locked (FLUSH 3 and 4): the same frame costs 0.7 percent and wins a frame, the pacer 5 percent on top and wins 1.37 frames: the line rises to 7.1 (3 plus 3 a frame won) and the pacer is kept\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.007f, 1.05f, 1.05f, 1, 1, 0, 3.0f, 2.0f, 0.6f, 0 };   // a2 is in frames of rung 2's own period: 0.6 x 1.05 = 0.63 of rung 1's, so the pacer wins 1.37
      Run(m, 400); while (adState != AD_STAY0 && adState != AD_STAY1 && adState != AD_STAY2 && gW < 480) Window(m); Story();   // at 400 the 300-window re-check is running: step to its verdict
      Check(Has("the pacer next"), "the same frame kept (0.7 percent for a frame) and the queue seen in use");
      Check(Has("(line 7.1), wins 1.37 frames: kept"), "the pacer at 5.7 percent in all, judged on the 7.1 percent line for its 1.37 frames: kept");
      Check(adAudits >= 1 && Count("over the line now") == 0 && Has(", line 8.1)"), "the audit judges the kept pacer on 8.1 (one point of hysteresis) and keeps it");
      Check(adState == AD_STAY2 && adRung == 2 && latPaceHold == 0, "resting on rung 2: same frame and pacer running"); }

    printf("S11 the same pacer cost where it wins only a third of a frame: the line stays near 3 (4.0), and the pacer is dropped\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.007f, 1.05f, 1.05f, 1, 1, 0, 3.0f, 2.0f, 1.6f, 0 };   // 1.6 x 1.05 = 1.68: the pacer wins 0.32 of a frame
      Run(m, 200); Story();
      Check(Has("(line 4.0), wins 0.32 frames: over the line"), "5.7 percent in all against a 4.0 line for a third of a frame: over the line");
      Check(adState == AD_STAY1 && latPaceHold == 1, "resting on rung 1, the pacer held"); }

    printf("S12 the cap: a pacer that wins three and a half frames may still cost no more than 9 percent (9.5 is dropped, 8.5 is kept)\n");
    { ResetLadder(); gW = 0; Machine m = { 16.0f, 1.00f, 1.095f, 1.095f, 1, 1, 0, 4.5f, 4.0f, 0.5f, 0 };
      Run(m, 200); Story();
      Check(Has("(line 9.0), wins 3.4") && Has("frames: over the line"), "9.5 percent for three and a half frames: over the 9 percent cap");
      ResetLadder(); gW = 0; m.fPaceYoung = m.fPaceSteady = 1.085f;
      Run(m, 200); Story();
      Check(Has("(line 9.0), wins 3.4") && Has("frames: kept for 300 windows"), "8.5 percent for three and a half frames: under the cap, kept"); }

    printf("S13 RTSS at 90 (the limiter sitting of 2026-10-11, leg B): 11.11 to 11.12 ms on either rung, nothing in flight, for 2000 windows; then the cap is taken off\n");
    { ResetLadder(); gW = 0; Machine m = { 11.115f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 2.76f, 1.95f, 1.95f, 0.0005f };
      Run(m, 2000); Story();
      Check(Has("the frame rate is HELD") && adHeld, "held, from the first measurement");
      Check(Count("measures the same frame") == 1 && adFlips == 3, "one measurement in 2000 windows (37 minutes at 90 frames a second), three switches in all: it would have been seven measurements");
      Check(adState == AD_STAY1 && adRung == 1 && latAsyncWant == 0 && latPaceHold == 1, "resting on rung 1: same frame on, the pacer held");
      m.p0 = 6.0f; Window(m); Window(m);
      Check(adHeld && adState == AD_STAY1, "two windows at 167 frames a second: still held");
      Window(m);
      Check(!adHeld && Has("not held any more") && adState == AD_MEASURE_SF && Has("the scene got lighter for three windows"), "three: not held any more, and re-measured in the same window (the rate rose by more than a quarter)"); }

    printf("S14 a free machine that stood still while it was measured, then moves on: three percent lighter\n");
    { ResetLadder(); gW = 0; Machine m = { 7.6f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 2.45f, 1.2f, 1.2f, 0 };
      Run(m, 100);
      Check(adHeld && Count("measures the same frame") == 1, "measured flat: taken for held");
      m.p0 = 7.6f * 0.97f; Run(m, 3);
      Check(!adHeld && Has("not held any more") && adState == AD_STAY1 && Count("measures the same frame") == 1, "three windows three percent lighter: let go, WITHOUT a measurement");
      { const int at = gW; while (Count("measures the same frame") == 1 && gW < at + 400) Window(m);
        Check(Has("the 300-window re-check") && gW - at >= 290 && gW - at <= 300, "and the usual re-check comes, within 300 windows of the letting go"); }
      Story(); }

    printf("S15 CONTROL: a free machine in the world, its period two percent up and down from window to window, nothing queued\n");
    { ResetLadder(); gW = 0; Machine m = { 7.6f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 2.45f, 1.2f, 1.2f, 0.02f };
      Run(m, 400); Story();
      Check(Has("no pacer for 300 windows") && !Has("HELD") && !adHeld, "the pairs are not flat: not held");
      Check(Count("measures the same frame") == 2 && Has("the 300-window re-check"), "re-measured at the 300-window check, as before"); }

    printf("S16 a held rate, then the scene gets forty percent heavier (the cap no longer binds): the band rule still speaks\n");
    { ResetLadder(); gW = 0; Machine m = { 11.115f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 2.76f, 1.95f, 1.95f, 0 };
      Run(m, 500);
      Check(adHeld && Count("measures the same frame") == 1, "held for 500 windows");
      m.p0 = 11.115f * 1.4f; Run(m, 3);
      Check(Has("the scene got heavier for three windows") && adState == AD_MEASURE_SF && !adHeld, "three heavy windows: re-measured, the mark cleared");
      Story(); }

    printf("S17 flat pairs with a frame in flight (a cap that leaves the queue full): NOT held, the pacer stage opens as before\n");
    { ResetLadder(); gW = 0; Machine m = { 10.0f, 1.0f, 1.0f, 1.0f, 1, 1, 0, 3.0f, 2.0f, 1.2f, 0 };
      Run(m, 60); Story();
      Check(Has("the pacer next") && !Has("HELD") && !adHeld, "a queue in use is not a held rate by this rule");
      ResetLadder(); gW = 0; latPacerOn = false; Run(m, 60); latPacerOn = true;
      Check(Has("kept; pacer=0") && Has("no pacer for 300 windows") && !Has("HELD") && !adHeld, "the same with pacer=0: kept, and not held either"); }

    printf("\n%s: %d check(s) failed\n", fails ? "FAILED" : "PASS", fails);
    return fails ? 1 : 0;
}
