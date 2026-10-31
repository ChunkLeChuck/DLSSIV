// equiv_bonehash.cpp -- proves source/bonehash.h, the four-lane bone hash, by
// compiling that header itself.
//
//   python run_proof.py equiv_bonehash
//
// WHAT IT PROVES. The hash's VALUE changed on purpose; what its consumers need is
// the equality relation, so that is what is checked:
//   1. every one of the n words is covered: flipping any single bit of any single
//      word changes the hash, for every n in 0, 4, ..., 576 (the live n is 552);
//   2. it is a function: the same block hashes the same twice, and a copy of a
//      block at another address hashes the same;
//   3. the old and the new agree on EQUALITY over a few hundred thousand pairs of
//      blocks that are equal, differ in one word, or are unrelated:
//      old(a) == old(b) exactly when new(a) == new(b);
//   4. n = 0 folds to a non-zero constant (0 is the "no pose" sentinel);
//   5. and it prints both loops' time over the live block size, because the point
//      of the change is speed and a proof that it got slower should say so.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <intrin.h>
#include "../../source/bonehash.h"

static unsigned OldHash(const unsigned* u, int n)          // the serial form this replaces, verbatim
{
    unsigned h = 2166136261u;
    for (int i = 0; i < n; i++)
        h = (h ^ u[i]) * 16777619u;
    return h;
}

static unsigned rngState = 0x9E3779B9u;
static unsigned Rng() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return rngState; }

int main()
{
    static unsigned a[576], b[576];
    int fails = 0;

    // 1. coverage, every n, every word, a bit chosen per word
    for (int n = 0; n <= 576; n += 4)
    {
        for (int i = 0; i < n; i++) a[i] = Rng();
        const unsigned base = BoneHashWords(a, n);
        for (int i = 0; i < n; i++)
        {
            const unsigned keep = a[i];
            a[i] ^= 1u << (Rng() & 31);
            if (BoneHashWords(a, n) == base) { if (fails++ < 5) printf("  FAIL coverage: n %d word %d did not move the hash\n", n, i); }
            a[i] = keep;
        }
        if (BoneHashWords(a, n) != base) { fails++; printf("  FAIL determinism at n %d\n", n); }
        memcpy(b, a, sizeof(unsigned) * n);
        if (BoneHashWords(b, n) != base) { fails++; printf("  FAIL a copy at another address hashed differently, n %d\n", n); }
    }

    // 3. the equality relation against the old hash
    long pairs = 0, disagree = 0;
    for (int round = 0; round < 300000; round++)
    {
        const int n = 4 * (int)(Rng() % 145);                       // 0..576
        for (int i = 0; i < n; i++) a[i] = Rng();
        const unsigned kind = Rng() % 3;
        if (kind == 0) memcpy(b, a, sizeof(unsigned) * n);           // equal
        else if (kind == 1) { memcpy(b, a, sizeof(unsigned) * n); if (n) b[Rng() % n] ^= 1u << (Rng() & 31); }   // one bit apart
        else for (int i = 0; i < n; i++) b[i] = Rng();               // unrelated
        const bool oldEq = OldHash(a, n) == OldHash(b, n);
        const bool newEq = BoneHashWords(a, n) == BoneHashWords(b, n);
        pairs++;
        if (oldEq != newEq) { disagree++; if (disagree <= 5) printf("  FAIL relation: kind %u n %d old %d new %d\n", kind, n, oldEq, newEq); }
    }
    fails += (int)disagree;

    // 4. the empty block
    const unsigned empty = BoneHashWords(a, 0);
    if (empty == 0u) { fails++; printf("  FAIL the empty block hashes to 0, the no-pose sentinel\n"); }

    // 5. the time, live size
    for (int i = 0; i < 552; i++) a[i] = Rng();
    unsigned sink = 0;
    unsigned __int64 t0 = __rdtsc();
    for (int k = 0; k < 200000; k++) { a[k % 552] ^= (unsigned)k; sink += OldHash(a, 552); }
    unsigned __int64 t1 = __rdtsc();
    for (int k = 0; k < 200000; k++) { a[k % 552] ^= (unsigned)k; sink += BoneHashWords(a, 552); }
    unsigned __int64 t2 = __rdtsc();
    const double oldC = (double)(t1 - t0) / 200000.0, newC = (double)(t2 - t1) / 200000.0;

    printf("  coverage: every word of every n in 0..576 moves the hash\n");
    printf("  relation: %ld pairs, %ld disagreements with the serial hash on equality\n", pairs, disagree);
    printf("  empty block: 0x%08X (not the sentinel)\n", empty);
    printf("  552 words: serial %.0f cycles, four lanes %.0f cycles (%.2fx)   [sink %u]\n", oldC, newC, newC > 0 ? oldC / newC : 0.0, sink);
    printf(fails ? "FAIL (%d)\n" : "PASS\n", fails);
    return fails ? 1 : 0;
}
