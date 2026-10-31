// X2'S STAND-IN FOR libxess.dll (2026-10-28): NOT INTEL'S FILE AND NOTHING OF IT. A small DLL of this tool's own, staged under
// Intel's file name by tools\test-xess\pipe.py so the helper's three refusals that this machine cannot produce with Intel's real
// file are each driven by the cause they name, through the helper's own probe (InitXess):
//   /DKIND_RUNTIME      imports a function from x2dep.dll, which is never staged: LoadLibraryEx fails with ERROR_MOD_NOT_FOUND
//                       (126), as Intel's file does without the Visual C++ runtime -> FEED_CAP_XESS_NORUNTIME
//   /DKIND_UNSUPPORTED  exports the thirteen entry points the helper takes; xessD3D12CreateContext answers
//                       XESS_RESULT_ERROR_UNSUPPORTED_DEVICE (-1), as Intel's does on a card it cannot run -> FEED_CAP_XESS_UNSUPPORTED
//   /DKIND_OTHER        exports none of them: a file that is not Intel's -> FEED_CAP_XESS_PROBED alone
//   /DKIND_DEP          x2dep.dll itself, built only for its import library
// Every entry point but the two the probe reaches first answers XESS_RESULT_ERROR_UNKNOWN (-1000).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#define X2E extern "C" __declspec(dllexport)
#if defined(KIND_DEP)
X2E int x2_missing_dep(void) { return 7; }
#elif defined(KIND_RUNTIME)
extern "C" __declspec(dllimport) int x2_missing_dep(void);
X2E int x2_use(void) { return x2_missing_dep(); }
#elif defined(KIND_UNSUPPORTED)
X2E int xessGetVersion(void* v) { if (v) memset(v, 0, 8); return 0; }
X2E int xessD3D12CreateContext(void*, void** ctx) { if (ctx) *ctx = nullptr; return -1; }
X2E int xessD3D12BuildPipelines(void*, void*, int, unsigned) { return -1000; }
X2E int xessD3D12Init(void*, const void*) { return -1000; }
X2E int xessD3D12Execute(void*, void*, const void*) { return -1000; }
X2E int xessDestroyContext(void*) { return -1000; }
X2E int xessGetOptimalInputResolution(void*, const void*, int, void*, void*, void*) { return -1000; }
X2E int xessGetProperties(void*, const void*, void*) { return -1000; }
X2E int xessIsOptimalDriver(void*) { return -1000; }
X2E int xessSetVelocityScale(void*, float, float) { return -1000; }
X2E int xessSetJitterScale(void*, float, float) { return -1000; }
X2E int xessSetLoggingCallback(void*, int, void*) { return -1000; }
X2E int xessGetPipelineBuildStatus(void*) { return -1000; }
#elif defined(KIND_INITFAIL) || defined(KIND_WARMFAIL)
// (fg18-fix, F6, the release audit of 2026-10-28) A FILE WHOSE CREATE WORKS AND WHOSE BUILD FAILS: the probe succeeds (a context, the driver
// optimal, the pipelines started and done), every setting answers a range that holds the render size, and then
//   /DKIND_INITFAIL  xessD3D12Init answers XESS_RESULT_ERROR_INVALID_ARGUMENT (-4): the build's context refused at its init
//   /DKIND_WARMFAIL  the init and the scales succeed, and xessD3D12Execute answers XESS_RESULT_ERROR_UNKNOWN (-1000): the warm-up fails,
//                    nothing submitted
// so automatic's fall from XeSS to FSR at a build (host\xess_host.h, CreateXessContext) runs through the helper's own build, on this desk's card.
// No handle is real: the context is a pointer to a static byte, never dereferenced by the helper.
struct X2Size { unsigned x, y; };
static char x2Ctx;
X2E int xessGetVersion(void* v) { if (v) memset(v, 0, 8); return 0; }
X2E int xessD3D12CreateContext(void*, void** ctx) { if (ctx) *ctx = &x2Ctx; return 0; }
X2E int xessD3D12BuildPipelines(void*, void*, int, unsigned) { return 0; }
X2E int xessGetPipelineBuildStatus(void*) { return 0; }
X2E int xessIsOptimalDriver(void*) { return 0; }
X2E int xessSetLoggingCallback(void*, int, void*) { return 0; }
X2E int xessDestroyContext(void*) { return 0; }
X2E int xessGetOptimalInputResolution(void*, const void* out, int, void* opt, void* mn, void* mx)
{
    const X2Size o = out ? *(const X2Size*)out : X2Size{ 0, 0 };
    if (opt) *(X2Size*)opt = { o.x * 2 / 3, o.y * 2 / 3 };
    if (mn) *(X2Size*)mn = { o.x / 4, o.y / 4 };
    if (mx) *(X2Size*)mx = o;
    return 0;
}
X2E int xessGetProperties(void*, const void*, void*) { return 0; }   // (the helper's own zeroed struct stands: nothing written)
X2E int xessSetVelocityScale(void*, float, float) { return 0; }
X2E int xessSetJitterScale(void*, float, float) { return 0; }
#if defined(KIND_INITFAIL)
X2E int xessD3D12Init(void*, const void*) { return -4; }
X2E int xessD3D12Execute(void*, void*, const void*) { return -1000; }
#else
X2E int xessD3D12Init(void*, const void*) { return 0; }
X2E int xessD3D12Execute(void*, void*, const void*) { return -1000; }
#endif
#else
X2E int x2_nothing(void) { return 0; }
#endif
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
