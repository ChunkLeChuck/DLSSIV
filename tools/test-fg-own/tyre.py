r"""THE TORN BOTTOM OF THE TYRE: WHO MAKES IT (2026-10-11; docs\reports\fg-3\desk-2026-10-11-ghost\env; the owner, of the capture
with the wheels' key on: "the same kind of broken or torn pattern on the bottom of the car tyre"; and: "it could be the
same issue as that universal ghosting thing").

In the product's generated frame the bottom of a tyre that stands still on the screen over fast ground is cut in steps
and a wedge of ground stands inside its outline; both real frames are clean. This script reads, on one captured run
replayed from its reset (tools\test-fg-own\env.py's replays are used where they lie), INSIDE THE STILL THING and IN THE
GROUND NEXT TO IT, a frame a line:

  THE TEAR     of every library (the product, v-ghost, v-env, and the product and v-env with nothing inpainted): the share
               of the still thing's pixels whose generated colour is 24 codes or more (of the 255 as shown) off the
               mean of the two real frames at the place, and the generated brightness over the real frames' there
  THE GAME'S   (the instrument "fields") the game field's half vector at those pixels as the interpolation pass reads
  VECTOR       it: still, or the ground's; and which samples the pass uses there, and where it asks for inpainting
  OPTICAL      (the instruments "flow" and "env-flow") optical flow's half vector at those pixels: none, still, or the
  FLOW         ground's; and THE SHARE OF THE GAME'S COLOUR against the colour fetched along optical flow's vector, in
               the product and in v-env
A pixel is "of the still thing" where the game's own vectors of BOTH real frames say under 8 display pixels a frame
(3 pixels in from its edge); "the ground next to it" is the fast ground within 16 pixels of it (2 pixels out).
And one picture: the rectangle in every judged frame, a row a library, A LARGE LABEL ON EVERY ROW.

    python tools\test-fg-own\tyre.py [--run=capw\run-03] [--rect=1940,1470,330,105] [--frames=8-15] [--out=<folder>] [--keep]
It takes the card for a few minutes (ONE GPU USER AT A TIME; it waits). The last line is DONE or FAIL.
"""
import importlib.util, os, sys, tempfile

import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = sys.argv[1:]
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
REL = arg("--run", r"capw\run-03")
X, Y, WD, HT = [int(v) for v in arg("--rect", "1940,1470,330,105").split(",")]
FRAMES = arg("--frames", "8-15")


def load_env():
    saved = sys.argv
    sys.argv = [saved[0], "--keep", "--frames=" + FRAMES] + [a for a in ARGS if a.startswith(("--runs=", "--out="))]
    spec = importlib.util.spec_from_file_location("fg_env_bench", os.path.join(HERE, "env.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


E = load_env()
W, H = E.W, E.H
M = 24                                  # the margin round the rectangle the masks are made with


def crop(a): return np.asarray(a[Y - M:Y + HT + M, X - M:X + WD + M, :3], dtype=np.float32)
def inner(q): return q[M:-M, M:-M]


def main():
    os.makedirs(E.OUT, exist_ok=True); os.makedirs(E.SHOW, exist_ok=True)
    sys.stdout = E.C.Tee(os.path.join(E.OUT, "result-tyre.txt"))
    run = os.path.join(E.RUNS, REL); tag = REL.replace("\\", "-")
    if not os.path.isfile(os.path.join(run, "index.txt")):
        print("FAIL: no run %s" % run); return 1
    if not os.path.exists(E.REPLAY) and not E.build_replay():
        print("FAIL: the replay did not compile"); return 1
    F0, F1 = E.F0, E.F1
    libs = [("product", E.PRODUCT, 1), ("ghost", E.var("ghost"), F0), ("env", E.var("env"), 1), ("alone", E.var("alone"), F0), ("env-alone", E.var("env-alone"), F0),
            ("fields", E.var("fields"), F0), ("flow", E.var("flow"), F0), ("env-flow", E.var("env-flow"), F0)]
    missing = [n for n, p, f in libs if not os.path.isfile(p)]
    if missing:
        print("FAIL: not built: %s (python tools\\ffx-own\\build_variants.py ...)" % ", ".join(missing)); return 1
    print("THE TORN BOTTOM OF THE TYRE: %s, the rectangle %d,%d (%d by %d), generated frames %d to %d" % (REL, X, Y, WD, HT, F0, F1))
    rep = {n: E.replay(run, tag, p, n, f) for n, p, f in libs}
    if E.failed:
        for f in E.failed: print("  failed: " + f)
        print("FAIL"); return 1
    g = lambda n, i: E.pic(os.path.join(rep[n][0], "f%02d-gen-replay.raw" % i))
    import re
    idx = open(os.path.join(run, "index.txt"), encoding="utf-8", errors="replace").read()
    rw, rh = [int(v) for v in re.search(r"(?m)^render: (\d+)x(\d+)", idx).groups()]

    def speed(k):
        mv = np.fromfile(os.path.join(run, "f%02d-mv.raw" % k), dtype=np.float16).reshape(rh, rw, 2).astype(np.float32)
        ys = (np.arange(Y - M, Y + HT + M) * rh // H)[:, None]; xs = (np.arange(X - M, X + WD + M) * rw // W)[None, :]
        return np.hypot(mv[ys, xs, 0] * W / rw, mv[ys, xs, 1] * H / rh)

    def box(m, r): return E.boxsum(m.astype(np.float32), r)
    rows = {}
    for k in range(F0, F1 + 1):
        sa, sb = speed(k), speed(k - 1)
        still = (sa <= 8.0) & (sb <= 8.0)
        thing = inner(box(still, 3) >= 48.5)                                   # 3 pixels in: every pixel of the 7 by 7 is still
        fast = (sa > 8.0) & (sb > 8.0)
        near = inner((box(still, 16) > 0.5) & (box(still, 2) < 0.5) & fast)     # the fast ground within 16 pixels of the still thing, 2 out
        B, A = inner(crop(E.real(run, k - 1))), inner(crop(E.real(run, k)))
        mean = (B + A) / 2
        line = ["  frame %d: the still thing %d pixels, the ground next to it %d; the ground's speed there %.0f display pixels a frame" % (k, int(thing.sum()), int(near.sum()), float(np.median(inner(sa)[near])) if near.any() else 0.0)]
        tear = []
        for n in ("product", "ghost", "env", "alone", "env-alone"):
            G = inner(crop(g(n, k)))
            d = np.abs(E.tone(G) - E.tone(mean)).max(axis=2)
            tear.append("%s %.1f%% (brightness %.2f)" % (n, 100.0 * (d[thing] >= 24).mean(), float(E.lum(G)[thing].mean() / max(E.lum(mean)[thing].mean(), 1e-6))))
            rows.setdefault(n, []).append(float((d[thing] >= 24).mean()))
        line.append("      THE TEAR, the still thing's pixels 24 codes or more off the real frames (and its brightness over theirs): " + "; ".join(tear))
        for fn, fwhat in (("fields", "the product"),):
            F = inner(crop(g(fn, k)))
            hv = np.hypot(F[..., 0], F[..., 1]); code = np.rint(F[..., 2]).astype(int)
            def codes(sel): return "both samples %.1f%%, the frame before's alone %.1f%%, the current frame's alone %.1f%%, none %.1f%%; inpainting asked for %.1f%%; THE VECTOR ITSELF: thrown there by a pixel of the current frame %.1f%% (as a second choice %.1f%%), filled in from the field's pyramid (no pixel was thrown there) %.1f%%" % (
                100.0 * ((code[sel] & 3) == 3).mean(), 100.0 * ((code[sel] & 3) == 1).mean(), 100.0 * ((code[sel] & 3) == 2).mean(), 100.0 * ((code[sel] & 3) == 0).mean(), 100.0 * ((code[sel] & 4) != 0).mean(),
                100.0 * ((code[sel] & 8) == 0).mean(), 100.0 * ((code[sel] & 16) != 0).mean(), 100.0 * ((code[sel] & 8) != 0).mean())
            line.append("      THE GAME'S VECTOR as the pass reads it in %s. In the still thing: a half vector over 4 pixels (the ground's) in %.1f%% of its pixels, the median %.1f; %s" % (fwhat, 100.0 * (hv[thing] > 4).mean(), float(np.median(hv[thing])), codes(thing)))
            if fn == "fields":
                wrong = thing & (hv > 4)
                if wrong.any(): line.append("                  of the still thing's pixels that are given the ground's vector: %s" % codes(wrong))
                if near.any(): line.append("                  In the ground next to it: under 4 pixels (the still thing's) in %.1f%%, the median %.1f; %s" % (100.0 * (hv[near] <= 4).mean(), float(np.median(hv[near])), codes(near)))
        for n in ("flow", "env-flow"):
            O = inner(crop(g(n, k)))
            ov = np.hypot(O[..., 0], O[..., 1]); share = O[..., 2]
            def flow(sel): return "none %.1f%%, under 4 pixels %.1f%%, over 4 (moving) %.1f%% (their median %.1f); THE GAME'S COLOUR'S SHARE: mean %.2f, under a half in %.1f%%" % (
                100.0 * (ov[sel] == 0).mean(), 100.0 * ((ov[sel] > 0) & (ov[sel] <= 4)).mean(), 100.0 * (ov[sel] > 4).mean(), float(np.median(ov[sel][ov[sel] > 4])) if (ov[sel] > 4).any() else 0.0,
                float(share[sel].mean()), 100.0 * (share[sel] < 0.5).mean())
            line.append("      OPTICAL FLOW in %s. In the still thing: %s" % ("the product" if n == "flow" else "v-env", flow(thing)))
            if near.any(): line.append("                                  In the ground next to it: %s" % flow(near))
        print("\n".join(line))
    print("  THE TEAR, a frame a number (percent of the still thing's pixels): " + "; ".join("%s %s" % (n, " ".join("%.1f" % (100 * v) for v in rows[n])) for n in rows))

    # ---- the picture: a row a library, a column a frame
    sc = 1.5
    names = [("REAL FRAME AFTER", None, (255, 255, 255)), ("GENERATED TODAY (the product)", "product", (255, 120, 120)), ("v-ghost", "ghost", (255, 220, 60)), ("THE NEW LIBRARY (v-env)", "env", (120, 255, 120))]
    frames = list(range(F0, F1 + 1))
    for part in range(0, len(frames), 4):
        fr = frames[part:part + 4]
        tw, th = int(WD * sc), int(HT * sc)
        sheet = Image.new("RGB", (len(fr) * (tw + 4) - 4, 44 + len(names) * (th + 44)), (20, 20, 20))
        d = ImageDraw.Draw(sheet)
        d.text((10, 7), "%s, the bottom of the tyre (display %d,%d, %d by %d), generated frames %d to %d" % (REL, X, Y, WD, HT, fr[0], fr[-1]), fill=(255, 255, 255), font=E.font(26))
        for r, (text, lib, colour) in enumerate(names):
            y = 44 + r * (th + 44)
            E.label(d, 10, y + 6, text, colour, E.font(26))
            for c, k in enumerate(fr):
                a = E.real(run, k) if lib is None else g(lib, k)
                sheet.paste(E.tile(a, (X, Y, WD, HT), sc), (c * (tw + 4), y + 40))
                E.label(d, c * (tw + 4) + tw - 100, y + 40 + th - 30, "frame %d" % k, (200, 200, 200), E.font(18))
        p = os.path.join(E.SHOW, "%s-tyre-bottom-f%02d-f%02d.png" % (tag, fr[0], fr[-1]))
        sheet.save(p); print("  sheet: %s" % p)
    print("DONE")
    return 0


if __name__ == "__main__":
    sys.exit(main())
