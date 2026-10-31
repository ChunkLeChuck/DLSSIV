"""XeSS'S MIP BIAS AND JITTER COUNT (package X4 of the XeSS build, 2026-10-28): the client's two numbers held to Intel's rules.

LodBiasForScale and JitterPhasesForScale (source/DLSS-IV.cpp) carry marker lines: LOD BIAS RULE START/END and JITTER PHASE
RULE START/END. This cuts the lines between each pair, scoped to its own function, compiles them as the game does (x86,
/O2, /EHa, /MT) into a bench with no Direct3D, and runs every render size the mod can make: kRatio's four scaled entries
(read from RsDecideRenderSize, with its rounding and its guard) and every custom percent the cfg allows (34 to 99), at
fifteen screen sizes from 1280x720 to 7680x4320. Each size goes through the cut as the game's sizes do.

What it holds, against Intel's guide (external\\xess\\doc\\xess_sr_developer_guide_english.md, "Mip Bias" and "Jitter
Sequence"; its formulas are images, its text gives the values):
  1. THE MIP BIAS is log2(1 / scale), scale = output / render. Intel's three stated values come out of the cut from sizes at
     exactly those scales: -1.202 at Performance (2.3x, 1000 -> 2300), -1.585 at Ultra Performance (3.0x, 1280 -> 3840),
     -1 at Balanced (2.0x, 1920 -> 3840), each rounded to three places as the guide prints it.
  2. At every size the mod makes, the cut's bias equals Intel's rule computed here in double (to 1e-6, the float the
     client keeps), and zero where the scale is off (the guard's sizes and the screen size itself).
  3. THE JITTER COUNT is at least 8 * scale^2 (Intel's minimum; 72 at 3.0x, the guide's own example) at every size the
     mod makes, on the axis the function reads (width), and within 128 (the function's ceiling) everywhere. The height's
     own scale is SAID, not failed (2026-10-28: Ultra Performance at 3840x1600 renders 533 tall, 3.0019x, which asks
     72.09 against 72): the guide gives one scale per preset and 72 for Ultra Performance at every screen size.
  4. CONTROLS, each a planted wrong rule cut and built the same way, which must FAIL: the bias with an extra -1 (the
     "full LOD" of the DECISIONS survey line), the bias with its sign turned, and a jitter count of 4 * scale^2.
     A planted edit that does not find its text exactly once fails the run: a control that changed nothing proves nothing.
  5. THE BENCH RAN: it prints one line per size and the count it ran; a short count fails.
The guide is committed with the SDK's headers (only libxess.dll is git-ignored): its text is read for the three values,
the 72 and the preset table, so a guide that changes them fails here. Where it is missing, that is said and the values
stand as this file's copies.

    python tools/test-lod-xess/gen.py                    exit code 0 = every check passed
    python tools/test-lod-xess/gen.py --guide <file>     read Intel's guide from another place (an SDK unpacked elsewhere)

Builds only in %TEMP% (removed on a pass); writes nothing in the tree.
"""
import os, re, sys, math, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "source", "DLSS-IV.cpp")
GUIDE = (sys.argv[sys.argv.index("--guide") + 1] if "--guide" in sys.argv
         else os.path.join(ROOT, "external", "xess", "doc", "xess_sr_developer_guide_english.md"))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-lod-xess-%d" % os.getpid())

SCREENS = [(1280, 720), (1366, 768), (1600, 900), (1920, 1080), (1920, 1200), (2560, 1080), (2560, 1440), (2560, 1600),
           (3440, 1440), (3840, 1600), (3840, 2160), (5120, 1440), (5120, 2160), (7680, 2160), (7680, 4320)]
STATED = [("Performance 2.3x", 2300, 1000, -1.202), ("Ultra Performance 3.0x", 3840, 1280, -1.585),
          ("Balanced 2.0x", 3840, 1920, -1.0)]

fails = 0
def check(ok, what):
    global fails
    print("  %s  %s" % ("PASS" if ok else "FAIL", what))
    if not ok: fails += 1
    return ok

def function_body(src, signature):
    """The text of one function, from its signature to the first closing brace at column 0 after it."""
    n = src.count(signature + "\n")
    if n != 1: raise SystemExit("FAIL: '%s' is defined %d times in DLSS-IV.cpp, not once" % (signature, n))
    a = src.index(signature + "\n")
    b = src.index("\n}\n", a)
    return src[a:b + 3]

def cut(body, start, end):
    """The lines strictly between the START marker's line and the END marker's line, both found once in the body."""
    for tag in (start, end):
        c = body.count(tag)
        if c != 1: raise SystemExit("FAIL: the marker '%s' is in the function %d times, not once" % (tag, c))
    a = body.index(start); a = body.index("\n", a) + 1
    b = body.index(end); b = body.rindex("\n", 0, b) + 1
    if b <= a: raise SystemExit("FAIL: the markers '%s' and '%s' hold nothing" % (start, end))
    return body[a:b]

def ratios(src):
    """kRatio and kName from RsDecideRenderSize, and its sizing text, which the cases below copy."""
    body = function_body(src, "static void RsDecideRenderSize(UINT w, UINT h)")
    m = re.findall(r"static const double kRatio\[5\] = \{([^}]*)\};", body)
    if len(m) != 1: raise SystemExit("FAIL: kRatio is not in RsDecideRenderSize once")
    vals = []
    for e in m[0].split(","):
        e = e.strip()
        if not re.fullmatch(r"[0-9.]+(\s*/\s*[0-9.]+)?", e): raise SystemExit("FAIL: a kRatio entry is not a number or a quotient: " + e)
        p = [float(x) for x in e.split("/")]
        vals.append(p[0] / p[1] if len(p) == 2 else p[0])
    names = re.findall(r"static const char\*\s+kName\[5\]\s*=\s*\{([^}]*)\};", body)
    if len(names) != 1: raise SystemExit("FAIL: kName is not in RsDecideRenderSize once")
    names = [x.strip().strip('"') for x in names[0].split(",")]
    for text in ("const double ratio = sc > 0 ? (double)sc / 100.0 : kRatio[q];",
                 "const UINT rw = (UINT)((double)w * ratio + 0.5), rh = (UINT)((double)h * ratio + 0.5);",
                 "if (rw < 64 || rh < 64 || rw >= w || rh >= h) return;"):
        if body.count(text) != 1: raise SystemExit("FAIL: RsDecideRenderSize no longer reads '%s' once: this bench's sizing is stale" % text)
    return vals, names

def custom_range():
    """The custom percent's clamp, read from IvtClampCfg's text in the transport header."""
    t = open(os.path.join(ROOT, "source", "transport", "DlssIvTransport.h"), encoding="utf-8").read()
    for text in ("if (c->custom < 0 || c->custom >= 100) c->custom = 0;",
                 "else if (c->custom > 0 && c->custom < 34) c->custom = 34;"):
        if t.count(text) != 1: raise SystemExit("FAIL: the custom clamp no longer reads '%s' once" % text)
    return range(34, 100)

def cases(kr, kn):
    """(label, outW, outH, renderW, renderH, active): every size the mod makes, as RsDecideRenderSize makes it."""
    out = []
    asks = [(kn[q], kr[q]) for q in range(1, 5)] + [("custom=%d" % pc, pc / 100.0) for pc in custom_range()]
    for (w, h) in SCREENS:
        out.append(("off", w, h, w, h, 0))
        for (label, r) in asks:
            rw = int(w * r + 0.5); rh = int(h * r + 0.5)
            active = not (rw < 64 or rh < 64 or rw >= w or rh >= h)
            out.append((label, w, h, rw if active else w, rh if active else h, 1 if active else 0))
    for (label, w, rw, _) in STATED:
        out.append(("stated " + label, w, w * 9 // 16, rw, rw * 9 // 16, 1))
    return out

BENCH = r"""
#include <math.h>
#include <stdio.h>
typedef unsigned int UINT;
static bool  rsActive = false;
static UINT  rsW = 0, rsH = 0, rsOutW = 0, rsOutH = 0;
static float lodBiasAuto = 0.0f;
static float BiasRule()
{
%(LOD)s    (void)was;
    return lodBiasAuto;
}
static int PhaseRule()
{
%(JIT)s    return p;
}
struct Case { UINT w, h, rw, rh; int active; };
static const Case kCases[] = {
%(CASES)s};
int main()
{
    int n = 0;
    for (const Case& c : kCases)
    {
        rsOutW = c.w; rsOutH = c.h; rsW = c.rw; rsH = c.rh; rsActive = c.active != 0;
        lodBiasAuto = 12345.0f;                 // a value no rule gives: a cut that did not write it shows
        const float b = BiasRule();
        const int   p = PhaseRule();
        printf("CASE %%d %%u %%u %%u %%u %%d %%.9f %%d\n", n, c.w, c.h, c.rw, c.rh, c.active, (double)b, p);
        n++;
    }
    printf("RAN %%d\n", n);
    return 0;
}
"""

def vcvars32():
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                  "-property", "installationPath"]).decode().strip()
    v = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    if not os.path.exists(v): raise SystemExit("FAIL: vcvars32.bat not found at " + v)
    return v

def build_run(name, lod, jit, cs, vc):
    d = os.path.join(OUT, name); os.makedirs(d, exist_ok=True)
    rows = "".join("    { %du, %du, %du, %du, %d },\n" % (c[1], c[2], c[3], c[4], c[5]) for c in cs)
    with open(os.path.join(d, "b.cpp"), "w", encoding="utf-8") as f:
        f.write("// GENERATED by tools/test-lod-xess/gen.py (%s): the bias and jitter rules cut from source/DLSS-IV.cpp\n" % name)
        f.write(BENCH % {"LOD": lod, "JIT": jit, "CASES": rows})
    exe = os.path.join(d, "b.exe")
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /O2 /EHa /MT /W4 /WX /std:c++17 b.cpp /Fe:b.exe' % vc,
                       shell=True, capture_output=True, text=True, cwd=d)
    if r.returncode != 0 or not os.path.exists(exe):
        print(r.stdout[-3000:], r.stderr[-2000:]); raise SystemExit("FAIL: the %s bench did not compile" % name)
    r = subprocess.run([exe], capture_output=True, text=True)
    if r.returncode != 0: raise SystemExit("FAIL: the %s bench exited %d" % (name, r.returncode))
    got = []
    for line in r.stdout.splitlines():
        if line.startswith("CASE "):
            f = line.split()
            got.append((int(f[2]), int(f[3]), int(f[4]), int(f[5]), int(f[6]), float(f[7]), int(f[8])))
    ran = [l for l in r.stdout.splitlines() if l.startswith("RAN ")]
    return got, (int(ran[0].split()[1]) if len(ran) == 1 else -1)

def judge(cs, got, ran, say):
    """The number of wrong answers: each case against Intel's rules computed here in double."""
    bad = 0; worst_b = 0.0; worst_p = None; tall = []
    if ran != len(cs) or len(got) != len(cs):
        if say: check(False, "the bench ran %d of %d sizes" % (ran, len(cs)))
        return 10**6
    for c, g in zip(cs, got):
        label, w, h, rw, rh, active = c
        if g[:5] != (w, h, rw, rh, active): bad += 1; continue
        b, p = g[5], g[6]
        intel_b = -math.log2(w / rw) if active and rw < w else 0.0     # log2(1 / scale), scale = output / render
        s = w / rw if active else 1.0
        need = 8.0 * s * s                                             # Intel's minimum count
        if abs(b - intel_b) > 1e-6: bad += 1
        if p < need or p > 128 or p < 8: bad += 1
        worst_b = max(worst_b, abs(b - intel_b))
        if active and (worst_p is None or p - need < worst_p[0]): worst_p = (p - need, label, w, h, rw, p, need)
        if active and not label.startswith("stated ") and p < 8.0 * (h / rh) ** 2: tall.append((label, w, h, rh, p, 8.0 * (h / rh) ** 2))
        if label.startswith("stated "):
            stated = [x for x in STATED if "stated " + x[0] == label][0][3]
            ok = round(b, 3) == stated
            if not ok: bad += 1
            if say: check(ok, "Intel's stated %s: the cut gives %+.4f from %u -> %u, %+.3f to three places (Intel: %+.3f)"
                          % (label[7:], b, rw, w, round(b, 3), stated))
    if say:
        check(worst_b <= 1e-6, "the bias equals log2(1 / scale) at all %d sizes (largest difference %.2e)" % (len(cs), worst_b))
        if worst_p:
            check(worst_p[0] >= 0, "the jitter count meets 8 * scale^2 at every scaled size (tightest: %s at %ux%u, %u wide, %d against %.4f)"
                  % (worst_p[1], worst_p[2], worst_p[3], worst_p[4], worst_p[5], worst_p[6]))
        # THE HEIGHT'S OWN SCALE, which the rounding can make a hair larger than the width's. Said, not failed: the guide
        # gives one scale per preset ("Ultra Performance ... at least 72", whatever the screen, though its rounded input
        # sizes differ by a pixel per axis), and the function reads the width, as NVIDIA's rule does.
        print("  NOTE  on the HEIGHT's own scale %d size(s) fall short of 8 * scale^2 by under one phase%s"
              % (len(tall), "".join("; %s at %ux%u, %u tall, %d against %.4f" % t for t in tall[:3])))
    return bad

def plant(text, old, new, what):
    n = text.count(old)
    if n != 1: raise SystemExit("FAIL: the control '%s' found its text %d times, not once" % (what, n))
    return text.replace(old, new)

def guide_text():
    if not os.path.exists(GUIDE):
        if "--guide" in sys.argv: check(False, "the guide named by --guide is not there: " + GUIDE); return
        print("  NOTE  Intel's guide is not at external\\xess\\doc in this tree: the stated values stand as this file's copies")
        return
    g = open(GUIDE, encoding="utf-8").read()
    mip = g[g.index("#### Mip Bias"):g.index("### Initialization")]
    check("a mip bias of `-1` should be applied for `2.0x` resolution scaling" in " ".join(mip.split()),
          "the guide's Mip Bias section still says -1 at 2.0x")
    up = " ".join(g[g.index("### Update from XeSS 1.2"):g.index("### XeSS-SR Components")].split())
    check("Performance quality preset should result in additional mip bias of `-1.202`" in up, "the guide still says -1.202 at Performance")
    check("Ultra Performance quality preset should result in additional mip bias of `-1.585`" in up, "the guide still says -1.585 at Ultra Performance")
    check("Ultra Performance quality preset should use at least `72`" in up, "the guide still says at least 72 phases at Ultra Performance")
    check("| Performance | 2.0x | 2.3x |" in up and "| Ultra Performance | N/A | 3.0x |" in up and "| Balanced | 1.7x | 2.0x |" in up,
          "the guide's table still gives Performance 2.3x, Ultra Performance 3.0x, Balanced 2.0x")

def main():
    src = open(SRC, encoding="utf-8-sig").read()       # universal newlines: CRLF reads as \n
    lod = cut(function_body(src, "static void LodBiasForScale()"), "// LOD BIAS RULE START", "// LOD BIAS RULE END")
    jit = cut(function_body(src, "static void JitterPhasesForScale()"), "// JITTER PHASE RULE START", "// JITTER PHASE RULE END")
    # the control symbols: each cut holds the assignment its bench returns
    if lod.count("lodBiasAuto =") != 1: raise SystemExit("FAIL: the bias cut does not assign lodBiasAuto once")
    if len(re.findall(r"\bint p =", jit)) != 1: raise SystemExit("FAIL: the jitter cut does not define p once")
    kr, kn = ratios(src)
    cs = cases(kr, kn)
    vc = vcvars32()
    print("test-lod-xess: %d sizes (%d screens, kRatio %s, custom 34..99, Intel's three stated scales)"
          % (len(cs), len(SCREENS), ", ".join("%.4f" % r for r in kr[1:])))
    guide_text()

    got, ran = build_run("real", lod, jit, cs, vc)
    check(ran == len(cs), "the bench ran all %d sizes (it printed RAN %d)" % (len(cs), ran))
    bad = judge(cs, got, ran, True)
    check(bad == 0, "every size the mod makes: %d wrong answers" % bad)

    # the table at 4K, for the report (what each mode gets under DLSS, FSR and XeSS: one rule, one value)
    print("  at 3840x2160:")
    for c, g in zip(cs, got):
        if c[1] == 3840 and c[2] == 2160 and (not c[0].startswith("custom") or c[0] in ("custom=34", "custom=50", "custom=75", "custom=99")):
            print("    %-18s %4u -> %4u  scale %.4fx  bias %+.4f  jitter %3d" % (c[0], c[3], c[1], c[1] / c[3], g[5], g[6]))

    controls = [
        ("bias minus one", plant(lod, "(float)(log(ratio) / log(2.0))", "(float)(log(ratio) / log(2.0) - 1.0)", "bias minus one"), jit),
        ("bias sign turned", plant(lod, "(float)(log(ratio) / log(2.0))", "(float)(-log(ratio) / log(2.0))", "bias sign turned"), jit),
        ("jitter half count", lod, plant(jit, "8.0 * ratio * ratio", "4.0 * ratio * ratio", "jitter half count")),
    ]
    for (name, l, j) in controls:
        g, r = build_run(name.replace(" ", "-"), l, j, cs, vc)
        cb = judge(cs, g, r, False)
        check(r == len(cs) and cb > 0, "control '%s' ran all %d sizes and FAILS (%d wrong answers)" % (name, len(cs), cb))

    if fails == 0:
        shutil.rmtree(OUT, ignore_errors=True)
        print("test-lod-xess: PASS")
        return 0
    print("test-lod-xess: FAIL (%d checks); the benches are in %s" % (fails, OUT))
    return 1

if __name__ == "__main__":
    sys.exit(main())
