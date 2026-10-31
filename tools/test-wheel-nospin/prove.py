"""THE WHEELS' SPIN LEFT OUT OF THE VECTORS, PROVEN AT A DESK (2026-10-11). Not shipped; no Direct3D, so prove-all can run it.
SINCE THE EVENING OF THAT DAY IT IS THE DEFAULT (the owner): a release build calls VelWheelNoSpin on every tyre that
moved; only the KEY mv_wheel (0 keeps the spin, to compare) and its counters are a development build's.

    python tools\\test-wheel-nospin\\prove.py

1. THE FENCE, as text: every line of the two sources that names the KEY (mv_wheel) or its counters (velStatWheel) is
   inside an `#if DLSSIV_DEV` block or an IVT_DEV_ONLY(...): a release build has no key and no counter.
2. THE WIRING, as text: the call sits in TrkOnDrawPre, on a tyre only (premul), BEFORE last frame's matrix is multiplied
   into the previous view and projection; a development build asks the key, a release build calls it always; the
   function stands at the file's end and its name and its axle memory at the top (anywhere between, they move the
   release's hot data).
3. THE MATHS: VelWheelNoSpin's own text, cut from source\\DLSS-IV.cpp between its two marker lines and built as wn.exe,
   against wheels made in double precision (wn.cpp says what a wheel is and what is wanted).
4. THREE CONTROLS: the cut spoiled in one place each (the rotation's sign, the half-angle term, the memory) must FAIL.
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
fails = []


def check(ok, what):
    print("  %s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok: fails.append(what)
    return ok


def read(rel): return open(os.path.join(ROOT, "source", rel), "rb").read().decode("utf-8", errors="replace").replace("\r\n", "\n")


cpp = read("DLSS-IV.cpp"); hdr = read(os.path.join("transport", "DlssIvTransport.h"))
NAMES = re.compile(r"mv_wheel|velStatWheel")


def unfenced(text):
    """Lines that name the key outside every #if DLSSIV_DEV block (comments count: a release's text carries none of it)."""
    out = []; stack = []
    for i, l in enumerate(text.split("\n"), 1):
        s = l.strip()
        if re.match(r"#\s*if", s): stack.append(bool(re.match(r"#\s*if\s+DLSSIV_DEV\b", s)))
        elif re.match(r"#\s*(else|elif)", s) and stack: stack[-1] = False
        elif re.match(r"#\s*endif", s) and stack: stack.pop()
        elif NAMES.search(l) and not any(stack) and not re.search(r"IVT_DEV_ONLY\(X\(mv_wheel\)\)", l): out.append(i)
    return out


print("1. the fence")
u1, u2 = unfenced(cpp), unfenced(hdr)
check(not u1, "DLSS-IV.cpp names the key only inside a development fence (%s)" % (u1 or "every line"))
check(not u2, "DlssIvTransport.h names the key only inside a development fence or IVT_DEV_ONLY (%s)" % (u2 or "every line"))
check(len(NAMES.findall(cpp)) >= 5 and len(NAMES.findall(hdr)) >= 6, "CONTROL: the key is named in both files at all (%d and %d times)" % (len(NAMES.findall(cpp)), len(NAMES.findall(hdr))))
spoiled = hdr.replace("#if DLSSIV_DEV\n    if (!ivtCfg.mv_wheel)", "#if 1\n    if (!ivtCfg.mv_wheel)", 1)   # (the fence's first line since the strip of 2026-10-25 took hdr_cs_test's echo out of it)
check(spoiled != hdr and unfenced(spoiled), "CONTROL: a fence opened for every build is caught")

print("2. the wiring")
a = cpp.index("static __forceinline void TrkOnDrawPre("); body = cpp[a:cpp.index("\nstatic HRESULT __stdcall hkDrawIndexed(", a)]
call = "if (premul) InterlockedIncrement(&velStatWheel[ivtCfg.mv_wheel ? VelWheelNoSpin(trackWorld, prevWorld, &velWheelAxle) : 5]);"
rel = "if (premul) VelWheelNoSpin(trackWorld, prevWorld, &velWheelAxle);"
check(body.count(call) == 1 and body.count(rel) == 1 and body.count("VelWheelNoSpin(") == 2, "TrkOnDrawPre calls it on a tyre only (premul), with this frame's matrix and last frame's: once for each kind of build")
check(re.search(r"#if DLSSIV_DEV\n\s*" + re.escape(call) + r"[^\n]*\n#else\n\s*" + re.escape(rel) + r"[^\n]*\n#endif\n", body) is not None,
      "a development build asks the key (mv_wheel=0 keeps the spin); A RELEASE BUILD CALLS IT ALWAYS")
check(cpp.index("static int VelWheelNoSpin(const float* cur, float* prev, volatile long* axleSeen);") < cpp.index("static void NoticeFromWarn(const char* line);")
      and cpp.index("static volatile long velWheelAxle = -1;") < cpp.index("static void NoticeFromWarn(const char* line);")
      and cpp.index("// ---- WHEEL NO SPIN START") > cpp.index("// ---- THE LADDER'S HELD FRAME RATE END ----"),
      "its name and its axle memory stand at the top of DLSS-IV.cpp, the function at its end")
check(call in body and body.index(call) < body.index("ToD(*(const Mat4*)prevWorld, &pw);") and body.index("mover = M4dNoteDraw<S>") < body.index(call),
      "after the tracker has given last frame's matrix and BEFORE it goes into the previous view and projection")
check(body.index("if (mover && S::CamPrevValid())") < body.index(call), "inside the mover's block: a tyre that did not move is not touched")

print("3. the maths")
m = re.search(r"(?ms)^// ---- WHEEL NO SPIN START[^\n]*\n(.*?)^// ---- WHEEL NO SPIN END ----", cpp)
if not check(bool(m) and "VelWheelNoSpin(const float* cur, float* prev, volatile long* axleSeen)" in m.group(1), "the function stands between its two marker lines"):
    print("FAIL"); sys.exit(1)
cut = m.group(1)


def build_and_run(text, name):
    open(os.path.join(HERE, "wn_cut.h"), "w", encoding="utf-8", newline="\n").write("// CUT FROM source\\DLSS-IV.cpp BY prove.py: do not edit\n#include <math.h>\n" + text)
    exe = os.path.join(HERE, name)
    b = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "build.ps1"), "-Out", exe], capture_output=True, text=True)
    if b.returncode or not os.path.isfile(exe): return None, (b.stdout + b.stderr)[-1500:]
    r = subprocess.run([exe], capture_output=True, text=True)
    return r.returncode, r.stdout


rc, out = build_and_run(cut, "wn.exe")
print(out.rstrip() if out else "")
check(rc == 0 and "PASS" in (out or ""), "wn.exe: every case")

print("4. the controls")
for what, old, new in (("the rotation turned the other way", "u[a][1] * v[a][2] - u[a][2] * v[a][1], u[a][2] * v[a][0] - u[a][0] * v[a][2], u[a][0] * v[a][1] - u[a][1] * v[a][0]",
                                                              "v[a][1] * u[a][2] - v[a][2] * u[a][1], v[a][2] * u[a][0] - v[a][0] * u[a][2], v[a][0] * u[a][1] - v[a][1] * u[a][0]"),
                       ("the half-angle term left out", " / (1.0f + c);", " * 0.0f;"),
                       ("the axle never remembered", "*axleSeen = a;", "(void)axleSeen;")):
    if not check(cut.count(old) == 1, "the control's place is in the cut (%s)" % what): continue
    rc, out = build_and_run(cut.replace(old, new), "wn_control.exe")
    check(rc == 1 and "FAIL" in (out or ""), "CONTROL: %s: wn fails (%s)" % (what, "; ".join(l.strip()[6:60] for l in (out or "").splitlines() if l.strip().startswith("FAIL  "))[:150]))
    try: os.remove(os.path.join(HERE, "wn_control.exe"))
    except OSError: pass
build_and_run(cut, "wn.exe")          # the real exe is what is left behind
try: os.remove(os.path.join(HERE, "wn_cut.h"))       # the cut is made afresh by every run and is not a file of the repository
except OSError: pass
print("FAIL: %d" % len(fails) if fails else "PASS")
sys.exit(1 if fails else 0)
