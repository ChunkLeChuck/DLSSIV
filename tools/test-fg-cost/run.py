r"""THE COST BENCH (2026-10-06, frame generation phase 3, its first stage; docs\reports\fg-3\cost-2026-10-06\reader-bench.md,
section 3, and critic.md, "THE ORDER OF WORK", steps 2 and 3).

WHAT DOES ONE GENERATED FRAME COST, BY ITS PARTS. The game's two cost legs priced a sum: reset dispatches, with wasted
prepares beside them and the two copies inside. A steady, interpolating pair had never been priced. This bench prices
AMD's prepare, AMD's dispatch (steady, told to reset, and every dispatch on the way back to steady) and the game's two
copies, each alone, at a desk, with no game and NO WINDOW, and then all of them together with the card as the limit:

  THE HELPER'S MARKS (host\fg_cost.h, development builds): four GPU timestamps on each of frame generation's own lists,
      a barrier before the stamp that closes AMD's work, the CPU's clock round AMD's calls, one row a list.
  THE FAKE CLIENT'S COST MODE (tools\test-fg-pipe\fgpipe_cost.h, --fake-fg=40): the product's two real copies out of an
      off-screen 3840x2160 FP16 back buffer, timed by Vulkan timestamps, on a moving picture with moving vectors, one
      arm for each part of a pair. Its head says what each arm sends.
  THIS SCRIPT builds the fake client under its own name (fgcost.exe) in its own folder under %TEMP%, stages the
      DEVELOPMENT helper as built in this tree with AMD's three libraries by their pins and NVIDIA's from external\ngx,
      runs the sessions, checks the controls and prints the tables. A session is one helper and one fake client.

THE SESSIONS (D: the guides at the display's size, DLAA; Q: at 2560x1440, DLSS Quality; both under 3840x2160 FP16):
  X            frame generation OFF (the helper started without --fg=1, with the marks' switch): the marks print nothing
  ID1, IQ1     IDLE, HUDLESS as the product sends it (a second copy of the same picture): every part of a pair alone on
               the card, ten arms, five rounds, the arms in a new order each round; then THE HASH PART, the whole
               generated image hashed behind every pair of five arms
  ID2, IQ2     the same once more: the same arm in two sessions must agree, and ONLY AGAINST THESE TWO may pictures be
               compared: AMD's library twice on the same inputs, which may or may not give the same image twice
  IDE, IQE     the same with NO HUD-LESS IMAGE: the set is asked without one (FEED_FG_BUILD_NO_HUDLESS, wire 2: the
               product's own shape since lever 1), one copy on this side, and the helper hands AMD an empty HUD-less
               colour because the set has no such image. (Until wire 2: a three-image set and the helper's
               --fghud_empty=1.) The helper's own line must say "two images"
  IDS          THE TRAP'S CONTROL (the specification's part 3): a THREE-image set, the copy into HUDLESS left out, and
               the helper still binds HUDLESS, which nothing ever wrote. Its steady pair must NOT sit on the half-way
               picture: that is what a game that dropped the copy beside a helper that still bound the image would show
  P1 P2 P4 P8  the marks' pad of 1, 2, 4 and 8 full-screen copies in each dispatch's list: a line through zero
  B0           the marks with NO barrier before the closing stamps: what the barrier changes
  H1, S0, S1,  DOES A BRACKET OF THE MARKS HOLD A STRANGER'S WORK (2026-10-10). In the game the prepare's bracket read 1,686 us
  UW, U0, U1   where this bench reads 48. The reading that was tested: the marks' head stamp has no barrier and their closing
               stamp has one, so the head would not wait for what the queue has in flight from the list BEFORE it (the
               upscaler's evaluate) and the closing stamp would. Two strangers, each with the bracket as it is and with
               a barrier before the head stamp too (the helper's --fgcost_head=1):
               S0, S1  THE STRANGER OF COPIES (--fgcost_pre=32: 32 full-screen copies in a list of the marks' own,
                       executed right before every prepare's list, nothing making them complete; sized by the pad's
                       line, sessions P1 to P8). DLAA guides. H1: the head barrier with no stranger.
               UW      A REAL UPSCALER'S EVALUATE every frame (the fake client's --fake-cost-upscale=1), WAITED FOR
                       before the pair's 'Q', as every idle arm waits: nothing is in flight. Quality guides.
               U0, U1  the same with 'Q' WRITTEN RIGHT BEHIND 'F' (--fake-cost-fq=1), the game's order: the evaluate is
                       in flight when the prepare's list reaches the queue. That it WAS in flight is read from the
                       marks' own "execute to first stamp", which must hold it.
               WHAT THEY SAID ON 2026-10-10: THE READING IS REFUTED. The bracket holds neither stranger, with or without
               the head barrier; the evaluate's time stands in "execute to first stamp", where it belongs
  LD1, LQ1     LOADED, at a baseline frame of about 5.4 ms and about 9.1 ms (the two frame times the game's cost legs
               had): the card is the limit, one frame always in flight, the period of the frame-end stamps, arm against
               baseline in halves A B B A. Arms: the null, prepare alone, prepare and copies, the steady pair, every
               pair reset, every second pair reset, the steady pair again, the steady pair under THE PRODUCT'S NO
               SOURCE RULE (the share of pairs it dispatches at a desk), the load's own honesty (8 more copies), frame
               generation off
  LDE, LQE     the same with HUDLESS empty: the null, prepare and copy, the steady pair, twice
  LDP          the marks' pad of 8 copies under load: what a helper's GPU time costs the frame
  LDM          THE MARKS OFF (the helper started without their switch): what the marks themselves cost the frame
  GC1, GC2     (the wire's one revision, 2026-10-06) the arms that RESET (FR every pair, F1 every second, FK every 17th),
               the camera cut said on 'Q' AND 'G' as the product writes it: a short idle part and the hash part, twice
  GCG          the same with the cut said on 'G' ALONE (the fake client's --fake-cost-cutg=1): the prepare is then not
               told to reset, only the dispatch. THE SAME IMAGES, pair by pair, say AMD's prepare does not need the
               reason (the specification's risk 1); read only against GC1 and GC2 agreeing with each other

HOW A NUMBER IS TAKEN, AND WHY NOT A PLAIN MEDIAN. Measured on this bench's first runs:
  - AMD's dispatch costs a different time for each of the picture's six moves (a fifth of it between the least and the
    largest), so a plain median of a cell lands on one move's cluster or the next. A cell's value is THE MEAN OVER THE
    SIX MOVES of one number a move: every move counts once.
  - THE CARD IS NEVER THIS BENCH'S ALONE: the desktop draws on it (measured: the desktop's own programs hold some 5
    percent of it), and what they draw lands inside a span as about 0.3 to 0.4 ms more, on a few samples or on most.
    Other work can only ADD to a span. So a GPU span's number a move is ITS LOWER QUARTILE (the samples nothing else
    touched), and where the medians read more than 2 percent higher that reading is printed beside it. The CPU's
    clocks have no such floor: they are medians, with their 99th percentile.
  - A PAIR IS STEADY FROM THE 12TH DISPATCH AFTER A RESET, NEVER EARLIER (AMD's optical flow search returns early for
    the first five dispatches after a reset and its colour stays out for about nine: its source at v2.3.0). Steady
    cells are taken BY THE HELPER'S OWN COUNT of dispatches since the last reset, a column of its rows, and every table
    says from which dispatch its steady cells were taken.
  - With the card as the limit the frame's period has several peaks even in the baseline: something else on this
    desktop takes the card for about 0.4 ms now and then (the owner uses this machine). So a loaded half is read two
    ways: ITS MEAN (what the frame rate paid, the other work included) and THE LOWER QUARTILE OF EACH MOVE, averaged
    (the frames nothing else touched). Both are printed; the median is printed last and trusted least.
EVERY ARM IS TAKEN AT LEAST THREE TIMES (rounds when idle, quads when loaded) and printed with its least and largest.

THE CONTROLS, each of which fails when broken: an empty bracket reads about zero on both sides; the pad's four points lie
on a line through zero; the same arm twice agrees within the null's noise; with frame generation off the marks print
nothing; the load's 8 extra copies cost the frame what their stamps say; every arm's correctness figure is what the arm
is (a steady pair sits on the half-way picture, a reset pair is a copy of this frame's, negated vectors spoil it).
EVERY COST IS PRINTED BESIDE ITS CORRECTNESS FIGURE: how far the generated image was from the half-way picture.

WHAT IS PRINTED BESIDE EVERY TABLE: the machine's load (the CPU's busy share before and over its session, and which
compilers were running), and for a loaded arm the GPU's clock before, during and after it (read with nvidia-smi, never
set). A loaded arm whose clock moved 5 percent is VOID and said so.

NOTHING IS LEFT BEHIND: a session that does not end is ended BY ITS OWN PROCESS, and at the end every process whose image
is under this tool's own folder is ended by its pid. Nothing is ever ended by name: a game's own helper must never be
touched. NO SESSION STARTS WHILE THE GAME RUNS, and a game that starts under a session ends that session (the card is
the game's then). A process that is only NAMED GTAIV.exe and whose image is under a megabyte is a test's stand-in (the
kit's own tests copy ping.exe under that name): it is said, and it stops nothing.

    python tools\test-fg-cost\run.py                 every session (66 minutes on 2026-10-06, 28 of them with the card as the limit)
    python tools\test-fg-cost\run.py --idle          the idle sessions and the controls only (38 minutes then)
    python tools\test-fg-cost\run.py --only=ID1,P4   the named sessions
    python tools\test-fg-cost\run.py --quick         every session at a fraction of its counts: the instrument, not numbers
    ... --keep                                       do not empty this tool's folder first
    ... --read                                       run nothing: read what the folder holds and print the tables
    ... --lib=NAME                                   WHICH LIBRARY GENERATES THE FRAMES (since the own build's stage 1b): loader (AMD's
                                                     through AMD's loader, the default), stock, own, own-md, alt-fi, alt-of. LIBS, below,
                                                     says what each is; a library other than the default has a folder of its own.
                                                     Every session proves which module it ran (check_identity).
                                                     tools\test-fg-own\run.py holds two libraries' pictures against each other
exit code 0 = the instrument held and every control held. The last line is PASS or FAIL. What it printed is kept as
result.txt in its folder. NOT part of tools\prove-all.py: it takes the card for minutes.
"""
import ctypes, ctypes.wintypes as wt, datetime, math, os, re, shutil, struct, subprocess, sys, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import ffx_pin

OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-cost")
HELPER = os.path.join(ROOT, "host", "dlssiv-helper.exe")
NGX = os.path.join(ROOT, "external", "ngx", "nvngx_dlss.dll")
ARGS = sys.argv[1:]
QUICK = "--quick" in ARGS
READ = "--read" in ARGS
KEEP = "--keep" in ARGS or READ
ONLY = next((a.split("=", 1)[1].split(",") for a in ARGS if a.startswith("--only=")), None)
NOWIN = subprocess.CREATE_NO_WINDOW
STEADY = 12               # a dispatch is steady from the 12th after one that reset (host\fg_cost.h, FGC_STEADY)
DROP_IDLE = 16            # pairs left out at the head of an idle cell, for what is read by a pair's place (this side's copies, the CPU's clocks)
DROP_LOAD = 32            # frames left out at the head of a loaded half (a reset's way back to steady, and the one read of GEN at frame 24)
ROUNDS, PAIRS = (2, 96) if QUICK else (5, 256)
QUADS, HALF = (2, 96) if QUICK else (4, 176)
HASH = 20 if QUICK else 48
LOADS = "5400,9100"
IDLE_ARMS = "A,P,PC,F,FR,F1,FK,FS,FN,FC"
FK_EVERY = 17             # arm FK: every 17th 'Q' is told a cut (fgpipe_cost.h, C_FK_EVERY)
STRANGER = 32             # sessions S0 and S1: the stranger's copies before each prepare's list (the helper's --fgcost_pre)
COMPILERS = ("cl.exe", "link.exe", "cmake.exe", "ninja.exe", "msbuild.exe", "fxc.exe", "dxc.exe")

NO_MARKS = ("X", "LDM", "OQ1", "OQ2", "OQ4", "OD1", "OD2", "OD4")   # the sessions whose helper is started without the marks' switch, or without frame generation (OQ, OD: the order part, lever 2)

# WHICH LIBRARY GENERATES THE FRAMES (2026-10-06, the own build's stage 1b; docs\DECISIONS.md, "PHASE 3: OUR OWN BUILD OF
# AMD'S FRAME GENERATION", section 4, stage 1). Until that day every session ran AMD's signed library through AMD's
# loader. Now the library is a named choice, and EVERY SESSION PROVES WHICH ONE IT RAN (check_identity): the helper's own
# lines name the module by its full path, its size and its SHA-256 (host\fg_lib.h), and they are held to the file this
# script staged.
#     --lib=loader    AMD's stock library through AMD's loader, as before the choice existed (the default)
#     --lib=stock     AMD's stock library, the same pinned file, loaded direct by the helper's --fglib: no loader in frame
#                     generation's path. The control of the direct load: it must give what the loader gives
#     --lib=own       ffx-own-fg.dll, this project's build of AMD's source (build-ffx-own.ps1)
#     --lib=own-md    its twin on the dynamic C runtime (build-ffx-own.ps1 -DynamicCrt): the first suspect when "own" differs
#     --lib=alt-fi    an own build with one constant changed in the interpolation shader (tools\ffx-own\build_alt.py): a
#                     compare of pictures MUST FAIL on it
#     --lib=alt-of    an own build with one constant changed in optical flow's search: the same
#     --lib=ours      dlssiv-fg.dll (the own build's stage 2, 2026-10-07): the same objects of AMD's two effects and backend
#                     behind THE MOD'S OWN INTERFACE (build-ffx-own.ps1 -Target dlssiv-fg), reached by the helper's --fgown
#                     and never by --fglib: three calls a pair, none of AMD's five entry points
#     --lib=ours-alt-fi, ours-alt-of   that library from the two altered sources (tools\ffx-own\build_alt.py): the same controls
#     --amd-fg-beside for an own library: AMD's frame generation file lies in the helper's folder all the same (it must not
#                     be mapped). Without it the helper's folder holds no frame generation file of AMD's at all
#     --lib-by-name   for an own library: it lies BESIDE THE HELPER (a folder made for it, with no file of AMD's) and the
#                     helper is given its bare file name, which it resolves to its own folder: the form a leg in the game uses
# NOTHING OF OURS IS EVER LINKED, NEVER LIES IN THE FOLDER WITH AMD'S FILES, AND NEVER CARRIES AN AMD NAME: an own library
# is COPIED into a folder of its own under this tool's folder (a hard link would let a write reach the source file), and the
# helper is given its full path.
OWN_OUT = os.path.join(ROOT, "obj")   # where build-ffx-own.ps1 and tools\ffx-own\build_alt.py leave what they build, a folder each
LIBS = {  # name: (what it is, where this tree's build of it lies, or None for AMD's pinned file)
    "loader": ("AMD's signed library through AMD's loader", None),
    "stock":  ("AMD's signed library loaded direct (--fglib), no loader", None),
    "own":    ("ffx-own-fg.dll, this project's build of AMD's source", os.path.join(OWN_OUT, "ffx-own", "ffx-own-fg.dll")),
    "own-md": ("ffx-own-fg-md.dll, the same on the dynamic C runtime", os.path.join(OWN_OUT, "ffx-own-md", "ffx-own-fg-md.dll")),
    "alt-fi": ("ffx-own-fg-alt-fi.dll, an own build ALTERED in the interpolation shader", os.path.join(OWN_OUT, "ffx-own-alt", "ffx-own-fg-alt-fi.dll")),
    "alt-of": ("ffx-own-fg-alt-of.dll, an own build ALTERED in optical flow's search", os.path.join(OWN_OUT, "ffx-own-alt", "ffx-own-fg-alt-of.dll")),
    "ours":   ("dlssiv-fg.dll, AMD's source behind the mod's own interface", os.path.join(OWN_OUT, "dlssiv-fg", "dlssiv-fg.dll")),
    "ours-alt-fi": ("dlssiv-fg-alt-fi.dll, the mod's own interface over a source ALTERED in the interpolation shader", os.path.join(OWN_OUT, "dlssiv-fg-alt", "dlssiv-fg-alt-fi.dll")),
    "ours-alt-of": ("dlssiv-fg-alt-of.dll, the mod's own interface over a source ALTERED in optical flow's search", os.path.join(OWN_OUT, "dlssiv-fg-alt", "dlssiv-fg-alt-of.dll")),
    # SINCE THE OWN BUILD'S STAGE 3 (2026-10-10) "ours" is THE PRODUCT: dlssiv-fg.dll with the cuts that are in (ffx-own\cuts);
    # the same build delivers stage 2's library with no cut under a reference name, and a control with one pixel changed
    "ref":     ("dlssiv-fg-ref.dll, THE REFERENCE: stage 2's library, no cut (its bytes call themselves dlssiv-fg.dll)", os.path.join(OWN_OUT, "dlssiv-fg-ref", "dlssiv-fg-ref.dll")),
    "ours-px": ("dlssiv-fg-px.dll, the exact library's cuts and ONE PIXEL of every generated image changed (a control)", os.path.join(OWN_OUT, "dlssiv-fg-alt", "dlssiv-fg-px.dll")),
    # SINCE 2026-10-12 THE PRODUCT'S PICTURE IS ITS OWN ("ours": the exact cuts and the picture cuts the owner accepted), and
    # "exact" is THE EXACT LIBRARY the same build delivers: the exact cuts and no picture cut, what the product was until
    # that day. It is the one the judge holds to the reference and to AMD's; the two altered libraries and the one-pixel
    # control above are built on ITS cuts (tools\ffx-own\cuts.py, at STATES; tools\test-fg-own\run.py --ours)
    "exact":   ("dlssiv-fg-exact.dll, THE EXACT LIBRARY: the exact cuts and no picture cut (its bytes call themselves dlssiv-fg.dll)", os.path.join(OWN_OUT, "dlssiv-fg-exact", "dlssiv-fg-exact.dll")),
}
OWN_IFACE = ("ours", "ours-alt-fi", "ours-alt-of", "ref", "ours-px", "exact")   # the libraries that speak the mod's own interface: the helper's --fgown (host\fg_own.h), never --fglib
LIB = next((a.split("=", 1)[1] for a in ARGS if a.startswith("--lib=")), "loader")
AMD_BESIDE = "--amd-fg-beside" in ARGS
BY_NAME = "--lib-by-name" in ARGS    # an own library lies beside the helper and is named by its bare file name, as a leg in the game names it
if LIB != "loader": OUT += "-" + LIB + ("-beside" if AMD_BESIDE else "") + ("-byname" if BY_NAME else "")   # a folder a library: two libraries' runs are kept side by side
GPU_USERS = ("dlssiv-helper.exe", "fgpipe*.exe", "fgcost*.exe", "dt_fg_test*.exe", "perfproof*.exe")   # one of these that is not this tool's own: somebody else has the card
GPU_LOOK = 30             # seconds between two looks while somebody else has the card (another bench's sessions follow each other within seconds: a caller that must not miss the gap sets it lower)
GPU_QUIET = 0             # seconds the card must have been nobody else's before a session starts (a caller whose sessions are TIMED sets it: session() says why)

passes, fails, standins = [], [], []


class Tee:
    def __init__(self, path): self.f = open(path, "w", encoding="utf-8"); self.o = sys.stdout
    def write(self, s): self.o.write(s); self.f.write(s)
    def flush(self): self.o.flush(); self.f.flush()


def proof(ok, sid, text):
    (passes if ok else fails).append((sid, text))
    print("  %s  [%s] %s" % ("PASS" if ok else "FAIL", sid, text))
    return ok


# ---------------------------------------------------------------------------------------------------------------- the machine
k32 = ctypes.WinDLL("kernel32", use_last_error=True)


class PE32(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ProcessID", wt.DWORD), ("th32DefaultHeapID", ctypes.c_size_t), ("th32ModuleID", wt.DWORD),
                ("cntThreads", wt.DWORD), ("th32ParentProcessID", wt.DWORD), ("pcPriClassBase", wt.LONG), ("dwFlags", wt.DWORD), ("szExeFile", wt.WCHAR * 260)]


k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [wt.DWORD, wt.DWORD]
k32.Process32FirstW.argtypes = [wt.HANDLE, ctypes.POINTER(PE32)]
k32.Process32NextW.argtypes = [wt.HANDLE, ctypes.POINTER(PE32)]
k32.OpenProcess.restype = wt.HANDLE
k32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
k32.QueryFullProcessImageNameW.argtypes = [wt.HANDLE, wt.DWORD, wt.LPWSTR, ctypes.POINTER(wt.DWORD)]
k32.TerminateProcess.argtypes = [wt.HANDLE, wt.UINT]
k32.CloseHandle.argtypes = [wt.HANDLE]
k32.GetSystemTimes.argtypes = [ctypes.POINTER(wt.FILETIME)] * 3


def processes():
    """[(pid, image name)] of every process, by a snapshot (no name is ever acted on: see ours())."""
    out = []
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    if not snap or snap == wt.HANDLE(-1).value: return out
    e = PE32(); e.dwSize = ctypes.sizeof(PE32)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        out.append((e.th32ProcessID, e.szExeFile))
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def image_of(pid):
    h = k32.OpenProcess(0x1000, False, pid)             # PROCESS_QUERY_LIMITED_INFORMATION
    if not h: return ""
    buf = ctypes.create_unicode_buffer(1024); n = wt.DWORD(1024)
    ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value if ok else ""


def ours():
    """The pids of the processes whose image is under THIS TOOL'S OWN FOLDER: the fake client and the helper it started.
    A game's helper runs from the game's folder and is never one of them."""
    root = os.path.normcase(os.path.abspath(OUT)) + os.sep
    return [(pid, name) for pid, name in processes() if name.lower() in ("fgcost.exe", "dlssiv-helper.exe") and os.path.normcase(image_of(pid)).startswith(root)]


def end_ours(why):
    left = ours()
    for pid, name in left:
        h = k32.OpenProcess(1, False, pid)              # PROCESS_TERMINATE
        if h:
            k32.TerminateProcess(h, 9); k32.CloseHandle(h)
        print("  ended by its pid (%s): %s, pid %d" % (why, name, pid))
    return left


def other_gpu_users():
    """The processes that take the card and are not this tool's own: another bench's helper or fake client, a proof's rig
    (GPU_USERS, by name; whose image is not under this tool's folder). ONE GPU USER AT A TIME: a session waits for them."""
    import fnmatch
    root = os.path.normcase(os.path.abspath(OUT)) + os.sep
    out = []
    for pid, name in processes():
        if not any(fnmatch.fnmatch(name.lower(), p) for p in GPU_USERS): continue
        img = image_of(pid)
        if img and os.path.normcase(img).startswith(root): continue
        out.append("%s pid %d (%s)" % (name, pid, img or "its image could not be read"))
    return out


def game_runs():
    """Is THE GAME running. A process named GTAIV.exe whose image is under a megabyte is not it (the game's is some fifteen;
    the kit's own tests run ping.exe under that name): it is noted and stops nothing. An image that cannot be read is taken
    for the game: the safe side."""
    for pid, name in processes():
        if name.lower() != "gtaiv.exe": continue
        img = image_of(pid)
        try: size = os.path.getsize(img) if img else -1
        except OSError: size = -1
        if 0 <= size < (1 << 20):
            note = "%s (%d bytes)" % (img, size)
            if note not in standins: standins.append(note)
            continue
        return True
    return False


def cpu_times():
    i, k, u = wt.FILETIME(), wt.FILETIME(), wt.FILETIME()
    k32.GetSystemTimes(ctypes.byref(i), ctypes.byref(k), ctypes.byref(u))
    f = lambda t: (t.dwHighDateTime << 32) | t.dwLowDateTime
    return f(i), f(k) + f(u)


def compilers_now():
    names = {}
    for _, name in processes():
        if name.lower() in COMPILERS: names[name.lower()] = names.get(name.lower(), 0) + 1
    return ", ".join("%s x%d" % kv for kv in sorted(names.items())) or "none"


def machine_load(seconds=3.0):
    """The CPU's busy share over a few seconds, and which compilers run: what stands beside every table (the machine is shared)."""
    i0, a0 = cpu_times(); time.sleep(seconds); i1, a1 = cpu_times()
    return {"cpu": 100.0 * (1.0 - (i1 - i0) / max(1, a1 - a0)), "compilers": compilers_now()}


def load_words(m):
    return ("CPU %.0f%% busy over 3 s before the session and %.0f%% over the whole of it (this tool's two processes among that); compilers running before it: %s, at its end: %s"
            % (m["cpu"], m["during"], m["compilers"], m["after"]))


class Smi:
    """nvidia-smi asked twice a second for the clock while a session runs (READ ONLY: this tool sets nothing), into a file."""
    def __init__(self, path):
        self.f = open(path, "w")
        try:
            self.p = subprocess.Popen(["nvidia-smi", "--query-gpu=timestamp,clocks.gr,utilization.gpu,power.draw,temperature.gpu", "--format=csv,noheader,nounits", "-lms", "500"],
                                      stdout=self.f, stderr=subprocess.DEVNULL, creationflags=NOWIN)
        except OSError:
            self.p = None

    def stop(self):
        if self.p:
            self.p.kill(); self.p.wait()
        self.f.close()


def read_smi(path):
    """[(unix ms, MHz, busy %, W, C)] from a sampler's file."""
    rows = []
    if not os.path.exists(path): return rows
    for l in open(path, encoding="utf-8", errors="replace"):
        p = [x.strip() for x in l.split(",")]
        if len(p) < 5: continue
        try:
            t = datetime.datetime.strptime(p[0], "%Y/%m/%d %H:%M:%S.%f").timestamp() * 1000.0
            rows.append((t, float(p[1]), float(p[2]), float(p[3]), float(p[4])))
        except ValueError:
            continue
    return rows


def clock_of(smi, t0, t1):
    """The GPU's clock before, during and after [t0, t1], and whether it moved 5 percent during."""
    during = [r for r in smi if t0 <= r[0] <= t1]
    before = [r[1] for r in smi if r[0] < t0][-1:]; after = [r[1] for r in smi if r[0] > t1][:1]
    if not during: return {"words": "the GPU's clock was not read during it", "void": False}
    mhz = sorted(r[1] for r in during); busy = sorted(r[2] for r in during); watt = sorted(r[3] for r in during)
    lo, hi, mid = mhz[0], mhz[-1], mhz[len(mhz) // 2]
    moved = (hi - lo) / mid if mid else 0.0
    return {"words": "GPU clock %s MHz before, %.0f during (%.0f to %.0f over %d reads; the card %.0f%% busy, %.0f W), %s after%s" % (
                ("%.0f" % before[0]) if before else "?", mid, lo, hi, len(mhz), busy[len(busy) // 2], watt[len(watt) // 2], ("%.0f" % after[0]) if after else "?",
                "; IT MOVED %.1f%% DURING THE ARM: VOID" % (100 * moved) if moved > 0.05 else ""),
            "void": moved > 0.05}


# ---------------------------------------------------------------------------------------------------------------- the build
def vcvars(bits):
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], creationflags=NOWIN).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars%d.bat" % bits)


def prepare():
    """The fake client built under this tool's own name, and a folder with the helper and the libraries. The folder, or None."""
    if not os.path.exists(HELPER):
        proof(False, "build", "%s is not built (build-host.ps1, no flags)" % HELPER); return None
    if b" development, backend" not in open(HELPER, "rb").read():
        proof(False, "build", "host\\dlssiv-helper.exe is not a development build: the cost marks exist in those alone (build-host.ps1 with no flags)"); return None
    bad = ffx_pin.cross_checks()
    if bad:
        for w in bad: print("  " + w)
        proof(False, "build", "the pins of AMD's libraries disagree with the packager's or with VERSION.txt"); return None
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /DWIN32 /I"%s" /I"%s" /I"%s" "%s" /Fe:fgcost.exe /Fo:fgcost.obj /link /INCREMENTAL:NO'
          % (vcvars(32), os.path.join(ROOT, "external", "vulkan"), os.path.join(ROOT, "source", "transport"), os.path.join(ROOT, "source", "fg"),
             os.path.join(ROOT, "tools", "test-fg-pipe", "fgpipe.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    if not proof(r.returncode == 0 and "warning C" not in r.stdout, "build", "the fake client compiles cleanly, 32-bit, under this tool's own name (fgcost.exe)"):
        print(r.stdout[-3000:]); return None
    d = os.path.join(OUT, "full")
    os.makedirs(d)
    shutil.copy2(HELPER, os.path.join(d, "dlssiv-helper.exe"))
    for n in (ffx_pin.LOADER, ffx_pin.UPSCALER, ffx_pin.FRAMEGEN):
        why = ffx_pin.check(n)
        if why:
            proof(False, "build", "%s: a library nobody has checked is not staged" % why); return None
        try: os.link(os.path.join(ffx_pin.FFX, n), os.path.join(d, n))
        except OSError: shutil.copy2(os.path.join(ffx_pin.FFX, n), os.path.join(d, n))
    shutil.copy2(NGX, os.path.join(d, os.path.basename(NGX)))
    # THE SAME FOLDER WITHOUT AMD'S FRAME GENERATION FILE: where an own library's sessions run, so that nothing of AMD's
    # frame generation is there to be found (--amd-fg-beside runs them in the folder above instead: the file must then
    # lie there unmapped)
    bare = os.path.join(OUT, "bare")
    os.makedirs(bare)
    shutil.copy2(HELPER, os.path.join(bare, "dlssiv-helper.exe"))
    for n in (ffx_pin.LOADER, ffx_pin.UPSCALER):
        try: os.link(os.path.join(ffx_pin.FFX, n), os.path.join(bare, n))
        except OSError: shutil.copy2(os.path.join(ffx_pin.FFX, n), os.path.join(bare, n))
    shutil.copy2(NGX, os.path.join(bare, os.path.basename(NGX)))
    print("the development helper as built in this tree, AMD's three libraries by their pins; the sessions run under %s" % OUT)
    return d


# ---------------------------------------------------------------------------------------------------------------- the library
def sha256_of(path):
    import hashlib
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""): h.update(b)
    return h.hexdigest()


_staged = {}


def stage_lib(lib):
    """The library a session runs, staged: {"lib", "what", "dir" (the helper's folder), "path" (the module's full path),
    "size", "sha", "args" (the helper's arguments), "direct", "own"}, or None with a FAIL line. AMD's file is the pinned one
    in the helper's folder. An own library is COPIED, never linked, into a folder of its own, never AMD's, under its own name."""
    key = (lib, AMD_BESIDE, BY_NAME)
    if key in _staged: return _staged[key]
    if lib not in LIBS:
        proof(False, "lib", "--lib=%s is not a library this bench knows (%s)" % (lib, ", ".join(LIBS))); return None
    what, src = LIBS[lib]
    full, bare = os.path.join(OUT, "full"), os.path.join(OUT, "bare")
    if src is None:
        path = os.path.join(full, ffx_pin.FRAMEGEN)
        st = {"lib": lib, "what": what, "dir": full, "path": path, "size": os.path.getsize(path), "sha": sha256_of(path), "direct": lib == "stock", "own": False}
        # (a file of AMD's name is refused by --fglib unless the control's own switch says it is meant: host\fg_lib.h)
        st["args"] = ["--fglib=" + path, "--fglib_sha=" + st["sha"], "--fglib_stock=1"] if lib == "stock" else ["--fgid=1"]
    else:
        if not os.path.exists(src):
            proof(False, "lib", "%s is not built: %s (build-ffx-own.ps1; -DynamicCrt for the twin; python tools\\ffx-own\\build_alt.py for the two altered ones)" % (lib, src)); return None
        name = os.path.basename(src)
        # BY ITS BARE NAME (--lib-by-name): the library lies BESIDE THE HELPER, as it will in the game's DLSSIV folder, and the
        # helper is given the file's name alone. That folder is made for it: the helper, NVIDIA's library and ours, and NO
        # file of AMD's (ours never lies in a folder with AMD's pinned files)
        own = os.path.join(OUT, "beside" if BY_NAME else "own")
        os.makedirs(own, exist_ok=True)
        if BY_NAME:
            for f in (HELPER, NGX):
                dst = os.path.join(own, os.path.basename(f))
                if not os.path.exists(dst): shutil.copy2(f, dst)
        path = os.path.join(own, name)
        if not os.path.exists(path): shutil.copyfile(src, path)   # a copy: never a link to the built file
        amd = [n for n in os.listdir(own) if n.lower().startswith("amd_")] + [n for n in os.listdir(bare) if n.lower() == ffx_pin.FRAMEGEN]
        samefile = os.path.samefile(src, path)
        if not proof(not name.lower().startswith("amd_") and not amd and not samefile and " " not in path, "lib",
                     "%s is staged as a COPY under its own name in a folder of its own (%s): no file there has an AMD name, and the helper's bare folder holds no %s" % (lib, path, ffx_pin.FRAMEGEN)):
            return None
        st = {"lib": lib, "what": what + (", BESIDE THE HELPER and named by its bare file name" if BY_NAME else ""), "dir": own if BY_NAME else full if AMD_BESIDE else bare,
              "path": path, "size": os.path.getsize(path), "sha": sha256_of(path), "direct": True, "own": True, "iface": lib in OWN_IFACE}
        if sha256_of(src) != st["sha"]:
            proof(False, "lib", "%s: the staged copy is not the built file any more (%s): empty this tool's folder" % (lib, src)); return None
        arg = "--fgown" if st["iface"] else "--fglib"
        st["args"] = [arg + "=" + (name if BY_NAME else path), arg + "_sha=" + st["sha"]]
    st["beside"] = os.path.exists(os.path.join(st["dir"], ffx_pin.FRAMEGEN))
    _staged[key] = st
    print("the library: %s (--lib=%s): %s, %d bytes, sha256 %s; the helper runs in %s, where AMD's %s %s" % (
        what, lib, st["path"], st["size"], st["sha"], st["dir"], ffx_pin.FRAMEGEN, "lies" if st["beside"] else "does NOT lie"))
    return st


def check_identity(s, st):
    """THE IDENTITY GATE of one session: the module that generated its frames, BY THE HELPER'S OWN LINES, is the file that was
    staged (full path, size, SHA-256), it was reached the way this library is meant to be reached, and for an own library no
    module of AMD's frame generation name was mapped in the helper at a set's end or at its exit."""
    sid, log = s["sid"], s["log"]
    norm = lambda p: os.path.normcase(os.path.abspath(p))
    frames = re.findall(r'frame generation: frames by (.+?), (\d+) bytes, sha256 ([0-9a-f]{64}), (loaded direct|mapped by AMD\'s loader), provider "([^"]+)" id (0x[0-9A-Fa-f]+)', log)
    direct = re.findall(r"frame generation: fglib (.+?), (\d+) bytes, sha256 ([0-9a-f]{64}): loaded direct", log)
    ours = re.findall(r"frame generation: fgown (.+?), (\d+) bytes, sha256 ([0-9a-f]{64}): loaded direct", log)
    iface = bool(st.get("iface"))
    if iface: direct, ours = ours, direct          # a library of the mod's own interface is loaded by --fgown, and --fglib must then have loaded nothing
    loader = re.findall(r"frame generation: fgid: (.+?), (\d+) bytes, sha256 ([0-9a-f]{64}): mapped by AMD's loader", log)
    mapped = re.findall(r"frame generation: fgid: (set end|exit): \S+ mapped: (no|yes, .*)", log)
    off = [l for l in log.splitlines() if "frame generation: fg" in l and l.rstrip().endswith(": off")]
    s["ident"] = {"frames": frames, "mapped": mapped}
    want = (norm(st["path"]), st["size"], st["sha"])
    got = [(norm(f[0]), int(f[1]), f[2]) for f in frames]
    how = "loaded direct" if st["direct"] else "mapped by AMD's loader"
    ok = proof(len(frames) == 1 and got[0] == want and frames[0][3] == how and not off, sid,
               "IDENTITY: the frames are by %s, %d bytes, sha256 %s, %s (the helper's own line, once; the staged file is that path, size and hash)%s" % (
                   frames[0][0] if frames else "NO MODULE NAMED", int(frames[0][1]) if frames else 0, frames[0][2][:16] + "..." if frames else "?", frames[0][3] if frames else "?",
                   "" if len(frames) == 1 and got[0] == want else "; STAGED: %s, %d bytes, %s" % (st["path"], st["size"], st["sha"])))
    first = direct if st["direct"] else loader
    ok = proof(len(first) >= 1 and all((norm(f[0]), int(f[1]), f[2]) == want for f in first) and not (loader if st["direct"] else direct) and not ours, sid,
               "IDENTITY: %s" % ("the helper hashed that file BEFORE it loaded it by its full path, and AMD's loader mapped no frame generation module for it" if st["direct"]
                                 else "AMD's loader mapped that file, and nothing was loaded direct")) and ok
    if iface:
        # THE MOD'S OWN INTERFACE: the library said what it is at its load, every set was created through that interface, and
        # the device answered every one of AMD's 18 pipelines (AMD's own code does not read those answers: host\fg_own.h does)
        said = re.findall(r'frame generation: fgown: interface (\d+), stage (\d+), "([^"]*)", AMD "([^"]+)" id (0x[0-9A-Fa-f]+)', log)
        sets = re.findall(r"frame generation: fgown: set: create (0x[0-9A-Fa-f]+); pipelines (\d+) asked, (\d+) refused; ([^\n]*)", log)
        built = len(re.findall(r"frame generation: created at ", log))
        s["own_iface"] = {"said": said, "sets": sets}
        ok = proof(len(said) >= 1 and len(set(said)) == 1 and len(sets) >= 1 and len(sets) == built and all(x[0] == "0x00000000" and x[1] == "18" and x[2] == "0" for x in sets), sid,
                   "IDENTITY: the frames are by THE MOD'S OWN INTERFACE (--fgown): the library says it is interface %s, stage %s, \"%s\"; %d set(s) created through it, each with 18 of AMD's pipelines asked and none refused" % (
                       said[0][0] if said else "?", said[0][1] if said else "?", said[0][2] if said else "?", len(sets))) and ok
    exits = [m for m in mapped if m[0] == "exit"]
    if st["own"]:
        ok = proof(len(exits) == 1 and mapped and all(m[1] == "no" for m in mapped), sid,
                   "IDENTITY: no module named %s was mapped in the helper at a set's end (%d line(s)) or at its exit (%d), with AMD's file %s its folder" % (
                       ffx_pin.FRAMEGEN, len(mapped) - len(exits), len(exits), "LYING IN" if st["beside"] else "not in")) and ok
    else:
        ok = proof(len(exits) == 1 and exits[0][1].startswith("yes, ") and norm(exits[0][1][5:].strip()) == want[0], sid,
                   "IDENTITY: at the exit the module of AMD's name that is mapped is the pinned file in the helper's folder") and ok
    return ok


# ---------------------------------------------------------------------------------------------------------------- the sessions
def P(sid, name): return os.path.join(OUT, "%s-%s" % (sid, name))


def plan():
    """[(id, what it is, the helper's arguments, the fake client's)]."""
    s = []
    idle = ["--fake-cost-idle=%d" % ROUNDS, "--fake-cost-pairs=%d" % PAIRS, "--fake-cost-arms=" + IDLE_ARMS, "--fake-cost-hash=%d" % HASH]
    load = ["--fake-cost-load=" + LOADS, "--fake-cost-quads=%d" % QUADS, "--fake-cost-half=%d" % HALF]
    few = ["--fake-cost-idle=%d" % min(ROUNDS, 3), "--fake-cost-pairs=%d" % min(PAIRS, 128)]
    on = ["--fg=1", "--gpu_probe=1"]
    gname = {"D": "DLAA", "Q": "Quality"}
    s.append(("X", "frame generation OFF: the helper started without --fg=1, with the marks' own switch", ["--gpu_probe=1"], ["--fake-want-fg=0"]))
    for g, gi in (("D", 0), ("Q", 1)):
        guides = ["--fake-cost-guides=%d" % gi]
        s.append(("I%s1" % g, "IDLE, %s guides, HUDLESS own" % gname[g], on, ["--fake-fg=40"] + guides + idle))
        s.append(("I%s2" % g, "the same once more", on, ["--fake-fg=40"] + guides + idle))
        s.append(("I%sE" % g, "IDLE, %s guides, NO HUD-LESS IMAGE (a two-image set, one copy on this side)" % gname[g], on, ["--fake-fg=40", "--fake-cost-hud=0"] + guides + idle))
    for k in (1, 2, 4, 8):
        s.append(("P%d" % k, "the marks' pad of %d full-screen cop%s in each dispatch's list (DLAA guides, the steady pair)" % (k, "y" if k == 1 else "ies"), on + ["--fgcost_pad=%d" % k],
                  ["--fake-fg=40"] + few + ["--fake-cost-arms=F"]))
    s.append(("B0", "the marks with NO barrier before the closing stamps (DLAA guides)", on + ["--fgcost_bar=0"], ["--fake-fg=40"] + few + ["--fake-cost-arms=P,F,FR"]))
    # DOES A BRACKET HOLD A STRANGER'S WORK (2026-10-10): the head barrier alone; a stranger of copies; a real evaluate in flight
    s.append(("H1", "the marks with a barrier before the head stamps too, no stranger (DLAA guides)", on + ["--fgcost_head=1"], ["--fake-fg=40"] + few + ["--fake-cost-arms=P,F,FR"]))
    s.append(("S0", "THE STRANGER OF COPIES (%d copies in a list of the marks' own before each prepare's list), the bracket as it is (DLAA guides)" % STRANGER,
              on + ["--fgcost_pre=%d" % STRANGER], ["--fake-fg=40"] + few + ["--fake-cost-arms=P,F,FR"]))
    s.append(("S1", "the same stranger with a barrier before the head stamps too (DLAA guides)", on + ["--fgcost_pre=%d" % STRANGER, "--fgcost_head=1"], ["--fake-fg=40"] + few + ["--fake-cost-arms=P,F,FR"]))
    up = ["--fake-fg=40", "--fake-cost-guides=1", "--fake-cost-upscale=1"] + few + ["--fake-cost-arms=P,F,FR"]
    s.append(("UW", "A REAL UPSCALER'S EVALUATE every frame, WAITED FOR before the pair's 'Q' (Quality guides): nothing is in flight in front of the prepare", on, up))
    s.append(("U0", "the same with 'Q' WRITTEN RIGHT BEHIND 'F', the game's order: the evaluate is in flight in front of the prepare; the bracket as it is", on, up + ["--fake-cost-fq=1"]))
    s.append(("U1", "the same with 'Q' right behind 'F' and a barrier before the head stamps too", on + ["--fgcost_head=1"], up + ["--fake-cost-fq=1"]))
    if "--idle" not in ARGS:
        for g, gi in (("D", 0), ("Q", 1)):
            guides = ["--fake-cost-guides=%d" % gi]
            s.append(("L%s1" % g, "LOADED, %s guides, HUDLESS own" % gname[g], on, ["--fake-fg=40"] + guides + load + ["--fake-cost-larms=NULL,P,PC,F,FR,F1,F2,FX,LK,OFF"]))
            s.append(("L%sE" % g, "LOADED, %s guides, NO HUD-LESS IMAGE" % gname[g], on, ["--fake-fg=40", "--fake-cost-hud=0"] + guides + load + ["--fake-cost-larms=NULL,PC,F,F2"]))
        s.append(("LDP", "LOADED, DLAA guides, the marks' pad of 8 copies in each dispatch's list", on + ["--fgcost_pad=8"], ["--fake-fg=40", "--fake-cost-guides=0"] + load + ["--fake-cost-larms=NULL,F,F2"]))
        s.append(("LDM", "LOADED, DLAA guides, THE MARKS OFF (no --gpu_probe)", ["--fg=1"], ["--fake-fg=40", "--fake-cost-guides=0"] + load + ["--fake-cost-larms=NULL,F,F2"]))
    # LEVER 1's CONTROL (2026-10-07): a three-image set, the HUD-less copy left out, HUDLESS still bound by the helper
    s.append(("IDS", "IDLE, DLAA guides, THE TRAP'S CONTROL: a three-image set, no copy into HUDLESS, the helper still binds it", on,
              ["--fake-fg=40", "--fake-cost-guides=0", "--fake-cost-hud=2"] + few + ["--fake-cost-arms=F,FR"]))
    # THE WIRE'S ONE REVISION (2026-10-06): a cut said on 'Q' and 'G' (twice), and on 'G' alone; the arms that reset, hashed
    cut = ["--fake-fg=40", "--fake-cost-guides=0"] + few + ["--fake-cost-arms=F,FR,F1,FK", "--fake-cost-hash=%d" % HASH, "--fake-cost-harms=FR,F1,FK"]
    s.append(("GC1", "IDLE, DLAA guides, the arms that reset, the camera cut on 'Q' and 'G' (as the product writes it)", on, cut))
    s.append(("GC2", "the same once more", on, cut))
    s.append(("GCG", "the same with the camera cut on 'G' ALONE", on, cut + ["--fake-cost-cutg=1"]))
    return [x for x in s if ONLY is None or x[0] in ONLY]


def plan_names():
    """Every session id this bench has, whatever --only and --idle leave of them this time."""
    global ONLY, ARGS
    only, args = ONLY, ARGS
    try:
        ONLY, ARGS = None, [a for a in ARGS if a != "--idle"]
        return {x[0] for x in plan()}
    finally:
        ONLY, ARGS = only, args


PLAN_NAMES = plan_names()


def session(d, sid, title, helper_args, fake_args):
    """One helper and one fake client, to its end or to the deadline. What it left is read by read_session()."""
    print("SESSION %s: %s" % (sid, title))
    waited = 0
    while game_runs():
        if waited == 0: print("  the game is running: the card is the game's. Waiting (no session starts while it runs)...")
        time.sleep(15); waited += 15
        if waited >= 1800:
            proof(False, sid, "the game ran for 30 minutes: this session was NOT RUN"); return False
    # ONE GPU USER AT A TIME: another bench's helper or fake client, or a proof's rig, has the card. Looked for every 30 s for
    # 30 minutes; then the session goes on and SAYS SO (its numbers shared the card)
    # A SESSION THAT IS TIMED (GPU_QUIET seconds, set by a caller whose numbers are times) WAITS FOR THE CARD TO HAVE BEEN
    # NOBODY ELSE'S THAT LONG: another bench starts its next session within seconds of its last, so "free at this look" is a
    # gap between two of its sessions and a session started in it would be measured beside the next one
    waited, quiet, others = 0, 0, other_gpu_users()
    while (others or quiet < GPU_QUIET) and waited < 1800:
        if waited == 0 and others: print("  another GPU user is running (%s): waiting, looking again every %d s, 30 minutes at most..." % ("; ".join(others), GPU_LOOK))
        step = GPU_LOOK if others else min(GPU_LOOK, 2)
        time.sleep(step); waited += step
        others = other_gpu_users()
        quiet = 0 if others else quiet + step
    if (others or quiet < GPU_QUIET) and GPU_QUIET:
        # a TIMED session does not go on beside somebody else: its numbers would be worth nothing and it would spoil theirs
        print("  THE CARD WAS NEVER NOBODY ELSE'S FOR %d s IN 30 MINUTES (%s): THIS TIMED SESSION IS NOT RUN" % (GPU_QUIET, "; ".join(others) or "another user came and went"))
        for name in ("rc.txt", "out.txt"):
            if os.path.exists(P(sid, name)): os.remove(P(sid, name))
        return False
    if others or quiet < GPU_QUIET: print("  STILL RUNNING AFTER 30 MINUTES, AND THIS SESSION GOES ON BESIDE IT: %s" % ("; ".join(others) or "(the card was never quiet for %d s)" % GPU_QUIET))
    elif waited > GPU_QUIET: print("  the card is free after %d s%s" % (waited, (" (nobody else's for the last %d s)" % GPU_QUIET) if GPU_QUIET else ""))
    m = machine_load()
    print("  the machine: CPU %.0f%% busy over 3 s; compilers running: %s" % (m["cpu"], m["compilers"]))
    args = list(fake_args) + list(helper_args)
    if sid != "X": args += ["--fake-want-fg=3", "--fake-cost-out=" + P(sid, "client.csv"), "--fake-cost-gen=" + P(sid, "gen.bin"), "--fake-cost-hashout=" + P(sid, "hash.csv")]
    if sid not in NO_MARKS: args += ["--fgcost_csv=" + P(sid, "helper.csv")]
    open(P(sid, "others.txt"), "w", encoding="utf-8").write("; ".join(others))
    if os.path.exists(P(sid, "beside.txt")): os.remove(P(sid, "beside.txt"))
    for name in ("client.csv", "gen.bin", "hash.csv", "helper.csv", "helper.log", "out.txt"):
        if os.path.exists(P(sid, name)): os.remove(P(sid, name))
    smi = Smi(P(sid, "smi.csv"))
    time.sleep(1.2)                                     # the clock is read at least twice before anything runs
    i0, a0 = cpu_times()
    p = subprocess.Popen([os.path.join(OUT, "fgcost.exe"), os.path.join(d, "dlssiv-helper.exe"), P(sid, "helper.log")] + args,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=d, creationflags=NOWIN)
    stop = threading.Event(); void = []; beside = []

    def watch():                                        # a game that starts under a session ends the session: its numbers would be the game's
        while not stop.wait(5.0):
            if game_runs():
                void.append("the game started during the session"); p.kill(); return
            for o in other_gpu_users():                 # and another GPU user that STARTS under a session is written down: its times shared the card
                if o not in beside: beside.append(o)
    threading.Thread(target=watch, daemon=True).start()
    try:
        out, _ = p.communicate(timeout=1800)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        p.kill(); out, _ = p.communicate(); rc = 99
        out = (out or "") + "\n  FAIL  the fake client did not finish in 1800 s and was ended by its own process\nFAIL\n"
    stop.set()
    i1, a1 = cpu_times()
    after = compilers_now()
    time.sleep(1.2)
    smi.stop()
    left = end_ours("left after session %s" % sid) if ours() else []
    open(P(sid, "out.txt"), "w", encoding="utf-8").write(out)
    open(P(sid, "beside.txt"), "w", encoding="utf-8").write("; ".join(beside))
    open(P(sid, "rc.txt"), "w").write("%d\n%s\n%d\n%.1f\n%s\n%.1f\n%s\n" % (rc, "; ".join(void), len(left), m["cpu"], m["compilers"], 100.0 * (1.0 - (i1 - i0) / max(1, a1 - a0)), after))
    return True


def read_csv(path):
    if not os.path.exists(path): return []
    rows = []
    with open(path, encoding="utf-8", errors="replace") as f:
        head = f.readline().strip().split(",")
        for l in f:
            p = l.strip().split(",")
            if len(p) != len(head): continue
            rows.append(dict(zip(head, p)))
    return rows


def read_session(sid):
    """Everything a session left, or None when it was not run."""
    if not os.path.exists(P(sid, "out.txt")) or not os.path.exists(P(sid, "rc.txt")): return None
    s = {"sid": sid}
    s["out"] = open(P(sid, "out.txt"), encoding="utf-8", errors="replace").read()
    rc = open(P(sid, "rc.txt")).read().split("\n")
    s["rc"] = int(rc[0]); s["void"] = rc[1]; s["left"] = int(rc[2])
    s["machine"] = {"cpu": float(rc[3]), "compilers": rc[4], "during": float(rc[5]), "after": rc[6]}
    s["others"] = open(P(sid, "others.txt"), encoding="utf-8").read().strip() if os.path.exists(P(sid, "others.txt")) else ""
    s["beside"] = open(P(sid, "beside.txt"), encoding="utf-8").read().strip() if os.path.exists(P(sid, "beside.txt")) else ""   # GPU users that started under it
    s["log"] = open(P(sid, "helper.log"), encoding="utf-8", errors="replace").read() if os.path.exists(P(sid, "helper.log")) else ""
    s["smi"] = read_smi(P(sid, "smi.csv"))
    lines = s["out"].splitlines()
    s["passed"] = [l for l in lines if l.startswith("  PASS  ")]; s["failed"] = [l for l in lines if l.startswith("  FAIL  ")]
    s["last"] = lines[-1].strip() if lines else ""
    kv = lambda l: dict(m.group(1, 2) for m in re.finditer(r"(\w+)=(\S+)", l))
    s["cells"] = [kv(l) for l in lines if l.startswith("  cost-cell ")]
    s["blocks"] = [kv(l) for l in lines if l.startswith("  cost-block ")]
    s["loads"] = [kv(l) for l in lines if l.startswith("  cost-load ")]
    ses = [kv(l) for l in lines if l.startswith("  cost-session ")]
    s["ses"] = ses[0] if ses else {}
    s["ns"] = float(s["ses"].get("ns_per_tick", "1")); s["qpc"] = float(s["ses"].get("qpc_hz", "10000000"))
    s["moves"] = [tuple(int(v) for v in mv.split(":")) for mv in s["ses"].get("moves", "").split("/") if ":" in mv]
    s["client"] = read_csv(P(sid, "client.csv"))
    for r in s["client"]:
        for k in ("rep", "half", "idx", "n", "flags", "move", "end_ticks", "t_start", "t_q", "t_g", "t_out", "t_end"): r[k] = int(r[k])
        for k in ("sc_us", "hm_us", "c1_us", "c1buf_us", "c2_us", "c2buf_us", "empty_us"): r[k] = float(r[k])
    s["move"] = {r["n"]: r["move"] for r in s["client"]}
    s["helper"] = read_csv(P(sid, "helper.csv"))
    for r in s["helper"]:
        for k in ("n", "reset", "since", "pad", "exec_qpc", "head_qpc"): r[k] = int(r[k])
        for k in ("gpu_us", "pad_us", "empty_us", "gap_us", "cpu_cfg_us", "cpu_begin_us", "cpu_rec_us", "cpu_exec_us"): r[k] = float(r[k])
    s["hP"] = {r["n"]: r for r in s["helper"] if r["kind"] == "P"}; s["hD"] = {r["n"]: r for r in s["helper"] if r["kind"] == "D"}
    s["hash"] = {(r["arm"], int(r["idx"])): r for r in read_csv(P(sid, "hash.csv"))}
    return s


# ---------------------------------------------------------------------------------------------------------------- arithmetic
def med(v):
    v = sorted(v)
    return v[len(v) // 2] if v else None


def pct(v, p):
    v = sorted(v)
    return v[int(p * (len(v) - 1))] if v else None


def fmt(x, nd=0):
    return "none" if x is None else ("%." + str(nd) + "f") % x


def spread(vals, nd=0):
    """The middle of several takes and how far they lie apart: "412 (405 to 419, 5 takes)"."""
    vals = [v for v in vals if v is not None]
    if not vals: return "none"
    return "%s (%s to %s, %d takes)" % (fmt(med(vals), nd), fmt(min(vals), nd), fmt(max(vals), nd), len(vals))


def agree(vals):
    """The design's rule for a cell that may be reported: its takes agree within 2 percent or 20 us."""
    vals = [v for v in vals if v is not None]
    if len(vals) < 2: return True
    return max(vals) - min(vals) <= max(20.0, 0.02 * abs(med(vals)))


def by_move(samples, low=False, floor=2):
    """[(move, value)] as ONE number in which every move of the picture counts once: each move's median (or its lower
    quartile), then their mean. And the moves' own numbers."""
    g = {}
    for mv, v in samples: g.setdefault(mv, []).append(v)
    per = {mv: (pct(v, 0.25) if low else med(v)) for mv, v in g.items() if len(v) >= floor}
    if not per: return None, {}
    return sum(per.values()) / len(per), per


def obs_of(cells):
    """The correctness figure of several cells together, weighted by the pairs each read: (pairs, mid, cur, still) or None."""
    n = sum(int(c.get("obs", 0)) for c in cells)
    if not n: return None
    if any("still" in c for c in cells): return (n, 0.0, 0.0, True)
    w = lambda k: sum(float(c[k]) * int(c["obs"]) for c in cells if int(c.get("obs", 0))) / n
    return (n, w("mid"), w("cur"), False)


def obs_words(o):
    if o is None: return "not read (no dispatch)"
    if o[3]: return "a still picture: nothing to be half-way between (%d pairs)" % o[0]
    return "%.3f from the half-way picture, %.3f from this frame's (%d pairs)" % (o[1], o[2], o[0])


# ---------------------------------------------------------------------------------------------------------------- idle
SERIES = [  # (key, what it is, the arm it is read in, which rows, the field)
    ("prep", "AMD's prepare", "F", "P0", "gpu_us"),
    ("prep_reset", "AMD's prepare, told to reset", "P", "P1", "gpu_us"),
    ("disp", "AMD's dispatch, STEADY (12th after a reset or later)", "F", "Ds", "gpu_us"),
    ("disp_reset", "AMD's dispatch, told to RESET", "FR", "Dr", "gpu_us"),
    ("disp_first", "AMD's dispatch, the FIRST after a reset (no search)", "F1", "Df", "gpu_us"),
    ("disp_still", "AMD's dispatch, steady, a STILL picture", "FS", "Ds", "gpu_us"),
    ("disp_neg", "AMD's dispatch, steady, NEGATED vectors", "FN", "Ds", "gpu_us"),
    ("disp_coll", "AMD's dispatch, steady, vectors that COLLIDE", "FC", "Ds", "gpu_us"),
    ("c1", "the game's copy into PRESENT", "F", "C", "c1_us"),
    ("c1buf", "its whole buffer (barriers, acquire, release)", "F", "C", "c1buf_us"),
    ("c2", "the game's copy into HUDLESS", "F", "C", "c2_us"),
    ("c2buf", "its whole buffer", "F", "C", "c2buf_us"),
    ("cpu_cfg", "CPU: AMD's configure", "F", "P0", "cpu_cfg_us"),
    ("cpu_pbegin", "CPU: the prepare's begin (the ring's slot, resets)", "F", "P0", "cpu_begin_us"),
    ("cpu_prec", "CPU: AMD's recording of the prepare", "F", "P0", "cpu_rec_us"),
    ("cpu_pexec", "CPU: the prepare's Close and Execute", "F", "P0", "cpu_exec_us"),
    ("cpu_dbegin", "CPU: the dispatch's begin", "F", "Ds", "cpu_begin_us"),
    ("cpu_drec", "CPU: AMD's recording of the dispatch", "F", "Ds", "cpu_rec_us"),
    ("cpu_dexec", "CPU: the dispatch's Close, Wait and Execute", "F", "Ds", "cpu_exec_us"),
    ("gap_prep", "execute to the prepare's first stamp", "F", "P0", "gap_us"),
    ("gap_disp", "execute to the dispatch's first stamp", "F", "Ds", "gap_us"),
    ("h_empty", "the helper's empty bracket", "F", "Ds", "empty_us"),
    ("c_empty", "this side's empty bracket", "F", "C", "empty_us"),
    ("pad", "the pad's copies", "F", "Ds", "pad_us"),
]


GPU_FIELDS = ("gpu_us", "pad_us", "c1_us", "c1buf_us", "c2_us", "c2buf_us")   # spans on the card: read by the lower quartile (other work only adds)


def cell_samples(s, cell, which, field):
    """One cell's samples of one series as [(move, value)], and the least count since a reset among the dispatches taken."""
    first, last = int(cell["first"]), int(cell["last"])
    out = []; least = None
    if which == "C":
        for r in s["client"]:
            if r["part"] == "idle" and first + DROP_IDLE <= r["n"] <= last and r[field] >= 0.0: out.append((r["move"], r[field]))
        return out, least
    for n in range(first, last + 1):
        d = s["hD"].get(n)
        if which[0] == "P":
            r = s["hP"].get(n)
            if r is None or n < first + DROP_IDLE or (which == "P0") == bool(r["reset"]): continue
            if which == "P0" and (d is None or d["since"] < STEADY): continue        # a steady pair's prepare
        else:
            r = d
            if r is None: continue
            if which == "Ds" and r["since"] < STEADY: continue
            if which == "Dr" and r["since"] != 0: continue
            if which == "Df" and r["since"] != 1: continue
            if which[:2] == "Dk" and r["since"] != int(which[2:]): continue
            if which == "Ds": least = r["since"] if least is None else min(least, r["since"])
        if r[field] < 0.0: continue
        out.append((s["move"].get(n, -1), r[field]))
    return out, least


def idle_table(s):
    """{key: [a value a round]} and the correctness figure of each arm, for one idle session."""
    t = {"takes": {}, "meds": {}, "obs": {}, "p99": {}, "min": {}, "moves": {}, "least": None}
    by_arm = {}
    for c in s["cells"]:
        if c.get("part") == "idle": by_arm.setdefault(c["arm"], []).append(c)
    for key, _, arm, which, field in SERIES:
        vals, meds, alls = [], [], []
        gpu = field in GPU_FIELDS
        for c in by_arm.get(arm, []):
            smp, least = cell_samples(s, c, which, field)
            v, _ = by_move(smp, low=gpu)
            if v is not None and len(smp) >= 8: vals.append(v); meds.append(by_move(smp)[0])
            alls += smp
            if key == "disp" and least is not None: t["least"] = least if t["least"] is None else min(t["least"], least)
        t["takes"][key] = vals; t["meds"][key] = meds
        t["p99"][key] = pct([x[1] for x in alls], 0.99); t["min"][key] = min([x[1] for x in alls]) if alls else None
        if key in ("disp", "disp_first", "prep"): t["moves"][key] = by_move(alls, low=True)[1]
    for arm, cells in by_arm.items(): t["obs"][arm] = obs_of(cells)
    # THE WAY BACK TO STEADY (arm FK: a cut every 17th pair): the dispatch's time by its count since the reset, every move once
    t["back"] = {}
    for k in range(FK_EVERY):
        vals, alls = [], []
        for c in by_arm.get("FK", []):
            smp, _ = cell_samples(s, c, "Dk%d" % k, "gpu_us")
            v, _ = by_move(smp, low=True, floor=1)
            if v is not None and len(smp) >= 4: vals.append(v)
            alls += smp
        t["back"][k] = (vals, by_move(alls, low=True)[0])
    # K's parts for a steady pair with every part alone: 'G' written to fgOut seen, split by the two sides' clocks (one clock: QPC)
    kparts = {"g_to_exec": [], "gap": [], "gpu": [], "end_to_seen": [], "whole": []}
    us = 1e6 / s["qpc"]
    for c in by_arm.get("F", []):
        first, last = int(c["first"]) + DROP_IDLE, int(c["last"])
        for r in s["client"]:
            if not (first <= r["n"] <= last) or not r["t_g"] or not r["t_out"]: continue
            d = s["hD"].get(r["n"])
            if d is None or d["gap_us"] < 0.0 or d["since"] < STEADY: continue
            kparts["g_to_exec"].append((d["exec_qpc"] - r["t_g"]) * us); kparts["gap"].append(d["gap_us"]); kparts["gpu"].append(d["gpu_us"] + d["pad_us"])
            kparts["end_to_seen"].append((r["t_out"] - d["exec_qpc"]) * us - d["gap_us"] - d["gpu_us"] - d["pad_us"]); kparts["whole"].append((r["t_out"] - r["t_g"]) * us)
    t["k"] = {k: (med(v), pct(v, 0.99)) for k, v in kparts.items() if v}
    return t


def move_words(s, per):
    if not per: return "none"
    name = lambda mv: ("(%d,%d)" % s["moves"][mv]) if 0 <= mv < len(s["moves"]) else "still"
    return ", ".join("%s %.0f" % (name(mv), v) for mv, v in sorted(per.items()))


def say_idle(name, sessions):
    """The idle table of one configuration, from the sessions that ran it (each a column of takes)."""
    have = [s for s in sessions if s is not None and s.get("idle") is not None]
    if not have: return
    print("\nIDLE, %s: every part of a pair ALONE on the card. Microseconds; GPU time by timestamps unless it says CPU." % name)
    print("  A round's value is the mean over the picture's six moves of each move's lower quartile (a GPU span) or median (the CPU's clocks, the gaps);")
    print("  printed: the middle round (least to largest round, rounds), then the least and the 99th of every sample, then the medians' reading where it is more than 2 percent above.")
    for s in have:
        cells = [x for x in s["cells"] if x.get("part") == "idle"]
        c = clock_of(s["smi"], min(int(x["t0"]) for x in cells), max(int(x["t1"]) for x in cells))
        print("  session %s: %s. %s" % (s["sid"], load_words(s["machine"]), c["words"].replace(": VOID", "")))
        print("             its steady cells were taken from dispatch %s after the last reset onward (%d or more is steady)" % (fmt(s["idle"]["least"]), STEADY))
    for key, what, arm, which, field in SERIES:
        if key == "pad" and not any(r["pad"] for s in have for r in s["helper"]): continue
        if not any(s["idle"]["takes"][key] for s in have): continue
        cols = []
        nd = 1 if (key.startswith("cpu") or key.endswith("empty") or key.startswith("gap")) else 0
        for s in have:
            v = s["idle"]["takes"][key]
            mm = med(s["idle"]["meds"][key]) if s["idle"]["meds"][key] else None
            cols.append("%s: %s%s%s%s" % (s["sid"], spread(v, nd), ("; samples %s to %s (99th)" % (fmt(s["idle"]["min"][key], nd), fmt(s["idle"]["p99"][key], nd))) if v else "",
                                          ("; BY THE MEDIANS %s: other work landed in these spans" % fmt(mm, nd)) if v and mm is not None and field in GPU_FIELDS and mm > 1.02 * med(v) + 2.0 else "",
                                          "" if agree(v) else " ROUNDS DISAGREE (more than 2 percent and 20 us)"))
        o = have[0]["idle"]["obs"].get(arm)
        print("  %-52s arm %-2s  %s%s" % (what, arm, "   ".join(cols), ("\n%60spicture: %s" % ("", obs_words(o))) if key.startswith("disp") else ""))
    for s in have:
        t = s["idle"]["takes"]
        m = lambda k: med(t[k]) if t[k] else None
        print("  BY THE PICTURE'S MOVE, %s (render pixels a frame): the steady dispatch %s; the first after a reset %s; the prepare %s" % (
            s["sid"], move_words(s, s["idle"]["moves"].get("disp")), move_words(s, s["idle"]["moves"].get("disp_first")), move_words(s, s["idle"]["moves"].get("prep"))))
        parts = [m("prep"), m("disp"), m("c1buf"), m("c2buf") if s["hud"] else 0.0]
        if None not in parts:
            print("  THE SUM FOR A STEADY PAIR, %s: prepare %.0f + dispatch %.0f + the cop%s %.0f = %.0f us of GPU time (HUDLESS %s)" % (
                s["sid"], parts[0], parts[1], "ies" if s["hud"] else "y", parts[2] + parts[3], sum(parts), "own" if s["hud"] else "empty"))
        back = s["idle"]["back"]
        if any(back[k][1] is not None for k in back):
            print("  THE WAY BACK TO STEADY, %s (arm FK, a reset every %dth pair; the dispatch's GPU us by its count since the reset, every move once): %s" % (
                s["sid"], FK_EVERY, ", ".join("%d: %s" % (k, fmt(back[k][1])) for k in sorted(back))))
            b3, late = back[3][1], [back[k][1] for k in range(STEADY, FK_EVERY) if back[k][1] is not None]
            if b3 is not None and late:
                print("      DISPATCH 3 AFTER A RESET %.0f us (rounds: %s); DISPATCH %d OR LATER %.0f us (the same arm), and %s in arm F: THE SEARCH AND WHAT COMES WITH IT COST %.0f us" % (
                    b3, spread(back[3][0]), STEADY, sum(late) / len(late), fmt(m("disp")), sum(late) / len(late) - b3))
        k = s["idle"]["k"]
        if k:
            print("  WHAT THE ANSWER WAITS FOR, %s (the steady pair, 'G' written to fgOut seen; medians, the 99th in brackets): the helper's thread to its execute %.0f us (%.0f),"
                  " execute to the first stamp %.0f (%.0f), the GPU %.0f (%.0f), the signal and this side's wake %.0f (%.0f): %.0f in all (%.0f)" % (
                      s["sid"], k["g_to_exec"][0], k["g_to_exec"][1], k["gap"][0], k["gap"][1], k["gpu"][0], k["gpu"][1], k["end_to_seen"][0], k["end_to_seen"][1], k["whole"][0], k["whole"][1]))


# ---------------------------------------------------------------------------------------------------------------- loaded
def block_stats(s, b):
    """One loaded arm: the period of the frame-end stamps by half, read three ways, the quads' differences, and what the marks read in its B halves."""
    part, arm, first, last = b["part"], b["arm"], int(b["first"]), int(b["last"])
    rows = [r for r in s["client"] if r["part"] == part and r["arm"] == arm and first <= r["n"] <= last]
    halves = {}
    for r in rows: halves.setdefault((r["rep"], r["half"]), []).append(r)
    ns = s["ns"]; hm = {}; quads = {}
    for (q, hf), fr in sorted(halves.items()):
        fr.sort(key=lambda r: r["idx"])
        per = [(fr[i]["move"], (fr[i]["end_ticks"] - fr[i - 1]["end_ticks"]) * ns / 1000.0) for i in range(1, len(fr))
               if fr[i]["idx"] > DROP_LOAD and fr[i]["end_ticks"] and fr[i - 1]["end_ticks"] and fr[i]["idx"] == fr[i - 1]["idx"] + 1]
        if len(per) < 24: continue
        v = [x[1] for x in per]
        quads.setdefault(q, {})[hf] = (sum(v) / len(v), by_move(per, low=True, floor=4)[0], med(v))
        hm[(q, hf)] = med([r["hm_us"] for r in fr if r["idx"] > DROP_LOAD and r["hm_us"] >= 0.0])
    d = [[], [], []]; base = [[], []]; hm_d = []
    for q, h in sorted(quads.items()):
        if len(h) != 4 or any(x[1] is None for x in h.values()): continue
        for k in range(3): d[k].append((h[1][k] + h[2][k]) / 2 - (h[0][k] + h[3][k]) / 2)
        base[0].append((h[0][0] + h[3][0]) / 2); base[1].append((h[0][1] + h[3][1]) / 2)
        hm_d.append((hm[(q, 1)] + hm[(q, 2)]) / 2 - (hm[(q, 0)] + hm[(q, 3)]) / 2)
    brows = [r for r in rows if (r["flags"] & 16) and r["idx"] > DROP_LOAD]
    bn = [r["n"] for r in brows]
    hd = [s["hD"][n] for n in bn if n in s["hD"]]; hp = [s["hP"][n] for n in bn if n in s["hP"]]
    c1 = [r["c1buf_us"] for r in brows if r["c1buf_us"] >= 0.0]; c2 = [r["c2buf_us"] for r in brows if r["c2buf_us"] >= 0.0]
    return {"arm": arm, "mean": d[0], "low": d[1], "med": d[2], "base": med(base[0]), "base_low": med(base[1]), "hm_delta": hm_d,
            "hp": med([r["gpu_us"] for r in hp]), "hd": med([r["gpu_us"] for r in hd]), "hd99": pct([r["gpu_us"] for r in hd], 0.99), "hgap": med([r["gap_us"] for r in hd if r["gap_us"] >= 0.0]),
            "hbeg": med([r["cpu_begin_us"] for r in hp]), "hbeg99": pct([r["cpu_begin_us"] for r in hp], 0.99), "hrec": med([r["cpu_rec_us"] for r in hd]),
            "hpad": med([r["pad_us"] for r in hd if r["pad"]]), "since": min([r["since"] for r in hd]) if hd else None,
            "c1": med(c1), "c2": med(c2), "clock": clock_of(s["smi"], int(b["t0"]), int(b["t1"])), "pairs": int(b.get("pairs", 0)), "disp": int(b.get("dispatched", 0)),
            "nosrc": int(b.get("nosource", 0)), "guards": int(b.get("guards", 0)), "frames": int(b["frames"]), "obs": obs_of([b]), "m": int(b["m"])}


LARM = {"NULL": "THE NULL: the baseline against itself", "P": "prepare alone ('Q', 'G' NOSOURCE)", "PC": "prepare and the copies ('G' WITHHELD)", "F": "THE STEADY PAIR, every pair dispatched",
        "FR": "every pair told to reset", "F1": "every second pair told to reset", "F2": "the steady pair once more", "FX": "the steady pair under the product's NO SOURCE rule",
        "LK": "the baseline with 8 more of the load's copies", "OFF": "frame generation OFF (the set released) against standby"}


def say_loaded(name, s):
    if s is None or not s.get("blocks"): return
    print("\nLOADED, %s, session %s: the card is the limit, one frame always in flight. WHAT THE ARM ADDS TO THE FRAME'S PERIOD, microseconds." % (name, s["sid"]))
    print("  Printed: by the halves' MEANS, the middle quad (least to largest quad, quads); then by each move's LOWER QUARTILE (the frames nothing else on this desktop touched); then by the halves' medians.")
    print("  the machine: %s" % load_words(s["machine"]))
    for part in sorted({b["part"] for b in s["blocks"]}, key=lambda p: int(p[4:])):
        st = {b["arm"]: block_stats(s, b) for b in s["blocks"] if b["part"] == part}
        ld = next((l for l in s["loads"] if "load" + l["target"] == part), {})
        any_ = next(iter(st.values())) if st else None
        print("  A BASELINE FRAME OF %s us by the halves' means, %s by the lower quartiles (asked: %s; %s full-screen copies a frame, each costing the frame %s us)" % (
            fmt(any_["base"]) if any_ else "?", fmt(any_["base_low"]) if any_ else "?", ld.get("target", "?"), ld.get("m", "?"), ld.get("copy_us", "?")))
        for arm in ("NULL", "P", "PC", "F", "FR", "F1", "F2", "FX", "LK", "OFF"):
            x = st.get(arm)
            if x is None: continue
            extra = ""
            if x["hd"] is not None:
                extra += ("; the marks under load (a span is elapsed time: the other queue's work runs inside it): prepare %s us, dispatch %s (99th %s), execute to first stamp %s;"
                          " CPU: the prepare's begin %s (99th %s), AMD's recording of the dispatch %s; the dispatches taken were the %sth after a reset or later" % (
                              fmt(x["hp"]), fmt(x["hd"]), fmt(x["hd99"]), fmt(x["hgap"]), fmt(x["hbeg"], 1), fmt(x["hbeg99"]), fmt(x["hrec"], 1), fmt(x["since"])))
            elif x["hp"] is not None: extra += "; the marks under load: prepare %s us" % fmt(x["hp"])
            if x["hpad"] is not None: extra += "; the pad's copies by the marks %s us" % fmt(x["hpad"])
            if x["c1"] is not None: extra += "; this side's copy buffers %s%s us" % (fmt(x["c1"]), (" and %s" % fmt(x["c2"])) if x["c2"] is not None else "")
            if arm == "FX": extra += "; dispatched %d of %d pairs (%.1f%%), no source %d" % (x["disp"], x["pairs"], 100.0 * x["disp"] / max(1, x["pairs"]), x["nosrc"])
            if arm == "LK": extra += "; the 8 copies by their own stamps: %s us" % spread(x["hm_delta"], 0)
            if x["pairs"]: extra += "; the guard was queued on %d of %d frames" % (x["guards"], x["frames"])
            apart = x["mean"] and x["low"] and abs(med(x["mean"]) - med(x["low"])) > max(60.0, 0.05 * abs(med(x["mean"])))
            print("    %-52s %-4s %s; lower quartiles %s; medians %s%s%s\n        %s%s" % (LARM[arm], arm, spread(x["mean"], 0), spread(x["low"], 0), fmt(med(x["med"])),
                                                                                      "; THE TWO READINGS ARE APART: other work on the card was not the same in this arm's halves" if apart else "",
                                                                                      ("\n        picture: " + obs_words(x["obs"])) if x["obs"] else "", x["clock"]["words"], extra))
        for which, label in (("mean", "by the means"), ("low", "by the lower quartiles")):
            g = lambda a: med(st[a][which]) if a in st and st[a][which] and not st[a]["clock"]["void"] else None
            if g("PC") is not None and g("F") is not None:
                p = g("P")
                print("    BY SUBTRACTION, %s: %sthe steady dispatch %.0f (F less PC)%s%s; THE STEADY PAIR IN ALL %.0f" % (
                    label, ("the prepare %.0f us of the frame, the cop%s %.0f (PC less P), " % (p, "ies" if s["hud"] else "y", g("PC") - p)) if p is not None else "",
                    g("F") - g("PC"), (", a reset dispatch %.0f (FR less PC)" % (g("FR") - g("PC"))) if g("FR") is not None else "",
                    (", the first dispatch after a reset %.0f (twice F1, less FR, less PC)" % (2 * g("F1") - g("FR") - g("PC"))) if g("FR") is not None and g("F1") is not None else "", g("F")))
        s.setdefault("lstats", {})[part] = st


def check_loaded(s):
    for part, st in sorted(s.get("lstats", {}).items(), key=lambda kv: int(kv[0][4:])):
        sid = s["sid"]
        null = st.get("NULL")
        if not null or not null["mean"]: continue
        noise = max(abs(x) for x in null["mean"])
        tol = max(3.0 * noise, 60.0)
        proof(abs(med(null["low"])) <= 25.0 and abs(med(null["mean"])) <= 100.0, sid, "%s: THE NULL reads %s us by the lower quartiles (25 us allowed) and %s by the means (100 allowed): the bench adds nothing of its own to a difference" % (
            part, spread(null["low"], 1), spread(null["mean"], 1)))
        if "F" in st and "F2" in st and st["F"]["mean"] and st["F2"]["mean"]:
            f, f2 = med(st["F"]["mean"]), med(st["F2"]["mean"])
            proof(abs(f - f2) <= max(tol, 0.03 * f), sid, "%s: THE SAME ARM TWICE: the steady pair adds %.0f us and, run again last, %.0f us: within the null's noise (%.0f us allowed)" % (part, f, f2, max(tol, 0.03 * f)))
        if "LK" in st and st["LK"]["low"]:
            lk, lkm, own8 = med(st["LK"]["low"]), med(st["LK"]["mean"]), med(st["LK"]["hm_delta"])
            proof(own8 > 50.0 and 0.85 <= lk / own8 <= 1.15, sid, "%s: THE LOAD IS HONEST: 8 more copies add %.0f us to the frame (%.0f by the means) and their own stamps say %.0f us (%.0f percent)" % (part, lk, lkm, own8, 100.0 * lk / own8))
        if "OFF" in st and st["OFF"]["low"]:
            proof(abs(med(st["OFF"]["low"])) <= 25.0, sid, "%s: FRAME GENERATION OFF AGAINST STANDBY (the set built and idle): %s us (by the means %s): a set that is not used costs the frame nothing" % (
                part, spread(st["OFF"]["low"], 1), fmt(med(st["OFF"]["mean"]), 1)))
        for arm in ("F", "F2"):
            x = st.get(arm)
            if not x: continue
            if x["obs"]: proof(x["obs"][1] <= 0.05, sid, "%s, arm %s under load: the picture: %s" % (part, arm, obs_words(x["obs"])))
            if sid in NO_MARKS: print("  (%s, arm %s: the marks are off, so no row says a dispatch's count since a reset: its frames are steady by their place, the %dth of a half and later, the half's first pair being the reset)" % (part, arm, DROP_LOAD + 1))
            else: proof(x["since"] is not None and x["since"] >= STEADY, sid, "%s, arm %s: every dispatch taken was the %sth after a reset or later (%d or more is steady)" % (part, arm, fmt(x["since"]), STEADY))
        voids = [a for a, x in st.items() if x["clock"]["void"]]
        proof(not voids, sid, "%s: the GPU's clock did not move 5 percent during any arm%s" % (part, (" (VOID: " + ", ".join(voids) + ")") if voids else ""))


# ---------------------------------------------------------------------------------------------------------------- the pictures
def read_gen(sid):
    """{(arm, rep, idx): the four tiles' bytes} from a session's tiles' file."""
    out = {}
    path = P(sid, "gen.bin")
    if not os.path.exists(path): return out
    b = open(path, "rb").read(); at = 0
    while at + 32 <= len(b):
        magic, arm, rep, idx, n, tiles, size = struct.unpack_from("<4siiiQii", b, at); at += 32
        if magic != b"GENT" or at + tiles * size > len(b): break
        out[(arm, rep, idx)] = b[at:at + tiles * size]; at += tiles * size
    return out


def half_to_float(h):
    return struct.unpack("<e", struct.pack("<H", h))[0]


def compare_gen(a, b):
    """Two sessions' tiles, pair by pair: (pairs compared, pairs equal bit for bit, differing values, all values, the largest difference)."""
    keys = sorted(set(a) & set(b))
    equal = diff = total = 0; worst = 0.0
    for k in keys:
        x, y = a[k], b[k]
        total += len(x) // 2
        if x == y: equal += 1; continue
        for i in range(0, len(x), 2):
            if x[i:i + 2] != y[i:i + 2]:
                diff += 1
                d = abs(half_to_float(struct.unpack_from("<H", x, i)[0]) - half_to_float(struct.unpack_from("<H", y, i)[0]))
                if d > worst: worst = d
    return len(keys), equal, diff, total, worst


def compare_hash(a, b):
    """Two sessions' whole-image hashes, arm by arm: {arm: (pairs, equal in the three colours, equal in all four channels, the
    largest share of the 144 blocks that differ in one pair, the mean share over the pairs that differ)}."""
    out = {}
    for arm in sorted({k[0] for k in a} & {k[0] for k in b}):
        keys = sorted(k for k in a if k[0] == arm and k in b)
        col = allc = 0; shares = []
        for k in keys:
            x, y = a[k], b[k]
            if x["all"] == y["all"]: allc += 1
            if x["colour"] == y["colour"]: col += 1; continue
            bx, by = x["blocks"], y["blocks"]
            shares.append(sum(1 for i in range(0, min(len(bx), len(by)), 8) if bx[i:i + 8] != by[i:i + 8]) / 144.0)
        out[arm] = (len(keys), col, allc, max(shares) if shares else 0.0, (sum(shares) / len(shares)) if shares else 0.0)
    return out


def hash_words(c):
    return "; ".join("arm %s: %d of %d pairs the same in the three colours (%d in all four channels)%s" % (
        arm, v[1], v[0], v[2], "" if v[1] == v[0] else ", a differing pair differs in %.0f%% of the picture's 144 blocks on average, %.0f%% at most" % (100 * v[4], 100 * v[3])) for arm, v in c.items())


# ---------------------------------------------------------------------------------------------------------------- the reading
def check_session(s, want_cost):
    sid = s["sid"]
    ok = proof(s["rc"] == 0 and s["last"] == "PASS" and not s["failed"] and not s["void"] and s["left"] == 0, sid,
               "the fake client's own verdict: %d proofs passed, %d failed, exit code %d%s; it left no process behind" % (len(s["passed"]), len(s["failed"]), s["rc"], ("; VOID: " + s["void"]) if s["void"] else ""))
    for l in s["failed"][:6]: print("      " + l.strip()[:260])
    if not want_cost: return ok
    log = s["log"]
    lines = [l for l in log.splitlines() if "frame generation cost:" in l]
    on = [l for l in lines if "the marks are ON" in l]; armed = [l for l in lines if ": ARMED on" in l]; ends = [l for l in lines if "at the set's end" in l]
    ok = proof(len(on) == 1 and len(armed) >= 1 and len(ends) == len(armed) and "NOT ARMED" not in log, sid,
               "the helper's marks: ON said once, ARMED for %d set(s), a closing line for each (%d)" % (len(armed), len(ends))) and ok
    nt = [int(m.group(1)) for m in re.finditer(r"not timed (\d+) of \d+ lists", "\n".join(ends))]
    lists = sum(int(m.group(1)) for m in re.finditer(r"not timed \d+ of (\d+) lists", "\n".join(ends)))
    ok = proof(bool(nt) and sum(nt) == 0 and "rows LOST" not in log and len(s["helper"]) == lists, sid,
               "every list of frame generation's was timed (%d not timed of %d) and every one has its row (%d rows)" % (sum(nt), lists, len(s["helper"]))) and ok
    ok = proof("RAISED" not in log and "did not retire its allocator" not in log and "failed to close" not in log and "DUPLICATE: " not in log and "DRIFT: " not in log, sid,
               "nothing of frame generation raised, its ring never refused a list, and the helper read no pair out of step") and ok
    # (the set's shape, by the helper's own words: its marks say "HUDLESS none" for a set with no HUD-less image, and its
    # create line counts the images)
    s["hud"] = 0 if ("--fghud_empty: every configure hands AMD an EMPTY" in log or ", HUDLESS none" in log) else 1
    # (WHICH SHAPE A SESSION MUST HAVE: this bench's own sessions by their names; a session of another runner's, which this
    # bench has no name for (tools\test-fg-own\run.py's), by what its fake client was told: --fake-cost-hud=0 asks for a set
    # with no HUD-less image, and the helper's own lines must then say so)
    if sid in ("IDE", "IQE", "LDE", "LQE") or (sid not in PLAN_NAMES and s["ses"].get("hud") == "0"):
        ok = proof(", two images " in log and ", three images " not in log and ", HUDLESS none" in log and "--fghud_empty" not in log and s["ses"].get("hud") == "0", sid,
                   "THIS SESSION'S SET HAS NO HUD-LESS IMAGE, by the helper's own lines: every set it created has \"two images\", its marks say \"HUDLESS none\", and its --fghud_empty lever was not used") and ok
    elif sid != "X":
        ok = proof(", two images " not in log and ", HUDLESS none" not in log, sid, "this session's sets have three images, by the helper's own lines") and ok
    m = re.search(r'frame generation: available, provider "([^"]+)"', log)
    s["provider"] = m.group(1) if m else "?"
    return ok



# ---------------------------------------------------------------------------------------------------------------- the order part
# LEVER 2 (2026-10-10; docs\reports\fg-3\lever2-2026-10-10): WHERE 'Q' IS WRITTEN, MEASURED AT THE DESK.
#     python tools\test-fg-cost\run.py --order [--quick] [--ogap=US] [--oload=US]
# One session a guide size and a regime (OQ1 to OD4: DLSS Quality and DLAA; the 59 cap, the 118 cap, the load), the development helper WITHOUT the cost marks (nothing of an
# instrument's on the helper's lists), the game's set (no HUD-less image). The fake client's order part (fgpipe_cost.h)
# runs arm F with every pair dispatched in three regimes (the frame paced at 59 and at 118 a second with no load, and the
# card as the limit), 'Q' AT THE HAND against 'Q' RIGHT BEHIND THE FRAME MESSAGE, three takes of each, alternating. This
# reads its cells and says each figure by take, with the spread. THE SESSIONS ARE TIMED: none starts until the card has
# been nobody else's for 20 s, and one under which another GPU user STARTED is thrown away and taken again (ORDER_TRIES).
ORDER = "--order" in ARGS
ORDER_FRAMES = 160 if QUICK else 400
ORDER_GAP = next((int(a.split("=", 1)[1]) for a in ARGS if a.startswith("--ogap=")), 2000)
ORDER_LOAD = next((int(a.split("=", 1)[1]) for a in ARGS if a.startswith("--oload=")), 9100)


ORDER_TRIES = 8           # a session under which another GPU user started is taken again, this often at most
ORDER_REGS = ((1, "the frame paced at 59 a second"), (2, "the frame paced at 118 a second"), (4, "the card as the limit"))


def order_plan():
    # A SESSION A REGIME AND A GUIDE SIZE (about half a minute each): on a card another bench shares, a short session is one
    # that can be had to itself
    base = ["--fake-fg=40", "--fake-cost-hud=0", "--fake-cost-order=%d" % ORDER_FRAMES, "--fake-cost-oload=%d" % ORDER_LOAD, "--fake-cost-ogap=%d" % ORDER_GAP]
    s = []
    for g, gi, gname in (("Q", 1, "DLSS Quality guides (2560x1440)"), ("D", 0, "DLAA guides (3840x2160)")):
        for bit, rname in ORDER_REGS:
            s.append(("O%s%d" % (g, bit), "THE ORDER OF 'Q', %s, %s, the marks off" % (gname, rname), ["--fg=1"], base + ["--fake-cost-guides=%d" % gi, "--fake-cost-oreg=%d" % bit]))
    return s


def order_cells(sid):
    """The fake client's "cost-order" lines of a session, as dicts; and what stood beside it."""
    out = open(P(sid, "out.txt"), encoding="utf-8", errors="replace").read() if os.path.exists(P(sid, "out.txt")) else ""
    cells = []
    for l in out.splitlines():
        if not l.strip().startswith("cost-order "): continue
        d = dict(kv.split("=", 1) for kv in l.split()[1:])
        cells.append({k: (v if k == "regime" else float(v)) for k, v in d.items()})
    rc = open(P(sid, "rc.txt")).read().split("\n") if os.path.exists(P(sid, "rc.txt")) else []
    rd = lambda n: open(P(sid, n), encoding="utf-8", errors="replace").read().strip() if os.path.exists(P(sid, n)) else ""
    return {"out": out, "cells": cells, "rc": int(rc[0]) if rc and rc[0].strip().lstrip("-").isdigit() else -1, "void": rc[1] if len(rc) > 1 else "",
            "cpu0": float(rc[3]) if len(rc) > 3 and rc[3] else -1.0, "comp0": rc[4] if len(rc) > 4 else "?", "cpu": float(rc[5]) if len(rc) > 5 and rc[5] else -1.0, "comp1": rc[6] if len(rc) > 6 else "?",
            "others": rd("others.txt"), "beside": rd("beside.txt"), "log": rd("helper.log")}


def order_main(d, st):
    global GPU_QUIET
    GPU_QUIET = max(GPU_QUIET, 20)
    names = {0: "'Q' AT THE HAND (the order until 2026-10-10; fg_q_at_hand=1)", 1: "'Q' RIGHT BEHIND THE FRAME MESSAGE (the product since)"}
    three = lambda vals, nd=0: (", ".join(("%." + str(nd) + "f") % v for v in vals) + " (spread %s)" % (("%." + str(nd) + "f") % (max(vals) - min(vals)))) if vals else "NO TAKES"
    mid = lambda vals: sorted(vals)[len(vals) // 2] if vals else 0.0
    print("\nTHE ORDER OF 'Q', AT THE DESK. 3840x2160 FP16, the development helper with no marks, the fake client, no game, no window. %d frames a cell (the first 32 left out), three takes a cell, every pair dispatched; the CPU waits %d us between the frame message and the hand; the loaded regime's baseline frame: %d us."
          % (ORDER_FRAMES, ORDER_GAP, ORDER_LOAD))
    for sid, title, hargs, fargs in order_plan():
        s = None
        for attempt in range(1, ORDER_TRIES + 1):
            if not READ and not (ONLY and sid not in ONLY):   # (--only=OQ1,...: the others are read as an earlier run with --keep left them)
                if not session(st["dir"], sid, title, hargs + st["args"], fargs):
                    proof(False, sid, "this session was NOT RUN (the card was never free)"); break
            s = order_cells(sid)
            if READ or not s["beside"]: break
            print("  [%s] ANOTHER GPU USER STARTED UNDER THIS SESSION (%s): ITS TABLE IS THROWN AWAY and taken again (try %d of %d)" % (sid, s["beside"], attempt, ORDER_TRIES))
            s = None
        if s is None:
            proof(False, sid, "no table: another GPU user started under each of %d tries, or the session did not run" % ORDER_TRIES); continue
        last = [l for l in s["out"].splitlines() if l.strip()][-1].strip() if s["out"].strip() else ""
        proof(s["rc"] == 0 and last == "PASS" and "  FAIL  " not in s["out"] and not s["void"], sid, "the fake client's own verdict: exit code %d, its last line %s%s" % (s["rc"], last or "NOTHING", ("; VOID: " + s["void"]) if s["void"] else ""))
        print("  [%s] THE MACHINE: CPU %.0f%% busy over 3 s before the session and %.0f%% over the whole of it (this tool's two processes among that); compilers running before it: %s, at its end: %s; another GPU user beside it: %s"
              % (sid, s["cpu0"], s["cpu"], s["comp0"], s["comp1"], s["others"] or s["beside"] or "none"))
        regimes = []
        for c in s["cells"]:
            if c["regime"] not in regimes: regimes.append(c["regime"])
        proof(len(s["cells"]) == 6 and len(regimes) == 1, sid, "its regime's six cells were run (%d cells in %s)" % (len(s["cells"]), ", ".join(regimes) or "NO REGIME"))
        for rg in regimes:
            what = "the frame paced at %s frames a second, no load" % rg[3:] if rg.startswith("cap") else "THE CARD IS THE LIMIT: the baseline frame takes %s us, one frame in flight" % rg[4:]
            print("\n  [%s] %s (%s):" % (sid, rg, what))
            med = {}
            for q in (0, 1):
                cs = sorted([c for c in s["cells"] if c["regime"] == rg and int(c["q"]) == q], key=lambda c: c["take"])
                pct = [100.0 * c["behind_n"] / max(1.0, c["pairs"]) for c in cs]
                print("      %s" % names[q])
                print("          the prepare retired BEHIND the handoff on %s percent of pairs, by (us, the mean over those) %s" % (three(pct), three([c["behind_mean_us"] for c in cs])))
                print("          the prepare's retire less the handoff's, the median (us; negative: the prepare came first): %s" % three([c["prep_minus_handoff_med_us"] for c in cs]))
                print("          K, the handoff's retire to the answer (us): median %s; 90th %s" % (three([c["k_med_us"] for c in cs]), three([c["k_p90_us"] for c in cs])))
                print("          'G' written to the answer seen (us): %s" % three([c["g_to_answer_med_us"] for c in cs]))
                print("          THE FRAME'S OWN ANSWER, the frame message written to its out-fence (us): median %s; 90th %s" % (three([c["f_answer_med_us"] for c in cs]), three([c["f_answer_p90_us"] for c in cs])))
                print("          the frame's period (us): %s; waits queued behind the prepare: %s; stamps taken late: %s; pairs: %s" % (
                    three([c["period_us"] for c in cs], 1), three([c["guards"] for c in cs]), three([c["late_stamps"] for c in cs]), three([c["pairs"] for c in cs])))
                med[q] = {k: mid([c[k] for c in cs]) for k in ("behind_mean_us", "prep_minus_handoff_med_us", "k_med_us", "k_p90_us", "g_to_answer_med_us", "f_answer_med_us", "f_answer_p90_us", "period_us")}
                med[q]["pct"] = mid(pct)
            if 0 in med and 1 in med:
                a, b = med[0], med[1]
                print("      THE DIFFERENCE, behind the frame message less at the hand (the middle take of three each): the prepare behind the handoff on %.0f against %.0f percent of pairs; prepare less handoff %+.0f us; K %+.0f us at the median and %+.0f at the 90th; 'G' to the answer %+.0f us; THE FRAME'S OWN ANSWER %+.0f us at the median and %+.0f at the 90th; the period %+.1f us"
                      % (b["pct"], a["pct"], b["prep_minus_handoff_med_us"] - a["prep_minus_handoff_med_us"], b["k_med_us"] - a["k_med_us"], b["k_p90_us"] - a["k_p90_us"],
                         b["g_to_answer_med_us"] - a["g_to_answer_med_us"], b["f_answer_med_us"] - a["f_answer_med_us"], b["f_answer_p90_us"] - a["f_answer_p90_us"], b["period_us"] - a["period_us"]))
        con = re.search(r"frame generation: this connection: ([^\n]*)", s["log"])
        print("  [%s] the helper's own count: %s" % (sid, con.group(1).strip() if con else "NO LINE FOR THE CONNECTION"))
        proof(con is not None and "DUPLICATE 0, DRIFT 0" in con.group(1), sid, "the helper counted no DUPLICATE and no DRIFT in either order")
    print("\n%d proofs passed, %d failed" % (len(passes), len(fails)))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


def main():
    if not READ:
        if not KEEP: shutil.rmtree(OUT, ignore_errors=True)
        os.makedirs(OUT, exist_ok=True)
    sys.stdout = Tee(os.path.join(OUT, "result.txt"))
    print("the cost bench, %s%s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), ", QUICK" if QUICK else ""))
    if not READ:
        if ours(): end_ours("left by an earlier run")
        for sub in ("full", "bare", "own", "beside"):
            if os.path.isdir(os.path.join(OUT, sub)): shutil.rmtree(os.path.join(OUT, sub))
        d = prepare()
        if d is None:
            print("FAIL"); return 1
    # THE LIBRARY (--lib): staged once, and every session with frame generation is started on it and held to it
    st = stage_lib(LIB) if os.path.isdir(os.path.join(OUT, "full")) else None
    if st is None:
        proof(False, "lib", "the library could not be staged (--lib=%s)" % LIB); print("FAIL"); return 1
    if ORDER: return order_main(d if not READ else None, st)   # lever 2: where 'Q' is written, and nothing else of the bench
    if not READ:
        for sid, title, hargs, fargs in plan():
            fg = "--fg=1" in hargs
            session(st["dir"] if fg else d, sid, title, hargs + (st["args"] if fg else []), fargs)
    S = {}
    for sid, title, hargs, fargs in plan():
        s = read_session(sid)
        if s is None:
            proof(False, sid, "this session left nothing: it was not run"); continue
        s["title"] = title
        check_session(s, sid not in NO_MARKS)
        if "--fg=1" in hargs: check_identity(s, st)
        if s["others"]: print("  [%s] ANOTHER GPU USER RAN BESIDE THIS SESSION (it waited 30 minutes first): %s" % (sid, s["others"]))
        if s["beside"]: print("  [%s] ANOTHER GPU USER STARTED UNDER THIS SESSION: ITS TIMES SHARED THE CARD: %s" % (sid, s["beside"]))
        if sid == "LDM": s["hud"] = 1; s["provider"] = "?"
        if sid != "X" and any(c.get("part") == "idle" for c in s["cells"]): s["idle"] = idle_table(s)
        S[sid] = s
    print("\n" + "=" * 118)
    prov = sorted({s.get("provider", "?") for s in S.values() if s["sid"] not in NO_MARKS})
    print("THE COST OF ONE GENERATED FRAME, BY ITS PARTS. 3840x2160 FP16 (scRGB), AMD's provider \"%s\", the development helper, the fake client, no game, no window." % ", ".join(prov))
    print("THE LIBRARY (--lib=%s): %s: %s, %d bytes, sha256 %s" % (LIB, st["what"], st["path"], st["size"], st["sha"]))
    if QUICK: print("QUICK RUN: THE INSTRUMENT ONLY, NOT NUMBERS TO REPORT.")
    shared = [sid for sid, s in S.items() if s["machine"]["compilers"] != "none" or s["machine"]["after"] != "none" or s["machine"]["cpu"] > 15.0 or s["machine"]["during"] > 25.0]
    print("THE MACHINE: %s" % ("SHARED during %s (a compiler ran at the session's start or end, or the CPU was more than 15%% busy before it or 25%% over it): the CPU's clocks may read high; GPU timestamps should not move" % ", ".join(shared)
                               if shared else "no compiler was seen at any session's start or end, and the CPU was under 15% busy before each and under 25% over each"))
    for n in standins: print("  a process named GTAIV.exe that is not the game ran beside the bench (a test's stand-in; it stopped nothing): %s" % n)

    # ---- THE WIRE'S ONE REVISION, risk 1: a camera cut said on 'G' alone against one said on 'Q' and 'G' both
    c1, c2, cg = S.get("GC1"), S.get("GC2"), S.get("GCG")
    if c1 and cg:
        said = lambda s: (s["ses"].get("cutg", "?"), (re.search(r"frame generation: this connection: [^\n]*resets by reason: cut (\d+)", s["log"]) or [None, "?"])[1])
        proof(said(c1)[0] == "0" and said(cg)[0] == "1" and said(c1)[1] == said(cg)[1] and said(c1)[1] not in ("?", "0"), "GCG",
              "the fake client said the cut on 'Q' and 'G' in GC1 (cutg=%s) and on 'G' ALONE in GCG (cutg=%s), and the helper counted the same cuts either way: %s and %s" % (said(c1)[0], said(cg)[0], said(c1)[1], said(cg)[1]))
        hg = compare_hash(c1["hash"], cg["hash"]); ctl = compare_hash(c1["hash"], c2["hash"]) if c2 else {}
        print("\n  A CAMERA CUT SAID ON 'G' ALONE AGAINST ONE SAID ON 'Q' AND 'G' (the whole generated image by its hash, every pair of the hash part): %s" % (hash_words(hg) if hg else "NO PAIRS"))
        if ctl: print("      and the same arms twice with the cut on both (AMD's library on the same inputs): %s" % hash_words(ctl))
        proof(sum(v[0] for v in hg.values()) >= 20 and len(hg) >= 3, "GCG", "the hash part ran in both sessions on the arms that reset: %d pairs of %d arm(s) compared by the whole image" % (sum(v[0] for v in hg.values()), len(hg)))
        for arm, v in hg.items():
            c = ctl.get(arm)
            if c is None: print("      arm %s: NOT JUDGED (no second session with the cut on both to read it against)" % arm); continue
            if c[1] == c[0]:
                proof(v[1] == v[0], "GCG", "arm %s: AMD's library gave the same image twice with the cut on 'Q' and 'G' (%d of %d pairs), and with the cut on 'G' ALONE it gives THAT SAME IMAGE (%d of %d pairs): its prepare does not need the reason" % (arm, c[1], c[0], v[1], v[0]))
            else:
                print("      arm %s: AMD's library is NOT repeatable on these inputs (%d of %d pairs the same twice): the cut on 'G' alone (%d of %d the same) can only be read against that" % (arm, c[1], c[0], v[1], v[0]))

    # ---- LEVER 1's CONTROL: a three-image set whose HUD-less copy is left out while the helper still binds HUDLESS
    ids = S.get("IDS")
    if ids:
        f = (ids.get("idle") or {}).get("obs", {}).get("F")
        print("\n  THE TRAP'S CONTROL (IDS: a three-image set, ONE copy, HUDLESS bound and never written): the steady pair's picture: %s" % (obs_words(f) if f else "NO PAIRS OBSERVED"))
        proof(ids["ses"].get("hud") == "2" and bool(f) and f[1] > 0.05, "IDS",
              "a set that HAS the HUD-less image, with the copy into it left out, does NOT give the half-way picture (%s): the game may drop the copy only for a set asked WITHOUT the image, which is what it does" % (obs_words(f) if f else "no pairs"))

    # ---- X: frame generation off
    x = S.get("X")
    if x:
        proof("frame generation cost" not in x["log"] and "frame generation" not in x["log"], "X",
              "FRAME GENERATION OFF (no --fg=1, the marks' switch --gpu_probe=1 given): the helper's log holds no line of the cost marks and none of frame generation at all")

    # ---- the idle tables
    for g, gname in (("D", "the guides at 3840x2160 (DLAA)"), ("Q", "the guides at 2560x1440 (DLSS Quality)")):
        own = [S.get("I%s1" % g), S.get("I%s2" % g)]; emp = S.get("I%sE" % g)
        say_idle(gname + ", HUDLESS own", own)
        say_idle(gname + ", HUDLESS EMPTY", [emp])
        a, b = own
        if a and b and a.get("idle") and b.get("idle"):
            for key in ("prep", "disp", "disp_reset", "disp_first", "c1"):
                x1, x2 = med(a["idle"]["takes"][key]), med(b["idle"]["takes"][key])
                if x1 is None or x2 is None: continue
                proof(abs(x1 - x2) <= max(20.0, 0.02 * x1), "I%s" % g, "THE SAME ARM TWICE (two sessions): %s reads %.0f and %.0f us: within 2 percent or 20 us" % (key, x1, x2))
        for s in own + [emp]:
            if not s or not s.get("idle"): continue
            t = s["idle"]
            he, ce = med(t["takes"]["h_empty"]), med(t["takes"]["c_empty"])
            proof(he is not None and ce is not None and he <= 2.0 and ce <= 2.0, s["sid"], "AN EMPTY BRACKET READS ABOUT ZERO: the helper's %s us, this side's %s us (2 us allowed)" % (fmt(he, 2), fmt(ce, 2)))
            proof(t["least"] is not None and t["least"] >= STEADY, s["sid"], "EVERY STEADY CELL IS STEADY: the earliest dispatch taken was the %sth after a reset (%d or more)" % (fmt(t["least"]), STEADY))
            k = t["k"].get("end_to_seen")
            if k: proof(-20.0 <= k[0] <= 250.0, s["sid"], "THE STAMPS ACCOUNT FOR THE WALL TIME: from the helper's execute to this side seeing the answer, the gap and the dispatch's stamped span leave %.0f us"
                                                               " (the queue's signal and this side's wake; a stamp that did not wait for the work would leave the work here)" % k[0])
            o = t["obs"]
            f, fr, f1, fn = o.get("F"), o.get("FR"), o.get("F1"), o.get("FN")
            proof(bool(f) and f[1] <= 0.05 and f[2] >= 0.3, s["sid"], "THE STEADY PAIR'S PICTURE: " + obs_words(f) + ": it sits on the half-way picture")
            proof(bool(fr) and fr[2] <= 0.05 and fr[1] >= 0.3, s["sid"], "A RESET PAIR'S PICTURE: " + obs_words(fr) + ": a copy of this frame's, not an interpolation")
            if f1: proof(f1[1] <= 0.10, s["sid"], "THE FIRST PAIR AFTER A RESET: " + obs_words(f1) + ": an interpolation")
            if fn: proof(bool(f) and fn[1] - f[1] >= 0.2, s["sid"], "NEGATED VECTORS (the correctness figure's own control): " + obs_words(fn) + ": NOT the half-way picture, as it must not be")
        # ---- the pictures: AMD's library twice on the same inputs FIRST, then HUDLESS own against empty, judged against that
        if a and b:
            ha = compare_hash(a["hash"], b["hash"])
            if ha:
                rep = all(v[1] == v[0] for v in ha.values())
                print("\n  IS AMD'S LIBRARY REPEATABLE (two sessions, the same inputs, HUDLESS own, %s; the WHOLE generated image by its hash, every pair of the hash part): %s" % (gname, "YES" if rep else "NOT EVERYWHERE"))
                print("      " + hash_words(ha))
                proof(sum(v[0] for v in ha.values()) >= 20, "I%s" % g, "the hash part ran in both sessions: %d pairs compared by the whole image" % sum(v[0] for v in ha.values()))
            ga, gb = read_gen(a["sid"]), read_gen(b["sid"])
            if ga and gb:
                n, eq, df, tot, worst = compare_gen(ga, gb)
                print("      and by the raw tiles (four 64x64 tiles a pair, two on the picture's edges, the idle arms): %d of %d pairs equal bit for bit, %d of %d values differ, the largest by %.6f" % (eq, n, df, tot, worst))
        if a and emp and a.get("idle") and emp.get("idle"):
            do, de = med(a["idle"]["takes"]["disp"]), med(emp["idle"]["takes"]["disp"])
            c2 = med(a["idle"]["takes"]["c2buf"])
            if None not in (do, de, c2):
                print("\n  HUDLESS OWN AGAINST EMPTY, idle, %s: the steady dispatch %.0f us with it and %.0f without (%.0f saved inside AMD's dispatch), and the second copy's buffer %.0f us:"
                      " %.0f us of GPU time a pair in all" % (gname, do, de, do - de, c2, do - de + c2))
            he = compare_hash(a["hash"], emp["hash"])
            ctl = compare_hash(a["hash"], b["hash"]) if b else {}
            if he:
                print("  HUDLESS OWN AGAINST EMPTY, THE PICTURES, %s (the whole generated image by its hash): %s" % (gname, hash_words(he)))
                for arm, v in he.items():
                    c = ctl.get(arm)
                    if c is None: continue
                    if c[1] == c[0]:
                        proof(v[1] == v[0], "I%s" % g, "arm %s: AMD's library gave the same image twice on the same inputs (%d of %d pairs), and with HUDLESS empty it gives THAT SAME IMAGE (%d of %d pairs)" % (arm, c[1], c[0], v[1], v[0]))
                    else:
                        print("      arm %s: AMD's library is NOT repeatable on these inputs (%d of %d pairs the same twice): own against empty (%d of %d the same) can only be read against that, never against zero" % (arm, c[1], c[0], v[1], v[0]))
            ga, ge = read_gen(a["sid"]), read_gen(emp["sid"])
            if ga and ge:
                n, eq, df, tot, worst = compare_gen(ga, ge)
                print("      and by the raw tiles: %d of %d pairs equal bit for bit, %d of %d values differ, the largest by %.6f" % (eq, n, df, tot, worst))

    # ---- the pad: a line through zero
    pad = [(k, S.get("P%d" % k)) for k in (1, 2, 4, 8)]
    pts = [(k, med(s["idle"]["takes"]["pad"]), med(s["idle"]["takes"]["disp"])) for k, s in pad if s and s.get("idle") and s["idle"]["takes"]["pad"]]
    if len(pts) >= 3:
        n = len(pts); sx = sum(p[0] for p in pts); sy = sum(p[1] for p in pts); sxx = sum(p[0] ** 2 for p in pts); sxy = sum(p[0] * p[1] for p in pts)
        slope = (n * sxy - sx * sy) / (n * sxx - sx * sx); icpt = (sy - slope * sx) / n
        worst = max(abs(p[1] - (slope * p[0] + icpt)) for p in pts)
        print("\nTHE PAD (the marks' known-size lever, DLAA guides, the steady pair): %s. The line: %.1f us a copy, %.1f us at no copy, the farthest point %.1f us off it" % (
            ", ".join("%d cop%s %.0f us (the dispatch beside it %.0f)" % (p[0], "y" if p[0] == 1 else "ies", p[1], p[2]) for p in pts), slope, icpt, worst))
        for k, s in pad:
            if s: print("  session %s: %s; rounds %s" % (s["sid"], load_words(s["machine"]), spread(s["idle"]["takes"]["pad"]) if s.get("idle") else "none"))
        proof(slope > 10.0 and abs(icpt) <= max(15.0, 0.25 * slope) and worst <= max(15.0, 0.25 * slope), "pad",
              "THE PAD'S POINTS LIE ON A LINE THROUGH ZERO: %.1f us a copy, %.1f us at zero copies (a quarter of a copy allowed), no point more than %.1f us off the line" % (slope, icpt, worst))
        d1 = S.get("ID1")
        if d1 and d1.get("idle") and d1["idle"]["takes"]["c1"]:
            print("  the same bytes on the other side: the game's copy into PRESENT reads %.0f us by Vulkan's stamps (the pad copies them with D3D12, out of a shared image)" % med(d1["idle"]["takes"]["c1"]))
    b0, d1 = S.get("B0"), S.get("ID1")
    if b0 and d1 and b0.get("idle") and d1.get("idle"):
        print("\nTHE BARRIER'S CONTROL (B0: no barrier before the closing stamps; %s):" % load_words(b0["machine"]))
        for key, what in (("prep_reset", "a reset prepare"), ("disp", "a steady dispatch"), ("disp_reset", "a reset dispatch")):
            x0, x1 = med(b0["idle"]["takes"][key]), med(d1["idle"]["takes"][key])
            if x0 is None or x1 is None: continue
            print("  %-18s without the barrier %s us, with it %.0f us (session ID1): %s" % (what, spread(b0["idle"]["takes"][key]), x1,
                  "the same: here the stamp waits for AMD's work without it" if abs(x0 - x1) <= max(20.0, 0.02 * x1) else ("WITHOUT THE BARRIER THE STAMP DOES NOT WAIT for the work" if x0 < x1 else "the barrier is not what makes it longer")))

    # ---- does a bracket of the marks hold a stranger's work (2026-10-10)
    said = lambda s, t: t in s["log"]
    same = lambda a, b: a is not None and b is not None and abs(a - b) <= max(20.0, 0.02 * b)
    low = lambda s, key: med(s["idle"]["takes"][key]) if s and s.get("idle") and s["idle"]["takes"][key] else None      # a round's value: every move's lower quartile
    mid = lambda s, key: med(s["idle"]["meds"][key]) if s and s.get("idle") and s["idle"]["meds"][key] else None        # and by the medians: work that landed in a span shows here
    cell = lambda s, key: "none" if low(s, key) is None else "%s, by the medians %s, 99th of every sample %s" % (spread(s["idle"]["takes"][key]), fmt(mid(s, key)), fmt(s["idle"]["p99"][key]))
    h1, s0, s1 = S.get("H1"), S.get("S0"), S.get("S1")
    if h1 and s0 and s1 and all(x.get("idle") for x in (h1, s0, s1)):
        per = slope if len(pts) >= 3 else None
        size = STRANGER * per if per else None
        print("\nDOES A BRACKET OF THE MARKS HOLD A STRANGER'S WORK, 1: A STRANGER OF COPIES (DLAA guides). %d full-screen copies in a list of the marks' own, executed on the helper's queue right before each prepare's list;"
              " nothing in that list makes them complete." % STRANGER)
        print("  the stranger's size: %s" % (("%d copies at %.1f us a copy by the pad's line: about %.0f us" % (STRANGER, per, size)) if per else "NOT SIZED (the pad's sessions P1 to P8 were not run with these)"))
        for s in (h1, s0, s1): print("  session %s: %s; its helper said: %s" % (s["sid"], load_words(s["machine"]), "; ".join(l.split("frame generation cost: ", 1)[1].strip() for l in s["log"].splitlines() if "cost: head barrier:" in l or "cost: stranger:" in l) or "nothing of a head barrier or a stranger"))
        for key, what in (("prep", "a steady pair's prepare"), ("prep_reset", "a reset prepare"), ("disp", "a steady dispatch"), ("disp_reset", "a reset dispatch")):
            print("  %-24s no stranger (session ID1): %s us; no stranger, head barrier: %s; BEHIND THE STRANGER: %s; behind the stranger, head barrier: %s" % (what, cell(d1, key), cell(h1, key), cell(s0, key), cell(s1, key)))
        proof(said(h1, "cost: head barrier: yes") and not said(h1, "cost: stranger:") and not said(s0, "cost: head barrier:") and said(s0, "cost: stranger: %d copies" % STRANGER)
              and said(s1, "cost: head barrier: yes") and said(s1, "cost: stranger: %d copies" % STRANGER), "head", "each of the three sessions' helpers said the head barrier and the stranger it was started with, and nothing of one it was not")
        for key in ("prep", "prep_reset"):
            base, b, c, hb = low(d1, key), low(s0, key), low(s1, key), low(h1, key)
            if None in (base, b, c, hb): proof(False, "head", "%s: a session gave no value (session ID1 is the one with no stranger)" % key); continue
            proof(same(b, base) and same(mid(s0, key), mid(d1, key)), "head", "THE BRACKET DOES NOT HOLD THE STRANGER'S COPIES (%s): %.0f us behind the stranger against %.0f with none (by the medians %.0f against %.0f)%s: within 20 us or 2 percent" % (
                key, b, base, mid(s0, key), mid(d1, key), (", of a stranger of about %.0f us" % size) if size else ""))
            proof(same(c, b) and same(hb, base), "head", "AND A BARRIER BEFORE THE HEAD STAMP CHANGES NOTHING (%s): behind the stranger %.0f us with it and %.0f without; with no stranger %.0f with it and %.0f without" % (key, c, b, hb, base))
        for key in ("disp", "disp_reset"):
            base, b, c, hb = low(d1, key), low(s0, key), low(s1, key), low(h1, key)
            if None in (base, b, c, hb): continue
            proof(same(b, base) and same(c, base) and same(hb, base), "head", "the dispatch, which no stranger stands before: %s reads %.0f us in session ID1 and %.0f, %.0f and %.0f in these three" % (key, base, hb, b, c))
    uw, u0, u1 = S.get("UW"), S.get("U0"), S.get("U1")
    if uw and u0 and u1 and all(x.get("idle") for x in (uw, u0, u1)):
        ev = {}
        for s in (uw, u0, u1):
            m = re.findall(r"serve timing, \d+ frames:.*?GPU (\d+) us mean, worst (\d+)", s["log"])
            ev[s["sid"]] = (sum(float(x[0]) for x in m) / len(m), max(float(x[1]) for x in m), len(m)) if m else None
        print("\nDOES A BRACKET OF THE MARKS HOLD A STRANGER'S WORK, 2: A REAL UPSCALER'S EVALUATE (Quality guides, the helper's own upscaler evaluating every frame).")
        print("  UW: every frame's answer is waited for before its pair's 'Q' is written (nothing in flight). U0, U1: 'Q' IS WRITTEN RIGHT BEHIND 'F', as the game writes it: the evaluate's list and the prepare's follow each other on the queue.")
        for s in (uw, u0, u1):
            e = ev[s["sid"]]
            print("  session %s: %s; the fake client said fq=%s; the evaluate by the helper's serve timer: %s; its helper said: %s" % (
                s["sid"], load_words(s["machine"]), s["ses"].get("fq", "?"), ("%.0f us of GPU time at the mean of %d lines, %.0f at the worst" % (e[0], e[2], e[1])) if e else "NOT READ",
                "; ".join(l.split("frame generation cost: ", 1)[1].strip() for l in s["log"].splitlines() if "cost: head barrier:" in l) or "nothing of a head barrier"))
        for key, what in (("prep", "a steady pair's prepare"), ("prep_reset", "a reset prepare"), ("gap_prep", "execute to its first stamp"), ("disp", "a steady dispatch"), ("disp_reset", "a reset dispatch")):
            print("  %-28s waited: %s us; 'Q' RIGHT BEHIND 'F': %s; the same, head barrier: %s" % (what, cell(uw, key), cell(u0, key), cell(u1, key)))
        proof(uw["ses"].get("fq") == "0" and u0["ses"].get("fq") == "1" and u1["ses"].get("fq") == "1" and not said(uw, "cost: head barrier:") and not said(u0, "cost: head barrier:") and said(u1, "cost: head barrier: yes")
              and all(ev[k] is not None and ev[k][0] >= 300.0 for k in ev), "head",
              "each session ran what it was started for: the fake client's order, the helper's head barrier, and A REAL EVALUATE in every frame's list (%s us of GPU time a frame by the serve timer, 300 or more asked: the transport's copy reads about 30)" % ", ".join("%.0f" % ev[k][0] for k in ("UW", "U0", "U1") if ev[k]))
        gw, g0, g1 = low(uw, "gap_prep"), low(u0, "gap_prep"), low(u1, "gap_prep")
        proof(None not in (gw, g0, g1) and g0 - gw >= 300.0 and g1 - gw >= 300.0, "head",
              "THE EVALUATE WAS IN FLIGHT IN FRONT OF THE PREPARE: from the prepare's execute to its first stamp %s us with 'Q' right behind 'F' (%s with the head barrier) against %s with the frame waited for: THE HEAD STAMP STANDS BEHIND THE EVALUATE, barrier or none" % (fmt(g0), fmt(g1), fmt(gw)))
        for key in ("prep", "prep_reset"):
            a, b, c = low(uw, key), low(u0, key), low(u1, key)
            if None in (a, b, c): proof(False, "head", "%s: a session gave no value" % key); continue
            proof(same(b, a) and same(mid(u0, key), mid(uw, key)), "head", "THE BRACKET DOES NOT HOLD THE EVALUATE (%s): %.0f us with the evaluate in flight in front of it against %.0f with nothing in flight (by the medians %.0f against %.0f): within 20 us or 2 percent."
                  " THE READING OF 2026-10-10 IS REFUTED AT A DESK: the game's 1,686 us are not the upscaler's evaluate held by a head stamp that does not wait" % (key, b, a, mid(u0, key), mid(uw, key)))
            proof(same(c, b), "head", "AND A BARRIER BEFORE THE HEAD STAMP CHANGES NOTHING (%s): %.0f us with it, %.0f without" % (key, c, b))
        for key in ("disp", "disp_reset"):
            a, b, c = low(uw, key), low(u0, key), low(u1, key)
            if None in (a, b, c): continue
            proof(same(b, a) and same(c, a), "head", "the dispatch (the prepare's own list stands before it, complete): %s reads %.0f, %.0f and %.0f us in the three" % (key, a, b, c))

    # ---- the loaded tables
    for g, gname in (("D", "the guides at 3840x2160 (DLAA)"), ("Q", "the guides at 2560x1440 (DLSS Quality)")):
        own, emp = S.get("L%s1" % g), S.get("L%sE" % g)
        say_loaded(gname + ", HUDLESS own", own)
        say_loaded(gname + ", HUDLESS EMPTY", emp)
        for s in (own, emp):
            if s: check_loaded(s)
        if own and emp and "lstats" in own and "lstats" in emp:
            for part in sorted(set(own["lstats"]) & set(emp["lstats"]), key=lambda p: int(p[4:])):
                fo, fe = own["lstats"][part].get("F"), emp["lstats"][part].get("F")
                if fo and fe and fo["mean"] and fe["mean"]:
                    print("  HUDLESS OWN AGAINST EMPTY, %s, %s: the steady pair adds %.0f us to the frame with it and %.0f without: %.0f us saved a pair (by the lower quartiles %.0f and %.0f: %.0f). Two sessions; each one's null is above" % (
                        gname, part, med(fo["mean"]), med(fe["mean"]), med(fo["mean"]) - med(fe["mean"]), med(fo["low"]), med(fe["low"]), med(fo["low"]) - med(fe["low"])))
    ldp, ld1, p8 = S.get("LDP"), S.get("LD1"), S.get("P8")
    if ldp and ldp.get("blocks"):
        say_loaded("the guides at 3840x2160 (DLAA), HUDLESS own, THE MARKS' PAD OF 8 COPIES in each dispatch's list", ldp)
        check_loaded(ldp)
        for part, st in sorted(ldp.get("lstats", {}).items(), key=lambda kv: int(kv[0][4:])):
            a, b = st.get("F"), (ld1 or {}).get("lstats", {}).get(part, {}).get("F")
            if not a or not b or not a["mean"] or not b["mean"] or a["hpad"] is None: continue
            more, morel = med(a["mean"]) - med(b["mean"]), med(a["low"]) - med(b["low"])
            idle8 = med(p8["idle"]["takes"]["pad"]) if p8 and p8.get("idle") and p8["idle"]["takes"]["pad"] else None
            print("  WHAT A HELPER'S GPU TIME COSTS THE FRAME, %s: with 8 copies more in each dispatch's list the steady pair adds %.0f us to the frame, without them %.0f (session LD1): %.0f us more (%.0f by the lower quartiles)."
                  " The 8 copies by the marks' own stamps: %.0f us under load%s. So the frame pays %.2f times the copies' time as stamped under load%s" % (
                      part, med(a["mean"]), med(b["mean"]), more, morel, a["hpad"], (", %.0f us with the card to themselves (session P8)" % idle8) if idle8 else "", more / a["hpad"],
                      (", %.2f times their time alone" % (more / idle8)) if idle8 else ""))

    ldm = S.get("LDM")
    if ldm and ldm.get("blocks"):
        say_loaded("the guides at 3840x2160 (DLAA), HUDLESS own, THE MARKS OFF", ldm)
        check_loaded(ldm)
        proof("frame generation cost" not in ldm["log"], "LDM", "the helper was started without the marks' switch and its log holds no line of theirs")
        for part, st in sorted(ldm.get("lstats", {}).items(), key=lambda kv: int(kv[0][4:])):
            a, b = st.get("F"), (ld1 or {}).get("lstats", {}).get(part, {}).get("F")
            if not a or not b or not a["mean"] or not b["mean"]: continue
            print("  WHAT THE MARKS THEMSELVES COST THE FRAME, %s: the steady pair adds %.0f us with the marks off and %.0f with them on (session LD1): %.0f us for four stamps, a barrier and a resolve on each of the pair's two lists"
                  " (by the lower quartiles %.0f and %.0f: %.0f). TWO SESSIONS: this is also how far the same arm lies apart from one session to the next, and no difference of two sessions smaller than it is resolved" % (
                      part, med(a["mean"]), med(b["mean"]), med(b["mean"]) - med(a["mean"]), med(a["low"]), med(b["low"]), med(b["low"]) - med(a["low"])))

    left = end_ours("left at the end") if ours() else []
    proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    for sid, l in fails[:20]: print("  failed: [%s] %s" % (sid, l.strip()[:240]))
    print("%d proofs passed, %d failed; the folder is kept: %s" % (len(passes), len(fails), OUT))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
