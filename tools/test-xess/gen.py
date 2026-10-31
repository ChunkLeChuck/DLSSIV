#!/usr/bin/env python3
r"""X0, THE HEADLESS XeSS SMOKE TEST'S RUNNER (2026-10-27; docs/reports/xess-plan-2026-10-17, PLAN.md X0, UPDATE-2026-10-27.md).

    python tools/test-xess/gen.py            build xess_smoke.exe (beside this file, in %TEMP%\dlssiv-xess), run it on
                                              external\\xess\\libxess.dll, write its report to docs\\reports\\xess-x0\\SMOKE.txt
                                              and hold its verdicts. Exit 0 and PASS, or 1 and FAILED.
    python tools/test-xess/gen.py --bench=N  the cost with N executes (default 300)

A desk tool: no game, no window (Direct3D 12 compute only). The motion and jitter proofs carry their own controls (the negated
arm and the arm with nothing handed must each be worse by a quarter or more), so a proof that cannot separate fails by name."""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-xess")
EXE = os.path.join(OUT, "xess_smoke.exe")
LIB = os.path.join(ROOT, "external", "xess", "libxess.dll")
REPORT = os.path.join(ROOT, "docs", "reports", "xess-x0", "SMOKE.txt")
NOWIN = 0x08000000


def vcvars():
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                  "-property", "installationPath"], creationflags=NOWIN).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars64.bat")


def build():
    os.makedirs(OUT, exist_ok=True)
    src = os.path.join(HERE, "xess_smoke.cpp")
    inc = os.path.join(ROOT, "external", "xess", "inc")
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /I"%s" "%s" /Fe:xess_smoke.exe /Fo:xess_smoke.obj '
          '/link /INCREMENTAL:NO d3d12.lib dxgi.lib' % (vcvars(), inc, src))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    if r.returncode != 0 or not os.path.exists(EXE):
        print(r.stdout[-4000:] + r.stderr[-2000:])
        raise SystemExit("FAILED: xess_smoke.cpp did not build")


WANT = [
    ("LOAD", r"loaded by its full path, every entry point found"),
    ("CREATE", r"xessD3D12CreateContext SUCCESS"),
    ("SETTING", r"is setting \d+"),
    ("FORMAT R16G16B16A16_FLOAT", r"RUNS"),
    ("MOTION", r"previous minus current, in render pixels"),
    ("JITTER", r"as the mod hands DLSS and FSR, jitter scale 1"),
    ("COST", r"median [\d.]+ ms"),
]


def main():
    global REPORT
    bench = 300
    for a in sys.argv[1:]:
        if a.startswith("--bench="): bench = int(a[8:])
        elif a.startswith("--out="): REPORT = os.path.abspath(a[6:])   # (the gate's: the committed report stays as it is)
        else: raise SystemExit("unknown argument " + a)
    if not os.path.isfile(LIB):
        raise SystemExit("FAILED: %s is not there (Intel's SDK: external\\xess\\VERSION.txt says where it came from)" % LIB)
    build()
    os.makedirs(os.path.dirname(REPORT), exist_ok=True)
    r = subprocess.run([EXE, "--lib=" + LIB, "--out=" + REPORT, "--bench=%d" % bench], capture_output=True, text=True,
                       creationflags=NOWIN, timeout=1800)
    text = open(REPORT, "r", encoding="utf-8", errors="replace").read() if os.path.exists(REPORT) else ""
    fails = []
    for tag, pat in WANT:
        m = re.search(r"(?m)^VERDICT %s(?: \([^)]*\))?: (.*)$" % re.escape(tag), text)
        if not m: fails.append("no verdict line for %s" % tag)
        elif not re.search(pat, m.group(1)): fails.append("%s: %s" % (tag, m.group(1)))
        else: print("  ok    %s: %s" % (tag, m.group(1)[:160]))
    for m in re.finditer(r"(?m)^VERDICT FORMAT (B8G8R8A8_UNORM|R8G8B8A8_UNORM).*$", text):
        print("  told  " + m.group(0)[8:][:170])
    if r.returncode not in (0, 1): fails.append("xess_smoke ended with %d" % r.returncode)
    for f in fails: print("  FAIL  " + f)
    print("the report: " + REPORT)
    print("PASS" if not fails else "FAILED")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
