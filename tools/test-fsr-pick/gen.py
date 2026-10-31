"""AUTOMATIC AND A DIRECT3D 12 TOO OLD FOR FSR 4 (2026-10-03). FsrBelowFsr4 (host/dlssiv-helper.cpp, between the FSR PICK
lines) decides, before anything of AMD's is created, whether automatic names the newest FSR 3 and leaves FSR 4 alone. The
one machine in hand that needs it is a player's (a Radeon RX 7800 XT on Windows 10: the library offers "4.1.1", "3.1.5",
"2.3.4", D3D12 offers shader model 6.5, FSR 4 raises in its first dispatch), so the rule is held here against made-up
lists: that list at 6.5 and at 6.6, a shader model that was not read, fsr_version naming a version, a list with no FSR 4,
a list with no FSR 3, several FSR 3 (the newest numerically), a name that is not a version, a missing name.

And two things the cut cannot show, read from CreateFsrContext's own text: the versions passed by are marked for the retry
loop (a failed FSR 3 create must not go on to the FSR 4 that raises), and the list is asked for at log=0 when it is needed.

THE FSR 4 START-UP TEST (2026-10-10, host/fsr4_test.h): where the rule would pass FSR 4 by, a copy of the helper tries
FSR 4 and its exit code is the answer; nothing is kept, the test runs once a session at every launch that meets the
rule. Held here on a bench with no device: the reading of the copy's exit code, with mutations that must fail. And read
from the text: the test is asked only where the rule speaks, main's three lines, no fault outside a development build,
and that nothing in the test's file opens a file but the one warning it may append.

    python tools/test-fsr-pick/gen.py        exit code 0 = every check passed

THE LIVE PART, BY HAND (--live): the BUILT DEVELOPMENT helper against a stand-in game that asks it for one build, with a
community FSR 4 library beside it, so the rule and the test run against AMD's own code on a machine with no AMD card. The
owner's D3D12 offers shader model 6.8, so the line is moved one above what the machine offers (--fsr4_min_sm, a
development helper's argument): the rule then speaks, and the test is asked. It passes when the copy runs FSR 4 and the
helper then runs it too, at every launch and with nothing left on disk, and each planted fault (--fsr4_test_fault)
gives the FSR 3 with the right words in the log: an exception, a clean failure, a copy that dies, a copy that never ends
(ended after its wait, and the warnings file says how to skip the test). And the controls: the line where a release has
it (no test), fsr_version=4 (FSR 4 runs, no test), AMD's own library (no FSR 4 on offer: nothing changes), log=0
(nothing written).

    python tools/test-fsr-pick/gen.py --live [--fsr4 <the community amd_fidelityfx_upscaler_dx12.dll>]
Needs build-host WITHOUT -Release, this machine's graphics card, and the game closed. It opens no window. prove-all runs
the part above only.
"""
import os, re, sys, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "host", "dlssiv-helper.cpp")
T4 = os.path.join(ROOT, "host", "fsr4_test.h")
IPC = os.path.join(ROOT, "source", "transport", "feed_ipc.h")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fsr-pick-%d" % os.getpid())
START = "// FSR PICK START"
END = "// FSR PICK END"
T4START = "// FSR4 TEST PURE START"
T4END = "// FSR4 TEST PURE END"

BENCH = r"""
static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) fails++; }
int main()
{
    const int MIN = FEED_FSR4_MIN_SM;
    Check(MIN == 0x66, "the line is shader model 6.6: below it automatic does not run FSR 4");
    const char* player[3] = { "4.1.1", "3.1.5", "2.3.4" };          // the RX 7800 XT player's list, in the library's order
    Check(FsrBelowFsr4(0, 0x65, MIN, player, 3) == 1, "the player's machine (6.5): automatic names \"3.1.5\"");
    Check(FsrBelowFsr4(0, 0x60, MIN, player, 3) == 1, "6.0: the same");
    Check(FsrBelowFsr4(0, 0x66, MIN, player, 3) == -1, "6.6 is not measured with AMD's FSR 4: nothing changes");
    Check(FsrBelowFsr4(0, 0x68, MIN, player, 3) == -1, "6.8: nothing changes");
    Check(FsrBelowFsr4(0, 0, MIN, player, 3) == -1, "a shader model that was not read: nothing changes");
    Check(FsrBelowFsr4(4, 0x65, MIN, player, 3) == -1, "fsr_version=4 named FSR 4: it is not passed by");
    Check(FsrBelowFsr4(3, 0x65, MIN, player, 3) == -1, "fsr_version=3: the setting's own match decides, not this rule");
    const char* nvidia[2] = { "3.1.5", "2.3.4" };                   // what AMD's library offers the owner's RTX 4090
    Check(FsrBelowFsr4(0, 0x65, MIN, nvidia, 2) == -1, "no FSR 4 on offer: nothing to pass by");
    const char* noThree[2] = { "4.1.1", "2.3.4" };
    Check(FsrBelowFsr4(0, 0x65, MIN, noThree, 2) == -1, "no FSR 3 on offer: nobody is dropped to FSR 2, the library's own pick stays");
    const char* several[4] = { "4.1.1", "3.1.4", "3.1.10", "3.1.9" };
    Check(FsrBelowFsr4(0, 0x65, MIN, several, 4) == 2, "several FSR 3: the newest by number (3.1.10 over 3.1.9)");
    const char* order[2] = { "3.1.5", "4.1.1" };
    Check(FsrBelowFsr4(0, 0x65, MIN, order, 2) == 0, "FSR 4 listed last: still passed by");
    const char* later[2] = { "5.0.0", "3.1.5" };
    Check(FsrBelowFsr4(0, 0x65, MIN, later, 2) == 1, "a later major than 4 is passed by as well");
    const char* odd[2] = { "bc250", "3.1.5" };
    Check(FsrBelowFsr4(0, 0x65, MIN, odd, 2) == -1, "a name that does not start with a digit is not an FSR 4: nothing changes");
    const char* hole[3] = { "4.1.1", nullptr, "3.1.5" };
    Check(FsrBelowFsr4(0, 0x65, MIN, hole, 3) == 2, "a missing name is no version, and is stepped over");
    Check(FsrBelowFsr4(0, 0x65, MIN, player, 0) == -1, "an empty list: nothing changes");
    if (fails) { printf("FAIL: %d check(s)\n", fails); return 1; }
    printf("PASS\n"); return 0;
}
"""

# THE START-UP TEST'S PART THAT NEEDS NO DEVICE: its reading of the copy's end, cut from host/fsr4_test.h.
T4HEAD = r"""// GENERATED by tools/test-fsr-pick/gen.py: the exit code's reading, cut from host/fsr4_test.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
"""
T4BENCH = r"""
static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "PASS" : "FAIL", what); if (!ok) fails++; }
int main()
{
    const char* w = "";
    Check(Fsr4TestAnswer(true, 40, &w) == 1 && !strcmp(w, "runs"), "the copy ended with 40: FSR 4 runs");
    Check(Fsr4TestAnswer(true, 41, &w) == 0 && Fsr4TestAnswer(true, 42, &w) == 0 && !strcmp(w, "does not run"), "41 (failed) and 42 (raised): it does not");
    Check(Fsr4TestAnswer(true, 0xC0000409u, &w) == 0 && Fsr4TestAnswer(true, 0xC00000FDu, &w) == 0 && Fsr4TestAnswer(true, 0xE06D7363u, &w) == 0,
          "a copy that died of an exception (a fast fail, a stack overflow, a C++ throw): it does not");
    Check(Fsr4TestAnswer(true, 0, &w) == -1 && Fsr4TestAnswer(true, 1, &w) == -1 && Fsr4TestAnswer(true, 2, &w) == -1
          && Fsr4TestAnswer(true, 259, &w) == -1 && !strcmp(w, "no answer"), "any other code (no device, no library, an old helper's 0, 1 or 2): no answer");
    Check(Fsr4TestAnswer(false, 0xFFFFFFFFu, &w) == -1 && !strcmp(w, "did not end"), "a copy that did not end: no answer, and said as that");
    Check(Fsr4TestAnswer(false, 40, &w) == -1, "a copy that was ended is not believed, whatever its code reads");
    if (fails) { printf("FAIL: %d check(s)\n", fails); return 1; }
    printf("PASS\n"); return 0;
}
"""
# each must make the bench FAIL: (what, the cut's text, its replacement)
T4MUTANTS = [
    ("a copy that died of an exception is no answer", "code == FSR4T_RAISED || code >= 0x80000000u", "code == FSR4T_RAISED"),
    ("a copy that did not end is believed", "if (!ended)            {", "if (!ended && code == 7) {"),
    ("any code is a yes", "if (code == FSR4T_RAN) {", "if (code != FSR4T_FAILED) {"),
]

def strip(text):
    return "\n".join(l.split("//")[0] for l in text.splitlines())

def dev_only(text, needle):
    """True when every line of text holding needle lies inside #if DLSSIV_DEV."""
    depth = []; inside = []
    for l in text.splitlines():
        s = l.strip()
        if s.startswith("#if"): depth.append(True if s.startswith("#if DLSSIV_DEV") else None)
        elif s.startswith("#else") and depth and depth[-1] is not None: depth[-1] = not depth[-1]
        elif s.startswith("#endif") and depth: depth.pop()
        if needle in l.split("//")[0]: inside.append(any(d is True for d in depth))
    return bool(inside) and all(inside), len(inside)

def text_checks(src, t4):
    """CreateFsrContext's own text, scoped to the function, comments stripped, with a control that must fail."""
    a = src.index("static bool CreateFsrContext(")
    b = src.index("// FSR'S OWN RCAS IS OFF", a)
    body = strip(src[a:b])
    fails = 0
    def check(ok, what):
        nonlocal fails
        print("  %s  %s" % ("PASS" if ok else "FAIL", what))
        if not ok: fails += 1
    def order(*parts, text=None):
        text = body if text is None else text
        at = -1
        for p in parts:
            at = text.find(p, at + 1)
            if at < 0: return False
        return True
    check(order("ReadTopShaderModel();", "smLow", "QueryFfxVersions(&v)"),
          "the shader model is read, and the list asked for when automatic needs it, before any create")
    check("|| smLow) ? QueryFfxVersions(&v)" in body, "the list is asked for at log=0 too when the shader model is low")
    check(order("FsrBelowFsr4(g_fsrVersion, g_topSm, g_fsr4MinSm, v.names, (int)v.n)", "pick = below;", "g_fsr4SkipSm = g_topSm;",
                "bool tried[8] = {};", "if (FfxMajor(v.names[i]) >= 4) tried[i] = true;", "int attempt = pick;", "SafeFfxCreate("),
          "the pick is made, said to the game side, and the FSR 4 versions marked tried before the first create")
    check(not order("SafeFfxCreate(", "FsrBelowFsr4("), "control: the same reader does not find the pick AFTER the create")
    check(body.count("g_fsr4SkipSm = 0;") == 1 and order("g_fsr4SkipSm = 0;", "FsrBelowFsr4("),
          "the note is cleared before each context's decision")
    # THE LINE CANNOT MOVE IN A RELEASE: its one write besides the initialiser lies inside #if DLSSIV_DEV
    depth = []; writes = []
    for l in src.splitlines():
        s = l.strip()
        if s.startswith("#if"): depth.append(True if s.startswith("#if DLSSIV_DEV") else None)
        elif s.startswith("#else") and depth and depth[-1] is not None: depth[-1] = not depth[-1]
        elif s.startswith("#endif") and depth: depth.pop()
        code = l.split("//")[0]
        if re.search(r"g_fsr4MinSm\s*=[^=]", code): writes.append(any(d is True for d in depth))
    check(writes == [False, True], "the line is written twice in the helper: its initialiser, and the argument inside #if DLSSIV_DEV")

    # THE START-UP TEST (2026-10-10)
    check(body.count("Fsr4TestSaysRun(") == 1 and order("FsrBelowFsr4(", "if (below >= 0 && Fsr4TestSaysRun(w, hh, outW, outH, flags)) below = -1;",
                "if (below >= 0)", "pick = below;", "SafeFfxCreate("),
          "the start-up test is asked once, only where the rule speaks, before the pick")
    check(not order("pick = below;", "Fsr4TestSaysRun("), "control: the same reader does not find the test AFTER the pick")
    check(order("g_fsrMajorInUse = known ? FfxMajor(pv.versionName) : 0;", "FSR provider in use"), "the FSR that survived is noted where its line is written")
    m = src.index("int main(int argc, char** argv)")
    main = strip(src[m:])
    check(order("Fsr4TestArg(argv[i]);", "InitDevice(Fsr4TestLuid())", "ResolveBackendAndProbe();", "if (g_fsr4Test) return Fsr4TestChild(pid);", "return Serve(pid);", text=main),
          "main: the copy's argument is read, its device made on its parent's adapter, the library loaded as ever, and it never serves")
    sv = strip(src[src.index("static int Serve(DWORD gamePid)"):m])
    check("Fsr4Test" not in sv and "g_f4" not in sv, "the serve loop does not know the test")
    t4s = strip(t4)
    ok1, n1 = dev_only(t4, "g_f4Fault ==")
    check(ok1 and n1 == 4, "the four planted faults are read inside #if DLSSIV_DEV only (%d reads)" % n1)
    check("#if !DLSSIV_DEV\n    g_f4Fault = 0; g_f4Loud = 0;" in t4.replace("\r\n", "\n"), "and a release copy zeroes what its command line gave for them")
    ok2, n2 = dev_only(t4, "g_f4MaxMs =")
    check(not ok2 and n2 == 2 and dev_only(t4, "g_f4MaxMs = (DWORD)v")[0], "the wait is written twice: its initialiser, and the argument inside #if DLSSIV_DEV")
    check('--backend=1 --fsr_version=0 --log=0' in t4s and "fsr4_min_sm" not in t4s,
          "the copy is started as automatic FSR with its own lines off, and is never handed the development line")
    check(order("if (g_fsr4Test) return true;", "if (g_f4Answer >= 0)", "wine_get_version", "g_f4MaxMs + FSR4T_AFTER_MS > FSR4T_BUDGET_MS", "CreateProcessA(", text=t4s),
          "the parent's half: the copy never asks itself, a session asks once, Wine keeps the rule, and the build's time comes before a copy is started")
    check(order("WaitForSingleObject(pi.hProcess, g_f4MaxMs)", "TerminateProcess(pi.hProcess, 1)", text=t4s) and "CREATE_NO_WINDOW" in t4s and "SEM_NOGPFAULTERRORBOX" in t4s,
          "a copy that does not end is ended; it has no window and can raise no error box")
    opens = re.findall(r"_fsopen\(|fopen|CreateFileA\(|CreateFileW\(|RegCreateKey|RegSetValue|CreateDirectory", t4s)
    check(opens == ["_fsopen("] and '_fsopen(path, "a", _SH_DENYNO)' in t4s and "DLSS-IV-WARNINGS.txt" in t4s,
          "NOTHING IS KEPT: the test's file opens one file, the game's warnings file, to append one line; no other file, folder or registry key")
    return fails

def t4_bench(t4, vcvars):
    """The exit code's reading on a bench, then each mutation, which must fail."""
    a = t4.index(T4START); b = t4.index(T4END, a)
    cut = t4[a:b].replace("\r\n", "\n")
    def build_run(name, text):
        d = os.path.join(OUT, name); os.makedirs(os.path.join(d, "scratch"), exist_ok=True)
        with open(os.path.join(d, "t4.cpp"), "w", encoding="utf-8") as f: f.write(T4HEAD + text + T4BENCH)
        r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /EHsc /W4 /WX /std:c++17 /O2 t4.cpp /Fe:t4.exe' % vcvars, shell=True, capture_output=True, text=True, cwd=d)
        if r.returncode != 0: return None, r.stdout[-3000:] + r.stderr[-1000:]
        r = subprocess.run([os.path.join(d, "t4.exe")], capture_output=True, text=True)
        return r.returncode, r.stdout
    rc, out = build_run("t4", cut)
    if rc is None: print(out); print("FAIL: the start-up test's cut did not compile"); return 1
    print(out, end="")
    fails = 1 if rc else 0
    for what, old, new in T4MUTANTS:
        if cut.count(old) != 1: print("  FAIL  control cannot be planted (%d x): %s" % (cut.count(old), what)); fails += 1; continue
        mrc, mout = build_run("t4-m%d" % T4MUTANTS.index((what, old, new)), cut.replace(old, new))
        ok = mrc is not None and mrc != 0
        print("  %s  control: %s -> the bench fails" % ("PASS" if ok else "FAIL", what))
        if not ok: fails += 1
    return fails

GAME = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "feed_ipc.h"
// game.exe <helper> <log> <log level> [helper arguments]      the stand-in game: a hello, one build, the answer's bits
static bool WriteAll(HANDLE p, const void* d, DWORD n) { DWORD w = 0; return WriteFile(p, d, n, &w, nullptr) && w == n; }
static bool ReadAll(HANDLE p, void* d, DWORD n)
{
    BYTE* b = (BYTE*)d;
    while (n) { DWORD r = 0; if (!ReadFile(p, b, n, &r, nullptr) || !r) return false; b += r; n -= r; }
    return true;
}
int main(int argc, char** argv)
{
    if (argc < 4) return 2;
    const DWORD me = GetCurrentProcessId();
    char name[128]; sprintf_s(name, FEED_PIPE_FMT, (unsigned long)me);
    char cmd[4096];
    int n = sprintf_s(cmd, "\"%s\" %lu --log=%s --sync=0 --backend=1 --logpath=\"%s\"", argv[1], (unsigned long)me, argv[3], argv[2]);
    for (int i = 4; i < argc; i++) n += sprintf_s(cmd + n, sizeof(cmd) - n, " %s", argv[i]);
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION helper = {};
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &helper)) { printf("helper did not start %lu\n", GetLastError()); return 4; }
    HANDLE p = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 600 && p == INVALID_HANDLE_VALUE; i++)      // 30 s: the helper makes its device first
    {
        p = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
        if (p == INVALID_HANDLE_VALUE && WaitForSingleObject(helper.hProcess, 50) == WAIT_OBJECT_0) break;
    }
    int rc = 5;
    if (p != INVALID_HANDLE_VALUE)
    {
        FeedHello hello = {};
        hello.magic = FEED_IPC_MAGIC; hello.version = FEED_IPC_VERSION; hello.pid = me; hello.client_kind = FEED_CLIENT_VULKAN;
        strcpy_s(hello.build, "unknown-source (the stand-in game)");
        FeedHelloAck ha = {};
        FeedBuild b = {};                                           // a build as the game's log shows one: Quality at 2560x1440
        b.width = 1706; b.height = 960; b.color_fmt = 10; b.output_fmt = 10; b.hdr = 1; b.depth_inverted = 1; b.flags_override = -1;
        b.mv_scale_x = b.mv_scale_y = 1.0f; b.target_width = 2560; b.target_height = 1440;
        FeedBuildAck a = {};
        const BYTE tag = 'B';
        const ULONGLONG t0 = GetTickCount64();
        if (WriteAll(p, &hello, sizeof(hello)) && ReadAll(p, &ha, sizeof(ha)) && WriteAll(p, &tag, 1) && WriteAll(p, &b, sizeof(b)) && ReadAll(p, &a, sizeof(a)))
        { printf("caps=0x%llX ok=%d flags=0x%X build_ms=%llu\n", (unsigned long long)ha.caps, a.ok, a.flags, GetTickCount64() - t0); rc = 0; }
        else printf("the exchange failed %lu\n", GetLastError());
        CloseHandle(p);
    }
    else printf("no pipe\n");
    if (WaitForSingleObject(helper.hProcess, 20000) != WAIT_OBJECT_0) TerminateProcess(helper.hProcess, 9);
    return rc;
}
'''

KIT_FSR4 = "(none: name a community FSR 4 amd_fidelityfx_upscaler_dx12.dll with --fsr4)"
SKIPPED, RAISED, FSR, SM_MASK, SM_SHIFT = 16, 8, 4, 0xFF00, 8
# before anything that makes a Direct3D device: nobody else's helper or test bench may be running. dt_fg_test is not
# in the list: it runs frame generation's machine on a fake DXVK and makes no device (the FG session, 2026-10-10).
OTHERS = ("gtaiv.exe", "dlssiv-helper.exe", "fgpipe", "vklines", "perfproof")

def live(vcvars):
    helper = os.path.join(ROOT, "host", "dlssiv-helper.exe")
    ffx = os.path.join(ROOT, "external", "ffx")
    fsr4 = sys.argv[sys.argv.index("--fsr4") + 1] if "--fsr4" in sys.argv else KIT_FSR4
    if not os.path.exists(helper): print("FAIL (live): %s is not built" % helper); return 1
    if not os.path.exists(fsr4): print("FAIL (live): no community FSR 4 library at %s (name one with --fsr4)" % fsr4); return 1
    tl = subprocess.run("tasklist /NH", shell=True, capture_output=True, text=True).stdout.lower()
    busy = sorted(set(l.split()[0] for l in tl.splitlines() if l.split() and any(l.startswith(o) for o in OTHERS)))
    if busy: print("FAIL (live): %s is running; not started" % ", ".join(busy)); return 1
    out = os.path.join(tempfile.gettempdir(), "dlssiv-test-fsr-live-%d" % os.getpid())
    fails = 0
    def check(ok, what):
        nonlocal fails
        print("  %s  %s" % ("PASS" if ok else "FAIL", what), flush=True)
        if not ok: fails += 1
    def stage(name, upscaler):
        d = os.path.join(out, name); shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
        shutil.copy(helper, os.path.join(d, "dlssiv-helper.exe"))
        for f in ("amd_fidelityfx_loader_dx12.dll", "amd_ags_x64.dll"): shutil.copy(os.path.join(ffx, f), os.path.join(d, f))
        shutil.copy(upscaler, os.path.join(d, "amd_fidelityfx_upscaler_dx12.dll"))
        return d
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "game.cpp"), "w") as f: f.write(GAME)
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /EHsc /W3 /std:c++17 /O2 /I"%s" game.cpp /Fe:game.exe'
                       % (vcvars, os.path.join(ROOT, "source", "transport")), shell=True, capture_output=True, text=True, cwd=out)
    if r.returncode != 0 or "warning C" in r.stdout:
        print(r.stdout[-3000:]); print("FAIL (live): the stand-in game did not compile cleanly"); return 1
    n = [0]; times = []
    def run(d, level, *args, env=None):
        n[0] += 1
        log = os.path.join(out, "helper-%d.log" % n[0])
        e = dict(os.environ); e.update(env or {})
        g = subprocess.run([os.path.join(out, "game.exe"), os.path.join(d, "dlssiv-helper.exe"), log, str(level)] + list(args),
                           capture_output=True, text=True, cwd=d, timeout=240, env=e)
        text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
        m = re.search(r"ok=(-?\d+) flags=0x([0-9A-Fa-f]+) build_ms=(\d+)", g.stdout)
        prov = re.findall(r'FSR provider in use: "([^"]*)"', text)
        tm = re.search(r"FSR 4 test: [^\n]*\(exit 0x([0-9A-F]+), (\d+) ms\)", text)
        if tm: times.append((int(tm.group(2)), int(tm.group(1), 16), " ".join(a for a in args if "fault" in a) or "no fault"))
        return {"ok": int(m.group(1)) if m else -1, "flags": int(m.group(2), 16) if m else 0, "ms": int(m.group(3)) if m else -1, "text": text,
                "provider": prov[-1] if prov else "", "providers": len(prov), "out": g.stdout.strip(),
                "exit": int(tm.group(1), 16) if tm else None, "test_ms": int(tm.group(2)) if tm else -1, "log": n[0]}
    def major(v): return int(v.split(".")[0]) if v[:1].isdigit() else 0
    warnings = os.path.join(out, "DLSS-IV-WARNINGS.txt")           # beside the log the helper is given, as in the game
    def warned(): return open(warnings, encoding="utf-8", errors="replace").read() if os.path.exists(warnings) else ""
    def left(d): return sorted(os.listdir(d))                     # what a launch leaves in the helper's folder
    # no line of the test may carry a word a kit reads as a fault of the session (PowerShell's -match ignores case)
    def clean(r): return not any(re.search(r"raised|fail|fault:|stopped|refused", l, re.I) for l in r["text"].splitlines() if "FSR 4 test" in l)
    def fsr4_ran(r): return clean(r) and r["ok"] == 1 and (r["flags"] & FSR) and not (r["flags"] & SKIPPED) and not (r["flags"] & RAISED) and major(r["provider"]) >= 4 and r["providers"] == 1
    def fsr3_ran(r): return (clean(r) and r["ok"] == 1 and (r["flags"] & FSR) and (r["flags"] & SKIPPED) and not (r["flags"] & RAISED) and major(r["provider"]) == 3 and r["providers"] == 1
                             and 'automatic runs "%s"' % r["provider"] in r["text"] and "warming up" not in r["text"] and "RAISED" not in r["text"])

    # THE CONTROL FIRST: the line where a release has it. It also says what this machine offers and what build the helper is.
    community = stage("community", fsr4)
    files0 = left(community)
    c = run(community, 1)
    sm = (c["flags"] & SM_MASK) >> SM_SHIFT
    dev = " development, " in c["text"]
    offers = re.search(r"offers (.*) on this GPU", c["text"])
    print("  this machine: shader model %d.%d; the community library offers %s; the helper is a %s build"
          % (sm >> 4, sm & 15, offers.group(1) if offers else "?", "development" if dev else "RELEASE"))
    check(fsr4_ran(c) and sm >= 0x66 and "FSR 4 test" not in c["text"],
          "control, the line at 6.6: FSR 4 runs (\"%s\"), nothing is passed by, the rule does not speak and so NO TEST IS RUN (the build took %d ms)" % (c["provider"], c["ms"]))
    if not dev:
        print("FAIL (live): the helper is a release build, which has no --fsr4_min_sm: build-host without -Release"); return 1
    lever = "--fsr4_min_sm=%d" % (sm + 1)

    # THE TEST RUNS, AND FSR 4 RUNS IN IT
    a = run(community, 1, lever)
    check(fsr4_ran(a) and a["exit"] == 40 and "FSR: FSR 4 test: runs (exit 0x28, " in a["text"] and "DEVELOPMENT: the FSR 4 line is moved" in a["text"],
          "the line one above this machine: the copy runs FSR 4 (exit 40, %d ms), so the helper runs FSR 4 (\"%s\"); the whole build took %d ms"
          % (a["test_ms"], a["provider"], a["ms"]))
    check(a["text"].count("FSR 4 test") == 1 and a["text"].count("warming up") == 1 and a["text"].count("FSR context ready") == 1 and a["text"].count("backend FSR, preset") == 1,
          "the copy wrote none of its own lines into the game's log: one test line, one warm-up, one context, one build line")
    a2 = run(community, 1, lever)
    check(fsr4_ran(a2) and a2["exit"] == 40 and a2["text"].count("FSR 4 test") == 1,
          "the next launch asks again (nothing is kept): the copy runs FSR 4 (%d ms), FSR 4 runs" % a2["test_ms"])
    v4 = run(community, 1, lever, "--fsr_version=4")
    check(fsr4_ran(v4) and "FSR 4 test" not in v4["text"], "fsr_version=4: FSR 4 runs (\"%s\"), named is named, no test" % v4["provider"])
    v3 = run(community, 1, lever, "--fsr_version=3")
    check(v3["ok"] == 1 and major(v3["provider"]) == 3 and "FSR 4 test" not in v3["text"], "fsr_version=3: FSR 3 runs (\"%s\"), no test" % v3["provider"])

    # THE PLANTED FAULTS
    b = run(community, 1, lever, "--fsr4_test_fault=1")
    check(fsr3_ran(b) and b["exit"] == 42 and "FSR: FSR 4 test: exception 0xE0F54001 in " in b["text"] and "FSR: FSR 4 test: does not run (exit 0x2A, " in b["text"],
          "an exception nothing catches in the copy: the copy says where, the helper runs FSR 3 (\"%s\") and never creates FSR 4 (%d ms)" % (b["provider"], b["test_ms"]))
    b2 = run(community, 1, lever)
    check(fsr4_ran(b2) and b2["exit"] == 40, "the next launch, the fault gone: asked again, FSR 4 runs (no stale answer can stand)")
    f = run(community, 1, lever, "--fsr4_test_fault=3")
    check(fsr3_ran(f) and f["exit"] == 41 and "FSR: FSR 4 test: no textures of 1706x960 -> 2560x1440" in f["text"], "a clean failure in the copy (exit 41): FSR 3")
    k = run(community, 1, lever, "--fsr4_test_fault=4")
    check(fsr3_ran(k) and k["exit"] == 0xC0000409 and "FSR: FSR 4 test: does not run (exit 0xC0000409, " in k["text"] and k["text"].count("FSR 4 test") == 1,
          "a copy that dies without a word (0xC0000409): FSR 3")
    check(warned() == "", "none of these wrote the warnings file: each is an answer, and the client's own warning says FSR 3.1 runs")
    s1 = run(community, 1, lever, "--fsr4_test_fault=2", "--fsr4_test_ms=3000")
    w = warned()
    check(fsr3_ran(s1) and s1["exit"] == 0xFFFFFFFF and "FSR: FSR 4 test: did not end (exit 0xFFFFFFFF, " in s1["text"] and 3000 <= s1["test_ms"] < 6000,
          "a copy that never ends is ended after its wait (%d ms of 3000): FSR 3 this session" % s1["test_ms"])
    check(w.count("\n") == 1 and "the test gave no answer in 3 seconds" in w and "fsr_version" in w and "3 for FSR 3.1, or 4 if you know" in w,
          "and the warnings file beside the log says so once, with how to skip the test")

    q = run(community, 0, lever)
    check(q["ok"] == 1 and not (q["flags"] & SKIPPED) and q["text"] == "", "log=0: nothing is written to the log, the test still runs (FSR 4: no bit says passed by)")
    check(left(community) == files0, "NOTHING IS LEFT in the helper's folder by any of these launches: %s" % ", ".join(left(community)))

    official = stage("official", os.path.join(ffx, "amd_fidelityfx_upscaler_dx12.dll"))
    o = run(official, 1, lever)
    check(o["ok"] == 1 and not (o["flags"] & SKIPPED) and major(o["provider"]) == 3 and "automatic runs" not in o["text"] and "FSR 4 test" not in o["text"],
          "AMD's own library, which offers this card no FSR 4: nothing changes (\"%s\"), no test" % o["provider"])

    running = subprocess.run(["powershell", "-NoProfile", "-Command",
                              "@(Get-Process dlssiv-helper -ErrorAction SilentlyContinue | Where-Object { $_.Path -like '%s*' }).Count" % out.replace("'", "''")],
                             capture_output=True, text=True).stdout.strip()
    check(running == "0", "no helper and no copy is left running (%s)" % running)
    # WHAT THE TEST COSTS THE LAUNCH THAT RUNS IT, as measured here: every copy this run started, its wait and its exit code
    print("  the copies: " + "; ".join("%d ms (exit 0x%X, %s)" % t for t in times))
    if fails: print("FAIL (live): %d check(s); the logs are in %s" % (fails, out))
    else: print("PASS (live)"); shutil.rmtree(out, ignore_errors=True)
    return fails

def main():
    src = open(SRC, encoding="utf-8").read()
    t4 = open(T4, encoding="utf-8").read()
    a = src.index(START); b = src.index(END, a)
    body = src[a:b]
    ipc = open(IPC, encoding="utf-8").read()
    line = [l for l in ipc.splitlines() if l.startswith("#define FEED_FSR4_MIN_SM")]
    if len(line) != 1:
        print("FAIL: feed_ipc.h does not define FEED_FSR4_MIN_SM once"); return 1
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "pick.cpp"), "w", encoding="utf-8") as f:
        f.write("// GENERATED by tools/test-fsr-pick/gen.py: FfxMajor, FfxNewer and FsrBelowFsr4 cut from host/dlssiv-helper.cpp\n")
        f.write("#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n")
        f.write(line[0] + "\n")
        f.write(body)
        f.write(BENCH)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars64.bat")
    cmd = 'call "%s" >nul 2>&1 && cl /nologo /EHsc /W4 /WX /std:c++17 /O2 pick.cpp /Fe:pick.exe' % vcvars
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0:
        print(r.stdout[-3000:], r.stderr[-2000:]); print("FAIL: it did not compile"); return 1
    r = subprocess.run([os.path.join(OUT, "pick.exe")], capture_output=True, text=True)
    print(r.stdout, end="")
    rc = r.returncode
    bf = t4_bench(t4, vcvars)
    if bf: print("FAIL: %d check(s) of the start-up test's bench" % bf)
    if rc == 0 and bf == 0: shutil.rmtree(OUT, ignore_errors=True)
    tf = text_checks(src, t4)
    if tf: print("FAIL: %d check(s) of the helper's text" % tf)
    lf = live(vcvars) if "--live" in sys.argv else 0
    return 1 if (rc or bf or tf or lf) else 0

if __name__ == "__main__":
    sys.exit(main())
