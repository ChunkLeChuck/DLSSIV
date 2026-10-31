r"""THE VARIANT LIBRARIES, PRICED AND HELD AGAINST THE PRODUCT'S PICTURE (2026-10-11; the cuts that CHANGE THE PICTURE,
made ready for the owner's eye and never put into the product: ffx-own\cuts\CUTS.txt, the state "picture";
tools\ffx-own\build_variants.py builds them into obj\dlssiv-fg-var).

A variant saves microseconds by generating ANOTHER picture than the product's. Whether that picture is still right is
judged by the owner on real frames (tools\fg-replay). This script says, at a desk, the two things that judgement needs
beside it: WHAT A VARIANT SAVES and HOW MUCH OF THE PICTURE IT CHANGES ON THE JUDGE'S SYNTHETIC PAIRS.

  THE PICTURE   the judge's own hash sessions (tools\test-fg-own\run.py: the cost bench's moving picture, 3840x2160
                FP16, a two-image set; QF Quality's guides and DF DLAA's), the product and each variant in turn:
                how many of the hashed pairs differ, by class; and for three steady pairs (arm F twice, arm FC once)
                the two WHOLE images: how many pixels differ at all, how many by 1, 4 and 16 codes or more of the 255
                of the picture as shown (the sheets' own tone curve, tools\fg-replay\sheets.py), the largest and the
                mean difference, and WHERE (a map of the picture's 16 by 9 blocks; and a picture of the difference, a
                quarter of the size, times 8, beside the variant: diff-<configuration>-<arm><pair>.png)
  THE PRICE     tools\test-fg-own\passes.py --cut, the product BEFORE and the variant AFTER, on the product's shape:
                the steady pair by pass and, with the stamps off, the pair's outer span, which judges the saving
                (its own picture control fails for a variant, as it must: the variant's images are not the product's)

    python tools\test-fg-own\variants.py                       every variant: the picture, then the price
    python tools\test-fg-own\variants.py --only=guide-cap,all  the named ones
    ... --no-price / --no-picture                              one half alone
    ... --stamped                                              the price by the stamped sessions alone (half the time;
                                                               the outer span with the stamps off is then not taken)
    python tools\test-fg-own\variants.py --runs=<folder>       THE VARIANTS ON THE GAME'S OWN FRAMES: every captured run
                                                               under the folder, replayed through each variant, as sheets
                                                               with the product's rows above and the variant's under them
                                                               (under %TEMP%\dlssiv-fg-variants-real; the replay must be
                                                               built: python tools\fg-replay\run.py --keep). With --only,
                                                               and with the sheets' --crop, --frames, --gain, --scale
The last line is DONE with the count of variants, or FAIL where a session did not run. NOT A PROOF AND NOT A JUDGE: a
variant's picture is the owner's to judge. It takes the card for minutes: ONE GPU USER AT A TIME, as every bench here.
Its folder is %TEMP%\dlssiv-test-fg-variants; what it printed is kept there as result.txt, and each variant's price
table and difference pictures beside the variant's library.
"""
import importlib.util, os, re, shutil, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ARGS = sys.argv[1:]
ONLY = next((a.split("=", 1)[1].split(",") for a in ARGS if a.startswith("--only=")), None)
KEYS = [("F", 30), ("F", 44), ("FC", 30)]      # three steady pairs whose whole images are compared
ARMS = "F,FK,FR,F1,FN,FC"                      # the judge's arms up to the last one named, so that every pair meets the judge's own history
W, H = 3840, 2160


def load(name, path, argv):
    saved = sys.argv
    sys.argv = [saved[0]] + argv
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    sys.argv = saved
    return m


def codes(x):
    """scRGB values as the 8-bit codes of the picture as the sheets show it (tools\\fg-replay\\sheets.py's one tone curve)"""
    import numpy as np
    x = np.clip(x, 0.0, None)
    t = np.clip(x * (1.0 + x / 156.25) / (1.0 + x), 0.0, 1.0)
    s = np.where(t <= 0.0031308, 12.92 * t, 1.055 * np.power(t, 1.0 / 2.4) - 0.055)
    return np.rint(255.0 * s)


def whole(pa, pb, png):
    """two whole FP16 images -> what differs, in words, and a picture of it"""
    import numpy as np
    from PIL import Image
    a = np.fromfile(pa, dtype=np.float16).reshape(H, W, 4)[..., :3].astype(np.float32)
    b = np.fromfile(pb, dtype=np.float16).reshape(H, W, 4)[..., :3].astype(np.float32)
    any_ = (a != b).any(axis=2)
    d = np.abs(codes(a) - codes(b)).max(axis=2)
    n = W * H
    grid = []
    for by in range(9):
        row = ""
        for bx in range(16):
            blk = d[by * H // 9:(by + 1) * H // 9, bx * W // 16:(bx + 1) * W // 16]
            share = float((blk >= 4).mean())
            row += "." if share == 0.0 else "+" if share < 0.01 else "o" if share < 0.10 else "#"
        grid.append(row)
    q = d.reshape(H // 4, 4, W // 4, 4).max(axis=(1, 3))
    Image.fromarray(np.clip(q * 8.0, 0, 255).astype(np.uint8), "L").save(png)
    c = lambda k: int((d >= k).sum())
    return {"any": int(any_.sum()), "c1": c(1), "c4": c(4), "c16": c(16), "max": int(d.max()), "mean": float(d[d >= 1].mean()) if c(1) else 0.0, "grid": grid,
            "words": "%d of %d pixels differ at all (%.2f%%); by 1 code or more of 255 as shown %.3f%%, by 4 or more %.3f%%, by 16 or more %.4f%%; the largest %d codes, the mean of those that differ by a code %.1f" % (
                int(any_.sum()), n, 100.0 * any_.sum() / n, 100.0 * c(1) / n, 100.0 * c(4) / n, 100.0 * c(16) / n, int(d.max()), float(d[d >= 1].mean()) if c(1) else 0.0)}


def real(C, B, names, folder):
    """THE VARIANTS ON THE GAME'S OWN FRAMES (--runs=<folder>): every captured run under the folder (host\\fg_cap.h; a run is a
    folder with an index.txt) is made again through each variant by the replay (tools\\fg-replay), and the sheets
    (tools\\fg-replay\\sheets.py) are written with THE PRODUCT'S ROWS ABOVE AND THE VARIANT'S UNDER THEM, the same crops.
    The variant's rows come from a folder of this tool's own that holds the run's files by links and the replay's
    generated images in the place of the saved ones. NOTHING IS WRITTEN INTO THE CAPTURED RUN."""
    replay = os.path.join(tempfile.gettempdir(), "dlssiv-fg-replay", "fgpipe-replay.exe")
    if not os.path.isfile(replay):
        print("FAIL: the replay is not built: %s (python tools\\fg-replay\\run.py --keep builds it and proves it)" % replay); return 1
    out = os.path.join(tempfile.gettempdir(), "dlssiv-fg-variants-real")
    runs = sorted(d for d, _, f in os.walk(folder) if "index.txt" in f)
    print("THE VARIANTS ON %d CAPTURED RUN(S) under %s; the sheets go under %s" % (len(runs), folder, out))
    bad = 0 if runs else 1
    for run in runs:
        index = open(os.path.join(run, "index.txt"), encoding="utf-8", errors="replace").read()
        for n in names:
            tag = re.sub(r"[^A-Za-z0-9._-]+", "_", os.path.relpath(run, folder)).strip("_.") or "run"      # two runs of one name in two folders stay two
            o = os.path.join(out, n, tag)
            shutil.rmtree(o, ignore_errors=True)
            for sub in ("replay", "run", "sheets"): os.makedirs(os.path.join(o, sub))
            waited = 0
            while (C.other_gpu_users() or C.game_runs()) and waited < 1800:
                if waited == 0: print("  another GPU user or the game is running: waiting...")
                time.sleep(5); waited += 5
            r = subprocess.run([replay, "--run=" + run, "--lib=" + B.path_of(n), "--out=" + os.path.join(o, "replay"), "--expect=differ", "--dump=0"], capture_output=True, text=True, errors="replace", creationflags=subprocess.CREATE_NO_WINDOW, timeout=1800)
            said = [l for l in r.stdout.splitlines() if l.strip()]
            made = 0
            for l in index.splitlines():
                if not l.startswith("file: "): continue
                name, kind, frame = l[6:].split(" ")[:3]
                src = os.path.join(o, "replay", "f%02d-gen-replay.raw" % int(frame)) if kind == "gen" else os.path.join(run, name)
                if kind == "gen" and not os.path.isfile(src): src = os.path.join(run, name)      # a frame the replay did not write: the saved one
                else: made += kind == "gen"
                try: os.link(src, os.path.join(o, "run", name))
                except OSError: shutil.copyfile(src, os.path.join(o, "run", name))
            shutil.copyfile(os.path.join(run, "index.txt"), os.path.join(o, "run", "index.txt"))
            s = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "fg-replay", "sheets.py"), "--run=" + run, "--beside=" + os.path.join(o, "run"), "--out=" + os.path.join(o, "sheets")] + [a for a in ARGS if a.startswith(("--crop=", "--frames=", "--gain=", "--scale="))],
                               capture_output=True, text=True, errors="replace")
            ok = made > 0 and s.stdout.strip().endswith("PASS")
            bad += 0 if ok else 1
            # (the replay was asked --expect=differ: its PASS is "at least one image is not the saved one", its FAIL "every image is")
            last = said[-1].strip() if said else ""
            print("  [%s, %s] the replay through the variant: %s" % (n, tag, "AT LEAST ONE GENERATED IMAGE DIFFERS from the saved one" if last == "PASS" else
                                                                     "EVERY GENERATED IMAGE EQUALS THE SAVED ONE: the variant changes nothing on this run" if last == "FAIL" and made else "it said: %s (exit code %d)" % (last[:200] or "nothing", r.returncode)))
            print("  [%s, %s] %d generated image(s) of the variant's; the sheets (the product's rows above, the variant's under them): %s: %s" % (n, tag, made, os.path.join(o, "sheets"), "written" if ok else "NOT WRITTEN: " + (s.stdout.strip().splitlines() or ["sheets.py said nothing"])[-1][:200]))
    print("%s: %d variant(s) on %d run(s); the folder is kept: %s" % ("DONE" if not bad else "FAIL (%d did not come out, or no run was found)" % bad, len(names), len(runs), out))
    return 1 if bad else 0


def main():
    sys.path.insert(0, os.path.join(ROOT, "tools", "ffx-own"))
    import build_variants as B
    # (2026-10-11) the ghost fix and its instruments (build_variants.FIXES) are not savings: they are taken only when named
    # (--only=ghost prices the fix and holds its picture against the product's on the bench's pairs)
    fixes = [f[0] for f in getattr(B, "FIXES", [])]
    names = [v[0] for v in B.VARIANTS if (ONLY and v[0] in ONLY) or (not ONLY and v[0] not in fixes)]
    R = load("fgown_judge", os.path.join(HERE, "run.py"), ["--ours", "--keep"])
    C = R.C
    runs = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--runs=")), None)
    if runs: return real(C, B, names, runs)
    C.OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-variants")
    shutil.rmtree(C.OUT, ignore_errors=True); os.makedirs(C.OUT, exist_ok=True)
    sys.stdout = C.Tee(os.path.join(C.OUT, "result.txt"))
    t0 = time.time()
    product = C.LIBS["ours"][1]
    missing = [n for n in names if not os.path.isfile(B.path_of(n))] + ([] if os.path.isfile(product) else ["the product"])
    if missing:
        print("FAIL: not built: %s (build-ffx-own.ps1 -Target dlssiv-fg; python tools\\ffx-own\\build_variants.py)" % ", ".join(missing)); return 1
    print("THE VARIANT LIBRARIES AGAINST THE PRODUCT. THE PRODUCT: %s, %d bytes, SHA-256 %s" % (product, os.path.getsize(product), C.sha256_of(product)))
    for n in names:
        p = B.path_of(n)
        C.LIBS["v-" + n] = ("%s, A VARIANT (%s)" % (os.path.basename(p), ", ".join(dict(B.VARIANTS)[n])), p)
        print("  %-22s %d bytes, SHA-256 %s" % (n, os.path.getsize(p), C.sha256_of(p)))
    C.OWN_IFACE = tuple(C.OWN_IFACE) + tuple("v-" + n for n in names)
    failed = 0
    table = {n: {} for n in names}
    if "--no-picture" not in ARGS:
        if C.prepare() is None:
            print("FAIL"); return 1
        for cfg in ("QF", "DF"):
            print("\n" + "=" * 118 + "\nTHE PICTURE, %s: %s" % (cfg, R.CFG[cfg][0]))
            base = R.hash_session(cfg, "ours", "-raw", raw=KEYS, arms=ARMS)
            if base is None: failed += 1; continue
            raw = lambda s, k: C.P(s["sid"], "raw") + "-%s-%d.bin" % k
            for n in names:
                s = R.hash_session(cfg, "v-" + n, "-raw", raw=KEYS, arms=ARMS)
                if s is None: failed += 1; continue
                cmp_, diff, same = R.compare(base, s)
                pairs = "; ".join("%s %d of %d" % (c, v[0] - v[2], v[0]) for c, v in cmp_.items())
                print("\n  [%s, %s] hashed pairs that DIFFER from the product's, by class: %s" % (cfg, n, pairs))
                worst = None
                for k in KEYS:
                    if not (os.path.exists(raw(base, k)) and os.path.exists(raw(s, k))):
                        print("    arm %s pair %d: an image is missing" % k); failed += 1; continue
                    png = os.path.join(os.path.dirname(B.path_of(n)), "diff-%s-%s%d.png" % (cfg, k[0], k[1]))
                    r = whole(raw(base, k), raw(s, k), png)
                    print("    arm %s pair %d (steady): %s" % (k[0], k[1], r["words"]))
                    if r["c4"]:
                        print("      where (the picture's 16 by 9 blocks; of a block's pixels those that differ by 4 codes or more: . none, + under 1%, o under 10%, # more):")
                        for row in r["grid"]: print("        " + row)
                    if worst is None or r["c4"] > worst["c4"]: worst = r
                    os.remove(raw(s, k))
                table[n][cfg] = (pairs, worst)
            for k in KEYS:
                if os.path.exists(raw(base, k)): os.remove(raw(base, k))
        if C.ours(): C.end_ours("left at the end")
    price = {}
    if "--no-price" not in ARGS:
        print("\n" + "=" * 118 + "\nTHE PRICE: tools\\test-fg-own\\passes.py --cut, the product BEFORE, the variant AFTER (each variant's whole table is kept beside its library as price.txt)")
        for n in names:
            cmd = [sys.executable, os.path.join(HERE, "passes.py"), "--cut", "--before=" + product, "--after=" + B.path_of(n)] + (["--stamped"] if "--stamped" in ARGS else [])
            r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=ROOT)
            open(os.path.join(os.path.dirname(B.path_of(n)), "price.txt"), "w", encoding="utf-8").write(r.stdout)
            sec = r.stdout.split("A STEADY DISPATCH", 1)[-1].split("A RESET DISPATCH", 1)[0]
            rows = [l for l in sec.splitlines() if l.rstrip().endswith("<")]
            pair = next((l for l in sec.splitlines() if "THE PAIR (the passes' sum)" in l), None)
            outer = next((l for l in r.stdout.splitlines() if "<== THE PAIR" in l), None)
            void = [l for l in r.stdout.splitlines() if "VOID after" in l or "sessions were not run" in l]
            print("\n  [%s] a steady pair, GPU us: Quality's guides before, after, change | DLAA's guides before, after, change" % n)
            for l in rows + [x for x in (pair, outer) if x]: print("  " + l.rstrip())
            for l in void: print("    NOT CLEAN: " + l.strip()[:200])
            if pair is None or void: failed += 1
            m = re.findall(r"([+-]\d+)", pair.split("|", 1)[1]) if pair else []
            mo = re.findall(r"([+-]\d+)", outer.split("|", 1)[1]) if outer else []
            price[n] = (m[0], m[1]) if len(m) >= 2 else ("?", "?")
            if len(mo) >= 2: price[n] += (mo[0], mo[1])
    print("\n" + "=" * 118 + "\nTHE VARIANTS: what each saves against how much of the picture it changes (3840x2160 FP16, the two-image set; the bench's moving picture)")
    for n in names:
        p = price.get(n)
        print("  %-22s %s" % (n, ("the steady pair by its stamped passes %s us at Quality's guides, %s at DLAA's%s" % (p[0], p[1], ("; by the outer span with the stamps off %s and %s" % (p[2], p[3])) if len(p) == 4 else "")) if p else "not priced"))
        for cfg in ("QF", "DF"):
            if cfg in table[n]:
                pairs, w = table[n][cfg]
                print("  %-22s   %s: pairs that differ: %s; the most changed of three steady images: %s" % ("", cfg, pairs, w["words"] if w else "none compared"))
    print("%s: %d variant(s), in %.0f s; the folder is kept: %s" % ("DONE" if not failed else "FAIL (%d part(s) did not run)" % failed, len(names), time.time() - t0, C.OUT))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
