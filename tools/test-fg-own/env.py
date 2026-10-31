r"""THE VARIANT v-env ON THE OWNER'S CAPTURED FRAMES: THE NUMBERS, THE SHEETS, THE FLIP SEQUENCES, WHERE IT ACTS (2026-10-11;
docs\reports\fg-3\desk-2026-10-11-ghost\BRIEF-env.md and READING.md; the variant is ffx-own\cuts\v-env.cut, built by
tools\ffx-own\build_variants.py env env-gate env-alone alone [env-brief env-half] into obj\dlssiv-fg-var).

The ghost behind the player's car (the ground arrives with the light of the place it was fetched from) is mended by
v-ghost near the car only. v-env is one rule for the whole picture: two envelopes of the real frames' log brightness kept
at the screen place, each fetched sample relit for the place it is put. This script puts beside it what a judgement
needs, on the captured runs (host\fg_cap.h; tools\fg-replay makes a run's generated frames again through any library):

  EVERY RUN IS REPLAYED FROM ITS RESET, and the generated frames --frames (8 to 15) are judged, each one: the envelopes need
  the frames before, and a fix that is right in one frame and flickers is wrong.
  THE THREE-REPLAY RULE (tools\test-fg-own\ghost.py found it: two replays of one library differ in some pixels, AMD's method
  races with itself). The product and the variant are replayed three times each. A NUMBER (a mean over a band or a
  crop) is taken from each replay and printed once where the three agree to the printed digit, else as its span. A
  PIXEL COUNT ("the variant changed this pixel") is said only of a pixel that is the same in all three replays of the
  one, the same in all three of the other, and not the same in both.
  THE NUMBERS, for the product, v-ghost and v-env (and the variants to compare with, where they are built):
     (a) "generated over real" a row over a band (mock5.py's own: the generated frame's brightness, the mean of the
         band's columns, over the mean of the two real frames', a row a line: its least, mean and most);
     (b) the same over the strip behind the car in the profile view;
     (c) mock5.py's "broad difference from the real frames" over a crop's fast background (the 13 by 13 mean of the
         generated frame's brightness less the two real frames' mean: its mean and 99th percentile);
     (d) THE VARIANT AGAINST THE PRODUCT over a crop where the product is right today (the road beside the car): how many
         pixels change by 1, 4 and 16 codes of the picture as shown, and the most; and the same over the whole frame
  WHERE IT ACTS  the instrument "env-gate" paints every pixel in which a sample was relit. With nothing inpainted in
     either ("alone" and "env-alone", three replays each): how many pixels the variant changed, and how many of
     them lie outside that map (0 is the proof that under its dead band the variant is the product's path)
  AFTER A RESET  frames 0 to 7 of the product's and the variant's first replay against the SAVED images, in pixels:
     the dispatch of the reset and the frames in which the variant does nothing must read as the product's own
  THE SHEETS     two by two, A LARGE LABEL ON EVERY TILE: 1 the real frame before, 2 generated today (the product),
     3 the new library, 4 the real frame after; a crop a sheet, at the last frame judged (and --sheet-frames)
  THE DIFFERENCE the whole frame, variant against product (a quarter of the size, times 8; of the steady pixels)
  THE FLIP SEQUENCE  one a run: the generated and the real frames --frames in their shown order at one crop, the product
     on the left and the variant on the right, as an animated PNG and a GIF (0.4 s a frame)

    python tools\test-fg-own\env.py [--runs=<folder>] [--out=<folder>] [--only=<run>,<run>] [--frames=8-15] [--keep]
           [--more=env-brief,env-half] [--variant=env] [--no-pictures]
        --runs    the folder of the captured runs (not given: %TEMP%\dlssiv-fg-runs); a run is a folder with index.txt
        --out     where the sheets go (not given: docs\reports\fg-3\desk-2026-10-11-ghost\env)
        --keep    a replay that lies in this tool's folder for the same library (by its SHA-256) is read, not made again
        --more    further variant libraries to print beside (one replay each)
        --variant the variant library judged (not given: env)
NOT A PROOF AND NOT A JUDGE: THE PICTURE IS THE OWNER'S TO JUDGE. It takes the card for many minutes: ONE GPU USER AT A TIME,
never beside the game (it waits while the game or another bench has the card). Its own folder is
%TEMP%\dlssiv-test-fg-env (the replay is built there as fgpipe-replay.exe, a name every proof's wait for the card
sees); nothing is written into a captured run. The last line is DONE, or FAIL where a replay did not run.
"""
import ctypes, importlib.util, os, re, subprocess, sys, tempfile, time

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
NOWIN = subprocess.CREATE_NO_WINDOW
def arg(name, dflt): return next((a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")), dflt)
RUNS = arg("--runs", os.path.join(tempfile.gettempdir(), "dlssiv-fg-runs"))
SHOW = arg("--out", os.path.join(ROOT, "docs", "reports", "fg-3", "desk-2026-10-11-ghost", "env"))
ONLY = [x for x in arg("--only", "").split(",") if x]
MORE = [x for x in arg("--more", "").split(",") if x]
VAR = arg("--variant", "env")
F0, F1 = [int(v) for v in arg("--frames", "8-15").split("-")]
SHEETF = [int(v) for v in arg("--sheet-frames", str(F1)).split(",")]
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-env")
REPLAY = os.path.join(OUT, "fgpipe-replay.exe")
def var(name): return os.path.join(ROOT, "obj", "dlssiv-fg-var", name, "dlssiv-fg-v-%s.dll" % name)
PRODUCT = os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll")
THREE = ("", "-again", "-third")
# (name, library, the first frame dumped): the first replay of the product and of the variant keeps the frames after the reset too
LIBS = [("product" + k, PRODUCT, 1 if not k else F0) for k in THREE] + [(VAR + k, var(VAR), 1 if not k else F0) for k in THREE]
LIBS += [("ghost", var("ghost"), F0), (VAR + "-gate", var(VAR + "-gate"), F0)]
LIBS += [("alone" + k, var("alone"), F0) for k in THREE] + [(VAR + "-alone" + k, var(VAR + "-alone"), F0) for k in THREE]
LIBS += [(m, var(m), F0) for m in MORE]
# THE BENCH, by a run's path under --runs. crops: the sheets (x, y, width, height in presented pixels, and the sheet's
# scale). bands: (a) and (b). broad: (c). road: (d). flip: the flip sequence's crop.
BENCH = {
    r"cap3\run-03": {"what": "SUNLIT, the chase camera, a hard shadow and passing shadows",
                     "crops": [("behind", (1900, 1400, 550, 280), 1.6), ("beside", (2450, 1300, 450, 500), 1.2), ("tyre", (2250, 1400, 350, 300), 2.0), ("shadow", (1300, 1380, 640, 400), 1.4), ("post", (2840, 300, 360, 300), 1.6)],
                     "bands": [("(a) the band under the car, columns 1960 to 2140, rows 1520 to 1552", 1960, 2140, 1520, 1552),
                               ("(a) the same at the sunlit side, columns 2200 to 2400, rows 1520 to 1552", 2200, 2400, 1520, 1552),
                               ("(a) THE HARD EDGE THAT MOVES, columns 2200 to 2400, rows 1572 to 1604 (its real is not the truth there)", 2200, 2400, 1572, 1604)],
                     "broad": [("behind", (1900, 1400, 550, 280)), ("beside", (2450, 1300, 450, 500))],
                     "road": [("the road beside the car", (2450, 1300, 450, 500)), ("the tree and the post", (2900, 250, 380, 380)), ("plain road", (900, 1750, 500, 300)), ("the car itself", (1750, 1050, 650, 300))],
                     "flip": (1940, 1375, 520, 300)},
    r"cap3\run-01": {"what": "SUNLIT, a three-quarter view",
                     "crops": [("back", (1250, 1200, 400, 400), 1.5), ("under", (1500, 1420, 700, 300), 1.3), ("tyre", (1280, 1330, 400, 320), 1.8)],
                     "broad": [("back", (1250, 1200, 400, 400))],
                     "road": [("the road before the car", (2600, 1500, 600, 400))],
                     "flip": (1300, 1330, 520, 300)},
    r"prof3\run-05": {"what": "RAIN, the profile, the tail light",
                      "crops": [("profile", (2650, 1150, 650, 400), 1.3), ("front", (1380, 1200, 600, 330), 1.4)],
                      "bands": [("(b) the strip behind the car, columns 2853 to 2914, rows 1330 to 1350", 2853, 2914, 1330, 1350)],
                      "broad": [("profile", (2650, 1150, 650, 400))],
                      "road": [("the ground below the car", (1700, 1600, 800, 300))],
                      "flip": (2680, 1180, 520, 300)},
    r"prof3\run-01": {"what": "RAIN, the profile from the other side",
                      "crops": [("back", (1150, 1130, 370, 400), 1.6), ("front", (2350, 1180, 600, 330), 1.4)],
                      "broad": [("back", (1150, 1130, 370, 400))],
                      "road": [("the ground below the car", (1700, 1600, 800, 300))],
                      "flip": (1100, 1180, 520, 300)},
    r"cap2\run-01": {"what": "SUNLIT, the chase camera, the shadow to the left",
                     "crops": [("back", (1380, 1230, 400, 400), 1.5), ("tyre", (2150, 1300, 420, 320), 1.8), ("shadow", (1000, 1350, 640, 400), 1.4)],
                     "broad": [("back", (1380, 1230, 400, 400))],
                     "road": [("the road right of the car", (2600, 1400, 600, 400))],
                     "flip": (1400, 1260, 520, 300)},
    r"cap2\run-03": {"what": "SUNLIT, the chase camera, close, a kerb",
                     "crops": [("back", (2100, 1420, 400, 330), 1.6), ("tyre", (2560, 1450, 420, 330), 1.8), ("left", (1600, 1350, 600, 400), 1.4)],
                     "broad": [("back", (2100, 1420, 400, 330))],
                     "road": [("the road left of the car", (900, 1500, 600, 400))],
                     "flip": (2300, 1430, 520, 300)},
    r"capw\run-03": {"what": "RAIN at dusk, a three-quarter view, THE WHEELS' KEY ON (the wheel carries the car body's vector)",
                     "crops": [("tyre", (1850, 1250, 560, 330), 1.5), ("back", (1250, 1300, 600, 400), 1.4)],
                     "broad": [("back", (1250, 1300, 600, 400))],
                     "road": [("the ground below the car", (1700, 1700, 800, 300))],
                     "flip": (1800, 1330, 520, 300)},
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
C.GPU_USERS = ("dlssiv-helper.exe", "fgpipe*.exe", "fgcost*.exe", "vklines*.exe", "fgreplay*.exe", "dt_fg_test*.exe")


def wait_card(what):
    waited = 0
    while (C.game_runs() or C.other_gpu_users()) and waited < 7200:
        if waited == 0: print("  the card is somebody else's (%s): %s waits, looking again every 5 s..." % ("the game" if C.game_runs() else "; ".join(C.other_gpu_users())[:160], what), flush=True)
        time.sleep(5); waited += 5
    if waited: print("  the card is free after %d s" % waited, flush=True)


def build_replay():
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 "%s" /Fe:fgpipe-replay.exe /Fo:fgpipe-replay.obj /link /INCREMENTAL:NO d3d12.lib dxgi.lib bcrypt.lib'
          % (C.vcvars(64), os.path.join(ROOT, "tools", "fg-replay", "fgreplay.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    return r.returncode == 0 and "warning C" not in r.stdout and os.path.exists(REPLAY)


def replay(run, tag, lib, name, first):
    out = os.path.join(OUT, tag, name)
    os.makedirs(out, exist_ok=True)
    mark = os.path.join(out, "library.txt"); said = os.path.join(out, "replay.txt"); sha = C.sha256_of(lib)
    have = "--keep" in ARGS and os.path.isfile(mark) and open(mark).read().strip() == "%s %d" % (sha, first) and os.path.isfile(said) and all(os.path.isfile(os.path.join(out, "f%02d-gen-replay.raw" % i)) for i in range(max(first, 1), F1 + 1))
    if have: text = open(said, encoding="utf-8", errors="replace").read()
    else:
        for n in os.listdir(out): os.remove(os.path.join(out, n))
        wait_card("the replay of %s through %s" % (tag, name))
        r = subprocess.run([REPLAY, "--run=" + run, "--lib=" + lib, "--out=" + out, "--expect=differ", "--dump=%d" % first], capture_output=True, text=True, errors="replace", creationflags=NOWIN, timeout=1800)
        text = r.stdout
        open(mark, "w").write("%s %d\n" % (sha, first))
    counts = {}
    for l in text.splitlines():
        m = re.match(r"frame (\d+) .*: (EQUAL|DIFFERS: (\d+) of)", l)
        if m: counts[int(m.group(1))] = 0 if m.group(2) == "EQUAL" else int(m.group(3))
    ok = len(counts) >= F1 + 1 and all(os.path.isfile(os.path.join(out, "f%02d-gen-replay.raw" % i)) for i in range(F0, F1 + 1))
    if not ok: failed.append("%s through %s: %s" % (tag, name, (text.strip().splitlines() or ["nothing said"])[-1][:200]))
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


def real(run, k): return pic(os.path.join(run, "f%02d-present.raw" % k))


def broad(run, i, gen, rect, rw, rh):
    """mock5.py's number: the mean and the 99th percentile, over the crop's fast background, of the 13 by 13 mean of the
    generated frame's brightness less the two real frames' mean brightness"""
    x, y, w, h = rect; m = 16
    B, A, G = [lum(f32(a, y - m, y + h + m, x - m, x + w + m)) for a in (real(run, i - 1), real(run, i), gen)]
    mv = np.fromfile(os.path.join(run, "f%02d-mv.raw" % i), dtype=np.float16).reshape(rh, rw, 2).astype(np.float32)
    ys = (np.arange(y, y + h) * rh // H)[:, None]; xs = (np.arange(x, x + w) * rw // W)[None, :]
    ground = np.hypot(mv[ys, xs, 0] * W / rw, mv[ys, xs, 1] * H / rh) > 8.0
    d = np.abs(boxsum(G - (B + A) / 2, 6) / 169.0)[m:-m, m:-m]
    return (float(d[ground].mean()), float(np.percentile(d[ground], 99))) if ground.any() else (float("nan"), float("nan"))


def band(run, i, gen, x0, x1, y0, y1):
    """mock5.py's row: the generated frame's brightness over the two real frames' mean, each the mean of the band's columns,
    a row a line: (least, mean, most)"""
    B = lum(f32(real(run, i - 1), y0, y1, x0, x1)).mean(axis=1); A = lum(f32(real(run, i), y0, y1, x0, x1)).mean(axis=1)
    r = lum(f32(gen, y0, y1, x0, x1)).mean(axis=1) / np.maximum((B + A) / 2, 1e-6)
    return float(r.min()), float(r.mean()), float(r.max())


def three(vals, fmt):
    """one number from three replays: once where they agree to the printed digit, else its span"""
    s = sorted(set(fmt % v for v in vals), key=float)
    return s[0] if len(s) == 1 else "%s to %s" % (s[0], s[-1])


def differ(a, b):
    """the map of the pixels of two whole images that differ in any channel"""
    d = np.zeros((H, W), dtype=bool)
    for y in range(0, H, 270): d[y:y + 270] = (np.asarray(a[y:y + 270, :, :3]) != np.asarray(b[y:y + 270, :, :3])).any(axis=2)
    return d


def codes(a, b, rect=None):
    """how far two images are apart, in codes of the 255 of the picture as shown, a pixel (the largest channel)"""
    x, y, w, h = rect if rect else (0, 0, W, H)
    c = np.zeros((h, w), dtype=np.float32)
    for yy in range(0, h, 270):
        c[yy:yy + 270] = np.abs(tone(f32(a, y + yy, min(y + yy + 270, y + h), x, x + w)) - tone(f32(b, y + yy, min(y + yy + 270, y + h), x, x + w))).max(axis=2)
    return c


def diff_png(a, b, path, keep):
    q = np.zeros((H // 4, W // 4), dtype=np.float32)
    for y in range(0, H, 216):
        c = np.abs(tone(np.asarray(a[y:y + 216, :, :3], dtype=np.float32)) - tone(np.asarray(b[y:y + 216, :, :3], dtype=np.float32))).max(axis=2) * keep[y:y + 216]
        q[y // 4:(y + 216) // 4] = c.reshape(54, 4, W // 4, 4).max(axis=(1, 3))
    Image.fromarray(np.clip(q * 8.0, 0, 255).astype(np.uint8), "L").save(path)


def font(size):
    try: return ImageFont.truetype("arialbd.ttf", size)
    except OSError: return ImageFont.load_default()


def tile(a, rect, scale):
    x, y, w, h = rect
    im = Image.fromarray((tone(f32(a, y, y + h, x, x + w)) + 0.5).astype(np.uint8))
    return im.resize((int(w * scale), int(h * scale)), Image.LANCZOS) if scale != 1.0 else im


def label(d, x, y, text, colour, f):
    box = d.textbbox((x, y), text, font=f)
    d.rectangle((box[0] - 6, box[1] - 4, box[2] + 6, box[3] + 4), fill=(0, 0, 0))
    d.text((x, y), text, fill=colour, font=f)


def sheet4(run, i, rect, scale, gen_product, gen_variant, title, path):
    """two by two with a large label on every tile: 1 the real frame before, 2 generated today, 3 the new library, 4 the real frame after"""
    scale = 640.0 / rect[2]         # (a tile is 640 pixels wide whatever the crop: the sheets are committed)
    tiles = [tile(real(run, i - 1), rect, scale), tile(gen_product, rect, scale), tile(gen_variant, rect, scale), tile(real(run, i), rect, scale)]
    tw, th = tiles[0].size
    big, head = font(24), font(20)
    sheet = Image.new("RGB", (tw * 2 + 4, th * 2 + 4 + 44), (20, 20, 20))
    d = ImageDraw.Draw(sheet)
    d.text((10, 7), title, fill=(255, 255, 255), font=head)
    names = [("1  REAL FRAME BEFORE", (255, 255, 255)), ("2  GENERATED TODAY (the product)", (255, 120, 120)), ("3  THE NEW LIBRARY (v-%s)" % VAR, (120, 255, 120)), ("4  REAL FRAME AFTER", (255, 255, 255))]
    for n, t in enumerate(tiles):
        x, y = (n % 2) * (tw + 4), 44 + (n // 2) * (th + 4)
        sheet.paste(t, (x, y))
        label(d, x + 10, y + th - 44, names[n][0], names[n][1], big)
    sheet.save(path)


def flip(run, rect, gp, gv, title, base, base_png):
    """the generated and the real frames in their shown order at one crop, the product left and the variant right"""
    x, y, w, h = rect
    big, head = font(18), font(18)
    frames = []
    for k in range(F0, F1 + 1):
        for kind in ("GENERATED", "REAL"):
            left = tile(gp(k) if kind == "GENERATED" else real(run, k), rect, 1.0); right = tile(gv(k) if kind == "GENERATED" else real(run, k), rect, 1.0)
            im = Image.new("RGB", (2 * w + 6, h + 40), (20, 20, 20)); d = ImageDraw.Draw(im)
            d.text((8, 8), title, fill=(255, 255, 255), font=head)
            im.paste(left, (0, 40)); im.paste(right, (w + 6, 40))
            col = (255, 220, 60) if kind == "GENERATED" else (255, 255, 255)
            label(d, 10, 40 + h - 30, "TODAY (the product): %s %d" % (kind, k), col, big)
            label(d, w + 16, 40 + h - 30, "THE NEW LIBRARY (v-%s): %s %d" % (VAR, kind, k), col, big)
            frames.append(im)
    frames[0].save(base_png + ".png", save_all=True, append_images=frames[1:], duration=400, loop=0)
    pal = [f.quantize(colors=255, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG) for f in frames]
    pal[0].save(base + ".gif", save_all=True, append_images=pal[1:], duration=400, loop=0)


def gate_map(gate):
    """the pixels the instrument painted: exactly 4 0 4"""
    m = np.zeros((H, W), dtype=bool)
    for y in range(0, H, 270):
        p = np.asarray(gate[y:y + 270, :, :3])
        m[y:y + 270] = (p[..., 0] == 4.0) & (p[..., 1] == 0.0) & (p[..., 2] == 4.0)
    return m


def main():
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: the replays inherit it
    os.makedirs(OUT, exist_ok=True); os.makedirs(SHOW, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(OUT, "result.txt"))
    t0 = time.time()
    missing = sorted({n.split("-again")[0].split("-third")[0] for n, p, f in LIBS if not os.path.isfile(p)})
    if missing:
        print("FAIL: not built: %s (build-ffx-own.ps1 -Target dlssiv-fg; python tools\\ffx-own\\build_variants.py ghost alone %s %s-gate %s-alone)" % (", ".join(missing), VAR, VAR, VAR)); return 1
    if not ("--keep" in ARGS and os.path.exists(REPLAY)) and not build_replay():
        print("FAIL: the replay did not compile"); return 1
    print("THE VARIANT v-%s ON THE CAPTURED RUNS under %s; generated frames %d to %d, each replayed from the run's reset; the sheets go to %s" % (VAR, RUNS, F0, F1, SHOW))
    for n, p, f in LIBS:
        if not n.endswith(("-again", "-third")): print("  %-11s %d bytes, SHA-256 %s" % (n, os.path.getsize(p), C.sha256_of(p)))
    runs = [r for r in BENCH if (not ONLY or r in ONLY or r.replace("\\", "-") in ONLY) and os.path.isfile(os.path.join(RUNS, r, "index.txt"))]
    if not runs:
        print("FAIL: no run of the bench under %s" % RUNS); return 1
    FR = list(range(F0, F1 + 1))
    for rel in runs:
        run = os.path.join(RUNS, rel); tag = rel.replace("\\", "-"); b = BENCH[rel]
        idx = open(os.path.join(run, "index.txt"), encoding="utf-8", errors="replace").read()
        rw, rh = [int(v) for v in re.search(r"(?m)^render: (\d+)x(\d+)", idx).groups()]
        print("\n" + "=" * 118 + "\n%s: %s (the capture's library: SHA-256 %s; HUD-less image: %s)" % (rel, b["what"], re.search(r"(?m)^library_sha256: (\w+)", idx).group(1)[:16], re.search(r"(?m)^hudless: (\d)", idx).group(1)))
        rep = {}
        for n, p, f in LIBS: rep[n] = replay(run, tag, p, n, f)
        if any(f.startswith(tag + " ") for f in failed):
            print("  A REPLAY DID NOT RUN: this run is left out"); continue
        g = lambda n, i: pic(os.path.join(rep[n][0], "f%02d-gen-replay.raw" % i))
        P3 = ["product" + k for k in THREE]; V3 = [VAR + k for k in THREE]
        shown = [("the product", P3), ("v-ghost", ["ghost"]), ("v-" + VAR, V3)] + [("v-" + m, [m]) for m in MORE]

        # ---- after a reset: frames 0 to 7 against the saved images
        print("  AFTER A RESET, pixels of a generated frame that differ from the SAVED one (of %d), frames 0 to 7: the product %s; v-%s %s" % (
            W * H, ", ".join(str(rep["product"][1].get(i)) for i in range(0, 8)), VAR, ", ".join(str(rep[VAR][1].get(i)) for i in range(0, 8))))
        early = []
        for i in range(1, min(F0, 8)):
            c = codes(g("product", i), g(VAR, i))
            early.append("%d: %d (%d by 4 codes, %d by 16)" % (i, int((c > 0).sum()), int((c >= 4).sum()), int((c >= 16).sum())))
        print("  AFTER A RESET, v-%s against the product, pixels that differ (one replay of each: the method's own race is in it), frame: %s" % (VAR, "; ".join(early)))

        # ---- the numbers, a frame a column
        for what, x0, x1, y0, y1 in b.get("bands", []):
            print("  %s: generated over real, a row, THE MEAN (least to most), frames %d to %d:" % (what, F0, F1))
            for name, libs in shown:
                cells = []
                for k in FR:
                    v = [band(run, k, g(n, k), x0, x1, y0, y1) for n in libs]
                    cells.append("%s (%s to %s)" % (three([q[1] for q in v], "%.2f"), three([q[0] for q in v], "%.2f"), three([q[2] for q in v], "%.2f")))
                print("      %-12s %s" % (name, " | ".join(cells)))
        for cname, rect in b.get("broad", []):
            print("  (c) THE BROAD DIFFERENCE FROM THE REAL FRAMES, crop %s %s, mean and 99th percentile, frames %d to %d:" % (cname, "%d,%d,%d,%d" % rect, F0, F1))
            for name, libs in shown:
                cells = []
                for k in FR:
                    v = [broad(run, k, g(n, k), rect, rw, rh) for n in libs]
                    cells.append("%s %s" % (three([q[0] for q in v], "%.4f"), three([q[1] for q in v], "%.4f")))
                print("      %-12s %s" % (name, " | ".join(cells)))

        # ---- the variant against the product: the steady pixels, and where it acts
        steady = {}
        for k in FR:
            s = np.ones((H, W), dtype=bool)
            for libs in (P3, V3):
                s &= ~differ(g(libs[0], k), g(libs[1], k)) & ~differ(g(libs[0], k), g(libs[2], k))
            steady[k] = s
        print("  (d) v-%s AGAINST THE PRODUCT, pixels that are the same in three replays of each and not the same in both: percent that change by 1, 4, 16 codes or more of the 255 as shown, and the most, frames %d to %d (the pixels not steady by the method's own race: %s percent of the picture):" % (
            VAR, F0, F1, three([100.0 * (1.0 - steady[k].mean()) for k in (F0, F1)], "%.3f")))
        for cname, rect in [("THE WHOLE FRAME", (0, 0, W, H))] + b.get("road", []):
            x, y, w, h = rect; cells = []
            for k in FR:
                c = codes(g("product", k), g(VAR, k), rect) * steady[k][y:y + h, x:x + w]
                cells.append("%.2f %.2f %.2f, %d" % (100.0 * (c >= 1).mean(), 100.0 * (c >= 4).mean(), 100.0 * (c >= 16).mean(), int(round(float(c.max())))))
            print("      %-26s %-20s %s" % (cname, "%d,%d,%d,%d" % rect, " | ".join(cells)))
        for m in MORE:
            for cname, rect in [("THE WHOLE FRAME", (0, 0, W, H))] + b.get("road", []):
                x, y, w, h = rect; cells = []
                for k in FR:
                    c = codes(g("product", k), g(m, k), rect)
                    cells.append("%.2f %.2f %.2f, %d" % (100.0 * (c >= 1).mean(), 100.0 * (c >= 4).mean(), 100.0 * (c >= 16).mean(), int(round(float(c.max())))))
                print("      v-%s (one replay of each, the race in it) %-26s %s" % (m, cname, " | ".join(cells)))
        A3 = ["alone" + k for k in THREE]; E3 = [VAR + "-alone" + k for k in THREE]
        cells = []
        for k in FR:
            gm = gate_map(g(VAR + "-gate", k))
            ch = differ(g(A3[0], k), g(E3[0], k))
            for libs in (A3, E3):
                ch &= ~differ(g(libs[0], k), g(libs[1], k)) & ~differ(g(libs[0], k), g(libs[2], k))
            out = ch & ~gm
            far = int((out & ~(boxsum(gm.astype(np.int32), 2) > 0)).sum()) if out.any() else 0
            big = int((codes(g(A3[0], k), g(E3[0], k)) * out >= 1).sum()) if out.any() else 0
            cells.append("%d changed, %d outside the map (%d of them by a code or more, %d farther than 2 pixels from it), the map %.2f%%" % (int(ch.sum()), int(out.sum()), big, far, 100.0 * gm.mean()))
        print("  WHERE IT ACTS. With nothing inpainted in either, the pixels v-%s changed (the same in three replays of each library, not the same in both), how many of them lie OUTSIDE the map the instrument painted (the map is ANOTHER library's replay: a pixel at the very edge of the dead band may be painted in the one and relit by a hair in the other; so, of those: how many change by one code of the 255 or more, and how many lie farther than 2 pixels from the map), and the map's share of the picture, frames %d to %d:" % (VAR, F0, F1))
        print("      " + " | ".join(cells))
        if "--no-pictures" in ARGS: continue

        # ---- the pictures
        for i in SHEETF:
            for cname, rect, scale in b["crops"]:
                p = os.path.join(SHOW, "%s-f%02d-%s.png" % (tag, i, cname))
                sheet4(run, i, rect, scale, g("product", i), g(VAR, i), "%s, generated frame %d, %s (display %d,%d, %d by %d)" % ((rel, i, cname) + rect), p)
                print("  sheet: %s" % p)
        i = F1
        p = os.path.join(SHOW, "%s-f%02d-difference.png" % (tag, i))
        diff_png(g("product", i), g(VAR, i), p, steady[i])
        print("  the whole frame's difference, v-%s against the product, frame %d (a quarter of the size, times 8; the pixels not steady are left black): %s" % (VAR, i, p))
        p = os.path.join(SHOW, "%s-f%02d-acts.png" % (tag, i))
        Image.fromarray((gate_map(g(VAR + "-gate", i)).reshape(H // 4, 4, W // 4, 4).max(axis=(1, 3)) * 255).astype(np.uint8), "L").save(p)
        print("  where it acts, frame %d (a quarter of the size; white: a sample was relit): %s" % (i, p))
        base = os.path.join(SHOW, "%s-flip-f%02d-f%02d" % (tag, F0, F1))
        base_png = os.path.join(OUT, "%s-flip-f%02d-f%02d" % (tag, F0, F1))
        flip(run, b["flip"], lambda k: g("product", k), lambda k: g(VAR, k), "%s: frames %d to %d as shown, at %d,%d" % ((rel, F0, F1) + b["flip"][:2]), base, base_png)
        print("  the flip sequence (generated %d, real %d, generated %d, ... real %d; the product left, v-%s right; 0.4 s a frame): %s.gif (and, in full colour and not committed, %s.png)" % (F0, F0, F0 + 1, F1, VAR, base, base_png))
    for f in failed: print("  failed: " + f)
    print("%s: %d run(s), in %.0f s; the replays are kept under %s" % ("DONE" if not failed else "FAIL (%d replay(s) did not run)" % len(failed), len(runs), time.time() - t0, OUT))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
