// dt_fake_ring_test.cpp: proves dt_fake_ring.h, the harness's fake of DXVK, before anything is proved WITH it
// (2026-09-25, phase 1C, stage S1).
//
//   python run_proof.py dt_fake_ring_test          (or: dt_fake_ring_test.exe [threaded])
//
// WHY A PROOF OF A FAKE. dt_fg_test.cpp will hold frame generation's machine to DXVK's ordering through this fake, and a
// fake that is kinder than DXVK would pass a machine that breaks in the game: a ring that never rotates hides the stale
// restore, a command stream that runs at once hides the one-image hazard, a recursive lock hides the nesting bug. So each
// behaviour the fake claims is shown HAPPENING here, and each has a negative case beside it that shows the check can
// fail: the rotation against the one-image ring that does not rotate, a surface-named call against a handle-named copy,
// the hazard with and without the flush, the stream order against a raw probe, the lock's release on a nested take
// against a recursive lock that keeps the other thread out, a violation made on purpose for every violation counter.
// The last case runs the game's own ColorFill, StretchRect, Clear and Present through the REAL device thread, armed, so
// the worker's replay is what writes the tags.
// WHAT THE VERIFIER'S REVIEW ADDED (2026-09-25): a Reset (case 10) and the sample classes (case 11); in the others, a
// failed Present, the back-buffer index refusal, the lock table beyond ColorFill with an unlocked control, the order of a
// submit's signal after its copies, a sample recorded before a Present, the command-buffer reservation per span, and the
// three CS modes where an answer depends on them. Each was a behaviour whose removal this test did not notice before
// (the verifier's mutants M13, M14, M18 to M20 and M24 to M26); and a join now fails the run instead of hanging it.
#include <windows.h>
#include <d3d9.h>
#include <intrin.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

// The layer's log and its latency and classifier stubs, exactly as dt_test.cpp has them (nothing about the device thread
// is stubbed by these).
static bool  telemetryOn = true;
static DWORD renderTid   = 0;
static int   rsOwnCreate = 0;
static int   gLayerLines = 0;
static void LogRecord(const char* fmt, ...)
{
    if (gLayerLines++ >= 200) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
static void LogHealth(const char* fmt, ...)
{
    if (gLayerLines++ >= 200) return;
    va_list a; va_start(a, fmt); printf("  [layer] "); vprintf(fmt, a); printf("\n"); va_end(a);
}
static void LogDetail(const char* fmt, ...) { (void)fmt; }
#define LOGMARK ""
#ifndef DEVSTAT
#define DEVSTAT(...) __VA_ARGS__
#endif
#ifndef DLSSIV_HOT
#define DLSSIV_HOT
#endif
static void IvtWarn(const char* fmt, ...) { (void)fmt; }
static bool gNoteRecheck = true;
#define DT_NOTE_RECHECK() gNoteRecheck
static bool gSyncOrdered = true;
#define DT_SYNC_ORDERED_ON() gSyncOrdered
enum { LAT_RING = 64 };
static bool latOn = false, latPacerOn = false;
static volatile LONG latPresFrame[LAT_RING];
DEVSTAT(alignas(8) static volatile LONGLONG latFlipRelQpc[LAT_RING]; static volatile float latFlipRelBlk[LAT_RING]; static volatile LONG latFlipRelN[LAT_RING];)
static volatile LONG frames = 0;
static void LatOnPresent(LONG, LONGLONG) {}
// THE REFRESH GUARD (next, 2026-09-26; merged into frame generation 2026-09-28): dt_res.h tests it after the pacing
// wait. Never armed here, as the rig parks it in the mod (FgRigParkGuard): a no-op, as dt_test's is.
static bool latGuard = false;
static void RgOnPresent() {}
#include "../../source/devthread/devthread.h"
// THE FAKE ASKS WHICH THREAD IS THE WORKER (DLSS-IV 1.0.8, folded in 2026-10-03): dt_fake.h counts the three device calls
// Windows' compatibility layer hooks whenever the worker makes one. The device thread's own id, as dt_test's is without
// its stand-in for that layer.
static DWORD FkWorkerTid() { return dtWorkerTid; }
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
#include "../../source/fg/fg_machine.h"
#include "dt_fake_ring.h"

typedef FgHarnessBinding B;
static FakeDev* gFd; static IDirect3DDevice9* D; static FakeRingSC* gSc; static IDirect3DSwapChain9* SC;
static IDirect3DSurface9* gOtherRt = nullptr;       // a render target that is not the ring's: the other thread's calls name it
static IDirect3DSurface9* gGameRt = nullptr;        // the game's own target, for the replayed StretchRect
static int gCases = 0;
static const DWORD kGen = 0xFFFF00FFu, kBlack = 0xFF000000u;

// A D3D9 call straight to the wrapped device, as a DT_DIRECT scope or the worker's replay makes one.
static void DevFill(IDirect3DSurface9* s, DWORD tag) { ((dtT_ColorFill)dtDx[35])(D, s, nullptr, tag); }
static void RawCopyFrom(int img, ULONGLONG h) { B::CbTake(1); B::QLock(); B::RecBegin(); B::RecCopyFromImg(img, h); B::RecSubmit(); B::QUnlock(); }
static void RawEmpty() { B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSubmit(); B::QUnlock(); }
static void RawSignal(int tl, LONG v) { B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSignal(tl, v); B::RecSubmit(); B::QUnlock(); }
static void Present() { B::DxvkPresent(SC, nullptr, nullptr, nullptr, 0); }
static DWORD CurTag() { return RingTag(RingCurrent()); }
static RingPresented LastPresented() { auto v = RingPresentedLog(); return v.empty() ? RingPresented{ 0xDEADu, 0, -1, -1 } : v.back(); }
static void DrainSamples() { int k, c; LONG p, f; while (B::SampleCollect(&k, &p, &f, &c)) {} }
static HANDLE Spawn(LPTHREAD_START_ROUTINE f, void* a = nullptr) { return CreateThread(nullptr, 0, f, a, 0, nullptr); }
// A JOIN THAT CANNOT HANG THE RUN (2026-09-25, after the verifier's review: an UnlockDevice that released nothing was
// caught only by a stall, killed at 600 s, because this waited INFINITE). A thread still running after 10 s is stuck on a
// modelled lock nobody will release, and every later case would stall behind it, so the run says which join and stops.
static void Join(HANDLE h, const char* what = "a helper thread")
{
    if (WaitForSingleObject(h, 10000) != WAIT_OBJECT_0)
    {
        printf("  HUNG: %s did not finish within 10 s (a modelled lock was never released)\nFAILED\n", what);
        ExitProcess(3);
    }
    CloseHandle(h);
}
static bool WaitFlag(volatile LONG* f, DWORD ms) { const ULONGLONG t = GetTickCount64() + ms; while (!*f && GetTickCount64() < t) Sleep(1); return *f != 0; }
static void Fresh(D3DSWAPEFFECT e, UINT bbc, BOOL w) { B::Flush(); RingConfigure(e, bbc, w); RingClearLogs(); }
static double Ms(LONGLONG t) { return (double)t * 1000.0 / (double)B::Qpf(); }

// ANOTHER THREAD'S D3D9 CALL, and when it returned.
static volatile LONG gForeignDone = 0;
static DWORD WINAPI ForeignFill(void*) { DevFill(gOtherRt, 0x5151u); _InterlockedExchange(&gForeignDone, 1); return 0; }

// ---------------------------------------------------------------- 1. the ring: how many images, and the rotation
// WHAT IT SHOWS. For each of the harness's four shapes, which image is under surface 0 after 1, 2, 3 and 4 presents, that
// the surface object never changes, that the fresh handle query follows the image, and that the present blit read the
// image that was current AT each Present. THE NEGATIVE: (DISCARD, 1) windowed is one image and never rotates, while the
// same BackBufferCount fullscreen is two images and does, so the fake's rule is DXVK's SwapWithFrontBuffer and not a
// count of back buffers.
static void CaseRotation()
{
    const struct { D3DSWAPEFFECT e; UINT bbc; BOOL win; int n; const char* what; } k[] = {
        { D3DSWAPEFFECT_DISCARD, 1, TRUE,  1, "DISCARD, 1, windowed" },
        { D3DSWAPEFFECT_FLIP,    1, TRUE,  2, "FLIP, 1" },
        { D3DSWAPEFFECT_DISCARD, 2, TRUE,  3, "DISCARD, 2 (the owner's)" },
        { D3DSWAPEFFECT_DISCARD, 3, TRUE,  4, "DISCARD, 3" },
        { D3DSWAPEFFECT_DISCARD, 1, FALSE, 2, "control: DISCARD, 1, fullscreen" } };
    for (const auto& c : k)
    {
        Fresh(c.e, c.bbc, c.win);
        if (gRing.n != c.n) { Err("rotation, %s: %d images, DXVK makes %d", c.what, gRing.n, c.n); continue; }
        int im0[4];
        for (int i = 0; i < c.n; i++) { im0[i] = RingUnder(i); RingSetTag(im0[i], 0xA000u + i); }
        IDirect3DSurface9* s0 = B::BbSurface(); s0->Release();
        const ULONGLONG h0 = B::BBImage();
        int after[4];
        for (int p = 1; p <= 4; p++)
        {
            Present();
            after[p - 1] = RingCurrent();
            if (after[p - 1] != im0[p % c.n]) Err("rotation, %s: after %d presents image %d is current, DXVK's rotation gives %d", c.what, p, after[p - 1], im0[p % c.n]);
            if (B::BBImage() != RingHandle(after[p - 1])) Err("rotation, %s: the fresh handle query did not follow the image", c.what);
            IDirect3DSurface9* s = B::BbSurface(); s->Release();
            if (s != s0) Err("rotation, %s: GetBackBuffer(0) handed out another object after a present", c.what);
        }
        B::Flush();
        const auto log = RingPresentedLog();
        if (log.size() != 4) Err("rotation, %s: %zu blits ran, not 4", c.what, log.size());
        else for (int p = 0; p < 4; p++)
            if (log[p].tag != 0xA000u + (DWORD)(p % c.n)) Err("rotation, %s: present %d showed %lX, the image current at it held %lX", c.what, p + 1, log[p].tag, 0xA000u + (DWORD)(p % c.n));
        const bool rotated = B::BBImage() != h0 || c.n == 4;    // after 4 presents a 4-ring (and a 2-ring) is back where it began
        printf("  rotation, %-32s %d image(s); current after 1-4 presents: slot %d %d %d %d; presented %lX %lX %lX %lX\n", c.what, c.n,
            (int)(std::find(im0, im0 + c.n, after[0]) - im0), (int)(std::find(im0, im0 + c.n, after[1]) - im0),
            (int)(std::find(im0, im0 + c.n, after[2]) - im0), (int)(std::find(im0, im0 + c.n, after[3]) - im0),
            log.size() > 0 ? log[0].tag : 0, log.size() > 1 ? log[1].tag : 0, log.size() > 2 ? log[2].tag : 0, log.size() > 3 ? log[3].tag : 0);
        if (c.n == 1 && (after[0] != im0[0] || after[3] != im0[0])) Err("rotation, %s: a one-image ring rotated", c.what);
        if (c.n >= 2 && after[0] == im0[0]) Err("rotation, %s: a ring of %d did not rotate", c.what, c.n);
        (void)rotated;
    }
    // A FAILED PRESENT (added 2026-09-25): a lost device answers before anything else (d3d9_swapchain.cpp:117-118), so
    // no blit is recorded and the ring does not rotate. The machine reads both after a fire's Present (fg_machine.h's
    // rotation test and dtPresHr), so a fake that rotated or blitted on a failure would lie to it.
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    const int curF = RingCurrent(); const size_t pendF = RingCsPending(); const LONG presF = gRingSt.presents, failF = gRingSt.presentsFailed;
    gFkPresentHr = (LONG)D3DERR_DEVICELOST;
    const HRESULT hrF = B::DxvkPresent(SC, nullptr, nullptr, nullptr, 0);
    if (hrF != D3DERR_DEVICELOST) Err("rotation, failed present: it answered %08lX, not the lost device", hrF);
    if (RingCurrent() != curF) Err("rotation, failed present: the ring rotated");
    if (RingCsPending() != pendF) Err("rotation, failed present: %zu items were recorded (a blit?)", RingCsPending() - pendF);
    if (gRingSt.presents != presF || gRingSt.presentsFailed != failF + 1) Err("rotation, failed present: counted as a present");
    // THE BACK-BUFFER INDEX (added 2026-09-25): the swapchain refuses an index at or past BackBufferCount (483-486), which
    // is the count the game asked for, not the ring's size; the device's refuses a swapchain other than 0 (d3d9_device.cpp:
    // 560-561). With BackBufferCount 2 the ring is 3 images, and index 2 is still refused.
    IDirect3DSurface9* s = nullptr;
    const HRESULT scIn = SC->GetBackBuffer(1, D3DBACKBUFFER_TYPE_MONO, &s); IDirect3DSurface9* s1 = s; if (s) s->Release();
    s = nullptr; const HRESULT scOut = SC->GetBackBuffer(2, D3DBACKBUFFER_TYPE_MONO, &s);
    typedef HRESULT(__stdcall* GetBb_t)(IDirect3DDevice9*, UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9**);
    s = (IDirect3DSurface9*)1; const HRESULT dvOut = ((GetBb_t)dtDx[18])(D, 0, 2, D3DBACKBUFFER_TYPE_MONO, &s); IDirect3DSurface9* sOut = s;
    s = nullptr; const HRESULT dvSc = ((GetBb_t)dtDx[18])(D, 1, 0, D3DBACKBUFFER_TYPE_MONO, &s);
    s = nullptr; const HRESULT dvIn = ((GetBb_t)dtDx[18])(D, 0, 1, D3DBACKBUFFER_TYPE_MONO, &s); if (s) s->Release();
    if (FAILED(scIn) || s1 != gRing.surf[1]) Err("rotation, GetBackBuffer: the swapchain's index 1 was refused or is not surface 1");
    if (scOut != D3DERR_INVALIDCALL) Err("rotation, GetBackBuffer: the swapchain's index 2 of 2 back buffers was not refused");
    if (dvOut != D3DERR_INVALIDCALL || sOut != nullptr) Err("rotation, GetBackBuffer: the device's index 2 was not refused with a null surface");
    if (dvSc != D3DERR_INVALIDCALL) Err("rotation, GetBackBuffer: the device's swapchain 1 was not refused");
    if (FAILED(dvIn)) Err("rotation, GetBackBuffer: the device's index 1 was refused");
    printf("  rotation: a failed present rotated nothing and blitted nothing; GetBackBuffer index 2 of 2 refused (swapchain and device), swapchain 1 refused\n");
    gCases++;
}

// ---------------------------------------------------------------- 2. a surface-named call against a handle-named copy
// WHAT IT SHOWS. After a Present, a D3D9 fill of GetBackBuffer(0) lands on the NEW current image, and a raw copy into the
// handle queried BEFORE the Present lands on the OLD one, which the ring rotated away. So a restore must query the handle
// after the Present (map 2.8, R4 3.2), and a D3D9 call naming the surface needs no query at all. AND THE RESOLUTION
// (dt_fake_ring.h's head): the fake takes the image at the call, as DXVK does; in DXVK's order that always equals the
// image when the list runs (csResolveDiffer stays 0). THE NEGATIVE: with the Present's own flush turned off, a fill made
// before the Present runs after the rotation, still lands on the image taken at its call, and the count moves to 1.
static void CaseSurfaceAgainstHandle()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    for (int i = 0; i < gRing.n; i++) RingSetTag(RingUnder(i), 0xB000u + i);
    RingSetTag(gRing.rigImg[FG_IMG_PRESENT], 0xC1u);
    const ULONGLONG h0 = B::BBImage(); const int old = RingImgOfHandle(h0);
    const LONG diff0 = gRingSt.csResolveDiffer;
    Present();
    IDirect3DSurface9* bb = B::BbSurface(); DevFill(bb, 0xC2u); bb->Release();
    RawCopyFrom(FG_IMG_PRESENT, h0);
    B::Flush();
    const int cur = RingCurrent();
    if (cur == old) Err("surface/handle: the ring did not rotate, the case proves nothing");
    if (RingTag(cur) != 0xC2u) Err("surface/handle: the fill of GetBackBuffer(0) made after the Present is not on the current image (%lX)", RingTag(cur));
    if (RingTag(old) != 0xC1u) Err("surface/handle: the raw copy into the handle queried before the Present is not on that image (%lX)", RingTag(old));
    if (RingTag(cur) == 0xC1u || RingTag(old) == 0xC2u) Err("surface/handle: a copy landed on the other image");
    if (gRingSt.csResolveDiffer != diff0) Err("surface/handle: %ld CS items ran on an image that was no longer current, with no work recorded before the Present", gRingSt.csResolveDiffer - diff0);
    // WHERE THE READINGS PART (revised 2026-09-25): a fill made before a Present lands on the image taken at its call
    // however far the CS thread got, and the map's reading ("the image current when it runs") is wrong exactly when the
    // CS thread had not run it before the rotation: 0 items with RING_CS_PRE and RING_CS_ALL, 1 with RING_CS_LAZY.
    const int cs0 = gRing.presentCs;                        // the default, put back after: case 8 relies on it
    const int modes[3] = { RING_CS_PRE, RING_CS_ALL, RING_CS_LAZY };
    LONG diffBy[3] = {}; int pre = -1;
    for (int m = 0; m < 3; m++)
    {
        gRing.presentCs = modes[m];
        const LONG d0 = gRingSt.csResolveDiffer;
        pre = RingCurrent();
        bb = B::BbSurface(); DevFill(bb, 0xC3u + m); bb->Release();
        Present();
        const int post = RingCurrent();
        B::Flush();
        diffBy[m] = gRingSt.csResolveDiffer - d0;
        if (RingTag(pre) != 0xC3u + m || RingTag(post) == 0xC3u + m) Err("surface/handle, cs mode %d: the fill made before the Present did not land on the image taken at its call", modes[m]);
    }
    gRing.presentCs = cs0;
    if (diffBy[0] != 0 || diffBy[1] != 0 || diffBy[2] != 1) Err("surface/handle: csResolveDiffer moved by %ld (PRE), %ld (ALL), %ld (LAZY), not 0 0 1", diffBy[0], diffBy[1], diffBy[2]);
    printf("  surface against handle: after a present the fill of GetBackBuffer(0) is on image %d (the new current), the copy into the old handle on image %d;"
           " a fill before the Present stayed on its image in every CS mode, and the map's reading differed on %ld (PRE) %ld (ALL) %ld (LAZY) items\n",
           cur, old, diffBy[0], diffBy[1], diffBy[2]);
    gCases++;
}

// ---------------------------------------------------------------- 3. the one-image hazard
// WHAT IT SHOWS (map 2.8, R2 F1, R4 3.2). At one image, the Present's blit is still in the command stream when Present
// returns, and a raw restore into the same image overtakes it: the screen shows the restored image, not the one presented.
// With the rig's post-present flush first, the blit reads what was presented. THE CONTROL: at three images the same raw
// restore without a flush is harmless, because it lands on another image. AND THE CS MODES (2026-09-25): the hazard can
// happen (LAZY and PRE, the CS thread had not run the blit when Present returned) and can also not happen (ALL, it had),
// which is why the rig's flush is unconditional at one image and a harness case must not rely on either outcome alone.
static void CaseOneImageHazard()
{
    const DWORD kI = 0xE1u, kSaved = 0xE2u;
    RingSetTag(gRing.rigImg[FG_IMG_SAVE], kSaved);
    Fresh(D3DSWAPEFFECT_DISCARD, 1, TRUE);
    const int cs0 = gRing.presentCs;
    DWORD byMode[3] = {};
    for (int m = RING_CS_LAZY; m <= RING_CS_ALL; m++)
    {
        gRing.presentCs = m;
        RingSetTag(RingCurrent(), kI);
        Present();
        RawCopyFrom(FG_IMG_SAVE, B::BBImage());             // the restore, with no flush after the Present
        B::Flush();
        byMode[m] = LastPresented().tag;
    }
    gRing.presentCs = cs0;
    const DWORD hazard = byMode[RING_CS_LAZY];
    if (byMode[RING_CS_LAZY] != kSaved || byMode[RING_CS_PRE] != kSaved) Err("one image, no flush: the blit showed %lX (LAZY) %lX (PRE); the restore should have overtaken it and shown %lX", byMode[RING_CS_LAZY], byMode[RING_CS_PRE], kSaved);
    if (byMode[RING_CS_ALL] != kI) Err("one image, no flush, the CS thread ahead (ALL): the blit showed %lX, not the presented %lX", byMode[RING_CS_ALL], kI);
    RingSetTag(RingCurrent(), kI);
    Present();
    B::Flush();                                             // the post-present flush
    RawCopyFrom(FG_IMG_SAVE, B::BBImage());
    B::Flush();
    const DWORD safe = LastPresented().tag;
    if (safe != kI) Err("one image, with the flush: the blit showed %lX, not the presented %lX", safe, kI);
    if (CurTag() != kSaved) Err("one image, with the flush: the restore did not put the saved image back (%lX)", CurTag());
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    RingSetTag(RingCurrent(), kI);
    Present();
    RawCopyFrom(FG_IMG_SAVE, B::BBImage());
    B::Flush();
    const DWORD three = LastPresented().tag;
    if (three != kI) Err("three images, no flush: the blit showed %lX, not the presented %lX", three, kI);
    printf("  one-image hazard: 1 image with no post-present flush presented %lX (LAZY) %lX (PRE) %lX (ALL, the CS thread ahead), with the flush %lX; 3 images with no flush %lX\n",
        hazard, byMode[RING_CS_PRE], byMode[RING_CS_ALL], safe, three);
    gCases++;
}

// ---------------------------------------------------------------- 4. the stream order (H1, the 2026-09-07 mechanism)
// WHAT IT SHOWS. A raw copy puts a lit image in the back buffer; another D3D9 call fills it black before the Present. A
// D3D9 sample recorded after the fill reads BLACK, as the blit does, because both are in DXVK's stream behind the fill.
// A raw probe made at the same point (a copy of the back buffer and a raw sample of it) reads LIT, because the fill has
// not reached the queue. THE CONTROL: a D3D9 sample recorded BEFORE the fill reads lit.
static void CaseStreamOrder()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE); DrainSamples();
    RingSetTag(gRing.rigImg[FG_IMG_PRESENT], 0xF1u);
    RawCopyFrom(FG_IMG_PRESENT, B::BBImage());
    B::D3D9Fill(FG_SURF_BB);
    const ULONGLONG hq = B::BBImage();                      // the query is a D3D9 call: before the queue, as the machine makes it
    B::CbTake(1); B::QLock(); B::RecBegin(); B::RecCopyToImg(hq, FG_IMG_HUDLESS); B::RecSampleRaw(FG_IMG_HUDLESS, 1, 0, FG_SMP_GEN); B::RecSubmit(); B::QUnlock();
    B::D3D9Sample(FG_SMP_GEN, 2, 0);
    Present();
    B::Flush();
    RawEmpty();
    int k[2], c[2]; LONG p[2], f[2]; bool got[2];
    for (int i = 0; i < 2; i++) got[i] = B::SampleCollect(&k[i], &p[i], &f[i], &c[i]);
    if (!got[0] || p[0] != 1 || c[0] != FG_CLS_LIT) Err("stream order: the raw pre-Present probe did not read LIT first (got %d, p %ld, class %d)", got[0], p[0], c[0]);
    if (!got[1] || p[1] != 2 || c[1] != FG_CLS_BLACK) Err("stream order: the CS-ordered sample did not read BLACK (got %d, p %ld, class %d)", got[1], p[1], c[1]);
    if (LastPresented().tag != kBlack) Err("stream order: the blit showed %lX, not the black fill", LastPresented().tag);
    // the control: the sample before the fill
    RawCopyFrom(FG_IMG_PRESENT, B::BBImage());
    B::D3D9Sample(FG_SMP_GEN, 3, 0);
    B::D3D9Fill(FG_SURF_BB);
    Present(); B::Flush(); RawEmpty();
    int kk, cc; LONG pp, ff;
    if (!B::SampleCollect(&kk, &pp, &ff, &cc) || pp != 3 || cc != FG_CLS_LIT) Err("stream order control: a sample recorded before the fill did not read LIT (p %ld, class %d)", pp, cc);
    printf("  stream order: raw probe before the Present read class %d (LIT), the CS-ordered sample class %d (BLACK), the blit %lX; a sample before the fill read class %d\n", c[0], c[1], LastPresented().tag, cc);
    gCases++;
}

// ---------------------------------------------------------------- 5. the locks
// WHAT IT SHOWS. (a) The interop lock keeps another thread's D3D9 call out until UnlockDevice. (b) A SECOND LockDevice by
// its owner RELEASES it (d3d9_interop.cpp:248-250): the other thread's call gets through while the owner still thinks it
// holds the lock. (c) THE NEGATIVE: a recursive lock taken twice keeps the other thread out, so a recursive fake would have
// passed a nesting machine. (d) The owner's own D3D9 calls, a flush and a Present inside the lock take empty guards and
// release nothing (d3d9_multithread.h:76-77). (e) A Present held in its SyncFrameLatency gate holds the device lock.
static volatile LONG gCsDone = 0; static CRITICAL_SECTION gRec;
static DWORD WINAPI ForeignRecursive(void*) { EnterCriticalSection(&gRec); LeaveCriticalSection(&gRec); _InterlockedExchange(&gCsDone, 1); return 0; }
static DWORD WINAPI PresentThread(void*) { Present(); return 0; }
// The lock table's probes, (f) below: one call each from another thread. The last is the control that takes no lock.
typedef HRESULT(__stdcall* tStretch_t)(IDirect3DDevice9*, IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const RECT*, D3DTEXTUREFILTERTYPE);
typedef HRESULT(__stdcall* tClear_t)(IDirect3DDevice9*, DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD);
typedef HRESULT(__stdcall* tDraw_t)(IDirect3DDevice9*, D3DPRIMITIVETYPE, UINT, UINT);
typedef HRESULT(__stdcall* tReset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef UINT(__stdcall* tTexMem_t)(IDirect3DDevice9*);
struct LockProbe { int what; volatile LONG done; };
static LockProbe gProbe[8];
static const char* const kProbeName[8] = { "StretchRect", "Clear", "DrawPrimitive", "the swapchain's GetBackBuffer", "FlushRenderingCommands",
                                           "the handle query", "Reset", "GetAvailableTextureMem (the unlocked control)" };
enum { kProbeNoLock = 7 };
static D3DPRESENT_PARAMETERS ResetParams(D3DSWAPEFFECT e, UINT bbc, BOOL w)
{ D3DPRESENT_PARAMETERS p = {}; p.BackBufferWidth = 1920; p.BackBufferHeight = 1080; p.BackBufferFormat = D3DFMT_X8R8G8B8; p.BackBufferCount = bbc; p.SwapEffect = e; p.Windowed = w; return p; }
static DWORD WINAPI LockProbeThread(void* a)
{
    LockProbe* p = (LockProbe*)a;
    IDirect3DSurface9* s = nullptr;
    D3DPRESENT_PARAMETERS pp = ResetParams(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    switch (p->what)
    {
    case 0: ((tStretch_t)dtDx[34])(D, gOtherRt, nullptr, gGameRt, nullptr, D3DTEXF_NONE); break;
    case 1: ((tClear_t)dtDx[43])(D, 0, nullptr, D3DCLEAR_TARGET, 0x4343u, 1.0f, 0); break;
    case 2: ((tDraw_t)dtDx[81])(D, D3DPT_TRIANGLELIST, 0, 1); break;
    case 3: if (SUCCEEDED(SC->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &s)) && s) s->Release(); break;
    case 4: B::Flush(); break;
    case 5: (void)B::BBImage(); break;
    case 6: ((tReset_t)dtDx[16])(D, &pp); break;
    case 7: (void)((tTexMem_t)dtDx[4])(D); break;
    }
    _InterlockedExchange(&p->done, 1);
    return 0;
}
static void CaseLocks()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    // (a)
    gForeignDone = 0; B::DevLock();
    HANDLE h = Spawn(ForeignFill); Sleep(60);
    const bool heldA = !gForeignDone;
    B::DevUnlock();
    const bool doneA = WaitFlag(&gForeignDone, 2000); Join(h);
    if (!heldA) Err("locks (a): another thread's D3D9 call got through the interop lock");
    if (!doneA) Err("locks (a): another thread's D3D9 call never got through after UnlockDevice");
    // (b)
    const LONG nest0 = gRingSt.interopNestRelease;
    gForeignDone = 0; B::DevLock();
    h = Spawn(ForeignFill); Sleep(60);
    const bool heldB = !gForeignDone;
    B::DevLock();                                           // the nested take
    const bool doneB = WaitFlag(&gForeignDone, 2000);
    B::DevUnlock(); Join(h);
    if (!heldB) Err("locks (b): the first take did not keep the other thread out");
    if (!doneB) Err("locks (b): a nested LockDevice by the owner did NOT release the lock; DXVK's single guard slot does");
    if (gRingSt.interopNestRelease != nest0 + 1) Err("locks (b): the nested release was counted %ld times, not once", gRingSt.interopNestRelease - nest0);
    if (gRingOwner != 0) Err("locks (b): the device lock is still owned by %ld after the owner's UnlockDevice", gRingOwner);
    // (c) the negative
    InitializeCriticalSection(&gRec); gCsDone = 0;
    EnterCriticalSection(&gRec); EnterCriticalSection(&gRec);
    h = Spawn(ForeignRecursive); Sleep(100);
    const bool heldC = !gCsDone;
    LeaveCriticalSection(&gRec); LeaveCriticalSection(&gRec); Join(h);
    if (!heldC) Err("locks (c): a recursive lock taken twice let the other thread in; the control shows nothing");
    // (d)
    const LONG nest1 = gRingSt.interopNestRelease;
    B::DevLock();
    IDirect3DSurface9* bb = B::BbSurface(); DevFill(bb, 0x0D0Du); bb->Release();
    B::D3D9Stretch(FG_SURF_BB, FG_SURF_HOLD); B::Flush(); Present(); B::Flush();
    gForeignDone = 0; h = Spawn(ForeignFill); Sleep(60);
    const bool heldD = !gForeignDone;
    B::DevUnlock();
    const bool doneD = WaitFlag(&gForeignDone, 2000); Join(h);
    if (!heldD) Err("locks (d): the owner's own D3D9 calls, flush or Present released the interop lock");
    if (!doneD) Err("locks (d): the other thread never got through");
    if (gRingSt.interopNestRelease != nest1) Err("locks (d): the owner's internal re-entry was counted as a nested take");
    if (RingTag(RingImgOfSurf(gRing.rigSurf[FG_SURF_HOLD])) != 0x0D0Du) Err("locks (d): the StretchRect into HOLD under the lock did not copy the back buffer");
    // (e)
    RingGateHoldNext();
    HANDLE hp = Spawn(PresentThread);
    long spin = 0; while (!RingGateHeld() && spin++ < 2000) Sleep(1);
    const bool inGate = RingGateHeld();
    gForeignDone = 0; h = Spawn(ForeignFill); Sleep(60);
    const bool heldE = !gForeignDone;
    RingGateRelease();
    const bool doneE = WaitFlag(&gForeignDone, 2000); Join(h); Join(hp);
    if (!inGate) Err("locks (e): the Present never reached the gate");
    if (!heldE) Err("locks (e): another thread's D3D9 call got through a Present held in its gate");
    if (!doneE) Err("locks (e): the other thread never got through after the gate opened");
    // (f) THE LOCK TABLE, BOTH SIDES (added 2026-09-25 after the verifier's review: only ColorFill was ever tried, so a
    // fake whose other slots took no lock passed). Each probe below is a call DXVK makes under its device lock, and each
    // must wait while the interop lock is held: StretchRect (d3d9_device.cpp:1219), Clear (1905), DrawPrimitive (3027), the
    // swapchain's GetBackBuffer (d3d9_swapchain.cpp:478), FlushRenderingCommands (Flush, d3d9_device.cpp:6177), the
    // mod's handle query (its GetBackBuffer), and Reset (449). THE CONTROL: GetAvailableTextureMem takes no lock (237-250,
    // RingSlotLocks), so it must get through at once; a fake that locked every slot fails here.
    const int kProbes = 8;
    for (int i = 0; i < kProbes; i++) gProbe[i] = { i, 0 };
    B::DevLock();
    HANDLE hs[kProbes];
    for (int i = 0; i < kProbes; i++) hs[i] = Spawn(LockProbeThread, &gProbe[i]);
    Sleep(100);
    bool heldF[kProbes];
    for (int i = 0; i < kProbes; i++) heldF[i] = !gProbe[i].done;
    B::DevUnlock();
    bool doneF = true;
    for (int i = 0; i < kProbes; i++) { if (!WaitFlag(&gProbe[i].done, 2000)) doneF = false; Join(hs[i], kProbeName[i]); }
    int blockedF = 0;
    for (int i = 0; i < kProbes; i++)
    {
        const bool shouldWait = i != kProbeNoLock;
        if (heldF[i]) blockedF++;
        if (heldF[i] != shouldWait) Err("locks (f): %s %s while the interop lock was held", kProbeName[i], shouldWait ? "got through" : "waited");
    }
    if (!doneF) Err("locks (f): a probe never finished after UnlockDevice");
    B::Flush();
    printf("  locks: exclusion %s, nested take released %s (counted %ld), recursive control kept out %s, owner re-entry kept it %s, gate held it %s;"
           " the table: %d of %d locking calls waited, the unlocked control %s\n",
        heldA && doneA ? "yes" : "NO", doneB ? "yes" : "NO", gRingSt.interopNestRelease - nest0, heldC ? "yes" : "NO", heldD ? "yes" : "NO", heldE ? "yes" : "NO",
        blockedF - (heldF[kProbeNoLock] ? 1 : 0), kProbes - 1, heldF[kProbeNoLock] ? "WAITED" : "went through");
    gRingSt.interopNestRelease = nest0;
    gCases++;
}

// ---------------------------------------------------------------- 6. the submission queue
// WHAT IT SHOWS. A D3D9 call by the thread holding the queue is counted (the rule the harness asserts is 0); so is the
// handle query, which is the mod's GetBackBuffer, QueryInterface, GetVulkanImageInfo and Release (revised 2026-09-25: it
// was exempt, so a query moved inside the queue would have passed); another thread's D3D9 call while it is held is allowed
// and counted apart; a D3D9 call after the release is not counted. The other counters: a raw submit without the queue, a
// second take of the queue by its holder.
static void CaseSubmissionQueue()
{
    const RingStats s0 = gRingSt;
    B::QLock();
    (void)B::BBImage();
    const LONG afterQuery = gRingSt.qD3D9;
    DevFill(gOtherRt, 1u);
    const LONG afterCall = gRingSt.qD3D9;
    B::QUnlock();
    DevFill(gOtherRt, 2u);
    const LONG afterRelease = gRingSt.qD3D9;
    B::QLock();
    gForeignDone = 0; HANDLE h = Spawn(ForeignFill);
    const bool foreignDone = WaitFlag(&gForeignDone, 2000);
    B::QUnlock(); Join(h);
    B::CbTake(1); B::RecBegin(); B::RecSubmit();
    B::QLock(); B::QLock(); B::QUnlock(); B::QUnlock();
    if (afterQuery != s0.qD3D9 + 1) Err("queue: the handle query under the queue was not counted as a D3D9 call");
    if (afterCall != afterQuery + 1) Err("queue: a D3D9 call under the queue by its holder was not counted");
    if (afterRelease != afterCall) Err("queue: a D3D9 call after the release was counted");
    if (!foreignDone || gRingSt.qD3D9Foreign != s0.qD3D9Foreign + 1 || gRingSt.qD3D9 != afterCall) Err("queue: another thread's D3D9 call under the queue was not allowed, or not counted apart");
    if (gRingSt.rawNoQLock != s0.rawNoQLock + 1) Err("queue: a raw submit without the queue was not counted");
    if (gRingSt.qRecursive != s0.qRecursive + 1) Err("queue: a second take by its holder was not counted");
    printf("  submission queue: holder's D3D9 call counted %ld, the handle query %ld, after release 0, another thread's allowed (%ld apart); raw submit without it %ld, second take %ld\n",
        afterCall - afterQuery, afterQuery - s0.qD3D9, gRingSt.qD3D9Foreign - s0.qD3D9Foreign, gRingSt.rawNoQLock - s0.rawNoQLock, gRingSt.qRecursive - s0.qRecursive);
    gRingSt.qD3D9 = s0.qD3D9; gRingSt.qD3D9Foreign = s0.qD3D9Foreign; gRingSt.rawNoQLock = s0.rawNoQLock; gRingSt.qRecursive = s0.qRecursive;
    gCases++;
}

// ---------------------------------------------------------------- 7. the timelines
// WHAT IT SHOWS. Two threads waiting on one timeline are both woken by one submit's signal, and not before it; a wait for
// a value never signalled times out; LatEnd wakes an END waiter and grows the lock ticks, and drops a value at or below
// the last; a backwards signal is counted.
struct TlWaiter { int tl; ULONGLONG v; DWORD ms; volatile LONG done; bool ok; LONGLONG t0, t1; };
static volatile DWORD gSigSaw = 0;
static DWORD WINAPI TlWaitThread(void* a) { TlWaiter* w = (TlWaiter*)a; w->t0 = B::Now(); w->ok = B::TlWait(w->tl, w->v, w->ms); w->t1 = B::Now(); _InterlockedExchange(&w->done, 1); return 0; }
static void CaseTimelines()
{
    const RingStats s0 = gRingSt;
    const LONG base = (LONG)B::TlValue(FG_TL_MARKI);
    TlWaiter a = { FG_TL_MARKI, (ULONGLONG)(base + 7), 3000 }, b = a;
    HANDLE ha = Spawn(TlWaitThread, &a), hb = Spawn(TlWaitThread, &b);
    Sleep(40);
    const bool early = a.done || b.done;
    RawSignal(FG_TL_MARKI, base + 7);
    const bool woke = WaitFlag(&a.done, 1000) && WaitFlag(&b.done, 1000);
    Join(ha); Join(hb);
    TlWaiter c = { FG_TL_MARKI, (ULONGLONG)(base + 8), 60 };
    HANDLE hc = Spawn(TlWaitThread, &c); Join(hc);
    if (early) Err("timelines: a waiter woke before the signal");
    if (!woke || !a.ok || !b.ok) Err("timelines: the signal did not wake both waiters");
    if (c.ok || Ms(c.t1 - c.t0) < 45.0) Err("timelines: a wait for a value never signalled returned %d after %.1f ms", c.ok, Ms(c.t1 - c.t0));
    if (B::TlValue(FG_TL_MARKI) != (ULONGLONG)(base + 7)) Err("timelines: TlValue is %llu, not %ld", B::TlValue(FG_TL_MARKI), base + 7);
    const LONG f0 = (LONG)B::TlValue(FG_TL_END) + 5;
    TlWaiter e = { FG_TL_END, (ULONGLONG)f0, 3000 };
    HANDLE he = Spawn(TlWaitThread, &e); Sleep(20);
    B::LatEnd(f0);
    Join(he);
    if (!e.ok) Err("timelines: LatEnd did not wake the END waiter");
    B::LatEnd(f0); B::LatEnd(f0 - 1);
    if (gRingSt.latEndDropped != s0.latEndDropped + 2) Err("timelines: LatEnd at or below the last was dropped %ld times, not 2", gRingSt.latEndDropped - s0.latEndDropped);
    RawSignal(FG_TL_MARKI, base + 3);
    if (gRingSt.tlBackwards != s0.tlBackwards + 1) Err("timelines: a backwards signal was not counted");
    // A SUBMIT SIGNALS AFTER ITS COPIES (added 2026-09-25): FG_TL_IN means "the HUD-less copy has retired" (fg_machine.h's
    // SpanHudless records the copy, then the signal), so whoever the signal wakes must find the copy done. The one-shot
    // hook runs inside the signal, before the value moves, and reads the copy's destination.
    RingSetTag(gRing.rigImg[FG_IMG_GEN], 0x2401u); RingSetTag(gRing.rigImg[FG_IMG_HUDLESS], 0x2400u);
    gSigSaw = 0;
    gRing.signalInside = [](int, ULONGLONG) { gSigSaw = RingTag(gRing.rigImg[FG_IMG_HUDLESS]); };
    const LONG vL = (LONG)B::TlValue(FG_TL_LOCAL) + 1;
    B::CbTake(1); B::QLock(); B::RecBegin(); B::RecCopyImg(FG_IMG_GEN, FG_IMG_HUDLESS); B::RecSignal(FG_TL_LOCAL, vL); B::RecSubmit(); B::QUnlock();
    if (gRing.signalInside) { Err("timelines: the submit's signal never ran the hook"); gRing.signalInside = nullptr; }
    if (gSigSaw != 0x2401u) Err("timelines: at the signal the copy's destination held %lX, not the copied %X: the signal came before the copy", gSigSaw, 0x2401u);
    printf("  timelines: two waiters woken by one signal %s, an unsignalled wait timed out after %.1f ms, LatEnd woke the END waiter %s, 2 LatEnds dropped, 1 backwards signal counted;"
           " a signal saw its buffer's copy done %s\n",
        woke ? "yes" : "NO", Ms(c.t1 - c.t0), e.ok ? "yes" : "NO", gSigSaw == 0x2401u ? "yes" : "NO");
    gRingSt.latEndDropped = s0.latEndDropped; gRingSt.tlBackwards = s0.tlBackwards;
    gCases++;
}

// ---------------------------------------------------------------- 8. SampleCollect's order
// WHAT IT SHOWS. A sample comes back when its readback retired and not before: a raw one at its own submit; a D3D9 one only
// after its StretchRect ran (a flush) AND a later rig submit carried the readback (map 2.8). Completed samples come back in
// the order they completed. THE NEGATIVE: after the flush alone nothing is collectable.
static void CaseSampleOrder()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE); DrainSamples();
    int k, c; LONG p, f;
    B::D3D9Sample(FG_SMP_REAL, 10, 0);
    B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSampleRaw(FG_IMG_PRESENT, 11, 0, FG_SMP_HAND); B::RecSubmit(); B::QUnlock();
    const bool g1 = B::SampleCollect(&k, &p, &f, &c); const LONG p1 = p;
    const bool g1b = B::SampleCollect(&k, &p, &f, &c);
    B::Flush();
    const bool g2 = B::SampleCollect(&k, &p, &f, &c);
    RawEmpty();
    const bool g3 = B::SampleCollect(&k, &p, &f, &c); const LONG p3 = p;
    B::D3D9Sample(FG_SMP_REAL, 12, 0); B::D3D9Sample(FG_SMP_REAL, 13, 0);
    B::Flush(); RawEmpty();
    LONG q[3] = { -1, -1, -1 }; int n = 0;
    while (n < 3 && B::SampleCollect(&k, &p, &f, &c)) q[n++] = p;
    if (!g1 || p1 != 11 || g1b) Err("samples: after the raw submit exactly the raw sample (11) should be back (got %d p %ld, then %d)", g1, p1, g1b);
    if (g2) Err("samples: a D3D9 sample came back after its flush with no rig submit to read it");
    if (!g3 || p3 != 10) Err("samples: the D3D9 sample (10) did not come back at the next rig submit");
    if (n != 2 || q[0] != 12 || q[1] != 13) Err("samples: two D3D9 samples came back as %ld %ld (%d of them), not 12 13", q[0], q[1], n);
    // A SAMPLE RECORDED BEFORE A PRESENT (added 2026-09-25 after the verifier's review): the Present's own flush does not
    // wait for the CS thread (the head of dt_fake_ring.h), so by default (RING_CS_LAZY) the StretchRect has not reached
    // the queue when Present returns, and a rig submit right after the Present cannot read it back; it can after an
    // interop flush (map 2.8: the readback follows a flush). With RING_CS_PRE the CS thread was ahead and it can.
    DrainSamples();
    B::D3D9Sample(FG_SMP_REAL, 20, 0); Present(); RawEmpty();
    const bool lazyBack = B::SampleCollect(&k, &p, &f, &c);
    B::Flush(); RawEmpty();
    const bool lazyAfter = B::SampleCollect(&k, &p, &f, &c); const LONG p20 = p;
    const int cs0 = gRing.presentCs;
    gRing.presentCs = RING_CS_PRE;
    B::D3D9Sample(FG_SMP_REAL, 21, 0); Present(); RawEmpty();
    const bool preBack = B::SampleCollect(&k, &p, &f, &c); const LONG p21 = p;
    gRing.presentCs = cs0;
    B::Flush(); DrainSamples();
    if (lazyBack) Err("samples: by default a D3D9 sample recorded before a Present came back at a submit after it with no flush");
    if (!lazyAfter || p20 != 20) Err("samples: the pre-Present sample (20) did not come back after a flush and a submit");
    if (!preBack || p21 != 21) Err("samples: with the CS thread ahead (PRE) the pre-Present sample (21) did not come back at the next submit");
    printf("  samples: raw back at its submit (%ld), D3D9 not back after its flush alone (%s), back at the next submit (%ld), then %ld %ld in order;"
           " one recorded before a Present: back at the next submit %s (LAZY), %s (PRE), after a flush %s\n",
        p1, g2 ? "BACK" : "none", p3, q[0], q[1], lazyBack ? "YES" : "no", preBack ? "yes" : "NO", lazyAfter ? "yes" : "NO");
    gCases++;
}

// ---------------------------------------------------------------- 9. the binding's other seams
// WHAT IT SHOWS. The pipe log records each write and its thread, and a write entered inside another is counted; PaceWait
// sleeps to its due on the real timer and returns 1 when the pace event wakes it early; a command buffer taken under the
// interop lock and a begin with none taken are counted, and CbTake can be made to refuse; S() is VirtualAlloc'd and
// attached; a chained observer is called once per call.
static volatile LONG gObsCalls = 0;
static void CountObs(LONG) { _InterlockedIncrement(&gObsCalls); }
static DWORD WINAPI WakePaceLater(void*) { Sleep(10); B::WakePace(); return 0; }
static void CaseSeams()
{
    const RingStats s0 = gRingSt;
    RingClearLogs();
    B::WriteQ(1); B::WriteG(1, 0);
    gRing.pipeInside = [] { B::WriteG(99, 0); };
    B::WriteQ(2);
    const auto pipe = RingPipeLog();
    if (pipe.size() != 4 || pipe[0].type != 'Q' || pipe[1].type != 'G' || pipe[0].tid != GetCurrentThreadId()) Err("seams: the pipe log is wrong (%zu writes)", pipe.size());
    if (gRingSt.pipeReentry != s0.pipeReentry + 1) Err("seams: a write inside a write was not counted");
    LONGLONG t0 = B::Now();
    const int r0 = B::PaceWait(t0 + B::Qpf() / 200);        // 5 ms
    const double slept = Ms(B::Now() - t0);
    HANDLE h = Spawn(WakePaceLater);
    t0 = B::Now();
    const int r1 = B::PaceWait(t0 + B::Qpf() / 2);          // 500 ms, woken at about 10
    const double woken = Ms(B::Now() - t0);
    Join(h);
    if (r0 != 0 || slept < 4.0 || slept > 60.0) Err("seams: PaceWait to a 5 ms due returned %d after %.2f ms", r0, slept);
    if (r1 != 1 || woken > 250.0) Err("seams: PaceWait woken by the pace event returned %d after %.2f ms", r1, woken);
    // THE COMMAND BUFFERS (revised 2026-09-25 after the verifier's review: the check went blind after the first span that
    // took more than it began, and this case only moved it by zeroing the reservation by hand). A reservation now belongs to
    // its span. (1) Taken under the lock: counted, and dropped at the unlock, so the begin after it has nothing taken.
    // (2) The machine's Z N fire: 3 taken, 2 begun, the lock released; the spare is dropped (a fact, cbUnused), so a later
    // begin with nothing taken IS counted. (3) A span with no lock: 2 taken, 1 begun, then a new CbTake(1) replaces what is
    // left; that one begin is covered, a second is not (a begin must use up the reservation, not merely find one).
    B::DevLock(); B::CbTake(1); B::DevUnlock();
    B::QLock(); B::RecBegin(); B::RecSubmit(); B::QUnlock();
    const LONG untaken1 = gRingSt.cbUntaken - s0.cbUntaken;
    B::CbTake(3); B::DevLock(); B::QLock(); B::RecBegin(); B::RecSubmit(); B::RecBegin(); B::RecSubmit(); B::QUnlock(); B::DevUnlock();
    const LONG spare2 = gRingSt.cbUnused - s0.cbUnused - 1;
    B::QLock(); B::RecBegin(); B::RecSubmit(); B::QUnlock();
    const LONG untaken2 = gRingSt.cbUntaken - s0.cbUntaken - untaken1;
    B::CbTake(2); B::QLock(); B::RecBegin(); B::RecSubmit(); B::QUnlock();
    B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSubmit(); B::RecBegin(); B::RecSubmit(); B::QUnlock();
    const LONG untaken3 = gRingSt.cbUntaken - s0.cbUntaken - untaken1 - untaken2;
    const LONG unused = gRingSt.cbUnused - s0.cbUnused;
    gRing.cbFailNext = 1; const bool refused = !B::CbTake(1);
    if (gRingSt.cbUnderLock != s0.cbUnderLock + 1) Err("seams: a command buffer taken under the interop lock was not counted");
    if (untaken1 != 1) Err("seams: a begin after the span that took its buffer under the lock was counted %ld times, not once", untaken1);
    if (spare2 != 1 || untaken2 != 1) Err("seams: after a span that took 3 and began 2, the spare counted %ld (not 1) and an untaken begin %ld (not 1)", spare2, untaken2);
    if (untaken3 != 1) Err("seams: a span that took 1 and began 2 counted %ld untaken begins, not 1", untaken3);
    if (unused != 3) Err("seams: %ld unused command buffers counted, not 3 (1 under the lock, 1 of the 3, 1 of the 2)", unused);
    if (!refused) Err("seams: CbTake did not refuse when told to");
    MEMORY_BASIC_INFORMATION mbi = {};
    FgState& st = B::S();
    VirtualQuery(st.pub, &mbi, sizeof(mbi));
    if (!st.pub || mbi.Type != MEM_PRIVATE || st.pub[0].seq != -1 || st.w.openP != -1) Err("seams: S() is not attached to VirtualAlloc'd rings");
    B::prevPresentObs = CountObs; gObsCalls = 0;
    B::PrevPresentObs(3);
    B::prevPresentObs = nullptr; B::PrevPresentObs(4);
    if (gObsCalls != 1 || B::nPrevPresent < 2) Err("seams: the chained present observer was called %ld times, not once", gObsCalls);
    if (B::OrigSwapPresentSlot() != (void*)&dtScPresent) Err("seams: origSwapPresent does not start as dtScPresent");
    if (B::RenderTid() != renderTid) Err("seams: RenderTid is not renderTid");
    printf("  seams: pipe %zu writes, 1 re-entry counted; PaceWait slept %.2f ms to a 5 ms due and woke at %.2f ms on the event; CbTake under the lock"
           " counted; untaken begins %ld %ld %ld (after the locked take, after a 3-for-2 span, a second begin on 1), %ld spares dropped; refusal %s; S() VirtualAlloc'd\n",
           pipe.size(), slept, woken, untaken1, untaken2, untaken3, unused, refused ? "yes" : "NO");
    gRingSt.pipeReentry = s0.pipeReentry; gRingSt.cbUnderLock = s0.cbUnderLock; gRingSt.cbUntaken = s0.cbUntaken; gRingSt.cbUnused = s0.cbUnused;
    RingClearLogs();
    gCases++;
}

// ---------------------------------------------------------------- 10. a Reset (added 2026-09-25)
// WHAT IT SHOWS (the verifier's first must: the fake had no Reset, so the map's H9 and "a foreign Reset with N pending"
// could not fail). A Reset through the device, dtDx[16], as the layer's own Reset reaches DXVK: (a) the work already in
// the command stream is not dropped: a blit and a fill recorded before it run during it, on the images they took at their
// calls, so the presented log keeps the present; (b) every old back-buffer image is dead, and the new ones are black;
// (c) THE NEGATIVE, H9's failure: a raw restore into the handle queried before the Reset is counted (deadImage) and lands
// nowhere, while the same restore into a handle queried after it lands; (d) the parameters set the new ring's shape, a
// BackBufferCount of 0 counting as 1; (e) Reset takes the device lock (another thread's Reset waits for UnlockDevice).
static volatile LONG gResetDone = 0;
static DWORD WINAPI ResetThread(void*) { D3DPRESENT_PARAMETERS pp = ResetParams(D3DSWAPEFFECT_DISCARD, 2, TRUE); ((tReset_t)dtDx[16])(D, &pp); _InterlockedExchange(&gResetDone, 1); return 0; }
static void CaseReset()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    const RingStats s0 = gRingSt;
    int old[4]; const int nOld = gRing.n;
    for (int i = 0; i < nOld; i++) { old[i] = RingUnder(i); RingSetTag(old[i], 0x9100u + i); }
    RingSetTag(gRing.rigImg[FG_IMG_PRESENT], 0x9Au);
    // (a) a present and a fill left in the stream (RING_CS_LAZY, the default), then the Reset
    const int shownImg = RingCurrent();
    Present();
    const ULONGLONG hBefore = B::BBImage(); const int imgBefore = RingImgOfHandle(hBefore);
    IDirect3DSurface9* bb = B::BbSurface(); DevFill(bb, 0x9Fu); bb->Release();
    const size_t pending = RingCsPending();
    D3DPRESENT_PARAMETERS pp = ResetParams(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    const HRESULT hr = ((tReset_t)dtDx[16])(D, &pp);
    const auto log = RingPresentedLog();
    if (FAILED(hr)) Err("reset: the Reset failed (%08lX)", hr);
    if (pending != 2 || RingCsPending() != 0) Err("reset: %zu items were pending before it and %zu after, not 2 and 0", pending, RingCsPending());
    if (log.size() != 1 || log[0].tag != 0x9100u + (DWORD)(std::find(old, old + nOld, shownImg) - old)) Err("reset: the present recorded before the Reset did not reach the presented log with its image's tag");
    if (RingTag(imgBefore) != 0x9Fu) Err("reset: the fill recorded before the Reset did not land on the image it took at its call (%lX)", RingTag(imgBefore));
    // (b)
    int deadN = 0; for (int i = 0; i < nOld; i++) deadN += RingImgDead(old[i]) ? 1 : 0;
    bool freshBlack = true, freshNew = true;
    for (int i = 0; i < gRing.n; i++) { const int u = RingUnder(i); if (RingTag(u) != kBlack) freshBlack = false; if (std::find(old, old + nOld, u) != old + nOld || RingImgDead(u)) freshNew = false; }
    const ULONGLONG hAfter = B::BBImage();
    if (deadN != nOld) Err("reset: %d of the %d old images are dead", deadN, nOld);
    if (!freshNew || hAfter == hBefore) Err("reset: the ring after the Reset reuses an old image");
    if (!freshBlack) Err("reset: the new images are not black");
    // (c) the negative and its control
    RawCopyFrom(FG_IMG_PRESENT, hBefore);
    const LONG deadHit = gRingSt.deadImage - s0.deadImage;
    const DWORD oldKept = RingTag(imgBefore);
    RawCopyFrom(FG_IMG_PRESENT, hAfter);
    if (deadHit != 1) Err("reset: a raw restore into the handle from before the Reset was counted %ld times, not once", deadHit);
    if (oldKept != 0x9Fu) Err("reset: the restore into the dead image changed it (%lX)", oldKept);
    if (gRingSt.deadImage != s0.deadImage + 1 || RingTag(RingImgOfHandle(hAfter)) != 0x9Au) Err("reset: the restore into the handle queried after the Reset did not land, or was counted");
    if (gRingSt.csResolveDiffer != s0.csResolveDiffer) Err("reset: an item on a destroyed image was counted as a resolve difference");
    // (d) the shape from the parameters
    pp = ResetParams(D3DSWAPEFFECT_DISCARD, 1, TRUE); ((tReset_t)dtDx[16])(D, &pp); const int n1 = gRing.n;
    pp = ResetParams(D3DSWAPEFFECT_FLIP, 0, TRUE);    ((tReset_t)dtDx[16])(D, &pp); const int n0 = gRing.n; const UINT bbc0 = gRing.bbc;
    pp = ResetParams(D3DSWAPEFFECT_DISCARD, 2, TRUE); ((tReset_t)dtDx[16])(D, &pp); const int n2 = gRing.n;
    if (n1 != 1 || n0 != 2 || bbc0 != 1 || n2 != 3) Err("reset: the parameters made rings of %d, %d (count %u), %d images, not 1, 2 (1), 3", n1, n0, bbc0, n2);
    // (e) the lock
    gResetDone = 0; B::DevLock();
    HANDLE h = Spawn(ResetThread); Sleep(60);
    const bool held = !gResetDone;
    B::DevUnlock();
    const bool done = WaitFlag(&gResetDone, 2000); Join(h, "the Reset thread");
    if (!held) Err("reset: another thread's Reset got through the interop lock");
    if (!done) Err("reset: another thread's Reset never ran after UnlockDevice");
    printf("  reset: a present and a fill pending before it ran during it (%zu items, the blit showed %lX); %d old images dead, new ones black;"
           " a restore into the old handle counted %ld and landed nowhere, into the new one landed; shapes %d %d %d; a foreign Reset waited for the lock %s\n",
        pending, log.empty() ? 0 : log[0].tag, deadN, deadHit, n1, n0, n2, held ? "yes" : "NO");
    RingClearLogs();
    gRingSt.deadImage = s0.deadImage;
    gCases++;
}

// ---------------------------------------------------------------- 11. the sample classes (added 2026-09-25)
// WHAT IT SHOWS (the verifier's third should). A sample's class depends on its kind: the tag kinds read TAG or NOTAG and
// nothing else, the classifier kinds read TAG, BLACK, DARK or LIT. THE NEGATIVE the old fake failed: a tag-kind sample of a
// lit image reads NOTAG, not LIT. Both paths are tried, the raw one and a D3D9 one through the command stream.
static void CaseClasses()
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE); DrainSamples();
    const DWORD kDark = 0xFF010101u, kLit = 0x00123456u;
    const DWORD tags[4] = { kGen, kBlack, kDark, kLit };
    const int wantClassifier[4] = { FG_CLS_TAG, FG_CLS_BLACK, FG_CLS_DARK, FG_CLS_LIT };
    const int wantTag[4] = { FG_CLS_TAG, FG_CLS_NOTAG, FG_CLS_NOTAG, FG_CLS_NOTAG };
    int seen[FG_CLS_N] = {}; int wrong = 0;
    for (int t = 0; t < 4; t++)
        for (int kind = 0; kind < FG_SMP_N; kind++)
        {
            RingSetTag(gRing.rigImg[FG_IMG_PRESENT], tags[t]);
            B::CbTake(1); B::QLock(); B::RecBegin(); B::RecSampleRaw(FG_IMG_PRESENT, t * 10 + kind, 0, kind); B::RecSubmit(); B::QUnlock();
            int k, c; LONG p, f;
            if (!B::SampleCollect(&k, &p, &f, &c)) { Err("classes: a raw sample did not come back"); continue; }
            const bool tagKind = kind == FG_SMP_PRESENTED || kind == FG_SMP_RESTORE || kind == FG_SMP_STANDBY;
            const int want = tagKind ? wantTag[t] : wantClassifier[t];
            if (c >= 0 && c < FG_CLS_N) seen[c]++;
            if (c != want) { wrong++; Err("classes: kind %d on tag %lX read class %d, not %d", kind, tags[t], c, want); }
        }
    // a D3D9 PRESENTED sample of a lit back buffer: NOTAG (fault 16's hit is this reading)
    RawCopyFrom(FG_IMG_PRESENT, B::BBImage());              // FG_IMG_PRESENT still holds kLit
    B::D3D9Sample(FG_SMP_PRESENTED, 99, 0); B::Flush(); RawEmpty();
    int k, c = -1; LONG p = -1, f;
    const bool got = B::SampleCollect(&k, &p, &f, &c);
    if (!got || p != 99 || c != FG_CLS_NOTAG) Err("classes: a D3D9 PRESENTED sample of a lit image read class %d (p %ld), not NOTAG", c, p);
    if (!seen[FG_CLS_DARK] || !seen[FG_CLS_NOTAG]) Err("classes: DARK (%d) or NOTAG (%d) never occurred", seen[FG_CLS_DARK], seen[FG_CLS_NOTAG]);
    printf("  classes: %d raw samples, classes seen LIT %d BLACK %d DARK %d TAG %d NOTAG %d, %d wrong; a D3D9 PRESENTED sample of a lit image read %s\n",
        4 * FG_SMP_N, seen[FG_CLS_LIT], seen[FG_CLS_BLACK], seen[FG_CLS_DARK], seen[FG_CLS_TAG], seen[FG_CLS_NOTAG], wrong, c == FG_CLS_NOTAG ? "NOTAG" : "something else");
    gCases++;
}

// ---------------------------------------------------------------- 12. the game's calls through the real device thread
// WHAT IT SHOWS (requirement 9 of the fake). With the layer ARMED, the game's Clear, ColorFill and StretchRect are
// recorded by the producer and replayed by the worker through dtDx, reach FakeDev (its log says the worker made them) and
// write the ring's tags through the command stream. THE NEGATIVE: once the worker has replayed them (the drain) the tags
// are still not in the image until a flush runs the list. Then the rig's OrigSwapPresent records the Present through
// dtScPresent, the worker runs the ring's Present, and the blit reads the game's final tag on the worker thread.
static DWORD LastFkTid(int slot)
{
    EnterCriticalSection(&gFkLock);
    DWORD t = 0;
    for (size_t i = gFk.size(); i-- > 0;) if (gFk[i].slot == slot) { t = gFk[i].tid; break; }
    LeaveCriticalSection(&gFkLock);
    return t;
}
static void Drain() { D->TestCooperativeLevel(); }
static void CaseThroughTheDeviceThread(int cfg)
{
    Fresh(D3DSWAPEFFECT_DISCARD, 2, TRUE);
    for (int i = 0; i < 300 && !dtArmed; i++) SC->Present(nullptr, nullptr, nullptr, nullptr, 0);
    if (!dtArmed) { Err("device thread: the layer did not arm after 300 presents (threaded=%d)", cfg); return; }
    Drain(); B::Flush(); RingClearLogs();
    IDirect3DSurface9* bb = nullptr; D->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    if (bb != gRing.surf[0]) Err("device thread: GetBackBuffer(0) is not the ring's surface 0");
    D->Clear(0, nullptr, D3DCLEAR_TARGET, 0x7102u, 1.0f, 0);
    Drain();
    const DWORD beforeFlush = CurTag();
    B::Flush();
    const DWORD afterClear = CurTag();
    D->ColorFill(gGameRt, nullptr, 0x7101u);
    D->StretchRect(gGameRt, nullptr, bb, nullptr, D3DTEXF_NONE);
    Drain(); B::Flush();
    const DWORD afterStretch = CurTag();
    D->ColorFill(bb, nullptr, 0x7100u);
    Drain(); B::Flush();
    const DWORD afterFill = CurTag();
    const DWORD tid43 = LastFkTid(43), tid34 = LastFkTid(34), tid35 = LastFkTid(35);
    const LONG rec0 = dtPresRecN;
    B::OrigSwapPresent(SC, nullptr, nullptr, nullptr, nullptr, 0);
    const LONG recorded = dtPresRecN - rec0;
    Drain(); B::Flush();
    const RingPresented shown = LastPresented();
    if (bb) bb->Release();
    if (beforeFlush == 0x7102u) Err("device thread: the replayed Clear reached the image before any flush ran the list");
    if (afterClear != 0x7102u) Err("device thread: the replayed Clear did not reach the current image (%lX)", afterClear);
    if (afterStretch != 0x7101u) Err("device thread: the replayed ColorFill and StretchRect did not bring the game target's tag (%lX)", afterStretch);
    if (afterFill != 0x7100u) Err("device thread: the replayed ColorFill of the back buffer did not land (%lX)", afterFill);
    if (tid43 != dtWorkerTid || tid34 != dtWorkerTid || tid35 != dtWorkerTid) Err("device thread: FakeDev got the calls from threads %lu %lu %lu, not the worker %lu", tid43, tid34, tid35, (DWORD)dtWorkerTid);
    if (recorded != 1) Err("device thread: OrigSwapPresent recorded %ld presents, not 1", recorded);
    if (shown.tag != 0x7100u || shown.tid != dtWorkerTid) Err("device thread: the blit showed %lX from thread %lu, not %X from the worker %lu", shown.tag, shown.tid, 0x7100u, (DWORD)dtWorkerTid);
    if (CurTag() == 0x7100u) Err("device thread: the ring did not rotate at the worker's Present");
    printf("  device thread (threaded=%d, armed): replayed Clear %lX (not before a flush: %lX), StretchRect %lX, ColorFill %lX, all from the worker;"
           " OrigSwapPresent recorded %ld and the worker's blit showed %lX\n", cfg, afterClear, beforeFlush, afterStretch, afterFill, recorded, shown.tag);
    gCases++;
}

int main(int argc, char** argv)
{
    const int cfg = argc > 1 ? atoi(argv[1]) : 2;
    setvbuf(stdout, nullptr, _IONBF, 0);
    printf("dt_fake_ring_test: threaded=%d, the fake of DXVK's ring, stream, queue and locks\n", cfg);
    InitializeCriticalSection(&gFkLock);
    renderTid = GetCurrentThreadId();
    gFd = new FakeDev; gSc = new FakeRingSC; gFd->sc = gSc; gFd->Defaults(); gDev = gFd; D = gFd; SC = gSc;
    RingInstall(gFd, gSc);
    RingDeclareTags(kGen, kBlack);
    if (!DtStart(256u << 10)) { printf("DtStart refused\nFAILED\n"); return 1; }
    void** vt = *(void***)gFd;
    memcpy(dtDx, vt, sizeof(dtDx));
    static const DWORD kVs[] = { 0xFFFE0300u, 0x0000FFFFu }, kPs[] = { 0xFFFF0300u, 0x0000FFFFu };
    if (!DtInstall(gFd, vt, cfg, kVs, kPs) || !dtArmable) { printf("DtInstall refused: %s\nFAILED\n", dtWhyNot); return 1; }
    if (!RingWrapDevice()) { printf("RingWrapDevice refused\nFAILED\n"); return 1; }
    FgHarnessInit();
    D->CreateRenderTarget(64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &gOtherRt, nullptr);
    D->CreateRenderTarget(1920, 1080, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &gGameRt, nullptr);

    CaseRotation();
    CaseSurfaceAgainstHandle();
    CaseOneImageHazard();
    CaseStreamOrder();
    CaseLocks();
    CaseSubmissionQueue();
    CaseTimelines();
    CaseSampleOrder();
    CaseSeams();
    CaseReset();
    CaseClasses();
    CaseThroughTheDeviceThread(cfg);

    // EVERY VIOLATION COUNTER IS 0 NOW: the cases above made each one move on purpose and put it back, so anything left is
    // a fault the fake met that no case asked for. csResolveDiffer is NOT among them (revised 2026-09-25): it counts where
    // the map's reading of DXVK would have been wrong, which DXVK's own asynchronous flush makes happen, so it is a fact.
    const RingStats& s = gRingSt;
    const LONG v = s.qD3D9 + s.qRecursive + s.rawNoQLock + s.recNoBegin + s.cbUnderLock + s.cbUntaken + s.tlBackwards + s.pipeReentry + s.badHandle + s.badSurface + s.deadImage;
    printf("\n  %d cases. The fake: %ld presents (%ld failed), %ld blits run, %ld flushes (%ld at a Present), %ld raw submits, %ld raw and %ld D3D9 samples, %ld D3D9 calls,"
           " %ld resets, %ld contended lock takes, %ld items the map's reading would have put elsewhere; violations %ld (queue %ld, recursive %ld, unlocked submit %ld,"
           " no begin %ld, taken under lock %ld, untaken %ld, backwards %ld, pipe %ld, handle %ld, surface %ld, dead image %ld)\n",
        gCases, s.presents, s.presentsFailed, s.blitsRun, s.flushes, s.presentFlushes, s.submits, s.samplesRaw, s.samplesD3D9, s.d3d9Calls, s.resets, s.devContended,
        s.csResolveDiffer, v, s.qD3D9, s.qRecursive, s.rawNoQLock, s.recNoBegin, s.cbUnderLock, s.cbUntaken, s.tlBackwards, s.pipeReentry, s.badHandle, s.badSurface, s.deadImage);
    if (v) { printf("  a violation counter moved outside the cases that make it on purpose\n"); gErr++; }
    if (gCases != 12) { printf("  only %d of 12 cases ran to the end\n", gCases); gErr++; }
    const int fails = gErr;
    if (fails) printf("  FAILED BECAUSE: %d error(s) counted\n", fails);
    printf("%s\n", fails ? "FAILED" : "PASS");
    return fails ? 1 : 0;
}
