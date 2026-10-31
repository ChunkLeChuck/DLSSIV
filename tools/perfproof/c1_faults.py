"""C1's planted faults (source/devthread/dt_sbtab.h, dt_res.h: DT_SBM_FAULT(n)), each run and each required to FAIL by the check
that names it.

    python c1_faults.py              build dt_test, run it clean at A and at B (both must PASS), then every fault
    python c1_faults.py 12 27        just those faults (a name with a B, "5B", is that fault at B)

A fault is one broken rule of C1's state-block record, planted through dt_test's fifth argument; in the mod the macro is a
variable nothing writes. A fault that makes the run PASS, or fail by some other check, is a hole in the proof, not a pass:
the case built for that rule cannot see it (docs/reports/c1-design-2026-09-28/03-proof-plan.txt, "prove-all must require each
control to FAIL"). It runs whether or not a GTA IV is open: until 2026-10-03 it stopped if one was, which failed prove-all
whenever the owner played in another folder ("I can decide on my own to enter the game or not if sometihng is running in
the backgorund").
"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "dt_test.exe")
# fault -> what its first error lines must say (any one of them), and the rule it breaks. A key with a B runs the fault at B
# (dt_test's sixth argument, the recorded Apply on); "VALMIS" means the run's own summary must count validation mismatches:
# the layer's threaded=2 compare, not the harness, must have seen the fault (the plan's NC22).
# NC21 (the Capture's fetch past the body gate) has no entry: at B no event forgets an entry while a body is out (the post makes
# every entry the gate guards known first), so the gate is defensive and no case can reach it. Written down, not claimed.
FAULTS = {
    4:  ("the declaration", "the writer applies a null declaration"),
    5:  ("stream", "a Capture takes a stream's offset"),
    6:  ("stream", "the writer keeps a null stream's offset and stride"),
    7:  ("the viewport", "the writer does not lift MaxZ"),
    9:  ("sampler", "a diverted sampler Set is not recorded"),
    10: ("render state", "a Set DXVK refuses is recorded"),
    11: ("would take the recorded path", "the would-be path ignores an open recording"),
    12: ("inside another recording", "a Capture made inside a recording is followed"),
    15: ("has no record", "CreateStateBlock does not overwrite"),
    16: ("H5 (b)", "another thread's Capture does not invalidate"),
    17: ("H5 (c)", "another thread's Set during an armed recording does not taint"),
    18: ("recorded unarmed", "recordings are learned only while armed"),
    19: ("H6", "an unarmed Capture does not invalidate"),
    20: ("the block holds", "a Capture copies an unknown entry as known"),
    23: ("no window", "recordings are learned only at B"),
    25: ("has no record", "a record is retired at every Release"),
    26: ("U1", "no bracket around another thread's tracked Set"),
    27: ("H7 (b)", "a Capture copies with another thread's Set in flight"),
    28: ("recheck", "no recheck after the Capture's command"),
    29: ("U5", "an arm and a disarm inside one recording are not seen"),
    30: ("H5 (f)", "another thread's End of the producer's recording runs no barrier"),
    31: ("U3", "the render thread named inside a recording is not looked at again"),
    32: ("U6", "the arm is not held off while a recording is open"),
    33: ("the worker's body Sets", "the worker's body Sets are bracketed as another thread's (R8)"),
    34: ("the body declaration check counted", "a slot the body binds is declared free (the declarations measured)"),
    35: ("after an idle barrier and a late publish", "the producer's answer to an idle barrier goes direct without waiting for what it published"),
    36: ("C1-F across another thread", "C1-F learns an FVF's object while another thread's declaration landed in the Get (C1-R2)"),
    37: ("the 20 ms net", "the worker's catch-up signal to a drain is dropped (wake-every-64, the final audit 2026-10-01; dt_core.h DT_WAKE_FAULT)"),
    "1B": ("differs", "the recorded Apply runs beside the ring"),
    "2B": ("sweep", "the recorded Apply leaves the shadow as it was"),
    "3B": ("H1", "the recorded Apply forgets its entries instead of writing them"),
    "24B": ("H10", "a failed recorded Apply is counted as a resource method, and nothing re-reads"),
    "5B": ("VALMIS", "NC22: NC5 at B, seen by the layer's own threaded=2 compare"),
}


def run(args):
    r = subprocess.run([EXE] + [str(a) for a in args], capture_output=True, text=True)
    return r.returncode, r.stdout


def key(a):
    return a if isinstance(a, str) else a


def main():
    picks = [(int(a) if a.isdigit() else a) for a in sys.argv[1:]] or sorted(FAULTS, key=lambda k: (isinstance(k, str), str(k).zfill(4)))
    if len(sys.argv) == 1:
        b = subprocess.run([sys.executable, os.path.join(HERE, "run_proof.py"), "dt_test", "2"], capture_output=True, text=True)
        if b.returncode != 0:
            print("the clean run at A did not PASS (exit %d):\n%s" % (b.returncode, b.stdout[-2000:])); return 1
        rc, out = run([2, 64, 200000, 60000, 0, 1])
        if rc != 0:
            print("the clean run at B did not PASS (exit %d):\n%s" % (rc, out[-2000:])); return 1
        print("clean: PASS at A and at B")
    bad = 0
    for n in picks:
        want, rule = FAULTS[n]
        atB = isinstance(n, str)
        num = int(n[:-1]) if atB else n
        rc, out = run([2, 64, 200000, 60000, num] + ([1] if atB else []))
        errs = [l.strip() for l in out.splitlines() if l.strip().startswith("ERROR:")][:5]
        if want == "VALMIS":
            import re
            m = re.search(r"FAILED BECAUSE: \d+ error\(s\) counted, (\d+) validation mismatch", out)
            named = bool(m and int(m.group(1)) > 0)
            if named: errs = ["the layer's own compare counted %s validation mismatch(es)" % m.group(1)] + errs
        else:
            named = any(want in e for e in errs)
        ok = rc != 0 and named
        print("NC%-4s %s  (%s)%s" % (n, "FAILS BY NAME" if ok else "HOLE", rule, "" if ok else ": exit %d, first error: %s" % (rc, errs[0] if errs else "none")))
        if ok and errs: print("       %s" % errs[0][:200])
        bad += not ok
    print("%d of %d planted faults caught by name" % (len(picks) - bad, len(picks)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
