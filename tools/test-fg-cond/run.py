#!/usr/bin/env python3
"""The bench of frame generation's conditions table and of its planned generation time.

    python tools/test-fg-cond/run.py

Compiles cond.cpp (beside this file) with the mod's own x86 toolchain into %TEMP%, and runs it twice:
  1. the bench: every combination of the session's facts through source/fg/fg_cond.h against an oracle written a second
     time, every row's one line rendered and held to its rules, the lifts, and source/fg/fg_plan.h's quantile. It must
     exit 0.
  2. THE CONTROL, WHICH MUST FAIL: the same checks with one constant of the table and one of the plan moved. It must
     exit 1 and say that all three planted faults were caught (phase 5 added the law's: a dwell of 2 and a back-off capped
     at 600; round 3 of its round, G4, the true fill's: the oracle's top at 0.90 of the refresh). A bench that could not
     fail proves nothing.
  The law's checks (phase 5, P1): source/fg/fg_cond.h's FgDrive*, the armed product's own driver, against the brief's numbers.
The headers are included unchanged: what the bench runs is what the mod compiles.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-cond")


def vcvars():
    root = os.environ.get("DLSSIV_VSROOT")
    if not root:
        vw = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), "Microsoft Visual Studio", "Installer", "vswhere.exe")
        if os.path.exists(vw):
            r = subprocess.run([vw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                "-property", "installationPath"], capture_output=True, text=True)
            if r.returncode == 0 and r.stdout.strip():
                root = r.stdout.strip().splitlines()[0].strip()
    if not root:
        root = r"C:\Program Files\Microsoft Visual Studio\2022\Community"
    p = os.path.join(root, "VC", "Auxiliary", "Build", "vcvars32.bat")
    if not os.path.exists(p):
        raise SystemExit("vcvars32.bat not found at " + p)
    return p


POLICY_PLANT = ("FgCondK k = { 480, 1030, 670, 380,", "FgCondK k = { 480, 1030, 670, 450,")   # (the control: the floor at 45, the old one)


# (round 9, L1) THE SCREEN'S RATE IS FED THE WAITER'S OWN SAMPLES: the three lines dt_fg_test's FX28 stands in for (it does not compile
# fg_arm_life.h or DLSS-IV.cpp), held as text scoped to their functions, each with a control symbol that must be there, and a control per line
# (that line taken out must fail). The memory rule: a stand-in proves the model; the writer is read and held.
WIRING = [
    ("source/DLSS-IV.cpp", "static void LatWaiterLoop()", "wP[wn] = (float)LatMs(tIn - tInPrev);", "FGA(FgpArmWindowStep(latWindows, mP, mAge, mH, wP, LAT_WIN);)",
     "the waiter fills wP with the injections' intervals and hands the window to FgpArmWindowStep"),
    ("source/fg/fg_arm_life.h", "static void FgpArmWindowStep(LONG latWin, float mP, float mAge, float mH, const float* wP, int wn)",
     "fgpFixWin[1] = FgCondFixedMeanUs(wP, wn, hz); fgpFixWin[0] = hz > 0 ? wn : 0;", "FgpCondStep(nullptr, 0, mP);",
     "FgpArmWindowStep reads the window's capped mean ahead of the conditions' step"),
    ("source/fg/fg_arm_life.h", "static void FgpCondRead(FgCondIn* out, IDirect3DDevice9* dev, int bbVkFormat, float mPms)",
     "in.freeFpsX10 = FgCondFixedFreeX10(in, fgpFixWin[0], fgpFixWin[1]);", "in.predFpsX10 = in.engaged ? 0 :",
     "FgpCondRead hands it to FAST standing by, ahead of the prediction"),
]


def wiring_holds(text, head, line, ctl):
    i = text.find(head)
    if i < 0: return False, "the function is not found"
    j = text.find("\n}", i)
    body = text[i:j if j > 0 else len(text)]
    c = body.find(ctl)
    if c < 0: return False, "its control symbol is not in it"
    k = body.find(line)
    if k < 0: return False, "the line is not in it"
    if k > c: return False, "the line stands behind its control symbol"
    return True, "held"


def wiring_check():
    root = os.path.normpath(os.path.join(HERE, "..", ".."))
    out = []
    for rel, head, line, ctl, what in WIRING:
        text = open(os.path.join(root, rel), "rb").read().decode("utf-8", "replace")
        ok, why = wiring_holds(text, head, line, ctl)
        cok, cwhy = wiring_holds(text.replace(line, "", 1), head, line, ctl)
        print("  the screen's rate (round 9, L1): %s (%s): %s; CONTROL, the line taken out: %s" % (what, rel, why, cwhy))
        if not ok: out.append("THE SCREEN'S RATE'S WIRING: %s: %s" % (what, why))
        if cok: out.append("THE CONTROL of the screen's rate's wiring passed with the line taken out (%s): the check cannot see it" % what)
    return out


def policy_check():
    """(round 6, G) THE POLICY'S ROWS (policy-rows.txt, written from docs/FG-POLICY-AND-PLAN-1.1.md and the plan, never from the code):
    policy.cpp asks fg_cond.h each row's session. It must hold every row not marked PENDING; and THE CONTROL, a copy of fg_cond.h in
    %TEMP% with the floor at 45, must fail a row, or the rows could not see a wrong table."""
    fg = os.path.normpath(os.path.join(HERE, "..", "..", "source", "fg"))
    rows = os.path.join(HERE, "policy-rows.txt")
    text = open(os.path.join(fg, "fg_cond.h"), "rb").read().decode("utf-8")
    plant = os.path.join(OUT, "policy-plant")
    os.makedirs(plant, exist_ok=True)
    out = []
    if text.count(POLICY_PLANT[0]) != 1:
        return ["the policy rows' control: its line is in fg_cond.h %d time(s), not once" % text.count(POLICY_PLANT[0])]
    with open(os.path.join(plant, "fg_cond.h"), "wb") as f:
        f.write(text.replace(POLICY_PLANT[0], POLICY_PLANT[1]).encode("utf-8"))
    res = {}
    for tag, inc in (("policy", fg), ("policy-ctl", plant)):
        exe, obj = os.path.join(OUT, tag + ".exe"), os.path.join(OUT, tag + ".obj")
        bat = os.path.join(OUT, tag + ".cmd")
        with open(bat, "w", newline="\r\n") as f:
            f.write('@echo off\n"%s" >nul 2>&1 && cl /nologo /O2 /EHsc /std:c++17 /W3 /I"%s" /I"%s" "%s" /Fe:"%s" /Fo:"%s"\n' % (vcvars(), inc, fg, os.path.join(HERE, "policy.cpp"), exe, obj))
        r = subprocess.run(["cmd.exe", "/c", bat], capture_output=True, text=True, errors="replace")
        if r.returncode != 0 or not os.path.exists(exe):
            print((r.stdout or "") + (r.stderr or ""))
            return ["policy.cpp did not compile (%s)" % tag]
        res[tag] = subprocess.run([exe, rows], capture_output=True, text=True, errors="replace")
    a, c = res["policy"], res["policy-ctl"]
    print("\n".join(l for l in a.stdout.splitlines() if not l.startswith("  holds:")))
    if a.returncode != 0: out.append("the policy's rows did not hold (exit %d)" % a.returncode)
    cf = [l.strip() for l in c.stdout.splitlines() if l.startswith("  FAIL")]
    if c.returncode != 1 or not cf: out.append("THE POLICY ROWS' CONTROL (the floor at 45) did not fail: the rows cannot see a wrong table")
    else: print("  the policy rows' control (the floor at 45) fails as it must: %d row(s), the first: %s" % (len(cf), cf[0][:160]))
    # (round 7, G) THE OVERRUN ROWS' CONTROL: the tree's own build, the adapter handing the overrun no L ("noover"): the row that stands
    # down by its bulk alone ("its bulk over two refreshes") must then fail, or the overrun's rows read nothing
    o = subprocess.run([os.path.join(OUT, "policy.exe"), rows, "noover"], capture_output=True, text=True, errors="replace")
    of = [l.strip() for l in o.stdout.splitlines() if l.startswith("  FAIL") and "its bulk over two refreshes" in l]
    if o.returncode != 1 or not of: out.append("THE OVERRUN ROWS' CONTROL (no L read) did not fail the bulk's row: the overrun's rows cannot see an overrun never read")
    else: print("  the overrun rows' control (no L read) fails as it must: %s" % of[0][:160])
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    if "--policy" in sys.argv:                                     # (round 6, G: the policy's rows alone, seconds; the gate's step)
        fails = policy_check()
        for f in fails:
            print("  FAIL: " + f)
        print("FAILED" if fails else "PASS")
        return 1 if fails else 0
    exe, obj = os.path.join(OUT, "cond.exe"), os.path.join(OUT, "cond.obj")
    for f in (exe, obj):
        if os.path.exists(f):
            os.remove(f)
    bat = os.path.join(OUT, "build.cmd")
    with open(bat, "w", newline="\r\n") as f:
        f.write('@echo off\n"%s" >nul 2>&1 && cl /nologo /O2 /EHsc /std:c++17 /W3 "%s" /Fe:"%s" /Fo:"%s"\n' % (vcvars(), os.path.join(HERE, "cond.cpp"), exe, obj))
    r = subprocess.run(["cmd.exe", "/c", bat], capture_output=True, text=True, errors="replace")
    if r.returncode != 0 or not os.path.exists(exe):
        print((r.stdout or "") + (r.stderr or ""))
        print("cond.cpp did not compile\nFAILED")
        return 1
    warns = (r.stdout or "").count("warning C")
    if warns:
        print((r.stdout or "").strip())
    fails = []
    a = subprocess.run([exe], capture_output=True, text=True, errors="replace")
    print(a.stdout.rstrip())
    if a.returncode != 0 or not a.stdout.rstrip().endswith("PASS"):
        fails.append("the bench did not pass (exit %d)" % a.returncode)
    c = subprocess.run([exe, "control"], capture_output=True, text=True, errors="replace")
    tail = [l for l in c.stdout.splitlines() if l.startswith("THE CONTROL") or "MISMATCH" in l or "NOT IT" in l]
    print("\n".join("  " + l.strip()[:300] for l in tail[:6]))
    if c.returncode != 1 or "all three were caught" not in c.stdout:   # (round 8's deep strip: the true fill's planted top, the fourth, went with the fill)
        fails.append("THE CONTROL did not fail as it must (exit %d): the bench cannot see a wrong table" % c.returncode)
    else:
        print("  the control failed as it must (exit 1)")
    fails += policy_check()                                        # (round 6, G: the policy's rows, policy-rows.txt)
    fails += wiring_check()                                        # (round 9, L1: the screen's rate fed the waiter's own samples)
    for f in fails:
        print("  FAIL: " + f)
    print("FAILED" if fails else "PASS")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
