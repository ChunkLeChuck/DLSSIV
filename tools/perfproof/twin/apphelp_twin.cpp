// A STAND-IN FOR WINDOWS' COMPATIBILITY LAYER ON THE DEVICE (1.0.8, 2026-10-01). run_proof.py builds it as
// twin\apphelp.dll and dt_test's `twin` mode loads it by full path, so the device thread's install meets device functions
// owned by a module whose file name is apphelp.dll, as it does on a machine where Windows applied its 8 and 16-bit colour
// mitigation (DWM8And16BitMitigation). It does to the vtable what that layer does (its hooks in QueryInterface, Release,
// CreateAdditionalSwapChain and Reset, slots 0, 2, 13 and 16: read from the real layer by tools/apphelp-probe) and nothing of
// what the layer does inside them: each hook counts the call, counts one made on the device thread's worker, and passes on
// to what the slot held. Present (slot 17) is hooked on request, for the refusal the install must still make. Never shipped,
// never loaded by the mod.
#include <windows.h>

enum { TW_QI, TW_CASC, TW_RESET, TW_RELEASE, TW_PRESENT, TW_N };
static const int kSlot[TW_N] = { 0, 13, 16, 2, 17 };
static void*  gOrig[TW_N];
static void** gVt = nullptr;
static volatile LONG gCalls[TW_N], gOnWorker[TW_N];
static DWORD(__cdecl* gWorkerTid)() = nullptr;      // the harness's: the worker's thread id now, 0 with none

static void Note(int k)
{
    InterlockedIncrement(&gCalls[k]);
    const DWORD w = gWorkerTid ? gWorkerTid() : 0;
    if (w && GetCurrentThreadId() == w) InterlockedIncrement(&gOnWorker[k]);
}
typedef ULONG(__stdcall* TwF0)(void*);
typedef HRESULT(__stdcall* TwF1)(void*, void*);
typedef HRESULT(__stdcall* TwF2)(void*, void*, void*);
typedef HRESULT(__stdcall* TwF4)(void*, void*, void*, void*, void*);
static HRESULT __stdcall TwQueryInterface(void* d, void* iid, void* pp)          { Note(TW_QI); return ((TwF2)gOrig[TW_QI])(d, iid, pp); }
static HRESULT __stdcall TwCreateAdditionalSwapChain(void* d, void* p, void* sc)  { Note(TW_CASC); return ((TwF2)gOrig[TW_CASC])(d, p, sc); }
static HRESULT __stdcall TwReset(void* d, void* p)                                { Note(TW_RESET); return ((TwF1)gOrig[TW_RESET])(d, p); }
static ULONG   __stdcall TwRelease(void* d)                                       { Note(TW_RELEASE); return ((TwF0)gOrig[TW_RELEASE])(d); }
static HRESULT __stdcall TwPresent(void* d, void* a, void* b, void* c, void* e)   { Note(TW_PRESENT); return ((TwF4)gOrig[TW_PRESENT])(d, a, b, c, e); }
static void* const kHook[TW_N] = { (void*)&TwQueryInterface, (void*)&TwCreateAdditionalSwapChain, (void*)&TwReset, (void*)&TwRelease, (void*)&TwPresent };

static bool Put(int slot, void* fn)
{
    DWORD old;
    if (!VirtualProtect(&gVt[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &old)) return false;
    gVt[slot] = fn;
    VirtualProtect(&gVt[slot], sizeof(void*), old, &old);
    return gVt[slot] == fn;
}

extern "C" {
// Hooks the four slots (and Present with withPresent) of the device vtable; returns how many took.
__declspec(dllexport) int __cdecl TwinPatch(void** vt, int withPresent, DWORD(__cdecl* workerTid)())
{
    gVt = vt; gWorkerTid = workerTid;
    int n = 0;
    for (int k = 0; k < (withPresent ? TW_N : TW_N - 1); k++) { gOrig[k] = vt[kSlot[k]]; if (Put(kSlot[k], kHook[k])) n++; }
    return n;
}
// Takes the Present hook out again; true if the slot holds what it held before.
__declspec(dllexport) int __cdecl TwinUnpatchPresent()
{
    return gVt && gVt[17] == kHook[TW_PRESENT] && Put(17, gOrig[TW_PRESENT]);
}
// calls[5] and onWorker[5], in the order QueryInterface, CreateAdditionalSwapChain, Reset, Release, Present.
__declspec(dllexport) void __cdecl TwinRead(LONG* calls, LONG* onWorker)
{
    for (int k = 0; k < TW_N; k++) { calls[k] = gCalls[k]; onWorker[k] = gOnWorker[k]; }
}
// WHAT WINDOWS DOES WHEN A SECOND DEVICE IS MADE on a vtable every device shares (tools/apphelp-probe `second`, on DXVK):
// its four hooks written into the slots again, still passing on to what they held the FIRST time, so whatever was written
// over them since is gone around. Returns how many slots it wrote (a slot that still held its hook is not counted).
__declspec(dllexport) int __cdecl TwinRepatch()
{
    int n = 0;
    if (!gVt) return 0;
    for (int k = 0; k < TW_N - 1; k++) if (gVt[kSlot[k]] != kHook[k] && Put(kSlot[k], kHook[k])) n++;
    return n;
}
// Every hook out again: each slot that holds one of them gets back what it held first. For the copy that must be refused.
__declspec(dllexport) int __cdecl TwinUnpatchAll()
{
    int n = 0;
    if (!gVt) return 0;
    for (int k = 0; k < TW_N; k++) if (gVt[kSlot[k]] == kHook[k] && Put(kSlot[k], gOrig[k])) n++;
    return n;
}
}
