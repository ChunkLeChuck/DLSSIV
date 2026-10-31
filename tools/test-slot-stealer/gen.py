"""THE SLOT STEALER'S BENCH (2026-09-27). The performance card's leg M1 puts tools/perf-card/slot-stealer.asi in the game
for one launch, to make the startup miss happen on purpose. Its first design lost a race in the game (it read DLSS-IV's
hook, "TOO LATE"), so the second design is proven here first: the SHIPPED slot-stealer.cpp, built as the kit builds it,
loaded into a small x86 program beside a fake d3d9.dll (one IDirect3D9 vtable whose slot 16 counts its calls) and a
stand-in DLSS-IV.asi that does what DLSS-IV's TryModule does (GetProcAddress on d3d9.dll, Direct3DCreate9, hook slot 16).
The program then plays the game: Direct3DCreate9 through the pointer it took before any ASI loaded (the game's import),
and CreateDevice through the vtable.

    python tools/test-slot-stealer/gen.py        builds and runs; exit code 0 = every check passed

Cases: the ASI loader's order (the stealer, then DLSS-IV): the game's CreateDevice must reach the fake's own slot 16 through
the stealer and NOT DLSS-IV's hook, and the stealer's log must hold the lines the kit's W reads. The controls: no stealer
(the game's call goes through DLSS-IV's hook), and DLSS-IV loaded first (the stealer says TOO LATE and changes nothing).

Needs the same Visual Studio the build uses (vswhere finds it). Writes into %TEMP%\\dlssiv-test-stealer-<pid> (removed when
it passes, kept to read when it fails).
"""
import os, sys, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
STEALER = os.path.join(ROOT, "tools", "perf-card", "slot-stealer.cpp")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-stealer-%d" % os.getpid())
NAME = "0-dlssiv-test-slot-stealer"

FAKE_D3D9 = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
static volatile LONG gOrig = 0;
static HRESULT __stdcall OrigCreateDevice(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9** out)
{
    InterlockedIncrement(&gOrig);
    if (out) *out = nullptr;
    return S_OK;
}
static void* gVtbl[17] = {};
static void* gObj = gVtbl;          // an "interface" is a pointer to its vtable
extern "C" IDirect3D9* __stdcall FakeCreate9(UINT) { if (!gVtbl[16]) gVtbl[16] = (void*)&OrigCreateDevice; return (IDirect3D9*)&gObj; }
extern "C" LONG __stdcall FakeOrigCalls() { return gOrig; }
#pragma comment(linker, "/EXPORT:Direct3DCreate9=_FakeCreate9@4")
#pragma comment(linker, "/EXPORT:FakeOrigCalls=_FakeOrigCalls@0")
'''

FAKE_DLSSIV = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
typedef HRESULT (__stdcall *CreateDeviceFn)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
static CreateDeviceFn gOrig = nullptr;
static volatile LONG gHook = 0;
static HRESULT __stdcall HkCreateDevice(IDirect3D9* d, UINT a, D3DDEVTYPE t, HWND w, DWORD f, D3DPRESENT_PARAMETERS* p, IDirect3DDevice9** o)
{
    InterlockedIncrement(&gHook);
    return gOrig(d, a, t, w, f, p, o);
}
// What DLSS-IV's TryModule does: ask the module by name, create, hook slot 16 in place.
extern "C" void* __stdcall Arm(HMODULE d3d9)
{
    typedef IDirect3D9* (WINAPI *Create9Fn)(UINT);
    Create9Fn create = (Create9Fn)GetProcAddress(d3d9, "Direct3DCreate9");
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) return nullptr;
    void** vtbl = *(void***)d3d;
    gOrig = (CreateDeviceFn)vtbl[16];
    DWORD old = 0;
    VirtualProtect(&vtbl[16], sizeof(void*), PAGE_READWRITE, &old);
    vtbl[16] = (void*)&HkCreateDevice;
    VirtualProtect(&vtbl[16], sizeof(void*), old, &old);
    return (void*)&HkCreateDevice;
}
extern "C" LONG __stdcall HookCalls() { return gHook; }
#pragma comment(linker, "/EXPORT:Arm=_Arm@4")
#pragma comment(linker, "/EXPORT:HookCalls=_HookCalls@0")
'''

PROGRAM = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstring>
typedef HRESULT (__stdcall *CreateDeviceFn)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef IDirect3D9* (WINAPI *Create9Fn)(UINT);
typedef void* (__stdcall *ArmFn)(HMODULE);
typedef LONG (__stdcall *CountFn)();
int main(int argc, char** argv)
{
    const char* mode = argc > 1 ? argv[1] : "stealer";
    HMODULE d3d9 = LoadLibraryA("d3d9.dll");                                  // the game's import, before any ASI
    Create9Fn gameCreate = (Create9Fn)GetProcAddress(d3d9, "Direct3DCreate9"); // bound at load, like the game's
    if (!strcmp(mode, "late")) { HMODULE dl = LoadLibraryA("DLSS-IV.asi"); ((ArmFn)GetProcAddress(dl, "Arm"))(d3d9); }
    if (strcmp(mode, "none")) LoadLibraryA("0-dlssiv-test-slot-stealer.asi");  // the ASI loader: the stealer sorts first
    HMODULE dl = LoadLibraryA("DLSS-IV.asi");
    void* hook = nullptr;
    if (strcmp(mode, "late")) hook = ((ArmFn)GetProcAddress(dl, "Arm"))(d3d9);
    IDirect3D9* d3d = gameCreate(D3D_SDK_VERSION);
    void** vtbl = *(void***)d3d;
    if (!strcmp(mode, "stealer"))                                               // the stealer's thread writes within ms
        for (int i = 0; i < 3000 && vtbl[16] == hook; i++) Sleep(1);
    IDirect3DDevice9* dev = nullptr;
    const HRESULT hr = ((CreateDeviceFn)vtbl[16])(d3d, 0, D3DDEVTYPE_HAL, nullptr, 0, nullptr, &dev);   // the game's CreateDevice
    printf("hr=0x%08lX dlss_hook=%ld original=%ld\n", (unsigned long)hr,
           ((CountFn)GetProcAddress(dl, "HookCalls"))(), ((CountFn)GetProcAddress(d3d9, "FakeOrigCalls"))());
    fflush(stdout);
    Sleep(100);                                                                 // the stealer's log line after the call
    return 0;
}
'''

def vcvars():
    vsw = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), r"Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")

def cl(vc, args, cwd):
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo %s' % (vc, args), shell=True, capture_output=True, text=True, cwd=cwd)
    if r.returncode != 0 or "warning C" in r.stdout:
        print(r.stdout); print(r.stderr); raise SystemExit("COMPILE FAILED: cl %s" % args)

def main():
    os.makedirs(OUT, exist_ok=True)
    vc = vcvars()
    for name, src in (("fake_d3d9.cpp", FAKE_D3D9), ("fake_dlssiv.cpp", FAKE_DLSSIV), ("prog.cpp", PROGRAM)):
        open(os.path.join(OUT, name), "w").write(src)
    cl(vc, '/LD /MT /W3 /O2 fake_d3d9.cpp /Fe:d3d9.dll /link /NOIMPLIB /NOEXP', OUT)
    cl(vc, '/LD /MT /W3 /O2 fake_dlssiv.cpp /Fe:DLSS-IV.asi /link /NOIMPLIB /NOEXP', OUT)
    # the stealer exactly as tools/perf-card/build.ps1 builds it
    cl(vc, '/LD /MT /W4 /WX /O2 "%s" /Fe:%s.asi /Fo:stealer.obj /link /NOIMPLIB /NOEXP' % (STEALER, NAME), OUT)
    cl(vc, '/MT /W3 /O2 prog.cpp /Fe:t.exe', OUT)

    fails = 0
    def run(mode):
        log = os.path.join(OUT, NAME + ".log")
        if os.path.exists(log): os.remove(log)
        r = subprocess.run([os.path.join(OUT, "t.exe"), mode], capture_output=True, text=True, cwd=OUT, timeout=60)
        lines = open(log).read().splitlines() if os.path.exists(log) else []
        return r.stdout.strip(), lines
    def check(ok, what):
        nonlocal fails
        fails += 0 if ok else 1
        print("  %s  %s" % ("PASS" if ok else "FAIL", what))

    out, lines = run("stealer")
    print("the ASI loader's order, the stealer first: %s" % out)
    for l in lines: print("      stealer: " + l)
    check(out == "hr=0x00000000 dlss_hook=0 original=1", "the game's CreateDevice reached the original and NOT DLSS-IV's hook")
    # WHOSE POINTER: on Windows 11 the program's d3d9 runs behind apphelp.dll's compatibility layer, so the slot read before
    # DLSS-IV hooks may be apphelp's rather than d3d9.dll's. What the test needs is that it is not DLSS-IV's.
    check(any("came through this module; read slot 16" in l and "DLSS-IV.asi" not in l for l in lines), "the slot was read at DLSS-IV's own Direct3DCreate9 call, before DLSS-IV's hook")
    check(any("in DLSS-IV.asi" in l and "slot 16 changed" in l for l in lines), "the stealer saw DLSS-IV's hook appear")
    check(any("WROTE this module's hook" in l for l in lines), "W's line: WROTE this module's hook")
    check(any("CreateDevice call 1 went through" in l for l in lines), "W's line: CreateDevice call 1 went through")
    check(not any("TOO LATE" in l or "NOT RUN" in l for l in lines), "no TOO LATE or NOT RUN")

    out, lines = run("none")
    print("the control, no stealer: %s" % out)
    check(out == "hr=0x00000000 dlss_hook=1 original=1", "the game's CreateDevice went through DLSS-IV's hook")

    out, lines = run("late")
    print("the control, DLSS-IV loaded first: %s" % out)
    for l in lines: print("      stealer: " + l)
    check(out == "hr=0x00000000 dlss_hook=1 original=1", "nothing taken: the game's call went through DLSS-IV's hook")
    check(any("TOO LATE" in l for l in lines) and not any("WROTE" in l for l in lines), "the stealer said TOO LATE and wrote nothing")

    print("PASS" if fails == 0 else "FAIL: %d check(s)" % fails)
    return 0 if fails == 0 else 1

if __name__ == "__main__":
    rc = main()
    if rc == 0: shutil.rmtree(OUT, ignore_errors=True)
    sys.exit(rc)
