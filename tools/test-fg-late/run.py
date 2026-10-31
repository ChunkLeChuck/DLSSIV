r"""THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT, AT A DESK (2026-10-14; source\fg\fg_machine.h, "THE HELPER'S ANSWER ON A CARD THAT IS
THE LIMIT"; source\fg\fg_xport.h, FgxCardClass and "THE ORDER'S OWN FENCE"; tools\test-fg-pipe\fgpipe_cost.h, "THE THREE MENDS AT A
DESK"). The fake client beside the real helper, on the real card, with no game and NO WINDOW. What the harness cannot
show (tools\perfproof\dt_fg_late.h runs the machine over a fake card): a real queue's order, a real scheduler, real costs.

THE SESSIONS (3840x2160 FP16, Quality guides, a baseline frame of 9.1 ms, one frame always in flight, every pair
dispatched; "this side first" is this program's own scheduler class raised, the stand-in for a game that outranks the
helper, which is what the legs of 2026-10-13 read):
  OV    THE COPY OVER A DISPATCH IN FLIGHT, this side first: every image read back in the queue's order sits on the half-way
        picture of the two frames it was made from, and some of them belong to pairs whose next pair copied with the
        dispatch unanswered
  OVN   the same with both programs at the normal class
  OVH   the same with the helper asleep before every 64th dispatch (its own --fgslow_d=30) and EVERY frame's image read
        back: a dispatch held, the next pair's copies submitted at once, and that pair's image among those read
  OVC   ITS CONTROL, this side first: the copies submitted AHEAD of the frame's home: images caught made from the wrong picture
  CA    THE HELPER'S PLACE ON THE CARD through the product's own line (FgxCardClass on the helper's process): it takes, and K
        over 4.5 ms is none
  CB    the fault: this side first, nothing asked: K over 4.5 ms on nearly every pair (arm "wait 0" of the next two too)
  O1    THE ORDER WAIT, THE PRODUCT'S (a fence of this side's own, released from the CPU), this side first: off against on
  O2    the direct form (a wait on the helper's own answer fence, which the product does not make), this side first:
        what the release's hop through this side costs is O1's frame less O2's
  ON    the product's order wait with both programs at the normal class (where the two share the card)
  H1 H2 H3   THE ORDER WAIT WITH THE HELPER HELD (asleep 4 s before its 64th dispatch): what is queued behind the wait does not
        retire; released from the CPU it does, with the helper still asleep, SUSPENDED, KILLED
  OLD   THE PARTS THAT WERE THERE BEFORE, short: the idle part (every part of a pair waited for) and the loaded part, whose
        hand's copies are the same lines moved into a piece of their own: the client's own proofs must all pass
  SOAK  (only with --soak) the helper at the top class for three minutes under load, a real upscaler evaluating every
        frame: no device lost, no frame refused, the helper alive at its end (the caution fg_xport.h reads of that class)

    python tools\test-fg-late\run.py --helper=PATH --lib=PATH [--only=OV,O1] [--soak] [--frames=N]
        --helper   a DEVELOPMENT dlssiv-helper.exe (default: this tree's host\dlssiv-helper.exe)
        --lib      dlssiv-fg.dll, the mod's own library (default: this tree's obj\dlssiv-fg\dlssiv-fg.dll)
        --reread   run nothing: judge again the sessions whose output the last run left in %TEMP%\dlssiv-test-fg-late
                   (for a judge of this file's that was wrong, not for a session that failed: the card is not used)
NOTHING IS LEFT BEHIND: a session that does not end is ended by its own process, and a helper of this tool's own that is
still there is ended by its pid. NO SESSION STARTS WHILE ANOTHER GPU USER RUNS (GTAIV.exe, another bench's helper or fake
client, a proof's rig): it says so and stops. Never a clock lock; no setting of the system (a process's scheduler class
ends with the process). NOT part of tools\prove-all.py: it takes the card for minutes.
exit code 0 = every session held what it must. The last line is PASS or FAIL.
"""
import hashlib, os, re, shutil, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-late")
ARGS = sys.argv[1:]
opt = lambda name, d=None: next((a.split("=", 1)[1] for a in ARGS if a.startswith("--%s=" % name)), d)
HELPER = opt("helper", os.path.join(ROOT, "host", "dlssiv-helper.exe"))
LIB = opt("lib", os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll"))
ONLY = opt("only")
FRAMES = int(opt("frames", "400"))
SOAK = "--soak" in ARGS
REREAD = "--reread" in ARGS
NOWIN = subprocess.CREATE_NO_WINDOW
EXE = "fgpipe-late.exe"
passes, fails = [], []


def proof(ok, what):
    (passes if ok else fails).append(what)
    print("  %s  %s" % ("PASS" if ok else "FAIL", what))
    return ok


def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""): h.update(b)
    return h.hexdigest()


def processes():
    out = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True, creationflags=NOWIN).stdout
    return [(m.group(1), int(m.group(2))) for m in (re.match(r'"([^"]+)","(\d+)"', l) for l in out.splitlines()) if m]


def gpu_users():
    names = ("gtaiv.exe", "dlssiv-helper.exe")
    pre = ("fgpipe", "fgcost", "vklines", "dt_fg_test", "perfproof")
    return [n for n, p in processes() if n.lower() in names or n.lower().startswith(pre)]


def vcvars32():
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], creationflags=NOWIN).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")


def prepare():
    if os.path.isdir(OUT): shutil.rmtree(OUT)
    bare, own = os.path.join(OUT, "bare"), os.path.join(OUT, "own")
    os.makedirs(bare); os.makedirs(own)
    if not proof(os.path.exists(HELPER) and b" development, backend" in open(HELPER, "rb").read(), "the helper is a development build: %s" % HELPER): return None
    if not proof(os.path.exists(LIB), "the mod's own library is there: %s" % LIB): return None
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /DWIN32 /I"%s" /I"%s" /I"%s" "%s" /Fe:%s /Fo:fgpipe-late.obj /link /INCREMENTAL:NO'
          % (vcvars32(), os.path.join(ROOT, "external", "vulkan"), os.path.join(ROOT, "source", "transport"), os.path.join(ROOT, "source", "fg"),
             os.path.join(ROOT, "tools", "test-fg-pipe", "fgpipe.cpp"), EXE))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    if not proof(r.returncode == 0 and "warning C" not in r.stdout, "the fake client compiles cleanly, 32-bit, under this tool's own name (%s)" % EXE):
        print(r.stdout[-3000:]); return None
    shutil.copy2(HELPER, os.path.join(bare, "dlssiv-helper.exe"))
    for n in ("amd_fidelityfx_loader_dx12.dll", "amd_fidelityfx_upscaler_dx12.dll"):
        shutil.copy2(os.path.join(ROOT, "external", "ffx", n), os.path.join(bare, n))
    shutil.copy2(os.path.join(ROOT, "external", "ngx", "nvngx_dlss.dll"), os.path.join(bare, "nvngx_dlss.dll"))
    shutil.copyfile(LIB, os.path.join(own, "dlssiv-fg.dll"))
    print("  the helper %d bytes, sha256 %s; the library %d bytes, sha256 %s" % (os.path.getsize(HELPER), sha(HELPER)[:16], os.path.getsize(LIB), sha(LIB)))
    return bare, os.path.join(own, "dlssiv-fg.dll")


def reread(sid, title):
    d = os.path.join(OUT, sid)
    if not os.path.exists(os.path.join(d, "out.txt")): return None
    print("SESSION %s: %s (NOT RUN NOW: its saved output is read again)" % (sid, title))
    out = open(os.path.join(d, "out.txt"), encoding="utf-8").read()
    hl = os.path.join(d, "helper.log")
    hlog = open(hl, encoding="utf-8", errors="replace").read() if os.path.exists(hl) else ""
    return {"sid": sid, "out": out, "rc": 0 if out.rstrip().endswith("PASS") else 1, "secs": os.path.getmtime(os.path.join(d, "out.txt")) - os.path.getctime(d), "helper": hlog,
            "fails": [l.strip() for l in out.splitlines() if l.strip().startswith("FAIL")],
            "kv": [dict(kv.split("=", 1) for kv in l.split()[1:] if "=" in kv) for l in out.splitlines() if l.strip().startswith(("cost-ord ", "cost-over ", "cost-held "))],
            "kind": [l.split()[0] for l in out.splitlines() if l.strip().startswith(("cost-ord ", "cost-over ", "cost-held "))]}


def session(sid, title, fake, helper, bare, lib, timeout=420):
    if REREAD: return reread(sid, title)
    print("SESSION %s: %s" % (sid, title))
    others = gpu_users()
    if others:
        proof(False, "%s: ANOTHER GPU USER IS RUNNING (%s): this session was NOT RUN" % (sid, ", ".join(sorted(set(others))))); return None
    d = os.path.join(OUT, sid); os.makedirs(d, exist_ok=True)
    args = ["--fake-fg=40", "--fake-cost-hud=1", "--fake-cost-guides=1", "--fake-cost-oload=9100", "--fake-cost-ogap=2000"] + fake
    args += ["--fg=1", "--fgown=" + lib, "--fgown_sha=" + sha(lib), "--fake-want-fg=3", "--fake-cost-out=" + os.path.join(d, "client.csv")] + helper
    t0 = time.time()
    p = subprocess.Popen([os.path.join(OUT, EXE), os.path.join(bare, "dlssiv-helper.exe"), os.path.join(d, "helper.log")] + args,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=bare, creationflags=NOWIN)
    time.sleep(0.5)
    mine = [pid for n, pid in processes() if n.lower() == "dlssiv-helper.exe"]
    try:
        out, _ = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill(); out, _ = p.communicate(); out += "\n  FAIL  the fake client did not finish in %d s and was ended by its own process\n" % timeout
    time.sleep(0.3)
    for n, pid in processes():
        if n.lower() == "dlssiv-helper.exe" and pid in mine:
            subprocess.run(["taskkill", "/pid", str(pid), "/f"], capture_output=True, creationflags=NOWIN)
    open(os.path.join(d, "out.txt"), "w", encoding="utf-8").write(out)
    hl = os.path.join(d, "helper.log")
    hlog = open(hl, encoding="utf-8", errors="replace").read() if os.path.exists(hl) else ""
    s = {"sid": sid, "out": out, "rc": p.returncode, "secs": time.time() - t0, "helper": hlog,
         "fails": [l.strip() for l in out.splitlines() if l.strip().startswith("FAIL")],
         "kv": [dict(kv.split("=", 1) for kv in l.split()[1:] if "=" in kv) for l in out.splitlines() if l.strip().startswith(("cost-ord ", "cost-over ", "cost-held "))],
         "kind": [l.split()[0] for l in out.splitlines() if l.strip().startswith(("cost-ord ", "cost-over ", "cost-held "))]}
    for l in out.splitlines():
        if l.strip().startswith(("cost-ord ", "cost-over ", "cost-held ")) or "IDLE" in l or "LOADED" in l or "THE OVER PART" in l or "THE HELD PART" in l or "scheduler's top class" in l or "class is raised" in l or "given back" in l:
            print("    " + l.strip()[:420])
    return s


def mids(s, wait, key):
    v = sorted(float(c[key]) for c in s["kv"] if c.get("wait") == str(wait))
    return v[len(v) // 2] if v else -1.0


def main():
    print("THE HELPER'S ANSWER ON A CARD THAT IS THE LIMIT, AT A DESK: the fake client beside the real helper, no game, no window")
    st = (None, None) if REREAD else prepare()
    if not st: print("FAIL"); return 1
    bare, lib = st
    order = ["--fake-cost-order=%d" % FRAMES, "--fake-cost-oreg=4"]
    over = ["--fake-cost-over=%d" % FRAMES]
    want = lambda sid: ONLY is None or sid in ONLY.split(",")
    S = {}
    plan = [
        ("OV",  "the copy over a dispatch in flight, this side first", over + ["--fake-cost-sclass=5"], []),
        ("OVN", "the same, both programs at the normal class", over, []),
        ("OVH", "the same, the helper asleep 30 ms before every 64th dispatch, every frame's image read back", over + ["--fake-cost-overall=1"], ["--fgslow_d=30"]),
        ("OVC", "ITS CONTROL: the copies ahead of the frame's home, this side first", over + ["--fake-cost-sclass=5", "--fake-cost-overctl=1"], []),
        ("CA",  "the helper's place on the card through the product's own line", order + ["--fake-cost-hclass=5", "--fake-cost-ord=1"], []),
        ("O1",  "the order wait, the product's, this side first", order + ["--fake-cost-sclass=5", "--fake-cost-ord=1"], []),
        ("O2",  "the direct form, this side first", order + ["--fake-cost-sclass=5", "--fake-cost-ord=2"], []),
        ("ON",  "the product's order wait, both programs at the normal class", order + ["--fake-cost-ord=1"], []),
        ("OLD", "the parts that were there before: the idle part and the loaded part, short", ["--fake-cost-idle=1", "--fake-cost-pairs=48", "--fake-cost-arms=P,PC,F,FR,F1", "--fake-cost-load=9100", "--fake-cost-quads=1", "--fake-cost-half=64", "--fake-cost-larms=NULL,PC,F,LK"], []),
        ("H1",  "the order wait with the helper asleep", ["--fake-cost-held=1"], ["--fgslow_d=4000"]),
        ("H2",  "the order wait with the helper SUSPENDED", ["--fake-cost-held=2"], ["--fgslow_d=4000"]),
        ("H3",  "the order wait with the helper KILLED", ["--fake-cost-held=3"], ["--fgslow_d=4000"]),
    ]
    if SOAK:
        plan.append(("SOAK", "the helper at the top class for three minutes under load, a real upscaler evaluating", ["--fake-cost-order=2800", "--fake-cost-oreg=4", "--fake-cost-hclass=5", "--fake-cost-ord=1", "--fake-cost-upscale=1"], []))
    for sid, title, fake, helper in plan:
        if not want(sid): continue
        s = session(sid, title, fake, helper, bare, lib, timeout=600 if sid == "SOAK" else 300)
        if s: S[sid] = s
    print("WHAT THEY SAY")
    for sid in ("OV", "OVN", "OVH"):
        s = S.get(sid)
        if not s: continue
        c = next((c for c, k in zip(s["kv"], s["kind"]) if k == "cost-over"), None)
        held = sid != "OVH" or (c is not None and int(c["held_over"]) >= 1 and c["held_over"] == c["held_right"])
        if c is not None and sid == "OVH": print("    OVH: %s of its images were copied over a dispatch that then took more than 25 ms to answer (the helper asleep ahead of it): %s of them right" % (c["held_over"], c["held_right"]))
        proof(c is not None and held and not s["fails"] and int(c["wrong_over"]) == 0 and int(c["wrong_plain"]) == 0 and int(c["over"]) >= 1 and int(c["no_ref"]) == 0 and int(c["ref_over"]) == 0,
              "%s: %s images read back in the queue's order, %s of them of pairs whose next pair copied with the dispatch unanswered: every one measures what the same pair of frames measured with the hand waiting on the CPU (%s and %s right, %s and %s wrong; off by %s of the change between the two frames at the mean, %s at the most, the bound 0.01; the reference: %s images of %s pairs)" % (
                  sid, c and c["observed"], c and c["over"], c and c["right_over"], c and c["right_plain"], c and c["wrong_over"], c and c["wrong_plain"], c and c["off_mean"], c and c["off_worst"], c and c["ref_observed"], c and c["ref_pairs"]) if c else "%s: no cost-over line (%s)" % (sid, "; ".join(s["fails"])[:300]))
    s = S.get("OVC")
    if s:
        c = next((c for c, k in zip(s["kv"], s["kind"]) if k == "cost-over"), None)
        proof(c is not None and int(c["wrong_over"]) + int(c["wrong_plain"]) >= 1 and int(c["ref_over"]) == 0 and int(c["ref_differ"]) == 0,
              "OVC, THE CONTROL: with the copies ahead of the home %s of %s images do NOT measure what the same pair of frames measured in the reference (off by %s of the change at the mean, %s at the least, %s at the most; the bound 0.01): the order is what holds the picture" % (
                  c and (int(c["wrong_over"]) + int(c["wrong_plain"])), c and c["observed"], c and c["off_mean"], c and c["off_least"], c and c["off_worst"]) if c else "OVC: no cost-over line (%s)" % "; ".join(s["fails"])[:300])
    line = lambda sid, w: "K median %.0f us, 99th %.0f, over 4.5 ms %.0f of %.0f pairs; the frame %.1f us; the wait's submit %.1f us, its signal %.1f us" % (
        mids(S[sid], w, "k_med_us"), mids(S[sid], w, "k_p99_us"), mids(S[sid], w, "k_over4500"), mids(S[sid], w, "pairs"), mids(S[sid], w, "period_us"), mids(S[sid], w, "wait_submit_us"), mids(S[sid], w, "signal_us"))
    if "CA" in S:
        s = S["CA"]
        took = "scheduler's top class" in s["out"] and not any("top class" in f for f in s["fails"])
        proof(took and not s["fails"] and mids(s, 0, "k_over4500") == 0, "CA, the helper's place asked through the product's own line: it %s; with no order wait: %s" % ("took" if took else "DID NOT TAKE", line("CA", 0)))
    if "O1" in S:
        s = S["O1"]
        proof(not s["fails"] and mids(s, 0, "k_over4500") >= 0.9 * mids(s, 0, "pairs") and mids(s, 1, "k_over4500") == 0,
              "O1, this side first: WITH NO ORDER WAIT (the legs' fault): %s. WITH THE PRODUCT'S ORDER WAIT: %s" % (line("O1", 0), line("O1", 1)))
        print("    AN ARMED PAIR'S COST where this side outranks the helper: %+.0f us a frame (the frame with the wait less the frame without)" % (mids(s, 1, "period_us") - mids(s, 0, "period_us")))
    if "O2" in S and "O1" in S:
        proof(not S["O2"]["fails"] and mids(S["O2"], 2, "k_over4500") == 0, "O2, the direct form, this side first: %s" % line("O2", 2))
        print("    WHAT THE RELEASE'S HOP THROUGH THIS SIDE COSTS: %+.0f us a frame (the product's form %.1f, the direct form %.1f)" % (
            mids(S["O1"], 1, "period_us") - mids(S["O2"], 2, "period_us"), mids(S["O1"], 1, "period_us"), mids(S["O2"], 2, "period_us")))
    if "ON" in S:
        s = S["ON"]
        proof(not s["fails"] and mids(s, 1, "k_over4500") == 0, "ON, both programs at the normal class: with no order wait: %s. With the product's: %s" % (line("ON", 0), line("ON", 1)))
        print("    AN ARMED PAIR'S COST where the two share the card: %+.0f us a frame" % (mids(s, 1, "period_us") - mids(s, 0, "period_us")))
    if "OLD" in S:
        s = S["OLD"]
        n = len([l for l in s["out"].splitlines() if l.strip().startswith("PASS")])
        proof(not s["fails"] and s["rc"] == 0 and "IDLE:" in s["out"] and "LOADED," in s["out"], "OLD, the idle part and the loaded part as they were: the client's own proofs all pass (%d of them; failed: %s)" % (n, "; ".join(s["fails"])[:300] or "none"))
    for sid, how in (("H1", "asleep"), ("H2", "suspended"), ("H3", "killed")):
        s = S.get(sid)
        if not s: continue
        c = next((c for c, k in zip(s["kv"], s["kind"]) if k == "cost-held"), None)
        proof(c is not None and not s["fails"] and c["standing"] == "1" and c["retired"] == "1" and float(c["retire_ms"]) < 100.0,
              "%s, the helper %s with an order wait standing: what was queued behind the wait had not retired; the fence signalled from the CPU, it retired %s ms later (fgOut %s where pair %s wants %s)" % (
                  sid, how, c and c["retire_ms"], c and c["fgout_before"], c and c["pair"], c and c["wanted"]) if c else "%s: no cost-held line (%s)" % (sid, "; ".join(s["fails"])[:300]))
    if "SOAK" in S:
        s = S["SOAK"]
        lost = [l.strip()[:200] for l in s["helper"].splitlines() if "removed" in l.lower() or "DEVICE_REMOVED" in l or "device lost" in l.lower() or "hung" in l.lower()]
        ended = "the helper ended by itself within 20 s of the pipe closing (exit code 0)" in s["out"]
        frames = re.search(r"pipe closed by the game after (\d+) frames \((\d+) failed\)", s["helper"])
        disp = re.search(r"'G' (\d+), dispatched (\d+), answered with no frame (\d+)", s["helper"])
        proof(not s["fails"] and s["rc"] == 0 and not lost and ended and frames is not None and frames.group(2) == "0" and disp is not None and disp.group(1) == disp.group(2),
              "SOAK: %.0f s with the helper at the top class under load, a real upscaler evaluating every frame: the client's proofs %s; the helper ended by itself with exit code 0 (%s); its log: %s frames, %s failed; %s dispatched of %s asked; lines of a lost device %d; K with no order wait: %s" % (
                  s["secs"], "all pass" if not s["fails"] else "FAIL: " + "; ".join(s["fails"])[:200], "yes" if ended else "NO", frames.group(1) if frames else "?", frames.group(2) if frames else "?",
                  disp.group(2) if disp else "?", disp.group(1) if disp else "?", len(lost), line("SOAK", 0)))
    left = [] if REREAD else [n for n, p in processes() if n.lower() == EXE]
    proof(not left, "nothing of this tool's is left running")
    print("%d passed, %d failed" % (len(passes), len(fails)))
    print("FAIL" if fails else "PASS")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
