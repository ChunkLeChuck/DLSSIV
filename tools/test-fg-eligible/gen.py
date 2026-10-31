r"""FRAME GENERATION'S PLATFORM TEST AND ITS STATIC CONDITIONS, RUN OVER EVERY CASE (2026-10-05, frame generation phase 2, stage
7; docs/DECISIONS.md "PHASE 2, STAGE 0", A6). source/fg/fg_client.h decides in two pure functions, between its FG PLATFORM
lines, whether a session may ask for frame generation at all: FgxPlatformOk (on Windows the native fences, sync 0, and not
the CPU bridge; SINCE 2026-10-24, round 7, L: under Wine or Proton the game's own semaphores, sync 1, and not the bridge,
by source/fg/fg_linux_rule.h's FgxWinePlatformOk, which the cut includes) and FgxStaticRefusal (the plan's static
conditions in the plan's order, the first that fails). No Linux machine is at this desk, so the rule is held here
against every case:

  - the platform over all twelve cases (Wine or not, sync 0, 1 and 2, the bridge or not): eligible in exactly two,
    Windows native and Wine on the game's own semaphores; the six Windows cases exactly as the base (e329cb62) had them;
  - the conditions' order and their texts, the plan's own sentence for Linux among them, byte for byte;
  - A CONTROL: the same bench on a copy of the cut whose Wine test is taken out must FAIL (a bench that could not fail
    would hold nothing); and THE BASE'S RULE, run by the same bench, must FAIL on the Wine case alone (the fail-first).

    python tools\test-fg-eligible\gen.py        exit code 0 = every check passed and the control failed as it must
"""
import os, re, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "source", "fg", "fg_client.h")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-eligible-%d" % os.getpid())
START = "// FG PLATFORM START"
END = "// FG PLATFORM END"

BENCH = r"""
static int fails = 0, checks = 0;
static void Check(bool ok, const char* what) { checks++; if (!ok) { printf("  FAIL  %s\n", what); fails++; } }
static bool Same(const char* a, const char* b) { return (a == nullptr && b == nullptr) || (a && b && strcmp(a, b) == 0); }
int main()
{
    // THE PLATFORM: twelve cases, eligible in one
    int eligible = 0;
    for (int wine = 0; wine < 2; wine++) for (int sync = 0; sync < 3; sync++) for (int bridge = 0; bridge < 2; bridge++)
    {
        const bool ok = FgxPlatformOk(wine != 0, sync, bridge != 0);
        const bool want = wine ? (sync == 1 && !bridge) : (sync == 0 && !bridge);
        char what[96]; sprintf_s(what, "the platform: wine %d, sync %d, bridge %d must be %s", wine, sync, bridge, want ? "eligible" : "refused");
        Check(ok == want, what);
        if (ok) eligible++;
    }
    Check(eligible == 2, "the platform is eligible in exactly two of the twelve cases: Windows native, and Wine on the game's own semaphores");
    // THE STATIC CONDITIONS, in the plan's order: each alone, then that an earlier one wins over a later one
    Check(Same(FgxStaticRefusal(1, true, 1, 2, 4, -1), nullptr), "fg=1 on Windows native with the shipped settings (pacer -1): may ask");
    Check(Same(FgxStaticRefusal(1, true, 1, 2, 4, 1), nullptr),  "the same with pacer=1: may ask");
    Check(Same(FgxStaticRefusal(0, true, 1, 2, 4, -1), "fg is not 1"), "fg=0: refused as \"fg is not 1\"");
    Check(Same(FgxStaticRefusal(2, true, 1, 2, 4, -1), "fg is not 1"), "fg=2: refused as \"fg is not 1\"");
    Check(Same(FgxStaticRefusal(1, false, 1, 2, 4, -1), "Windows native path only in this version"), "fg=1 off Windows native: the plan's own sentence, byte for byte");
    Check(Same(FgxStaticRefusal(1, true, 0, 2, 4, -1), "threaded is not 1"), "threaded=0");
    Check(Same(FgxStaticRefusal(1, true, 2, 2, 4, -1), "threaded is not 1"), "threaded=2");
    Check(Same(FgxStaticRefusal(1, true, 1, 0, 4, -1), "inject_worker is not 2"), "inject_worker=0");
    Check(Same(FgxStaticRefusal(1, true, 1, 2, 3, -1), "responsiveness is not 4"), "responsiveness=3");
    Check(Same(FgxStaticRefusal(1, true, 1, 2, 4, 0), "pacer is 0"), "pacer=0");
    Check(Same(FgxStaticRefusal(0, false, 0, 0, 0, 0), "fg is not 1"), "fg=0 wins over everything: a session that did not ask says nothing of the platform");
    Check(Same(FgxStaticRefusal(1, false, 0, 0, 0, 0), "Windows native path only in this version"), "the platform wins over the settings: Linux gets the one sentence whatever its file says");
    Check(Same(FgxStaticRefusal(1, true, 0, 0, 0, 0), "threaded is not 1"), "then threaded, before inject_worker, responsiveness and the pacer");
    printf("%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
"""


def cut():
    t = open(SRC, encoding="utf-8").read()
    a = t.find(START); b = t.find(END)
    if a < 0 or b < 0 or b < a: raise SystemExit("FAIL: fg_client.h has no FG PLATFORM START / END pair in that order")
    body = t[a:b]
    if "static bool FgxPlatformOk(" not in body or "static const char* FgxStaticRefusal(" not in body:
        raise SystemExit("FAIL: the cut does not hold both functions")
    return body


def build_run(name, body):
    src = os.path.join(OUT, name + ".cpp")
    rule = os.path.join(ROOT, "source", "fg", "fg_linux_rule.h").replace("\\", "/")
    open(src, "w", encoding="utf-8").write("#include <cstdio>\n#include <cstring>\n#include \"%s\"\n" % rule + body + "\n" + BENCH)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /W3 "%s" /Fe:%s.exe /Fo:%s.obj' % (vcvars, src, name, name),
                       shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0:
        print(r.stdout[-2000:]); raise SystemExit("FAIL: the bench did not compile (%s)" % name)
    g = subprocess.run([os.path.join(OUT, name + ".exe")], capture_output=True, text=True, cwd=OUT)
    return g.returncode, g.stdout


def main():
    os.makedirs(OUT, exist_ok=True)
    try:
        body = cut()
        rc, out = build_run("real", body)
        print(out.rstrip())
        m = re.search(r"(\d+) checks, (\d+) failed", out)
        ok = rc == 0 and m is not None and int(m.group(1)) >= 26 and m.group(2) == "0"
        print("the cut as it is: %s" % ("every case holds" if ok else "FAILED"))
        spoiled = body.replace("    if (wine) return FgxWinePlatformOk(syncMode, bridge);\n", "")
        if spoiled == body: raise SystemExit("FAIL: the control could not spoil the cut (the platform test's line is not as this script reads it)")
        rc2, out2 = build_run("spoiled", spoiled)
        m2 = re.search(r"(\d+) checks, (\d+) failed", out2)
        ctl = rc2 != 0 and m2 is not None and int(m2.group(2)) >= 1
        print("the control (the Wine test taken out): %s" % (("FAILS as it must, %s check(s)" % m2.group(2)) if ctl else "DID NOT FAIL: the bench holds nothing"))
        # THE FAIL-FIRST (2026-10-24, round 7, L): the base's own rule, e329cb62's line, in the same cut. It must fail on the Wine case
        # alone (wine 1, sync 1, no bridge: refused there, eligible here) and on the count, and on nothing else: the Windows cases are
        # the base's
        base = body.replace("    if (wine) return FgxWinePlatformOk(syncMode, bridge);\n    return syncMode == 0 && !bridge;", "    return !wine && syncMode == 0 && !bridge;")
        if base == body: raise SystemExit("FAIL: the base's rule could not be put in the cut (the platform test's lines are not as this script reads them)")
        rc3, out3 = build_run("base", base)
        fl = [l.strip() for l in out3.splitlines() if l.strip().startswith("FAIL")]
        first = rc3 != 0 and fl == ["FAIL  the platform: wine 1, sync 1, bridge 0 must be eligible", "FAIL  the platform is eligible in exactly two of the twelve cases: Windows native, and Wine on the game's own semaphores"]
        print("the base's rule (e329cb62): %s" % ("FAILS FIRST on the Wine case alone, as it must: %s" % "; ".join(fl) if first else "DID NOT FAIL AS IT MUST: %s" % ("; ".join(fl) or "no FAIL line")))
        print("PASS" if ok and ctl and first else "FAIL")
        return 0 if ok and ctl and first else 1
    finally:
        shutil.rmtree(OUT, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
