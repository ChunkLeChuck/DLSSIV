r"""THE LIBRARY'S LAST ROUND OF PHASE 3 ON THE OWNER'S CAPTURED FRAMES (2026-10-12; docs\reports\fg-3\sitting-2026-10-12\BRIEF-lib4.md):
what is left of the ghost with v-env and what each mend takes away, the picture cuts each alone on top of v-env, and
the candidate library v-ship beside the product. The bench and its rules are env.py's (BRIEF-env.md): every run is
replayed FROM ITS RESET, the generated frames 8 to 15 are judged, and THE THREE-REPLAY RULE holds for every judgement (a
number is taken from each of three replays and printed once where they agree to the printed digit, else as its span;
a pixel is said to have changed only where it is the same in three replays of the one library, the same in three of
the other, and not the same in both).

TWO THINGS ARE THIS TOOL'S OWN:
  A PIXEL IN THE WRONG LIGHT  the generated frame's BROAD brightness there (the mean of 13 by 13 pixels: the ground's grain is
      averaged away, what stays is the light) is MORE THAN HALF OFF the two real frames' (their mean's). The ghost is
      such pixels: sunlit ground under the car, a shadow where none is. Counted a frame over a run's ZONE (the car's
      lower half and its shadow) and over PLACES of 96 by 64 pixels: the worst place of the library the owner ran in
      the game, and of the new one (worst by the broad difference summed over frames 10 to 15). Beside it THE BROAD
      DIFFERENCE itself (env.py's and mock5.py's number, over the real frames' brightness; the mean over the rect).
  THE SECOND LAP  a captured run is 16 frames from a reset: at its last frame v-env's envelopes have seen 15 frames, in
      the game every frame since the last cut. tools\test-fg-own\fgtime.cpp drives the run twice with no reset
      between; the frames 8 to 15 of the second lap are made with the whole run before them. v-env-1 (the library of
      the owner's leg, whose envelopes begin a frame after the reset) reads far better from the reset than it is in
      the game; the second lap says what the game showed. One replay a library, printed beside.

    python tools\test-fg-own\lib4.py env   [--only=<run>,...]   part 1: the product, v-env-1, v-env and v-env less one mend each
    python tools\test-fg-own\lib4.py cuts  [--only=...]         part 4: each picture cut alone on top of v-env, against v-env
    python tools\test-fg-own\lib4.py ship  [--only=...]         part 5: the product, v-env and v-ship; a flip sequence a run
    python tools\test-fg-own\lib4.py same  [--only=...] [--other=<variant>]   (2026-10-12, BRIEF-ship.md) THE PRODUCT'S PICTURES ARE
                                                               ANOTHER LIBRARY'S, BIT FOR BIT (not given: the frozen v-ship, the
                                                               library the owner ran): three replays of each on every run of
                                                               cap5 and prof5; a pixel counts where it is the same in three
                                                               replays of each, and NONE may differ between the two. The
                                                               replays' own spread (the race of AMD's secondary vectors) is
                                                               printed beside. Its last line is PASS or FAIL
    python tools\test-fg-own\lib4.py dlaa  --runs=<folder> [--only=...]   (2026-10-14, BRIEF-busy-cost.md Part 1, G) THE GUIDES CAPPED AT
                                                               QUALITY'S SIZE (the variant guide-cap-product: the product and
                                                               v-guide-cap, tools\ffx-own\build_variants.py) AGAINST THE PRODUCT on
                                                               a capture AT DLAA (the kit's F1C capa: every run whose index.txt
                                                               says render = display; any other run is left out and said):
                                                               three replays of each, and the second lap of each (the game's);
                                                               pixels in the wrong light and the broad difference over the whole
                                                               frame and at the worst place; percent of the steady pixels that
                                                               change by 1, 4, 16 codes; a sheet at the worst place and a flip
                                                               sequence a run, for the owner's eye. Its last line is DONE or FAIL
                                                               (--smoke: a run at Quality is taken too, where the cap changes nothing:
                                                               the instrument end to end, every change reading 0)
        --out=<folder>   where the sheets go (not given: docs\reports\fg-3\sitting-2026-10-12\lib4)
        --no-pictures    the numbers alone
NOT A PROOF AND NOT A JUDGE: THE PICTURE IS THE OWNER'S TO JUDGE. ONE GPU USER AT A TIME (it waits while the game or another
bench has the card). The replays lie under %TEMP%\dlssiv-test-fg-env, a library's by its SHA-256; nothing is written
into a captured run. What it printed is kept as result-<mode>.txt in --out. The last line is DONE or FAIL.
"""
import importlib.util, os, subprocess, sys, time

import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
MODE = next((a for a in ARGS if not a.startswith("--")), "")
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
SHOW = arg("--out", os.path.join(ROOT, "docs", "reports", "fg-3", "sitting-2026-10-12", "lib4"))
ONLY = [x for x in arg("--only", "").split(",") if x]


def load(name, path, argv):
    saved = sys.argv; sys.argv = [saved[0]] + argv
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); sys.argv = saved
    return m


E = load("env_bench", os.path.join(HERE, "env.py"), ["--keep"] + [a for a in ARGS if a.startswith("--runs=")])   # (2026-10-14: --runs= goes through, for the mode dlaa's capture)
BV = load("build_variants_", os.path.join(ROOT, "tools", "ffx-own", "build_variants.py"), [])
sys.path.insert(0, HERE)
import devices
devices.narrow(E.C)
W, H, THREE = E.W, E.H, E.THREE
F0, F1 = 8, 15
FR = list(range(F0, F1 + 1))
# a run's ZONE: the car's lower half and its shadow (x, y, width, height in presented pixels)
RUNS = {
    r"cap5\run-01": ("SUNLIT, dry, from behind", (1500, 1350, 1200, 500)), r"cap5\run-02": ("SUNLIT, dry, from behind, the shadow to the left", (1200, 1300, 1500, 700)),
    r"cap5\run-03": ("SUNLIT, dry, from behind, the shadow to the right", (1650, 1250, 1450, 600)), r"cap5\run-04": ("SUNLIT, dry, from behind, the shadow to the left", (1400, 1150, 1300, 700)),
    r"cap5\run-05": ("SUNLIT, dry, from behind, the shadow below", (1300, 1300, 1300, 800)), r"prof5\run-01": ("SUNLIT, dry, three quarters, the tail light", (1500, 1200, 1500, 600)),
    r"prof5\run-02": ("SUNLIT, dry, from the side, close", (1000, 1250, 2000, 700)), r"prof5\run-03": ("SUNLIT, dry, from the side", (1000, 1200, 2000, 700)),
    r"prof5\run-04": ("SUNLIT, dry, from behind, the tail lights", (1400, 1200, 1300, 600)), r"prof5\run-05": ("SUNLIT, dry, three quarters from the other side", (700, 1200, 2100, 650)),
    r"cap3\run-03": ("an older run: SUNLIT, the chase camera, passing shadows (no wheels' key)", (1700, 1300, 1300, 500)),
    r"prof3\run-05": ("an older run: RAIN, the profile, the tail light (no wheels' key)", (2400, 1100, 1000, 500)),
}
CUTS = [("filtered-lookups", "v-interpolation-filtered-lookups"), ("no-scd", "v-no-scene-change-detector"), ("no-inpainting-pyramid", "v-no-inpainting-pyramid"),
        ("guide-cap", "v-guide-cap"), ("search-half", "v-search-half")]
failed = []


# (2026-10-12, 15:13) THE PRODUCT TOOK v-ship's CUTS (BRIEF-ship.md). The three modes of that morning (env, cuts, ship) call "the product"
# the library that had no picture cut: it is THE EXACT LIBRARY now (obj\dlssiv-fg-exact), and they go on reading IT, so each
# still prints what it printed (their labels say "the product", as that round did). "product" is the product of the day.
OLD = "exact"
EXACT = os.path.join(ROOT, "obj", "dlssiv-fg-exact", "dlssiv-fg-exact.dll")
# ("product2" is the product again under another name: the control of the mode "same", the product against itself by two sets of three replays)
def lib(name): return E.PRODUCT if name in ("product", "product2") else EXACT if name == "exact" else BV.path_of(name)


def rep(rel, name, k=0):
    out, counts = E.replay(os.path.join(E.RUNS, rel), rel.replace("\\", "-"), lib(name), name + THREE[k], F0)
    return out


def gen(rel, name, i, k=0): return E.pic(os.path.join(rep(rel, name, k), "f%02d-gen-replay.raw" % i))
def real(rel, i): return E.real(os.path.join(E.RUNS, rel), i)


def lap2(rel, name):
    """the folder of a run's SECOND LAP through a library (fgtime.cpp --lap-reset=0): frames 8 to 15 with the whole run before them"""
    exe = os.path.join(E.OUT, "fgpipe-time.exe"); src = os.path.join(HERE, "fgtime.cpp")
    if not os.path.exists(exe) or os.path.getmtime(exe) < os.path.getmtime(src):
        cl = 'call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 "%s" /Fe:fgpipe-time.exe /Fo:fgpipe-time.obj /link /INCREMENTAL:NO d3d12.lib dxgi.lib bcrypt.lib' % (E.C.vcvars(64), src)
        r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=E.OUT, creationflags=E.NOWIN)
        if r.returncode != 0 or "warning C" in r.stdout: raise SystemExit("fgtime.cpp did not compile: " + r.stdout[-2000:])
    out = os.path.join(E.OUT, rel.replace("\\", "-"), name + "-lap2"); os.makedirs(out, exist_ok=True)
    mark = os.path.join(out, "library.txt"); sha = E.C.sha256_of(lib(name))
    if os.path.isfile(mark) and open(mark).read().strip() == "%s %d" % (sha, F0) and all(os.path.isfile(os.path.join(out, "f%02d-gen-replay.raw" % i)) for i in FR): return out
    for n in os.listdir(out): os.remove(os.path.join(out, n))
    E.wait_card("the second lap of %s through %s" % (rel, name))
    r = subprocess.run([exe, "--run=" + os.path.join(E.RUNS, rel), "--lib=" + lib(name), "--loops=2", "--lap-reset=0", "--out=" + out, "--dump=%d" % F0], capture_output=True, text=True, errors="replace", creationflags=E.NOWIN, timeout=900)
    if "DONE" not in r.stdout: failed.append("%s second lap through %s: %s" % (rel, name, (r.stdout.strip().splitlines() or ["nothing said"])[-2][:160])); return None
    open(mark, "w").write("%s %d\n" % (sha, F0))
    return out


def gen2(rel, name, i): return E.pic(os.path.join(lap2(rel, name), "f%02d-gen-replay.raw" % i))


def shown(a, rect):
    x, y, w, h = rect
    q = E.tone(E.f32(a, y, y + h, x, x + w))
    return 0.2126 * q[..., 0] + 0.7152 * q[..., 1] + 0.0722 * q[..., 2]


def relmap(rel, i, g, rect):
    """the broad difference of one generated frame from the two real frames, over their brightness, a pixel of a rect"""
    x, y, w, h = rect; m = 8; R = (x - m, y - m, w + 2 * m, h + 2 * m)
    B, A, G = [E.lum(E.f32(a, R[1], R[1] + R[3], R[0], R[0] + R[2])) for a in (real(rel, i - 1), real(rel, i), g)]
    d = np.abs(E.boxsum(G - (B + A) / 2, 6) / 169.0)[m:-m, m:-m]
    return d / np.maximum((B + A)[m:-m, m:-m] / 2, 0.02)


def ghostmap(rel, i, g, rect): return relmap(rel, i, g, rect) > 0.5


def ghost(rel, get, rect, frames=FR):
    """pixels in the wrong light a frame over a rect, the mean of the frames"""
    return float(np.mean([ghostmap(rel, i, get(i), rect).sum() for i in frames]))


def broad(rel, get, rect, frames=FR):
    return float(np.mean([relmap(rel, i, get(i), rect).mean() for i in frames]))


def worst_place(rel, get, zone, pw=96, ph=64):
    """the place of the zone with the largest broad difference summed over frames 10 to 15, and the frame it is worst in"""
    x, y, w, h = zone
    maps = {i: np.minimum(relmap(rel, i, get(i), zone), 8.0) for i in range(10, F1 + 1)}
    tot = sum(maps.values())
    best = (-1.0, 0, 0)
    for by in range(0, h - ph + 1, ph // 4):
        for bx in range(0, w - pw + 1, pw // 4):
            s = float(tot[by:by + ph, bx:bx + pw].sum())
            if s > best[0]: best = (s, bx, by)
    s, bx, by = best
    fr = max(maps, key=lambda i: float(maps[i][by:by + ph, bx:bx + pw].sum()))
    return (x + bx, y + by, pw, ph), fr


def three(vals, fmt): return E.three(vals, fmt)


def steady(rel, a3, b3, i):
    s = np.ones((H, W), dtype=bool)
    for names in (a3, b3):
        s &= ~E.differ(gen(rel, names[0], i, 0), gen(rel, names[0], i, 1)) & ~E.differ(gen(rel, names[0], i, 0), gen(rel, names[0], i, 2))
    return s


def changed(rel, a, b, rect=None, frames=(10, 12, 15)):
    """b against a, pixels the same in three replays of each and not the same in both: percent by 1, 4, 16 codes, the most; the mean of the frames"""
    x, y, w, h = rect or (0, 0, W, H); out = []
    for i in frames:
        c = E.codes(gen(rel, a, i), gen(rel, b, i), rect) * steady(rel, [a], [b], i)[y:y + h, x:x + w]
        out.append((100.0 * (c >= 1).mean(), 100.0 * (c >= 4).mean(), 100.0 * (c >= 16).mean(), float(c.max())))
    return tuple(float(np.mean([o[k] for o in out])) for k in range(3)) + (max(o[3] for o in out),)


def tiles_sheet(tiles, title, path, cols):
    tw, th = tiles[0][0].size; rows = (len(tiles) + cols - 1) // cols
    big, head = E.font(22), E.font(20)
    im = Image.new("RGB", (tw * cols + 4 * (cols - 1), th * rows + 4 * (rows - 1) + 44), (20, 20, 20))
    d = ImageDraw.Draw(im); d.text((10, 7), title, fill=(255, 255, 255), font=head)
    for n, (t, text, colour) in enumerate(tiles):
        x, y = (n % cols) * (tw + 4), 44 + (n // cols) * (th + 4)
        im.paste(t, (x, y)); E.label(d, x + 10, y + th - 42, text, colour, big)
    im.save(path)


def around(place, w, h):
    x, y, pw, ph = place
    return (int(min(max(x + pw // 2 - w // 2, 0), W - w)), int(min(max(y + ph // 2 - h // 2, 0), H - h)), w, h)


WHITE, RED, GREEN, YELLOW = (255, 255, 255), (255, 120, 120), (120, 255, 120), (255, 220, 60)


def flip(rel, rect, left, right, lname, rname, title, base):
    x, y, w, h = rect; frames = []
    for k in FR:
        for kind in ("GENERATED", "REAL"):
            a = E.tile(left(k) if kind == "GENERATED" else real(rel, k), rect, 1.0); b = E.tile(right(k) if kind == "GENERATED" else real(rel, k), rect, 1.0)
            im = Image.new("RGB", (2 * w + 6, h + 40), (20, 20, 20)); d = ImageDraw.Draw(im)
            d.text((8, 8), title, fill=WHITE, font=E.font(18)); im.paste(a, (0, 40)); im.paste(b, (w + 6, 40))
            col = YELLOW if kind == "GENERATED" else WHITE
            E.label(d, 10, 40 + h - 30, "%s: %s %d" % (lname, kind, k), col, E.font(18)); E.label(d, w + 16, 40 + h - 30, "%s: %s %d" % (rname, kind, k), col, E.font(18))
            frames.append(im)
    pal = [f.quantize(colors=255, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG) for f in frames]
    pal[0].save(base + ".gif", save_all=True, append_images=pal[1:], duration=400, loop=0)


def runs_of(names):
    r = [n for n in names if (not ONLY or n in ONLY or n.replace("\\", "-") in ONLY) and os.path.isfile(os.path.join(E.RUNS, n, "index.txt"))]
    if not r: raise SystemExit("FAIL: no run of this mode under %s" % E.RUNS)
    return r


def head(libs):
    for n in libs: print("  %-22s %d bytes, SHA-256 %s" % (n, os.path.getsize(lib(n)), E.C.sha256_of(lib(n))))


def mode_env():
    """part 1: what is left with v-env-1, and what the new text and each of its mends take away"""
    less = ["env-less-creep", "env-less-apart", "env-less-moved"]
    head([OLD, "env-1", "env"] + less)
    for rel in runs_of(list(RUNS)):
        what, zone = RUNS[rel]; tag = rel.replace("\\", "-")
        print("\n" + "=" * 110 + "\n%s: %s; the zone %d,%d,%d,%d" % ((rel, what) + zone))
        for n in (OLD, "env"):
            for k in range(3): rep(rel, n, k)
        for n in ["env-1"] + less: rep(rel, n)
        lap = {n: lap2(rel, n) for n in ("env-1", "env")}
        if any(f.startswith(tag + " ") or f.startswith(rel) for f in failed): print("  A REPLAY DID NOT RUN: this run is left out"); continue
        P3 = lambda k: (lambda i: gen(rel, OLD, i, k)); V3 = lambda k: (lambda i: gen(rel, "env", i, k))
        one = lambda n: (lambda i: gen(rel, n, i)); two = lambda n: (lambda i: gen2(rel, n, i))
        place1, fr1 = worst_place(rel, two("env-1"), zone); place2, fr2 = worst_place(rel, V3(0), zone)
        rows = [("the product", [P3(k) for k in range(3)]), ("v-env-1 from the reset", [one("env-1")]), ("v-env-1, THE SECOND LAP (the game)", [two("env-1")]),
                ("v-env (new)", [V3(k) for k in range(3)]), ("v-env (new), the second lap", [two("env")])] + [("v-" + n, [one(n)]) for n in less]
        print("  PIXELS IN THE WRONG LIGHT A FRAME (frames 8 to 15; then 10 to 15) and the broad difference: in the zone | at the worst place of v-env-1 in the game %d,%d,%d,%d | at the worst place of the new v-env %d,%d,%d,%d" % (place1 + place2))
        for name, gets in rows:
            cells = []
            for rect in (zone, place1, place2):
                cells.append("%s (%s) %s" % (three([ghost(rel, g, rect) for g in gets], "%.0f"), three([ghost(rel, g, rect, range(10, F1 + 1)) for g in gets], "%.0f"), three([broad(rel, g, rect) for g in gets], "%.3f")))
            print("      %-36s %-30s | %-26s | %s" % (name, cells[0], cells[1], cells[2]))
        c = changed(rel, OLD, "env"); c1 = E.codes(gen(rel, OLD, 15), gen(rel, "env-1", 15))
        print("  AGAINST THE PRODUCT, the whole frame, percent of the steady pixels that change by 1, 4, 16 codes (frames 10, 12 and 15): v-env %.2f %.2f %.2f; v-env-1 (one replay of each, frame 15, the race in it) %.2f %.2f %.2f" % (
            c[:3] + (100.0 * (c1 >= 1).mean(), 100.0 * (c1 >= 4).mean(), 100.0 * (c1 >= 16).mean())))
        if rel in E.BENCH:
            run = os.path.join(E.RUNS, rel)
            for what2, x0, x1, y0, y1 in E.BENCH[rel].get("bands", []):
                print("  %s, generated over real, the mean, frames 8 to 15:" % what2)
                for name, get in (("the product", P3(0)), ("v-env-1", one("env-1")), ("v-env (new)", V3(0))):
                    print("      %-14s %s" % (name, " ".join("%.2f" % E.band(run, k, get(k), x0, x1, y0, y1)[1] for k in FR)))
            for rname, rect in E.BENCH[rel].get("road", []):
                c = changed(rel, OLD, "env", rect)
                print("  v-env against the product, %s %d,%d,%d,%d: %.2f %.2f %.2f percent by 1, 4, 16 codes, the most %d" % ((rname,) + rect + c[:3] + (int(c[3]),)))
        if "--no-pictures" in ARGS: continue
        for pname, place, fr in (("left-this-morning", place1, fr1), ("left-now", place2, fr2)):
            rect = around(place, 300, 170); sc = 2.0
            t = lambda a: E.tile(a, rect, sc)
            tiles = [(t(real(rel, fr - 1)), "1  REAL FRAME BEFORE", WHITE), (t(gen(rel, OLD, fr)), "2  THE PRODUCT", RED), (t(gen2(rel, "env-1", fr)), "3  v-env AS IN THE GAME TODAY", YELLOW),
                     (t(gen(rel, "env", fr)), "4  THE NEW v-env", GREEN), (t(real(rel, fr)), "5  REAL FRAME AFTER", WHITE), (t(gen(rel, "env-less-creep", fr)), "6  the new one LESS mend 3", (200, 200, 200))]
            p = os.path.join(SHOW, "%s-f%02d-%s.png" % (tag, fr, pname))
            tiles_sheet(tiles, "%s, generated frame %d, at %d,%d (%d by %d)" % ((rel, fr) + rect), p, 3)
            print("  sheet: %s" % p)
        frect = around(place1, 520, 300)
        base = os.path.join(SHOW, "%s-flip-env-1-against-env" % tag)
        flip(rel, frect, two("env-1"), V3(0), "v-env AS IN THE GAME TODAY", "THE NEW v-env", "%s: frames 8 to 15 as shown, at %d,%d" % ((rel,) + frect[:2]), base)
        print("  flip: %s.gif" % base)


def mode_cuts():
    """part 4: each picture cut alone on top of v-env, against v-env"""
    names = ["env-" + c for c, _ in CUTS]
    head(["env"] + names)
    runs = runs_of([r"cap5\run-01", r"cap5\run-04", r"prof5\run-02", r"prof5\run-05", r"cap3\run-03", r"prof3\run-05"])
    for rel in runs:
        what, zone = RUNS[rel]; tag = rel.replace("\\", "-")
        print("\n" + "=" * 110 + "\n%s: %s" % (rel, what))
        for n in ["env"] + names:
            for k in range(3): rep(rel, n, k)
        base3 = [(lambda k: (lambda i: gen(rel, "env", i, k)))(k) for k in range(3)]
        print("      %-26s pixels in the wrong light a frame in the zone %s, the broad difference %s" % ("v-env", three([ghost(rel, g, zone) for g in base3], "%.0f"), three([broad(rel, g, zone) for g in base3], "%.4f")))
        for (cut, _), n in zip(CUTS, names):
            cut3 = [(lambda k: (lambda i: gen(rel, n, i, k)))(k) for k in range(3)]
            c = changed(rel, "env", n); cz = changed(rel, "env", n, zone); edge = changed(rel, "env", n, (W - 240, 0, 240, H))
            print("      %-26s against v-env, percent of the steady pixels by 1, 4, 16 codes and the most: the whole frame %.3f %.3f %.3f, %d | the zone %.3f %.3f %.3f, %d | the screen's right edge (240 pixels) %.3f %.3f %.3f, %d | pixels in the wrong light in the zone %s, the broad difference %s" % (
                (cut,) + c[:3] + (int(c[3]),) + cz[:3] + (int(cz[3]),) + edge[:3] + (int(edge[3]),) + (three([ghost(rel, g, zone) for g in cut3], "%.0f"), three([broad(rel, g, zone) for g in cut3], "%.4f"))))
            if "--no-pictures" in ARGS or c[0] == 0.0: continue
            # the worst place: the 96 by 64 pixels where the steady pixels change most (frame 15), and a sheet round it
            d = E.codes(gen(rel, "env", 15), gen(rel, n, 15)) * steady(rel, ["env"], [n], 15)
            s = E.boxsum(np.minimum(d, 64.0), 0).reshape(H // 8, 8, W // 8, 8).sum(axis=(1, 3))
            s2 = s[:-7, :-11] * 0
            for dy in range(8):
                for dx in range(12): s2 = s2 + s[dy:dy + s2.shape[0], dx:dx + s2.shape[1]]
            by, bx = np.unravel_index(int(np.argmax(s2)), s2.shape)
            rect = around((bx * 8, by * 8, 96, 64), 300, 170); t = lambda a: E.tile(a, rect, 2.0)
            x, y, w, h = rect
            dd = np.clip(E.codes(gen(rel, "env", 15), gen(rel, n, 15), rect) * 8.0, 0, 255).astype(np.uint8)
            tiles = [(t(real(rel, 14)), "1  REAL FRAME BEFORE", WHITE), (t(gen(rel, "env", 15)), "2  v-env", GREEN), (t(gen(rel, n, 15)), "3  v-env AND THE CUT", YELLOW), (t(real(rel, 15)), "4  REAL FRAME AFTER", WHITE),
                     (Image.fromarray(dd, "L").convert("RGB").resize(t(real(rel, 15)).size, Image.NEAREST), "5  WHAT THE CUT CHANGED, times 8", (200, 200, 200)), (t(gen(rel, OLD, 15)), "6  THE PRODUCT", RED)]
            p = os.path.join(SHOW, "cut-%s-%s-f15-worst.png" % (cut, tag))
            tiles_sheet(tiles, "%s on %s, generated frame 15, where it changes most: %d,%d (%d by %d)" % ((cut, rel) + rect), p, 3)
            print("          sheet: %s" % p)


def mode_ship():
    """part 5: the candidate v-ship beside the product and v-env"""
    head([OLD, "env", "ship"])
    for rel in runs_of(list(RUNS)):
        what, zone = RUNS[rel]; tag = rel.replace("\\", "-")
        print("\n" + "=" * 110 + "\n%s: %s; the zone %d,%d,%d,%d" % ((rel, what) + zone))
        for n in (OLD, "env", "ship"):
            for k in range(3): rep(rel, n, k)
        lap2(rel, "ship")
        G3 = lambda n: [(lambda k: (lambda i: gen(rel, n, i, k)))(k) for k in range(3)]
        place, fr = worst_place(rel, G3("ship")[0], zone)
        print("  PIXELS IN THE WRONG LIGHT A FRAME (frames 8 to 15; then 10 to 15) in the zone | at the worst place of v-ship %d,%d,%d,%d | the broad difference over the zone" % place)
        for name, gets in (("the product", G3(OLD)), ("v-env", G3("env")), ("v-ship", G3("ship")), ("v-ship, the second lap", [lambda i: gen2(rel, "ship", i)])):
            print("      %-26s %s (%s) | %s (%s) | %s" % (name, three([ghost(rel, g, zone) for g in gets], "%.0f"), three([ghost(rel, g, zone, range(10, F1 + 1)) for g in gets], "%.0f"),
                                                          three([ghost(rel, g, place) for g in gets], "%.0f"), three([ghost(rel, g, place, range(10, F1 + 1)) for g in gets], "%.0f"), three([broad(rel, g, zone) for g in gets], "%.4f")))
        c = changed(rel, OLD, "ship"); ce = changed(rel, "env", "ship")
        print("  v-ship, percent of the steady pixels that change by 1, 4, 16 codes (frames 10, 12 and 15), the whole frame: against the product %.2f %.2f %.2f; against v-env %.3f %.3f %.3f, the most %d" % (c[:3] + ce[:3] + (int(ce[3]),)))
        if rel in E.BENCH:
            run = os.path.join(E.RUNS, rel)
            for what2, x0, x1, y0, y1 in E.BENCH[rel].get("bands", []):
                print("  %s, generated over real, the mean, frames 8 to 15:" % what2)
                for name in (OLD, "env", "ship"):
                    print("      %-14s %s" % (name, " ".join("%.2f" % E.band(run, k, gen(rel, name, k), x0, x1, y0, y1)[1] for k in FR)))
        if "--no-pictures" in ARGS: continue
        x, y, w, h = zone
        for pname, rect, sc, f in (("car", (x + w // 2 - 480, y + h // 2 - 270, 960, 540), 0.67, 15), ("left", around(place, 300, 170), 2.0, fr)):
            t = lambda a: E.tile(a, rect, sc)
            tiles = [(t(real(rel, f - 1)), "1  REAL FRAME BEFORE", WHITE), (t(gen(rel, OLD, f)), "2  GENERATED TODAY (the product)", RED), (t(gen(rel, "ship", f)), "3  THE CANDIDATE (v-ship)", GREEN), (t(real(rel, f)), "4  REAL FRAME AFTER", WHITE)]
            p = os.path.join(SHOW, "ship-%s-f%02d-%s.png" % (tag, f, pname))
            tiles_sheet(tiles, "%s, generated frame %d, at %d,%d (%d by %d)" % ((rel, f) + rect), p, 2)
            print("  sheet: %s" % p)
        frect = (int(min(max(x + w // 2 - 260, 0), W - 520)), int(min(max(y + h // 2 - 100, 0), H - 300)), 520, 300)
        base = os.path.join(SHOW, "ship-%s-flip-f08-f15" % tag)
        flip(rel, frect, lambda i: gen(rel, OLD, i), lambda i: gen(rel, "ship", i), "TODAY (the product)", "THE CANDIDATE (v-ship)", "%s: frames 8 to 15 as shown, at %d,%d" % ((rel,) + frect[:2]), base)
        print("  flip: %s.gif" % base)


def mode_same():
    """THE PRODUCT'S PICTURES ARE ANOTHER LIBRARY'S, BIT FOR BIT, on the captured runs, by the three-replay rule"""
    other = arg("--other", "ship")
    head(["product", other])
    bad = 0; total = 0; frames = 0; within = [0, 0]; across = 0; whole = 0
    for rel in runs_of([r for r in RUNS if r.startswith(("cap5", "prof5"))]):
        print("\n" + "=" * 110 + "\n%s: %s" % (rel, RUNS[rel][0]))
        for n in ("product", other):
            for k in range(3): rep(rel, n, k)
        if any(f.startswith(rel.replace("\\", "-") + " ") for f in failed + E.failed): print("  A REPLAY DID NOT RUN: this run is left out"); bad += 1; continue
        row = []
        for i in FR:
            P = [gen(rel, "product", i, k) for k in range(3)]; S = [gen(rel, other, i, k) for k in range(3)]
            sp = ~E.differ(P[0], P[1]) & ~E.differ(P[0], P[2]); ss = ~E.differ(S[0], S[1]) & ~E.differ(S[0], S[2])
            d = E.differ(P[0], S[0])
            both = sp & ss
            n = int((d & both).sum())
            same_whole = sum(1 for a in P for b in S if not E.differ(a, b).any())
            row.append((i, n, int((~sp).sum()), int((~ss).sum()), int(d.sum()), same_whole))
            total += n; frames += 1; within[0] += int((~sp).sum()); within[1] += int((~ss).sum()); across += int(d.sum()); whole += 1 if same_whole else 0
        print("  frame: pixels that are the same in three replays of each library AND DIFFER BETWEEN THE TWO | not the same in three replays of the product | of v-%s | one replay of the product against one of v-%s | pairs of replays (of 9) that are the same whole image" % (other, other))
        for i, n, a, b, c, w in row: print("      %02d: %d | %d | %d | %d | %d" % (i, n, a, b, c, w))
        if sum(r[1] for r in row): bad += 1
        if "--keep-replays" not in ARGS:                 # two more replays a library a run are a gigabyte each: read, then gone
            import shutil
            for n in ("product", other):
                for k in (1, 2): shutil.rmtree(os.path.join(E.OUT, rel.replace("\\", "-"), n + THREE[k]), ignore_errors=True)
    print("\nOVER %d FRAMES OF %s (generated frames %d to %d, 8,294,400 pixels each): %d pixel(s) that are steady in three replays of each library differ between the product and v-%s; pixels that are not steady in three replays: the product %d, v-%s %d (the race of AMD's secondary vectors, each library against itself); one replay against one: %d differ; in %d of the %d frames a replay of the product is the same whole image as a replay of v-%s" % (
        frames, "cap5 and prof5", F0, F1, total, other, within[0], other, within[1], across, whole, frames, other))
    if bad or total or not frames: failed.append("the product's pictures are NOT v-%s's: %d steady pixel(s) differ, %d run(s) bad" % (other, total, bad))
    else: print("THE PRODUCT'S PICTURES ARE v-%s's, BIT FOR BIT, wherever a picture is the same from replay to replay" % other)


def mode_dlaa():
    """(2026-10-14, G) the guides capped at Quality's size, on the product, against the product, on a capture at DLAA"""
    var = "guide-cap-product"
    head(["product", var])
    found = []
    for d1 in sorted(os.listdir(E.RUNS)):
        p1 = os.path.join(E.RUNS, d1)
        if os.path.isfile(os.path.join(p1, "index.txt")): found.append(d1)
        elif os.path.isdir(p1): found += [os.path.join(d1, d2) for d2 in sorted(os.listdir(p1)) if os.path.isfile(os.path.join(p1, d2, "index.txt"))]
    runs = []
    for rel in runs_of(found):
        idx = dict(l.split(": ", 1) for l in open(os.path.join(E.RUNS, rel, "index.txt"), encoding="utf-8", errors="replace").read().splitlines() if ": " in l)
        # (--smoke: THE INSTRUMENT ON A RUN AT QUALITY, where the cap changes nothing: every line of this mode runs and the change must read 0)
        if (idx.get("render") != idx.get("display") and "--smoke" not in ARGS) or idx.get("display") != "%dx%d" % (W, H):
            print("  %s: NOT A RUN AT DLAA (display %s, render %s): the cap changes nothing there, left out" % (rel, idx.get("display"), idx.get("render"))); continue
        runs.append(rel)
    if not runs: failed.append("no run at DLAA under %s (the kit's F1C capa makes them)" % E.RUNS); return
    whole = (64, 64, W - 128, H - 128)                   # (the frame less a border of 64 pixels: the broad mean and the map need room round a pixel)
    for rel in runs:
        tag = rel.replace("\\", "-")
        print("\n" + "=" * 110 + "\n%s: the guides at the display's size (DLAA)" % rel)
        for n in ("product", var):
            for k in range(3): rep(rel, n, k)
            lap2(rel, n)
        if any(f.startswith(tag + " ") or f.startswith(rel) for f in failed + E.failed): print("  A REPLAY DID NOT RUN: this run is left out"); continue
        G3 = lambda n: [(lambda k: (lambda i: gen(rel, n, i, k)))(k) for k in range(3)]
        place, fr = worst_place(rel, G3(var)[0], (W // 8, H // 8, 3 * W // 4, 3 * H // 4))
        print("  PIXELS IN THE WRONG LIGHT A FRAME (frames 8 to 15; then 10 to 15) over the frame less a 64-pixel border | at the worst place of the capped one %d,%d,%d,%d | the broad difference over the same" % place)
        for name, gets in (("the product", G3("product")), ("the guides capped", G3(var)), ("the product, the second lap", [lambda i: gen2(rel, "product", i)]), ("the guides capped, the second lap", [lambda i: gen2(rel, var, i)])):
            print("      %-34s %s (%s) | %s (%s) | %s" % (name, three([ghost(rel, g, whole) for g in gets], "%.0f"), three([ghost(rel, g, whole, range(10, F1 + 1)) for g in gets], "%.0f"),
                                                          three([ghost(rel, g, place) for g in gets], "%.0f"), three([ghost(rel, g, place, range(10, F1 + 1)) for g in gets], "%.0f"), three([broad(rel, g, whole) for g in gets], "%.4f")))
        c = changed(rel, "product", var); edge = changed(rel, "product", var, (W - 240, 0, 240, H))
        print("  the guides capped against the product, percent of the steady pixels by 1, 4, 16 codes and the most (frames 10, 12 and 15): the whole frame %.3f %.3f %.3f, %d | the screen's right edge (240 pixels) %.3f %.3f %.3f, %d" % (c[:3] + (int(c[3]),) + edge[:3] + (int(edge[3]),)))
        if "--no-pictures" in ARGS: continue
        d = E.codes(gen(rel, "product", 15), gen(rel, var, 15)) * steady(rel, ["product"], [var], 15)
        s = E.boxsum(np.minimum(d, 64.0), 0).reshape(H // 8, 8, W // 8, 8).sum(axis=(1, 3))
        s2 = s[:-7, :-11] * 0
        for dy in range(8):
            for dx in range(12): s2 = s2 + s[dy:dy + s2.shape[0], dx:dx + s2.shape[1]]
        by, bx = np.unravel_index(int(np.argmax(s2)), s2.shape)
        for pname, rect, sc, f in (("most", around((bx * 8, by * 8, 96, 64), 300, 170), 2.0, 15), ("left", around(place, 300, 170), 2.0, fr)):
            t = lambda a: E.tile(a, rect, sc)
            tiles = [(t(real(rel, f - 1)), "1  REAL FRAME BEFORE", WHITE), (t(gen(rel, "product", f)), "2  THE PRODUCT", RED), (t(gen(rel, var, f)), "3  THE GUIDES CAPPED", GREEN), (t(real(rel, f)), "4  REAL FRAME AFTER", WHITE)]
            p = os.path.join(SHOW, "dlaa-cap-%s-f%02d-%s.png" % (tag, f, pname))
            tiles_sheet(tiles, "%s at DLAA, generated frame %d, at %d,%d (%d by %d)" % ((rel, f) + rect), p, 2)
            print("  sheet: %s" % p)
        frect = around(place, 520, 300)
        base = os.path.join(SHOW, "dlaa-cap-%s-flip-f08-f15" % tag)
        flip(rel, frect, lambda i: gen2(rel, "product", i), lambda i: gen2(rel, var, i), "THE PRODUCT (as in the game)", "THE GUIDES CAPPED (as in the game)", "%s at DLAA: frames 8 to 15 as shown, at %d,%d" % ((rel,) + frect[:2]), base)
        print("  flip: %s.gif" % base)


def main():
    if MODE not in ("env", "cuts", "ship", "same", "dlaa"):
        print(__doc__); return 2
    os.makedirs(SHOW, exist_ok=True); os.makedirs(E.OUT, exist_ok=True)
    sys.stdout = E.C.Tee(os.path.join(SHOW, "result-%s.txt" % MODE))
    t0 = time.time()
    if not os.path.exists(E.REPLAY) and not E.build_replay():
        print("FAIL: the replay did not compile"); return 1
    print("THE LIBRARY'S LAST ROUND ON THE CAPTURED RUNS under %s (mode %s): generated frames %d to %d, each run replayed from its reset; the three-replay rule; the sheets go to %s" % (E.RUNS, MODE, F0, F1, SHOW))
    {"env": mode_env, "cuts": mode_cuts, "ship": mode_ship, "same": mode_same, "dlaa": mode_dlaa}[MODE]()
    failed.extend(f for f in E.failed if f not in failed)
    for f in failed: print("  failed: " + f)
    if MODE == "same": print("%s, in %.0f s" % ("PASS" if not failed else "FAIL", time.time() - t0))
    else: print("%s, in %.0f s" % ("DONE" if not failed else "FAIL (%d replay(s) did not run)" % len(failed), time.time() - t0))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
