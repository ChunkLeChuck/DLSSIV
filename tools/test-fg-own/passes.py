r"""THE FIRST MEASURED TABLE OF WHAT EACH PASS OF A GENERATED FRAME COSTS (2026-10-10, the own build's stage 2, "THE
NUMBERS"; docs\DECISIONS.md, "PHASE 3: OUR OWN BUILD OF AMD'S FRAME GENERATION", section 4, stage 2;
docs\reports\fg-3\own-build-2026-10-06\reader-anatomy.md, whose pass table this one measures).

Until today every share of a pass was an estimate from operation counts. dlssiv-fg.dll (AMD's frame interpolation,
optical flow and backend behind the mod's own interface; build-ffx-own.ps1 -Target dlssiv-fg) can put a GPU timestamp
behind EVERY JOB AMD's backend executes (ffx-own\glue\dlssiv_fg.cpp, a development switch: the helper's
--fgown_stamps=1), each with the job's name as AMD's code names it. This script runs the cost bench's moving picture
on it (tools\test-fg-cost: the fake client's mode 40, the idle arms, every part of a pair ALONE on the card, no game,
no window), 3840x2160 FP16, the guides at 2560x1440 (Quality) and at 3840x2160 (DLAA), and prints:

  THE TABLE      for every pass, by reader-anatomy.md's names (P0, P1, O1 to O8, I1 to I11), its GPU microseconds in
                 a STEADY dispatch (the 12th after a reset or later), a RESET dispatch, and the FIRST after a reset, at
                 both guide sizes, three takes a cell with their spread, and the reader's estimated share beside the
                 measured one; then dispatches 1 to 9 after a reset, pass by pass
  THE CPU        the serve thread's clock round each of the three recordings (prepare, optical flow, interpolate)
  THE CONTROLS   each of which fails when broken:
     the sum         in every list the stamped spans add up to the cost marks' own bracket round the same calls
                     (host\fg_cost.h: two stamps of the helper's, outside the library), within 10 us or 1 percent
     the outer span  that bracket with the stamps ON, with the stamps OFF, and with AMD'S SIGNED LIBRARY through AMD's
                     loader in the same sessions: the stamps may stretch a call (a barrier stands before each), and
                     by how much is said; with the stamps off ours must cost what AMD's costs
     the picture     the whole generated image by its hash, pair by pair: the stamps on, the stamps on with no barrier,
                     the stamps off and AMD's library give the same images
     the empty       a stamp behind a job that recorded nothing (AMD's barrier jobs are kept for the next job) reads
     bracket         about zero
     the barrier     the same table with NO barrier before the stamps (--fgown_stamps=2): what the barrier changes
     the rows        every list's stamps were read, none lost; every pass's name is one this script knows (a name it
                     does not know is printed and fails: nothing is summed away unseen)
  THE MACHINE    beside every session: the CPU's load, the compilers running, the GPU's clock; a session beside
                 which another GPU user started is VOID and its table is not printed

THE TWO SHAPES. "HUDLESS own" is a three-image set with the HUD-less picture bound (what the cost bench's headline
figures and the reader's estimates were taken on); "no HUD-less image" is the two-image set the product asks for
since lever 1. The table is taken on the first; the second's steady column is printed beside it.

A NUMBER IS TAKEN AS THE COST BENCH TAKES ONE: a round's value is the mean over the picture's six moves of each
move's lower quartile (other work on the card can only add to a span), three rounds a session.

    python tools\test-fg-own\passes.py            every session (about 20 minutes)
    python tools\test-fg-own\passes.py --quick    fewer pairs: the instrument, not numbers
    ... --keep / --read                           do not empty the folder first / run nothing, read what it holds
    ... --keep --only=TQ3,OD2                     run the named sessions again, read every other one as the folder holds it
    ... --csv=<file>                              also write every cell's takes as rows (the report keeps it)
    python tools\test-fg-own\passes.py --cut      THE PRICE OF THE CUTS (the own build's stage 3, 2026-10-10): BEFORE AGAINST
                                                  AFTER, BY PASS. Two libraries of the mod's own interface, in turn, on
                                                  the product's shape (the two-image set), at both guide sizes, each with
                                                  the stamps on (the table by pass) and off (the outer span, which judges
                                                  the saving): by default THE REFERENCE (stage 2's library, no cut) against
                                                  THE PRODUCT (the cuts that are in).
    ... --before=<library> --after=<library>      two other libraries (full paths): one cut alone is priced by a build
                                                  without it (build-ffx-own.ps1 -Target dlssiv-fg -Cuts ... -OutDir ...)
                                                  against a build with it
    ... --three                                   also on the three-image set (with the HUD-less image)
    ... --stamped                                 the stamped sessions alone (an experiment's quick look: the outer span
                                                  with the stamps off, which judges a saving, is then not taken; it ends FAIL)
                                                  Its controls: every list read and none lost, the stamped spans add up to
                                                  the marks' bracket, every name has a row, the two libraries' images are
                                                  the same by their hashes on every hashed pair, and a session another GPU
                                                  user ran beside is taken again. Its folder is %TEMP%\dlssiv-test-fg-cut.
exit code 0 = every control held. The last line is PASS or FAIL. Its folder is %TEMP%\dlssiv-test-fg-passes.
NOT part of tools\prove-all.py: it takes the card for minutes and its numbers are times.
"""
import csv, datetime, importlib.util, os, re, shutil, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
QUICK = "--quick" in ARGS
READ = "--read" in ARGS
KEEP = "--keep" in ARGS or READ
CSV_OUT = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--csv=")), None)
CUT = "--cut" in ARGS                  # the price of the cuts: before against after, by pass (main_cut)
BEFORE = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--before=")), None)
AFTER = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--after=")), None)
THREE = "--three" in ARGS
STAMPED = "--stamped" in ARGS
ONLY = next((a.split("=", 1)[1].split(",") for a in ARGS if a.startswith("--only=")), None)   # with --keep: these sessions are run, every other one is read as the folder holds it
TRIES = 12                # takes of a session before it is given up as void (6 until 2026-10-10: with four agents on one card and their builds beside, six takes were all shared once)
STEADY = 12
ROUNDS, PAIRS, HASH = (2, 96, 12) if QUICK else (3, 256, 24)
ARMS = "F,FR,FK"
HARMS = "F,FR,F1,FN,FC"
FK_EVERY = 17
DROP = 16                 # pairs left out at the head of a cell (the cost bench's DROP_IDLE)


def load_cost():
    saved = sys.argv
    sys.argv = [saved[0]] + [a for a in ARGS if a in ("--keep", "--read")]
    spec = importlib.util.spec_from_file_location("fgcost_bench", os.path.join(ROOT, "tools", "test-fg-cost", "run.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


C = load_cost()
# (2026-10-11) dt_fg_test.exe COUNTS AS A GPU USER ONLY WHERE IT MAKES A DEVICE: tools\test-fg-own\devices.py says why and how it is read
sys.path.insert(0, HERE)
import devices
devices.narrow(C)
C.OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-cut" if CUT else "dlssiv-test-fg-passes")
C.GPU_LOOK = 2
C.GPU_QUIET = 4           # every session here is TIMED: the card must have been nobody else's for 4 s (another bench's sessions follow each other within seconds; one that starts under a session of this script's voids it, and it is taken again)
proof = C.proof

# ---------------------------------------------------------------------------------------------------------------- the passes
# reader-anatomy.md's table: (key, the reader's name, the job's name as AMD's code names it (a regular expression) or the
# job's kind, the call it stands in (1 prepare, 2 optical flow, 3 interpolate), the reader's estimated share of a steady
# pair in percent at Quality and at DLAA)
PASSES = [
    ("P0",  "clear, reconstructed previous depth",        r"Clear Reconstructed Previous Nearest Depth", 1, 0.4, 0.6),
    ("P1",  "reconstruct and dilate",                     r"RECONSTRUCT_AND_DILATE", 1, 5.7, 9.0),
    ("O1",  "prepare luma",                               r"OF PrepareLuma", 2, 2.6, 1.8),
    ("O2",  "luminance pyramid",                          r"OF GenerateOpticalFlowInputPyramid", 2, 0.5, 0.4),
    ("O3",  "scene change histogram and divergence",      r"OF (GenerateSCDHistogram|ComputeSCDDivergence)", 2, 0.5, 0.3),
    ("O4",  "search, level 0",                            r"OF 0 Search", 2, 22.9, 16.0),
    ("O5",  "search, level 1",                            r"OF 1 Search", 2, 5.8, 4.0),
    ("O6",  "search, levels 2 to 6",                      r"OF [2-6] Search", 2, 1.9, 1.4),
    ("O7",  "filter, 7 levels",                           r"OF \d Filter", 2, 0.2, 0.1),
    ("O8",  "scale, 6 levels",                            r"OF \d (Filter and )?Scale", 2, 0.6, 0.4),       # (a cut of stage 3 may put a level's filter into its scale: the job's name then says so)
    ("Oc",  "(a reset's clears, optical flow)",           "job0", 2, None, None),
    ("I1",  "setup",                                      r"SETUP|Clear Setup Images", 3, 2.1, 3.3),         # (a cut of stage 3 writes the setup pass's zeroes by clears of that name)
    ("I2",  "clear, interpolated-frame depth",            r"Clear Reconstructed Depth Interpolated Fram\w*", 3, 0.4, 0.6),     # (a mark holds 43 characters of a name)
    ("I3",  "reconstruct previous depth",                 r"RECONSTRUCT_PREV_DEPTH", 3, 4.2, 6.7),
    ("I4",  "game motion vector field",                   r"GAME_MOTION_VECTOR_FIELD", 3, 18.1, 25.4),
    ("I5",  "game vector field pyramid",                  r"GAME_VECTOR_FIELD_INPAINTING_PYRAMID", 3, 1.3, 2.0),
    ("I6",  "optical flow vector field",                  r"OPTICAL_FLOW_VECTOR_FIELD", 3, 0.5, 0.4),
    ("I7",  "disocclusion mask",                          r"DISOCCLUSION_MASK", 3, 4.8, 7.5),
    ("I8",  "interpolation",                              r"INTERPOLATION", 3, 14.6, 11.1),
    ("I9",  "inpainting pyramid",                         r"INPAINTING_PYRAMID", 3, 3.0, 2.1),
    ("I10", "inpainting and HUD compare",                 r"INPAINTING", 3, 6.4, 4.4),
    ("I11", "copy, source to AMD's \"previous\"",         "job1", 3, 3.5, 2.4),
    ("Id",  "(debug view)",                               r"DEBUG_VIEW", 3, None, None),
    ("X1",  "(prepare: barriers, discards, the call's end)", "rest", 1, None, None),
    ("X2",  "(optical flow: barriers, discards, the end)",   "rest", 2, None, None),
    ("X3",  "(interpolate: barriers, discards, the end)",    "rest", 3, None, None),
]
KEYS = [p[0] for p in PASSES]
NAME = {p[0]: p[1] for p in PASSES}
CALLS = {1: "prepare", 2: "optical flow", 3: "interpolate"}
CLASSES = ["steady", "reset"] + ["k%d" % k for k in range(1, 10)]


def key_of(call, kind, job, name, unknown):
    """which row of the table a stamp belongs to. kind: 0 a call's head, 1 behind a job, 2 behind the call's last barriers."""
    if kind == 1:
        for key, _, rx, c, _, _ in PASSES:
            if c != call: continue
            if rx == "job0" and job == 0 and not name.startswith("Clear Reconstructed"): return key
            if rx == "job1" and job == 1: return key
            if rx not in ("job0", "job1", "rest") and job in (0, 2) and re.fullmatch(rx, name): return key
        if job == 2 or (job in (0, 1) and call != 2):          # a compute pass, a clear or a copy this script has no row for: SAID, never summed away
            unknown.add("%s: %s (job kind %d)" % (CALLS.get(call, "?"), name, job))
    return "X%d" % call


def read_own(path):
    """the own arm's rows (host\fg_own.h): {(n, list): [row, ...]} in the list's order"""
    out = {}
    if not os.path.exists(path): return out
    with open(path, encoding="ascii", errors="replace", newline="") as f:
        for r in csv.DictReader(f):
            try: k = (int(r["n"]), r["list"])
            except (KeyError, ValueError): continue
            out.setdefault(k, []).append({"call": int(r["call"]), "idx": int(r["idx"]), "kind": int(r["kind"]), "job": int(r["job"]), "waited": int(r["waited"]),
                                          "us": float(r["us"]), "cpu": float(r["cpu_us"]), "name": r["name"]})
    for v in out.values(): v.sort(key=lambda r: r["idx"])
    return out


# ---------------------------------------------------------------------------------------------------------------- sessions
def run_session(sid, title, lib, guides, hud, stamps):
    st = C.stage_lib(lib)
    if st is None: return None
    fake = ["--fake-fg=40", "--fake-cost-guides=%d" % guides, "--fake-cost-hud=%d" % hud, "--fake-cost-idle=%d" % ROUNDS, "--fake-cost-pairs=%d" % PAIRS, "--fake-cost-arms=" + ARMS,
            "--fake-cost-hash=%d" % HASH, "--fake-cost-harms=" + HARMS]
    helper = ["--fg=1", "--gpu_probe=1"] + st["args"]
    own_iface = lib in C.OWN_IFACE          # a library of the mod's own interface: the product, the reference, a cut's before and after
    if own_iface:
        helper += ["--fgown_csv=" + C.P(sid, "own.csv")] + (["--fgown_stamps=%d" % stamps] if stamps else [])
    # A SESSION BESIDE WHICH ANOTHER GPU USER RAN IS VOID AND IS TAKEN AGAIN, here, up to TRIES times: other agents' benches
    # use this card too, and a timed table that shared it is thrown away, never averaged in
    run = not READ and (ONLY is None or sid in ONLY)
    s = None
    for attempt in range(1, TRIES + 1):
        if run:
            if os.path.exists(C.P(sid, "own.csv")): os.remove(C.P(sid, "own.csv"))
            C.session(st["dir"], sid, title + ("" if attempt == 1 else " (take %d: the take before shared the card)" % attempt), helper, fake)
        s = C.read_session(sid)
        if s is None or not run: break
        # (2026-10-10, stage 3) A SESSION A COMPILER RAN BESIDE IS TAKEN AGAIN TOO: the machine is shared with other agents'
        # builds. If every take had one, the last take is kept and SAID (it cannot be had clean): the rounds' spread speaks for it.
        shared = "; ".join(x for x in (s["others"], s["beside"]) if x)
        comp = sorted({x for x in (s["machine"].get("compilers", "none"), s["machine"].get("after", "none")) if x and x != "none"})
        s["compiled"] = ", ".join(comp)
        if not (shared or comp): break
        if attempt == TRIES:
            if comp and not shared: print("  [%s] A COMPILER RAN BESIDE EVERY ONE OF ITS %d TAKES (%s): the last take is KEPT AND SAID; its rounds' spread is what speaks for it" % (sid, TRIES, s["compiled"]))
            break
        print("  [%s] VOID, take %d: %s: it is taken again" % (sid, attempt, ("another GPU user ran beside it (%s)" % shared) if shared else ("a compiler ran beside it (%s)" % s["compiled"])))
    if s is None:
        proof(False, sid, "this session left nothing: it was not run (the card was never this tool's alone, or the bench could not start it)"); return None
    s["st"] = st; s["lib"] = lib; s["stamps"] = stamps; s["title"] = title
    C.check_session(s, True)
    C.check_identity(s, st)
    s["idle"] = C.idle_table(s)
    s["own"] = read_own(C.P(sid, "own.csv")) if own_iface else {}
    s["void"] = bool(s["others"] or s["beside"])
    if s["void"]: proof(False, sid, "ANOTHER GPU USER RAN BESIDE THIS SESSION (%s): ITS TIMES SHARED THE CARD: THE SESSION IS VOID after %d takes, and its numbers are NOT to be read" % ("; ".join(x for x in (s["others"], s["beside"]) if x), TRIES))
    cells = [x for x in s["cells"] if x.get("part") == "idle"]
    clock = C.clock_of(s["smi"], min(int(x["t0"]) for x in cells), max(int(x["t1"]) for x in cells)) if cells else {"words": "no idle cell"}
    print("  [%s] THE MACHINE: %s. %s" % (sid, C.load_words(s["machine"]), clock["words"].replace(": VOID", "")))
    m = re.search(r"frame generation: fgown: set: (create [^\n]*)", s["log"])
    s["set"] = m.group(1).strip() if m else None
    m = re.search(r"frame generation: fgown: set end: lists (\d+), read (\d+), not read (\d+), stamps lost (\d+), rows lost (\d+)", s["log"])
    s["end"] = tuple(int(x) for x in m.groups()) if m else None
    return s


def classify(s):
    """every list of the session's idle cells by what it is: {(class, rep): [(move, n)]} for the dispatches, and the same for
    the prepares ("steady": a steady pair's prepare, "reset": one told to reset)."""
    disp, prep = {}, {}
    for c in s["cells"]:
        if c.get("part") != "idle": continue
        arm, rep, first, last = c["arm"], int(c["rep"]), int(c["first"]), int(c["last"])
        for n in range(first, last + 1):
            d, p = s["hD"].get(n), s["hP"].get(n)
            mv = s["move"].get(n, -1)
            if d is not None:
                cls = None
                if arm == "F" and d["since"] >= STEADY and n >= first + DROP: cls = "steady"
                elif arm == "FR" and d["since"] == 0: cls = "reset"
                elif arm == "FK" and 1 <= d["since"] <= 9: cls = "k%d" % d["since"]
                if cls: disp.setdefault((cls, rep), []).append((mv, n))
            if p is not None and n >= first + DROP:
                if arm == "F" and not p["reset"] and d is not None and d["since"] >= STEADY: prep.setdefault(("steady", rep), []).append((mv, n))
                elif arm == "FR" and p["reset"]: prep.setdefault(("reset", rep), []).append((mv, n))
    return disp, prep


def table_of(s):
    """{(key, class): [a value a round]}, the CPU's clocks {(call, class): [median a round]}, the sum's control, the empty
    brackets and the names this script has no row for, from one session with the stamps on."""
    disp, prep = classify(s)
    t = {"cell": {}, "cpu": {}, "sumdiff": [], "empty": [], "unknown": set(), "outer": {}, "lists": 0, "calls": {}}
    floor = lambda cls: 1 if cls.startswith("k") else 2

    def one(lists, kind, cls, rep):
        per = {}                                   # key -> [(move, us)]
        cpu = {1: [], 2: [], 3: []}
        outer = {1: [], 2: [], 3: [], "list": []}
        for mv, n in lists:
            rows = s["own"].get((n, kind))
            if not rows: continue
            t["lists"] += 1
            got = {}; span = {1: 0.0, 2: 0.0, 3: 0.0}; total = 0.0
            for r in rows:
                if r["idx"] <= 0:                   # the list's first stamp has no span before it (with the stamps off a call's row is the CPU's clock alone, index -1)
                    if r["kind"] == 0: cpu[r["call"]].append(r["cpu"])
                    continue
                total += r["us"]
                if r["kind"] == 0:                  # the second call's head: the span between two calls, with a barrier at both ends and nothing between
                    cpu[r["call"]].append(r["cpu"]); t["empty"].append(r["us"]); got["X%d" % r["call"]] = got.get("X%d" % r["call"], 0.0) + r["us"]
                    continue
                k = key_of(r["call"], r["kind"], r["job"], r["name"], t["unknown"])
                got[k] = got.get(k, 0.0) + r["us"]
                span[r["call"]] += r["us"]
                if r["kind"] == 1 and r["job"] == 3: t["empty"].append(r["us"])      # behind a job that is only a barrier: nothing was recorded
            for k, v in got.items(): per.setdefault(k, []).append((mv, v))
            for c in (1, 2, 3):
                if span[c] > 0.0 or (kind == "P") == (c == 1): outer[c].append((mv, span[c]))
            outer["list"].append((mv, total))
            marks = (s["hD"] if kind == "D" else s["hP"]).get(n)
            if marks is not None and marks["gpu_us"] >= 0.0: t["sumdiff"].append((marks["gpu_us"] - total, marks["gpu_us"]))
        for k, smp in per.items():
            v, _ = C.by_move(smp, low=True, floor=floor(cls))
            if v is not None and len(smp) >= 4: t["cell"].setdefault((k, cls), []).append(v)
        for c in (1, 2, 3):
            if cpu[c]: t["cpu"].setdefault((c, cls), []).append(C.med(cpu[c]))
            if outer[c]:
                v, _ = C.by_move(outer[c], low=True, floor=floor(cls))
                if v is not None and len(outer[c]) >= 4: t["calls"].setdefault((c, cls), []).append(v)

    for (cls, rep), lists in sorted(disp.items()): one(lists, "D", cls, rep)
    for (cls, rep), lists in sorted(prep.items()): one(lists, "P", cls, rep)
    return t


def outer_of(s):
    """the cost marks' own brackets (outside the library) by class: {("prep"|"disp", class): [a value a round]}"""
    disp, prep = classify(s)
    out = {}
    for which, groups, rowsof in (("disp", disp, s["hD"]), ("prep", prep, s["hP"])):
        for (cls, rep), lists in sorted(groups.items()):
            smp = [(mv, rowsof[n]["gpu_us"]) for mv, n in lists if n in rowsof and rowsof[n]["gpu_us"] >= 0.0]
            v, _ = C.by_move(smp, low=True, floor=1 if cls.startswith("k") else 2)
            if v is not None and len(smp) >= 4: out.setdefault((which, cls), []).append(v)
    return out


def val(t, key, cls):
    v = t["cell"].get((key, cls))
    return C.med(v) if v else None


def cellw(v, nd=0):
    return "-" if not v else "%s (%s to %s)" % (C.fmt(C.med(v), nd), C.fmt(min(v), nd), C.fmt(max(v), nd))


# ---------------------------------------------------------------------------------------------------------------- main
def main():
    if not READ:
        if not KEEP: shutil.rmtree(C.OUT, ignore_errors=True)
        os.makedirs(C.OUT, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(C.OUT, "result-read.txt" if READ else "result.txt"))
    print("the first measured table of what each pass of a generated frame costs, %s%s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), ", QUICK: THE INSTRUMENT, NOT NUMBERS" if QUICK else ""))
    t0 = time.time()
    if not READ:
        if C.ours(): C.end_ours("left by an earlier run")
        for sub in ("full", "bare", "own", "beside"):
            if os.path.isdir(os.path.join(C.OUT, sub)): shutil.rmtree(os.path.join(C.OUT, sub))
        if C.prepare() is None:
            print("FAIL"); return 1
    lib = C.LIBS["ours"][1]
    if not proof(os.path.exists(lib), "build", "dlssiv-fg.dll is built in this tree (%s)" % lib):
        print("FAIL"); return 1
    rec = {}
    rp = os.path.join(ROOT, "tools", "ffx-own", "RECORD-dlssiv-fg.txt")
    if os.path.exists(rp):
        for l in open(rp, encoding="ascii", errors="replace"):
            f = l.split()
            if len(f) == 2 and not l.startswith("#"): rec[f[0]] = f[1]
    proof(rec.get("sha256") == C.sha256_of(lib) and rec.get("size") == str(os.path.getsize(lib)), "build",
          "dlssiv-fg.dll as built in this tree IS THE SECOND RECORD'S LIBRARY (tools\\ffx-own\\RECORD-dlssiv-fg.txt: %s bytes, SHA-256 %s)" % (rec.get("size", "no"), rec.get("sha256", "NO RECORD")))

    S = {}
    gname = {"Q": "the guides at 2560x1440 (Quality)", "D": "the guides at 3840x2160 (DLAA)"}
    hname = {1: "HUDLESS own (a three-image set)", 0: "no HUD-less image (a two-image set, the product's shape)"}
    plan = []
    for g, gi in (("Q", 1), ("D", 0)):
        plan += [("T%s3" % g, "ours", gi, 1, 1, "THE STAMPS ON, a barrier before each"), ("O%s3" % g, "ours", gi, 1, 0, "the stamps OFF"), ("A%s3" % g, "loader", gi, 1, 0, "AMD'S SIGNED LIBRARY through AMD's loader"),
                 ("N%s3" % g, "ours", gi, 1, 2, "the stamps on with NO barrier"), ("T%s2" % g, "ours", gi, 0, 1, "THE STAMPS ON, a barrier before each"), ("O%s2" % g, "ours", gi, 0, 0, "the stamps OFF")]
    for sid, lb, gi, hud, stamps, what in plan:
        g = sid[1]
        s = run_session(sid, "%s, %s; %s" % (gname[g], hname[hud], what), lb, gi, hud, stamps)
        if s is not None: S[sid] = s
    if len(S) != len(plan):
        print("FAIL: %d of %d sessions were not run" % (len(plan) - len(S), len(plan))); return 1

    T = {sid: table_of(s) for sid, s in S.items() if s["stamps"]}
    OUT = {sid: outer_of(s) for sid, s in S.items()}
    print("\n" + "=" * 118)
    print("WHAT EACH PASS OF A GENERATED FRAME COSTS. 3840x2160 FP16 (scRGB), the cost bench's moving picture, every part of a pair alone on the card, GPU microseconds by a timestamp behind every job of AMD's backend.")
    print("THE LIBRARY: %s, %d bytes, SHA-256 %s" % (lib, os.path.getsize(lib), C.sha256_of(lib)))
    sets = sorted({s["set"] for s in S.values() if s.get("set")})
    for x in sets: print("THE SET, by the helper's own line: %s" % x)
    dev = sorted({m.group(1).strip() for s in S.values() for m in [re.search(r"frame generation: device: (.*)", s["log"])] if m})
    for x in dev: print("THE DEVICE, by the helper's own line: %s" % x)
    proof(len(sets) >= 1 and all(re.match(r"create 0x00000000; pipelines 18 asked, 0 refused; ", x) for x in sets), "set", "every set of ours was created with 18 of AMD's pipelines asked and none refused")
    variant = sorted({m.group(1) for x in sets for m in [re.search(r": AMD's ([^;]*);", x)] if m})
    print("WHICH SHADER VARIANT RAN: AMD's %s" % (", ".join(variant) or "NOT SAID"))
    proof(len(variant) == 1, "set", "one shader variant in every session of ours: AMD's %s" % ", ".join(variant))

    # ---- the rows: read, none lost, every name known
    for sid, s in S.items():
        if s["lib"] != "ours": continue
        e = s["end"]
        proof(e is not None and e[2] == 0 and e[3] == 0 and e[4] == 0 and e[0] == e[1], sid, "every list of the own arm's was read (%s lists, %s read, %s not read), no stamp was lost (%s) and no row (%s)" % (e if e else ("?",) * 5))
        want = {1: "stamps on, a barrier before each", 2: "stamps on, NO barrier", 0: "stamps off"}[s["stamps"]]
        proof(s["set"] is not None and ("; stamps " + want.split(" ", 1)[1]) in s["set"], sid, "the helper said this session's stamps: %s" % (s["set"].rsplit("; ", 1)[-1] if s["set"] else "NOTHING"))
    for sid, t in T.items():
        proof(not t["unknown"], sid, "every compute pass, clear and copy the stamps named has a row in the table%s" % ((": NO ROW FOR " + "; ".join(sorted(t["unknown"]))) if t["unknown"] else ""))

    # ---- the table
    rows_csv = []
    for hud in (1,):
        tq, td = T["TQ3"], T["TD3"]
        for cls, what in (("steady", "A STEADY DISPATCH (the %dth after a reset or later) and its pair's prepare" % STEADY), ("reset", "A RESET DISPATCH and a prepare told to reset"), ("k1", "THE FIRST DISPATCH AFTER A RESET (prepare: as steady)")):
            pcls = "steady" if cls.startswith("k") else cls
            print("\nTHE TABLE, %s, %s. GPU us: the middle round (least to largest round, %d rounds)." % (hname[hud], what, ROUNDS))
            totq = sum(val(tq, p[0], pcls if p[3] == 1 else cls) or 0.0 for p in PASSES)
            totd = sum(val(td, p[0], pcls if p[3] == 1 else cls) or 0.0 for p in PASSES)
            print("  %-4s %-46s %-24s %-24s %s" % ("", "pass", "Quality guides", "DLAA guides", "share of the pair, Quality / DLAA: measured" + ("; the reader's estimate" if cls == "steady" else "")))
            for key, name, rx, call, eq, ed in PASSES:
                c = pcls if call == 1 else cls
                vq, vd = tq["cell"].get((key, c)), td["cell"].get((key, c))
                if not vq and not vd: continue
                sq = 100.0 * C.med(vq) / totq if vq and totq else None; sd = 100.0 * C.med(vd) / totd if vd and totd else None
                est = ("; %s / %s" % (C.fmt(eq, 1), C.fmt(ed, 1))) if (cls == "steady" and eq is not None) else ""
                print("  %-4s %-46s %-24s %-24s %s / %s%s" % (key, name, cellw(vq), cellw(vd), C.fmt(sq, 1), C.fmt(sd, 1), est))
                for g, v in (("Q", vq), ("D", vd)):
                    for i, x in enumerate(v or []): rows_csv.append(("three-image", g, cls, key, name, i, "%.2f" % x))
            for call in (1, 2, 3):
                c = pcls if call == 1 else cls
                print("  %-4s %-46s %-24s %-24s" % ("", "THE CALL: " + CALLS[call], cellw(tq["calls"].get((call, c))), cellw(td["calls"].get((call, c)))))
            print("  %-4s %-46s %-24s %-24s" % ("", "THE PAIR (the three calls)", C.fmt(totq), C.fmt(totd)))
    # ---- the two-image set's steady column beside it
    print("\nTHE PRODUCT'S SHAPE BESIDE IT (no HUD-less image: a two-image set). A steady dispatch and its prepare, GPU us; the three-image set's value behind it.")
    for key, name, rx, call, eq, ed in PASSES:
        a = [T[x]["cell"].get((key, "steady")) for x in ("TQ2", "TQ3", "TD2", "TD3")]
        if not any(a): continue
        print("  %-4s %-46s Quality %-22s (three-image %s)   DLAA %-22s (three-image %s)" % (key, name, cellw(a[0]), C.fmt(C.med(a[1]) if a[1] else None), cellw(a[2]), C.fmt(C.med(a[3]) if a[3] else None)))
        for g, v in (("Q", a[0]), ("D", a[2])):
            for i, x in enumerate(v or []): rows_csv.append(("two-image", g, "steady", key, name, i, "%.2f" % x))
    for call in (1, 2, 3):
        a = [T[x]["calls"].get((call, "steady")) for x in ("TQ2", "TQ3", "TD2", "TD3")]
        print("  %-4s %-46s Quality %-22s (three-image %s)   DLAA %-22s (three-image %s)" % ("", "THE CALL: " + CALLS[call], cellw(a[0]), C.fmt(C.med(a[1]) if a[1] else None), cellw(a[2]), C.fmt(C.med(a[3]) if a[3] else None)))
    # ---- dispatches 1 to 9 after a reset
    for sid, g in (("TQ3", "Quality"), ("TD3", "DLAA")):
        t = T[sid]
        print("\nDISPATCHES 1 TO 9 AFTER A RESET, %s guides, %s (arm FK: a reset every %dth pair). GPU us, the middle round; beside them the reset dispatch and the steady one." % (g, hname[1], FK_EVERY))
        print("  %-4s %-40s %7s | %s | %7s" % ("", "pass", "reset", " ".join("%6d" % k for k in range(1, 10)), "steady"))
        for key, name, rx, call, eq, ed in PASSES:
            if call == 1: continue
            vals = [val(t, key, "k%d" % k) for k in range(1, 10)]
            if not any(v is not None for v in vals) and val(t, key, "reset") is None and val(t, key, "steady") is None: continue
            print("  %-4s %-40s %7s | %s | %7s" % (key, name[:40], C.fmt(val(t, key, "reset")), " ".join("%6s" % C.fmt(v) for v in vals), C.fmt(val(t, key, "steady"))))
            for k in range(1, 10):
                for i, x in enumerate(t["cell"].get((key, "k%d" % k), [])): rows_csv.append(("three-image", sid[1], "k%d" % k, key, name, i, "%.2f" % x))
        for call in (2, 3):
            print("  %-4s %-40s %7s | %s | %7s" % ("", "THE CALL: " + CALLS[call], C.fmt(C.med(t["calls"].get((call, "reset"), [])) if t["calls"].get((call, "reset")) else None),
                                                " ".join("%6s" % (C.fmt(C.med(t["calls"][(call, "k%d" % k)])) if t["calls"].get((call, "k%d" % k)) else "none") for k in range(1, 10)),
                                                C.fmt(C.med(t["calls"].get((call, "steady"), [])) if t["calls"].get((call, "steady")) else None)))
    # ---- the CPU's clocks
    print("\nTHE CPU, the serve thread's clock round each of the three recordings (microseconds, the median of a round; the middle round and the rounds' least and largest). Said, not judged: the machine is shared.")
    for sid in ("TQ3", "TD3", "OQ3", "OD3"):
        s = S[sid]
        t = T.get(sid) or table_of(s)
        print("  %s (%s): " % (sid, "stamps on" if s["stamps"] else "stamps off") + "; ".join("%s %s, at a reset %s" % (CALLS[c], cellw(t["cpu"].get((c, "steady")), 1), cellw(t["cpu"].get((c, "reset")), 1)) for c in (1, 2, 3)))

    # ---- the controls
    print("\nTHE CONTROLS")
    for sid, t in T.items():
        d = sorted(abs(x[0]) for x in t["sumdiff"]); rel = sorted(abs(x[0]) / max(1.0, x[1]) for x in t["sumdiff"])
        if not d: proof(False, sid, "THE SUM: no list had both the stamps and the marks' bracket"); continue
        p50, p99 = d[len(d) // 2], d[int(0.99 * (len(d) - 1))]
        signed = sorted(x[0] for x in t["sumdiff"])[len(d) // 2]
        proof(p50 <= 10.0 and (p99 <= 10.0 or rel[int(0.99 * (len(d) - 1))] <= 0.01), sid,
              "THE SUM: in %d lists the stamped spans add up to the cost marks' own bracket round the same calls: the marks read %+.1f us more at the median, %.1f us apart at the 99th percentile (10 us or 1 percent allowed)" % (len(d), signed, p99))
    for sid, t in T.items():
        e = sorted(t["empty"])
        if e: proof(e[len(e) // 2] <= 2.0 and e[int(0.99 * (len(e) - 1))] <= 5.0, sid, "AN EMPTY BRACKET READS ABOUT ZERO: %d stamps behind a job that recorded nothing (AMD's barrier jobs, and the head of the list's second call): %.2f us at the median, %.2f at the 99th percentile" % (
            len(e), e[len(e) // 2], e[int(0.99 * (len(e) - 1))]))
    print("  THE OUTER SPAN, by the cost marks' own bracket (two stamps of the helper's, outside the library). GPU us, the middle round (least to largest round):")
    for g in ("Q", "D"):
        for which, cls, what in (("prep", "steady", "the prepare"), ("disp", "steady", "the steady dispatch"), ("disp", "reset", "the reset dispatch"), ("disp", "k1", "the first after a reset")):
            a, o, t_, n_ = (OUT[x % g].get((which, cls)) for x in ("A%s3", "O%s3", "T%s3", "N%s3"))
            print("    %-8s %-24s AMD's library %-20s ours, stamps OFF %-20s ours, stamps ON %-20s ours, stamps on, no barrier %s" % (gname[g].split("(")[1].rstrip(")"), what, cellw(a), cellw(o), cellw(t_), cellw(n_)))
            if not (a and o and t_): proof(False, "outer", "%s, %s: a session gave no value" % (g, what)); continue
            ma, mo, mt = C.med(a), C.med(o), C.med(t_)
            proof(abs(mo - ma) <= max(20.0, 0.02 * ma), "outer", "%s, %s: WITH THE STAMPS OFF OURS COSTS WHAT AMD'S LIBRARY COSTS: %.0f us against %.0f (%+.0f us; 2 percent or 20 us allowed)" % (gname[g], what, mo, ma, mo - ma))
            proof(mt - mo <= max(30.0, 0.05 * mo) and mt - mo >= -max(20.0, 0.02 * mo), "outer", "%s, %s: THE STAMPS STRETCH IT BY %+.0f us (%+.1f percent): %.0f us with them, %.0f without (5 percent or 30 us allowed: the table attributes, the span with the stamps off judges)" % (
                gname[g], what, mt - mo, 100.0 * (mt - mo) / mo, mt, mo))
    print("  THE PICTURE, the whole generated image by its hash, pair by pair (arms %s, %d pairs each):" % (HARMS, HASH))
    for g in ("Q", "D"):
        ref = S["A%s3" % g]
        for other, what in (("O%s3" % g, "ours with the stamps OFF"), ("T%s3" % g, "ours with the stamps ON"), ("N%s3" % g, "ours with the stamps on and no barrier")):
            cmp_ = C.compare_hash(ref["hash"], S[other]["hash"])
            n = sum(v[0] for v in cmp_.values()); same = sum(v[2] for v in cmp_.values())
            proof(n >= len(HARMS.split(",")) * HASH - 2 and same == n, other, "%s, %s gives AMD'S LIBRARY'S IMAGES: %d of %d pairs the same in all four channels (%s)" % (gname[g], what, same, n, C.hash_words(cmp_)))
        cmp_ = C.compare_hash(S["O%s2" % g]["hash"], S["T%s2" % g]["hash"])
        n = sum(v[0] for v in cmp_.values()); same = sum(v[2] for v in cmp_.values())
        proof(n >= len(HARMS.split(",")) * HASH - 2 and same == n, "T%s2" % g, "%s, the two-image set: the stamps on give the images of the stamps off: %d of %d pairs the same in all four channels" % (gname[g], same, n))
    print("  THE BARRIER BEFORE EACH STAMP: what it changes, pass by pass (a steady dispatch; GPU us with it, and without it):")
    for g in ("Q", "D"):
        a, b = T["T%s3" % g], T["N%s3" % g]
        diffs = []
        for key in KEYS:
            x, y = val(a, key, "steady"), val(b, key, "steady")
            if x is None or y is None: continue
            if abs(x - y) > max(5.0, 0.05 * max(x, y)): diffs.append("%s %.0f against %.0f" % (key, x, y))
        sa = sum(val(a, k, "steady") or 0.0 for k in KEYS); sb = sum(val(b, k, "steady") or 0.0 for k in KEYS)
        print("    %s: the pair's passes sum to %.0f us with the barriers and %.0f without; passes that read more than 5 us and 5 percent apart: %s" % (gname[g], sa, sb, "; ".join(diffs) or "none"))

    # ---- in plain words
    print("\nIN PLAIN WORDS")
    for g, sid in (("Quality", "TQ3"), ("DLAA", "TD3")):
        t = T[sid]
        cells = sorted(((val(t, k, "steady") or 0.0, k) for k in KEYS), reverse=True)
        tot = sum(c[0] for c in cells)
        if not tot: continue
        share = lambda keys: 100.0 * sum(val(t, k, "steady") or 0.0 for k in keys) / tot
        print("  %s guides, a steady pair of %.0f us: the three passes that cost most are %s. Optical flow's search (O4 to O6) is %.1f percent of the pair, all of optical flow (O1 to O8) %.1f; the unconditional copy (I11) %.1f;"
              " the inpainting (I9 and I10) %.1f; the prepare (P0, P1) %.1f; the interpolation's passes (I1 to I11) %.1f." % (
                  g, tot, ", ".join("%s %s (%.0f us, %.1f percent)" % (k, NAME[k], v, 100.0 * v / tot) for v, k in cells[:3]), share(["O4", "O5", "O6"]), share(["O%d" % i for i in range(1, 9)]),
                  share(["I11"]), share(["I9", "I10"]), share(["P0", "P1", "X1"]), share(["I%d" % i for i in range(1, 12)])))
    if CSV_OUT:
        with open(CSV_OUT, "w", newline="", encoding="ascii") as f:
            w = csv.writer(f); w.writerow(["set", "guides", "class", "pass", "name", "round", "gpu_us"])
            for r in rows_csv: w.writerow(r)
        print("\nevery cell's rounds: %s (%d rows)" % (CSV_OUT, len(rows_csv)))
    left = C.end_ours("left at the end") if C.ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    for sid, l in C.fails[:30]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    print("%d proofs passed, %d failed, in %.0f s; the folder is kept: %s" % (len(C.passes), len(C.fails), time.time() - t0, C.OUT))
    print("PASS" if not C.fails else "FAIL")
    return 0 if not C.fails else 1


# ---------------------------------------------------------------------------------------------------------------- the cuts
def main_cut():
    """THE PRICE OF THE CUTS: two libraries of the mod's own interface, before against after, by pass."""
    if not READ:
        if not KEEP: shutil.rmtree(C.OUT, ignore_errors=True)
        os.makedirs(C.OUT, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(C.OUT, "result-read.txt" if READ else "result.txt"))
    print("the price of the cuts, before against after, by pass, %s%s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), ", QUICK: THE INSTRUMENT, NOT NUMBERS" if QUICK else ""))
    t0 = time.time()
    src = {"before": os.path.abspath(BEFORE) if BEFORE else C.LIBS["ref"][1], "after": os.path.abspath(AFTER) if AFTER else C.LIBS["ours"][1]}
    for k, p in src.items():
        if not proof(os.path.exists(p), "build", "the library %s is there: %s" % (k.upper(), p)):
            print("FAIL"); return 1
    if not READ:
        if C.ours(): C.end_ours("left by an earlier run")
        for sub in ("full", "bare", "own", "beside", "src"):
            if os.path.isdir(os.path.join(C.OUT, sub)): shutil.rmtree(os.path.join(C.OUT, sub))
        if C.prepare() is None:
            print("FAIL"); return 1
        os.makedirs(os.path.join(C.OUT, "src"))
    # the two libraries under two names of this tool's own (two builds of one name could not be staged side by side): COPIES
    what = {}
    for k, p in src.items():
        d = os.path.join(C.OUT, "src", "cut-%s.dll" % k)
        if not READ: shutil.copyfile(p, d)
        cuts = os.path.join(os.path.dirname(p), "cuts.txt")
        names = [l.split()[1] for l in open(cuts, encoding="ascii").read().splitlines() if l.startswith("cut ")] if os.path.exists(cuts) else None
        what[k] = "%s, %d bytes, SHA-256 %s; its cuts, by the build's list beside it: %s" % (p, os.path.getsize(p), C.sha256_of(p), "NO LIST (the reference has none: no cut)" if names is None else "%d%s" % (len(names), (": " + ", ".join(names)) if names else ""))
        C.LIBS[k] = ("%s: %s" % (k.upper(), os.path.basename(p)), d)
        print("%s: %s" % (k.upper(), what[k]))
    C.OWN_IFACE = tuple(C.OWN_IFACE) + ("before", "after")
    proof(C.sha256_of(src["before"]) != C.sha256_of(src["after"]), "build", "the two libraries are two files (with one file on both sides the table below is the instrument's own null)")

    gname = {"Q": "the guides at 2560x1440 (Quality)", "D": "the guides at 3840x2160 (DLAA)"}
    hname = {1: "HUDLESS own (a three-image set)", 0: "no HUD-less image (a two-image set, the product's shape)"}
    S = {}; plan = []
    for hud in (0, 1) if THREE else (0,):
        for g, gi in (("Q", 1), ("D", 0)):
            for stamps, st in ((1, "T"),) if STAMPED else ((1, "T"), (0, "O")):
                for lib in ("before", "after"):
                    plan.append(("%s%s%d-%s" % (st, g, 3 if hud else 2, lib), lib, gi, hud, stamps, "%s, %s; %s; %s" % (gname[g], hname[hud], lib.upper(), "THE STAMPS ON, a barrier before each" if stamps else "the stamps OFF")))
    for sid, lb, gi, hud, stamps, title in plan:
        s = run_session(sid, title, lb, gi, hud, stamps)
        if s is not None: S[sid] = s
    if len(S) != len(plan):
        print("FAIL: %d of %d sessions were not run" % (len(plan) - len(S), len(plan))); return 1
    T = {sid: table_of(s) for sid, s in S.items() if s["stamps"]}
    OUT = {sid: outer_of(s) for sid, s in S.items()}
    for sid, s in S.items():
        e = s["end"]
        proof(e is not None and e[2] == 0 and e[3] == 0 and e[4] == 0 and e[0] == e[1], sid, "every list of the own arm's was read (%s lists, %s read, %s not read), no stamp was lost (%s) and no row (%s)" % (e if e else ("?",) * 5))
    for sid, t in T.items():
        proof(not t["unknown"], sid, "every compute pass, clear and copy the stamps named has a row in the table%s" % ((": NO ROW FOR " + "; ".join(sorted(t["unknown"]))) if t["unknown"] else ""))
        d = sorted(abs(x[0]) for x in t["sumdiff"]); rel = sorted(abs(x[0]) / max(1.0, x[1]) for x in t["sumdiff"])
        if not d: proof(False, sid, "THE SUM: no list had both the stamps and the marks' bracket"); continue
        p50, p99 = d[len(d) // 2], d[int(0.99 * (len(d) - 1))]
        proof(p50 <= 10.0 and (p99 <= 10.0 or rel[int(0.99 * (len(d) - 1))] <= 0.01), sid, "THE SUM: in %d lists the stamped spans add up to the cost marks' own bracket round the same calls: %.1f us apart at the median, %.1f at the 99th percentile" % (len(d), p50, p99))
    sets = sorted({"%s: %s" % (s["lib"], s["set"]) for s in S.values() if s.get("set")})
    for x in sets: print("THE SET, by the helper's own line: %s" % x)

    print("\n" + "=" * 118)
    print("THE PRICE OF THE CUTS, BY PASS. 3840x2160 FP16 (scRGB), the cost bench's moving picture, every part of a pair alone on the card. GPU us by a timestamp behind every job: the middle round (least to largest round, %d rounds)." % ROUNDS)
    print("BEFORE: %s" % what["before"]); print("AFTER:  %s" % what["after"])
    rows_csv = []; saved = {}
    dw = lambda a, b: "-" if (a is None or b is None) else "%+.0f" % (b - a)
    for hud in (0, 1) if THREE else (0,):
        n = 3 if hud else 2
        tb = {g: T["T%s%d-before" % (g, n)] for g in "QD"}; ta = {g: T["T%s%d-after" % (g, n)] for g in "QD"}
        for cls, title in (("steady", "A STEADY DISPATCH (the %dth after a reset or later) and its pair's prepare" % STEADY), ("reset", "A RESET DISPATCH and a prepare told to reset"), ("k1", "THE FIRST DISPATCH AFTER A RESET (prepare: as steady)")):
            pcls = "steady" if cls.startswith("k") else cls
            print("\n%s, %s." % (hname[hud].upper(), title))
            print("  %-4s %-40s | %-18s %-18s %6s | %-18s %-18s %6s" % ("", "pass", "Quality: before", "after", "change", "DLAA: before", "after", "change"))
            tot = {(g, w): 0.0 for g in "QD" for w in "ba"}
            for key, name, rx, call, eq, ed in PASSES:
                c = pcls if call == 1 else cls
                v = {(g, w): t[g]["cell"].get((key, c)) for g in "QD" for w, t in (("b", tb), ("a", ta))}
                if not any(v.values()): continue
                m = {k: (C.med(x) if x else None) for k, x in v.items()}
                for k, x in m.items(): tot[k] += x or 0.0
                # (a pass the cut removed has no row after: it counts as 0 there)
                sq = (m[("Q", "b")] or 0.0) - (m[("Q", "a")] or 0.0); sd = (m[("D", "b")] or 0.0) - (m[("D", "a")] or 0.0)
                mark = "  <" if max(abs(sq), abs(sd)) >= 3.0 else ""
                print("  %-4s %-40s | %-18s %-18s %6s | %-18s %-18s %6s%s" % (key, name[:40], cellw(v[("Q", "b")]), cellw(v[("Q", "a")]), "%+.0f" % -sq, cellw(v[("D", "b")]), cellw(v[("D", "a")]), "%+.0f" % -sd, mark))
                for (g, w), x in v.items():
                    for i, y in enumerate(x or []): rows_csv.append(("three-image" if hud else "two-image", g, cls, key, name, "before" if w == "b" else "after", i, "%.2f" % y))
            for call in (1, 2, 3):
                c = pcls if call == 1 else cls
                v = {(g, w): t[g]["calls"].get((call, c)) for g in "QD" for w, t in (("b", tb), ("a", ta))}
                m = {k: (C.med(x) if x else None) for k, x in v.items()}
                print("  %-4s %-40s | %-18s %-18s %6s | %-18s %-18s %6s" % ("", "THE CALL: " + CALLS[call], cellw(v[("Q", "b")]), cellw(v[("Q", "a")]), dw(m[("Q", "b")], m[("Q", "a")]), cellw(v[("D", "b")]), cellw(v[("D", "a")]), dw(m[("D", "b")], m[("D", "a")])))
            print("  %-4s %-40s | %-18s %-18s %6s | %-18s %-18s %6s" % ("", "THE PAIR (the passes' sum)", C.fmt(tot[("Q", "b")]), C.fmt(tot[("Q", "a")]), "%+.0f" % (tot[("Q", "a")] - tot[("Q", "b")]), C.fmt(tot[("D", "b")]), C.fmt(tot[("D", "a")]), "%+.0f" % (tot[("D", "a")] - tot[("D", "b")])))
            if cls == "steady": saved[hud] = (tot[("Q", "b")] - tot[("Q", "a")], tot[("D", "b")] - tot[("D", "a")])
        # the outer span: the marks' own bracket, outside the library, with the stamps OFF and ON
        print("\n  THE OUTER SPAN, %s, by the cost marks' own bracket (two stamps of the helper's, outside the library): what a pair costs with nothing of the stamps in it. GPU us." % hname[hud])
        for which, cls, title in (("prep", "steady", "the prepare"), ("disp", "steady", "the steady dispatch"), ("disp", "reset", "the reset dispatch"), ("disp", "k1", "the first after a reset")):
            for st, stw in (("T", "stamps on"),) if STAMPED else (("O", "stamps OFF"), ("T", "stamps on")):
                v = {(g, w): OUT["%s%s%d-%s" % (st, g, n, w)].get((which, cls)) for g in "QD" for w in ("before", "after")}
                m = {k: (C.med(x) if x else None) for k, x in v.items()}
                print("    %-24s %-10s | %-18s %-18s %6s | %-18s %-18s %6s" % (title, stw, cellw(v[("Q", "before")]), cellw(v[("Q", "after")]), dw(m[("Q", "before")], m[("Q", "after")]), cellw(v[("D", "before")]), cellw(v[("D", "after")]), dw(m[("D", "before")], m[("D", "after")])))
                if st == "O" and which == "disp" and cls == "steady" and all(x is not None for x in m.values()):
                    pb = {g: C.med(OUT["O%s%d-before" % (g, n)].get(("prep", "steady")) or [0.0]) for g in "QD"}; pa = {g: C.med(OUT["O%s%d-after" % (g, n)].get(("prep", "steady")) or [0.0]) for g in "QD"}
                    print("    %-24s %-10s | %-18s %-18s %6s | %-18s %-18s %6s   <== THE PAIR, the judge of the saving" % ("THE PAIR (prepare + dispatch)", stw, C.fmt(pb["Q"] + m[("Q", "before")]), C.fmt(pa["Q"] + m[("Q", "after")]), "%+.0f" % (pa["Q"] + m[("Q", "after")] - pb["Q"] - m[("Q", "before")]),
                                                                                                                               C.fmt(pb["D"] + m[("D", "before")]), C.fmt(pa["D"] + m[("D", "after")]), "%+.0f" % (pa["D"] + m[("D", "after")] - pb["D"] - m[("D", "before")])))
        # the picture: the two libraries' images, by the whole-image hash
        for g in "QD":
            for st in ("T",) if STAMPED else ("T", "O"):
                a, b = S["%s%s%d-before" % (st, g, n)], S["%s%s%d-after" % (st, g, n)]
                cmp_ = C.compare_hash(a["hash"], b["hash"])
                cnt = sum(v[0] for v in cmp_.values()); same = sum(v[2] for v in cmp_.values())
                proof(cnt >= len(HARMS.split(",")) * HASH - 2 and same == cnt, b["sid"], "%s, %s, stamps %s: AFTER GIVES BEFORE'S IMAGES: %d of %d hashed pairs the same in all four channels (%s)" % (gname[g], hname[hud], "on" if st == "T" else "off", same, cnt, C.hash_words(cmp_)))
    if CSV_OUT:
        with open(CSV_OUT, "w", newline="", encoding="ascii") as f:
            w = csv.writer(f); w.writerow(["set", "guides", "class", "pass", "name", "library", "round", "gpu_us"])
            for r in rows_csv: w.writerow(r)
        print("\nevery cell's rounds: %s (%d rows)" % (CSV_OUT, len(rows_csv)))
    left = C.end_ours("left at the end") if C.ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    for sid, l in C.fails[:30]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    print("%d proofs passed, %d failed, in %.0f s; the folder is kept: %s" % (len(C.passes), len(C.fails), time.time() - t0, C.OUT))
    if STAMPED: proof(False, "scope", "--stamped: THE STAMPED SESSIONS ALONE: the outer span with the stamps off, which judges a saving, was not taken")
    if not C.fails: print("PASS: a steady pair's stamped passes, the two-image set: AFTER is %+.0f us at Quality's guides and %+.0f us at DLAA's against BEFORE; the same images by every hash." % (-saved[0][0], -saved[0][1]))
    else: print("FAIL")
    return 0 if not C.fails else 1


if __name__ == "__main__":
    sys.exit(main_cut() if CUT else main())
