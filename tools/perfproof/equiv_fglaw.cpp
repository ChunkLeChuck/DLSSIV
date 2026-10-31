// equiv_fglaw.cpp: THE PACING LAW (source/fg/fg_law.h) AGAINST ITS ORACLE (fglaw_oracle.py), step by step (2026-10-05).
//
// Built and run by equiv_fglaw.py, never alone: that script makes the streams, runs the oracle over them, and writes the
// samples, their times and the oracle's answers to a file in %TEMP% whose path it gives in a generated header. This
// program steps fg_law.h over the same samples and compares, at every step and exactly, what the law answers: the period,
// E, L, the regime and the hitch flag. Exactly, because both sides are the same IEEE double arithmetic in the same order
// (this proof builds 32-bit with /O2 and the default floating point, as build.ps1 builds the mod); a difference of one
// bit is a difference.
//
// It also checks what no oracle is needed for: the rings' own books after every stream (the sorted array is sorted, and
// holds the FIFO's samples), and FgLawFloor, the machine's join when the floor moves.
//
//   equiv_fglaw            the proof: 0 differences wanted (exit 0)
//   equiv_fglaw control    the answers on file are an oracle's with one constant changed: differences wanted (exit 1)
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// THE PUSH'S PATHS (round 9's LW: the steady state in one shift, fg_law.h's FgLawPush), each counted; every one must be taken
static long long gPath[5];
#define FG_LAW_PATH(k) (gPath[(k)]++)
#include "../../source/fg/fg_law.h"
#include "equiv_fglaw_gen.h"

static FgLaw gLaw;                                   // 123 KB: not on the stack

template <int CAP> static int RingBad(const FgLawRing<CAP>& r, const char* which, const char* stream)
{
    int bad = 0;
    if (r.n < 0 || r.n > CAP || r.head < 0 || r.head >= CAP) { printf("    BOOKS: %s, %s: n %d head %d\n", stream, which, r.n, r.head); return 1; }
    for (int i = 1; i < r.n; i++) if (r.s[i - 1] > r.s[i]) bad++;
    double a = 0.0, b = 0.0;
    for (int i = 0; i < r.n; i++) { a += r.s[i]; b += r.c[(r.head + i) % CAP]; }
    for (int i = 1; i < r.n; i++) if (r.t[(r.head + i) % CAP] < r.t[(r.head + i - 1) % CAP]) bad++;
    if (fabs(a - b) > 1e-6 * (1.0 + fabs(a))) bad++;
    if (bad) printf("    BOOKS: %s, %s: %d faults (n %d, sorted sum %.9f, FIFO sum %.9f)\n", stream, which, bad, r.n, a, b);
    return bad;
}

int main(int argc, char** argv)
{
    const bool control = argc > 1 && !strcmp(argv[1], "control");
    FILE* f = fopen(EQUIV_FGLAW_DATA, "rb");
    if (!f) { printf("EQUIV_FGLAW FAIL: cannot open %s\n", EQUIV_FGLAW_DATA); return 2; }
    char magic[8]; int nStreams = 0;
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, "FGLAW02", 8) || fread(&nStreams, 4, 1, f) != 1 || nStreams != EQUIV_FGLAW_STREAMS)
    { printf("EQUIV_FGLAW FAIL: %s is not this run's file (%d streams, %d wanted)\n", EQUIV_FGLAW_DATA, nStreams, EQUIV_FGLAW_STREAMS); return 2; }
    long long steps = 0, diffs = 0, moves = 0, hitches = 0; int books = 0, shown = 0, streamsBad = 0, form1 = 0;
    for (int si = 0; si < nStreams; si++)
    {
        int nameLen = 0, n = 0; char name[256] = {}; double hdr[5];
        if (fread(&nameLen, 4, 1, f) != 1 || nameLen <= 0 || nameLen > 255 || fread(name, 1, (size_t)nameLen, f) != (size_t)nameLen
            || fread(hdr, 8, 5, f) != 5 || fread(&n, 4, 1, f) != 1 || n <= 0 || (hdr[4] != 0.0 && hdr[4] != 1.0))
        { printf("EQUIV_FGLAW FAIL: stream %d's head is short\n", si); return 2; }
        double* v = (double*)malloc((size_t)n * 7 * sizeof(double));
        if (!v || fread(v, 8, (size_t)n * 7, f) != (size_t)n * 7) { printf("EQUIV_FGLAW FAIL: stream %d (%s) is short\n", si, name); return 2; }
        FgLawReset(gLaw, hdr[0], hdr[1], hdr[2], hdr[3]);
        const bool form = hdr[4] == 1.0;                          // (2026-10-17: the free band's form, FgLawStepForm, as the merged path runs it)
        form1 += form ? 1 : 0;
        long long d = 0; double prev = gLaw.per;
        for (int i = 0; i < n; i++)
        {
            const double* x = v + (size_t)i * 7;
            const double P = form ? FgLawStepForm(gLaw, x[0], x[1]) : FgLawStep(gLaw, x[0], x[1]);
            if (P != prev) moves++;
            prev = P;
            hitches += gLaw.hitch;
            const bool same = P == x[2] && gLaw.E == x[3] && gLaw.L == x[4] && (double)gLaw.regime == x[5] && (double)gLaw.hitch == x[6]
                           && P == gLaw.per && gLaw.regime == FgLawReg(gLaw, P);
            if (!same)
            {
                d++;
                if (shown < 12)
                {
                    shown++;
                    printf("    MISMATCH: %s, step %d (sample %.6f at %.3f): period %.9f (oracle %.9f), E %.9f (%.9f), L %.9f (%.9f), regime %d (%.0f), hitch %d (%.0f)\n",
                           name, i, x[0], x[1], P, x[2], gLaw.E, x[3], gLaw.L, x[4], gLaw.regime, x[5], gLaw.hitch, x[6]);
                }
            }
        }
        books += RingBad(gLaw.fast, "the 4 s ring", name) + RingBad(gLaw.slow, "the 12 s ring", name);
        if (d) streamsBad++;
        steps += n; diffs += d;
        free(v);
    }
    if (fgetc(f) != EOF) { printf("EQUIV_FGLAW FAIL: bytes after the last stream\n"); return 2; }
    fclose(f);
    if (steps != EQUIV_FGLAW_STEPS) { printf("EQUIV_FGLAW FAIL: %lld steps read, %lld written\n", steps, EQUIV_FGLAW_STEPS); return 2; }
    if (form1 * 3 != nStreams * 2) { printf("EQUIV_FGLAW FAIL: %d of %d streams in the free band's form, two thirds wanted\n", form1, nStreams); return 2; }

    // FgLawFloor, with no oracle: a period at the old floor follows the floor either way; a period off it stays unless the
    // new floor is over it; the edges are derived again from what was asked.
    int floorBad = 0;
    {
        FgLaw& w = gLaw;
        FgLawReset(w, 16.916, 17.4, 20.0, 1.06);
        FgLawFloor(w, 16.916); if (w.per != 16.916 || w.holdMax != 17.4 || w.freeMin != 20.0) floorBad++;
        FgLawFloor(w, 16.700); if (w.per != 16.700 || w.floor != 16.700 || w.holdMax != 17.4 || w.freeMin != 20.0) floorBad++;       // down, at the floor: follows
        FgLawFloor(w, 17.600); if (w.per != 17.600 || w.holdMax != 17.600 || w.freeMin != 20.0 || FgLawReg(w, w.per) != 0) floorBad++;  // up past the band: no band
        w.per = 20.0; FgLawFloor(w, 16.916); if (w.per != 20.0 || w.holdMax != 17.4 || FgLawReg(w, w.per) != 2) floorBad++;          // off the floor: stays
        FgLawFloor(w, 20.3); if (w.per != 20.3 || w.holdMax != 20.3 || w.freeMin != 20.3 || FgLawReg(w, w.per) != 0) floorBad++;     // the floor over it: the floor
        FgLawReset(w, 20.3, 0.0, 20.0, 1.06); if (w.holdMax != 20.3 || w.freeMin != 20.3 || w.per != 20.3 || w.warm != 150) floorBad++;
        if (FgLawSnap(w, 20.31) != 20.31 || FgLawSnap(w, 19.0) != 20.3) floorBad++;
        FgLawReset(w, 16.916, 17.4, 20.0, 1.06);
        if (FgLawSnap(w, 16.95) != 16.916 + ceil((16.95 - 16.916) / 0.2 - 1e-9) * 0.2 || FgLawSnap(w, 17.39) != 17.4 || FgLawSnap(w, 17.41) != 20.0 || FgLawSnap(w, 23.5) != 23.5) floorBad++;
        if (floorBad) printf("    FLOOR: %d of FgLawFloor's and FgLawSnap's own checks failed\n", floorBad);
    }

    printf("  the C++ over %d streams: %lld steps, %lld moves, %lld hitches; %lld steps differ from the oracle (%d streams); %d of the streams in the free band's form\n", nStreams, steps, moves, hitches, diffs, streamsBad, form1);
    printf("  sizeof(FgLaw) %u bytes\n", (unsigned)sizeof(FgLaw));
    printf("  the push's paths (fg_law.h, FgLawPush): the oldest out and the new in by one shift, at the count's cap %lld, past the window %lld;"
           " the new over the oldest %lld, under it %lld; a plain push %lld\n", gPath[0], gPath[1], gPath[2], gPath[3], gPath[4]);
    if (!gPath[0] || !gPath[1] || !gPath[2] || !gPath[3] || !gPath[4]) { printf("EQUIV_FGLAW FAIL: a path of the push never taken (a 0 above): not proved\n"); return 2; }
    if (books || floorBad) { printf("EQUIV_FGLAW FAIL: %d faults in the rings' books, %d in the floor's checks\n", books, floorBad); return 2; }
    if (diffs)
    {
        if (control) printf("EQUIV_FGLAW FAIL: %lld differences between fg_law.h and the oracle (MISMATCH above); the control failed at the comparison, as it must\n", diffs);
        else printf("EQUIV_FGLAW FAIL: %lld differences between fg_law.h and the oracle (MISMATCH above)\n", diffs);
        return 1;
    }
    if (control) { printf("EQUIV_FGLAW FAIL: the control PASSED: the comparison reads nothing\n"); return 3; }
    printf("EQUIV_FGLAW PASS\n");
    return 0;
}
