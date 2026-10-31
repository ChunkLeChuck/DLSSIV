r"""X2, THE PIPE SMOKE OF THE HELPER'S XeSS ARM (2026-10-28; docs/reports/xess-plan-2026-10-17, PLAN.md X2's proofs).

tools\test-xess\pipe_smoke.cpp is a 64-bit stand-in for the game (its head says what it proves and why it is not
tools\test-fg-pipe's fake client, which asks transport builds only). This script builds it, stages the helper AS BUILT IN THE
TREE (host\dlssiv-helper.exe, the development build unless --helper= names another) with the libraries each session needs in
folders under %TEMP% (Intel's libxess.dll held to external\xess\VERSION.txt's size and SHA-256; AMD's two by tools\ffx_pin.py;
NVIDIA's from external\ngx), runs the sessions below, and reads the helper's own log beside each.

  X   XeSS (--backend=2), the FP16 capture 2560x1440 -> 3840x2160 (the mod's Quality, ratio 1.5): the build is ok, the ack
      says FEED_ACK_SR_ACTIVE | FEED_ACK_BACKEND_XESS and not FSR, the caps say 16384 and 32768 and no reason; the helper's
      log names XeSS's version, "Ultra Quality (setting 104" (picked by ratio), the memory, the start-up context taken; 330
      frames answered, the helper's own count of evaluated frames 330 with none failed, the Output black before the first
      frame (THE CONTROL of the reader) and the pattern after the last; the GPU time of each frame's list (the serve timer,
      --gpu_probe=1, one line per 300 frames)
      THE REBUILD (--rebuild=1920x1080, 330 frames more): ReleaseSet gives the set's context back, a new one is made, ratio 2.0
      picks Balanced (102), the picture again
  XS  XeSS, the SDR capture (B8G8R8A8, hdr 0): LDR_INPUT_COLOR, a context of the build's own (the start-up one was built
      for FP16, and is given back first), the same proofs of the picture, 60 frames
  V   (development helper) THE BARRIERS UNDER DIRECT3D 12's DEBUG LAYER: two XeSS builds and 240 frames with the helper's
      --d3d12_debug=2 (its control, a junk pipeline, must count at the probe); nothing at the rebuild's teardown, 0 at the exit
  U   an ultrawide output, 2293x960 -> 3440x1440: the mod's Quality lands one pixel under Ultra Quality's least (Intel rounds
      its optimal size up); held, Ultra Quality (104), and Intel's library says nothing of it
  D   DLSS (--backend=0) and F FSR (--backend=1), the same build and 330 frames: what the helper says of them is what it said
      before XeSS (no XeSS bit in the caps or the ack), and their GPU times beside XeSS's
  A   automatic (--backend=-1) on this machine's NVIDIA card, every library beside the helper: DLSS, and XeSS NOT ASKED (no
      XeSS bit at all: automatic on NVIDIA and AMD is unchanged)
  N   THE NOFILE CONTROL: --backend=2 with no libxess.dll beside the helper: the caps say 16384 and 65536 (NOFILE) without
      32768, the build is REFUSED with FEED_ACK_BACKEND_XESS, and the helper's line names the missing file
  NR, NU, NO  THE OTHER REFUSALS, each driven by its own cause through the helper's own probe, with a STAND-IN of this tool's
      own staged under Intel's file name (tools\test-xess\xess_standin.cpp; nothing of Intel's): a file that imports a module
      which is not there (LoadLibrary error 126: 65536's sibling NORUNTIME, 262144), a file whose xessD3D12CreateContext
      answers ERROR_UNSUPPORTED_DEVICE (UNSUPPORTED, 131072), and a file without Intel's entry points (16384 ALONE). Each
      build is refused with FEED_ACK_BACKEND_XESS and the reason's own words
  AI, AW  (development helper; fg18-fix, F6) AUTOMATIC FALLS FROM XeSS TO FSR AT A BUILD: a stand-in whose create works and whose init (AI)
      or warm-up (AW) fails, FSR's files beside it, --xess_auto=1 (automatic chooses as on Intel graphics): the first build refused with an
      ack that names FSR, the same build asked again answered by FSR, 60 frames. A2, THE CONTROL: upscaler=2 does not fall (its ack names XeSS)
  XS also prints its first build's time (fg18-fix, F7: the start-up context built for FP16 is not waited for at an SDR build)

    python tools\test-xess\pipe.py                    every session above; exit code 0 = all passed
    python tools\test-xess\pipe.py --helper=<exe>     the same against another helper (the shipped one, build-host -Release -Fg)
Needs this machine's graphics card and nothing else on it; opens no window.
"""
import hashlib, os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import ffx_pin  # noqa: E402

HELPER = os.path.join(ROOT, "host", "dlssiv-helper.exe")
XESS = os.path.join(ROOT, "external", "xess", "libxess.dll")
NGX = os.path.join(ROOT, "external", "ngx", "nvngx_dlss.dll")
TRANSPORT = os.path.join(ROOT, "source", "transport")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-xess-pipe-%d" % os.getpid())

CAP_XESS_PROBED, CAP_XESS, CAP_XESS_NOFILE, CAP_XESS_UNSUPPORTED, CAP_XESS_NORUNTIME = 16384, 32768, 65536, 131072, 262144
XESS_BITS = CAP_XESS_PROBED | CAP_XESS | CAP_XESS_NOFILE | CAP_XESS_UNSUPPORTED | CAP_XESS_NORUNTIME
ACK_SR_ACTIVE, ACK_BACKEND_FSR, ACK_BACKEND_XESS = 1, 4, 64

fails = []


def check(ok, what):
    print("  %s  %s" % ("PASS" if ok else "FAIL", what))
    if not ok: fails.append(what)
    return ok


def xess_pin():
    """Intel's file as external\\xess\\VERSION.txt records it: its size and SHA-256 read from that file, then from the DLL."""
    vt = open(os.path.join(ROOT, "external", "xess", "VERSION.txt"), encoding="utf-8", errors="replace").read()
    m = re.search(r"libxess\.dll\s+bin/libxess\.dll, ([\d,]+) bytes, SHA-256 ([0-9a-f]{64})", vt)
    if not m: raise SystemExit("FAIL: external\\xess\\VERSION.txt does not record libxess.dll's size and SHA-256 in the form this reads")
    size, sha = int(m.group(1).replace(",", "")), m.group(2)
    if not os.path.exists(XESS): raise SystemExit("FAIL: %s is not there" % XESS)
    got = hashlib.sha256(open(XESS, "rb").read()).hexdigest()
    if os.path.getsize(XESS) != size or got != sha:
        raise SystemExit("FAIL: %s is %d bytes, SHA-256 %s; VERSION.txt says %d, %s: a file nobody checked is not staged" % (XESS, os.path.getsize(XESS), got, size, sha))
    return size, sha


def stage(name, helper, xess=False, ngx=False, ffx=False):
    d = os.path.join(OUT, name)
    os.makedirs(d, exist_ok=True)
    shutil.copy2(helper, os.path.join(d, "dlssiv-helper.exe"))
    if xess: shutil.copy2(XESS, os.path.join(d, "libxess.dll"))
    if ngx: shutil.copy2(NGX, os.path.join(d, "nvngx_dlss.dll"))
    if ffx:
        for n in (ffx_pin.LOADER, ffx_pin.UPSCALER): ffx_pin.copy_pinned(n, d)
    return d


def run(name, folder, args):
    log = os.path.join(OUT, name + ".log")
    if os.path.exists(log): os.remove(log)
    p = subprocess.run([os.path.join(OUT, "pipe_smoke.exe"), os.path.join(folder, "dlssiv-helper.exe"), log] + args,
                       capture_output=True, text=True, cwd=OUT, timeout=600)
    text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
    out = p.stdout
    caps = re.search(r"^CAPS 0x([0-9A-F]+)", out, re.M)
    ack = re.search(r"^ACK \d+x\d+ ok (-?\d+) flags 0x([0-9A-F]+)", out, re.M)   # the first build's
    gpu = re.search(r"serve timing, (\d+) frames: .*?GPU (\d+) us mean, worst (\d+) \((\d+)\)", text)
    closed = re.search(r"pipe closed by (.+?) after (\d+) frames \((\d+) failed\)", text)
    acks = [(int(m.group(1)), int(m.group(2), 16), int(m.group(3))) for m in re.finditer(r"^ACK \d+x\d+ ok (-?\d+) flags 0x([0-9A-F]+) output_fmt \d+ result 0x[0-9A-F]+ \((\d+) ms\)", out, re.M)]
    r = {"out": out, "log": text, "rc": p.returncode, "acks": acks,
         "caps": int(caps.group(1), 16) if caps else None,
         "ok": int(ack.group(1)) if ack else None, "flags": int(ack.group(2), 16) if ack else None,
         "gpu": (int(gpu.group(2)), int(gpu.group(3)), int(gpu.group(4))) if gpu else None,
         "closed": (closed.group(1), int(closed.group(2)), int(closed.group(3))) if closed else None}
    print("== session %s: the stand-in's own verdict %s (exit %d), caps %s, ack ok %s flags %s" % (
        name, "PASS" if p.returncode == 0 else "FAIL", p.returncode, "0x%X" % r["caps"] if r["caps"] is not None else "none",
        r["ok"], "0x%X" % r["flags"] if r["flags"] is not None else "none"))
    for line in out.splitlines():
        if line.startswith("  FAIL") or line.startswith("OUTPUT") or line.startswith("BLACK") or line.startswith("FRAMES"): print("   ", line)
    check(p.returncode == 0, "%s: every proof of the stand-in game passed" % name)
    return r


def frames_proof(name, r, n):
    c = r["closed"]
    check(c is not None and c[0] == "the game" and c[1] == n and c[2] == 0,
          "%s: the helper's own count: %s" % (name, "closed by %s after %d frames (%d failed); %d expected, none failed" % (c[0], c[1], c[2], n) if c else "NO closing line"))


def main():
    helper = HELPER
    for a in sys.argv[1:]:
        if a.startswith("--helper="): helper = os.path.abspath(a[9:])
    if not os.path.exists(helper): print("FAIL: %s is not built" % helper); return 1
    size, sha = xess_pin()
    print("  Intel's libxess.dll: %d bytes, SHA-256 %s (external\\xess\\VERSION.txt's)" % (size, sha))
    print("  the helper: %s, %d bytes, SHA-256 %s" % (helper, os.path.getsize(helper), hashlib.sha256(open(helper, "rb").read()).hexdigest()))
    os.makedirs(OUT, exist_ok=True)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars64.bat")
    b = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /I"%s" "%s" /Fe:pipe_smoke.exe /Fo:pipe_smoke.obj /link d3d12.lib dxgi.lib'
                       % (vcvars, TRANSPORT, os.path.join(HERE, "pipe_smoke.cpp")), shell=True, capture_output=True, text=True, cwd=OUT)
    if b.returncode != 0 or "warning C" in b.stdout:
        print(b.stdout[-3000:]); print("FAIL: the stand-in game did not compile cleanly"); return 1

    probe = ["--gpu_probe=1"]   # the serve timer, a development helper's (a release helper skips the flag it does not know)
    sx = stage("xess", helper, xess=True)
    sd = stage("dlss", helper, ngx=True)
    sf = stage("fsr", helper, ffx=True)
    sa = stage("auto", helper, xess=True, ngx=True, ffx=True)
    sn = stage("nofile", helper)

    # ---- X: XeSS, FP16, the mod's Quality at 4K
    r = run("X", sx, ["--backend=2", "--frames=330", "--rebuild=1920x1080"] + probe)
    check(r["caps"] is not None and (r["caps"] & XESS_BITS) == (CAP_XESS_PROBED | CAP_XESS), "X: the caps say XeSS asked and able (16384 | 32768), no reason bit")
    check(r["flags"] is not None and (r["flags"] & (ACK_SR_ACTIVE | ACK_BACKEND_XESS | ACK_BACKEND_FSR)) == (ACK_SR_ACTIVE | ACK_BACKEND_XESS),
          "X: the ack says FEED_ACK_SR_ACTIVE | FEED_ACK_BACKEND_XESS, not FSR")
    L = r["log"]
    dev = re.search(r"host: build \S+ development, backend XeSS", L) is not None
    print("  the helper says it is %s" % ("a development build" if dev else "NOT a development build (no serve timer)"))
    check(re.search(r"XeSS \d+\.\d+\.\d+ loaded from .*libxess\.dll; a context created on this device", L) is not None, "X: the helper's log names XeSS's version and the file it loaded")
    check("2560x1440 -> 3840x2160 (ratio 1.500) is Ultra Quality (setting 104" in L, "X: the setting is picked BY RATIO: the mod's 1.5 is XeSS's Ultra Quality (104)")
    check(re.search(r"XeSS context ready: 2560x1440 -> 3840x2160, Ultra Quality, HDR colour, flags 0x102, the start-up context, .*temporary memory [\d.]+ MB of textures", L) is not None,
          "X: the context line: HDR, flags 0x102 (INVERTED_DEPTH | ENABLE_AUTOEXPOSURE), the start-up context taken, its memory")
    check(L.count("XeSS: warm-up done") == 2, "X: one warm-up execute inside each build (%d for 2 builds)" % L.count("XeSS: warm-up done"))
    check("1920x1080 -> 3840x2160 (ratio 2.000) is Balanced (setting 102" in L
          and re.search(r"XeSS context ready: 1920x1080 -> 3840x2160, Balanced, HDR colour, flags 0x102, a new context", L) is not None,
          "X: THE REBUILD at 1920x1080 (ratio 2.0): the set's context given back by ReleaseSet, a new one, Balanced (102) by ratio")
    check(len(re.findall(r"^\s*ACK \d+x\d+ ok 1 flags 0x41 ", r["out"], re.M)) == 2, "X: both builds' acks say SR_ACTIVE | BACKEND_XESS")
    frames_proof("X", r, 660)
    gx = r["gpu"]
    if dev: check(gx is not None and gx[2] > 0, "X: the serve timer read the frames' lists on the GPU: %s" % ("%d us mean, worst %d, over %d lists" % gx if gx else "NO LINE"))

    # ---- XS: XeSS, the SDR capture
    r = run("XS", sx, ["--backend=2", "--frames=60", "--sdr"])
    check(r["flags"] is not None and (r["flags"] & (ACK_SR_ACTIVE | ACK_BACKEND_XESS | ACK_BACKEND_FSR)) == (ACK_SR_ACTIVE | ACK_BACKEND_XESS), "XS: the ack says SR_ACTIVE | BACKEND_XESS")
    check(re.search(r"XeSS context ready: 2560x1440 -> 3840x2160, Ultra Quality, SDR colour, flags 0x142, a new context", r["log"]) is not None,
          "XS: the SDR build: flags 0x142 (with LDR_INPUT_COLOR), a context of its own")
    if dev:   # a log=2 line, which a release helper does not have
        # (fg18-fix, F7: NOT WAITED FOR at the build: given back there if its pipelines are done, else left parked and given back at the
        # device's release, waited for there; the old form waited up to 10 s for a build nobody uses)
        given = "XeSS: the start-up context is given back (built for the FP16 capture, this build is SDR)" in r["log"]
        parked = ("XeSS: the start-up context is still building its pipelines (built for the FP16 capture, this build is SDR): left parked" in r["log"]
                  and "XeSS: the start-up context is given back (the device goes)" in r["log"])
        check(given or parked, "XS: the start-up context, built for the FP16 capture, is given back at once or left parked and given back at the device's release (%s)"
              % ("given back at the build" if given else "parked" if parked else "NEITHER LINE"))
    xs_ms = r["acks"][0][2] if r["acks"] else None
    print("  XS: the first build (SDR, the start-up context built for FP16) answered in %s ms" % xs_ms)
    frames_proof("XS", r, 60)

    # ---- V: THE BARRIERS UNDER DIRECT3D 12's DEBUG LAYER (a development helper's lever, host\fg_lib.h: --d3d12_debug needs
    # --fg=1; frame generation is then asked and finds no provider here, which changes nothing of the upscaler). Two builds of
    # XeSS and 240 frames; the layer's count is read at the probe (with --d3d12_debug=2 a junk pipeline is made there on
    # purpose: THE INSTRUMENT'S CONTROL, it must count), at the rebuild's teardown (said only when it holds a message) and at
    # the exit.
    if dev:
        r = run("V", sx, ["--backend=2", "--frames=120", "--rebuild=1920x1080", "--fg=1", "--d3d12_debug=2"])
        L = r["log"]
        pm = re.search(r"d3d12 debug: the probe: (\d+) messages", L)
        check("d3d12 debug: on, asked before the first device" in L and pm is not None and int(pm.group(1)) >= 1,
              "V: the debug layer is on and its control counts (the probe: %s message(s), the planted junk pipeline)" % (pm.group(1) if pm else "NO LINE"))
        check(L.count("XeSS context ready") == 2, "V: both XeSS builds were made under the layer")
        check("d3d12 debug: teardown" not in L and "d3d12 debug: exit: 0 messages" in L,
              "V: 240 XeSS frames, a rebuild and the exit: the debug layer says NOTHING (no message at the rebuild's teardown, 0 at the exit)")
        frames_proof("V", r, 240)

    # ---- U: an ultrawide output, the mod's Quality one pixel under Ultra Quality's least (Intel rounds its optimal size up)
    r = run("U", sx, ["--backend=2", "--frames=60", "--output=3440x1440", "--render=2293x960"])
    check("2293x960 -> 3440x1440 (ratio 1.500) is Ultra Quality (setting 104, its optimal 2294x960" in r["log"]
          and "OUTSIDE its range" not in r["log"] and "XeSS ERROR" not in r["log"] and "XeSS warning" not in r["log"],
          "U: 2293x960 -> 3440x1440 is Ultra Quality (104), held one pixel under its least, and Intel's library says nothing of it")
    frames_proof("U", r, 60)

    # ---- D and F: DLSS and FSR as they were
    r = run("D", sd, ["--backend=0", "--frames=330"] + probe)
    check(r["caps"] is not None and (r["caps"] & XESS_BITS) == 0, "D: DLSS: no XeSS bit in the caps (XeSS not asked)")
    check(r["flags"] is not None and (r["flags"] & (ACK_SR_ACTIVE | ACK_BACKEND_XESS | ACK_BACKEND_FSR)) == ACK_SR_ACTIVE, "D: the ack says SR_ACTIVE alone (DLSS)")
    check("XeSS" not in r["log"], "D: the helper's log says nothing of XeSS")
    frames_proof("D", r, 330)
    gd = r["gpu"]
    r = run("F", sf, ["--backend=1", "--frames=330"] + probe)
    check(r["caps"] is not None and (r["caps"] & XESS_BITS) == 0, "F: FSR: no XeSS bit in the caps")
    check(r["flags"] is not None and (r["flags"] & (ACK_SR_ACTIVE | ACK_BACKEND_XESS | ACK_BACKEND_FSR)) == (ACK_SR_ACTIVE | ACK_BACKEND_FSR), "F: the ack says SR_ACTIVE | BACKEND_FSR")
    check("XeSS" not in r["log"], "F: the helper's log says nothing of XeSS")
    frames_proof("F", r, 330)
    gf = r["gpu"]

    # ---- A: automatic on NVIDIA
    r = run("A", sa, ["--backend=-1", "--frames=60"])
    check(r["caps"] is not None and (r["caps"] & XESS_BITS) == 0, "A: automatic on NVIDIA: XeSS not asked (no XeSS bit)")
    check("backend AUTO -> DLSS (NVIDIA; DLSS is available)" in r["log"] and "XeSS" not in r["log"], "A: automatic picked DLSS and said nothing of XeSS")
    frames_proof("A", r, 60)

    # ---- N: THE NOFILE CONTROL
    r = run("N", sn, ["--backend=2", "--expect-refused"])
    check(r["caps"] is not None and (r["caps"] & XESS_BITS) == (CAP_XESS_PROBED | CAP_XESS_NOFILE), "N: the caps say 16384 | 65536 (NOFILE) and not 32768")
    check(r["ok"] == 0 and r["flags"] is not None and (r["flags"] & (ACK_BACKEND_XESS | ACK_SR_ACTIVE | ACK_BACKEND_FSR)) == ACK_BACKEND_XESS,
          "N: the build is REFUSED and its ack names XeSS (BACKEND_XESS without SR_ACTIVE)")
    check("libxess.dll is not there: XeSS cannot run" in r["log"] and "this build wants XeSS and Intel's DLSSIV\\libxess.dll is missing: build refused" in r["log"],
          "N: the helper's lines name the missing file, at the probe and at the refused build")

    # ---- NR, NU, NO: the other refusals, each from its own cause, through stand-ins of this tool's own (never Intel's file)
    sd_ = os.path.join(OUT, "standins"); os.makedirs(sd_, exist_ok=True)
    src = os.path.join(HERE, "xess_standin.cpp")
    for kind, extra in (("DEP", ""), ("RUNTIME", " x2dep.lib"), ("UNSUPPORTED", ""), ("OTHER", ""), ("INITFAIL", ""), ("WARMFAIL", "")):
        b = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /LD /O2 /MT /W3 /DKIND_%s "%s" /Fe:%s.dll /Fo:%s.obj /link%s'
                           % (vcvars, kind, src, "x2dep" if kind == "DEP" else "standin-" + kind.lower(), kind.lower(), extra),
                           shell=True, capture_output=True, text=True, cwd=sd_)
        if b.returncode != 0: print(b.stdout[-2000:]); check(False, "the stand-in %s did not build" % kind); return 1
    for name, kind, want, probe_line, why in (
            ("NR", "runtime", CAP_XESS_PROBED | CAP_XESS_NORUNTIME, "would not load (error 126, a module it needs is missing",
             "libxess.dll would not load: it needs the Microsoft Visual C++ runtime 14.40 or later"),
            ("NU", "unsupported", CAP_XESS_PROBED | CAP_XESS_UNSUPPORTED, "xessD3D12CreateContext said ERROR_UNSUPPORTED_DEVICE: XeSS cannot run on this device",
             "Intel's library says this graphics card or driver cannot run XeSS"),
            ("NO", "other", CAP_XESS_PROBED, "is not Intel's XeSS library (entry points missing): XeSS cannot run",
             "libxess.dll could not be used (the XeSS line above says why)")):
        d = stage("standin-" + kind, helper)
        shutil.copy2(os.path.join(sd_, "standin-%s.dll" % kind), os.path.join(d, "libxess.dll"))
        r = run(name, d, ["--backend=2", "--expect-refused"])
        check(r["caps"] is not None and (r["caps"] & XESS_BITS) == want, "%s: the caps say 0x%X of XeSS's bits (%s)" % (name, want, kind))
        check(r["ok"] == 0 and r["flags"] is not None and (r["flags"] & (ACK_BACKEND_XESS | ACK_SR_ACTIVE | ACK_BACKEND_FSR)) == ACK_BACKEND_XESS,
              "%s: the build is REFUSED and its ack names XeSS" % name)
        check(probe_line in r["log"] and ("this build wants XeSS and %s: build refused" % why) in r["log"],
              "%s: the probe's line and the refusal's reason are the %s ones" % (name, kind))

    # ---- (fg18-fix, F6, the release audit of 2026-10-28) AUTOMATIC FALLS FROM XeSS TO FSR WHERE A BUILD'S XeSS CONTEXT FAILS. A stand-in whose
    # create works (the probe passes: the caps say XeSS runs) and whose init (AI) or warm-up (AW) fails; FSR's two files beside it. A development
    # helper's --xess_auto=1 makes automatic choose as on Intel graphics on this NVIDIA card. The first build is REFUSED and its ack names FSR
    # (the helper's g_backend is FSR's from the fall), the same build asked again is answered by FSR with its frames. A2, THE CONTROL (every
    # helper): upscaler=2 with the init stand-in does not fall: its refused ack names XeSS, and the helper says nothing of a fall.
    BACKS = ACK_SR_ACTIVE | ACK_BACKEND_XESS | ACK_BACKEND_FSR
    for name, kind, fail_line in (("AI", "initfail", "XeSS: xessD3D12Init said ERROR_INVALID_ARGUMENT"),
                                  ("AW", "warmfail", "XeSS: the warm-up execute FAILED; nothing was submitted: build refused")):
        if not dev: print("  %s: skipped (--xess_auto is a development helper's lever)" % name); continue
        d = stage("standin-" + kind, helper, ffx=True)
        shutil.copy2(os.path.join(sd_, "standin-%s.dll" % kind), os.path.join(d, "libxess.dll"))
        r = run(name, d, ["--backend=-1", "--xess_auto=1", "--refused-then-ok", "--frames=60"])
        a = r["acks"]
        check(r["caps"] is not None and (r["caps"] & XESS_BITS) == (CAP_XESS_PROBED | CAP_XESS), "%s: the probe passed: the caps say XeSS runs (16384 | 32768)" % name)
        check("backend AUTO -> XeSS (Intel; XeSS can run here" in r["log"], "%s: automatic chose XeSS at the probe" % name)
        check(len(a) == 2 and a[0][0] == 0 and (a[0][1] & BACKS) == ACK_BACKEND_FSR,
              "%s: the first build is REFUSED and its ack names FSR, not XeSS (the fall): %s" % (name, "ok %d flags 0x%X" % a[0][:2] if a else "NO ACK"))
        check(len(a) == 2 and a[1][0] == 1 and (a[1][1] & BACKS) == (ACK_SR_ACTIVE | ACK_BACKEND_FSR),
              "%s: the same build asked again is answered by FSR (SR_ACTIVE | BACKEND_FSR): %s" % (name, "ok %d flags 0x%X" % a[1][:2] if len(a) > 1 else "NO SECOND ACK"))
        check(fail_line in r["log"] and "backend AUTO: XeSS failed at this build (the XeSS line above says why): this build is refused, and the next runs FSR" in r["log"],
              "%s: the helper's lines say why XeSS's build failed and that the next build runs FSR" % name)
        frames_proof(name, r, 60)
    d = stage("standin-initfail-2", helper, ffx=True)
    shutil.copy2(os.path.join(sd_, "standin-initfail.dll"), os.path.join(d, "libxess.dll"))
    r = run("A2", d, ["--backend=2", "--expect-refused"])
    a = r["acks"]
    check(len(a) == 1 and a[0][0] == 0 and (a[0][1] & BACKS) == ACK_BACKEND_XESS and "backend AUTO: XeSS failed" not in r["log"],
          "A2 (THE CONTROL): upscaler=2 does not fall: its refused ack names XeSS, and the helper says nothing of a fall (%s)" % ("ok %d flags 0x%X" % a[0][:2] if a else "NO ACK"))

    if dev:
        print("  THE GPU TIME OF A FRAME'S LIST on this card (the serve timer: barriers and the upscaler's work, 2560x1440 -> 3840x2160, FP16):")
        for k, g in (("XeSS", gx), ("DLSS", gd), ("FSR", gf)):
            print("    %-5s %s" % (k, "%d us mean, worst %d, over %d lists" % g if g else "no reading"))
    print("FAIL: %d check(s)" % len(fails) if fails else "PASS")
    if not fails: shutil.rmtree(OUT, ignore_errors=True)
    else: print("  the logs are kept in %s" % OUT)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
