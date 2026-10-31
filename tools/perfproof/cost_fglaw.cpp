// cost_fglaw.cpp: WHAT THE PACING LAW COSTS THE RENDER THREAD A PRESENT, MEASURED (round 9, 2026-10-27; the owner's standing rule:
// "every item built as optimized as possible, cost measured").
//
// FgPace runs one step of the law a present (fg_machine_render.h, PaceLaw: FgLawStepForm on the merged path). This program times
// that step, and beside it the step of the law it replaces (A5's, FgPace's steps 2 and 3 as they stood to round 9: the running
// mean, the slowly forgotten worst, the target and the bounded move), on the same sample streams, each step on its own:
//   - the owner's recorded F1C-5a of 2026-10-05 (streams/5a-2026-10-05-free-us.txt, the pacer's own samples) at the 120 Hz floor;
//   - seeded streams for the panels the recorded run is not: a 60 Hz panel's floor with a game at 57, a 144 Hz, a 240 Hz and a
//     360 Hz panel's with a game near each floor (the rings hold 4 s and 12 s of samples, so the faster the real rate the longer
//     they are), and THE BOUND: a stream that keeps the 12 s ring at its cap of 4096 (no panel reaches it: 341 real frames a
//     second for 12 s).
// The time each sample is given is the pacer's own (the oracle's loop: arrival = release + cost, a sleep to the due).
// TWO WAYS, each step timed by the time stamp counter, calibrated against QueryPerformanceCounter:
//   warm   the steps back to back, the law's state in the cache (the bench's best case);
//   cold   every sixteenth step, 192 MB written before it (more than the 96 MB last-level cache of the owner's 7900X3D: a game
//          frame's worth of other memory), so the law's rings are read from memory as they are in the game (the case that
//          matters).
// Prints the median and the 99th percentile of each, in microseconds, and the law's ring lengths at the end. Exit 0.
//
//   python run_proof.py cost_fglaw
//   python run_proof.py cost_fglaw -DCOST_FGLAW_OTHER -I<folder>     the same over another fg_law.h in <folder> (an A/B)
// THREE WAYS SINCE THE A/B OF ROUND 9's PUSH: warm, L2 (8 MB written before a timed step: the core's own caches gone, the
// last-level cache kept, which is closest to the owner's 96 MB 7900X3D in play) and cold (192 MB: from memory, the bound).
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <random>
#ifdef COST_FGLAW_OTHER
#include <fg_law.h>                                  // (another fg_law.h, by -I: the A/B of a change to the law's rings, e.g. the base's)
#else
#include "../../source/fg/fg_law.h"
#endif

static FgLaw gLaw;                                   // 123 KB: not on the stack
static volatile double gSink;

// A5'S STEPS 2 AND 3 (fg_machine_render.h, Pace, to round 9), line for line on doubles of its own; the pad 0, the room floor off.
struct A5 { double avg, worst, period, prevPeriod; int dwell, warmLeft; long moves; };
static __declspec(noinline) double A5Step(A5& r, double freeMs, double floorMs)
{
    if (freeMs > 0.2 && freeMs < 100.0)
    {
        r.avg += (freeMs - r.avg) * 0.0625;
        if (freeMs > r.worst) r.worst = freeMs;
        else r.worst += (r.avg - r.worst) * 0.02;
    }
    double want = r.worst * 1.06;
    if (want < floorMs) want = floorMs;
    if (want > 40.0) want = 40.0;
    const double band = r.period * 0.03 > 0.35 ? r.period * 0.03 : 0.35;
    const int dwellNeed = (want > r.period) ? 6 : 60;
    if (r.warmLeft == 0 && ++r.dwell >= dwellNeed && fabs(want - r.period) > band)
    {
        double d = want - r.period;
        if (d > 1.5) d = 1.5;
        if (d < -0.4) d = -0.4;
        r.prevPeriod = r.period; r.period += d; r.dwell = 0; r.moves++;
    }
    if (r.warmLeft > 0) r.warmLeft--;
    return r.period;
}
static __declspec(noinline) double LawStep(double c, double t) { return FgLawStepForm(gLaw, c, t); }

static double gTicksPerUs = 0.0;
static void Calibrate()
{
    LARGE_INTEGER f, a, b; QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&a); const unsigned long long ta = __rdtsc();
    do QueryPerformanceCounter(&b); while ((b.QuadPart - a.QuadPart) * 1000 < f.QuadPart * 300);   // 300 ms
    const unsigned long long tb = __rdtsc();
    gTicksPerUs = (double)(tb - ta) / ((double)(b.QuadPart - a.QuadPart) * 1e6 / (double)f.QuadPart);
}
static std::vector<unsigned char> gTrash(192u << 20);
static void Trash(size_t bytes) { for (size_t i = 0; i < bytes && i < gTrash.size(); i += 64) gTrash[i] = (unsigned char)(gTrash[i] + 1); }

struct Res { double med, p99, max; };
static Res Stat(std::vector<double>& v)
{
    std::sort(v.begin(), v.end());
    Res r; r.med = v[v.size() / 2]; r.p99 = v[(size_t)(v.size() * 0.99)]; r.max = v.back(); return r;
}

// The pacer's loop around a step (fglaw_oracle.py, loop): the times the law is given. kind 0 the law, 1 A5.
static void Run(const char* name, const std::vector<double>& cost, double floorMs, size_t trash, Res* law, Res* a5, int* nFast, int* nSlow)
{
    const bool cold = trash > 0;
    for (int kind = 0; kind < 2; kind++)
    {
        FgLawReset(gLaw, floorMs, 0.0, 0.0, 1.0);
        A5 r = {}; r.period = floorMs; r.prevPeriod = floorMs; r.warmLeft = 150;
        std::vector<double> us; us.reserve(cost.size());
        double t = 0.0, due = -1.0, P = floorMs;
        for (size_t i = 0; i < cost.size(); i++)
        {
            const double c = cost[i];
            double now = t + c;
            const bool timed = i >= 300 && (!cold || i % 16 == 0);          // (behind the warm-up's 150 steps and the rings' first fill)
            if (cold && timed) Trash(trash);
            const unsigned long long t0 = __rdtsc();
            P = kind == 0 ? LawStep(c, now) : A5Step(r, c, floorMs);
            const unsigned long long t1 = __rdtsc();
            if (timed) us.push_back((double)(t1 - t0) / gTicksPerUs);
            if (due < 0.0 || now > due + (4.0 * P < 120.0 ? 4.0 * P : 120.0)) due = now;
            else if (now > due) due = now;
            else if (due - now > 0.05) { if (due - now > 20.0) due = now + 20.0; now = due; }
            due += P; t = now;
        }
        gSink = P;
        if (kind == 0) { *law = Stat(us); *nFast = gLaw.fast.n; *nSlow = gLaw.slow.n; } else *a5 = Stat(us);
    }
    (void)name;
}

static std::vector<double> Recorded()
{
    std::vector<double> v;
    FILE* f = fopen("streams/5a-2026-10-05-free-us.txt", "rb");
    if (!f) return v;
    char ln[4096];
    while (fgets(ln, sizeof(ln), f))
    {
        if (ln[0] == '#') continue;
        char* p = ln; char* e;
        for (;;) { const long x = strtol(p, &e, 10); if (e == p) break; v.push_back(x > 0 ? x / 1000.0 : 0.0); p = e; }
    }
    fclose(f);
    return v;
}
static std::vector<double> Gauss(unsigned seed, size_t n, double mean, double sd, double lo)
{
    std::mt19937 g(seed); std::normal_distribution<double> d(mean, sd);
    std::vector<double> v(n); for (auto& x : v) { x = d(g); if (x < lo) x = lo; } return v;
}

int main()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetThreadAffinityMask(GetCurrentThread(), 1u << 2);
    Calibrate();
    printf("cost_fglaw: the pacing law's step against A5's, a present, on the render thread's terms (TSC %.1f ticks a us)\n", gTicksPerUs);
    struct S { const char* name; std::vector<double> cost; double floor; };
    std::vector<S> streams;
    std::vector<double> rec = Recorded();
    if (rec.size() != 16799) { printf("COST_FGLAW FAIL: the recorded stream holds %u values, 16799 wanted\n", (unsigned)rec.size()); return 2; }
    const double hz60 = 2.0 * 1.015 * 1000.0 / 60.0, hz120 = 2.0 * 1.015 * 1000.0 / 120.0, hz144 = 2.0 * 1.015 * 1000.0 / 144.0,
                 hz240 = 2.0 * 1.015 * 1000.0 / 240.0, hz360 = 2.0 * 1.015 * 1000.0 / 360.0;
    streams.push_back({ "F1C-5a, recorded, VRR 120 (floor 16.92)", rec, hz120 });
    streams.push_back({ "fixed 60 (floor 33.83), a game of 17.5 ms", Gauss(1, 12000, 17.5, 1.2, 0.3), hz60 });
    streams.push_back({ "VRR 120, a game at 55 (18.2 ms)", Gauss(2, 16000, 18.2, 1.0, 0.3), hz120 });
    streams.push_back({ "144 Hz (floor 14.10), a game of 13 ms", Gauss(3, 20000, 13.0, 0.9, 0.3), hz144 });
    streams.push_back({ "240 Hz (floor 8.46), a game of 7.5 ms", Gauss(4, 30000, 7.5, 0.6, 0.3), hz240 });
    streams.push_back({ "360 Hz (floor 5.64), a game of 5 ms", Gauss(5, 40000, 5.0, 0.4, 0.3), hz360 });
    streams.push_back({ "THE BOUND: the 12 s ring at its cap (2.9 ms)", Gauss(6, 40000, 2.9, 0.05, 0.3), 2.93 });
    printf("  %-48s %5s %5s | %-24s | %-24s | %-24s\n", "stream", "4 s", "12 s", "warm: law / A5, med p99", "L2: law / A5, med p99", "cold: law / A5, med p99 (us)");
    for (auto& s : streams)
    {
        Res lw, aw, l2, a2, lc, ac; int nf = 0, ns = 0;
        Run(s.name, s.cost, s.floor, 0, &lw, &aw, &nf, &ns);
        Run(s.name, s.cost, s.floor, 8u << 20, &l2, &a2, &nf, &ns);
        Run(s.name, s.cost, s.floor, gTrash.size(), &lc, &ac, &nf, &ns);
        printf("  %-48s %5d %5d | %5.2f %5.2f / %4.2f %4.2f | %5.2f %5.2f / %4.2f %4.2f | %5.2f %5.2f / %4.2f %4.2f   (cold max: law %.2f, A5 %.2f)\n",
               s.name, nf, ns, lw.med, lw.p99, aw.med, aw.p99, l2.med, l2.p99, a2.med, a2.p99, lc.med, lc.p99, ac.med, ac.p99, lc.max, ac.max);
    }
    printf("COST_FGLAW DONE\n");
    return 0;
}
