r"""THE OWN BUILD OF AMD'S FRAME GENERATION AGAINST AMD'S SIGNED LIBRARY, ON THE CARD, PAIR BY PAIR (2026-10-06, frame
generation phase 3, the own build's stage 1b: THE REPRODUCTION; docs\DECISIONS.md, "PHASE 3: OUR OWN BUILD OF AMD'S FRAME
GENERATION", section 4, stage 1; docs\reports\fg-3\own-build-2026-10-06\stage-1a-review\skeptic.md, list c).

THE QUESTION. ffx-own-fg.dll is AMD's unmodified source built by this project (build-ffx-own.ps1). Does it generate THE SAME
PICTURE as AMD's signed library, on the same inputs? The answer is a hash of the WHOLE generated image behind every pair
(the cost bench's hash part: tools\test-fg-pipe\fgpipe_cost.h, --fake-fg=40), one session a library, the lists held
against each other. Nothing of AMD's binary is opened: both libraries are used through their five entry points.

THE ORDER IS THE SKEPTIC'S, and a later step is not read while an earlier one fails:
  1. REFUSALS. The development load (host\fg_lib.h, --fglib) refuses and NEVER FALLS BACK, with AMD's own frame generation
     file lying beside the helper: a path that does not exist, a wrong expected hash, a file with no ffx-api entry point,
     a path that is not full. Each is one line, frame generation not offered, and no module of AMD's name mapped.
  2. THE DEBUG LAYER (--d3d12_debug). Its own control first (a junk pipeline must count as a message), then the create and
     a few pairs on AMD's library (the control of the instrument on real work), then on ours: no error, no corruption, and
     no message ours has that AMD's does not. AMD's code throws away the result of each of its 18 pipeline creations, so
     this, and not "the create answered OK", is what says the pipelines exist.
  3. THE DEVICE'S LINE: shader model, wave lanes, 16-bit support: which shader variants AMD's code takes on this card.
  4. THE CONTEXT'S MEMORY FIGURE: ours equals AMD's in each configuration.
  5. AMD'S AGAINST AMD'S: through the loader twice, and loaded direct. Equal hash lists, or the spread is what ours is held to.
  6. THE TWO ALTERED BUILDS MUST FAIL on steady pairs (from the 12th dispatch after a reset, on the moving picture): one is
     changed in an interpolation shader, one in an optical flow shader. An altered build that passes means the compare is
     blind to that half of the passes.
  7. ONLY THEN OURS AGAINST AMD'S: steady pairs, reset pairs, pairs 1 to 11 after a reset, by the helper's own count of
     dispatches since the last reset; FP16 and 8-bit; DLAA and Quality guides; every move of the bench's picture; and once
     with AMD's file lying in the helper's folder, where it must stay unmapped.
EVERY SESSION CARRIES THE IDENTITY GATE (tools\test-fg-cost\run.py, check_identity): which module generated its frames, by
the helper's own line (full path, size, SHA-256), held to the file that was staged.
WHERE TWO LISTS DIFFER the pair is run again on both libraries with its whole image written out, and the two files say
how many pixels differ and by how many steps of the format (--raw-all does that for every differing class; without it,
for the altered builds' first steady pair and for anything ours differs in).

THE CONFIGURATIONS (all 3840x2160, each on A TWO-IMAGE SET, the game's shape since the wire's revision of 2026-10-06: no
HUD-less image, one copy a pair): DF the guides at 3840x2160 (DLAA), FP16; QF at 2560x1440 (Quality), FP16; D8 and Q8 the
same with a B8G8R8A8 back buffer; and D3, DLAA and FP16 on a THREE-image set (the shape before that revision), ours
against AMD's alone. THE ARMS of every session: F (a reset, the way back, steady pairs), FK (a reset every
17th pair), FR (every pair reset), F1 (every second), FN (negated vectors), FC (vectors that collide), FS (nothing moves).

    python tools\test-fg-own\run.py              THE PROOF: every control in all four configurations
    python tools\test-fg-own\run.py --ours       THE SECOND LIBRARY'S PROOF (the own build's stage 2, 2026-10-07): dlssiv-fg.dll,
                                                 the same objects of AMD's behind THE MOD'S OWN INTERFACE (three calls a pair,
                                                 the helper's --fgown), held against AMD's signed library AND against stage 1's
                                                 library, pair by pair, with every control above in its own shape: the refusals
                                                 of --fgown, the debug layer, AMD's against AMD's, the two altered builds OF THAT
                                                 LIBRARY (they must fail), the identity gate, an upscaler alive beside it. Its
                                                 folder is its own (%TEMP%\dlssiv-test-fg-ours)
    python tools\test-fg-own\run.py --quick      the controls in DF alone, QF, D8 and Q8 ours against AMD's only (it ends FAIL:
                                                 not the whole proof)
    python tools\test-fg-own\run.py --cost       ALSO part 5: the cost of a pair, ours against AMD's, on the bench's idle
                                                 table (timing: needs the card to itself; three rounds a cell, two sessions
                                                 a library, the CPU's load and the compilers beside each)
    python tools\test-fg-own\run.py --cost-only  the cost tables alone (it ends FAIL: not the whole proof)
    ... --only=DF,Q8                             the named configurations (it ends FAIL: not the whole proof)
    ... --pairs=N                                pairs an arm (48)
    ... --own-only                               every configuration with AMD's once and ours once, no control (it ends FAIL)
    ... --keep                                   do not empty this tool's folder first
    ... --resume                                 a session the folder already holds whole is read and not run again: a run
                                                 that was cut short, or that another GPU user held up, goes on where it stood
    ... --read                                   run nothing: read what the folder holds
SINCE THE OWN BUILD'S STAGE 3 (2026-10-10) --ours IS THE JUDGE OF THE CUTS. dlssiv-fg.dll is then THE PRODUCT: the second
library with the cuts that are in (ffx-own\cuts; build-ffx-own.ps1 -Target dlssiv-fg), and a cut may not change one bit
of a generated image. The same build delivers THE REFERENCE, stage 2's library with no cut (dlssiv-fg-ref.dll). --ours
holds every whole-image hash of the product to AMD's signed library, to stage 1's AND TO THE REFERENCE'S, in every
configuration, with every control above in force, AND WITH ONE MORE: dlssiv-fg-px.dll, the product's cuts and ONE PIXEL
of every generated image changed (ffx-own\cuts\control-one-pixel.cut), MUST FAIL on every pair of every class, and the
whole images of a steady and of a reset pair must differ in exactly one pixel. A judge that passed it could not see a
cut that changes a pixel.
SINCE 2026-10-12 THE PRODUCT'S PICTURE IS ITS OWN, AND --ours HOLDS TWO LAWS (the owner accepted in the game a library with three
cuts that change the picture, ffx-own\cuts\CUTS.txt: "accepted", and it is the product; docs\reports\fg-3\sitting-2026-10-12b):
  THE EXACT CUTS (CUTS.txt: "in") ARE PROVED AS BEFORE, ON THE EXACT LIBRARY: dlssiv-fg-exact.dll, the exact cuts and no picture
     cut, built by the same run as the product (obj\dlssiv-fg-exact; tools\ffx-own\RECORD-dlssiv-fg-exact.txt), the bytes
     the product had until that day. EVERYTHING ABOVE IS SAID OF IT, unchanged: every whole-image hash against AMD's
     signed library, stage 1's and the reference's in every configuration, every control; the two altered libraries
     and the one-pixel control are built on ITS cuts. Nothing of that proof was lost or loosened.
  THE PRODUCT'S PICTURES ARE PINNED (step 9). dlssiv-fg.dll (RECORD-dlssiv-fg-cut.txt) runs the same pairs of every
     configuration, twice: it must give the same image twice, and one SHA-256 a configuration over its pairs' whole-
     image hashes (each pair with its arm, its index and its count of dispatches since a reset) must be the one in
     tools\ffx-own\PICTURES-dlssiv-fg.txt. ANY CHANGE OF THE PRODUCT'S PICTURE, WANTED OR NOT, FAILS HERE until somebody says
     it is wanted by making the pin again. Its controls: the exact library's digest of the same pairs is NOT the pin
     (the pin holds the product's own picture, and the picture cuts reach the judge's pictures in that configuration);
     the one-pixel control's digest is not the exact library's (one pixel in one image moves a digest). And the life
     of the product itself: the debug layer on it (no error, no corruption), AMD's file lying beside it, by its bare
     name beside the helper, and an upscaler alive beside it, each with the pictures it gives alone.
    python tools\test-fg-own\run.py --ours --new-pictures   MAKES THE PIN AGAIN, ON PURPOSE, from the product's sessions alone
                                                 (each configuration twice, which must agree), and prints every value
                                                 it replaced. That run is never a PASS: commit the file, run the judge.
    python tools\test-fg-own\run.py --ours --pictures       the product's pictures against the pin and nothing else, one
                                                 session a configuration (about a minute: the gate's step). Its last
                                                 line is "PASS: THE PRODUCT'S PICTURES ARE THE PINNED ONES ..." or FAIL
    ... --pictures --pictures-of=<library>       ALSO another library of the mod's interface, by its full path, held to the
                                                 same pin and to the product pair by pair (a frozen variant: v-ship)
  THE POSITION'S CONTROL (2026-10-17, step 6c; ffx-own\cuts\fi-position.cut). Since the exact cut fi-position the place of the
     generated frame between its two real frames is a constant of the dispatch, and every caller asks for the middle, where
     the exact library must equal the reference. That proves the cut exact only if the constant reaches the shaders, so a
     control asks for another place: dlssiv-fg-pos.dll, the exact library's cuts with the glue telling AMD's dispatch 0.4 for
     the middle (ffx-own\cuts\control-position.cut; build-ffx-own.ps1 -Target dlssiv-fg -Cuts exact,+control-position
     -OutDir obj\dlssiv-fg-pos, held here under that name). It MUST DIFFER from AMD's on every arm whose picture moves
     (F, FK, F1, FN, FC) and must EQUAL it on the reset pairs, where a position cannot show.
  WHAT A NEW CUT DOES: an exact one changes the exact library's record and the product's and NOT the pin (the judge must
  say both: equal to the reference, and the product's pictures still the pinned ones); a picture cut the owner
  accepted changes the product's record and the pin, each made again on purpose (ffx-own\cuts\CUTS.txt says the steps).
exit code 0 = every step held. The last line is PASS or FAIL. What it printed is kept as result.txt in its folder
(%TEMP%\dlssiv-test-fg-own). NOT part of tools\prove-all.py: it takes the card for minutes.
ONE GPU USER AT A TIME: every session waits while another bench's helper or fake client runs (30 minutes at most).
"""
import datetime, importlib.util, os, re, shutil, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
QUICK = "--quick" in ARGS
OURS = "--ours" in ARGS                # THE SECOND LIBRARY (stage 2): dlssiv-fg.dll through the mod's own interface is what is held to AMD's
SUBJ = "exact" if OURS else "own"      # the library this run holds to AMD's (--ours: THE EXACT LIBRARY since 2026-10-12, when the product's picture became its own: the paragraph "SINCE 2026-10-12" above)
SUBJ_FILE = "dlssiv-fg-exact.dll" if OURS else "ffx-own-fg.dll"
NEW_PICTURES = "--new-pictures" in ARGS   # --ours: the pin of the product's pictures is made again, ON PURPOSE, from the product's sessions alone (never a PASS)
PICTURES = "--pictures" in ARGS           # --ours: the product's pictures against their pin, and nothing else (the gate's quick step)
PICTURES_OF = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--pictures-of=")), None)   # with --pictures: ANOTHER library of the mod's interface, by its full path, held to the same pin
PIN = os.path.join(ROOT, "tools", "ffx-own", "PICTURES-dlssiv-fg.txt")
ALTS = (("ours-alt-fi", "the interpolation shader"), ("ours-alt-of", "optical flow's search")) if OURS else (("alt-fi", "the interpolation shader"), ("alt-of", "optical flow's search"))
COST = "--cost" in ARGS
COST_ONLY = "--cost-only" in ARGS      # the cost tables alone (the run then ends FAIL: not the whole proof)
OWN_ONLY = "--own-only" in ARGS        # every configuration with AMD's once and ours once, no control (the run then ends FAIL: not the whole proof)
RESUME = "--resume" in ARGS            # a session this folder already holds whole is read, not run again (with --keep: a run that was cut short goes on)
READ = "--read" in ARGS
KEEP = "--keep" in ARGS or READ or "--resume" in ARGS
RAW_ALL = "--raw-all" in ARGS
NO_ALT = "--no-alt" in ARGS            # development: the two altered builds left out (the run then FAILS, and says why)
ONLY = next((a.split("=", 1)[1].split(",") for a in ARGS if a.startswith("--only=")), None)
PAIRS = int(next((a.split("=", 1)[1] for a in ARGS if a.startswith("--pairs=")), "48"))
W, H = 3840, 2160
STEADY = 12
ARMS = "F,FK,FR,F1,FN,FC,FS"
MOVING = ["F", "FK", "F1", "FN", "FC"]   # (2026-10-17) the arms whose picture moves between two real frames: FR resets every pair, FS holds still
# (2026-10-17) THE POSITION'S CONTROL (the cut fi-position; ffx-own\cuts\control-position.cut), a library of the mod's own interface the
# cost bench does not list: build-ffx-own.ps1 -Target dlssiv-fg -Cuts exact,+control-position -OutDir obj\dlssiv-fg-pos
# The build leaves it as dlssiv-fg.dll, the product's file name, and the cost bench stages a library by its file name: so it is
# held under a name of its own beside the build, dlssiv-fg-pos.dll (pos_lib() below makes that copy and checks what it is).
POS_BUILT = os.path.join(ROOT, "obj", "dlssiv-fg-pos", "dlssiv-fg.dll")
POS_LIB = ("ours-pos", "dlssiv-fg-pos.dll: the exact library's cuts, and the glue tells AMD's dispatch the position 0.4 where the caller says the middle (a control)",
           os.path.join(ROOT, "obj", "dlssiv-fg-pos", "dlssiv-fg-pos.dll"))
# THE SET'S SHAPE (the wire's one revision, 2026-10-06): the game asks for a set WITHOUT the HUD-less image since lever 1, so
# its set has TWO images (PRESENT and GEN) and one copy a pair. That is the shape the four configurations run in. D3 keeps
# the shape before it (three images, both copies) under one compare of ours against AMD's.
CFG = {  # name: (what it is, --fake-cost-guides, --fake-cost-fmt, --fake-cost-hud)
    "DF": ("the guides at 3840x2160 (DLAA), FP16, a two-image set", 0, 0, 0),
    "QF": ("the guides at 2560x1440 (Quality), FP16, a two-image set", 1, 0, 0),
    "D8": ("the guides at 3840x2160 (DLAA), B8G8R8A8, a two-image set", 0, 1, 0),
    "Q8": ("the guides at 2560x1440 (Quality), B8G8R8A8, a two-image set", 1, 1, 0),
    "D3": ("the guides at 3840x2160 (DLAA), FP16, a THREE-image set (with the HUD-less image, the shape before wire 2)", 0, 0, 1),
}
CLASSES = ("steady", "reset", "1 to 11")
CUTS_WORDS = "?"                      # --ours: the cuts in the product, by the build's own list beside it (main reads it)
NO_CONTROLS = ("D3",)                 # the configurations that are ours against AMD's alone, whatever the run


def load_cost():
    """The cost bench as a module (its staging, its session, its readers, its identity gate), with none of this script's
    arguments in its sight."""
    saved = sys.argv
    sys.argv = [saved[0]] + [a for a in ARGS if a in ("--keep", "--read")]
    spec = importlib.util.spec_from_file_location("fgcost_bench", os.path.join(ROOT, "tools", "test-fg-cost", "run.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


C = load_cost()
C.LIBS[POS_LIB[0]] = POS_LIB[1:]                       # (2026-10-17) the position's control, a library of the mod's own interface
C.OWN_IFACE = tuple(C.OWN_IFACE) + (POS_LIB[0],)


def pos_lib():
    """(2026-10-17) THE POSITION'S CONTROL under its own file name: build-ffx-own.ps1 -Cuts exact,+control-position -OutDir obj\\dlssiv-fg-pos
    leaves dlssiv-fg.dll and cuts.txt there; the copy dlssiv-fg-pos.dll beside them is made again when it is not those bytes. Its
    cuts.txt must name the exact library's cuts and control-position, and nothing else. -> None, or why it cannot be used."""
    cp = os.path.join(os.path.dirname(POS_BUILT), "cuts.txt")
    if not os.path.isfile(POS_BUILT) or not os.path.isfile(cp):
        return "not built (build-ffx-own.ps1 -Target dlssiv-fg -Cuts exact,+control-position -OutDir obj\\dlssiv-fg-pos)"
    names = [l.split()[1] for l in open(cp, encoding="ascii").read().splitlines() if l.startswith("cut ")]
    sys.path.insert(0, os.path.join(ROOT, "tools", "ffx-own"))
    import cuts as K
    want = K.chosen("exact,+control-position", ROOT)
    if names != want: return "its cuts.txt names %s, not the exact library's cuts and control-position (%s): build it again" % (", ".join(names) or "none", ", ".join(want))
    if not READ and (not os.path.isfile(POS_LIB[2]) or C.sha256_of(POS_LIB[2]) != C.sha256_of(POS_BUILT)): shutil.copyfile(POS_BUILT, POS_LIB[2])
    return None


# (2026-10-11) dt_fg_test.exe COUNTS AS A GPU USER ONLY WHERE IT MAKES A DEVICE: tools\test-fg-own\devices.py says why and how it is read
sys.path.insert(0, HERE)
import devices
devices.narrow(C)
C.OUT = os.path.join(tempfile.gettempdir(), ("dlssiv-test-fg-pictures" if (PICTURES or NEW_PICTURES) else "dlssiv-test-fg-ours") if OURS else "dlssiv-test-fg-own")   # (the product's pictures alone have a folder of their own: they do not empty the whole proof's)
C.GPU_LOOK = 5                         # another bench's sessions follow each other within seconds: looked for every 5 s, so the gap between two is not missed


def held(sid):
    """--resume: this folder holds that session whole (what the fake client printed and how it ended), so it is not run again."""
    if not (RESUME and os.path.exists(C.P(sid, "out.txt")) and (os.path.exists(C.P(sid, "rc.txt")) or sid.startswith("R-"))): return False
    print("SESSION %s: HELD, as it ended at %s (--resume: read, not run again)" % (sid, datetime.datetime.fromtimestamp(os.path.getmtime(C.P(sid, "out.txt"))).strftime("%Y-%m-%d %H:%M:%S")))
    return True
proof = C.proof
NOWIN = subprocess.CREATE_NO_WINDOW
SESSIONS = {}                          # sid -> what it left, read once


# ---------------------------------------------------------------------------------------------------------------- sessions
def stage(lib, beside=False, by_name=False):
    C.AMD_BESIDE = beside; C.BY_NAME = by_name
    return C.stage_lib(lib)


def hash_session(cfg, lib, tag="", beside=False, helper_extra=(), pairs=None, arms=ARMS, raw=None, by_name=False, fake_extra=()):
    """One helper and one fake client: the hash part alone, on one library. What it left, or None."""
    st = stage(lib, beside, by_name)
    if st is None: return None
    what, guides, fmt, hud = CFG[cfg]
    sid = "%s-%s%s" % (cfg, lib, tag)
    fake = ["--fake-fg=40", "--fake-cost-guides=%d" % guides, "--fake-cost-fmt=%d" % fmt, "--fake-cost-hud=%d" % hud, "--fake-cost-idle=0", "--fake-cost-hash=%d" % (pairs or PAIRS), "--fake-cost-harms=" + arms] + list(fake_extra)
    if raw: fake += ["--fake-cost-raw=" + ",".join("%s:%d" % k for k in raw), "--fake-cost-rawout=" + C.P(sid, "raw")]
    helper = ["--fg=1", "--gpu_probe=1"] + st["args"] + list(helper_extra)
    # (a session that writes whole images out is never "held": its images are removed when they have been compared, and
    # the next compare of the run asks the same session for other pairs)
    if not READ and (raw or not held(sid)): C.session(st["dir"], sid, "%s; %s%s" % (what, st["what"], "; AMD's file lying in the helper's folder" if st["own"] and st["beside"] else ""), helper, fake)
    s = C.read_session(sid)
    if s is None:
        proof(False, sid, "this session left nothing: it was not run"); return None
    s["st"] = st; s["cfg"] = cfg; s["lib"] = lib
    ok = C.check_session(s, True)
    ok = C.check_identity(s, st) and ok
    if s["others"]: print("  [%s] ANOTHER GPU USER RAN BESIDE THIS SESSION (it waited 30 minutes first): %s" % (sid, s["others"]))
    s["ok"] = ok
    # every hashed pair, by the helper's own count of dispatches since the last reset
    s["pair"] = {}
    for key, r in s["hash"].items():
        d = s["hD"].get(int(r["n"]))
        since = d["since"] if d else None
        cls = None if since is None else "reset" if since == 0 else "steady" if since >= STEADY else "1 to 11"
        s["pair"][key] = {"n": int(r["n"]), "since": since, "cls": cls, "all": r["all"], "colour": r["colour"], "blocks": r["blocks"], "move": s["move"].get(int(r["n"]), -9)}
    proof(len(s["pair"]) >= 20 and all(p["cls"] for p in s["pair"].values()), sid, "%d pairs were hashed whole, and the helper's own rows say for each how many dispatches after a reset it was" % len(s["pair"]))
    m = re.search(r"frame generation: fgid: set (\d+)x(\d+) format (\d+), guides (\d+)x(\d+): context (\d+) bytes", s["log"])
    s["ctx"] = int(m.group(6)) if m else None
    m = re.search(r"frame generation: device: (.*)", s["log"])
    s["device"] = m.group(1).strip() if m else None
    SESSIONS[sid] = s
    return s


def hello_session(sid, helper_dir, helper_args, want):
    """A session that ends after the handshake: the helper started with the given arguments, and what the handshake said of
    frame generation must be `want` (1: asked, nothing usable; 3: offered). The helper's log."""
    log, out = C.P(sid, "helper.log"), C.P(sid, "out.txt")
    if not READ and not held(sid):
        for p in (log, out):
            if os.path.exists(p): os.remove(p)
        waited = 0
        while C.other_gpu_users() and waited < 1800:
            if waited == 0: print("  another GPU user is running: waiting...")
            time.sleep(C.GPU_LOOK); waited += C.GPU_LOOK
        r = subprocess.run([os.path.join(C.OUT, "fgcost.exe"), os.path.join(helper_dir, "dlssiv-helper.exe"), log, "--fake-hello-only", "--fake-want-fg=%d" % want, "--fg=1"] + list(helper_args),
                           capture_output=True, text=True, errors="replace", cwd=helper_dir, creationflags=NOWIN, timeout=120)
        open(out, "w", encoding="utf-8").write(r.stdout + "\nrc=%d\n" % r.returncode)
        if C.ours(): C.end_ours("left after session %s" % sid)
    o = open(out, encoding="utf-8", errors="replace").read() if os.path.exists(out) else ""
    text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
    return text, ("rc=0" in o and "\nPASS" in o and "  FAIL  " not in o), o


# ---------------------------------------------------------------------------------------------------------------- comparing
def compare(a, b):
    """Two sessions' hash lists, pair by pair: {class: [pairs, the same in the three colours, the same in all four channels]},
    the differing pairs' keys by class, and whether the two helpers counted every pair the same way."""
    out = {c: [0, 0, 0] for c in CLASSES}; diff = {c: [] for c in CLASSES}; same_count = True
    for key in sorted(set(a["pair"]) & set(b["pair"])):
        x, y = a["pair"][key], b["pair"][key]
        if x["since"] != y["since"] or x["move"] != y["move"]: same_count = False
        c = x["cls"]
        if c is None: same_count = False; continue
        out[c][0] += 1
        if x["colour"] == y["colour"]: out[c][1] += 1
        else: diff[c].append(key)
        if x["all"] == y["all"]: out[c][2] += 1
    return out, diff, same_count and len(a["pair"]) == len(b["pair"])


def words(cmp):
    return "; ".join("%s: %d of %d pairs the same in the three colours (%d in all four channels)" % (c, v[1], v[0], v[2]) for c, v in cmp.items())


def equal(cmp):
    return all(v[0] > 0 and v[1] == v[0] and v[2] == v[0] for v in cmp.values())


def moves_of(s, cls):
    return sorted({p["move"] for p in s["pair"].values() if p["cls"] == cls})


def raw_diff(pa, pb, fmt):
    """Two whole images: (pixels that differ in a colour, the largest colour difference in steps of the format, pixels that
    differ in alpha alone, the 16 by 9 blocks that hold a differing pixel), or None without numpy or without the files."""
    try: import numpy as np
    except ImportError: return None
    if not (os.path.exists(pa) and os.path.exists(pb)): return None
    if fmt == 0:
        a = np.fromfile(pa, dtype=np.uint16).reshape(H, W, 4); b = np.fromfile(pb, dtype=np.uint16).reshape(H, W, 4)
        # a half float's bits in order: sign and magnitude become one signed whole number, so a difference is a count of
        # representable values between the two (steps of the half float)
        st = lambda x: np.where(x & 0x8000, -(x & 0x7FFF).astype(np.int32), (x & 0x7FFF).astype(np.int32))
        d = np.abs(st(a) - st(b))
        col, alpha = d[..., :3], d[..., 3]
    else:
        a = np.fromfile(pa, dtype=np.uint8).reshape(H, W, 4).astype(np.int16); b = np.fromfile(pb, dtype=np.uint8).reshape(H, W, 4).astype(np.int16)
        d = np.abs(a - b)
        col, alpha = d[..., :3], d[..., 3]                 # B, G, R, then alpha
    px = col.any(axis=2)
    blocks = int(sum(1 for by in range(9) for bx in range(16) if px[by * H // 9:(by + 1) * H // 9, bx * W // 16:(bx + 1) * W // 16].any()))
    return int(px.sum()), int(col.max()), int(((alpha != 0) & ~px).sum()), blocks


def raw_compare(cfg, lib_a, lib_b, keys, why, beside=False):
    """The named pairs once more on two libraries with their whole images written out, and what the files say."""
    fmt = CFG[cfg][2]
    # THE SAME SEQUENCE AS THE SESSION THAT FOUND THEM: every arm before the last one named runs whole, so each pair meets
    # the same move and the same history
    upto = ARMS.split(",")[:max(ARMS.split(",").index(k[0]) for k in keys) + 1]
    sa = hash_session(cfg, lib_a, "-rawa", raw=keys, arms=",".join(upto))
    sb = hash_session(cfg, lib_b, "-rawb", raw=keys, arms=",".join(upto), beside=beside)
    out = []
    kept = C.P(sb["sid"], "rawdiff.csv") if sb else None   # what the two files said, kept: the files themselves are removed, and --read reads this
    known = {}
    if kept and os.path.exists(kept):
        for l in open(kept):
            f = l.strip().split(",")
            if len(f) == 6: known[(f[0], int(f[1]))] = tuple(int(x) for x in f[2:])
    for arm, idx in keys:
        r = raw_diff(C.P(sa["sid"], "raw") + "-%s-%d.bin" % (arm, idx), C.P(sb["sid"], "raw") + "-%s-%d.bin" % (arm, idx), fmt) if sa and sb else None
        if r is not None:
            known[(arm, idx)] = r
            with open(kept, "w") as fh:
                for (a2, i2), v in sorted(known.items()): fh.write("%s,%d,%d,%d,%d,%d\n" % ((a2, i2) + tuple(v)))
        else: r = known.get((arm, idx))
        p = sa["pair"].get((arm, idx)) if sa else None
        if r is None: print("    %s, arm %s pair %d: the two images were not compared (no numpy, or a file is missing)" % (why, arm, idx)); continue
        print("    %s, arm %s pair %d (%s, the %sth dispatch after a reset): %d of %d pixels differ in a colour (%.4f%%), by %d step(s) of the %s at most; %d more in alpha alone; in %d of the picture's 144 blocks" % (
            why, arm, idx, p["cls"] if p else "?", p["since"] if p else "?", r[0], W * H, 100.0 * r[0] / (W * H), r[1], "half float" if fmt == 0 else "8-bit value", r[2], r[3]))
        out.append((arm, idx, r))
    for s in (sa, sb):                                     # the files are 32 or 63 MiB each: read, then gone
        if s is None: continue
        for arm, idx in keys:
            p = C.P(s["sid"], "raw") + "-%s-%d.bin" % (arm, idx)
            if os.path.exists(p) and not KEEP: os.remove(p)
    return out


# ---------------------------------------------------------------------------------------------------------------- the steps
def step_refusals(full_dir):
    print("\n1. REFUSALS: the development load refuses and never falls back (in every case with a full path, AMD's own frame generation file lies beside the helper)")
    own = stage("own", beside=True)
    named = stage("own", by_name=True)
    if own is None or named is None: return False
    wrong = "0" * 64
    amd = os.path.join(full_dir, C.ffx_pin.FRAMEGEN)
    esc = re.escape
    cases = [  # (session, the helper's folder, its arguments, what the handshake must say, the helper's line, what it is)
        ("R-missing", full_dir, ["--fglib=" + os.path.join(C.OUT, "own", "no-such-file.dll")], 1, r"frame generation: fglib .*no-such-file\.dll: not read \(2\): off", "a full path that does not exist"),
        ("R-hash", full_dir, ["--fglib=" + own["path"], "--fglib_sha=" + wrong], 1, r"frame generation: fglib .*, %d bytes, sha256 %s: not the expected sha256 %s: off" % (own["size"], own["sha"], wrong), "a hash that is not the expected one"),
        ("R-entry", full_dir, ["--fglib=" + os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", "version.dll")], 1, r"frame generation: fglib .*version\.dll, \d+ bytes, sha256 [0-9a-f]{64}: lacks an ffx-api entry point: off", "a file that lacks an entry point (Windows' own version.dll)"),
        ("R-relative", full_dir, ["--fglib=own\\ffx-own-fg.dll"], 1, r"frame generation: fglib own\\ffx-own-fg\.dll: not a full path: off", "a path with a separator that is not full"),
        ("R-badsha", full_dir, ["--fglib=" + own["path"], "--fglib_sha=abc"], 1, r"frame generation: fglib_sha: not 64 hex digits: off", "an expected hash that is not 64 hex digits"),
        ("R-amd", full_dir, ["--fglib=" + amd], 1, r"frame generation: fglib %s: an AMD name: off" % esc(amd), "AMD's own file by its full path, WITHOUT the control's switch (it lies there, and it would load)"),
        ("R-amd-name", full_dir, ["--fglib=" + C.ffx_pin.FRAMEGEN], 1, r"frame generation: fglib %s: an AMD name: off" % esc(C.ffx_pin.FRAMEGEN), "AMD's own file by its bare name, beside the helper"),
        ("R-name-missing", full_dir, ["--fglib=" + os.path.basename(own["path"])], 1, r"frame generation: fglib %s: not read \(2\): off" % esc(os.path.join(full_dir, os.path.basename(own["path"]))),
         "a bare name with no such file beside the helper (ours lies in another folder: it is not searched for)"),
        ("R-name-up", full_dir, ["--fglib=.."], 1, r"frame generation: fglib \.\.: not a file beside the helper: off", "a bare name that leaves the helper's folder"),
        ("R-control", full_dir, ["--fglib=" + own["path"], "--fglib_sha=" + own["sha"]], 3, r"frame generation: fglib %s, %d bytes, sha256 %s: loaded direct" % (esc(own["path"]), own["size"], own["sha"]), "THE CONTROL, by its full path: the right file with its right hash"),
        ("R-control-name", named["dir"], named["args"], 3, r"frame generation: fglib %s, %d bytes, sha256 %s: loaded direct" % (esc(named["path"]), named["size"], named["sha"]),
         "THE CONTROL, BY ITS BARE NAME (%s): the file beside the helper, loaded by the full path the helper made" % " ".join(named["args"])[:60]),
        ("R-name-hash", named["dir"], [named["args"][0], "--fglib_sha=" + wrong], 1, r"frame generation: fglib %s, %d bytes, sha256 %s: not the expected sha256 %s: off" % (esc(named["path"]), named["size"], named["sha"], wrong),
         "by its bare name, with a hash that is not the expected one"),
    ]
    allok = True
    # BESIDE AMD'S OWN LOADER IN THE PROCESS (the helper told --backend=1: FSR's library is loaded at its start, as in a
    # session of a player who runs FSR): ours is still loaded direct, and what AMD's loader maps by itself is READ, not
    # assumed. Without AMD's frame generation file in the folder nothing of that name may be mapped; with it there, the line
    # says whether AMD's loader mapped it unasked (which decides what the product's folder may hold)
    for sid, st, where in (("R-fsr-bare", stage("own"), "no frame generation file of AMD's in the folder"), ("R-fsr-beside", own, "AMD's frame generation file lying in the folder")):
        if st is None: allok = False; continue
        log, passed, out = hello_session(sid, st["dir"], ["--backend=1"] + st["args"], 3)
        loaded = re.search(r"frame generation: fglib %s, %d bytes, sha256 %s: loaded direct" % (esc(st["path"]), st["size"], st["sha"]), log) is not None
        fsr = "backend FSR" in log or "FSR" in "".join(l for l in log.splitlines() if "backend" in l)
        up = re.search(r"the upscaler's library is (mapped|not mapped) in this process", log)
        amdmap = re.findall(r"frame generation: fgid: exit: \S+ mapped: (no|yes.*)", log)
        print("  [%s] beside AMD's loader (--backend=1), %s: ours loaded direct: %s; AMD's upscaler library %s; at the exit a module of AMD's frame generation name: %s" % (
            sid, where, "yes" if loaded else "NO", up.group(1) if up else "?", amdmap[0] if amdmap else "NO LINE"))
        allok = proof(passed and loaded and fsr and len(amdmap) == 1 and (st["beside"] or amdmap[0] == "no"), sid,
                      "with AMD's own loader in the process (FSR as the upscaler) ours is loaded direct and offered%s" % (
                          "; AMD's loader mapped NO frame generation module" if amdmap and amdmap[0] == "no" else "; AMD'S LOADER MAPPED ITS OWN FRAME GENERATION FILE UNASKED (it lies in the folder): two frame generation modules in the process")) and allok
    # SITTING A'S MESSAGE HOOK (host\fg_lib.h since round 8's deep strip, --fgcheck=1) IS INSTALLED AT EVERY LOAD OF THE LIBRARY: at a direct load of
    # ours as behind AMD's loader, and ours answers it what AMD's answers (the union of the two branches, 2026-10-07)
    hook = {}
    for sid, st in (("R-check-amd", stage("loader")), ("R-check-own", stage("own"))):
        if st is None: allok = False; continue
        log, passed, out = hello_session(sid, st["dir"], st["args"] + ["--fgcheck=1"], 3)
        m = re.search(r"frame generation check: AMD's message hook: (\d+) \(([^)]*)\)(.*)", log)
        hook[sid] = (m.group(1), m.group(2), m.group(3).strip()) if m and passed else None
    allok = proof(hook.get("R-check-own") is not None and hook.get("R-check-own") == hook.get("R-check-amd") and not hook["R-check-own"][2], "R-check-own",
                  "sitting A's message hook is installed at a direct load of ours, and ours answers it what AMD's library answers behind AMD's loader: %s (AMD's: %s)" % (
                      hook.get("R-check-own"), hook.get("R-check-amd"))) and allok
    for sid, helper_dir, args, want, line, what in cases:
        log, passed, out = hello_session(sid, helper_dir, args, want)
        said = re.search(line, log) is not None
        offs = len(re.findall(r"frame generation: fg\w+.*: off\s*$", log, re.M))
        fell = "mapped by AMD's loader" in log or (want == 1 and 'frame generation: available, provider' in log)
        amd = re.findall(r"frame generation: fgid: exit: \S+ mapped: (no|yes.*)", log)
        ok = passed and said and not fell and amd == ["no"] and offs == (0 if want == 3 else 1)
        allok = proof(ok, sid, "%s: %s; the handshake said %s; AMD's loader was not asked; at the exit no module of AMD's frame generation name was mapped%s" % (
            what, "ONE line, and frame generation is off for the process" if want == 1 else "loaded direct, and offered", "\"asked, nothing usable\"" if want == 1 else "\"asked and available\"",
            "" if ok else " [the fake client passed: %s; the line was said: %s; off lines: %d; a fallback: %s; at the exit: %s]" % (passed, said, offs, fell, amd))) and allok
    return allok


def step_refusals_ours(full_dir):
    """1 (--ours). THE OWN ARM'S REFUSALS (host\\fg_own.h, --fgown): each is one line ending ": off", the handshake says "asked,
    nothing usable", AMD's loader is never asked and --fglib's load is never tried, and no module of AMD's name is mapped.
    AMD's own frame generation file lies beside the helper in every case with a full path."""
    print("\n1. REFUSALS: the own arm (--fgown) refuses and never falls back (in every case with a full path, AMD's own frame generation file lies beside the helper)")
    ours = stage(SUBJ, beside=True)
    named = stage(SUBJ, by_name=True)
    first = stage("own", beside=True)
    if ours is None or named is None or first is None: return False
    wrong = "0" * 64
    amd = os.path.join(full_dir, C.ffx_pin.FRAMEGEN)
    esc = re.escape
    base = os.path.basename(ours["path"])
    head = r"frame generation: fgown %s, %d bytes, sha256 %s" % (esc(ours["path"]), ours["size"], ours["sha"])
    cases = [  # (session, the helper's folder, its arguments, what the handshake must say, the helper's line, what it is)
        ("R-missing", full_dir, ["--fgown=" + os.path.join(C.OUT, "own", "no-such-file.dll")], 1, r"frame generation: fgown .*no-such-file\.dll: not read \(2\): off", "a full path that does not exist"),
        ("R-hash", full_dir, ["--fgown=" + ours["path"], "--fgown_sha=" + wrong], 1, head + r": not the expected sha256 %s: off" % wrong, "a hash that is not the expected one"),
        ("R-entry", full_dir, ["--fgown=" + os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", "version.dll")], 1,
         r"frame generation: fgown .*version\.dll, \d+ bytes, sha256 [0-9a-f]{64}: lacks a function of the mod's interface: off", "a file that lacks the interface (Windows' own version.dll)"),
        ("R-stage1", full_dir, ["--fgown=" + first["path"]], 1, r"frame generation: fgown %s, %d bytes, sha256 %s: lacks a function of the mod's interface: off" % (esc(first["path"]), first["size"], first["sha"]),
         "STAGE 1'S LIBRARY handed to --fgown (it has AMD's five entry points and none of the mod's seven)"),
        ("R-relative", full_dir, ["--fgown=own\\" + base], 1, r"frame generation: fgown own\\%s: not a full path: off" % esc(base), "a path with a separator that is not full"),
        ("R-badsha", full_dir, ["--fgown=" + ours["path"], "--fgown_sha=abc"], 1, r"frame generation: fgown_sha: not 64 hex digits: off", "an expected hash that is not 64 hex digits"),
        ("R-amd", full_dir, ["--fgown=" + amd], 1, r"frame generation: fgown %s: an AMD name: off" % esc(amd), "AMD's own file by its full path (it lies there; this arm has no switch that lets it in)"),
        ("R-amd-name", full_dir, ["--fgown=" + C.ffx_pin.FRAMEGEN], 1, r"frame generation: fgown %s: an AMD name: off" % esc(C.ffx_pin.FRAMEGEN), "AMD's own file by its bare name, beside the helper"),
        ("R-name-missing", full_dir, ["--fgown=" + base], 1, r"frame generation: fgown %s: not read \(2\): off" % esc(os.path.join(full_dir, base)), "a bare name with no such file beside the helper (ours lies in another folder: it is not searched for)"),
        ("R-name-up", full_dir, ["--fgown=.."], 1, r"frame generation: fgown \.\.: not a file beside the helper: off", "a bare name that leaves the helper's folder"),
        ("R-both", full_dir, ["--fgown=" + ours["path"], "--fglib=" + first["path"]], 1, r"frame generation: fgown: --fglib is given beside it: off", "--fglib given beside --fgown: neither is loaded"),
        ("R-lone", full_dir, ["--fgown_stamps=1"], 1, r"frame generation: fgown: --fgown_sha, --fgown_stamps or --fgown_csv without --fgown: off", "the stamps asked for without a library"),
        ("R-control", full_dir, ["--fgown=" + ours["path"], "--fgown_sha=" + ours["sha"]], 3, head + r": loaded direct", "THE CONTROL, by its full path: the right file with its right hash"),
        ("R-control-name", named["dir"], named["args"], 3, r"frame generation: fgown %s, %d bytes, sha256 %s: loaded direct" % (esc(named["path"]), named["size"], named["sha"]),
         "THE CONTROL, BY ITS BARE NAME (%s): the file beside the helper, loaded by the full path the helper made" % " ".join(named["args"])[:60]),
        ("R-name-hash", named["dir"], [named["args"][0], "--fgown_sha=" + wrong], 1, r"frame generation: fgown %s, %d bytes, sha256 %s: not the expected sha256 %s: off" % (esc(named["path"]), named["size"], named["sha"], wrong),
         "by its bare name, with a hash that is not the expected one"),
    ]
    allok = True
    # BESIDE AMD'S OWN LOADER IN THE PROCESS (--backend=1: FSR's library is loaded at the helper's start): ours is still loaded
    # direct, and what AMD's loader maps by itself is READ
    for sid, st, where in (("R-fsr-bare", stage(SUBJ), "no frame generation file of AMD's in the folder"), ("R-fsr-beside", ours, "AMD's frame generation file lying in the folder")):
        if st is None: allok = False; continue
        log, passed, out = hello_session(sid, st["dir"], ["--backend=1"] + st["args"], 3)
        loaded = re.search(r"frame generation: fgown %s, %d bytes, sha256 %s: loaded direct" % (esc(st["path"]), st["size"], st["sha"]), log) is not None
        fsr = "backend FSR" in log or "FSR" in "".join(l for l in log.splitlines() if "backend" in l)
        up = re.search(r"the upscaler's library is (mapped|not mapped) in this process", log)
        amdmap = re.findall(r"frame generation: fgid: exit: \S+ mapped: (no|yes.*)", log)
        print("  [%s] beside AMD's loader (--backend=1), %s: ours loaded direct: %s; AMD's upscaler library %s; at the exit a module of AMD's frame generation name: %s" % (
            sid, where, "yes" if loaded else "NO", up.group(1) if up else "?", amdmap[0] if amdmap else "NO LINE"))
        allok = proof(passed and loaded and fsr and len(amdmap) == 1 and (st["beside"] or amdmap[0] == "no"), sid,
                      "with AMD's own loader in the process (FSR as the upscaler) ours is loaded direct and offered%s" % (
                          "; AMD's loader mapped NO frame generation module" if amdmap and amdmap[0] == "no" else "; AMD'S LOADER MAPPED ITS OWN FRAME GENERATION FILE UNASKED (it lies in the folder): two frame generation modules in the process")) and allok
    for sid, helper_dir, args, want, line, what in cases:
        log, passed, out = hello_session(sid, helper_dir, args, want)
        said = re.search(line, log) is not None
        offs = len(re.findall(r"frame generation: fg\w+.*: off\s*$", log, re.M))
        fell = "mapped by AMD's loader" in log or "frame generation: fglib " in log or (want == 1 and 'frame generation: available, provider' in log)
        amdm = re.findall(r"frame generation: fgid: exit: \S+ mapped: (no|yes.*)", log)
        ok = passed and said and not fell and amdm == ["no"] and offs == (0 if want == 3 else 1)
        allok = proof(ok, sid, "%s: %s; the handshake said %s; neither AMD's loader nor --fglib's load was tried; at the exit no module of AMD's frame generation name was mapped%s" % (
            what, "ONE line, and frame generation is off for the process" if want == 1 else "loaded direct, and offered", "\"asked, nothing usable\"" if want == 1 else "\"asked and available\"",
            "" if ok else " [the fake client passed: %s; the line was said: %s; off lines: %d; a fallback: %s; at the exit: %s]" % (passed, said, offs, fell, amdm))) and allok
    return allok


def debug_counts(log):
    """{where: [messages, corruption, error, warning, info, message, not read, dropped]} and the messages printed in full."""
    out = {}
    for m in re.finditer(r"d3d12 debug: (.+?): (\d+) messages \((\d+) corruption, (\d+) error, (\d+) warning, (\d+) info, (\d+) message\)(?:; (\d+) not read, (\d+) dropped)?", log):
        k = m.group(1); v = [int(x or 0) for x in m.groups()[1:]]
        out[k] = [a + b for a, b in zip(out[k], v)] if k in out else v
    full = re.findall(r"d3d12 debug: #\d+ (\w+) id (\d+): (.*)", log)
    return out, full


def step_debug():
    print("\n2. THE DEBUG LAYER: its own control, then the create and a few pairs on AMD's library, then on ours (DLAA guides, FP16)")
    few = dict(pairs=16, arms="F,FR")
    ctl = hash_session("DF", "loader", "-dbg2", helper_extra=["--d3d12_debug=2"], **few)
    a = hash_session("DF", "loader", "-dbg", helper_extra=["--d3d12_debug=1"], **few)
    b = hash_session("DF", SUBJ, "-dbg", helper_extra=["--d3d12_debug=1"], **few)
    if not (ctl and a and b): return False
    ok = True
    for s in (ctl, a, b):
        ok = proof("d3d12 debug: on, asked before the first device" in s["log"] and "has no message queue" not in s["log"] and "not available here" not in s["log"], s["sid"],
                   "the debug layer is on the helper's device (asked before the first device; the device has its message queue)") and ok
    cc, cfull = debug_counts(ctl["log"])
    probe = cc.get("the probe")
    planted = "d3d12 debug: control: a compute pipeline of 16 junk bytes answered 0x" in ctl["log"]
    ok = proof(planted and probe is not None and probe[0] >= 1 and probe[1] + probe[2] >= 1, ctl["sid"],
               "THE COUNT'S OWN CONTROL: a compute pipeline of junk bytes was refused, and the count behind it holds %s message(s), %s of them an error or a corruption: a refused pipeline state IS a message here" % (
                   probe[0] if probe else "no", (probe[1] + probe[2]) if probe else "none")) and ok
    for m in cfull[:3]: print("      the control's message: %s id %s: %s" % (m[0], m[1], m[2][:200]))
    ca, fa = debug_counts(a["log"]); cb, fb = debug_counts(b["log"])
    for s, c, f in ((a, ca, fa), (b, cb, fb)):
        print("  %s (%s):" % (s["sid"], s["st"]["what"]))
        for k, v in c.items(): print("      %-22s %d messages (corruption %d, error %d, warning %d, info %d, message %d; not read %d, dropped %d)" % (k, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]))
        for m in f[:20]: print("      in full: %s id %s: %s" % (m[0], m[1], m[2][:220]))
    need = ("the probe", "create and warm-up", "set end", "exit")
    ok = proof(all(k in ca for k in need) and all(k in cb for k in need), "debug", "both arms counted at the probe, behind the create and the warm-up, at the set's end and at the exit") and ok
    tot = lambda c, i: sum(v[i] for v in c.values())
    ok = proof(tot(ca, 1) + tot(ca, 2) == 0, a["sid"], "AMD'S LIBRARY (the control of the instrument on real work): no error and no corruption in the whole session (%d messages in all: %d warning, %d info, %d message)" % (
        tot(ca, 0), tot(ca, 3), tot(ca, 4), tot(ca, 5))) and ok
    ok = proof(tot(cb, 1) + tot(cb, 2) == 0, b["sid"], "OURS: no error and no corruption in the whole session (%d messages in all: %d warning, %d info, %d message): no pipeline state was refused or null" % (
        tot(cb, 0), tot(cb, 3), tot(cb, 4), tot(cb, 5))) and ok
    for k in need:
        ok = proof(cb.get(k, [9] * 8)[:6] == ca.get(k, [0] * 8)[:6], "debug", "%s: ours counts what AMD's counts (ours %s, AMD's %s: messages, corruption, error, warning, info, message)" % (k, cb.get(k, [])[:6], ca.get(k, [])[:6])) and ok
    ids = lambda f: sorted({(m[0], m[1]) for m in f})
    ok = proof(set(ids(fb)) <= set(ids(fa)), "debug", "ours prints no kind of message AMD's does not (ours %s, AMD's %s)" % (ids(fb) or "none", ids(fa) or "none")) and ok
    cmpd, _, same = compare(a, b)
    print("  and with the layer on, ours against AMD's by the whole image: %s" % words(cmpd))
    return ok


def by_arm(a, b, cls):
    """{arm: [pairs of that class (None: every class), of them the same in all four channels]} of two sessions."""
    out = {}
    for key, p in a["pair"].items():
        if cls is not None and p["cls"] != cls: continue
        q = b["pair"].get(key)
        v = out.setdefault(key[0], [0, 0]); v[0] += 1; v[1] += 1 if q and q["all"] == p["all"] else 0
    return out


def step_config(cfg, controls, first):
    what, guides, fmt, hud = CFG[cfg]
    print("\n" + "=" * 118)
    print("CONFIGURATION %s: 3840x2160, %s. %d pairs of each of the arms %s." % (cfg, what, PAIRS, ARMS))
    ok = True
    ref = hash_session(cfg, "loader", "-1")
    if ref is None: return False
    libs = {"loader-1": ref}; sees = {}; seen = {}
    # ---- 5. AMD's against AMD's
    if controls:
        print("\n5. [%s] AMD'S LIBRARY AGAINST ITSELF: through AMD's loader twice, and loaded direct by the helper" % cfg)
        # (--ours: AMD's file loaded direct is --fglib's own control and says nothing of --fgown: AMD's library twice is the control)
        again = hash_session(cfg, "loader", "-2"); direct = again if OURS else hash_session(cfg, "stock")
        if again is None or direct is None: return False
        libs["loader-2"] = again
        if not OURS: libs["stock"] = direct
        c1, d1, s1 = compare(ref, again); c2, d2, s2 = compare(ref, direct)
        print("  through the loader, twice:           %s" % words(c1))
        print("  through the loader against direct:   %s" % words(c2))
        ok = proof(s1 and s2, cfg, "the three sessions sent the same pairs: each pair the same move and the same count of dispatches since a reset") and ok
        rep = equal(c1)
        ok = proof(rep, cfg, "AMD'S LIBRARY GIVES THE SAME IMAGE TWICE on these inputs: every pair of every class, all four channels") and ok
        if not OURS: ok = proof(equal(c2), cfg, "LOADED DIRECT IT GIVES WHAT IT GIVES THROUGH AMD'S LOADER: every pair of every class, all four channels (the direct load changes nothing)") and ok
        if not rep:
            keys = [d1[c][0] for c in CLASSES if d1[c]]
            print("  THE SPREAD of AMD's own library, which is all ours can be held to where it shows:")
            raw_compare(cfg, "loader", "loader", keys, "AMD's against AMD's")
            return False                                   # the order: the spread is fixed before ours is looked at
        # ---- 6. the altered builds must fail
        print("\n6. [%s] THE TWO ALTERED BUILDS MUST FAIL the compare on steady pairs" % cfg)
        for lib, where in ALTS:
            if NO_ALT: continue
            alt = hash_session(cfg, lib)
            if alt is None: ok = False; continue
            libs[lib] = alt
            ca, da, sa = compare(ref, alt)
            print("  AMD's against the build altered in %s: %s" % (where, words(ca)))
            pa = by_arm(ref, alt, "steady")
            sees[lib] = sorted(a for a, v in pa.items() if v[1] < v[0])
            seen[lib] = sorted(k for k, p in ref["pair"].items() if p["cls"] == "steady" and k in alt["pair"] and alt["pair"][k]["all"] != p["all"])
            blind = sorted(a for a, v in pa.items() if v[0] and v[1] == v[0])
            print("    its steady pairs that DIFFER from AMD's, by arm: %s" % ", ".join("%s %d of %d" % (a, v[0] - v[1], v[0]) for a, v in sorted(pa.items())))
            # how soon after a reset the change reaches the picture: by the helper's count, over every pair that differs
            early = {}
            for key, p in ref["pair"].items():
                q = alt["pair"].get(key)
                if p["since"] is None or p["since"] > 16 or q is None: continue
                e = early.setdefault(p["since"], [0, 0]); e[0] += 1; e[1] += 0 if q["all"] == p["all"] else 1
            print("    pairs that differ, by the dispatch's count since the last reset (0 is the reset itself): %s" % ", ".join("%d: %d of %d" % (k, v[1], v[0]) for k, v in sorted(early.items())))
            st = ca["steady"]
            ok = proof(sa and st[0] >= 12 and st[1] < st[0] and sees[lib], cfg, "THE BUILD ALTERED IN %s FAILS THE COMPARE: %d of its %d steady pairs differ from AMD's, on the arms %s: there the compare sees that half of the passes" % (
                where.upper(), st[0] - st[1], st[0], ", ".join(sees[lib]) or "NONE")) and ok
            if blind: print("    ON THE ARMS %s THIS ALTERED BUILD GIVES AMD'S PICTURE: on those arms an equal picture says nothing about %s (it does not reach the picture there)" % (", ".join(blind), where))
            if da["steady"] and (first or RAW_ALL):
                keys = [next(k for k in da["steady"] if k[0] == a) for a in sees[lib][:2]]
                raw_compare(cfg, "loader", lib, keys, "altered in %s" % where)
        # ---- 6b. THE ONE-PIXEL CONTROL (stage 3): the product's cuts and one pixel of every generated image changed
        if OURS and not NO_ALT:
            print("\n6b. [%s] THE ONE-PIXEL CONTROL MUST FAIL the compare on EVERY pair: the exact library's cuts and the red of one pixel changed in the last pass" % cfg)
            px = hash_session(cfg, "ours-px")
            if px is None: ok = False
            else:
                libs["ours-px"] = px
                cp, dp, sp = compare(ref, px)
                print("  AMD's against the one-pixel control: %s" % words(cp))
                ok = proof(sp and all(v[0] > 0 and v[1] == 0 and v[2] == 0 for v in cp.values()), cfg,
                           "THE ONE-PIXEL CONTROL FAILS THE COMPARE ON EVERY PAIR: %s differ from AMD's: the judge sees one changed pixel in a whole image, on a steady pair, on a reset pair and on the way back" % (
                               ", ".join("%s %d of %d" % (c, v[0] - v[1], v[0]) for c, v in cp.items()))) and ok
                if first or RAW_ALL:
                    keys = [dp[c][0] for c in ("steady", "reset") if dp[c]]
                    got = raw_compare(cfg, "loader", "ours-px", keys, "one pixel changed") if keys else []
                    ok = proof(len(got) == len(keys) == 2 and all(r[0] == 1 and r[2] == 0 for a, i, r in got), cfg,
                               "BY THE WHOLE IMAGES THE CONTROL DIFFERS FROM AMD'S IN EXACTLY ONE PIXEL, on a steady pair and on a reset pair (%s): that is the size of change the hash compare is shown to catch" % (
                                   "; ".join("arm %s pair %d: %d pixel(s), %d step(s)" % (a, i, r[0], r[1]) for a, i, r in got) or "THE IMAGES WERE NOT COMPARED: no numpy, or a file is missing")) and ok
        # ---- 6c. THE POSITION'S CONTROL (2026-10-17, the cut fi-position): the exact library's cuts, and the glue tells AMD's dispatch the
        # position 0.4 where the caller (the helper) says the middle. At the middle the exact library must equal AMD's; that proves the cut
        # exact only if the constant REACHES the shaders, so this library must differ from AMD's on every arm whose picture moves, and
        # may equal it only where a position cannot show: a reset pair (the current picture is copied)
        if OURS and not NO_ALT:
            print("\n6c. [%s] THE POSITION'S CONTROL MUST FAIL the compare on EVERY ARM THAT MOVES (%s): the exact library's cuts, and the glue tells AMD's dispatch the position 0.4 for the middle" % (cfg, ", ".join(MOVING)))
            pc = hash_session(cfg, "ours-pos")
            if pc is None: ok = False
            else:
                libs["ours-pos"] = pc
                cq, dq, sq = compare(ref, pc)
                print("  AMD's against the position's control: %s" % words(cq))
                arms = {}
                for key, p in ref["pair"].items():
                    q = pc["pair"].get(key)
                    if q is None or p["cls"] is None: continue
                    v = arms.setdefault(key[0], {"reset": [0, 0], "moved": [0, 0]})[("reset" if p["cls"] == "reset" else "moved")]
                    v[0] += 1; v[1] += 0 if q["all"] == p["all"] else 1
                print("    by arm, pairs that DIFFER from AMD's (after a reset / the reset pairs themselves): %s" % ", ".join("%s %d of %d / %d of %d" % (a, v["moved"][1], v["moved"][0], v["reset"][1], v["reset"][0]) for a, v in sorted(arms.items())))
                fails_on = [a for a in MOVING if a in arms and arms[a]["moved"][0] and arms[a]["moved"][1] > 0]
                ok = proof(sq and fails_on == MOVING, cfg, "THE POSITION'S CONTROL FAILS THE COMPARE ON EVERY ARM THAT MOVES: %s: the position the glue is told reaches the shaders, so the exact library's equal pictures at the middle are the cut's" % (
                    ", ".join("%s %d of %d pairs after a reset differ" % (a, arms[a]["moved"][1], arms[a]["moved"][0]) if a in arms else "%s NOT RUN" % a for a in MOVING))) and ok
                rs = [arms[a]["reset"] for a in arms]
                ok = proof(sum(v[0] for v in rs) > 0 and sum(v[1] for v in rs) == 0, cfg, "and on the reset pairs it EQUALS AMD's (%d of %d): it changes only what a position reaches, so its difference is the position's and nothing else's" % (
                    sum(v[0] - v[1] for v in rs), sum(v[0] for v in rs))) and ok
    # ---- 7. ours against AMD's
    print("\n7. [%s] OURS AGAINST AMD'S" % cfg)
    own = hash_session(cfg, SUBJ)
    if own is None: return False
    libs["own"] = own
    co, do, so = compare(ref, own)
    if OURS:
        # AND AGAINST STAGE 1'S LIBRARY: the same objects of AMD's two effects and backend behind AMD's own provider layer
        # and its five entry points (ffx-own-fg.dll, --fglib), which stage 1 proved equal to AMD's signed library
        one = hash_session(cfg, "own")
        if one is None: return False
        libs["stage 1"] = one
        c1o, d1o, s1o = compare(one, own)
        print("  THE SECOND LIBRARY AGAINST STAGE 1'S (the same objects behind AMD's provider layer): %s" % words(c1o))
        ok = proof(s1o and equal(c1o), cfg, "THE SECOND LIBRARY EQUALS STAGE 1'S: every pair of every class, all four channels (%d pairs): the mod's own interface and its three calls change nothing of the picture" % sum(v[0] for v in c1o.values())) and ok
        # AND AGAINST THE REFERENCE (stage 3): stage 2's library with no cut, built by the same run as the product
        refl = hash_session(cfg, "ref")
        if refl is None: return False
        libs["ref"] = refl
        cro, dro, sro = compare(refl, own); cra, dra, sra = compare(ref, refl)
        print("  THE EXACT LIBRARY (%s) AGAINST THE REFERENCE (no cut): %s" % (CUTS_WORDS, words(cro)))
        print("  the reference against AMD's:                      %s" % words(cra))
        ok = proof(sro and equal(cro), cfg, "THE EXACT LIBRARY, WITH ITS CUTS, EQUALS THE REFERENCE WITH NONE: every pair of every class, all four channels (%d pairs): %s" % (sum(v[0] for v in cro.values()), CUTS_WORDS)) and ok
        ok = proof(sra and equal(cra), cfg, "and the reference equals AMD's signed library: every pair of every class, all four channels (%d pairs)" % sum(v[0] for v in cra.values())) and ok
    print("  %s" % words(co))
    ok = proof(so, cfg, "both sessions sent the same pairs: each pair the same move and the same count of dispatches since a reset") and ok
    for c in CLASSES:
        v = co[c]
        ok = proof(v[0] > 0 and v[1] == v[0] and v[2] == v[0], cfg, "OURS EQUALS AMD'S, %s pairs: %d of %d the same in the three colours, %d in all four channels (moves %s)" % (
            c.upper() if c != "1 to 11" else "PAIRS 1 TO 11 AFTER A RESET,", v[1], v[0], v[2], moves_of(ref, c))) and ok
    print("  by arm, pairs that are the same in all four channels: %s" % ", ".join("%s %d of %d" % (a, v[1], v[0]) for a, v in sorted(by_arm(ref, own, None).items())))
    if controls and not NO_ALT:
        # ON THE VERY PAIRS WHERE A CHANGED SHADER SHOWS: a steady pair on which the altered build's image differs from AMD's
        # is a pair whose inputs carry that half of the passes into the picture; ours must equal AMD's on each of those
        for lib, half in ((ALTS[0][0], "THE INTERPOLATION HALF"), (ALTS[1][0], "THE OPTICAL FLOW HALF")):
            keys = seen.get(lib, [])
            same = sum(1 for k in keys if k in own["pair"] and own["pair"][k]["all"] == ref["pair"][k]["all"])
            ok = proof(len(keys) >= 12 and same == len(keys), cfg, "%s: the build altered there changes the picture of %d steady pairs (arms %s), and ON EVERY ONE OF THOSE PAIRS ours equals AMD's (%d of %d)" % (
                half, len(keys), ", ".join("%s %d" % (a, sum(1 for k in keys if k[0] == a)) for a in sorted({k[0] for k in keys})) or "none", same, len(keys))) and ok
    if not equal(co):
        keys = [do[c][0] for c in CLASSES if do[c]][:3]
        print("  WHERE OURS DIFFERS, by the whole images:")
        raw_compare(cfg, "loader", SUBJ, keys, "ours against AMD's")
        if not OURS and os.path.exists(C.LIBS["own-md"][1]):
            md = hash_session(cfg, "own-md")
            if md:
                cm, dm, sm = compare(ref, md)
                print("  THE FIRST SUSPECT, the twin on the dynamic C runtime (ffx-own-fg-md.dll) against AMD's: %s" % words(cm))
        elif not OURS: print("  the twin on the dynamic C runtime is not built (build-ffx-own.ps1 -DynamicCrt): the first suspect was not tried")
    if first:
        # ONCE A RUN: with AMD's file planted beside the helper, and by its bare name beside the helper (the form a leg uses)
        beside = hash_session(cfg, SUBJ, "-beside", beside=True)
        if beside:
            cb, db, sb = compare(own, beside)
            ok = proof(sb and equal(cb), cfg, "WITH AMD'S FILE LYING IN THE HELPER'S FOLDER ours gives the same images as without it (%s), and the identity gate above says AMD's file stayed unmapped" % words(cb)) and ok
        else: ok = False
        named = hash_session(cfg, SUBJ, "-byname", by_name=True)
        if named:
            cn, dn, sn = compare(own, named)
            ok = proof(sn and equal(cn), cfg, "BY ITS BARE NAME BESIDE THE HELPER (%s) ours gives the same images as by its full path (%s)" % (named["st"]["args"][0], words(cn))) and ok
        else: ok = False
        ok = step_upscalers(cfg, own) and ok
        # THE TWIN ON THE DYNAMIC C RUNTIME, where it is built (build-ffx-own.ps1 -DynamicCrt): AMD's own library takes its
        # float math from the system's C runtime and ours from the static one; the twin says whether that reaches the picture
        if OURS: pass
        elif os.path.exists(C.LIBS["own-md"][1]):
            md = hash_session(cfg, "own-md")
            if md:
                cm, dm, sm = compare(ref, md)
                ok = proof(sm and equal(cm), cfg, "THE TWIN ON THE DYNAMIC C RUNTIME (ffx-own-fg-md.dll) gives AMD's images too (%s): which C runtime the library is built on does not reach the picture" % words(cm)) and ok
            else: ok = False
        else: print("  (the twin on the dynamic C runtime is not built in this tree: build-ffx-own.ps1 -DynamicCrt. It is not needed while ours equals AMD's)")
    # ---- 4. the context's memory figure
    ctx = {k: s["ctx"] for k, s in libs.items()}
    ok = proof(len(set(ctx.values())) == 1 and None not in ctx.values() and ctx["own"] > 0, cfg, "THE CONTEXT'S MEMORY FIGURE is the same on every library: %s" % ", ".join("%s %s bytes" % kv for kv in sorted(ctx.items()))) and ok
    return ok


def step_upscalers(cfg, own, lib=None, pre=""):
    """8. OURS BESIDE AN UPSCALER THAT IS ALIVE AND WORKING on the same device (DECISIONS, the own build's rule 8): the fake
    client's set is a REAL build there (--fake-cost-upscale=1), so the helper makes its upscaler and every frame is an
    evaluate: NVIDIA's DLSS, and AMD's FSR through AMD's own loader (two of AMD's backends in one process: the upscaler's,
    inside AMD's signed file, and ours). Frame generation reads the guides and never the upscaler's output, so the generated
    images must be the ones of the session without an upscaler; and no module of AMD's frame generation name may be mapped."""
    print("\n8. [%s] %s BESIDE AN UPSCALER THAT IS ALIVE AND WORKING on the helper's device" % (cfg, "OURS" if lib is None else "THE PRODUCT"))
    ok = True
    lib = lib or SUBJ
    # (which upscaler it is: the helper's own line for FSR; for DLSS the helper says "backend AUTO -> DLSS" and the build's
    # ack carries NGX's result 0x1, which the fake client prints: the helper's own "feature ready" line is a log=2 line)
    for tag, extra, name, mark, built in (("-dlss", [], "NVIDIA's DLSS", r"backend AUTO -> DLSS|serve timing", r"REAL build .*\(ok 1, result 0x1,"),
                                          ("-fsr", ["--backend=1"], "AMD's FSR, through AMD's loader", r"FSR context ready|serve timing", r"REAL build .*\(ok 1,")):
        s = hash_session(cfg, lib, pre + tag, helper_extra=extra, fake_extra=["--fake-cost-upscale=1"])
        if s is None: ok = False; continue
        lines = [l.strip() for l in s["log"].splitlines() if re.search(mark, l)]
        closed = re.search(r"pipe closed by \S+ \S+ after (\d+) frames \((\d+) failed\)", s["log"])
        real = (re.search(built, s["out"]) is not None and any(re.search(mark.split("|")[0], l) for l in lines)
                and closed is not None and int(closed.group(1)) > len(s["pair"]) and int(closed.group(2)) == 0)
        for l in [l for l in s["out"].splitlines() if "REAL build" in l][:1] + lines[:1] + lines[-1:]: print("      %s" % l.strip()[:260])
        ok = proof(real, s["sid"], "%s IS ALIVE AND WORKING beside frame generation: the helper made it for a real build and evaluated %s frames, %s failed" % (
            name, closed.group(1) if closed else "?", closed.group(2) if closed else "?")) and ok
        c, d, same = compare(own, s)
        ok = proof(same and equal(c), s["sid"], "beside %s ours gives the images it gives alone (%s)" % (name, words(c))) and ok
    return ok


# ---------------------------------------------------------------------------------------------------------------- the product's pictures
PRODUCT_WORDS = "?"                   # --ours: the cuts in the product, by the build's own list beside it
PIN_HEAD = r"""# THE PIN OF THE PRODUCT'S PICTURES (2026-10-12): what dlssiv-fg.dll, THE PRODUCT, generates on the judge's own pictures, by
# one SHA-256 a configuration. The product's picture is its own since the owner accepted, in the game, cuts that change
# it (ffx-own\cuts\CUTS.txt: "accepted"), so it is not held to AMD's any more: it is held to THIS. A change of its picture,
# wanted or not, fails the judge (python tools\test-fg-own\run.py --ours, and its quick form --ours --pictures) until
# somebody says it is wanted by making this file again.
#     picture-cuts, picture-cuts-sha256   the accepted picture cuts this was made for (python tools\ffx-own\cuts.py; the
#                                         proof with no device, tools\prove-ffx-own.py, holds these two)
#     arms, pairs-an-arm                  the judge's arms and how many pairs of each were hashed
#     <configuration> <pairs> <SHA-256>   of the lines "<arm> <index> <dispatches since the last reset> <the hash of the
#                                         whole generated image, all four channels>", by arm and index, each ended by LF
# AN EXACT CUT DOES NOT CHANGE THIS FILE: the product's pictures must still be these. NEVER EDITED BY HAND: python
# tools\test-fg-own\run.py --ours --new-pictures writes it (the product's sessions twice, which must agree) and prints
# what it replaced.
"""


def digest_of(s):
    """(pairs, one SHA-256) of a session's generated pictures: the form the pin's head says"""
    import hashlib
    text = "".join("%s %d %d %s\n" % (k[0], k[1], s["pair"][k]["since"], s["pair"][k]["all"]) for k in sorted(s["pair"]))
    return len(s["pair"]), hashlib.sha256(text.encode("ascii")).hexdigest()


def accepted_cuts():
    """(the product's accepted picture cuts, their digest), by tools\\ffx-own\\cuts.py"""
    sys.path.insert(0, os.path.join(ROOT, "tools", "ffx-own"))
    import cuts as K
    names = K.accepted_names()
    return names, K.digest(names)


def read_pin():
    """{key: [values]} of the pin, or None when it is not there"""
    if not os.path.exists(PIN): return None
    out = {}
    for l in open(PIN, encoding="ascii").read().splitlines():
        f = l.split()
        if f and not l.startswith("#"): out[f[0]] = f[1:]
    return out


def write_pin(values):
    """writes the pin; -> [(key, old, new)] for every value that changed (old is None where there was none, new where it went)"""
    old = read_pin() or {}
    lines = PIN_HEAD.splitlines() + ["%s %s" % (k, " ".join(v)) for k, v in values]
    open(PIN, "wb").write(("\r\n".join(lines) + "\r\n").encode("ascii"))
    return [(k, old.get(k), v) for k, v in values if old.get(k) != v] + [(k, old[k], None) for k in old if k not in dict(values)]


def step_product(cfgs, twice, life):
    """9 (--ours). THE PRODUCT'S OWN PICTURES AGAINST THEIR PIN. -> (every proof held, [(configuration, [pairs, digest])] of
    the configurations whose pictures were read and, where they were made twice, were the same twice)."""
    print("\n" + "=" * 118)
    print("9. THE PRODUCT, WHOSE PICTURE IS ITS OWN: dlssiv-fg.dll (%s). Its pictures on the judge's own set, %s, %s" % (
        PRODUCT_WORDS, "each configuration twice" if twice else "each configuration once", "FOR A NEW PIN" if NEW_PICTURES else "against the pin (tools\\ffx-own\\PICTURES-dlssiv-fg.txt)"))
    acc, adigest = accepted_cuts()
    pin = None if NEW_PICTURES else read_pin()
    ok = True; got = []
    if not NEW_PICTURES:
        if not proof(pin is not None, "pin", "the pin of the product's pictures is there (tools\\ffx-own\\PICTURES-dlssiv-fg.txt)%s" % ("" if pin is not None else ": IT IS NOT. It is made on purpose: python tools\\test-fg-own\\run.py --ours --new-pictures")): return False, got
        ok = proof(pin.get("picture-cuts") == [str(len(acc))] and pin.get("picture-cuts-sha256") == [adigest], "pin",
                   "the pin was made for the product's picture cuts: %d (%s), digest %s...%s" % (len(acc), ", ".join(acc) or "none", adigest[:16], "" if pin.get("picture-cuts-sha256") == [adigest] else
                    "; THE PIN SAYS %s WITH THE DIGEST %s" % (" ".join(pin.get("picture-cuts", ["?"])), " ".join(pin.get("picture-cuts-sha256", ["?"]))))) and ok
        ok = proof(pin.get("arms") == [ARMS] and pin.get("pairs-an-arm") == [str(PAIRS)], "pin", "and for these pairs: the arms %s, %d pairs of each (the pin: %s, %s)" % (ARMS, PAIRS, " ".join(pin.get("arms", ["?"])), " ".join(pin.get("pairs-an-arm", ["?"])))) and ok
    for cfg in cfgs:
        print("\n9. [%s] THE PRODUCT'S PICTURES: %s" % (cfg, CFG[cfg][0]))
        a = hash_session(cfg, "ours", "-p1")
        if a is None: ok = False; continue
        ok = a["ok"] and ok
        n, d = digest_of(a)
        if twice:
            b = hash_session(cfg, "ours", "-p2")
            if b is None: ok = False; continue
            c, diff, same = compare(a, b)
            rep = proof(same and equal(c), cfg, "THE PRODUCT GIVES THE SAME IMAGE TWICE on these inputs: every pair of every class, all four channels (%d pairs): a pin of its pictures can hold (%s)" % (sum(v[0] for v in c.values()), words(c)))
            ok = rep and ok
            if not rep: continue                           # pictures that are not the same twice are neither pinned nor held to a pin
        got.append((cfg, [str(n), d]))
        if not NEW_PICTURES:
            want = pin.get(cfg)
            ok = proof(want == [str(n), d], cfg, "THE PRODUCT'S PICTURES ARE THE PINNED ONES: %d pairs of every class by one digest, %s...%s" % (
                n, d[:16], "" if want == [str(n), d] else "; THE PIN SAYS %s. THE PRODUCT'S PICTURE HAS CHANGED. If that is wanted, say so by making the pin again: python tools\\test-fg-own\\run.py --ours --new-pictures" % (" ".join(want) if want else "nothing of this configuration"))) and ok
        else: print("  [%s] the product's pictures: %d pairs, digest %s" % (cfg, n, d))
        # THE PIN'S CONTROLS, where this run has the exact library's session of the same configuration (the whole proof): the pin is
        # not AMD's picture, and one pixel in one image moves a digest
        x = SESSIONS.get("%s-%s" % (cfg, SUBJ)); px = SESSIONS.get("%s-ours-px" % cfg)
        if x is not None:
            nx, dx = digest_of(x)
            cx, diffx, samex = compare(x, a)
            print("  the product against the exact library: %s" % words(cx))
            if acc:
                ok = proof(samex and nx == n and dx != d, cfg, "THE PIN'S CONTROL: the exact library's digest of the same %d pairs (%s...) is NOT the product's: the pin holds the product's own picture and not AMD's; %d of the pairs differ (%s)" % (
                    nx, dx[:16], sum(v[0] - v[2] for v in cx.values()), ", ".join("%s %d of %d" % (k, v[0] - v[2], v[0]) for k, v in cx.items()))) and ok
            else:
                ok = proof(samex and nx == n and dx == d, cfg, "no picture cut is accepted: the product's digest IS the exact library's (%s...)" % dx[:16]) and ok
            if px is not None:
                ok = proof(digest_of(px)[1] != dx, cfg, "and the one-pixel control's digest (%s...) is not the exact library's: one pixel of one image moves a digest" % digest_of(px)[1][:16]) and ok
            print("  THE CONTEXT'S MEMORY FIGURE (said, not judged): the product %s bytes, the exact library %s bytes" % (a["ctx"], x["ctx"]))
        if PICTURES_OF:
            o = hash_session(cfg, "other")
            if o is None: ok = False; continue
            co, do, so = compare(a, o)
            no, dd = digest_of(o)
            ok = proof(so and equal(co) and (NEW_PICTURES or pin.get(cfg) == [str(no), dd]), cfg, "ANOTHER LIBRARY BY ITS PATH (%s, SHA-256 %s...) MAKES THE PRODUCT'S PICTURES: every pair of every class equals the product's, all four channels (%s), and its digest %s... is the pin's" % (
                os.path.basename(PICTURES_OF), o["st"]["sha"][:16], words(co), dd[:16])) and ok
    if life and cfgs:
        cfg = cfgs[0]; a = SESSIONS.get("%s-ours-p1" % cfg)
        print("\n9. [%s] THE PRODUCT'S LIFE: the debug layer on it, AMD's file beside it, by its bare name, an upscaler alive beside it" % cfg)
        few = dict(pairs=16, arms="F,FR")
        b = hash_session(cfg, "ours", "-pdbg", helper_extra=["--d3d12_debug=1"], **few)
        if b is None: ok = False
        else:
            cb, fb = debug_counts(b["log"])
            for k, v in cb.items(): print("      %-22s %d messages (corruption %d, error %d, warning %d, info %d, message %d; not read %d, dropped %d)" % (k, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]))
            for m in fb[:20]: print("      in full: %s id %s: %s" % (m[0], m[1], m[2][:220]))
            need = ("the probe", "create and warm-up", "set end", "exit")
            on = "d3d12 debug: on, asked before the first device" in b["log"] and "has no message queue" not in b["log"] and "not available here" not in b["log"]
            tot = lambda i: sum(v[i] for v in cb.values())
            ok = proof(on and all(k in cb for k in need) and tot(1) + tot(2) == 0, b["sid"], "THE DEBUG LAYER ON THE PRODUCT: on the helper's device, counted at the probe, behind the create and the warm-up, at the set's end and at the exit, and NO ERROR AND NO CORRUPTION in the whole session (%d messages in all: %d warning, %d info, %d message): its picture cuts' own images and passes are made and used as Direct3D 12 wants" % (
                tot(0), tot(3), tot(4), tot(5))) and ok
            amd = SESSIONS.get("DF-loader-dbg")
            if amd is not None:
                ca, fa = debug_counts(amd["log"])
                ids = lambda f: sorted({(m[0], m[1]) for m in f})
                ok = proof(set(ids(fb)) <= set(ids(fa)), b["sid"], "and the product prints no kind of message AMD's library does not (the product %s, AMD's %s)" % (ids(fb) or "none", ids(fa) or "none")) and ok
        if a is not None:
            for tag, kw, what in (("-pbeside", dict(beside=True), "WITH AMD'S FILE LYING IN THE HELPER'S FOLDER the product gives the same images as without it, and the identity gate above says AMD's file stayed unmapped"),
                                  ("-pbyname", dict(by_name=True), "BY ITS BARE NAME BESIDE THE HELPER (the form a leg in the game uses: --fgown=dlssiv-fg.dll) the product gives the same images as by its full path")):
                s = hash_session(cfg, "ours", tag, **kw)
                if s is None: ok = False; continue
                c, diff, same = compare(a, s)
                ok = proof(same and equal(c), cfg, "%s (%s)" % (what, words(c))) and ok
            ok = step_upscalers(cfg, a, "ours", "-p") and ok
    return ok, got


def product_record():
    """THE PRODUCT IS ITS RECORD'S, and which cuts are in it by the build's own list beside it. -> its record, or None"""
    global PRODUCT_WORDS
    rec3 = own_record("RECORD-dlssiv-fg-cut.txt", "dlssiv-fg.dll")
    built3 = C.LIBS["ours"][1]
    proof(rec3 is not None and C.sha256_of(built3) == rec3["sha256"] and os.path.getsize(built3) == int(rec3["size"]), "build",
          "dlssiv-fg.dll as built in this tree IS THE PRODUCT'S RECORD'S LIBRARY (tools\\ffx-own\\RECORD-dlssiv-fg-cut.txt: %s bytes, SHA-256 %s, %s cut(s), digest %s)" % (
              (rec3["size"], rec3["sha256"], rec3.get("cuts", "?"), rec3.get("cuts-sha256", "?")) if rec3 else ("no", "NO RECORD", "?", "?")))
    cp = os.path.join(os.path.dirname(built3), "cuts.txt")
    names = [l.split()[1] for l in open(cp, encoding="ascii").read().splitlines() if l.startswith("cut ")] if os.path.exists(cp) else None
    acc = accepted_cuts()[0]
    PRODUCT_WORDS = "NO cuts.txt BESIDE IT" if names is None else "%d cut(s) in it: %d exact and the %d accepted picture cut(s) %s" % (len(names), len(names) - len(acc), len(acc), ", ".join(acc) or "(none)")
    print("THE PRODUCT'S CUTS, by the build's own list beside it: %s" % PRODUCT_WORDS)
    proof(names is not None and rec3 is not None and str(len(names)) == rec3.get("cuts") and names[len(names) - len(acc):] == acc, "build", "the build's list beside the product names as many cuts as its record, the accepted picture cuts last: %s" % PRODUCT_WORDS)
    return rec3


def main_pictures(t0):
    """--ours --pictures and --ours --new-pictures: the product's pictures alone"""
    if not proof(os.path.exists(C.LIBS["ours"][1]), "build", "the product is built in this tree (%s; build-ffx-own.ps1 -Target dlssiv-fg)" % C.LIBS["ours"][1]):
        print("FAIL"); return 1
    product_record()
    if PICTURES_OF:
        full = os.path.abspath(PICTURES_OF)
        if not proof(os.path.isfile(full), "build", "--pictures-of names a library that is there: %s" % full):
            print("FAIL"); return 1
        C.LIBS["other"] = ("%s, ANOTHER LIBRARY BY ITS PATH (--pictures-of)" % os.path.basename(full), full)
        C.OWN_IFACE = tuple(C.OWN_IFACE) + ("other",)
    cfgs = [c for c in CFG if not ONLY or c in ONLY]
    ok, got = step_product(cfgs, NEW_PICTURES, False)
    left = C.end_ours("left at the end") if C.ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    if ONLY: proof(False, "scope", "NOT EVERY CONFIGURATION: --only named %s" % ",".join(ONLY))
    for sid, l in C.fails[:30]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    print("%d proofs passed, %d failed, in %.0f s; the folder is kept: %s" % (len(C.passes), len(C.fails), time.time() - t0, C.OUT))
    if NEW_PICTURES:
        if C.fails or len(got) != len(CFG):
            print("FAIL: THE PIN WAS NOT WRITTEN: a pin is made from every configuration, each run twice with the same pictures, with every proof of the sessions held"); return 1
        acc, adigest = accepted_cuts()
        changed = write_pin([("picture-cuts", [str(len(acc))]), ("picture-cuts-sha256", [adigest]), ("arms", [ARMS]), ("pairs-an-arm", [str(PAIRS)])] + got)
        for k, old, new in changed: print("  replaced  %-20s %s  ->  %s" % (k, " ".join(old) if old is not None else "(no pin)", " ".join(new) if new is not None else "(gone)"))
        print("THE PIN OF THE PRODUCT'S PICTURES WRITTEN, AND THIS RUN IS NOT A PASS: tools\\ffx-own\\PICTURES-dlssiv-fg.txt now holds the pictures of dlssiv-fg.dll (%s) in %d configurations, each made twice and the same twice; %s. "
              "Commit it with what it was made from, then run the judge (python tools\\test-fg-own\\run.py --ours)." % (PRODUCT_WORDS, len(got), ("%d value(s) replaced" % len(changed)) if changed else "it replaced nothing: the pin already said all of it"))
        return 3
    if not C.fails:
        print("PASS: THE PRODUCT'S PICTURES ARE THE PINNED ONES: dlssiv-fg.dll (%s), its record's library, generates in every configuration (%s) the pictures tools\\ffx-own\\PICTURES-dlssiv-fg.txt pins, pair by pair by one digest a configuration%s;"
              " every session proved which module ran and through which interface. NOT SAID HERE: that the exact cuts are exact (the whole judge: python tools\\test-fg-own\\run.py --ours)." % (
                  PRODUCT_WORDS, ", ".join(c for c, _ in got), ("; and so does %s, pair by pair" % os.path.basename(PICTURES_OF)) if PICTURES_OF else ""))
    else: print("FAIL")
    return 0 if not C.fails else 1


# ---------------------------------------------------------------------------------------------------------------- the cost
COST_KEYS = (("prep", "AMD's prepare"), ("prep_reset", "the prepare, told to reset"), ("disp", "the dispatch, STEADY (12th after a reset or later)"),
             ("disp_reset", "the dispatch, told to RESET"), ("disp_first", "the dispatch, the FIRST after a reset"))


def cost_session(cfg, lib, take):
    st = stage(lib)
    if st is None: return None
    what, guides, fmt, hud = CFG[cfg]
    sid = "K%s-%s-%d" % (cfg, lib, take)
    fake = ["--fake-fg=40", "--fake-cost-guides=%d" % guides, "--fake-cost-hud=%d" % hud, "--fake-cost-idle=3", "--fake-cost-pairs=256", "--fake-cost-arms=P,F,FR,F1"]
    # A TIMED SESSION WANTS THE CARD TO ITSELF: it starts only when the card has been nobody else's for 20 s (another bench's
    # sessions follow each other within seconds), and one that another GPU user started under is TAKEN AGAIN, once
    s = None
    for attempt in range(1, 3):
        if not READ and not held(sid):
            C.GPU_QUIET = 20
            try: C.session(st["dir"], sid, "COST, idle: %s; %s%s" % (what, st["what"], "" if attempt == 1 else " (take %d: the one before shared the card)" % attempt), ["--fg=1", "--gpu_probe=1"] + st["args"], fake)
            finally: C.GPU_QUIET = 0
        s = C.read_session(sid)
        if s is None or READ or not (s["beside"] or s["others"]) or attempt == 2: break
        print("  [%s] another GPU user ran beside this timed session (%s): it is taken again" % (sid, s["beside"] or s["others"]))
        os.remove(C.P(sid, "rc.txt"))
    if s is None:
        proof(False, sid, "this session left nothing: it was not run"); return None
    s["st"] = st
    C.check_session(s, True); C.check_identity(s, st)
    s["idle"] = C.idle_table(s)
    cells = [x for x in s["cells"] if x.get("part") == "idle"]
    s["clock"] = C.clock_of(s["smi"], min(int(x["t0"]) for x in cells), max(int(x["t1"]) for x in cells)) if cells else {"words": "no cell"}
    return s


def step_cost():
    print("\n" + "=" * 118)
    A, B = ("own", "ours") if OURS else ("loader", "own")      # --ours: the second library against stage 1's (which stage 1b held to AMD's)
    NA, NB = ("stage 1's", "the second library's") if OURS else ("AMD's", "ours")
    print("THE COST OF A PAIR, %s AGAINST %s (TIMING). Idle: every part alone on the card. GPU microseconds by the helper's own stamps;" % (NB.upper(), NA.upper()))
    print("a round's value is the mean over the picture's six moves of each move's lower quartile; three rounds a session, two sessions a library, the libraries in turn.")
    ok = True; card = True
    for cfg in ("DF", "QF"):
        if ONLY and cfg not in ONLY: continue
        ses = {A: [], B: []}
        for take in (1, 2):
            for lib in (A, B):
                s = cost_session(cfg, lib, take) if card else None   # (a timed session that could not have the card ends the step: the next would wait as long)
                if s: ses[lib].append(s)
                else: card = False
        if len(ses[A]) < 2 or len(ses[B]) < 2:
            ok = proof(False, "cost", "%s: NOT MEASURED: a timed session was not run (the card was never this tool's alone, or the step had ended)" % cfg); continue
        print("\n  %s:" % CFG[cfg][0])
        shared = []
        for lib in (A, B):
            for s in ses[lib]:
                beside = "; ".join(x for x in (s["others"], s["beside"]) if x)
                if beside: shared.append(s["sid"])
                print("    %-16s %s. %s%s" % (s["sid"], C.load_words(s["machine"]), s["clock"]["words"].replace(": VOID", ""), ("; ANOTHER GPU USER RAN BESIDE IT, ITS TIMES SHARED THE CARD: " + beside) if beside else "; the card was this session's alone"))
        if shared: ok = proof(False, "cost", "%s: NOT MEASURED ALONE: another GPU user ran beside %s, after two takes: the table below is what was read and is NOT JUDGED" % (cfg, ", ".join(shared)))
        print("    %-52s %-34s %-34s %s" % ("", "%s (two sessions)" % NA, "%s (two sessions)" % NB, "the second less the first; the first's own two sessions apart"))
        for key, what in COST_KEYS:
            tk = lambda s: s["idle"]["takes"][key]
            a = [C.med(tk(s)) for s in ses[A] if tk(s)]; b = [C.med(tk(s)) for s in ses[B] if tk(s)]
            if len(a) < 2 or len(b) < 2:
                ok = proof(False, "cost", "%s, %s: a session gave no value" % (cfg, key)); continue
            rounds = lambda L: " / ".join(C.spread(tk(s)) for s in L)
            ma, mb = sum(a) / len(a), sum(b) / len(b)
            null = abs(a[0] - a[1]); delta = mb - ma
            tol = max(20.0, 0.02 * ma, 2.0 * null)
            print("    %-52s %-34s %-34s %+.0f us (%+.1f%%); %.0f us" % (what, rounds(ses[A]), rounds(ses[B]), delta, 100.0 * delta / ma, null))
            if shared: continue
            ok = proof(abs(delta) <= tol, "cost", "%s, %s: %s %.0f us, %s %.0f us: %+.0f us, within the bench's own null (%.0f us allowed: 2 percent, 20 us, or twice what the first's own two sessions lie apart)" % (
                cfg, what, NB, mb, NA, ma, delta, tol)) and ok
        # THE CPU'S SIDE, SAID AND NOT JUDGED: the two libraries are two compilations of the same source (AMD's own flags are
        # not published), and the serve thread's clocks follow whatever else the machine does
        for key, what in (("cpu_cfg", "CPU: AMD's configure"), ("cpu_prec", "CPU: the recording of the prepare"), ("cpu_drec", "CPU: the recording of the steady dispatch")):
            tk = lambda s: s["idle"]["takes"][key]
            print("    %-52s %-34s %-34s (medians of a round, us; said, not judged)" % (what, " / ".join(C.spread(tk(s), 1) for s in ses[A]), " / ".join(C.spread(tk(s), 1) for s in ses[B])))
    return ok


# ---------------------------------------------------------------------------------------------------------------- main
def main():
    if (PICTURES or NEW_PICTURES or PICTURES_OF) and (not OURS or READ or COST or COST_ONLY or (PICTURES_OF and not (PICTURES or NEW_PICTURES))):
        raise SystemExit("usage: --pictures and --new-pictures are --ours's: python tools\\test-fg-own\\run.py --ours --pictures [--pictures-of=<library>] | --ours --new-pictures")
    if not READ:
        if not KEEP: shutil.rmtree(C.OUT, ignore_errors=True)
        os.makedirs(C.OUT, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(C.OUT, "result-read.txt" if READ else "result.txt"))   # a reading never writes over what the run itself printed
    print("the own build of AMD's frame generation against AMD's signed library, %s%s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M"),
          "; THE PRODUCT'S PICTURES ALONE (--ours %s): dlssiv-fg.dll against the pin of its pictures" % ("--new-pictures" if NEW_PICTURES else "--pictures") if (PICTURES or NEW_PICTURES) else
          "; THE SECOND LIBRARY (--ours): dlssiv-fg-exact.dll, the exact cuts behind the mod's own interface, against AMD's signed library, stage 1's and the reference; and dlssiv-fg.dll, the product, against the pin of its pictures" if OURS else ""))
    t0 = time.time()
    full_dir = os.path.join(C.OUT, "full")
    if not READ:
        if C.ours(): C.end_ours("left by an earlier run")
        for sub in ("full", "bare", "own", "beside"):
            if os.path.isdir(os.path.join(C.OUT, sub)): shutil.rmtree(os.path.join(C.OUT, sub))
        if C.prepare() is None:
            print("FAIL"); return 1
    if PICTURES or NEW_PICTURES: return main_pictures(t0)
    why_pos = pos_lib() if OURS and not NO_ALT else None
    if why_pos: proof(False, "build", "THE POSITION'S CONTROL (step 6c) cannot be used: %s" % why_pos)
    missing = [lib for lib in ("own",) + (("ours", "ref", "exact") if OURS else ()) + (() if NO_ALT else tuple(a[0] for a in ALTS) + (("ours-px", "ours-pos") if OURS else ())) if not os.path.exists(C.LIBS[lib][1])]
    if not proof(not missing, "build", "the own library and its two altered builds are built in this tree (%s)%s" % (C.OWN_OUT, (": MISSING " + ", ".join(missing)) if missing else "")):
        print("FAIL"); return 1
    if NO_ALT: proof(False, "build", "--no-alt: THE ALTERED BUILDS WERE LEFT OUT, so nothing says the compare can see a changed shader: this run proves nothing about equality (a development switch)")
    rec = own_record()
    built = C.LIBS["own"][1]
    proof(rec is not None and C.sha256_of(built) == rec["sha256"] and os.path.getsize(built) == int(rec["size"]), "build",
          "ffx-own-fg.dll as built in this tree IS THE RECORDED LIBRARY (tools\\ffx-own\\RECORD.txt: %s bytes, SHA-256 %s)" % ((rec["size"], rec["sha256"]) if rec else ("no", "NO RECORD")))
    rec2 = own_record("RECORD-dlssiv-fg.txt", "dlssiv-fg.dll") if OURS else None
    recx = own_record("RECORD-dlssiv-fg-exact.txt", "dlssiv-fg-exact.dll") if OURS else None
    rec3 = None
    if OURS:
        global CUTS_WORDS
        built2 = C.LIBS["ref"][1]
        proof(rec2 is not None and C.sha256_of(built2) == rec2["sha256"] and os.path.getsize(built2) == int(rec2["size"]), "build",
              "dlssiv-fg-ref.dll as built in this tree IS THE SECOND RECORD'S LIBRARY, stage 2's with no cut: THE REFERENCE (tools\\ffx-own\\RECORD-dlssiv-fg.txt: %s bytes, SHA-256 %s)" % ((rec2["size"], rec2["sha256"]) if rec2 else ("no", "NO RECORD")))
        builtx = C.LIBS["exact"][1]
        proof(recx is not None and C.sha256_of(builtx) == recx["sha256"] and os.path.getsize(builtx) == int(recx["size"]), "build",
              "dlssiv-fg-exact.dll as built in this tree IS THE EXACT LIBRARY'S RECORD'S (tools\\ffx-own\\RECORD-dlssiv-fg-exact.txt: %s bytes, SHA-256 %s, %s cut(s), digest %s)" % (
                  (recx["size"], recx["sha256"], recx.get("cuts", "?"), recx.get("cuts-sha256", "?")) if recx else ("no", "NO RECORD", "?", "?")))
        # which cuts are in it, by the build's own list beside the library; and that no picture cut is among them
        cp = os.path.join(os.path.dirname(builtx), "cuts.txt")
        text = open(cp, encoding="ascii").read().splitlines() if os.path.exists(cp) else None
        names = [l.split()[1] for l in text if l.startswith("cut ")] if text is not None else None
        CUTS_WORDS = "NO cuts.txt BESIDE IT" if names is None else "%d cut(s) in it%s" % (len(names), (": " + ", ".join(names)) if names else "")
        print("THE EXACT LIBRARY'S CUTS, by the build's own list beside it: %s" % CUTS_WORDS)
        proof(names is not None and recx is not None and str(len(names)) == recx.get("cuts"), "build", "the build's list beside the exact library names as many cuts as its record: %s" % CUTS_WORDS)
        acc = accepted_cuts()[0]
        proof(text is not None and "exact yes" in text and not [n for n in (names or []) if n in acc] and not [l for l in text if l.startswith(("variant ", "picture-cuts "))], "build",
              "and that list says it is the exact library: no accepted picture cut of the product's (%s) and no variant's is in it" % (", ".join(acc) or "none is accepted"))
        rec3 = product_record()
        proof(bool(acc) == (rec3 is not None and recx is not None and rec3["sha256"] != recx["sha256"]), "build", "the product %s the exact library's bytes: %d picture cut(s) are accepted" % ("is NOT" if acc else "IS", len(acc)))
    for lib in () if NO_ALT else tuple(a[0] for a in ALTS) + (("ours-px", "ours-pos") if OURS else ()):
        proof(rec is not None and C.sha256_of(C.LIBS[lib][1]) not in (rec["sha256"], (rec2 or {}).get("sha256"), (rec3 or {}).get("sha256"), (recx or {}).get("sha256")), "build", "%s is NOT a recorded library (SHA-256 %s): a control that could be the product would prove nothing" % (os.path.basename(C.LIBS[lib][1]), C.sha256_of(C.LIBS[lib][1])))
    ok = True
    if not COST_ONLY:
        ok = step_refusals_ours(full_dir) if OURS else step_refusals(full_dir)
        if "--refusals" in ARGS:               # development: step 1 alone
            print("%d proofs passed, %d failed (step 1 alone: --refusals)" % (len(C.passes), len(C.fails)))
            print("FAIL: step 1 alone proves nothing about the picture" if not C.fails else "FAIL"); return 1
        ok = step_debug() and ok
        for s in SESSIONS.values():
            if s.get("device"):
                print("\n3. THE DEVICE'S LINE (the helper's own): %s" % s["device"]); break
        proof(any(s.get("device") for s in SESSIONS.values()), "device", "the helper said its device's shader model, wave lanes and 16-bit support once")
        ran = 0
        for cfg in CFG:
            if ONLY and cfg not in ONLY: continue
            plain = OWN_ONLY or cfg in NO_CONTROLS
            ok = step_config(cfg, not plain and (not QUICK or ran == 0), not plain and ran == 0) and ok
            ran += 0 if plain else 1
        if OURS:
            pok, pgot = step_product([c for c in CFG if not ONLY or c in ONLY], True, not OWN_ONLY)
            ok = pok and ok
    if COST_ONLY or ONLY is not None or QUICK or OWN_ONLY:
        proof(False, "scope", "NOT THE WHOLE PROOF: %s" % ("--cost-only ran the cost tables alone" if COST_ONLY else "--only named %s" % ",".join(ONLY) if ONLY else
                                                         "--own-only ran no control" if OWN_ONLY else "--quick ran the controls in the first configuration alone"))
    if COST or COST_ONLY: ok = step_cost() and ok
    left = C.end_ours("left at the end") if C.ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    for sid, l in C.fails[:30]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    print("%d proofs passed, %d failed, in %.0f s; the folder is kept: %s" % (len(C.passes), len(C.fails), time.time() - t0, C.OUT))
    if not C.fails:
        if OURS: print("PASS: dlssiv-fg-exact.dll, THE EXACT LIBRARY (%s), generates AMD's picture through the mod's own interface: every whole-image hash of every compared pair equals the signed library's, stage 1's AND THE REFERENCE'S with no cut"
                       " (steady, reset and the way back; FP16 and 8-bit; DLAA and Quality guides; the two-image set and the three-image one), AMD's library is repeatable, both altered builds of it fail, THE ONE-PIXEL CONTROL FAILS ON EVERY PAIR, THE POSITION'S CONTROL FAILS ON EVERY ARM THAT MOVES,"
                       " the debug layer is silent on both, and every session proved which module ran and through which interface; AND dlssiv-fg.dll, THE PRODUCT (%s), WHOSE PICTURE IS ITS OWN, gives the same image twice and THE PINNED PICTURES"
                       " in every configuration (tools\\ffx-own\\PICTURES-dlssiv-fg.txt), which are not the exact library's; the debug layer finds no error in it, and it gives those pictures beside AMD's file, by its bare name and beside a living upscaler." % (CUTS_WORDS, PRODUCT_WORDS))
        else: print("PASS: ffx-own-fg.dll generates AMD's picture: every whole-image hash of every compared pair equals the signed library's (steady, reset and the way back; FP16 and 8-bit; DLAA and Quality guides),"
              " AMD's library is repeatable and loads direct unchanged, both altered builds fail, the debug layer is silent on both, and every session proved which module ran.")
    else: print("FAIL")
    return 0 if not C.fails else 1


def own_record(name="RECORD.txt", library="ffx-own-fg.dll"):
    """The committed record of ffx-own-fg.dll (tools\\ffx-own\\RECORD.txt: "key value" lines; tools\\ffx-own\\ffx_record.py writes
    it and tools\\prove-ffx-own.py holds a build to it): {key: value}, or None when this tree has none. With the two
    arguments: the second library's (RECORD-dlssiv-fg.txt, dlssiv-fg.dll)."""
    p = os.path.join(ROOT, "tools", "ffx-own", name)
    if not os.path.exists(p): return None
    rec = {}
    for l in open(p, encoding="ascii", errors="replace"):
        f = l.split()
        if len(f) == 2 and not l.startswith("#"): rec[f[0]] = f[1]
    return rec if rec.get("library") == library and re.fullmatch(r"[0-9a-f]{64}", rec.get("sha256", "")) and rec.get("size", "").isdigit() else None


if __name__ == "__main__":
    sys.exit(main())
