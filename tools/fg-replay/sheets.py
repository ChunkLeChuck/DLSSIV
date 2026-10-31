r"""THE SHEETS: A CAPTURED RUN AS PICTURES A PERSON CAN JUDGE (2026-10-10, frame generation phase 3, after first light).

A run is what the development helper saved on a leg (host\fg_cap.h): for each of its frames the presented picture, the
generated image, the depth and the motion vectors, whole and raw, and an index. This turns one run, or two side by
side, into PNG files and tables. It needs numpy and Pillow and nothing of the helper's. It uses no graphics card.

    python tools\fg-replay\sheets.py --run=<a run's folder> --out=<folder> [options]

WHAT IT WRITES
  overview-fNN.png        the last chosen frame's real picture at a quarter of its size with every crop's rectangle and
                          every line drawn on it and named: the place to look first, and to aim the crops from
  sheet-<crop>.png        ONE SHEET A CROP. A row a chosen frame: the real frame before, THE GENERATED FRAME, the real
                          frame after, THE DIFFERENCE of the generated frame against the half-way blend of the two real
                          ones, and the frame's motion vectors drawn as arrows over the real picture. Magnified by whole
                          pixels (no smoothing). With --beside the second run's rows stand under the first's, the same
                          crop, each block headed by ITS setting as the mod itself said it (mv_glass, sharpness)
  line-<name>.txt         THE VECTORS ALONG A LINE, as numbers, a row a pixel of the line: the vector, the depth and the
                          brightness of the real and of the generated picture there; under it, where each of them steps
                          most, so that it can be read whether the vectors' step lies on the picture's edge or beside it
  with --full, at the full size:
  tNNN-real-fNN.png, tNNN-gen-fNN.png   every chosen generated frame and the real frames round it. SORTED BY NAME THEY ARE
                          IN THE ORDER OF TIME: the real frame before, the generated frame, the real frame after
  vectors-fNN.png         the motion vectors as a picture: the colour is the direction (red right, green-yellow down,
                          cyan left, violet up: the wheel in the corner says it), the brightness the length, full at
                          --mvfull pixels (16 when not given); black is no motion
  depth-fNN.png           the depth as greys: the nearest white, the farthest black, between the 1st and the 99th
                          percentile of the frame (the line printed says the two values)
  debugview-fNN.png       with --replay=<folder of a replay run with --debug>: AMD's own debug drawing, as the library
                          drew it into the generated image (fgreplay --debug=0x400 --dump=K)

THE TONE CURVE, ONE AND FIXED, for a picture of scRGB values (FP16; 1.0 is 80 nits): per channel
t = x * (1 + x / 156.25) / (1 + x), then the sRGB encode to 8 bits; negatives are 0; 12.5 (1000 nits) and above is white.
It was the crop saver's own (host\fg_stills.h, out in round 8's deep strip): 0.18 lands at code 109, 1.0 at 188, 5.0 at 239. THE DIFFERENCE is taken on
the scRGB values, its size times --gain (4 when not given), then through the same curve: black is no difference.
An 8-bit picture is shown as its own bytes.

OPTIONS
  --frames=A-B            the frames whose GENERATED image is shown (a generated frame i lies between the real frames
                          i-1 and i). Not given: the run's steady frames (the index says from which), or its last four
  --crop=name:x,y,w,h     a crop, in pixels of the presented picture; as many as wanted. Not given: THE THREE OF THE CHASE
                          CAMERA, as shares of the picture (they are aimed by eye at where a car sits in GTA IV's chase
                          camera and were never held against a real frame: look at the overview, then aim them):
                          cabin (the cabin through the rear glass), wheel (a rear wheel), bumper (the road right under
                          the rear bumper, the bumper's lower edge in the crop)
  --line=name:x0,y0,x1,y1 a line, in pixels of the presented picture. Not given, with the default crops: one down the
                          middle of the bumper crop, from the car onto the road
  --at=x,y                the vector, the depth and the brightness under one pixel, a line a frame
  --beside=<run folder>   a second run (the other setting of the same drive): its rows under the first's on every sheet
  --replay=<folder>       fNN-gen-replay.raw files of a replay: shown as debugview-fNN.png (with --full)
  --scale=N               the magnification of a sheet's crops (not given: as large as fits 768 pixels a picture)
  --gain=G, --mvfull=P, --full
The last line is PASS, or FAIL with what was missing.
"""
import math, os, sys

import numpy as np
from PIL import Image, ImageDraw

ARGS = sys.argv[1:]


def arg(name, dflt=None):
    for a in ARGS:
        if a.startswith(name + "="): return a.split("=", 1)[1]
    return dflt


def args(name): return [a.split("=", 1)[1] for a in ARGS if a.startswith(name + "=")]


def read_index(run):
    p = os.path.join(run, "index.txt")
    kv, frames, files, ended = {}, [], {}, False
    for l in open(p, encoding="utf-8", errors="replace").read().splitlines():
        if l == "end": ended = True
        elif l.startswith("frame: "): frames.append(dict(t.split("=", 1) for t in l[7:].split(" ") if "=" in t))
        elif l.startswith("file: "):
            name, kind, frame, w, h, fmt, bpp, nbytes = l[6:].split(" ")
            files[(int(frame), kind)] = (name, int(w), int(h), int(fmt), int(bpp), int(nbytes))
        elif ": " in l and not l.startswith("frame_plain: "): k, v = l.split(": ", 1); kv[k] = v
    if not ended: raise SystemExit("FAIL: %s has no end line: the run is not whole" % p)
    return {"dir": run, "kv": kv, "frames": frames, "files": files}


def load(ix, frame, kind):
    """A raw picture as floats: (h, w, 3) linear values for a colour picture, (h, w, 2) for the vectors, (h, w) for the depth; and whether it is scRGB."""
    name, w, h, fmt, bpp, nbytes = ix["files"][(frame, kind)]
    return load_raw(os.path.join(ix["dir"], name), w, h, fmt)


def load_raw(path, w, h, fmt):
    if fmt == 10:   # R16G16B16A16_FLOAT
        return np.fromfile(path, dtype=np.float16).reshape(h, w, 4)[..., :3].astype(np.float32), True
    if fmt == 34:   # R16G16_FLOAT
        return np.fromfile(path, dtype=np.float16).reshape(h, w, 2).astype(np.float32), False
    if fmt == 41:   # R32_FLOAT
        return np.fromfile(path, dtype=np.float32).reshape(h, w), False
    if fmt in (28, 29):   # R8G8B8A8
        return np.fromfile(path, dtype=np.uint8).reshape(h, w, 4)[..., :3].astype(np.float32) / 255.0, False
    if fmt in (87, 88, 91):   # B8G8R8A8
        return np.fromfile(path, dtype=np.uint8).reshape(h, w, 4)[..., 2::-1].astype(np.float32) / 255.0, False
    if fmt == 24:   # R10G10B10A2
        v = np.fromfile(path, dtype=np.uint32).reshape(h, w)
        return np.stack([(v & 1023), ((v >> 10) & 1023), ((v >> 20) & 1023)], axis=-1).astype(np.float32) / 1023.0, False
    raise SystemExit("FAIL: a picture of format %d is not one the sheets read" % fmt)


def tone(x, hdr):
    """Linear values to 8 bits: the one fixed curve for scRGB (the head says it), the picture's own bytes otherwise."""
    if not hdr: return np.clip(np.nan_to_num(x) * 255.0 + 0.5, 0, 255).astype(np.uint8)
    x = np.nan_to_num(np.maximum(x, 0.0), nan=0.0, posinf=1e6)
    t = np.minimum(x * (1.0 + x / 156.25) / (1.0 + x), 1.0)
    s = np.where(t <= 0.0031308, 12.92 * t, 1.055 * np.power(np.maximum(t, 1e-12), 1.0 / 2.4) - 0.055)
    return np.clip(s * 255.0 + 0.5, 0, 255).astype(np.uint8)


def luma(x): return 0.2126 * x[..., 0] + 0.7152 * x[..., 1] + 0.0722 * x[..., 2]


def mv_picture(mv, full):
    """The vectors as colours: hue the direction, brightness the length (full at `full` pixels)."""
    ang = np.arctan2(mv[..., 1], mv[..., 0])                        # 0 right, +90 degrees down (y grows downward)
    length = np.hypot(mv[..., 0], mv[..., 1])
    hgt = (ang / (2.0 * math.pi)) % 1.0 * 6.0
    k = np.clip(length / full, 0.0, 1.0)
    f = hgt - np.floor(hgt); i = np.floor(hgt).astype(np.int32) % 6
    q, t0 = 1.0 - f, f
    r = np.choose(i, [1.0, q, 0.0, 0.0, t0, 1.0]); g = np.choose(i, [t0, 1.0, 1.0, q, 0.0, 0.0]); b = np.choose(i, [0.0, 0.0, t0, 1.0, 1.0, q])
    return (np.stack([r, g, b], axis=-1) * k[..., None] * 255.0 + 0.5).astype(np.uint8)


def wheel(size, full):
    """The legend: the same colours on a disc, the length growing to `full` at its rim."""
    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    c = (size - 1) / 2.0
    v = np.stack([(x - c) / c * full, (y - c) / c * full], axis=-1)
    pic = mv_picture(v, full)
    pic[np.hypot(x - c, y - c) > c] = 32
    return pic


def depth_picture(d):
    lo, hi = np.percentile(d, 1.0), np.percentile(d, 99.0)
    if hi <= lo: hi = lo + 1e-6
    g = np.clip((d - lo) / (hi - lo), 0.0, 1.0)
    return (np.sqrt(g) * 255.0 + 0.5).astype(np.uint8), float(lo), float(hi)


def big(a, s): return np.repeat(np.repeat(a, s, axis=0), s, axis=1)


def save(path, a): Image.fromarray(a).save(path, compress_level=1)


def crop_of(a, r): x, y, w, h = r; return a[y:y + h, x:x + w]


def render_xy(ix, x, y, w, h):
    """A pixel of the presented picture as a pixel of the guides (the vectors and the depth are at the render size)."""
    _, rw, rh, _, _, _ = ix["files"][(0, "mv")]
    return min(rw - 1, int(x * rw / w)), min(rh - 1, int(y * rh / h)), rw, rh


def arrows(img, mv, r, s, sx, sy, step=16):
    """The vectors of a crop as arrows on its magnified picture: one every `step` presented pixels, drawn from where the
    pixel is to where the vector says it came from or goes to, in presented pixels times the magnification."""
    d = ImageDraw.Draw(img)
    x0, y0, w, h = r
    for cy in range(step // 2, h, step):
        for cx in range(step // 2, w, step):
            rx, ry = min(mv.shape[1] - 1, int((x0 + cx) / sx)), min(mv.shape[0] - 1, int((y0 + cy) / sy))
            vx, vy = float(mv[ry, rx, 0]) * sx, float(mv[ry, rx, 1]) * sy          # in presented pixels
            px, py = cx * s, cy * s
            d.line([(px, py), (px + vx * s, py + vy * s)], fill=(255, 255, 0), width=1)
            d.ellipse([px - 1, py - 1, px + 1, py + 1], fill=(255, 64, 64))
    return img


def setting(ix):
    kv = ix["kv"]
    return "mv_glass %s, sharpness %s" % (kv.get("mv_glass", "?"), kv.get("sharpness", "?"))


def pick_frames(ix):
    n = len(ix["frames"])
    f = arg("--frames")
    if f:
        a, b = (f.split("-") + [f])[:2]
        out = [i for i in range(int(a), int(b) + 1) if 1 <= i < n]
    else:
        out = [i for i in range(1, n) if int(ix["frames"][i]["since"]) >= int(ix["kv"].get("steady_from", "12"))][-4:] or list(range(max(1, n - 4), n))
    if not out: raise SystemExit("FAIL: no frame of the run is chosen (a generated frame needs the real frame before it: 1 to %d)" % (n - 1))
    return out


def main():
    run, out = arg("--run"), arg("--out")
    if not run or not out:
        print(__doc__); print("FAIL: --run and --out are needed"); return 2
    os.makedirs(out, exist_ok=True)
    ix = read_index(run)
    other = read_index(arg("--beside")) if arg("--beside") else None
    frames = pick_frames(ix)
    _, W, H, fmt, _, _ = ix["files"][(0, "present")]
    gain, mvfull, full = float(arg("--gain", "4")), float(arg("--mvfull", "16")), "--full" in ARGS
    crops = {}
    for c in args("--crop"):
        name, r = c.split(":", 1); crops[name] = tuple(int(v) for v in r.split(","))
    lines = {}
    for c in args("--line"):
        name, r = c.split(":", 1); lines[name] = tuple(int(v) for v in r.split(","))
    if not crops:
        # THE CHASE CAMERA'S THREE, as shares of the picture: aimed by eye, never held against a real frame (the head says so)
        crops = {"cabin": (int(0.40 * W), int(0.44 * H), int(0.20 * W), int(0.14 * H)), "wheel": (int(0.36 * W), int(0.60 * H), int(0.10 * W), int(0.12 * H)),
                 "bumper": (int(0.40 * W), int(0.62 * H), int(0.20 * W), int(0.14 * H))}
        if not lines: lines = {"bumper": (W // 2, int(0.62 * H), W // 2, int(0.76 * H) - 1)}
    for name, (x, y, w, h) in crops.items():
        if x < 0 or y < 0 or w < 8 or h < 8 or x + w > W or y + h > H: raise SystemExit("FAIL: the crop %s (%d,%d,%d,%d) does not lie in the picture of %dx%d" % (name, x, y, w, h, W, H))
    print("the sheets of %s" % run)
    print("  the run: %d frames, %dx%d format %d, captured by %s; the library %s" % (len(ix["frames"]), W, H, fmt, ix["kv"].get("helper_build", "?"), ix["kv"].get("library_sha256", "(not named)")))
    print("  THE SETTING, in the mod's own words: %s (%s)" % (ix["kv"].get("mod_cfg_line", "?"), setting(ix)))
    if other: print("  BESIDE IT: %s: %s (%s)" % (other["dir"], other["kv"].get("mod_cfg_line", "?"), setting(other)))
    print("  the frames whose generated image is shown: %s; the tone curve: t = x (1 + x / 156.25) / (1 + x), then sRGB (scRGB pictures); the difference times %g" % (", ".join(str(i) for i in frames), gain))
    made = []

    # ---- every chosen frame's pictures, loaded once
    def pictures(x, i):
        prev, hdr = load(x, i - 1, "present"); cur, _ = load(x, i, "present"); gen, _ = load(x, i, "gen")
        mv, _ = load(x, i, "mv"); dep, _ = load(x, i, "depth")
        return {"i": i, "prev": prev, "cur": cur, "gen": gen, "mv": mv, "depth": dep, "hdr": hdr, "n": x["frames"][i]["n"], "since": x["frames"][i]["since"], "reset": x["frames"][i]["disp_reset"] == "1"}
    rows = [(ix, pictures(ix, i)) for i in frames]
    if other:
        of = [i for i in frames if i < len(other["frames"])]
        rows += [(other, pictures(other, i)) for i in of]
    rw, rh = rows[0][1]["mv"].shape[1], rows[0][1]["mv"].shape[0]
    sx, sy = W / rw, H / rh

    # ---- the overview: where the crops and the lines lie
    last = rows[len(frames) - 1][1]
    ov = Image.fromarray(tone(last["cur"][::4, ::4], last["hdr"]))
    d = ImageDraw.Draw(ov)
    for name, (x, y, w, h) in crops.items():
        d.rectangle([x // 4, y // 4, (x + w) // 4, (y + h) // 4], outline=(255, 255, 0), width=2); d.text((x // 4 + 3, y // 4 + 2), name, fill=(255, 255, 0))
    for name, (x0, y0, x1, y1) in lines.items():
        d.line([(x0 // 4, y0 // 4), (x1 // 4, y1 // 4)], fill=(255, 64, 255), width=2); d.text((x1 // 4 + 4, y1 // 4 - 10), "line " + name, fill=(255, 64, 255))
    p = os.path.join(out, "overview-f%02d.png" % last["i"]); ov.save(p, compress_level=1); made.append(p)

    # ---- one sheet a crop
    for name, r in crops.items():
        x, y, w, h = r
        s = int(arg("--scale", "0")) or max(1, min(8, 768 // w))
        cw, ch, pad, top = w * s, h * s, 6, 34
        cols = ["the real frame before", "THE GENERATED FRAME", "the real frame after", "the difference against the half-way blend, x%g" % gain, "the vectors of the frame after (arrows: x%d)" % s]
        sheet = Image.new("RGB", (len(cols) * (cw + pad) + pad, 46 + len(rows) * (ch + top)), (24, 24, 24))
        dr = ImageDraw.Draw(sheet)
        dr.text((pad, 4), "crop %s: %d,%d %dx%d of %dx%d, magnified x%d. Tone curve t = x(1 + x/156.25)/(1 + x), then sRGB. Arrows: the frame's vector at every 16th pixel, in presented pixels." % (name, x, y, w, h, W, H, s), fill=(255, 255, 255))
        yy = 22
        for k, (src, pc) in enumerate(rows):
            if k == 0 or (other and k == len(frames)):
                dr.text((pad, yy), "RUN %s: %s; %s" % (os.path.basename(os.path.dirname(src["dir"])) + "\\" + os.path.basename(src["dir"]), setting(src), src["kv"].get("mod_cfg_line", "")[:150]), fill=(255, 220, 96)); yy += 16
            blend = 0.5 * (crop_of(pc["prev"], r) + crop_of(pc["cur"], r))
            cells = [tone(crop_of(pc["prev"], r), pc["hdr"]), tone(crop_of(pc["gen"], r), pc["hdr"]), tone(crop_of(pc["cur"], r), pc["hdr"]),
                     tone(np.abs(crop_of(pc["gen"], r) - blend) * gain, pc["hdr"]), tone(crop_of(pc["cur"], r), pc["hdr"])]
            for c, cell in enumerate(cells):
                im = Image.fromarray(big(cell, s))
                if c == 4: im = arrows(im, pc["mv"], r, s, sx, sy)
                x0 = pad + c * (cw + pad)
                sheet.paste(im, (x0, yy + 16))
                what = ["frame %d (pair %s)" % (pc["i"] - 1, src["frames"][pc["i"] - 1]["n"]), "GENERATED for frame %d, %s since the reset%s" % (pc["i"], pc["since"], " (A RESET FRAME: a copy)" if pc["reset"] else ""),
                        "frame %d (pair %s)" % (pc["i"], pc["n"]), "", ""][c]
                dr.text((x0, yy), (cols[c] + ": " + what) if what else cols[c], fill=(220, 220, 220))
            yy += ch + top
        p = os.path.join(out, "sheet-%s.png" % name); sheet.save(p, compress_level=1); made.append(p)
        print("  sheet-%s.png: %d row(s) of %d pictures, the crop %d,%d %dx%d magnified x%d" % (name, len(rows), len(cols), x, y, w, h, s))

    # ---- the vectors as numbers
    for xy in args("--at"):
        ax, ay = (int(v) for v in xy.split(","))
        for src, pc in rows:
            rx, ry, _, _ = render_xy(src, ax, ay, W, H)
            v = pc["mv"][ry, rx]
            print("  at %d,%d of frame %d (pair %s, %s): the guides' pixel %d,%d: vector %.3f %.3f px at the render size (%.3f %.3f px of the picture), depth %.6f, brightness real %.4f generated %.4f" % (
                ax, ay, pc["i"], pc["n"], setting(src), rx, ry, v[0], v[1], v[0] * sx, v[1] * sy, pc["depth"][ry, rx], luma(pc["cur"][ay, ax]), luma(pc["gen"][ay, ax])))
    for name, (x0, y0, x1, y1) in lines.items():
        n = max(abs(x1 - x0), abs(y1 - y0)) + 1
        text = []
        for src, pc in rows:
            text.append("THE LINE %s: %d,%d to %d,%d (%d pixels of the picture), frame %d (pair %s), %s since the reset; %s" % (name, x0, y0, x1, y1, n, pc["i"], pc["n"], pc["since"], setting(src)))
            text.append("  the vector is in pixels AT THE RENDER SIZE (%dx%d; one is %.3f x %.3f pixels of the picture); the brightness is the luminance of the scRGB values" % (rw, rh, sx, sy))
            text.append("    i     x     y      mv x      mv y    length       depth      real       gen")
            tab = []
            for k in range(n):
                px, py = x0 + (x1 - x0) * k // max(1, n - 1), y0 + (y1 - y0) * k // max(1, n - 1)
                rx, ry, _, _ = render_xy(src, px, py, W, H)
                v = pc["mv"][ry, rx]
                tab.append((k, px, py, float(v[0]), float(v[1]), float(np.hypot(v[0], v[1])), float(pc["depth"][ry, rx]), float(luma(pc["cur"][py, px])), float(luma(pc["gen"][py, px]))))
                text.append("  %3d %5d %5d %9.3f %9.3f %9.3f %11.6f %9.4f %9.4f" % tab[-1])
            if n > 1:
                step = lambda col: max(range(n - 1), key=lambda k: abs(tab[k + 1][col] - tab[k][col]))
                kv, kd, kr = max(range(n - 1), key=lambda k: math.hypot(tab[k + 1][3] - tab[k][3], tab[k + 1][4] - tab[k][4])), step(6), step(7)
                dv = math.hypot(tab[kv + 1][3] - tab[kv][3], tab[kv + 1][4] - tab[kv][4])
                text.append("  THE LARGEST STEPS ALONG IT: the vectors' between rows %d and %d (%.3f px at the render size); the depth's between rows %d and %d; the real picture's brightness between rows %d and %d." % (kv, kv + 1, dv, kd, kd + 1, kr, kr + 1))
                text.append("  SO: the vectors' step lies %d pixel(s) of the picture from the brightness's step and %d from the depth's (0: on it; the guides' own pixel is %.1f of the picture's, so less than that says nothing)%s" % (
                    abs(kv - kr), abs(kv - kd), max(sx, sy), "" if dv > 0.05 else ". THE VECTORS DO NOT STEP ALONG THIS LINE (under 0.05 px): nothing to place"))
            text.append("")
        p = os.path.join(out, "line-%s.txt" % name)
        open(p, "w", encoding="ascii", newline="").write("\r\n".join(text) + "\r\n")
        for l in text: print(l)

    # ---- the full-size pictures
    if full:
        done = set()
        for src, pc in rows[:len(frames)]:
            i = pc["i"]
            for j, key in ((i - 1, "prev"), (i, "cur")):
                if j in done: continue
                done.add(j)
                p = os.path.join(out, "t%03d-real-f%02d.png" % (2 * j + 1, j)); save(p, tone(pc[key], pc["hdr"])); made.append(p)
            p = os.path.join(out, "t%03d-gen-f%02d.png" % (2 * i, i)); save(p, tone(pc["gen"], pc["hdr"])); made.append(p)
            v = Image.fromarray(mv_picture(pc["mv"], mvfull)).resize((W, H), Image.NEAREST)
            v.paste(Image.fromarray(wheel(160, mvfull)), (16, 16))
            ImageDraw.Draw(v).text((16, 180), "the vectors of frame %d: the colour is the direction, the brightness the length, full at %g px (render size); the disc is the key" % (i, mvfull), fill=(255, 255, 255))
            p = os.path.join(out, "vectors-f%02d.png" % i); v.save(p, compress_level=1); made.append(p)
            g, lo, hi = depth_picture(pc["depth"])
            p = os.path.join(out, "depth-f%02d.png" % i); Image.fromarray(g).resize((W, H), Image.NEAREST).save(p, compress_level=1); made.append(p)
            print("  frame %d: the vectors are %.3f px long in the middle and %.3f at the 99th percentile; the depth's greys run from %.6f (black) to %.6f (white)" % (
                i, float(np.median(np.hypot(pc["mv"][..., 0], pc["mv"][..., 1]))), float(np.percentile(np.hypot(pc["mv"][..., 0], pc["mv"][..., 1]), 99.0)), lo, hi))
        rp = arg("--replay")
        if rp:
            for n in sorted(os.listdir(rp)):
                if not n.endswith("-gen-replay.raw"): continue
                a, hdr = load_raw(os.path.join(rp, n), W, H, fmt)
                p = os.path.join(out, "debugview-%s.png" % n[:3]); save(p, tone(a, hdr)); made.append(p)
    missing = [p for p in made if not os.path.exists(p) or os.path.getsize(p) < 100]
    print("  %d picture(s) written into %s" % (len(made), out))
    print("PASS" if made and not missing else "FAIL: %s" % missing)
    return 0 if made and not missing else 1


if __name__ == "__main__":
    sys.exit(main())
