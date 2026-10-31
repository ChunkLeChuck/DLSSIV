r"""THE CAPTURE AND THE REPLAY, PROVED AT A DESK WITHOUT THE GAME (2026-10-10, frame generation phase 3, after first light;
docs\reports\fg-3\after-firstlight-2026-10-10\BRIEF-capture.md, parts A to C).

The development helper can save, on a leg in the game, short runs of consecutive frames with everything the frame
generation library was given and the image it gave back (host\fg_cap.h, --fgcap). tools\fg-replay\fgreplay.cpp makes
those images again at a desk through any build of the library, and sheets.py turns a run into pictures a person can
judge. Every later picture decision is judged on this, so IT MUST NOT LIE. This script is the proof that it ran and
that it measures the right thing, with no game:

PART 1, THE REFUSALS (the helper alone, a handshake and no frame): the capture asked for without its own number, without
frame generation, into a folder that cannot be made: each is ONE line beginning "frame generation capture: OFF", and the
helper goes on. (The refusal beside the pair stamp went with the stamp, host\fg_stills.h, in round 8's deep strip.)

PART 2, THE CAPTURE IN THE REAL HELPER. The fake client (tools\test-fg-pipe\fgpipe.cpp, built here as fgcost.exe by the
cost bench's own staging) feeds the development helper its mode 40: a moving picture with known vectors at 3840x2160
FP16 with Quality's guides and the game's own set (no HUD-less image), every pair dispatched, first with a camera cut
every 17th pair (arm FK), then with none (arm F). The helper runs THE MOD'S OWN LIBRARY (the product, dlssiv-fg.dll, by
the own arm) with --fgcap on and --fgcap_go=1: a go file that lies there before the helper starts must be REMOVED AND
NOT OBEYED, and the runs begin only when this script writes a new one. What is then held: the helper's own lines (asked,
armed with the megabytes, the textures, every run written, the closing line with the serving thread's time), every file
of every run against its index, a run's frame 0 a reset and its frames consecutive, and the setting quoted from the
log in the mod's own words (this script plants such a line into the log before the helper starts; a second session
with none must say "none").
TWO MORE SESSIONS: the video memory rule and the disk rule each FORCED (--fgcap_refuse): one OFF line with the numbers,
no run, and the session itself still passes.

PART 3, THE REPLAY'S CONTROL. Every run is replayed through THE LIBRARY THE HELPER RAN: every generated image must EQUAL
the saved one, byte for byte, both SHA-256 on every frame's line. THE PLANTED FAULTS, each of which must be SEEN: the
prepare's motion vector scale negated (every frame behind the reset differs), and ONE PIXEL of the last presented picture
changed (that frame alone differs). TWO OTHER BUILDS BY THEIR PATHS (the replay takes any build by its path and says which
by its hash): the reference library (the same source with no cut) and the exact library (the exact cuts, no picture
cut) must give THE SAME IMAGE FOR EVERY FRAME; since 2026-10-12, when the product's picture became its own, those are
not the product's saved images behind a reset, and the replay must see that. MEASURED AND SAID, not judged: the run with
no warm-up pair before it, and the run begun three and eight frames late with a reset there (how deep the library's
history is: from which frame the images are the saved ones again).

PART 4, THE SHEETS: sheets.py on one run writes its pictures; each is a valid PNG of the size it should have, the
vectors along a line are printed as numbers, and AMD's own debug view is drawn by the library in a replay with --debug.

IT NEEDS THE GRAPHICS CARD: every session and every replay waits until no other GPU user runs (the cost bench's own
list, by the process list) and until no game runs. Everything is built and run under %TEMP%\dlssiv-fg-replay, this
tool's own folder; the replay is built as fgpipe-replay.exe so that every other proof's wait sees it. The captured runs
are several GiB: they are removed at the end unless --keep is given.

    python tools\fg-replay\run.py            all four parts (about four minutes on a free card)
    python tools\fg-replay\run.py --keep     the same, and the captured runs stay in the folder
exit code 0 = every requirement held. The last line is PASS or FAIL.
"""
import datetime, importlib.util, os, re, shutil, struct, subprocess, sys, tempfile, time, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
KEEP = "--keep" in ARGS
NOWIN = subprocess.CREATE_NO_WINDOW
FRAMES, RUNS, GAP = 14, 3, 1500        # the desk's capture: frames a run, runs, pairs between runs
PAIRS = 2500                           # pairs a cell of the fake client's (two cells: FK, then F). WITH THE GAP ABOVE THE FIRST RUN LIES IN FK AND THE THIRD IN F:
                                       # the first begins within some hundred pairs of the go, the third two gaps and two writes later, past pair 2500
W, H, RW, RH = 3840, 2160, 2560, 1440
FRAME_MIB = (2 * W * H * 8 + RW * RH * 4 + RW * RH * 4) / 1048576.0   # the presented and the generated image, depth, vectors
# THE FLOOR. Raised in the commit that adds proofs; a run with fewer is a proof that stopped running. (79 since round 8's
# deep strip, 2026-10-26: R-stamp, the capture refused beside the pair stamp, went with the stamp, host\fg_stills.h)
MIN_PROOFS = 79
MOD_LINE = "  transport: cfg sharpness=0.00 lod_bias=0.00 preset=11 backend=-1 mv_glass=2 rs_ds=0 threaded=1"
MOVERS_LINE = "  velocity: movers drawn with depth writes off 7.89 a frame (blended rigid 5.10, blended skinned 0.00, not blended rigid 2.79, not blended skinned 0.00), 7.89 left out of the pass (mv_glass=2)"


def load_cost():
    """The cost bench as a module (its staging of the helper and the libraries, its waits for the card, its process list),
    with none of this script's arguments in its sight."""
    saved = sys.argv
    sys.argv = [saved[0]] + (["--keep"] if KEEP else [])
    spec = importlib.util.spec_from_file_location("fgcost_bench", os.path.join(ROOT, "tools", "test-fg-cost", "run.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


C = load_cost()
C.OUT = os.path.join(tempfile.gettempdir(), "dlssiv-fg-replay")
C.GPU_LOOK = 5
# WHO HAS THE CARD, for this proof: the helper, a fake client under any of its names, the line-drawing rig; and the game,
# which wait_card asks for by itself. The cost bench's own list also holds the CPU harnesses (dt_fg_test, perfproof),
# because ITS sessions are timed; nothing here is a time that is judged, and those take no card.
C.GPU_USERS = ("dlssiv-helper.exe", "fgpipe*.exe", "fgcost*.exe", "vklines*.exe")
proof = C.proof
REPLAY = os.path.join(C.OUT, "fgpipe-replay.exe")
CAP = os.path.join(C.OUT, "cap")


def wait_card(what):
    """ONE GPU USER AT A TIME, and never beside the game: looked for every 5 s, 30 minutes at most, then it goes on and says so."""
    waited = 0
    while (C.game_runs() or C.other_gpu_users()) and waited < 1800:
        if waited == 0: print("  the card is somebody else's (%s): %s waits, looking again every 5 s..." % ("the game" if C.game_runs() else "; ".join(C.other_gpu_users()), what))
        time.sleep(5); waited += 5
    if C.game_runs() or C.other_gpu_users(): print("  STILL SOMEBODY ELSE'S AFTER 30 MINUTES: %s goes on beside it" % what)
    elif waited: print("  the card is free after %d s" % waited)


def build_replay():
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 "%s" /Fe:fgpipe-replay.exe /Fo:fgpipe-replay.obj /link /INCREMENTAL:NO d3d12.lib dxgi.lib bcrypt.lib'
          % (C.vcvars(64), os.path.join(HERE, "fgreplay.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=C.OUT, creationflags=NOWIN)
    ok = proof(r.returncode == 0 and "warning C" not in r.stdout and os.path.exists(REPLAY), "build", "the replay compiles cleanly, 64-bit, as fgpipe-replay.exe (a name every proof's wait for the card sees)")
    if not ok: print(r.stdout[-3000:])
    return ok


def fake(sid, d, helper_args, fake_args, seed=None, go=None, timeout=900):
    """One helper and one fake client, to its end. seed: lines put into the helper's log BEFORE it starts (the mod's own
    lines, which in the game are written into the same file). go: (folder) the go file is written there once the
    helper's log says it waits for it. What the fake client printed, its exit code, and the helper's log."""
    log, out = C.P(sid, "helper.log"), C.P(sid, "out.txt")
    for p in (log, out):
        if os.path.exists(p): os.remove(p)
    if seed: open(log, "w", encoding="utf-8", newline="").write("".join(l + "\r\n" for l in seed))
    wait_card("session %s" % sid)
    print("SESSION %s" % sid)
    cmd = [os.path.join(C.OUT, "fgcost.exe"), os.path.join(d, "dlssiv-helper.exe"), log] + list(fake_args) + list(helper_args)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=d, creationflags=NOWIN)
    went = None
    if go:
        t0 = time.time()
        while p.poll() is None and time.time() - t0 < 120:
            text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
            if "waiting for the go file" in text:
                time.sleep(1.0)
                open(os.path.join(go, "go.txt"), "w").write("go\n")
                went = time.time() - t0
                break
            time.sleep(0.1)
    try:
        text, _ = p.communicate(timeout=timeout); rc = p.returncode
    except subprocess.TimeoutExpired:
        p.kill(); text, _ = p.communicate(); rc = 99
        text = (text or "") + "\n  FAIL  the fake client did not finish in %d s and was ended by its own process\nFAIL\n" % timeout
    left = C.end_ours("left after session %s" % sid) if C.ours() else []
    open(out, "w", encoding="utf-8").write(text + "\nrc=%d\n" % rc)
    lines = text.splitlines()
    s = {"sid": sid, "rc": rc, "out": text, "passed": [l for l in lines if l.startswith("  PASS  ")], "failed": [l for l in lines if l.startswith("  FAIL  ")],
         "last": lines[-1].strip() if lines else "", "left": len(left), "went": went,
         "log": open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""}
    s["cap"] = [l.split("frame generation capture: ", 1)[1] for l in s["log"].splitlines() if "frame generation capture: " in l]
    return s


def clean(s):
    ok = proof(s["rc"] == 0 and s["last"] == "PASS" and not s["failed"] and s["left"] == 0, s["sid"],
               "the fake client's own verdict: %d proofs passed, %d failed, exit code %d; it left no process behind" % (len(s["passed"]), len(s["failed"]), s["rc"]))
    for l in s["failed"][:6]: print("      " + l.strip()[:260])
    return ok


def replay(tag, run, lib, extra=(), expect="equal"):
    """One replay of one run through one library. Its lines, its exit code."""
    out = os.path.join(C.OUT, "replay-" + tag)
    shutil.rmtree(out, ignore_errors=True)
    wait_card("replay %s" % tag)
    r = subprocess.run([REPLAY, "--run=" + run, "--lib=" + lib, "--out=" + out, "--expect=" + expect] + list(extra), capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=900)
    lines = r.stdout.splitlines()
    fr = [l for l in lines if l.startswith("frame ")]
    return {"tag": tag, "rc": r.returncode, "lines": lines, "frames": fr, "equal": [l for l in fr if l.endswith(": EQUAL")], "differs": [l for l in fr if ": DIFFERS: " in l],
            "last": lines[-1].strip() if lines else "", "out": out, "head": [l for l in lines if l.startswith("fg-replay: ")]}


def read_index(run):
    p = os.path.join(run, "index.txt")
    if not os.path.exists(p): return None
    kv, frames, files, ended = {}, [], [], False
    for l in open(p, encoding="utf-8", errors="replace").read().splitlines():
        if l == "end": ended = True
        elif l.startswith("frame: "): frames.append(dict(t.split("=", 1) for t in l[7:].split(" ") if "=" in t))
        elif l.startswith("file: "): files.append(l[6:].split(" "))
        elif ": " in l and not l.startswith("frame_plain: "): k, v = l.split(": ", 1); kv[k] = v
    return {"kv": kv, "frames": frames, "files": files, "ended": ended, "text": open(p, "rb").read()}


def png_size(path):
    """(width, height) of a PNG whose every chunk has its right checksum and whose picture inflates to its own size, or None."""
    try: b = open(path, "rb").read()
    except OSError: return None
    if b[:8] != b"\x89PNG\r\n\x1a\n": return None
    at, idat, w, h, bpp = 8, b"", 0, 0, 0
    while at + 12 <= len(b):
        n, kind = struct.unpack(">I4s", b[at:at + 8]); body = b[at + 8:at + 8 + n]
        if zlib.crc32(kind + body) != struct.unpack(">I", b[at + 8 + n:at + 12 + n])[0]: return None
        if kind == b"IHDR": w, h, depth, ctype = struct.unpack(">IIBB", body[:10]); bpp = {2: 3, 6: 4, 0: 1}.get(ctype, 0)
        if kind == b"IDAT": idat += body
        at += 12 + n
    try: raw = zlib.decompress(idat)
    except zlib.error: return None
    return (w, h) if bpp and len(raw) == h * (1 + w * bpp) else None


# ---------------------------------------------------------------------------------------------------------------- the parts
def part_refusals(full, st):
    print("\n1. THE REFUSALS: each is one line beginning \"frame generation capture: OFF\", and the helper goes on")
    cases = [
        ("R-lone", ["--fg=1", "--fgcap_runs=2"] + st["args"], r"OFF: --fgcap_runs, --fgcap_gap, --fgcap_go, --fgcap_dir or --fgcap_refuse without --fgcap=N", "its runs asked for without its own number"),
        ("R-nofg", ["--fgcap=8"], r"OFF: asked for \(--fgcap=8\) with no frame generation state", "asked for without --fg=1"),
        ("R-dir", ["--fg=1", "--fgcap=8", "--fgcap_dir=..\\up"] + st["args"], r"OFF: --fgcap_dir is neither a full path nor a plain path with no drive and no \"\.\.\"", "a folder that climbs out by \"..\""),
        ("R-control", ["--fg=1", "--fgcap=8", "--fgcap_dir=" + os.path.join(C.OUT, "cap-hello")] + st["args"], r"asked: 8 frames a run, 1 run\(s\), 600 pairs between runs", "THE CONTROL: asked for rightly, it says so and no OFF line"),
    ]
    for sid, hargs, pat, what in cases:
        log = C.P(sid, "helper.log")
        if os.path.exists(log): os.remove(log)
        wait_card("session %s" % sid)
        want = 3 if "--fg=1" in hargs else 0
        r = subprocess.run([os.path.join(C.OUT, "fgcost.exe"), os.path.join(st["dir"], "dlssiv-helper.exe"), log, "--fake-hello-only", "--fake-want-fg=%d" % want] + hargs,
                           capture_output=True, text=True, errors="replace", cwd=st["dir"], creationflags=NOWIN, timeout=120)
        if C.ours(): C.end_ours("left after session %s" % sid)
        text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
        cap = [l for l in text.splitlines() if "frame generation capture: " in l]
        off = [l for l in cap if "capture: OFF" in l]
        hit = re.search("frame generation capture: " + pat, text) is not None
        if sid == "R-control":
            end = [l for l in cap if "capture: at the connection's end: 0 run(s) written of 1 asked" in l]
            proof(hit and not off and len(cap) == 2 and len(end) == 1 and r.returncode == 0, sid, "%s: its asked line and its closing line, and no other (%d line(s) of the capture's, %d of them OFF)" % (what, len(cap), len(off)))
        else: proof(hit and len(off) == 1 and len(cap) == 1 and r.returncode == 0, sid, "%s: one OFF line and no other of the capture's, and the handshake went through (exit code %d)" % (what, r.returncode))


def check_run(sid, run, k, want_mod, hud=False):
    """One run's folder against its index, and the index against what a run must be."""
    ix = read_index(run)
    if not proof(ix is not None and ix["ended"] and ix["kv"].get("format") == "1", sid, "run %d: its index is there and whole (it is written last, and ends \"end\")" % k): return None
    kv, fr = ix["kv"], ix["frames"]
    proof(b"\r\n" in ix["text"] and ix["text"].replace(b"\r\n", b"").count(b"\n") == 0 and all(c < 127 for c in ix["text"]), sid, "run %d: the index is plain ASCII with CRLF line ends" % k)
    proof(kv.get("frames") == str(FRAMES) and len(fr) == FRAMES and kv.get("run") == str(k) and kv.get("display") == "%dx%d" % (W, H) and kv.get("render") == "%dx%d" % (RW, RH) and kv.get("hudless") == ("1" if hud else "0")
          and all(f["hud"] == ("1" if hud else "0") for f in fr), sid,
          "run %d: %d frames at %s with guides %s, %s, by its index and by every frame's record" % (k, len(fr), kv.get("display"), kv.get("render"), "WITH the HUD-less image" if hud else "no HUD-less image"))
    sizes = {"present": (W, H, 10, 8), "gen": (W, H, 10, 8), "depth": (RW, RH, 41, 4), "mv": (RW, RH, 34, 4)}
    if hud: sizes["hud"] = (W, H, 10, 8)
    bad = []
    for name, kind, frame, w, h, fmt, bpp, nbytes in ix["files"]:
        want = sizes.get(kind)
        p = os.path.join(run, name)
        if not want or (int(w), int(h), int(fmt), int(bpp)) != want or int(nbytes) != want[0] * want[1] * want[3] or not os.path.exists(p) or os.path.getsize(p) != int(nbytes): bad.append(name)
    extra = sorted(set(os.listdir(run)) - {f[0] for f in ix["files"]} - {"index.txt"})
    proof(len(ix["files"]) == len(sizes) * FRAMES and not bad and not extra, sid, "run %d: every one of its %d files is there with the size and format its index says, and the folder holds no other (%s)" % (
        k, len(ix["files"]), "none wrong" if not bad and not extra else "WRONG: %s %s" % (bad[:4], extra[:4])))
    ids = [int(f["id"]) for f in fr]; ns = [int(f["n"]) for f in fr]
    proof(fr[0]["prep_reset"] == "1" and fr[0]["disp_reset"] == "1" and fr[0]["since"] == "0" and ids == list(range(ids[0], ids[0] + FRAMES)) and ns == sorted(set(ns)), sid,
          "run %d: frame 0 was told to reset at its prepare and at its dispatch, and the %d frames are consecutive (AMD's frame ids %d to %d, pairs %d to %d)" % (k, FRAMES, ids[0], ids[-1], ns[0], ns[-1]))
    proof(all(f["mvscale"] == "3F800000,3F800000" and f["jitter"] == "00000000,00000000" and f["transfer"] == "2" and f["rect"] == "0,0,%d,%d" % (W, H) and f["render"] == "%dx%d" % (RW, RH) for f in fr), sid,
          "run %d: every frame's record holds what the helper hands the library today: the vectors' scale 1 x 1, no jitter, scRGB, the whole display" % k)
    if want_mod:
        proof(kv.get("mod_cfg_line") == MOD_LINE.strip() and kv.get("sharpness") == "0.00" and kv.get("mv_glass") == "2" and kv.get("mod_movers_line") == MOVERS_LINE.strip(), sid,
              "run %d: THE SETTING IS QUOTED FROM THE LOG IN THE MOD'S OWN WORDS: sharpness %s, mv_glass %s, and the line of the movers left out" % (k, kv.get("sharpness"), kv.get("mv_glass")))
    else:
        proof(kv.get("mod_cfg_line") == "(none in the helper's log)" and kv.get("sharpness") == "?" and kv.get("mv_glass") == "?", sid, "run %d: ITS CONTROL: a log with no line of the mod's gives \"none\" and \"?\", never a guess" % k)
    return ix


def part_capture(st):
    print("\n2. THE CAPTURE IN THE REAL HELPER: mode 40 at 3840x2160 FP16, Quality's guides, the game's set; %d frames a run, %d runs, a go file" % (FRAMES, RUNS))
    shutil.rmtree(CAP, ignore_errors=True); os.makedirs(CAP)
    open(os.path.join(CAP, "go.txt"), "w").write("stale\n")          # a go file that lies there BEFORE the helper starts: it must be removed and not obeyed
    fk = ["--fake-want-fg=3", "--fake-fg=40", "--fake-cost-guides=1", "--fake-cost-fmt=0", "--fake-cost-hud=0", "--fake-cost-idle=1", "--fake-cost-pairs=%d" % PAIRS, "--fake-cost-arms=FK,F",
          "--fake-cost-out=" + C.P("CAP", "client.csv"), "--fake-cost-gen=" + C.P("CAP", "gen.bin"), "--fake-cost-hashout=" + C.P("CAP", "hash.csv")]
    hp = ["--fg=1"] + st["args"] + ["--fgcap=%d" % FRAMES, "--fgcap_runs=%d" % RUNS, "--fgcap_gap=%d" % GAP, "--fgcap_go=1", "--fgcap_dir=" + CAP]
    s = fake("CAP", st["dir"], hp, fk, seed=[MOD_LINE, MOVERS_LINE], go=CAP)
    clean(s)
    cap, log = s["cap"], s["log"]
    for l in cap: print("    host: frame generation capture: " + l[:330])
    proof(any(l.startswith("asked: %d frames a run, %d run(s), %d pairs between runs; a run begins with a reset of the helper's own, at a steady pair (the 12th dispatch after a reset or later); no run before the go file" % (FRAMES, RUNS, GAP)) for l in cap),
          "CAP", "the helper said what was asked: %d frames a run, %d runs, %d pairs between them, no run before the go file" % (FRAMES, RUNS, GAP))
    order = [i for i, l in enumerate(cap) if l.startswith("go: a go file older than this helper was removed")], [i for i, l in enumerate(cap) if l.startswith("go: the go file was seen and removed")], \
            [i for i, l in enumerate(cap) if l.startswith("textures: ")]
    proof(len(order[0]) == 1 and len(order[1]) == 1 and len(order[2]) == 1 and order[0][0] < order[1][0] < order[2][0] and s["went"] is not None and not os.path.exists(os.path.join(CAP, "go.txt")), "CAP",
          "THE GO FILE: the one that lay there first was removed and NOT obeyed; the textures were made only after this script's own was seen (written %.1f s into the session); neither is left" % (s["went"] or -1))
    m = re.search(r"ARMED: the presented and the generated image 3840x2160 format 10, no HUD-less image, depth 2560x1440 format 41, vectors format 34; a frame is ([\d.]+) MiB, a run of %d frames ([\d.]+) MiB of video memory and of files, %d run\(s\) ([\d.]+) MiB of files" % (FRAMES, RUNS), log)
    proof(m is not None and abs(float(m.group(1)) - FRAME_MIB) < 0.06 and abs(float(m.group(2)) - FRAME_MIB * FRAMES) < 0.6, "CAP",
          "ARMED, with the megabytes: a frame %s MiB (by arithmetic %.1f), a run %s MiB, the %d runs %s MiB" % ((m.group(1), FRAME_MIB, m.group(2), RUNS, m.group(3)) if m else ("?", FRAME_MIB, "?", RUNS, "?")))
    wr = re.findall(r"capture: run (\d+) of %d written: %d frames, pairs (\d+) to (\d+), (\d+) of them steady, (\d+) files and 1 index, ([\d.]+) MiB, (\d+) ms \(read back (\d+) ms, written (\d+) ms\), in (.+)" % (RUNS, FRAMES), log)
    proof(len(wr) == RUNS and [int(x[0]) for x in wr] == list(range(1, RUNS + 1)) and "NOT written" not in log and "capture: OFF" not in log and "run broken" not in log, "CAP",
          "%d runs written of %d asked, each said by the writer's own line when its files were on the disk; none broken, none refused" % (len(wr), RUNS))
    end = re.search(r"capture: at the connection's end: (\d+) run\(s\) written of (\d+) asked \((\d+) handed to the writer, (\d+) not written\), (\d+) broken, (\d+) frame\(s\) of an open run dropped; go seen; the serving thread: ([\d.]+) us a pair of a run, ([\d.]+) at most, over (\d+) pair\(s\)", log)
    proof(end is not None and end.group(1) == end.group(2) == end.group(3) == str(RUNS) and end.group(4) == "0" and end.group(9) == str(RUNS * FRAMES), "CAP",
          "the closing line: %s" % (("%s written of %s asked, %s not written, %s broken; the serving thread %s us a pair of a run, %s at most, over %s pairs" % (end.group(1), end.group(2), end.group(4), end.group(5), end.group(7), end.group(8), end.group(9))) if end else "NOT THERE"))
    own = re.search(r"resets by reason: cut (\d+), build (\d+), device (\d+), gap (\d+), the helper's own (\d+)", log)
    proof(own is not None and int(own.group(5)) >= RUNS, "CAP", "the runs' resets are the helper's own, counted on the connection's line (%s; at least %d)" % (own.group(5) if own else "no line", RUNS))
    runs = []
    for x in wr:
        k, run = int(x[0]), x[9].strip()
        ix = check_run("CAP", run, k, True)
        if ix: runs.append({"k": k, "dir": run, "ix": ix, "steady": int(x[3]), "mib": float(x[5]), "ms": int(x[6]), "read": int(x[7]), "write": int(x[8])})
    if len(runs) == RUNS:
        mid = [r for r in runs if any(f["disp_reset"] == "1" for f in r["ix"]["frames"][1:])]
        plain = [r for r in runs if r not in mid]
        proof(bool(mid) and bool(plain), "CAP", "the runs hold both kinds: %d with a reset the game told inside it (the fake client's cut every 17th pair), %d with none; steady frames a run: %s" % (
            len(mid), len(plain), ", ".join(str(r["steady"]) for r in runs)))
        serve = [float(f["serve_us"]) for r in runs for f in r["ix"]["frames"]]
        gp = [float(f["gpu_us"].split(",")[0]) for r in runs for f in r["ix"]["frames"]]; gd = [float(f["gpu_us"].split(",")[1]) for r in runs for f in r["ix"]["frames"]]
        proof(len(serve) == RUNS * FRAMES and min(serve) > 0.0 and sum(serve) / len(serve) < 500.0 and min(gp) > 0.0 and min(gd) > 0.0, "CAP",
              "THE CAPTURE'S COST A PAIR OF A RUN, by its own clocks, every frame read: the serving thread %.1f us on average, %.1f at most (under 500: a judgment); the card %.0f us for the prepare's two copies and %.0f us for the dispatch's two (medians)" % (
                  sum(serve) / len(serve), max(serve), sorted(gp)[len(gp) // 2], sorted(gd)[len(gd) // 2]))
        s["cost"] = (sum(serve) / len(serve), max(serve), sorted(gp)[len(gp) // 2], sorted(gd)[len(gd) // 2], sum(r["ms"] for r in runs) / len(runs), sum(r["mib"] for r in runs) / len(runs))
    s["runs"] = runs
    # THE SETTING'S CONTROL, and the two rules forced: short sessions (one cell of F)
    fk2 = [a for a in fk if not a.startswith(("--fake-cost-arms", "--fake-cost-pairs", "--fake-cost-out", "--fake-cost-gen", "--fake-cost-hashout"))] + ["--fake-cost-arms=F", "--fake-cost-pairs=400"]
    for sid, extra, what in (("CAP-vram", ["--fgcap_refuse=1"], "the video memory rule"), ("CAP-disk", ["--fgcap_refuse=2"], "the disk")):
        d = os.path.join(C.OUT, "cap-" + sid.lower())
        shutil.rmtree(d, ignore_errors=True)
        t = fake(sid, st["dir"], ["--fg=1"] + st["args"] + ["--fgcap=%d" % FRAMES, "--fgcap_dir=" + d] + extra,
                 fk2 + ["--fake-cost-out=" + C.P(sid, "client.csv"), "--fake-cost-gen=" + C.P(sid, "gen.bin"), "--fake-cost-hashout=" + C.P(sid, "hash.csv")])
        clean(t)
        off = [l for l in t["cap"] if l.startswith("OFF: ")]
        wrote = [n for _, _, fs in os.walk(d) for n in fs] if os.path.isdir(d) else []
        proof(len(off) == 1 and off[0].startswith("OFF: %s (FORCED by --fgcap_refuse): " % what) and re.search(r"[\d.]+ MiB", off[0]) is not None and off[0].endswith("nothing is captured, nothing else is affected") and not wrote
              and "THE CAPTURE GAVE UP" in t["log"], sid, "%s FORCED: one OFF line with the numbers, no file written, the closing line says it gave up, and the session itself passed: %s" % (what, off[0][:200] if off else "NO OFF LINE"))
    d = os.path.join(C.OUT, "cap-nomod")
    shutil.rmtree(d, ignore_errors=True)
    t = fake("CAP-nomod", st["dir"], ["--fg=1"] + st["args"] + ["--fgcap=%d" % FRAMES, "--fgcap_dir=" + d],
             fk2 + ["--fake-cost-out=" + C.P("CAP-nomod", "client.csv"), "--fake-cost-gen=" + C.P("CAP-nomod", "gen.bin"), "--fake-cost-hashout=" + C.P("CAP-nomod", "hash.csv")])
    clean(t)
    w2 = re.findall(r"capture: run 1 of 1 written: .*, in (.+)", t["log"])
    proof(len(w2) == 1 and any(l.startswith("asked: %d frames a run, 1 run(s), 600 pairs between runs" % FRAMES) and "the first run at once (no go file is waited for)" in l for l in t["cap"]), "CAP-nomod",
          "with no go file asked for the one run begins at once and is written")
    if w2: check_run("CAP-nomod", w2[0].strip(), 1, False)
    # THE THREE-IMAGE SET (a set WITH the HUD-less image, the shape the game had before wire 2 and may have again): the
    # HUD-less picture is captured too, bound in the replay, and the images must be equal there as well
    d = os.path.join(C.OUT, "cap-hud")
    shutil.rmtree(d, ignore_errors=True)
    fk3 = [("--fake-cost-hud=1" if a == "--fake-cost-hud=0" else a) for a in fk2]
    t = fake("CAP-hud", st["dir"], ["--fg=1"] + st["args"] + ["--fgcap=%d" % FRAMES, "--fgcap_dir=" + d],
             fk3 + ["--fake-cost-out=" + C.P("CAP-hud", "client.csv"), "--fake-cost-gen=" + C.P("CAP-hud", "gen.bin"), "--fake-cost-hashout=" + C.P("CAP-hud", "hash.csv")])
    clean(t)
    w3 = re.findall(r"capture: run 1 of 1 written: .*, in (.+)", t["log"])
    m3 = re.search(r"ARMED: the presented and the generated image 3840x2160 format 10, the HUD-less image too, .*; a frame is ([\d.]+) MiB", t["log"])
    proof(len(w3) == 1 and m3 is not None and abs(float(m3.group(1)) - (FRAME_MIB + W * H * 8 / 1048576.0)) < 0.06, "CAP-hud",
          "A SET WITH THE HUD-LESS IMAGE: armed with it (a frame %s MiB, by arithmetic %.1f), and its one run is written" % (m3.group(1) if m3 else "?", FRAME_MIB + W * H * 8 / 1048576.0))
    s["hudrun"] = None
    if w3 and check_run("CAP-hud", w3[0].strip(), 1, False, hud=True): s["hudrun"] = w3[0].strip()
    return s


def replayed(a):
    """{frame: the SHA-256 of the image a replay made of it}, from its own frame lines"""
    out = {}
    for l in a["frames"]:
        m = re.match(r"frame (\d+) .*replayed sha256 ([0-9a-f]{64})", l)
        if m: out[int(m.group(1))] = m.group(2)
    return out


def part_replay(s, st, ref, exact):
    print("\n3. THE REPLAY'S CONTROL: every generated image of every run, through the library the helper ran, equals the saved one; a planted fault is seen")
    runs = s.get("runs") or []
    if not proof(len(runs) == RUNS, "replay", "the %d captured runs are there to replay" % RUNS): return
    lib = st["path"]
    for r in runs:
        a = replay("run%d-same" % r["k"], r["dir"], lib)
        same = [l for l in a["head"] if "THE SAME FILE" in l and st["sha"] in l]
        for l in a["frames"]: print("    " + l[:230])
        proof(a["rc"] == 0 and a["last"] == "PASS" and len(a["equal"]) == FRAMES and not a["differs"] and len(same) == 1, "replay",
              "run %d THROUGH THE LIBRARY THE HELPER RAN (named by its SHA-256, the capture's own): %d of %d generated images EQUAL the saved ones, byte for byte" % (r["k"], len(a["equal"]), FRAMES))
    plain = next((r for r in runs if not any(f["disp_reset"] == "1" for f in r["ix"]["frames"][1:])), runs[-1])
    for r, tag in ((plain, "plain"), (next((x for x in runs if x is not plain), plain), "cut")):
        a = replay("plant-mvscale-" + tag, r["dir"], lib, ["--plant=mvscale"], "differ")
        resets = ["frame %02d " % i for i, f in enumerate(r["ix"]["frames"]) if f["disp_reset"] == "1"]
        proof(a["rc"] == 0 and a["last"] == "PASS" and len(a["differs"]) == FRAMES - len(resets) and sorted(l[:9] for l in a["equal"]) == resets, "replay",
              "A PLANTED FAULT, the prepare's vector scale negated (run %d): SEEN in every frame that was not told to reset (%d of %d differ; the %d reset frame(s) are that frame's own picture whatever the vectors say): %s" % (
                  r["k"], len(a["differs"]), FRAMES, len(resets), (a["differs"][-1].split("DIFFERS: ", 1)[1] if a["differs"] else "NOTHING DIFFERS")))
    a = replay("plant-pixel", plain["dir"], lib, ["--plant=pixel"], "differ")
    proof(a["rc"] == 0 and a["last"] == "PASS" and len(a["differs"]) == 1 and a["differs"][0].startswith("frame %02d " % (FRAMES - 1)) and len(a["equal"]) == FRAMES - 1, "replay",
          "A PLANTED FAULT, ONE PIXEL of the last presented picture changed: SEEN in that frame and in no other: %s" % (a["differs"][0].split("DIFFERS: ", 1)[1] if a["differs"] else "NOTHING DIFFERS"))
    # ANOTHER BUILD BY ITS PATH. Until 2026-10-12 the product's picture was AMD's and the reference gave the product's images. THE
    # PRODUCT'S PICTURE IS ITS OWN SINCE (ffx-own\cuts\CUTS.txt: "accepted"), so two other builds are replayed by their paths, the
    # reference (no cut) and the exact library (the exact cuts, no picture cut): each is said to be another file than the
    # capture's, THE TWO GIVE THE SAME IMAGE FOR EVERY FRAME (the exact cuts are exact on a captured run as well), and where
    # picture cuts are accepted those images are NOT the product's saved ones behind the reset (the replay sees a picture cut).
    own_picture = st["sha"] != exact["sha"]
    a = replay("ref", plain["dir"], ref["path"], expect="differ" if own_picture else "equal")
    b = replay("exact", plain["dir"], exact["path"], expect="differ" if own_picture else "equal")
    other = [l for l in a["head"] if "ANOTHER FILE" in l and ref["sha"] in l] + [l for l in b["head"] if "ANOTHER FILE" in l and exact["sha"] in l]
    ra, rb = replayed(a), replayed(b)
    # (with no picture cut accepted the exact library IS the capture's file, and only the reference is another)
    proof(a["rc"] == 0 and b["rc"] == 0 and a["last"] == "PASS" and b["last"] == "PASS" and len(other) == (2 if own_picture else 1) and len(ra) == FRAMES and ra == rb
          and ((len(a["differs"]) >= 1 and len(b["differs"]) == len(a["differs"])) if own_picture else (len(a["equal"]) == FRAMES and len(b["equal"]) == FRAMES)), "replay",
          "ANOTHER BUILD BY ITS PATH, TWO OF THEM: the reference library (no cut; SHA-256 %s) and the exact library (the exact cuts and no picture cut; SHA-256 %s), each said to be another file than the capture's, GIVE THE SAME IMAGE FOR EVERY ONE OF THE %d FRAMES; %s" % (
              ref["sha"][:16], exact["sha"][:16], FRAMES,
              "and %d of those are NOT the product's saved images (the product's picture is its own: its accepted picture cuts), %d are (a frame told to reset is that frame's own picture)" % (len(a["differs"]), len(a["equal"])) if own_picture
              else "and they are the product's saved images, %d of %d equal (no picture cut is accepted: the product is the exact library)" % (len(a["equal"]), FRAMES)))
    if proof(s.get("hudrun") is not None, "replay", "the run of the set with the HUD-less image is there to replay"):
        a = replay("hud", s["hudrun"], lib)
        proof(a["rc"] == 0 and a["last"] == "PASS" and len(a["equal"]) == FRAMES and any("with the HUD-less image" in l for l in a["head"]), "replay",
              "THE THREE-IMAGE SET, through the library the helper ran, the HUD-less picture bound as the helper bound it: %d of %d generated images EQUAL the saved ones" % (len(a["equal"]), FRAMES))
    print("  MEASURED AND SAID, not judged:")
    a = replay("nowarm", plain["dir"], lib, ["--warmup=0"])
    print("    with NO warm-up pair before the run: %d of %d equal (%s)" % (len(a["equal"]), FRAMES, a["last"]))
    s["nowarm"] = (len(a["equal"]), FRAMES)
    s["depth"] = []
    for k in (3, 8):
        a = replay("start%d" % k, plain["dir"], lib, ["--start=%d" % k], "equal")
        tail = [l for l in a["head"] if "generated images EQUAL" in l]
        print("    begun at frame %d with a reset there: %s" % (k, tail[0][11:] if tail else "NO RESULT LINE"))
        s["depth"].append((k, tail[0][11:] if tail else "?", [l[:8] for l in a["equal"]]))
    proof(all(isinstance(x[1], str) and x[1] != "?" for x in s["depth"]) and s["nowarm"][1] == FRAMES, "replay", "the three measuring replays ran to their end and said their counts")
    s["plain"] = plain


def part_sheets(s, st):
    print("\n4. THE SHEETS: one run as pictures, the vectors as numbers, AMD's own debug view")
    plain = s.get("plain")
    if not proof(plain is not None, "sheets", "a captured run is there for the sheets"): return
    dbg = replay("debugview", plain["dir"], st["path"], ["--debug=0x400", "--dump=%d" % (FRAMES - 2)])
    dumps = sorted(n for n in os.listdir(dbg["out"]) if n.endswith("-gen-replay.raw")) if os.path.isdir(dbg["out"]) else []
    proof(dbg["rc"] == 0 and len(dumps) == 2 and all(os.path.getsize(os.path.join(dbg["out"], n)) == W * H * 8 for n in dumps) and any("WITH DEBUG DRAWING (not compared)" in l for l in dbg["frames"]), "sheets",
          "a replay with --debug=0x400 draws AMD's debug view into the image, dumps the last two frames whole and compares nothing")
    out = os.path.join(C.OUT, "sheets")
    shutil.rmtree(out, ignore_errors=True)
    line = "%d,%d,%d,%d" % (W // 2 - 40, H // 2, W // 2 + 40, H // 2)
    r = subprocess.run([sys.executable, os.path.join(HERE, "sheets.py"), "--run=" + plain["dir"], "--out=" + out, "--frames=%d-%d" % (FRAMES - 2, FRAMES - 1), "--crop=middle:%d,%d,256,192" % (W // 2 - 128, H // 2 - 96),
                        "--line=middle:" + line, "--at=%d,%d" % (W // 2, H // 2), "--replay=" + dbg["out"], "--full"], capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=900)
    lines = r.stdout.splitlines()
    for l in lines[-28:]: print("    " + l[:230])
    made = sorted(os.listdir(out)) if os.path.isdir(out) else []
    pngs = {n: png_size(os.path.join(out, n)) for n in made if n.endswith(".png")}
    proof(r.returncode == 0 and lines and lines[-1].strip() == "PASS" and len(pngs) >= 12 and all(v is not None for v in pngs.values()), "sheets",
          "sheets.py wrote %d pictures, every one a whole PNG (each chunk's checksum right, the picture as large as its header says)" % len(pngs))
    full = [n for n, v in pngs.items() if v == (W, H)]
    proof(len([n for n in full if "-gen-" in n]) == 2 and len([n for n in full if "-real-" in n]) >= 2 and any(n.startswith("vectors-") for n in full) and any(n.startswith("depth-") for n in full) and any(n.startswith("debugview-") for n in full),
          "sheets", "the full-size pictures: each generated frame and its real frames, the vectors as a picture, the depth, AMD's debug view (%d at %dx%d)" % (len(full), W, H))
    sheet = [n for n in pngs if n.startswith("sheet-middle")]
    proof(len(sheet) == 1 and pngs[sheet[0]][0] >= 4 * 256 * 3, "sheets", "the crop's sheet: real, generated, real and the difference against the half-way blend, magnified (%s, %s)" % (sheet[0] if sheet else "NONE", pngs.get(sheet[0]) if sheet else ""))
    num = [l for l in lines if re.match(r"\s+\d+ +\d+ +\d+ +-?\d+\.\d+ +-?\d+\.\d+ ", l)]
    at = [l for l in lines if l.startswith("  at %d,%d of frame" % (W // 2, H // 2))]
    proof(len(num) >= 2 * 81 and len(at) == 2 and os.path.exists(os.path.join(out, "line-middle.txt")), "sheets",
          "THE VECTORS AS NUMBERS: under one pixel (%d lines) and along a line of 81 pixels a frame (%d rows), each with the depth and the picture's brightness beside it; the table is kept as line-middle.txt" % (len(at), len(num)))
    # the fake client's picture moves by whole pixels with known vectors: the numbers must be that move, the same all along the line
    vx = sorted({l.split()[3] for l in num}); vy = sorted({l.split()[4] for l in num})
    proof(len(vx) <= 2 and len(vy) <= 2, "sheets", "the numbers are the fake client's own move, one vector for the whole picture in a frame (x: %s; y: %s px at the render size)" % (", ".join(vx[:4]), ", ".join(vy[:4])))


def main():
    if not KEEP: shutil.rmtree(C.OUT, ignore_errors=True)
    os.makedirs(C.OUT, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(C.OUT, "result.txt"))
    print("the capture and the replay at a desk, %s" % datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
    t0 = time.time()
    if C.ours(): C.end_ours("left by an earlier run")
    for sub in ("full", "bare", "own", "beside"):
        if os.path.isdir(os.path.join(C.OUT, sub)): shutil.rmtree(os.path.join(C.OUT, sub))
    if C.prepare() is None or not build_replay():
        print("FAIL"); return 1
    st = C.stage_lib("ours"); ref = C.stage_lib("ref"); exact = C.stage_lib("exact")
    if not proof(st is not None and ref is not None and exact is not None, "build", "the product (dlssiv-fg.dll), the reference (dlssiv-fg-ref.dll) and the exact library (dlssiv-fg-exact.dll) are built in this tree and staged as copies (build-ffx-own.ps1 -Target dlssiv-fg)"):
        print("FAIL"); return 1
    free = shutil.disk_usage(C.OUT).free
    if not proof(free > (RUNS + 2) * FRAMES * FRAME_MIB * 1048576 * 1.5, "build", "the disk has room for the desk's runs (%.1f GiB free; they take about %.1f GiB and are removed at the end)" % (free / 2 ** 30, (RUNS + 1) * FRAMES * FRAME_MIB / 1024)):
        print("FAIL"); return 1
    part_refusals(os.path.join(C.OUT, "full"), st)
    s = part_capture(st)
    part_replay(s, st, ref, exact)
    part_sheets(s, st)
    left = C.end_ours("left at the end") if C.ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    if "cost" in s:
        c = s["cost"]
        print("\nTHE CAPTURE'S NUMBERS, AS MEASURED AT THIS DESK (3840x2160 FP16, Quality's guides, two images): a frame %.1f MiB; a run of %d frames %.1f MiB, written in %.0f ms on average; "
              "the serving thread %.1f us a pair of a run (%.1f at most); the card's time for the capture's copies %.0f us behind the prepare and %.0f us behind the dispatch" % (FRAME_MIB, FRAMES, c[5], c[4], c[0], c[1], c[2], c[3]))
        print("THE LIBRARY'S HISTORY, AS MEASURED: no warm-up pair: %d of %d equal; %s" % (s["nowarm"][0], s["nowarm"][1], "; ".join("begun at frame %d: %s" % (k, t) for k, t, _ in s.get("depth", []))))
    if not KEEP:
        for d in [CAP] + [os.path.join(C.OUT, n) for n in os.listdir(C.OUT) if n.startswith(("cap-", "replay-"))]:
            raw = [os.path.join(dp, n) for dp, _, fs in os.walk(d) for n in fs if n.endswith(".raw")]
            for p in raw: os.remove(p)
        print("the raw pictures are removed (the indexes, the results and the sheets are kept in %s); --keep leaves them" % C.OUT)
    for sid, l in C.fails[:30]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    n = len(C.passes)
    if n < MIN_PROOFS: proof(False, "floor", "ONLY %d PROOFS RAN, and this tool's floor is %d: a part did not run" % (n, MIN_PROOFS))
    print("%d proofs passed, %d failed, in %.0f s (the floor is %d)" % (len(C.passes), len(C.fails), time.time() - t0, MIN_PROOFS))
    print("PASS" if not C.fails else "FAIL")
    return 0 if not C.fails else 1


if __name__ == "__main__":
    sys.exit(main())
