r"""THE LEFTOVERS OF THE PICTURE ON THE OWNER'S CAPTURED FRAMES: THE PRODUCT BESIDE A VARIANT BUILT ON TOP OF IT (2026-10-12, the main
session's addition to docs\reports\fg-3\sitting-2026-10-12b\BRIEF-ship.md, part 6). The bench, the replays and the
three-replay rule are lib4.py's and env.py's; this tool adds one number of its own:

  A PIXEL OF A STILL THING THAT IS WRONG  the two real frames round a generated frame agree at the place within STILL codes of the
      255 as shown (something stands still on the screen there: the car, its tyre, the sky; or the place is flat), and
      the generated frame is WRONG codes or more off BOTH. Whatever was put there did not belong: a piece of fast ground
      thrown into a still tyre's outline or along the bumper's edge is such pixels, and so is any harm a mend does to
      a still thing. Counted a frame, over a run's zone (the car's lower half and its shadow) and over the whole
      frame, the mean of the generated frames 8 to 15, from each of three replays.

    python tools\test-fg-own\next.py [--variant=next] [--only=<run>,...] [--out=<folder>] [--no-pictures] [--one]
        --variant   the variant library, by its name in tools\ffx-own\build_variants.py (next)
        --one       one replay a library and no three-replay rule: a quick look, not a judgement (it ends FAIL)
For each run: the still thing's wrong pixels and the pixels in the wrong light (lib4.py), the product and the variant;
what the variant changes against the product by 1, 4 and 16 codes; a sheet of four tiles with a large label on every
tile at the place where the variant changes most in the zone, and at the run's own named places; a flip sequence
(the product left, the variant right). NOT A PROOF AND NOT A JUDGE: THE PICTURE IS THE OWNER'S. ONE GPU USER AT A TIME.
What it printed is kept as result-<variant>.txt in --out. The last line is DONE or FAIL.
"""
import importlib.util, os, shutil, sys, time

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
VAR = arg("--variant", "next")
SHOW = arg("--out", os.path.join(ROOT, "docs", "reports", "fg-3", "sitting-2026-10-12b", "next"))
ONE = "--one" in ARGS
STILL, WRONG = 3.0, 16.0


def load(name, path, argv):
    saved = sys.argv; sys.argv = [saved[0]] + argv
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); sys.argv = saved
    return m


L = load("lib4_bench", os.path.join(HERE, "lib4.py"), ["none"] + [a for a in ARGS if a.startswith("--only=")])
E = L.E
W, H, FR = L.W, L.H, L.FR
# a run's named places (x, y, width, height in presented pixels): where the last rounds' reports found a leftover
PLACES = {
    r"capw\run-03": [("tyre", (1940, 1470, 330, 105))],
    r"cap5\run-02": [("shadow-tip", (1170, 1679, 300, 170))],
}
MORE = {r"capw\run-03": ("the capture with the wheels' key: the chase camera, the still tyre's bottom", (1500, 1200, 1300, 600)),
        r"cap3\run-03": L.RUNS[r"cap3\run-03"]}
RUNS = dict([(r, L.RUNS[r]) for r in L.RUNS if r.startswith(("cap5", "prof5"))] + list(MORE.items()))


def repeats(rel, i, g, rect=None):
    """A PIXEL THAT REPEATS THE NEWER REAL FRAME: the two real frames differ at the place by MOVES codes or more (something moves there)
    and the generated frame is the newer one's pixel to the code. A generated frame is made of what lies BETWEEN two
    real frames; where it repeats one, nothing was generated. (THE YARDSTICK ABOVE CANNOT SEE THIS, and "nearer a real
    frame" reads it as a gain: the first form of v-field-depth-then scored best exactly where it repeated the frame.)"""
    return (E.codes(L.real(rel, i - 1), L.real(rel, i), rect) >= 8.0) & (E.codes(g, L.real(rel, i), rect) <= 1.0)


_agree = {}


def still_wrong(rel, i, g, rect):
    """the map of the pixels of a rect where the two real frames agree and the generated frame is off both"""
    if (rel, i) not in _agree:
        if len(_agree) > 8: _agree.clear()
        _agree[(rel, i)] = E.codes(L.real(rel, i - 1), L.real(rel, i)) <= STILL
    x, y, w, h = rect
    return _agree[(rel, i)][y:y + h, x:x + w] & (np.minimum(E.codes(g, L.real(rel, i - 1), rect), E.codes(g, L.real(rel, i), rect)) >= WRONG)


def main():
    os.makedirs(SHOW, exist_ok=True); os.makedirs(E.OUT, exist_ok=True)
    sys.stdout = E.C.Tee(os.path.join(SHOW, "result-%s.txt" % VAR))
    t0 = time.time()
    if not os.path.exists(E.REPLAY) and not E.build_replay():
        print("FAIL: the replay did not compile"); return 1
    reps = 1 if ONE else 3
    print("THE PRODUCT BESIDE v-%s ON THE CAPTURED RUNS under %s: generated frames %d to %d, each run replayed from its reset, %d replay(s) a library; a still thing's pixel is wrong where the two real frames agree within %d codes and the generated frame is %d or more off both" % (
        VAR, E.RUNS, L.F0, L.F1, reps, STILL, WRONG))
    L.head(["product", VAR])
    full = (0, 0, W, H)
    tot = {"product": [0.0, 0.0], VAR: [0.0, 0.0]}; nruns = 0
    for rel in [r for r in RUNS if (not L.ONLY or r in L.ONLY or r.replace("\\", "-") in L.ONLY) and os.path.isfile(os.path.join(E.RUNS, r, "index.txt"))]:
        what, zone = RUNS[rel]; tag = rel.replace("\\", "-")
        print("\n" + "=" * 110 + "\n%s: %s; the zone %d,%d,%d,%d" % ((rel, what) + zone))
        for n in ("product", VAR):
            for k in range(reps): L.rep(rel, n, k)
        if any(f.startswith(tag + " ") for f in L.failed + E.failed): print("  A REPLAY DID NOT RUN: this run is left out"); continue
        nruns += 1
        G = lambda n, k: (lambda i: L.gen(rel, n, i, k))
        print("  a frame, the mean of frames %d to %d:   A STILL THING'S WRONG PIXELS in the zone | in the whole frame | PIXELS IN THE WRONG LIGHT in the zone (lib4.py) | the broad difference over the zone" % (L.F0, L.F1))
        for n in ("product", VAR):
            sz = [float(np.mean([still_wrong(rel, i, L.gen(rel, n, i, k), zone).sum() for i in FR])) for k in range(reps)]
            sf = [float(np.mean([still_wrong(rel, i, L.gen(rel, n, i, 0), full).sum() for i in (10, 12, 15)]))]      # (the whole frame: one replay, frames 10, 12 and 15)
            tot[n][0] += float(np.mean(sz)); tot[n][1] += float(np.mean(sf))
            rp = float(np.mean([repeats(rel, i, L.gen(rel, n, i, 0)).sum() for i in (10, 12, 15)]))
            print("      %-10s %s | %s | %s | %s | PIXELS THAT REPEAT THE NEWER REAL FRAME where the two differ by 8 codes or more, the whole frame: %.0f" % (
                "the product" if n == "product" else "v-" + n, E.three(sz, "%.0f"), E.three(sf, "%.0f"),
                E.three([L.ghost(rel, G(n, k), zone) for k in range(reps)], "%.0f"), E.three([L.broad(rel, G(n, k), zone) for k in range(reps)], "%.4f"), rp))
        if not ONE:
            c = L.changed(rel, "product", VAR); cz = L.changed(rel, "product", VAR, zone)
            print("  v-%s against the product, percent of the steady pixels that change by 1, 4, 16 codes and the most (frames 10, 12 and 15): the whole frame %.3f %.3f %.3f, %d | the zone %.3f %.3f %.3f, %d" % ((VAR,) + c[:3] + (int(c[3]),) + cz[:3] + (int(cz[3]),)))
            # of the pixels it moves by 16 codes: from wrong to right, from right to wrong (against the two real frames' span), frame 15
            i = 15; st = L.steady(rel, ["product"], [VAR], i)
            d = (E.codes(L.gen(rel, "product", i), L.gen(rel, VAR, i)) >= 16) & st
            off = lambda g: np.minimum(E.codes(g, L.real(rel, i - 1)), E.codes(g, L.real(rel, i)))
            op, ov = off(L.gen(rel, "product", i)), off(L.gen(rel, VAR, i))
            print("  of the %d pixels it moves by 16 codes or more in frame 15: %d were 16 or more off both real frames and are not any more (mended), %d were not and are now (broken), %d are nearer a real frame than before, %d farther" % (
                int(d.sum()), int((d & (op >= 16) & (ov < 16)).sum()), int((d & (op < 16) & (ov >= 16)).sum()), int((d & (ov < op)).sum()), int((d & (ov > op)).sum())))
        if "--no-pictures" not in ARGS:
            places = list(PLACES.get(rel, []))
            dd = E.codes(L.gen(rel, "product", 15), L.gen(rel, VAR, 15), zone)
            if dd.max() > 0:
                s = E.boxsum(np.minimum(dd, 64.0), 24); by, bx = np.unravel_index(int(np.argmax(s)), s.shape)
                places.append(("most-changed", L.around((zone[0] + bx - 48, zone[1] + by - 32, 96, 64), 300, 170)))
            for pname, rect in places:
                for f in (12, 15):
                    t = lambda a: E.tile(a, rect, 2.0)
                    sw = lambda n: int(still_wrong(rel, f, L.gen(rel, n, f), rect).sum())
                    tiles = [(t(L.real(rel, f - 1)), "1  REAL FRAME BEFORE", L.WHITE), (t(L.gen(rel, "product", f)), "2  THE PRODUCT (%d wrong)" % sw("product"), L.RED),
                             (t(L.gen(rel, VAR, f)), "3  v-%s (%d wrong)" % (VAR, sw(VAR)), L.GREEN), (t(L.real(rel, f)), "4  REAL FRAME AFTER", L.WHITE)]
                    p = os.path.join(SHOW, "%s-%s-f%02d-%s.png" % (VAR, tag, f, pname))
                    L.tiles_sheet(tiles, "%s, generated frame %d, at %d,%d (%d by %d): a still thing's wrong pixels in the tile's place" % ((rel, f) + rect), p, 2)
                    print("  sheet: %s" % p)
            x, y, w, h = zone
            frect = (int(min(max(x + w // 2 - 260, 0), W - 520)), int(min(max(y + h // 2 - 100, 0), H - 300)), 520, 300)
            base = os.path.join(SHOW, "%s-%s-flip-f08-f15" % (VAR, tag))
            L.flip(rel, frect, lambda i: L.gen(rel, "product", i), lambda i: L.gen(rel, VAR, i), "THE PRODUCT", "v-%s" % VAR, "%s: frames 8 to 15 as shown, at %d,%d" % ((rel,) + frect[:2]), base)
            print("  flip: %s.gif" % base)
        if not ONE and "--keep-replays" not in ARGS:
            for n in ("product", VAR):
                for k in (1, 2): shutil.rmtree(os.path.join(E.OUT, tag, n + L.THREE[k]), ignore_errors=True)
    if nruns:
        print("\nOVER %d RUNS, a still thing's wrong pixels a frame, the runs' mean: in the zone the product %.0f, v-%s %.0f; in the whole frame the product %.0f, v-%s %.0f" % (
            nruns, tot["product"][0] / nruns, VAR, tot[VAR][0] / nruns, tot["product"][1] / nruns, VAR, tot[VAR][1] / nruns))
    failed = L.failed + [f for f in E.failed if f not in L.failed]
    if ONE: failed.append("--one: a quick look, not the three-replay rule")
    for f in failed: print("  failed: " + f)
    print("%s, in %.0f s" % ("DONE" if not failed else "FAIL", time.time() - t0))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
