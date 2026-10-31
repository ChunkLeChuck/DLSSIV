"""THE CPU PLACEMENT (2026-09-30, step 5's Stage E), in three parts.

1. THE PLAN. CpuPlPlanFor (source/cpu_place.h, between the CPU PLAN lines) decides the pin and the wall from the CPU sets
Windows reports. The owner's machine is one CPU, so the plan is held here against made-up ones, each the shape a real part
reports: the owner's 7900X3D, a 7950X, a 7800X3D, a four-core with SMT, four cores without, six without, two cores, a Zen 2
3900X (four three-core CCXs), an Intel hybrid, a Strix Point laptop, a Meteor Lake with its low-power island, laptops with four
and with two fast cores beside efficient ones, the V-Cache on the second CCD, equal caches listed in reverse, a set allocated
to another process, and no device thread.
2. THE SOURCE. IvtTick calls CpuPlaceTick outside its frame timer, which runs only at log=1 and up (the review of 2026-09-30,
CP-1: inside it the shipped log=0 placed nothing). Scoped to IvtTick's body, // comments stripped, with a control symbol.
3. THE STATE MACHINE, whole (cpu_place.h below the plan too, the transport's few names stubbed), on this test's own process with
Windows' real CPU-set calls: the stand-downs, the world counted with slack, the apply on its worker, what Windows then holds,
the withdraw after another program's change, and another program's set found before the apply.

    python tools/test-cpu-place/gen.py        exit code 0 = every check passed
"""
import os, re, sys, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "source", "cpu_place.h")
TRANSPORT = os.path.join(ROOT, "source", "transport", "DlssIvTransport.h")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-cpu-place-%d" % os.getpid())
START = "// CPU PLAN START"
END = "// CPU PLAN END"

BENCH = r"""
static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) fails++; }
// A made-up CPU: lps logical processors, smt per core, caches of `per` cores each with their sizes, efficiency by core.
struct Cpu { CpuPlSet s[64]; int n = 0; CpuPlLlc l[8]; int nl = 0; };
static void AddCache(Cpu& c, BYTE llc, DWORD kb) { c.l[c.nl].llc = llc; c.l[c.nl].kb = kb; c.nl++; }
static void AddCore(Cpu& c, BYTE llc, BYTE eff, int smt)
{
    const BYTE core = (BYTE)c.n;              // CoreIndex: the core's first processor, as Windows numbers them
    for (int t = 0; t < smt; t++) { CpuPlSet& x = c.s[c.n]; x.id = 256 + c.n; x.lp = (BYTE)c.n; x.core = core; x.llc = llc; x.eff = eff; x.usable = true; c.n++; }
}
static bool Walls(const CpuPlPlan& p, std::initializer_list<int> lps)
{
    if (p.nwall != (int)lps.size()) return false;
    int i = 0; for (int lp : lps) if (p.wallLp[i++] != lp) return false;
    return true;
}
static bool ProcHas(const CpuPlPlan& p, int lp) { for (ULONG i = 0; i < p.procN; i++) if (p.proc[i] == (ULONG)(256 + lp)) return true; return false; }
static void Show(const char* name, const CpuPlPlan& p)
{
    printf("    %s: pin %d (%d cores), domain %d cores (%d fast, %d processors), walled", name, p.pin, p.pinCores, p.domCores, p.fastCores, p.domLps);
    for (int i = 0; i < p.nwall; i++) printf(" %d", p.wallLp[i]);
    printf("; default set %lu\n", p.procN);
}
int main()
{
    CpuPlPlan p;
    {   // THE OWNER'S 7900X3D: CCD0 six cores, 96 MB; CCD1 six cores, 32 MB
        Cpu c; AddCache(c, 0, 98304); AddCache(c, 1, 32768);
        for (int k = 0; k < 6; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 6; k++) AddCore(c, 1, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("7900X3D", p);
        Check(p.pin == 0 && p.pinCores == 6 && Walls(p, { 2, 4, 6, 8 }) && p.procN == 8 && ProcHas(p, 3) && ProcHas(p, 0) && !ProcHas(p, 12),
              "7900X3D: pinned to the 96 MB CCD, one processor each on cores 2 to 5 (2, 4, 6, 8), their siblings and core 1 and 6 left to the rest, the other CCD out");
    }
    {   // A 7950X: two CCDs of eight cores, the same 32 MB each
        Cpu c; AddCache(c, 0, 32768); AddCache(c, 1, 32768);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 1, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("7950X", p);
        Check(p.pin == 0 && p.pinCores == 8 && Walls(p, { 2, 4, 6, 8 }) && p.procN == 12, "7950X: the halves equal, pinned to the lower-numbered CCD (Z21), the wall inside it");
    }
    {   // A 7800X3D: one CCD, eight cores
        Cpu c; AddCache(c, 0, 98304); for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("7800X3D", p);
        Check(p.pin < 0 && p.domCores == 8 && Walls(p, { 2, 4, 6, 8 }) && p.procN == 12, "7800X3D: one cache, no pin; four walled, the rest on the other twelve");
    }
    {   // A four-core with SMT (an i3-12100): two cores kept, two walled
        Cpu c; AddCache(c, 0, 12288); for (int k = 0; k < 4; k++) AddCore(c, 0, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("4c/8t", p);
        Check(p.pin < 0 && Walls(p, { 2, 4 }) && p.procN == 6, "four cores with SMT: two walled (Z11), two cores and the siblings for the rest");
    }
    {   // Four cores without SMT (an i5-6600): the rest would keep two logical processors, unmeasured: no wall (CP-3)
        Cpu c; AddCache(c, 0, 6144); for (int k = 0; k < 4; k++) AddCore(c, 0, 1, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("4c/4t", p);
        Check(p.pin < 0 && p.nwall == 0 && p.domLps == 4, "four cores without SMT: no wall (the placement stands down)");
    }
    {   // Six cores without SMT (an i5-9400F): four walled, two cores for the rest (Z12)
        Cpu c; AddCache(c, 0, 9216); for (int k = 0; k < 6; k++) AddCore(c, 0, 1, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("6c/6t", p);
        Check(p.pin < 0 && Walls(p, { 1, 2, 3, 4 }) && p.procN == 2 && ProcHas(p, 0) && ProcHas(p, 5), "six cores without SMT: four walled, processors 0 and 5 for the rest");
    }
    {   // Two cores with SMT: nothing to spare
        Cpu c; AddCache(c, 0, 4096); for (int k = 0; k < 2; k++) AddCore(c, 0, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("2c/4t", p);
        Check(p.pin < 0 && p.nwall == 0, "two cores: no pin, no wall (the placement stands down)");
    }
    {   // A Zen 2 3900X: four CCXs of three cores, 16 MB each: too small to pin to; the wall in one CCX where it fits
        Cpu c; for (int x = 0; x < 4; x++) AddCache(c, (BYTE)x, 16384);
        for (int x = 0; x < 4; x++) for (int k = 0; k < 3; k++) AddCore(c, (BYTE)x, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("3900X", p);
        Check(p.pin < 0 && p.domCores == 12 && Walls(p, { 6, 8, 10, 12 }) && p.procN == 20,
              "3900X: no pin (three-core CCXs); three walled in the second CCX together, the fourth in the next full one (the first holds core 0)");
    }
    {   // An Intel 12700K: eight P-cores with SMT, four E-cores, one ring cache
        Cpu c; AddCache(c, 0, 25600);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 4; k++) AddCore(c, 0, 0, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("12700K", p);
        Check(p.pin < 0 && Walls(p, { 2, 4, 6, 8 }) && p.procN == 16 && ProcHas(p, 16), "Intel hybrid: the wall on P-cores (EfficiencyClass), the E-cores left to the rest");
    }
    {   // A Ryzen AI 9 HX 370: four Zen 5 cores (16 MB, faster), eight Zen 5c (8 MB)
        Cpu c; AddCache(c, 0, 16384); AddCache(c, 1, 8192);
        for (int k = 0; k < 4; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 1, 0, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("HX 370", p);
        Check(p.pin < 0 && p.fastCores == 4 && Walls(p, { 2, 4 }) && p.procN == 22,
              "Strix Point: not pinned to the four-core cache; two walled on the Zen 5 cores, two of them kept (the reserve counts fast cores, CP-2)");
    }
    {   // A Meteor Lake: six P-cores and eight E-cores on the ring (24 MB), two low-power E-cores on their own (no L3)
        Cpu c; AddCache(c, 0, 24576); AddCache(c, 1, 0);
        for (int k = 0; k < 6; k++) AddCore(c, 0, 2, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 1);
        for (int k = 0; k < 2; k++) AddCore(c, 1, 0, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("Meteor Lake", p);
        Check(p.pin == 0 && Walls(p, { 2, 4, 6, 8 }) && p.procN == 16 && !ProcHas(p, 20), "Meteor Lake: pinned to the ring (the low-power island left out), the wall on P-cores");
    }
    {   // An Intel laptop, four P-cores with SMT and eight E-cores on one ring: two P-cores walled, two kept (CP-2)
        Cpu c; AddCache(c, 0, 12288);
        for (int k = 0; k < 4; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 0, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("4P+8E", p);
        Check(p.pin < 0 && p.fastCores == 4 && Walls(p, { 2, 4 }) && p.procN == 14, "4P+8E: two walled on P-cores, two P-cores and the E-cores for the rest");
    }
    {   // An Intel laptop, two P-cores with SMT and eight E-cores (a 1255U): no fast core to spare
        Cpu c; AddCache(c, 0, 12288);
        for (int k = 0; k < 2; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 0, 1);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("2P+8E", p);
        Check(p.pin < 0 && p.nwall == 0, "2P+8E: no wall (the E-cores are not counted as spare fast cores)");
    }
    {   // The V-Cache on the SECOND CCD (processors 12 to 23), the 32 MB cache listed first: the larger one is pinned (F8)
        Cpu c; AddCache(c, 0, 32768); AddCache(c, 1, 98304);
        for (int k = 0; k < 6; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 6; k++) AddCore(c, 1, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("V-Cache second", p);
        Check(p.pin == 1 && Walls(p, { 12, 14, 16, 18 }) && p.procN == 8 && !ProcHas(p, 0),
              "the V-Cache on the second CCD: pinned there by size, not by order (processor 0's core is outside it)");
    }
    {   // Equal caches listed in reverse processor order: the one holding processor 0 is pinned, whatever the list's order
        Cpu c; AddCache(c, 1, 32768); AddCache(c, 0, 32768);
        for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 8; k++) AddCore(c, 1, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("reverse list", p);
        Check(p.pin == 1 && Walls(p, { 2, 4, 6, 8 }) && ProcHas(p, 0) && !ProcHas(p, 16), "equal caches listed in reverse: the one holding processor 0, not the first listed");
    }
    {   // The 7900X3D with processor 2 allocated to another process
        Cpu c; AddCache(c, 0, 98304); AddCache(c, 1, 32768);
        for (int k = 0; k < 6; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 6; k++) AddCore(c, 1, 1, 2);
        c.s[2].usable = false;
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p); Show("7900X3D, 2 allocated", p);
        Check(p.pin == 0 && p.nwall == 4 && p.wallLp[0] != 2 && !ProcHas(p, 2), "a set allocated to another process: never walled, never in the default set");
    }
    {   // No device thread (threaded=0): three threads
        Cpu c; AddCache(c, 0, 98304); for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 3, &p); Show("three threads", p);
        Check(Walls(p, { 2, 4, 6 }) && p.procN == 13, "three threads to wall: three processors");
    }
    {   // NO THREAD ASKED FOR (cpu_place=0, the default since 1.1): the pin alone on a two-cache CPU, the whole cache the default set
        Cpu c; AddCache(c, 0, 98304); AddCache(c, 1, 32768);
        for (int k = 0; k < 6; k++) AddCore(c, 0, 1, 2);
        for (int k = 0; k < 6; k++) AddCore(c, 1, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 0, &p); Show("7900X3D, no thread", p);
        Check(p.pin == 0 && p.nwall == 0 && p.procN == 12 && ProcHas(p, 2) && ProcHas(p, 8) && !ProcHas(p, 12),
              "no thread asked for on the 7900X3D: pinned, nothing walled, all twelve processors of the cache in the default set");
    }
    {   // ...and nothing at all on one cache: eight cores (the first stutter report's CPU shape), and a hybrid (the second's)
        Cpu c; AddCache(c, 0, 98304); for (int k = 0; k < 8; k++) AddCore(c, 0, 1, 2);
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 0, &p); Show("eight cores, no thread", p);
        Check(p.pin < 0 && p.nwall == 0, "no thread asked for on eight cores with one cache: no pin, no wall (the placement stands down)");
        CpuPlPlanFor(c.s, c.n, c.l, c.nl, 4, &p);
        Check(Walls(p, { 2, 4, 6, 8 }) && p.procN == 12, "the control: asked for four, the same CPU walls 2, 4, 6 and 8 and leaves twelve (the report's own cpu line)");
        Cpu h; AddCache(h, 0, 36864);
        for (int k = 0; k < 8; k++) AddCore(h, 0, 1, 2);
        for (int k = 0; k < 16; k++) AddCore(h, 0, 0, 1);
        CpuPlPlanFor(h.s, h.n, h.l, h.nl, 0, &p); Show("8P+16E, no thread", p);
        Check(p.pin < 0 && p.nwall == 0, "no thread asked for on a 32-processor hybrid: no pin, no wall");
        CpuPlPlanFor(h.s, h.n, h.l, h.nl, 3, &p);
        Check(Walls(p, { 2, 4, 6 }) && p.procN == 29, "the control: asked for three (no device thread), it walls 2, 4 and 6 and leaves 29 (the second report's own cpu line)");
    }
    if (fails) { printf("FAIL: %d check(s)\n", fails); return 1; }
    printf("PASS\n"); return 0;
}
"""

# THE TRANSPORT'S NAMES, stubbed: the placement reads these and nothing else of it.
MACHINE = r"""
struct { int unpin = 1; int cpu_place = 1; } ivtCfg;   // the wall asked for (cpu_place=1, not the default since 1.1): every case but the last runs as 1.0.7 did
static bool ivtWine = false;
static volatile LONG ivtInjections = 0;
static DWORD renderTid = 0, dtWorkerTid = 0, ivtMainTid = 0;
static HANDLE ivtHost = nullptr;
static char lastLog[600] = "";
static void LogRecord(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(lastLog, sizeof(lastLog), fmt, ap); va_end(ap); printf("    log: %s\n", lastLog); }
"""

MACHINE_MAIN = r"""
static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) fails++; }
static HANDLE gStop;
static DWORD WINAPI Sleeper(void*) { WaitForSingleObject(gStop, INFINITE); return 0; }
static void Reset() { cpuPlState = 0; cpuPlRun = 0; cpuPlDry = 0; cpuPlCheck = 0; cpuPlLastInj = -1; }
static void Frames(int n, bool moving) { for (int i = 0; i < n; i++) { if (moving) InterlockedIncrement(&ivtInjections); CpuPlaceTick(); while (cpuPlBusy) Sleep(1); } }
static bool ThreadHolds(HANDLE h, ULONG id)
{
    typedef BOOL (WINAPI* G)(HANDLE, PULONG, ULONG, PULONG);
    static const G get = (G)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadSelectedCpuSets");
    ULONG ids[8]; ULONG n = 0;
    return get && get(h, ids, 8, &n) && n == 1 && ids[0] == id;
}
static ULONG ThreadSets(HANDLE h)
{
    typedef BOOL (WINAPI* G)(HANDLE, PULONG, ULONG, PULONG);
    static const G get = (G)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadSelectedCpuSets");
    ULONG ids[8]; ULONG n = 0;
    return get && get(h, ids, 8, &n) ? n : 999;
}
static ULONG ProcSetN() { ULONG ids[64]; ULONG n = 0; return cpuPlGetProc && cpuPlGetProc(GetCurrentProcess(), ids, 64, &n) ? n : 999; }
int main()
{
    gStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    DWORD tid[4]; HANDLE th[4];
    for (int i = 0; i < 4; i++) th[i] = CreateThread(nullptr, 0, Sleeper, nullptr, 0, &tid[i]);
    typedef HRESULT (WINAPI* SD)(HANDLE, PCWSTR);
    const SD setDesc = (SD)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");
    if (setDesc) setDesc(th[2], L"dxvk-cs");
    renderTid = tid[0]; dtWorkerTid = tid[1]; ivtMainTid = tid[3];
    // 1. unpin=0 stands down at once
    ivtCfg.unpin = 0; Frames(1, true);
    Check(cpuPlState == 2 && strstr(lastLog, "unpin=0"), "unpin=0: stood down, and said so");
    // 2. Wine stands down
    Reset(); ivtCfg.unpin = 1; ivtWine = true; Frames(1, true);
    Check(cpuPlState == 2 && strstr(lastLog, "Wine"), "under Wine: stood down, and said so");
    ivtWine = false;
    // 3. the world counted with slack: a loading screen (60 frames without an injection) starts the count again
    Reset(); Frames(500, true); Frames(60, false); Frames(500, true);
    Check(cpuPlState == 0, "a loading screen inside the count starts it again: nothing applied after 500, a load, 500");
    Frames(150, true);
    // 4. applied, on its worker: what Windows holds is the plan
    const bool applied = cpuPlState == 1;
    Check(applied, "applied after 600 frames of the world in a row");
    if (applied)
    {
        Check(ProcSetN() == cpuPlP.procN && CpuPlIsOurs(GetCurrentProcess()), "the process's default CPU set is the plan's");
        int held = 0;
        for (int i = 0; i < cpuPlTN; i++) held += ThreadHolds(cpuPlT[i].h, cpuPlT[i].id) ? 1 : 0;
        Check(held == cpuPlTN && cpuPlTN == cpuPlP.nwall && cpuPlTN > 0, "every walled thread holds its one CPU set");
        bool dxvkFound = false; for (int i = 0; i < cpuPlTN; i++) dxvkFound |= cpuPlT[i].dxvk && cpuPlT[i].tid == tid[2];
        Check(cpuPlP.nwall < 3 || dxvkFound, "DXVK's command thread found by its name and walled (where three or more are walled)");
        // 5. another program narrows the affinity: withdrawn at the next look, and the default set taken back
        DWORD_PTR pm = 0, sm = 0; GetProcessAffinityMask(GetCurrentProcess(), &pm, &sm);
        const DWORD_PTR narrow = pm & (pm - 1);   // all but the lowest processor
        SetProcessAffinityMask(GetCurrentProcess(), narrow);
        Frames(600, true);
        Check(cpuPlState == 3 && strstr(lastLog, "withdrew"), "another program's affinity: the placement withdrew, and said so");
        Check(ProcSetN() == 0, "after the withdraw the default CPU set the mod wrote is gone (the walled processors are not left idle)");
        ULONG still = 0; for (int i = 0; i < 4; i++) still += ThreadSets(th[i]);
        Check(cpuPlTN == 0 && still == 0, "after the withdraw no thread keeps a set, and the handles are closed");
        SetProcessAffinityMask(GetCurrentProcess(), pm);
    }
    // 6. a default CPU set another program gave before the apply: stood down, its set left alone
    Reset();
    ULONG someId = cpuPlN ? cpuPl[cpuPlN - 1].id : 0;
    if (someId && cpuPlSetProc(GetCurrentProcess(), &someId, 1))
    {
        Frames(700, true);
        Check(cpuPlState == 2 && strstr(lastLog, "another program gave the game a default CPU set"), "another program's default set before the apply: stood down");
        Check(ProcSetN() == 1, "...and its set left as it was");
        cpuPlSetProc(GetCurrentProcess(), nullptr, 0);
    }
    else Check(false, "the bench could not give itself a default CPU set to stand for another program's");
    // 7. cpu_place=0 (the default since 1.1): NO THREAD gets a processor of its own on any CPU. Where this machine has a
    // cache to pin to, the pin alone is applied (the whole cache the default set); where it has one cache, it stands down.
    Reset(); ivtCfg.cpu_place = 0; Frames(700, true);
    {
        ULONG still = 0; for (int i = 0; i < 4; i++) still += ThreadSets(th[i]);
        Check(cpuPlTN == 0 && still == 0 && cpuPlP.nwall == 0, "cpu_place=0: no thread holds a CPU set");
        Check(strstr(lastLog, "cpu_place=0") != nullptr, "cpu_place=0: the log line says so");
        if (cpuPlNl >= 2 && cpuPlP.pin >= 0)
            Check(cpuPlState == 1 && ProcSetN() == cpuPlP.procN && cpuPlP.procN == (ULONG)cpuPlP.domLps, "cpu_place=0 on a CPU with a cache to pin to: the pin alone, every processor of that cache in the default set");
        else
            Check(cpuPlState == 2 && ProcSetN() == 0, "cpu_place=0 on a CPU with one cache: stood down, nothing written");
        printf("    (this machine: %d cache(s), pin %d, state %d)\n", cpuPlNl, cpuPlP.pin, cpuPlState);
        CpuPlClear();
        Check(ProcSetN() == 0, "...and the bench's own default set is cleared again");
    }
    ivtCfg.cpu_place = 1;
    SetEvent(gStop);
    if (fails) { printf("FAIL: %d check(s)\n", fails); return 1; }
    printf("PASS\n"); return 0;
}
"""


def strip_line_comments(t):
    # only // and /// (IvtTick holds no "//" inside a string); block comments stay
    return "\n".join(re.sub(r"//.*$", "", l) for l in t.splitlines())


def source_check():
    """IvtTick calls CpuPlaceTick after IvtReadCfg and before its frame timer's `if (telemetryOn)`."""
    t = open(TRANSPORT, encoding="utf-8").read().replace("\r\n", "\n")
    a = t.index("static void IvtTick(IDirect3DDevice9* dev)")
    b = t.index("\n}\n", a)
    body = strip_line_comments(t[a:b])
    ok = True
    for need in ("CpuPlaceTick();", "IvtReadCfg();", "if (telemetryOn)"):
        if need not in body:
            print("  FAIL  IvtTick's body has no %s (the scope or the control is wrong)" % need); ok = False
    if not ok:
        return False
    read = body.index("IvtReadCfg();"); tel = body.index("if (telemetryOn)")
    calls = [m.start() for m in re.finditer(r"CpuPlaceTick\(\);", body)]
    good = any(read < c < tel for c in calls) and not any(c > tel for c in calls)
    print("  %s  IvtTick: CpuPlaceTick after IvtReadCfg and before the frame timer's if (telemetryOn), none inside it (CP-1; %d calls)"
          % ("PASS" if good else "FAIL", len(calls)))
    return good


def compile_run(name, text):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name + ".cpp"), "w", encoding="utf-8") as f:
        f.write(text)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    cmd = 'call "%s" >nul 2>&1 && cl /nologo /EHsc /W4 /WX /std:c++17 /O2 %s.cpp /Fe:%s.exe' % (vcvars, name, name)
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0:
        print(r.stdout[-3000:], r.stderr[-2000:]); print("FAIL: %s did not compile" % name); return 1
    r = subprocess.run([os.path.join(OUT, name + ".exe")], capture_output=True, text=True)
    print(r.stdout, end="")
    return r.returncode


def main():
    src = open(SRC, encoding="utf-8").read()
    a = src.index(START); b = src.index(END, a)
    head = "#define WIN32_LEAN_AND_MEAN\n#include <windows.h>\n#include <cstdio>\n#include <cstdarg>\n#include <cstring>\n#include <initializer_list>\n"
    print("  the plan:")
    rc1 = compile_run("place", "// GENERATED by tools/test-cpu-place/gen.py: CpuPlPlanFor cut from source/cpu_place.h\n" + head + src[a:b] + BENCH)
    print("  the source:")
    rc2 = 0 if source_check() else 1
    print("  the state machine, on this process:")
    whole = src[src.index("#pragma once") + len("#pragma once"):]
    rc3 = compile_run("machine", "// GENERATED by tools/test-cpu-place/gen.py: source/cpu_place.h whole, the transport stubbed\n#define DLSSIV_DEV 0\n"
                      + head + MACHINE + whole + MACHINE_MAIN)
    rc = rc1 or rc2 or rc3
    if rc == 0:
        shutil.rmtree(OUT, ignore_errors=True)
    print("PASS: the plan, the source and the state machine" if rc == 0 else "FAIL")
    return rc


if __name__ == "__main__":
    sys.exit(main())
