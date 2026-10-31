"""THE GLASS CENSUS'S PROOF (2026-10-10; roadmap item 5). Not shipped, not part of prove-all (it needs the game's shader
files and creates a device).

    python tools\\test-glass-census\\prove.py "C:\\...\\Grand Theft Auto IV\\GTAIV1080"

1. THE TEXT: the census stands inside one development fence at the end of source\\DLSS-IV.cpp, each of the four draw
   hooks calls it once inside a fence of its own with its own number, and the switch is read where the draw hook first
   speaks. Each check is made inside the function it is about, with a control that the function was found.
2. THE CENSUS ITSELF: its text is cut into gc_cut.h and compiled into gc.exe (gc.cpp says what it plays), which runs on
   DXVK's Direct3D 9 with three of the game's own pixel shaders. The table it writes and the files it dumps are held
   against what was played: five kinds, their counts a frame, their order, their states, the mark for a state that
   differed, the second target, the seam, the four draw calls, and each shader's hash and bytes against this script's
   own reading of the .fxc file.
3. TWO CONTROLS: a harness that plays two kinds as one, and one that never changes the alpha reference, must each FAIL
   the check they are about.
It waits first until none of the other session's device programs has run for 60 seconds.
"""
import os, re, shutil, struct, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CPP = os.path.join(ROOT, "source", "DLSS-IV.cpp")
OTHERS = ("gtaiv.exe", "dlssiv-helper.exe", "fgpipe", "vklines", "perfproof")
fails = []


def check(ok, what):
    print("  %s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok: fails.append(what)
    return ok


def body_of(text, head):
    """The text of the function whose definition starts with `head`: from the head to the first line that is a lone }."""
    a = text.index(head)
    b = text.index("\n}\n", a)
    return text[a:b]


def strip_comments(s):
    return "\n".join(l.split("//")[0] for l in s.splitlines())


def fragments(b, off):
    n = b[off]; off += 1; out = []
    for _ in range(n):
        nv = b[off]; off += 1
        for _ in range(nv):
            typ, reg, ln = struct.unpack_from("<HHB", b, off); off += 5 + ln
        size, s2 = struct.unpack_from("<HH", b, off); off += 4
        out.append(b[off:off + size]); off += size
    return out, off


def fnv(b):
    h = 2166136261
    for x in b: h = ((h ^ x) * 16777619) & 0xFFFFFFFF
    return h


def wait_quiet():
    quiet = None; t0 = time.time()
    while True:
        tl = subprocess.run("tasklist /NH", shell=True, capture_output=True, text=True).stdout.lower()
        busy = [l.split()[0] for l in tl.splitlines() if l.split() and any(l.startswith(o) for o in OTHERS)]
        if busy: quiet = None
        elif quiet is None: quiet = time.time()
        if quiet is not None and time.time() - quiet >= 60: return
        if time.time() - t0 > 3 * 3600: print("still busy after three hours:", busy); sys.exit(3)
        time.sleep(5)


def table(log):
    rows = []
    for l in log.splitlines():
        m = re.match(r"  glass: ps ([0-9A-F]{8})\s+(\d+)\s+([\d.]+) draws\s+([\d.]+) prims a frame  order (-?\d+)\.\.(-?\d+)  blend (\d+) (\S+)/(\S+)  zwrite (\d+) ztest (\d+) zfunc (\d+)"
                     r"  alphatest (\d+) ref (\d+) func (\d+)  mask ([0-9A-F]+) cull (\d+) sep (\d+)  varies ([0-9A-F]+)  mrt (-?\d+) afterseam (-?\d+)  calls (\d+)/(\d+)/(\d+)/(\d+)$", l)
        if m:
            g = m.groups()
            rows.append(dict(hash=g[0], size=int(g[1]), draws=float(g[2]), prims=float(g[3]), o0=int(g[4]), o1=int(g[5]), blend=int(g[6]), src=g[7], dst=g[8],
                             zw=int(g[9]), at=int(g[12]), ref=int(g[13]), varies=int(g[18], 16), mrt=int(g[19]), seam=int(g[20]), calls=[int(x) for x in g[21:25]]))
    return rows


def main():
    game = sys.argv[1].rstrip("/\\")
    src = open(CPP, "rb").read().decode("utf-8").replace("\r\n", "\n")
    print("1. the text")
    a = "// ---- THE GLASS CENSUS. "; z = "// ---- END OF THE GLASS CENSUS ----"
    check(src.count(a) == 1 and src.count(z) == 1, "the census's two marks stand once each")
    cut = src[src.index(a):src.index(z)]
    before = src[:src.index(a)].rstrip("\n").splitlines()[-1]; after = src[src.index(z):].splitlines()[1]
    check(before == "#if DLSSIV_DEV" and after == "#endif", "the census is inside one development fence (#if DLSSIV_DEV ... #endif)")
    check("LatWaitIn" not in cut and "static void GlassCensus(IDirect3DDevice9* dev, int kind, UINT prims)\n{" in cut, "the cut holds the census's definition and nothing of the function before it (control)")
    call = "#if DLSSIV_DEV\n    if (gcOn && curRT0 && curRT0 == sceneBufSurf && !rsOwn && !ivtInOwnDraw) GlassCensus(dev, %d, primCount);"
    for i, head in enumerate(("static HRESULT __stdcall hkDrawIndexed(IDirect3DDevice9* dev,", "static HRESULT __stdcall hkDrawPrim(IDirect3DDevice9* dev,",
                              "static HRESULT __stdcall hkDrawPrimUP(IDirect3DDevice9* dev,", "static HRESULT __stdcall hkDrawIdxUP(IDirect3DDevice9* dev,")):
        check(src.count(head) == 1, "%s is defined once (control)" % head.split("(")[0].split()[-1])
        fn = body_of(src, head)
        code = strip_comments(fn)
        ok = "DT_NOT_ON_WORKER(" in code                                       # the control: this IS the hook's body
        first = code.index("DT_NOT_ON_WORKER(") if ok else 0
        want = strip_comments(call % i)
        check(ok and code.count("GlassCensus(") == 1 and want in code and code.index(want) > first and code[code.index(want) + len(want):].lstrip().startswith("#endif"),
              "%s calls the census once, as kind %d, after the worker's early return, inside a fence of its own" % (head.split("(")[0].split()[-1], i))
    fn = strip_comments(body_of(src, "static HRESULT __stdcall hkDrawIndexed(IDirect3DDevice9* dev,"))
    k = fn.index("GlassCensusInit();") if "GlassCensusInit();" in fn else -1
    check(k > 0 and fn.count("GlassCensusInit();") == 1 and fn.rfind("#if DLSSIV_DEVLOG", 0, k) > fn.rfind("#endif", 0, k) and "toldDrawHook = true;" in fn[:k],
          "the switch is read once, where the draw hook first speaks, inside the development log's fence")
    check(strip_comments(src).count("GlassCensus(") == 6 and strip_comments(src).count("GlassCensusInit(") == 3, "no other caller (4 calls, a prototype and the definition; the switch: a call, a prototype, the definition)")

    print("2. the census itself")
    blobs = []
    for name, idx in (("gta_vehicle_vehglass", 2), ("gta_vehicle_vehglass", 4), ("gta_projtex", 1)):
        p = None
        for sub in ("update/common/shaders/win32_30", "common/shaders/win32_30"):
            q = os.path.join(game, sub, name + ".fxc")
            if os.path.exists(q): p = q; break
        b = open(p, "rb").read(); vs, off = fragments(b, 4); ps, off = fragments(b, off)
        blobs.append((name, idx, ps[idx]))
    work = tempfile.mkdtemp(prefix="gcproof-")
    open(os.path.join(HERE, "gc_cut.h"), "w", encoding="utf-8", newline="\n").write(cut + z + "\n")
    r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "build.ps1")], capture_output=True, text=True)
    os.remove(os.path.join(HERE, "gc_cut.h"))
    if not check(r.returncode == 0 and os.path.exists(os.path.join(HERE, "gc.exe")), "gc.exe builds from the cut"):
        print(r.stdout[-3000:], r.stderr[-1500:]); return 1
    paths = []
    for i, (name, idx, code) in enumerate(blobs):
        p = os.path.join(work, "in%d.bin" % i); open(p, "wb").write(code); paths.append(p)
    print("   waiting for 60 quiet seconds before the device ...", flush=True)
    wait_quiet()

    def run(tag, mutant):
        side = os.path.join(work, tag); os.makedirs(side)
        log = os.path.join(work, tag + ".log")
        a = [os.path.join(HERE, "gc.exe"), "--d3d9", os.path.join(ROOT, "external", "dxvk", "x32", "d3d9.dll"), "--side", side, "--log", log]
        for p in paths: a += ["--ps", p]
        if mutant: a += ["--mutant", str(mutant)]
        rr = subprocess.run(a, capture_output=True, text=True, timeout=300)
        return rr.returncode, (open(log).read() if os.path.exists(log) else ""), side

    rc, log, side = run("real", 0)
    check(rc == 0 and "HARNESS done" in log, "the harness played its frames (exit %d)" % rc)
    check("HARNESS without the folder: gcOn 0" in log and "HARNESS with the folder: gcOn 1" in log and log.count("glass census: ON") == 1, "no folder, no census; the folder, the census, said once")
    heads = re.findall(r"  glass census: table (\d+), (\d+) frames, ([\d.]+) draws a frame into the scene buffer, (\d+) kinds \((\d+) draws found the table full\)", log)
    check(heads == [("1", "1800", "6.0", "5", "0")], "one table: 1800 frames, 6.0 draws a frame, 5 kinds, none lost (%s)" % heads)
    rows = table(log)
    h = ["%08X" % fnv(c) for n, i, c in blobs]
    def judge(rows, quiet=False):
        res = {}
        res["five"] = len(rows) == 5
        if not res["five"]: return res
        A, B, C, D, E = rows
        res["order"] = [(x["o0"], x["o1"]) for x in rows] == [(0, 0), (1, 2), (3, 3), (4, 4), (5, 5)]
        res["names"] = [x["hash"] for x in rows] == [h[0], h[0], h[1], h[2], "00000000"] and [x["size"] for x in rows] == [len(blobs[0][2]), len(blobs[0][2]), len(blobs[1][2]), len(blobs[2][2]), 0]
        res["counts"] = [x["draws"] for x in rows] == [1.0, 2.0, 1.0, 1.0, 1.0] and [x["prims"] for x in rows] == [10.0, 14.0, 3.0, 2.0, 1.0]
        res["blend"] = [(x["blend"], x["src"], x["dst"], x["zw"]) for x in rows[:4]] == [(1, "SRCALPHA", "INVSRCALPHA", 0), (1, "SRCALPHA", "INVSRCALPHA", 1), (1, "SRCALPHA", "ONE", 0), (0, "SRCALPHA", "ONE", 1)]
        res["alpha"] = (A["at"], A["ref"]) == (1, 100) and D["at"] == 0
        res["varies"] = C["varies"] == 1 << 5 and all(x["varies"] == 0 for x in (A, B, D, E))
        res["mrt"] = [x["mrt"] for x in rows] == [0, 0, 0, 1800, 0] and [x["seam"] for x in rows] == [0, 0, 0, 0, 1800]
        res["calls"] = [x["calls"] for x in rows] == [[1800, 0, 0, 0], [3600, 0, 0, 0], [0, 1800, 0, 0], [0, 0, 1800, 0], [0, 0, 0, 1800]]
        return res
    res = judge(rows)
    check(res.get("five"), "five kinds in the table")
    if res.get("five"):
        check(res["order"], "in the frame's own order: 0, 1..2, 3, 4, 5")
        check(res["names"], "each kind carries its shader's hash and size as this script reads them from the .fxc (%s), and none for no shader" % ", ".join(h))
        check(res["counts"], "draws and primitives a frame: 1/10, 2/14, 1/3, 1/2, 1/1")
        check(res["blend"], "blending, its factors and the depth write as played; the same shader with the depth write on is a kind of its own")
        check(res["alpha"], "the alpha test and its reference as played (on at 100; off)")
        check(res["varies"], "the alpha reference that changed is marked on the third kind and on no other")
        check(res["mrt"], "the second target is counted on the fourth kind only, the seam on the fifth only")
        check(res["calls"], "each kind under the draw call that made it")
    dumped = sorted(os.listdir(os.path.join(side, "DLSSIV-glass")))
    want = sorted(set("ps_%s_%d.bin" % (h[i], len(blobs[i][2])) for i in range(3)))
    check(dumped == want and all(open(os.path.join(side, "DLSSIV-glass", "ps_%s_%d.bin" % (h[i], len(blobs[i][2]))), "rb").read() == blobs[i][2] for i in range(3)),
          "three shader files written, named by hash and size, each the game's bytes exactly (%s)" % ", ".join(dumped))

    print("3. the controls")
    rc, log1, _ = run("mutant1", 1)
    r1 = judge(table(log1))
    check(rc == 0 and not r1.get("five"), "CONTROL: two kinds played as one are counted as one, and the check for five kinds fails")
    rc, log2, _ = run("mutant2", 2)
    r2 = judge(table(log2))
    check(rc == 0 and r2.get("five") and not r2["varies"] and r2["counts"], "CONTROL: an alpha reference that never changes is not marked, and the check for the mark fails")
    shutil.rmtree(work, ignore_errors=True)
    print("FAIL: %d" % len(fails) if fails else "PASS")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
