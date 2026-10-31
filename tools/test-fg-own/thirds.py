r"""THE FORMS OF A PAIR THROUGH THE MOD'S OWN LIBRARY, INTERFACE 3, PIXEL FOR PIXEL (2026-10-18, frame generation phase 5, the true
fill's package G1: docs\reports\fg-5\brief-truefill\BRIEF-G1-lib.md; the cut ffx-own\cuts\fi-store-apart.cut).

THE QUESTION. The true fill makes a pair into ONE generated frame (as today), TWO (at 1/3 and 2/3: interpolate(1/3, NO_STORE),
interpolate(2/3)) or NONE (prepare, optical flow, interpolate(STORE_ONLY)). Is each frame of every form the frame a single
interpolate at its position makes, and is the pair after a form the pair it is after a whole dispatch? The answer is the
whole generated image of every frame, by its SHA-256, through tools\test-fg-own\fgtime.cpp (built here as fgpipe-thirds.exe,
a name every proof's wait for the card sees), one process a replay, the context created as the helper creates it (no stamps).

TWO KINDS OF RUN.
  STRICT, ON MADE RUNS (equal hashes, every frame). AMD's method races with itself on the game's frames (two replays of one
     library differ in some pixels: tools\test-fg-own\env.py, THE THREE-REPLAY RULE), so the strict proof is made on runs
     whose picture cannot race: one presented frame of a captured run (--from-run, cap5\run-01's frame 0), slid across the
     screen by 6 pixels a frame (4 of the guides), its depth slid with it and ONE vector for every pixel, the motion the
     pictures really have. "slide" is that run with no HUD-less image (the game's shape); "slide-hud" adds a HUD-less image
     (the presented one has a HUD drawn over it at fixed places, and the HUD-less one an alpha of 0 where a 3D overlay
     would cover it), the shape in which the inpainting pass keeps the source itself (fi-inpainting-keeps-the-source) and
     the product's fi-cover-in-alpha acts. FIRST THE INSTRUMENT: a library replayed twice gives the same hashes (else the
     run is not one a hash can judge, and the strict part says so and fails).
  BY THE THREE-REPLAY RULE, ON A CAPTURED RUN (--captured, cap5\run-01: 3840x2160, FP16, the guides at 2560x1440): each side
     replayed EIGHT times (--replays; three let one racing pixel through on 2026-10-18), frames 8 to 15 read back whole; a
     pixel counts only where it is the same in every replay of each side, and NONE may differ between the two sides; and no
     pixel may take values in one side that it never takes in the other (the race cannot explain that). The pixels the race
     leaves unsteady are printed beside.
THE CASES (each library: the product, dlssiv-fg.dll, and the exact library, dlssiv-fg-exact.dll):
  A  TWO FRAMES OF ONE PAIR: --form=two; its first image of every frame equals a run of single interpolates at 1/3, its second
     a run at 2/3. (A frame of the second interpolate that RESET would be the presented picture, and differ.)
  B  THE CONTROL: --form=two-control, the first interpolate told nothing (it keeps the pair's picture, and the second, not a later
     frame of an open pair, counts the pair again): the second must DIFFER from the run at 2/3 on every frame that moves
     (frames 1 to 15); the first equals the run at 1/3 until the count kept twice crosses 10.
  B2 THE LIBRARY CONTROL OF THE STORE (the exact library's cuts and control-store-first: the glue lets AMD store where NO_STORE
     is asked, and still does not count the pair again): its second image must DIFFER from 2/3 on every moving frame.
  B3 THE LIBRARY CONTROL OF THE COUNT (control-count-again: the glue never says AGAIN, every dispatch of a pair is counted): equal
     on pairs 0 to 4; on pairs 5 to 9 it is printed whether the count reaches the picture of the run at all.
  C  NO FRAME (the flow-only pair): --bare=4,7,8,12 (two of them in a row), a single interpolate at the middle on every other
     pair: every other frame equals the full run at the middle, the frames right after a bare pair among them.
  D  ITS CONTROL: --bare-control=1, the bare pairs with no STORE_ONLY call: the frame after each must DIFFER.
  E  THE TRUE FILL'S MIX: --form=two with the same bare pairs: every other frame's two images equal the runs at 1/3 and 2/3.
  F  A PAIR LEFT OPEN: --open=6 at 1/3 (its interpolate told NO_STORE, nothing closes it): every frame equals a run at 1/3
     told RESET at frame 7, and frame 7 DIFFERS from the plain run at 1/3 (the reset was needed and was made).
  G  A PAIR CLOSED BY STORE_ONLY: --open=6 --close=1 at 1/3: every frame equals the plain run at 1/3.
THE PRICE (--price): the stamps (fgtime.cpp with DLSSIV_FG_CREATE_STAMPS) on the captured run, loops 12, the steady frames,
   the product: a single interpolate at 1/2, 1/3 and 2/3; the form two; every pair bare (the store alone). THE CALL lines
   are each call's spans together (lower quartile, median). The card's clocks are read before and after (nvidia-smi).

    python tools\test-fg-own\thirds.py [--libs=product,exact] [--strict-only | --captured-only] [--keep]
    python tools\test-fg-own\thirds.py --price
    python tools\test-fg-own\thirds.py --env      v-env-position (ffx-own\cuts\v-env-position.cut): the product against the product
                                                  less it on the made runs: the same 16 images at the middle, and where it changes
                                                  the picture at 1/3 and 2/3
ONE GPU USER AT A TIME: before every replay it waits while the game, a helper, another bench's client or a dt_fg_test that
holds a graphics library is running (tools\test-fg-own\devices.py). It opens no window. Its folder is
%TEMP%\dlssiv-test-fg-thirds (the made runs lie there, about 4.2 GB, and are kept; --keep keeps the replays too). The last
line is PASS or FAIL (--price: DONE).
"""
import ctypes, hashlib, os, re, shutil, subprocess, sys, tempfile, time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
NOWIN = subprocess.CREATE_NO_WINDOW
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-thirds")
RUNS = arg("--runs", os.path.join(tempfile.gettempdir(), "dlssiv-fg-runs"))
CAPTURED = arg("--captured-run", r"cap5\run-01")
LIBS = {"product": os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll"), "exact": os.path.join(ROOT, "obj", "dlssiv-fg-exact", "dlssiv-fg-exact.dll"),
        # THE CONTROLS (ffx-own\cuts\control-store-first.cut, control-count-again.cut), on the exact library's cuts:
        #   build-ffx-own.ps1 -Target dlssiv-fg -Cuts exact,+control-store-first -OutDir obj\dlssiv-fg-store
        #   build-ffx-own.ps1 -Target dlssiv-fg -Cuts exact,+control-count-again -OutDir obj\dlssiv-fg-count
        "store-control": os.path.join(ROOT, "obj", "dlssiv-fg-store", "dlssiv-fg.dll"), "count-control": os.path.join(ROOT, "obj", "dlssiv-fg-count", "dlssiv-fg.dll"),
        # (--env) the product less v-env-position: build-ffx-own.ps1 -Target dlssiv-fg -Cuts -v-env-position -OutDir obj\dlssiv-fg-noenvpos
        "noenvpos": os.path.join(ROOT, "obj", "dlssiv-fg-noenvpos", "dlssiv-fg.dll")}
COUNT_SEEN, ENV_SEEN = [], []
WANT = [x for x in arg("--libs", "product,exact").split(",") if x]
T1, T2 = "0.33333333", "0.66666667"
BARE = "4,7,8,12"
REPLAYS = int(arg("--replays", "8"))      # (the captured run: replays a side)
EXE = os.path.join(OUT, "fgpipe-thirds.exe")
MADE = "made-3"          # the made runs' version: a run folder whose made.txt says another is made again
sys.path.insert(0, HERE)
import devices

passes, fails = [], []


def proof(ok, text):
    (passes if ok else fails).append(text)
    print("  %s: %s" % ("PASS" if ok else "FAIL", text), flush=True)
    return ok


# ---------------------------------------------------------------------------------------------------------------- the card
GPU_NAMES = re.compile(r"^(gtaiv\.exe|dlssiv-helper.*\.exe|fgpipe.*\.exe|fgcost.*\.exe|vklines.*\.exe|dt_fg_test.*\.exe)$", re.I)


def gpu_users():
    r = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True, errors="replace", creationflags=NOWIN)
    out = []
    for l in r.stdout.splitlines():
        f = [x.strip('"') for x in l.split('","')]
        if len(f) < 2 or not GPU_NAMES.match(f[0]): continue
        if f[0].lower().startswith("dt_fg_test") and not devices.makes_device(int(f[1]))[0]: continue
        out.append("%s pid %s" % (f[0], f[1]))
    return out


def wait_card(what):
    said = False
    while True:
        u = gpu_users()
        if not u: return
        if not said: print("  (waiting for the card before %s: %s)" % (what, ", ".join(u)), flush=True); said = True
        time.sleep(10)


def clocks():
    r = subprocess.run(["nvidia-smi", "--query-gpu=name,pstate,clocks.gr,clocks.mem,clocks.max.gr,temperature.gpu,power.draw", "--format=csv,noheader"], capture_output=True, text=True, errors="replace", creationflags=NOWIN)
    return r.stdout.strip() or "nvidia-smi said nothing"


# ---------------------------------------------------------------------------------------------------------------- the tool
def vcvars():
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], creationflags=NOWIN).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars64.bat")


def build_exe():
    src = os.path.join(HERE, "fgtime.cpp")
    deps = [src, os.path.join(ROOT, "tools", "fg-replay", "fgreplay.cpp"), os.path.join(ROOT, "ffx-own", "glue", "dlssiv_fg.h")]
    if os.path.exists(EXE) and os.path.getmtime(EXE) > max(os.path.getmtime(p) for p in deps): return
    cl = 'call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 "%s" /Fe:fgpipe-thirds.exe /Fo:fgpipe-thirds.obj /link /INCREMENTAL:NO d3d12.lib dxgi.lib bcrypt.lib' % (vcvars(), src)
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    if r.returncode != 0 or "warning C" in r.stdout: raise SystemExit("fgtime.cpp did not compile: " + r.stdout[-2000:])


def fg(run, lib, tag, dump=0, raw=False, **kw):
    """one replay; -> {(frame, image tag): sha256}, the folder"""
    out = os.path.join(OUT, "replays", tag)
    if os.path.isdir(out): shutil.rmtree(out)
    os.makedirs(out)
    cmd = [EXE, "--run=" + run, "--lib=" + LIBS[lib], "--loops=2", "--stamps=0", "--out=" + out, "--dump=%d" % dump] + (["--raw=1"] if raw else [])
    cmd += ["--%s=%s" % (k.replace("_", "-"), v) for k, v in kw.items()]
    wait_card(tag)
    r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=1800)
    if "DONE" not in r.stdout or r.returncode != 0:
        raise SystemExit("the replay %s did not end: %s" % (tag, " | ".join(r.stdout.strip().splitlines()[-3:])))
    if not re.search(r"interface 3;", r.stdout): raise SystemExit("the replay %s: the library does not say interface 3: %s" % (tag, r.stdout.splitlines()[0][:400]))
    h = {}
    for l in open(os.path.join(out, "hashes.txt"), encoding="ascii").read().splitlines():
        f = l.split()
        h[(int(f[0][1:]), f[1])] = f[2]
    return h, out


# ---------------------------------------------------------------------------------------------------------------- the made runs
def make_slide(name, with_hud):
    """a run of 16 frames made of one captured frame slid across the screen: 6 display pixels a frame to the left (4 of the
    guides), its depth slid with it, one vector (+4, 0) for every pixel of the guides (from the current frame to the
    previous one: AMD's sense). With a HUD-less image: the presented picture is it with a HUD drawn at fixed places, and the
    HUD-less one has an alpha of 0 under a rectangle a 3D overlay covers (also drawn into the presented one)."""
    src = os.path.join(RUNS, CAPTURED)
    dst = os.path.join(OUT, "runs", name)
    mark = os.path.join(dst, "made.txt")
    if os.path.isfile(mark) and open(mark).read().strip() == MADE: return dst
    if os.path.isdir(dst): shutil.rmtree(dst)
    os.makedirs(dst)
    lines = open(os.path.join(src, "index.txt"), encoding="ascii").read().splitlines()
    W, H, RW, RH = 3840, 2160, 2560, 1440
    p0 = np.fromfile(os.path.join(src, "f00-present.raw"), dtype=np.float16).reshape(H, W, 4)
    d0 = np.fromfile(os.path.join(src, "f00-depth.raw"), dtype=np.float32).reshape(RH, RW)
    mv = np.zeros((RH, RW, 2), dtype=np.float16); mv[..., 0] = 4.0
    out = []
    for l in lines:
        if l.startswith("file: "): continue
        if l.startswith("hudless: "): l = "hudless: %d" % (1 if with_hud else 0)
        if l.startswith("frame: "):
            l = re.sub(r" hud=\d", " hud=%d" % (1 if with_hud else 0), l)
        if l.startswith("frame_plain: "): continue
        if l == "end": continue
        out.append(l)
    out.append("made: tools\\test-fg-own\\thirds.py from %s frame 0, slid 6 display pixels a frame, one vector (+4, 0) of the guides%s" % (CAPTURED, ", a HUD and a cover in the HUD-less alpha" if with_hud else ""))
    for k in range(16):
        scene = np.roll(p0, -6 * k, axis=1)
        if with_hud:
            hudless = scene.copy()
            present = scene.copy()
            for (x, y, w, h, c) in ((120, 1900, 700, 140, 2.0), (3300, 80, 420, 90, 0.05), (1700, 60, 400, 40, 4.0)):
                present[y:y + h, x:x + w, 0:3] = c
            hudless[1200:1800, 2900:3300, 3] = 0.0           # the cover: a phone at a fixed screen place
            present[1200:1800, 2900:3300, 0:3] = np.array([0.3, 0.6, 1.2], dtype=np.float16)
            hudless.tofile(os.path.join(dst, "f%02d-hud.raw" % k))
        else:
            present = scene
        present.tofile(os.path.join(dst, "f%02d-present.raw" % k))
        np.roll(d0, -4 * k, axis=1).tofile(os.path.join(dst, "f%02d-depth.raw" % k))
        mv.tofile(os.path.join(dst, "f%02d-mv.raw" % k))
        out.append("file: f%02d-present.raw present %d %d %d 10 8 %d" % (k, k, W, H, W * H * 8))
        out.append("file: f%02d-gen.raw gen %d %d %d 10 8 %d" % (k, k, W, H, W * H * 8))       # (its size and format alone are read)
        out.append("file: f%02d-depth.raw depth %d %d %d 41 4 %d" % (k, k, RW, RH, RW * RH * 4))
        out.append("file: f%02d-mv.raw mv %d %d %d 34 4 %d" % (k, k, RW, RH, RW * RH * 4))
        if with_hud: out.append("file: f%02d-hud.raw hud %d %d %d 10 8 %d" % (k, k, W, H, W * H * 8))
    out.append("end")
    open(os.path.join(dst, "index.txt"), "w", encoding="ascii", newline="\n").write("\n".join(out) + "\n")
    open(mark, "w").write(MADE + "\n")
    return dst


# ---------------------------------------------------------------------------------------------------------------- the compares
def same_frames(a, ta, b, tb, frames):
    """the frames whose image ta of a equals image tb of b, and those that differ"""
    eq = [i for i in frames if a.get((i, ta)) is not None and a.get((i, ta)) == b.get((i, tb))]
    return eq, [i for i in frames if i not in eq]


def strict(run, lib, label):
    print("\n[%s, %s] STRICT: equal hashes, every frame" % (label, lib), flush=True)
    every = list(range(16))
    moved = list(range(1, 16))
    m1, _ = fg(run, lib, "%s-%s-mid-1" % (label, lib))
    m2, _ = fg(run, lib, "%s-%s-mid-2" % (label, lib))
    eq, ne = same_frames(m1, "gen-replay", m2, "gen-replay", every)
    if not proof(not ne, "[%s, %s] THE INSTRUMENT: two replays at the middle give the same 16 images (else a hash cannot judge this run)%s" % (label, lib, "" if not ne else ": frames %s differ" % ne)):
        return
    r1, _ = fg(run, lib, "%s-%s-t1" % (label, lib), position=T1)
    r2, _ = fg(run, lib, "%s-%s-t2" % (label, lib), position=T2)
    eq, ne = same_frames(r1, "gen-replay", m1, "gen-replay", moved)
    proof(len(ne) == len(moved), "[%s, %s] the run at 1/3 is not the run at the middle on any moving frame (%d of %d differ): the position reaches the picture" % (label, lib, len(ne), len(moved)))
    # A
    two, _ = fg(run, lib, "%s-%s-two" % (label, lib), form="two", t1=T1, t2=T2)
    ea, na = same_frames(two, "gen-a", r1, "gen-replay", every)
    eb, nb = same_frames(two, "gen-b", r2, "gen-replay", every)
    proof(not na and not nb, "[%s, %s] A. TWO FRAMES OF ONE PAIR: interpolate(1/3, NO_STORE) then interpolate(2/3) equal single interpolates at 1/3 and at 2/3 on all 16 frames (%d and %d equal)%s" % (
        label, lib, len(ea), len(eb), "" if not (na or nb) else ": DIFFER at 1/3 %s, at 2/3 %s" % (na, nb)))
    eq, ne = same_frames(two, "gen-b", m1, "gen-replay", moved)
    proof(len(ne) == len(moved), "[%s, %s]    and the second is not the middle's picture on any moving frame (it is at its own place)" % (label, lib))
    # B
    ctl, _ = fg(run, lib, "%s-%s-two-control" % (label, lib), form="two-control", t1=T1, t2=T2)
    ea, na = same_frames(ctl, "gen-a", r1, "gen-replay", every)
    eb, nb = same_frames(ctl, "gen-b", r2, "gen-replay", moved)
    # (its first is a single interpolate at 1/3 only until the count it keeps twice a pair reaches a bound the picture reads: 10 for
    # optical flow's colour, from pair 6; v-env's ramp of frames 3 to 7 in the product, from pair 3. B2 and B3 take the store and the
    # count apart, each alone)
    proof(len(nb) == len(moved) and not [i for i in na if i < 3], "[%s, %s] B. THE CONTROL (the first interpolate told nothing: it KEEPS the pair's picture, and the second counts the pair again) FAILS THE SECOND on every moving frame (%d of %d differ from the run at 2/3); its first equals the run at 1/3 on pairs 0 to 2 and differs on %s, where the count kept twice reaches a bound the picture reads" % (
        label, lib, len(nb), len(moved), na or "none"))
    if "--to-b" in ARGS: return                  # (a development switch: the cases up to B alone)
    # C
    bare_i = [int(x) for x in BARE.split(",")]
    kept = [i for i in every if i not in bare_i]
    after = [i + 1 for i in bare_i if i + 1 not in bare_i and i + 1 < 16]
    br, _ = fg(run, lib, "%s-%s-bare" % (label, lib), bare=BARE)
    eq, ne = same_frames(br, "gen-replay", m1, "gen-replay", kept)
    proof(not ne and all((i, "gen-replay") not in br for i in bare_i), "[%s, %s] C. NO FRAME (prepare, optical flow, interpolate STORE_ONLY) at pairs %s: the %d other frames equal the full run at the middle, the frames right after them (%s) among them, and the bare pairs made no image%s" % (
        label, lib, BARE, len(eq), ",".join(map(str, after)), "" if not ne else ": DIFFER %s" % ne))
    # D
    bc, _ = fg(run, lib, "%s-%s-bare-control" % (label, lib), bare=BARE, bare_control=1)
    eq, ne = same_frames(bc, "gen-replay", m1, "gen-replay", after)
    proof(len(ne) == len(after), "[%s, %s] D. ITS CONTROL (no STORE_ONLY call: the pair is not kept) FAILS on every frame after a bare pair (%d of %d differ)" % (label, lib, len(ne), len(after)))
    # E
    mix, _ = fg(run, lib, "%s-%s-two-bare" % (label, lib), form="two", t1=T1, t2=T2, bare=BARE)
    ea, na = same_frames(mix, "gen-a", r1, "gen-replay", kept)
    eb, nb = same_frames(mix, "gen-b", r2, "gen-replay", kept)
    proof(not na and not nb, "[%s, %s] E. THE MIX (two frames a pair, no frame at pairs %s): every other pair's two images equal the runs at 1/3 and 2/3 (%d and %d of %d)%s" % (
        label, lib, BARE, len(ea), len(eb), len(kept), "" if not (na or nb) else ": DIFFER %s %s" % (na, nb)))
    # F
    op, _ = fg(run, lib, "%s-%s-open" % (label, lib), position=T1, open="6")
    rs, _ = fg(run, lib, "%s-%s-t1-reset7" % (label, lib), position=T1, reset_interp_at="7")
    eq, ne = same_frames(op, "gen-replay", rs, "gen-replay", every)
    e7, n7 = same_frames(op, "gen-replay", r1, "gen-replay", [7])
    proof(not ne and n7 == [7], "[%s, %s] F. A PAIR LEFT OPEN (pair 6 told NO_STORE, nothing closes it): all 16 frames equal the run at 1/3 whose interpolate is told RESET at pair 7, and frame 7 differs from the plain run at 1/3 (the reset was needed and the glue made it)%s" % (
        label, lib, "" if not ne else ": DIFFER %s" % ne))
    # G
    cl, _ = fg(run, lib, "%s-%s-closed" % (label, lib), position=T1, open="6", close=1)
    eq, ne = same_frames(cl, "gen-replay", r1, "gen-replay", every)
    proof(not ne, "[%s, %s] G. A PAIR CLOSED BY STORE_ONLY (pair 6: interpolate(1/3, NO_STORE), then interpolate(STORE_ONLY)): all 16 frames equal the plain run at 1/3%s" % (label, lib, "" if not ne else ": DIFFER %s" % ne))
    if lib != "exact": return
    # THE TWO LIBRARY CONTROLS, on the exact library's cuts: each changes one thing of the store apart
    sc, _ = fg(run, "store-control", "%s-store-control-two" % label, form="two", t1=T1, t2=T2)
    ea, na = same_frames(sc, "gen-a", r1, "gen-replay", every)
    eb, nb = same_frames(sc, "gen-b", r2, "gen-replay", moved)
    proof(not na and len(nb) == len(moved), "[%s, control-store-first] B2. THE LIBRARY THAT STORES AT THE FIRST OF TWO FRAMES (NO_STORE asked, the pair still not counted again) FAILS THE SECOND on every moving frame (%d of %d), and its first equals the run at 1/3 (%d of 16)" % (
        label, len(nb), len(moved), len(ea)))
    cc, _ = fg(run, "count-control", "%s-count-control-two" % label, form="two", t1=T1, t2=T2)
    early = list(range(0, 5)); late = list(range(5, 10))
    ea, na = same_frames(cc, "gen-a", r1, "gen-replay", early)
    eb, nb = same_frames(cc, "gen-b", r2, "gen-replay", early)
    la, lna = same_frames(cc, "gen-a", r1, "gen-replay", late)
    lb, lnb = same_frames(cc, "gen-b", r2, "gen-replay", late)
    proof(not na and not nb, "[%s, control-count-again] B3. THE LIBRARY THAT COUNTS EVERY DISPATCH OF A PAIR equals single interpolates on pairs 0 to 4 (where a count of two a pair stays on the same side of 0 and of 10)" % label)
    print("  (B3: on pairs 5 to 9, where a count of two a pair crosses 10, it differs at 1/3 on %s and at 2/3 on %s: %s)" % (
        lna or "none", lnb or "none", "THE COUNT REACHES THE PICTURE ON THIS RUN, so the equal frames of A prove the count of pairs" if (lna or lnb) else
        "on this run the count does not reach the picture (optical flow's colour and the game's agree), so A proves the count of pairs only where another run shows it"), flush=True)
    COUNT_SEEN.append(bool(lna or lnb))


def load(folder, i, tag):
    return np.fromfile(os.path.join(folder, "f%02d-%s.raw" % (i, tag)), dtype=np.uint64)


def three(run, lib, label):
    print("\n[%s, %s] BY THE THREE-REPLAY RULE: frames 8 to 15, a pixel counts where it is the same in three replays of each side" % (label, lib), flush=True)
    F = list(range(8, 16))
    sides = {}
    for name, kw in (("t1", dict(position=T1)), ("t2", dict(position=T2)), ("mid", {}), ("two", dict(form="two", t1=T1, t2=T2)),
                     ("control", dict(form="two-control", t1=T1, t2=T2)), ("bare", dict(bare="9,12,13"))):
        sides[name] = [fg(run, lib, "%s-%s-%s-%d" % (label, lib, name, k), dump=8, raw=True, **kw)[1] for k in range(REPLAYS)]

    # (2026-10-18, the first run: with THREE replays a side one pixel of A's second image was steady in each side with two values; eight
    # replays of each found it, and every pixel that moved, sharing a value between the sides: the race. So EIGHT replays a side, and
    # a second count beside the rule's: the pixels where the two sides' replays share NO value, which the race cannot explain)
    def judge(sa, ta, sb, tb, frames):
        steady_a = steady_b = differ = disjoint = total = 0
        for i in frames:
            a = np.stack([load(f, i, ta) for f in sides[sa]]); b = np.stack([load(f, i, tb) for f in sides[sb]])
            sa_ = (a == a[0]).all(axis=0); sb_ = (b == b[0]).all(axis=0)
            differ += int(np.count_nonzero(sa_ & sb_ & (a[0] != b[0])))
            moved = np.nonzero(~sa_ | ~sb_ | (a[0] != b[0]))[0]
            if moved.size <= 200000:
                disjoint += sum(1 for p in moved if not (set(a[:, p].tolist()) & set(b[:, p].tolist())))
            else:
                disjoint += int(moved.size)          # (a control: so many pixels move that a count of each is not needed)
            steady_a += int(np.count_nonzero(sa_)); steady_b += int(np.count_nonzero(sb_)); total += a[0].size
        return differ, 100.0 * (1 - steady_a / total), 100.0 * (1 - steady_b / total), disjoint

    w = "%d replays" % REPLAYS
    d, ua, ub, dj = judge("two", "gen-a", "t1", "gen-replay", F)
    proof(d == 0 and dj == 0, "[%s, %s] A. the first of two frames of a pair against single interpolates at 1/3, %s a side: %d steady pixels differ over 8 frames, %d share no value (unsteady by the race: %.3f%% and %.3f%%)" % (label, lib, w, d, dj, ua, ub))
    d, ua, ub, dj = judge("two", "gen-b", "t2", "gen-replay", F)
    proof(d == 0 and dj == 0, "[%s, %s] A. the second against single interpolates at 2/3: %d steady pixels differ, %d share no value (unsteady %.3f%% and %.3f%%)" % (label, lib, d, dj, ua, ub))
    d, ua, ub, dj = judge("control", "gen-b", "t2", "gen-replay", F)
    proof(d > 0 and dj > 0, "[%s, %s] B. THE CONTROL's second against 2/3: %d steady pixels differ, %d or more share no value (it must FAIL: unsteady %.3f%% and %.3f%%)" % (label, lib, d, dj, ua, ub))
    kept = [i for i in F if i not in (9, 12, 13)]
    d, ua, ub, dj = judge("bare", "gen-replay", "mid", "gen-replay", kept)
    proof(d == 0 and dj == 0, "[%s, %s] C. no frame at pairs 9, 12, 13 against the full run at the middle, frames %s: %d steady pixels differ, %d share no value (unsteady %.3f%% and %.3f%%)" % (label, lib, ",".join(map(str, kept)), d, dj, ua, ub))


def env(run, label):
    """(--env) v-env-position: the product against the product less that cut, on a made run. At the middle every frame must be
    the same image; at 1/3 and 2/3 the frames where it changes the picture, and by how many pixels, are printed."""
    print("\n[%s] v-env-position: the product against the product less it (obj\\dlssiv-fg-noenvpos)" % label, flush=True)
    every = list(range(16))
    for name, short, kw in (("the middle", "mid", {}), ("1/3", "t1", dict(position=T1)), ("2/3", "t2", dict(position=T2))):
        a, fa = fg(run, "product", "%s-env-product-%s" % (label, short), raw=True, **kw)
        b, fb = fg(run, "noenvpos", "%s-env-noenvpos-%s" % (label, short), raw=True, **kw)
        eq, ne = same_frames(a, "gen-replay", b, "gen-replay", every)
        if name == "the middle":
            proof(not ne, "[%s] AT THE MIDDLE v-env-position IS v-env: all 16 frames the same image as the product less it%s" % (label, "" if not ne else ": DIFFER %s" % ne))
            continue
        px = []
        for i in ne:
            x, y = load(fa, i, "gen-replay"), load(fb, i, "gen-replay")
            px.append("f%d %d" % (i, int(np.count_nonzero(x != y))))
        print("  at %s it changes %d of 16 frames, pixels a frame: %s" % (name, len(ne), ", ".join(px) or "none"), flush=True)
        ENV_SEEN.append(bool(ne))


def price(run):
    print("\nTHE PRICE: the stamps on %s through the product, 12 loops, the steady frames 12 to 15 of loops 2 to 12" % run, flush=True)
    print("  the card before: %s" % clocks(), flush=True)
    # (a warm-up replay first, not printed: on 2026-10-18 the first replay of a cold card ran at 2550 MHz and the next ones at 2790)
    wait_card("the warm-up")
    subprocess.run([EXE, "--run=" + run, "--lib=" + LIBS["product"], "--loops=12"], capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=1800)
    print("  the card after the warm-up: %s" % clocks(), flush=True)
    for label, kw in (("one at 1/2", {}), ("one at 1/3", dict(position=T1)), ("one at 2/3", dict(position=T2)), ("two (1/3 NO_STORE, 2/3)", dict(form="two", t1=T1, t2=T2)),
                      ("bare (the store alone)", dict(bare="all"))):
        wait_card(label)
        cmd = [EXE, "--run=" + run, "--lib=" + LIBS["product"], "--loops=12"] + ["--%s=%s" % (k.replace("_", "-"), v) for k, v in kw.items()]
        r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=1800)
        print("  %s:" % label)
        for l in r.stdout.splitlines():
            if "THE CALL" in l or "THE LIST" in l or "STOPPED" in l: print("    " + l.strip())
        print("    the card after it: %s" % clocks(), flush=True)
    print("DONE")


def main():
    os.makedirs(OUT, exist_ok=True)
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)
    t0 = time.time()
    build_exe()
    for lib in WANT + (["store-control", "count-control"] if "exact" in WANT else []):
        if not os.path.isfile(LIBS[lib]): raise SystemExit("%s is not built (build-ffx-own.ps1 -Target dlssiv-fg; the controls: the lines at LIBS in this file)" % LIBS[lib])
        print("the library %s: %s, SHA-256 %s" % (lib, LIBS[lib], hashlib.sha256(open(LIBS[lib], "rb").read()).hexdigest()))
    cap = os.path.join(RUNS, CAPTURED)
    if "--price" in ARGS:
        price(cap); return 0
    if "--env" in ARGS:
        if not os.path.isfile(LIBS["noenvpos"]): raise SystemExit("%s is not built (the line at LIBS)" % LIBS["noenvpos"])
        for name, hud in (("slide", False), ("slide-hud", True)): env(make_slide(name, hud), name)
        proof(any(ENV_SEEN), "v-env-position changes the picture at a third on a made run (else this run cannot show what it does there)")
        print("\n%d proofs passed, %d failed, in %.0f s" % (len(passes), len(fails), time.time() - t0)); print("PASS" if not fails else "FAIL")
        return 0 if not fails else 1
    if "--captured-only" not in ARGS:
        for name, hud in (("slide", False), ("slide-hud", True)):
            run = make_slide(name, hud)
            for lib in WANT: strict(run, lib, name)
    if "--strict-only" not in ARGS:
        three(cap, "product", CAPTURED.replace("\\", "-"))
    if "--keep" not in ARGS and os.path.isdir(os.path.join(OUT, "replays")): shutil.rmtree(os.path.join(OUT, "replays"))
    print("\n%d proofs passed, %d failed, in %.0f s" % (len(passes), len(fails), time.time() - t0))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
