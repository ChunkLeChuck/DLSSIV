// dt_bbs_cost.cpp: WHAT THE TRACKER'S ONE TEST COSTS A REPLAYED CALL, PRICED IN A MODEL OF THE WORKER'S LOOP (2026-10-12).
//
//   python run_proof.py dt_bbs_cost
//
// In a build whose product can arm, the device thread's worker makes one more test on every device call it replays
// (source/devthread/dt_core.h, DtExec: `if (DtBbsLooks(slot)) DtBbsSeen(slot, c, hr);`, a compare of one byte of
// devthread/dt_bbs.h's state and a branch), whether or not frame generation is engaged. The game's stream is about
// 45,000 calls a frame in the world (the layer's own line in the owner's logs), so the test is made that often.
//
// THIS IS A MODEL, NOT THE MOD'S LOOP: a ring of commands (a header with the slot and the argument count, the arguments),
// replayed by a loop of DtExec's shape (the switch on the argument count, the call through a table of __stdcall
// pointers by slot, the test of the answer, the test of the slot's observer pointer), with callees that do NOTHING.
// The mod's callees are DXVK's methods, tens of nanoseconds each, so the test is a far larger share of this loop than
// of the real one: what is read here is AN UPPER BOUND of what the test adds to a call. Two loops, the same but for
// the test, run in turn, twelve rounds of 40,000,000 calls each; the least time of each is taken (the least is the
// run nothing disturbed), and the difference is the test's.
// IT ENDS "PASS" WHEN IT RAN: it is a price, not a proof, and nothing is held to a threshold.
#include <windows.h>
#include <stdio.h>
#include <intrin.h>

typedef HRESULT(__stdcall* F0)(void*);
typedef HRESULT(__stdcall* F1)(void*, DWORD);
typedef HRESULT(__stdcall* F2)(void*, DWORD, DWORD);
typedef HRESULT(__stdcall* F3)(void*, DWORD, DWORD, DWORD);
typedef HRESULT(__stdcall* F4)(void*, DWORD, DWORD, DWORD, DWORD);
static volatile LONG gSink;
static __declspec(noinline) HRESULT __stdcall C0(void*) { return S_OK; }
static __declspec(noinline) HRESULT __stdcall C1(void*, DWORD) { return S_OK; }
static __declspec(noinline) HRESULT __stdcall C2(void*, DWORD, DWORD) { return S_OK; }
static __declspec(noinline) HRESULT __stdcall C3(void*, DWORD, DWORD, DWORD) { return S_OK; }
static __declspec(noinline) HRESULT __stdcall C4(void*, DWORD, DWORD, DWORD, DWORD) { return S_OK; }
static void* gDx[128];
static void (*gObs[128])(const DWORD*);
static unsigned char gLooks[128];
static volatile LONG gErrs, gSeen;
static __declspec(noinline) void Seen(DWORD, const DWORD*, HRESULT) { _InterlockedIncrement(&gSeen); }

enum { RING_DW = 1 << 16 };
static DWORD gRing[RING_DW + 8];
static DWORD gCalls;

// The stream: the slots a frame of the game is mostly made of (states, textures, constants, draws), by a fixed
// pseudo-random draw, with 0 to 4 arguments. None of them is a slot the tracker looks at: the byte is 0, as it is for
// every call of a frame but a handful.
static void Fill()
{
    static const struct { DWORD slot, argc; } kinds[] = { { 57, 2 }, { 65, 2 }, { 69, 3 }, { 94, 3 }, { 109, 3 }, { 100, 4 }, { 104, 1 }, { 92, 1 }, { 107, 1 }, { 87, 1 }, { 41, 0 }, { 47, 1 } };
    DWORD at = 0, s = 0x9E3779B9u; gCalls = 0;
    for (;;)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        const auto& k = kinds[s % (sizeof(kinds) / sizeof(kinds[0]))];
        if (at + 1 + k.argc > RING_DW) break;
        gRing[at] = (k.slot << 8) | k.argc;
        for (DWORD i = 0; i < k.argc; i++) gRing[at + 1 + i] = s + i;
        at += 1 + k.argc; gCalls++;
    }
    gRing[at] = 0xFFFFFFFFu;
}
template <bool TEST> static __declspec(noinline) void Replay(void* dev, int laps)
{
    for (int lap = 0; lap < laps; lap++)
    {
        const DWORD* c = gRing;
        for (;;)
        {
            const DWORD h = *c;
            if (h == 0xFFFFFFFFu) break;
            const DWORD slot = (h >> 8) & 127, n = h & 0xFF;
            HRESULT hr;
            switch (n)
            {
            case 0: hr = ((F0)gDx[slot])(dev); break;
            case 1: hr = ((F1)gDx[slot])(dev, c[1]); break;
            case 2: hr = ((F2)gDx[slot])(dev, c[1], c[2]); break;
            case 3: hr = ((F3)gDx[slot])(dev, c[1], c[2], c[3]); break;
            default: hr = ((F4)gDx[slot])(dev, c[1], c[2], c[3], c[4]); break;
            }
            if (FAILED(hr)) _InterlockedIncrement(&gErrs);
            if (TEST) { if (gLooks[slot] != 0) Seen(slot, c, hr); }
            if (gObs[slot]) gObs[slot](c);
            c += 1 + n;
        }
    }
}

int main()
{
    for (int i = 0; i < 128; i++) gDx[i] = (void*)&C0;
    gDx[57] = gDx[65] = (void*)&C2; gDx[69] = gDx[94] = gDx[109] = (void*)&C3; gDx[100] = (void*)&C4;
    gDx[104] = gDx[92] = gDx[107] = gDx[87] = gDx[47] = (void*)&C1;
    Fill();
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1 << 4);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    LARGE_INTEGER qf; QueryPerformanceFrequency(&qf);
    const int laps = (int)(40000000u / gCalls) + 1;
    const double calls = (double)laps * (double)gCalls;
    double best[2] = { 1e30, 1e30 };
    Replay<false>(nullptr, laps / 8); Replay<true>(nullptr, laps / 8);        // warm
    for (int round = 0; round < 12; round++)
        for (int arm = 0; arm < 2; arm++)
        {
            const int which = (round & 1) ? 1 - arm : arm;                    // (the order turned every round)
            LARGE_INTEGER t0, t1; QueryPerformanceCounter(&t0);
            if (which) Replay<true>(nullptr, laps); else Replay<false>(nullptr, laps);
            QueryPerformanceCounter(&t1);
            const double ns = (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)qf.QuadPart / calls;
            if (ns < best[which]) best[which] = ns;
        }
    const double d = best[1] - best[0];
    printf("  the model loop, %u calls a lap of %d laps, callees that do nothing, twelve rounds, the least of each:\n", gCalls, laps);
    printf("  without the test %.3f ns a call; with it %.3f ns a call; THE TEST: %.3f ns a call\n", best[0], best[1], d);
    printf("  at 45,000 replayed calls a frame (the owner's logs in the world) that is %.1f us a frame of the worker's time at most: the mod's callees are DXVK's, tens of nanoseconds each, and the test is a smaller share of that loop\n", d * 45000.0 / 1000.0);
    printf("  (errors %ld, seen %ld: both 0 in this stream)\n", (long)gErrs, (long)gSeen);
    printf("PASS\n");
    return 0;
}
