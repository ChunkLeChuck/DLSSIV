r"""THE GHOST VARIANT ON THE OWNER'S CAPTURED FRAMES: THE NUMBERS, THE STRIPS, THE GATE (2026-10-11;
docs\reports\fg-3\sitting-2026-10-11b\BRIEF-ghost.md; the variant is ffx-own\cuts\v-ghost.cut, built by
tools\ffx-own\build_variants.py ghost ghost-gate into obj\dlssiv-fg-var).

Behind a thing that stands still on the screen over a fast background the generated frame shows a second, faint
outline of it (READING.md of that folder, 5b to 5e). The variant library "ghost" is the fix for the owner's eye; this
script puts beside it what a judgement needs, on the captured runs (host\fg_cap.h; tools\fg-replay makes a run's
generated frames again through any library):

  THE BENCH IS SOUND?   each run replayed through THE PRODUCT (twice) and THE REFERENCE (AMD's source with no cut): how
                        many pixels of a steady generated frame differ from the saved one, and between two replays of
                        one library. They are NOT byte-equal on the game's frames: AMD's method races with itself
                        (said with the counts; the variant's own changes are held against that floor)
  THE NUMBERS           for the saved frame, the product, the reference and the variant:
                        (a) the band under the car in the chase camera (stills\band.py's ratio, generated over real
                            frame after, a row a line, its least, mean and most over the band's rows);
                        (b) the strip behind the car in the profile view (stills\bandx.py's row: brightness over the
                            real frame after's, and red over green, in the strip's two halves);
                        (c) stills\mock3.py's "broad difference from the real frames" over its two crops
  THE STRIPS            a PNG a crop: the real frame before, THE GENERATED FRAME BY THE PRODUCT, BY THE VARIANT, the real
                        frame after; and the whole frame's difference, variant against product (a quarter of the
                        size, times 8), of the pixels that are the same in two replays of each library
  THE GATE              the instrument library "ghost-gate" paints every pixel of the variant's own path one flat
                        colour: how many pixels the variant changed, how many of them lie outside that map, how far
                        from it at most, and how many pixels two replays of the product differ in by themselves

    python tools\test-fg-own\ghost.py [--runs=<folder>] [--out=<folder>] [--only=<run>,<run>] [--frames=13-15]
        --runs   the folder of the captured runs (not given: %TEMP%\dlssiv-fg-runs); a run is a folder with index.txt
        --out    where the strips go (not given: docs\reports\fg-3\sitting-2026-10-11b\ghost)
        --keep   a replay that lies in this tool's folder for the same library (by its SHA-256) is read and not made again
NOT A PROOF AND NOT A JUDGE: THE PICTURE IS THE OWNER'S TO JUDGE. It takes the card for some minutes: ONE GPU USER AT A
TIME, never beside the game. Its own folder is %TEMP%\dlssiv-test-fg-ghost (the replay is built there as
fgpipe-replay.exe, a name every proof's wait for the card sees); nothing is written into a captured run. The last line
is DONE, or FAIL where a replay did not run.
"""
import ctypes, importlib.util, os, re, subprocess, sys, tempfile, time

import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
NOWIN = subprocess.CREATE_NO_WINDOW
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
RUNS = arg("--runs", os.path.join(tempfile.gettempdir(), "dlssiv-fg-runs"))
SHOW = arg("--out", os.path.join(ROOT, "docs", "reports", "fg-3", "sitting-2026-10-11b", "ghost"))
ONLY = [x for x in arg("--only", "").split(",") if x]
F0, F1 = [int(v) for v in arg("--frames", "13-15").split("-")]
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-ghost")
REPLAY = os.path.join(OUT, "fgpipe-replay.exe")
LIBS = [("product", os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll")),
        ("product-again", os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll")),
        ("reference", os.path.join(ROOT, "obj", "dlssiv-fg-ref", "dlssiv-fg-ref.dll")),
        ("ghost", os.path.join(ROOT, "obj", "dlssiv-fg-var", "ghost", "dlssiv-fg-v-ghost.dll")),
        ("ghost-again", os.path.join(ROOT, "obj", "dlssiv-fg-var", "ghost", "dlssiv-fg-v-ghost.dll")),
        ("ghost-gate", os.path.join(ROOT, "obj", "dlssiv-fg-var", "ghost-gate", "dlssiv-fg-v-ghost-gate.dll")),
        ("alone", os.path.join(ROOT, "obj", "dlssiv-fg-var", "alone", "dlssiv-fg-v-alone.dll")),
        ("ghost-alone", os.path.join(ROOT, "obj", "dlssiv-fg-var", "ghost-alone", "dlssiv-fg-v-ghost-alone.dll"))]
# the two libraries with nothing inpainted are replayed THREE times each: AMD's method races with itself (two replays of one
# library differ in some pixels), so "the variant changed this pixel" is said only of a pixel that is the same in all
# three replays of the one, the same in all three of the other, and not the same in both
LIBS += [(n + k, p) for n, p in LIBS[-2:] for k in ("-again", "-third")]
# THE CROPS, by a run's path under --runs (x, y, width, height in presented pixels), and which numbers are taken there.
# The first of prof3\run-05 and of cap3\run-03 are stills\mock3.py's own (the second's was not written down: this one
# reads the saved frame's 0.0061 where READING.md says 0.0062).
BENCH = {
    r"prof3\run-05": {"crops": [("back", (2620, 1130, 370, 400)), ("front", (1480, 1150, 370, 400))], "broad": "back",
                      "bandx": [("the strip, the first half step behind the car", 1330, 1350, 2858, 2918), ("the half step beyond it", 1330, 1350, 2924, 2984)]},
    r"prof3\run-01": {"crops": [("back", (1150, 1130, 370, 400)), ("front", (2330, 1100, 370, 400))], "broad": "back"},
    r"cap3\run-03": {"crops": [("under", (1810, 1340, 370, 300)), ("band", (1900, 1400, 560, 300)), ("pole", (2800, 1000, 370, 400))], "broad": "under",
                     "band": [("columns 1960 to 2140", 1960, 2140, 1520, 1552), ("columns 2200 to 2400", 2200, 2400, 1528, 1570)]},
    r"cap3\run-01": {"crops": [("back", (1250, 1200, 400, 400)), ("barrels", (620, 950, 400, 400))], "broad": "back"},
    r"cap2\run-01": {"crops": [("back", (1380, 1230, 400, 400)), ("rail", (100, 950, 400, 400))], "broad": "back"},
    r"cap2\run-03": {"crops": [("back", (2100, 1420, 400, 330)), ("pole", (3050, 1100, 370, 450))], "broad": "back"},
}
W, H = 3840, 2160
failed = []


def load_cost():
    saved = sys.argv
    sys.argv = [saved[0], "--keep"]
    spec = importlib.util.spec_from_file_location("fgcost_bench", os.path.join(ROOT, "tools", "test-fg-cost", "run.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


C = load_cost()
C.OUT = OUT
C.GPU_USERS = ("dlssiv-helper.exe", "fgpipe*.exe", "fgcost*.exe", "vklines*.exe", "fgreplay*.exe")


def wait_card(what):
    waited = 0
    while (C.game_runs() or C.other_gpu_users()) and waited < 3600:
        if waited == 0: print("  the card is somebody else's (%s): %s waits, looking again every 5 s..." % ("the game" if C.game_runs() else "; ".join(C.other_gpu_users())[:160], what), flush=True)
        time.sleep(5); waited += 5
    if waited: print("  the card is free after %d s" % waited, flush=True)


def build_replay():
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 "%s" /Fe:fgpipe-replay.exe /Fo:fgpipe-replay.obj /link /INCREMENTAL:NO d3d12.lib dxgi.lib bcrypt.lib'
          % (C.vcvars(64), os.path.join(ROOT, "tools", "fg-replay", "fgreplay.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    return r.returncode == 0 and "warning C" not in r.stdout and os.path.exists(REPLAY)


def replay(run, tag, lib, name):
    out = os.path.join(OUT, tag, name)
    os.makedirs(out, exist_ok=True)
    mark = os.path.join(out, "library.txt"); said = os.path.join(out, "replay.txt"); sha = C.sha256_of(lib)
    have = "--keep" in ARGS and os.path.isfile(mark) and open(mark).read().strip() == "%s %d" % (sha, F0) and os.path.isfile(said) and all(os.path.isfile(os.path.join(out, "f%02d-gen-replay.raw" % i)) for i in range(F0, F1 + 1))
    if have: text = open(said, encoding="utf-8", errors="replace").read()
    else:
        for n in os.listdir(out): os.remove(os.path.join(out, n))
        wait_card("the replay of %s through %s" % (tag, name))
        r = subprocess.run([REPLAY, "--run=" + run, "--lib=" + lib, "--out=" + out, "--expect=differ", "--dump=%d" % F0], capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=1800)
        text = r.stdout
        open(mark, "w").write("%s %d\n" % (sha, F0))
    class r: stdout = text
    fr = [l for l in r.stdout.splitlines() if l.startswith("frame ")]
    counts = {}
    for l in fr:
        m = re.match(r"frame (\d+) .*: (EQUAL|DIFFERS: (\d+) of)", l)
        if m: counts[int(m.group(1))] = 0 if m.group(2) == "EQUAL" else int(m.group(3))
    ok = len(counts) >= F1 + 1 and all(os.path.isfile(os.path.join(out, "f%02d-gen-replay.raw" % i)) for i in range(F0, F1 + 1))
    if not ok: failed.append("%s through %s: %s" % (tag, name, (r.stdout.strip().splitlines() or ["nothing said"])[-1][:200]))
    return out, counts


def pic(path): return np.memmap(path, dtype=np.float16, mode="r").reshape(H, W, 4)
def lum(c): return 0.2126 * c[..., 0] + 0.7152 * c[..., 1] + 0.0722 * c[..., 2]
def f32(a, y0, y1, x0, x1): return np.asarray(a[y0:y1, x0:x1, :3], dtype=np.float32)


def tone(q):
    q = np.clip(q, 0, None); t = np.minimum(q * (1 + q / 156.25) / (1 + q), 1.0)
    s = np.where(t <= 0.0031308, 12.92 * t, 1.055 * np.power(np.clip(t, 1e-9, None), 1 / 2.4) - 0.055)
    return np.clip(s, 0, 1) * 255


def boxsum(q, r):
    c = np.cumsum(np.cumsum(np.pad(q, ((r + 1, r), (r + 1, r))), axis=0), axis=1); k = 2 * r + 1
    return c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]


def broad(run, i, gen, rect, rw, rh):
    """stills\\mock3.py's number: the mean and the 99th percentile, over the crop's fast background, of the 13 by 13 mean of
    the generated frame's brightness less the two real frames' mean brightness"""
    x, y, w, h = rect; m = 16
    B, A, G = [lum(f32(a, y - m, y + h + m, x - m, x + w + m)) for a in (pic(os.path.join(run, "f%02d-present.raw" % (i - 1))), pic(os.path.join(run, "f%02d-present.raw" % i)), gen)]
    mv = np.fromfile(os.path.join(run, "f%02d-mv.raw" % i), dtype=np.float16).reshape(rh, rw, 2).astype(np.float32)
    ys = (np.arange(y, y + h) * rh // H)[:, None]; xs = (np.arange(x, x + w) * rw // W)[None, :]
    ground = np.hypot(mv[ys, xs, 0] * W / rw, mv[ys, xs, 1] * H / rh) > 8.0
    d = np.abs(boxsum(G - (B + A) / 2, 6) / 169.0)[m:-m, m:-m]
    return (float(d[ground].mean()), float(np.percentile(d[ground], 99))) if ground.any() else (float("nan"), float("nan"))


def band(run, i, gen, x0, x1, y0, y1):
    """stills\\band.py's last column but one, over the band's rows: the generated frame's brightness over the real frame
    after's, each the mean of the columns, a row a line: (least, mean, most)"""
    A = lum(f32(pic(os.path.join(run, "f%02d-present.raw" % i)), y0, y1, x0, x1)).mean(axis=1)
    G = lum(f32(gen, y0, y1, x0, x1)).mean(axis=1)
    r = G / np.maximum(A, 1e-6)
    return float(r.min()), float(r.mean()), float(r.max())


def bandx(run, i, gen, y0, y1, x0, x1):
    """stills\\bandx.py's row over a stretch of columns: the generated frame's brightness over the real frame after's, and red
    over green of the generated frame, of the real frame before and of the real frame after"""
    q = [f32(a, y0, y1, x0, x1).reshape(-1, 3).mean(axis=0) for a in (gen, pic(os.path.join(run, "f%02d-present.raw" % (i - 1))), pic(os.path.join(run, "f%02d-present.raw" % i)))]
    rg = lambda c: float(c[0] / max(c[1], 1e-6))
    return float(lum(q[0]) / max(lum(q[2]), 1e-6)), rg(q[0]), rg(q[1]), rg(q[2])


def compare(a, b):
    """two whole images: (pixels that differ at all, by 4 codes of 255 as shown or more, the largest in codes, the map)"""
    n = c4 = big = 0
    diff = np.zeros((H, W), dtype=bool)
    for y in range(0, H, 270):
        p, q = np.asarray(a[y:y + 270, :, :3]), np.asarray(b[y:y + 270, :, :3])
        d = (p != q).any(axis=2)
        diff[y:y + 270] = d
        if d.any():
            c = np.abs(tone(p.astype(np.float32)) - tone(q.astype(np.float32))).max(axis=2)
            n += int(d.sum()); c4 += int((c >= 4).sum()); big = max(big, int(round(float(c.max()))))
    return n, c4, big, diff


def diff_png(a, b, path, keep):
    q = np.zeros((H // 4, W // 4), dtype=np.float32)
    for y in range(0, H, 216):
        c = np.abs(tone(np.asarray(a[y:y + 216, :, :3], dtype=np.float32)) - tone(np.asarray(b[y:y + 216, :, :3], dtype=np.float32))).max(axis=2) * keep[y:y + 216]
        q[y // 4:(y + 216) // 4] = c.reshape(54, 4, W // 4, 4).max(axis=(1, 3))
    Image.fromarray(np.clip(q * 8.0, 0, 255).astype(np.uint8), "L").save(path)


def strip(run, i, rect, gens, title, path):
    x, y, w, h = rect
    tiles = [("real frame before", pic(os.path.join(run, "f%02d-present.raw" % (i - 1))))] + gens + [("real frame after", pic(os.path.join(run, "f%02d-present.raw" % i)))]
    sheet = Image.new("RGB", (len(tiles) * (w + 4) - 4, h + 18), (40, 40, 40))
    dr = ImageDraw.Draw(sheet)
    dr.text((4, 3), title, fill=(255, 255, 255))
    for n, (name, a) in enumerate(tiles):
        im = Image.fromarray((tone(f32(a, y, y + h, x, x + w)) + 0.5).astype(np.uint8))
        ImageDraw.Draw(im).text((4, 4), name, fill=(255, 255, 0))
        sheet.paste(im, (n * (w + 4), 18))
    sheet.save(path)


def gate_map(gate):
    """the pixels the instrument painted: exactly 4 0 4"""
    m = np.zeros((H, W), dtype=bool)
    for y in range(0, H, 270):
        p = np.asarray(gate[y:y + 270, :, :3])
        m[y:y + 270] = (p[..., 0] == 4.0) & (p[..., 1] == 0.0) & (p[..., 2] == 4.0)
    return m


def grow(m, r):
    return boxsum(m.astype(np.int32), r) > 0


def main():
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: the replays inherit it
    os.makedirs(OUT, exist_ok=True); os.makedirs(SHOW, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(OUT, "result.txt"))
    t0 = time.time()
    missing = [n for n, p in LIBS if not os.path.isfile(p)]
    if missing:
        print("FAIL: not built: %s (build-ffx-own.ps1 -Target dlssiv-fg; python tools\\ffx-own\\build_variants.py ghost ghost-gate)" % ", ".join(missing)); return 1
    if not ("--keep" in ARGS and os.path.exists(REPLAY)) and not build_replay():
        print("FAIL: the replay did not compile"); return 1
    print("THE GHOST VARIANT ON THE CAPTURED RUNS under %s; generated frames %d to %d; the strips go to %s" % (RUNS, F0, F1, SHOW))
    for n, p in LIBS:
        if not n.endswith(("-again", "-third")): print("  %-10s %d bytes, SHA-256 %s" % (n, os.path.getsize(p), C.sha256_of(p)))
    runs = [r for r in BENCH if (not ONLY or r in ONLY or r.replace("\\", "-") in ONLY) and os.path.isfile(os.path.join(RUNS, r, "index.txt"))]
    if not runs:
        print("FAIL: no run of the bench under %s" % RUNS); return 1
    for rel in runs:
        run = os.path.join(RUNS, rel); tag = rel.replace("\\", "-")
        idx = open(os.path.join(run, "index.txt"), encoding="utf-8", errors="replace").read()
        rw, rh = [int(v) for v in re.search(r"(?m)^render: (\d+)x(\d+)", idx).groups()]
        print("\n" + "=" * 118 + "\n%s (the capture's library: SHA-256 %s; HUD-less image: %s)" % (rel, re.search(r"(?m)^library_sha256: (\w+)", idx).group(1)[:16], re.search(r"(?m)^hudless: (\d)", idx).group(1)))
        rep = {}
        for n, p in LIBS: rep[n] = replay(run, tag, p, n)
        if any(tag in f for f in failed):
            print("  A REPLAY DID NOT RUN: this run is left out"); continue
        g = lambda n, i: pic(os.path.join(rep[n][0], "f%02d-gen-replay.raw" % i))
        saved = lambda i: pic(os.path.join(run, "f%02d-gen.raw" % i))
        print("  IS THE BENCH SOUND? pixels of a generated frame that differ from the SAVED one (of %d), frames %d to %d: the product %s; the product again %s; the reference %s; frame 0 (the reset): %s" % (
            W * H, F0, F1, ", ".join(str(rep["product"][1].get(i)) for i in range(F0, F1 + 1)), ", ".join(str(rep["product-again"][1].get(i)) for i in range(F0, F1 + 1)),
            ", ".join(str(rep["reference"][1].get(i)) for i in range(F0, F1 + 1)), ", ".join("%s %s" % (n, "EQUAL" if rep[n][1].get(0) == 0 else "DIFFERS in %s" % rep[n][1].get(0)) for n in ("product", "reference", "ghost"))))
        b = BENCH[rel]
        for i in range(F0, F1 + 1):
            pp = compare(g("product", i), g("product-again", i)); pr = compare(g("product", i), g("reference", i)); pv = compare(g("product", i), g("ghost", i))
            gm = gate_map(g("ghost-gate", i))
            outside = pv[3] & ~gm
            pa = compare(g("alone", i), g("ghost-alone", i))
            alone_out = pa[3] & ~gm
            steady = pa[3].copy()
            for n in ("alone", "ghost-alone"):
                for k in ("-again", "-third"): steady &= ~compare(g(n, i), g(n + k, i))[3]
            steady_out = steady & ~gm
            steady_in = int((steady & gm).sum())
            print("  frame %d: TWO REPLAYS OF THE PRODUCT differ in %d pixels (%d by 4 codes or more, the largest %d); the reference from the product in %d (%d, %d)" % (i, pp[0], pp[1], pp[2], pr[0], pr[1], pr[2]))
            print("  frame %d: THE VARIANT from the product in %d pixels (%.2f%% of the picture; %d by 4 codes or more, the largest %d). THE GATE: the instrument painted %d pixels (%.2f%% of the picture)" % (
                i, pv[0], 100.0 * pv[0] / (W * H), pv[1], pv[2], int(gm.sum()), 100.0 * gm.mean()))
            print("  frame %d: OUTSIDE THE GATE. With nothing inpainted in either (the interpolation pass's own picture): the variant from the product in %d pixels, %d of them outside the gate's map (%d by 4 codes or more). OF THOSE, THE SAME IN ALL THREE REPLAYS OF EACH LIBRARY (so not the method's own race): %d, %d of them farther than 4 pixels from the map (ITS CONTROL: the same rule finds %d such pixels INSIDE the map). With the inpainting as it is: %d outside it (the inpainting pass fills its pixels from a pyramid of the whole neighbourhood). Two replays of the product by themselves: %d outside it" % (
                i, pa[0], int(alone_out.sum()), int((alone_out & (np.abs(tone(np.asarray(g("alone", i)[..., :3], dtype=np.float32)) - tone(np.asarray(g("ghost-alone", i)[..., :3], dtype=np.float32))).max(axis=2) >= 4)).sum()) if alone_out.any() else 0,
                int(steady_out.sum()), int((steady_out & ~grow(gm, 4)).sum()) if steady_out.any() else 0, steady_in, int(outside.sum()), int((pp[3] & ~gm).sum())))
        i = F1
        for what, x0, x1, y0, y1 in b.get("band", []):
            for k in range(F0, F1 + 1):
                print("  (a) THE BAND, %s, rows %d to %d, frame %d: generated over the real frame after, least mean most: %s" % (what, y0, y1, k, "; ".join(
                    "%s %.2f %.2f %.2f" % ((n,) + band(run, k, a, x0, x1, y0, y1)) for n, a in (("saved", saved(k)), ("product", g("product", k)), ("reference", g("reference", k)), ("VARIANT", g("ghost", k))))))
        for what, y0, y1, x0, x1 in b.get("bandx", []):
            for k in range(F0, F1 + 1):
                print("  (b) THE STRIP, %s (rows %d to %d, columns %d to %d), frame %d: brightness over the real frame after's, red over green: %s; the real frames' red over green: before %.2f, after %.2f" % (
                    what, y0, y1, x0, x1, k, "; ".join("%s %.2f %.2f" % ((n,) + bandx(run, k, a, y0, y1, x0, x1)[:2]) for n, a in (("saved", saved(k)), ("product", g("product", k)), ("reference", g("reference", k)), ("VARIANT", g("ghost", k)))),
                    bandx(run, k, saved(k), y0, y1, x0, x1)[2], bandx(run, k, saved(k), y0, y1, x0, x1)[3]))
        rect = dict(b["crops"])[b["broad"]]
        for k in range(F0, F1 + 1):
            print("  (c) THE BROAD DIFFERENCE FROM THE REAL FRAMES (stills\\mock3.py's number), crop %s %s, frame %d, mean and 99th percentile: %s" % (b["broad"], "%d,%d,%d,%d" % rect, k, "; ".join(
                "%s %.4f %.4f" % ((n,) + broad(run, k, a, rect, rw, rh)) for n, a in (("saved", saved(k)), ("product", g("product", k)), ("reference", g("reference", k)), ("VARIANT", g("ghost", k))))))
        for name, rect in b["crops"]:
            p = os.path.join(SHOW, "%s-f%02d-%s.png" % (tag, i, name))
            strip(run, i, rect, [("GENERATED: the product", g("product", i)), ("GENERATED: THE VARIANT", g("ghost", i))], "%s, generated frame %d, crop %s at %d,%d (%dx%d)" % ((rel, i, name) + rect), p)
            print("  strip: %s" % p)
        p = os.path.join(SHOW, "%s-f%02d-difference.png" % (tag, i))
        same = ~compare(g("product", i), g("product-again", i))[3] & ~compare(g("ghost", i), g("ghost-again", i))[3]
        diff_png(g("product", i), g("ghost", i), p, same)
        print("  the whole frame's difference, the variant against the product (a quarter of the size, times 8; of the pixels that are the same in two replays of each library: %.2f%% of the picture are not, by the method's own race, and are left black): %s" % (100.0 * (1.0 - same.mean()), p))
        p = os.path.join(SHOW, "%s-f%02d-gate.png" % (tag, i))
        Image.fromarray((gate_map(g("ghost-gate", i)).reshape(H // 4, 4, W // 4, 4).max(axis=(1, 3)) * 255).astype(np.uint8), "L").save(p)
        print("  the gate's map (a quarter of the size; white: the variant's own path): %s" % p)
    for f in failed: print("  failed: " + f)
    print("%s: %d run(s), in %.0f s; the replays are kept under %s" % ("DONE" if not failed else "FAIL (%d replay(s) did not run)" % len(failed), len(runs), time.time() - t0, OUT))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
