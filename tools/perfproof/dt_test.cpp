// dt_test.cpp -- proves the device thread (source/devthread/) on real threads,
// against a fake device, with the layer's own headers included unchanged.
//
//   python run_proof.py dt_test          (or: dt_test.exe [threaded] [sleepEvery] [steps])
//
// WHAT IT PROVES, per phase, each ending in an exact comparison:
//   1. the calls that must reach DXVK in order reach the fake device in exactly
//      the order the test made them, with identical arguments and identical
//      copies of every array they point at (constants, vertices, rects...);
//   2. every Get the shadow answers equals the reference state the test keeps in
//      program order (and, at threaded=2, the layer checks itself against the
//      fake device and must count no mismatch);
//   3. no call ever reaches the fake device naming an object that was already
//      dead: the deferred Release, the handed-over foreign Release, the pacing,
//      the drain at every SYNC call and the wrap of a deliberately small ring;
//   4. every Release returns the count a native one would, a texture level's
//      included (its count is its texture's); a Present that failed on the worker
//      is handed back exactly once; another device's objects are never recorded.
//   5. a Reset from a thread that is not the producer still finds every earlier
//      Release performed (recorded or handed over) and the layer unarmed;
//   6. after every drain the fake device holds exactly the reference's state, and the
//      fake's state blocks keep DXVK 3.1's rules: conformance cases with worked
//      expectations, which the old snapshot fake must fail, each by name (2026-09-28).
// The fake device sleeps on some calls so the worker falls behind and the ring
// fills and wraps; the phases run unarmed, armed, through a Reset and re-arm.
#include <windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <new>
#include <unordered_map>

static bool  telemetryOn = true;
static DWORD renderTid   = 0;
static int   rsOwnCreate = 0;
static int   gLayerLines = 0;
static char  gKeptLine[400] = "";      // the install's line for device slots left to Windows' compatibility layer (the `twin` mode)
static char  gRetakeLine[400] = "";    // DtRetakeSlots' line, the last one said
static void LogRecord(const char* fmt, ...)
{
    if (strstr(fmt, "belonged to apphelp.dll before the mod's hooks (Windows'"))
    { va_list k; va_start(k, fmt); _vsnprintf_s(gKeptLine, sizeof(gKeptLine), _TRUNCATE, fmt, k); va_end(k); }
    if (strstr(fmt, "TAKEN BACK"))
    { va_list k; va_start(k, fmt); _vsnprintf_s(gRetakeLine, sizeof(gRetakeLine), _TRUNCATE, fmt, k); va_end(k); }
    if (gLayerLines++ >= 200) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
// The layer's periodic report and its install line are health (log=2) since
// 2026-09-14; the harness prints them the same way.
static void LogHealth(const char* fmt, ...)
{
    if (gLayerLines++ >= 200) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
static void LogDetail(const char* fmt, ...) { (void)fmt; }
// The development log's print-on-change marker (LOGMARK, DLSS-IV.cpp). The hold itself is behind
// DLSSIV_DEVLOG, which this harness does not define, so the layer's periodic line prints every
// time here as it always has; only the note at the end of it needs a value. Stubbed exactly as
// the three log functions above are: nothing about the device thread is stubbed by this.
#define LOGMARK ""
// DEVSTAT, for the same reason equiv_c227.template.cpp carries it (final audit 2026-09-19): no devthread header and not
// ds_bracket.h uses it today, and the first one that does would leave a function-like macro undefined at file scope
// here, a compile error a long way from its cause, in a file build.ps1 never compiles. As in a development build.
#ifndef DEVSTAT
#define DEVSTAT(...) __VA_ARGS__
#endif
#ifndef DLSSIV_HOT
#define DLSSIV_HOT          // the mod's is __declspec(allocate(".data$dlhot")) with a #pragma section; placement is nothing to a harness
#endif
static void IvtWarn(const char* fmt, ...) { (void)fmt; }     // the warnings file: nothing to write in a harness
// DtNote's re-test after the post's fetch (dt_core.h, F1.1) is a macro so that NoteAcrossDisarm can switch it OFF and show
// the orphaned note it prevents. In the mod it is the constant true.
static bool gNoteRecheck = true;
#define DT_NOTE_RECHECK() gNoteRecheck
// The same for another thread's state-capturing SYNC calls (dt_device_sync.h, F1.7): ForeignStateBlockHeld's control.
static bool gSyncOrdered = true;
#define DT_SYNC_ORDERED_ON() gSyncOrdered
// C1'S SWITCH (docs/reports/c1-design-2026-09-28/04, 1.13): true is the recorded state-block Apply (B), false today's
// drained one (A). The layer reads it through DT_SB_REC_ON (dt_core.h); argv[6] picks the arm. At A every NO-WAIT check below
// must SEE the drained Apply's wait, which is the proof that it can see one (NoWaitCheck).
static bool gSbB = false;
#define DT_SB_REC_ON() gSbB
// THE FOREIGN BARRIER'S TWO READS (2026-09-28; dt_install.h, DtForeignBarrier): the order, which ForeignBarrierLap's control
// switches back, and a one-shot hook between them that widens the gap, fired on the first turn of the wait.
static bool gBarrierCurFirst = true;
#define DT_BARRIER_CUR_FIRST() gBarrierCurFirst
static void (*volatile gBarrierHook)() = nullptr;
static void (*volatile gBarrierEachTurn)() = nullptr;   // ...and one on EVERY turn (2026-10-01): the timed full-ring case
#define DT_BARRIER_BETWEEN_READS() do { void (*f_)() = gBarrierHook; if (f_) { gBarrierHook = nullptr; f_(); } void (*e_)() = gBarrierEachTurn; if (e_) e_(); } while (0)
// THE FOREIGN BARRIER'S COUNT (2026-10-01, the owner's decision 6; dt_install.h, DT_BARRIER_BY_COUNT): true in the mod; false
// puts back the ring-position test, ForeignBarrierLap's control.
static bool gBarrierByCount = true;
#define DT_BARRIER_BY_COUNT() gBarrierByCount
// WAKE-EVERY-64 (2026-10-01, the final audit; dt_core.h, DT_WAKE_KIND): the kind a drain and the one-frame-ahead wait ask for
// (2 in the mod; 1, the old way, is WakeHeld's control), the worker's signals counted, and fault 37 (the catch-up's signal
// dropped), live only inside WakeHeld so it cannot slow the rest of the run.
static LONG gWakeKind = 2;
#define DT_WAKE_KIND() gWakeKind
static volatile LONG gProdSignals = 0;
#define DT_PROD_SIGNAL() (_InterlockedIncrement(&gProdSignals), SetEvent(dtProdEv))
static volatile bool gWakeFaultLive = false;
#define DT_WAKE_FAULT() (gSbmFault == 37 && gWakeFaultLive)
// ...and the producer parked between DtCommit's two stores (dt_core.h, DT_COMMIT_AT_END), with the barriers' reading of
// that moment, which ForeignAtRingEnd's control switches off.
static bool gBarrierNormCur = true;
#define DT_BARRIER_NORM_CUR() gBarrierNormCur
static void (*volatile gCommitEndHook)() = nullptr;
#define DT_COMMIT_AT_END() do { void (*f_)() = gCommitEndHook; if (f_) { gCommitEndHook = nullptr; f_(); } } while (0)
// THE LATENCY INSTRUMENT'S RENDER-THREAD HALF (2026-09-24): dt_res.h stamps each recorded Present for it (latPresFrame,
// LatOnPresent), and the refresh guard holds it (2026-09-26, latGuard, RgOnPresent). Here both are no-ops, like the log:
// nothing about the device thread is stubbed by this.
enum { LAT_RING = 64 };
static bool latOn = false, latPacerOn = false;
static volatile LONG latPresFrame[LAT_RING];
// AND THE FLIP CLASSIFIER'S RETURN RING (2026-09-24, frame generation phase 1B, instrument I-B2): the worker stores each
// Present's return into it in a development build (dt_core.h, after the Present; ivt_latency.h declares it in the mod).
// Here it is written and never read, as in the mod with the log off: nothing about the device thread is stubbed by this.
DEVSTAT(alignas(8) static volatile LONGLONG latFlipRelQpc[LAT_RING]; static volatile float latFlipRelBlk[LAT_RING]; static volatile LONG latFlipRelN[LAT_RING];)
static volatile LONG frames = 0;
static void LatOnPresent(LONG, LONGLONG) {}
static bool latGuard = false;
static void RgOnPresent() {}
// THE DEVELOPMENT SIDE IS COMPILED (2026-09-28). (The GPU probe's two query calls in dt_res.h, DtQIssueProbe and
// DtQGetDataProbe, with their case ProbeQueries, and the frame recorder's stamp on the worker's Present, RecP, were the
// reason; they went in the strip of 2026-10-25.)
#ifndef DLSSIV_DEV
#define DLSSIV_DEV 1
#endif
// C1'S PLANTED FAULTS (2026-09-29, step 3c; dt_sbtab.h, DT_SBM_FAULT): dt_test's fifth argument names one, and the run must
// then FAIL, by the check that names it. 0, the default, plants none. In the mod the macro is the constant false.
static int gSbmFault = 0;
#define DT_SBM_FAULT(n) (gSbmFault == (n))
#define DT_SB_CAP_N 1024           // C1's table, larger than the mod's 64 (dt_sbtab.h): H2 keeps hundreds of blocks alive
// C1'S ARM DEFERRAL (dt_sbtab.h, DT_SB_ARM_DEFER): on, as in the mod, except in U5, which switches it off to reach the arm
// generation's check behind it (an arm can then land inside a recording).
static bool gArmDefer = true;
#define DT_SB_ARM_DEFER_ON() gArmDefer
// BETWEEN A CAPTURE'S COPY AND ITS COMMAND (dt_res.h, DT_SB_CAP_BETWEEN): a one-shot hook, for the cases that look at the record
// there or start another thread's call there (H7 (b), the Capture's recheck).
static void (*volatile gCapBetween)() = nullptr;
#define DT_SB_CAP_BETWEEN() { void (*f_)() = gCapBetween; if (f_) { gCapBetween = nullptr; f_(); } }
// THE STAND-IN'S FOLDER (1.0.8): the install lets a module named apphelp.dll keep its slots only from Windows' own folders
// (dt_install.h, DtForeignDirOk). The harness's stand-in is twin\apphelp.dll beside this exe, which is let through here and
// nowhere else: a copy of it in twin\other\ is the control that must be refused.
static bool FkTwinDir(const char* dir)
{
    char me[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, me, sizeof(me))) return false;
    char* cut = strrchr(me, '\\');
    if (!cut || cut + 6 > me + sizeof(me)) return false;
    memcpy(cut + 1, "twin", 5);
    return !_stricmp(dir, me);
}
#define DT_TEST_FOREIGN_DIR(dir) FkTwinDir(dir)
#include "../../source/devthread/devthread.h"
static bool WriteSlot(void** vtbl, int slot, void* fn)
{
    DWORD old;
    if (!VirtualProtect(&vtbl[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    vtbl[slot] = fn;
    VirtualProtect(&vtbl[slot], sizeof(void*), old, &old);
    return vtbl[slot] == fn;
}
#include "dt_fake.h"
#include "dt_fake_dev.h"
static DWORD gFkPretendWorker = 0;     // the control's: the calling thread counted as the worker (Caller3Case)
static DWORD FkWorkerTid() { return gFkPretendWorker ? gFkPretendWorker : dtWorkerTid; }
static LONG gCaller3Ran = 0;
// THE `twin` MODE (1.0.8): twin\apphelp.dll (twin/apphelp_twin.cpp) holds QueryInterface, Release, CreateAdditionalSwapChain
// and Reset in the fake's vtable before the install, as Windows' compatibility layer does in DXVK's, so the whole run goes
// through a foreign module's hooks on the four: the install's accept branch runs, and the stand-in itself counts every call
// and any made on the worker. Without the word the run is what it always was, and the install's line for kept slots must not come.
static bool    gTwin = false;
static HMODULE gTwinMod = nullptr;
static int   (__cdecl* gTwinPatch)(void**, int, DWORD(__cdecl*)()) = nullptr;
static int   (__cdecl* gTwinUnpatchPresent)() = nullptr;
static void  (__cdecl* gTwinRead)(LONG*, LONG*) = nullptr;
static int   (__cdecl* gTwinRepatch)() = nullptr;
static bool    gTwinDirRefused = false;         // a copy of the stand-in from another folder was refused by name
static LONG    gRetakeRan = 0;                  // RetakeCase ran whole
static LONG    gTwinRelBase = 0;                // the fake's Release count when the stand-in went in
static LONG    gTwinBase[3] = { 0, 0, 0 };      // the fake's own counts when the stand-in went in
static LONG    gTwinControl = 0;                // the control's call, which the stand-in must count as the worker's
static bool    gTwinRefused = false;            // the install refused the stand-in on Present, naming it and the four slots
static DWORD __cdecl TwinWorkerTid() { return FkWorkerTid(); }
#include "../../source/ds_bracket.h"     // the mod's own rule for its save/restore brackets, tested below as it ships

static FakeDev* gFd; static IDirect3DDevice9* D; static IDirect3DSwapChain9* SC;
static FkState R;                      // the reference: what the device holds, in program order
static void E(int slot, std::initializer_list<DWORD> a, const void* d1 = nullptr, size_t n1 = 0, const void* d2 = nullptr, size_t n2 = 0)
{ LogTo(gExp, slot, a, d1, n1, d2, n2); }
static unsigned gSeed = 0x9E3779B9u;
static unsigned Rnd() { gSeed ^= gSeed << 13; gSeed ^= gSeed >> 17; gSeed ^= gSeed << 5; return gSeed; }
static float Rf() { return (float)(Rnd() % 100000) * 0.01f - 500.0f; }
static DWORD SampNum(int i) { return i < 16 ? (DWORD)i : (DWORD)(i + 240); }

static std::vector<IDirect3DTexture9*> gTex; static std::vector<IDirect3DSurface9*> gRt, gDs;
static std::vector<IDirect3DVertexBuffer9*> gVb; static std::vector<IDirect3DIndexBuffer9*> gIb;
static std::vector<IDirect3DVertexShader9*> gVs; static std::vector<IDirect3DPixelShader9*> gPs;
static std::vector<IDirect3DVertexDeclaration9*> gDecl; static IDirect3DQuery9* gQ; static long gGets = 0, gPresents = 0;
static long gDeclGets = 0, gFvfGets = 0, gPscGets = 0, gDeclReused = 0;   // the 2026-09-19 entries: each must have been asked for
static IDirect3DVertexBuffer9* gVbM = nullptr;      // a MANAGED buffer: its NOOVERWRITE lock is an ordered one
static FakeDev* gFd2 = nullptr;                      // a second device, for objects that are not ours
static long gIssues = 0, gGetDatas = 0, gGetDataBesideBody = 0;   // Issues made of gQ; GetDatas made; of which beside a running body with no wait
static long gRefusedRtMade = 0;                      // refused SetRenderTarget calls made; the fake counts the ones that reached it
static long gExpReplayErr = 0;                       // replayed calls the test makes fail on purpose

// THE REFERENCE'S STATE BLOCKS (2026-09-28, C1 step 1). DXVK 3.1's rules written out a second time, from its source and
// not from the fake's code (dt_fake_dev.h): a capture set and its values; the typed masks from DXVK's two lists (shared,
// being DXVK's data); a recorder that keeps what it is given raw; a Capture that never widens the set and never takes a
// stream's offset; an Apply that skips a null declaration and goes through the device's own rules (FkState::Stream and
// FkState::Vp, the rules the reference has always used for a Set). Until today the reference was `R = snap` with the render
// targets kept, which walked the old fake's wrong rule with it. Compare now sets the reference against the fake device
// after every phase (RefAgainstDevice), so the two block models answer for each other.
struct RefBlk
{
    bool rs[256], samp[21][14], tex[21], vb[16], psc[224];
    bool decl, ib, vs, ps, vp, sc;
    bool vsc[256], tss[8][32], freq[16], xf[14];   // the four SYNC-answered kinds (dt_fake.h)
    FkState v;
};
static std::unordered_map<const void*, RefBlk*> gRefBlk;
// BLOCKS C1'S TABLE HAD NO ROOM FOR when they were made (the full-table case lowers the limit on purpose, and a million steps of
// H2 keep more blocks alive than even the harness's 1024): the oracle's "every live block has a record" leaves them out.
static std::unordered_map<const void*, int> gOrSkip;
static RefBlk* rRec = nullptr;          // the recording the test is inside, BeginStateBlock..EndStateBlock
// A recordable Set in the reference: into the open recording, raw as DXVK's recorder keeps it, or to R by the device's rules.
static void RRs(DWORD s, DWORD x)                 { if (rRec) { rRec->rs[s] = true; rRec->v.rs[s] = x; } else R.rs[s] = x; }
static void RSamp(int i, DWORD t, DWORD x)        { if (rRec) { rRec->samp[i][t] = true; rRec->v.samp[i][t] = x; } else R.samp[i][t] = x; }
static void RTex(int i, IDirect3DBaseTexture9* t) { if (rRec) { rRec->tex[i] = true; rRec->v.tex[i] = t; } else R.tex[i] = t; }
static void RVb(UINT i, IDirect3DVertexBuffer9* b, UINT o, UINT st)
{ if (rRec) { rRec->vb[i] = true; rRec->v.vb[i] = b; rRec->v.off[i] = o; rRec->v.str[i] = st; } else R.Stream(i, b, o, st); }
static void RIb(IDirect3DIndexBuffer9* b)         { if (rRec) { rRec->ib = true; rRec->v.ib = b; } else R.ib = b; }
static void RVs(IDirect3DVertexShader9* x)        { if (rRec) { rRec->vs = true; rRec->v.vs = x; } else R.vs = x; }
static void RPs(IDirect3DPixelShader9* x)         { if (rRec) { rRec->ps = true; rRec->v.ps = x; } else R.ps = x; }
static void RDecl(IDirect3DVertexDeclaration9* x) { if (rRec) { rRec->decl = true; rRec->v.decl = x; } else R.decl = x; }
static void RVp(const D3DVIEWPORT9& x)            { if (rRec) { rRec->vp = true; rRec->v.vp = x; } else R.Vp(x); }
static void RSc(const RECT& x)                    { if (rRec) { rRec->sc = true; rRec->v.sc = x; } else R.sc = x; }
static void RPsc(UINT s, const float* p, UINT n)
{ if (rRec) { for (UINT r = s; r < s + n; r++) rRec->psc[r] = true; memcpy(rRec->v.psc[s], p, n * 16); } else memcpy(R.psc[s], p, n * 16); }
static void RVsc(UINT s, const float* p, UINT n)
{ if (rRec) { for (UINT r = s; r < s + n; r++) rRec->vsc[r] = true; memcpy(rRec->v.vsc[s], p, n * 16); } else memcpy(R.vsc[s], p, n * 16); }
static void RTss(UINT g, DWORD k, DWORD x)       { if (rRec) { rRec->tss[g][k] = true; rRec->v.tss[g][k] = x; } else R.tss[g][k] = x; }
static void RFreq(UINT i, UINT f)                { if (rRec) { rRec->freq[i] = true; rRec->v.freq[i] = f; } else R.freq[i] = f; }
static void RXf(int k, const D3DMATRIX& m)       { if (rRec) { rRec->xf[k] = true; rRec->v.xf[k] = m; } else R.xf[k] = m; }
// The typed masks (d3d9_stateblock.cpp, CaptureType). PIXEL: its render states; sampler states 1 to 12 on samplers 0 to
// 16, the displacement-map sampler included; the pixel shader and every pixel constant; every stage state. VERTEX: its
// render states; DMAPOFFSET alone on the four vertex samplers; the vertex shader and its 256 float constants, the
// declaration and every stream frequency. ALL: both, and every texture and stream, the indices, the viewport, the
// scissor and every transform.
static void RefTyped(RefBlk& b, D3DSTATEBLOCKTYPE t)
{
    const bool all = t == D3DSBT_ALL;
    if (all || t == D3DSBT_PIXELSTATE)
    {
        for (DWORD s : kFkPixelRs) b.rs[s] = true;
        for (int i = 0; i <= 16; i++) for (DWORD k = D3DSAMP_ADDRESSU; k <= D3DSAMP_ELEMENTINDEX; k++) b.samp[i][k] = true;
        b.ps = true; for (int r = 0; r < 224; r++) b.psc[r] = true;
        for (int g = 0; g < 8; g++) for (int k = 0; k < 32; k++) b.tss[g][k] = true;
    }
    if (all || t == D3DSBT_VERTEXSTATE)
    {
        for (DWORD s : kFkVertexRs) b.rs[s] = true;
        for (int i = 17; i < 21; i++) b.samp[i][D3DSAMP_DMAPOFFSET] = true;
        b.vs = b.decl = true;
        for (int r = 0; r < 256; r++) b.vsc[r] = true;
        for (int i = 0; i < 16; i++) b.freq[i] = true;
    }
    if (all) { for (int i = 0; i < 21; i++) b.tex[i] = true; for (int i = 0; i < 16; i++) b.vb[i] = true; b.ib = b.vp = b.sc = true;
               for (int k = 0; k < 14; k++) b.xf[k] = true; }
}
// offsets: a typed block's creation takes a stream's offset; a Capture never does (stateblock.h, the <Capture, true> case).
static void RefCapture(RefBlk& b, bool offsets)
{
    for (int s = 0; s < 256; s++) if (b.rs[s]) b.v.rs[s] = R.rs[s];
    for (int i = 0; i < 21; i++) for (int k = 0; k < 14; k++) if (b.samp[i][k]) b.v.samp[i][k] = R.samp[i][k];
    for (int i = 0; i < 21; i++) if (b.tex[i]) b.v.tex[i] = R.tex[i];
    for (int i = 0; i < 16; i++) if (b.vb[i]) { b.v.vb[i] = R.vb[i]; b.v.str[i] = R.str[i]; if (offsets) b.v.off[i] = R.off[i]; }
    for (int r = 0; r < 224; r++) if (b.psc[r]) memcpy(b.v.psc[r], R.psc[r], 16);
    if (b.decl) b.v.decl = R.decl;                  // even a null one
    if (b.ib) b.v.ib = R.ib;
    if (b.vs) b.v.vs = R.vs;
    if (b.ps) b.v.ps = R.ps;
    if (b.vp) b.v.vp = R.vp;
    if (b.sc) b.v.sc = R.sc;
    for (int r = 0; r < 256; r++) if (b.vsc[r]) memcpy(b.v.vsc[r], R.vsc[r], 16);
    for (int g = 0; g < 8; g++) for (int k = 0; k < 32; k++) if (b.tss[g][k]) b.v.tss[g][k] = R.tss[g][k];
    for (int i = 0; i < 16; i++) if (b.freq[i]) b.v.freq[i] = R.freq[i];
    for (int k = 0; k < 14; k++) if (b.xf[k]) b.v.xf[k] = R.xf[k];
}
static void RefApply(const RefBlk& b)
{
    for (int s = 0; s < 256; s++) if (b.rs[s]) R.rs[s] = b.v.rs[s];
    for (int i = 0; i < 21; i++) for (int k = 0; k < 14; k++) if (b.samp[i][k]) R.samp[i][k] = b.v.samp[i][k];
    for (int i = 0; i < 21; i++) if (b.tex[i]) R.tex[i] = b.v.tex[i];
    for (int i = 0; i < 16; i++) if (b.vb[i]) R.Stream(i, b.v.vb[i], b.v.off[i], b.v.str[i]);
    for (int r = 0; r < 224; r++) if (b.psc[r]) memcpy(R.psc[r], b.v.psc[r], 16);
    if (b.decl && b.v.decl) R.decl = b.v.decl;      // a null one is never applied
    if (b.ib) R.ib = b.v.ib;
    if (b.vs) R.vs = b.v.vs;
    if (b.ps) R.ps = b.v.ps;
    if (b.vp) R.Vp(b.v.vp);
    if (b.sc) R.sc = b.v.sc;
    for (int r = 0; r < 256; r++) if (b.vsc[r]) memcpy(R.vsc[r], b.v.vsc[r], 16);
    for (int g = 0; g < 8; g++) for (int k = 0; k < 32; k++) if (b.tss[g][k]) R.tss[g][k] = b.v.tss[g][k];
    for (int i = 0; i < 16; i++) if (b.freq[i]) R.Freq(i, b.v.freq[i]);       // through the device's check, as DXVK applies it
    for (int k = 0; k < 14; k++) if (b.xf[k]) R.xf[k] = b.v.xf[k];
}
static RefBlk* RefOf(const void* sb)
{
    const auto it = gRefBlk.find(sb);
    if (it != gRefBlk.end()) return it->second;
    Err("a state block the reference never saw made (%p)", sb);
    static RefBlk none; return &none;
}
// THE TEST'S STATE BLOCK CALLS: each logged as the fake logs it and walked in the reference.
static IDirect3DStateBlock9* SbCreate(D3DSTATEBLOCKTYPE t)
{
    IDirect3DStateBlock9* sb = nullptr;
    const LONG full0 = dtSbFull;
    const HRESULT hr = D->CreateStateBlock(t, &sb); E(59, { (DWORD)t });
    if (FAILED(hr) || !sb) { Err("CreateStateBlock(%d) was refused (0x%08lX)", (int)t, (unsigned long)hr); return nullptr; }
    if (dtSbFull != full0) gOrSkip[sb] = 1;
    RefBlk* b = new RefBlk(); RefTyped(*b, t); RefCapture(*b, true);
    gRefBlk[sb] = b;
    return sb;
}
static void SbBegin()
{
    const HRESULT hr = D->BeginStateBlock(); E(60, {});
    if (FAILED(hr)) Err("BeginStateBlock was refused (0x%08lX)", (unsigned long)hr);
    rRec = new RefBlk();
}
static IDirect3DStateBlock9* SbEnd()
{
    IDirect3DStateBlock9* sb = nullptr;
    const LONG full0 = dtSbFull;
    const HRESULT hr = D->EndStateBlock(&sb); E(61, {});
    if (FAILED(hr) || !sb) { Err("EndStateBlock was refused (0x%08lX)", (unsigned long)hr); delete rRec; rRec = nullptr; return nullptr; }
    if (dtSbFull != full0) gOrSkip[sb] = 1;
    gRefBlk[sb] = rRec; rRec = nullptr;
    return sb;
}
// A CAPTURE IS RECORDED BY THE LAYER and answered D3D_OK (dt_res.h, dtSbCapture). Made while a block records, DXVK refuses
// it when the worker replays it: the block keeps what it had, and that one failed replay is counted (C1's P4). Unarmed, the
// call is DXVK's and so is the refusal.
static void SbCapture(IDirect3DStateBlock9* sb)
{
    const HRESULT hr = sb->Capture(); E(1904, { P(sb) });
    if (rRec)
    {
        if (dtArmed && GetCurrentThreadId() == dtProducer) { if (hr != D3D_OK) Err("a recorded Capture answered 0x%08lX", (unsigned long)hr); gExpReplayErr++; }
        else if (hr != D3DERR_INVALIDCALL) Err("an unarmed Capture while a block records answered 0x%08lX, not DXVK's refusal", (unsigned long)hr);
        return;
    }
    if (hr != D3D_OK) Err("a Capture answered 0x%08lX", (unsigned long)hr);
    RefCapture(*RefOf(sb), false);
}
static long gOrWouldOk = 0, gOrWindowApplies = 0;     // C1's would-be path, counted by SbApply for the oracle's summary
static void SbApply(IDirect3DStateBlock9* sb)
{
    _InterlockedExchange(&dtSbLastWhy, -1);
    const HRESULT hr = sb->Apply(); E(1905, { P(sb) });
    // C1's would-be path (dt_res.h, DtSbWould): an Apply made while a block records never takes it (NC11 ignores the recording).
    if (dtArmed && GetCurrentThreadId() == dtProducer)
    {
        if (rRec) { gOrWindowApplies++; if (dtSbLastWhy != DT_SBF_WINDOW) Err("C1: an Apply made while a block records would take the recorded path (the answer was %ld)", (long)dtSbLastWhy); }
        else if (dtSbLastWhy == DT_SBF_OK) gOrWouldOk++;
    }
    if (rRec) { if (hr != D3DERR_INVALIDCALL) Err("an Apply while a block records answered 0x%08lX, not DXVK's refusal", (unsigned long)hr); return; }
    if (hr != D3D_OK) Err("an Apply answered 0x%08lX", (unsigned long)hr);
    RefApply(*RefOf(sb));
}
static void SbRelease(IDirect3DStateBlock9* sb)
{
    const auto it = gRefBlk.find(sb);
    if (it != gRefBlk.end()) { delete it->second; gRefBlk.erase(it); }
    gOrSkip.erase(sb);
    const ULONG left = sb->Release();
    if (left != 0) Err("the last Release of a state block returned %lu, not 0", left);
}

// C1'S ORACLE (2026-09-29, step 3c; docs/reports/c1-design-2026-09-28/03, "REFERENCE AND ORACLE"). Every record the product
// keeps in its state-block table (dt_sbtab.h) is held against the fake device's REAL block, never against the product itself:
//   - the record names a live block of its device, of the kind the block was made (typed or recorded);
//   - a clean recorded one covers exactly what the block covers, of the kinds the shadow answers (render states 7..209, sampler
//     states 1..13, textures, streams, the shaders, the indices, the viewport, the scissor, the declaration, pixel constants);
//   - one whose values are known holds the block's values, raw (a stream's recorded offset, an unlifted recorded viewport);
//   - and its writer (dt_res.h, DtSbMirWrite), run on a shadow that knows the whole device, gives what the fake's own Apply
//     gives the device (the null declaration skipped, a null stream zeroed, MaxZ lifted), where the record says it knows.
// And while the table has never been full, every live block the reference knows has a record. Run at every Compare, and in
// H2 after every Apply. The negative controls (DT_SBM_FAULT) must each fail one of these checks, by its words.
static long gOrEnt = 0, gOrClean = 0, gOrVals = 0, gOrWrite = 0, gOrLive = 0, gOrFvf = 0, gOrRuns = 0;
// THE CAPTURE-COPIED RECORDS THE ORACLE CHECKED WITH THEIR VALUES KNOWN, by kind (review of 2026-09-29: a record straight from a
// recording met every earlier floor, and the Capture copy's branches for a stream, the declaration's three forms and the pixel
// constants may never have been checked).
static long gOrCapVals = 0, gOrCapStream = 0, gOrCapVp = 0, gOrCapDeclObj = 0, gOrCapDeclFvf = 0, gOrCapDeclUnk = 0, gOrCapPsc = 0;
static unsigned gOrSeed = 0x2545F491u;                  // the writer check's own draws: H2's program does not move with them
static unsigned OrRnd() { gOrSeed ^= gOrSeed << 13; gOrSeed ^= gOrSeed >> 17; gOrSeed ^= gOrSeed << 5; return gOrSeed; }
static void SbOracle(const char* phase)
{
    if (gFkSnapshot) return;
    gOrRuns++;
    AcquireSRWLockShared(&dtSbLock);
    for (int k = 0; k < DT_SB_CAP; k++)
    {
        const DtSbEnt& e = dtSbTab[k];
        if (e.kind == DT_SBK_FREE) continue;
        gOrEnt++;
        const FakeSB* b = (const FakeSB*)e.sb;
        if (IsDead(b)) { if (!dtPendMiss) Err("%s: C1's table holds a record for a dead state block %p", phase, e.sb); continue; }
        const void* dev = b->fkDev ? (const void*)b->fkDev : (const void*)gDev;
        if (e.dev != dev) Err("%s: C1's record of %p names device %p, the block is %p's", phase, e.sb, e.dev, dev);
        if (e.kind != b->made) Err("%s: C1's record of %p is of kind %d, the block was made as %d (1 typed, 2 recorded)", phase, e.sb, (int)e.kind, (int)b->made);
        if (e.kind != DT_SBK_REC || !e.clean) continue;
        gOrClean++;
        const FkCap& cp = b->cap; const DtSbMir& m = e.m;
        for (DWORD r = 0; r < 256; r++)
            if (cp.Rs(r) != ((m.rsM[r >> 5] >> (r & 31) & 1u) != 0)) { Err("%s: render state %lu: the block %s it, C1's clean record %s", phase, r, cp.Rs(r) ? "holds" : "does not hold", cp.Rs(r) ? "does not" : "does"); break; }
        for (int i = 0; i < 21; i++)
            if (cp.samp[i] != m.sampM[i]) { Err("%s: sampler %d: the block holds states 0x%04X, C1's clean record 0x%04X", phase, i, cp.samp[i], m.sampM[i]); break; }
        if (cp.tex != m.texM) Err("%s: textures: the block holds 0x%06lX, C1's clean record 0x%06lX", phase, cp.tex, m.texM);
        if (cp.vb != m.vbM) Err("%s: streams: the block holds 0x%04lX, C1's clean record 0x%04lX", phase, cp.vb, m.vbM);
        if (cp.vs != m.vsM || cp.ps != m.psM || cp.ib != m.ibM || cp.vp != m.vpM || cp.sc != m.scM || cp.decl != (m.declForm != DT_SBD_NONE))
            Err("%s: the block holds vs %d ps %d ib %d vp %d sc %d decl %d, C1's clean record %d %d %d %d %d %d", phase, cp.vs, cp.ps, cp.ib, cp.vp, cp.sc, cp.decl,
                m.vsM, m.psM, m.ibM, m.vpM, m.scM, m.declForm != DT_SBD_NONE);
        for (int w = 0; w < 7; w++) if (cp.psc[w] != m.pscM[w]) { Err("%s: pixel constants %d..%d: the block holds 0x%08lX, C1's clean record 0x%08lX", phase, w * 32, w * 32 + 31, cp.psc[w], m.pscM[w]); break; }
        if (!e.valsOk) continue;
        gOrVals++;
        for (DWORD r = 0; r < 256; r++) if ((m.rsM[r >> 5] >> (r & 31) & 1u) && m.rs[r] != b->s.rs[r]) { Err("%s: render state %lu: the block holds 0x%lX, C1's record 0x%lX", phase, r, b->s.rs[r], m.rs[r]); break; }
        for (int i = 0; i < 21; i++) for (DWORD t = 1; t <= 13; t++)
            if ((m.sampM[i] >> t & 1u) && m.samp[i][t] != b->s.samp[i][t]) { Err("%s: sampler %d state %lu: the block holds 0x%lX, C1's record 0x%lX", phase, i, t, b->s.samp[i][t], m.samp[i][t]); i = 21; break; }
        for (int i = 0; i < 21; i++) if ((m.texM >> i & 1u) && m.tex[i] != b->s.tex[i]) { Err("%s: texture %d: the block holds %p, C1's record %p", phase, i, (void*)b->s.tex[i], (void*)m.tex[i]); break; }
        for (int i = 0; i < 16; i++)
            if ((m.vbM >> i & 1u) && (m.vb[i] != b->s.vb[i] || m.vbOff[i] != b->s.off[i] || m.vbStr[i] != b->s.str[i]))
            { Err("%s: stream %d: the block holds %p +%u /%u, C1's record %p +%u /%u", phase, i, (void*)b->s.vb[i], b->s.off[i], b->s.str[i], (void*)m.vb[i], m.vbOff[i], m.vbStr[i]); break; }
        if ((m.vsM && m.vs != b->s.vs) || (m.psM && m.ps != b->s.ps) || (m.ibM && m.ib != b->s.ib)) Err("%s: a shader or the indices: C1's record differs from the block", phase);
        if (m.vpM && memcmp(&m.vp, &b->s.vp, sizeof(m.vp))) Err("%s: the viewport: C1's record Z %.4f to %.4f, the block %.4f to %.4f", phase, m.vp.MinZ, m.vp.MaxZ, b->s.vp.MinZ, b->s.vp.MaxZ);
        if (m.scM && memcmp(&m.sc, &b->s.sc, sizeof(m.sc))) Err("%s: the scissor: C1's record differs from the block", phase);
        if (m.declForm == DT_SBD_OBJ && (m.decl != b->s.decl || (m.fvfOk && (!b->s.decl || ((const FakeDecl*)b->s.decl)->fvf != m.fvf))))
            Err("%s: the declaration: C1's record %p (FVF %s 0x%lX), the block %p", phase, (void*)m.decl, m.fvfOk ? "known" : "unknown", m.fvf, (void*)b->s.decl);
        if (m.declForm == DT_SBD_FVF && (IDirect3DVertexDeclaration9*)FkFvfFind(m.fvf) != b->s.decl)
            Err("%s: the declaration: C1's record says the table's for FVF 0x%lX, the block holds %p", phase, m.fvf, (void*)b->s.decl);
        for (int w = 0; w < 7; w++)
            for (DWORD x = m.pscM[w] & m.pscK[w]; x; x &= x - 1)
            { DWORD bit; _BitScanForward(&bit, x); if (memcmp(m.psc[w * 32 + bit], b->s.psc[w * 32 + bit], 16)) { Err("%s: pixel constant %lu: C1's record differs from the block", phase, w * 32 + bit); w = 7; break; } }
        if (e.caps)
        {
            gOrCapVals++;
            if (m.vbM) gOrCapStream++;
            if (m.vpM) gOrCapVp++;
            if (m.declForm == DT_SBD_OBJ) gOrCapDeclObj++; else if (m.declForm == DT_SBD_FVF) gOrCapDeclFvf++; else if (m.declForm == DT_SBD_UNKNOWN) gOrCapDeclUnk++;
            for (int w = 0; w < 7; w++) if (m.pscK[w]) { gOrCapPsc++; break; }
        }
        // THE WRITER, against the fake's own Apply on the same device state, FROM A RANDOM SHADOW (review of 2026-09-29): each
        // entry known or not (an unknown one holding garbage), the declaration in any of its states, the pixel constants followed
        // or not. After the writer every entry the record covers must be known with the value the device's own Apply gives, the
        // declaration must be in the state its form says, and every entry it does not cover must be exactly as it was.
        {
            FkState exp = gFd->st; FkApplyTo(b, exp);
            const FkState& dv = gFd->st;
            DtShadow* const saved = new DtShadow(dtSh); const bool savedC = dtShConsts;
            memset(&dtSh, 0, sizeof(dtSh));
            auto kn = [&]() { return (OrRnd() & 3u) != 0; };
            for (DWORD r = 7; r <= 209; r++) { dtSh.rsOk[r] = kn(); dtSh.rs[r] = dtSh.rsOk[r] ? dv.rs[r] : OrRnd(); }
            for (int i = 0; i < 21; i++)
            {
                for (DWORD t = 1; t <= 13; t++) { dtSh.sampOk[i][t] = kn(); dtSh.samp[i][t] = dtSh.sampOk[i][t] ? dv.samp[i][t] : OrRnd(); }
                dtSh.texOk[i] = kn(); dtSh.tex[i] = dtSh.texOk[i] ? dv.tex[i] : (IDirect3DBaseTexture9*)(UINT_PTR)(OrRnd() | 1u);
            }
            for (int i = 0; i < 16; i++)
            {
                dtSh.vbOk[i] = kn();
                if (dtSh.vbOk[i]) { dtSh.vb[i] = dv.vb[i]; dtSh.vbOff[i] = dv.off[i]; dtSh.vbStr[i] = dv.str[i]; }
                else { dtSh.vb[i] = (IDirect3DVertexBuffer9*)(UINT_PTR)(OrRnd() | 1u); dtSh.vbOff[i] = OrRnd(); dtSh.vbStr[i] = OrRnd(); }
            }
            dtSh.vsOk = kn(); dtSh.vs = dtSh.vsOk ? dv.vs : (IDirect3DVertexShader9*)(UINT_PTR)(OrRnd() | 1u);
            dtSh.psOk = kn(); dtSh.ps = dtSh.psOk ? dv.ps : (IDirect3DPixelShader9*)(UINT_PTR)(OrRnd() | 1u);
            dtSh.ibOk = kn(); dtSh.ib = dtSh.ibOk ? dv.ib : (IDirect3DIndexBuffer9*)(UINT_PTR)(OrRnd() | 1u);
            dtSh.vpOk = kn(); if (dtSh.vpOk) dtSh.vp = dv.vp; else { dtSh.vp.X = OrRnd(); dtSh.vp.MinZ = 0.25f; dtSh.vp.MaxZ = 0.75f; }
            dtSh.scOk = kn(); if (dtSh.scOk) dtSh.sc = dv.sc; else dtSh.sc.left = (LONG)OrRnd();
            switch (OrRnd() % 3)
            {
            case 0: dtSh.decl = dv.decl; dtSh.declOk = true; if (dv.decl && (OrRnd() & 1u)) { dtSh.fvf = ((const FakeDecl*)dv.decl)->fvf; dtSh.fvfOk = true; } break;
            case 1: dtSh.declOk = false; dtSh.declQueued = true; dtSh.fvf = dv.decl ? ((const FakeDecl*)dv.decl)->fvf : 0x152u; dtSh.fvfOk = true; break;
            default: dtSh.declOk = false; dtSh.declQueued = false; dtSh.fvfOk = false; dtSh.decl = (IDirect3DVertexDeclaration9*)(UINT_PTR)(OrRnd() | 1u); break;
            }
            for (int w = 0; w < 7; w++) dtSh.pscOk[w] = OrRnd();
            for (UINT r = 0; r < 224; r++) { if (dtSh.pscOk[r >> 5] >> (r & 31) & 1u) memcpy(dtSh.psc[r], dv.psc[r], 16); else { dtSh.psc[r][0] = (float)OrRnd(); } }
            dtShConsts = (OrRnd() & 1u) != 0;
            const DtShadow start = dtSh; const bool consts = dtShConsts;
            DtSbMirWrite(m);
            char why[240] = "";
            for (DWORD r = 7; r <= 209 && !why[0]; r++)
            {
                const bool cov = (m.rsM[r >> 5] >> (r & 31) & 1u) != 0;
                if (cov ? (!dtSh.rsOk[r] || dtSh.rs[r] != exp.rs[r]) : (dtSh.rsOk[r] != start.rsOk[r] || dtSh.rs[r] != start.rs[r])) sprintf_s(why, "render state %lu (%s)", r, cov ? "covered" : "not covered");
            }
            for (int i = 0; i < 21 && !why[0]; i++)
            {
                for (DWORD t = 1; t <= 13 && !why[0]; t++)
                {
                    const bool cov = (m.sampM[i] >> t & 1u) != 0;
                    if (cov ? (!dtSh.sampOk[i][t] || dtSh.samp[i][t] != exp.samp[i][t]) : (dtSh.sampOk[i][t] != start.sampOk[i][t] || dtSh.samp[i][t] != start.samp[i][t]))
                        sprintf_s(why, "sampler %d state %lu (%s)", i, t, cov ? "covered" : "not covered");
                }
                const bool covT = (m.texM >> i & 1u) != 0;
                if (!why[0] && (covT ? (!dtSh.texOk[i] || dtSh.tex[i] != exp.tex[i]) : (dtSh.texOk[i] != start.texOk[i] || dtSh.tex[i] != start.tex[i]))) sprintf_s(why, "texture %d", i);
            }
            for (int i = 0; i < 16 && !why[0]; i++)
            {
                const bool cov = (m.vbM >> i & 1u) != 0;
                if (cov ? (!dtSh.vbOk[i] || dtSh.vb[i] != exp.vb[i] || dtSh.vbOff[i] != exp.off[i] || dtSh.vbStr[i] != exp.str[i])
                        : (dtSh.vbOk[i] != start.vbOk[i] || dtSh.vb[i] != start.vb[i] || dtSh.vbOff[i] != start.vbOff[i] || dtSh.vbStr[i] != start.vbStr[i]))
                    sprintf_s(why, "stream %d (%p +%u /%u against the device's %p +%u /%u)", i, (void*)dtSh.vb[i], dtSh.vbOff[i], dtSh.vbStr[i], (void*)exp.vb[i], exp.off[i], exp.str[i]);
            }
            auto one = [&](bool cov, bool ok, bool ok0, const void* v, const void* want, const void* v0, const char* what) {
                if (!why[0] && (cov ? (!ok || v != want) : (ok != ok0 || v != v0))) sprintf_s(why, "%s", what); };
            one(m.vsM, dtSh.vsOk, start.vsOk, dtSh.vs, exp.vs, start.vs, "the vertex shader");
            one(m.psM, dtSh.psOk, start.psOk, dtSh.ps, exp.ps, start.ps, "the pixel shader");
            one(m.ibM, dtSh.ibOk, start.ibOk, dtSh.ib, exp.ib, start.ib, "the indices");
            if (!why[0] && (m.vpM ? (!dtSh.vpOk || memcmp(&dtSh.vp, &exp.vp, sizeof(exp.vp))) : (dtSh.vpOk != start.vpOk || memcmp(&dtSh.vp, &start.vp, sizeof(exp.vp)))))
                sprintf_s(why, "the viewport (Z %.4f to %.4f against the device's %.4f to %.4f)", dtSh.vp.MinZ, dtSh.vp.MaxZ, exp.vp.MinZ, exp.vp.MaxZ);
            if (!why[0] && (m.scM ? (!dtSh.scOk || memcmp(&dtSh.sc, &exp.sc, sizeof(exp.sc))) : (dtSh.scOk != start.scOk || memcmp(&dtSh.sc, &start.sc, sizeof(exp.sc)))))
                sprintf_s(why, "the scissor");
            if (!why[0])
            {
                const bool same = dtSh.declOk == start.declOk && dtSh.declQueued == start.declQueued && dtSh.fvfOk == start.fvfOk
                                  && (!start.declOk || dtSh.decl == start.decl) && (!start.fvfOk || dtSh.fvf == start.fvf);
                switch (m.declForm)
                {
                case DT_SBD_NONE: if (!same) sprintf_s(why, "the declaration, not covered, changed"); break;
                case DT_SBD_OBJ:
                    if (!m.decl) { if (!same) sprintf_s(why, "the declaration: a null one was applied"); }
                    else if (!dtSh.declOk || dtSh.declQueued || dtSh.decl != exp.decl || dtSh.fvfOk != m.fvfOk || (m.fvfOk && dtSh.fvf != m.fvf)) sprintf_s(why, "the declaration (%p against the device's %p)", (void*)dtSh.decl, (void*)exp.decl);
                    break;
                case DT_SBD_FVF:
                    if (dtSh.declOk ? (dtSh.decl != exp.decl || !dtSh.fvfOk || dtSh.fvf != m.fvf)
                                    : (!dtSh.declQueued || !dtSh.fvfOk || dtSh.fvf != m.fvf || (IDirect3DVertexDeclaration9*)FkFvfFind(m.fvf) != exp.decl)) sprintf_s(why, "the declaration, as an FVF");
                    break;
                default: if (dtSh.declOk || dtSh.declQueued || dtSh.fvfOk) sprintf_s(why, "the declaration, left known though the record does not know it"); break;
                }
            }
            for (UINT r = 0; r < 224 && !why[0]; r++)
            {
                const bool known = (dtSh.pscOk[r >> 5] >> (r & 31) & 1u) != 0, known0 = (start.pscOk[r >> 5] >> (r & 31) & 1u) != 0;
                const bool cov = (m.pscM[r >> 5] >> (r & 31) & 1u) != 0, kv = (m.pscK[r >> 5] >> (r & 31) & 1u) != 0;
                if (!consts || !cov) { if (known != known0 || memcmp(dtSh.psc[r], start.psc[r], 16)) sprintf_s(why, "pixel constant %u, not written, changed", r); }
                else if (!kv) { if (known) sprintf_s(why, "pixel constant %u, unknown in the record, left known", r); }
                else if (!known || memcmp(dtSh.psc[r], exp.psc[r], 16)) sprintf_s(why, "pixel constant %u", r);
            }
            dtSh = *saved; delete saved; dtShConsts = savedC;
            if (why[0]) Err("%s: C1's writer on the record of %p differs from the device's own Apply: %s", phase, e.sb, why);
            gOrWrite++;
        }
    }
    ReleaseSRWLockShared(&dtSbLock);
    for (const auto& kv : gRefBlk)
        {
            if (gOrSkip.count(kv.first)) continue;          // left without a record by a full table, on purpose
            bool found = false;
            for (int k = 0; k < DT_SB_CAP && !found; k++) found = dtSbTab[k].kind != DT_SBK_FREE && dtSbTab[k].sb == kv.first;
            if (!found) { Err("%s: the live state block %p has no record in C1's table, and was not left out by a full one", phase, kv.first); break; }
            gOrLive++;
        }
    for (unsigned i = 0; i < dtShFvfObjN; i++, gOrFvf++)
        if ((IDirect3DVertexDeclaration9*)FkFvfFind(dtShFvfObj[i].fvf) != dtShFvfObj[i].obj)
            Err("%s: C1-F learned %p for FVF 0x%lX, the device's table holds %p", phase, (void*)dtShFvfObj[i].obj, dtShFvfObj[i].fvf, (void*)FkFvfFind(dtShFvfObj[i].fvf));
}
static bool SbRecordValsOk(const void* sb)
{
    AcquireSRWLockShared(&dtSbLock);
    bool ok = false;
    for (int k = 0; k < DT_SB_CAP; k++) if (dtSbTab[k].kind != DT_SBK_FREE && dtSbTab[k].sb == sb) { ok = dtSbTab[k].valsOk; break; }
    ReleaseSRWLockShared(&dtSbLock);
    return ok;
}
// A record whose values the case knows are known (the Capture copied them), or not.
static void SbExpectValues(IDirect3DStateBlock9* sb, bool known, const char* what)
{
    AcquireSRWLockShared(&dtSbLock);
    const DtSbEnt* e = nullptr;
    for (int k = 0; k < DT_SB_CAP; k++) if (dtSbTab[k].kind != DT_SBK_FREE && dtSbTab[k].sb == sb) { e = &dtSbTab[k]; break; }
    const bool got = e && e->valsOk;
    ReleaseSRWLockShared(&dtSbLock);
    if (got != known) Err("%s: C1's record's values are %s, the case expects them %s", what, got ? "known" : "not known", known ? "known" : "not known");
}
// A recording the case knows was made clean (no other thread, nothing refused, the layer's arm unchanged), and its opposite.
static void SbExpectRecord(IDirect3DStateBlock9* sb, LONG taint, const char* what)
{
    AcquireSRWLockShared(&dtSbLock);
    const DtSbEnt* e = nullptr;
    for (int k = 0; k < DT_SB_CAP; k++) if (dtSbTab[k].kind != DT_SBK_FREE && dtSbTab[k].sb == sb) { e = &dtSbTab[k]; break; }
    const LONG got = e ? e->taint : -1; const bool rec = e && e->kind == DT_SBK_REC;
    ReleaseSRWLockShared(&dtSbLock);
    if (!rec) Err("%s: C1's table has no recorded record of the block", what);
    else if (got != taint) Err("%s: C1's record of the block is \"%s\", not \"%s\"", what, kDtSbTaintName[got], kDtSbTaintName[taint]);
}

// STAGE 2.0'S OBSERVERS, NOTES AND HANDOFF (2026-09-14), proved here because the shipped
// mod installs none of them yet. The observers on SetRenderState (57) and
// DrawIndexedPrimitive (82) count what the worker replays, and Compare() holds each
// count against the fake device's own record of calls it received from the worker for
// that slot. The note observer checks every test note's sequence and bytes. The Present
// observer writes the last marker note it saw into the handoff, and Present() below
// reads the previous present's handoff after its pacing wait and checks the marker.
static long  gObsRs = 0, gObsDip = 0, gObsBad = 0;
static long  gNotesSeen = 0, gNotesBad = 0, gHandoffChecked = 0;
static DWORD gNoteLastSeq = 0, gObsLastMarker = 0;
static void ObsRs(const DWORD* c)  { if (DT_SLOT_OF(c[0]) != 57 || (c[0] & 0xFF) != DT_OP_CALL0 + 2) gObsBad++; gObsRs++; }
static void ObsDip(const DWORD* c) { if (DT_SLOT_OF(c[0]) != 82 || (c[0] & 0xFF) != DT_OP_CALL0 + 6 || (c[0] >> 16) != 7) gObsBad++; gObsDip++; }
static void ObsNote(DWORD kind, const DWORD* p, DWORD bytes)
{
    if (kind != DT_NOTE_TEST) { gNotesBad++; return; }
    if (bytes == 8 && p[0] == 0xFFFFFFFFu) { gObsLastMarker = p[1]; return; }   // a present marker (see Present)
    if (bytes < 4) { gNotesBad++; return; }
    const DWORD seq = p[0];
    if (seq != gNoteLastSeq + 1) gNotesBad++;
    gNoteLastSeq = seq;
    const BYTE* b = (const BYTE*)(p + 1);
    for (DWORD i = 0; i < bytes - 4; i++) if (b[i] != (BYTE)(seq * 0x9E3779B9u + i)) { gNotesBad++; break; }
    gNotesSeen++;
}
static void ObsPresent(LONG present) { const DWORD w[2] = { gObsLastMarker, (DWORD)gObsRs }; DtHandoffWrite(present, w, 2); }

// Every armed Present is preceded by a marker note carrying a fresh number, and after
// the Present returns the handoff for the PREVIOUS present must carry the marker that
// preceded that one: complete, and the right frame's.
// AND THE ARM IT WAS MARKED IN (review 2026-09-14): the ordinals keep counting across
// a disarm and re-arm, so the first armed Present after a Reset asks for the last
// pre-Reset ordinal, which the layer must now refuse (DtMaybeArm stamps the slots
// -1). A mark from an earlier arm therefore asserts the OPPOSITE: no handoff.
static DWORD gMarker = 0, gMarkerOf[2] = {}; static LONG gMarkedPres[2] = { -1, -1 }, gMarkedArm[2] = { -1, -1 };
static long  gHandoffRefusedAcrossArm = 0;
static void Present()
{
    const bool armed = dtArmed && GetCurrentThreadId() == dtProducer;
    if (armed)
    {
        const DWORD m[2] = { 0xFFFFFFFFu, ++gMarker };
        if (DtNote(DT_NOTE_TEST, m, 8)) { const LONG k = dtPresRecN + 1; gMarkedPres[k & 1] = k; gMarkerOf[k & 1] = gMarker; gMarkedArm[k & 1] = dtArmCount; }
        else Err("a marker note was refused while armed");
    }
    SC->Present(nullptr, nullptr, nullptr, nullptr, 0); E(1803, { 0u, 0u }); gPresents++;
    if (armed)
    {
        const LONG k = dtPresRecN, prev = k - 1;
        if (gMarkedPres[prev & 1] == prev)
        {
            const DtHandoff* h = DtHandoffFor(prev);
            if (gMarkedArm[prev & 1] != dtArmCount) { if (h) Err("the handoff for present %ld outlived a disarm and re-arm", prev); else gHandoffRefusedAcrossArm++; }
            else if (!h) Err("no handoff for present %ld once present %ld had returned", prev, k);
            else if (h->words[0] != gMarkerOf[prev & 1]) Err("the handoff for present %ld carries marker %lu, not %lu", prev, h->words[0], gMarkerOf[prev & 1]);
            else gHandoffChecked++;
        }
    }
}
static void Drain()   { D->TestCooperativeLevel(); E(3, {}); }
static IDirect3DTexture9* NewTex()
{
    const UINT w = 64 + (Rnd() % 4) * 16, h = 32;
    IDirect3DTexture9* t = nullptr;
    D->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr);
    E(23, { w, h, 1u, 0u, (DWORD)D3DFMT_A8R8G8B8, (DWORD)D3DPOOL_MANAGED });
    return t;
}
static IDirect3DVertexBuffer9* NewVb()
{
    IDirect3DVertexBuffer9* v = nullptr;
    D->CreateVertexBuffer(4096, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &v, nullptr);
    E(26, { 4096u, (DWORD)(D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY), 0u, (DWORD)D3DPOOL_DEFAULT });
    return v;
}

// A Get answered by the shadow, against the reference. Every returned
// reference is released again, which is itself a deferred Release, and the
// count it returns must be the native one: the test holds one reference on
// every pooled object, and two on the texture behind gLevel (itself and the
// level), so that is what is left once the Get's own reference goes.
static IDirect3DSurface9* gLevel = nullptr;
static void Rel(IUnknown* o)
{
    if (!o) return;
    const ULONG want = (o == (IUnknown*)gLevel) ? 2u : 1u, got = o->Release();
    if (got != want) Err("a Release after a Get returned %lu, not %lu", got, want);
}
static const DWORD kXfType[14] = { D3DTS_VIEW, D3DTS_PROJECTION, D3DTS_TEXTURE0, D3DTS_TEXTURE1, D3DTS_TEXTURE2, D3DTS_TEXTURE3,
    D3DTS_TEXTURE4, D3DTS_TEXTURE5, D3DTS_TEXTURE6, D3DTS_TEXTURE7, 256, 257, 258, 259 };   // FkXf's order
static long gSyncKindGets = 0;
static void OneGet()
{
    gGets++;
    switch (Rnd() % 15)
    {
    // THE FOUR KINDS THE SHADOW DOES NOT ANSWER (2026-09-28, C1 step 2): each a SYNC, so it drains, and reads the device.
    case 14: { gSyncKindGets++;
               switch (Rnd() % 4)
               {
               case 0: { const DWORD g = Rnd() % 9, t = 1 + Rnd() % 32; DWORD v = 0xDEADu; D->GetTextureStageState(g, (D3DTEXTURESTAGESTATETYPE)t, &v); E(66, { g, t });
                         const DWORD want = R.tss[g < 7 ? g : 7][FkTssIdx(t)];
                         if (v != want) Err("GetTextureStageState(%lu, %lu) = 0x%08lX, the reference says 0x%08lX", g, t, v, want); break; }
               case 1: { const int k = (int)(Rnd() % 14); D3DMATRIX m; D->GetTransform((D3DTRANSFORMSTATETYPE)kXfType[k], &m); E(45, { kXfType[k] });
                         if (memcmp(&m, &R.xf[k], 64)) Err("GetTransform(%lu) differs from the reference", kXfType[k]); break; }
               case 2: { const UINT i = Rnd() % 16; UINT f = 0xDEADu; D->GetStreamSourceFreq(i, &f); E(103, { i });
                         if (f != R.freq[i]) Err("GetStreamSourceFreq(%u) = 0x%X, the reference says 0x%X", i, f, R.freq[i]); break; }
               default: { const UINT st = Rnd() % 256, mx = 256 - st, n = 1 + Rnd() % (mx < 8 ? mx : 8); float c[8 * 4];
                          D->GetVertexShaderConstantF(st, c, n); E(95, { st, n });
                          if (memcmp(c, R.vsc[st], n * 16)) Err("GetVertexShaderConstantF(%u, %u) differs from the reference", st, n); break; }
               }
               break; }
    // The three the shadow took over on 2026-09-19. The FVF is the reference's BOUND OBJECT's.
    case 11: { gDeclGets++; IDirect3DVertexDeclaration9* d = nullptr; D->GetVertexDeclaration(&d);
               if (d != R.decl) Err("GetVertexDeclaration = %p, the reference says %p", (void*)d, (void*)R.decl); Rel(d); break; }
    case 12: { gFvfGets++; DWORD f = 0xDEADu; D->GetFVF(&f); if (f != R.Fvf()) Err("GetFVF = 0x%lX, the reference says 0x%lX (declaration %p)", f, R.Fvf(), (void*)R.decl); break; }
    case 13: { gPscGets++; const UINT s = Rnd() % 224, mx = 224 - s, n = 1 + Rnd() % (mx < 9 ? mx : 9); float c[9 * 4];
               D->GetPixelShaderConstantF(s, c, n);
               if (memcmp(c, R.psc[s], n * 16)) Err("GetPixelShaderConstantF(%u, %u) differs from the reference", s, n); break; }
    case 0: { const DWORD s = 7 + Rnd() % 203; DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)s, &v);
              if (v != R.rs[s]) Err("GetRenderState(%lu) = 0x%08lX, the reference says 0x%08lX", s, v, R.rs[s]); break; }
    case 1: { const int i = Rnd() % 21; const DWORD t = 1 + Rnd() % 13; DWORD v = 0; D->GetSamplerState(SampNum(i), (D3DSAMPLERSTATETYPE)t, &v);
              if (v != R.samp[i][t]) Err("GetSamplerState(%d,%lu) differs", i, t); break; }
    case 2: { const int i = Rnd() % 21; IDirect3DBaseTexture9* t = nullptr; D->GetTexture(SampNum(i), &t);
              if (t != R.tex[i]) Err("GetTexture(%d) = %p, the reference says %p", i, (void*)t, (void*)R.tex[i]); Rel(t); break; }
    case 3: { const DWORD i = Rnd() % 4; IDirect3DSurface9* s = nullptr; const HRESULT hr = D->GetRenderTarget(i, &s);
              if (s != R.rt[i] || hr != (R.rt[i] ? D3D_OK : D3DERR_NOTFOUND)) Err("GetRenderTarget(%lu) differs", i); Rel(s); break; }
    case 4: { IDirect3DSurface9* s = nullptr; const HRESULT hr = D->GetDepthStencilSurface(&s);
              if (s != R.ds || hr != (R.ds ? D3D_OK : D3DERR_NOTFOUND)) Err("GetDepthStencilSurface differs"); Rel(s); break; }
    case 5: { D3DVIEWPORT9 v; D->GetViewport(&v); if (memcmp(&v, &R.vp, sizeof(v))) Err("GetViewport differs (%lux%lu against %lux%lu)", v.Width, v.Height, R.vp.Width, R.vp.Height); break; }
    case 6: { RECT r; D->GetScissorRect(&r); if (memcmp(&r, &R.sc, sizeof(r))) Err("GetScissorRect differs"); break; }
    case 7: { IDirect3DVertexShader9* s = nullptr; D->GetVertexShader(&s); if (s != R.vs) Err("GetVertexShader differs"); Rel(s); break; }
    case 8: { IDirect3DPixelShader9* s = nullptr; D->GetPixelShader(&s); if (s != R.ps) Err("GetPixelShader differs"); Rel(s); break; }
    case 9: { const UINT i = Rnd() % 16; IDirect3DVertexBuffer9* v = nullptr; UINT o = 0, st = 0; D->GetStreamSource(i, &v, &o, &st);
              if (v != R.vb[i] || o != R.off[i] || st != R.str[i]) Err("GetStreamSource(%u) differs", i); Rel(v); break; }
    default: { IDirect3DIndexBuffer9* b = nullptr; D->GetIndices(&b); if (b != R.ib) Err("GetIndices differs"); Rel(b); break; }
    }
}

// THE BOUND OBJECT DIES: bound and drawn with (both recorded), then the test's
// only reference goes; every binding of it is replaced after that. The fake
// device must see the bind and the draw with the object alive.
static void KillBoundTexture()
{
    const size_t k = Rnd() % gTex.size();
    IDirect3DTexture9* T = gTex[k];
    const int i = Rnd() % 21;
    D->SetTexture(SampNum(i), T); E(65, { SampNum(i), P(T) }); R.tex[i] = T;
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    const ULONG left = T->Release();
    if (left != 0) Err("the last Release of a texture returned %lu, not 0", left);
    for (int j = 0; j < 21; j++)
        if (R.tex[j] == (IDirect3DBaseTexture9*)T) { D->SetTexture(SampNum(j), nullptr); E(65, { SampNum(j), 0u }); R.tex[j] = nullptr; }
    gTex[k] = NewTex();
}
static void KillBoundBuffer()
{
    const size_t k = Rnd() % gVb.size();
    IDirect3DVertexBuffer9* V = gVb[k];
    D->SetStreamSource(0, V, 0, 16); E(100, { 0u, P(V), 0u, 16u }); R.Stream(0, V, 0, 16);
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    if (V->Release() != 0) Err("the last Release of a buffer did not return 0");
    for (UINT j = 0; j < 16; j++)
        if (R.vb[j] == V) { D->SetStreamSource(j, nullptr, 0, 0); E(100, { j, 0u, 0u, 0u }); R.Stream(j, nullptr, 0, 0); }
    gVb[k] = NewVb();
}

// A DECLARATION DIES AND ITS ADDRESS COMES BACK AS ANOTHER OBJECT WITH ANOTHER FVF. The
// layer remembers an FVF per declaration OBJECT, keyed by address, so this is the one way
// it could answer for the wrong object: bound and asked about (so it is remembered), its
// last reference released, then a new declaration made in the same memory with a
// different FVF, bound, and asked about. The answer must be the new object's.
static const DWORD kDeclFvf[4] = { 0u, D3DFVF_XYZ | D3DFVF_NORMAL, D3DFVF_XYZRHW | D3DFVF_TEX2, D3DFVF_XYZ | D3DFVF_TEX1 };
static void KillBoundDecl()
{
    const size_t k = Rnd() % gDecl.size();
    IDirect3DVertexDeclaration9* A = gDecl[k];
    const DWORD was = ((FakeDecl*)A)->fvf;
    D->SetVertexDeclaration(A); E(87, { P(A) }); R.decl = A;
    DWORD f = 0xDEADu; D->GetFVF(&f); gFvfGets++; if (f != was) Err("GetFVF of a bound declaration = 0x%lX, not its own 0x%lX", f, was);
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    D->SetVertexDeclaration(nullptr); E(87, { 0u }); R.decl = nullptr;      // a freed object is bound nowhere
    if (A->Release() != 0) Err("the last Release of a declaration did not return 0");
    static const D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    DWORD now = kDeclFvf[Rnd() % 4]; if (now == was) now = was ^ D3DFVF_DIFFUSE;      // always a different one
    gFkReuseDecl = (FakeDecl*)A; gFkNextDeclFvf = now;
    IDirect3DVertexDeclaration9* B = nullptr; D->CreateVertexDeclaration(el, &B); E(86, {});
    if (B != A) Err("the fake did not hand the declaration's memory back"); else gDeclReused++;
    gDecl[k] = B;
    D->SetVertexDeclaration(B); E(87, { P(B) }); R.decl = B;
    f = 0xDEADu; D->GetFVF(&f); gFvfGets++;
    if (f != now) Err("THE SAME ADDRESS, A NEW DECLARATION: GetFVF = 0x%lX, the new object's is 0x%lX (the dead one's was 0x%lX)", f, now, was);
}

// A LEVEL AND ITS TEXTURE SHARE ONE COUNT (DXVK forwards a level's to its texture),
// so each Release of either must see the pair still queued against the other.
// Released in both orders, with a queued call still naming the texture.
static void KillLevelAndTexture()
{
    const size_t k = Rnd() % gTex.size();
    IDirect3DTexture9* T = gTex[k];
    IDirect3DSurface9* L = nullptr; T->GetSurfaceLevel(0, &L);           // the texture's count is 2 now
    const int i = Rnd() % 21;
    D->SetTexture(SampNum(i), T); E(65, { SampNum(i), P(T) }); R.tex[i] = T;
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    ULONG a, b;
    if (Rnd() & 1) { a = L->Release(); b = T->Release(); } else { a = T->Release(); b = L->Release(); }
    if (a != 1 || b != 0) Err("a level and its texture released to %lu then %lu, not 1 then 0", a, b);
    for (int j = 0; j < 21; j++)
        if (R.tex[j] == (IDirect3DBaseTexture9*)T) { D->SetTexture(SampNum(j), nullptr); E(65, { SampNum(j), 0u }); R.tex[j] = nullptr; }
    gTex[k] = NewTex();
}

static void OneOp(bool lifetime)
{
    static float fbuf[256 * 4]; static BYTE vbuf[4096]; static WORD ibuf[64];
    const unsigned op = Rnd() % 1000;
    if (op < 200) { const DWORD s = 7 + Rnd() % 203, v = Rnd(); D->SetRenderState((D3DRENDERSTATETYPE)s, v); E(57, { s, v }); RRs(s, v); }
    else if (op < 260) { const int i = Rnd() % 21; const DWORD t = 1 + Rnd() % 13, v = Rnd();
        D->SetSamplerState(SampNum(i), (D3DSAMPLERSTATETYPE)t, v); E(69, { SampNum(i), t, v }); RSamp(i, t, v); }
    else if (op < 320) { const int i = Rnd() % 21; IDirect3DTexture9* t = (Rnd() % 4) ? gTex[Rnd() % gTex.size()] : nullptr;
        D->SetTexture(SampNum(i), t); E(65, { SampNum(i), P(t) }); RTex(i, t); }
    else if (op < 360) { const UINT i = Rnd() % 4; IDirect3DVertexBuffer9* v = (Rnd() % 5) ? gVb[Rnd() % gVb.size()] : nullptr; const UINT o = (Rnd() % 64) * 4, s = 16 + (Rnd() % 4) * 4;
        D->SetStreamSource(i, v, o, s); E(100, { i, P(v), o, s }); RVb(i, v, o, s); }
    else if (op < 380) { IDirect3DIndexBuffer9* b = (Rnd() % 5) ? gIb[Rnd() % gIb.size()] : nullptr; D->SetIndices(b); E(104, { P(b) }); RIb(b); }
    else if (op < 400) { IDirect3DVertexShader9* s = gVs[Rnd() % gVs.size()]; D->SetVertexShader(s); E(92, { P(s) }); RVs(s); }
    else if (op < 420) { IDirect3DPixelShader9* s = gPs[Rnd() % gPs.size()]; D->SetPixelShader(s); E(107, { P(s) }); RPs(s); }
    else if (op < 435) { const DWORD i = Rnd() % 4; IDirect3DSurface9* s = (i == 0 || Rnd() % 2) ? gRt[Rnd() % gRt.size()] : nullptr;
        D->SetRenderTarget(i, s); E(37, { i, P(s) }); R.Rt(i, s); }          // even inside a state block: see dtSbRec
    else if (op < 440) { IDirect3DSurface9* s = (Rnd() % 3) ? gDs[Rnd() % gDs.size()] : nullptr; D->SetDepthStencilSurface(s); E(39, { P(s) }); R.ds = s; }
    else if (op < 455) { D3DVIEWPORT9 v = { Rnd() % 100, Rnd() % 100, 1 + Rnd() % 4000, 1 + Rnd() % 3000, 0.0f, 1.0f };
        switch (Rnd() % 8) { case 0: v.MinZ = 0.5f; v.MaxZ = 0.25f; break; case 1: v.MinZ = v.MaxZ = 0.75f; break; }   // DXVK lifts MaxZ
        D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }
    else if (op < 465) { RECT r = { (LONG)(Rnd() % 100), (LONG)(Rnd() % 100), (LONG)(200 + Rnd() % 3000), (LONG)(200 + Rnd() % 2000) };
        D->SetScissorRect(&r); E(75, { 1u }, &r, sizeof(r)); RSc(r); }
    // Counts: one in forty is ZERO (the wrapper must record it and write no payload), one in
    // four runs to the register limit (past the inline row copy's sixteen rows, into memcpy),
    // the rest are the one-to-sixteen rows the game mostly sends (sweep 2026-09-17, O4).
    else if (op < 600) { const UINT start = Rnd() % 256, max = 256 - start,
                                    n = (Rnd() % 40 == 0) ? 0u : 1 + Rnd() % ((Rnd() % 4) ? (max < 16 ? max : 16) : max);
        for (UINT k = 0; k < n * 4; k++) fbuf[k] = Rf();
        D->SetVertexShaderConstantF(start, fbuf, n); E(94, { start, 0u, n }, fbuf, n * 16); RVsc(start, fbuf, n); }
    else if (op < 700) { const UINT start = Rnd() % 224, max = 224 - start, n = 1 + Rnd() % (max < 8 ? max : 8);
        for (UINT k = 0; k < n * 4; k++) fbuf[k] = Rf();
        D->SetPixelShaderConstantF(start, fbuf, n); E(109, { start, 0u, n }, fbuf, n * 16); RPsc(start, fbuf, n); }
    else if (op < 710) { const UINT start = Rnd() % 16, n = 1 + Rnd() % (16 - start); BOOL b[16]; for (UINT k = 0; k < n; k++) b[k] = Rnd() & 1;
        D->SetPixelShaderConstantB(start, b, n); E(113, { start, 0u, n }, b, n * 4); }
    else if (op < 770) { const UINT pc = 1 + Rnd() % 500; const INT bv = (INT)(Rnd() % 100); const UINT si = Rnd() % 90;
        D->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, bv, 0, 64, si, pc); E(82, { (DWORD)D3DPT_TRIANGLELIST, (DWORD)bv, 0u, 64u, si, pc }); }
    else if (op < 790) { const UINT pc = 1 + Rnd() % 500, sv = Rnd() % 64;
        D->DrawPrimitive(D3DPT_TRIANGLESTRIP, sv, pc); E(81, { (DWORD)D3DPT_TRIANGLESTRIP, sv, pc }); }
    else if (op < 795) { const UINT pc = 1 + Rnd() % 8, nv = pc * 3; for (UINT k = 0; k < nv * 16; k++) vbuf[k] = (BYTE)Rnd();
        D->DrawPrimitiveUP(D3DPT_TRIANGLELIST, pc, vbuf, 16); E(83, { (DWORD)D3DPT_TRIANGLELIST, pc, 0u, 16u }, vbuf, nv * 16); R.Up(false); }
    else if (op < 800) { const UINT pc = 2, nv = 4; for (int k = 0; k < 6; k++) ibuf[k] = (WORD)(Rnd() % 4); for (UINT k = 0; k < nv * 16; k++) vbuf[k] = (BYTE)Rnd();
        D->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, nv, pc, ibuf, D3DFMT_INDEX16, vbuf, 16);
        E(84, { (DWORD)D3DPT_TRIANGLELIST, 0u, nv, pc, 0u, (DWORD)D3DFMT_INDEX16, 0u, 16u }, ibuf, 12, vbuf, nv * 16); R.Up(true); }
    else if (op < 810) { const DWORD n = Rnd() % 3; D3DRECT r[2] = { { 0, 0, 10, 10 }, { 5, 5, 50, 60 } }; const float z = Rf(); const DWORD c = Rnd();
        const D3DRECT* pr = (n || (Rnd() & 1)) ? r : nullptr;       // (0, a list) clears nothing; (0, NULL) the viewport
        D->Clear(n, pr, D3DCLEAR_TARGET, c, z, 0); E(43, { n, pr ? 1u : 0u, (DWORD)D3DCLEAR_TARGET, c, F(z), 0u }, pr, n * sizeof(D3DRECT)); }
    else if (op < 815) { IDirect3DSurface9* s = gRt[Rnd() % gRt.size()]; IDirect3DSurface9* t = gRt[Rnd() % gRt.size()]; RECT a = { 0, 0, 8, 8 };
        const bool hs = Rnd() & 1, ht = Rnd() & 1; D->StretchRect(s, hs ? &a : nullptr, t, ht ? &a : nullptr, D3DTEXF_LINEAR);
        E(34, { P(s), hs ? 1u : 0u, P(t), ht ? 1u : 0u, (DWORD)D3DTEXF_LINEAR }, hs ? &a : nullptr, hs ? sizeof(a) : 0, ht ? &a : nullptr, ht ? sizeof(a) : 0); }
    else if (op < 820) { D->BeginScene(); E(41, {}); D->EndScene(); E(42, {}); }
    // One in eight is SetFVF(0), which DXVK ignores; the rest bind the device's own table
    // declaration for the value, whose address the reference gets from the same table.
    else if (op < 830) { const DWORD f = (Rnd() % 8 == 0) ? 0u : (D3DFVF_XYZ | (Rnd() & 1 ? D3DFVF_DIFFUSE : 0) | (Rnd() & 1 ? D3DFVF_TEX1 : 0));
        D->SetFVF(f); E(89, { f }); if (f) RDecl(FkFvfDecl(f)); }
    else if (op < 840) { IDirect3DVertexDeclaration9* d = (Rnd() % 6) ? gDecl[Rnd() % gDecl.size()] : nullptr;
        D->SetVertexDeclaration(d); E(87, { P(d) }); RDecl(d); }
    else if (op < 845) { D3DMATRIX m; for (int k = 0; k < 16; k++) (&m._11)[k] = Rf(); const int x = (int)(Rnd() % 14); const DWORD t = kXfType[x];
        D->SetTransform((D3DTRANSFORMSTATETYPE)t, &m); E(44, { t, 1u }, &m, 64); RXf(x, m); }
    else if (op < 850) { IDirect3DVertexBuffer9* v = gVb[Rnd() % gVb.size()]; void* p = nullptr;
        switch (Rnd() % 6)
        {
        case 0: v->Lock(0, 0, &p, D3DLOCK_DISCARD); E(1011, { P(v), 0u, 0u, (DWORD)D3DLOCK_DISCARD }); break;
        case 1: v->Lock(0, 64, &p, D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE); break;       // DXVK drops the DISCARD: direct, unordered
        case 2: { const UINT o = Rnd() % 1024; v = gVbM; v->Lock(o, 256, &p, D3DLOCK_NOOVERWRITE);  // MANAGED: DXVK drops NOOVERWRITE, so it drains
                  E(1011, { P(v), o, 256u, (DWORD)D3DLOCK_NOOVERWRITE }); break; }
        default: v->Lock(Rnd() % 1024, 256, &p, D3DLOCK_NOOVERWRITE); break;             // direct, unordered: not logged
        }
        v->Unlock(); }
    else if (op < 853) { IDirect3DTexture9* t = gTex[Rnd() % gTex.size()]; D3DLOCKED_RECT lr; t->LockRect(0, &lr, nullptr, 0); E(1219, { P(t), 0u, 0u, 0u });
        t->UnlockRect(0); E(1220, { P(t), 0u }); }
    // A QUERY'S GetData WAITS FOR ITS OWN Issue AND NOTHING ELSE (2026-09-19). Here the Issue was
    // recorded a moment ago, so the ask has to wait for it: fewer Issues at the device than the
    // test has made means the answer was for the wrong Issue.
    else if (op < 856) { gQ->Issue(D3DISSUE_END); E(1706, { P(gQ), (DWORD)D3DISSUE_END }); gIssues++; DWORD v = 0; gQ->GetData(&v, 4, D3DGETDATA_FLUSH); gGetDatas++;
                         if (((FakeQuery*)gQ)->seenAtGet != gIssues) Err("GetData reached the device with %ld of its query's %ld Issues run", ((FakeQuery*)gQ)->seenAtGet, gIssues); }
    else if (op < 858) { D->GetAvailableTextureMem(); E(4, {}); }
    else if (op < 860) { UINT n = 16; float c[16 * 4]; D->GetVertexShaderConstantF(0, c, n); E(95, { 0u, n });
        if (memcmp(c, R.vsc[0], sizeof(c))) Err("GetVertexShaderConstantF(0, 16) differs from the reference"); }
    else if (op < 862 && lifetime) KillBoundTexture();
    else if (op < 864 && lifetime) KillBoundBuffer();
    else if (op < 866 && lifetime) KillLevelAndTexture();
    else if (op < 868 && lifetime) KillBoundDecl();
    // THE WRONG KIND OF SURFACE: DXVK refuses both and changes nothing, so the
    // reference stays as it is and the shadow must not follow either.
    // A REFUSED SetRenderTarget MUST NOT DRAIN (2026-09-19): all three of DXVK's refusals, each
    // answered at once with DXVK's own code, no wait counted, nothing in the ordered log, and
    // the reference untouched. One of these a frame was the first drain behind the seam.
    else if (op < 870) { if (Rnd() & 1) { IDirect3DSurface9* s = gDs[Rnd() % gDs.size()]; DWORD i = 1 + Rnd() % 3;
                             const DWORD kind = Rnd() % 3; if (kind == 1) { s = nullptr; i = 0; } else if (kind == 2) { s = gRt[Rnd() % gRt.size()]; i = 4 + Rnd() % 4; }
                             const LONG waits = dtWaitN[37]; const HRESULT hr = D->SetRenderTarget(i, s); gRefusedRtMade++;
                             if (hr != D3DERR_INVALIDCALL) Err("a refused SetRenderTarget(%lu) returned 0x%08lX", (unsigned long)i, (unsigned long)hr);
                             if (dtWaitN[37] != waits) Err("a refused SetRenderTarget drained the ring: DXVK decides it from the arguments alone"); }
                         else { IDirect3DSurface9* s = gRt[Rnd() % gRt.size()]; D->SetDepthStencilSurface(s); E(39, { P(s) }); } }
    else if (op < 876) { const UINT i = Rnd() % 4; UINT f;
        switch (Rnd() % 4) { case 0: f = 1; break; case 1: f = D3DSTREAMSOURCE_INDEXEDDATA | (1 + Rnd() % 8); break; case 2: f = D3DSTREAMSOURCE_INSTANCEDATA | 1; break; default: f = 0; }
        D->SetStreamSourceFreq(i, f); E(102, { i, f }); if (FkFreqOk(i, f)) RFreq(i, f); }
    else if (op < 882) { const UINT i = Rnd() % 4, o = (Rnd() % 64) * 4, s = 16 + (Rnd() % 4) * 4;
        D->SetStreamSource(i, nullptr, o, s); E(100, { i, 0u, o, s }); RVb(i, nullptr, o, s); }   // recorded raw; set, zeroed
    // UP DRAWS DXVK DOES NOT DRAW, AND NONSENSE ONES. A zero stride is refused and no
    // vertices is S_OK, both before any unbinding; a count and a stride whose product
    // wraps must not be copied. All reach the device as they are, drained.
    else if (op < 884) { switch (Rnd() % 4)
        {
        case 0: D->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vbuf, 0); E(83, { (DWORD)D3DPT_TRIANGLELIST, 1u, 0u, 0u }); break;
        case 1: D->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 0, 2, ibuf, D3DFMT_INDEX16, vbuf, 16);
                E(84, { (DWORD)D3DPT_TRIANGLELIST, 0u, 0u, 2u, 0u, (DWORD)D3DFMT_INDEX16, 0u, 16u }); break;
        case 2: { const UINT pc = 1431693603u, str = 0xFFFE449Au;
                  D->DrawPrimitiveUP(D3DPT_TRIANGLELIST, pc, vbuf, str); E(83, { (DWORD)D3DPT_TRIANGLELIST, pc, 0u, str }); R.Up(false); break; }
        default: { const UINT mi = 0xFFFFFFFFu, nv = 65538u, pc = 32770u, str = 0xFFFF0000u;
                   D->DrawIndexedPrimitiveUP(D3DPT_POINTLIST, mi, nv, pc, ibuf, D3DFMT_INDEX16, vbuf, str);
                   E(84, { (DWORD)D3DPT_POINTLIST, mi, nv, pc, 0u, (DWORD)D3DFMT_INDEX16, 0u, str }); R.Up(true); break; }
        } }
    // A stage state, the stage past 7 now and then (DXVK clamps it, and the type, rather than refuse).
    else if (op < 888) { const DWORD g = Rnd() % 9, t = 1 + Rnd() % 32, v = Rnd();
        D->SetTextureStageState(g, (D3DTEXTURESTAGESTATETYPE)t, v); E(67, { g, t, v }); RTss(g < 7 ? g : 7, FkTssIdx(t), v); }
    else if (op < 990) OneGet();
    else Present();
}

// NOTES: two thousand of them, interleaved with recorded calls, every payload length
// from empty to sixty bytes, then one larger than a command and one of an unknown
// kind, both refused and counted. The Compare after it proves the fake device saw
// none: the call stream is identical with them in it.
static void Notes()
{
    Drain();
    BYTE buf[4 + 64];
    for (DWORD seq = 1; seq <= 2000; seq++)
    {
        const DWORD len = seq % 61;
        memcpy(buf, &seq, 4);
        for (DWORD i = 0; i < len; i++) buf[4 + i] = (BYTE)(seq * 0x9E3779B9u + i);
        if (!DtNote(DT_NOTE_TEST, buf, 4 + len)) Err("note %lu was refused", seq);
        for (int k = 0; k < 3; k++) OneOp(false);
    }
    static BYTE big[(DT_MAX_CMD_DW + 8) * 4];
    const LONG refusedBefore = dtNoteRefused;
    if (DtNote(DT_NOTE_TEST, big, sizeof(big))) Err("a note larger than a command was accepted");
    if (DtNote(DT_NOTE_N, buf, 4)) Err("a note of an unknown kind was accepted");
    if (dtNoteRefused != refusedBefore + 2) Err("two refused notes were counted as %ld", dtNoteRefused - refusedBefore);
    Drain();
    if (gNotesSeen != 2000 || gNotesBad) Err("the worker saw %ld of 2000 notes in order and intact, %ld bad", gNotesSeen, gNotesBad);
}

// A NOTE WHOSE BODY DRIVES THE DEVICE (2026-09-19; dt_core.h, dtNoteDrives). The mod's
// injection body runs on the worker, binds its own state and puts it all back, so the
// stream is unchanged by it but THE DEVICE HOLDS THE BODY'S STATE WHILE IT RUNS. The
// shadow fetches an unknown entry from the device directly, which is exact only while no
// queued command names it, and such a note names what its body touches. Each round:
// every entry is made unknown (an Apply of the state as it stands), the note is posted,
// the test waits until the worker is INSIDE the body with the sentinel values on the
// device, and then asks for exactly the entries the body has changed. The answer must be
// the stream's, never the sentinel's; and a sampler slot the body is declared not to
// touch must be answered WITHOUT waiting for it, which is the whole point of the mask.
static volatile LONG gBodyIn = 0;
static long gBodyRounds = 0, gBodyGated = 0, gBodyFree = 0, gBodyKnown = 0, gBodyFreeRs = 0, gBodyKnownBound = 0;
static volatile LONG gBodyDirectRs = 0;      // SetRenderStates a body made itself: on the worker's thread, but not replayed, so not observed
static const DWORD kBodySentinel = 0x5E771AE1u;
static void ObsBody(DWORD kind, const DWORD* p, DWORD bytes)
{
    if (kind != DT_NOTE_INJECT || bytes != 16) { gNotesBad++; return; }
    // On the worker these go straight to the device, as the mod's own body's calls do.
    D->SetRenderState((D3DRENDERSTATETYPE)p[0], kBodySentinel); _InterlockedExchangeAdd(&gBodyDirectRs, 2);
    D->SetTexture(0, (IDirect3DBaseTexture9*)(UINT_PTR)p[2]);
    _InterlockedExchange(&gBodyIn, 1);
    // A REFERENCE TAKEN AND GIVEN BACK ON THE WORKER, through the vtable as the mod's own body code would (1.0.8): the
    // device's Release is the one slot of Windows' four that the worker may reach, and main() wants to have seen it there.
    D->AddRef(); D->Release();
    Sleep(3);                                   // long enough that the producer's asks land inside
    D->SetRenderState((D3DRENDERSTATETYPE)p[0], p[1]);
    D->SetTexture(0, (IDirect3DBaseTexture9*)(UINT_PTR)p[3]);
    _InterlockedExchange(&gBodyIn, 0);
}
static void Bodies()
{
    Drain();
    const LONG uf0 = dtSbUfSeq;     // C1's bracket: a body's Sets on the worker are the stream's, never another thread's (R8)
    const LONG bs0 = dtBodySets, bu0 = dtBodyUndeclared; const long br0 = gBodyRounds;   // the declarations measured (dt_core.h, DtBodyDecl)
    bool undeclSaid = false;        // said at the first body that sets one, ahead of the wrong answers it leads to (NC34)
    dtNoteObs[DT_NOTE_INJECT] = ObsBody; dtNoteDrives[DT_NOTE_INJECT] = true;
    dtBodySampFree = 0x1FFFFFu & ~(DT_SBM_FAULT(34) ? 0u : 1u);   // the body binds sampler slot 0 and no other; NC34 declares it free
    for (int round = 0; round < 90; round++)
    {
        IDirect3DStateBlock9* sb = SbCreate(D3DSBT_ALL);
        SbCapture(sb);
        SbApply(sb);                            // the state as it stands, so R is unchanged and every entry is unknown
        SbRelease(sb);
        const DWORD rs = 7 + Rnd() % 203;
        // THE BODY DECLARES THE ONE RENDER STATE IT SETS (2026-09-19), so the post fetches it
        // while nothing is out and it is KNOWN beside the body; every other one is free.
        memset(dtBodyRsTouched, 0, sizeof(dtBodyRsTouched)); dtBodyRsTouched[rs >> 5] |= 1u << (rs & 31);
        IDirect3DBaseTexture9* other = gTex[Rnd() % gTex.size()]; if (other == R.tex[0]) other = nullptr;
        const DWORD pay[4] = { rs, R.rs[rs], P(other), P(R.tex[0]) };
        if (R.rs[rs] == kBodySentinel) continue;
        // THE OWNER'S CASE: the query is issued AHEAD of the body, and its data is asked for while
        // the body runs. The ask needs only that Issue, which the worker has passed (it says
        // where it is before it starts a body), so it must not wait at all.
        gQ->Issue(D3DISSUE_END); E(1706, { P(gQ), (DWORD)D3DISSUE_END }); gIssues++;
        // what the body will send the device, in the order it sends it, at the note's place in the stream
        E(57, { rs, kBodySentinel }); E(65, { 0u, P(other) }); E(57, { rs, R.rs[rs] }); E(65, { 0u, P(R.tex[0]) });
        if (!DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) { Err("the body note was refused"); continue; }
        DtPublishFenced();
        for (int spin = 0; !gBodyIn && spin < 200000000; spin++) _mm_pause();
        if (!gBodyIn) { Err("the worker never entered the body"); continue; }
        gBodyRounds++;
        if (dtBodyUndeclared != bu0 && !undeclSaid) { undeclSaid = true; Err("the body declaration check counted %ld Sets outside the declarations (the first 0x%lX)", (long)(dtBodyUndeclared - bu0), (long)dtBodyUndeclFirst); }
        { const LONG hit = dtWaitHit[DT_R_QGETDATA]; DWORD v = 0; gQ->GetData(&v, 4, D3DGETDATA_FLUSH); gGetDatas++;
          if (((FakeQuery*)gQ)->seenAtGet != gIssues) Err("GetData beside a body reached the device with %ld of its query's %ld Issues run", ((FakeQuery*)gQ)->seenAtGet, gIssues);
          if (dtWaitHit[DT_R_QGETDATA] != hit) Err("GetData waited beside a body for an Issue recorded AHEAD of that body"); else gGetDataBesideBody++; }
        // A FREE SLOT, asked while the body is inside: answered at once.
        const LONG w0 = dtWaitN[DT_R_BODY];
        { const int i = 1 + Rnd() % 20; IDirect3DBaseTexture9* t = nullptr; D->GetTexture(SampNum(i), &t);
          if (t != R.tex[i]) Err("GetTexture(%d) beside a body = %p, the reference says %p", i, (void*)t, (void*)R.tex[i]); Rel(t);
          if (dtWaitN[DT_R_BODY] != w0) Err("a sampler slot declared free waited for the body"); else gBodyFree++; }
        // A RENDER STATE THE BODY DOES NOT SET, unknown since the Apply: fetched beside it, at once.
        { DWORD fr = 7 + Rnd() % 203; if (fr == rs) fr = (fr == 209) ? 7 : fr + 1; DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)fr, &v);
          if (v != R.rs[fr]) Err("GetRenderState(%lu), a state the body does not set, beside it = 0x%08lX, the reference says 0x%08lX", fr, v, R.rs[fr]);
          if (dtWaitN[DT_R_BODY] != w0) Err("a render state declared untouched waited for the body"); else gBodyFreeRs++; }
        // WHAT THE BODY HAS CHANGED, asked while it is inside: the stream's value, never the
        // body's. Three cases, a round each (2026-09-19, when the post began to make EVERYTHING
        // the body touches known, not only its render states):
        //   1  the render state it sets: known at the post, answered from the copy, no wait;
        //   2  the texture and a sampler state of the slot it binds: the same, and this is the
        //      owner's case (our HDR code asking for sampler 0's SRGBTEXTURE 0.26 ms behind
        //      the post, and waiting 0.40 ms for it);
        //   0  THE GATE ITSELF, which nothing reaches any more by the ordinary road. It is
        //      still the net under a post that was skipped, so the entry is made unknown by
        //      hand, as a skipped post would have left it, and its fetch must wait.
        const bool still = gBodyIn != 0;
        if (round % 3 == 1) { DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)rs, &v);
                         if (v != R.rs[rs]) Err("GetRenderState(%lu) beside a body = 0x%08lX (the sentinel is 0x%08lX), the reference says 0x%08lX", rs, v, kBodySentinel, R.rs[rs]);
                         if (dtWaitN[DT_R_BODY] != w0) Err("a render state the post made known still waited for the body"); else gBodyKnown++; }
        else if (round % 3 == 2)
                       { IDirect3DBaseTexture9* t = nullptr; D->GetTexture(0, &t);
                         if (t != R.tex[0]) Err("GetTexture(0) beside a body, known at the post = %p (the body's is %p), the reference says %p", (void*)t, (void*)other, (void*)R.tex[0]); Rel(t);
                         DWORD v = 0; D->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &v);
                         if (v != R.samp[0][D3DSAMP_SRGBTEXTURE]) Err("GetSamplerState(0, SRGBTEXTURE) beside a body = 0x%08lX, the reference says 0x%08lX", v, R.samp[0][D3DSAMP_SRGBTEXTURE]);
                         if (dtWaitN[DT_R_BODY] != w0) Err("the texture or a sampler state of the slot the body binds, made known at the post, still waited for the body");
                         else gBodyKnownBound++; }
        else           { dtSh.texOk[0] = false;          // as a skipped post would have left it
                         IDirect3DBaseTexture9* t = nullptr; D->GetTexture(0, &t);
                         if (t != R.tex[0]) Err("GetTexture(0) beside a body = %p (the body's is %p), the reference says %p", (void*)t, (void*)other, (void*)R.tex[0]); Rel(t);
                         if (still && dtWaitN[DT_R_BODY] == w0) Err("an entry the body touches was fetched without waiting for it");
                         if (dtWaitN[DT_R_BODY] != w0) gBodyGated++; }
        for (int k = 0; k < 5; k++) OneOp(false);
    }
    Drain();
    if (dtShBodies != 0) Err("%ld bodies are still counted as outstanding after a drain", (long)dtShBodies);
    dtNoteObs[DT_NOTE_INJECT] = nullptr; dtNoteDrives[DT_NOTE_INJECT] = false; dtBodySampFree = 0;
    memset(dtBodyRsTouched, 0xFF, sizeof(dtBodyRsTouched));
    if (dtSbUfSeq != uf0) Err("C1's bracket counted %ld of the worker's body Sets as another thread's (R8)", (long)(dtSbUfSeq - uf0));
    if (dtBodySets - bs0 != 4 * (gBodyRounds - br0)) Err("the body declaration check saw %ld Sets, the bodies made %ld", (long)(dtBodySets - bs0), 4 * (gBodyRounds - br0));
    if (dtBodyUndeclared != bu0 && !undeclSaid) Err("the body declaration check counted %ld Sets outside the declarations (the first 0x%lX)", (long)(dtBodyUndeclared - bu0), (long)dtBodyUndeclFirst);
}

// THE REFERENCE AGAINST THE FAKE DEVICE (2026-09-28, C1 step 1). After a drain the device holds exactly what the reference
// says, entry by entry. The shadow's Gets have always been held to the reference; this holds the reference to the device,
// so a block rule the reference walks one way and the fake another cannot hide behind a layer that fetched the fake's answer.
static void RefAgainstDevice(const char* phase)
{
    const FkState& d = gFd->st;
    int n = 0;
    char what[128];
    auto bad = [&](const char* w) { if (n++ < 6) Err("%s: the fake device and the reference differ at %s", phase, w); };
    for (int s = 0; s < 256; s++) if (d.rs[s] != R.rs[s]) { sprintf_s(what, "render state %d (0x%08lX against 0x%08lX)", s, d.rs[s], R.rs[s]); bad(what); }
    for (int i = 0; i < 21; i++) for (int t = 0; t < 14; t++) if (d.samp[i][t] != R.samp[i][t]) { sprintf_s(what, "sampler %d state %d", i, t); bad(what); }
    for (int i = 0; i < 21; i++) if (d.tex[i] != R.tex[i]) { sprintf_s(what, "texture %d", i); bad(what); }
    for (int i = 0; i < 4; i++) if (d.rt[i] != R.rt[i]) { sprintf_s(what, "render target %d", i); bad(what); }
    if (d.ds != R.ds) bad("the depth stencil");
    if (memcmp(&d.vp, &R.vp, sizeof(d.vp))) bad("the viewport");
    if (memcmp(&d.sc, &R.sc, sizeof(d.sc))) bad("the scissor");
    if (d.vs != R.vs) bad("the vertex shader");
    if (d.ps != R.ps) bad("the pixel shader");
    for (int i = 0; i < 16; i++) if (d.vb[i] != R.vb[i] || d.off[i] != R.off[i] || d.str[i] != R.str[i])
    { sprintf_s(what, "stream %d (offset %u against %u, stride %u against %u)", i, d.off[i], R.off[i], d.str[i], R.str[i]); bad(what); }
    if (d.ib != R.ib) bad("the indices");
    if (d.decl != R.decl) bad("the declaration");
    for (int r = 0; r < 224; r++) if (memcmp(d.psc[r], R.psc[r], 16)) { sprintf_s(what, "pixel constant %d", r); bad(what); }
    for (int r = 0; r < 256; r++) if (memcmp(d.vsc[r], R.vsc[r], 16)) { sprintf_s(what, "vertex constant %d", r); bad(what); }
    for (int g = 0; g < 8; g++) for (int k = 0; k < 32; k++) if (d.tss[g][k] != R.tss[g][k]) { sprintf_s(what, "stage %d state index %d", g, k); bad(what); }
    for (int i = 0; i < 16; i++) if (d.freq[i] != R.freq[i]) { sprintf_s(what, "stream frequency %d (%u against %u)", i, d.freq[i], R.freq[i]); bad(what); }
    for (int k = 0; k < 14; k++) if (memcmp(&d.xf[k], &R.xf[k], 64)) { sprintf_s(what, "transform index %d", k); bad(what); }
    if (n > 6) Err("%s: ...and %d more differences between the fake device and the reference", phase, n - 6);
}

// The ordered part of what the fake device received against what the test sent.
static bool Compare(const char* phase)
{
    Drain();
    RefAgainstDevice(phase);
    SbOracle(phase);
    EnterCriticalSection(&gFkLock);
    const size_t n = gFk.size() < gExp.size() ? gFk.size() : gExp.size();
    size_t bad = (size_t)-1, fromWorker = 0;
    long wRs = 0, wDip = 0;
    for (size_t i = 0; i < n; i++) if (!(gFk[i] == gExp[i])) { bad = i; break; }
    for (const FkRec& r : gFk) if (r.tid == dtWorkerTid) { fromWorker++; if (r.slot == 57) wRs++; else if (r.slot == 82) wDip++; }
    // THE OBSERVERS SAW EXACTLY WHAT THE WORKER REPLAYED for their two slots, no more
    // (a direct call, unarmed or from another thread, is not observed) and no less.
    wRs -= _InterlockedExchange(&gBodyDirectRs, 0);      // a body's own calls are the worker's but not replayed commands
    if (gObsRs != wRs || gObsDip != wDip || gObsBad)
        Err("%s: observers saw %ld SetRenderState and %ld DrawIndexedPrimitive, the worker replayed %ld and %ld (%ld malformed headers)",
            phase, gObsRs, gObsDip, wRs, wDip, gObsBad);
    gObsRs = gObsDip = 0;
    const bool ok = bad == (size_t)-1 && gFk.size() == gExp.size();
    printf("  %-26s %8zu calls reached the device, %8zu of them from the worker: %s\n",
        phase, gFk.size(), fromWorker, ok ? "in order, identical" : "DIFFERENT");
    if (!ok)
    {
        if (bad == (size_t)-1) bad = n;
        printf("    first difference at #%zu (%zu sent, %zu received)\n", bad, gExp.size(), gFk.size());
        for (int k = 0; k < 2; k++)
        {
            const std::vector<FkRec>& v = k ? gFk : gExp;
            if (bad >= v.size()) continue;
            printf("    %s slot %d, args", k ? "received:" : "sent:    ", v[bad].slot);
            for (DWORD a : v[bad].a) printf(" %08lX", a);
            printf(", %zu data bytes\n", v[bad].data.size());
        }
        gErr++;
    }
    gFk.clear(); gExp.clear();
    LeaveCriticalSection(&gFkLock);
    return ok;
}

static bool ReplayErrHas(DWORD who, HRESULT hr);
static void StateBlocks()
{
    // A block made BEFORE the recording, for the Capture made inside it.
    IDirect3DStateBlock9* pre = SbCreate(D3DSBT_PIXELSTATE);
    // RECORDED. Between Begin and End a Set goes into the block and changes nothing, so every Get in here must still see
    // the state from before Begin; the reference records what DXVK's recorder keeps (rRec).
    SbBegin();
    for (int i = 0; i < 150; i++) OneOp(false);
    // Refused, and the block goes on recording (DXVK 3.1): the shadow must go on ignoring Sets.
    if (D->EndStateBlock(nullptr) != D3DERR_INVALIDCALL) Err("EndStateBlock(nullptr) was not refused");
    E(61, {});
    // A CAPTURE INSIDE THE RECORDING (C1's P4): answered D3D_OK, refused when the worker replays it, counted against a
    // resource method; the block keeps its creation values, which the Apply of `pre` below proves.
    SbCapture(pre);
    for (int i = 0; i < 150; i++) OneOp(false);
    IDirect3DStateBlock9* rec = SbEnd();
    Drain();
    if (!ReplayErrHas(DT_WHO_RES, D3DERR_INVALIDCALL)) Err("a Capture refused on the worker inside a recording was not counted against a resource method");
    // THE RECORDED BLOCK: its Apply puts back exactly what was recorded, by the device's rules; then Captured (the set
    // does not widen, a stream keeps its recorded offset) and applied again.
    for (int i = 0; i < 300; i++) OneOp(false);
    SbApply(rec);
    for (int i = 0; i < 300; i++) OneGet();
    for (int i = 0; i < 300; i++) OneOp(false);
    SbCapture(rec);
    for (int i = 0; i < 300; i++) OneOp(false);
    SbApply(rec);
    for (int i = 0; i < 200; i++) OneGet();
    SbApply(pre);
    for (int i = 0; i < 200; i++) OneGet();
    // TYPED ALL. Apply changes the device behind the shadow's back: the shadow must follow.
    IDirect3DStateBlock9* sb = SbCreate(D3DSBT_ALL);
    for (int i = 0; i < 500; i++) OneOp(false);
    SbCapture(sb);
    for (int i = 0; i < 500; i++) OneOp(false);
    // An Apply puts back everything a block can hold, which is NOT the render targets or the depth stencil (dt_fake_dev.h,
    // FakeSB::Apply). The 500 ops between the Capture and here rebind both, so a layer that kept a stale one shows below.
    SbApply(sb);
    const LONG fetchedBefore = dtShFetched;
    for (int i = 0; i < 4; i++) { IDirect3DSurface9* s = nullptr; const HRESULT hr = D->GetRenderTarget((DWORD)i, &s);
        if (s != R.rt[i] || hr != (R.rt[i] ? D3D_OK : D3DERR_NOTFOUND)) Err("GetRenderTarget(%d) after an Apply differs from the reference", i); Rel(s); }
    { IDirect3DSurface9* s = nullptr; D->GetDepthStencilSurface(&s); if (s != R.ds) Err("GetDepthStencilSurface after an Apply differs from the reference"); Rel(s); }
    if (dtShFetched != fetchedBefore) Err("the render targets or the depth stencil were FETCHED after an Apply: a block cannot hold them, so the copy should still know them");
    for (int i = 0; i < 300; i++) OneGet();
    // TYPED PIXEL AND VERTEX: each puts back its own half and leaves the rest as the ops left it.
    for (int k = 0; k < 2; k++)
    {
        IDirect3DStateBlock9* t = SbCreate(k ? D3DSBT_VERTEXSTATE : D3DSBT_PIXELSTATE);
        for (int i = 0; i < 500; i++) OneOp(false);
        if (Rnd() & 1) { SbCapture(t); for (int i = 0; i < 300; i++) OneOp(false); }
        SbApply(t);
        for (int i = 0; i < 300; i++) OneGet();
        SbRelease(t);
    }
    // A BLOCK KEEPS WHAT IT HOLDS ALIVE (DXVK's private references): a texture bound only by a block, its last public
    // reference gone, is put back by the Apply and dies with the block. Through the layer the block's Release is deferred
    // like any other, so the texture must outlive every call queued ahead of it.
    {
        IDirect3DTexture9* T = NewTex();
        const int i = 1 + Rnd() % 20;
        D->SetTexture(SampNum(i), T); E(65, { SampNum(i), P(T) }); RTex(i, T);
        IDirect3DStateBlock9* keep = SbCreate(D3DSBT_ALL);
        D->SetTexture(SampNum(i), nullptr); E(65, { SampNum(i), 0u }); RTex(i, nullptr);
        if (T->Release() != 0) Err("the last public Release of a texture a block holds did not return 0");
        Drain();
        if (IsDead(T)) Err("a texture died while a state block still held it");
        SbApply(keep);                                         // T is bound again, by the block
        D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
        D->SetTexture(SampNum(i), nullptr); E(65, { SampNum(i), 0u }); RTex(i, nullptr);   // before any Get could hand it out
        SbRelease(keep);
        Drain();
        if (!IsDead(T)) Err("a texture outlived the only state block that held it");
    }
    SbRelease(rec); SbRelease(sb); SbRelease(pre);
}

static IDirect3DTexture9* gForeignT = nullptr;
static IDirect3DSurface9* gForeignDs = nullptr;
static DWORD WINAPI ForeignSetDs(void*) { D->SetDepthStencilSurface(gForeignDs); return 0; }
static DWORD WINAPI ForeignSet(void*) { D->SetRenderState(D3DRS_ZENABLE, 0xABCu); return 0; }
static DWORD WINAPI ForeignRelease(void*) { const ULONG r = gForeignT->Release(); if (r != 0) Err("a last Release from another thread returned %lu", r); return 0; }
static void RunThread(LPTHREAD_START_ROUTINE f) { HANDLE h = CreateThread(nullptr, 0, f, nullptr, 0, nullptr); WaitForSingleObject(h, INFINITE); CloseHandle(h); }
// ANOTHER THREAD'S ORDERED CALL WAITS FOR THE WORKER AND THE LAYER STAYS ARMED (2026-09-19, the second form;
// dt_core.h, DtOrderForeign). The first form unarmed the layer at every such call, which a caller that comes
// every frame (IVSDKDotNet) turned into a layer that was never armed and a 30 ms frame at every attempt. So
// every case below must END ARMED, with no ask left up, and with the call counted as put in order.
static long gForeignStayedArmed = 0;
static void RearmAfterForeign(const char* what)      // the name is the first form's; what it checks is the second's
{
    if (!dtArmed) Err("%s UNARMED the layer: a caller that comes every frame would keep it off for good", what);
    else gForeignStayedArmed++;
    if (dtDisarmAsk) Err("%s left an ask up", what);
    for (int i = 0; i < 2000; i++) OneOp(true);
}
static void Foreign()
{
    // A Set from another thread: it is counted before and after it lands, so the next Get settles and asks DXVK.
    Drain();
    const LONG ordered0 = dtForeignOrdered;
    RunThread(ForeignSet); E(57, { (DWORD)D3DRS_ZENABLE, 0xABCu }); R.rs[D3DRS_ZENABLE] = 0xABCu;
    DWORD v = 0; D->GetRenderState(D3DRS_ZENABLE, &v); gGets++;
    if (v != 0xABCu) Err("after another thread's Set GetRenderState said 0x%lX, not 0xABC", v);
    if (dtForeignOrdered != ordered0 + 1) Err("another thread's Set was put in order %ld times, not once", dtForeignOrdered - ordered0);
    RearmAfterForeign("another thread's SetRenderState");
    D->GetRenderState(D3DRS_ZENABLE, &v); gGets++;
    if (v != R.rs[D3DRS_ZENABLE]) Err("armed again after another thread's Set, the fresh shadow said 0x%lX, the reference 0x%lX", v, R.rs[D3DRS_ZENABLE]);   // the re-arm's own traffic may have set it since
    // A Release from another thread, of an object a queued call still names.
    gForeignT = NewTex();
    D->SetTexture(0, gForeignT); E(65, { 0u, P(gForeignT) }); R.tex[0] = gForeignT;
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    RunThread(ForeignRelease);
    if (IsDead(gForeignT)) Err("a Release from another thread killed a texture a queued call still names");
    D->SetTexture(0, nullptr); E(65, { 0u, 0u }); R.tex[0] = nullptr;
    Present();
    Drain();
    if (!IsDead(gForeignT)) Err("the handed-over Release never ran");
    // ANOTHER THREAD'S SET LANDS INSIDE THE VALIDATOR'S OWN READ (2026-09-19). The whole-copy
    // compare reads several hundred entries out of the device; a Set from another thread in
    // the middle of that leaves the device newer than the copy for one frame, which is not a
    // mismatch (the next frame sees the count move and reloads). The owner's threaded=2 run
    // showed exactly that, once, on the depth stencil. The hook fires inside the read, after
    // the render targets and before the depth stencil, which is where it has to land to show.
    if (dtValidate)
    {
        const LONG mis = dtValMis, raced = dtValRaced;
        gForeignDs = (gDs[0] == R.ds) ? gDs[1] : gDs[0];
        { IDirect3DSurface9* s = nullptr; D->GetDepthStencilSurface(&s); Rel(s); }   // the copy KNOWS the old one, or nothing is compared
        gFkMidRead = [] { RunThread(ForeignSetDs); };
        Present();
        E(39, { P(gForeignDs) }); R.ds = gForeignDs;
        if (gFkMidRead) { gFkMidRead = nullptr; Err("the mid-read hook never fired: no whole-copy compare ran at that Present"); }
        if (dtValMis != mis) Err("another thread's Set during the validator's read was reported as a MISMATCH (%ld)", dtValMis - mis);
        if (dtValRaced != raced + 1) Err("the raced compare was not counted as thrown away (%ld)", dtValRaced - raced);
        IDirect3DSurface9* s = nullptr; D->GetDepthStencilSurface(&s);
        if (s != gForeignDs) Err("after the raced compare GetDepthStencilSurface = %p, the other thread set %p", (void*)s, (void*)gForeignDs); Rel(s);
        RearmAfterForeign("another thread's SetDepthStencilSurface inside the validator's read");
    }
}

// THE HAZARD ITSELF, AND THE PROOF IT IS CLOSED (2026-09-19). The owner's black sky: GTA IV's main thread binds
// the depth stencil as a load ends while the render thread's own binds are still in the ring, and until today
// the other thread's call reached the device FIRST and the older queued one landed on top of it.
//   held: the worker is held inside a note's observer (a gate) with a Set of the same state queued behind it.
//     Another thread then sets that state. It must NOT reach the device while the ring cannot drain (looked
//     at 60 ms in, past the barrier's 25 ms settle), and once the gate opens the device must get the QUEUED
//     Set first and the other thread's second, so it ends on the other thread's value, as it does natively.
//   recording: the producer records Sets flat out while another thread sets one. The interleaving is not
//     fixed, but one thing is: once the other thread's call has reached the device, NOTHING the worker
//     replays may reach it after, because the ring was drained and the layer unarmed before it landed.
// THE HARNESS'S GATE NOTE (moved from kind 6 to 15 on 2026-09-25, frame generation phase 1C, S1): kind 6 is now the rig's
// DT_NOTE_FG_HUDLESS (source/fg/fg_machine.h), and tools/perfproof/dt_fg_test.cpp includes both, so the gate takes the
// last kind the device thread accepts (DT_NOTE_N is 16), which nothing in the mod will ever register.
enum : DWORD { kHarnessGate = 15 };
static_assert(kHarnessGate < DT_NOTE_N, "the gate is a kind the device thread accepts");
static HANDLE gGateEv = nullptr;
static volatile LONG gGateIn = 0;
static void ObsGate(DWORD, const DWORD*, DWORD) { _InterlockedExchange(&gGateIn, 1); WaitForSingleObject(gGateEv, INFINITE); _InterlockedExchange(&gGateIn, 0); }
static DWORD gOrdRs = 0, gOrdVal = 0;
static volatile DWORD gOrdTid = 0;
static DWORD WINAPI ForeignSetOrdered(void*) { gOrdTid = GetCurrentThreadId(); D->SetRenderState((D3DRENDERSTATETYPE)gOrdRs, gOrdVal); return 0; }
static long gOrderedHeld = 0, gOrderedBusy = 0;
static void ForeignOrderedHeld()
{
    Drain();
    if (!dtArmed) { Err("the held round needs the layer armed"); return; }
    const DWORD X = D3DRS_STENCILREF, queued = 0x11u; gOrdRs = X; gOrdVal = 0x22u;
    if (!DtNote(kHarnessGate, &X, 4)) { Err("the gate note was refused"); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, queued); E(57, { X, queued });          // made BEFORE the other thread's call
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("the worker never reached the gate"); SetEvent(gGateEv); return; }
    HANDLE h = CreateThread(nullptr, 0, ForeignSetOrdered, nullptr, 0, nullptr);
    Sleep(60);
    if (gFd->st.rs[X] == gOrdVal) Err("another thread's Set reached the device while the ring still held an older Set of the same state");
    if (!dtArmed) Err("the layer was unarmed while the worker was still held with the ring not drained");
    if (!dtForeignWaited) Err("the other thread is said never to have waited for the worker, which was held");
    SetEvent(gGateEv);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    E(57, { X, gOrdVal }); R.rs[X] = gOrdVal;                                        // AFTER the queued one: the native order
    if (gFd->st.rs[X] != gOrdVal) Err("the device ended on 0x%lX, not on the other thread's 0x%lX: the older Set landed on top of it", gFd->st.rs[X], gOrdVal);
    DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &v); gGets++;
    if (v != gOrdVal) Err("after the ordered Set GetRenderState said 0x%lX, not 0x%lX", v, gOrdVal);
    gOrderedHeld++;
    RearmAfterForeign("another thread's SetRenderState behind a held worker");
}
static volatile LONG gOrdGo = 0, gOrdDone = 0;
static volatile LONG gBusyI = 0, gOrdSawI = 0;           // the producer's last RETURNED Set; and what that was when the other thread made its call
static DWORD WINAPI ForeignSetOrderedLate(void*)
{
    while (!gOrdGo) Sleep(0);
    Sleep(5);                                            // the producer is well into its loop
    gOrdSawI = gBusyI;                                   // every producer Set up to this one had RETURNED, so it was committed, before this call is made
    ForeignSetOrdered(nullptr);
    _InterlockedExchange(&gOrdDone, 1);
    return 0;
}
// THE FOREIGN BARRIER ACROSS A FULL RING (2026-09-28). The recording-producer case above failed about one run in ten: a
// ring's worth of the producer's Sets reached the device AFTER another thread's call. The barrier read where the worker
// was and then where the producer was; with the ring full the producer refills each slot the worker frees at once, so a
// producer position read a moment after the worker's could already lie past it, [done, cur) was a short span without w in
// it, and the barrier let the call go while the worker was still nearly a ring behind. Here the other thread calls only
// once the ring is three quarters full, and a hook spins 200 us between the barrier's two reads on its first turn: long
// enough for the worker to move and the producer to refill, far too short for a whole ring. In the product's order (cur
// first) every Set made before the call must be on the device ahead of it. THE CONTROL reads in the old order and must
// see the early pass: Sets made before the call that land after it.
// SINCE 2026-10-01 THE BARRIER WAITS ON A COUNT (dt_install.h, DtForeignPassedCount; the owner's decision 6), which has no read
// order: the case runs the product that way, and the control is the ring-position test in the order that failed. The third
// run (timed) samples the wait only every millisecond, as its sleeps do, and requires the call back well inside the
// one-second cap the ring test waited out with the ring full; BarrierRule, below, holds both tests to that state exactly.
static volatile LONG gLapFull = 0;
static void BarrierHookSpin()
{
    LARGE_INTEGER t0, t; QueryPerformanceCounter(&t0);
    do { _mm_pause(); QueryPerformanceCounter(&t); } while ((t.QuadPart - t0.QuadPart) * 1000000 < 200 * dtQpf);
}
static volatile LONGLONG gLapCallTicks = 0;            // the other thread's call, timed
static void BarrierSleepOneMs() { Sleep(1); }
static DWORD WINAPI ForeignSetLapped(void*)
{
    while (!gOrdGo) Sleep(0);
    const ULONGLONG t0 = GetTickCount64();
    for (;;)                                             // wait for a ring three quarters full, at most 2 s
    {
        const DWORD cur = *(volatile DWORD*)&dtW, done = (DWORD)dtDone;
        const DWORD backlog = cur >= done ? cur - done : cur + dtRingDw - done;
        if (backlog > dtRingDw / 4 * 3) { _InterlockedExchange(&gLapFull, 1); break; }
        if (GetTickCount64() - t0 > 2000) break;
        _mm_pause();
    }
    gOrdSawI = gBusyI;
    gBarrierHook = BarrierHookSpin;
    LARGE_INTEGER c0, c1; QueryPerformanceCounter(&c0);
    ForeignSetOrdered(nullptr);
    QueryPerformanceCounter(&c1); gLapCallTicks = c1.QuadPart - c0.QuadPart;
    gBarrierHook = nullptr;
    _InterlockedExchange(&gOrdDone, 1);
    return 0;
}
static long gLapRan = 0;
static void ForeignBarrierLap(bool curFirst, bool timed = false)
{
    const char* how = timed ? "another thread's call across a full ring, the wait sampled every millisecond"
                    : curFirst ? "another thread's call across a full ring" : "another thread's call across a full ring, CONTROL";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    gBarrierCurFirst = curFirst; gBarrierByCount = curFirst;   // the control: the ring test, in the order that failed
    gBarrierEachTurn = timed ? BarrierSleepOneMs : nullptr; gLapCallTicks = 0;
    gOrdRs = D3DRS_STENCILMASK; gOrdVal = 0xF00Eu; gOrdTid = 0; gOrdGo = 0; gOrdDone = 0; gBusyI = 0; gOrdSawI = 0; gLapFull = 0;
    HANDLE h = CreateThread(nullptr, 0, ForeignSetLapped, nullptr, 0, nullptr);
    gOrdGo = 1;
    const ULONGLONG t0 = GetTickCount64();
    LONG i = 0;
    while (!gOrdDone && GetTickCount64() - t0 < 5000) { i++; D->SetRenderState(D3DRS_STENCILREF, (DWORD)i); gBusyI = i; }
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    gBarrierCurFirst = true; gBarrierByCount = true; gBarrierEachTurn = nullptr;
    const double callMs = gLapCallTicks * 1000.0 / (double)dtQpf;
    Drain();
    EnterCriticalSection(&gFkLock);
    size_t at = gFk.size(); long late = 0, ahead = 0;
    for (size_t k = 0; k < gFk.size(); k++) if (gFk[k].tid == gOrdTid && gFk[k].slot == 57) at = k;
    const bool found = at < gFk.size();
    for (size_t k = 0; k < gFk.size(); k++)
    {
        if (gFk[k].slot != 57 || gFk[k].a.size() < 2 || gFk[k].a[0] != (DWORD)D3DRS_STENCILREF || (LONG)gFk[k].a[1] > gOrdSawI) continue;
        if (k < at) ahead++; else late++;
    }
    gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0;
    LeaveCriticalSection(&gFkLock);
    printf("  %s: the ring %s at the call, %ld Sets, %ld made before the call; on the device after it: %ld; the call took %.1f ms\n",
           how, gLapFull ? "three quarters full" : "NOT FULL", (long)i, (long)gOrdSawI, late, callMs);
    if (!found) Err("%s: the other thread's Set never reached the device", how);
    else if (!gLapFull) Err("%s: the ring never filled, so the case tested nothing", how);
    else if (ahead + late != gOrdSawI) Err("%s: %ld of the %ld Sets made before the call are on the device", how, ahead + late, (long)gOrdSawI);
    else if (curFirst && late) Err("%s: %ld Set(s) made BEFORE the call reached the device AFTER it: the barrier let it go early", how, late);
    else if (!curFirst && !late) Err("%s: with the old read order no Set landed after the call, so the case above proves nothing", how);
    else if (timed && callMs >= 500.0) Err("%s: the call took %.0f ms: the barrier waited toward its one-second cap", how, callMs);
    else gLapRan++;
    { FkState keep = gFd->st; R = keep; }                                             // the two interleave: the reference restarts from the device
    RearmAfterForeign(how);
}

// THE BARRIER'S RULE ON THE FULL RING, EXACTLY (2026-10-01, the owner's decision 6). With the ring full the producer refills each
// slot the worker frees, so once the worker has passed w the producer's place lies just behind the worker's and w falls
// inside [done, cur) again: the ring-position test read it as queued and the barrier waited out its one-second cap (four of
// ten default runs met it). The count must read every case right, across its own 32-bit wrap too; THE CONTROL, the ring
// test, must misread the full ring (and be right where it always was, so it is the old test).
static long gBarrierRuleRan = 0;
static void BarrierRule()
{
    struct Case { DWORD w, done, cur, wDw, doneDw; bool passed; const char* what; };
    static const Case cs[] = {
        { 100, 150, 149, 5000, 5050, true,  "the full ring: the worker 50 past w, the producer refilled to just behind it" },
        { 100,  50, 120, 5000, 4950, false, "the worker 50 short of w" },
        { 100, 100, 180, 5000, 5000, true,  "the worker exactly at w" },
        { 100, 101, 100, 5000, 5001, true,  "the ring full, the producer refilled to w itself, the worker one past it" },
    };
    int bad = 0;
    for (const Case& c : cs)
        if (DtForeignPassedCount(c.wDw, c.doneDw) != c.passed) { Err("the barrier's count reads %s as %s", c.what, c.passed ? "not passed" : "passed"); bad++; }
    if (!DtForeignPassedCount(0xFFFFFFF0u, 0x10u)) { Err("the barrier's count does not read a worker past w across 2^32 as passed"); bad++; }
    if (DtForeignPassedCount(0x10u, 0xFFFFFFF0u)) { Err("the barrier's count reads a worker short of w across 2^32 as passed"); bad++; }
    const bool ringFull = DtForeignPassedRing(cs[0].w, cs[0].done, cs[0].cur), ringFull2 = DtForeignPassedRing(cs[3].w, cs[3].done, cs[3].cur);
    const bool ringOld = DtForeignPassedRing(cs[2].w, cs[2].done, cs[2].cur) && !DtForeignPassedRing(cs[1].w, cs[1].done, cs[1].cur);
    printf("  the barrier's rule on the full ring: the count reads all %d cases right%s; the ring test reads the full ring as %s and %s\n",
           (int)(sizeof(cs) / sizeof(cs[0])) + 2, bad ? " NOT" : "", ringFull ? "passed" : "QUEUED", ringFull2 ? "passed" : "QUEUED");
    if (ringFull || ringFull2) Err("the ring-position test read a full ring rightly, so BarrierRule proves nothing");
    else if (!ringOld) Err("the ring-position test is wrong outside the full ring too: the control is not the old test");
    else if (!bad) gBarrierRuleRan++;
}

// WAKE-EVERY-64 (2026-10-01, the final audit; dt_core.h, DT_WAKE_KIND). The worker is held at the gate with 4096 Sets queued
// behind it while the render thread sleeps in a drain, or in the one-frame-ahead wait (two Presents made behind the Sets: the
// second waits for the first); the gate opens 5 ms later, its opener started first, because at threaded=2 every Present's
// frame end validates with a drain of its own. There the Present case is a drain case again; at threaded=1 the
// one-frame-ahead wait must have slept, or the case tested nothing. The render thread must be back within 10 ms of the gate opening: the 20 ms
// safety net is the failure, and fault 37 (the catch-up's signal dropped) must land there, in the drain. And the worker must
// have signalled it at most twice for the one wait. THE CONTROL: kind 1, the old way, must count at least 16 signals (about
// 64, one every 64 commands), or the count proves nothing.
static LARGE_INTEGER gGateOpenedAt;
// 5 ms ON THE QPC CLOCK, SPUN (2026-10-01): Sleep(5) wakes on the scheduler's 15.6 ms tick, and could then open the gate on the
// same tick as the producer's 20 ms net; fault 37's drain then read as back at once and the fault went uncaught (a hole about
// one run in twenty-five, seen twice in prove-all). Spun, the gate opens at 5 ms and the net is always 15 ms or more after it.
static DWORD WINAPI OpenGateAfter5(void*)
{
    LARGE_INTEGER t0, t; QueryPerformanceCounter(&t0);
    do { _mm_pause(); QueryPerformanceCounter(&t); } while ((t.QuadPart - t0.QuadPart) * 1000 < 5 * dtQpf);
    QueryPerformanceCounter(&gGateOpenedAt); SetEvent(gGateEv); return 0;
}
static long gWakeRan = 0;
static void WakeHeld(bool pace, bool kind2)
{
    char how[160];
    sprintf_s(how, "%s behind a held worker%s", pace ? "the one-frame-ahead wait" : "a drain", kind2 ? "" : ", CONTROL (kind 1, the old way)");
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    const DWORD X = D3DRS_STENCILREF;
    if (!DtNote(kHarnessGate, &X, 4)) { Err("%s: the gate note was refused", how); return; }
    for (DWORD k = 0; k < 4096; k++) { D->SetRenderState((D3DRENDERSTATETYPE)X, k); E(57, { X, k }); RRs(X, k); }
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("%s: the worker never reached the gate", how); SetEvent(gGateEv); return; }
    gWakeKind = kind2 ? 2 : 1; gWakeFaultLive = true;
    gGateOpenedAt.QuadPart = 0;
    const LONG s0 = gProdSignals, pace0 = dtWaitBlocked[DT_R_PACE];
    HANDLE g = CreateThread(nullptr, 0, OpenGateAfter5, nullptr, 0, nullptr);
    if (pace) { Present(); Present(); } else Drain();      // the second Present waits for the first; the drain for all of it
    LARGE_INTEGER back; QueryPerformanceCounter(&back);
    WaitForSingleObject(g, INFINITE); CloseHandle(g);
    gWakeKind = 2; gWakeFaultLive = false;
    const LONG sig = gProdSignals - s0;
    const double ms = gGateOpenedAt.QuadPart ? (back.QuadPart - gGateOpenedAt.QuadPart) * 1000.0 / (double)dtQpf : -1.0;
    printf("  wake: %s: back %.2f ms after the gate opened, %ld signal(s)\n", how, ms, (long)sig);
    if (ms < 0) Err("%s: the render thread came back before the gate opened", how);
    else if (kind2 && ms >= 10.0) Err("%s: back %.1f ms after the gate opened: the 20 ms net woke it, not the worker", how, ms);
    else if (kind2 && sig > 2) Err("%s: the worker signalled %ld times for one wait: wake-every-64 is back", how, (long)sig);
    else if (!kind2 && sig < 16) Err("%s: only %ld signal(s) with kind 1, so the count above proves nothing", how, (long)sig);
    else if (pace && !dtValidate && dtWaitBlocked[DT_R_PACE] == pace0) Err("%s: the one-frame-ahead wait never slept, so the case tested nothing", how);
    else gWakeRan++;
}
// THREE OF THE FOUR DEVICE CALLS WINDOWS' COMPATIBILITY LAYER HOOKS, MADE WHILE ARMED (1.0.8, 2026-10-01). apphelp.dll's 8 and
// 16-bit colour mitigation patches QueryInterface, Release, CreateAdditionalSwapChain and Reset in DXVK's vtable; DtInstall
// lets that stand (DtForeignSlotOk): Release because a device may be released from any thread, the other three because the
// layer makes them on the CALLING thread, so Windows' hook runs where it would without the layer. This case is about those
// three. Here the first two go through the wrappers on the armed device and must reach the device once each, on this
// thread, and leave the layer armed; Reset is counted in the cases that reset. THE CONTROL: the same call counted as the
// worker's is seen. main() refuses a run in which any of the three was ever made on the worker.
static void Caller3Case()
{
    if (!dtArmed) { Err("the caller-thread case needs the layer armed"); return; }
    static const GUID kAny = { 0x4D1C0001, 0x0001, 0x0001, { 1, 2, 3, 4, 5, 6, 7, 8 } };
    const LONG q0 = gFkCaller3[0], c0 = gFkCaller3[1], w0 = gFkOnWorker3;
    void* p = nullptr;
    if (FAILED(D->QueryInterface(kAny, &p)) || p != (void*)gFd) Err("QueryInterface through the wrapper did not answer with the device");
    InterlockedDecrement(&gFd->ref);                       // the fake's QueryInterface took a reference
    D3DPRESENT_PARAMETERS pp = {}; IDirect3DSwapChain9* sc2 = nullptr;
    if (FAILED(D->CreateAdditionalSwapChain(&pp, &sc2)) || !sc2) Err("CreateAdditionalSwapChain through the wrapper failed");
    E(13, {});
    if (gFkCaller3[0] != q0 + 1 || gFkCaller3[1] != c0 + 1)
        Err("the wrappers reached the device %ld and %ld time(s) for QueryInterface and CreateAdditionalSwapChain, not once each",
            (long)(gFkCaller3[0] - q0), (long)(gFkCaller3[1] - c0));
    if (gFkOnWorker3 != w0) Err("QueryInterface or CreateAdditionalSwapChain ran on the worker");
    if (!dtArmed) Err("QueryInterface or CreateAdditionalSwapChain unarmed the layer");
    gFkPretendWorker = GetCurrentThreadId();               // the control
    if (gTwin) { ((HRESULT(__stdcall*)(void*, const GUID*, void**))dtDx[0])(gFd, &kAny, &p); gTwinControl++; }   // through the stand-in, which must see it too
    else gFd->FakeDev::QueryInterface(kAny, &p);
    InterlockedDecrement(&gFd->ref);
    gFkPretendWorker = 0;
    if (gFkOnWorker3 != w0 + 1) Err("the control: a QueryInterface made as the worker was not counted");
    else { InterlockedDecrement(&gFkOnWorker3); gCaller3Ran = 1; }
}
// WINDOWS WRITES ITS HOOKS BACK, AND THE MOD TAKES ITS SLOTS BACK (1.0.8; dt_install.h, DtRetakeSlots). The stand-in does what
// the real layer was measured to do when a second device is made on a vtable every device shares (tools/apphelp-probe
// `second`, on DXVK): its four hooks into the slots again, passing on to what they held the first time, so the wrappers
// written over them are gone around. DtRetakeSlots must put the four wrappers back, say so, and do nothing at a second look.
// THE CONTROL: a hook someone ELSE wrote over a wrapper later is not what the slot held before the layer, and is left alone.
static HRESULT __stdcall FkThirdHook(void*) { return S_OK; }
static void RetakeCase()
{
    if (!gTwin)
    {
        if (DtRetakeSlots("in a run without the stand-in") != 0) Err("slots were taken back with nothing but the layer on the device");
        gRetakeRan = 1;
        return;
    }
    Drain();
    void** const vt = *(void***)gFd;
    void* const w17 = vt[17];
    WriteSlot(vt, 17, (void*)&FkThirdHook);
    if (DtRetakeSlots("with someone else's hook on Present") != 0 || vt[17] != (void*)&FkThirdHook) Err("a hook someone else wrote over a wrapper was taken back");
    WriteSlot(vt, 17, w17);
    gRetakeLine[0] = 0;
    const int wrote = gTwinRepatch();
    bool around = true;
    for (int s : { 0, 2, 13, 16 }) around = around && vt[s] == dtDx[s] && vt[s] != dtDevFn[s];
    if (wrote != 4 || !around) Err("the stand-in did not write its four hooks back over the wrappers (%d written)", wrote);
    const int took = DtRetakeSlots("in the harness");
    bool back = true;
    for (int s : { 0, 2, 13, 16 }) back = back && vt[s] == dtDevFn[s];
    if (took != 4 || !back) Err("DtRetakeSlots took %d slot(s) back and the four %s their wrappers", took, back ? "hold" : "do NOT hold");
    if (!strstr(gRetakeLine, "device slots 0, 2, 13, 16 held again") || !strstr(gRetakeLine, "apphelp.dll's"))
        Err("the take-back did not name its slots and the module: \"%s\"", gRetakeLine);
    if (DtRetakeSlots("a second look") != 0) Err("a second look took slots back again");
    // AND THE FRAME'S LOOK (hkEndScene's, settle): it waits one look before it acts, because the write it sees may still be
    // going on in another thread, and acts at the next. A look that finds the table as it was forgets what it saw.
    if (gTwinRepatch() != 4) Err("the stand-in did not write its hooks back a second time");
    if (DtRetakeSlots("the frame's first look", true) != 0) Err("the frame's look acted at once");
    bool still = true;
    for (int s : { 0, 2, 13, 16 }) still = still && vt[s] == dtDx[s];
    if (!still) Err("the frame's first look changed a slot");
    if (DtRetakeSlots("the frame's second look", true) != 4) Err("the frame's second look did not take the four slots back");
    back = true;
    for (int s : { 0, 2, 13, 16 }) back = back && vt[s] == dtDevFn[s];
    if (!back) Err("after the frame's second look the four do not hold their wrappers");
    if (DtRetakeSlots("the frame's third look", true) != 0) Err("the frame's third look took slots back again");
    if (!gErr) gRetakeRan = 1;
}

static void WakeCases()
{
    if (!gGateEv) gGateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    dtNoteObs[kHarnessGate] = ObsGate;
    WakeHeld(false, true);
    WakeHeld(false, false);                                // the control
    WakeHeld(true, true);
    WakeHeld(true, false);                                 // the control
    dtNoteObs[kHarnessGate] = nullptr;
    if (gWakeRan != 4) { printf("  the wake cases and their controls did not all prove what they must (%ld of 4)\n", gWakeRan); gErr++; }
}

static void ResetRearm(const char* how);
// A PRODUCER PARKED BETWEEN DtCommit'S TWO STORES (2026-09-28, the review of ac9e4478). A command that ends exactly at the
// end of the ring leaves dtW at dtRingDw until the second store wraps it to 0. Read raw there by another thread, w (which the
// barrier always normalised) and cur (which it did not) disagreed, and the foreign barrier took w for published and passed:
// the call reached DXVK ahead of the commands still queued. The Reset barrier's idle path, quiet that long, would have
// published dtRingDw itself, which the worker never meets. Here the producer fills the ring to its last dword (Sets of three
// dwords and one pixel-constant upload of four and a multiple of four, last), and DT_COMMIT_AT_END parks it there.
static const UINT kEndReg = 201;                 // the last command's start register: no other upload here uses it
// Fills from dtW to exactly dtRingDw and returns the count of the last command (0 to 2 registers); false if it cannot start.
static bool FillToRingEnd(UINT* lastCount)
{
    Drain();
    DWORD p = *(volatile DWORD*)&dtW;
    if (dtRingDw - p < 64 || p == 0)             // too close to the end, or at 0 (the end is committable only while dtDone != 0)
    {
        for (int k = 0; k < 40 && (dtRingDw - *(volatile DWORD*)&dtW < 64 || *(volatile DWORD*)&dtW == 0); k++)
        { D->SetRenderState(D3DRS_STENCILREF, 0x33u); E(57, { (DWORD)D3DRS_STENCILREF, 0x33u }); RRs(D3DRS_STENCILREF, 0x33u); }
        Drain(); p = *(volatile DWORD*)&dtW;
        if (dtRingDw - p < 64 || p == 0) return false;
    }
    const DWORD m = dtRingDw - p;
    UINT c = 0; while (c < 3 && (m - (4 + 4 * c)) % 3 != 0) c++;
    for (DWORD k = 0; k < (m - (4 + 4 * c)) / 3; k++)
    { const DWORD v = k & 0xFFFFu; D->SetRenderState(D3DRS_STENCILREF, v); E(57, { (DWORD)D3DRS_STENCILREF, v }); RRs(D3DRS_STENCILREF, v); }
    static float cst[2 * 4] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    D->SetPixelShaderConstantF(kEndReg, cst, c); E(109, { kEndReg, 0u, c }, cst, c * 16); if (c) RPsc(kEndReg, cst, c);
    *lastCount = c;
    return true;
}
static volatile LONG gAtEndIn = 0, gAtEndCallDone = 0;
static HANDLE gAtEndThread = nullptr;
static DWORD WINAPI AtEndForeignSet(void*) { gOrdTid = GetCurrentThreadId(); D->SetRenderState(D3DRS_STENCILMASK, 0xE17Du); _InterlockedExchange(&gAtEndCallDone, 1); return 0; }
static void AtEndHookForeign()                   // on the producer, parked: another thread's call, given 300 ms to land
{
    _InterlockedExchange(&gAtEndIn, 1);
    gAtEndThread = CreateThread(nullptr, 0, AtEndForeignSet, nullptr, 0, nullptr);
    const ULONGLONG t0 = GetTickCount64();
    while (!gAtEndCallDone && GetTickCount64() - t0 < 300) Sleep(1);
}
static long gAtEndRan = 0;
static void ForeignAtRingEnd(bool norm)
{
    const char* how = norm ? "another thread's call while the producer is parked at the ring's end" : "another thread's call while the producer is parked at the ring's end, CONTROL";
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    gBarrierNormCur = norm; gBarrierByCount = norm; gAtEndIn = 0; gAtEndCallDone = 0; gAtEndThread = nullptr; gOrdTid = 0;   // the control: the ring test with the end read raw (the review of 9123f945: the count's re-publish would publish the raw end, which the worker never meets)
    UINT c = 0;
    gCommitEndHook = AtEndHookForeign;
    const bool filled = FillToRingEnd(&c);
    gCommitEndHook = nullptr;
    const bool parkedDone = gAtEndCallDone != 0;
    if (gAtEndThread) { WaitForSingleObject(gAtEndThread, INFINITE); CloseHandle(gAtEndThread); gAtEndThread = nullptr; }
    gBarrierNormCur = true; gBarrierByCount = true;
    Drain();
    EnterCriticalSection(&gFkLock);
    size_t lastAt = gFk.size(), callAt = gFk.size();
    for (size_t k = 0; k < gFk.size(); k++)
    {
        if (gFk[k].slot == 109 && gFk[k].a.size() >= 3 && gFk[k].a[0] == kEndReg && gFk[k].a[2] == c) lastAt = k;
        if (gFk[k].slot == 57 && gFk[k].tid == gOrdTid && gOrdTid) callAt = k;
    }
    const size_t nLog = gFk.size();
    gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0;
    LeaveCriticalSection(&gFkLock);
    const bool found = lastAt < nLog && callAt < nLog;
    printf("  %s: the fill %s, the hook %s, the call %s while parked; the last command at #%zu, the call at #%zu\n", how,
           filled ? "reached the end" : "COULD NOT START", gAtEndIn ? "fired" : "NEVER FIRED", parkedDone ? "landed" : "had not landed", lastAt, callAt);
    if (!filled || !gAtEndIn) Err("%s: the producer was never parked at the ring's end, so the case tested nothing", how);
    else if (!found) Err("%s: the last command or the other thread's call is missing from the device's log", how);
    else if (norm && callAt < lastAt) Err("%s: the call reached the device AHEAD of the command committed before it", how);
    else if (norm && !parkedDone) Err("%s: the call did not land while the producer was parked: the barrier did not publish through the end", how);
    else if (!norm && callAt > lastAt) Err("%s: with the end read raw the call still waited, so the case above proves nothing", how);
    else gAtEndRan++;
    { FkState keep = gFd->st; R = keep; }                                             // the two interleave: the reference restarts from the device
    RearmAfterForeign(how);
}
// THE SAME PARK AGAINST ANOTHER THREAD'S RESET (no control: the old reading published dtRingDw, which sends the worker into
// the last lap's commands). The Reset's idle path must publish through the end, see the worker finish, and disarm, with every
// command committed before it on the device ahead of the Reset.
static volatile LONG gAtEndResetDone = 0;
static DWORD WINAPI AtEndForeignReset(void*) { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); _InterlockedExchange(&gAtEndResetDone, 1); return 0; }
static void AtEndHookReset()
{
    _InterlockedExchange(&gAtEndIn, 1);
    gAtEndThread = CreateThread(nullptr, 0, AtEndForeignReset, nullptr, 0, nullptr);
    const ULONGLONG t0 = GetTickCount64();
    while (!gAtEndResetDone && GetTickCount64() - t0 < 2000) Sleep(1);                // past the barrier's 25 ms of quiet
}
static void ResetAtRingEnd()
{
    const char* how = "another thread's Reset while the producer is parked at the ring's end";
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    gAtEndIn = 0; gAtEndResetDone = 0; gAtEndThread = nullptr;
    UINT c = 0;
    gCommitEndHook = AtEndHookReset;
    const bool filled = FillToRingEnd(&c);
    gCommitEndHook = nullptr;
    const bool parkedDone = gAtEndResetDone != 0;
    if (gAtEndThread) { WaitForSingleObject(gAtEndThread, INFINITE); CloseHandle(gAtEndThread); gAtEndThread = nullptr; }
    EnterCriticalSection(&gFkLock);
    size_t lastAt = gFk.size(), resetAt = gFk.size();
    for (size_t k = 0; k < gFk.size(); k++)
    {
        if (gFk[k].slot == 109 && gFk[k].a.size() >= 3 && gFk[k].a[0] == kEndReg && gFk[k].a[2] == c) lastAt = k;
        if (gFk[k].slot == 16) resetAt = k;
    }
    const size_t n = gFk.size();
    gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0;
    LeaveCriticalSection(&gFkLock);
    printf("  %s: the fill %s, the hook %s, the Reset %s while parked, %s; the last command at #%zu, the Reset at #%zu of %zu\n", how,
           filled ? "reached the end" : "COULD NOT START", gAtEndIn ? "fired" : "NEVER FIRED", parkedDone ? "returned" : "had not returned",
           dtArmed ? "STILL ARMED" : "unarmed", lastAt, resetAt, n);
    if (!filled || !gAtEndIn) Err("%s: the producer was never parked at the ring's end, so the case tested nothing", how);
    else if (!parkedDone) Err("%s: the Reset did not complete while the producer was parked", how);
    else if (lastAt >= n || resetAt >= n || resetAt < lastAt) Err("%s: the Reset reached the device ahead of a command committed before it (last #%zu, Reset #%zu)", how, lastAt, resetAt);
    else if (dtArmed) Err("%s: the layer was left armed", how);
    else gAtEndRan++;
    R.Reset(&gFd->sc->bb);
    ResetRearm(how);
}

static void ForeignOrderedRecording()
{
    Drain();
    if (!dtArmed) { Err("the recording round needs the layer armed"); return; }
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    gOrdRs = D3DRS_STENCILMASK; gOrdVal = 0xF00Du; gOrdTid = 0; gOrdGo = 0; gOrdDone = 0; gBusyI = 0; gOrdSawI = 0;
    HANDLE h = CreateThread(nullptr, 0, ForeignSetOrderedLate, nullptr, 0, nullptr);
    gOrdGo = 1;
    const ULONGLONG t0 = GetTickCount64();
    LONG i = 0;
    // The value IS the ordinal, on one state the other thread does not touch, so the device's log says which was which.
    while (!gOrdDone && GetTickCount64() - t0 < 3000) { i++; D->SetRenderState(D3DRS_STENCILREF, (DWORD)i); gBusyI = i; }
    if (!gOrdDone) Err("another thread's Set did not come back within 3 s under a recording producer");
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    Drain();
    if (!dtArmed) Err("another thread's Set UNARMED a recording producer");
    if (dtDisarmAsk) Err("an ask was left up under a recording producer");
    EnterCriticalSection(&gFkLock);
    size_t at = gFk.size(); long lateBefore = 0, seenBefore = 0;
    for (size_t k = 0; k < gFk.size(); k++) if (gFk[k].tid == gOrdTid && gFk[k].slot == 57) at = k;
    const bool found = at < gFk.size();
    // EVERY producer Set that had returned before the other thread made its call must be on the device BEFORE that call.
    for (size_t k = 0; k < gFk.size(); k++)
    {
        if (gFk[k].slot != 57 || gFk[k].a.size() < 2 || gFk[k].a[0] != (DWORD)D3DRS_STENCILREF || (LONG)gFk[k].a[1] > gOrdSawI) continue;
        if (k < at) seenBefore++; else lateBefore++;
    }
    gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0;
    LeaveCriticalSection(&gFkLock);
    if (!found) Err("the other thread's Set never reached the device");
    if (lateBefore) Err("%ld Set(s) the producer had made BEFORE another thread's call reached the device AFTER it: the call went ahead of the ring", lateBefore);
    if (seenBefore != gOrdSawI) Err("%ld of the %ld Sets made before the other thread's call were found ahead of it", seenBefore, (long)gOrdSawI);
    printf("  recording producer: %ld Sets, %ld of them made before another thread's call and every one on the device ahead of it\n", (long)i, (long)gOrdSawI);
    { FkState keep = gFd->st; R = keep; }                                             // the two interleave in no fixed order: the reference restarts from the device
    gOrderedBusy++;
    RearmAfterForeign("another thread's SetRenderState under a recording producer");
}
// ANOTHER THREAD'S SET THAT LANDS INSIDE THE ARM'S OWN SHADOW LOAD (2026-09-19). The load reads the render
// states first and the render targets after them, and the fake fires a hook from inside GetRenderTarget(3):
// so a render state set from there is one the load has ALREADY read. The layer is unarmed at that instant
// (dtArmed goes up after the load), so no barrier stands in front of the Set, and until today it was not
// counted either: the fresh copy held the old value and answered with it. Now it is counted while unarmed,
// the load's own before-and-after compare sees the count move, and the first Get after the arm settles.
static long gArmRaceRan = 0;
static DWORD WINAPI ForeignSetArmRace(void*) { D->SetRenderState(D3DRS_STENCILWRITEMASK, 0x77u); return 0; }
static void ForeignDuringArm()
{
    Drain();
    { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); E(16, {}); R.Reset(&gFd->sc->bb); }   // a Reset unarms the layer
    if (dtArmed) { Err("the arm-race case could not unarm the layer"); return; }
    if (R.rs[D3DRS_STENCILWRITEMASK] == 0x77u) { D->SetRenderState(D3DRS_STENCILWRITEMASK, 0x78u); E(57, { (DWORD)D3DRS_STENCILWRITEMASK, 0x78u }); R.rs[D3DRS_STENCILWRITEMASK] = 0x78u; }
    static bool fired; fired = false;
    gFkMidRead = [] { fired = true; RunThread(ForeignSetArmRace); };
    bool noted = false;
    for (int i = 0; i < 300 && !dtArmed; i++)
    {
        Present();                                  // nothing else here asks for render target 3, so the hook fires in the ARM's load
        if (fired && !noted) { noted = true; E(57, { (DWORD)D3DRS_STENCILWRITEMASK, 0x77u }); R.rs[D3DRS_STENCILWRITEMASK] = 0x77u; }
    }
    if (gFkMidRead) { gFkMidRead = nullptr; Err("the arm never read render target 3: the hook did not fire"); }
    if (!dtArmed) { Err("the layer did not arm in the arm-race case"); return; }
    if (!noted) { Err("the other thread's Set was never made"); return; }
    DWORD v = 0; D->GetRenderState(D3DRS_STENCILWRITEMASK, &v); gGets++;
    if (v != 0x77u) Err("a Set from another thread landed inside the arm's shadow load and the copy still answers 0x%lX, not 0x77", v);
    gArmRaceRan++;
    for (int i = 0; i < 2000; i++) OneOp(true);
}

// THE OTHER END OF THE SAME WINDOW (2026-09-19). The count the shadow watches was raised only AFTER another
// thread's Set had returned, so between the moment DXVK holds the new value and that raise the producer's
// Get was answered from the copy: the old value, for a state the device no longer holds. GTA IV's RAGE puts
// back what such a Get tells it, which is half of how the black sky was carried. The count is raised BEFORE
// the call as well now. Here the fake holds the other thread INSIDE SetRenderState, after the state has
// changed, while the producer asks; the answer must be the new value. (At threaded=2 every Get asks the
// device anyway, so this bites at threaded=1, which is where the control runs it.)
static HANDLE gLandedEv = nullptr, gLetGoEv = nullptr;
static DWORD WINAPI ForeignSetHeldInside(void*) { D->SetRenderState(D3DRS_STENCILREF, 0x99u); return 0; }
static long gGetBesideSetRan = 0;
static void ForeignGetBesideSet()
{
    if (!gLandedEv) { gLandedEv = CreateEventW(nullptr, FALSE, FALSE, nullptr); gLetGoEv = CreateEventW(nullptr, FALSE, FALSE, nullptr); }
    D->SetRenderState(D3DRS_STENCILREF, 0x98u); E(57, { (DWORD)D3DRS_STENCILREF, 0x98u }); R.rs[D3DRS_STENCILREF] = 0x98u;   // the copy KNOWS the old value
    Drain();
    gFkAfterSet = [] { SetEvent(gLandedEv); WaitForSingleObject(gLetGoEv, 5000); };
    HANDLE h = CreateThread(nullptr, 0, ForeignSetHeldInside, nullptr, 0, nullptr);
    if (WaitForSingleObject(gLandedEv, 5000) != WAIT_OBJECT_0) { Err("the other thread's Set never reached the device"); SetEvent(gLetGoEv); }
    DWORD v = 0; D->GetRenderState(D3DRS_STENCILREF, &v); gGets++;
    if (v != 0x99u) Err("the device held another thread's 0x99 and the producer's Get answered 0x%lX from the copy", v);
    SetEvent(gLetGoEv);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    E(57, { (DWORD)D3DRS_STENCILREF, 0x99u }); R.rs[D3DRS_STENCILREF] = 0x99u;
    D->GetRenderState(D3DRS_STENCILREF, &v); gGets++;
    if (v != 0x99u) Err("after another thread's Set had returned the producer's Get answered 0x%lX", v);
    if (!dtArmed) Err("a Get beside another thread's Set unarmed the layer");
    gGetBesideSetRan++;
}

// THE BLACK SKY UNDER THE SECOND FORM, REPRODUCED (2026-09-19, the owner's launch of 12:54). Another thread's
// Set is WAITING for the worker (held at the gate, with an older Set of the same state queued behind it), and
// the PRODUCER asks for that state. Under the second form the copy answered at once with the queued, older
// value, because the count it watches was raised only when the wait ended; RAGE puts back what such a Get
// tells it, and the old depth stencil went back over the main thread's bind. Now the Get must NOT come back
// until the other thread's call has landed, and must then be the other thread's value. The producer is this
// thread and it is about to block, so a third thread opens the gate after 100 ms.
static DWORD WINAPI OpenGateLater(void*) { Sleep(100); SetEvent(gGateEv); return 0; }
static DWORD WINAPI ForeignSetWaiting(void*) { D->SetRenderState(D3DRS_STENCILREF, 0xB2u); return 0; }
static long gGetDuringWaitRan = 0;
static void ForeignGetDuringWait()
{
    Drain();
    if (!dtArmed) { Err("the Get-during-the-wait case needs the layer armed"); return; }
    const DWORD X = D3DRS_STENCILREF;
    if (!DtNote(kHarnessGate, &X, 4)) { Err("the gate note was refused"); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, 0xB1u); E(57, { X, 0xB1u });            // queued behind the gate: the copy KNOWS 0xB1
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("the worker never reached the gate"); SetEvent(gGateEv); return; }
    const LONG waited0 = dtShWaitedOut;
    const LONGLONG spun0 = dtShWaitedOutT, reload0 = dtWaitT[DT_R_RELOAD];
    HANDLE h = CreateThread(nullptr, 0, ForeignSetWaiting, nullptr, 0, nullptr);
    for (int spin = 0; !dtForeignInFlight && spin < 200000000; spin++) _mm_pause();   // the other thread's call has begun, and is waiting for the worker
    if (!dtForeignInFlight) Err("the other thread's call never went in flight");
    HANDLE g = CreateThread(nullptr, 0, OpenGateLater, nullptr, 0, nullptr);
    const ULONGLONG t0 = GetTickCount64();
    DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &v); gGets++;               // must wait the call out
    const ULONGLONG took = GetTickCount64() - t0;
    E(57, { X, 0xB2u }); R.rs[X] = 0xB2u;                                            // on the device AFTER the queued 0xB1
    if (v != 0xB2u) Err("another thread's Set was in flight and the producer's Get answered 0x%lX: RAGE would now put THAT back over it", v);
    if (took < 50) Err("the producer's Get came back in %llu ms, before the gate opened: it did not wait the call out", took);
    if (dtShWaitedOut != waited0 + 1) Err("the settle did not count a call waited out (%ld)", dtShWaitedOut - waited0);
    // THE WAIT IS TIMED AND REACHES THE VERDICT'S SUM (final audit 2026-09-19, F1.3): at least 40 of the 50 ms it took.
    {
        const LONGLONG ms40 = 40 * dtQpf / 1000;
        if (dtShWaitedOutT - spun0 < ms40) Err("the settle waited a call out for %llu ms and timed %lld ticks of it", took, dtShWaitedOutT - spun0);
        if (dtWaitT[DT_R_RELOAD] - reload0 < ms40) Err("the waited-out time did not reach the waits the report and the verdict sum");
        // THE CONTROL: a settle with NO call in flight must add nothing to the waited-out time.
        const LONGLONG spun1 = dtShWaitedOutT; const LONG waited1 = dtShWaitedOut;
        dtShNeedDrain = true;                                   // forces the next Get through DtShadowSettle
        DWORD v2 = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &v2); gGets++;
        if (dtShWaitedOutT != spun1 || dtShWaitedOut != waited1) Err("a settle with no call in flight was timed as a wait for one");
        if (v2 != 0xB2u) Err("the control's Get answered 0x%lX", v2);
    }
    WaitForSingleObject(h, INFINITE); CloseHandle(h); WaitForSingleObject(g, INFINITE); CloseHandle(g);
    if (dtForeignInFlight) Err("a call was left in flight (%ld)", (long)dtForeignInFlight);
    if (!dtArmed) Err("the Get-during-the-wait case unarmed the layer");
    gGetDuringWaitRan++;
}

// A BRACKET OF OURS WITH ANOTHER THREAD'S DEPTH-STENCIL BIND INSIDE IT (2026-09-19, the owner's fourth launch of
// the third form: the bind the main thread makes as the menu comes up was lost one time in eight, with every
// device-thread counter in order). A full-screen pass of ours asks which depth stencil is bound, binds none, draws
// and puts back what it was told; a bind from another thread that lands in between is overwritten by that
// put-back, for good, because the engine then reads the old surface and keeps putting THAT back. The rule under
// test is ds_bracket.h's: the bracket puts back the surface the other thread bound. The hook's half (the marks
// around the call) is made here by hand, exactly as hkSetDepthStencil makes it.
static IDirect3DSurface9* gBrkDs = nullptr;
static HRESULT gBrkHr = S_OK;
static DWORD WINAPI ForeignBindMarked(void*) { DsForeignBegin(gBrkDs); gBrkHr = D->SetDepthStencilSurface(gBrkDs); DsForeignEnd(gBrkHr); return 0; }
static long gBracketRan = 0;
static void BracketForeignBind()
{
    Drain();
    if (!dtArmed) { Err("the bracket case needs the layer armed"); return; }
    IDirect3DSurface9* const oldDs = gDs[0], * const newDs = gDs[1];
    D->SetDepthStencilSurface(oldDs); E(39, { P(oldDs) }); R.ds = oldDs;
    // 1. nobody else binds: the bracket puts back what it saved, and counts nothing
    const LONG crossed0 = dsBracketsCrossed;
    {
        const LONG seq = DsBracketOpen();
        IDirect3DSurface9* saved = nullptr; D->GetDepthStencilSurface(&saved); gGets++;
        D->SetDepthStencilSurface(nullptr); E(39, { 0u });
        IDirect3DSurface9* const back = DsBracketPutBack(saved, seq);
        if (back != oldDs) Err("an undisturbed bracket would put back %p, not the %p it saved", (void*)back, (void*)oldDs);
        D->SetDepthStencilSurface(back); E(39, { P(back) }); Rel(saved);
        if (dsBracketsCrossed != crossed0) Err("an undisturbed bracket was counted as crossed");
    }
    // 2. another thread's bind is REFUSED inside the bracket (a render target is no depth stencil): nothing changed, so the saved one goes back
    {
        const LONG seq = DsBracketOpen();
        IDirect3DSurface9* saved = nullptr; D->GetDepthStencilSurface(&saved); gGets++;
        gBrkDs = gRt[0]; RunThread(ForeignBindMarked); E(39, { P(gRt[0]) });
        if (SUCCEEDED(gBrkHr)) Err("the fake took a render target as a depth stencil: the refused case tests nothing");
        IDirect3DSurface9* const back = DsBracketPutBack(saved, seq);
        if (back != oldDs) Err("after a REFUSED bind from another thread the bracket would put back %p, not the %p it saved", (void*)back, (void*)oldDs);
        Rel(saved);
    }
    // 3. THE HAZARD: the other thread's bind lands between the bracket's question and its put-back
    {
        const LONG seq = DsBracketOpen();
        IDirect3DSurface9* saved = nullptr; D->GetDepthStencilSurface(&saved); gGets++;
        if (saved != oldDs) Err("the bracket was told %p, not %p", (void*)saved, (void*)oldDs);
        gBrkDs = newDs; RunThread(ForeignBindMarked); E(39, { P(newDs) }); R.ds = newDs;
        D->SetDepthStencilSurface(nullptr); E(39, { 0u });
        D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
        IDirect3DSurface9* const back = DsBracketPutBack(saved, seq);
        D->SetDepthStencilSurface(back); E(39, { P(back) }); Rel(saved);
        Drain();
        if (gFd->st.ds != newDs) Err("the bracket put the OLD depth stencil back over another thread's bind: the device holds %p, the other thread bound %p", (void*)gFd->st.ds, (void*)newDs);
        if (dsBracketsCrossed != crossed0 + 1) Err("the crossed bracket was counted %ld times, not once", dsBracketsCrossed - crossed0);
        IDirect3DSurface9* s = nullptr; D->GetDepthStencilSurface(&s); gGets++;
        if (s != newDs) Err("after the crossed bracket GetDepthStencilSurface = %p: the engine would now put THAT back every frame, the other thread bound %p", (void*)s, (void*)newDs);
        Rel(s);
    }
    if (dsForeignSeq & 1) Err("a bind was left marked in flight (%ld)", (long)dsForeignSeq);
    RearmAfterForeign("another thread's bind inside a bracket of ours");
    gBracketRan++;
}

// THE ENGINE'S OWN BRACKET (2026-09-19, the owner's black launch of 14:20 as the instrument recorded it): the render
// thread asks which depth stencil is bound and is told the OLD one; it binds none; the main thread binds the NEW one
// and its call returns; the render thread puts back what it was told. Until today that put-back landed on top and the
// engine was then told the old surface every frame, for the whole session. The hook's halves are made by hand here,
// exactly as hkGetDepthStencil and hkSetDepthStencil make them.
static long gEngineRan = 0;
static void EngineStalePutBack()
{
    Drain();
    if (!dtArmed) { Err("the engine put-back case needs the layer armed"); return; }
    IDirect3DSurface9* const oldDs = gDs[0], * const newDs = gDs[1];
    D->SetDepthStencilSurface(oldDs); E(39, { P(oldDs) }); R.ds = oldDs;
    const LONG stale0 = dsStalePutBacks;
    // 1. nobody else binds: the put-back of the answer goes to the device as it is
    {
        const LONG seq = DsBracketOpen();
        IDirect3DSurface9* told = nullptr; const HRESULT hr = D->GetDepthStencilSurface(&told); gGets++;
        DsAskedNote(told, SUCCEEDED(hr), seq);
        D->SetDepthStencilSurface(DsStalePutBack(nullptr)); E(39, { 0u });
        IDirect3DSurface9* const back = DsStalePutBack(told);
        if (back != oldDs) Err("with no other thread binding, the engine's put-back became %p, not the %p it was told", (void*)back, (void*)oldDs);
        D->SetDepthStencilSurface(back); E(39, { P(back) }); Rel(told);
        if (dsStalePutBacks != stale0) Err("an undisturbed put-back was counted as stale");
    }
    // 2. THE HAZARD
    {
        const LONG seq = DsBracketOpen();
        IDirect3DSurface9* told = nullptr; const HRESULT hr = D->GetDepthStencilSurface(&told); gGets++;
        DsAskedNote(told, SUCCEEDED(hr), seq);
        if (told != oldDs) Err("the engine was told %p, not %p", (void*)told, (void*)oldDs);
        D->SetDepthStencilSurface(DsStalePutBack(nullptr)); E(39, { 0u });                     // its deliberate unbind
        gBrkDs = newDs; RunThread(ForeignBindMarked); E(39, { P(newDs) }); R.ds = newDs;        // the main thread binds, and its call RETURNS
        if (DsStalePutBack(nullptr) != nullptr) Err("the engine's deliberate unbind would be rewritten: only a bind of the surface it was TOLD is a put-back");
        IDirect3DSurface9* const back = DsStalePutBack(told);                                  // the put-back, 11 microseconds later in the owner's log
        D->SetDepthStencilSurface(back); E(39, { P(back) }); Rel(told);
        Drain();
        if (gFd->st.ds != newDs) Err("the engine's put-back of the OLD depth stencil landed over another thread's bind: the device holds %p, the other thread bound %p", (void*)gFd->st.ds, (void*)newDs);
        if (dsStalePutBacks != stale0 + 1) Err("the stale put-back was counted %ld times, not once", dsStalePutBacks - stale0);
        // 3. the next question is answered with the new surface, and from then on a bind of the OLD one is the engine's own business
        const LONG seq2 = DsBracketOpen();
        IDirect3DSurface9* s = nullptr; const HRESULT hr2 = D->GetDepthStencilSurface(&s); gGets++;
        DsAskedNote(s, SUCCEEDED(hr2), seq2);
        if (s != newDs) Err("after the corrected put-back the engine is told %p, and would put THAT back every frame; the other thread bound %p", (void*)s, (void*)newDs);
        if (DsStalePutBack(oldDs) != oldDs) Err("a bind of the old surface made AFTER a fresh question was rewritten");
        if (DsStalePutBack(s) != newDs) Err("a put-back of a fresh answer was rewritten");
        Rel(s);
    }
    RearmAfterForeign("another thread's bind inside the engine's own bracket");
    gEngineRan++;
}

// ANOTHER THREAD'S CreateStateBlock BEHIND A HELD WORKER (final audit 2026-09-19, F1.7). DXVK captures the device inside
// the block's constructor. With the worker held at the gate and a Set of X queued behind it, a block made by another
// thread must WAIT, and must capture the QUEUED value: proved by changing X afterwards and Applying the block. THE CONTROL
// switches the ordering off and must see what it prevents: the call returns while the worker is still held, and the block
// holds the value from BEFORE the queued Set, a state the stream never had at that point.
static IDirect3DStateBlock9* gCsb = nullptr;
static volatile LONG gCsbDone = 0;
static long gCsbHeldRan = 0;
static DWORD WINAPI ForeignCsb(void*) { D->CreateStateBlock(D3DSBT_ALL, &gCsb); _InterlockedExchange(&gCsbDone, 1); return 0; }
static void ForeignStateBlockHeld(bool ordered)
{
    const char* how = ordered ? "another thread's CreateStateBlock behind a held worker" : "another thread's CreateStateBlock, CONTROL";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    const DWORD X = D3DRS_STENCILREF, old = R.rs[X], queued = old ^ 0x5Au, later = old ^ 0xA5u;
    if (!DtNote(kHarnessGate, &X, 4)) { Err("%s: the gate note was refused", how); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, queued); R.rs[X] = queued;               // queued BEFORE the other thread's call
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("%s: the worker never reached the gate", how); SetEvent(gGateEv); return; }
    gSyncOrdered = ordered; gCsb = nullptr; _InterlockedExchange(&gCsbDone, 0);
    HANDLE h = CreateThread(nullptr, 0, ForeignCsb, nullptr, 0, nullptr);
    Sleep(60);
    const bool early = gCsbDone != 0;                                                 // returned while the ring could not drain
    SetEvent(gGateEv);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    gSyncOrdered = true;
    Drain();
    if (!gCsb) { Err("%s: no block was made", how); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, later); Drain();
    gCsb->Apply(); Drain();
    const DWORD held = gFd->st.rs[X];                                                  // what the block captured
    gCsb->Release(); gCsb = nullptr;
    if (ordered)
    {
        if (early) Err("%s: the call returned while the worker was held with an older Set still queued", how);
        if (held != queued) Err("%s: the block captured 0x%lX, not the queued 0x%lX: it was made ahead of the queue", how, held, queued);
    }
    else if (!early || held != old) Err("%s: with the ordering OFF the block was not made ahead of the queue (early %d, captured 0x%lX, old 0x%lX), so the case above proves nothing", how, early, held, old);
    // Another thread's call lands in the log wherever it lands: both logs start again, and the reference follows the device.
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    R.rs[X] = held;
    if (!dtArmed) Err("%s: the layer was unarmed", how);
    printf("  %s: returned early %d, captured 0x%lX (old 0x%lX, queued 0x%lX)\n", how, early, held, old, queued);
    gCsbHeldRan++;
}

// C1 STEP 3a'S GATE: ANOTHER THREAD'S BeginStateBlock BEHIND A HELD WORKER (docs/reports/c1-design-2026-09-28, 1.9, P3). The
// producer queues Set X while the worker is held at a gate; another thread then Begins and Ends a block. Ordered, its Begin
// waits for the gate, so the queued Set reaches the DEVICE and the other thread's block records nothing. THE CONTROL turns the
// ordering off: the Begin goes ahead of the queue, the device records, and the worker's replay of Set X is diverted into the
// other thread's block, so the device keeps its old X. The control must show exactly that, or the case proves nothing.
static volatile LONG gFbDone = 0; static IDirect3DStateBlock9* volatile gFbBlk = nullptr; static HANDLE gFbEndEv = nullptr;
// The recording stays open until the producer says the worker has run what was queued: in the control that replay is what
// the open recording takes; ordered, the Begin itself came after it.
static DWORD WINAPI ForeignBeginEnd(void*)
{
    D->BeginStateBlock(); _InterlockedExchange(&gFbDone, 1);
    WaitForSingleObject(gFbEndEv, INFINITE);
    IDirect3DStateBlock9* b = nullptr; D->EndStateBlock(&b); gFbBlk = b;
    return 0;
}
static long gFbHeldRan = 0;
static void ForeignBeginHeld(bool ordered)
{
    const char* how = ordered ? "another thread's BeginStateBlock behind a held worker" : "another thread's BeginStateBlock, CONTROL";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    const DWORD X = D3DRS_STENCILREF, old = R.rs[X], queued = old ^ 0x3Cu;
    if (!DtNote(kHarnessGate, &X, 4)) { Err("%s: the gate note was refused", how); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, queued);                                 // queued BEFORE the other thread's Begin
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("%s: the worker never reached the gate", how); SetEvent(gGateEv); return; }
    gSyncOrdered = ordered; gFbBlk = nullptr; _InterlockedExchange(&gFbDone, 0);
    if (!gFbEndEv) gFbEndEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE h = CreateThread(nullptr, 0, ForeignBeginEnd, nullptr, 0, nullptr);
    Sleep(60);
    const bool early = gFbDone != 0;                                                  // Began while the ring could not drain
    SetEvent(gGateEv);
    for (int spin = 0; gGateIn && spin < 200000000; spin++) _mm_pause();              // the worker has left the gate
    Sleep(60);                                                                        // and run the queued Set
    SetEvent(gFbEndEv);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    gSyncOrdered = true;
    Drain();
    const DWORD dev = gFd->st.rs[X];
    if (!gFbBlk) Err("%s: the other thread's block was not made", how);
    else { if (gFbBlk->Release() != 0) Err("%s: the other thread's block outlived its last Release", how); gFbBlk = nullptr; }
    Drain();
    if (ordered)
    {
        if (early) Err("%s: the Begin returned while the worker was held with an older Set still queued", how);
        if (dev != queued) Err("%s: the device holds 0x%lX, not the queued 0x%lX: the Set was diverted into the other thread's block", how, dev, queued);
    }
    else if (!early || dev != old) Err("%s: with the ordering OFF the Begin did not go ahead of the queue (early %d, device 0x%lX, old 0x%lX), so the case above proves nothing", how, early, dev, old);
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    R.rs[X] = dev;
    if (!dtArmed) Err("%s: the layer was unarmed", how);
    printf("  %s: began early %d, the device holds 0x%lX (old 0x%lX, queued 0x%lX)\n", how, early, dev, old, queued);
    gFbHeldRan++;
}

// ---- C1'S HARNESS CASES (2026-09-28, step 2; docs/reports/c1-design-2026-09-28/03-proof-plan.txt, section 2) -------------
// Written against the product as it ships, where the switch has one arm, A: every truth and order check must PASS, and every
// NO-WAIT check must FAIL, by seeing the drained Apply it exists to rule out. At B (step 4) the same checks must pass.
// Every wait but the validator's (threaded=2 drains before each Get it checks: that is the check's own cost, not the path's).
static LONG WaitsHit() { LONG n = 0; for (int i = 0; i < DT_R_N; i++) if (i != DT_R_VALIDATE) n += dtWaitHit[i]; return n; }
struct NoWaitMark { LONG hit, sb; NoWaitMark() : hit(WaitsHit()), sb(dtWaitN[DT_R_SBAPPLY]) {} };
static long gNoWaitA = 0, gNoWaitB = 0, gNoWaitFellBack = 0;
// AT B AN APPLY THE RECORD CANNOT VOUCH FOR KEEPS TODAY'S DRAIN (a typed block, a tainted recording, values not known): in H2's
// random programs that is counted, not failed (gNoWaitMayFallBack); in every other case the block was made to be eligible,
// so a fallback there is the case's premise broken, and says so with the reason.
static bool gNoWaitMayFallBack = false;
// mustHit: the case holds the worker (a gate or a live body), so at A the Apply's drain must have found it busy.
static void NoWaitCheck(const char* what, const NoWaitMark& m, bool mustHit)
{
    const LONG hit = WaitsHit() - m.hit, sb = dtWaitN[DT_R_SBAPPLY] - m.sb;
    if (DT_SB_REC_ON() && dtSbLastWhy != DT_SBF_OK)
    {
        if (gNoWaitMayFallBack) gNoWaitFellBack++;
        else Err("%s: at B the Apply fell back to today's drain (%s)", what, dtSbLastWhy >= 0 && dtSbLastWhy < DT_SBF_N ? kDtSbWhyName[dtSbLastWhy] : "no answer");
        return;
    }
    if (DT_SB_REC_ON()) { if (hit || sb) Err("%s WAITED at B: %ld waits, %ld Apply drains", what, (long)hit, (long)sb); else gNoWaitB++; }
    else if (!sb || (mustHit && !hit)) Err("%s did not wait at A (%ld waits, %ld Apply drains): the no-wait check cannot see the drain it exists to rule out", what, (long)hit, (long)sb);
    else gNoWaitA++;
}

// C1'S BOOKKEEPING, PRICED (2026-09-29; the design's step 5 cost line, DECISIONS 02:40). It runs at both arms, so no A/B in the
// game sees it: a Capture of a recorded block copies the shadow's values into the record (DtSbCaptureCopy; at B after a pass
// that fetches any covered entry the shadow does not know), and every Apply is classified first (DtSbApplyTry). Both are
// timed here directly, on a light block (three render states, the shape of FusionFix's small effects) and a heavy one
// (sixteen render states, four samplers of five states, four textures, both shaders, the viewport), with every covered entry
// known so each call takes the whole path. At B the whole recorded Apply is timed too (the render thread's call, with the
// command committed). THE LOOPS MUST HAVE DONE THE WORK: the copy counter moves once a call, the classifier answers "would
// take it" once a call, the recorded Applies count once a call, or the line is refused. Afterwards each block is Captured
// for real, so the fake's block and the record agree again. The frame figure takes 36 of each (S5 counted 35 to 41 Applies
// a frame through D3DX).
static long gC1CostRan = 0;
static void C1Cost()
{
    Drain();
    if (!dtArmed || !dtSbProbeOk) { Err("C1's cost bench needs the layer armed and the probe passed"); return; }
    SbBegin();
    D->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u); E(57, { (DWORD)D3DRS_ALPHABLENDENABLE, 1u }); RRs(D3DRS_ALPHABLENDENABLE, 1u);
    D->SetRenderState(D3DRS_SRCBLEND, 5u); E(57, { (DWORD)D3DRS_SRCBLEND, 5u }); RRs(D3DRS_SRCBLEND, 5u);
    D->SetRenderState(D3DRS_DESTBLEND, 6u); E(57, { (DWORD)D3DRS_DESTBLEND, 6u }); RRs(D3DRS_DESTBLEND, 6u);
    IDirect3DStateBlock9* const lt = SbEnd(); if (!lt) return;
    static const DWORD kRs[16] = { D3DRS_ZENABLE, D3DRS_ZWRITEENABLE, D3DRS_ALPHATESTENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_CULLMODE,
        D3DRS_ZFUNC, D3DRS_ALPHAREF, D3DRS_ALPHAFUNC, D3DRS_ALPHABLENDENABLE, D3DRS_FOGENABLE, D3DRS_STENCILENABLE, D3DRS_COLORWRITEENABLE,
        D3DRS_SCISSORTESTENABLE, D3DRS_SRGBWRITEENABLE, D3DRS_BLENDOP };
    static const DWORD kSamp[5] = { D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_MAGFILTER, D3DSAMP_MINFILTER, D3DSAMP_MIPFILTER };
    SbBegin();
    for (int k = 0; k < 16; k++) { const DWORD v = 1u + (DWORD)k % 3; D->SetRenderState((D3DRENDERSTATETYPE)kRs[k], v); E(57, { kRs[k], v }); RRs(kRs[k], v); }
    for (int i = 0; i < 4; i++)
    {
        for (int t = 0; t < 5; t++) { const DWORD v = 1u + (DWORD)t % 2; D->SetSamplerState((DWORD)i, (D3DSAMPLERSTATETYPE)kSamp[t], v); E(69, { (DWORD)i, kSamp[t], v }); RSamp(i, kSamp[t], v); }
        IDirect3DBaseTexture9* const tx = gTex[(size_t)i % gTex.size()];
        D->SetTexture((DWORD)i, tx); E(65, { (DWORD)i, P(tx) }); RTex(i, tx);
    }
    D->SetVertexShader(gVs[0]); E(92, { P(gVs[0]) }); RVs(gVs[0]);
    D->SetPixelShader(gPs[0]); E(107, { P(gPs[0]) }); RPs(gPs[0]);
    { D3DVIEWPORT9 v = { 0, 0, 1280, 720, 0.0f, 1.0f }; D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }
    IDirect3DStateBlock9* const hv = SbEnd(); if (!hv) { SbRelease(lt); return; }
    // every covered entry known: a Get of each (an unknown one is fetched)
    auto known = [&]() {
        DWORD v = 0;
        for (int k = 0; k < 16; k++) D->GetRenderState((D3DRENDERSTATETYPE)kRs[k], &v);
        for (int i = 0; i < 4; i++)
        {
            for (int t = 0; t < 5; t++) D->GetSamplerState((DWORD)i, (D3DSAMPLERSTATETYPE)kSamp[t], &v);
            IDirect3DBaseTexture9* tx = nullptr; D->GetTexture((DWORD)i, &tx); Rel(tx);
        }
        IDirect3DVertexShader9* vs = nullptr; D->GetVertexShader(&vs); Rel(vs);
        IDirect3DPixelShader9* ps = nullptr; D->GetPixelShader(&ps); Rel(ps);
        D3DVIEWPORT9 vp; D->GetViewport(&vp);
    };
    known();
    SbCapture(lt); SbCapture(hv);          // both records' values known before the loops
    Drain(); known();
    LARGE_INTEGER q, t0, t1; QueryPerformanceFrequency(&q);
    const int N = 100000;
    auto ns = [&]() { return (double)(t1.QuadPart - t0.QuadPart) * 1e9 / (double)q.QuadPart / N; };
    double cp[2] = {}, cl[2] = {}, ap[2] = {};
    IDirect3DStateBlock9* const blk[2] = { lt, hv };
    for (int j = 0; j < 2; j++)
    {
        const LONG c0 = dtSbCapCopied;
        QueryPerformanceCounter(&t0);
        for (int i = 0; i < N; i++) DtSbCaptureCopy(blk[j]);
        QueryPerformanceCounter(&t1);
        cp[j] = ns();
        if (dtSbCapCopied - c0 != N) Err("C1 COST: %ld of %d Capture copies on the %s block took the copy: the line would measure nothing", (long)(dtSbCapCopied - c0), N, j ? "heavy" : "light");
    }
    if (!DT_SB_REC_ON())
    {
        for (int j = 0; j < 2; j++)
        {
            const LONG w0 = dtSbApplyWhy[DT_SBF_OK];
            QueryPerformanceCounter(&t0);
            for (int i = 0; i < N; i++) DtSbApplyTry(blk[j]);
            QueryPerformanceCounter(&t1);
            cl[j] = ns();
            if (dtSbApplyWhy[DT_SBF_OK] - w0 != N) Err("C1 COST: %ld of %d classifications of the %s block said \"would take it\"", (long)(dtSbApplyWhy[DT_SBF_OK] - w0), N, j ? "heavy" : "light");
        }
        printf("  C1 COST at A: a Capture's copy %.1f ns (light) %.1f ns (heavy); an Apply's classification %.1f / %.1f ns. At 36 of each a frame: %.2f to %.2f us\n",
               cp[0], cp[1], cl[0], cl[1], 36.0 * (cp[0] + cl[0]) / 1000.0, 36.0 * (cp[1] + cl[1]) / 1000.0);
    }
    else
    {
        for (int j = 0; j < 2; j++)
        {
            // FEW ENOUGH THAT THE RING NEVER FILLS: a full ring would time the worker's replay, not the render thread's call.
            const int NB = 2000;
            const LONG r0 = dtSbRecApplies;
            QueryPerformanceCounter(&t0);
            for (int i = 0; i < NB; i++) blk[j]->Apply();
            QueryPerformanceCounter(&t1);
            ap[j] = ns() * N / NB;
            for (int i = 0; i < NB; i++) E(1905, { P(blk[j]) });
            RefApply(*RefOf(blk[j]));
            if (dtSbRecApplies - r0 != NB) Err("C1 COST: %ld of %d Applies of the %s block were recorded", (long)(dtSbRecApplies - r0), NB, j ? "heavy" : "light");
            Drain();
        }
        printf("  C1 COST at B: a Capture's copy with its fetch pass %.1f ns (light) %.1f ns (heavy); a recorded Apply, the whole call, %.1f / %.1f ns. At 36 of each a frame: %.2f to %.2f us\n",
               cp[0], cp[1], ap[0], ap[1], 36.0 * (cp[0] + ap[0]) / 1000.0, 36.0 * (cp[1] + ap[1]) / 1000.0);
    }
    SbCapture(lt); SbCapture(hv);          // the fake's blocks catch up with what the loops copied into the records
    Drain();
    SbRelease(lt); SbRelease(hv);
    gC1CostRan++;
}

// C1'S RECORDS, CASE BY CASE (2026-09-29, step 3c; dt_sbtab.h). The random programs meet each rule by chance; these meet it
// on purpose. A recording of every kind the shadow answers, with the three the writer must treat as DXVK does (a null
// declaration never applied, a null stream's offset and stride zeroed, a viewport with MinZ = MaxZ lifted), against a device
// that then holds other values for all three, is Compared, which runs the oracle (NC4, NC6, NC7). A recorded FVF is clean
// too. A render state DXVK refuses taints its recording (NC10). And a fresh clean record would take the recorded path.
static long gC1RecRan = 0;
static DWORD gC1Scratch = 0;
static void C1Records()
{
    Drain();
    if (!dtArmed) { Err("C1's record cases need the layer armed"); return; }
    SbBegin();
    D->SetRenderState(D3DRS_ZENABLE, 1u); E(57, { (DWORD)D3DRS_ZENABLE, 1u }); RRs(D3DRS_ZENABLE, 1u);
    D->SetSamplerState(2, D3DSAMP_MINFILTER, 2u); E(69, { 2u, (DWORD)D3DSAMP_MINFILTER, 2u }); RSamp(2, D3DSAMP_MINFILTER, 2u);
    D->SetTexture(1, gTex[0]); E(65, { 1u, P(gTex[0]) }); RTex(1, gTex[0]);
    D->SetStreamSource(1, nullptr, 64, 32); E(100, { 1u, 0u, 64u, 32u }); RVb(1, nullptr, 64, 32);        // raw: the pair kept
    D->SetStreamSource(2, gVb[0], 128, 16); E(100, { 2u, P(gVb[0]), 128u, 16u }); RVb(2, gVb[0], 128, 16);
    D->SetIndices(gIb[0]); E(104, { P(gIb[0]) }); RIb(gIb[0]);
    D->SetVertexShader(gVs[0]); E(92, { P(gVs[0]) }); RVs(gVs[0]);
    D->SetPixelShader(gPs[0]); E(107, { P(gPs[0]) }); RPs(gPs[0]);
    { D3DVIEWPORT9 v = { 0, 0, 640, 480, 0.5f, 0.5f }; D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }   // lifted at Apply
    { RECT rc = { 1, 2, 300, 200 }; D->SetScissorRect(&rc); E(75, { 1u }, &rc, sizeof(rc)); RSc(rc); }
    D->SetVertexDeclaration(nullptr); E(87, { 0u }); RDecl(nullptr);                                        // never applied
    { float cc[8]; for (int j = 0; j < 8; j++) cc[j] = 0.25f * (float)(j + 1); D->SetPixelShaderConstantF(10, cc, 2); E(109, { 10u, 0u, 2u }, cc, 32); RPsc(10, cc, 2); }
    IDirect3DStateBlock9* const b = SbEnd(); if (!b) return;
    SbExpectRecord(b, DT_SBT_NONE, "C1: a recording of every kind the shadow answers");
    D->SetVertexDeclaration(gDecl[0]); E(87, { P(gDecl[0]) }); RDecl(gDecl[0]);
    D->SetStreamSource(1, gVb[0], 32, 16); E(100, { 1u, P(gVb[0]), 32u, 16u }); RVb(1, gVb[0], 32, 16);
    { D3DVIEWPORT9 v = { 0, 0, 800, 600, 0.0f, 1.0f }; D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }
    Compare("C1: a recording of every kind, then other state");
    const DWORD f = D3DFVF_XYZ | D3DFVF_TEX1;
    SbBegin(); D->SetFVF(f); E(89, { f }); RDecl(FkFvfDecl(f));
    IDirect3DStateBlock9* const b2 = SbEnd(); if (!b2) { SbRelease(b); return; }
    SbExpectRecord(b2, DT_SBT_NONE, "C1: a recorded FVF");
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)4, 0x44u); E(57, { 4u, 0x44u });                     // DXVK keeps nothing of it
    IDirect3DStateBlock9* const b3 = SbEnd(); if (!b3) { SbRelease(b); SbRelease(b2); return; }
    SbExpectRecord(b3, DT_SBT_RS, "C1: a recording with a render state DXVK refuses");
    Compare("C1: a recorded FVF, and a refused render state");
    SbApply(b2);
    if (dtSbLastWhy != DT_SBF_OK) Err("C1: the Apply of a fresh clean record would not take the recorded path (the answer was %ld)", (long)dtSbLastWhy);
    // CAPTURES, ON PURPOSE (review of 2026-09-29): every entry of b set anew outside a recording, so the copy knows them all and
    // they differ from what b holds; stream 2's device offset differs from the one b recorded, which a Capture never takes (NC5).
    // A Get first, so an older foreign call is settled BEFORE the Sets (a settle forgets every entry).
    D->GetRenderState(D3DRS_ZENABLE, &gC1Scratch); gGets++;
    D->SetRenderState(D3DRS_ZENABLE, 2u); E(57, { (DWORD)D3DRS_ZENABLE, 2u }); RRs(D3DRS_ZENABLE, 2u);
    D->SetSamplerState(2, D3DSAMP_MINFILTER, 3u); E(69, { 2u, (DWORD)D3DSAMP_MINFILTER, 3u }); RSamp(2, D3DSAMP_MINFILTER, 3u);
    D->SetTexture(1, gTex[1]); E(65, { 1u, P(gTex[1]) }); RTex(1, gTex[1]);
    D->SetStreamSource(1, gVb[1], 48, 24); E(100, { 1u, P(gVb[1]), 48u, 24u }); RVb(1, gVb[1], 48, 24);
    D->SetStreamSource(2, gVb[1], 256, 24); E(100, { 2u, P(gVb[1]), 256u, 24u }); RVb(2, gVb[1], 256, 24);
    D->SetIndices(gIb[1]); E(104, { P(gIb[1]) }); RIb(gIb[1]);
    D->SetVertexShader(gVs[1]); E(92, { P(gVs[1]) }); RVs(gVs[1]);
    D->SetPixelShader(gPs[1]); E(107, { P(gPs[1]) }); RPs(gPs[1]);
    { D3DVIEWPORT9 v = { 5, 6, 700, 500, 0.3f, 0.3f }; D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }
    { RECT rc = { 3, 4, 310, 210 }; D->SetScissorRect(&rc); E(75, { 1u }, &rc, sizeof(rc)); RSc(rc); }
    D->SetVertexDeclaration(gDecl[1]); E(87, { P(gDecl[1]) }); RDecl(gDecl[1]);
    { float cc[8]; for (int j = 0; j < 8; j++) cc[j] = 1.5f * (float)(j + 3); D->SetPixelShaderConstantF(10, cc, 2); E(109, { 10u, 0u, 2u }, cc, 32); RPsc(10, cc, 2); }
    SbCapture(b);
    SbExpectValues(b, true, "C1: a Capture over every kind, all known");
    Compare("C1: a Capture over every kind");
    // the FVF form: b2 covers the declaration, and a recorded SetFVF leaves the copy's declaration as an FVF. A value nothing
    // else uses, so C1-F's map (at B) cannot already name its object and turn the form into a named declaration.
    const DWORD f2 = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;
    D->SetFVF(f2); E(89, { f2 }); RDecl(FkFvfDecl(f2));
    SbCapture(b2);
    SbExpectValues(b2, true, "C1: a Capture over an FVF");
    Compare("C1: a Capture over an FVF");
    // an UNKNOWN declaration (the copy knows neither the object nor an FVF), and the pixel constants not followed
    Drain();
    dtSh.declOk = false; dtSh.declQueued = false; dtSh.fvfOk = false;
    const bool consts = dtShConsts; dtShConsts = false;
    SbCapture(b);
    dtShConsts = consts;
    SbExpectValues(b, true, "C1: a Capture over an unknown declaration, the constants not followed");
    Compare("C1: a Capture over an unknown declaration");
    // a Capture inside another recording: DXVK refuses it when the worker replays it, so the record must not follow it (NC12)
    D->SetRenderState(D3DRS_ZENABLE, 4u); E(57, { (DWORD)D3DRS_ZENABLE, 4u }); RRs(D3DRS_ZENABLE, 4u);
    D->GetRenderState(D3DRS_ZENABLE, &gC1Scratch); gGets++;
    SbBegin();
    SbCapture(b);                                   // the harness's helper counts the refused replay
    IDirect3DStateBlock9* const b4 = SbEnd();
    SbExpectValues(b, false, "C1: a Capture made inside another recording");
    Compare("C1: a Capture inside another recording");
    SbRelease(b); SbRelease(b2); SbRelease(b3); if (b4) SbRelease(b4);
    Drain();
    gC1RecRan++;
}

// C1'S TABLE, FULL (review of 2026-09-29): the mod keeps 64 records and the harness 1024, so it never saw a full table. Here the
// live limit is lowered to what is live plus three: of five recordings three get a record, two do not, and those two must be
// counted, answer "no record" at the Apply, and stay out of the oracle's "every live block has a record".
static long gC1FullRan = 0;
static void C1FullTable()
{
    Drain();
    if (!dtArmed) { Err("C1's full table needs the layer armed"); return; }
    LONG live = 0;
    AcquireSRWLockShared(&dtSbLock); for (int k = 0; k < DT_SB_CAP; k++) live += dtSbTab[k].kind != DT_SBK_FREE; ReleaseSRWLockShared(&dtSbLock);
    const LONG full0 = dtSbFull, not0 = dtSbNotStored;
    dtSbCapLimit = (int)live + 3;
    IDirect3DStateBlock9* bl[5] = {};
    int dropped = 0;
    for (int k = 0; k < 5; k++)
    {
        SbBegin(); const DWORD v = 0x0F10u + k; D->SetRenderState(D3DRS_TEXTUREFACTOR, v); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, v }); RRs(D3DRS_TEXTUREFACTOR, v);
        bl[k] = SbEnd();
        bool has = false;
        AcquireSRWLockShared(&dtSbLock); for (int j = 0; j < DT_SB_CAP; j++) has |= dtSbTab[j].kind != DT_SBK_FREE && dtSbTab[j].sb == bl[k]; ReleaseSRWLockShared(&dtSbLock);
        if (!has) { gOrSkip[bl[k]] = 1; dropped++; }
        if ((k < 3) != has) Err("C1's full table: recording %d of 5 %s a record with %ld live and room for 3", k + 1, has ? "got" : "did not get", (long)live);
    }
    if (dtSbFull - full0 != 2 || dtSbNotStored - not0 != 2 || dropped != 2)
        Err("C1's full table: %d recordings went without a record, counted full %ld and not stored %ld (each should be 2)", dropped, dtSbFull - full0, dtSbNotStored - not0);
    SbApply(bl[4]);
    if (dtSbLastWhy != DT_SBF_NOENTRY) Err("C1's full table: the Apply of a block with no record answered %ld, not \"no record\"", (long)dtSbLastWhy);
    Compare("C1: a full table");
    dtSbCapLimit = DT_SB_CAP;
    for (IDirect3DStateBlock9* x : bl) if (x) { gOrSkip.erase(x); SbRelease(x); }
    Drain();
    gC1FullRan++;
}


// AT B, A BLOCK THE RECORD CANNOT VOUCH FOR BY DESIGN (a typed block, another thread's recording, values an unarmed or another
// thread's Capture made unknown, another thread's call unsettled) must take today's drain, and for exactly that reason; every
// other case's block must take the recorded path with no wait (NoWaitCheck).
static long gNoWaitByDesign = 0;
static void NoWaitOrFallBack(const char* what, const NoWaitMark& m, bool mustHit, LONG atB)
{
    if (DT_SB_REC_ON() && atB != DT_SBF_OK)
    {
        if (dtSbLastWhy != atB)
            Err("%s: at B the Apply's path was \"%s\", not \"%s\"", what, dtSbLastWhy >= 0 && dtSbLastWhy < DT_SBF_N ? kDtSbWhyName[dtSbLastWhy] : "no answer", kDtSbWhyName[atB]);
        else gNoWaitByDesign++;
        return;
    }
    NoWaitCheck(what, m, mustHit);
}

// H1, AN APPLY BEHIND A HELD WORKER (the ObsGate shape). The gate note, Set X = v1, Apply a recorded block holding X = v0
// and Y = w0, Set Y = w2; then Get X and Get Y. The answers are v0 and w2, the device ends on them, and the log reads Set X,
// Apply, Set Y. At B the Gets come back while the worker is still held, with no wait; at A the Apply drains, so it waits
// for the gate, which a third thread opens after 100 ms. Variants: 1, X unknown to the copy beforehand; 2, the block
// released right after its Apply, so at B its queued Apply must still find it alive.
static long gH1Ran = 0;
static void H1HeldApply(int variant)
{
    Drain();
    if (!dtArmed) { Err("H1 needs the layer armed"); return; }
    const DWORD X = D3DRS_STENCILREF, Y = D3DRS_STENCILMASK;
    const DWORD v0 = R.rs[X] ^ 0x10u, w0 = R.rs[Y] ^ 0x20u, v1 = v0 ^ 0x1u, w2 = w0 ^ 0x2u;
    SbBegin();
    D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    D->SetRenderState((D3DRENDERSTATETYPE)Y, w0); E(57, { Y, w0 }); RRs(Y, w0);
    IDirect3DStateBlock9* blk = SbEnd();
    if (!blk) return;
    Drain();
    if (variant == 1) dtSh.rsOk[X] = false;                   // as a foreign Set's settle, or an A-arm Apply, leaves it
    if (!DtNote(kHarnessGate, &X, 4)) { Err("H1: the gate note was refused"); SbRelease(blk); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1); E(57, { X, v1 }); RRs(X, v1);
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("H1: the worker never reached the gate"); SetEvent(gGateEv); SbRelease(blk); return; }
    HANDLE g = CreateThread(nullptr, 0, OpenGateLater, nullptr, 0, nullptr);
    const NoWaitMark m;
    SbApply(blk);
    if (variant == 2) { SbRelease(blk); blk = nullptr; }
    D->SetRenderState((D3DRENDERSTATETYPE)Y, w2); E(57, { Y, w2 }); RRs(Y, w2);
    const bool held = gGateIn != 0;
    DWORD x = 0, y = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); D->GetRenderState((D3DRENDERSTATETYPE)Y, &y); gGets += 2;
    NoWaitCheck("H1: an Apply and two Gets behind a held worker", m, true);
    if (DT_SB_REC_ON() && !held) Err("H1 (variant %d): the gate was open before the Gets, so they were not asked beside a held worker", variant);
    if (x != v0 || y != w2) Err("H1 (variant %d): Get X = 0x%lX and Get Y = 0x%lX; the stream says 0x%lX and 0x%lX", variant, x, y, v0, w2);
    WaitForSingleObject(g, INFINITE); CloseHandle(g);
    Drain();
    if (gFd->st.rs[X] != v0 || gFd->st.rs[Y] != w2) Err("H1 (variant %d): the device ended on X = 0x%lX, Y = 0x%lX, not 0x%lX, 0x%lX", variant, gFd->st.rs[X], gFd->st.rs[Y], v0, w2);
    if (blk) SbRelease(blk);
    gH1Ran++;
}

// THE SWEEP AFTER AN APPLY (H2, H3): Gets of entries the block holds, sampled, against the reference. The shadow answers
// these; the SYNC kinds and the declaration and pixel constants (whose Gets drain when unknown, which the design allows)
// are swept apart, after the no-wait check.
struct SwEnt { int kind, a, b; };
static int SweepList(const RefBlk& b, SwEnt* out, int cap)
{
    int c = 0;
    auto add = [&](int kind, int a, int x) { const SwEnt e = { kind, a, x }; if (c < cap) out[c] = e; else { const int j = (int)(Rnd() % (unsigned)(c + 1)); if (j < cap) out[j] = e; } c++; };
    for (int s = 7; s < 210; s++) if (b.rs[s]) add(0, s, 0);
    for (int i = 0; i < 21; i++) for (int t = 1; t < 14; t++) if (b.samp[i][t]) add(1, i, t);
    for (int i = 0; i < 21; i++) if (b.tex[i]) add(2, i, 0);
    for (int i = 0; i < 16; i++) if (b.vb[i]) add(3, i, 0);
    if (b.vp) add(4, 0, 0);
    if (b.sc) add(5, 0, 0);
    if (b.vs) add(6, 0, 0);
    if (b.ps) add(7, 0, 0);
    if (b.ib) add(8, 0, 0);
    return c < cap ? c : cap;
}
static long gSweepGets = 0;
static void SweepGet(const SwEnt& e)
{
    gGets++; gSweepGets++;
    switch (e.kind)
    {
    case 0: { DWORD v = 0; D->GetRenderState((D3DRENDERSTATETYPE)e.a, &v); if (v != R.rs[e.a]) Err("sweep: GetRenderState(%d) = 0x%08lX, the reference says 0x%08lX", e.a, v, R.rs[e.a]); break; }
    case 1: { DWORD v = 0; D->GetSamplerState(SampNum(e.a), (D3DSAMPLERSTATETYPE)e.b, &v); if (v != R.samp[e.a][e.b]) Err("sweep: GetSamplerState(%d, %d) differs", e.a, e.b); break; }
    case 2: { IDirect3DBaseTexture9* t = nullptr; D->GetTexture(SampNum(e.a), &t); if (t != R.tex[e.a]) Err("sweep: GetTexture(%d) differs", e.a); Rel(t); break; }
    case 3: { IDirect3DVertexBuffer9* v = nullptr; UINT o = 0, st = 0; D->GetStreamSource(e.a, &v, &o, &st);
              if (v != R.vb[e.a] || o != R.off[e.a] || st != R.str[e.a]) Err("sweep: GetStreamSource(%d) = (%p, %u, %u), the reference says (%p, %u, %u)", e.a, (void*)v, o, st, (void*)R.vb[e.a], R.off[e.a], R.str[e.a]); Rel(v); break; }
    case 4: { D3DVIEWPORT9 v; D->GetViewport(&v); if (memcmp(&v, &R.vp, sizeof(v))) Err("sweep: GetViewport differs"); break; }
    case 5: { RECT r; D->GetScissorRect(&r); if (memcmp(&r, &R.sc, sizeof(r))) Err("sweep: GetScissorRect differs"); break; }
    case 6: { IDirect3DVertexShader9* x = nullptr; D->GetVertexShader(&x); if (x != R.vs) Err("sweep: GetVertexShader differs"); Rel(x); break; }
    case 7: { IDirect3DPixelShader9* x = nullptr; D->GetPixelShader(&x); if (x != R.ps) Err("sweep: GetPixelShader differs"); Rel(x); break; }
    default: { IDirect3DIndexBuffer9* x = nullptr; D->GetIndices(&x); if (x != R.ib) Err("sweep: GetIndices differs"); Rel(x); break; }
    }
}
// The declaration, a pixel constant the block holds, and one of each of the four SYNC kinds it holds.
static void SweepDrained(const RefBlk& b)
{
    if (b.decl) { IDirect3DVertexDeclaration9* x = nullptr; D->GetVertexDeclaration(&x); gGets++; if (x != R.decl) Err("sweep: GetVertexDeclaration differs"); Rel(x); }
    for (int k = 0; k < 4; k++)
    {
        const UINT r = Rnd() % 224;
        if (!b.psc[r]) continue;
        float c[4]; D->GetPixelShaderConstantF(r, c, 1); gGets++;
        if (memcmp(c, R.psc[r], 16)) Err("sweep: GetPixelShaderConstantF(%u) differs", r);
        break;
    }
    for (int k = 0; k < 4; k++)
    {
        const UINT r = Rnd() % 256;
        if (!b.vsc[r]) continue;
        float c[4]; D->GetVertexShaderConstantF(r, c, 1); E(95, { r, 1u }); gGets++;
        if (memcmp(c, R.vsc[r], 16)) Err("sweep: GetVertexShaderConstantF(%u) differs", r);
        break;
    }
    for (int k = 0; k < 8; k++)                          // a stage state the block holds
    {
        const UINT g = Rnd() % 8, t = Rnd() % 32;
        if (!b.tss[g][t]) continue;
        DWORD v = 0xDEADu; D->GetTextureStageState(g, (D3DTEXTURESTAGESTATETYPE)(t + 1), &v); E(66, { g, t + 1 }); gGets++;
        if (v != R.tss[g][t]) Err("sweep: GetTextureStageState(%u, %u) differs", g, t + 1);
        break;
    }
    for (int k = 0; k < 4; k++)                          // a stream frequency
    {
        const UINT i = Rnd() % 16;
        if (!b.freq[i]) continue;
        UINT f = 0xDEADu; D->GetStreamSourceFreq(i, &f); E(103, { i }); gGets++;
        if (f != R.freq[i]) Err("sweep: GetStreamSourceFreq(%u) = 0x%X, the reference says 0x%X", i, f, R.freq[i]);
        break;
    }
    for (int k = 0; k < 4; k++)                          // a transform
    {
        const int x = (int)(Rnd() % 14);
        if (!b.xf[x]) continue;
        D3DMATRIX m; D->GetTransform((D3DTRANSFORMSTATETYPE)kXfType[x], &m); E(45, { kXfType[x] }); gGets++;
        if (memcmp(&m, &R.xf[x], 64)) Err("sweep: GetTransform(%lu) differs", kXfType[x]);
        break;
    }
}
static void SweepBlock(IDirect3DStateBlock9* sb, const NoWaitMark& m, const char* what, bool mustHit)
{
    const RefBlk& b = *RefOf(sb);
    SwEnt e[24]; const int n = SweepList(b, e, 24);
    for (int i = 0; i < n; i++) SweepGet(e[i]);
    NoWaitCheck(what, m, mustHit);
    SweepDrained(b);
}

// H2, RANDOM PROGRAMS: the ordinary traffic with state block calls mixed in. Begin (refused while recording) and End (the
// recorder handed over; refused, and End(nullptr), when there is nothing to end), Create of every type and of an invalid
// one (refused while recording), Capture and Apply (both refused by DXVK while recording: the Capture after the layer
// answered D3D_OK, counted as a failed replay), AddRef and Release (a block may outlive a Release that leaves a reference).
// After every Apply the block's entries are swept against the reference, and at A the Apply must have been a drain.
struct H2Blk { IDirect3DStateBlock9* sb; LONG refs; };
static std::vector<H2Blk> gH2Live;
static long gH2Ops[8] = {}, gH2Applies = 0;
static void H2Release(size_t k)
{
    IDirect3DStateBlock9* const sb = gH2Live[k].sb;
    const LONG want = --gH2Live[k].refs;
    if (!want) { const auto it = gRefBlk.find(sb); if (it != gRefBlk.end()) { delete it->second; gRefBlk.erase(it); } gOrSkip.erase(sb); gH2Live.erase(gH2Live.begin() + k); }
    const ULONG got = sb->Release();
    if ((LONG)got != want) Err("H2: a state block's Release returned %lu, not %ld", got, (long)want);
}
static void H2Step()
{
    // A recording ends after about 25 steps (the design's 0 to 40 Sets), now and then with an End(nullptr) DXVK refuses first.
    if (rRec && Rnd() % 25 == 0)
    {
        gH2Ops[1]++;
        if (Rnd() % 8) { if (IDirect3DStateBlock9* sb = SbEnd()) gH2Live.push_back({ sb, 1 }); }
        else { const HRESULT hr = D->EndStateBlock(nullptr); E(61, {}); if (hr != D3DERR_INVALIDCALL) Err("H2: EndStateBlock(nullptr) answered 0x%08lX", (unsigned long)hr); }
        return;
    }
    const unsigned r = Rnd() % 100;
    if (r < 90 || (gH2Live.empty() && r >= 94)) { OneOp(false); return; }
    switch (r)
    {
    case 90:                                                     // Begin, or a second Begin DXVK refuses
        gH2Ops[0]++;
        if (!rRec) SbBegin();
        else { const HRESULT hr = D->BeginStateBlock(); E(60, {}); if (hr != D3DERR_INVALIDCALL) Err("H2: a second BeginStateBlock answered 0x%08lX", (unsigned long)hr); }
        break;
    case 91: case 92:                                            // End, End with nothing recording, End(nullptr)
        gH2Ops[1]++;
        if (Rnd() & 1) { const HRESULT hr = D->EndStateBlock(nullptr); E(61, {}); if (hr != D3DERR_INVALIDCALL) Err("H2: EndStateBlock(nullptr) answered 0x%08lX", (unsigned long)hr); }
        else if (rRec) { if (IDirect3DStateBlock9* sb = SbEnd()) gH2Live.push_back({ sb, 1 }); }
        else { IDirect3DStateBlock9* sb = nullptr; const HRESULT hr = D->EndStateBlock(&sb); E(61, {}); if (hr != D3DERR_INVALIDCALL || sb) Err("H2: an EndStateBlock with nothing recording answered 0x%08lX", (unsigned long)hr); }
        break;
    case 93:                                                     // Create, of each type and of none
    {
        gH2Ops[2]++;
        static const DWORD kT[4] = { D3DSBT_ALL, D3DSBT_PIXELSTATE, D3DSBT_VERTEXSTATE, 0 };
        const DWORD t = kT[Rnd() % 4];
        if (rRec || !t)
        {
            IDirect3DStateBlock9* sb = nullptr; const HRESULT hr = D->CreateStateBlock((D3DSTATEBLOCKTYPE)t, &sb); E(59, { t });
            if (hr != D3DERR_INVALIDCALL || sb) Err("H2: a CreateStateBlock(%lu) DXVK refuses answered 0x%08lX", t, (unsigned long)hr);
        }
        else if (gH2Live.size() < 24) { if (IDirect3DStateBlock9* sb = SbCreate((D3DSTATEBLOCKTYPE)t)) gH2Live.push_back({ sb, 1 }); }
        break;
    }
    case 94: case 95:                                            // Capture
        gH2Ops[3]++;
        SbCapture(gH2Live[Rnd() % gH2Live.size()].sb);
        break;
    case 96: case 97:                                            // Apply, and the sweep
    {
        gH2Ops[4]++;
        IDirect3DStateBlock9* const sb = gH2Live[Rnd() % gH2Live.size()].sb;
        const NoWaitMark m;
        SbApply(sb);
        if (!rRec) { gH2Applies++; gNoWaitMayFallBack = true; SweepBlock(sb, m, "H2: an Apply and the sweep of its block", false); gNoWaitMayFallBack = false;
                     if (Rnd() % 8 == 0) { Drain(); SbOracle("H2: after an Apply"); } }
        break;
    }
    case 98:                                                     // AddRef
    { gH2Ops[5]++; H2Blk& b = gH2Live[Rnd() % gH2Live.size()]; b.sb->AddRef(); b.refs++; break; }
    default:                                                     // Release
        gH2Ops[6]++;
        H2Release(Rnd() % gH2Live.size());
        break;
    }
}
static void H2Program(int steps)
{
    Drain();
    for (int i = 0; i < steps; i++) H2Step();
    if (rRec) { if (IDirect3DStateBlock9* sb = SbEnd()) gH2Live.push_back({ sb, 1 }); }
    while (!gH2Live.empty()) H2Release(gH2Live.size() - 1);
    printf("  H2: %d steps; Begin %ld, End %ld, Create %ld, Capture %ld, Apply %ld (%ld swept), AddRef %ld, Release %ld\n",
        steps, gH2Ops[0], gH2Ops[1], gH2Ops[2], gH2Ops[3], gH2Ops[4], gH2Applies, gH2Ops[5], gH2Ops[6]);
}

// H3, THE THREE SHAPES BEHIND A LIVE BODY. A body is posted that holds the worker 400 us and touches nothing, and then:
//   the sprite (D3DX's ID3DXSprite, five times a frame): Capture three save blocks, Apply three set blocks, a UP draw,
//     Apply the three save blocks (docs/reports/step5-s5 and fusionfix-call-sites.md, "HOW THE COUNTS FIT");
//   the effect (D3DX's state saving): Capture five save blocks, Sets and a draw, Apply the five;
//   Wine's builtin D3DX (REASONING, from memory of Wine's source): one recorded block, Capture, direct Sets, Apply.
// The blocks hold render states, sampler states, the viewport, the scissor, pixel constants and two textures nothing
// kills; half were recorded before the layer armed (H3RecordUnarmed), half after. The entries they hold are swept while the
// body runs. At A the first Apply waits for the body.
static IDirect3DStateBlock9* gH3Save[5] = {}, * gH3Set[3] = {};
static IDirect3DTexture9* gH3Tex[2] = {};
static LONGLONG gH3HoldTicks = 0;
static long gH3Frames = 0, gH3BodyStillIn = 0;
static void ObsHold(DWORD kind, const DWORD*, DWORD)
{
    if (kind != DT_NOTE_INJECT) { gNotesBad++; return; }
    _InterlockedExchange(&gBodyIn, 1);
    LARGE_INTEGER t0, t; QueryPerformanceCounter(&t0);
    do { _mm_pause(); QueryPerformanceCounter(&t); } while (t.QuadPart - t0.QuadPart < gH3HoldTicks);
    _InterlockedExchange(&gBodyIn, 0);
}
static const DWORD kH3Rs[8] = { D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_ZENABLE, D3DRS_CULLMODE,
                                D3DRS_ALPHATESTENABLE, D3DRS_FOGENABLE, D3DRS_LIGHTING };
// k picks a block's own values; a save block's values are only a starting point, since it is Captured before each use.
static IDirect3DStateBlock9* H3Record(int k)
{
    SbBegin();
    for (int j = 0; j < 8; j++)
        if ((k + j) % 3 != 0) { const DWORD s = kH3Rs[j], v = 0x1000u * (k + 1) + s; D->SetRenderState((D3DRENDERSTATETYPE)s, v); E(57, { s, v }); RRs(s, v); }
    for (int i = 0; i < 2; i++)
    {
        const DWORD v = 1 + (k + i) % 3;
        D->SetSamplerState(i, D3DSAMP_MAGFILTER, v); E(69, { (DWORD)i, (DWORD)D3DSAMP_MAGFILTER, v }); RSamp(i, D3DSAMP_MAGFILTER, v);
        D->SetSamplerState(i, D3DSAMP_ADDRESSU, v); E(69, { (DWORD)i, (DWORD)D3DSAMP_ADDRESSU, v }); RSamp(i, D3DSAMP_ADDRESSU, v);
    }
    if (k & 1) { IDirect3DTexture9* t = gH3Tex[k & 2 ? 1 : 0]; D->SetTexture(0, t); E(65, { 0u, P(t) }); RTex(0, t); }
    if (k % 3 == 0) { D3DVIEWPORT9 v = { 0, 0, 320u + 16 * k, 240u + 8 * k, 0.0f, 1.0f }; D->SetViewport(&v); E(47, { 1u }, &v, sizeof(v)); RVp(v); }
    if (k % 3 == 1) { RECT rc = { 0, 0, 100 + k, 80 + k }; D->SetScissorRect(&rc); E(75, { 1u }, &rc, sizeof(rc)); RSc(rc); }
    float c[8]; for (int j = 0; j < 8; j++) c[j] = (float)(k * 8 + j);
    D->SetPixelShaderConstantF(4 * k, c, 2); E(109, { 4u * k, 0u, 2u }, c, 32); RPsc(4 * k, c, 2);
    return SbEnd();
}
static void H3RecordUnarmed()
{
    for (int i = 0; i < 2; i++) gH3Tex[i] = NewTex();
    for (int k = 0; k < 3; k++) gH3Save[k] = H3Record(k);
    gH3Set[0] = H3Record(10);
}
static void H3Shapes()
{
    Drain();
    if (!dtArmed) { Err("H3 needs the layer armed"); return; }
    for (int k = 3; k < 5; k++) gH3Save[k] = H3Record(k);
    for (int k = 1; k < 3; k++) gH3Set[k] = H3Record(10 + k);
    // C1's records of them are clean: no other thread, nothing refused, recorded unarmed (0 to 2 and set 0) or armed (NC18, NC23)
    for (int k = 0; k < 5; k++) SbExpectRecord(gH3Save[k], DT_SBT_NONE, k < 3 ? "H3: a save block recorded unarmed" : "H3: a save block recorded armed");
    for (int k = 0; k < 3; k++) SbExpectRecord(gH3Set[k], DT_SBT_NONE, k < 1 ? "H3: a set block recorded unarmed" : "H3: a set block recorded armed");
    { LARGE_INTEGER q; QueryPerformanceFrequency(&q); gH3HoldTicks = q.QuadPart * 400 / 1000000; }
    dtNoteObs[DT_NOTE_INJECT] = ObsHold; dtNoteDrives[DT_NOTE_INJECT] = true;
    dtBodySampFree = 0x1FFFFFu; memset(dtBodyRsTouched, 0, sizeof(dtBodyRsTouched));   // the body touches nothing the shadow would fetch
    static BYTE vbuf[3 * 16];
    const DWORD pay[4] = {};
    for (int f = 0; f < 90; f++)
    {
        Drain();
        SbOracle("H3: a frame of the three shapes");
        if (!DtNote(DT_NOTE_INJECT, pay, sizeof(pay))) { Err("H3: the body note was refused"); continue; }
        DtPublishFenced();
        for (int spin = 0; !gBodyIn && spin < 200000000; spin++) _mm_pause();
        if (!gBodyIn) { Err("H3: the worker never entered the body"); continue; }
        const NoWaitMark m;
        IDirect3DStateBlock9* swept = nullptr;
        switch (f % 3)
        {
        case 0:                                                  // the sprite, five times
            for (int rr = 0; rr < 5; rr++)
            {
                for (int k = 0; k < 3; k++) SbCapture(gH3Save[k]);
                for (int k = 0; k < 3; k++) SbApply(gH3Set[k]);
                D->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vbuf, 16); E(83, { (DWORD)D3DPT_TRIANGLELIST, 1u, 0u, 16u }, vbuf, 48); R.Up(false);
                for (int k = 0; k < 3; k++) SbApply(gH3Save[k]);
            }
            swept = gH3Save[f % 5 < 3 ? f % 5 : 0];
            break;
        case 1:                                                  // the effect
            for (int k = 0; k < 5; k++) SbCapture(gH3Save[k]);
            for (int j = 0; j < 8; j++) { const DWORD s = kH3Rs[j], v = Rnd(); D->SetRenderState((D3DRENDERSTATETYPE)s, v); E(57, { s, v }); RRs(s, v); }
            D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
            for (int k = 0; k < 5; k++) SbApply(gH3Save[k]);
            swept = gH3Save[f % 5];
            break;
        default:                                                 // Wine's builtin shape
        {
            IDirect3DStateBlock9* const b = gH3Save[(f / 3) % 5];
            SbCapture(b);
            for (int j = 0; j < 4; j++) { const DWORD s = kH3Rs[(f + j) % 8], v = Rnd(); D->SetRenderState((D3DRENDERSTATETYPE)s, v); E(57, { s, v }); RRs(s, v); }
            SbApply(b);
            swept = b;
            break;
        }
        }
        if (gBodyIn) gH3BodyStillIn++;
        SweepBlock(swept, m, "H3: a shape's Applies and the sweep behind a live body", true);
        gH3Frames++;
        Present();
    }
    Drain();
    dtNoteObs[DT_NOTE_INJECT] = nullptr; dtNoteDrives[DT_NOTE_INJECT] = false; dtBodySampFree = 0;
    memset(dtBodyRsTouched, 0xFF, sizeof(dtBodyRsTouched));
    for (IDirect3DStateBlock9*& b : gH3Save) { SbRelease(b); b = nullptr; }
    for (IDirect3DStateBlock9*& b : gH3Set) { SbRelease(b); b = nullptr; }
    // A set block's Apply may have left one of the two bound: unbound first, as KillBoundTexture does, or its death
    // would leave the device naming a dead texture.
    for (IDirect3DTexture9*& t : gH3Tex)
    {
        for (int j = 0; j < 21; j++)
            if (R.tex[j] == (IDirect3DBaseTexture9*)t) { D->SetTexture(SampNum(j), nullptr); E(65, { SampNum(j), 0u }); RTex(j, nullptr); }
        if (t->Release() != 0) Err("H3: the last Release of one of its textures did not return 0");
        t = nullptr;
    }
    Drain();
    if (gFd->blocksLive) Err("H3: %ld state block(s) still alive after it released its own", (long)gFd->blocksLive);
    printf("  H3: %ld frames of the three shapes behind a live body, the body still running at the sweep on %ld\n", gH3Frames, gH3BodyStillIn);
    if (DT_SB_REC_ON() && gH3BodyStillIn * 5 < gH3Frames * 4) Err("H3 at B: the body had finished before the sweep on %ld of %ld frames, so the sweep was not beside it", gH3Frames - gH3BodyStillIn, gH3Frames);
}

// H4, ADDRESS REUSE (C1's R12; 03-proof-plan, H4). A records X = a and is Applied, so C1's per-block record would learn it;
// A dies; the fake hands its memory to the next block (gFkReuseSB), made four ways: a typed Create, the producer's
// Begin/End, the producer's Begin/End across its own Reset and re-arm, and another thread's Begin/End. The new block's Apply
// must give ITS values, never the dead block's. At A every Apply drains; at B (step 4) the record must have been overwritten
// on each path (NC15 skips one).
static long gH4Ran = 0, gSbReused = 0;
static IDirect3DStateBlock9* volatile gH4Foreign = nullptr;
static DWORD WINAPI H4ForeignRecord(void*)
{
    D->BeginStateBlock();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x4444u);
    D->SetRenderState(D3DRS_AMBIENT, 0x5555u);
    IDirect3DStateBlock9* b = nullptr; D->EndStateBlock(&b); gH4Foreign = b;
    return 0;
}
static void H4Reuse()
{
    Drain();
    if (!dtArmed) { Err("H4 needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, Y = D3DRS_AMBIENT;
    for (int way = 0; way < 4; way++)
    {
        const DWORD a = 0x1100u + way;
        SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, a); E(57, { X, a }); RRs(X, a);
        IDirect3DStateBlock9* A = SbEnd(); if (!A) return;
        SbApply(A); Drain();
        SbRelease(A);
        Drain();                                                 // armed, the Release is queued: the worker makes A dead
        if (way == 2)
        {   // across the producer's own Reset (every block dead, as the game's order has it) and a re-arm
            if (gFd->blocksLive) { Err("H4: %ld block(s) alive before its Reset", (long)gFd->blocksLive); return; }
            { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); E(16, {}); R.Reset(&gFd->sc->bb); }
            for (int i = 0; i < 300 && !dtArmed; i++) Present();
            if (!dtArmed) { Err("H4: the layer did not arm again after its Reset"); return; }
        }
        if (way == 3) Compare("H4: the producer's traffic before another thread records");
        gFkReuseSB = (FakeSB*)A;
        IDirect3DStateBlock9* B = nullptr;
        const DWORD bx = 0x2200u + way, by = 0x3300u + way;
        if (way == 0)
        {   // a typed Create takes X (a pixel state) from the device at once
            D->SetRenderState((D3DRENDERSTATETYPE)X, bx); E(57, { X, bx }); RRs(X, bx);
            B = SbCreate(D3DSBT_PIXELSTATE);
        }
        else if (way == 3)
        {   // another thread's recording: its calls land in the log wherever they land, so both logs start again, and the
            // reference takes the block as that thread recorded it (the ForeignCsb pattern)
            Drain();
            HANDLE h = CreateThread(nullptr, 0, H4ForeignRecord, nullptr, 0, nullptr);
            WaitForSingleObject(h, INFINITE); CloseHandle(h);
            Drain();
            B = gH4Foreign; gH4Foreign = nullptr;
            EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
            if (B) { RefBlk* rb = new RefBlk(); rb->rs[X] = rb->rs[Y] = true; rb->v.rs[X] = 0x4444u; rb->v.rs[Y] = 0x5555u; gRefBlk[B] = rb; }
        }
        else
        {
            SbBegin();
            D->SetRenderState((D3DRENDERSTATETYPE)X, bx); E(57, { X, bx }); RRs(X, bx);
            D->SetRenderState((D3DRENDERSTATETYPE)Y, by); E(57, { Y, by }); RRs(Y, by);
            B = SbEnd();
        }
        if (!B) { Err("H4 (way %d): no block was made", way); gFkReuseSB = nullptr; return; }
        if (B != A) Err("H4 (way %d): the fake did not hand the dead block's memory back", way); else gSbReused++;
        Drain(); SbOracle("H4: a block at a dead block's address");
        D->SetRenderState((D3DRENDERSTATETYPE)X, 0x9900u); E(57, { X, 0x9900u }); RRs(X, 0x9900u);
        D->SetRenderState((D3DRENDERSTATETYPE)Y, 0x9901u); E(57, { Y, 0x9901u }); RRs(Y, 0x9901u);
        const NoWaitMark m;
        SbApply(B);
        DWORD x = 0, y = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); D->GetRenderState((D3DRENDERSTATETYPE)Y, &y); gGets += 2;
        NoWaitOrFallBack("H4: an Apply of a block at a dead block's address", m, false, way == 0 ? DT_SBF_TYPED : way == 3 ? DT_SBF_TAINTED : DT_SBF_OK);
        if (x != R.rs[X] || y != R.rs[Y]) Err("H4 (way %d): Get X = 0x%lX, Y = 0x%lX; the reference says 0x%lX, 0x%lX", way, x, y, R.rs[X], R.rs[Y]);
        if (x == a) Err("H4 (way %d): Get X answered the DEAD block's value 0x%lX", way, a);
        SbRelease(B);
        gH4Ran++;
    }
    Drain();
    printf("  H4: %ld ways, %ld blocks made at a dead block's address\n", gH4Ran, gSbReused);
}

// H11, LIFETIME (03-proof-plan, H11): a block released while an extra reference is held lives on and applies its own values;
// only the final Release ends it. At B the record must survive the first Release and retire at the last (NC25 retires it on
// every Release).
static long gH11Ran = 0;
static void H11Lifetime()
{
    Drain();
    if (!dtArmed) { Err("H11 needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x6611u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    SbApply(b);
    b->AddRef();
    const ULONG left = b->Release();
    if (left != 1) Err("H11: a Release with an extra reference held returned %lu, not 1", left);
    Drain(); SbOracle("H11: after a Release that left a reference");
    D->SetRenderState((D3DRENDERSTATETYPE)X, v0 ^ 1u); E(57, { X, v0 ^ 1u }); RRs(X, v0 ^ 1u);
    const NoWaitMark m;
    SbApply(b);
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    NoWaitCheck("H11: an Apply of a block whose first Release left a reference", m, false);
    if (x != v0) Err("H11: Get X = 0x%lX after the Apply; the block holds 0x%lX", x, v0);
    SbRelease(b);
    Drain();
    if (gFd->blocksLive) Err("H11: %ld state block(s) alive after its final Release", (long)gFd->blocksLive);
    gH11Ran++;
}

// ANOTHER THREAD, RUN TO ITS END WHILE THE PRODUCER WAITS (C1's H5 and H7). Its calls land in the fake's log wherever they
// land, so the producer's traffic is compared first and both logs start again after it (the ForeignCsb pattern).
static void (*gC1Fn)() = nullptr;
static DWORD WINAPI C1Foreign(void*) { gC1Fn(); return 0; }
static void C1RunForeign(const char* what, void (*fn)())
{
    Compare(what);
    Drain();
    gC1Fn = fn;
    HANDLE h = CreateThread(nullptr, 0, C1Foreign, nullptr, 0, nullptr);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
}

// C1-F ACROSS ANOTHER THREAD'S DECLARATION (review 2026-09-30, C1-R2). Behind a recorded SetFVF the producer's Get drains and asks
// DXVK, and C1-F learns the answer as DXVK's table object for that FVF. Another thread's SetVertexDeclaration that reaches DXVK
// between the drain and the Get (here: inside it, the fake's GetVertexDeclaration runs the other thread to its end before it
// answers) makes the answer that thread's object, and C1-F must not learn the pair: at B a wrong pair is never corrected. The
// oracle's C1-F check (SbOracle) finds a wrong pair; NC36 learns it anyway and must fail there.
static long gC1FvfRan = 0;
static HANDLE gFvfThread = nullptr;
static DWORD WINAPI FvfForeignDecl(void*) { D->SetVertexDeclaration(gDecl[0]); return 0; }
static void FvfOnGetDecl()
{
    if (gFvfThread) return;                                   // the case's own Get only
    gFvfThread = CreateThread(nullptr, 0, FvfForeignDecl, nullptr, 0, nullptr);
    if (gFvfThread) WaitForSingleObject(gFvfThread, INFINITE);
}
static void C1FvfForeign()
{
    Compare("C1-F across another thread's declaration: before");
    Drain();
    if (!dtArmed) { Err("C1-F across another thread's declaration needs the layer armed"); return; }
    const DWORD f = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX3;     // a value nothing else uses, so no pair for it is known
    const unsigned learned0 = dtShFvfObjN;
    D->SetFVF(f); E(89, { f }); RDecl(FkFvfDecl(f));
    gFvfThread = nullptr; gFkOnGetDecl = FvfOnGetDecl;
    IDirect3DVertexDeclaration9* got = nullptr;
    D->GetVertexDeclaration(&got); gGets++;
    gFkOnGetDecl = nullptr;
    if (!gFvfThread) Err("C1-F across another thread's declaration: the Get never reached the fake, so nothing landed inside it");
    else { CloseHandle(gFvfThread); gFvfThread = nullptr; }
    if (got != gDecl[0]) Err("C1-F across another thread's declaration: the Get answered %p, not the other thread's %p", (void*)got, (void*)gDecl[0]);
    if (got) got->Release();
    for (unsigned i = learned0; i < dtShFvfObjN; i++)
        if (dtShFvfObj[i].fvf == f && dtShFvfObj[i].obj == gDecl[0])
            Err("C1-F across another thread's declaration: FVF 0x%lX was learned as the other thread's declaration %p", f, (void*)gDecl[0]);
    Drain();
    SbOracle("C1-F across another thread's declaration");
    // the other thread's call landed wherever it landed: both logs start again (C1RunForeign's pattern), the device holds its object
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    RDecl(gDecl[0]);
    gC1FvfRan++;
}

// H10, FAILURES (03-proof-plan, H10; C1's R5). An Apply the fake fails (gFkApplyHr) changes nothing on the device, and the next
// Get must say so. At A the Apply is drained and made here, so its caller sees the failure. At B (step 4) it is recorded: the
// caller sees D3D_OK, the worker's replay fails, is counted against the Apply's own code, and raises the foreign count so the
// next Get settles and re-reads (NC24: counted as a resource method, no re-sync, must fail).
static long gH10Ran = 0;
static void H10Failures()
{
    Drain();
    if (!dtArmed) { Err("H10 needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x7711u, v1 = 0x7722u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1); E(57, { X, v1 }); RRs(X, v1);
    Drain();
    _InterlockedExchange(&gFkApplyHr, (LONG)D3DERR_INVALIDCALL);
    const HRESULT hr = b->Apply(); E(1905, { P(b) });
    if (DT_SB_REC_ON()) { if (hr != D3D_OK) Err("H10: a recorded Apply answered 0x%08lX, not D3D_OK", (unsigned long)hr); gExpReplayErr++; }
    else if (hr != D3DERR_INVALIDCALL) Err("H10: a failed Apply answered 0x%08lX at A, not the fake's failure", (unsigned long)hr);
    Drain();                                                      // at B the Apply is recorded: the worker's replay takes the failure
    if (gFkApplyHr) { Err("H10: the fake's failure was never taken: the Apply did not reach it"); _InterlockedExchange(&gFkApplyHr, 0); }
    if (DT_SB_REC_ON() && !ReplayErrHas(DT_WHO_SBAPPLY, D3DERR_INVALIDCALL)) Err("H10: a recorded Apply that failed on the worker was not counted against its own code");
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    if (x != v1) Err("H10: Get X = 0x%lX after a failed Apply; the device holds 0x%lX", x, v1);
    // the same block, not failed, then applies as ever
    const NoWaitMark m;
    SbApply(b);
    D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    NoWaitCheck("H10: an Apply after a failed one", m, false);
    if (x != v0) Err("H10: Get X = 0x%lX after the block's next Apply; it holds 0x%lX", x, v0);
    SbRelease(b);
    Drain();
    gH10Ran++;
}

// H6, A RESET BETWEEN A CAPTURE AND AN APPLY (03-proof-plan, H6 (a) and (c); C1's R13). With a block alive DXVK refuses the
// Reset, but the barrier has disarmed the layer first (so the ring is empty at the refused call). Unarmed, a Set and a
// Capture go straight to DXVK: the block now holds what that Capture took. Armed again, its Apply must give THAT, never
// what the block held when it was last Captured or Applied armed (NC19 keeps the stale record at B, and must fail).
static long gH6Ran = 0;
static void H6ResetBetween()
{
    Drain();
    if (!dtArmed) { Err("H6 needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x8811u, v2 = 0x8833u, v3 = 0x8844u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    SbApply(b);
    { DWORD k = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &k); gGets++; }   // X known, so C1's record takes it at the Capture
    SbCapture(b);                                                 // armed: Applied and Captured, so a record would hold v0
    SbExpectValues(b, true, "H6: the producer's Capture over a known entry");
    Drain();
    const LONG refusedBefore = gFkResetRefused;
    { D3DPRESENT_PARAMETERS pp = {}; const HRESULT hr = D->Reset(&pp); E(16, {});
      if (hr != D3DERR_INVALIDCALL) Err("H6: a Reset with a block alive answered 0x%08lX, not DXVK's refusal", (unsigned long)hr);
      R.Reset(&gFd->sc->bb); }                                    // DXVK clears the state FIRST, then refuses (d3d9_device.cpp:465-517)
    if (gFkResetRefused != refusedBefore + 1) Err("H6: the fake did not count the refused Reset");
    _InterlockedExchange(&gFkResetRefused, refusedBefore);        // this case's, on purpose (as SB10's)
    if (dtArmed) Err("H6: the refused Reset left the layer armed");
    D->SetRenderState((D3DRENDERSTATETYPE)X, v2); E(57, { X, v2 }); RRs(X, v2);
    SbCapture(b);                                                 // unarmed: straight to DXVK, the block takes v2
    Drain(); SbOracle("H6: after an unarmed Capture");           // NC19 keeps the record's v0 as known
    D->SetRenderState((D3DRENDERSTATETYPE)X, v3); E(57, { X, v3 }); RRs(X, v3);
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { Err("H6: the layer did not arm again after the refused Reset"); SbRelease(b); return; }
    const NoWaitMark m;
    SbApply(b);
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    NoWaitOrFallBack("H6: an Apply after an unarmed Capture across a disarm", m, false, DT_SBF_VALS);
    if (x != v2) Err("H6: Get X = 0x%lX; the block's unarmed Capture took 0x%lX (it held 0x%lX when armed)", x, v2, v0);
    SbRelease(b);
    Drain();
    gH6Ran++;
}

// H7, AN UNKNOWN ENTRY AT CAPTURE (03-proof-plan, H7, the first of its three ways; C1's R6 and T2). Another thread's Set makes
// X unknown to the copy; the producer Captures a block holding X BEFORE any Get settles, then Applies it. The Apply must give
// the other thread's value. At B (step 4) the Capture fetches X or marks the block's values invalid, never copies a stale X
// (NC20 copies it as known, and must fail).
static long gH7Ran = 0;
static void H7Foreign() { D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x9955u); }
static void H7UnknownAtCapture()
{
    Drain();
    if (!dtArmed) { Err("H7 needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x9911u, v3 = 0x9933u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    D->SetRenderState((D3DRENDERSTATETYPE)X, v0 ^ 1u); E(57, { X, v0 ^ 1u }); RRs(X, v0 ^ 1u);
    C1RunForeign("H7: the producer's traffic before another thread's Set", H7Foreign);
    R.rs[X] = 0x9955u;
    SbCapture(b);                                                 // no Get between: X is unknown to the copy here
    D->SetRenderState((D3DRENDERSTATETYPE)X, v3); E(57, { X, v3 }); RRs(X, v3);
    const NoWaitMark m;
    SbApply(b);
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    NoWaitOrFallBack("H7: an Apply of a block Captured over an entry another thread had changed", m, false, DT_SBF_VALS);
    if (x != 0x9955u) Err("H7: Get X = 0x%lX; the block Captured the other thread's 0x9955", x);
    SbRelease(b);
    Drain();
    gH7Ran++;
}

// H7 (b), A CAPTURE WITH ANOTHER THREAD'S SET IN FLIGHT AND THE ENTRY STILL KNOWN (review of 2026-09-29; NC27). The copy knows X,
// but another thread's Set of X is inside the device: at the moment between the copy and the command (the test hook) the
// record's values must already be unknown. The recheck after the commit would catch it too, which is why the hook looks first.
static long gH7bRan = 0; static bool gH7bSaw = true; static IDirect3DStateBlock9* gH7bBlk = nullptr;
static DWORD WINAPI H7bForeignSlow(void*) { gFkSlowSetMs = 60; gFkSlowSetTid = GetCurrentThreadId(); D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x7BFFu); gFkSlowSetTid = 0; return 0; }
static void H7bLook() { gH7bSaw = SbRecordValsOk(gH7bBlk); }
static void H7bUnsettledKnown()
{
    Drain();
    if (!dtArmed) { Err("H7 (b) needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x7B11u, v1 = 0x7B22u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    { DWORD k = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &k); gGets++; }         // an older foreign call settled first
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1); E(57, { X, v1 }); RRs(X, v1);   // X known in the copy
    Compare("H7 (b): before another thread's Set");
    Drain();
    _InterlockedExchange(&gFkSlowSetIn, 0);
    HANDLE h = CreateThread(nullptr, 0, H7bForeignSlow, nullptr, 0, nullptr);
    for (int spin = 0; !gFkSlowSetIn && spin < 200000000; spin++) _mm_pause();
    if (!gFkSlowSetIn) Err("H7 (b): the other thread's Set never went inside the device");
    gH7bBlk = b; gH7bSaw = true; gCapBetween = H7bLook;
    b->Capture();
    if (gCapBetween) { Err("H7 (b): the Capture's test hook did not run"); gCapBetween = nullptr; }
    Drain();                                                   // the Capture replays now, before the other thread's Set lands
    RefCapture(*RefOf(b), false);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    Drain();
    R.rs[X] = 0x7BFFu;
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    if (gH7bSaw) Err("H7 (b): between the copy and the command the record's values were known, with another thread's Set of X inside the device");
    SbExpectValues(b, false, "H7 (b): a Capture with another thread's Set in flight");
    SbOracle("H7 (b): after the Capture");
    SbRelease(b);
    Drain();
    gH7bRan++;
}
// THE CAPTURE'S RECHECK AFTER ITS COMMAND (design 1.4, step 7; review of 2026-09-29; NC28). The copy finds nothing unsettled and
// takes the values; then, before the command is in the ring, another thread's Set begins (the test hook starts it). It may land
// before the Capture's replay, so after the commit the record's values must be unknown.
static long gCapRecheckRan = 0;
static DWORD WINAPI CapRecheckForeign(void*) { D->SetRenderState(D3DRS_STENCILMASK, 0x7C33u); return 0; }
static HANDLE gCapRecheckThread = nullptr;
static void CapRecheckStart()
{
    const LONG f0 = dtForeign;
    gCapRecheckThread = CreateThread(nullptr, 0, CapRecheckForeign, nullptr, 0, nullptr);
    for (int spin = 0; !dtForeignInFlight && dtForeign == f0 && spin < 200000000; spin++) _mm_pause();
}
static void CapRecheck()
{
    Drain();
    if (!dtArmed) { Err("the Capture's recheck needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x7C11u, v1 = 0x7C22u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    { DWORD k = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &k); gGets++; }         // an older foreign call settled first
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1); E(57, { X, v1 }); RRs(X, v1);
    Compare("the Capture's recheck: before");
    Drain();
    gCapBetween = CapRecheckStart;
    SbCapture(b);
    if (gCapBetween) { Err("the Capture's recheck: the test hook did not run"); gCapBetween = nullptr; }
    if (gCapRecheckThread) { WaitForSingleObject(gCapRecheckThread, INFINITE); CloseHandle(gCapRecheckThread); gCapRecheckThread = nullptr; }
    Drain();
    R.rs[D3DRS_STENCILMASK] = 0x7C33u;
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    SbExpectValues(b, false, "the Capture's recheck: another thread's Set began between the copy and the command");
    SbRelease(b);
    Drain();
    gCapRecheckRan++;
}

// C1'S UNARMED AND CROSS-ARM CASES (review of 2026-09-29). The layer is disarmed by the producer, as a Reset would, and armed
// again by 300 Presents after. Unarmed, D3DX records its blocks (P2), and only the bracket sees another thread's Set:
//   U1 another thread's tracked Set inside the render thread's recording: "another thread (bracketed)" (NC26 has no bracket);
//   U2 the same Set still inside the device when the recording ends: "another thread's Set still inside DXVK";
//   U3 a recording opened by another thread while the render thread is not yet known, the render thread named inside it and
//      its Set, not bracketed, going into that recording: "off the render thread" (NC31 does not look again at the close);
// and armed: U5, a recording across a disarm and an arm, "an arm or disarm inside it" (NC29 compares the armed flag only), and
// H5 (f), another thread's End of the producer's recording: "ended by another thread", the layer disarmed by the barrier so
// the producer's later Set is followed, and the window shut by its owner's own refused End (NC30 has no barrier).
static long gC1UnarmedRan = 0;
static DWORD WINAPI U1Foreign(void*) { D->SetRenderState(D3DRS_STENCILMASK, 0x0B01u); return 0; }
static DWORD WINAPI U2ForeignSlow(void*) { gFkSlowSetMs = 30; gFkSlowSetTid = GetCurrentThreadId(); D->SetRenderState(D3DRS_STENCILREF, 0x0B02u); gFkSlowSetTid = 0; return 0; }
static HANDLE gU3Ready = nullptr, gU3Go = nullptr; static IDirect3DStateBlock9* volatile gU3Blk = nullptr;
static DWORD WINAPI U3Other(void*)
{
    D->BeginStateBlock();
    SetEvent(gU3Ready); WaitForSingleObject(gU3Go, INFINITE);
    IDirect3DStateBlock9* x = nullptr; D->EndStateBlock(&x); gU3Blk = x;
    return 0;
}
static IDirect3DStateBlock9* volatile gH5fBlk = nullptr; static HRESULT gH5fHr = S_OK;
static DWORD WINAPI H5fForeignEnd(void*) { IDirect3DStateBlock9* x = nullptr; gH5fHr = D->EndStateBlock(&x); gH5fBlk = x; return 0; }
static void C1Unarmed()
{
    Drain();
    if (!dtArmed) { Err("C1's unarmed cases need the layer armed going in"); return; }
    DtDisarm("C1's unarmed cases");
    if (dtArmed) { Err("C1's unarmed cases: the layer did not disarm"); return; }
    // U1
    SbBegin();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x0B00u); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, 0x0B00u }); RRs(D3DRS_TEXTUREFACTOR, 0x0B00u);
    C1RunForeign("U1: the render thread's unarmed recording before another thread's Set", [] { HANDLE h = CreateThread(nullptr, 0, U1Foreign, nullptr, 0, nullptr); WaitForSingleObject(h, INFINITE); CloseHandle(h); });
    rRec->rs[D3DRS_STENCILMASK] = true; rRec->v.rs[D3DRS_STENCILMASK] = 0x0B01u;   // DXVK's recorder is the device's
    IDirect3DStateBlock9* const u1 = SbEnd();
    SbExpectRecord(u1, DT_SBT_UFOREIGN, "U1: another thread's Set inside an unarmed recording");
    Compare("U1: another thread's Set inside an unarmed recording");
    // U2
    SbBegin();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x0B10u); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, 0x0B10u }); RRs(D3DRS_TEXTUREFACTOR, 0x0B10u);
    Compare("U2: before another thread's slow Set");
    _InterlockedExchange(&gFkSlowSetIn, 0);
    HANDLE h2 = CreateThread(nullptr, 0, U2ForeignSlow, nullptr, 0, nullptr);
    for (int spin = 0; !gFkSlowSetIn && spin < 200000000; spin++) _mm_pause();
    if (!gFkSlowSetIn) Err("U2: the other thread's Set never went inside the device");
    IDirect3DStateBlock9* const u2 = SbEnd();       // ends while that Set sleeps inside the device, before it has landed
    WaitForSingleObject(h2, INFINITE); CloseHandle(h2);
    R.rs[D3DRS_STENCILREF] = 0x0B02u;                // it landed after the End, on the device
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    SbExpectRecord(u2, DT_SBT_USTUCK, "U2: another thread's Set still inside the device at the End");
    Compare("U2: another thread's Set inside the device at the End");
    // U3
    const DWORD rt = renderTid;
    renderTid = 0;
    gU3Ready = CreateEventW(nullptr, FALSE, FALSE, nullptr); gU3Go = CreateEventW(nullptr, FALSE, FALSE, nullptr); gU3Blk = nullptr;
    HANDLE h3 = CreateThread(nullptr, 0, U3Other, nullptr, 0, nullptr);
    WaitForSingleObject(gU3Ready, INFINITE);
    renderTid = rt;                                  // the render thread becomes known inside the other thread's recording
    D->SetRenderState(D3DRS_BLENDFACTOR, 0x0B03u);  // not bracketed now: it goes into that recording, and the device keeps its own
    SetEvent(gU3Go);
    WaitForSingleObject(h3, INFINITE); CloseHandle(h3); CloseHandle(gU3Ready); CloseHandle(gU3Go);
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    if (!gU3Blk) Err("U3: the other thread's recording made no block");
    else
    {
        SbExpectRecord(gU3Blk, DT_SBT_OTHERTHREAD, "U3: the render thread named inside another thread's recording");
        SbOracle("U3: another thread's recording with the render thread's Set in it");
        gU3Blk->Release(); gU3Blk = nullptr;
    }
    // armed again
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { Err("C1's unarmed cases: the layer did not arm again"); return; }
    SbRelease(u1); SbRelease(u2);
    // U5, with the arm deferral switched off so the arm can land inside the recording (the generation's check behind it)
    SbBegin();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x0B50u); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, 0x0B50u }); RRs(D3DRS_TEXTUREFACTOR, 0x0B50u);
    DtDisarm("C1's U5: a disarm inside a recording");
    D->SetRenderState(D3DRS_STENCILMASK, 0x0B51u); E(57, { (DWORD)D3DRS_STENCILMASK, 0x0B51u }); RRs(D3DRS_STENCILMASK, 0x0B51u);
    gArmDefer = false;
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    gArmDefer = true;
    IDirect3DStateBlock9* const u5 = SbEnd();
    if (!u5) return;
    if (!dtArmed) Err("U5: the layer did not arm again inside the recording with the deferral off");
    SbExpectRecord(u5, DT_SBT_ARM, "U5: a disarm and an arm inside one recording");
    Compare("U5: a disarm and an arm inside one recording");
    SbRelease(u5);
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    // U6, THE ARM DEFERRAL ITSELF (design 1.10): a recording open across the Presents that would arm the layer holds the arm off,
    // so the arm's dtSbRec = false cannot let the copy follow Sets DXVK is diverting into a block (NC32: no deferral)
    SbBegin();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x0B60u); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, 0x0B60u }); RRs(D3DRS_TEXTUREFACTOR, 0x0B60u);
    DtDisarm("C1's U6: a disarm inside a recording, the arm held off");
    const LONG deferred0 = dtSbArmDeferred;
    for (int i = 0; i < 330 && !dtArmed; i++) Present();
    const bool armedInside = dtArmed;
    IDirect3DStateBlock9* const u6 = SbEnd();
    if (armedInside) Err("U6: the layer armed while a recording was open (the arm was not held off)");
    else if (dtSbArmDeferred == deferred0) Err("U6: the arm was not counted as held off");
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { Err("U6: the layer did not arm after the recording ended"); return; }
    if (u6) { SbExpectRecord(u6, DT_SBT_ARM, "U6: a disarm inside a recording"); SbRelease(u6); }
    Compare("U6: the arm held off while a recording is open");
    // H5 (f)
    Drain();
    SbBegin();
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x0F51u); E(57, { (DWORD)D3DRS_TEXTUREFACTOR, 0x0F51u }); RRs(D3DRS_TEXTUREFACTOR, 0x0F51u);
    gH5fBlk = nullptr;
    C1RunForeign("H5 (f): the producer's recording before another thread's End", [] { HANDLE h = CreateThread(nullptr, 0, H5fForeignEnd, nullptr, 0, nullptr); WaitForSingleObject(h, INFINITE); CloseHandle(h); });
    IDirect3DStateBlock9* const fb = gH5fBlk;
    if (FAILED(gH5fHr) || !fb) { Err("H5 (f): another thread's End of the producer's recording was refused (0x%08lX)", (unsigned long)gH5fHr); return; }
    gRefBlk[fb] = rRec; rRec = nullptr;              // the block is the other thread's now
    SbExpectRecord(fb, DT_SBT_FOREIGNEND, "H5 (f): another thread's End of the producer's recording");
    if (dtArmed) Err("H5 (f): the layer stayed armed after another thread ended the producer's recording (no barrier)");
    { IDirect3DStateBlock9* x = nullptr; const HRESULT hr = D->EndStateBlock(&x); E(61, {});
      if (hr != D3DERR_INVALIDCALL || x) Err("H5 (f): the producer's own End answered 0x%08lX, not DXVK's refusal", (unsigned long)hr); }
    if (dtSbWinDev) Err("H5 (f): the producer's window is still open after its own End was refused");
    const DWORD q = 0x0F52u;
    D->SetRenderState(D3DRS_STENCILREF, q); E(57, { (DWORD)D3DRS_STENCILREF, q }); R.rs[D3DRS_STENCILREF] = q;
    { DWORD got = 0; D->GetRenderState(D3DRS_STENCILREF, &got); gGets++;
      if (got != q) Err("H5 (f): after another thread's End, the producer's Set of 0x%lX was not followed: the copy answers 0x%lX", q, got); }
    Compare("H5 (f): another thread's End of the producer's recording");
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { Err("H5 (f): the layer did not arm again"); return; }
    SbRelease(fb);
    Drain();
    gC1UnarmedRan++;
}

// H5 (b), ANOTHER THREAD'S CAPTURE, THEN THE PRODUCER'S APPLY (03-proof-plan, H5 (b); C1's R9). The producer's block learned
// X = v0; another thread Captures it over X = vf; the producer's Apply must give vf (NC16, the foreign Capture not
// invalidating the record at B, must fail).
static long gH5bRan = 0;
static IDirect3DStateBlock9* gH5bBlk = nullptr;
static void H5bForeign() { gH5bBlk->Capture(); }
static void H5bForeignCapture()
{
    Drain();
    if (!dtArmed) { Err("H5 (b) needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0xAA11u, vf = 0xAA22u, v3 = 0xAA33u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    SbApply(b);
    { DWORD k = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &k); gGets++; }   // X known, so C1's record takes it at the Capture
    SbCapture(b);                                                 // the producer's own: a record would hold v0
    SbExpectValues(b, true, "H5 (b): the producer's Capture over a known entry");
    D->SetRenderState((D3DRENDERSTATETYPE)X, vf); E(57, { X, vf }); RRs(X, vf);
    gH5bBlk = b;
    C1RunForeign("H5 (b): the producer's traffic before another thread's Capture", H5bForeign);
    RefCapture(*RefOf(b), false);                                 // the other thread's Capture took vf
    SbOracle("H5 (b): after another thread's Capture");          // NC16 keeps the record's v0 as known
    D->SetRenderState((D3DRENDERSTATETYPE)X, v3); E(57, { X, v3 }); RRs(X, v3);
    const NoWaitMark m;
    SbApply(b);
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    NoWaitOrFallBack("H5 (b): the producer's Apply of a block another thread Captured", m, false, DT_SBF_VALS);
    if (x != vf) Err("H5 (b): Get X = 0x%lX; the other thread's Capture took 0x%lX (the producer's had 0x%lX)", x, vf, v0);
    SbRelease(b);
    Drain();
    gH5bRan++;
}

// H5 (a), ANOTHER THREAD'S APPLY BEHIND A HELD WORKER (03-proof-plan, H5 (a)). The producer's block holds X = v0; the producer
// queues Set X = v1 with the worker held at the gate; another thread Applies the block. Natively that Apply follows the Set
// made before it, so it must wait for the gate and land after v1: the device ends on v0. The copy still holds v1 as known
// (another thread's Apply does not touch it), so the producer's next Get answers v0 only if it settles. THE CONTROL makes the
// same Apply through DXVK's own entry, past the layer: it must return while the worker is held and be overwritten by the
// queued v1, or the case proves nothing.
static long gH5aRan = 0;
static IDirect3DStateBlock9* volatile gH5aBlk = nullptr; static volatile LONG gH5aDone = 0; static volatile bool gH5aHooked = true;
static DWORD WINAPI H5aForeign(void*)
{
    if (gH5aHooked) gH5aBlk->Apply(); else ((DtF0)dtSbApplyO)(gH5aBlk);
    _InterlockedExchange(&gH5aDone, 1);
    return 0;
}
static void H5aForeignApply(bool hooked)
{
    const char* how = hooked ? "H5 (a): another thread's Apply behind a held worker" : "H5 (a), CONTROL: the same Apply past the layer";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = hooked ? 0x5A11u : 0x5A33u, v1 = hooked ? 0x5A22u : 0x5A44u;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    Compare(how);                                                  // the producer's traffic so far; the other thread's lands wherever it lands
    Drain();
    if (!DtNote(kHarnessGate, &X, 4)) { Err("%s: the gate note was refused", how); SbRelease(b); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1);                  // queued BEFORE the other thread's Apply
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("%s: the worker never reached the gate", how); SetEvent(gGateEv); SbRelease(b); return; }
    gH5aBlk = b; gH5aHooked = hooked; _InterlockedExchange(&gH5aDone, 0);
    HANDLE h = CreateThread(nullptr, 0, H5aForeign, nullptr, 0, nullptr);
    Sleep(60);
    const bool early = gH5aDone != 0;                                                  // returned while the ring could not drain
    SetEvent(gGateEv);
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    const bool copyKnew = dtSh.rsOk[X] && dtSh.rs[X] == v1;
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    Drain();
    const DWORD dev = gFd->st.rs[X];
    if (hooked)
    {
        if (early) Err("%s: the Apply returned while the worker was held with an older Set still queued", how);
        if (dev != v0) Err("%s: the device ended on 0x%lX, not the block's 0x%lX: the Apply landed ahead of the queued 0x%lX", how, dev, v0, v1);
        if (!copyKnew) Err("%s: the copy did not hold the producer's 0x%lX as known before the Get, so the Get's answer proves no settle", how, v1);
        if (x != dev) Err("%s: the producer's Get answered 0x%lX; the device holds 0x%lX", how, x, dev);
    }
    else if (!early || dev != v1) Err("%s: past the layer the Apply did not land ahead of the queue (early %d, device 0x%lX, queued 0x%lX), so the case above proves nothing", how, early, dev, v1);
    else if (x != dev) Err("%s: the producer's Get answered 0x%lX; the device holds 0x%lX", how, x, dev);
    SbRelease(b);
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    R.rs[X] = dev;
    if (!dtArmed) Err("%s: the layer was unarmed", how);
    printf("  %s: returned early %d, the device holds 0x%lX, the Get answered 0x%lX (block 0x%lX, queued 0x%lX)\n", how, early, dev, x, v0, v1);
    gH5aRan++;
}

// H5 (d), A FOREIGN CALL IN FLIGHT ACROSS AN APPLY (03-proof-plan, H5 (d); the ForeignGetDuringWait shape). The producer's
// block holds X = v0; the producer queues Set X = v1 with the worker held; another thread's Set of X = vf goes in flight,
// waiting for the worker; the producer Applies the block and asks for X. Natively the other thread's call and the Apply may
// land in either order after v1, so the device ends on v0 or vf, never v1, and the Get must answer what the device ends on.
// Variant 0 holds the other thread's call inside the fake for 80 ms (gFkSlowSetTid), so it is still in flight when the Get is
// asked and lands after the Apply: the Get must wait it out and answer vf. At B (step 4) the Apply's record tells the copy
// X = v0, which that late vf must still overrule. Variant 1 lets the two race. The no-wait check covers the Apply alone:
// waiting out a call in flight is the Get's job at either arm.
static long gH5dRan = 0, gH5dAfter = 0;
static volatile bool gH5dSlow = false;
static DWORD WINAPI H5dForeignSet(void*)
{
    if (gH5dSlow) { gFkSlowSetMs = 80; gFkSlowSetTid = GetCurrentThreadId(); }
    D->SetRenderState(D3DRS_TEXTUREFACTOR, 0x5DFFu);
    gFkSlowSetTid = 0;
    return 0;
}
static void H5dInFlightAcrossApply(int variant)
{
    const char* how = "H5 (d): another thread's Set in flight across the producer's Apply";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed", how); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, v0 = 0x5D11u + (DWORD)variant, v1 = 0x5D22u + (DWORD)variant, vf = 0x5DFFu;
    SbBegin(); D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    Compare(how);
    Drain();
    if (!DtNote(kHarnessGate, &X, 4)) { Err("%s: the gate note was refused", how); SbRelease(b); return; }
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1);
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("%s: the worker never reached the gate", how); SetEvent(gGateEv); SbRelease(b); return; }
    gH5dSlow = variant == 0;
    HANDLE h = CreateThread(nullptr, 0, H5dForeignSet, nullptr, 0, nullptr);
    for (int spin = 0; !dtForeignInFlight && spin < 200000000; spin++) _mm_pause();   // the other thread's call has begun, and waits for the worker
    if (!dtForeignInFlight) Err("%s (variant %d): the other thread's call never went in flight", how, variant);
    HANDLE g = CreateThread(nullptr, 0, OpenGateLater, nullptr, 0, nullptr);
    const NoWaitMark m;
    const HRESULT hr = b->Apply();
    NoWaitOrFallBack("H5 (d): an Apply with another thread's call in flight", m, true, DT_SBF_FOREIGN);
    if (hr != D3D_OK) Err("%s (variant %d): the Apply answered 0x%08lX", how, variant, (unsigned long)hr);
    const bool inFlightAtGet = dtForeignInFlight != 0;
    DWORD x = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); gGets++;
    WaitForSingleObject(h, INFINITE); CloseHandle(h); WaitForSingleObject(g, INFINITE); CloseHandle(g);
    Drain();
    const DWORD dev = gFd->st.rs[X];
    if (dev != v0 && dev != vf) Err("%s (variant %d): the device ended on 0x%lX, neither the block's 0x%lX nor the other thread's 0x%lX (queued 0x%lX)", how, variant, dev, v0, vf, v1);
    if (x != dev) Err("%s (variant %d): the producer's Get answered 0x%lX; the device ended on 0x%lX", how, variant, x, dev);
    if (variant == 0)
    {
        if (!inFlightAtGet) Err("%s (variant 0): the other thread's call had landed before the Get, so the Get waited nothing out", how);
        if (dev != vf) Err("%s (variant 0): the other thread's call, held 80 ms inside the device, did not land last (the device holds 0x%lX)", how, dev);
    }
    if (dev == vf) gH5dAfter++;
    if (dtForeignInFlight) Err("%s (variant %d): a call was left in flight (%ld)", how, variant, (long)dtForeignInFlight);
    SbRelease(b);
    Drain();
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    R.rs[X] = dev;
    if (!dtArmed) Err("%s: the layer was unarmed", how);
    printf("  %s (variant %d): in flight at the Get %d, the device holds 0x%lX, the Get answered 0x%lX (block 0x%lX, other thread 0x%lX)\n",
        how, variant, inFlightAtGet, dev, x, v0, vf);
    gH5dRan++;
}

// H5 (c), ANOTHER THREAD'S SET AND BEGIN DURING THE PRODUCER'S RECORDING (03-proof-plan, H5 (c); C1's P3). DXVK's recorder is
// the device's, not a thread's: a Set from any thread between the producer's Begin and End goes into the producer's block and
// leaves the device as it was, and another thread's Begin is refused (one recording at a time), leaving the recording as it
// was. So a Get of that state during the recording answers the device's value, and the block's Apply gives the other
// thread's value along with the producer's. At B (step 3's code) such a block is TAINTED and never takes the recorded path:
// its Apply must take the drained path at both arms, which this case checks (NC17, no taint, must fail it at B).
static long gH5cRan = 0;
static HRESULT gH5cBeginHr = S_OK;
static void H5cForeignSet() { D->SetRenderState(D3DRS_STENCILMASK, 0x5CFFu); }
static void H5cForeignBegin() { gH5cBeginHr = D->BeginStateBlock(); }
static void H5cForeignDuringRecording()
{
    Drain();
    if (!dtArmed) { Err("H5 (c) needs the layer armed"); return; }
    const DWORD X = D3DRS_TEXTUREFACTOR, Y = D3DRS_STENCILMASK, Z = D3DRS_STENCILREF;
    const DWORD v0 = 0x5C11u, v1 = 0x5C22u, yf = 0x5CFFu, z0 = R.rs[Z] ^ 0x5Cu, y0 = R.rs[Y];
    SbBegin();
    D->SetRenderState((D3DRENDERSTATETYPE)X, v0); E(57, { X, v0 }); RRs(X, v0);
    C1RunForeign("H5 (c): the producer's recording before another thread's Set", H5cForeignSet);
    rRec->rs[Y] = true; rRec->v.rs[Y] = yf;                        // into the producer's block, as DXVK's recorder takes it
    DWORD y = 0; D->GetRenderState((D3DRENDERSTATETYPE)Y, &y); gGets++;
    if (y != y0) Err("H5 (c): during the recording Get Y = 0x%lX; the device still holds 0x%lX (the other thread's 0x%lX went into the block)", y, y0, yf);
    gH5cBeginHr = S_OK;
    C1RunForeign("H5 (c): the producer's recording before another thread's Begin", H5cForeignBegin);
    if (gH5cBeginHr != D3DERR_INVALIDCALL) Err("H5 (c): another thread's Begin during the producer's recording answered 0x%08lX, not DXVK's refusal", (unsigned long)gH5cBeginHr);
    D->SetRenderState((D3DRENDERSTATETYPE)Z, z0); E(57, { Z, z0 }); RRs(Z, z0);   // the recording goes on
    IDirect3DStateBlock9* b = SbEnd(); if (!b) return;
    SbExpectRecord(b, DT_SBT_FOREIGN, "H5 (c): C1's record of a block another thread's Set went into");
    D->SetRenderState((D3DRENDERSTATETYPE)X, v1); E(57, { X, v1 }); RRs(X, v1);
    const LONG sb0 = dtWaitN[DT_R_SBAPPLY];
    SbApply(b);
    DWORD x = 0, z = 0; D->GetRenderState((D3DRENDERSTATETYPE)X, &x); D->GetRenderState((D3DRENDERSTATETYPE)Y, &y); D->GetRenderState((D3DRENDERSTATETYPE)Z, &z); gGets += 3;
    if (dtWaitN[DT_R_SBAPPLY] == sb0) Err("H5 (c): the Apply of a block another thread's Set went into did not take the drained path (a tainted block never takes the recorded one)");
    if (x != v0 || y != yf || z != z0) Err("H5 (c): after the Apply X = 0x%lX, Y = 0x%lX, Z = 0x%lX; the block holds 0x%lX, the other thread's 0x%lX and 0x%lX", x, y, z, v0, yf, z0);
    SbRelease(b);
    Drain();
    gH5cRan++;
}

static void ForeignOrdered()
{
    if (!gGateEv) gGateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    dtNoteObs[kHarnessGate] = ObsGate;
    ForeignOrderedHeld();
    ForeignGetDuringWait();
    ForeignStateBlockHeld(true);
    ForeignStateBlockHeld(false);                              // the negative control
    ForeignBeginHeld(true);                                    // C1 step 3a
    ForeignBeginHeld(false);                                   // its negative control
    dtNoteObs[kHarnessGate] = nullptr;
    if (gCsbHeldRan != 2) { printf("  the state-block-behind-a-held-worker case and its control did not both run\n"); gErr++; }
    if (gFbHeldRan != 2) { printf("  the Begin-behind-a-held-worker case (C1 step 3a) and its control did not both run\n"); gErr++; }
    if (gGetDuringWaitRan != 1) { printf("  the Get-during-the-wait case did not run\n"); gErr++; }
}

// A FAILED REPLAY IS COUNTED AGAINST WHO FAILED (O9, 2026-09-17). The command no longer
// carries its function, so the report names a method from the slot in the header, and
// a Present or a resource method by a code of its own. One recorded draw is made to
// fail on the worker: the game still gets D3D_OK (it was answered before it ran), and
// the count must stand against slot 81 with the fake's own HRESULT. The Present that
// Presents() fails below must stand against DT_WHO_PRESENT, checked at the end.
static bool ReplayErrHas(DWORD who, HRESULT hr)
{
    for (int i = 0; i < 8; i++) if (dtReplayErrCnt[i] && dtReplayErrWho[i] == who && dtReplayErrHr[i] == hr) return true;
    return false;
}
static void FailedDraw()
{
    Drain();
    gFkDrawHr = (LONG)D3DERR_INVALIDCALL;
    const HRESULT a = D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    Drain();
    gExpReplayErr++;
    if (a != D3D_OK) Err("a recorded draw answered 0x%08lX, not D3D_OK", (unsigned long)a);
    if (!ReplayErrHas(81, D3DERR_INVALIDCALL)) Err("a draw that failed on the worker was not counted against slot 81");
    // A CLEAR OF DEPTH WITH NO DEPTH-STENCIL BOUND, DXVK's one refusal (d3d9_device.cpp:1910), and
    // what the owner's intermittent "replayed calls that failed: Clear" are made of. The game is
    // answered D3D_OK and the worker counts the failure against slot 43. (A census counted these
    // from the caller's side on 2026-09-19 and came out unanswered: no launch that day failed.)
    D->SetDepthStencilSurface(nullptr); E(39, { 0u }); R.ds = nullptr;
    const DWORD cf = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
    const HRESULT b = D->Clear(0, nullptr, cf, 0, 1.0f, 0); E(43, { 0u, 0u, cf, 0u, F(1.0f), 0u }, nullptr, 0);
    Drain();
    gExpReplayErr++;
    if (b != D3D_OK) Err("a recorded Clear answered 0x%08lX, not D3D_OK", (unsigned long)b);
    if (!ReplayErrHas(43, D3DERR_INVALIDCALL)) Err("a Clear that failed on the worker was not counted against slot 43");
    // and one WITH a depth-stencil bound succeeds and is not counted
    D->SetDepthStencilSurface(gDs[0]); E(39, { P(gDs[0]) }); R.ds = gDs[0];
    D->Clear(0, nullptr, cf, 0, 1.0f, 0); E(43, { 0u, 0u, cf, 0u, F(1.0f), 0u }, nullptr, 0);
    Drain();
    if ((long)dtReplayErrN != gExpReplayErr) Err("a depth Clear WITH a depth-stencil bound was counted as a failed replay");
}

// A PRESENT THE WORKER SAW FAIL IS HANDED BACK EXACTLY ONCE: by that Present, if it
// has run by the time it returns, or else by the next. A DONOTWAIT Present is recorded
// like any other since stage 1.5 (DXVK 3.1 never reads the flag), so with no failure
// pending it is answered D3D_OK and the fake receives it from the worker.
static void Presents()
{
    Drain();
    gFkPresentHr = (LONG)D3DERR_DEVICELOST;
    const HRESULT a = SC->Present(nullptr, nullptr, nullptr, nullptr, 0); E(1803, { 0u, 0u });
    Drain();
    const HRESULT b = SC->Present(nullptr, nullptr, nullptr, nullptr, 0); E(1803, { 0u, 0u });
    const HRESULT c = SC->Present(nullptr, nullptr, nullptr, nullptr, 0); E(1803, { 0u, 0u });
    gPresents += 3; gExpReplayErr++;
    const int lost = (a == D3DERR_DEVICELOST) + (b == D3DERR_DEVICELOST);
    if (lost != 1 || (a != D3D_OK && a != D3DERR_DEVICELOST) || (b != D3D_OK && b != D3DERR_DEVICELOST) || c != D3D_OK)
        Err("a failed Present came back as 0x%08lX, 0x%08lX, 0x%08lX: once, as D3DERR_DEVICELOST, is right",
            (unsigned long)a, (unsigned long)b, (unsigned long)c);
    const HRESULT d = SC->Present(nullptr, nullptr, nullptr, nullptr, D3DPRESENT_DONOTWAIT);
    E(1803, { 0u, (DWORD)D3DPRESENT_DONOTWAIT }); gPresents++;
    if (d != D3D_OK) Err("a DONOTWAIT Present returned 0x%08lX", (unsigned long)d);
}

// ANOTHER DEVICE'S OBJECT IS NOT RECORDED: its last Release runs at once, right here.
static void OtherDevice()
{
    Drain();
    FakeTex* t = new FakeTex(16, 16); t->fkDev = gFd2;
    const ULONG r = ((IDirect3DTexture9*)t)->Release();
    if (r != 0 || !IsDead(t)) Err("another device's texture was not released at once (%lu, %s)", r, IsDead(t) ? "dead" : "alive");
    // AND ANOTHER DEVICE'S STATE BLOCK, APPLIED ON THE RENDER THREAD (2026-09-19). It changes
    // that device, not ours: our ring is not drained for it and our copy forgets nothing.
    // dtSbApply was the one resource hook that did not ask, so this failed before it did.
    FakeSB* b = new FakeSB; b->fkDev = gFd2;               // an empty capture set: its Apply changes nothing, on either device
    const LONG dirtied = dtShDirtied, waits = dtWaitN[DT_R_SBAPPLY];
    ((IDirect3DStateBlock9*)b)->Apply(); E(1905, { P(b) });
    if (dtShDirtied != dirtied) Err("another device's Apply marked OUR copy of the state unknown");
    if (dtWaitN[DT_R_SBAPPLY] != waits) Err("another device's Apply drained OUR ring");
    ((IDirect3DStateBlock9*)b)->Release();
}

// A RESET FROM ANOTHER THREAD (field report R1, 2026-09-14; the barrier redesigned after
// the review of fd5192b). One texture is released by the producer (recorded), one by
// the thread that then Resets (handed over). DXVK refuses a Reset while a default-pool
// object lives, so both must be dead by the time the fake device's Reset runs,
// whichever thread asked; and the layer is unarmed after. Three ways in:
//   idle, published:   the producer made a Present and then waits (the game's shape);
//                      the barrier finds an idle ring and disarms from the Reset's thread.
//   idle, unpublished: the producer recorded past its last Present and waits; the
//                      barrier must publish that itself before it can disarm.
//   recording:         the producer is in a loop of Sets the whole time; it must
//                      answer the ask from inside a wrapper entry and record nothing
//                      past it.
static IDirect3DTexture9* gResetT[2] = {};
static DWORD WINAPI ForeignResetRelease(void*)
{ const ULONG r = gResetT[1]->Release(); if (r != 0) Err("a last Release from the resetting thread returned %lu", r); return 0; }
static void AtForeignReset()
{
    if (!IsDead(gResetT[0])) Err("a Reset from another thread ran before the producer's recorded Release");
    if (!IsDead(gResetT[1])) Err("a Reset from another thread ran before its own handed-over Release");
    if (dtArmed) Err("the fake device's Reset ran with the layer still armed");
    if (dtW != (DWORD)dtPub || (DWORD)dtDone != (DWORD)dtPub) Err("the fake device's Reset ran with the ring not empty");
}
static DWORD WINAPI ForeignReset(void*) { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); return 0; }
static void ResetTexturesReleased()
{
    gResetT[0] = NewTex(); gResetT[1] = NewTex();
    D->SetTexture(0, gResetT[0]); E(65, { 0u, P(gResetT[0]) }); R.tex[0] = gResetT[0];
    D->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1); E(81, { (DWORD)D3DPT_TRIANGLELIST, 0u, 1u });
    D->SetTexture(0, nullptr); E(65, { 0u, 0u }); R.tex[0] = nullptr;
    gResetT[0]->Release();                    // the producer's: recorded behind the draw
    RunThread(ForeignResetRelease);           // another thread's: handed over
}
static void ResetRearm(const char* how)
{
    if (dtArmed) Err("a Reset from another thread (%s) left the layer armed", how);
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) Err("the layer did not arm again after a Reset from another thread (%s)", how);
    if (dtDisarmAsk) Err("the ask was still up after the layer armed again (%s)", how);
    for (int i = 0; i < 5000; i++) OneOp(true);
}
// A RESET FROM ANOTHER THREAD THAT LANDS INSIDE THE POST OF A DRIVING NOTE (final audit 2026-09-19, F1.1). The post fetches
// what the body will touch and hands each pointer back through Release; a Release on the producer answers a disarm ask.
// The fake's GetTexture starts the other thread's Reset and returns only once its ask is up, so the very next Release
// disarms the layer from inside DtNote. WITH the re-test the note must be REFUSED: nothing in the ring, the body count at
// zero, the observer never run. THE CONTROL switches the re-test off and must find what it prevents: a note answered
// true into a ring nobody publishes, which the re-arm then replays.
static volatile LONG gNadObsRan = 0, gNadHookFired = 0;
static HANDLE gNadThread = nullptr;
static void ObsNad(DWORD, const DWORD*, DWORD) { _InterlockedIncrement(&gNadObsRan); }
static DWORD WINAPI NadForeignReset(void*) { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); return 0; }
static void NadOnGetTexture()
{
    if (gNadThread) return;                                   // the post's FIRST texture fetch only
    _InterlockedIncrement(&gNadHookFired);
    gNadThread = CreateThread(nullptr, 0, NadForeignReset, nullptr, 0, nullptr);
    const ULONGLONG t0 = GetTickCount64();
    while (!dtDisarmAsk && dtArmed && GetTickCount64() - t0 < 3000) Sleep(0);
}
static void NoteAcrossDisarm(bool recheck)
{
    const char* how = recheck ? "a note across a disarm" : "a note across a disarm, CONTROL";
    Drain();
    if (!dtArmed) { Err("%s: the layer was not armed going in", how); return; }
    IDirect3DBaseTexture9* t = gTex[0];
    D->SetTexture(0, t); E(65, { 0u, P(t) }); R.tex[0] = t;
    IDirect3DStateBlock9* sb = SbCreate(D3DSBT_ALL);
    SbCapture(sb);
    SbApply(sb);                                               // every texture entry is unknown now, so the post fetches slot 0
    SbRelease(sb);
    Drain();
    dtNoteObs[DT_NOTE_INJECT] = ObsNad; dtNoteDrives[DT_NOTE_INJECT] = true;
    dtBodySampFree = 0x1FFFFFu & ~1u;
    const LONG ran0 = gNadObsRan, fired0 = gNadHookFired;
    gNoteRecheck = recheck; gNadThread = nullptr; gFkOnGetTexture = NadOnGetTexture;
    const DWORD pay[4] = { 1, 2, 3, 4 };
    const bool posted = DtNote(DT_NOTE_INJECT, pay, sizeof(pay));
    gFkOnGetTexture = nullptr; gNoteRecheck = true;
    if (gNadThread) { WaitForSingleObject(gNadThread, INFINITE); CloseHandle(gNadThread); gNadThread = nullptr; }
    const bool orphan = dtW != (DWORD)dtPub || (DWORD)dtDone != (DWORD)dtPub;
    if (gNadHookFired != fired0 + 1) Err("%s: the post never fetched a texture, so the ask was never raised inside it", how);
    if (dtArmed) Err("%s: the layer was still armed after the other thread's Reset", how);
    if (recheck)
    {
        if (posted) Err("%s: DtNote answered TRUE for a note posted into a layer its own fetch had disarmed", how);
        if (orphan) Err("%s: a note was left in a ring nobody publishes", how);
        if (dtShBodies) Err("%s: the body count was left at %ld", how, (long)dtShBodies);
    }
    else if (!posted || !orphan) Err("%s: with the re-test OFF the note was not orphaned (posted %d, orphan %d), so the case above proves nothing", how, posted, orphan);
    // The two threads interleave in no fixed order: both logs and the reference start again, as after ResetWhileRecording.
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    R.Reset(&gFd->sc->bb);
    ResetRearm(how);
    Drain();
    // What the re-arm did with it: nothing to replay with the re-test, ONE stale replay without (the hazard itself).
    if (gNadObsRan - ran0 != (recheck ? 0 : 1)) Err("%s: the note's observer ran %ld time(s) after the re-arm", how, (long)(gNadObsRan - ran0));
    if (dtShBodies) Err("%s: the body count is %ld after the re-arm", how, (long)dtShBodies);
    dtNoteObs[DT_NOTE_INJECT] = nullptr; dtNoteDrives[DT_NOTE_INJECT] = false; dtBodySampFree = 0;
    printf("  %s: posted %d, orphan %d, replayed %ld\n", how, posted, orphan, (long)(gNadObsRan - ran0));
}
// THE LATE PUBLISH AFTER AN IDLE BARRIER (2026-09-29; DtRecAsked's unarmed branch, dt_install.h). Another thread's Reset
// barrier disarms by its idle path once nothing has been recorded for its settle window. A producer preempted that long
// inside DtCmd and DtCommit then publishes its last commands itself, before its next wrapper entry; that entry answers the
// ask unarmed and goes direct. UNTIL 2026-09-29 IT DID NOT WAIT FOR THE WORKER in that case (its log line said the worker
// "has already run them"), so its direct call could reach the device BEFORE the published commands it follows. Emulated:
// after the idle disarm, a note that holds the worker and a Set are written and published as the preempted producer would
// have (DtCmdDev and DtCommit do not look at the arm; the note is DtNote's own layout), then the producer's next Set of the
// same state while the worker is held. The device must end on the producer's LAST Set. NC35 is the old branch.
static long gLateRan = 0;
static void LatePublishAfterIdleBarrier()
{
    Drain();
    if (!dtArmed) { Err("the late-publish case needs the layer armed"); return; }
    ResetTexturesReleased();
    gFkOnReset = AtForeignReset;
    RunThread(ForeignReset); E(16, {}); R.Reset(&gFd->sc->bb);
    gFkOnReset = nullptr;
    if (dtArmed || !dtDisarmAsk) { Err("the idle barrier did not leave the layer unarmed with the ask up (armed %d, ask %ld)", (int)dtArmed, (long)dtDisarmAsk); return; }
    dtNoteObs[kHarnessGate] = ObsGate;
    const DWORD X = D3DRS_STENCILREF, late = 0x5Au, after = 0xA5u;
    { const DWORD n = 3; DWORD* p = DtReserve(n); p[0] = DT_OP_NOTE | DT_SLOT(kHarnessGate) | (n << 16); p[1] = 4; p[2] = X; DtCommit(n); }
    { DWORD* p = DtCmdDev(DT_OP_CALL0 + 2 | DT_SLOT(57), 3); p[1] = X; p[2] = late; DtCommit(3); }
    E(57, { X, late });
    DtPublishFenced();
    for (int spin = 0; !gGateIn && spin < 200000000; spin++) _mm_pause();
    if (!gGateIn) { Err("the late-publish case: the worker never reached the gate"); SetEvent(gGateEv); dtNoteObs[kHarnessGate] = nullptr; return; }
    HANDLE h = CreateThread(nullptr, 0, OpenGateLater, nullptr, 0, nullptr);   // 100 ms
    D->SetRenderState((D3DRENDERSTATETYPE)X, after); E(57, { X, after }); R.rs[X] = after;
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    for (int i = 0; i < 5000 && (DWORD)dtDone != dtW; i++) Sleep(1);
    dtNoteObs[kHarnessGate] = nullptr;
    if (gFd->st.rs[X] != after)
    {
        Err("after an idle barrier and a late publish, the device ended on 0x%lX, not on the producer's last Set 0x%lX: its direct call landed before the published commands it follows", gFd->st.rs[X], after);
        R.rs[X] = gFd->st.rs[X];                                // one error, not one per later compare
    }
    if (dtDisarmAsk) Err("the late-publish case: the ask was left up after the producer answered it");
    gLateRan++;
    ResetRearm("late publish");
}
static void ResetFromAnotherThread(bool presentFirst)
{
    const char* how = presentFirst ? "idle, published" : "idle, unpublished";
    Drain();
    ResetTexturesReleased();
    if (presentFirst) Present();              // published: the worker may still be running it
    const LONG disarmsBefore = dtDisarms;
    gFkOnReset = AtForeignReset;
    RunThread(ForeignReset); E(16, {}); R.Reset(&gFd->sc->bb);
    gFkOnReset = nullptr;
    if (dtDisarms != disarmsBefore + 1) Err("a Reset from another thread (%s) disarmed %ld times, not once", how, dtDisarms - disarmsBefore);
    ResetRearm(how);
}
// The recording producer: the main thread loops Sets until the layer is unarmed under
// it, while this thread Resets (the handed-over Release is already on the list, from
// ResetTexturesReleased). Nothing here is compared, the two interleave in no fixed
// order; the reference and both logs are reset after.
static volatile LONG gBusyGo = 0;
static DWORD WINAPI ForeignResetBusy(void*)
{
    while (!gBusyGo) Sleep(0);
    Sleep(5);                                                  // the producer is well into its loop
    D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp);
    return 0;
}
static void ResetWhileRecording()
{
    Drain();
    ResetTexturesReleased();
    gFkOnReset = AtForeignReset;
    gBusyGo = 0;
    HANDLE h = CreateThread(nullptr, 0, ForeignResetBusy, nullptr, 0, nullptr);
    gBusyGo = 1;
    const ULONGLONG t0 = GetTickCount64();
    unsigned i = 0;
    while (dtArmed && GetTickCount64() - t0 < 3000) { D->SetRenderState((D3DRENDERSTATETYPE)(7 + i % 200), i); i++; }
    if (dtArmed) Err("a Reset from another thread did not unarm a recording producer within 3 s");
    WaitForSingleObject(h, INFINITE); CloseHandle(h);
    gFkOnReset = nullptr;
    if (dtDisarmAsk) Err("the ask was left up after the producer answered it");
    if (dtW != (DWORD)dtPub || (DWORD)dtDone != (DWORD)dtPub) Err("the ring was not empty after the producer answered a Reset's ask");
    printf("  recording producer: %u Sets before the layer was unarmed under it\n", i);
    EnterCriticalSection(&gFkLock); gFk.clear(); gExp.clear(); gObsRs = gObsDip = 0; LeaveCriticalSection(&gFkLock);
    // THE SET THAT ANSWERED THE ASK goes to the device directly and may land on either side of the Reset (2026-09-28, when
    // the reference began to be held to the device): everything must be the Reset's but at most that one render state,
    // and the reference restarts from the device.
    {
        FkState z; z.Reset(&gFd->sc->bb);
        FkState d = gFd->st; int n = 0;
        // The one allowed is the producer's last Set: render state 7 + (i - 1) % 200 with the value i - 1.
        const DWORD lastRs = 7 + (i - 1) % 200, lastV = i - 1;
        for (int k = 7; k < 207; k++) if (d.rs[k] != z.rs[k]) { if ((DWORD)k != lastRs || d.rs[k] != lastV) n += 100; n++; d.rs[k] = z.rs[k]; }
        if (n > 1 || memcmp(&d, &z, sizeof(d))) Err("after a Reset under a recording producer the device holds state that is neither the Reset's nor the producer's last Set (%d)", n);
        R = gFd->st;
    }
    ResetRearm("recording");
}

// ---- CONFORMANCE: THE FAKE'S STATE BLOCKS AGAINST DXVK 3.1 (2026-09-28, C1 step 1) ------------------------------------
// One case a rule, each expectation worked out by hand from DXVK's source (docs/reports/c1-design-2026-09-28/
// 03-proof-plan.txt, section 1; d3d9_stateblock.cpp, d3d9_device.cpp). Each case runs on a device of its own, before the
// layer is installed. THE CONTROL runs the same cases on the fake as it was until today (gFkSnapshot: a block is a whole-
// device snapshot, a recording records nothing, nothing is refused): every case must FAIL there, by name. A case that
// passes on the snapshot fake cannot see the rule it is named for.
static const D3DVERTEXELEMENT9 kCfEl[] = { { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
static IDirect3DVertexBuffer9* CfVb(FakeDev* d) { IDirect3DVertexBuffer9* v = nullptr; d->CreateVertexBuffer(64, 0, 0, D3DPOOL_MANAGED, &v, nullptr); return v; }
static IDirect3DTexture9* CfTex(FakeDev* d) { IDirect3DTexture9* t = nullptr; d->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr); return t; }
static IDirect3DVertexDeclaration9* CfDecl(FakeDev* d) { IDirect3DVertexDeclaration9* x = nullptr; gFkNextDeclFvf = 0; d->CreateVertexDeclaration(kCfEl, &x); return x; }
static IDirect3DStateBlock9* CfMake(FakeDev* d, D3DSTATEBLOCKTYPE t) { IDirect3DStateBlock9* b = nullptr; d->CreateStateBlock(t, &b); return b; }
template <class F> static IDirect3DStateBlock9* CfRecord(FakeDev* d, F body)
{ d->BeginStateBlock(); body(); IDirect3DStateBlock9* b = nullptr; d->EndStateBlock(&b); return b; }
static void CfRel(IDirect3DStateBlock9* b) { if (b) b->Release(); }
static char gCfWhy[16][240]; static int gCfWhyN = 0;
static const char* Why(const char* fmt, ...)
{ char* b = gCfWhy[gCfWhyN++ & 15]; va_list a; va_start(a, fmt); vsnprintf(b, 240, fmt, a); va_end(a); return b; }
static const char* CfWhich(const void* p, const void* a, const void* b, const void* c)
{ return !p ? "null" : p == a ? "the first buffer" : p == b ? "the second" : p == c ? "the third" : "another"; }
#define CF_MADE(b) do { if (!(b)) return "a block was not made"; } while (0)

static const char* CfRecordedOffset(FakeDev* d)        // stateblock.h:224-230: Capture takes a stream's buffer and stride only
{
    IDirect3DVertexBuffer9* const a = CfVb(d), * const b = CfVb(d), * const c = CfVb(d);
    IDirect3DStateBlock9* sb = CfRecord(d, [&] { d->SetStreamSource(0, a, 64, 16); }); CF_MADE(sb);
    d->SetStreamSource(0, b, 128, 32); sb->Capture();
    d->SetStreamSource(0, c, 0, 16); sb->Apply();
    const FkState& s = d->st;
    const char* why = (s.vb[0] == b && s.off[0] == 64 && s.str[0] == 32) ? nullptr
        : Why("stream 0 after the Apply is (%s, %u, %u); DXVK's is (the second, 64 as recorded, 32 as captured)", CfWhich(s.vb[0], a, b, c), s.off[0], s.str[0]);
    CfRel(sb); return why;
}
static const char* CfTypedOffset(FakeDev* d)           // stateblock.cpp:580-585: a typed block takes the offset at creation
{
    IDirect3DVertexBuffer9* const a = CfVb(d), * const b = CfVb(d), * const c = CfVb(d);
    d->SetStreamSource(0, a, 64, 16);
    IDirect3DStateBlock9* sb = CfMake(d, D3DSBT_ALL); CF_MADE(sb);
    d->SetStreamSource(0, b, 128, 32); sb->Capture();
    d->SetStreamSource(0, c, 0, 16); sb->Apply();
    const FkState& s = d->st;
    const char* why = (s.vb[0] == b && s.off[0] == 64 && s.str[0] == 32) ? nullptr
        : Why("stream 0 after the Apply is (%s, %u, %u); DXVK's is (the second, 64 from the creation, 32 from the Capture)", CfWhich(s.vb[0], a, b, c), s.off[0], s.str[0]);
    CfRel(sb); return why;
}
static const char* CfNullDecl(FakeDev* d)              // stateblock.cpp:71-72: a declaration is applied only when not null
{
    IDirect3DVertexDeclaration9* const x = CfDecl(d);   // the fresh device has none bound
    IDirect3DStateBlock9* const bl[3] = { CfMake(d, D3DSBT_ALL), CfMake(d, D3DSBT_VERTEXSTATE), CfRecord(d, [&] { d->SetVertexDeclaration(nullptr); }) };
    static const char* const nm[3] = { "an ALL block", "a VERTEX block", "a recorded block" };
    const char* why = nullptr;
    for (int k = 0; k < 3 && !why; k++)
    {
        if (!bl[k]) { why = "a block was not made"; break; }
        d->SetVertexDeclaration(x); bl[k]->Apply();
        if (d->st.decl != x) why = Why("%s that holds a null declaration unbound the bound one", nm[k]);
    }
    for (IDirect3DStateBlock9* b : bl) CfRel(b);
    return why;
}
static const char* CfNullStream(FakeDev* d)            // device.cpp:3736-3744: a null stream is offset 0, stride 0
{
    IDirect3DVertexBuffer9* const a = CfVb(d);
    IDirect3DStateBlock9* sb = CfRecord(d, [&] { d->SetStreamSource(1, nullptr, 48, 24); }); CF_MADE(sb);
    d->SetStreamSource(1, a, 16, 16); sb->Apply();
    const FkState& s = d->st;
    const char* why = (!s.vb[1] && !s.off[1] && !s.str[1]) ? nullptr
        : Why("stream 1 after applying a recorded null stream is (%s, %u, %u); DXVK's is (null, 0, 0)", s.vb[1] ? "a buffer" : "null", s.off[1], s.str[1]);
    CfRel(sb); return why;
}
static const char* CfMaxZ(FakeDev* d)                  // device.cpp:2131: MaxZ is lifted to MinZ + 0.001 when not above it
{
    const D3DVIEWPORT9 low = { 8, 8, 640, 480, 0.5f, 0.25f }, other = { 0, 0, 800, 600, 0.0f, 1.0f };
    IDirect3DStateBlock9* sb = CfRecord(d, [&] { d->SetViewport(&low); }); CF_MADE(sb);
    d->SetViewport(&other); sb->Apply();
    const D3DVIEWPORT9& v = d->st.vp; const float lifted = 0.5f + 0.001f;
    const char* why = (v.Width == 640 && v.MinZ == 0.5f && v.MaxZ == lifted) ? nullptr
        : Why("the viewport after the Apply is %lux%lu, Z %.4f to %.4f; DXVK's is 640x480, Z 0.5 to 0.501", v.Width, v.Height, v.MinZ, v.MaxZ);
    CfRel(sb); return why;
}
static const char* CfRefusedWhileRecording(FakeDev* d) // stateblock.cpp:51-53, 67-69
{
    d->SetRenderState(D3DRS_ZENABLE, 1);
    IDirect3DStateBlock9* sb = CfMake(d, D3DSBT_PIXELSTATE); CF_MADE(sb);
    d->SetRenderState(D3DRS_ZENABLE, 2);
    HRESULT hc = S_OK, ha = S_OK;
    IDirect3DStateBlock9* r = CfRecord(d, [&] { hc = sb->Capture(); ha = sb->Apply(); });
    const DWORD during = d->st.rs[D3DRS_ZENABLE];
    sb->Apply();
    const DWORD after = d->st.rs[D3DRS_ZENABLE];
    const char* why = hc != D3DERR_INVALIDCALL ? Why("a Capture while a block records answered 0x%08lX", (unsigned long)hc)
                    : ha != D3DERR_INVALIDCALL ? Why("an Apply while a block records answered 0x%08lX", (unsigned long)ha)
                    : during != 2 ? "an Apply while a block records changed the device"
                    : after != 1 ? Why("the block put back ZENABLE %lu, not the 1 it was made with: the refused Capture took a value", after) : nullptr;
    CfRel(r); CfRel(sb); return why;
}
static const char* CfFvf(FakeDev* d)                   // device.cpp:3488-3504: SetFVF(0) returns first; any other binds the table's
{
    IDirect3DVertexDeclaration9* const x = CfDecl(d);
    const DWORD f = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    IDirect3DStateBlock9* a = CfRecord(d, [&] { d->SetFVF(f); }), * z = CfRecord(d, [&] { d->SetFVF(0); });
    if (!a || !z) { CfRel(a); CfRel(z); return "a block was not made"; }
    d->SetVertexDeclaration(x); a->Apply();
    const bool table = d->st.decl == FkFvfDecl(f);
    d->SetVertexDeclaration(x); z->Apply();
    const bool kept = d->st.decl == x;
    CfRel(a); CfRel(z);
    return !table ? "a recorded SetFVF did not put back the device's table declaration for its FVF"
         : !kept ? "a recorded SetFVF(0) changed the declaration" : nullptr;
}
static const char* CfPrivate(FakeDev* d)               // d3d9_state.h, Com<T, false>; stateblock.cpp:157-163, TextureChangePrivate
{
    IDirect3DTexture9* const T = CfTex(d);
    d->SetTexture(0, T);
    IDirect3DStateBlock9* sb = CfMake(d, D3DSBT_ALL); CF_MADE(sb);
    d->SetTexture(0, nullptr);
    const ULONG left = T->Release();
    const bool alive = !IsDead(T);
    bool bound = false;
    if (alive) { sb->Apply(); bound = d->st.tex[0] == T; d->SetTexture(0, nullptr); }   // never a call naming a dead one
    CfRel(sb);
    const bool died = IsDead(T);
    return left != 0 ? Why("the texture's last public Release returned %lu", left)
         : !alive ? "the texture died at its last public Release while a block still held it"
         : !bound ? "the Apply did not bind the texture the block held"
         : !died ? "the texture outlived the block, its last holder" : nullptr;
}
static const char* CfMasks(FakeDev* d)                 // stateblock.cpp:364-586, CaptureType
{
    IDirect3DTexture9* const T[2] = { CfTex(d), CfTex(d) };
    IDirect3DVertexShader9* V[2]; IDirect3DPixelShader9* Q[2]; IDirect3DVertexDeclaration9* X[2];
    for (int k = 0; k < 2; k++) { d->CreateVertexShader(nullptr, &V[k]); d->CreatePixelShader(nullptr, &Q[k]); X[k] = CfDecl(d); }
    static const float C[2][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 } };
    D3DMATRIX W[2]; for (int k = 0; k < 2; k++) for (int e = 0; e < 16; e++) (&W[k]._11)[e] = (float)(k * 16 + e);
    const DWORD NONE = 10;                               // a render state in neither list: no typed block holds it
    auto set = [&](int k)                                // k = 0 or 1: every entry to its k-th value
    {
        const DWORD x = 1 + k;
        d->SetRenderState(D3DRS_ZENABLE, x); d->SetRenderState(D3DRS_CULLMODE, x); d->SetRenderState(D3DRS_SHADEMODE, x);
        d->SetRenderState(D3DRS_LIGHTING, x); d->SetRenderState((D3DRENDERSTATETYPE)NONE, x);
        d->SetSamplerState(0, D3DSAMP_MAGFILTER, x); d->SetSamplerState(0, D3DSAMP_DMAPOFFSET, x);
        d->SetSamplerState(D3DDMAPSAMPLER, D3DSAMP_MAGFILTER, x);
        d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, x); d->SetSamplerState(D3DVERTEXTEXTURESAMPLER0, D3DSAMP_DMAPOFFSET, x);
        d->SetTexture(0, T[k]); d->SetVertexShader(V[k]); d->SetPixelShader(Q[k]); d->SetVertexDeclaration(X[k]);
        d->SetPixelShaderConstantF(5, C[k], 1);
        d->SetVertexShaderConstantF(3, C[k], 1); d->SetTextureStageState(2, D3DTSS_COLOROP, x); d->SetStreamSourceFreq(1, x);
        d->SetTransform(D3DTS_WORLD, &W[k]);
    };
    set(0);
    IDirect3DStateBlock9* const bl[3] = { CfMake(d, D3DSBT_PIXELSTATE), CfMake(d, D3DSBT_VERTEXSTATE), CfMake(d, D3DSBT_ALL) };
    // What each type puts back (1) or leaves at the later value (0), in the order of `got` below.
    static const char* const nm[3] = { "PIXEL", "VERTEX", "ALL" };
    static const char* const en[19] = { "ZENABLE", "CULLMODE", "SHADEMODE", "LIGHTING", "render state 10", "sampler 0 MAGFILTER",
        "sampler 0 DMAPOFFSET", "the DMAP sampler's MAGFILTER", "vertex sampler 0 MAGFILTER", "vertex sampler 0 DMAPOFFSET",
        "texture 0", "the vertex shader", "the pixel shader", "the declaration", "pixel constant 5",
        "vertex constant 3", "stage 2's COLOROP", "stream 1's frequency", "the WORLD transform" };
    static const bool want[3][19] = {
        { 1, 0, 1, 0, 0,  1, 0, 1, 0, 0,  0, 0, 1, 0, 1,  0, 1, 0, 0 },
        { 0, 1, 1, 1, 0,  0, 0, 0, 0, 1,  0, 1, 0, 1, 0,  1, 0, 1, 0 },
        { 1, 1, 1, 1, 0,  1, 0, 1, 0, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1 } };
    const char* why = nullptr;
    for (int k = 0; k < 3 && !why; k++)
    {
        if (!bl[k]) { why = "a block was not made"; break; }
        set(1); bl[k]->Apply();
        const FkState& s = d->st;
        const bool got[19] = { s.rs[D3DRS_ZENABLE] == 1, s.rs[D3DRS_CULLMODE] == 1, s.rs[D3DRS_SHADEMODE] == 1, s.rs[D3DRS_LIGHTING] == 1,
            s.rs[NONE] == 1, s.samp[0][D3DSAMP_MAGFILTER] == 1, s.samp[0][D3DSAMP_DMAPOFFSET] == 1, s.samp[16][D3DSAMP_MAGFILTER] == 1,
            s.samp[17][D3DSAMP_MAGFILTER] == 1, s.samp[17][D3DSAMP_DMAPOFFSET] == 1, s.tex[0] == T[0], s.vs == V[0], s.ps == Q[0],
            s.decl == X[0], memcmp(s.psc[5], C[0], 16) == 0,
            memcmp(s.vsc[3], C[0], 16) == 0, s.tss[2][FkTssIdx(D3DTSS_COLOROP)] == 1, s.freq[1] == 1, memcmp(&s.xf[FkXf(D3DTS_WORLD)], &W[0], 64) == 0 };
        for (int e = 0; e < 19 && !why; e++)
            if (got[e] != want[k][e]) why = Why("a %s block %s %s", nm[k], want[k][e] ? "did not put back" : "put back", en[e]);
    }
    for (IDirect3DStateBlock9* b : bl) CfRel(b);
    return why;
}
static const char* CfResetRefused(FakeDev* d)          // device.cpp:465-517: the state is cleared, then refused while a block lives
{
    D3DPRESENT_PARAMETERS pp = {};
    IDirect3DStateBlock9* sb = CfMake(d, D3DSBT_PIXELSTATE); CF_MADE(sb);
    IDirect3DStateBlock9* r = CfRecord(d, [&] { d->SetRenderState(D3DRS_ZENABLE, 3); });
    d->SetRenderState(D3DRS_CULLMODE, 7);
    const HRESULT h1 = d->Reset(&pp);
    const bool cleared = d->st.rs[D3DRS_CULLMODE] == 0;
    CfRel(sb);
    const HRESULT h2 = d->Reset(&pp);                     // the recorded block still lives
    CfRel(r);
    const HRESULT h3 = d->Reset(&pp);
    return h1 != D3DERR_INVALIDCALL ? Why("a Reset with a made block alive answered 0x%08lX", (unsigned long)h1)
         : !cleared ? "the refused Reset left the state as it was"
         : h2 != D3DERR_INVALIDCALL ? Why("a Reset with a recorded block alive answered 0x%08lX", (unsigned long)h2)
         : h3 != D3D_OK ? Why("a Reset with no block alive answered 0x%08lX", (unsigned long)h3) : nullptr;
}
static const char* CfNoWiden(FakeDev* d)               // stateblock.cpp:48-61: a Capture takes only what the block holds
{
    IDirect3DStateBlock9* sb = CfRecord(d, [&] { d->SetRenderState(D3DRS_ZENABLE, 1); }); CF_MADE(sb);
    d->SetRenderState(D3DRS_ZENABLE, 2); d->SetRenderState(D3DRS_CULLMODE, 5); sb->Capture();
    d->SetRenderState(D3DRS_ZENABLE, 3); d->SetRenderState(D3DRS_CULLMODE, 6); sb->Apply();
    const DWORD z = d->st.rs[D3DRS_ZENABLE], c = d->st.rs[D3DRS_CULLMODE];
    CfRel(sb);
    return z != 2 ? Why("the Capture did not take ZENABLE: the Apply put back %lu, not 2", z)
         : c != 6 ? Why("the Capture widened the block: CULLMODE went back to %lu, not the 6 it was left at", c) : nullptr;
}
static const char* CfCreateWhileRecording(FakeDev* d)  // device.cpp:2690-2732
{
    HRESULT hc = S_OK, hb = S_OK; IDirect3DStateBlock9* x = nullptr;
    IDirect3DStateBlock9* r = CfRecord(d, [&] { hc = d->CreateStateBlock(D3DSBT_ALL, &x); hb = d->BeginStateBlock(); });
    if (SUCCEEDED(hc) && x) x->Release();
    IDirect3DStateBlock9* y = nullptr; const HRESULT hi = d->CreateStateBlock((D3DSTATEBLOCKTYPE)0, &y);
    if (SUCCEEDED(hi) && y) y->Release();
    CfRel(r);
    return hc != D3DERR_INVALIDCALL ? Why("a CreateStateBlock while a block records answered 0x%08lX", (unsigned long)hc)
         : hb != D3DERR_INVALIDCALL ? Why("a second BeginStateBlock answered 0x%08lX", (unsigned long)hb)
         : hi != D3DERR_INVALIDCALL ? Why("a CreateStateBlock of type 0 answered 0x%08lX", (unsigned long)hi) : nullptr;
}
static const char* CfRefusedNotRecorded(FakeDev* d)    // device.cpp:2318, 2803, 2864, 3702: validated before the recorder
{
    IDirect3DTexture9* const T = CfTex(d); IDirect3DVertexBuffer9* const v = CfVb(d);
    static const float c[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    d->SetRenderState(D3DRS_ZENABLE, 1);
    IDirect3DStateBlock9* sb = CfRecord(d, [&]
    {
        d->SetRenderState((D3DRENDERSTATETYPE)3, 9); d->SetRenderState((D3DRENDERSTATETYPE)300, 9);
        d->SetTexture(40, T); d->SetSamplerState(40, D3DSAMP_MAGFILTER, 9); d->SetStreamSource(16, v, 0, 16);
        d->SetPixelShaderConstantF(223, c, 2); d->SetViewport(nullptr); d->SetScissorRect(nullptr);
        d->SetRenderState(D3DRS_ZENABLE, 5);             // the one DXVK accepts
    });
    CF_MADE(sb);
    T->Release();
    const bool held = !IsDead(T);                         // a recorded SetTexture would hold it privately
    FkState want = d->st; want.rs[D3DRS_ZENABLE] = 5;
    sb->Apply();
    const bool same = memcmp(&d->st, &want, sizeof(want)) == 0;
    CfRel(sb);
    return held ? "a SetTexture of an invalid sampler was recorded: the block holds the texture"
         : d->st.rs[D3DRS_ZENABLE] != 5 ? "the accepted SetRenderState was not recorded"
         : !same ? "the Apply changed something the recording refused" : nullptr;
}
struct CfCase { const char* name; const char* (*fn)(FakeDev*); };
static const CfCase kCf[] = {
    { "SB1 a recorded stream offset survives a Capture", CfRecordedOffset },
    { "SB2 a typed block takes offsets at creation, a Capture does not", CfTypedOffset },
    { "SB3 a captured null declaration is never applied", CfNullDecl },
    { "SB4 a recorded null stream applies as offset 0, stride 0", CfNullStream },
    { "SB5 an applied viewport's MaxZ is lifted above MinZ", CfMaxZ },
    { "SB6 Capture and Apply are refused while a block records", CfRefusedWhileRecording },
    { "SB7 SetFVF records the table declaration, SetFVF(0) nothing", CfFvf },
    { "SB8 a block's private reference keeps a texture alive", CfPrivate },
    { "SB9 the three typed masks", CfMasks },
    { "SB10 a Reset is refused while a block lives", CfResetRefused },
    { "SB11 a Capture never widens the block", CfNoWiden },
    { "SB12 Create and a second Begin are refused while a block records", CfCreateWhileRecording },
    { "SB13 a Set DXVK refuses is not recorded, an accepted one is", CfRefusedNotRecorded },
};
static void Conformance()
{
    const int N = (int)(sizeof(kCf) / sizeof(kCf[0]));
    int held = 0, caught = 0;
    for (int pass = 0; pass < 2; pass++)
    {
        gFkSnapshot = pass == 1;
        for (const CfCase& c : kCf)
        {
            FakeDev* d = new FakeDev; d->sc = new FakeSC; d->Defaults();
            const char* why = c.fn(d);
            if (!pass)
            {
                if (why) { printf("  conformance %s: WRONG: %s\n", c.name, why); gErr++; }
                else if (d->blocksLive || d->rec) { printf("  conformance %s: the case left %ld block(s) alive or a recording open\n", c.name, (long)d->blocksLive); gErr++; }
                else held++;
            }
            else if (!why) { printf("  conformance CONTROL %s: PASSED on the snapshot fake, so it cannot see its rule\n", c.name); gErr++; }
            else { caught++; printf("  control fails %s: %s\n", c.name, why); }
        }
    }
    gFkSnapshot = false;
    _InterlockedExchange(&gFkResetRefused, 0);             // SB10's, on purpose
    printf("  conformance: %d of %d of DXVK 3.1's state block rules hold on the fake; the snapshot fake fails %d of %d, each by name above\n", held, N, caught, N);
}


int main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)                               // the word `twin`, anywhere: taken out before the numbers are read
        if (!strcmp(argv[i], "twin")) { gTwin = true; for (int k = i; k + 1 < argc; k++) argv[k] = argv[k + 1]; argc--; i--; }
    const int cfg = argc > 1 ? atoi(argv[1]) : 2;
    gFkSleepEvery = argc > 2 ? atoi(argv[2]) : 64;
    const int steps = argc > 3 ? atoi(argv[3]) : 200000;
    const int h2Steps = argc > 4 ? atoi(argv[4]) : 60000;       // C1's H2; the design's gate runs it once at 1000000
    gSbmFault = argc > 5 ? atoi(argv[5]) : 0;                    // C1's planted fault (dt_sbtab.h): the run must then FAIL
    if (gSbmFault) printf("dt_test: C1's planted fault NC%d is IN: this run must FAIL\n", gSbmFault);
    gSbB = argc > 6 && atoi(argv[6]) != 0;                      // C1's switch (DT_SB_REC_ON): 1 runs every case at B, the recorded Apply
    if (gSbB) printf("dt_test: C1 at B: the recorded state-block Apply is ON\n");
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("dt_test: threaded=%d, the fake device sleeps on one call in %d, %d armed steps\n", cfg, gFkSleepEvery, steps);
    InitializeCriticalSection(&gFkLock);
    renderTid = GetCurrentThreadId();
    gFd = new FakeDev; gFd->sc = new FakeSC; gFd->Defaults(); gDev = gFd; D = gFd; SC = gFd->sc;
    gFd2 = new FakeDev; gFd2->sc = new FakeSC; gFd2->Defaults();
    R.Reset(&gFd->sc->bb);

    {   // THE DXVK CONFIG READER against DXVK 3.1's own rules (util/config/config.cpp): an
        // unscoped line applies, another exe's section does not, this exe's does; quotes
        // are dropped and a value ends at whitespace; the last one set wins; ';' splits
        // the environment's line.
        const char* conf = "# a comment\r\nd3d9.forceDrawTimeBufferUpload = True\n[other.exe]\n"
                           "d3d9.forceDrawTimeBufferUpload = False\n[game.exe]\n  d3d9.allowDirectBufferMapping=\"False\"  # trailing\n"
                           "[]\nd3d9.maxFrameRate = 60\n";
        char v[64] = ""; bool have = false;
        DtDxvkParse(conf, strlen(conf), '\n', "d3d9.forceDrawTimeBufferUpload", "game.exe", v, &have);
        if (!have || strcmp(v, "True")) Err("dxvk.conf reader: got '%s' for a key set unscoped and again for another exe", v);
        v[0] = 0; have = false;
        DtDxvkParse(conf, strlen(conf), '\n', "d3d9.allowDirectBufferMapping", "game.exe", v, &have);
        if (!have || strcmp(v, "False")) Err("dxvk.conf reader: got '%s' for a quoted value in this exe's section", v);
        v[0] = 0; have = false;
        DtDxvkParse(conf, strlen(conf), '\n', "d3d9.maxFrameRate", "game.exe", v, &have);
        if (have) Err("dxvk.conf reader: a key under an empty section header was applied");
        const char* envl = "d3d9.forceDrawTimeBufferUpload=TRUE;x.y=1;d3d9.forceDrawTimeBufferUpload=false";
        v[0] = 0; have = false;
        DtDxvkParse(envl, strlen(envl), ';', "d3d9.forceDrawTimeBufferUpload", "game.exe", v, &have);
        if (!have || strcmp(v, "false")) Err("dxvk.conf reader: the last of two values in the environment's line did not win");
        if (DtDxvkBool("d3d9.forceDrawTimeBufferUpload", false) || !DtDxvkBool("d3d9.allowDirectBufferMapping", true))
            Err("dxvk.conf reader: this folder has no dxvk.conf, yet an option read as set");
    }

    {   // THE threaded=0 ADVICE (1.0.5, 2026-09-22), held to the owner's labelled runs: silent wherever threaded=1 is
        // right, said on a frame where the two threads ran in series. Each is the block with the smallest overlap in
        // the log named; cost is the waits less the worker's time inside Present, overlap is the frame less the
        // worker's idle less the waits (DtReport). The old rule, cost above 2 alone, said it on the first two.
        struct { const char* what; bool whole; double cost, overlap; bool say; } c[] = {
            { "the owner's 60 Hz v-sync run, DLSSIV-tests\\pace\\spot-run1-as-it-ships.log", true, 12.954, 2.298, false },
            { "the owner's main folder at 139 fps, DLAA, v-sync off, GTAIV\\DLSS-IV.log",   true,  4.556, 1.708, false },
            { "an old build that blocked inside Present, DLSS-IV-census-183b313.log",       true,  0.000, 0.173, false },
            { "a serial frame: 3 ms of waits, 0.5 ms side by side",                          true,  3.000, 0.500, true  },
            { "the same frame in a window that spans an arm",                                false, 3.000, 0.500, false },
            { "a serial frame whose waits are under 2 ms",                                   true,  1.900, 0.100, false },
        };
        for (const auto& k : c)
            if (DtSayThreaded0(k.whole, k.cost, k.overlap) != k.say)
                Err("threaded=0 advice: %s: it was %s", k.what, k.say ? "not said" : "said");
    }

    {   // THE FAKE'S NEW RULES AGAINST DXVK 3.1'S OWN VALUES (2026-09-28, the review of ac9e4478): the reference and the fake share
        // FkXf, FkTssIdx and FkFreqOk, so a mistake in them would pass everywhere else. GetTransformIndex (d3d9_util.h),
        // RemapTextureStageStateType and SetStateTextureStageState's clamps, SetStreamSourceFreq's checks (d3d9_device.cpp).
        const bool xf = FkXf(D3DTS_VIEW) == 0 && FkXf(D3DTS_PROJECTION) == 1 && FkXf(D3DTS_TEXTURE0) == 2 && FkXf(D3DTS_TEXTURE7) == 9 &&
                        FkXf(D3DTS_WORLD) == 10 && FkXf(D3DTS_WORLDMATRIX(3)) == 13 && FkXf(D3DTS_WORLDMATRIX(4)) == -1 && FkXf(4) == -1;
        const bool tss = FkTssIdx(D3DTSS_COLOROP) == 0 && FkTssIdx(D3DTSS_CONSTANT) == 31 && FkTssIdx(40) == 31 && FkTssIdx(D3DTSS_ALPHAOP) == 3;
        const bool fq = FkFreqOk(0, 1) && !FkFreqOk(0, D3DSTREAMSOURCE_INSTANCEDATA | 1) && FkFreqOk(1, D3DSTREAMSOURCE_INSTANCEDATA | 1) &&
                        FkFreqOk(1, D3DSTREAMSOURCE_INDEXEDDATA | 3) && !FkFreqOk(1, D3DSTREAMSOURCE_INSTANCEDATA | D3DSTREAMSOURCE_INDEXEDDATA | 1) &&
                        !FkFreqOk(1, 0) && !FkFreqOk(16, 1);
        FakeDev* d = new FakeDev; d->sc = new FakeSC; d->Defaults();
        d->SetTextureStageState(9, D3DTSS_ALPHAOP, 5); float c4[16]; for (int k = 0; k < 16; k++) c4[k] = 1.0f; d->SetVertexShaderConstantF(254, c4, 4);
        const bool clamp = d->st.tss[7][3] == 5 && d->st.freq[3] == 1 && d->st.vsc[255][0] == 1.0f
                           && d->SetVertexShaderConstantF(300, c4, 1) == D3D_OK && d->SetVertexShaderConstantF(8192, c4, 1) == D3DERR_INVALIDCALL;
        if (!xf || !tss || !fq || !clamp) Err("the fake's rules differ from DXVK's values: transforms %d, stage states %d, frequencies %d, clamps and defaults %d", xf, tss, fq, clamp);
        EnterCriticalSection(&gFkLock); gFk.clear(); LeaveCriticalSection(&gFkLock);
    }

    {   // THE PIXEL-CONSTANT MARK (2026-09-28, sitting 15's Q2; dt_shadow.h, DtShPscMark): the bits it sets a word at a time
        // must be the bits the per-register loop it replaced set, for every (start, count) the wrapper can pass, from an empty
        // set and from a random one. THE CONTROL: a mask that forgets to stop at the end of the range, compared the same
        // way, must be caught.
        DWORD keep[7]; memcpy(keep, dtSh.pscOk, sizeof(keep));
        long cases = 0, ctlCaught = 0;
        for (int pass = 0; pass < 2; pass++)
            for (UINT s0 = 0; s0 < 224; s0++)
                for (UINT n = 1; n <= 224 - s0; n++)
                {
                    DWORD init[7], want[7], ctl[7];
                    for (int w = 0; w < 7; w++) init[w] = pass ? Rnd() : 0u;
                    memcpy(want, init, sizeof(want));
                    for (UINT r = s0; r < s0 + n; r++) want[r >> 5] |= 1u << (r & 31);
                    memcpy(dtSh.pscOk, init, sizeof(init));
                    DtShPscMark(s0, n);
                    if (memcmp(dtSh.pscOk, want, sizeof(want))) { Err("DtShPscMark(%u, %u) set other bits than the per-register loop", s0, n); break; }
                    memcpy(ctl, init, sizeof(ctl));
                    for (UINT r = s0; r < s0 + n; ) { const UINT lo = r & 31; ctl[r >> 5] |= 0xFFFFFFFFu << lo; r += 32 - lo; }
                    if (memcmp(ctl, want, sizeof(want))) ctlCaught++;
                    cases++;
                }
        memcpy(dtSh.pscOk, keep, sizeof(keep));
        printf("  pixel-constant mark: %ld ranges, every one the per-register loop's bits; the unclamped control caught on %ld\n", cases, ctlCaught);
        if (cases != 2 * 224 * 225 / 2 || ctlCaught < 1000) { printf("  the pixel-constant mark's proof did not run as it must\n"); gErr++; }
    }

    {   // WHICH FOREIGN HOOKS ON THE DEVICE THE INSTALL LETS STAND (1.0.8; dt_install.h, DtForeignSlotOk): apphelp.dll, on the
        // four slots the real layer hooks (tools/apphelp-probe): the three the layer calls on the calling thread, and Release.
        // Nothing else. A rule that let apphelp through on any slot is caught by AddRef (1) and Present (17); one that let any
        // module through on the four by the other names.
        static const struct { int slot; const char* mod; bool ok; } c[] = {
            { 0, "apphelp.dll", true }, { 2, "apphelp.dll", true }, { 13, "apphelp.dll", true }, { 16, "apphelp.dll", true }, { 16, "APPHELP.DLL", true },
            { 1, "apphelp.dll", false }, { 3, "apphelp.dll", false }, { 17, "apphelp.dll", false }, { 42, "apphelp.dll", false },
            { 118, "apphelp.dll", false }, { 0, "other.asi", false }, { 2, "other.asi", false }, { 13, "d3d9.dll", false }, { 16, "notapphelp.dll", false },
            { 0, "apphelp.dll.asi", false }, { 0, "no module", false }, { 0, nullptr, false } };
        int okN = 0;
        for (const auto& k : c)
        {
            if (DtForeignSlotOk(k.slot, k.mod) != k.ok) Err("the foreign-hook rule: slot %d of %s is %s", k.slot, k.mod ? k.mod : "(none)", k.ok ? "refused" : "let through");
            okN += k.ok;
        }
        for (int s = 0; s < 119; s++)   // and by exhaustion: exactly four slots
            if (DtForeignSlotOk(s, "apphelp.dll") != (s == 0 || s == 2 || s == 13 || s == 16)) Err("the foreign-hook rule: apphelp.dll on slot %d is judged wrongly", s);
        printf("  the foreign-hook rule: apphelp.dll on QueryInterface, Release, CreateAdditionalSwapChain and Reset only (%d cases let through, %d refused)\n",
               okN, (int)(sizeof(c) / sizeof(c[0])) - okN);
    }

    Conformance();

    // Install over a ring at the smallest size the layer accepts, so it wraps and fills.
    if (!DtStart(256u << 10)) { printf("DtStart refused\nFAILED\n"); return 1; }
    void** vt = *(void***)gFd;
    static const DWORD kVs[] = { 0xFFFE0300u, 0x0000FFFFu }, kPs[] = { 0xFFFF0300u, 0x0000FFFFu };
    if (gTwin)
    {
        // THE STAND-IN GOES IN, WITH PRESENT TOO AT FIRST: the install must refuse a foreign hook on a slot the worker
        // replays, name it, and name every slot another module held. Then Present comes out and the same install must take.
        char path[MAX_PATH]; GetModuleFileNameA(nullptr, path, sizeof(path));
        char* cut = strrchr(path, '\\'); strcpy_s(cut ? cut + 1 : path, 32, "twin\\apphelp.dll");
        gTwinMod = LoadLibraryA(path);
        if (gTwinMod)
        {
            gTwinPatch = (int(__cdecl*)(void**, int, DWORD(__cdecl*)()))GetProcAddress(gTwinMod, "TwinPatch");
            gTwinUnpatchPresent = (int(__cdecl*)())GetProcAddress(gTwinMod, "TwinUnpatchPresent");
            gTwinRead = (void(__cdecl*)(LONG*, LONG*))GetProcAddress(gTwinMod, "TwinRead");
            gTwinRepatch = (int(__cdecl*)())GetProcAddress(gTwinMod, "TwinRepatch");
        }
        if (!gTwinPatch || !gTwinUnpatchPresent || !gTwinRead || !gTwinRepatch) { printf("the stand-in %s did not load (error %lu)\nFAILED\n", path, GetLastError()); return 1; }
        // A COPY OF THE STAND-IN IN ANOTHER FOLDER, FIRST (the review of 2026-10-01): the same file name, the same four slots,
        // but neither Windows' own file nor the harness's folder. The install must refuse it and say why (DtForeignDirOk).
        {
            char other[MAX_PATH]; strcpy_s(other, path);
            char* c2 = strrchr(other, '\\'); if (c2) *c2 = 0;
            strcat_s(other, "\\other"); CreateDirectoryA(other, nullptr); strcat_s(other, "\\apphelp.dll");
            HMODULE om = CopyFileA(path, other, FALSE) ? LoadLibraryA(other) : nullptr;
            int(__cdecl* oPatch)(void**, int, DWORD(__cdecl*)()) = om ? (int(__cdecl*)(void**, int, DWORD(__cdecl*)()))GetProcAddress(om, "TwinPatch") : nullptr;
            int(__cdecl* oUnpatch)() = om ? (int(__cdecl*)())GetProcAddress(om, "TwinUnpatchAll") : nullptr;
            if (!oPatch || !oUnpatch || oPatch(vt, 0, &TwinWorkerTid) != 4) { printf("the copy of the stand-in (%s) did not load or hook (error %lu)\nFAILED\n", other, GetLastError()); return 1; }
            memcpy(dtDx, vt, sizeof(dtDx));
            if (DtInstall(gFd, vt, cfg, kVs, kPs) || dtArmable || dtInstalled)
                Err("the install took with a file named apphelp.dll from another folder holding the four slots");
            else if (!strstr(dtWhyNot, "device slot 0 (QueryInterface) belonged to apphelp.dll (a file of that name outside Windows' own folders)")
                     || !strstr(dtWhyNot, "(4 slots were another module's: 0, 2, 13, 16)"))
                Err("the refusal for an apphelp.dll outside Windows' folders did not say so: %s", dtWhyNot);
            else { gTwinDirRefused = true; printf("  a copy of the stand-in in another folder: refused, as it must be: %s\n", dtWhyNot); }
            if (oUnpatch() != 4) { printf("the copy of the stand-in could not take its hooks out\nFAILED\n"); return 1; }
            FreeLibrary(om);
        }
        for (int k = 0; k < 3; k++) gTwinBase[k] = gFkCaller3[k];
        gTwinRelBase = gFkRelease;
        if (gTwinPatch(vt, 1, &TwinWorkerTid) != 5) { printf("the stand-in could not hook the fake's vtable\nFAILED\n"); return 1; }
        memcpy(dtDx, vt, sizeof(dtDx));
        if (DtInstall(gFd, vt, cfg, kVs, kPs) || dtArmable || dtInstalled)
            Err("the install took with Present held by another module");
        else if (!strstr(dtWhyNot, "device slot 17 (Present) belonged to apphelp.dll before the mod's hooks, not DXVK")
                 || !strstr(dtWhyNot, "(5 slots were another module's: 0, 2, 13, 16, 17)"))
            Err("the refusal for a foreign Present did not name it and the five slots: %s", dtWhyNot);
        else { gTwinRefused = true; printf("  the stand-in on Present too: refused, as it must be: %s\n", dtWhyNot); }
        if (gKeptLine[0]) Err("a refused install said it kept slots: %s", gKeptLine);
        if (!gTwinUnpatchPresent()) { printf("the stand-in could not give Present back\nFAILED\n"); return 1; }
    }
    memcpy(dtDx, vt, sizeof(dtDx));
    if (!DtInstall(gFd, vt, cfg, kVs, kPs) || !dtArmable) { printf("DtInstall refused: %s\nFAILED\n", dtWhyNot); return 1; }
    if (gTwin)
    {
        if (!strstr(gKeptLine, "device slots 0 (QueryInterface), 2 (Release), 13 (CreateAdditionalSwapChain), 16 (Reset) belonged to apphelp.dll"))
            Err("the install did not say which slots it left to Windows' layer: \"%s\"", gKeptLine);
        char b[MAX_PATH];
        for (int s : { 0, 2, 13, 16 })
            if (DtModOf(dtDx[s]) != gTwinMod) Err("slot %d's original is %s's, not the stand-in's", s, DtModName(dtDx[s], b, sizeof(b)));
    }
    else if (gKeptLine[0]) Err("the install said it kept slots with no foreign module on the device: %s", gKeptLine);
    // THE TABLE AS THE INSTALL LEFT IT, as the mod takes it (DLSS-IV.cpp, after DtInstall): DtRetakeSlots' reference. Nothing has
    // moved yet, so a look now must take nothing back and say nothing.
    DtSnapSlots(vt);
    if (DtRetakeSlots("at once") != 0 || gRetakeLine[0]) Err("a look at an untouched table took slots back: \"%s\"", gRetakeLine);
    int wrapped = 0;
    for (int i = 0; i < 119; i++) wrapped += vt[i] == dtDevFn[i];
    printf("  install: %d of 119 device slots hold their wrapper\n", wrapped);
    // C1-P, THE DXVK CONFORMANCE PROBE (2026-09-29, step 3d; dt_sbtab.h): it ran in the install, on this fake, and every rule
    // held. THE CONTROLS: on a fresh device of its own, the probe must say which rule the fake then breaks, one knob each
    // (gFkBreak, dt_fake.h), and nothing on a device that breaks none.
    {
        if (!dtSbProbeOk) Err("C1's DXVK probe failed on the fake at install: %s", dtSbProbeWhy);
        static const char kRule[9] = { 0, '1', '2', '3', '4', '5', '6', '5', '5' };
        int caught = 0;
        for (int k = 0; k <= 8; k++)
        {
            FakeDev* pd = new FakeDev; pd->sc = new FakeSC; pd->Defaults();
            IDirect3DVertexBuffer9* pvb = nullptr; IDirect3DVertexDeclaration9* pde = nullptr;
            const D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
            pd->CreateVertexBuffer(64, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &pvb, nullptr);
            pd->CreateVertexDeclaration(el, &pde);
            // what the probe touches, which it must put back
            pd->SetStreamSource(0, pvb, 4, 8); pd->SetStreamSource(1, nullptr, 0, 0); pd->SetVertexDeclaration(pde); pd->SetRenderState(D3DRS_TEXTUREFACTOR, 0x5EEDu);
            { D3DVIEWPORT9 v = { 1, 2, 30, 40, 0.1f, 0.9f }; pd->SetViewport(&v); }
            const FkState before = pd->st;
            gFkBreak = k;
            const char* why = DtSbProbeRun(pd, pvb, pde);
            gFkBreak = 0;
            if (pd->st.vb[0] != before.vb[0] || pd->st.off[0] != before.off[0] || pd->st.str[0] != before.str[0] || pd->st.vb[1] != before.vb[1]
                || pd->st.decl != before.decl || pd->st.rs[D3DRS_TEXTUREFACTOR] != before.rs[D3DRS_TEXTUREFACTOR] || memcmp(&pd->st.vp, &before.vp, sizeof(before.vp)))
                Err("C1's DXVK probe did not put back what it touched (control %d)", k);
            if (!k) { if (why) Err("C1's DXVK probe failed on a fresh fake that breaks no rule: %s", why); }
            else if (!why || why[0] != kRule[k]) Err("C1's DXVK probe CONTROL %d: the fake broke rule %c and the probe said %s", k, kRule[k], why ? why : "every rule holds");
            else { caught++; printf("  C1's DXVK probe, control %d: %s\n", k, why); }
            if (pd->blocksLive || pd->rec) Err("C1's DXVK probe left %ld block(s) alive or a recording open (control %d)", (long)pd->blocksLive, k);
        }
        printf("  C1's DXVK probe: at install %s; the controls caught %d of 8 broken rules, each by its number\n", dtSbProbeWhy, caught);
        if (caught != 8) gErr++;
    }
    if (wrapped != 119) gErr++;
    EnterCriticalSection(&gFkLock); gFk.clear(); LeaveCriticalSection(&gFkLock);   // the install's own throwaways
    dtObs[57] = ObsRs; dtObs[82] = ObsDip; dtNoteObs[DT_NOTE_TEST] = ObsNote; dtPresentObs = ObsPresent;   // stage 2.0's mechanism, under test
    // The pixel constants are followed only once a body has been posted to the worker AND
    // somebody asks (dt_shadow.h). The unarmed and arming phases run without it, which is the
    // mod's default; from here on the harness says one has been, so the copy is what answers.
    dtShBodiesEver = true;

    for (int i = 0; i < 8; i++) gTex.push_back(NewTex());
    for (UINT i = 0; i < 4; i++)
    {
        IDirect3DSurface9* s = nullptr; const UINT w = 320 + 64 * i, h = 200 + 32 * i;
        D->CreateRenderTarget(w, h, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &s, nullptr);
        E(28, { w, h, (DWORD)D3DFMT_A8R8G8B8, 0u, 0u, 0u }); gRt.push_back(s);
    }
    {   // a render-target texture's level: it counts on its texture
        IDirect3DTexture9* t = nullptr;
        D->CreateTexture(256, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &t, nullptr);
        E(23, { 256u, 128u, 1u, (DWORD)D3DUSAGE_RENDERTARGET, (DWORD)D3DFMT_A8R8G8B8, (DWORD)D3DPOOL_DEFAULT });
        t->GetSurfaceLevel(0, &gLevel); gRt.push_back(gLevel);
    }
    for (UINT i = 0; i < 2; i++)
    {
        IDirect3DSurface9* s = nullptr; D->CreateDepthStencilSurface(640, 360, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &s, nullptr);
        E(29, { 640u, 360u, (DWORD)D3DFMT_D24S8, 0u, 0u, 0u }); gDs.push_back(s);
    }
    for (int i = 0; i < 4; i++) gVb.push_back(NewVb());
    D->CreateVertexBuffer(4096, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &gVbM, nullptr);
    E(26, { 4096u, (DWORD)D3DUSAGE_WRITEONLY, 0u, (DWORD)D3DPOOL_MANAGED });
    for (int i = 0; i < 3; i++)
    {
        IDirect3DIndexBuffer9* b = nullptr; D->CreateIndexBuffer(2048, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &b, nullptr);
        E(27, { 2048u, (DWORD)(D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY), (DWORD)D3DFMT_INDEX16, (DWORD)D3DPOOL_DEFAULT }); gIb.push_back(b);
        IDirect3DVertexShader9* vs = nullptr; D->CreateVertexShader(kVs, &vs); E(91, {}); gVs.push_back(vs);
        IDirect3DPixelShader9* ps = nullptr; D->CreatePixelShader(kPs, &ps); E(106, {}); gPs.push_back(ps);
    }
    const D3DVERTEXELEMENT9 el[] = { { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, D3DDECL_END() };
    for (int i = 0; i < 4; i++) { IDirect3DVertexDeclaration9* d = nullptr; gFkNextDeclFvf = kDeclFvf[i]; D->CreateVertexDeclaration(el, &d); E(86, {}); gDecl.push_back(d); }
    D->CreateQuery(D3DQUERYTYPE_EVENT, &gQ); E(118, { (DWORD)D3DQUERYTYPE_EVENT });

    for (int i = 0; i < 3000; i++) OneOp(false);
    H3RecordUnarmed();                                         // H3's blocks recorded before the layer arms
    Compare("unarmed (pass-through)");
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { printf("  the layer did not arm after 300 presents\nFAILED\n"); return 1; }
    Compare("arming presents");
    for (int i = 1; i <= steps; i++) { OneOp(true); if (i % 50000 == 0) Compare("armed traffic"); }
    Compare("armed traffic");
    Notes();
    Compare("notes");
    Bodies();
    Compare("worker-side bodies");
    StateBlocks();
    Compare("state blocks");
    if (!gGateEv) gGateEv = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    C1Records();
    Compare("C1: the records, case by case");
    C1Cost();
    Compare("C1: the bookkeeping's cost");
    if (gC1CostRan != 1) { printf("  C1's cost bench did not run\n"); gErr++; }
    dtNoteObs[kHarnessGate] = ObsGate;
    for (int v = 0; v < 3; v++) H1HeldApply(v);
    dtNoteObs[kHarnessGate] = nullptr;
    Compare("H1: an Apply behind a held worker");
    H2Program(h2Steps);
    Compare("H2: random programs with state blocks");
    H3Shapes();
    Compare("H3: the three shapes behind a live body");
    H4Reuse();
    Compare("H4: a block at a dead block's address");
    H11Lifetime();
    Compare("H11: a block that outlives a Release");
    H10Failures();
    Compare("H10: a failed Apply");
    H6ResetBetween();
    Compare("H6: a refused Reset between a Capture and an Apply");
    H7UnknownAtCapture();
    Compare("H7: a Capture over an entry another thread changed");
    H7bUnsettledKnown();
    Compare("H7 (b): a Capture with another thread's Set in flight");
    CapRecheck();
    Compare("the Capture's recheck after its command");
    C1FullTable();
    Compare("C1: a full table");
    C1Unarmed();
    Compare("C1: unarmed and cross-arm cases");
    H5bForeignCapture();
    Compare("H5 (b): another thread's Capture, then the producer's Apply");
    dtNoteObs[kHarnessGate] = ObsGate;
    H5aForeignApply(true);
    H5aForeignApply(false);                                        // its control
    for (int v = 0; v < 2; v++) H5dInFlightAcrossApply(v);
    dtNoteObs[kHarnessGate] = nullptr;
    Compare("H5 (a) and (d): another thread's Apply, and a call in flight across the producer's");
    H5cForeignDuringRecording();
    Compare("H5 (c): another thread's Set and Begin during the producer's recording");
    C1FvfForeign();
    Compare("C1-F across another thread's declaration");
    Foreign();
    Compare("another thread");
    ForeignOrdered();
    Compare("another thread, behind a held worker");
    BracketForeignBind();
    Compare("another thread's bind inside a bracket of ours");
    EngineStalePutBack();
    Compare("another thread's bind inside the engine's own bracket");
    if (gEngineRan != 1) { printf("  the engine put-back case did not run\n"); gErr++; }
    if (gBracketRan != 1) { printf("  the bracket case did not run\n"); gErr++; }
    ForeignOrderedRecording();
    BarrierRule();                                             // the count against the ring test on the full ring, exactly
    if (gBarrierRuleRan != 1) { printf("  the barrier's rule case did not prove what it must\n"); gErr++; }
    ForeignBarrierLap(true);
    ForeignBarrierLap(false);                                  // the control: the ring test, in the old read order, lets the call go early
    ForeignBarrierLap(true, true);                             // the full ring with the wait sampled every millisecond: well inside the cap
    ForeignAtRingEnd(true);
    ForeignAtRingEnd(false);                                   // the control: the end read raw lets the call go early
    if (gLapRan != 3) { printf("  the full-ring barrier cases and their control did not all prove what they must (%ld of 3)\n", gLapRan); gErr++; }
    Compare("another thread, under a recording producer, the barrier's cases");
    WakeCases();
    Compare("wake-every-64: a drain and the one-frame-ahead wait behind a held worker");
    Caller3Case();
    Compare("the three calls Windows' compatibility layer hooks, through the wrappers while armed");
    RetakeCase();
    Compare("Windows' hooks written back, and the slots taken back");
    Compare("another thread, under a recording producer");
    ForeignGetBesideSet();
    Compare("another thread, a Get beside its Set");
    if (gGetBesideSetRan != 1) { printf("  the Get-beside-a-Set case did not run\n"); gErr++; }
    ForeignDuringArm();
    Compare("another thread, inside the arm's shadow load");
    if (gArmRaceRan != 1) { printf("  the arm-race case did not run\n"); gErr++; }
    if (gOrderedHeld != 1 || gOrderedBusy != 1 || gForeignStayedArmed < 3) { printf("  the foreign-order phases did not all run (held %ld, recording %ld, stayed armed %ld)\n", gOrderedHeld, gOrderedBusy, gForeignStayedArmed); gErr++; }
    Presents();
    Compare("presents");
    if (!ReplayErrHas(DT_WHO_PRESENT, D3DERR_DEVICELOST)) { printf("  the failed Present was not counted against DT_WHO_PRESENT\n"); gErr++; }
    FailedDraw();
    Compare("a failed replay");
    OtherDevice();
    Compare("another device");
    ResetFromAnotherThread(true);
    Compare("foreign Reset, idle");
    LatePublishAfterIdleBarrier();
    Compare("foreign Reset, idle, then a late publish");
    if (gLateRan != 1) { printf("  the late-publish case did not run\n"); gErr++; }
    ResetFromAnotherThread(false);
    Compare("foreign Reset, unpublished");
    ResetWhileRecording();
    Compare("foreign Reset, recording");
    ResetAtRingEnd();
    Compare("foreign Reset, the producer parked at the ring's end");
    if (gAtEndRan != 3) { printf("  the ring-end cases and their control did not all prove what they must (%ld of 3)\n", gAtEndRan); gErr++; }
    NoteAcrossDisarm(true);
    Compare("a note across a disarm");
    NoteAcrossDisarm(false);                                   // the negative control: the orphan the re-test prevents
    Compare("a note across a disarm, control");
    Drain();
    { D3DPRESENT_PARAMETERS pp = {}; D->Reset(&pp); E(16, {}); R.Reset(&gFd->sc->bb); }
    if (dtArmed) { printf("  a Reset left the layer armed\n"); gErr++; }
    for (int i = 0; i < 300 && !dtArmed; i++) Present();
    if (!dtArmed) { printf("  the layer did not arm again after the Reset\n"); gErr++; }
    for (int i = 0; i < 20000; i++) OneOp(true);
    Compare("after a Reset");

    printf("\n  %ld Gets checked against the reference, %ld presents. The layer: armed %ld times, disarmed %ld,"
        " %ld shadow loads and settles, %ld entries fetched on demand after %ld dirty-alls, %ld replayed calls failed (%ld on purpose, each against its own slot);"
        " validation %ld Gets and %ld whole compares, %ld mismatches; %ld Sets and %ld releases from another thread, %ld releases run early\n",
        gGets, gPresents, dtArmCount, dtDisarms, dtShReloads, dtShFetched, dtShDirtied, (long)dtReplayErrN, gExpReplayErr,
        dtValGets, dtValChecks, dtValMis, (long)dtForeign, dtForeignRels, dtFrOom);
    // THE ON-DEMAND FETCH MUST HAVE RUN (stage 1.5 §2d): the state-block phase applies
    // a block and then Gets, and the foreign phase Sets from another thread and Gets.
    // A count of 0 here would mean the phases were answered from a copy that was never
    // marked unknown, and this proof would be proving nothing about the new path.
    if (!dtShFetched || !dtShDirtied) { printf("  the on-demand shadow fetch never ran (fetched %ld, dirty-alls %ld)\n", dtShFetched, dtShDirtied); gErr++; }
    // THE DECLARATION, THE FVF AND THE PIXEL CONSTANTS MUST HAVE BEEN ASKED FOR AND ANSWERED
    // FROM THE COPY (2026-09-19). The constants are followed only after a first ask, so the
    // flag itself is part of the proof. An FVF ask goes to DXVK the first time for an object
    // and again after anything that could have made a new object, and THIS TEST MAKES ONE
    // EVERY FEW HUNDRED STEPS, on purpose, so most asks here legitimately go there (a game
    // makes declarations while it loads, not while it draws). What must hold: some went to
    // DXVK, at least a quarter were answered from the copy, every answer was right (the
    // error count), and an address came back as a new declaration, or the one case the
    // per-object memory could get wrong was never put to it.
    printf("  declaration %ld Gets, FVF %ld Gets of which %ld went to DXVK, pixel constants %ld Gets (%s), %ld declaration addresses reused\n",
        gDeclGets, gFvfGets, dtShFvfAsked, gPscGets, dtShConsts ? "followed" : "NOT FOLLOWED", gDeclReused);
    printf("  stage states, transforms, stream frequencies and vertex constants: %ld Gets checked against the reference\n", gSyncKindGets);
    printf("  C1's cases at %s: H1 %ld of 3, H2 %ld Applies swept, H3 %ld frames, H4 %ld of 4 ways (%ld reused), H11 %ld, H10 %ld, H6 %ld, H7 %ld, H5b %ld of 1, H5a %ld of 2, H5c %ld, H5d %ld of 2 (the other thread's Set last %ld); %ld sweep Gets; no-wait checks that saw today's drain %ld, that held %ld, H2's that fell back %ld, fell back by design %ld\n",
        DT_SB_REC_ON() ? "B" : "A", gH1Ran, gH2Applies, gH3Frames, gH4Ran, gSbReused, gH11Ran, gH10Ran, gH6Ran, gH7Ran, gH5bRan, gH5aRan, gH5cRan, gH5dRan, gH5dAfter, gSweepGets, gNoWaitA, gNoWaitB, gNoWaitFellBack, gNoWaitByDesign);
    printf("  C1's table (step 3c): %ld recordings learned clean, not clean by reason:", dtSbWinLearned);
    for (int k = 1; k < DT_SBT_N; k++) if (dtSbWinTaintN[k]) printf(" %s %ld,", kDtSbTaintName[k], dtSbWinTaintN[k]);
    printf(" %ld Begins while one was learned; Captures %ld copied, %ld in a recording, %ld unsettled, %ld over an unknown entry, %ld foreign, %ld unarmed or another device's; peak %ld records, full %ld\n",
        dtSbWinBusy, dtSbCapCopied, dtSbCapInRec, dtSbCapInvalid, dtSbCapUnknown, (long)dtSbCapForeign, (long)dtSbCapNotOurs, dtSbLivePeak, dtSbFull);
    printf("  C1's oracle: %ld runs, %ld records checked, %ld clean, %ld with values, %ld writer runs, %ld live blocks found, %ld FVF objects; Applies: %ld would take the path, %ld made in a recording; the would-be path by reason:",
        gOrRuns, gOrEnt, gOrClean, gOrVals, gOrWrite, gOrLive, gOrFvf, gOrWouldOk, gOrWindowApplies);
    for (int k = 0; k < DT_SBF_N; k++) printf(" %s %ld,", kDtSbWhyName[k], dtSbApplyWhy[k]);
    printf(" the record cases %ld of 1\n", gC1RecRan);
    printf("  C1's Capture-copied records checked with their values known: %ld (a stream %ld, the viewport %ld, a declaration object %ld, an FVF %ld,"
        " unknown %ld, pixel constants %ld); H7 (b) %ld, the recheck %ld, the full table %ld, the unarmed and cross-arm cases %ld, C1-F across another thread %ld\n",
        gOrCapVals, gOrCapStream, gOrCapVp, gOrCapDeclObj, gOrCapDeclFvf, gOrCapDeclUnk, gOrCapPsc, gH7bRan, gCapRecheckRan, gC1FullRan, gC1UnarmedRan, gC1FvfRan);
    if (gC1RecRan != 1 || gOrRuns < 100 || gOrClean < 200 || gOrVals < 100 || gOrWrite < 100 || gOrLive < 200 || gOrWouldOk < 10 || gOrWindowApplies < 10
        || dtSbWinLearned < 50 || dtSbCapCopied < 20)
    { printf("  C1's oracle did not run often enough to mean anything\n"); gErr++; }
    if (gOrCapVals < 20 || !gOrCapStream || !gOrCapVp || !gOrCapDeclObj || !gOrCapDeclFvf || !gOrCapDeclUnk || !gOrCapPsc
        || gH7bRan != 1 || gCapRecheckRan != 1 || gC1FullRan != 1 || gC1UnarmedRan != 1 || gC1FvfRan != 1)
    { printf("  C1's Capture copies were not checked on every kind, or a case did not run\n"); gErr++; }
    if (gH1Ran != 3 || gH2Applies < 200 || gH3Frames < 60 || gH4Ran != 4 || gSbReused != 4 || gH11Ran != 1 || gH10Ran != 1 || gH6Ran != 1 || gH7Ran != 1 || gH5bRan != 1 || gH5aRan != 2 || gH5cRan != 1 || gH5dRan != 2 || (DT_SB_REC_ON() ? gNoWaitB : gNoWaitA) < 250)
    { printf("  C1's cases did not all run, or too few of their no-wait checks were made\n"); gErr++; }
    if (gSyncKindGets < 100) { printf("  the four SYNC-answered kinds were not asked for often enough to mean anything\n"); gErr++; }
    if (gDeclGets < 100 || gFvfGets < 100 || gPscGets < 100 || gDeclReused < 5 || !dtShConsts || !dtShFvfAsked || (gFvfGets - dtShFvfAsked) * 4 < gFvfGets)
    { printf("  the declaration, FVF or pixel constant path was not exercised as it must be\n"); gErr++; }
    printf("  worker-side bodies: %ld rounds with the worker inside one; texture slot 0 waited for it %ld times; a free sampler slot did not %ld times,"
           " a render state it does not set did not %ld times, and the one it does set was already known %ld times\n",
           gBodyRounds, gBodyGated, gBodyFree, gBodyFreeRs, gBodyKnown);
    printf("  ...and the texture and sampler state of the slot it binds were already known %ld times\n", gBodyKnownBound);
    if (gBodyRounds < 60 || gBodyGated < 20 || gBodyFree < 60 || gBodyFreeRs < 60 || gBodyKnown < 20 || gBodyKnownBound < 20) { printf("  the fetch gate was not exercised\n"); gErr++; }
    printf("  query GetData: %ld made, every one after its own Issue; %ld of them beside a running body without waiting for it\n", gGetDatas, gGetDataBesideBody);
    if (gGetDatas < 100 || gGetDataBesideBody < 60) { printf("  the aimed GetData was not exercised\n"); gErr++; }
    // EVERY REFUSED SetRenderTarget REACHED THE DEVICE, and enough were made to mean something.
    printf("  refused SetRenderTarget calls: %ld made, %ld reached the device\n", gRefusedRtMade, (long)gFkRefusedRt);
    if (gRefusedRtMade < 20 || gFkRefusedRt != gRefusedRtMade) { printf("  the refused-SetRenderTarget path was not exercised, or a refusal never reached the device\n"); gErr++; }
    // STAGE 2.0'S MECHANISM MUST HAVE BEEN EXERCISED: notes seen by the worker, handoffs
    // read by the producer. Zero of either means the phases above proved nothing.
    printf("  stage 2.0: the worker consumed %ld notes (%ld refused on purpose), the producer read %ld handoffs and was refused"
           " %ld pre-Reset ones after a re-arm\n", gNotesSeen, dtNoteRefused, gHandoffChecked, gHandoffRefusedAcrossArm);
    if (gNotesSeen < 2000 || gHandoffChecked < 100) { printf("  the note or handoff path was not exercised\n"); gErr++; }
    // Every re-arm's first armed Present asks for the last pre-Reset ordinal, which
    // the layer must refuse (review 2026-09-14): zero here means that path was not
    // walked, and the four re-arms above walk it four times.
    if (gHandoffRefusedAcrossArm < 1) { printf("  no ask for a pre-Reset handoff was made after a re-arm: the case was not exercised\n"); gErr++; }
    // THE PAIR TABLE'S LIVE COUNT (2026-09-17): with everything queued run, no object
    // has a pair outstanding, so the count is 0; and a peak of 0 would mean no Release
    // was ever deferred and the line above it proved nothing about the counter.
    Present();
    DtDrain(DT_R_DIRECT);
    AcquireSRWLockExclusive(&dtPendLock);
    const int pendLive = dtPendLive, pendPeak = dtPendPeak, pendUsed = dtPendUsed;
    int pendWalk = 0;
    for (int i = 0; i < DT_PEND_CAP; i++) if (dtPend[i].obj && dtPend[i].n > 0) pendWalk++;
    ReleaseSRWLockExclusive(&dtPendLock);
    printf("  pair table: %d live now (%d by walking it), peak %d live, %d entries in use of %d, %ld misses\n",
        pendLive, pendWalk, pendPeak, pendUsed, DT_PEND_CAP, dtPendMiss);
    if (pendLive != 0 || pendWalk != 0) { printf("  the pair table's live count did not return to 0 after a drain\n"); gErr++; }
    if (pendPeak < 1) { printf("  the pair table's peak never moved: no deferred Release was counted\n"); gErr++; }
    // THE THREE CALLS WINDOWS' COMPATIBILITY LAYER HOOKS (1.0.8): every one of the run was made on the calling thread.
    printf("  the calls Windows' compatibility layer hooks: QueryInterface %ld, CreateAdditionalSwapChain %ld, Reset %ld; on the worker %ld\n",
           (long)gFkCaller3[0], (long)gFkCaller3[1], (long)gFkCaller3[2], (long)gFkOnWorker3);
    if (gFkOnWorker3 || !gCaller3Ran || gFkCaller3[2] < 1)
    { printf("  one of the three was made on the worker, or the case and its control did not run, or no Reset was made\n"); gErr++; }
    if (!gRetakeRan) { printf("  the take-back case did not run whole\n"); gErr++; }
    if (gTwin)
    {
        // THE STAND-IN'S OWN COUNT: every call of the three that reached the fake since it went in came through it (the
        // wrapper, then the foreign hook, then the device), and it saw the worker make only the control's.
        // RELEASE is the fourth slot the real layer holds, and the one the worker may call (dt_install.h). Held to the same
        // count as the others (the review of 2026-10-01): every Release the fake device took since the stand-in went in came
        // through it, and the worker made some of them (the body case releases there), so the path the fix argues for ran.
        LONG tc[5] = {}, tw[5] = {}; gTwinRead(tc, tw);    // QueryInterface, CreateAdditionalSwapChain, Reset, Release, Present
        const LONG tw3 = tw[0] + tw[1] + tw[2];
        printf("  the stand-in (twin\\apphelp.dll) passed on QueryInterface %ld, CreateAdditionalSwapChain %ld, Reset %ld, Present %ld; as the worker %ld (the control's %ld); Release %ld (%ld on the worker); a foreign Present refused first: %s; a copy from another folder refused: %s\n",
               (long)tc[0], (long)tc[1], (long)tc[2], (long)tc[4], (long)tw3, (long)gTwinControl, (long)tc[3], (long)tw[3], gTwinRefused ? "yes" : "NO", gTwinDirRefused ? "yes" : "NO");
        if (!gTwinRefused || !gTwinDirRefused) gErr++;
        if (tc[3] != gFkRelease - gTwinRelBase)
        { printf("  the stand-in passed on %ld Release(s) and the device took %ld\n", (long)tc[3], (long)(gFkRelease - gTwinRelBase)); gErr++; }
        if (tw[3] < 1) { printf("  no Release reached the stand-in on the worker: the path the fix lets stand did not run\n"); gErr++; }
        for (int k = 0; k < 3; k++)
            if (tc[k] != gFkCaller3[k] - gTwinBase[k])
            { printf("  the stand-in passed on %ld call(s) of kind %d and the device took %ld\n", (long)tc[k], k, (long)(gFkCaller3[k] - gTwinBase[k])); gErr++; }
        if (tc[0] < 2 || tc[1] < 1 || tc[2] < 1 || tc[3] < 1 || tc[4] != 0) { printf("  the stand-in was not called on each of the four, or was called on Present\n"); gErr++; }
        if (tw3 != gTwinControl || gTwinControl != 1) { printf("  the stand-in saw %ld call(s) of the three made as the worker, the control made %ld\n", (long)tw3, (long)gTwinControl); gErr++; }
    }
    // NO RESET WAS REFUSED FOR A LIVE BLOCK outside the conformance case that makes one, and the reference forgot none.
    if (gFkResetRefused) { printf("  %ld Reset(s) were refused because a state block was alive\n", (long)gFkResetRefused); gErr++; }
    if (!gRefBlk.empty() || rRec) { printf("  the reference still holds %zu state block(s)%s\n", gRefBlk.size(), rRec ? " and an open recording" : ""); gErr++; }
    const int fails = gErr + (dtValMis ? 1 : 0) + ((long)dtReplayErrN != gExpReplayErr ? 1 : 0) + (dtFrOom ? 1 : 0);
    // A FAILED WITH NO LINE ABOVE IT SAYING WHY IS NOT A RESULT (2026-09-19: a control failed that
    // way and the four things summed here had to be guessed at). Say which of them it was.
    if (fails) printf("  FAILED BECAUSE: %d error(s) counted, %ld validation mismatch(es), %ld replayed call(s) failed against %ld expected, %ld release(s) run early\n",
        (int)gErr, (long)dtValMis, (long)dtReplayErrN, gExpReplayErr, (long)dtFrOom);
    printf("%s\n", fails ? "FAILED" : "PASS");
    return fails ? 1 : 0;
}
