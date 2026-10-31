r"""THE VARIANT v-env WITH A HUD-LESS IMAGE BOUND: DOES IT TOUCH THE INTERFACE'S PIXELS (2026-10-11; BRIEF-env.md, "what must not
happen"). No captured run has a HUD-less image, so this script MAKES one out of a captured run, in a folder of its own
under %TEMP% (the captured run is read and never written):

    the HUD-less picture of a frame   the run's presented picture as it is
    the presented picture             the same with AN INTERFACE drawn over it, at the same screen places in every frame:
        1  an opaque white plate        on plain moving ground
        2  an opaque white plate        IN THE BAND UNDER THE CAR, where the variant relights most
        3  a half-transparent grey one  beside it
        4  a FAINT one (the scene times 1.05: under AMD's ten percent) beside that

and replays it through the product and through the variant. AMD's code draws the interface in its last pass: where the
presented picture is 10 percent or more off the HUD-less one in a channel the pixel IS the presented picture's, under that
a mix of it and the generated one. v-env changes neither that pass nor what it reads; its envelopes are made from the
HUD-less picture. So in plates 1 to 3 the product's and the variant's generated frames must be THE SAME PIXELS, and the
presented picture's; in plate 4 the scene under the plate shows through, relit or not, as it does everywhere.
THE CONTROL: outside the plates, in the band under the car, the variant must DIFFER from the product (it acts there).

    python tools\test-fg-own\env_hud.py [--run=cap3\run-03] [--frames=8-15] [--keep]
It takes the card for a minute (ONE GPU USER AT A TIME; it waits). The last line is PASS or FAIL.
"""
import importlib.util, os, re, shutil, sys, tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = sys.argv[1:]
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
REL = arg("--run", r"cap3\run-03")
PLATES = [("1 an opaque white plate on plain moving ground", (900, 1750, 300, 120), "white"),
          ("2 an opaque white plate in the band under the car", (1980, 1515, 120, 40), "white"),
          ("3 a half-transparent grey plate beside it", (2110, 1515, 60, 40), "half"),
          ("4 a faint plate (the scene times 1.05)", (2200, 1515, 120, 40), "faint")]
CONTROL = (1960, 1560, 360, 30)        # the band under the car, under the plates: the variant acts here


def load_env():
    saved = sys.argv
    sys.argv = [saved[0], "--keep", "--frames=" + arg("--frames", "8-15")] + [a for a in ARGS if a.startswith("--runs=")]
    spec = importlib.util.spec_from_file_location("fg_env_bench", os.path.join(HERE, "env.py"))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


E = load_env()
W, H = E.W, E.H


def make(src, dst):
    """the run with a HUD-less image: the index says so, a frame's HUD-less picture is the captured presented one, and
    the presented one carries the plates"""
    os.makedirs(dst, exist_ok=True)
    idx = open(os.path.join(src, "index.txt"), "rb").read().decode("utf-8", "replace")
    nl = "\r\n" if "\r\n" in idx else "\n"
    out = []
    for line in idx.replace("\r\n", "\n").split("\n"):
        if line.startswith("hudless: "): line = "hudless: 1"
        if line.startswith("frame: "): line = re.sub(r" hud=\d", " hud=1", line)
        out.append(line)
        m = re.match(r"file: (f(\d+)-present\.raw) present (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)$", line)
        if m: out.append("file: f%s-hud.raw hud %s %s %s %s %s %s" % (m.group(2), m.group(3), m.group(4), m.group(5), m.group(6), m.group(7), m.group(8)))
    open(os.path.join(dst, "index.txt"), "wb").write(nl.join(out).encode("utf-8"))
    for n in sorted(os.listdir(src)):
        if n == "index.txt": continue
        s, d = os.path.join(src, n), os.path.join(dst, n)
        if not n.endswith("-present.raw"):
            if not os.path.isfile(d) or os.path.getsize(d) != os.path.getsize(s): shutil.copyfile(s, d)
            continue
        hud = os.path.join(dst, n.replace("-present.raw", "-hud.raw"))
        if not os.path.isfile(hud) or os.path.getsize(hud) != os.path.getsize(s): shutil.copyfile(s, hud)
        a = np.fromfile(s, dtype=np.float16).reshape(H, W, 4)
        for _, (x, y, w, h), kind in PLATES:
            q = a[y:y + h, x:x + w, :3].astype(np.float32)
            if kind == "white": q[:] = 1.5
            elif kind == "half": q = 0.5 * q + 0.5 * 0.4
            else: q = q * 1.05
            a[y:y + h, x:x + w, :3] = q.astype(np.float16)
        a.tofile(d)


def main():
    os.makedirs(E.OUT, exist_ok=True)
    sys.stdout = E.C.Tee(os.path.join(E.OUT, "result-hud.txt"))
    src = os.path.join(E.RUNS, REL); tag = REL.replace("\\", "-") + "-hud"
    dst = os.path.join(E.OUT, "run-" + tag)
    if not os.path.isfile(os.path.join(src, "index.txt")):
        print("FAIL: no run %s" % src); return 1
    if not os.path.exists(E.REPLAY) and not E.build_replay():
        print("FAIL: the replay did not compile"); return 1
    print("THE VARIANT v-env WITH A HUD-LESS IMAGE BOUND: %s with an interface drawn over its presented pictures (made in %s), generated frames %d to %d" % (REL, dst, E.F0, E.F1))
    make(src, dst)
    rep = {n: E.replay(dst, tag, p, n, E.F0) for n, p in (("product", E.PRODUCT), ("env", E.var("env")))}
    if E.failed:
        for f in E.failed: print("  failed: " + f)
        print("FAIL"); return 1
    said = open(os.path.join(rep["env"][0], "replay.txt"), encoding="utf-8", errors="replace").read()
    with_hud = "with the HUD-less image" in said
    print("  the replay says of the run: %s" % ("with the HUD-less image" if with_hud else "NO HUD-LESS IMAGE (the made run was not read as one)"))
    bad = 0 if with_hud else 1
    g = lambda n, i: E.pic(os.path.join(rep[n][0], "f%02d-gen-replay.raw" % i))
    for what, (x, y, w, h), kind in PLATES:
        diff = same_present = 0; most = 0.0
        for k in range(E.F0, E.F1 + 1):
            P, V = E.f32(g("product", k), y, y + h, x, x + w), E.f32(g("env", k), y, y + h, x, x + w)
            R = E.f32(E.pic(os.path.join(dst, "f%02d-present.raw" % k)), y, y + h, x, x + w)
            d = (P != V).any(axis=2); diff += int(d.sum())
            same_present += int(((V == R).all(axis=2)).sum())
            if d.any(): most = max(most, float(np.abs(E.tone(P) - E.tone(V)).max()))
        n = w * h * (E.F1 - E.F0 + 1)
        must = kind != "faint"
        ok = (diff == 0 and same_present == n) if must else True
        bad += 0 if ok else 1
        print("  plate %s (%d,%d, %d by %d), %d pixels over the frames: the variant differs from the product in %d of them (the most %.0f codes); the variant's pixel IS the presented picture's in %d: %s" % (
            what, x, y, w, h, n, diff, most, same_present, ("as it must be" if ok else "NOT AS IT MUST BE") if must else "the scene shows through a faint plate, relit or not (said, not held)"))
    x, y, w, h = CONTROL; diff = 0
    for k in range(E.F0, E.F1 + 1): diff += int((E.f32(g("product", k), y, y + h, x, x + w) != E.f32(g("env", k), y, y + h, x, x + w)).any(axis=2).sum())
    n = w * h * (E.F1 - E.F0 + 1)
    ok = diff > n // 4
    bad += 0 if ok else 1
    print("  THE CONTROL, the band under the car outside the plates (%d,%d, %d by %d): the variant differs from the product in %d of %d pixels: %s" % (x, y, w, h, diff, n, "it acts there, as it must" if ok else "IT DOES NOT ACT THERE: the test says nothing"))
    print("PASS: with a HUD-less image bound the variant leaves the interface's pixels the product's and the presented picture's" if not bad else "FAIL")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
